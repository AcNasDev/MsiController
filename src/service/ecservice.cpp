#include "ecservice.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusError>
#include <QDir>
#include <QFile>
#include <QPair>
#include <QVector>
#include <utility>

#include "iobuffer.h"
#include "struct.h"
#include "supportconfig.h"

namespace {
QVariant unwrapDbusMsg(const QDBusVariant& value) {
    return qdbus_cast<Msi::Msg>(value.variant()).variant;
}

QDBusVariant wrapDbusMsg(const QVariant& value) {
    return QDBusVariant(QVariant::fromValue(Msi::Msg(value)));
}

QVariantList bytesToVariantList(const QByteArray& bytes) {
    QVariantList result;
    result.reserve(bytes.size());
    for (char byte : bytes) {
        result.append(static_cast<int>(static_cast<unsigned char>(byte)));
    }
    return result;
}

QVariantMap memoryResult(bool ok, const QString& error, int bufferSize) {
    QVariantMap result;
    result.insert(QStringLiteral("ok"), ok);
    result.insert(QStringLiteral("error"), error);
    result.insert(QStringLiteral("size"), bufferSize);
    return result;
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

bool validateMemoryRange(int offset, int length, int bufferSize, QString* errorMessage) {
    if (offset < 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Offset must be greater than or equal to 0");
        }
        return false;
    }
    if (length < 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Length must be greater than or equal to 0");
        }
        return false;
    }
    if (offset > bufferSize || length > bufferSize - offset) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Range 0x%1..0x%2 is outside EC buffer size %3")
                                .arg(offset, 2, 16, QLatin1Char('0'))
                                .arg(offset + qMax(0, length - 1), 2, 16, QLatin1Char('0'))
                                .arg(bufferSize);
        }
        return false;
    }
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
} // namespace

EcService::ProfileRegistrationScope::ProfileRegistrationScope(EcService& service) : mService(&service) {
    mService->beginProfileRegistration();
}

EcService::ProfileRegistrationScope::~ProfileRegistrationScope() {
    if (mService) {
        mService->endProfileRegistration();
    }
}

EcService::EcService(QObject* parent) : EcServiceAbstract(parent) {}

EcService::~EcService() {
    qDeleteAll(mProfileObjects);
    qDeleteAll(mParameters);
}

void EcService::registerParameter(Parameter* param) {
    if (mParameters.contains(param->name())) {
        qWarning() << "Duplicate parameter name:" << param->name();
        delete param;
        return;
    }

    mParameters.insert(param->name(), param);
    if (mProfileRegistrationDepth > 0) {
        mProfileParameterNames.insert(param->name());
    }
    param->setParent(this);

    connect(param, &Parameter::valueChanged, this, [this, param](const QVariant& value) {
        emit parameterChanged(QDBusVariant(QVariant::fromValue(Msi::Msg(param->name()))),
                              QDBusVariant(QVariant::fromValue(Msi::Msg(value))));
    });
}

void EcService::registerProfileObject(QObject* object) {
    if (!object) {
        return;
    }

    object->setParent(this);
    mProfileObjects.append(object);
}

void EcService::clearProfileScope() {
    qDeleteAll(mProfileObjects);
    mProfileObjects.clear();

    for (const QVariant& name : std::as_const(mProfileParameterNames)) {
        if (Parameter* parameter = mParameters.take(name)) {
            delete parameter;
        }
    }
    mProfileParameterNames.clear();
}

QDBusVariant EcService::availableParameters() const {
    return QDBusVariant(QVariant::fromValue(Msi::Msg(mParameters.keys())));
}

QDBusVariant EcService::readParameter(const QDBusVariant& name) const {
    QVariant arg = qdbus_cast<Msi::Msg>(name.variant()).variant;
    if (mParameters.contains(arg)) {
        return QDBusVariant(QVariant::fromValue(Msi::Msg(mParameters[arg]->value())));
    }
    return {};
}

