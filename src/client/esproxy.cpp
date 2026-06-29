#include "esproxy.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QDebug>

#include "deviceprofileclient.h"
#include "ecmemoryclient.h"
#include "parameterclient.h"
#include "proxyparameter.h"

namespace {
constexpr auto serviceName = "com.msi.ec";
} // namespace

EsProxy::EsProxy(QObject* parent) : QObject(parent) {
    registerMetaType();
    QDBusConnection connection{QDBusConnection::systemBus()};

    mParameters = new ParameterClient(connection, this);
    mProfiles = new DeviceProfileClient(connection, this);
    mMemory = new EcMemoryClient(connection, this);

    connect(mProfiles, &DeviceProfileClient::deviceProfilesChanged, this, &EsProxy::deviceProfilesChanged);
    connect(mProfiles, &DeviceProfileClient::activeDeviceProfileChanged, this, &EsProxy::activeDeviceProfileChanged);
    connect(mProfiles, &DeviceProfileClient::statusChanged, this, &EsProxy::deviceProfileStatusChanged);

    QDBusServiceWatcher* serviceWatcher =
        new QDBusServiceWatcher(QString::fromLatin1(serviceName),
                                connection,
                                QDBusServiceWatcher::WatchForUnregistration | QDBusServiceWatcher::WatchForRegistration,
                                this);

    connect(serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString& service) {
        if (service == QString::fromLatin1(serviceName)) {
            setConnected(false);
            qWarning() << "EC service disconnected!";
        }
    });

    connect(serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, [this](const QString& service) {
        if (service == QString::fromLatin1(serviceName)) {
            setConnected(true);
            qInfo() << "EC service reconnected!";
        }
    });

    setConnected(connection.interface()->isServiceRegistered(QString::fromLatin1(serviceName)));
}

bool EsProxy::isConnected() const {
    return mIsConnected;
}

QVariantList EsProxy::deviceProfiles() const {
    return mProfiles->deviceProfiles();
}

QVariantMap EsProxy::activeDeviceProfile() const {
    return mProfiles->activeDeviceProfile();
}

QString EsProxy::deviceProfileStatus() const {
    return mProfiles->status();
}

ProxyParameter* EsProxy::getProxyParameter(const Msi::Parametr& name) const {
    return mParameters->getProxyParameter(name);
}

void EsProxy::setCpuScalingMaxFrequencies(const QVariantList& frequenciesKhz) {
    mParameters->setCpuScalingMaxFrequencies(frequenciesKhz);
}

void EsProxy::setCpuGovernor(const QString& governor) {
    mParameters->setCpuGovernor(governor);
}

void EsProxy::setGpuControlValue(const QString& deviceId, const QString& key, const QVariant& value) {
    mParameters->setGpuControlValue(deviceId, key, value);
}

void EsProxy::refreshDeviceProfiles() {
    mProfiles->refreshDeviceProfiles();
}

void EsProxy::saveDeviceProfile(const QVariantMap& profile) {
    mProfiles->saveDeviceProfile(profile);
}

void EsProxy::removeDeviceProfile(const QString& profileId) {
    mProfiles->removeDeviceProfile(profileId);
}

QVariantMap EsProxy::readEcMemory(int offset, int length) const {
    return mMemory->readEcMemory(offset, length);
}

QVariantMap EsProxy::writeEcMemory(int offset, const QVariantList& bytes) {
    return mMemory->writeEcMemory(offset, bytes);
}

QVariantMap EsProxy::writeEcMemoryBits(int offset, int mask, int value) {
    return mMemory->writeEcMemoryBits(offset, mask, value);
}

void EsProxy::setConnected(bool connected) {
    if (mIsConnected == connected) {
        return;
    }

    mIsConnected = connected;
    mParameters->setConnected(mIsConnected);
    mProfiles->setConnected(mIsConnected);
    mMemory->setConnected(mIsConnected);
    emit connectionChanged(mIsConnected);
}
