#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QImage>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLocale>
#include <QMetaObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTimer>
#include <QTranslator>
#include <QWindow>

#include "curveutils.h"
#include "esproxy.h"
#include "gpucpuperformancegraph.h"
#include "gpulinechart.h"
#include "proxyparameter.h"
#include "struct.h"

const char* DEFAULT_INSTANCE_KEY = "MsiControlCenterUniqueKey";
const char* INSTANCE_COMMAND_RAISE = "raise";
const char* INSTANCE_COMMAND_QUIT = "quit";

struct CommandLineOptions {
    bool smokeTest = false;
    bool quitExisting = false;
};

namespace {
CommandLineOptions parseCommandLine(int argc, char* argv[]) {
    CommandLineOptions options;
    for (int i = 1; i < argc; ++i) {
        const QString argument = QString::fromLocal8Bit(argv[i]);
        if (argument == QStringLiteral("--smoke-test")) {
            options.smokeTest = true;
        } else if (argument == QStringLiteral("--quit-existing")) {
            options.quitExisting = true;
        }
    }
    return options;
}

QString instanceServerName() {
    return qEnvironmentVariable("MSICONTROLLER_INSTANCE_KEY", DEFAULT_INSTANCE_KEY);
}

bool sendExistingInstanceCommand(const QByteArray& command, int timeoutMs) {
    QLocalSocket socket;
    socket.connectToServer(instanceServerName());
    if (!socket.waitForConnected(timeoutMs)) {
        return false;
    }

    socket.write(command);
    socket.flush();
    const bool written = socket.waitForBytesWritten(timeoutMs);
    socket.disconnectFromServer();
    return written;
}

QString executableDir(const char* argv0) {
    QFileInfo executable(QString::fromLocal8Bit(argv0 ? argv0 : ""));
    if (executable.isSymLink()) {
        const QString target = executable.symLinkTarget();
        const QFileInfo targetInfo(target);
        executable.setFile(targetInfo.isAbsolute() ? target : executable.dir().absoluteFilePath(target));
    }
    if (executable.isAbsolute()) {
        return executable.absolutePath();
    }
    return QString();
}

void prependEnvPath(const char* name, const QString& path) {
    if (path.isEmpty() || !QDir(path).exists()) {
        return;
    }

    const QByteArray current = qgetenv(name);
    QByteArray updated = QFile::encodeName(path);
    if (!current.isEmpty()) {
        updated += ':';
        updated += current;
    }
    qputenv(name, updated);
}

void raiseRootWindows(QQmlApplicationEngine& engine) {
    const auto rootObjs = engine.rootObjects();
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

void quitRootApplication(QQmlApplicationEngine& engine) {
    bool quitHandledByQml = false;
    const auto rootObjs = engine.rootObjects();
    for (QObject* object : rootObjs) {
        quitHandledByQml = QMetaObject::invokeMethod(object, "exitApplication") || quitHandledByQml;
    }

    if (!quitHandledByQml) {
        QCoreApplication::quit();
    }
}
} // namespace

int main(int argc, char* argv[]) {
    const CommandLineOptions options = parseCommandLine(argc, argv);

    if (options.quitExisting) {
        QCoreApplication app(argc, argv);
        sendExistingInstanceCommand(QByteArray(INSTANCE_COMMAND_QUIT), 1000);
        return 0;
    }

    if (options.smokeTest) {
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    }
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", QByteArray("xcb"));
    }

    const QString appDir = argc > 0 ? executableDir(argv[0]) : QString();
    const QString bundledPluginPath = QDir(appDir).absoluteFilePath("../qt/plugins");
    const QString bundledQmlPath = QDir(appDir).absoluteFilePath("../qt/qml");
    prependEnvPath("QT_PLUGIN_PATH", bundledPluginPath);
    prependEnvPath("QML_IMPORT_PATH", bundledQmlPath);
    prependEnvPath("QML2_IMPORT_PATH", bundledQmlPath);

    QApplication app(argc, argv);
    QLocalServer server;
    if (!options.smokeTest) {
        if (sendExistingInstanceCommand(QByteArray(INSTANCE_COMMAND_RAISE), 100)) {
            return 0;
        }

        const QString serverName = instanceServerName();
        server.removeServer(serverName);
        server.listen(serverName);
    }

