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
QString const SupportRequester::DEFAULT_DOWNLOAD_URL = "http://integral-info.ru/wp-content/uploads";
//--------------------------------------------------------------------------------------------------
SupportRequester::
SupportRequester(QObject* parent)
  : QObject(parent)
  , download_url_(DEFAULT_DOWNLOAD_URL)
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
start(RequestSettings const& settings, RequestContext const& ctx)
{
  if(!tmpdir_.isValid())
    return false;
  request_settings_ = settings;
  ctx_ = ctx;
  emit log("Подготовка инструментария...");
  downloader_->download(QUrl(download_url_ + "/scan.zip"));
  return true;
}
//--------------------------------------------------------------------------------------------------
void
SupportRequester::
fileDownloaded(QByteArray const& data)
{
  emit log("Распаковка компонентов...");
  QBuffer buffer(const_cast<QByteArray*>(&data));
  QStringList files = JlCompress::extractDir(&buffer, tmpdir_.path());
  if(files.isEmpty())
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
      report_exists = false;
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
bool
SupportRequester::
checkSettings(RequestSettings const& settings, QString& error_text)
{
  SmtpClient smtp(settings.smtp_server, settings.smtp_port, settings.conn_type);
  smtp.setAuthMethod(SmtpClient::AuthLogin);
  smtp.setUser(settings.smtp_user);
  smtp.setPassword(settings.smtp_password);

  if(!smtp.connectToHost())
  {
    error_text = "Не удалось подключиться к почтовому серверу.";
    return false;
  }

  if(!smtp.login())
  {
    error_text = "Не удалось авторизоваться на почтовом сервере.";
    return false;
  }

  return true;
}
//--------------------------------------------------------------------------------------------------
