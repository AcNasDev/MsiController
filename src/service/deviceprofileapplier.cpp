#include "deviceprofileapplier.h"

#include <QDebug>

#include "ecparameterfactory.h"
#include "ecservice.h"
#include "parameter.h"
#include "struct.h"
#include "supportconfig.h"

DeviceProfileApplier::DeviceProfileApplier(EcService* service,
                                           IOBuffer* ioBuffer,
                                           SupportConfigRepository* supportConfig,
                                           QObject* parent)
    : QObject(parent), mService(service), mIoBuffer(ioBuffer), mSupportConfig(supportConfig) {}

bool DeviceProfileApplier::apply(QString* errorMessage) {
    if (!mService || !mIoBuffer || !mSupportConfig) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Device profile applier is not ready");
        }
        return false;
    }

    const auto firmwareParam = mService->parameter(QVariant::fromValue(Msi::Parametr::FirmwareVersionEc));
    const QString currentFw = firmwareParam ? firmwareParam->value().toString() : QString();
    qDebug() << "Current firmware version:" << currentFw;

    auto config = mSupportConfig->configForFirmware(currentFw);
    if (config.isEmpty()) {
        const QString message = QStringLiteral("No configuration found for firmware version: %1").arg(currentFw);
        qWarning() << message;
        if (errorMessage) {
            *errorMessage = message;
        }
        return false;
    }

    mService->clearProfileScope();
    EcService::ProfileRegistrationScope profileRegistration(*mService->parameters());
    EcParameterFactory(mIoBuffer).registerProfileParameters(*mService, config);
    return true;
}
