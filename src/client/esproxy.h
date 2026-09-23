#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

#include "proxyparameter.h"
#include "struct.h"

class AutoProfileController;
class AppSettingsController;
class BehaviorProfileController;
class DeviceProfileClient;
class DiagnosticsClient;
class EcMemoryClient;
class ParameterClient;
class TelemetryHistoryController;

class EsProxy : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionChanged)
    Q_PROPERTY(QVariantList deviceProfiles READ deviceProfiles NOTIFY deviceProfilesChanged)
    Q_PROPERTY(QVariantMap activeDeviceProfile READ activeDeviceProfile NOTIFY activeDeviceProfileChanged)
    Q_PROPERTY(QString deviceProfileStatus READ deviceProfileStatus NOTIFY deviceProfileStatusChanged)
    Q_PROPERTY(QVariantMap diagnostics READ diagnostics NOTIFY diagnosticsChanged)
    Q_PROPERTY(QString diagnosticsStatus READ diagnosticsStatus NOTIFY diagnosticsStatusChanged)
    Q_PROPERTY(bool restartRequired READ restartRequired NOTIFY diagnosticsChanged)
    Q_PROPERTY(bool autoProfileEnabled READ autoProfileEnabled WRITE setAutoProfileEnabled NOTIFY autoProfileChanged)
    Q_PROPERTY(QString autoProfileStatus READ autoProfileStatus NOTIFY autoProfileChanged)
    Q_PROPERTY(QVariantList behaviorProfiles READ behaviorProfiles NOTIFY behaviorProfilesChanged)
    Q_PROPERTY(QString activeBehaviorProfile READ activeBehaviorProfile NOTIFY behaviorProfilesChanged)
    Q_PROPERTY(QString behaviorProfileStatus READ behaviorProfileStatus NOTIFY behaviorProfilesChanged)
    Q_PROPERTY(QVariantList telemetryHistory READ telemetryHistory NOTIFY telemetryHistoryChanged)
    Q_PROPERTY(QString appSettingsStatus READ appSettingsStatus NOTIFY appSettingsStatusChanged)
public:
    explicit EsProxy(QObject* parent = nullptr);
    ~EsProxy() override = default;

    Q_INVOKABLE bool isConnected() const;
    QVariantList deviceProfiles() const;
    QVariantMap activeDeviceProfile() const;
    QString deviceProfileStatus() const;
    QVariantMap diagnostics() const;
    QString diagnosticsStatus() const;
    bool restartRequired() const;
    bool autoProfileEnabled() const;
    QString autoProfileStatus() const;
    QVariantList behaviorProfiles() const;
    QString activeBehaviorProfile() const;
    QString behaviorProfileStatus() const;
    QVariantList telemetryHistory() const;
    QString appSettingsStatus() const;
    Q_INVOKABLE ProxyParameter* getProxyParameter(const Msi::Parametr& name) const;
    Q_INVOKABLE void setCpuScalingMaxFrequencies(const QVariantList& frequenciesKhz);
    Q_INVOKABLE void setCpuGovernor(const QString& governor);
    Q_INVOKABLE void setGpuControlValue(const QString& deviceId, const QString& key, const QVariant& value);
    Q_INVOKABLE void refreshDeviceProfiles();
    Q_INVOKABLE void saveDeviceProfile(const QVariantMap& profile);
    Q_INVOKABLE void removeDeviceProfile(const QString& profileId);
    Q_INVOKABLE QVariantMap importDeviceProfile(const QString& pathOrUrl);
    Q_INVOKABLE QVariantMap exportDeviceProfile(const QVariantMap& profile, const QString& pathOrUrl);
    Q_INVOKABLE QVariantMap readEcMemory(int offset, int length) const;
    Q_INVOKABLE QVariantMap writeEcMemory(int offset, const QVariantList& bytes);
    Q_INVOKABLE QVariantMap writeEcMemoryBits(int offset, int mask, int value);
    Q_INVOKABLE void refreshDiagnostics();
    Q_INVOKABLE QVariantMap saveSupportBundle(const QString& pathOrUrl);
    Q_INVOKABLE void setAutoProfileEnabled(bool enabled);
    Q_INVOKABLE void applyBehaviorProfile(const QString& id);
    Q_INVOKABLE QVariantMap exportTelemetryHistory(const QString& pathOrUrl) const;
    Q_INVOKABLE QVariantMap exportAppSettings(const QString& pathOrUrl);
    Q_INVOKABLE QVariantMap importAppSettings(const QString& pathOrUrl);

signals:
    void connectionChanged(bool isConnected);
    void deviceProfilesChanged();
    void activeDeviceProfileChanged();
    void deviceProfileStatusChanged();
    void diagnosticsChanged();
    void diagnosticsStatusChanged();
    void autoProfileChanged();
    void behaviorProfilesChanged();
    void telemetryHistoryChanged();
    void appSettingsStatusChanged();

private:
    bool mIsConnected{false};
    ParameterClient* mParameters{nullptr};
    DeviceProfileClient* mProfiles{nullptr};
    EcMemoryClient* mMemory{nullptr};
    DiagnosticsClient* mDiagnostics{nullptr};
    AutoProfileController* mAutoProfile{nullptr};
    BehaviorProfileController* mBehaviorProfiles{nullptr};
    TelemetryHistoryController* mTelemetryHistory{nullptr};
    AppSettingsController* mAppSettings{nullptr};

    void setConnected(bool connected);
};
