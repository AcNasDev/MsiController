#pragma once

#include <QByteArray>
#include <QMap>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantMap>
#include <memory>
#include <cstring>
#include <type_traits>

class EcMemoryBackend;

class IOBuffer : public QObject {
    Q_OBJECT
public:
    IOBuffer(const QString& fileName, QObject* parent = nullptr);
    IOBuffer(std::unique_ptr<EcMemoryBackend> backend, QObject* parent = nullptr);
    ~IOBuffer() = default;

    const QByteArray& buffer() const;
    bool writeBytes(const QByteArray& bytes, uint address = 0);
    QVariantMap backendDiagnostics() const;
    bool isSimulator() const;

    template <typename T>
    bool write(const T& value, uint address = 0) {
        static_assert(std::is_integral_v<T> || std::is_enum_v<T>, "EC writes require a scalar value");
        QByteArray bytes(sizeof(T), '\0');
        std::memcpy(bytes.data(), &value, sizeof(T));
        return writeBytes(bytes, address);
    }

signals:
    void bufferChanged(const QByteArray& newBuffer);

private:
    std::unique_ptr<EcMemoryBackend> mBackend;
    QByteArray mBuffer;
    QMap<uint, QByteArray> mDataCache;
    QTimer mPollTimer;

    void startRead();
};
