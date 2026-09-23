#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <memory>
#include <limits>

#include "ecservice.h"
#include "ecmemorybackend.h"
#include "iobuffer.h"
#include "ioparameter.h"
#include "ecsysregistrar.h"
#include "struct.h"
#include "supportconfig.h"

class FailingOnceEcBackend final : public EcMemoryBackend {
public:
    FailingOnceEcBackend() : mMemory(256, '\0') {}

    std::optional<QByteArray> readAll(const QMap<uint, QByteArray>& pendingWrites) override {
        if (!pendingWrites.isEmpty()) {
            ++mWriteAttempts;
            if (mWriteAttempts == 1) {
                return std::nullopt;
            }
            for (auto it = pendingWrites.cbegin(); it != pendingWrites.cend(); ++it) {
                mMemory.replace(static_cast<qsizetype>(it.key()), it.value().size(), it.value());
            }
        }
        return mMemory;
    }

    QString displayName() const override { return QStringLiteral("failing-once-test"); }
    int writeAttempts() const { return mWriteAttempts; }
    const QByteArray& memory() const { return mMemory; }

private:
    QByteArray mMemory;
    int mWriteAttempts{0};
};

class TestStringParameter : public IOParameterString {
public:
    using IOParameterString::IOParameterString;
    using IOParameterString::writeValue;
};

class FakeEcBackendTest : public QObject {
    Q_OBJECT

private slots:
    void fileBackendDoesNotCreateMissingDevice() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        const QString missingDevice = tempDir.filePath(QStringLiteral("missing-ec"));
        FileEcMemoryBackend backend(missingDevice);
        QVERIFY(!backend.readAll({}).has_value());
        QVERIFY(!QFile::exists(missingDevice));
    }

    void stringParameterWritesTextBytes() {
        IOBuffer buffer(std::make_unique<SimulatedEcMemoryBackend>());
        TestStringParameter parameter(&buffer, 0xc8, QStringLiteral("test-string"), {}, false, 8);
        QVERIFY(parameter.writeValue(QStringLiteral("abcdefghij")));
        QCOMPARE(buffer.buffer().mid(0xc8, 8), QByteArray("abcdefgh"));
    }

    void queuedWritesRespectBoundsAndRetryAfterBackendFailure() {
        auto backend = std::make_unique<FailingOnceEcBackend>();
        auto* backendPtr = backend.get();
        IOBuffer buffer(std::move(backend));
        QCOMPARE(buffer.buffer().size(), 256);

        QVERIFY(!buffer.writeBytes(QByteArray(2, 'x'), 255));
        QVERIFY(!buffer.writeBytes(QByteArray(1, 'x'), std::numeric_limits<uint>::max()));
        QCOMPARE(backendPtr->writeAttempts(), 0);

        QVERIFY(buffer.writeBytes(QByteArray(1, 'x'), 255));
        QTRY_COMPARE(backendPtr->writeAttempts(), 2);
        QCOMPARE(backendPtr->memory().at(255), 'x');
    }

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
