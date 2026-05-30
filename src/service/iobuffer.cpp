#include "iobuffer.h"

#include <QDebug>
#include <QFile>
#include <QIODevice>

namespace {
constexpr int ecPollIntervalMs = 1000;

QByteArray readEcBuffer(QString fileName, QMap<uint, QByteArray> dataCache) {
    QFile file(fileName);
    if (!file.open(QIODevice::ReadWrite)) {
        qWarning() << "Failed to open file:" << fileName;
        return QByteArray();
    }
    for (auto it = dataCache.begin(); it != dataCache.end(); ++it) {
        file.seek(it.key());
        file.write(it.value());
    }
    file.seek(0);
    QByteArray result = file.readAll();
    file.close();
    return result;
}
} // namespace

IOBuffer::IOBuffer(const QString& fileName, QObject* parent) : QObject(parent), mFileName(fileName) {
    mBuffer = readEcBuffer(mFileName, mDataCache);
    mWatcher = new QFutureWatcher<QByteArray>(this);
    connect(mWatcher, &QFutureWatcher<QByteArray>::finished, this, [this]() {
        mBuffer = mWatcher->result();
        emit bufferChanged(mBuffer);
        if (!mDataCache.isEmpty()) {
            startRead();
        }
    });

    mPollTimer.setInterval(ecPollIntervalMs);
    connect(&mPollTimer, &QTimer::timeout, this, &IOBuffer::startRead);
    mPollTimer.start();
    startRead();
}

const QByteArray& IOBuffer::buffer() const {
    return mBuffer;
}

void IOBuffer::startRead() {
    if (mWatcher->isRunning()) {
        return;
    }

    const QMap<uint, QByteArray> pendingWrites = mDataCache;
    mDataCache.clear();
    QFuture<QByteArray> future = QtConcurrent::run(readEcBuffer, mFileName, pendingWrites);
    mWatcher->setFuture(future);
}
