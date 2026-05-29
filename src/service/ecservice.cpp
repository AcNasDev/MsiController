#include "ecservice.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusError>
#include <QDir>
#include <QFile>
#include <QPair>
#include <QVector>

#include <utility>

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

QDBusVariant EcService::writeParameters(const QDBusVariant& updates) {
    const auto values = qdbus_cast<Msi::Msg>(updates.variant()).variant.toList();
    QVariantList result;
    if (values.size() % 2 != 0) {
        qWarning() << "Invalid batch write payload size:" << values.size();
        return QDBusVariant(QVariant::fromValue(Msi::Msg(result)));
    }

    QVector<QPair<QVariant, QVariant>> requests;
    requests.reserve(values.size() / 2);
    for (qsizetype i = 0; i < values.size(); i += 2) {
        const QVariant name = values.at(i);
        const QVariant value = values.at(i + 1);
        if (!mParameters.contains(name) || mParameters[name]->isReadOnly()) {
            qWarning() << "Skipping invalid batch write parameter:" << name;
            continue;
        }
        requests.append(qMakePair(name, value));
    }

    for (const auto& request : std::as_const(requests)) {
        mParameters[request.first]->setValue(request.second);
    }

    for (const auto& request : std::as_const(requests)) {
        if (Parameter* parameter = mParameters.value(request.first, nullptr)) {
            result << request.first << parameter->value();
        }
    }

    return QDBusVariant(QVariant::fromValue(Msi::Msg(result)));
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
