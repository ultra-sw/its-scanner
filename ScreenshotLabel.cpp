#include "ScreenshotLabel.h"
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
//--------------------------------------------------------------------------------------------------
ScreenshotLabel::
ScreenshotLabel(QPixmap const& screenshot, QWidget* parent)
  : QLabel(parent)
  , screenshot_(screenshot)
  , close_overlay_(":/resources/Cancel.png")
{
  setFrameStyle(QFrame::Panel | QFrame::Sunken);
  setFixedHeight(150);
  setPixmap(screenshot_.scaled(
    contentsRect().size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
  close_overlay_rect_ = QRect(QLabel::pixmap()->rect().right()-28, 4, 24, 24);
  setMouseTracking(true);
}
//--------------------------------------------------------------------------------------------------
ScreenshotLabel::
~ScreenshotLabel()
{
}
//--------------------------------------------------------------------------------------------------

void
ScreenshotLabel::
enterEvent(QEvent* event)
{
  is_overlay_ = true;
  QPixmap pixmap = QLabel::pixmap()->copy();
  QPainter painter(&pixmap);
  painter.setOpacity(0.4);
  QRect rect = pixmap.rect();
  painter.fillRect(rect, Qt::black);
  setPixmap(pixmap);
  QLabel::enterEvent(event);
}
//--------------------------------------------------------------------------------------------------
void
ScreenshotLabel::
mouseMoveEvent(QMouseEvent* event)
{
  bool need_highlight = close_overlay_rect_.contains(event->pos());
  if(need_highlight && !highlight_close_overlay_)
  {
    highlight_close_overlay_ = true;
    update();
  }
  else if(!need_highlight && highlight_close_overlay_)
  {
    highlight_close_overlay_ = false;
    update();
  }
  QLabel::mouseMoveEvent(event);
}
//--------------------------------------------------------------------------------------------------
void
ScreenshotLabel::
leaveEvent(QEvent* event)
{
  is_overlay_ = false;
  setPixmap(screenshot_.scaled(
    contentsRect().size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
  QLabel::leaveEvent(event);
}
//--------------------------------------------------------------------------------------------------
void
ScreenshotLabel::
paintEvent(QPaintEvent* event)
{
  QLabel::paintEvent(event);
  if(is_overlay_)
  {
    QPainter painter(this);
    painter.save();
    if(highlight_close_overlay_)
      painter.setOpacity(1.0);
    else
      painter.setOpacity(0.75);
    painter.drawPixmap(close_overlay_rect_, close_overlay_, close_overlay_.rect());
    painter.restore();
  }
}
//--------------------------------------------------------------------------------------------------
void
ScreenshotLabel::
mousePressEvent(QMouseEvent* event)
{
  if(event->button() == Qt::LeftButton)
    this->deleteLater();
}
//--------------------------------------------------------------------------------------------------
