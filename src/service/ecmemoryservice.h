#pragma once

#include <QByteArray>
#include <QObject>
#include <QVariantMap>

class IOBuffer;

class EcMemoryService : public QObject {
    Q_OBJECT
public:
    explicit EcMemoryService(QObject* parent = nullptr);

    void setIoBuffer(IOBuffer* buffer);
    QVariantMap readMemory(int offset, int length) const;
    QVariantMap writeMemory(int offset, const QByteArray& bytes);
    QVariantMap writeMemoryBits(int offset, int mask, int value);

private:
    IOBuffer* mIoBuffer{nullptr};
};
