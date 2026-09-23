#include "gpucontrolparameter.h"

#include <QDebug>
#include <algorithm>
#include <optional>
#include <utility>

#include "systemaccess.h"

namespace {
constexpr int gpuControlRefreshIntervalMs = 5000;
constexpr qint64 milliunitsPerUnit = 1000;
constexpr qint64 microwattPerWatt = 1000000;

QString readText(const SystemAccess& access, const QString& filePath) {
    return access.files().readText(filePath);
}

bool writeText(const SystemAccess& access, const QString& filePath, const QString& value) {
    QString errorMessage;
    if (!access.files().writeText(filePath, value, &errorMessage)) {
        qWarning() << "Failed to write GPU control:" << filePath << errorMessage;
        return false;
    }
    return true;
}

std::optional<double> parseDoubleField(QString value) {
    value = value.trimmed();
    if (value.isEmpty() || value.compare(QStringLiteral("N/A"), Qt::CaseInsensitive) == 0 ||
        value.compare(QStringLiteral("[N/A]"), Qt::CaseInsensitive) == 0) {
        return std::nullopt;
    }

    bool ok = false;
    const double parsed = value.toDouble(&ok);
    if (!ok) {
        return std::nullopt;
    }
    return parsed;
}

double readScaledInteger(const SystemAccess& access, const QString& filePath, qint64 scale) {
    bool ok = false;
    const qint64 value = readText(access, filePath).toLongLong(&ok);
    return ok ? static_cast<double>(value) / static_cast<double>(scale) : 0.0;
}

QString canonicalPath(const SystemAccess& access, const QString& filePath) {
    const QString path = access.files().canonicalPath(filePath);
    return path.isEmpty() ? access.files().absolutePath(filePath) : path;
}

QString driverName(const SystemAccess& access, const QString& devicePath) {
    const QString path = access.files().canonicalPath(devicePath + QStringLiteral("/driver"));
    return path.isEmpty() ? QString() : access.files().fileName(path);
}

QString normalizePciBusId(QString busId) {
    busId = busId.trimmed().toLower();
    const qsizetype colon = busId.indexOf(QLatin1Char(':'));
    if (colon == 8) {
        busId = busId.mid(4);
    }
    return busId;
}

QStringList splitCsvLine(const QString& line) {
    QStringList result;
    QString current;
    bool quoted = false;
    for (QChar ch : line) {
        if (ch == QLatin1Char('"')) {
            quoted = !quoted;
            continue;
        }
        if (ch == QLatin1Char(',') && !quoted) {
            result.append(current.trimmed());
            current.clear();
            continue;
        }
        current.append(ch);
    }
    result.append(current.trimmed());
    return result;
}

QVariantMap readNvidiaSmiDevices(const SystemAccess& access) {
    QVariantMap resultByBusId;
    const QString nvidiaSmi = access.processes().findExecutable(QStringLiteral("nvidia-smi"));
    if (nvidiaSmi.isEmpty()) {
        return resultByBusId;
    }

    const ProcessResult process =
        access.processes().run(nvidiaSmi,
                               {QStringLiteral("--query-gpu=index,pci.bus_id,name,temperature.gpu,power.draw,power."
                                               "limit,power.min_limit,power.max_limit,persistence_mode"),
                                QStringLiteral("--format=csv,noheader,nounits")},
                               1500);
    if (!process.ok()) {
        return resultByBusId;
    }

    const QString output = QString::fromUtf8(process.standardOutput);
    for (const QString& rawLine : output.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        const QStringList fields = splitCsvLine(rawLine);
        if (fields.size() < 9) {
            continue;
        }

        QVariantMap gpu;
        bool ok = false;
        gpu.insert(QStringLiteral("nvidiaIndex"), fields.at(0).toInt(&ok));
        if (!ok) {
            continue;
        }
        gpu.insert(QStringLiteral("name"), fields.at(2));
        if (const auto value = parseDoubleField(fields.at(3))) {
            gpu.insert(QStringLiteral("temperatureC"), *value);
        }
        if (const auto value = parseDoubleField(fields.at(4))) {
            gpu.insert(QStringLiteral("powerDrawWatts"), *value);
        }
        if (const auto value = parseDoubleField(fields.at(5))) {
            gpu.insert(QStringLiteral("powerLimitWatts"), *value);
        }
        if (const auto value = parseDoubleField(fields.at(6))) {
            gpu.insert(QStringLiteral("minPowerLimitWatts"), *value);
        }
        if (const auto value = parseDoubleField(fields.at(7))) {
            gpu.insert(QStringLiteral("maxPowerLimitWatts"), *value);
        }
        gpu.insert(QStringLiteral("persistenceMode"),
                   fields.at(8).compare(QStringLiteral("Enabled"), Qt::CaseInsensitive) == 0);
        resultByBusId.insert(normalizePciBusId(fields.at(1)), gpu);
    }
    return resultByBusId;
}

QString firstHwmonDir(const SystemAccess& access, const QString& devicePath) {
    const QString hwmonPath = devicePath + QStringLiteral("/hwmon");
    const QStringList entries = access.files().entryList(hwmonPath, {}, true);
    if (entries.isEmpty()) {
        return {};
    }
    return hwmonPath + QLatin1Char('/') + entries.first();
}

QVariantMap readDrmGpu(const SystemAccess& access, const QString& cardName, const QVariantMap& nvidiaDevicesByBusId) {
    const QString devicePath =
        canonicalPath(access, QStringLiteral("/sys/class/drm/") + cardName + QStringLiteral("/device"));
    const QString vendorId = readText(access, devicePath + QStringLiteral("/vendor")).toLower();
    if (devicePath.isEmpty() || vendorId.isEmpty()) {
        return {};
    }

    const QString pciBusId = access.files().fileName(devicePath).toLower();
    const bool isNvidia = vendorId == QStringLiteral("0x10de");
    const bool isAmd = vendorId == QStringLiteral("0x1002") || vendorId == QStringLiteral("0x1022");
    if (!isNvidia && !isAmd) {
        return {};
    }

    QVariantMap gpu;
    gpu.insert(QStringLiteral("id"), cardName);
    gpu.insert(QStringLiteral("pciBusId"), pciBusId);
    gpu.insert(QStringLiteral("vendor"), isNvidia ? QStringLiteral("NVIDIA") : QStringLiteral("AMD"));
    gpu.insert(QStringLiteral("driver"), driverName(access, devicePath));

    const QString hwmonDir = firstHwmonDir(access, devicePath);
    if (!hwmonDir.isEmpty()) {
        const QString name = readText(access, hwmonDir + QStringLiteral("/name"));
        if (!name.isEmpty()) {
            gpu.insert(QStringLiteral("name"), name);
        }

        const double temperature =
            readScaledInteger(access, hwmonDir + QStringLiteral("/temp1_input"), milliunitsPerUnit);
        if (temperature > 0.0) {
            gpu.insert(QStringLiteral("temperatureC"), temperature);
        }
        double powerDraw = readScaledInteger(access, hwmonDir + QStringLiteral("/power1_average"), microwattPerWatt);
        if (powerDraw <= 0.0) {
            powerDraw = readScaledInteger(access, hwmonDir + QStringLiteral("/power1_input"), microwattPerWatt);
        }
        if (powerDraw > 0.0) {
            gpu.insert(QStringLiteral("powerDrawWatts"), powerDraw);
        }

        const QString powerCapPath = hwmonDir + QStringLiteral("/power1_cap");
        const double powerLimit = readScaledInteger(access, powerCapPath, microwattPerWatt);
        if (powerLimit > 0.0) {
            const double minPowerLimit =
                readScaledInteger(access, hwmonDir + QStringLiteral("/power1_cap_min"), microwattPerWatt);
            const double maxPowerLimit =
                readScaledInteger(access, hwmonDir + QStringLiteral("/power1_cap_max"), microwattPerWatt);
            gpu.insert(QStringLiteral("powerLimitWatts"), powerLimit);
            gpu.insert(QStringLiteral("minPowerLimitWatts"), minPowerLimit > 0.0 ? minPowerLimit : powerLimit);
            gpu.insert(QStringLiteral("maxPowerLimitWatts"), maxPowerLimit > 0.0 ? maxPowerLimit : powerLimit);
            gpu.insert(QStringLiteral("powerLimitPath"), powerCapPath);
            gpu.insert(QStringLiteral("canSetPowerLimit"), access.files().isWritable(powerCapPath));
        }
    }

    if (isAmd) {
        const QString performancePath = devicePath + QStringLiteral("/power_dpm_force_performance_level");
        const QString performanceLevel = readText(access, performancePath);
        if (!performanceLevel.isEmpty()) {
            gpu.insert(QStringLiteral("performanceLevel"), performanceLevel);
            gpu.insert(QStringLiteral("availablePerformanceLevels"),
                       QStringList{QStringLiteral("auto"),
                                   QStringLiteral("low"),
                                   QStringLiteral("high"),
                                   QStringLiteral("manual")});
            gpu.insert(QStringLiteral("performanceLevelPath"), performancePath);
            gpu.insert(QStringLiteral("canSetPerformanceLevel"), access.files().isWritable(performancePath));
        }
    }

    if (isNvidia) {
        const QVariantMap nvidia = nvidiaDevicesByBusId.value(pciBusId).toMap();
        for (auto it = nvidia.cbegin(); it != nvidia.cend(); ++it) {
            gpu.insert(it.key(), it.value());
        }
        if (gpu.contains(QStringLiteral("nvidiaIndex"))) {
            const double powerLimit = gpu.value(QStringLiteral("powerLimitWatts")).toDouble();
            const double minPowerLimit = gpu.value(QStringLiteral("minPowerLimitWatts")).toDouble();
            const double maxPowerLimit = gpu.value(QStringLiteral("maxPowerLimitWatts")).toDouble();
            gpu.insert(QStringLiteral("canSetPowerLimit"),
                       powerLimit > 0.0 && minPowerLimit > 0.0 && maxPowerLimit >= minPowerLimit);
            gpu.insert(QStringLiteral("canSetPersistenceMode"), true);
        }
    }

    if (!gpu.contains(QStringLiteral("name"))) {
        gpu.insert(QStringLiteral("name"),
                   gpu.value(QStringLiteral("vendor")).toString() + QStringLiteral(" ") + pciBusId);
    }
    return gpu;
}

QVariantList readGpuDevices(const SystemAccess& access) {
    const QVariantMap nvidiaDevicesByBusId = readNvidiaSmiDevices(access);
    QStringList cards =
        access.files().entryList(QStringLiteral("/sys/class/drm"), QStringList{QStringLiteral("card[0-9]*")}, true);
    std::sort(cards.begin(), cards.end(), [](const QString& left, const QString& right) {
        return QStringView{left}.mid(4).toInt() < QStringView{right}.mid(4).toInt();
    });

    QVariantList devices;
    for (const QString& card : std::as_const(cards)) {
        QVariantMap gpu = readDrmGpu(access, card, nvidiaDevicesByBusId);
        const bool hasControls = gpu.value(QStringLiteral("canSetPowerLimit")).toBool() ||
                                 gpu.value(QStringLiteral("canSetPerformanceLevel")).toBool() ||
                                 gpu.value(QStringLiteral("canSetPersistenceMode")).toBool();
        if (!gpu.isEmpty() && hasControls) {
            devices.append(gpu);
        }
    }
    return devices;
}

QVariantMap mapById(const QVariantList& devices) {
    QVariantMap result;
    for (const QVariant& item : devices) {
        const QVariantMap map = item.toMap();
        result.insert(map.value(QStringLiteral("id")).toString(), map);
    }
    return result;
}

bool writeNvidiaPowerLimit(const SystemAccess& access, const QVariantMap& gpu, double watts) {
    const QString nvidiaSmi = access.processes().findExecutable(QStringLiteral("nvidia-smi"));
    if (nvidiaSmi.isEmpty()) {
        return false;
    }

    const int index = gpu.value(QStringLiteral("nvidiaIndex"), -1).toInt();
    if (index < 0) {
        return false;
    }

    const ProcessResult process = access.processes().run(
        nvidiaSmi,
        {QStringLiteral("-i"), QString::number(index), QStringLiteral("-pl"), QString::number(qRound(watts))},
        2500);
    if (!process.ok()) {
        qWarning() << "Failed to set NVIDIA power limit:" << process.standardError;
        return false;
    }
    return true;
}

bool writeNvidiaPersistence(const SystemAccess& access, const QVariantMap& gpu, bool enabled) {
    const QString nvidiaSmi = access.processes().findExecutable(QStringLiteral("nvidia-smi"));
    if (nvidiaSmi.isEmpty()) {
        return false;
    }

    const int index = gpu.value(QStringLiteral("nvidiaIndex"), -1).toInt();
    if (index < 0) {
        return false;
    }

    const ProcessResult process = access.processes().run(nvidiaSmi,
                                                         {QStringLiteral("-i"),
                                                          QString::number(index),
                                                          QStringLiteral("-pm"),
                                                          enabled ? QStringLiteral("1") : QStringLiteral("0")},
                                                         2500);
    if (!process.ok()) {
        qWarning() << "Failed to set NVIDIA persistence mode:" << process.standardError;
        return false;
    }
    return true;
}
} // namespace

