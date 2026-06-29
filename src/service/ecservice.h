#pragma once

#include <QObject>
#include <QString>
#include <functional>

#include "deviceprofileservice.h"
#include "ecmemoryservice.h"
#include "parameterregistry.h"

class IOBuffer;
class Parameter;
class SupportConfigRepository;

class EcService : public QObject {
    Q_OBJECT
public:
    using ProfileRegistrationScope = ParameterRegistry::ProfileRegistrationScope;

    explicit EcService(QObject* parent = nullptr);

    ParameterRegistry* parameters();
    const ParameterRegistry* parameters() const;
    EcMemoryService* memory();
    DeviceProfileService* profiles();

    void registerParameter(Parameter* param);
    void registerProfileObject(QObject* object);
    void clearProfileScope();
    Parameter* parameter(const QVariant& name) const;
    void setIoBuffer(IOBuffer* buffer);
    void setSupportConfigRepository(SupportConfigRepository* repository);
    void setSupportProfileApplier(std::function<bool(QString*)> applier);
    bool applySupportProfile(QString* errorMessage = nullptr);

private:
    ParameterRegistry mParameters;
    EcMemoryService mMemory;
    DeviceProfileService mProfiles;
};
