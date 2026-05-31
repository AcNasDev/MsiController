#pragma once
#include <QDBusContext>
#include <QDBusVariant>
#include <QObject>

class EcServiceAbstract : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.msi.ec")
public:
    explicit EcServiceAbstract(QObject* parent = nullptr) : QObject(parent) {};
    ~EcServiceAbstract() override = default;

public slots:
    Q_SCRIPTABLE virtual QDBusVariant availableParameters() const = 0;
    Q_SCRIPTABLE virtual QDBusVariant readParameter(const QDBusVariant& name) const = 0;
    Q_SCRIPTABLE virtual bool writeParameter(const QDBusVariant& name, const QDBusVariant& value) = 0;
    Q_SCRIPTABLE virtual QDBusVariant writeParameters(const QDBusVariant& updates) = 0;
    Q_SCRIPTABLE virtual QDBusVariant availableValues(const QDBusVariant& name) const = 0;
    Q_SCRIPTABLE virtual QDBusVariant readEcMemory(const QDBusVariant& offset, const QDBusVariant& length) const = 0;
    Q_SCRIPTABLE virtual QDBusVariant writeEcMemory(const QDBusVariant& offset, const QDBusVariant& bytes) = 0;
    Q_SCRIPTABLE virtual QDBusVariant writeEcMemoryBits(const QDBusVariant& offset,
                                                        const QDBusVariant& mask,
                                                        const QDBusVariant& value) = 0;
    Q_SCRIPTABLE virtual QDBusVariant supportedDeviceProfiles() const = 0;
    Q_SCRIPTABLE virtual QDBusVariant activeDeviceProfile() const = 0;
    Q_SCRIPTABLE virtual QDBusVariant saveSupportedDeviceProfile(const QDBusVariant& profile) = 0;
    Q_SCRIPTABLE virtual QDBusVariant removeSupportedDeviceProfile(const QDBusVariant& profileId) = 0;

signals:
    Q_SCRIPTABLE void parameterChanged(const QDBusVariant& name, const QDBusVariant& value);
    Q_SCRIPTABLE void availableParametersChanged();
    Q_SCRIPTABLE void supportedDeviceProfilesChanged();
};
