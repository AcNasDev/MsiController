#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

#include "struct.h"

class DeviceProfileClient;
class EcMemoryClient;
class ParameterClient;
class ProxyParameter;

class EsProxy : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionChanged)
    Q_PROPERTY(QVariantList deviceProfiles READ deviceProfiles NOTIFY deviceProfilesChanged)
    Q_PROPERTY(QVariantMap activeDeviceProfile READ activeDeviceProfile NOTIFY activeDeviceProfileChanged)
    Q_PROPERTY(QString deviceProfileStatus READ deviceProfileStatus NOTIFY deviceProfileStatusChanged)
public:
    explicit EsProxy(QObject* parent = nullptr);
    ~EsProxy() override = default;

    Q_INVOKABLE bool isConnected() const;
    QVariantList deviceProfiles() const;
    QVariantMap activeDeviceProfile() const;
    QString deviceProfileStatus() const;
    Q_INVOKABLE ProxyParameter* getProxyParameter(const Msi::Parametr& name) const;
    Q_INVOKABLE void setCpuScalingMaxFrequencies(const QVariantList& frequenciesKhz);
    Q_INVOKABLE void setCpuGovernor(const QString& governor);
    Q_INVOKABLE void setGpuControlValue(const QString& deviceId, const QString& key, const QVariant& value);
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
    bool mIsConnected{false};
    ParameterClient* mParameters{nullptr};
    DeviceProfileClient* mProfiles{nullptr};
    EcMemoryClient* mMemory{nullptr};

    void setConnected(bool connected);
};
