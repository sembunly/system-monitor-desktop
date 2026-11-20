#pragma once

#include <QString>

struct DiskSample {
    double usage = -1;
    quint64 used = 0;
    quint64 total = 0;
    QString path;
};

class DiskSampler {
public:
    DiskSample sample() const;
};
