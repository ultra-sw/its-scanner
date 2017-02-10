#pragma once
#include <QObject>
#include <QUrl>
#include <QTemporaryDir>
#include <QProcess>
#include <QList>
#include <QPixmap>
//--------------------------------------------------------------------------------------------------
class FileDownloader;
class QProcess;
//--------------------------------------------------------------------------------------------------
struct RequestContext
{
  QString subject;
  QString body;
  QString email;
  QString phone;
  QString company;
  QList<QPixmap> screenshots;
};
//--------------------------------------------------------------------------------------------------
class SupportRequester : public QObject
{
  Q_OBJECT
public:
  explicit SupportRequester(QObject* parent = nullptr);

  bool
  start(RequestContext const& ctx);

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
  scanProcessFinished(int code, QProcess::ExitStatus status);

private:
  FileDownloader* downloader_ = nullptr;
  QTemporaryDir tmpdir_;
  QProcess* scan_process_;
  RequestContext ctx_;
};
//--------------------------------------------------------------------------------------------------
