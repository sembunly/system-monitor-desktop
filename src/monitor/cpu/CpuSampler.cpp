#include "CpuSampler.h"

#include <QFile>
#include <QSysInfo>
#include <algorithm>

#ifdef Q_OS_MACOS
#include <mach/mach.h>
#include <mach/processor_info.h>
#include <sys/sysctl.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#endif

CpuSample CpuSampler::sample()
{
    CpuSample result;
    std::uint64_t total = 0;
    std::uint64_t idle = 0;
    bool cpuValid = false;
    if (cpuModel_.isEmpty()) cpuModel_ = QSysInfo::currentCpuArchitecture();
    result.model = cpuModel_;

#ifdef Q_OS_MACOS
    const mach_port_t host = mach_host_self();
    char brand[256]{};
    size_t brandSize = sizeof(brand);
    if (sysctlbyname("machdep.cpu.brand_string", brand, &brandSize, nullptr, 0) == 0) {
        cpuModel_ = QString::fromUtf8(brand).trimmed();
        result.model = cpuModel_;
    }
    processor_info_array_t cores = nullptr;
    mach_msg_type_number_t coreInfoCount = 0;
    natural_t coreCount = 0;
    if (host_processor_info(host, PROCESSOR_CPU_LOAD_INFO, &coreCount, &cores, &coreInfoCount) == KERN_SUCCESS) {
        QList<std::uint64_t> totals, idleTicks;
        for (natural_t core = 0; core < coreCount; ++core) {
            const auto offset = core * CPU_STATE_MAX;
            std::uint64_t coreTotal = 0;
            for (int state = 0; state < CPU_STATE_MAX; ++state)
                coreTotal += static_cast<unsigned int>(cores[offset + state]);
            const std::uint64_t coreIdle = static_cast<unsigned int>(cores[offset + CPU_STATE_IDLE]);
            double usage = -1;
            if (previousCoreTotals_.size() == coreCount && coreTotal > previousCoreTotals_[core] &&
                coreIdle >= previousCoreIdle_[core]) {
                usage = std::clamp(100.0 * (1.0 - double(coreIdle - previousCoreIdle_[core]) /
                    double(coreTotal - previousCoreTotals_[core])), 0.0, 100.0);
            }
            result.cores.append(usage);
            totals.append(coreTotal);
            idleTicks.append(coreIdle);
        }
        previousCoreTotals_ = totals;
        previousCoreIdle_ = idleTicks;
        vm_deallocate(mach_task_self(), reinterpret_cast<vm_address_t>(cores),
                      coreInfoCount * sizeof(integer_t));
    }
    host_cpu_load_info_data_t cpu{};
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    if (host_statistics(host, HOST_CPU_LOAD_INFO,
                        reinterpret_cast<host_info_t>(&cpu), &count) == KERN_SUCCESS) {
        for (auto ticks : cpu.cpu_ticks) total += ticks;
        idle = cpu.cpu_ticks[CPU_STATE_IDLE];
        cpuValid = true;
    }
    mach_port_deallocate(mach_task_self(), host);
#elif defined(Q_OS_LINUX)
    QFile stat("/proc/stat");
    if (stat.open(QIODevice::ReadOnly)) {
        const auto fields = stat.readLine().simplified().split(' ');
        if (fields.size() >= 5 && fields[0] == "cpu") {
            for (int i = 1; i < std::min<int>(fields.size(), 9); ++i)
                total += fields[i].toULongLong();
            idle = fields[4].toULongLong();
            if (fields.size() > 5) idle += fields[5].toULongLong();
            cpuValid = true;
        }
    }
#elif defined(Q_OS_WIN)
    FILETIME idleTime{}, kernelTime{}, userTime{};
    auto ticks = [](FILETIME time) {
        return (std::uint64_t(time.dwHighDateTime) << 32) | time.dwLowDateTime;
    };
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        total = ticks(kernelTime) + ticks(userTime);
        idle = ticks(idleTime);
        cpuValid = true;
    }
#endif

    if (cpuValid) {
        if (hasPrevious_ && total > previousTotal_ && idle >= previousIdle_) {
            result.usage = std::clamp(100.0 * (1.0 - double(idle - previousIdle_) /
                double(total - previousTotal_)), 0.0, 100.0);
        }
        previousTotal_ = total;
        previousIdle_ = idle;
        hasPrevious_ = true;
    }
    return result;
}
