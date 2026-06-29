#pragma once

#include <QDBusVariant>
#include <QObject>

class EcMemoryAbstract : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.msi.ec.Memory")
public:
    explicit EcMemoryAbstract(QObject* parent = nullptr) : QObject(parent) {}
    ~EcMemoryAbstract() override = default;

public slots:
    Q_SCRIPTABLE virtual QDBusVariant readEcMemory(const QDBusVariant& offset, const QDBusVariant& length) const = 0;
    Q_SCRIPTABLE virtual QDBusVariant writeEcMemory(const QDBusVariant& offset, const QDBusVariant& bytes) = 0;
    Q_SCRIPTABLE virtual QDBusVariant
    writeEcMemoryBits(const QDBusVariant& offset, const QDBusVariant& mask, const QDBusVariant& value) = 0;
};
