#pragma once

#include <QList>
#include <QWidget>

class CoreBarChart : public QWidget
{
    Q_OBJECT
public:
    explicit CoreBarChart(QWidget *parent = nullptr);
    void setValues(const QList<double> &values);
    int coreCount() const { return values_.size(); }

protected:
    void paintEvent(QPaintEvent *) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *) override;

private:
    QRectF plotRect() const;
    QList<double> values_;
};
