#include "ecmemoryservice.h"

#include <QtGlobal>

#include "iobuffer.h"

namespace {
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
} // namespace

EcMemoryService::EcMemoryService(QObject* parent) : QObject(parent), mWritePolicy(this) {}

void EcMemoryService::setIoBuffer(IOBuffer* buffer) {
    mIoBuffer = buffer;
}

IOBuffer* EcMemoryService::ioBuffer() const {
    return mIoBuffer;
}

EcWritePolicy* EcMemoryService::writePolicy() {
    return &mWritePolicy;
}

const EcWritePolicy* EcMemoryService::writePolicy() const {
    return &mWritePolicy;
}

QVariantMap EcMemoryService::readMemory(int offset, int length) const {
    if (!mIoBuffer) {
        return memoryResult(false, QStringLiteral("EC buffer is not available"), 0);
    }

    QString errorMessage;
    const int bufferSize = mIoBuffer->buffer().size();
    if (!validateMemoryRange(offset, length, bufferSize, &errorMessage)) {
        return memoryResult(false, errorMessage, bufferSize);
    }

    QVariantMap result = memoryResult(true, QString(), bufferSize);
    const QByteArray bytes = mIoBuffer->buffer().mid(offset, length);
    result.insert(QStringLiteral("offset"), offset);
    result.insert(QStringLiteral("length"), length);
    result.insert(QStringLiteral("bytes"), bytesToVariantList(bytes));
    return result;
}

QVariantMap EcMemoryService::writeMemory(int offset, const QByteArray& bytes) {
    if (!mIoBuffer) {
        return memoryResult(false, QStringLiteral("EC buffer is not available"), 0);
    }

    QString errorMessage;
    const int bufferSize = mIoBuffer->buffer().size();
    if (!validateMemoryRange(offset, bytes.size(), bufferSize, &errorMessage)) {
        return memoryResult(false, errorMessage, bufferSize);
    }
    if (!mWritePolicy.allows(offset, bytes.size(), &errorMessage)) {
        mWritePolicy.recordWrite(QStringLiteral("writeBytes"), offset, bytes, false, errorMessage);
        return memoryResult(false, errorMessage, bufferSize);
    }

    const QByteArray previousBytes = mIoBuffer->buffer().mid(offset, bytes.size());
    const bool queued = mIoBuffer->writeBytes(bytes, static_cast<uint>(offset));
    mWritePolicy.recordWrite(QStringLiteral("writeBytes"),
                             offset,
                             bytes,
                             queued,
                             queued ? QString() : QStringLiteral("Failed to queue EC write"));

    QVariantMap result =
        memoryResult(queued, queued ? QString() : QStringLiteral("Failed to queue EC write"), bufferSize);
    result.insert(QStringLiteral("offset"), offset);
    result.insert(QStringLiteral("length"), bytes.size());
    result.insert(QStringLiteral("bytes"), bytesToVariantList(bytes));
    result.insert(QStringLiteral("previousBytes"), bytesToVariantList(previousBytes));
    result.insert(QStringLiteral("queued"), queued);
    return result;
}

QVariantMap EcMemoryService::writeMemoryBits(int offset, int mask, int value) {
    if (!mIoBuffer) {
        return memoryResult(false, QStringLiteral("EC buffer is not available"), 0);
    }

    QString errorMessage;
    const int bufferSize = mIoBuffer->buffer().size();
    if (!validateMemoryRange(offset, 1, bufferSize, &errorMessage)) {
        return memoryResult(false, errorMessage, bufferSize);
    }
    if (mask < 0 || mask > 0xff || value < 0 || value > 0xff) {
        return memoryResult(false, QStringLiteral("Mask and value must be in range 0x00..0xff"), bufferSize);
    }
    if (!mWritePolicy.allows(offset, 1, &errorMessage)) {
        const QByteArray rejectedBytes(1, static_cast<char>(value & mask));
        mWritePolicy.recordWrite(QStringLiteral("writeBits"), offset, rejectedBytes, false, errorMessage);
        return memoryResult(false, errorMessage, bufferSize);
    }

    const auto currentByte = static_cast<quint8>(mIoBuffer->buffer().at(offset));
    const auto maskByte = static_cast<quint8>(mask);
    const auto valueByte = static_cast<quint8>(value);
    const auto nextByte = static_cast<quint8>((currentByte & static_cast<quint8>(~maskByte)) | (valueByte & maskByte));
    const QByteArray rawBytes(1, static_cast<char>(nextByte));
    const bool queued = mIoBuffer->writeBytes(rawBytes, static_cast<uint>(offset));
    mWritePolicy.recordWrite(QStringLiteral("writeBits"),
                             offset,
                             rawBytes,
                             queued,
                             queued ? QString() : QStringLiteral("Failed to queue EC bit write"));

    QVariantMap result =
        memoryResult(queued, queued ? QString() : QStringLiteral("Failed to queue EC bit write"), bufferSize);
    result.insert(QStringLiteral("offset"), offset);
    result.insert(QStringLiteral("length"), 1);
    result.insert(QStringLiteral("mask"), mask);
    result.insert(QStringLiteral("value"), value);
    result.insert(QStringLiteral("previousByte"), static_cast<int>(currentByte));
    result.insert(QStringLiteral("byte"), static_cast<int>(nextByte));
    result.insert(QStringLiteral("bytes"), bytesToVariantList(rawBytes));
    result.insert(QStringLiteral("queued"), queued);
    return result;
}

QVariantMap EcMemoryService::backendDiagnostics() const {
    return mIoBuffer ? mIoBuffer->backendDiagnostics() : QVariantMap{};
}
