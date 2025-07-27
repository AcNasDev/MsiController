#pragma once

template <typename T>
IOParameter<T>::IOParameter(IOBuffer* buffer,
                            quint16 address,
                            const QVariant& name,
                            const QVariant& available,
                            bool isReadOnly,
                            BaseType mask,
                            QDataStream::ByteOrder byteOrder,
                            QObject* parent)
    : Parameter(name, available, isReadOnly, parent), mBuffer(buffer), mAddress(address), mMask(mask),
      mByteOrder(byteOrder) {
    connect(mBuffer, &IOBuffer::bufferChanged, this, &IOParameter<T>::update);
}

template <typename T>
T IOParameter<T>::rawRead() const {
    const QByteArray& data = mBuffer->buffer();
    const size_t typeSize = sizeof(BaseType);
    if (static_cast<size_t>(mAddress) + typeSize > static_cast<size_t>(data.size())) {
        qCritical() << "Buffer overflow at address:" << mAddress;
        return T{};
    }
    BaseType value;
    std::memcpy(&value, data.constData() + mAddress, typeSize);
    return static_cast<T>(value);
}

template <typename T>
void IOParameter<T>::setEnumHash(const QHash<T, BaseType>& hash) {
    mEnumHash = hash;
}

template <typename T>
bool IOParameter<T>::writeValue(const QVariant& value) {
    qDebug() << "Writing value:" << value << "to address:" << mAddress;
    BaseType current = static_cast<BaseType>(rawRead());
    BaseType new_val = static_cast<BaseType>(value.value<T>());
    if (mEnumHash.contains(value.value<T>())) {
        new_val = mEnumHash.value(value.value<T>());
    }
    qDebug() << "Writing value:" << value << "to address:" << mAddress;
    BaseType updated = (current & ~mMask) | (new_val & mMask);
    if (mByteOrder == QDataStream::BigEndian) {
        updated = qToBigEndian(updated);
    } else {
        updated = qToLittleEndian(updated);
    }
    return mBuffer->write(updated, mAddress);
}

template <typename T>
QVariant IOParameter<T>::readValue() const {
    BaseType value = static_cast<BaseType>(rawRead());
    if (mByteOrder == QDataStream::BigEndian) {
        value = qFromBigEndian(value);
    } else {
        value = qFromLittleEndian(value);
    }
    value &= mMask;
    if (mEnumHash.size() > 0) {
        return QVariant::fromValue(mEnumHash.key(static_cast<BaseType>(value)));
    }
    return QVariant::fromValue(static_cast<T>(value));
}