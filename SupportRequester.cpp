#include "SupportRequester.h"
#include "FileDownloader.h"
#include "quazip/JlCompress.h"
#include "qsmtp/SmtpMime"
#include <QFile>
#include <QDataStream>
#include <QBuffer>
#include <QStringList>
#include <QSettings>
#include <QFileInfo>
//--------------------------------------------------------------------------------------------------
QString const SupportRequester::DOWNLOAD_URL = "http://integral-info.ru/wp-content/uploads/scan.zip";
//--------------------------------------------------------------------------------------------------
SupportRequester::
SupportRequester(QObject* parent)
  : QObject(parent)
{
  downloader_ = new FileDownloader(this);
  connect(downloader_, &FileDownloader::downloaded, this, &SupportRequester::fileDownloaded);
  connect(downloader_, &FileDownloader::failed, this, &SupportRequester::fileDownloadFailed);
  scan_process_ = new QProcess(this);
  connect(scan_process_, &QProcess::errorOccurred, this, &SupportRequester::scanProcessError);
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
  emit log("Подготовка инструментария...");
  downloader_->download(QUrl(DOWNLOAD_URL));
  return true;
}
//--------------------------------------------------------------------------------------------------
bool
SupportRequester::
readSettings(QString const& inifile)
{
  if(!QFileInfo(inifile).exists())
    return false;

  QSettings settings(inifile, QSettings::IniFormat);
  QString smtp_auth = settings.value("smtp_auth").toString();
  if(smtp_auth == "ssl")
    request_settings_.conn_type = SmtpClient::SslConnection;
  else if(smtp_auth == "tls")
    request_settings_.conn_type = SmtpClient::TlsConnection;
  else
    request_settings_.conn_type = SmtpClient::TcpConnection;

  request_settings_.conn_type = SmtpClient::TlsConnection;

  request_settings_.smtp_server = settings.value("smtp_server").toString();
  request_settings_.smtp_port = settings.value("smtp_port").toInt();
  request_settings_.smtp_user = settings.value("smtp_user").toString();
  request_settings_.smtp_password = settings.value("smtp_password").toString();
  request_settings_.from_email = settings.value("from_email").toString();
  request_settings_.support_email = settings.value("support_email").toString();
  request_settings_.support_recipient = settings.value("support_recipient").toString();

  QFile(inifile).remove();
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

  QString inifile = tmpdir_.path() + QDir::separator() + "settings.ini";
  if(!readSettings(inifile))
  {
    emit failed("Распаковка компонентов завершилась неудачно.");
    return;
  }

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
fileDownloadFailed()
{
  emit failed("Подготовка инструментария завершилась неудачно.");
}
//--------------------------------------------------------------------------------------------------
void
SupportRequester::
scanProcessError(QProcess::ProcessError error)
{
  Q_UNUSED(error);
  sendReport(false);
}
//--------------------------------------------------------------------------------------------------
void
SupportRequester::
scanProcessFinished(int code, QProcess::ExitStatus status)
{
//  if(status == QProcess::CrashExit || code != 0)
//  {
//    emit failed("Сбор данных был завершен неудачно.");
//    return;
//  }
  bool report_exists = (status == QProcess::NormalExit && code == 0);

  emit log("Подготовка отчета...");

  QString report_file = tmpdir_.path() + QDir::separator() + "report.zip";
  if(report_exists)
  {
    QString report_dir = tmpdir_.path() + QDir::separator() + "Reports";
    if(!JlCompress::compressDir(report_file, report_dir))
    {
//      emit failed("Подготовка отчета не удалась.");
//      return;
      report_exists = false;
    }
  }
  sendReport(report_exists ? report_file : QString());
}
//--------------------------------------------------------------------------------------------------
void
SupportRequester::
sendReport(QString const& report_file)
{
  emit log("Отправка отчета...");
  SmtpClient smtp(request_settings_.smtp_server, request_settings_.smtp_port,
                  request_settings_.conn_type);
  smtp.setAuthMethod(SmtpClient::AuthLogin);
  smtp.setUser(request_settings_.smtp_user);
  smtp.setPassword(request_settings_.smtp_password);

  MimeMessage message;
  message.setSender(new EmailAddress(request_settings_.from_email, ctx_.name));
  message.addRecipient(new EmailAddress(request_settings_.support_email,
                                        request_settings_.support_recipient));
  message.setSubject(ctx_.company + ": " + ctx_.subject);

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
    "%3\r\n%4\r\nТел: %5").arg(header).arg(ctx_.body).arg(ctx_.name).arg(ctx_.company).arg(ctx_.phone));
  message.addPart(&text);

  if(!report_file.isEmpty() && QFileInfo(report_file).exists())
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
