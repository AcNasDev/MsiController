#include <QCoreApplication>
#include <QDBusConnection>
#include <QTest>

#include "dbusapi.h"
#include "dbuscodec.h"
#include "dbusendpoints.h"
#include "deviceprofileservice.h"
#include "diagnosticsservice.h"
#include "echealthinterface.h"
#include "ecmemoryinterface.h"
#include "ecmemoryservice.h"
#include "ecparametersinterface.h"
#include "ecprofilesinterface.h"
#include "healthadaptor.h"
#include "memoryadaptor.h"
#include "parameterregistry.h"
#include "parametersadaptor.h"
#include "profilesadaptor.h"

class MemoryParameter : public Parameter {
public:
    MemoryParameter(const QVariant& name, const QVariant& initialValue)
        : Parameter(name, QVariantList{initialValue}, false, nullptr, nullptr, Parameter::Persistence::Volatile),
          mStoredValue(initialValue) {
        update();
    }

protected:
    QVariant readValue() const override { return mStoredValue; }

    bool writeValue(const QVariant& value) override {
        mStoredValue = value;
        return true;
    }

private:
    QVariant mStoredValue;
};

class DbusContractTest : public QObject {
    Q_OBJECT

private slots:
    void splitEndpointsExposeVersionedContract() {
        registerMetaType();

        const QString serviceName = QStringLiteral("com.msi.ec.contract.p%1").arg(QCoreApplication::applicationPid());
        QDBusConnection connection = QDBusConnection::sessionBus();
        QVERIFY(connection.isConnected());
        QVERIFY(connection.registerService(serviceName));

        ParameterRegistry registry;
        registry.registerParameter(new MemoryParameter(QVariant::fromValue(Msi::Parametr::CpuTempEc),
                                                       QVariant::fromValue(static_cast<quint8>(61))));
        EcMemoryService memory;
        DeviceProfileService profiles;
        DiagnosticsService diagnostics(&registry, &memory, &profiles);

        EcParametersEndpoint parametersEndpoint(&registry);
        EcMemoryEndpoint memoryEndpoint(&memory);
        EcProfilesEndpoint profilesEndpoint(&profiles);
        EcHealthEndpoint healthEndpoint(&diagnostics);
        ParametersAdaptor parametersAdaptor(&parametersEndpoint);
        MemoryAdaptor memoryAdaptor(&memoryEndpoint);
        ProfilesAdaptor profilesAdaptor(&profilesEndpoint);
        HealthAdaptor healthAdaptor(&healthEndpoint);

        QVERIFY(connection.registerObject(QString::fromLatin1(MsiDbusApi::parametersPath), &parametersEndpoint));
        QVERIFY(connection.registerObject(QString::fromLatin1(MsiDbusApi::memoryPath), &memoryEndpoint));
        QVERIFY(connection.registerObject(QString::fromLatin1(MsiDbusApi::profilesPath), &profilesEndpoint));
        QVERIFY(connection.registerObject(QString::fromLatin1(MsiDbusApi::healthPath), &healthEndpoint));

        ComMsiEcParametersInterface parameters(serviceName,
                                               QString::fromLatin1(MsiDbusApi::parametersPath),
                                               connection);
        QCOMPARE(parameters.apiVersion().value(), MsiDbusApi::currentApiVersion);

        auto availableReply = parameters.availableParameters();
        availableReply.waitForFinished();
        QVERIFY(!availableReply.isError());
        const QVariantList names = MsiDbusCodec::unwrap(availableReply.argumentAt<0>()).toList();
        QCOMPARE(names.size(), 1);

        auto readReply = parameters.readParameter(MsiDbusCodec::wrapValue(Msi::Parametr::CpuTempEc));
        readReply.waitForFinished();
        QVERIFY(!readReply.isError());
        QCOMPARE(MsiDbusCodec::unwrap(readReply.argumentAt<0>()).toUInt(), 61u);

        ComMsiEcMemoryInterface memoryInterface(serviceName, QString::fromLatin1(MsiDbusApi::memoryPath), connection);
        auto memoryReply = memoryInterface.readEcMemory(MsiDbusCodec::wrap(0), MsiDbusCodec::wrap(1));
        memoryReply.waitForFinished();
        QVERIFY(!memoryReply.isError());
        QCOMPARE(MsiDbusCodec::unwrap(memoryReply.argumentAt<0>()).toMap().value(QStringLiteral("ok")).toBool(), false);

        ComMsiEcProfilesInterface profilesInterface(serviceName,
                                                    QString::fromLatin1(MsiDbusApi::profilesPath),
                                                    connection);
        QCOMPARE(profilesInterface.apiVersion().value(), MsiDbusApi::currentApiVersion);

        ComMsiEcHealthInterface health(serviceName, QString::fromLatin1(MsiDbusApi::healthPath), connection);
        QCOMPARE(health.apiVersion().value(), MsiDbusApi::currentApiVersion);
        auto diagnosticsReply = health.diagnostics();
        diagnosticsReply.waitForFinished();
        QVERIFY(!diagnosticsReply.isError());
        const QVariantMap healthMap = MsiDbusCodec::unwrap(diagnosticsReply.argumentAt<0>()).toMap();
        QCOMPARE(healthMap.value(QStringLiteral("apiVersion")).toInt(), MsiDbusApi::currentApiVersion);
        QCOMPARE(healthMap.value(QStringLiteral("parameterCount")).toInt(), 1);

        connection.unregisterObject(QString::fromLatin1(MsiDbusApi::parametersPath));
        connection.unregisterObject(QString::fromLatin1(MsiDbusApi::memoryPath));
        connection.unregisterObject(QString::fromLatin1(MsiDbusApi::profilesPath));
        connection.unregisterObject(QString::fromLatin1(MsiDbusApi::healthPath));
        connection.unregisterService(serviceName);
    }
};

QTEST_GUILESS_MAIN(DbusContractTest)

#include "dbuscontracttest.moc"
