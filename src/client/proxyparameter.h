#pragma once

#include <QObject>

#include "struct.h"

class ProxyParameter : public QObject {
    Q_OBJECT

    Q_PROPERTY(Msi::Parametr parameter READ parameter WRITE setParameter NOTIFY parameterChanged)
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(QVariant availableValues READ availableValues NOTIFY availableValuesChanged)
    Q_PROPERTY(bool isValid READ isValid NOTIFY isValidChanged)

public:
    explicit ProxyParameter(QObject *parent = nullptr);

    Msi::Parametr parameter() const;
    void setParameter(Msi::Parametr param);

    QVariant value() const;
    void setValue(const QVariant &value);

    QVariant availableValues() const;
    void setAvailableValues(const QVariant &values);

    bool isValid() const;
    void setIsValid(bool valid);

    void setBlockSignalsForEsProxy(bool block) { mBlockSignalsForEsProxy = block; }
    bool blockSignalsForEsProxy() const { return mBlockSignalsForEsProxy; }

signals:
    void parameterChanged();
    void valueChanged();
    void availableValuesChanged();
    void isValidChanged();
    void valueChangedForEsProxy();

private:
    Msi::Parametr mParameter;
    QVariant mValue;
    QVariant mAvailableValues;
    bool mIsValid{ false };
    bool mBlockSignalsForEsProxy{ false };
};

