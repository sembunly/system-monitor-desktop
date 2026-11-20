#include "MetricCard.h"

#include <QLabel>
#include <QVBoxLayout>

MetricCard::MetricCard(
    const QString &title,
    const QString &value,
    QWidget *parent
) : QFrame(parent)
{
    setObjectName("metricCard");
    setMinimumHeight(150);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    auto *titleLabel = new QLabel(title.toUpper(), this);
    titleLabel->setObjectName("metricTitle");

    valueLabel_ = new QLabel(value, this);
    valueLabel_->setObjectName("metricValue");
    const QString accent = title.startsWith("CPU") ? "#38BDF8" :
        title.startsWith("Disk") ? "#34D399" : "#A78BFA";
    valueLabel_->setStyleSheet("color: " + accent + "; font-size: 34px; font-weight: bold;");

    layout->addWidget(titleLabel);
    layout->addWidget(valueLabel_);
    auto *caption = new QLabel(title.startsWith("Disk") ? "HOME VOLUME" : "SYSTEM UTILIZATION", this);
    caption->setObjectName("mutedText");
    layout->addWidget(caption);
    layout->addStretch();
}

void MetricCard::setValue(const QString &value)
{
    valueLabel_->setText(value);
}
