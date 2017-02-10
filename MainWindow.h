#pragma once
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
  explicit MainWindow(QWidget* parent = 0);

  ~MainWindow();

private slots:
  void
  on_next_btn_clicked();

  void
  on_prev_btn_clicked();

  void
  on_take_screenshot_btn_clicked();

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
};
//--------------------------------------------------------------------------------------------------
