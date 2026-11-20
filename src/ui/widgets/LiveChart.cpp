#include "LiveChart.h"

#include <QPainter>
#include <QPainterPath>
#include <algorithm>
#include <cmath>

namespace {
constexpr qint64 windowMs = 120000;
constexpr int maxSamples = 240;
}

LiveChart::LiveChart(const QString &title, const QStringList &labels,
                     const QList<QColor> &colors, QWidget *parent)
    : QWidget(parent), title_(title), labels_(labels), colors_(colors)
{
    setMinimumHeight(240);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAccessibleName(title);
    clock_.start();
}

void LiveChart::appendSample(const QList<double> &values)
{
    if (values.size() != labels_.size() || colors_.size() != labels_.size()) return;
    const qint64 now = clock_.elapsed();
    samples_.push_back({now, values});
    while (samples_.size() > maxSamples || samples_.front().time < now - windowMs)
        samples_.pop_front();
    update();
}

void LiveChart::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QColor("#25334A"));
    painter.setBrush(QColor("#111D30"));
    painter.drawRoundedRect(QRectF(rect()).adjusted(1, 1, -1, -1), 16, 16);

    auto titleFont = font();
    titleFont.setPointSize(13);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(QColor("#E7EDF7"));
    painter.drawText(QRectF(22, 15, width() - 44, 25), title_);
    auto labelFont = font();
    labelFont.setPointSize(10);
    painter.setFont(labelFont);
    for (int i = 0; i < labels_.size(); ++i) {
        const qreal x = 24 + i * 115;
        painter.setBrush(colors_[i]);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(x + 3, 57), 3, 3);
        painter.setPen(QColor("#94A6BF"));
        painter.drawText(QRectF(x + 13, 47, 98, 20), labels_[i]);
    }

    const QRectF plot(52, 83, width() - 76, height() - 119);
    for (int value = 0; value <= 100; value += 25) {
        const qreal y = plot.bottom() - value / 100.0 * plot.height();
        painter.setPen(QPen(QColor("#25334A"), 1, Qt::DotLine));
        painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
        painter.setPen(QColor("#8B9DB7"));
        painter.drawText(QRectF(3, y - 9, 41, 18), Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(value) + "%");
    }
    painter.drawText(QRectF(plot.left(), plot.bottom() + 8, 90, 20), "2 min ago");
    painter.drawText(QRectF(plot.right() - 45, plot.bottom() + 8, 45, 20), Qt::AlignRight, "Latest");
    if (samples_.empty()) return;
    const qint64 end = samples_.back().time;
    painter.save();
    painter.setClipRect(plot.adjusted(-3, -3, 3, 3));
    for (int series = 0; series < labels_.size(); ++series) {
        QPainterPath path;
        bool connected = false;
        QPointF last;
        for (const auto &sample : samples_) {
            const double value = sample.values[series];
            if (!std::isfinite(value) || value < 0) { connected = false; continue; }
            const QPointF point(plot.right() - double(end - sample.time) / windowMs * plot.width(),
                plot.bottom() - std::clamp(value, 0.0, 100.0) / 100.0 * plot.height());
            if (connected) path.lineTo(point); else path.moveTo(point);
            last = point;
            connected = true;
        }
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(colors_[series], 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawPath(path);
        if (connected) {
            painter.setBrush(colors_[series]);
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(last, 3.5, 3.5);
        }
    }
    painter.restore();
}
