#include "parameter.h"

#include <QSettings>
#include <QCoreApplication>
#include <QTimer>

Parameter::Parameter(const QVariant &name, const QVariant &available, const bool &isReadOnly, QObject *parent)
    : QObject(parent), mName(name), mAvailable(available), mIsReadOnly(isReadOnly) 
{
    QTimer::singleShot(0, this, [this]() {
        QSettings settings("/etc/MsiController/settings.ini", QSettings::IniFormat);
        settings.beginGroup("Parameters");
        if(settings.contains(mName.toString())) {
            setValue(settings.value(mName.toString()));
        }
        settings.endGroup();
    });

}

void Parameter::setValue(const QVariant &value) 
{
    if(value == mValue) return;
    if(mIsReadOnly) return;

    QSettings settings("/etc/MsiController/settings.ini", QSettings::IniFormat);
    settings.beginGroup("Parameters");
    qDebug() << "Setting value for parameter:" << mName << "to" << value;
    settings.setValue(mName.toString(), value);
    settings.endGroup();
    qDebug() << "Value set for parameter:" << mName;

    bool success{ writeValue(value) };
    if(success) {
        update();
    } else {
        qWarning() << "Failed to set value for parameter:" << mName;
    }
}

QVariant Parameter::value() const 
{
    return mValue;
}

QVariant Parameter::name() const 
{
    return mName;
}

QVariant Parameter::available() const 
{
    return mAvailable;
}

bool Parameter::isReadOnly() const 
{
    return mIsReadOnly;
}

void Parameter::update() 
{
    auto newValue = readValue();
    if(newValue != mValue) {
        mValue = newValue;
        emit valueChanged(mValue);
    }
}