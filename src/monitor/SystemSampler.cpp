#include "SystemSampler.h"

SystemSample SystemSampler::sample()
{
    const auto cpu = cpu_.sample();
    const auto ram = ram_.sample();
    const auto disk = disk_.sample();

    SystemSample result;
    result.cpu = cpu.usage;
    result.cpuCores = cpu.cores;
    result.cpuModel = cpu.model;
    result.memory = ram.usage;
    result.memoryUsed = ram.used;
    result.memoryTotal = ram.total;
    result.swap = ram.swapUsage;
    result.swapUsed = ram.swapUsed;
    result.swapTotal = ram.swapTotal;
    result.disk = disk.usage;
    result.diskUsed = disk.used;
    result.diskTotal = disk.total;
    result.diskPath = disk.path;
    return result;
}
