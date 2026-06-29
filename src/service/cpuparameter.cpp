#include "cpuparameter.h"

#include <QDebug>

#include "cpufiles.h"
#include "struct.h"

namespace {
static constexpr int cpuTelemetryIntervalMs = 1000;
static constexpr int controlRefreshIntervalTicks = 30;
} // namespace
CpuParameter::CpuParameter(const QVariant& name, QObject* parent, CpuFiles::CpuBackend* backend)
    : Parameter(name, QVariant(), false, parent), mBackend(backend ? backend : &CpuFiles::defaultBackend()) {
    mCpuDirs = mBackend->discoverCpuDirs();
    updateConfig();
    mTimer.start(cpuTelemetryIntervalMs);
    connect(&mTimer, &QTimer::timeout, this, &CpuParameter::updateConfig);
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
        cpu.currentFreq = mBackend->readCurrentFreq(cpuDir);

        if (refreshControls) {
            cpu = mBackend->readControl(cpuDir, &cpu);
        }
        cpuConfig.cpus.append(cpu);
    }
    mCpuCoreStatsPrev = mCpuCoreStatsCur;
    mCpuCoreStatsCur = mBackend->readCoreStats();
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
    bool success = mBackend->writeControls(mCpuDirs, cpuConfig, currentConfig);

    if (success) {
        mForceControlRefresh = true;
        updateConfig();
    }
    return success;
}
