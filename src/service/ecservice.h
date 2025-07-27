#pragma once
#include <QDBusContext>
#include <QDBusVariant>
#include <QFileSystemWatcher>
#include <QHash>
#include <QMap>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariant>

#include "ecserviceabstract.h"
#include "parameter.h"

inline uint qHash(const QVariant& v, uint seed = 0) {
    return qHashMulti(seed, v.toString(), v.typeId());
}

class EcService : public EcServiceAbstract {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.msi.ec")
public:
    explicit EcService(QObject* parent = nullptr);
    ~EcService();
    void registerParameter(Parameter* param);
    Parameter* parameter(const QVariant& name) const;

    QDBusVariant availableParameters() const override;
    QDBusVariant readParameter(const QDBusVariant& name) const override;
    QDBusVariant availableValues(const QDBusVariant& name) const override;
    bool writeParameter(const QDBusVariant& name, const QDBusVariant& value) override;

private:
    QHash<QVariant, Parameter*> mParameters;
};