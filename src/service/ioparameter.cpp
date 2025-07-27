#include "ioparameter.h"

#include <QtConcurrent/QtConcurrentRun>

IOParameterString::IOParameterString(IOBuffer* buffer,
                                     quint16 address,
                                     const QVariant& name,
                                     const QVariant& available,
                                     bool isReadOnly,
                                     quint16 length,
                                     QObject* parent)
    : Parameter(name, available, isReadOnly, parent), mBuffer(buffer), mAddress(address), mLength(length) {
    connect(mBuffer, &IOBuffer::bufferChanged, this, &IOParameterString::update);
}

QString IOParameterString::rawRead() const {
    const QByteArray& data = mBuffer->buffer();
    if (static_cast<size_t>(mAddress) + mLength > static_cast<size_t>(data.size())) {
        qCritical() << "Buffer overflow at address:" << mAddress;
        return QString();
    }
    QByteArray strData = data.mid(mAddress, mLength);
    return QString::fromUtf8(strData);
}

bool IOParameterString::writeValue(const QVariant& value) {
    QString strValue = value.toString();
    QByteArray strData = strValue.toUtf8();
    if (strData.size() > mLength) {
        qWarning() << "String too long for IOParameterString, truncating:" << strData;
        strData = strData.left(mLength);
    }
    return mBuffer->write(strData, mAddress);
}

QVariant IOParameterString::readValue() const {
    QString value = rawRead();
    return QVariant::fromValue(value);
}
