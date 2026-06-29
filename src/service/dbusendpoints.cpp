#include "dbusendpoints.h"

#include "dbusapi.h"
#include "dbuscodec.h"
#include "deviceprofileservice.h"
#include "diagnosticsservice.h"
#include "ecmemoryservice.h"
#include "parameterregistry.h"

namespace {
bool extractInt(const QDBusVariant& value, int* out, const QString& name, QString* errorMessage) {
    bool ok = false;
    const int parsed = MsiDbusCodec::unwrap(value).toInt(&ok);
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
                    emit parameterChanged(MsiDbusCodec::wrap(name), MsiDbusCodec::wrap(value));
                });
        connect(mRegistry,
                &ParameterRegistry::availableParametersChanged,
                this,
                &EcParametersEndpoint::availableParametersChanged);
    }
}

int EcParametersEndpoint::apiVersion() const {
    return MsiDbusApi::currentApiVersion;
}

QDBusVariant EcParametersEndpoint::availableParameters() const {
    return MsiDbusCodec::wrap(mRegistry ? mRegistry->availableParameterNames() : QVariantList{});
}

QDBusVariant EcParametersEndpoint::readParameter(const QDBusVariant& name) const {
    return MsiDbusCodec::wrap(mRegistry ? mRegistry->readParameter(MsiDbusCodec::unwrap(name)) : QVariant{});
}

bool EcParametersEndpoint::writeParameter(const QDBusVariant& name, const QDBusVariant& value) {
    return mRegistry ? mRegistry->writeParameter(MsiDbusCodec::unwrap(name), MsiDbusCodec::unwrap(value)) : false;
}

QDBusVariant EcParametersEndpoint::writeParameters(const QDBusVariant& updates) {
    return MsiDbusCodec::wrap(mRegistry ? mRegistry->writeParameters(MsiDbusCodec::unwrap(updates).toList())
                                        : QVariantList{});
}

QDBusVariant EcParametersEndpoint::availableValues(const QDBusVariant& name) const {
    return MsiDbusCodec::wrap(mRegistry ? mRegistry->availableValues(MsiDbusCodec::unwrap(name)) : QVariant{});
}

EcMemoryEndpoint::EcMemoryEndpoint(EcMemoryService* memoryService, QObject* parent)
    : EcMemoryAbstract(parent), mMemoryService(memoryService) {}

int EcMemoryEndpoint::apiVersion() const {
    return MsiDbusApi::currentApiVersion;
}

QDBusVariant EcMemoryEndpoint::readEcMemory(const QDBusVariant& offset, const QDBusVariant& length) const {
    QString errorMessage;
    int offsetValue = 0;
    int lengthValue = 0;
    if (!extractInt(offset, &offsetValue, QStringLiteral("offset"), &errorMessage) ||
        !extractInt(length, &lengthValue, QStringLiteral("length"), &errorMessage)) {
        return MsiDbusCodec::wrap(memoryInputError(errorMessage));
    }
    return MsiDbusCodec::wrap(mMemoryService ? mMemoryService->readMemory(offsetValue, lengthValue)
                                             : memoryInputError(QStringLiteral("EC memory service is not available")));
}

QDBusVariant EcMemoryEndpoint::writeEcMemory(const QDBusVariant& offset, const QDBusVariant& bytes) {
    QString errorMessage;
    int offsetValue = 0;
    QByteArray rawBytes;
    if (!extractInt(offset, &offsetValue, QStringLiteral("offset"), &errorMessage) ||
        !variantListToBytes(MsiDbusCodec::unwrap(bytes), &rawBytes, &errorMessage)) {
        return MsiDbusCodec::wrap(memoryInputError(errorMessage));
    }
    return MsiDbusCodec::wrap(mMemoryService ? mMemoryService->writeMemory(offsetValue, rawBytes)
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
        return MsiDbusCodec::wrap(memoryInputError(errorMessage));
    }
    return MsiDbusCodec::wrap(mMemoryService ? mMemoryService->writeMemoryBits(offsetValue, maskValue, valueValue)
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

int EcProfilesEndpoint::apiVersion() const {
    return MsiDbusApi::currentApiVersion;
}

QDBusVariant EcProfilesEndpoint::supportedDeviceProfiles() const {
    return MsiDbusCodec::wrap(mProfileService ? mProfileService->supportedProfiles() : QVariantList{});
}

QDBusVariant EcProfilesEndpoint::activeDeviceProfile() const {
    return MsiDbusCodec::wrap(mProfileService ? mProfileService->activeProfile() : QVariantMap{});
}

QDBusVariant EcProfilesEndpoint::saveSupportedDeviceProfile(const QDBusVariant& profile) {
    return MsiDbusCodec::wrap(
        mProfileService
            ? mProfileService->saveProfile(MsiDbusCodec::unwrap(profile).toMap())
            : QVariantMap{{QStringLiteral("ok"), false},
                          {QStringLiteral("error"), QStringLiteral("Device profile service is not available")}});
}

QDBusVariant EcProfilesEndpoint::removeSupportedDeviceProfile(const QDBusVariant& profileId) {
    return MsiDbusCodec::wrap(
        mProfileService
            ? mProfileService->removeProfile(MsiDbusCodec::unwrap(profileId).toString())
            : QVariantMap{{QStringLiteral("ok"), false},
                          {QStringLiteral("error"), QStringLiteral("Device profile service is not available")}});
}

EcHealthEndpoint::EcHealthEndpoint(DiagnosticsService* diagnosticsService, QObject* parent)
    : EcHealthAbstract(parent), mDiagnosticsService(diagnosticsService) {}

int EcHealthEndpoint::apiVersion() const {
    return MsiDbusApi::currentApiVersion;
}

QDBusVariant EcHealthEndpoint::diagnostics() const {
    return MsiDbusCodec::wrap(mDiagnosticsService ? mDiagnosticsService->diagnostics() : QVariantMap{});
}

QDBusVariant EcHealthEndpoint::supportBundle() const {
    return MsiDbusCodec::wrap(mDiagnosticsService ? mDiagnosticsService->supportBundle() : QVariantMap{});
}