bool EcService::writeParameter(const QDBusVariant& name, const QDBusVariant& value) {
    auto vName = qdbus_cast<Msi::Msg>(name.variant()).variant;
    auto vValue = qdbus_cast<Msi::Msg>(value.variant()).variant;
    if (mParameters.contains(vName) && !mParameters[vName]->isReadOnly()) {
        mParameters[vName]->setValue(vValue);
        return true;
    }
    return false;
}

QDBusVariant EcService::writeParameters(const QDBusVariant& updates) {
    const auto values = qdbus_cast<Msi::Msg>(updates.variant()).variant.toList();
    QVariantList result;
    if (values.size() % 2 != 0) {
        qWarning() << "Invalid batch write payload size:" << values.size();
        return QDBusVariant(QVariant::fromValue(Msi::Msg(result)));
    }

    QVector<QPair<QVariant, QVariant>> requests;
    requests.reserve(values.size() / 2);
    for (qsizetype i = 0; i < values.size(); i += 2) {
        const QVariant name = values.at(i);
        const QVariant value = values.at(i + 1);
        if (!mParameters.contains(name) || mParameters[name]->isReadOnly()) {
            qWarning() << "Skipping invalid batch write parameter:" << name;
            continue;
        }
        requests.append(qMakePair(name, value));
    }

    for (const auto& request : std::as_const(requests)) {
        mParameters[request.first]->setValue(request.second);
    }

    for (const auto& request : std::as_const(requests)) {
        if (Parameter* parameter = mParameters.value(request.first, nullptr)) {
            result << request.first << parameter->value();
        }
    }

    return QDBusVariant(QVariant::fromValue(Msi::Msg(result)));
}

QDBusVariant EcService::availableValues(const QDBusVariant& name) const {
    QVariant arg = qdbus_cast<Msi::Msg>(name.variant()).variant;
    if (mParameters.contains(arg)) {
        return QDBusVariant(QVariant::fromValue(Msi::Msg(mParameters[arg]->available())));
    }
    return {};
}

Parameter* EcService::parameter(const QVariant& name) const {
    return mParameters.value(name, nullptr);
}

void EcService::setIoBuffer(IOBuffer* buffer) {
    mIoBuffer = buffer;
}

QDBusVariant EcService::readEcMemory(const QDBusVariant& offset, const QDBusVariant& length) const {
    if (!mIoBuffer) {
        return wrapDbusMsg(memoryResult(false, QStringLiteral("EC buffer is not available"), 0));
    }

    QString errorMessage;
    int offsetValue = 0;
    int lengthValue = 0;
    const int bufferSize = mIoBuffer->buffer().size();
    if (!extractInt(offset, &offsetValue, QStringLiteral("offset"), &errorMessage) ||
        !extractInt(length, &lengthValue, QStringLiteral("length"), &errorMessage) ||
        !validateMemoryRange(offsetValue, lengthValue, bufferSize, &errorMessage)) {
        return wrapDbusMsg(memoryResult(false, errorMessage, bufferSize));
    }

    QVariantMap result = memoryResult(true, QString(), bufferSize);
    const QByteArray bytes = mIoBuffer->buffer().mid(offsetValue, lengthValue);
    result.insert(QStringLiteral("offset"), offsetValue);
    result.insert(QStringLiteral("length"), lengthValue);
    result.insert(QStringLiteral("bytes"), bytesToVariantList(bytes));
    return wrapDbusMsg(result);
}

