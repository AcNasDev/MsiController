#include <QCoreApplication>
#include <QDBusConnection>
#include <QDebug>

#include "dbusapi.h"
#include "dbusendpoints.h"
#include "ecservice.h"
#include "ecsysregistrar.h"
#include "gpucontrolparameter.h"
#include "healthadaptor.h"
#include "logging.h"
#include "memoryadaptor.h"
#include "parametersadaptor.h"
#include "profilesadaptor.h"
#include "servicehealthnotifier.h"
#include "struct.h"
#include "supportconfig.h"

namespace {
bool registerDbusObjects(QDBusConnection& connection,
                         EcParametersEndpoint& parametersEndpoint,
                         EcMemoryEndpoint& memoryEndpoint,
                         EcProfilesEndpoint& profilesEndpoint,
                         EcHealthEndpoint& healthEndpoint) {
    return connection.isConnected() &&
           connection.registerObject(QString::fromLatin1(MsiDbusApi::parametersPath), &parametersEndpoint) &&
           connection.registerObject(QString::fromLatin1(MsiDbusApi::memoryPath), &memoryEndpoint) &&
           connection.registerObject(QString::fromLatin1(MsiDbusApi::profilesPath), &profilesEndpoint) &&
           connection.registerObject(QString::fromLatin1(MsiDbusApi::healthPath), &healthEndpoint);
}
} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    if (app.arguments().contains(QStringLiteral("--fake-ec"))) {
        qputenv("MSICONTROLLER_FAKE_EC", QByteArray("1"));
    }
    registerMetaType();

    EcService service;
    auto* supportConfig = new SupportConfigRepository(&service);
    service.setSupportConfigRepository(supportConfig);

    EcParametersEndpoint parametersEndpoint(service.parameters(), &service);
    EcMemoryEndpoint memoryEndpoint(service.memory(), &service);
    EcProfilesEndpoint profilesEndpoint(service.profiles(), &service);
    EcHealthEndpoint healthEndpoint(service.diagnostics(), &service);
    ParametersAdaptor parametersAdaptor(&parametersEndpoint);
    MemoryAdaptor memoryAdaptor(&memoryEndpoint);
    ProfilesAdaptor profilesAdaptor(&profilesEndpoint);
    HealthAdaptor healthAdaptor(&healthEndpoint);

    QDBusConnection connection = QDBusConnection::systemBus();
    if (!registerDbusObjects(connection, parametersEndpoint, memoryEndpoint, profilesEndpoint, healthEndpoint)) {
        qCCritical(msiDbusLog) << "Failed to connect to system D-Bus or register service objects.";
        return 1;
    }
    if (!connection.registerService(QString::fromLatin1(MsiDbusApi::serviceName))) {
        qCCritical(msiDbusLog) << "Failed to register D-Bus service on system bus.";
        return 1;
    }
    qCInfo(msiDbusLog) << "D-Bus service registered successfully";

    if (qEnvironmentVariableIsSet("MSICONTROLLER_DEBUG_EVENTS")) {
        QObject::connect(service.parameters(),
                         &ParameterRegistry::parameterChanged,
                         [](const QVariant& name, const QVariant& value) {
                             qCDebug(msiServiceLog) << "Parameter changed:" << name << "new value:" << value;
                         });
    }

    service.registerParameter(new GpuControlParameter(QVariant::fromValue(Msi::Parametr::GpuControlConfig), &service));
    registerEcSys(service, *supportConfig);

    ServiceHealthNotifier healthNotifier;
    healthNotifier.notifyReady();
    healthNotifier.startWatchdog();

    return app.exec();
}
