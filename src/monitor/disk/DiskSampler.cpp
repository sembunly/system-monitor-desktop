#include "DiskSampler.h"

#include <QDir>
#include <QFile>
#include <QStorageInfo>
#include <algorithm>

#ifdef Q_OS_MACOS
#include <sys/mount.h>
#endif

DiskSample DiskSampler::sample() const
{
    DiskSample result;
#ifdef Q_OS_MACOS
    struct statfs volume{};
    const QByteArray home = QFile::encodeName(QDir::homePath());
    if (statfs(home.constData(), &volume) == 0 && volume.f_blocks > 0) {
        result.path = QString::fromLocal8Bit(volume.f_mntonname);
        result.total = quint64(volume.f_blocks) * volume.f_bsize;
        const quint64 free = quint64(volume.f_bfree) * volume.f_bsize;
        result.used = result.total - std::min(free, result.total);
        result.usage = 100.0 * double(result.used) / double(result.total);
    }
#else
    QStorageInfo storage(QDir::homePath());
    if (storage.isValid() && storage.isReady() && storage.bytesTotal() > 0) {
        result.path = storage.rootPath();
        result.total = storage.bytesTotal();
        result.used = storage.bytesTotal() - storage.bytesFree();
        result.usage = 100.0 * double(result.used) / double(result.total);
    }
#endif
    return result;
}
