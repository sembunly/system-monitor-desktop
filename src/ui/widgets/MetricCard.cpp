#include "MetricCard.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QFont>

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

    auto *titleLabel = new QLabel(title, this);
    titleLabel->setObjectName("metricTitle");

    valueLabel_ = new QLabel(value, this);
    valueLabel_->setObjectName("metricValue");

    layout->addWidget(titleLabel);
    layout->addWidget(valueLabel_);
    layout->addStretch();
}

void MetricCard::setValue(const QString &value)
{
    valueLabel_->setText(value);
}
