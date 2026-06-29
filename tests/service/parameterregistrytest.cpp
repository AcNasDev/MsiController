#include "parameterregistry.h"

#include <QSignalSpy>
#include <QTest>

class MemoryParameter : public Parameter {
public:
    MemoryParameter(const QVariant& name, const QVariant& initialValue, bool readOnly = false)
        : Parameter(name, QVariantList{initialValue}, readOnly, nullptr, nullptr, Parameter::Persistence::Volatile),
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

class ParameterRegistryTest : public QObject {
    Q_OBJECT

private slots:
    void registersAndReadsParameter() {
        ParameterRegistry registry;
        registry.registerParameter(new MemoryParameter(QStringLiteral("alpha"), 10));

        QCOMPARE(registry.availableParameterNames().size(), 1);
        QCOMPARE(registry.readParameter(QStringLiteral("alpha")).toInt(), 10);
        QCOMPARE(registry.availableValues(QStringLiteral("alpha")).toList().first().toInt(), 10);
    }

    void writesParameterAndPublishesChange() {
        ParameterRegistry registry;
        registry.registerParameter(new MemoryParameter(QStringLiteral("alpha"), 10));
        QSignalSpy spy(&registry, &ParameterRegistry::parameterChanged);

        QVERIFY(registry.writeParameter(QStringLiteral("alpha"), 42));

        QCOMPARE(registry.readParameter(QStringLiteral("alpha")).toInt(), 42);
        QCOMPARE(spy.size(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), QStringLiteral("alpha"));
        QCOMPARE(spy.at(0).at(1).toInt(), 42);
    }

    void skipsReadOnlyAndInvalidBatchWrites() {
        ParameterRegistry registry;
        registry.registerParameter(new MemoryParameter(QStringLiteral("alpha"), 10));
        registry.registerParameter(new MemoryParameter(QStringLiteral("readOnly"), 7, true));

        const QVariantList result = registry.writeParameters(
            {QStringLiteral("alpha"), 11, QStringLiteral("missing"), 12, QStringLiteral("readOnly"), 13});

        QCOMPARE(result.size(), 2);
        QCOMPARE(result.at(0).toString(), QStringLiteral("alpha"));
        QCOMPARE(result.at(1).toInt(), 11);
        QCOMPARE(registry.readParameter(QStringLiteral("readOnly")).toInt(), 7);
    }

    void clearsProfileScopeOnly() {
        ParameterRegistry registry;
        registry.registerParameter(new MemoryParameter(QStringLiteral("base"), 1));
        {
            ParameterRegistry::ProfileRegistrationScope scope(registry);
            registry.registerParameter(new MemoryParameter(QStringLiteral("profile"), 2));
        }

        QCOMPARE(registry.availableParameterNames().size(), 2);
        registry.clearProfileScope();

        QVERIFY(registry.parameter(QStringLiteral("base")));
        QVERIFY(!registry.parameter(QStringLiteral("profile")));
    }
};

QTEST_GUILESS_MAIN(ParameterRegistryTest)

#include "parameterregistrytest.moc"
