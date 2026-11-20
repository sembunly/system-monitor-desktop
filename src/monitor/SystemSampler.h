#pragma once

#include <QString>
#include <QList>
#include "cpu/CpuSampler.h"
#include "ram/RamSampler.h"
#include "disk/DiskSampler.h"

struct SystemSample {
    double cpu = -1;
    double memory = -1;
    double disk = -1;
    double swap = -1;
    quint64 swapUsed = 0;
    quint64 swapTotal = 0;
    QList<double> cpuCores;
    QString cpuModel;
    quint64 memoryUsed = 0;
    quint64 memoryTotal = 0;
    quint64 diskUsed = 0;
    quint64 diskTotal = 0;
    QString diskPath;
};

class SystemSampler {
public:
    SystemSample sample();

private:
    CpuSampler cpu_;
    RamSampler ram_;
    DiskSampler disk_;
};
