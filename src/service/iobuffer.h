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
    bool writeBytes(const QByteArray& bytes, uint address = 0);

    template <typename T>
    bool write(const T& value, uint address = 0) {
        const QByteArray bytes(reinterpret_cast<const char*>(&value), sizeof(T));
        return writeBytes(bytes, address);
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
