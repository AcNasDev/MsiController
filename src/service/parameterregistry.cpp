#include "parameterregistry.h"

#include <QDebug>
#include <QPair>
#include <utility>

ParameterRegistry::ProfileRegistrationScope::ProfileRegistrationScope(ParameterRegistry& registry)
    : mRegistry(&registry) {
    mRegistry->beginProfileRegistration();
}

ParameterRegistry::ProfileRegistrationScope::~ProfileRegistrationScope() {
    if (mRegistry) {
        mRegistry->endProfileRegistration();
    }
}

ParameterRegistry::ParameterRegistry(QObject* parent) : QObject(parent) {}

ParameterRegistry::~ParameterRegistry() {
    qDeleteAll(mProfileObjects);
    qDeleteAll(mParameters);
}

void ParameterRegistry::registerParameter(Parameter* param) {
    if (!param) {
        return;
    }
    if (mParameters.contains(param->name())) {
        qWarning() << "Duplicate parameter name:" << param->name();
        delete param;
        return;
    }

    mParameters.insert(param->name(), param);
    if (mProfileRegistrationDepth > 0) {
        mProfileParameterNames.insert(param->name());
    }
    param->setParent(this);

    connect(param, &Parameter::valueChanged, this, [this, param](const QVariant& value) {
        emit parameterChanged(param->name(), value);
    });
}

void ParameterRegistry::registerProfileObject(QObject* object) {
    if (!object) {
        return;
    }

    object->setParent(this);
    mProfileObjects.append(object);
}

void ParameterRegistry::clearProfileScope() {
    qDeleteAll(mProfileObjects);
    mProfileObjects.clear();

    for (const QVariant& name : std::as_const(mProfileParameterNames)) {
        if (Parameter* parameter = mParameters.take(name)) {
            delete parameter;
        }
    }
    mProfileParameterNames.clear();
}

Parameter* ParameterRegistry::parameter(const QVariant& name) const {
    return mParameters.value(name, nullptr);
}

QVariantList ParameterRegistry::availableParameterNames() const {
    QVariantList result;
    result.reserve(mParameters.size());
    for (auto it = mParameters.cbegin(); it != mParameters.cend(); ++it) {
        result.append(it.key());
    }
    return result;
}

QVariant ParameterRegistry::readParameter(const QVariant& name) const {
    if (Parameter* param = mParameters.value(name, nullptr)) {
        return param->value();
    }
    return {};
}

QVariant ParameterRegistry::availableValues(const QVariant& name) const {
    if (Parameter* param = mParameters.value(name, nullptr)) {
        return param->available();
    }
    return {};
}

bool ParameterRegistry::writeParameter(const QVariant& name, const QVariant& value) {
    Parameter* param = mParameters.value(name, nullptr);
    if (!param || param->isReadOnly()) {
        return false;
    }

    param->setValue(value);
    return true;
}

QVariantList ParameterRegistry::writeParameters(const QVariantList& updates) {
    QVariantList result;
    if (updates.size() % 2 != 0) {
        qWarning() << "Invalid batch write payload size:" << updates.size();
        return result;
    }

    QVector<QPair<QVariant, QVariant>> requests;
    requests.reserve(updates.size() / 2);
    for (qsizetype i = 0; i < updates.size(); i += 2) {
        const QVariant name = updates.at(i);
        const QVariant value = updates.at(i + 1);
        Parameter* param = mParameters.value(name, nullptr);
        if (!param || param->isReadOnly()) {
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
    return result;
}

void ParameterRegistry::notifyAvailableParametersChanged() {
    emit availableParametersChanged();
}

void ParameterRegistry::beginProfileRegistration() {
    ++mProfileRegistrationDepth;
}

void ParameterRegistry::endProfileRegistration() {
    mProfileRegistrationDepth = qMax(0, mProfileRegistrationDepth - 1);
}
