#include "SystemSampler.h"

#include <QDir>
#include <QFile>
#include <QStorageInfo>
#include <algorithm>

#ifdef Q_OS_MACOS
#include <mach/mach.h>
#include <sys/sysctl.h>
#include <sys/mount.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#endif

SystemSample SystemSampler::sample()
{
    SystemSample result;
    std::uint64_t total = 0;
    std::uint64_t idle = 0;
    bool cpuValid = false;

#ifdef Q_OS_MACOS
    const mach_port_t host = mach_host_self();
    host_cpu_load_info_data_t cpu{};
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    if (host_statistics(host, HOST_CPU_LOAD_INFO,
                        reinterpret_cast<host_info_t>(&cpu), &count) == KERN_SUCCESS) {
        for (auto ticks : cpu.cpu_ticks) total += ticks;
        idle = cpu.cpu_ticks[CPU_STATE_IDLE];
        cpuValid = true;
    }
    std::uint64_t physical = 0;
    size_t size = sizeof(physical);
    vm_statistics64_data_t vm{};
    count = HOST_VM_INFO64_COUNT;
    vm_size_t pageSize = 0;
    if (sysctlbyname("hw.memsize", &physical, &size, nullptr, 0) == 0 &&
        host_page_size(host, &pageSize) == KERN_SUCCESS &&
        host_statistics64(host, HOST_VM_INFO64,
                          reinterpret_cast<host_info64_t>(&vm), &count) == KERN_SUCCESS) {
        // Active + wired + compressed pages; excludes reclaimable inactive cache.
        result.memoryTotal = physical;
        result.memoryUsed = std::min<quint64>(physical,
            (quint64(vm.active_count) + vm.wire_count + vm.compressor_page_count) * pageSize);
    }
    mach_port_deallocate(mach_task_self(), host);
#elif defined(Q_OS_LINUX)
    QFile stat("/proc/stat");
    if (stat.open(QIODevice::ReadOnly)) {
        const auto fields = stat.readLine().simplified().split(' ');
        if (fields.size() >= 5 && fields[0] == "cpu") {
            // Guest counters are already included in user/nice counters.
            for (int i = 1; i < std::min<int>(fields.size(), 9); ++i)
                total += fields[i].toULongLong();
            idle = fields[4].toULongLong();
            if (fields.size() > 5) idle += fields[5].toULongLong();
            cpuValid = true;
        }
    }
    QFile mem("/proc/meminfo");
    if (mem.open(QIODevice::ReadOnly)) {
        quint64 available = 0;
        const auto lines = mem.readAll().split('\n');
        for (const auto &line : lines) {
            const auto fields = line.simplified().split(' ');
            if (fields.size() < 2) continue;
            if (fields[0] == "MemTotal:") result.memoryTotal = fields[1].toULongLong() * 1024;
            if (fields[0] == "MemAvailable:") available = fields[1].toULongLong() * 1024;
        }
        result.memoryUsed = result.memoryTotal - std::min(available, result.memoryTotal);
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
    MEMORYSTATUSEX mem{};
    mem.dwLength = sizeof(mem);
    if (GlobalMemoryStatusEx(&mem)) {
        result.memoryTotal = mem.ullTotalPhys;
        result.memoryUsed = mem.ullTotalPhys - mem.ullAvailPhys;
    }
#endif

    if (cpuValid) {
        if (hasPrevious_ && total > previousTotal_ && idle >= previousIdle_) {
            result.cpu = std::clamp(100.0 * (1.0 - double(idle - previousIdle_) /
                double(total - previousTotal_)), 0.0, 100.0);
        }
        previousTotal_ = total;
        previousIdle_ = idle;
        hasPrevious_ = true;
    }
    if (result.memoryTotal)
        result.memory = 100.0 * double(result.memoryUsed) / double(result.memoryTotal);

    // Monitor the volume containing the user's home, including macOS's Data volume.
    #ifdef Q_OS_MACOS
    struct statfs volume{};
    const QByteArray home = QFile::encodeName(QDir::homePath());
    if (statfs(home.constData(), &volume) == 0 && volume.f_blocks > 0) {
        result.diskPath = QString::fromLocal8Bit(volume.f_mntonname);
        result.diskTotal = quint64(volume.f_blocks) * volume.f_bsize;
        const quint64 free = quint64(volume.f_bfree) * volume.f_bsize;
        result.diskUsed = result.diskTotal - std::min(free, result.diskTotal);
        result.disk = 100.0 * double(result.diskUsed) / double(result.diskTotal);
    }
    #else
    QStorageInfo storage(QDir::homePath());
    if (storage.isValid() && storage.isReady() && storage.bytesTotal() > 0) {
        result.diskPath = storage.rootPath();
        result.diskTotal = storage.bytesTotal();
        result.diskUsed = storage.bytesTotal() - storage.bytesFree();
        result.disk = 100.0 * double(result.diskUsed) / double(result.diskTotal);
    }
    #endif
    return result;
}
