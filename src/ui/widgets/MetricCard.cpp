#include "MetricCard.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

class UsageRing : public QWidget
{
public:
    UsageRing(const QString &title, QWidget *parent) : QWidget(parent), title_(title)
    {
        setFixedSize(54, 54);
        setStyleSheet("background: transparent;");
        color_ = title.startsWith("CPU") ? QColor("#4C9CEC") :
                 title.startsWith("RAM") ? QColor("#29B8CD") :
                 title.startsWith("SWAP") ? QColor("#ED647B") : QColor("#AC5DCD");
    }
    void setUsage(double usage) { usage_ = usage; update(); }
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        p.fillRect(rect(), QColor("#0D141C"));
        const QRectF circle(5, 5, 44, 44);
        p.setPen(QPen(QColor("#28313D"), 5)); p.drawEllipse(circle);
        if (std::isfinite(usage_) && usage_ >= 0) {
            p.setPen(QPen(color_, 5, Qt::SolidLine, Qt::RoundCap));
            p.drawArc(circle, 90 * 16, -qRound(std::clamp(usage_, 0.0, 100.0) * 3.6 * 16));
        }
        auto f = font(); f.setPointSize(8); f.setBold(true); p.setFont(f);
        p.setPen(QColor("#A7B3C3")); p.drawText(rect(), Qt::AlignCenter, title_);
    }
private:
    QString title_;
    QColor color_;
    double usage_ = -1;
};

MetricCard::MetricCard(const QString &title, const QString &value, QWidget *parent)
    : QFrame(parent)
{
    setObjectName("metricCard");
    setMinimumHeight(94);
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(14);
    ring_ = new UsageRing(title.toUpper(), this);
    layout->addWidget(ring_);
    auto *text = new QVBoxLayout;
    text->setSpacing(6);
    auto *label = new QLabel(title.toUpper(), this);
    label->setObjectName("metricTitle");
    valueLabel_ = new QLabel(value, this);
    valueLabel_->setObjectName("metricValue");
    valueLabel_->setWordWrap(true);
    valueLabel_->setStyleSheet("color: #E2E7EE; font-size: 15px; font-weight: bold;");
    text->addWidget(label);
    text->addWidget(valueLabel_);
    layout->addLayout(text, 1);
}

void MetricCard::setValue(const QString &value) { valueLabel_->setText(value); }
void MetricCard::setUsage(double usage) { ring_->setUsage(usage); }
