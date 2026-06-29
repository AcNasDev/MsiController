#include "ecsysregistrar.h"

#include <QDebug>

#include "cpucontrolparameter.h"
#include "cpuparameter.h"
#include "deviceprofileapplier.h"
#include "ecparameterfactory.h"
#include "ecservice.h"
#include "iobuffer.h"
#include "struct.h"
#include "systemaccess.h"

namespace {
QString findEcBuffer(SystemAccess& access) {
    QString bufferName{QStringLiteral("/sys/devices/platform/msiec/ec")};
    if (access.files().exists(bufferName)) {
        return bufferName;
    }

    bufferName = QStringLiteral("/sys/kernel/debug/ec/ec0/io");
    const QString modprobe = access.processes().findExecutable(QStringLiteral("modprobe"));
    access.processes().run(modprobe.isEmpty() ? QStringLiteral("modprobe") : modprobe,
                           {QStringLiteral("ec_sys"), QStringLiteral("write_support=1")},
                           1000);
    return access.files().exists(bufferName) ? bufferName : QString();
}
} // namespace

bool registerEcSys(EcService& service, SupportConfigRepository& supportConfig, SystemAccess* systemAccess) {
    SystemAccess& access = systemAccess ? *systemAccess : defaultSystemAccess();
    const QString bufferName = findEcBuffer(access);
    if (bufferName.isEmpty()) {
        qWarning() << "EC sysfs path does not exist";
        return false;
    }

    IOBuffer* ioBuffer = new IOBuffer(bufferName, &service);
    service.setIoBuffer(ioBuffer);
    const bool isIoBufferReady = ioBuffer->buffer().size() > 0;
    qDebug() << "IOBuffer size:" << ioBuffer->buffer().size() << ioBuffer->buffer().mid(0xa0, 12);

    if (isIoBufferReady) {
        EcParameterFactory factory(ioBuffer);
        factory.registerFirmwareParameters(service);
        emit ioBuffer->bufferChanged(ioBuffer->buffer());

        auto* profileApplier = new DeviceProfileApplier(&service, ioBuffer, &supportConfig, &service);
        service.setSupportProfileApplier(
            [profileApplier](QString* errorMessage) { return profileApplier->apply(errorMessage); });
        QString applyError;
        if (!service.applySupportProfile(&applyError)) {
            qWarning() << applyError;
        }
    }

    service.registerParameter(new CpuParameter(QVariant::fromValue(Msi::Parametr::CpuConfig), &service));
    service.registerParameter(new CpuControlParameter(QVariant::fromValue(Msi::Parametr::CpuControlConfig), &service));
    service.parameters()->notifyAvailableParametersChanged();
    return true;
}
