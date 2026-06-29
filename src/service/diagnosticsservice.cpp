#include "diagnosticsservice.h"

#include <QDateTime>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QSysInfo>
#include <QtGlobal>

#include "dbusapi.h"
#include "deviceprofileservice.h"
#include "ecmemoryservice.h"
#include "parameterregistry.h"
#include "struct.h"

namespace {
QString appVersion() {
    return QString(CMAKE_TOOLS_GIT_TAG_MAJOR) + QStringLiteral(".") + QString(CMAKE_TOOLS_GIT_TAG_MINOR) +
           QStringLiteral(".") + QString(CMAKE_TOOLS_GIT_TAG_PATCH);
}

QString isoNow() {
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}

QVariantMap commandSnapshot(const QString& title, const QString& program, const QStringList& arguments) {
    QVariantMap result;
    result.insert(QStringLiteral("title"), title);
    result.insert(QStringLiteral("program"), program);
    result.insert(QStringLiteral("arguments"), arguments);

    const QString executable = QStandardPaths::findExecutable(program);
    if (executable.isEmpty()) {
        result.insert(QStringLiteral("available"), false);
        result.insert(QStringLiteral("error"), QStringLiteral("Executable not found"));
        return result;
    }

    QProcess process;
    process.start(executable, arguments);
    if (!process.waitForFinished(1800)) {
        process.kill();
        process.waitForFinished(300);
        result.insert(QStringLiteral("available"), true);
        result.insert(QStringLiteral("timedOut"), true);
        result.insert(QStringLiteral("error"), QStringLiteral("Command timed out"));
        return result;
    }

    result.insert(QStringLiteral("available"), true);
    result.insert(QStringLiteral("exitCode"), process.exitCode());
    result.insert(QStringLiteral("stdout"), QString::fromLocal8Bit(process.readAllStandardOutput()).left(12000));
    result.insert(QStringLiteral("stderr"), QString::fromLocal8Bit(process.readAllStandardError()).left(4000));
    return result;
}

QVariantList hostCommandSnapshots() {
    return {
        commandSnapshot(QStringLiteral("systemd service"),
                        QStringLiteral("systemctl"),
                        {QStringLiteral("status"), QStringLiteral("msi-ec-service"), QStringLiteral("--no-pager")}),
        commandSnapshot(QStringLiteral("service journal"),
                        QStringLiteral("journalctl"),
                        {QStringLiteral("-u"),
                         QStringLiteral("msi-ec-service"),
                         QStringLiteral("-n"),
                         QStringLiteral("120"),
                         QStringLiteral("--no-pager")}),
        commandSnapshot(QStringLiteral("dkms status"), QStringLiteral("dkms"), {QStringLiteral("status")}),
        commandSnapshot(QStringLiteral("loaded modules"), QStringLiteral("lsmod"), {}),
        commandSnapshot(QStringLiteral("module info"), QStringLiteral("modinfo"), {QStringLiteral("msiecmodule")}),
        commandSnapshot(QStringLiteral("deb package"),
                        QStringLiteral("dpkg-query"),
                        {QStringLiteral("-W"), QStringLiteral("msicontroller")}),
        commandSnapshot(QStringLiteral("rpm package"),
                        QStringLiteral("rpm"),
                        {QStringLiteral("-q"), QStringLiteral("msicontroller")}),
    };
}

QVariantList currentWarnings(bool moduleLoaded,
                             int parameterCount,
                             const QVariantMap& activeProfile,
                             const QVariantMap& memoryStatus,
                             bool simulator) {
    QVariantList warnings;
    if (simulator) {
        warnings.append(QStringLiteral("EC simulator is active; hardware writes are not touching a real controller"));
    } else if (!moduleLoaded) {
        warnings.append(QStringLiteral("Kernel module msiecmodule is not loaded"));
    }
    if (parameterCount == 0) {
        warnings.append(QStringLiteral("No EC parameters are available"));
    }
    if (activeProfile.isEmpty()) {
        warnings.append(QStringLiteral("No active supported-device profile was matched"));
    }
    if (!memoryStatus.value(QStringLiteral("ok")).toBool()) {
        const QString error = memoryStatus.value(QStringLiteral("error")).toString();
        warnings.append(error.isEmpty() ? QStringLiteral("EC memory is unavailable")
                                        : QStringLiteral("EC memory is unavailable: %1").arg(error));
    }
    return warnings;
}
} // namespace

