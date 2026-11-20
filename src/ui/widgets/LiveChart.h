#pragma once

#include <QColor>
#include <QDateTime>
#include <QElapsedTimer>
#include <QList>
#include <QStringList>
#include <QWidget>
#include <deque>

class QButtonGroup;

class LiveChart : public QWidget
{
    Q_OBJECT

public:
    LiveChart(const QString &title, const QStringList &labels,
              const QList<QColor> &colors, QWidget *parent = nullptr);
    void appendSample(const QList<double> &values);
    void setScale(double maximum, const QString &unit);
    void setTimeWindow(qint64 milliseconds);
    void setZoomControlsVisible(bool visible);
    qint64 timeWindow() const { return windowMs_; }
    int sampleCount() const { return static_cast<int>(samples_.size()); }

protected:
    void paintEvent(QPaintEvent *) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *) override;

private:
    struct Sample { qint64 time; QList<double> values; };
    qint64 visibleSpan() const;
    QRectF plotRect() const;
    QString axisLabel(double value) const;
    QString title_;
    QStringList labels_;
    QList<QColor> colors_;
    QElapsedTimer clock_;
    QDateTime started_ = QDateTime::currentDateTime();
    std::deque<Sample> samples_;
    double maximum_ = 100;
    QString unit_ = "%";
    qint64 windowMs_ = 60000;
    qint64 hoverTime_ = -1;
    QWidget *controls_;
    QButtonGroup *ranges_;
};
