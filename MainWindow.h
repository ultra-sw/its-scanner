#pragma once
#include "SupportRequester.h"
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
  on_next_btn_clicked();

  void
  on_prev_btn_clicked();

  void
  on_take_screenshot_btn_clicked();

  void
  on_make_request_btn_clicked();

  void
  appendLog(QString const& message);

  void
  processFinished();

  void
  processFailed(QString const& message);

private:
  void
  setFgColor(QWidget* widget, QColor color);

  bool
  checkContactsPage();

  bool
  checkInfoPage();

  Ui::MainWindow* ui_;
  QList<QPixmap> screenshots_;
  QList<QLabel> screenshot_perviews_;
  FlowLayout* screenshots_layout_;

  SupportRequester support_requester_;
};
//--------------------------------------------------------------------------------------------------
