#pragma once

#include "parameter.h"
#include <QFile>
#include <QTimer>
#include <QFileSystemWatcher>

#include "iobuffer.h"

template <typename T, typename Enable = void>
struct base_type_helper 
{
    using type = T;
};

template <typename T>
struct base_type_helper<T, std::enable_if_t<std::is_enum_v<T>>> 
{
    using type = std::underlying_type_t<T>;
};

template <typename T>
using base_type = typename base_type_helper<T>::type;

template<typename T>
class IOParameter : public Parameter {
public:
    using BaseType = base_type<T>;

    explicit IOParameter(IOBuffer* buffer,
        quint16 address,
        const QVariant& name,
        const QVariant& available,
        bool isReadOnly,
        BaseType mask = std::numeric_limits<BaseType>::max(),
        QDataStream::ByteOrder byteOrder = QDataStream::LittleEndian,
        QObject* parent = nullptr);

    ~IOParameter() = default;
    T rawRead() const;
    void setEnumHash(const QHash<T, BaseType>& hash);

protected:
    bool writeValue(const QVariant& value) override;
    QVariant readValue() const override;

private:
    IOBuffer* mBuffer;
    quint16 mAddress;
    BaseType mMask;
    QDataStream::ByteOrder mByteOrder;
    QHash<T, BaseType> mEnumHash;
};

class IOParameterString : public Parameter {
public:
    IOParameterString(IOBuffer* buffer, quint16 address, const QVariant& name, const QVariant& available, bool isReadOnly, quint16 length, QObject* parent = nullptr);
    ~IOParameterString() = default;
    QString rawRead() const;

protected:
    bool writeValue(const QVariant& value) override;
    QVariant readValue() const override;
private:
    IOBuffer* mBuffer;
    quint16 mAddress;
    quint16 mLength;
};

#include "ioparameterimpl.h"
