#include "LiveChart.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QToolTip>
#include <algorithm>
#include <cmath>

namespace {
constexpr qint64 retentionMs = 1800000;
constexpr int maxSamples = 3600;
constexpr qint64 ranges[]{60000, 300000, 1800000, 0};
}

LiveChart::LiveChart(const QString &title, const QStringList &labels,
                     const QList<QColor> &colors, QWidget *parent)
    : QWidget(parent), title_(title), labels_(labels), colors_(colors)
{
    setMinimumHeight(240);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAccessibleName(title);
    setMouseTracking(true);
    clock_.start();
    controls_ = new QWidget(this);
    controls_->setGeometry(18, 41, 235, 27);
    controls_->setStyleSheet("background: transparent;");
    auto *layout = new QHBoxLayout(controls_);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(3);
    ranges_ = new QButtonGroup(this);
    const QStringList names{"1m", "5m", "30m", "All"};
    for (int i = 0; i < names.size(); ++i) {
        auto *button = new QPushButton(names[i], controls_);
        button->setCheckable(true);
        button->setCursor(Qt::PointingHandCursor);
        button->setStyleSheet(
            "QPushButton { background: transparent; color: #8D9DAE; border: none; "
            "border-radius: 5px; padding: 3px 8px; font-size: 11px; }"
            "QPushButton:checked { background: #DDE5F2; color: #111820; }"
            "QPushButton:hover { background: #283345; }");
        button->setToolTip(i == 3 ? "All retained samples (up to 30 minutes)" : "Show the last " + names[i]);
        ranges_->addButton(button, i);
        layout->addWidget(button);
    }
    ranges_->button(0)->setChecked(true);
    connect(ranges_, &QButtonGroup::idClicked, this, [this](int id) { setTimeWindow(ranges[id]); });
    controls_->hide();
}

void LiveChart::setZoomControlsVisible(bool visible)
{
    controls_->setVisible(visible);
}

void LiveChart::setScale(double maximum, const QString &unit)
{
    maximum_ = std::isfinite(maximum) && maximum > 0 ? maximum : 1;
    unit_ = unit;
    update();
}

void LiveChart::setTimeWindow(qint64 milliseconds)
{
    if (milliseconds < 0) return;
    windowMs_ = milliseconds ? std::clamp<qint64>(milliseconds, 1000, retentionMs) : 0;
    for (int i = 0; i < 4; ++i)
        if (ranges[i] == windowMs_) ranges_->button(i)->setChecked(true);
    hoverTime_ = -1;
    QToolTip::hideText();
    update();
}

void LiveChart::appendSample(const QList<double> &values)
{
    if (values.size() != labels_.size() || colors_.size() != labels_.size()) return;
    const qint64 now = clock_.elapsed();
    samples_.push_back({now, values});
    while (samples_.size() > maxSamples || samples_.front().time < now - retentionMs)
        samples_.pop_front();
    update();
}

qint64 LiveChart::visibleSpan() const
{
    if (windowMs_) return windowMs_;
    return samples_.empty() ? 1000 : std::max<qint64>(1000, samples_.back().time - samples_.front().time);
}

QRectF LiveChart::plotRect() const
{
    return QRectF(24, 84, std::max(1, width() - 99), std::max(1, height() - 122));
}

QString LiveChart::axisLabel(double value) const
{
    return QString::number(value, 'f', unit_ == "%" ? 0 : 2) + (unit_ == "%" ? "%" : " " + unit_);
}

