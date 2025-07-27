#include "iobuffer.h"

#include <QDebug>
#include <QFile>
#include <QIODevice>

inline QByteArray doHeavyRead(QString fileName, QMap<uint, QByteArray> dataCache) {
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

IOBuffer::IOBuffer(const QString& fileName, QObject* parent) : QObject(parent), mFileName(fileName) {
    mBuffer = doHeavyRead(mFileName, mDataCache);
    mWatcher = new QFutureWatcher<QByteArray>(this);
    connect(mWatcher, &QFutureWatcher<QByteArray>::finished, this, [this]() {
        if (mDataCache.isEmpty()) {
            mBuffer = mWatcher->result();
            emit bufferChanged(mBuffer);
        }
        resetThread();
    });
    resetThread();
}

QByteArray IOBuffer::buffer() const {
    return mBuffer;
}

void IOBuffer::resetThread() {
    QFuture<QByteArray> future = QtConcurrent::run(doHeavyRead, mFileName, mDataCache);
    mWatcher->setFuture(future);
    mDataCache.clear();
}