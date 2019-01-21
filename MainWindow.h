#pragma once
#include "SupportRequester.h"
#include "FileDownloader.h"
#include <QMainWindow>
#include <QList>
#include <QPixmap>
#include <QLabel>
//--------------------------------------------------------------------------------------------------
namespace Ui
{
  class MainWindow;
}
class FlowLayout;
//--------------------------------------------------------------------------------------------------
class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);

  ~MainWindow();

private slots:
  void
  on_check_settings_btn_clicked();

  void
  on_next_btn_clicked();

  void
  on_prev_btn_clicked();

  void
  on_take_screenshot_btn_clicked();

  void
  on_make_request_btn_clicked();

  void
  getParamsFromServer();

  void
  appendLog(QString const& message);

  void
  processFinished();

  void
  processFailed(QString const& message);

  void
  onIncidentTimeChanged(QDateTime const& date_time);

  void
  fileDownloaded(QByteArray const& data);

  void
  fileDownloadFailed();

private:
  void
  setFgColor(QWidget* widget, QColor color);

  bool
  checkContactsPage();

  bool
  checkInfoPage();

  void
  loadUserParams();

  void
  saveUserParams();

  RequestSettings
  prepareRequest() const;

  QString
  encrypt(QString const& data);

  QString
  decrypt(QString const& data);

  static quint64 const SALT;
  Ui::MainWindow* ui_;
  QDateTime incident_time_;
  FlowLayout* screenshots_layout_;

  SupportRequester support_requester_;
  FileDownloader downloader_;
};
//--------------------------------------------------------------------------------------------------
