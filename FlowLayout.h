#pragma once
#include <QLayout>
#include <QRect>
#include <QStyle>
//--------------------------------------------------------------------------------------------------
class FlowLayout : public QLayout
{
public:
  explicit FlowLayout(QWidget* parent, int margin=-1, int h_spacing=-1, int v_spacing=-1);

  explicit FlowLayout(int margin=-1, int h_spacing=-1, int v_spacing=-1);

  ~FlowLayout();

  virtual void
  addItem(QLayoutItem* item) override;

  int
  horizontalSpacing() const;

  int
  verticalSpacing() const;

  virtual Qt::Orientations
  expandingDirections() const override;

  virtual bool
  hasHeightForWidth() const override;

  virtual int
  heightForWidth(int) const override;

  virtual int
  count() const override;

  virtual QLayoutItem*
  itemAt(int index) const override;

  virtual QSize
  minimumSize() const override;

  virtual void
  setGeometry(QRect const& rect) override;

  virtual QSize
  sizeHint() const override;

  virtual QLayoutItem*
  takeAt(int index) override;

private:
  int
  doLayout(QRect const& rect, bool test_only) const;

  int
  smartSpacing(QStyle::PixelMetric pm) const;

  QList<QLayoutItem*> items_;
  int h_spacing_;
  int v_spacing_;
};
//--------------------------------------------------------------------------------------------------
