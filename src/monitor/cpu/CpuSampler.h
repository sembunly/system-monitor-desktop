#pragma once

#include <QList>
#include <QString>
#include <cstdint>

struct CpuSample {
    double usage = -1;
    QList<double> cores;
    QString model;
};

class CpuSampler {
public:
    CpuSample sample();

private:
    std::uint64_t previousTotal_ = 0;
    std::uint64_t previousIdle_ = 0;
    bool hasPrevious_ = false;
    QList<std::uint64_t> previousCoreTotals_;
    QList<std::uint64_t> previousCoreIdle_;
    QString cpuModel_;
};
