#pragma once

#include <QString>
#include <cstdint>

struct SystemSample {
    double cpu = -1;
    double memory = -1;
    double disk = -1;
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
    std::uint64_t previousTotal_ = 0;
    std::uint64_t previousIdle_ = 0;
    bool hasPrevious_ = false;
};
