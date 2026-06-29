#include "servicehealthnotifier.h"

#include <QtGlobal>

namespace {
constexpr int fallbackWatchdogIntervalMs = 15000;
}

ServiceHealthNotifier::ServiceHealthNotifier(QObject* parent)
    : QObject(parent), mSystemdLibrary(QStringLiteral("libsystemd.so.0")) {
    if (mSystemdLibrary.load()) {
        mSdNotify = reinterpret_cast<SdNotifyFn>(mSystemdLibrary.resolve("sd_notify"));
    }

    connect(&mWatchdogTimer, &QTimer::timeout, this, [this]() { notify("WATCHDOG=1"); });
}

void ServiceHealthNotifier::notifyReady() {
    notify("READY=1\nSTATUS=MSI EC service is running");
}

void ServiceHealthNotifier::startWatchdog() {
    if (!mSdNotify || qEnvironmentVariableIsEmpty("WATCHDOG_USEC")) {
        return;
    }

    mWatchdogTimer.setInterval(watchdogIntervalMs());
    mWatchdogTimer.start();
    notify("WATCHDOG=1");
}

bool ServiceHealthNotifier::notify(const char* state) {
    return mSdNotify && mSdNotify(0, state) >= 0;
}

int ServiceHealthNotifier::watchdogIntervalMs() const {
    bool ok = false;
    const qulonglong watchdogUsec = qEnvironmentVariable("WATCHDOG_USEC").toULongLong(&ok);
    if (!ok || watchdogUsec == 0) {
        return fallbackWatchdogIntervalMs;
    }
    const qulonglong halfMs = qMax<qulonglong>(1000, watchdogUsec / 2000);
    return static_cast<int>(qMin<qulonglong>(halfMs, fallbackWatchdogIntervalMs));
}
