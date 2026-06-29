#include "esproxy.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QDebug>

#include "appsettingscontroller.h"
#include "autoprofilecontroller.h"
#include "behaviorprofilecontroller.h"
#include "dbusapi.h"
#include "deviceprofileclient.h"
#include "diagnosticsclient.h"
#include "ecmemoryclient.h"
#include "logging.h"
#include "parameterclient.h"
#include "proxyparameter.h"
#include "telemetryhistorycontroller.h"

EsProxy::EsProxy(QObject* parent) : QObject(parent) {
    registerMetaType();
    QDBusConnection connection{QDBusConnection::systemBus()};

    mParameters = new ParameterClient(connection, this);
    mProfiles = new DeviceProfileClient(connection, this);
    mMemory = new EcMemoryClient(connection, this);
    mDiagnostics = new DiagnosticsClient(connection, this);
    mAutoProfile = new AutoProfileController(mParameters, this);
    mBehaviorProfiles = new BehaviorProfileController(mParameters, mAutoProfile, this);
    mTelemetryHistory = new TelemetryHistoryController(mParameters, this);
    mAppSettings = new AppSettingsController(this);

    connect(mProfiles, &DeviceProfileClient::deviceProfilesChanged, this, &EsProxy::deviceProfilesChanged);
    connect(mProfiles, &DeviceProfileClient::activeDeviceProfileChanged, this, &EsProxy::activeDeviceProfileChanged);
    connect(mProfiles, &DeviceProfileClient::statusChanged, this, &EsProxy::deviceProfileStatusChanged);
    connect(mDiagnostics, &DiagnosticsClient::diagnosticsChanged, this, &EsProxy::diagnosticsChanged);
    connect(mDiagnostics, &DiagnosticsClient::statusChanged, this, &EsProxy::diagnosticsStatusChanged);
    connect(mAutoProfile, &AutoProfileController::enabledChanged, this, &EsProxy::autoProfileChanged);
    connect(mAutoProfile, &AutoProfileController::statusChanged, this, &EsProxy::autoProfileChanged);
    connect(mBehaviorProfiles,
            &BehaviorProfileController::activeProfileChanged,
            this,
            &EsProxy::behaviorProfilesChanged);
    connect(mBehaviorProfiles, &BehaviorProfileController::statusChanged, this, &EsProxy::behaviorProfilesChanged);
    connect(mTelemetryHistory, &TelemetryHistoryController::historyChanged, this, &EsProxy::telemetryHistoryChanged);
    connect(mAppSettings, &AppSettingsController::statusChanged, this, &EsProxy::appSettingsStatusChanged);

    QDBusServiceWatcher* serviceWatcher =
        new QDBusServiceWatcher(QString::fromLatin1(MsiDbusApi::serviceName),
                                connection,
                                QDBusServiceWatcher::WatchForUnregistration | QDBusServiceWatcher::WatchForRegistration,
                                this);

    connect(serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString& service) {
        if (service == QString::fromLatin1(MsiDbusApi::serviceName)) {
            setConnected(false);
            qCWarning(msiClientLog) << "EC service disconnected!";
        }
    });

    connect(serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, [this](const QString& service) {
        if (service == QString::fromLatin1(MsiDbusApi::serviceName)) {
            setConnected(true);
            qCInfo(msiClientLog) << "EC service reconnected!";
        }
    });

    setConnected(connection.interface()->isServiceRegistered(QString::fromLatin1(MsiDbusApi::serviceName)));
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

QVariantMap EsProxy::diagnostics() const {
    return mDiagnostics->diagnostics();
}

QString EsProxy::diagnosticsStatus() const {
    return mDiagnostics->status();
}

bool EsProxy::restartRequired() const {
    return mDiagnostics->restartRequired();
}

bool EsProxy::autoProfileEnabled() const {
    return mAutoProfile->isEnabled();
}

QString EsProxy::autoProfileStatus() const {
    return mAutoProfile->status();
}

QVariantList EsProxy::behaviorProfiles() const {
    return mBehaviorProfiles->profiles();
}

QString EsProxy::activeBehaviorProfile() const {
    return mBehaviorProfiles->activeProfile();
}

QString EsProxy::behaviorProfileStatus() const {
    return mBehaviorProfiles->status();
}

QVariantList EsProxy::telemetryHistory() const {
    return mTelemetryHistory->history();
}

QString EsProxy::appSettingsStatus() const {
    return mAppSettings->status();
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

QVariantMap EsProxy::importDeviceProfile(const QString& pathOrUrl) {
    return mProfiles->importDeviceProfile(pathOrUrl);
}

QVariantMap EsProxy::exportDeviceProfile(const QVariantMap& profile, const QString& pathOrUrl) {
    return mProfiles->exportDeviceProfile(profile, pathOrUrl);
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

void EsProxy::refreshDiagnostics() {
    mDiagnostics->refresh();
}

QVariantMap EsProxy::saveSupportBundle(const QString& pathOrUrl) {
    QVariantMap clientData;
    clientData.insert(QStringLiteral("telemetryHistory"), mTelemetryHistory->snapshot());
    clientData.insert(QStringLiteral("appSettings"), mAppSettings->snapshot());
    clientData.insert(QStringLiteral("behaviorProfile"), mBehaviorProfiles->activeProfile());
    return mDiagnostics->saveSupportBundle(pathOrUrl, clientData);
}

void EsProxy::setAutoProfileEnabled(bool enabled) {
    mAutoProfile->setEnabled(enabled);
}

void EsProxy::applyBehaviorProfile(const QString& id) {
    mBehaviorProfiles->applyProfile(id);
}

QVariantMap EsProxy::exportTelemetryHistory(const QString& pathOrUrl) const {
    return mTelemetryHistory->exportHistory(pathOrUrl);
}

QVariantMap EsProxy::exportAppSettings(const QString& pathOrUrl) {
    return mAppSettings->exportSettings(pathOrUrl);
}

QVariantMap EsProxy::importAppSettings(const QString& pathOrUrl) {
    return mAppSettings->importSettings(pathOrUrl);
}

void EsProxy::setConnected(bool connected) {
    if (mIsConnected == connected) {
        return;
    }

    mIsConnected = connected;
    mParameters->setConnected(mIsConnected);
    mProfiles->setConnected(mIsConnected);
    mMemory->setConnected(mIsConnected);
    mDiagnostics->setConnected(mIsConnected);
    emit connectionChanged(mIsConnected);
}
