#pragma once

#include <QWidget>

class MetricCard;
class LiveChart;

class DashboardPage : public QWidget
{
public:
    explicit DashboardPage(QWidget *parent = nullptr);

    void updateCpu(double usage);
    void updateMemory(double usage);
    void updateDisk(double usage);
    void appendSample(double cpu, double memory, double disk);

private:
    MetricCard *cpuCard_;
    MetricCard *memoryCard_;
    MetricCard *diskCard_;
    LiveChart *chart_;
};
