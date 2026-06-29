#include "ecservice.h"

#include <utility>

EcService::EcService(QObject* parent) : QObject(parent), mParameters(this), mMemory(this), mProfiles(this) {
    connect(&mProfiles,
            &DeviceProfileService::supportProfileApplied,
            &mParameters,
            &ParameterRegistry::notifyAvailableParametersChanged);
}

ParameterRegistry* EcService::parameters() {
    return &mParameters;
}

const ParameterRegistry* EcService::parameters() const {
    return &mParameters;
}

EcMemoryService* EcService::memory() {
    return &mMemory;
}

DeviceProfileService* EcService::profiles() {
    return &mProfiles;
}

void EcService::registerParameter(Parameter* param) {
    mParameters.registerParameter(param);
}

void EcService::registerProfileObject(QObject* object) {
    mParameters.registerProfileObject(object);
}

void EcService::clearProfileScope() {
    mParameters.clearProfileScope();
}

Parameter* EcService::parameter(const QVariant& name) const {
    return mParameters.parameter(name);
}

void EcService::setIoBuffer(IOBuffer* buffer) {
    mMemory.setIoBuffer(buffer);
}

void EcService::setSupportConfigRepository(SupportConfigRepository* repository) {
    mProfiles.setRepository(repository);
}

void EcService::setSupportProfileApplier(std::function<bool(QString*)> applier) {
    mProfiles.setProfileApplier(std::move(applier));
}

bool EcService::applySupportProfile(QString* errorMessage) {
    return mProfiles.applySupportProfile(errorMessage);
}
