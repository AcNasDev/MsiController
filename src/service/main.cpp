#include <QCoreApplication>
#include <QDBusConnection>
#include <QDebug>

#include "dbusendpoints.h"
#include "ecservice.h"
#include "ecsysregistrar.h"
#include "gpucontrolparameter.h"
#include "memoryadaptor.h"
#include "parametersadaptor.h"
#include "profilesadaptor.h"
#include "struct.h"
#include "supportconfig.h"

namespace {
constexpr auto serviceName = "com.msi.ec";
constexpr auto parametersPath = "/Parameters";
constexpr auto memoryPath = "/Memory";
constexpr auto profilesPath = "/Profiles";

bool registerDbusObjects(QDBusConnection& connection,
                         EcParametersEndpoint& parametersEndpoint,
                         EcMemoryEndpoint& memoryEndpoint,
                         EcProfilesEndpoint& profilesEndpoint) {
    return connection.isConnected() &&
           connection.registerObject(QString::fromLatin1(parametersPath), &parametersEndpoint) &&
           connection.registerObject(QString::fromLatin1(memoryPath), &memoryEndpoint) &&
           connection.registerObject(QString::fromLatin1(profilesPath), &profilesEndpoint);
}
} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    registerMetaType();

    EcService service;
    auto* supportConfig = new SupportConfigRepository(&service);
    service.setSupportConfigRepository(supportConfig);

    EcParametersEndpoint parametersEndpoint(service.parameters(), &service);
    EcMemoryEndpoint memoryEndpoint(service.memory(), &service);
    EcProfilesEndpoint profilesEndpoint(service.profiles(), &service);
    ParametersAdaptor parametersAdaptor(&parametersEndpoint);
    MemoryAdaptor memoryAdaptor(&memoryEndpoint);
    ProfilesAdaptor profilesAdaptor(&profilesEndpoint);

    QDBusConnection connection = QDBusConnection::systemBus();
    if (!registerDbusObjects(connection, parametersEndpoint, memoryEndpoint, profilesEndpoint)) {
        qCritical("Failed to connect to system D-Bus or register service objects.");
        return 1;
    }
    if (!connection.registerService(QString::fromLatin1(serviceName))) {
        qCritical("Failed to register D-Bus service on system bus.");
        return 1;
    }
    qInfo("D-Bus service registered system successfully.");

    if (qEnvironmentVariableIsSet("MSICONTROLLER_DEBUG_EVENTS")) {
        QObject::connect(service.parameters(),
                         &ParameterRegistry::parameterChanged,
                         [](const QVariant& name, const QVariant& value) {
                             qDebug() << "Parameter changed:" << name << "new value:" << value;
                         });
    }

    service.registerParameter(new GpuControlParameter(QVariant::fromValue(Msi::Parametr::GpuControlConfig), &service));
    registerEcSys(service, *supportConfig);

    return app.exec();
}
