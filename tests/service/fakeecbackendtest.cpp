#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <memory>

#include "ecservice.h"
#include "ecsysregistrar.h"
#include "struct.h"
#include "supportconfig.h"

class FakeEcBackendTest : public QObject {
    Q_OBJECT

private slots:
    void fakeBackendRegistersRealProfileAndSafetyPolicy() {
        qputenv("MSICONTROLLER_FAKE_EC", QByteArray("1"));
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        qputenv("MSICONTROLLER_BUILTIN_DEVICE_CONFIG",
                QFile::encodeName(QStringLiteral(SOURCE_DIR) + QStringLiteral("/src/service/supported-devices.json")));
        qputenv("MSICONTROLLER_USER_DEVICE_CONFIG",
                QFile::encodeName(tempDir.filePath(QStringLiteral("devices.json"))));

        registerMetaType();
        EcService service;
        auto supportConfig = std::make_unique<SupportConfigRepository>();
        service.setSupportConfigRepository(supportConfig.get());

        QVERIFY(registerEcSys(service, *supportConfig));
        QVERIFY(service.memory()->backendDiagnostics().value(QStringLiteral("simulator")).toBool());
        QVERIFY(service.parameters()->parameter(QVariant::fromValue(Msi::Parametr::CpuTempEc)));
        QVERIFY(service.memory()->writePolicy()->allowedRangesForDbus().size() > 0);

        QTRY_VERIFY(service.parameters()->readParameter(QVariant::fromValue(Msi::Parametr::CpuTempEc)).toInt() > 0);

        const QVariantMap allowed = service.memory()->writeMemory(0xf2, QByteArray(1, static_cast<char>(0xc2)));
        QVERIFY2(allowed.value(QStringLiteral("ok")).toBool(),
                 qPrintable(allowed.value(QStringLiteral("error")).toString()));

        const QVariantMap denied = service.memory()->writeMemory(0xa0, QByteArray(1, static_cast<char>('X')));
        QVERIFY(!denied.value(QStringLiteral("ok")).toBool());
        QVERIFY(denied.value(QStringLiteral("error")).toString().contains(QStringLiteral("allowlist")));

        qunsetenv("MSICONTROLLER_FAKE_EC");
        qunsetenv("MSICONTROLLER_BUILTIN_DEVICE_CONFIG");
        qunsetenv("MSICONTROLLER_USER_DEVICE_CONFIG");
    }
};

QTEST_MAIN(FakeEcBackendTest)

#include "fakeecbackendtest.moc"
