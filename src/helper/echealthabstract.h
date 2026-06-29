#pragma once

#include <QDBusVariant>
#include <QObject>

class EcHealthAbstract : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.msi.ec.Health")
public:
    explicit EcHealthAbstract(QObject* parent = nullptr) : QObject(parent) {}
    ~EcHealthAbstract() override = default;

public slots:
    Q_SCRIPTABLE virtual int apiVersion() const = 0;
    Q_SCRIPTABLE virtual QDBusVariant diagnostics() const = 0;
    Q_SCRIPTABLE virtual QDBusVariant supportBundle() const = 0;
};
