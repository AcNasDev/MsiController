#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext> 

#include "ecinterface.h"
#include "esproxy.h"
#include "curveutils.h"

#include "struct.h"

#ifndef VERSION
#define VERSION "1.0.0"
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    qmlRegisterUncreatableMetaObject(Msi::staticMetaObject, "Msi", 1, 0, "Msi", "Enums only");
    qmlRegisterType<Msi::Range>("Msi", 1, 0, "range");
    qmlRegisterType<EsProxy>("MsiController", 1, 0, "EsProxy");
    qmlRegisterType<CurveUtils>("CurveUtils", 1, 0, "CurveUtils");
    qmlRegisterSingletonType<EnumHelper>(
    "MSI.Helpers", 1, 0, "EnumHelper",
    [](QQmlEngine* engine, QJSEngine* scriptEngine) -> QObject* {
        Q_UNUSED(engine)
        Q_UNUSED(scriptEngine)
        return new EnumHelper();
    }
    );
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("qtversion", QString(qVersion()));
    engine.rootContext()->setContextProperty("appversion", QString(VERSION));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("client", "Main");
    return app.exec();
}
