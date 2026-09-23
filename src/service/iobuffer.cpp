#include "iobuffer.h"

#include "ecmemorybackend.h"

namespace {
constexpr int ecPollIntervalMs = 1000;
} // namespace

IOBuffer::IOBuffer(const QString& fileName, QObject* parent)
    : IOBuffer(std::make_unique<FileEcMemoryBackend>(fileName), parent) {}

IOBuffer::IOBuffer(std::unique_ptr<EcMemoryBackend> backend, QObject* parent)
    : QObject(parent), mBackend(std::move(backend)) {
    mPollTimer.setInterval(ecPollIntervalMs);
    connect(&mPollTimer, &QTimer::timeout, this, &IOBuffer::startRead);
    mPollTimer.start();
    startRead();
}

const QByteArray& IOBuffer::buffer() const {
    return mBuffer;
}

bool IOBuffer::writeBytes(const QByteArray& bytes, uint address) {
    if (bytes.isEmpty()) {
        return true;
    }

    const auto bufferSize = static_cast<quint64>(mBuffer.size());
    if (!mBackend || address > bufferSize || static_cast<quint64>(bytes.size()) > bufferSize - address) {
        return false;
    }

    mDataCache[address] = bytes;
    mBuffer.replace(static_cast<qsizetype>(address), bytes.size(), bytes);
    emit bufferChanged(mBuffer);
    QTimer::singleShot(0, this, &IOBuffer::startRead);
    return true;
}

QVariantMap IOBuffer::backendDiagnostics() const {
    return mBackend ? mBackend->diagnostics() : QVariantMap{};
}

bool IOBuffer::isSimulator() const {
    return mBackend && mBackend->isSimulator();
}

void IOBuffer::startRead() {
    if (!mBackend) {
        return;
    }

    QMap<uint, QByteArray> pendingWrites;
    mDataCache.swap(pendingWrites);
    const auto nextBuffer = mBackend->readAll(pendingWrites);
    if (!nextBuffer) {
        for (auto it = pendingWrites.cbegin(); it != pendingWrites.cend(); ++it) {
            if (!mDataCache.contains(it.key())) {
                mDataCache.insert(it.key(), it.value());
            }
        }
        return;
    }
    if (*nextBuffer == mBuffer && pendingWrites.isEmpty()) {
        return;
    }

    mBuffer = *nextBuffer;
    emit bufferChanged(mBuffer);
}
