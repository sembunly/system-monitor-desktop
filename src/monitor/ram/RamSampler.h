#pragma once

#include <QtGlobal>

struct RamSample {
    double usage = -1;
    quint64 used = 0;
    quint64 total = 0;
    double swapUsage = -1;
    quint64 swapUsed = 0;
    quint64 swapTotal = 0;
};

class RamSampler {
public:
    RamSample sample() const;
};
