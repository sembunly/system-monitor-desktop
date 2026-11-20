#include "DashboardPage.h"
#include "../widgets/MetricCard.h"
#include "../widgets/LiveChart.h"
#include "../widgets/CoreBarChart.h"

#include <QGridLayout>
#include <QLabel>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSysInfo>
#include <QThread>
#include <QVBoxLayout>

namespace {
QString percent(double value)
{
    return value < 0 ? "--" : QString::number(value, 'f', 1) + "%";
}
QString capacity(quint64 used, quint64 total)
{
    return QString("%1 / %2 GB").arg(double(used) / 1e9, 0, 'f', 2).arg(double(total) / 1e9, 0, 'f', 2);
}
}

DashboardPage::DashboardPage(QWidget *parent) : QWidget(parent)
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    auto *scroll = new QScrollArea(this);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(26, 26, 26, 18);
    layout->setSpacing(16);
    auto *header = new QHBoxLayout;
    auto *title = new QLabel("Dashboard", content);
    title->setObjectName("pageTitle");
    header->addWidget(title);
    systemInfo_ = new QLabel("Reading system information…", content);
    systemInfo_->setObjectName("mutedText");
    systemInfo_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    systemInfo_->setWordWrap(true);
    header->addWidget(systemInfo_, 1);
    layout->addLayout(header);

    cardsLayout_ = new QGridLayout;
    cardsLayout_->setSpacing(12);
    const QStringList titles{"CPU", "RAM", "SWAP", "DISK"};
    for (int i = 0; i < 4; ++i) {
        cards_[i] = new MetricCard(titles[i], "--", content);
        cardsLayout_->addWidget(cards_[i], 0, i);
        cardsLayout_->setColumnStretch(i, 1);
    }
    layout->addLayout(cardsLayout_);

    cpuChart_ = new LiveChart("CPU Usage", {"CPU"}, {QColor("#E47393")}, content);
    ramChart_ = new LiveChart("RAM Usage", {"RAM"}, {QColor("#29B8CD")}, content);
    swapChart_ = new LiveChart("SWAP Usage", {"Swap"}, {QColor("#4D9CCC")}, content);
    for (auto *chart : {cpuChart_, ramChart_, swapChart_}) {
        chart->setZoomControlsVisible(true);
        chart->setTimeWindow(0);
    }
    cpuChart_->setMinimumHeight(320);
    ramChart_->setMinimumHeight(260);
    swapChart_->setMinimumHeight(260);
    cores_ = new CoreBarChart(content);
    auto *top = new QHBoxLayout;
    top->setSpacing(12);
    top->addWidget(cpuChart_, 3);
    top->addWidget(cores_, 2);
    layout->addLayout(top, 3);
    auto *bottom = new QHBoxLayout;
    bottom->setSpacing(12);
    bottom->addWidget(ramChart_, 1);
    bottom->addWidget(swapChart_, 1);
    layout->addLayout(bottom, 2);
    summary_ = new QLabel(content);
    summary_->setObjectName("mutedText");
    summary_->setAlignment(Qt::AlignRight);
    summary_->setWordWrap(true);
    layout->addWidget(summary_);
    scroll->setWidget(content);
    outer->addWidget(scroll);
}

void DashboardPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    const int columns = width() >= 1000 ? 4 : 2;
    if (columns == cardColumns_) return;
    cardColumns_ = columns;
    for (int i = 0; i < 4; ++i) cardsLayout_->removeWidget(cards_[i]);
    for (int i = 0; i < 4; ++i) cardsLayout_->setColumnStretch(i, i < columns ? 1 : 0);
    for (int i = 0; i < 4; ++i) cardsLayout_->addWidget(cards_[i], i / columns, i % columns);
}

void DashboardPage::updateSample(const SystemSample &sample)
{
    cards_[0]->setValue(percent(sample.cpu));
    cards_[1]->setValue(sample.memory < 0 ? "--" : capacity(sample.memoryUsed, sample.memoryTotal));
    cards_[2]->setValue(sample.swap < 0 ? "--" : capacity(sample.swapUsed, sample.swapTotal));
    cards_[3]->setValue(sample.disk < 0 ? "--" : capacity(sample.diskUsed, sample.diskTotal));
    const double values[]{sample.cpu, sample.memory, sample.swap, sample.disk};
    for (int i = 0; i < 4; ++i) cards_[i]->setUsage(values[i]);
    cards_[2]->setToolTip(sample.swap < 0 ? "Swap metrics unavailable" :
        sample.swapTotal == 0 ? "No swap is currently allocated" : "Swap space in use / allocated swap capacity");
    cards_[3]->setToolTip(sample.diskPath);
    cpuChart_->appendSample({sample.cpu});
    ramChart_->setScale(double(sample.memoryTotal) / 1e9, "GB");
    ramChart_->appendSample({sample.memory < 0 ? -1 : double(sample.memoryUsed) / 1e9});
    swapChart_->setScale(sample.swapTotal ? double(sample.swapTotal) / 1e9 : 1, "GB");
    swapChart_->appendSample({sample.swap < 0 ? -1 : double(sample.swapUsed) / 1e9});
    cores_->setValues(sample.cpuCores);
    systemInfo_->setText(QString("CPU: %1  ·  Logical cores: %2\nOS: %3  ·  Kernel: %4")
        .arg(sample.cpuModel).arg(sample.cpuCores.isEmpty() ? QThread::idealThreadCount() : sample.cpuCores.size())
        .arg(QSysInfo::prettyProductName(), QSysInfo::kernelVersion()));
    summary_->setText(QString("RAM %1  ·  CPU %2  ·  SWAP %3  ·  DISK %4")
        .arg(percent(sample.memory), percent(sample.cpu), percent(sample.swap), percent(sample.disk)));
}
