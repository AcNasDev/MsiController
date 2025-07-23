#pragma once
#include <QObject>
#include <QMap>
#include <QString>
#include <QVariant>
#include <QDBusContext>
#include <QDBusVariant>
#include <QTimer>
#include <QFileSystemWatcher>
#include <QHash>

#include "parameter.h"
#include "ecserviceabstract.h"

inline uint qHash(const QVariant &v, uint seed = 0)
{
    return qHashMulti(seed, v.toString(), v.typeId());
}

class EcService : public EcServiceAbstract {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.msi.ec")
public:
    explicit EcService(QObject* parent = nullptr);
    ~EcService();
    void registerParameter(Parameter* param);
    Parameter *parameter(const QVariant &name) const;

public slots:
    QDBusVariant availableParameters() const override;
    QDBusVariant readParameter(const QDBusVariant& name) const override;
    bool writeParameter(const QDBusVariant& name, const QDBusVariant& value) override;
    QDBusVariant availableValues(const QDBusVariant& name) const override;

private:
    QHash<QVariant, Parameter*> mParameters;
};