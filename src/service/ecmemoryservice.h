#pragma once

#include <QByteArray>
#include <QObject>
#include <QVariantMap>

#include "ecwritepolicy.h"

class IOBuffer;

class EcMemoryService : public QObject {
    Q_OBJECT
public:
    explicit EcMemoryService(QObject* parent = nullptr);

    void setIoBuffer(IOBuffer* buffer);
    IOBuffer* ioBuffer() const;
    EcWritePolicy* writePolicy();
    const EcWritePolicy* writePolicy() const;
    QVariantMap readMemory(int offset, int length) const;
    QVariantMap writeMemory(int offset, const QByteArray& bytes);
    QVariantMap writeMemoryBits(int offset, int mask, int value);
    QVariantMap backendDiagnostics() const;

private:
    IOBuffer* mIoBuffer{nullptr};
    EcWritePolicy mWritePolicy;
};
