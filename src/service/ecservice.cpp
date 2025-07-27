#include "ecservice.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusError>
#include <QDir>
#include <QFile>

#include "struct.h"

EcService::EcService(QObject* parent) : EcServiceAbstract(parent) {}

EcService::~EcService() {
    qDeleteAll(mParameters);
}

void EcService::registerParameter(Parameter* param) {
    if (mParameters.contains(param->name())) {
        qWarning() << "Duplicate parameter name:" << param->name();
        delete param;
        return;
    }

    mParameters.insert(param->name(), param);
    param->setParent(this);

    connect(param, &Parameter::valueChanged, this, [this, param](const QVariant& value) {
        emit parameterChanged(QDBusVariant(QVariant::fromValue(Msi::Msg(param->name()))),
                              QDBusVariant(QVariant::fromValue(Msi::Msg(value))));
    });
}

QDBusVariant EcService::availableParameters() const {
    return QDBusVariant(QVariant::fromValue(Msi::Msg(mParameters.keys())));
}

QDBusVariant EcService::readParameter(const QDBusVariant& name) const {
    QVariant arg = qdbus_cast<Msi::Msg>(name.variant()).variant;
    if (mParameters.contains(arg)) {
        return QDBusVariant(QVariant::fromValue(Msi::Msg(mParameters[arg]->value())));
    }
    return {};
}

bool EcService::writeParameter(const QDBusVariant& name, const QDBusVariant& value) {
    auto vName = qdbus_cast<Msi::Msg>(name.variant()).variant;
    auto vValue = qdbus_cast<Msi::Msg>(value.variant()).variant;
    if (mParameters.contains(vName) && !mParameters[vName]->isReadOnly()) {
        mParameters[vName]->setValue(vValue);
        return true;
    }
    return false;
}

QDBusVariant EcService::availableValues(const QDBusVariant& name) const {
    QVariant arg = qdbus_cast<Msi::Msg>(name.variant()).variant;
    if (mParameters.contains(arg)) {
        return QDBusVariant(QVariant::fromValue(Msi::Msg(mParameters[arg]->available())));
    }
    return {};
}

Parameter* EcService::parameter(const QVariant& name) const {
    return mParameters.value(name, nullptr);
}