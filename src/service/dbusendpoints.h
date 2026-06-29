#pragma once

#include <QDBusVariant>

#include "ecmemoryabstract.h"
#include "ecparametersabstract.h"
#include "ecprofilesabstract.h"

class DeviceProfileService;
class EcMemoryService;
class ParameterRegistry;

class EcParametersEndpoint : public EcParametersAbstract {
    Q_OBJECT
public:
    explicit EcParametersEndpoint(ParameterRegistry* registry, QObject* parent = nullptr);

    QDBusVariant availableParameters() const override;
    QDBusVariant readParameter(const QDBusVariant& name) const override;
    bool writeParameter(const QDBusVariant& name, const QDBusVariant& value) override;
    QDBusVariant writeParameters(const QDBusVariant& updates) override;
    QDBusVariant availableValues(const QDBusVariant& name) const override;

private:
    ParameterRegistry* mRegistry{nullptr};
};

class EcMemoryEndpoint : public EcMemoryAbstract {
    Q_OBJECT
public:
    explicit EcMemoryEndpoint(EcMemoryService* memoryService, QObject* parent = nullptr);

    QDBusVariant readEcMemory(const QDBusVariant& offset, const QDBusVariant& length) const override;
    QDBusVariant writeEcMemory(const QDBusVariant& offset, const QDBusVariant& bytes) override;
    QDBusVariant
    writeEcMemoryBits(const QDBusVariant& offset, const QDBusVariant& mask, const QDBusVariant& value) override;

private:
    EcMemoryService* mMemoryService{nullptr};
};

class EcProfilesEndpoint : public EcProfilesAbstract {
    Q_OBJECT
public:
    explicit EcProfilesEndpoint(DeviceProfileService* profileService, QObject* parent = nullptr);

    QDBusVariant supportedDeviceProfiles() const override;
    QDBusVariant activeDeviceProfile() const override;
    QDBusVariant saveSupportedDeviceProfile(const QDBusVariant& profile) override;
    QDBusVariant removeSupportedDeviceProfile(const QDBusVariant& profileId) override;

private:
    DeviceProfileService* mProfileService{nullptr};
};
