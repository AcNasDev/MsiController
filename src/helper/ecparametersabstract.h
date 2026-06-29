#pragma once

#include <QDBusVariant>
#include <QObject>

class EcParametersAbstract : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.msi.ec.Parameters")
public:
    explicit EcParametersAbstract(QObject* parent = nullptr) : QObject(parent) {}
    ~EcParametersAbstract() override = default;

public slots:
    Q_SCRIPTABLE virtual int apiVersion() const = 0;
    Q_SCRIPTABLE virtual QDBusVariant availableParameters() const = 0;
    Q_SCRIPTABLE virtual QDBusVariant readParameter(const QDBusVariant& name) const = 0;
    Q_SCRIPTABLE virtual bool writeParameter(const QDBusVariant& name, const QDBusVariant& value) = 0;
    Q_SCRIPTABLE virtual QDBusVariant writeParameters(const QDBusVariant& updates) = 0;
    Q_SCRIPTABLE virtual QDBusVariant availableValues(const QDBusVariant& name) const = 0;

signals:
    Q_SCRIPTABLE void parameterChanged(const QDBusVariant& name, const QDBusVariant& value);
    Q_SCRIPTABLE void availableParametersChanged();
};
