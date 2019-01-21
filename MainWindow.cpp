#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "FlowLayout.h"
#include "ScreenshotLabel.h"
#include "SimpleCrypt.h"
#include <QMessageBox>
#include <QRegExp>
#include <QClipboard>
#include <QImage>
#include <QSettings>
#include <QNetworkProxyFactory>
#include <QTemporaryFile>
//--------------------------------------------------------------------------------------------------
quint64 const MainWindow::SALT = 0x927b6670b16901d8;
//--------------------------------------------------------------------------------------------------
MainWindow::
MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , ui_(new Ui::MainWindow)
{
  ui_->setupUi(this);
  ui_->connect_widget->setVisible(false);

  ui_->smtp_encryption_combo_box->addItem("TLS", SmtpClient::TlsConnection);
  ui_->smtp_encryption_combo_box->addItem("SSL", SmtpClient::SslConnection);
  ui_->smtp_encryption_combo_box->addItem("TCP", SmtpClient::TcpConnection);
  ui_->smtp_encryption_combo_box->setCurrentIndex(-1);

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

  connect(&support_requester_, &SupportRequester::log, this, &MainWindow::appendLog);
  connect(&support_requester_, &SupportRequester::finished, this, &MainWindow::processFinished);
  connect(&support_requester_, &SupportRequester::failed, this, &MainWindow::processFailed);
  connect(&downloader_, &FileDownloader::downloaded, this, &MainWindow::fileDownloaded);
  connect(&downloader_, &FileDownloader::failed, this, &MainWindow::fileDownloadFailed);
  connect(ui_->check_connect_btn, &QPushButton::clicked, this, &MainWindow::getParamsFromServer);

  getParamsFromServer();
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
  setFgColor(ui_->smtp_address_edit_label, Qt::black);
  setFgColor(ui_->smtp_port_spin_box_label, Qt::black);
  setFgColor(ui_->smtp_encryption_combo_box_label, Qt::black);
  setFgColor(ui_->smtp_username_edit_label, Qt::black);
  setFgColor(ui_->smtp_password_edit_label, Qt::black);

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
  if(ui_->smtp_address_edit->text().trimmed().isEmpty())
  {
    setFgColor(ui_->smtp_address_edit_label, Qt::red);
    can_proceed = false;
  }
  if(ui_->smtp_port_spin_box->value() <= 0)
  {
    setFgColor(ui_->smtp_port_spin_box_label, Qt::red);
    can_proceed = false;
  }
  if(ui_->smtp_encryption_combo_box->currentIndex() == -1)
  {
    setFgColor(ui_->smtp_encryption_combo_box_label, Qt::red);
    can_proceed = false;
  }
  if(ui_->smtp_username_edit->text().trimmed().isEmpty())
  {
    setFgColor(ui_->smtp_username_edit_label, Qt::red);
    can_proceed = false;
  }
  if(ui_->smtp_password_edit->text().trimmed().isEmpty())
  {
    setFgColor(ui_->smtp_password_edit_label, Qt::red);
    can_proceed = false;
  }

  if(!can_proceed)
  {
    QMessageBox::critical(this, "Контактные сведения", "Заполните поля, помеченные красным");
    return false;
  }

  QRegExp mail_rex("\\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,63}\\b", Qt::CaseInsensitive);
  if(!mail_rex.exactMatch(ui_->email_edit->text().trimmed()))
  {
    setFgColor(ui_->email_edit_label, Qt::red);
    QMessageBox::critical(this, "Контактные сведения", "Неправильно заполнен email");
    return false;
  }

  QRegExp host_rex("^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$|^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]*[a-zA-Z0-9])\\.)+([A-Za-z]|[A-Za-z][A-Za-z0-9\\-]*[A-Za-z0-9])$");
  if(//!ip_rex.exactMatch(ui_->smtp_address_edit->text().trimmed()) &&
     !host_rex.exactMatch(ui_->smtp_address_edit->text().trimmed()))
  {
    setFgColor(ui_->smtp_address_edit_label, Qt::red);
    QMessageBox::critical(this, "Настройки почты", "Неправильно указан SMTP сервер");
    return false;
  }

  return true;
}
//--------------------------------------------------------------------------------------------------
bool
MainWindow::
checkInfoPage()
{
  if(ui_->queue_combo_box->currentIndex() == -1)
  {
    QMessageBox::critical(this, "Описание проблемы",
      "Пожалуйста, выберите категорию поддержки.");
    return false;
  }
  if(ui_->subject_edit->text().trimmed().isEmpty() ||
     ui_->body_edit->toPlainText().trimmed().isEmpty())
  {
    QMessageBox::critical(this, "Описание проблемы",
      "Пожалуйста, заполните краткие и полные сведения о проблеме.");
    return false;
  }
  return true;
}
//--------------------------------------------------------------------------------------------------
void
MainWindow::
on_check_settings_btn_clicked()
{
  if(checkContactsPage())
  {
    QString message;
    if(!support_requester_.checkSettings(prepareRequest(), message))
    {
      QMessageBox::critical(this, "Настройки почты",
        QString("Ошибки в настройках почты:\n%1").arg(message));
    }
    else
    {
      QMessageBox::information(this, "Проверка настроек",
        "Ошибки в настройках не обнаружены.");
    }
  }
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
RequestSettings
MainWindow::
prepareRequest() const
{
  RequestSettings request_settings;
  request_settings.conn_type = SmtpClient::ConnectionType(ui_->smtp_encryption_combo_box->currentData().toInt());
  request_settings.smtp_server = ui_->smtp_address_edit->text();
  request_settings.smtp_port = ui_->smtp_port_spin_box->value();
  request_settings.smtp_user = ui_->smtp_username_edit->text();
  request_settings.smtp_password = ui_->smtp_password_edit->text();
  request_settings.from_email = ui_->email_edit->text();
  request_settings.support_email = ui_->queue_combo_box->currentData().toString();
  request_settings.support_recipient = ui_->queue_combo_box->currentText();
  return request_settings;
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

  support_requester_.start(prepareRequest(), ctx);

  ui_->make_request_btn->setEnabled(false);
  ui_->prev_btn->setEnabled(false);
}
//--------------------------------------------------------------------------------------------------
void
MainWindow::
getParamsFromServer()
{
  QNetworkProxyFactory::setUseSystemConfiguration(ui_->system_proxy_check_box->isChecked());
  downloader_.download(QUrl(support_requester_.downloadUrl() + "/scan.ini"));
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
  settings.setIniCodec("UTF-8");

  ui_->name_edit->setText(settings.value("name").toString());
  ui_->company_edit->setText(settings.value("company").toString());
  ui_->phone_edit->setText(settings.value("phone").toString());
  ui_->email_edit->setText(settings.value("email").toString());

  QString smtp_auth = settings.value("smtp_auth").toString();
  if(smtp_auth == "ssl")
  {
    ui_->smtp_encryption_combo_box->setCurrentIndex(
          ui_->smtp_encryption_combo_box->findData(SmtpClient::SslConnection));
  }
  else if(smtp_auth == "tls")
  {
    ui_->smtp_encryption_combo_box->setCurrentIndex(
          ui_->smtp_encryption_combo_box->findData(SmtpClient::TlsConnection));
  }
  else if(smtp_auth == "tcp")
  {
    ui_->smtp_encryption_combo_box->setCurrentIndex(
          ui_->smtp_encryption_combo_box->findData(SmtpClient::TcpConnection));
  }

  ui_->smtp_address_edit->setText(settings.value("smtp_server").toString());
  ui_->smtp_port_spin_box->setValue(settings.value("smtp_port").toInt());
  ui_->smtp_username_edit->setText(settings.value("smtp_user").toString());
  ui_->smtp_password_edit->setText(decrypt(settings.value("smtp_password").toString()));
  ui_->system_proxy_check_box->setChecked(settings.value("use_system_proxy").toBool());

  QString download_url = settings.value("download_url").toString();
  if(!download_url.isEmpty())
    support_requester_.setDownloadUrl(download_url);
}
//--------------------------------------------------------------------------------------------------
void
MainWindow::
saveUserParams()
{
  QSettings settings("TechSupport.ini", QSettings::IniFormat);
  settings.setIniCodec("UTF-8");
  settings.setValue("name", ui_->name_edit->text());
  settings.setValue("company", ui_->company_edit->text());
  settings.setValue("phone", ui_->phone_edit->text());
  settings.setValue("email", ui_->email_edit->text());

  if(ui_->smtp_encryption_combo_box->currentIndex() != -1)
  {
    SmtpClient::ConnectionType conn_type =
        SmtpClient::ConnectionType(ui_->smtp_encryption_combo_box->currentData().toInt());
    if(conn_type == SmtpClient::SslConnection)
      settings.setValue("smtp_auth", "ssl");
    else if(conn_type == SmtpClient::TlsConnection)
      settings.setValue("smtp_auth", "tls");
    else if(conn_type == SmtpClient::TcpConnection)
      settings.setValue("smtp_auth", "tcp");
  }
  settings.setValue("smtp_server", ui_->smtp_address_edit->text());
  settings.setValue("smtp_port", ui_->smtp_port_spin_box->value());
  settings.setValue("smtp_user", ui_->smtp_username_edit->text());
  settings.setValue("smtp_password", encrypt(ui_->smtp_password_edit->text()));
  settings.setValue("use_system_proxy", ui_->system_proxy_check_box->isChecked());
}
//--------------------------------------------------------------------------------------------------
void
MainWindow::
fileDownloaded(QByteArray const& data)
{
  QTemporaryFile tmp_file;
  tmp_file.setAutoRemove(false);
  if(!tmp_file.open())
  {
    QMessageBox::critical(this, "Ошибка",
      "Возникла непредвиденная ошибка при чтении настроек с сервера поддержки");
    ui_->next_btn->setEnabled(false);
    return;
  }
  tmp_file.write(data);
  QString file_name = tmp_file.fileName();
  tmp_file.close();

  ui_->connect_widget->setVisible(false);
  ui_->next_btn->setEnabled(true);

  QSettings settings(file_name, QSettings::IniFormat);
  settings.setIniCodec("UTF-8");
  settings.beginGroup("Queues");
  QStringList keys = settings.childKeys();
  settings.endGroup();
  for(QString const& key : keys)
  {
    settings.beginGroup("Queues");
    QString email = settings.value(key).toString();
    settings.endGroup();
    if(!email.isEmpty())
    {
      settings.beginGroup(email);
      QString name = settings.value("name").toString();
      if(!name.isEmpty())
        ui_->queue_combo_box->addItem(name, email);
      settings.endGroup();
    }
  }
  ui_->queue_combo_box->setCurrentIndex(-1);

  QFile::remove(file_name);
}
//--------------------------------------------------------------------------------------------------
void
MainWindow::
fileDownloadFailed()
{
  ui_->connect_widget->setVisible(true);
  ui_->next_btn->setEnabled(false);
}
//--------------------------------------------------------------------------------------------------
QString
MainWindow::
encrypt(QString const& data)
{
  return SimpleCrypt(SALT).encryptToString(data);
}
//--------------------------------------------------------------------------------------------------
QString
MainWindow::
decrypt(QString const& data)
{
  return SimpleCrypt(SALT).decryptToString(data);
}
//--------------------------------------------------------------------------------------------------
