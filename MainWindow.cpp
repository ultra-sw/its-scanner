#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "FlowLayout.h"
#include <QMessageBox>
#include <QRegExp>
#include <QClipboard>
//--------------------------------------------------------------------------------------------------
MainWindow::
MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , ui_(new Ui::MainWindow)
{
  ui_->setupUi(this);
  ui_->prev_btn->setEnabled(false);
  ui_->stacked_widget->setCurrentWidget(ui_->contacts_page);
  QFont font = ui_->body_edit->font();
  font.setPointSize(font.pointSize()+2);
  ui_->body_edit->setFont(font);

  screenshots_layout_ = new FlowLayout(ui_->screenshots_area_contents);
  ui_->screenshots_area_contents->setLayout(screenshots_layout_);

  connect(&support_requester_, &SupportRequester::log,
          this, &MainWindow::appendLog);
  connect(&support_requester_, &SupportRequester::finished,
          this, &MainWindow::processFinished);
  connect(&support_requester_, &SupportRequester::failed,
          this, &MainWindow::processFailed);
}
//--------------------------------------------------------------------------------------------------
MainWindow::
~MainWindow()
{
  delete ui_;
}
//--------------------------------------------------------------------------------------------------
void
MainWindow::
setFgColor(QWidget* widget, QColor color)
{
  QPalette palette = widget->palette();
  palette.setColor(widget->foregroundRole(), color);
  widget->setPalette(palette);
}
//--------------------------------------------------------------------------------------------------
bool
MainWindow::
checkContactsPage()
{
  bool can_proceed = true;
  setFgColor(ui_->name_edit_label, Qt::black);
  setFgColor(ui_->company_edit_label, Qt::black);
  setFgColor(ui_->phone_edit_label, Qt::black);
  setFgColor(ui_->email_edit_label, Qt::black);
  if(ui_->name_edit->text().trimmed().isEmpty())
  {
    setFgColor(ui_->name_edit_label, Qt::red);
    can_proceed = false;
  }
  if(ui_->company_edit->text().trimmed().isEmpty())
  {
    setFgColor(ui_->company_edit_label, Qt::red);
    can_proceed = false;
  }
  if(ui_->phone_edit->text().trimmed().isEmpty())
  {
    setFgColor(ui_->phone_edit_label, Qt::red);
    can_proceed = false;
  }
  if(ui_->email_edit->text().trimmed().isEmpty())
  {
    setFgColor(ui_->email_edit_label, Qt::red);
    can_proceed = false;
  }
  else
  {
    QRegExp mail_rex("\\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,63}\\b");
    mail_rex.setCaseSensitivity(Qt::CaseInsensitive);
    mail_rex.setPatternSyntax(QRegExp::RegExp);
    if(!mail_rex.exactMatch(ui_->email_edit->text().trimmed()))
    {
      QMessageBox::critical(this, "Контактные сведения", "Неправильно заполнен email");
      return false;
    }
  }
  if(!can_proceed)
    QMessageBox::critical(this, "Контактные сведения", "Заполните поля, помеченные красным");
  return can_proceed;
}
//--------------------------------------------------------------------------------------------------
bool
MainWindow::
checkInfoPage()
{
  if(ui_->subject_edit->text().trimmed().isEmpty() ||
     ui_->body_edit->toPlainText().trimmed().isEmpty())
  {
    QMessageBox::critical(this, "Описание проблемы",
      "Пожалуйста, заполните краткие и полные сведения о проблеме");
    return false;
  }
  return true;
}
//--------------------------------------------------------------------------------------------------
void
MainWindow::
on_next_btn_clicked()
{
  if(ui_->stacked_widget->currentWidget() == ui_->contacts_page)
  {
//    if(checkContactsPage())
    {
      ui_->stacked_widget->setCurrentWidget(ui_->info_page);
      ui_->prev_btn->setEnabled(true);
    }
  }
  else if(ui_->stacked_widget->currentWidget() == ui_->info_page)
  {
//    if(checkInfoPage())
      ui_->stacked_widget->setCurrentWidget(ui_->screenshots_page);
  }
  else if(ui_->stacked_widget->currentWidget() == ui_->screenshots_page)
  {
    ui_->stacked_widget->setCurrentWidget(ui_->final_page);
    ui_->next_btn->setEnabled(false);
  }
}
//--------------------------------------------------------------------------------------------------
void
MainWindow::
on_prev_btn_clicked()
{
  if(ui_->stacked_widget->currentWidget() == ui_->final_page)
  {
    ui_->stacked_widget->setCurrentWidget(ui_->screenshots_page);
    ui_->next_btn->setEnabled(true);
  }
  else if(ui_->stacked_widget->currentWidget() == ui_->screenshots_page)
  {
    ui_->stacked_widget->setCurrentWidget(ui_->info_page);
  }
  else if(ui_->stacked_widget->currentWidget() == ui_->info_page)
  {
    ui_->stacked_widget->setCurrentWidget(ui_->contacts_page);
    ui_->prev_btn->setEnabled(false);
  }
}
//--------------------------------------------------------------------------------------------------
void
MainWindow::
on_take_screenshot_btn_clicked()
{
  QClipboard* clipboard = QApplication::clipboard();
  QPixmap pixmap = clipboard->pixmap();
  if(!pixmap.isNull())
  {
    QLabel* preview = new QLabel(this);
    preview->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    preview->setFixedHeight(150);
    preview->setPixmap(pixmap.scaled(
      preview->contentsRect().size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    preview->adjustSize();
    screenshots_layout_->addWidget(preview);
    screenshots_.append(pixmap);
  }
}
//--------------------------------------------------------------------------------------------------
void
MainWindow::
on_make_request_btn_clicked()
{
  ui_->console_text_edit->clear();
  RequestContext ctx;
  ctx.subject = ui_->subject_edit->text();
  ctx.body = ui_->body_edit->toPlainText();
  ctx.email = ui_->email_edit->text();
  ctx.phone = ui_->phone_edit->text();
  ctx.company = ui_->company_edit->text();
  ctx.screenshots = screenshots_;
  support_requester_.start(ctx);
  ui_->make_request_btn->setEnabled(false);
}
//--------------------------------------------------------------------------------------------------
void
MainWindow::
appendLog(QString const& message)
{
  ui_->console_text_edit->appendPlainText(message);
}
//--------------------------------------------------------------------------------------------------
void
MainWindow::
processFinished()
{
  appendLog("Процесс успешно завершен!");
  ui_->make_request_btn->setEnabled(true);
}
//--------------------------------------------------------------------------------------------------
void
MainWindow::
processFailed(QString const& message)
{
  appendLog(QString("ОШИБКА! %1").arg(message));
  ui_->make_request_btn->setEnabled(true);
}
//--------------------------------------------------------------------------------------------------
