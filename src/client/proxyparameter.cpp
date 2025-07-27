#include "proxyparameter.h"

ProxyParameter::ProxyParameter(QObject* parent) : QObject(parent) {}

Msi::Parametr ProxyParameter::parameter() const {
    return mParameter;
}

void ProxyParameter::setParameter(Msi::Parametr param) {
    if (mParameter != param) {
        mParameter = param;
        emit parameterChanged();
    }
}

QVariant ProxyParameter::value() const {
    return mValue;
}

void ProxyParameter::setValue(const QVariant& value) {
    if (mValue != value) {
        mValue = value;
        emit valueChanged();
        if (!mBlockSignalsForEsProxy) {
            emit valueChangedForEsProxy();
        }
    }
}

QVariant ProxyParameter::availableValues() const {
    return mAvailableValues;
}

void ProxyParameter::setAvailableValues(const QVariant& values) {
    if (mAvailableValues != values) {
        mAvailableValues = values;
        emit availableValuesChanged();
    }
}

bool ProxyParameter::isValid() const {
    return mIsValid;
}

void ProxyParameter::setIsValid(bool valid) {
    if (mIsValid != valid) {
        mIsValid = valid;
        emit isValidChanged();
    }
}
