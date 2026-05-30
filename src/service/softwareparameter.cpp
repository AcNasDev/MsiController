#include "softwareparameter.h"

#include <QTimer>

SoftwareParameter::SoftwareParameter(const QVariant& name,
                                     const QVariant& available,
                                     const QVariant& defaultValue,
                                     QObject* parent)
    : Parameter(name, available, false, parent), mValue(defaultValue) {
    QTimer::singleShot(0, this, [this]() { update(); });
}

QVariant SoftwareParameter::readValue() const {
    return mValue;
}

bool SoftwareParameter::writeValue(const QVariant& value) {
    mValue = value;
    return true;
}