QDBusVariant EcService::writeEcMemory(const QDBusVariant& offset, const QDBusVariant& bytes) {
    if (!mIoBuffer) {
        return wrapDbusMsg(memoryResult(false, QStringLiteral("EC buffer is not available"), 0));
    }

    QString errorMessage;
    int offsetValue = 0;
    QByteArray rawBytes;
    const int bufferSize = mIoBuffer->buffer().size();
    if (!extractInt(offset, &offsetValue, QStringLiteral("offset"), &errorMessage) ||
        !variantListToBytes(unwrapDbusMsg(bytes), &rawBytes, &errorMessage) ||
        !validateMemoryRange(offsetValue, rawBytes.size(), bufferSize, &errorMessage)) {
        return wrapDbusMsg(memoryResult(false, errorMessage, bufferSize));
    }

    const QByteArray previousBytes = mIoBuffer->buffer().mid(offsetValue, rawBytes.size());
    const bool queued = mIoBuffer->writeBytes(rawBytes, static_cast<uint>(offsetValue));

    QVariantMap result = memoryResult(queued, queued ? QString() : QStringLiteral("Failed to queue EC write"), bufferSize);
    result.insert(QStringLiteral("offset"), offsetValue);
    result.insert(QStringLiteral("length"), rawBytes.size());
    result.insert(QStringLiteral("bytes"), bytesToVariantList(rawBytes));
    result.insert(QStringLiteral("previousBytes"), bytesToVariantList(previousBytes));
    result.insert(QStringLiteral("queued"), queued);
    return wrapDbusMsg(result);
}

QDBusVariant EcService::writeEcMemoryBits(const QDBusVariant& offset,
                                          const QDBusVariant& mask,
                                          const QDBusVariant& value) {
    if (!mIoBuffer) {
        return wrapDbusMsg(memoryResult(false, QStringLiteral("EC buffer is not available"), 0));
    }

    QString errorMessage;
    int offsetValue = 0;
    int maskValue = 0;
    int valueValue = 0;
    const int bufferSize = mIoBuffer->buffer().size();
    if (!extractInt(offset, &offsetValue, QStringLiteral("offset"), &errorMessage) ||
        !extractInt(mask, &maskValue, QStringLiteral("mask"), &errorMessage) ||
        !extractInt(value, &valueValue, QStringLiteral("value"), &errorMessage) ||
        !validateMemoryRange(offsetValue, 1, bufferSize, &errorMessage)) {
        return wrapDbusMsg(memoryResult(false, errorMessage, bufferSize));
    }
    if (maskValue < 0 || maskValue > 0xff || valueValue < 0 || valueValue > 0xff) {
        return wrapDbusMsg(memoryResult(false, QStringLiteral("Mask and value must be in range 0x00..0xff"), bufferSize));
    }

    const auto currentByte = static_cast<quint8>(mIoBuffer->buffer().at(offsetValue));
    const auto maskByte = static_cast<quint8>(maskValue);
    const auto valueByte = static_cast<quint8>(valueValue);
    const auto nextByte = static_cast<quint8>((currentByte & static_cast<quint8>(~maskByte)) | (valueByte & maskByte));
    const QByteArray rawBytes(1, static_cast<char>(nextByte));
    const bool queued = mIoBuffer->writeBytes(rawBytes, static_cast<uint>(offsetValue));

    QVariantMap result = memoryResult(queued, queued ? QString() : QStringLiteral("Failed to queue EC bit write"), bufferSize);
    result.insert(QStringLiteral("offset"), offsetValue);
    result.insert(QStringLiteral("length"), 1);
    result.insert(QStringLiteral("mask"), maskValue);
    result.insert(QStringLiteral("value"), valueValue);
    result.insert(QStringLiteral("previousByte"), static_cast<int>(currentByte));
    result.insert(QStringLiteral("byte"), static_cast<int>(nextByte));
    result.insert(QStringLiteral("bytes"), bytesToVariantList(rawBytes));
    result.insert(QStringLiteral("queued"), queued);
    return wrapDbusMsg(result);
}

void EcService::setSupportConfigRepository(SupportConfigRepository* repository) {
    if (mSupportConfigRepository == repository) {
        return;
    }

    if (mSupportConfigRepository) {
        disconnect(mSupportConfigRepository, nullptr, this, nullptr);
    }
    mSupportConfigRepository = repository;
    if (mSupportConfigRepository) {
        connect(mSupportConfigRepository, &SupportConfigRepository::profilesChanged, this, [this]() {
            emit supportedDeviceProfilesChanged();
        });
    }
}

