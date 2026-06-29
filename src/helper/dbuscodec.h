#pragma once

#include <QDBusVariant>
#include <QVariant>

#include "struct.h"

namespace MsiDbusCodec {
inline QDBusVariant wrap(const QVariant& value) {
    return QDBusVariant(QVariant::fromValue(Msi::Msg(value)));
}

template <typename T>
QDBusVariant wrapValue(const T& value) {
    return wrap(QVariant::fromValue(value));
}

inline QVariant unwrap(const QDBusVariant& value) {
    return qdbus_cast<Msi::Msg>(value.variant()).variant;
}

inline QVariant unwrapReplyArgument(const QVariant& argument) {
    return qdbus_cast<Msi::Msg>(argument.value<QDBusVariant>().variant()).variant;
}
} // namespace MsiDbusCodec
