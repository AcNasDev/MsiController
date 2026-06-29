#pragma once

#include <QDBusConnection>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QTimer>
#include <QVariant>

#include "proxyparameter.h"
#include "struct.h"

class ComMsiEcParametersInterface;

class ParameterClient : public QObject {
    Q_OBJECT
public:
    explicit ParameterClient(const QDBusConnection& connection, QObject* parent = nullptr);

    void setConnected(bool connected);
    ProxyParameter* getProxyParameter(const Msi::Parametr& name) const;
    void setCpuScalingMaxFrequencies(const QVariantList& frequenciesKhz);
    void setCpuGovernor(const QString& governor);
    void setGpuControlValue(const QString& deviceId, const QString& key, const QVariant& value);

private:
    struct PendingConfirmation {
        QVariant expectedValue;
        qint64 startedAtMs{0};
        int retryCount{0};
        bool refreshScheduled{false};
    };

    ComMsiEcParametersInterface* mParametersInterface{nullptr};
    bool mIsConnected{false};
    QMap<Msi::Parametr, ProxyParameter*> mProxyParameters;
    QMap<Msi::Parametr, QVariant> mPendingWrites;
    QMap<Msi::Parametr, QVariant> mInFlightWrites;
    QMap<Msi::Parametr, PendingConfirmation> mConfirmingWrites;
    QTimer mWriteFlushTimer;

    void init();
    ProxyParameter* cpuControlWriteParameter() const;
    void applyRemoteValue(Msi::Parametr param, const QVariant& value, bool markValid = false);
    void handleRemoteValue(Msi::Parametr param, const QVariant& value, bool markValid = false);
    void queueWrite(Msi::Parametr param, const QVariant& value);
    void flushPendingWrites();
    void refreshParameter(Msi::Parametr param);
    void beginConfirmation(Msi::Parametr param, const QVariant& expectedValue);
    void scheduleConfirmationRefresh(Msi::Parametr param);
    bool confirmationMatches(Msi::Parametr param, const QVariant& expectedValue, const QVariant& actualValue) const;
    bool confirmationExpired(const PendingConfirmation& confirmation) const;
    bool hasQueuedWrite(Msi::Parametr param) const;
    bool hasWriteInProgress(Msi::Parametr param) const;
    void clearRuntimeState(bool markInvalid);
};
