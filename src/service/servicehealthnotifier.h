#pragma once

#include <QLibrary>
#include <QObject>
#include <QTimer>

class ServiceHealthNotifier : public QObject {
    Q_OBJECT
public:
    explicit ServiceHealthNotifier(QObject* parent = nullptr);

    void notifyReady();
    void startWatchdog();

private:
    using SdNotifyFn = int (*)(int, const char*);

    QTimer mWatchdogTimer;
    QLibrary mSystemdLibrary;
    SdNotifyFn mSdNotify{nullptr};

    bool notify(const char* state);
    int watchdogIntervalMs() const;
};