LinuxGpuControlBackend::LinuxGpuControlBackend(SystemAccess* systemAccess)
    : mSystemAccess(systemAccess ? systemAccess : &defaultSystemAccess()) {}

QVariantList LinuxGpuControlBackend::readDevices() const {
    return readGpuDevices(*mSystemAccess);
}

bool LinuxGpuControlBackend::writeDevices(const QVariantList& desiredDevices) const {
    const QVariantMap currentById = mapById(readGpuDevices(*mSystemAccess));
    bool success = true;

    for (const QVariant& item : desiredDevices) {
        const QVariantMap desired = item.toMap();
        const QString id = desired.value(QStringLiteral("id")).toString();
        const QVariantMap current = currentById.value(id).toMap();
        if (current.isEmpty()) {
            continue;
        }

        if (current.value(QStringLiteral("canSetPerformanceLevel")).toBool() &&
            desired.value(QStringLiteral("performanceLevel")).toString() !=
                current.value(QStringLiteral("performanceLevel")).toString()) {
            success = writeText(*mSystemAccess,
                                current.value(QStringLiteral("performanceLevelPath")).toString(),
                                desired.value(QStringLiteral("performanceLevel")).toString()) &&
                      success;
        }

        if (current.value(QStringLiteral("canSetPowerLimit")).toBool() &&
            !qFuzzyCompare(desired.value(QStringLiteral("powerLimitWatts")).toDouble() + 1.0,
                           current.value(QStringLiteral("powerLimitWatts")).toDouble() + 1.0)) {
            double requestedWatts = desired.value(QStringLiteral("powerLimitWatts")).toDouble();
            const double minWatts = current.value(QStringLiteral("minPowerLimitWatts")).toDouble();
            const double maxWatts = current.value(QStringLiteral("maxPowerLimitWatts")).toDouble();
            if (minWatts > 0.0 && maxWatts >= minWatts) {
                requestedWatts = std::clamp(requestedWatts, minWatts, maxWatts);
            }
            if (current.contains(QStringLiteral("powerLimitPath"))) {
                success = writeText(*mSystemAccess,
                                    current.value(QStringLiteral("powerLimitPath")).toString(),
                                    QString::number(qRound64(requestedWatts * microwattPerWatt))) &&
                          success;
            } else if (current.contains(QStringLiteral("nvidiaIndex"))) {
                success = writeNvidiaPowerLimit(*mSystemAccess, current, requestedWatts) && success;
            }
        }

        if (current.value(QStringLiteral("canSetPersistenceMode")).toBool() &&
            desired.value(QStringLiteral("persistenceMode")).toBool() !=
                current.value(QStringLiteral("persistenceMode")).toBool()) {
            success = writeNvidiaPersistence(*mSystemAccess,
                                             current,
                                             desired.value(QStringLiteral("persistenceMode")).toBool()) &&
                      success;
        }
    }
    return success;
}

GpuControlBackend& defaultGpuControlBackend() {
    static LinuxGpuControlBackend backend;
    return backend;
}

GpuControlParameter::GpuControlParameter(const QVariant& name, QObject* parent, GpuControlBackend* backend)
    : Parameter(name, QVariant(), false, parent, nullptr, Parameter::Persistence::Volatile),
      mBackend(backend ? backend : &defaultGpuControlBackend()) {
    updateConfig();
    connect(&mTimer, &QTimer::timeout, this, &GpuControlParameter::updateConfig);
    mTimer.start(gpuControlRefreshIntervalMs);
}

QVariant GpuControlParameter::readValue() const {
    return mDevices;
}

bool GpuControlParameter::writeValue(const QVariant& value) {
    const bool success = mBackend->writeDevices(value.toList());
    if (!success) {
        qWarning() << "Failed to apply one or more GPU controls";
    }
    return success;
}

void GpuControlParameter::updateConfig() {
    const QVariantList devices = mBackend->readDevices();
    if (devices != mDevices) {
        mDevices = devices;
        publishValue(QVariant::fromValue(mDevices));
    }
}