    app.setApplicationName("MSI Control Center");
    app.setApplicationVersion(QString(CMAKE_TOOLS_GIT_TAG_MAJOR) + "." + QString(CMAKE_TOOLS_GIT_TAG_MINOR) + "." +
                              QString(CMAKE_TOOLS_GIT_TAG_PATCH));
    app.setOrganizationName("AcNas");
    app.setOrganizationDomain("acnas.net");
    app.setWindowIcon(QIcon(":/resources/icon/logo.svg"));

    QTranslator translator;
    if (QLocale::system().language() == QLocale::Russian &&
        translator.load(QStringLiteral(":/i18n/msicontroller_ru_RU.qm"))) {
        app.installTranslator(&translator);
    }

    qmlRegisterUncreatableMetaObject(Msi::staticMetaObject, "Msi", 1, 0, "Msi", "Enums only");
    qRegisterMetaType<ProxyParameter*>("ProxyParameter*");
    qmlRegisterType<EsProxy>("MsiController", 1, 0, "EsProxy");
    qmlRegisterUncreatableType<ProxyParameter>("MsiController",
                                               1,
                                               0,
                                               "ProxyParameter",
                                               "ProxyParameter instances are provided by EsProxy");
    qmlRegisterType<GpuCpuPerformanceGraph>("MsiController", 1, 0, "GpuCpuPerformanceGraph");
    qmlRegisterType<GpuLineChart>("MsiController", 1, 0, "GpuLineChart");
    qmlRegisterType<CurveUtils>("CurveUtils", 1, 0, "CurveUtils");
    qmlRegisterSingletonType<EnumHelper>("MSI.Helpers",
                                         1,
                                         0,
                                         "EnumHelper",
                                         [](QQmlEngine* engine, QJSEngine* scriptEngine) -> QObject* {
                                             Q_UNUSED(engine)
                                             Q_UNUSED(scriptEngine)
                                             return new EnumHelper();
                                         });

    QQmlApplicationEngine engine;
    if (QDir(bundledQmlPath).exists()) {
        engine.addImportPath(bundledQmlPath);
    }
    engine.rootContext()->setContextProperty("qtversion", QString(qVersion()));
    engine.rootContext()->setContextProperty("appversion",
                                             QString(CMAKE_TOOLS_GIT_TAG_MAJOR) + "." +
                                                 QString(CMAKE_TOOLS_GIT_TAG_MINOR) + "." +
                                                 QString(CMAKE_TOOLS_GIT_TAG_PATCH));
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
            const QByteArray msg = client->readAll().trimmed();
            if (msg == INSTANCE_COMMAND_RAISE) {
                raiseRootWindows(engine);
            } else if (msg == INSTANCE_COMMAND_QUIT) {
                quitRootApplication(engine);
            }
            client->disconnectFromServer();
        }
    });
    if (options.smokeTest) {
        QTimer::singleShot(1600, &app, [&app, &engine]() {
            bool nonBlankFrame = false;
            const auto rootObjs = engine.rootObjects();
            for (QObject* object : rootObjs) {
                auto* window = qobject_cast<QQuickWindow*>(object);
                if (!window) {
                    continue;
                }

                const QImage image = window->grabWindow();
                if (image.isNull()) {
                    continue;
                }
                const QString screenshotPath = qEnvironmentVariable("MSICONTROLLER_SMOKE_SCREENSHOT");
                if (!screenshotPath.isEmpty()) {
                    image.save(screenshotPath);
                }

                QRgb firstPixel = image.pixel(0, 0);
                for (int y = 0; y < image.height() && !nonBlankFrame; y += qMax(1, image.height() / 8)) {
                    for (int x = 0; x < image.width(); x += qMax(1, image.width() / 8)) {
                        if (image.pixel(x, y) != firstPixel) {
                            nonBlankFrame = true;
                            break;
                        }
                    }
                }
            }
            app.exit(nonBlankFrame ? 0 : 3);
        });
    }
    return app.exec();
}
