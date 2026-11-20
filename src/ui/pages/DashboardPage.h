#pragma once

#include <QWidget>

class MetricCard;

class DashboardPage : public QWidget
{
public:
    explicit DashboardPage(QWidget *parent = nullptr);

    void updateCpu(double usage);
    void updateMemory(double usage);
    void updateDisk(double usage);

private:
    MetricCard *cpuCard_;
    MetricCard *memoryCard_;
    MetricCard *diskCard_;
};
