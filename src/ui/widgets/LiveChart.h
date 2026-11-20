#pragma once

#include <QColor>
#include <QElapsedTimer>
#include <QList>
#include <QStringList>
#include <QWidget>
#include <deque>

class LiveChart : public QWidget
{
    Q_OBJECT

public:
    LiveChart(const QString &title, const QStringList &labels,
              const QList<QColor> &colors, QWidget *parent = nullptr);
    void appendSample(const QList<double> &values);
    int sampleCount() const { return static_cast<int>(samples_.size()); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    struct Sample { qint64 time; QList<double> values; };
    QString title_;
    QStringList labels_;
    QList<QColor> colors_;
    QElapsedTimer clock_;
    std::deque<Sample> samples_;
};
