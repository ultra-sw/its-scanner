#pragma once
#include "qsmtp/smtpclient.h"
#include <QObject>
#include <QUrl>
#include <QTemporaryDir>
#include <QProcess>
#include <QList>
#include <QPixmap>
#include <QDateTime>
//--------------------------------------------------------------------------------------------------
class FileDownloader;
class QProcess;
//--------------------------------------------------------------------------------------------------
struct RequestContext
{
  enum ProblemPlace {UNKNOWN_PLACE, THIS_COMPUTER, ANOTHER_COMPUTER};
  ProblemPlace place = UNKNOWN_PLACE;
  QDateTime incident_time;
  QString subject;
  QString body;
  QString name;
  QString email;
  QString phone;
  QString company;
  QList<QPixmap> screenshots;
};
//--------------------------------------------------------------------------------------------------
struct RequestSettings
{
  SmtpClient::ConnectionType conn_type;
  QString smtp_server;
  int smtp_port;
  QString smtp_user;
  QString smtp_password;
  QString from_email;
  QString support_email;
  QString support_recipient;
};
//--------------------------------------------------------------------------------------------------
class SupportRequester : public QObject
{
  Q_OBJECT
public:
  explicit SupportRequester(QObject* parent = nullptr);

  bool
  start(RequestSettings const& settings, RequestContext const& ctx);

  void
  setDownloadUrl(QString const& url)
  {
    download_url_ = url;
  }

  QString
  downloadUrl() const
  {
    return download_url_;
  }

  bool
  checkSettings(RequestSettings const& settings, QString& error_text);

signals:
  void
  log(QString const& message);

  void
  finished();

  void
  failed(QString const& message);

private slots:
  void
  fileDownloaded(QByteArray const& data);

  void
  fileDownloadFailed();

  void
  scanProcessFinished(int code, QProcess::ExitStatus status);

  void
  scanProcessError(QProcess::ProcessError error);

private:
  static QString const DEFAULT_DOWNLOAD_URL;

  void
  sendReport(QString const& report_file);

  FileDownloader* downloader_ = nullptr;
  QTemporaryDir tmpdir_;
  QProcess* scan_process_;
  RequestContext ctx_;
  RequestSettings request_settings_;
  QString download_url_;
};
//--------------------------------------------------------------------------------------------------
