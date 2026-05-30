#include "parameter.h"

#include <QCoreApplication>
#include <QSettings>
#include <QTimer>

Parameter::Parameter(const QVariant& name, const QVariant& available, bool isReadOnly, QObject* parent)
    : QObject(parent), mName(name), mAvailable(available), mIsReadOnly(isReadOnly) {
    QTimer::singleShot(0, this, [this]() {
        QSettings settings("/etc/MsiController/settings.ini", QSettings::IniFormat);
        settings.beginGroup("Parameters");
        if (settings.contains(mName.toString())) {
            setValue(settings.value(mName.toString()));
        }
        settings.endGroup();
    });
}

void Parameter::setValue(const QVariant& value) {
    if (value == mValue)
        return;
    if (mIsReadOnly)
        return;

    if (qEnvironmentVariableIsSet("MSICONTROLLER_DEBUG_WRITES")) {
        qDebug() << "Setting value for parameter:" << mName << "to" << value;
    }

    bool success{writeValue(value)};
    if (success) {
        QSettings settings("/etc/MsiController/settings.ini", QSettings::IniFormat);
        settings.beginGroup("Parameters");
        settings.setValue(mName.toString(), value);
        settings.endGroup();
        if (qEnvironmentVariableIsSet("MSICONTROLLER_DEBUG_WRITES")) {
            qDebug() << "Value set for parameter:" << mName;
        }
        update();
    } else {
        qWarning() << "Failed to set value for parameter:" << mName;
    }
}

QVariant Parameter::value() const {
    return mValue;
}

QVariant Parameter::name() const {
    return mName;
}

QVariant Parameter::available() const {
    return mAvailable;
}

bool Parameter::isReadOnly() const {
    return mIsReadOnly;
}

void Parameter::update() {
    auto newValue = readValue();
    publishValue(newValue);
}

bool Parameter::publishValue(const QVariant& value) {
    if (value == mValue)
        return false;

    mValue = value;
    emit valueChanged(mValue);
    return true;
}
