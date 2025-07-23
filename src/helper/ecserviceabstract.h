#pragma once
#include <QObject>
#include <QDBusContext>
#include <QDBusVariant>

class EcServiceAbstract : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.msi.ec")
public:
    explicit EcServiceAbstract(QObject* parent = nullptr) : QObject(parent) {};
    ~EcServiceAbstract() override = default;

public slots:
    Q_SCRIPTABLE virtual QDBusVariant availableParameters() const = 0;
    Q_SCRIPTABLE virtual QDBusVariant readParameter(const QDBusVariant& name) const = 0;
    Q_SCRIPTABLE virtual bool writeParameter(const QDBusVariant& name, const QDBusVariant& value) = 0;
    Q_SCRIPTABLE virtual QDBusVariant availableValues(const QDBusVariant& name) const = 0;

signals:
    Q_SCRIPTABLE void parameterChanged(const QDBusVariant& name, const QDBusVariant& value);
};