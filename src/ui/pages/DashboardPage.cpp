#include "DashboardPage.h"
#include "../widgets/MetricCard.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

DashboardPage::DashboardPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(24);

    auto *title = new QLabel("Overview", this);
    title->setObjectName("pageTitle");

    auto *subtitle = new QLabel(
        "Real-time system performance",
        this
    );

    auto *cards = new QHBoxLayout();
    cards->setSpacing(16);

    cpuCard_ = new MetricCard("CPU Usage", "--", this);
    memoryCard_ = new MetricCard("Memory Usage", "--", this);
    diskCard_ = new MetricCard("Disk Usage", "--", this);

    cards->addWidget(cpuCard_);
    cards->addWidget(memoryCard_);
    cards->addWidget(diskCard_);

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addLayout(cards);
    layout->addStretch();
}

void DashboardPage::updateCpu(double usage)
{
    cpuCard_->setValue(QString::number(usage, 'f', 1) + "%");
}

void DashboardPage::updateMemory(double usage)
{
    memoryCard_->setValue(QString::number(usage, 'f', 1) + "%");
}

void DashboardPage::updateDisk(double usage)
{
    diskCard_->setValue(QString::number(usage, 'f', 1) + "%");
}
