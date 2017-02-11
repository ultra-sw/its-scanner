#include "SupportRequester.h"
#include "FileDownloader.h"
#include "quazip/JlCompress.h"
#include "qsmtp/SmtpMime"
#include <QFile>
#include <QDataStream>
#include <QBuffer>
#include <QStringList>
#include <QSettings>
//--------------------------------------------------------------------------------------------------
SupportRequester::
SupportRequester(QObject* parent)
  : QObject(parent)
{
  downloader_ = new FileDownloader(this);
  connect(downloader_, &FileDownloader::downloaded, this, &SupportRequester::fileDownloaded);
  scan_process_ = new QProcess(this);
  connect(scan_process_, SIGNAL(finished(int, QProcess::ExitStatus)),
          this, SLOT(scanProcessFinished(int, QProcess::ExitStatus)));
}
//--------------------------------------------------------------------------------------------------
bool
SupportRequester::
start(RequestContext const& ctx)
{
  if(!tmpdir_.isValid())
    return false;
  ctx_ = ctx;
  emit log("Скачивание компонентов...");
  QSettings settings("TechSupport.ini", QSettings::IniFormat);
  downloader_->download(QUrl(settings.value("download_url").toString()));
  return true;
}
//--------------------------------------------------------------------------------------------------
void
SupportRequester::
fileDownloaded(QByteArray const& data)
{
  emit log("Распаковка компонентов...");
  QBuffer buffer(const_cast<QByteArray*>(&data));
  JlCompress::extractDir(&buffer, tmpdir_.path());

  emit log("Запуск сбора данных...");
  QString report_dir = tmpdir_.path() + QDir::separator() + "Reports";
  QStringList args;
  args.append("/R");
  args.append(report_dir + QDir::separator() + "$HOSTNAME_$DATE_FULL");
  args.append("/HTML");
  args.append("/SILENT");
  args.append("/AUDIT");
  scan_process_->start(tmpdir_.path() + QDir::separator() + "aida64.exe", args);
}
//--------------------------------------------------------------------------------------------------
void
SupportRequester::
scanProcessFinished(int code, QProcess::ExitStatus status)
{
  if(status == QProcess::CrashExit || code != 0)
  {
    emit failed("Сбор данных был завершен неудачно.");
    return;
  }

  emit log("Подготовка отчета...");

  QString report_file = tmpdir_.path() + QDir::separator() + "report.zip";
  QString report_dir = tmpdir_.path() + QDir::separator() + "Reports";
  if(!JlCompress::compressDir(report_file, report_dir))
  {
    emit failed("Подготовка отчета не удалась.");
    return;
  }

  emit log("Отправка отчета...");
  QSettings settings("TechSupport.ini", QSettings::IniFormat);
  QString smtp_auth = settings.value("smtp_auth").toString();
  SmtpClient::ConnectionType conn_type = SmtpClient::TcpConnection;
  if(smtp_auth == "ssl")
    conn_type = SmtpClient::SslConnection;
  else if(smtp_auth == "tls")
    conn_type = SmtpClient::TlsConnection;
  SmtpClient smtp(settings.value("smtp_server").toString(), settings.value("smtp_port").toInt(),
                  conn_type);
  smtp.setUser(settings.value("smtp_user").toString());
  smtp.setPassword(settings.value("smtp_password").toString());

  MimeMessage message;
  message.setSender(new EmailAddress(ctx_.email, ctx_.name));
  message.addRecipient(new EmailAddress(settings.value("support_email").toString(), "Техподдержка"));
  message.setSubject(ctx_.subject);

  MimeText text;
  QString header;
  if(ctx_.incident_time.isValid())
    header = QString("Время возникновения проблемы: %1\r\n").arg(ctx_.incident_time.toString());
  if(ctx_.place != RequestContext::UNKNOWN_PLACE)
  {
    header += "Проблема возникла на этом компьютере? ";
    if(ctx_.place == RequestContext::THIS_COMPUTER)
      header += "Да.";
    else if(ctx_.place == RequestContext::THIS_COMPUTER)
      header += "Нет.";
  }
  if(ctx_.incident_time.isValid() || ctx_.place != RequestContext::UNKNOWN_PLACE)
    header += "\r\n-------------------------------------------------------\r\n\r\n";

  text.setText(QString("%1%2\r\n-------------------------------------------------------\r\n\r\n"
    "%3\r\nТел: %4").arg(header).arg(ctx_.body).arg(ctx_.company).arg(ctx_.phone));
  message.addPart(&text);

  message.addPart(new MimeAttachment(new QFile(report_file)));

  int number = 0;
  for(QPixmap const& screenshot : ctx_.screenshots)
  {
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    screenshot.save(&buffer, "PNG");
    MimeAttachment* attachment =
        new MimeAttachment(buffer.buffer(), QString("Screenshot%1.png").arg(++number));
    attachment->setContentType("image/jpeg");
    message.addPart(attachment);
  }

  if(!smtp.connectToHost())
  {
    emit failed("Не удалось подключиться к почтовому серверу.");
    return;
  }

  if(!smtp.login())
  {
    emit failed("Не удалось авторизоваться на почтовом сервере.");
    return;
  }

  if(!smtp.sendMail(message))
  {
    emit failed("Не удалось отправить почтовое сообщение.");
    return;
  }

  smtp.quit();

  emit finished();
}
//--------------------------------------------------------------------------------------------------
