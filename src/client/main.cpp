#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext> 
#include <QIcon>

#include "ecinterface.h"
#include "esproxy.h"
#include "curveutils.h"
#include "struct.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("MSI Control Center");
    app.setApplicationVersion(QString(CMAKE_TOOLS_GIT_TAG_MAJOR) + "." +
                             QString(CMAKE_TOOLS_GIT_TAG_MINOR) + "." +
                             QString(CMAKE_TOOLS_GIT_TAG_PATCH));
    app.setOrganizationName("AcNas");
    app.setOrganizationDomain("acnas.net");
    app.setWindowIcon(QIcon(":/resources/icon/logo.svg"));
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
    engine.rootContext()->setContextProperty("appversion", QString(CMAKE_TOOLS_GIT_TAG_MAJOR) + "." +
        QString(CMAKE_TOOLS_GIT_TAG_MINOR) + "." + QString(CMAKE_TOOLS_GIT_TAG_PATCH));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("client", "Main");
    return app.exec();
}
