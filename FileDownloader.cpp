#include "FileDownloader.h"
//--------------------------------------------------------------------------------------------------
FileDownloader::
FileDownloader(QObject* parent)
  : QObject(parent)
{
  connect(&web_ctrl_, SIGNAL(finished(QNetworkReply*)),
          this, SLOT(fileDownloaded(QNetworkReply*)));
}
//--------------------------------------------------------------------------------------------------
FileDownloader::
~FileDownloader()
{
}
//--------------------------------------------------------------------------------------------------
void
FileDownloader::
download(QUrl const& url)
{
  QNetworkRequest request(url);
  web_ctrl_.get(request);
}
//--------------------------------------------------------------------------------------------------
void
FileDownloader::
fileDownloaded(QNetworkReply* reply)
{
  if(reply->error() == QNetworkReply::NoError)
  {
    downloaded_data_ = reply->readAll();
    reply->deleteLater();
    emit downloaded(downloaded_data_);
  }
  else
    emit failed();
}
//--------------------------------------------------------------------------------------------------
QByteArray
FileDownloader::
downloadedData() const
{
  return downloaded_data_;
}
//--------------------------------------------------------------------------------------------------
