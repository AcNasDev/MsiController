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

    mDataCache[address] = bytes;
    if (address + bytes.size() <= static_cast<uint>(mBuffer.size())) {
        mBuffer.replace(static_cast<int>(address), bytes.size(), bytes);
        emit bufferChanged(mBuffer);
    }
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

    const QMap<uint, QByteArray> pendingWrites = mDataCache;
    mDataCache.clear();
    const QByteArray nextBuffer = mBackend->readAll(pendingWrites);
    if (nextBuffer == mBuffer && pendingWrites.isEmpty()) {
        return;
    }

    mBuffer = nextBuffer;
    emit bufferChanged(mBuffer);
}
