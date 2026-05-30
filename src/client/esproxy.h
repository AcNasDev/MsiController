#pragma once

#include <QAbstractListModel>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariant>

#include "proxyparameter.h"
#include "struct.h"

class ComMsiEcInterface;
class EsProxy : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionChanged)
public:
    explicit EsProxy(QObject* parent = nullptr);
    virtual ~EsProxy() = default;

    Q_INVOKABLE bool isConnected() const;
    Q_INVOKABLE ProxyParameter* getProxyParameter(const Msi::Parametr& name) const;
    Q_INVOKABLE void setCpuScalingMaxFrequencies(const QVariantList& frequenciesKhz);
    Q_INVOKABLE void setCpuGovernor(const QString& governor);

signals:
    void connectionChanged(bool isConnected);

private:
    struct PendingConfirmation {
        QVariant expectedValue;
        qint64 startedAtMs{0};
        int retryCount{0};
        bool refreshScheduled{false};
    };

    ComMsiEcInterface* mEcInterface{nullptr};
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
};
