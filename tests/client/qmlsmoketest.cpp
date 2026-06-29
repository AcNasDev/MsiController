#include <QQmlComponent>
#include <QQmlEngine>
#include <QTest>

#include "esproxy.h"
#include "proxyparameter.h"
#include "struct.h"

class QmlSmokeTest : public QObject {
    Q_OBJECT

private slots:
    void proxyParameterReturnTypeIsVisibleToQml() {
        registerMetaType();
        qRegisterMetaType<ProxyParameter*>("ProxyParameter*");
        qmlRegisterUncreatableMetaObject(Msi::staticMetaObject, "Msi", 1, 0, "Msi", "Enums only");
        qmlRegisterType<EsProxy>("MsiController", 1, 0, "EsProxy");
        qmlRegisterUncreatableType<ProxyParameter>("MsiController",
                                                   1,
                                                   0,
                                                   "ProxyParameter",
                                                   "ProxyParameter instances are provided by EsProxy");

        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQml
            import Msi 1.0
            import MsiController 1.0
            QtObject {
                property var proxy: EsProxy {}
                property var cpu: proxy.getProxyParameter(Msi.Parametr.CpuTempEc)
                property bool ok: cpu !== undefined && cpu !== null &&
                                  cpu.parameter !== Msi.Parametr.CountEs &&
                                  proxy.behaviorProfiles.length >= 4 &&
                                  proxy.telemetryHistory !== undefined
            }
        )",
                          QUrl(QStringLiteral("qrc:/qmlsmoketest.qml")));

        QScopedPointer<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        QVERIFY(object->property("ok").toBool());
    }
};

QTEST_GUILESS_MAIN(QmlSmokeTest)

#include "qmlsmoketest.moc"