void EcService::setSupportProfileApplier(std::function<bool(QString*)> applier) {
    mSupportProfileApplier = std::move(applier);
}

bool EcService::applySupportProfile(QString* errorMessage) {
    if (!mSupportProfileApplier) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Live profile apply is not available");
        }
        return false;
    }

    const bool ok = mSupportProfileApplier(errorMessage);
    if (ok) {
        emit availableParametersChanged();
        emit supportedDeviceProfilesChanged();
    }
    return ok;
}

QDBusVariant EcService::supportedDeviceProfiles() const {
    if (!mSupportConfigRepository) {
        return QDBusVariant(QVariant::fromValue(Msi::Msg(QVariantList{})));
    }
    return QDBusVariant(QVariant::fromValue(Msi::Msg(mSupportConfigRepository->profilesForDbus())));
}

QDBusVariant EcService::activeDeviceProfile() const {
    if (!mSupportConfigRepository) {
        return QDBusVariant(QVariant::fromValue(Msi::Msg(QVariantMap{})));
    }
    return QDBusVariant(QVariant::fromValue(Msi::Msg(mSupportConfigRepository->activeProfileForDbus())));
}

QDBusVariant EcService::saveSupportedDeviceProfile(const QDBusVariant& profile) {
    QVariantMap result;
    if (!mSupportConfigRepository) {
        result.insert(QStringLiteral("ok"), false);
        result.insert(QStringLiteral("error"), QStringLiteral("Support config repository is not available"));
        return QDBusVariant(QVariant::fromValue(Msi::Msg(result)));
    }

    QString errorMessage;
    const QVariantMap profileMap = qdbus_cast<Msi::Msg>(profile.variant()).variant.toMap();
    const bool saved = mSupportConfigRepository->saveUserProfile(profileMap, &errorMessage);
    bool applied = false;
    if (saved) {
        QString applyError;
        applied = applySupportProfile(&applyError);
        if (!applied) {
            errorMessage = QStringLiteral("Profile saved, but live apply failed: %1").arg(applyError);
        }
    }
    result.insert(QStringLiteral("ok"), saved && applied);
    result.insert(QStringLiteral("saved"), saved);
    result.insert(QStringLiteral("applied"), applied);
    result.insert(QStringLiteral("error"), errorMessage);
    return QDBusVariant(QVariant::fromValue(Msi::Msg(result)));
}

QDBusVariant EcService::removeSupportedDeviceProfile(const QDBusVariant& profileId) {
    QVariantMap result;
    if (!mSupportConfigRepository) {
        result.insert(QStringLiteral("ok"), false);
        result.insert(QStringLiteral("error"), QStringLiteral("Support config repository is not available"));
        return QDBusVariant(QVariant::fromValue(Msi::Msg(result)));
    }

    QString errorMessage;
    const QString id = qdbus_cast<Msi::Msg>(profileId.variant()).variant.toString();
    const bool removed = mSupportConfigRepository->removeUserProfile(id, &errorMessage);
    bool applied = false;
    if (removed) {
        QString applyError;
        applied = applySupportProfile(&applyError);
        if (!applied) {
            errorMessage = QStringLiteral("Profile removed, but live apply failed: %1").arg(applyError);
        }
    }
    result.insert(QStringLiteral("ok"), removed && applied);
    result.insert(QStringLiteral("removed"), removed);
    result.insert(QStringLiteral("applied"), applied);
    result.insert(QStringLiteral("error"), errorMessage);
    return QDBusVariant(QVariant::fromValue(Msi::Msg(result)));
}

void EcService::beginProfileRegistration() {
    ++mProfileRegistrationDepth;
}

void EcService::endProfileRegistration() {
    mProfileRegistrationDepth = qMax(0, mProfileRegistrationDepth - 1);
}
