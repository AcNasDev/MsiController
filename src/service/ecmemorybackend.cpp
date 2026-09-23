#include "ecmemorybackend.h"

#include <QDateTime>
#include <QFile>
#include <QIODevice>
#include <QtGlobal>
#include <utility>

EcMemoryBackend::EcMemoryBackend(QObject* parent) : QObject(parent) {}

bool EcMemoryBackend::isSimulator() const {
    return false;
}

QVariantMap EcMemoryBackend::diagnostics() const {
    QVariantMap result;
    result.insert(QStringLiteral("name"), displayName());
    result.insert(QStringLiteral("simulator"), isSimulator());
    return result;
}

FileEcMemoryBackend::FileEcMemoryBackend(QString fileName, QObject* parent)
    : EcMemoryBackend(parent), mFileName(std::move(fileName)) {}

std::optional<QByteArray> FileEcMemoryBackend::readAll(const QMap<uint, QByteArray>& pendingWrites) {
    QFile file(mFileName);
    if (!file.open(QIODevice::ReadWrite | QIODevice::ExistingOnly)) {
        qWarning() << "Failed to open EC memory backend:" << mFileName << file.errorString();
        return std::nullopt;
    }

    for (auto it = pendingWrites.cbegin(); it != pendingWrites.cend(); ++it) {
        if (!file.seek(it.key()) || file.write(it.value()) != it.value().size()) {
            qWarning() << "Failed to write EC memory backend:" << mFileName << file.errorString();
            return std::nullopt;
        }
    }
    if (!pendingWrites.isEmpty() && !file.flush()) {
        qWarning() << "Failed to flush EC memory backend:" << mFileName << file.errorString();
        return std::nullopt;
    }

    if (!file.seek(0)) {
        qWarning() << "Failed to rewind EC memory backend:" << mFileName << file.errorString();
        return std::nullopt;
    }
    QByteArray bytes = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        qWarning() << "Failed to read EC memory backend:" << mFileName << file.errorString();
        return std::nullopt;
    }
    return bytes;
}

QString FileEcMemoryBackend::displayName() const {
    return mFileName;
}

QVariantMap FileEcMemoryBackend::diagnostics() const {
    QVariantMap result = EcMemoryBackend::diagnostics();
    result.insert(QStringLiteral("path"), mFileName);
    return result;
}

SimulatedEcMemoryBackend::SimulatedEcMemoryBackend(QObject* parent) : EcMemoryBackend(parent), mMemory(256, '\0') {
    writeAscii(0xa0, 12, QByteArrayLiteral("14C1EMS1.012"));
    writeAscii(0xac, 8, QByteArrayLiteral("06012026"));
    writeAscii(0xb4, 8, QByteArrayLiteral("08:00:00"));

    writeByte(0x31, 0x03);
    writeByte(0x42, 76);
    writeByte(0xef, 90);
    writeByte(0x2b, 0x00);
    writeByte(0x2c, 0x00);
    writeByte(0x2e, 0x02);
    writeByte(0x2f, 0x00);
    writeByte(0x98, 0x00);
    writeByte(0xbf, 0x08);
    writeByte(0xf2, 0xc1);
    writeByte(0xf3, 0x82);
    writeByte(0xf4, 0x0d);

    for (int i = 0; i < 7; ++i) {
        writeByte(0x72 + i, static_cast<quint8>(30 + i * 10));
        writeByte(0x8a + i, static_cast<quint8>(30 + i * 10));
    }
    for (int i = 0; i < 6; ++i) {
        writeByte(0x6a + i, static_cast<quint8>(45 + i * 7));
        writeByte(0x82 + i, static_cast<quint8>(45 + i * 7));
    }
    refreshTelemetry();
}

std::optional<QByteArray> SimulatedEcMemoryBackend::readAll(const QMap<uint, QByteArray>& pendingWrites) {
    applyWrites(pendingWrites);
    refreshTelemetry();
    return mMemory;
}

QString SimulatedEcMemoryBackend::displayName() const {
    return QStringLiteral("simulated-msi-ec");
}

bool SimulatedEcMemoryBackend::isSimulator() const {
    return true;
}

QVariantMap SimulatedEcMemoryBackend::diagnostics() const {
    QVariantMap result = EcMemoryBackend::diagnostics();
    result.insert(QStringLiteral("size"), mMemory.size());
    result.insert(QStringLiteral("firmware"), QString::fromLatin1(mMemory.mid(0xa0, 12)).trimmed());
    result.insert(QStringLiteral("tick"), mTick);
    return result;
}

void SimulatedEcMemoryBackend::applyWrites(const QMap<uint, QByteArray>& pendingWrites) {
    for (auto it = pendingWrites.cbegin(); it != pendingWrites.cend(); ++it) {
        const int offset = static_cast<int>(it.key());
        const QByteArray& bytes = it.value();
        if (offset < 0 || offset + bytes.size() > mMemory.size()) {
            continue;
        }
        mMemory.replace(offset, bytes.size(), bytes);
    }
}

void SimulatedEcMemoryBackend::refreshTelemetry() {
    ++mTick;
    writeByte(0x68, static_cast<quint8>(54 + (mTick % 17)));
    writeByte(0x80, static_cast<quint8>(47 + ((mTick * 2) % 15)));
    writeByte(0x71, static_cast<quint8>(35 + ((mTick * 3) % 26)));
    writeByte(0x89, static_cast<quint8>(32 + ((mTick * 2) % 24)));
    if (mTick % 90 == 0) {
        const int battery = qBound(40, static_cast<int>(static_cast<quint8>(mMemory.at(0x42))) - 1, 100);
        writeByte(0x42, static_cast<quint8>(battery));
    }
}

void SimulatedEcMemoryBackend::writeAscii(int offset, int length, const QByteArray& text) {
    if (offset < 0 || length < 0 || offset + length > mMemory.size()) {
        return;
    }

    QByteArray padded = text.left(length);
    padded.resize(length);
    mMemory.replace(offset, length, padded);
}

void SimulatedEcMemoryBackend::writeByte(int offset, quint8 value) {
    if (offset < 0 || offset >= mMemory.size()) {
        return;
    }
    mMemory[offset] = static_cast<char>(value);
}
