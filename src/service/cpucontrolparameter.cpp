#include "cpucontrolparameter.h"

#include "cpufiles.h"

namespace {
static constexpr int cpuControlRefreshIntervalMs = 5000;
static constexpr int cpuControlReadbackDelayMs = 1200;
} // namespace

CpuControlParameter::CpuControlParameter(const QVariant& name, QObject* parent, CpuFiles::CpuBackend* backend)
    : Parameter(name, QVariant(), false, parent), mBackend(backend ? backend : &CpuFiles::defaultBackend()),
      mCpuDirs(mBackend->discoverCpuDirs()) {
    updateConfig();
    mTimer.start(cpuControlRefreshIntervalMs);
    connect(&mTimer, &QTimer::timeout, this, &CpuControlParameter::updateConfig);
}

QVariant CpuControlParameter::readValue() const {
    return mValue;
}

bool CpuControlParameter::writeValue(const QVariant& value) {
    const Msi::CpuConfig desiredConfig = value.value<Msi::CpuConfig>();
    const Msi::CpuConfig currentConfig = mValue.value<Msi::CpuConfig>();
    const bool success = mBackend->writeControls(mCpuDirs, desiredConfig, currentConfig);
    if (success) {
        mValue = QVariant::fromValue(desiredConfig);
        publishValue(mValue);

        mTimer.stop();
        mTimer.start(cpuControlRefreshIntervalMs);
        QTimer::singleShot(cpuControlReadbackDelayMs, this, &CpuControlParameter::updateConfig);
    }
    return success;
}

void CpuControlParameter::updateConfig() {
    const Msi::CpuConfig previousConfig = mValue.value<Msi::CpuConfig>();
    const Msi::CpuConfig cpuConfig = mBackend->readControls(mCpuDirs, previousConfig);
    const QVariant newValue = QVariant::fromValue(cpuConfig);
    if (newValue != mValue) {
        mValue = newValue;
        publishValue(mValue);
    }
}