void LiveChart::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QColor("#25303C"));
    painter.setBrush(QColor("#0D141C"));
    painter.drawRoundedRect(QRectF(rect()).adjusted(1, 1, -1, -1), 10, 10);
    auto titleFont = font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(QColor("#E2E7EE"));
    painter.drawText(QRectF(20, 17, width() - 40, 24), title_);
    auto labelFont = font();
    labelFont.setPointSize(9);
    painter.setFont(labelFont);
    if (controls_->isHidden()) {
        for (int i = 0; i < labels_.size(); ++i) {
            painter.setPen(colors_[i]);
            painter.drawText(QRectF(24 + i * 100, 47, 98, 20), labels_[i]);
        }
    }
    const QRectF plot = plotRect();
    for (int step = 0; step <= 4; ++step) {
        const qreal y = plot.bottom() - step / 4.0 * plot.height();
        painter.setPen(QPen(QColor("#29313D"), 1));
        painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
        painter.setPen(QColor("#8D9DAE"));
        painter.drawText(QRectF(plot.right() + 9, y - 9, 65, 18), Qt::AlignVCenter,
                         axisLabel(maximum_ * step / 4));
    }
    const QString startLabel = windowMs_ ? QString("%1 min ago").arg(visibleSpan() / 60000.0, 0, 'f', 0) :
        samples_.empty() ? QStringLiteral("Waiting for samples") :
        started_.addMSecs(samples_.front().time).toString("HH:mm:ss");
    painter.drawText(QRectF(plot.left(), plot.bottom() + 9, 120, 20), startLabel);
    painter.drawText(QRectF(plot.right() - 45, plot.bottom() + 9, 45, 20), Qt::AlignRight, "Latest");
    if (samples_.empty()) return;
    const qint64 end = samples_.back().time;
    auto pointFor = [&](const Sample &sample, double value) {
        return QPointF(plot.right() - double(end - sample.time) / visibleSpan() * plot.width(),
            plot.bottom() - std::clamp(value / maximum_, 0.0, 1.0) * plot.height());
    };
    painter.save();
    painter.setClipRect(plot.adjusted(-3, -3, 3, 3));
    for (int series = 0; series < labels_.size(); ++series) {
        QPainterPath segment;
        QPointF first, last;
        bool active = false;
        auto flush = [&] {
            if (!active) return;
            QPainterPath fill = segment;
            fill.lineTo(last.x(), plot.bottom());
            fill.lineTo(first.x(), plot.bottom());
            fill.closeSubpath();
            QLinearGradient gradient(0, plot.top(), 0, plot.bottom());
            QColor top = colors_[series]; top.setAlpha(150);
            QColor bottom = colors_[series]; bottom.setAlpha(8);
            gradient.setColorAt(0, top); gradient.setColorAt(1, bottom);
            painter.fillPath(fill, gradient);
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(colors_[series], 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.drawPath(segment);
            active = false;
            segment = {};
        };
        for (const auto &sample : samples_) {
            const double value = sample.values[series];
            if (!std::isfinite(value) || value < 0) { flush(); continue; }
            const QPointF point = pointFor(sample, value);
            if (active) segment.lineTo(point);
            else { segment.moveTo(point); first = point; active = true; }
            last = point;
        }
        const bool hasLatest = active;
        flush();
        if (hasLatest) {
            painter.setPen(Qt::NoPen); painter.setBrush(colors_[series]);
            painter.drawEllipse(last, 3, 3);
        }
    }
    if (hoverTime_ >= 0) {
        const qreal x = plot.right() - double(end - hoverTime_) / visibleSpan() * plot.width();
        painter.setPen(QPen(QColor("#CBD5E1"), 1, Qt::DashLine));
        painter.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
    }
    painter.restore();
}

void LiveChart::mouseMoveEvent(QMouseEvent *event)
{
    const QRectF plot = plotRect();
    if (samples_.empty() || !plot.contains(event->position())) {
        hoverTime_ = -1; QToolTip::hideText(); update(); return;
    }
    const qint64 target = samples_.back().time -
        qint64((plot.right() - event->position().x()) / plot.width() * visibleSpan());
    const auto nearest = std::min_element(samples_.begin(), samples_.end(),
        [target](const Sample &a, const Sample &b) { return std::abs(a.time - target) < std::abs(b.time - target); });
    hoverTime_ = nearest->time;
    QString text = started_.addMSecs(nearest->time).toString("HH:mm:ss");
    for (int i = 0; i < labels_.size(); ++i) {
        const double value = nearest->values[i];
        text += "\n" + labels_[i] + ": " + (std::isfinite(value) && value >= 0 ? axisLabel(value) : "--");
    }
    QToolTip::showText(event->globalPosition().toPoint(), text, this);
    update();
}

void LiveChart::leaveEvent(QEvent *)
{
    hoverTime_ = -1;
    QToolTip::hideText();
    update();
}
