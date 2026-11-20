#include "DashboardPage.h"
#include "../widgets/MetricCard.h"
#include "../widgets/LiveChart.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

DashboardPage::DashboardPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(24);

    auto *title = new QLabel("System overview", this);
    title->setObjectName("pageTitle");

    auto *subtitle = new QLabel(
        "Your system at a glance · live resource utilization",
        this
    );
    subtitle->setObjectName("mutedText");

    auto *cards = new QHBoxLayout();
    cards->setSpacing(16);

    cpuCard_ = new MetricCard("CPU Usage", "--", this);
    memoryCard_ = new MetricCard("RAM Usage", "--", this);
    diskCard_ = new MetricCard("Disk Usage", "--", this);

    cards->addWidget(cpuCard_);
    cards->addWidget(memoryCard_);
    cards->addWidget(diskCard_);

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addLayout(cards);
    chart_ = new LiveChart("Resource activity", {"CPU", "RAM", "Disk"},
        {QColor("#38BDF8"), QColor("#A78BFA"), QColor("#34D399")}, this);
    layout->addWidget(chart_, 1);
}

void DashboardPage::appendSample(double cpu, double memory, double disk)
{
    chart_->appendSample({cpu, memory, disk});
}

void DashboardPage::updateCpu(double usage)
{
    cpuCard_->setValue(usage < 0 ? "--" : QString::number(usage, 'f', 1) + "%");
}

void DashboardPage::updateMemory(double usage)
{
    memoryCard_->setValue(usage < 0 ? "--" : QString::number(usage, 'f', 1) + "%");
}

void DashboardPage::updateDisk(double usage)
{
    diskCard_->setValue(usage < 0 ? "--" : QString::number(usage, 'f', 1) + "%");
}
