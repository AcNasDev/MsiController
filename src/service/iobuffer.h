#pragma once

#include <QThread>
#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>


class IOBuffer : public QObject {
    Q_OBJECT
public:
    IOBuffer(const QString &fileName, QObject *parent = nullptr);
    ~IOBuffer() = default;

    QByteArray buffer() const;
    template<typename T>
    bool write(const T &value, const uint &address = 0) {
        mDataCache[address] = QByteArray(reinterpret_cast<const char*>(&value), sizeof(T));
        return true;
    }

signals:
    void bufferChanged(const QByteArray &newBuffer);

private:
    QString mFileName;
    QByteArray mBuffer;
    QFutureWatcher<QByteArray> *mWatcher{ nullptr };
    QMap<uint, QByteArray> mDataCache;

    void resetThread();
};