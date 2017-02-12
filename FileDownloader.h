#pragma once
#include <QObject>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
//--------------------------------------------------------------------------------------------------
class FileDownloader : public QObject
{
 Q_OBJECT
 public:
  explicit FileDownloader(QObject* parent = nullptr);

  virtual ~FileDownloader();

  void
  download(QUrl const& url);

  QByteArray
  downloadedData() const;

 signals:
  void
  downloaded(QByteArray const& data);

  void
  failed();

 private slots:
  void
  fileDownloaded(QNetworkReply* pReply);

 private:
  QNetworkAccessManager web_ctrl_;
  QByteArray downloaded_data_;
};
//--------------------------------------------------------------------------------------------------
