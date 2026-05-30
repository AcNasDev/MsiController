#include "cpuparameter.h"

#include <QDebug>
#include <QFile>
#include <QRegularExpression>

#include "cpufiles.h"
#include "struct.h"

namespace {
static const char* statPath = "/proc/stat";
static constexpr int cpuTelemetryIntervalMs = 1000;
static constexpr int controlRefreshIntervalTicks = 30;
} // namespace
CpuParameter::CpuParameter(const QVariant& name, QObject* parent) : Parameter(name, QVariant(), false, parent) {
    mCpuDirs = CpuFiles::discoverCpuDirs();
    updateConfig();
    mTimer.start(cpuTelemetryIntervalMs);
    connect(&mTimer, &QTimer::timeout, this, &CpuParameter::updateConfig);
}

QVector<CpuParameter::CpuCoreStat> CpuParameter::readCoreStats() const {
    QVector<CpuCoreStat> stats;
    static const QRegularExpression rx("^cpu([0-9]+)\\s+");
    QFile file(statPath);
    QRegularExpressionMatch match;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = QString::fromUtf8(file.readAll());
        QStringList lines = content.split('\n', Qt::SkipEmptyParts);
        for (const QString& strLine : std::as_const(lines)) {
            match = rx.match(strLine.trimmed());
            if (match.hasMatch()) {
                QStringList parts = strLine.simplified().split(' ');
                if (parts.size() < 5)
                    continue;
                quint64 user = parts[1].toULongLong();
                quint64 nice = parts[2].toULongLong();
                quint64 system = parts[3].toULongLong();
                quint64 idle = parts[4].toULongLong();
                quint64 iowait = parts.value(5, "0").toULongLong();
                quint64 irq = parts.value(6, "0").toULongLong();
                quint64 softirq = parts.value(7, "0").toULongLong();
                quint64 steal = parts.value(8, "0").toULongLong();
                CpuCoreStat stat;
                stat.idle = idle + iowait;
                stat.total = user + nice + system + idle + iowait + irq + softirq + steal;
                stats.append(stat);
            }
        }
    } else {
        qWarning() << "Failed to open /proc/stat";
    }
    return stats;
}

void CpuParameter::updateConfig() {
    const Msi::CpuConfig previousConfig = mValue.value<Msi::CpuConfig>();
    const bool hasPreviousConfig = previousConfig.cpus.size() == mCpuDirs.size();
    const bool refreshControls =
        mForceControlRefresh || !hasPreviousConfig || mTicksSinceControlRefresh >= controlRefreshIntervalTicks;
    if (refreshControls) {
        mForceControlRefresh = false;
        mTicksSinceControlRefresh = 0;
    } else {
        ++mTicksSinceControlRefresh;
    }

    Msi::CpuConfig cpuConfig;
    for (int i = 0; i < mCpuDirs.size(); ++i) {
        const QString& cpuDir = mCpuDirs[i];
        Msi::Cpu cpu = hasPreviousConfig ? previousConfig.cpus[i] : Msi::Cpu{};
        cpu.currentFreq = CpuFiles::readCurrentFreq(cpuDir);

        if (refreshControls) {
            cpu = CpuFiles::readControl(cpuDir, &cpu);
        }
        cpuConfig.cpus.append(cpu);
    }
    mCpuCoreStatsPrev = mCpuCoreStatsCur;
    mCpuCoreStatsCur = readCoreStats();
    for (int i = 0; i < mCpuCoreStatsCur.size() && i < mCpuCoreStatsPrev.size(); ++i) {
        quint64 idleDiff = mCpuCoreStatsCur[i].idle - mCpuCoreStatsPrev[i].idle;
        quint64 totalDiff = mCpuCoreStatsCur[i].total - mCpuCoreStatsPrev[i].total;
        double usage = totalDiff ? 100.0 * (1.0 - static_cast<double>(idleDiff) / static_cast<double>(totalDiff)) : 0.0;
        if (i < cpuConfig.cpus.size()) {
            cpuConfig.cpus[i].usage = usage;
        }
    }
    QVariant newValue = QVariant::fromValue(cpuConfig);
    if (newValue != mValue) {
        mValue = newValue;
        publishValue(mValue);
    }
}

QVariant CpuParameter::readValue() const {
    return mValue;
}

bool CpuParameter::writeValue(const QVariant& value) {
    Msi::CpuConfig cpuConfig = value.value<Msi::CpuConfig>();
    Msi::CpuConfig currentConfig = mValue.value<Msi::CpuConfig>();
    bool success = CpuFiles::writeControls(mCpuDirs, cpuConfig, currentConfig);

    if (success) {
        mForceControlRefresh = true;
        updateConfig();
    }
    return success;
}
