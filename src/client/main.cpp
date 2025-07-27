#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext> 
#include <QIcon>
#include <QLocalServer>
#include <QLocalSocket>
#include <QWindow>

#include "ecinterface.h"
#include "esproxy.h"
#include "curveutils.h"
#include "struct.h"

const char* UNIQUE_KEY = "MsiControlCenterUniqueKey";

int main(int argc, char *argv[])
{
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", QByteArray("xcb"));
    }

    QApplication app(argc, argv);
    QLocalSocket socket;
    socket.connectToServer(UNIQUE_KEY);
    if (socket.waitForConnected(100)) {
        socket.write("raise");
        socket.flush();
        socket.waitForBytesWritten(100);
        return 0;
    }

    QLocalServer server;
    server.removeServer(UNIQUE_KEY);
    server.listen(UNIQUE_KEY);

    app.setApplicationName("MSI Control Center");
    app.setApplicationVersion(QString(CMAKE_TOOLS_GIT_TAG_MAJOR) + "." +
                             QString(CMAKE_TOOLS_GIT_TAG_MINOR) + "." +
                             QString(CMAKE_TOOLS_GIT_TAG_PATCH));
    app.setOrganizationName("AcNas");
    app.setOrganizationDomain("acnas.net");
    app.setWindowIcon(QIcon(":/resources/icon/logo.svg"));
    qmlRegisterUncreatableMetaObject(Msi::staticMetaObject, "Msi", 1, 0, "Msi", "Enums only");
    qmlRegisterType<Msi::Range>("Msi", 1, 0, "Range");
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

    QObject::connect(&server, &QLocalServer::newConnection, &server, [&engine, &server]() {
        QLocalSocket* client = server.nextPendingConnection();
        if (client) {
            client->waitForReadyRead(100);
            QByteArray msg = client->readAll();
            if (msg == "raise") {
                auto rootObjs = engine.rootObjects();
                for (auto obj : std::as_const(rootObjs)) {
                    if (auto window = qobject_cast<QWindow*>(obj)) {
                        if (window->visibility() != QWindow::Windowed) {
                            window->showNormal();
                        }
                        window->raise();
                        window->requestActivate();
                    }
                }
            }
            client->disconnectFromServer();
        }
    });
    return app.exec();
}
