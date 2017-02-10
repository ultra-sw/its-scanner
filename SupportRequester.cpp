#include "SupportRequester.h"
#include "FileDownloader.h"
#include "quazip/JlCompress.h"
#include "qsmtp/SmtpMime"
#include <QFile>
#include <QDataStream>
#include <QBuffer>
#include <QStringList>
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
  downloader_->download(QUrl("http://perchits.integral-info.ru/data/scan.zip"));
  return true;
}
//--------------------------------------------------------------------------------------------------
void
SupportRequester::
fileDownloaded(QByteArray const& data)
{
  emit log("Распаковка компонентов...");
  QBuffer buffer(const_cast<QByteArray*>(&data));
  JlCompress::extractDir(&buffer);

  emit log("Запуск сбора данных...");
  QStringList args;
  args.append("/R");
  args.append("$HOSTNAME_$DATE_FULL");
  args.append("/HTML");
  args.append("/SILENT");
  args.append("/AUDIT");
  scan_process_->start("aida64.exe", args);
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
  JlCompress::compressDir("report.zip", "Reports");

  emit log("Отправка отчета...");
  SmtpClient smtp("smtp.gmail.com", 465, SmtpClient::SslConnection);
  smtp.setUser("perchits@gmail.com");
  smtp.setPassword("borland1973");

  MimeMessage message;
  message.setSender(new EmailAddress(ctx_.email, ctx_.company));
  message.addRecipient(new EmailAddress("andperch@yandex.ru", "Андрей Перчиц"));
  message.setSubject(ctx_.subject);

  MimeText text;
  text.setText(QString("%1\r\n\r\n%2\r\nТел: %3").arg(ctx_.body).arg(ctx_.company).arg(ctx_.phone));
  message.addPart(&text);

//  MimeAttachment attachment (new QFile("report.zip"));
//  attachment.setContentType("application/octet-stream");
  message.addPart(new MimeAttachment(new QFile("report.zip")));


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
