#include "CoreBarChart.h"

#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>
#include <algorithm>
#include <cmath>

CoreBarChart::CoreBarChart(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(300);
    setMouseTracking(true);
    setAccessibleName("CPU logical cores");
}

void CoreBarChart::setValues(const QList<double> &values)
{
    values_ = values;
    update();
}

QRectF CoreBarChart::plotRect() const
{
    return QRectF(46, 64, std::max(1, width() - 68), std::max(1, height() - 119));
}

void CoreBarChart::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QColor("#25303C")); p.setBrush(QColor("#0D141C"));
    p.drawRoundedRect(QRectF(rect()).adjusted(1, 1, -1, -1), 10, 10);
    auto title = font(); title.setPointSize(12); title.setBold(true); p.setFont(title);
    p.setPen(QColor("#E2E7EE"));
    p.drawText(QRectF(20, 17, width() - 40, 24), "CPU logical cores");
    auto label = font(); label.setPointSize(9); p.setFont(label);
    const QRectF plot = plotRect();
    for (int value = 0; value <= 100; value += 25) {
        const qreal y = plot.bottom() - value / 100.0 * plot.height();
        p.setPen(QColor("#29313D")); p.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
        p.setPen(QColor("#8D9DAE"));
        p.drawText(QRectF(2, y - 9, 37, 18), Qt::AlignRight | Qt::AlignVCenter, QString::number(value) + "%");
    }
    if (values_.isEmpty()) {
        p.drawText(plot, Qt::AlignCenter, "Per-core metrics unavailable");
        return;
    }
    const qreal step = plot.width() / values_.size();
    const int labelStep = std::max(1, int(std::ceil(values_.size() / 16.0)));
    for (int i = 0; i < values_.size(); ++i) {
        const bool valid = std::isfinite(values_[i]) && values_[i] >= 0;
        const qreal height = valid ? std::clamp(values_[i], 0.0, 100.0) / 100 * plot.height() : 3;
        const qreal x = plot.left() + i * step + step * 0.15;
        QLinearGradient gradient(0, plot.top(), 0, plot.bottom());
        gradient.setColorAt(0, QColor("#BE75F2")); gradient.setColorAt(1, QColor("#7039A6"));
        p.setPen(Qt::NoPen); p.setBrush(valid ? QBrush(gradient) : QBrush(QColor("#475569")));
        p.drawRoundedRect(QRectF(x, plot.bottom() - height, std::max(1.0, step * 0.7), height), 2, 2);
        if (i % labelStep == 0) {
            p.save(); p.translate(x + step * 0.35, plot.bottom() + 13); p.rotate(45);
            p.setPen(QColor("#8D9DAE")); p.drawText(0, 0, QString("CPU %1").arg(i + 1)); p.restore();
        }
    }
}

void CoreBarChart::mouseMoveEvent(QMouseEvent *event)
{
    const auto plot = plotRect();
    if (values_.isEmpty() || !plot.contains(event->position())) { QToolTip::hideText(); return; }
    const int core = std::clamp(int((event->position().x() - plot.left()) / plot.width() * values_.size()),
                                0, int(values_.size()) - 1);
    const QString value = values_[core] < 0 ? "Waiting for next sample" : QString::number(values_[core], 'f', 1) + "%";
    QToolTip::showText(event->globalPosition().toPoint(), QString("CPU %1: %2").arg(core + 1).arg(value), this);
}

void CoreBarChart::leaveEvent(QEvent *) { QToolTip::hideText(); }
