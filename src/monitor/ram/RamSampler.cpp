#include "RamSampler.h"

#include <QFile>
#include <algorithm>

#ifdef Q_OS_MACOS
#include <mach/mach.h>
#include <sys/sysctl.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#endif

RamSample RamSampler::sample() const
{
    RamSample result;
#ifdef Q_OS_MACOS
    const mach_port_t host = mach_host_self();
    struct xsw_usage swap{};
    size_t swapSize = sizeof(swap);
    if (sysctlbyname("vm.swapusage", &swap, &swapSize, nullptr, 0) == 0) {
        result.swapTotal = swap.xsu_total;
        result.swapUsed = std::min(swap.xsu_used, swap.xsu_total);
        result.swapUsage = result.swapTotal ? 100.0 * double(result.swapUsed) / double(result.swapTotal) : 0;
    }
    std::uint64_t physical = 0;
    size_t size = sizeof(physical);
    vm_statistics64_data_t vm{};
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    vm_size_t pageSize = 0;
    if (sysctlbyname("hw.memsize", &physical, &size, nullptr, 0) == 0 &&
        host_page_size(host, &pageSize) == KERN_SUCCESS &&
        host_statistics64(host, HOST_VM_INFO64,
                          reinterpret_cast<host_info64_t>(&vm), &count) == KERN_SUCCESS) {
        result.total = physical;
        result.used = std::min<quint64>(physical,
            (quint64(vm.active_count) + vm.wire_count + vm.compressor_page_count) * pageSize);
    }
    mach_port_deallocate(mach_task_self(), host);
#elif defined(Q_OS_LINUX)
    QFile mem("/proc/meminfo");
    if (mem.open(QIODevice::ReadOnly)) {
        quint64 available = 0;
        const auto lines = mem.readAll().split('\n');
        for (const auto &line : lines) {
            const auto fields = line.simplified().split(' ');
            if (fields.size() < 2) continue;
            if (fields[0] == "MemTotal:") result.total = fields[1].toULongLong() * 1024;
            if (fields[0] == "MemAvailable:") available = fields[1].toULongLong() * 1024;
            if (fields[0] == "SwapTotal:") result.swapTotal = fields[1].toULongLong() * 1024;
            if (fields[0] == "SwapFree:") result.swapUsed = fields[1].toULongLong() * 1024;
        }
        result.used = result.total - std::min(available, result.total);
        result.swapUsed = result.swapTotal - std::min(result.swapUsed, result.swapTotal);
        result.swapUsage = result.swapTotal ? 100.0 * double(result.swapUsed) / double(result.swapTotal) : 0;
    }
#elif defined(Q_OS_WIN)
    MEMORYSTATUSEX mem{};
    mem.dwLength = sizeof(mem);
    if (GlobalMemoryStatusEx(&mem)) {
        result.total = mem.ullTotalPhys;
        result.used = mem.ullTotalPhys - mem.ullAvailPhys;
    }
#endif
    if (result.total)
        result.usage = 100.0 * double(result.used) / double(result.total);
    return result;
}
