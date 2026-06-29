#include "dbusendpoints.h"

#include "deviceprofileservice.h"
#include "ecmemoryservice.h"
#include "parameterregistry.h"
#include "struct.h"

namespace {
QVariant unwrapDbusMsg(const QDBusVariant& value) {
    return qdbus_cast<Msi::Msg>(value.variant()).variant;
}

QDBusVariant wrapDbusMsg(const QVariant& value) {
    return QDBusVariant(QVariant::fromValue(Msi::Msg(value)));
}

bool extractInt(const QDBusVariant& value, int* out, const QString& name, QString* errorMessage) {
    bool ok = false;
    const int parsed = unwrapDbusMsg(value).toInt(&ok);
    if (!ok) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Invalid %1").arg(name);
        }
        return false;
    }
    *out = parsed;
    return true;
}

bool variantListToBytes(const QVariant& value, QByteArray* bytes, QString* errorMessage) {
    const QVariantList values = value.toList();
    bytes->clear();
    bytes->reserve(values.size());
    for (const QVariant& item : values) {
        bool ok = false;
        const int byteValue = item.toInt(&ok);
        if (!ok || byteValue < 0 || byteValue > 0xff) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Byte values must be in range 0x00..0xff");
            }
            return false;
        }
        bytes->append(static_cast<char>(byteValue));
    }
    return true;
}

QVariantMap memoryInputError(const QString& errorMessage) {
    QVariantMap result;
    result.insert(QStringLiteral("ok"), false);
    result.insert(QStringLiteral("error"), errorMessage);
    result.insert(QStringLiteral("size"), 0);
    return result;
}
} // namespace

EcParametersEndpoint::EcParametersEndpoint(ParameterRegistry* registry, QObject* parent)
    : EcParametersAbstract(parent), mRegistry(registry) {
    if (mRegistry) {
        connect(mRegistry,
                &ParameterRegistry::parameterChanged,
                this,
                [this](const QVariant& name, const QVariant& value) {
                    emit parameterChanged(wrapDbusMsg(name), wrapDbusMsg(value));
                });
        connect(mRegistry,
                &ParameterRegistry::availableParametersChanged,
                this,
                &EcParametersEndpoint::availableParametersChanged);
    }
}

QDBusVariant EcParametersEndpoint::availableParameters() const {
    return wrapDbusMsg(mRegistry ? mRegistry->availableParameterNames() : QVariantList{});
}

QDBusVariant EcParametersEndpoint::readParameter(const QDBusVariant& name) const {
    return wrapDbusMsg(mRegistry ? mRegistry->readParameter(unwrapDbusMsg(name)) : QVariant{});
}

bool EcParametersEndpoint::writeParameter(const QDBusVariant& name, const QDBusVariant& value) {
    return mRegistry ? mRegistry->writeParameter(unwrapDbusMsg(name), unwrapDbusMsg(value)) : false;
}

QDBusVariant EcParametersEndpoint::writeParameters(const QDBusVariant& updates) {
    return wrapDbusMsg(mRegistry ? mRegistry->writeParameters(unwrapDbusMsg(updates).toList()) : QVariantList{});
}

QDBusVariant EcParametersEndpoint::availableValues(const QDBusVariant& name) const {
    return wrapDbusMsg(mRegistry ? mRegistry->availableValues(unwrapDbusMsg(name)) : QVariant{});
}

EcMemoryEndpoint::EcMemoryEndpoint(EcMemoryService* memoryService, QObject* parent)
    : EcMemoryAbstract(parent), mMemoryService(memoryService) {}

QDBusVariant EcMemoryEndpoint::readEcMemory(const QDBusVariant& offset, const QDBusVariant& length) const {
    QString errorMessage;
    int offsetValue = 0;
    int lengthValue = 0;
    if (!extractInt(offset, &offsetValue, QStringLiteral("offset"), &errorMessage) ||
        !extractInt(length, &lengthValue, QStringLiteral("length"), &errorMessage)) {
        return wrapDbusMsg(memoryInputError(errorMessage));
    }
    return wrapDbusMsg(mMemoryService ? mMemoryService->readMemory(offsetValue, lengthValue)
                                      : memoryInputError(QStringLiteral("EC memory service is not available")));
}

QDBusVariant EcMemoryEndpoint::writeEcMemory(const QDBusVariant& offset, const QDBusVariant& bytes) {
    QString errorMessage;
    int offsetValue = 0;
    QByteArray rawBytes;
    if (!extractInt(offset, &offsetValue, QStringLiteral("offset"), &errorMessage) ||
        !variantListToBytes(unwrapDbusMsg(bytes), &rawBytes, &errorMessage)) {
        return wrapDbusMsg(memoryInputError(errorMessage));
    }
    return wrapDbusMsg(mMemoryService ? mMemoryService->writeMemory(offsetValue, rawBytes)
                                      : memoryInputError(QStringLiteral("EC memory service is not available")));
}

QDBusVariant
EcMemoryEndpoint::writeEcMemoryBits(const QDBusVariant& offset, const QDBusVariant& mask, const QDBusVariant& value) {
    QString errorMessage;
    int offsetValue = 0;
    int maskValue = 0;
    int valueValue = 0;
    if (!extractInt(offset, &offsetValue, QStringLiteral("offset"), &errorMessage) ||
        !extractInt(mask, &maskValue, QStringLiteral("mask"), &errorMessage) ||
        !extractInt(value, &valueValue, QStringLiteral("value"), &errorMessage)) {
        return wrapDbusMsg(memoryInputError(errorMessage));
    }
    return wrapDbusMsg(mMemoryService ? mMemoryService->writeMemoryBits(offsetValue, maskValue, valueValue)
                                      : memoryInputError(QStringLiteral("EC memory service is not available")));
}

EcProfilesEndpoint::EcProfilesEndpoint(DeviceProfileService* profileService, QObject* parent)
    : EcProfilesAbstract(parent), mProfileService(profileService) {
    if (mProfileService) {
        connect(mProfileService,
                &DeviceProfileService::profilesChanged,
                this,
                &EcProfilesEndpoint::supportedDeviceProfilesChanged);
        connect(mProfileService,
                &DeviceProfileService::activeProfileChanged,
                this,
                &EcProfilesEndpoint::activeDeviceProfileChanged);
    }
}

QDBusVariant EcProfilesEndpoint::supportedDeviceProfiles() const {
    return wrapDbusMsg(mProfileService ? mProfileService->supportedProfiles() : QVariantList{});
}

QDBusVariant EcProfilesEndpoint::activeDeviceProfile() const {
    return wrapDbusMsg(mProfileService ? mProfileService->activeProfile() : QVariantMap{});
}

QDBusVariant EcProfilesEndpoint::saveSupportedDeviceProfile(const QDBusVariant& profile) {
    return wrapDbusMsg(mProfileService ? mProfileService->saveProfile(unwrapDbusMsg(profile).toMap())
                                       : QVariantMap{{QStringLiteral("ok"), false},
                                                     {QStringLiteral("error"),
                                                      QStringLiteral("Device profile service is not available")}});
}

QDBusVariant EcProfilesEndpoint::removeSupportedDeviceProfile(const QDBusVariant& profileId) {
    return wrapDbusMsg(mProfileService ? mProfileService->removeProfile(unwrapDbusMsg(profileId).toString())
                                       : QVariantMap{{QStringLiteral("ok"), false},
                                                     {QStringLiteral("error"),
                                                      QStringLiteral("Device profile service is not available")}});
}
