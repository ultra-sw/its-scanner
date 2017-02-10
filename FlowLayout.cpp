#include <QtWidgets>
#include "FlowLayout.h"
//--------------------------------------------------------------------------------------------------
FlowLayout::
FlowLayout(QWidget* parent, int margin, int h_spacing, int v_spacing)
    : QLayout(parent), h_spacing_(h_spacing), v_spacing_(v_spacing)
{
  setContentsMargins(margin, margin, margin, margin);
}
//--------------------------------------------------------------------------------------------------
FlowLayout::
FlowLayout(int margin, int h_spacing, int v_spacing)
    : h_spacing_(h_spacing), v_spacing_(v_spacing)
{
  setContentsMargins(margin, margin, margin, margin);
}
//--------------------------------------------------------------------------------------------------
FlowLayout::
~FlowLayout()
{
  QLayoutItem* item;
  while((item = takeAt(0)))
    delete item;
}
//--------------------------------------------------------------------------------------------------
void
FlowLayout::
addItem(QLayoutItem* item)
{
  items_.append(item);
}
//--------------------------------------------------------------------------------------------------
int
FlowLayout::
horizontalSpacing() const
{
  if(h_spacing_ >= 0)
    return h_spacing_;
  else
    return smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
}
//--------------------------------------------------------------------------------------------------
int
FlowLayout::
verticalSpacing() const
{
  if(v_spacing_ >= 0)
    return v_spacing_;
  else
    return smartSpacing(QStyle::PM_LayoutVerticalSpacing);
}
//--------------------------------------------------------------------------------------------------
int
FlowLayout::
count() const
{
  return items_.size();
}
//--------------------------------------------------------------------------------------------------
QLayoutItem*
FlowLayout::
itemAt(int index) const
{
  return items_.value(index);
}
//--------------------------------------------------------------------------------------------------
QLayoutItem*
FlowLayout::
takeAt(int index)
{
  if (index >= 0 && index < items_.size())
    return items_.takeAt(index);
  else
    return 0;
}
//--------------------------------------------------------------------------------------------------
Qt::Orientations
FlowLayout::
expandingDirections() const
{
  return 0;
}
//--------------------------------------------------------------------------------------------------
bool
FlowLayout::
hasHeightForWidth() const
{
    return true;
}
//--------------------------------------------------------------------------------------------------
int
FlowLayout::
heightForWidth(int width) const
{
  return doLayout(QRect(0, 0, width, 0), true);
}
//--------------------------------------------------------------------------------------------------
void
FlowLayout::
setGeometry(QRect const& rect)
{
  QLayout::setGeometry(rect);
  doLayout(rect, false);
}
//--------------------------------------------------------------------------------------------------
QSize
FlowLayout::
sizeHint() const
{
  return minimumSize();
}
//--------------------------------------------------------------------------------------------------
QSize
FlowLayout::
minimumSize() const
{
  QSize size;
  for(QLayoutItem* item : items_)
    size = size.expandedTo(item->minimumSize());
  size += QSize(2*margin(), 2*margin());
  return size;
}
//--------------------------------------------------------------------------------------------------
int
FlowLayout::
doLayout(QRect const& rect, bool test_only) const
{
  int left, top, right, bottom;
  getContentsMargins(&left, &top, &right, &bottom);
  QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
  int x = effectiveRect.x();
  int y = effectiveRect.y();
  int line_height = 0;

  for(QLayoutItem* item : items_)
  {
    QWidget* widget = item->widget();
    int space_x = horizontalSpacing();
    if(space_x == -1)
    {
      space_x = widget->style()->layoutSpacing(
        QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);
    }
    int space_y = verticalSpacing();
    if(space_y == -1)
    {
      space_y = widget->style()->layoutSpacing(
        QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);
    }

    int next_x = x + item->sizeHint().width() + space_x;
    if(next_x - space_x > effectiveRect.right() && line_height > 0)
    {
      x = effectiveRect.x();
      y = y + line_height + space_y;
      next_x = x + item->sizeHint().width() + space_x;
      line_height = 0;
    }

    if(!test_only)
      item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));

    x = next_x;
    line_height = qMax(line_height, item->sizeHint().height());
  }
  return y + line_height - rect.y() + bottom;
}
//--------------------------------------------------------------------------------------------------
int
FlowLayout::
smartSpacing(QStyle::PixelMetric pm) const
{
  QObject* parent = this->parent();
  if(!parent)
    return -1;
  else if(parent->isWidgetType())
  {
    QWidget* pw = static_cast<QWidget*>(parent);
    return pw->style()->pixelMetric(pm, 0, pw);
  }
  else
    return static_cast<QLayout*>(parent)->spacing();
}
//--------------------------------------------------------------------------------------------------
