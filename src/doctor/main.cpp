#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusReply>
#include <QFileInfo>
#include <QJsonDocument>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>
#include <QVariantMap>

#include "dbusapi.h"
#include "dbuscodec.h"
#include "echealthinterface.h"
#include "struct.h"

namespace {
QString appVersion() {
    return QString(CMAKE_TOOLS_GIT_TAG_MAJOR) + QStringLiteral(".") + QString(CMAKE_TOOLS_GIT_TAG_MINOR) +
           QStringLiteral(".") + QString(CMAKE_TOOLS_GIT_TAG_PATCH);
}

QVariantMap checkCommand(const QString& label, const QString& program, const QStringList& args) {
    QVariantMap result;
    result.insert(QStringLiteral("label"), label);
    result.insert(QStringLiteral("program"), program);

    const QString executable = QStandardPaths::findExecutable(program);
    if (executable.isEmpty()) {
        result.insert(QStringLiteral("ok"), false);
        result.insert(QStringLiteral("status"), QStringLiteral("missing"));
        return result;
    }

    QProcess process;
    process.start(executable, args);
    if (!process.waitForFinished(1500)) {
        process.kill();
        process.waitForFinished(300);
        result.insert(QStringLiteral("ok"), false);
        result.insert(QStringLiteral("status"), QStringLiteral("timeout"));
        return result;
    }

    result.insert(QStringLiteral("ok"), process.exitCode() == 0);
    result.insert(QStringLiteral("status"), QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed());
    result.insert(QStringLiteral("stderr"), QString::fromLocal8Bit(process.readAllStandardError()).trimmed());
    return result;
}

void printTextReport(const QVariantMap& report) {
    QTextStream out(stdout);
    out << "MSI Control Center doctor " << report.value(QStringLiteral("clientVersion")).toString() << '\n';
    out << "D-Bus service: "
        << (report.value(QStringLiteral("serviceRegistered")).toBool() ? "registered" : "not registered") << '\n';
    out << "API version: " << report.value(QStringLiteral("apiVersion")).toInt() << " expected "
        << MsiDbusApi::currentApiVersion << '\n';
    out << "Kernel module: " << (report.value(QStringLiteral("kernelModuleLoaded")).toBool() ? "loaded" : "missing")
        << '\n';

    const QVariantList checks = report.value(QStringLiteral("checks")).toList();
    for (const QVariant& value : checks) {
        const QVariantMap check = value.toMap();
        out << "- " << check.value(QStringLiteral("label")).toString() << ": "
            << (check.value(QStringLiteral("ok")).toBool() ? "ok" : "check") << " "
            << check.value(QStringLiteral("status")).toString() << '\n';
    }

    const QVariantList warnings = report.value(QStringLiteral("warnings")).toList();
    for (const QVariant& warning : warnings) {
        out << "warning: " << warning.toString() << '\n';
    }
}
} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    registerMetaType();

    const bool jsonOutput = app.arguments().contains(QStringLiteral("--json"));
    QVariantMap report;
    report.insert(QStringLiteral("clientVersion"), appVersion());
    report.insert(QStringLiteral("expectedApiVersion"), MsiDbusApi::currentApiVersion);
    report.insert(QStringLiteral("kernelModuleLoaded"), QFileInfo::exists(QStringLiteral("/sys/module/msiecmodule")));

    QVariantList checks;
    checks.append(checkCommand(QStringLiteral("systemd service"),
                               QStringLiteral("systemctl"),
                               {QStringLiteral("is-active"), QStringLiteral("msi-ec-service")}));
    checks.append(checkCommand(QStringLiteral("dkms"), QStringLiteral("dkms"), {QStringLiteral("status")}));
    checks.append(checkCommand(QStringLiteral("deb package"),
                               QStringLiteral("dpkg-query"),
                               {QStringLiteral("-W"), QStringLiteral("msicontroller")}));
    checks.append(checkCommand(QStringLiteral("rpm package"),
                               QStringLiteral("rpm"),
                               {QStringLiteral("-q"), QStringLiteral("msicontroller")}));
    report.insert(QStringLiteral("checks"), checks);

    QDBusConnection connection = QDBusConnection::systemBus();
    const bool serviceRegistered =
        connection.isConnected() &&
        connection.interface()->isServiceRegistered(QString::fromLatin1(MsiDbusApi::serviceName)).value();
    report.insert(QStringLiteral("serviceRegistered"), serviceRegistered);

    QVariantList warnings;
    if (serviceRegistered) {
        ComMsiEcHealthInterface health(QString::fromLatin1(MsiDbusApi::serviceName),
                                       QString::fromLatin1(MsiDbusApi::healthPath),
                                       connection);
        const int apiVersion = health.apiVersion().value();
        report.insert(QStringLiteral("apiVersion"), apiVersion);

        auto diagnosticsReply = health.diagnostics();
        diagnosticsReply.waitForFinished();
        if (!diagnosticsReply.isError()) {
            const QVariantMap diagnostics = MsiDbusCodec::unwrap(diagnosticsReply.argumentAt<0>()).toMap();
            report.insert(QStringLiteral("serviceDiagnostics"), diagnostics);
            warnings = diagnostics.value(QStringLiteral("warnings")).toList();
        }
    } else {
        warnings.append(QStringLiteral("D-Bus service is not registered on the system bus"));
    }
    report.insert(QStringLiteral("warnings"), warnings);

    if (jsonOutput) {
        QTextStream(stdout) << QJsonDocument::fromVariant(report).toJson(QJsonDocument::Indented);
    } else {
        printTextReport(report);
    }
    return serviceRegistered ? 0 : 2;
}
