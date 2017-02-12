#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "FlowLayout.h"
#include "ScreenshotLabel.h"
#include <QMessageBox>
#include <QRegExp>
#include <QClipboard>
#include <QImage>
#include <QSettings>
//--------------------------------------------------------------------------------------------------
MainWindow::
MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , ui_(new Ui::MainWindow)
{
  ui_->setupUi(this);

  loadUserParams();

  ui_->close_btn->setVisible(false);
  ui_->prev_btn->setEnabled(false);
  ui_->stacked_widget->setCurrentWidget(ui_->contacts_page);

  ui_->incident_time_edit->setDateTime(QDateTime::currentDateTime());
  connect(ui_->incident_time_edit, &QDateTimeEdit::dateTimeChanged,
          this, &MainWindow::onIncidentTimeChanged);

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
  saveUserParams();
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
    if(checkContactsPage())
    {
      ui_->stacked_widget->setCurrentWidget(ui_->info_page);
      ui_->prev_btn->setEnabled(true);
    }
  }
  else if(ui_->stacked_widget->currentWidget() == ui_->info_page)
  {
    if(checkInfoPage())
      ui_->stacked_widget->setCurrentWidget(ui_->screenshots_page);
  }
  else if(ui_->stacked_widget->currentWidget() == ui_->screenshots_page)
  {
    ui_->stacked_widget->setCurrentWidget(ui_->request_page);
    ui_->next_btn->setEnabled(false);
  }
}
//--------------------------------------------------------------------------------------------------
void
MainWindow::
on_prev_btn_clicked()
{
  if(ui_->stacked_widget->currentWidget() == ui_->request_page)
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
    QImage clipboard_image = pixmap.toImage();
    for(QObject* obj : ui_->screenshots_area_contents->children())
    {
      if(ScreenshotLabel* label = qobject_cast<ScreenshotLabel*>(obj))
      {
        if(clipboard_image == label->screenshot().toImage())
          return;
      }
    }
    screenshots_layout_->addWidget(new ScreenshotLabel(pixmap, this));
  }
}
//--------------------------------------------------------------------------------------------------
void
MainWindow::
on_make_request_btn_clicked()
{
  saveUserParams();
  ui_->console_text_edit->clear();
  RequestContext ctx;
  if(ui_->this_comp_yes_rbtn->isChecked())
    ctx.place = RequestContext::THIS_COMPUTER;
  else if(ui_->this_comp_no_rbtn->isChecked())
    ctx.place = RequestContext::ANOTHER_COMPUTER;
  ctx.incident_time = incident_time_;
  ctx.subject = ui_->subject_edit->text();
  ctx.body = ui_->body_edit->toPlainText();
  ctx.name = ui_->name_edit->text();
  ctx.email = ui_->email_edit->text();
  ctx.phone = ui_->phone_edit->text();
  ctx.company = ui_->company_edit->text();
  for(QObject* obj : ui_->screenshots_area_contents->children())
  {
    if(ScreenshotLabel* label = qobject_cast<ScreenshotLabel*>(obj))
      ctx.screenshots += label->screenshot();
  }
  support_requester_.start(ctx);

  ui_->make_request_btn->setEnabled(false);
  ui_->prev_btn->setEnabled(false);
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
  appendLog("Готово!");
  ui_->next_btn->setVisible(false);
  ui_->prev_btn->setVisible(false);
  ui_->close_btn->setVisible(true);
  ui_->stacked_widget->setCurrentWidget(ui_->final_page);
}
//--------------------------------------------------------------------------------------------------
void
MainWindow::
processFailed(QString const& message)
{
  appendLog(QString("ОШИБКА! %1").arg(message));
  ui_->make_request_btn->setEnabled(true);
  ui_->prev_btn->setEnabled(true);
}
//--------------------------------------------------------------------------------------------------
void
MainWindow::
onIncidentTimeChanged(QDateTime const& date_time)
{
  incident_time_ = date_time;
}
//--------------------------------------------------------------------------------------------------
void
MainWindow::
loadUserParams()
{
  QSettings settings("TechSupport.ini", QSettings::IniFormat);
  ui_->name_edit->setText(settings.value("name").toString());
  ui_->company_edit->setText(settings.value("company").toString());
  ui_->phone_edit->setText(settings.value("phone").toString());
  ui_->email_edit->setText(settings.value("email").toString());
}
//--------------------------------------------------------------------------------------------------
void
MainWindow::
saveUserParams()
{
  QSettings settings("TechSupport.ini", QSettings::IniFormat);
  settings.setValue("name", ui_->name_edit->text());
  settings.setValue("company", ui_->company_edit->text());
  settings.setValue("phone", ui_->phone_edit->text());
  settings.setValue("email", ui_->email_edit->text());
}
//--------------------------------------------------------------------------------------------------
