#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>
#include <QVariant>
#include <QVector>

#include "parameter.h"

inline size_t qHash(const QVariant& v, size_t seed = 0) noexcept {
    return qHashMulti(seed, v.toString(), v.typeId());
}

class ParameterRegistry : public QObject {
    Q_OBJECT
public:
    class ProfileRegistrationScope {
    public:
        explicit ProfileRegistrationScope(ParameterRegistry& registry);
        ~ProfileRegistrationScope();

        ProfileRegistrationScope(const ProfileRegistrationScope&) = delete;
        ProfileRegistrationScope& operator=(const ProfileRegistrationScope&) = delete;

    private:
        ParameterRegistry* mRegistry{nullptr};
    };

    explicit ParameterRegistry(QObject* parent = nullptr);
    ~ParameterRegistry() override;

    void registerParameter(Parameter* param);
    void registerProfileObject(QObject* object);
    void clearProfileScope();
    Parameter* parameter(const QVariant& name) const;

    QVariantList availableParameterNames() const;
    QVariant readParameter(const QVariant& name) const;
    QVariant availableValues(const QVariant& name) const;
    bool writeParameter(const QVariant& name, const QVariant& value);
    QVariantList writeParameters(const QVariantList& updates);
    void notifyAvailableParametersChanged();

signals:
    void parameterChanged(const QVariant& name, const QVariant& value);
    void availableParametersChanged();

private:
    QHash<QVariant, Parameter*> mParameters;
    QSet<QVariant> mProfileParameterNames;
    QList<QObject*> mProfileObjects;
    int mProfileRegistrationDepth{0};

    void beginProfileRegistration();
    void endProfileRegistration();
};