DiagnosticsService::DiagnosticsService(ParameterRegistry* parameters,
                                       EcMemoryService* memory,
                                       DeviceProfileService* profiles,
                                       QObject* parent)
    : QObject(parent), mParameters(parameters), mMemory(memory), mProfiles(profiles) {}

QVariantMap DiagnosticsService::diagnostics() const {
    const bool moduleLoaded = kernelModuleLoaded();
    const int parameterCount = mParameters ? mParameters->availableParameterNames().size() : 0;
    const QVariantMap activeProfile = mProfiles ? mProfiles->activeProfile() : QVariantMap{};
    const QVariantMap memoryStatus = mMemory ? mMemory->readMemory(0, 0) : QVariantMap{};
    const QVariantMap memoryBackend = mMemory ? mMemory->backendDiagnostics() : QVariantMap{};
    const bool simulator = memoryBackend.value(QStringLiteral("simulator")).toBool();

    QVariantMap result;
    result.insert(QStringLiteral("apiVersion"), MsiDbusApi::currentApiVersion);
    result.insert(QStringLiteral("serviceVersion"), appVersion());
    result.insert(QStringLiteral("qtVersion"), QString::fromLatin1(qVersion()));
    result.insert(QStringLiteral("generatedAt"), isoNow());
    result.insert(QStringLiteral("kernelModuleLoaded"), moduleLoaded);
    result.insert(QStringLiteral("parameterCount"), parameterCount);
    result.insert(QStringLiteral("firmware"), firmwareInfo());
    result.insert(QStringLiteral("activeProfile"), activeProfile);
    result.insert(QStringLiteral("memory"), memoryStatus);
    result.insert(QStringLiteral("memoryBackend"), memoryBackend);
    result.insert(QStringLiteral("ecWritePolicy"),
                  mMemory && mMemory->writePolicy() ? mMemory->writePolicy()->diagnostics() : QVariantMap{});
    result.insert(QStringLiteral("warnings"),
                  currentWarnings(moduleLoaded, parameterCount, activeProfile, memoryStatus, simulator));
    result.insert(QStringLiteral("system"), systemInfo());

    QVariantMap dbus;
    dbus.insert(QStringLiteral("service"), QString::fromLatin1(MsiDbusApi::serviceName));
    dbus.insert(QStringLiteral("parametersPath"), QString::fromLatin1(MsiDbusApi::parametersPath));
    dbus.insert(QStringLiteral("memoryPath"), QString::fromLatin1(MsiDbusApi::memoryPath));
    dbus.insert(QStringLiteral("profilesPath"), QString::fromLatin1(MsiDbusApi::profilesPath));
    dbus.insert(QStringLiteral("healthPath"), QString::fromLatin1(MsiDbusApi::healthPath));
    result.insert(QStringLiteral("dbus"), dbus);

    return result;
}

QVariantMap DiagnosticsService::supportBundle() const {
    QVariantMap result;
    result.insert(QStringLiteral("diagnostics"), diagnostics());
    result.insert(QStringLiteral("deviceProfiles"), mProfiles ? mProfiles->supportedProfiles() : QVariantList{});
    result.insert(QStringLiteral("activeDeviceProfile"), mProfiles ? mProfiles->activeProfile() : QVariantMap{});
    result.insert(QStringLiteral("hostCommandSnapshots"), hostCommandSnapshots());
    return result;
}

QVariantMap DiagnosticsService::firmwareInfo() const {
    QVariantMap result;
    if (!mParameters) {
        return result;
    }

    result.insert(QStringLiteral("version"),
                  mParameters->readParameter(QVariant::fromValue(Msi::Parametr::FirmwareVersionEc)));
    result.insert(QStringLiteral("date"),
                  mParameters->readParameter(QVariant::fromValue(Msi::Parametr::FirmwareReleaseDateEc)));
    result.insert(QStringLiteral("time"),
                  mParameters->readParameter(QVariant::fromValue(Msi::Parametr::FirmwareReleaseTimeEc)));
    return result;
}

QVariantMap DiagnosticsService::systemInfo() const {
    QVariantMap result;
    result.insert(QStringLiteral("kernel"), QSysInfo::kernelType() + QStringLiteral(" ") + QSysInfo::kernelVersion());
    result.insert(QStringLiteral("product"), QSysInfo::prettyProductName());
    result.insert(QStringLiteral("architecture"), QSysInfo::currentCpuArchitecture());
    return result;
}

bool DiagnosticsService::kernelModuleLoaded() const {
    return QFileInfo::exists(QStringLiteral("/sys/module/msiecmodule"));
}
