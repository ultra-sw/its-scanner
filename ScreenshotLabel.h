#pragma once
#include <QLabel>
//--------------------------------------------------------------------------------------------------
class ScreenshotLabel : public QLabel
{
  Q_OBJECT
public:
  explicit ScreenshotLabel(QPixmap const& screenshot, QWidget* parent=nullptr);

  virtual
  ~ScreenshotLabel();

  QPixmap const&
  screenshot() const
  {
    return screenshot_;
  }

protected:
  virtual void
  enterEvent(QEvent* event) override;

  virtual void
  mouseMoveEvent(QMouseEvent* event) override;

  virtual void
  leaveEvent(QEvent* event) override;

  virtual void
  paintEvent(QPaintEvent* event) override;

  virtual void
  mousePressEvent(QMouseEvent* event) override;
private:
  QPixmap screenshot_;
  QPixmap close_overlay_;
  QRect close_overlay_rect_;
  bool is_overlay_ = false;
  bool highlight_close_overlay_ = false;
};
//--------------------------------------------------------------------------------------------------
