#include "parameter.h"

Parameter::Parameter(const QVariant &name, const QVariant &available, const bool &isReadOnly, QObject *parent)
    : QObject(parent), mName(name), mAvailable(available), mIsReadOnly(isReadOnly) 
{
}

void Parameter::setValue(const QVariant &value) 
{
    if(value == mValue) return;
    if(mIsReadOnly) return;
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