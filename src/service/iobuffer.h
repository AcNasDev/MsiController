#pragma once

#include <QByteArray>
#include <QFuture>
#include <QFutureWatcher>
#include <QMap>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QtConcurrent/QtConcurrentRun>

class IOBuffer : public QObject {
    Q_OBJECT
public:
    IOBuffer(const QString& fileName, QObject* parent = nullptr);
    ~IOBuffer() = default;

    const QByteArray& buffer() const;
    template <typename T>
    bool write(const T& value, uint address = 0) {
        const QByteArray bytes(reinterpret_cast<const char*>(&value), sizeof(T));
        mDataCache[address] = bytes;
        if (address + bytes.size() <= static_cast<uint>(mBuffer.size())) {
            mBuffer.replace(static_cast<int>(address), bytes.size(), bytes);
            emit bufferChanged(mBuffer);
        }
        if (mWatcher && !mWatcher->isRunning()) {
            QTimer::singleShot(0, this, &IOBuffer::startRead);
        }
        return true;
    }

signals:
    void bufferChanged(const QByteArray& newBuffer);

private:
    QString mFileName;
    QByteArray mBuffer;
    QFutureWatcher<QByteArray>* mWatcher{nullptr};
    QMap<uint, QByteArray> mDataCache;
    QTimer mPollTimer;

    void startRead();
};
