#pragma once

#include <QAbstractListModel>
#include <QDBusPendingReply>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QStringList>
#include <QTimer>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

#include "proxyparameter.h"
#include "struct.h"

class ComMsiEcInterface;
class EsProxy : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionChanged)
    Q_PROPERTY(QVariantList deviceProfiles READ deviceProfiles NOTIFY deviceProfilesChanged)
    Q_PROPERTY(QVariantMap activeDeviceProfile READ activeDeviceProfile NOTIFY activeDeviceProfileChanged)
    Q_PROPERTY(QString deviceProfileStatus READ deviceProfileStatus NOTIFY deviceProfileStatusChanged)
public:
    explicit EsProxy(QObject* parent = nullptr);
    virtual ~EsProxy() = default;

    Q_INVOKABLE bool isConnected() const;
    QVariantList deviceProfiles() const;
    QVariantMap activeDeviceProfile() const;
    QString deviceProfileStatus() const;
    Q_INVOKABLE ProxyParameter* getProxyParameter(const Msi::Parametr& name) const;
    Q_INVOKABLE void setCpuScalingMaxFrequencies(const QVariantList& frequenciesKhz);
    Q_INVOKABLE void setCpuGovernor(const QString& governor);
    Q_INVOKABLE void refreshDeviceProfiles();
    Q_INVOKABLE void saveDeviceProfile(const QVariantMap& profile);
    Q_INVOKABLE void removeDeviceProfile(const QString& profileId);
    Q_INVOKABLE QVariantMap readEcMemory(int offset, int length) const;
    Q_INVOKABLE QVariantMap writeEcMemory(int offset, const QVariantList& bytes);
    Q_INVOKABLE QVariantMap writeEcMemoryBits(int offset, int mask, int value);

signals:
    void connectionChanged(bool isConnected);
    void deviceProfilesChanged();
    void activeDeviceProfileChanged();
    void deviceProfileStatusChanged();

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
    QVariantList mDeviceProfiles;
    QVariantMap mActiveDeviceProfile;
    QString mDeviceProfileStatus;

    void init();
    QVariantMap ecMemoryReplyToMap(QDBusPendingReply<QDBusVariant>& reply) const;
    void refreshActiveDeviceProfile();
    ProxyParameter* cpuControlWriteParameter() const;
    void setDeviceProfileStatus(const QString& status);
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
