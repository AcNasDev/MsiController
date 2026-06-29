#pragma once

#include <QDBusVariant>
#include <QObject>

class EcProfilesAbstract : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.msi.ec.Profiles")
public:
    explicit EcProfilesAbstract(QObject* parent = nullptr) : QObject(parent) {}
    ~EcProfilesAbstract() override = default;

public slots:
    Q_SCRIPTABLE virtual QDBusVariant supportedDeviceProfiles() const = 0;
    Q_SCRIPTABLE virtual QDBusVariant activeDeviceProfile() const = 0;
    Q_SCRIPTABLE virtual QDBusVariant saveSupportedDeviceProfile(const QDBusVariant& profile) = 0;
    Q_SCRIPTABLE virtual QDBusVariant removeSupportedDeviceProfile(const QDBusVariant& profileId) = 0;

signals:
    Q_SCRIPTABLE void supportedDeviceProfilesChanged();
    Q_SCRIPTABLE void activeDeviceProfileChanged();
};
