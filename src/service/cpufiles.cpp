#include "cpufiles.h"

#include <QDebug>
#include <QDBusArgument>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusVariant>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QStringList>

#include <algorithm>

namespace {
static const char* rootPath = "/sys/devices/system/cpu/";
static const char* scalingCurFreqPath = "/cpufreq/scaling_cur_freq";
static const char* scalingFreqMinPath = "/cpufreq/scaling_min_freq";
static const char* scalingFreqMaxPath = "/cpufreq/scaling_max_freq";
static const char* cpuinfoFreqMaxPath = "/cpufreq/cpuinfo_max_freq";
static const char* cpuinfoFreqMinPath = "/cpufreq/cpuinfo_min_freq";
static const char* scalingGovernorPath = "/cpufreq/scaling_governor";
static const char* availableGovernorsPath = "/cpufreq/scaling_available_governors";
static const char* powerProfilesService = "org.freedesktop.UPower.PowerProfiles";
static const char* powerProfilesPath = "/org/freedesktop/UPower/PowerProfiles";
static const char* dbusPropertiesInterface = "org.freedesktop.DBus.Properties";

QString cpuPath(const QString& cpuDir, const char* filePath) {
    return QString::fromLatin1(rootPath) + cpuDir + QString::fromLatin1(filePath);
}

QString normalizePowerProfile(const QString& value) {
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("powersave") || normalized == QStringLiteral("power_saver") ||
        normalized == QStringLiteral("power saver")) {
        return QStringLiteral("power-saver");
    }
    if (normalized == QStringLiteral("balanced") || normalized == QStringLiteral("performance") ||
        normalized == QStringLiteral("power-saver")) {
        return normalized;
    }
    return value.trimmed();
}

QStringList defaultPowerProfiles(const QString& activeProfile) {
    QStringList profiles{QStringLiteral("power-saver"), QStringLiteral("balanced"), QStringLiteral("performance")};
    if (!activeProfile.isEmpty() && !profiles.contains(activeProfile)) {
        profiles.prepend(activeProfile);
    }
    return profiles;
}

QStringList parsePowerProfileList(const QVariant& value) {
    QStringList profiles;
    if (!value.canConvert<QDBusArgument>()) {
        const QVariantList list = value.toList();
        for (const QVariant& item : list) {
            const QVariantMap map = item.toMap();
            const QString profile = map.value(QStringLiteral("Profile")).toString();
            if (!profile.isEmpty() && !profiles.contains(profile)) {
                profiles.append(profile);
            }
        }
        return profiles;
    }

    QDBusArgument argument = value.value<QDBusArgument>();
    argument.beginArray();
    while (!argument.atEnd()) {
        QString profile;
        argument.beginMap();
        while (!argument.atEnd()) {
            QString key;
            QDBusVariant entryValue;
            argument.beginMapEntry();
            argument >> key >> entryValue;
            argument.endMapEntry();
            if (key == QStringLiteral("Profile")) {
                profile = entryValue.variant().toString();
            }
        }
        argument.endMap();

        if (!profile.isEmpty() && !profiles.contains(profile)) {
            profiles.append(profile);
        }
    }
    argument.endArray();
    return profiles;
}

struct PowerProfileState {
    bool available{false};
    QString activeProfile;
    QStringList profiles;
};

PowerProfileState readPowerProfiles() {
    PowerProfileState state;
    QDBusInterface properties(QString::fromLatin1(powerProfilesService),
                              QString::fromLatin1(powerProfilesPath),
                              QString::fromLatin1(dbusPropertiesInterface),
                              QDBusConnection::systemBus());

    QDBusReply<QDBusVariant> activeReply =
        properties.call(QStringLiteral("Get"), QString::fromLatin1(powerProfilesService), QStringLiteral("ActiveProfile"));
    if (!activeReply.isValid()) {
        return state;
    }

    state.available = true;
    state.activeProfile = activeReply.value().variant().toString();

    QDBusReply<QDBusVariant> profilesReply =
        properties.call(QStringLiteral("Get"), QString::fromLatin1(powerProfilesService), QStringLiteral("Profiles"));
    if (profilesReply.isValid()) {
        state.profiles = parsePowerProfileList(profilesReply.value().variant());
    }
    if (state.profiles.isEmpty()) {
        state.profiles = defaultPowerProfiles(state.activeProfile);
    }
    if (!state.activeProfile.isEmpty() && !state.profiles.contains(state.activeProfile)) {
        state.profiles.prepend(state.activeProfile);
    }
    return state;
}

bool writePowerProfile(const QString& profile) {
    const QString normalizedProfile = normalizePowerProfile(profile);
    if (normalizedProfile.isEmpty()) {
        return false;
    }

    QDBusInterface properties(QString::fromLatin1(powerProfilesService),
                              QString::fromLatin1(powerProfilesPath),
                              QString::fromLatin1(dbusPropertiesInterface),
                              QDBusConnection::systemBus());
    QDBusReply<void> reply = properties.call(QStringLiteral("Set"),
                                             QString::fromLatin1(powerProfilesService),
                                             QStringLiteral("ActiveProfile"),
                                             QVariant::fromValue(QDBusVariant(normalizedProfile)));
    if (!reply.isValid()) {
        qWarning() << "Failed to set power profile:" << normalizedProfile << reply.error();
        return false;
    }
    return true;
}
} // namespace

QVector<QString> CpuFiles::discoverCpuDirs() {
    QDir dir(rootPath);
    if (!dir.exists()) {
        qWarning() << "CPU directory does not exist:" << rootPath;
        return {};
    }

    static const QRegularExpression cpuRegex("^cpu[0-9]+$");
    QStringList cpuDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot).filter(cpuRegex);
    std::sort(cpuDirs.begin(), cpuDirs.end(), [](const QString& a, const QString& b) {
        return QStringView{a}.mid(3).toInt() < QStringView{b}.mid(3).toInt();
    });

    QVector<QString> result;
    result.reserve(cpuDirs.size());
    for (const QString& cpuDir : std::as_const(cpuDirs))
        result.append(cpuDir);
    return result;
}

QString CpuFiles::readText(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file:" << filePath;
        return {};
    }
    return QString::fromUtf8(file.readAll().trimmed());
}

quint32 CpuFiles::readCurrentFreq(const QString& cpuDir) {
    return readText(cpuPath(cpuDir, scalingCurFreqPath)).toUInt();
}

Msi::Cpu CpuFiles::readControl(const QString& cpuDir, const Msi::Cpu* fallback) {
    Msi::Cpu cpu = fallback ? *fallback : Msi::Cpu{};
    cpu.minFreq = readText(cpuPath(cpuDir, cpuinfoFreqMinPath)).toUInt();
    cpu.maxFreq = readText(cpuPath(cpuDir, cpuinfoFreqMaxPath)).toUInt();
    cpu.scalingMinFreq = readText(cpuPath(cpuDir, scalingFreqMinPath)).toUInt();
    cpu.scalingMaxFreq = readText(cpuPath(cpuDir, scalingFreqMaxPath)).toUInt();
    cpu.availableGovernor = readText(cpuPath(cpuDir, scalingGovernorPath));
    cpu.availableGovernors = readText(cpuPath(cpuDir, availableGovernorsPath))
                                  .split(QLatin1Char(' '), Qt::SkipEmptyParts);
    return cpu;
}

Msi::CpuConfig CpuFiles::readControls(const QVector<QString>& cpuDirs, const Msi::CpuConfig& fallback) {
    Msi::CpuConfig config;
    const bool hasFallback = fallback.cpus.size() == cpuDirs.size();
    const PowerProfileState powerProfiles = readPowerProfiles();
    for (int i = 0; i < cpuDirs.size(); ++i) {
        const Msi::Cpu* previous = hasFallback ? &fallback.cpus.at(i) : nullptr;
        Msi::Cpu cpu = readControl(cpuDirs.at(i), previous);
        if (powerProfiles.available) {
            cpu.availableGovernor = powerProfiles.activeProfile;
            cpu.availableGovernors = powerProfiles.profiles;
        }
        config.cpus.append(cpu);
    }
    return config;
}

bool CpuFiles::writeControls(const QVector<QString>& cpuDirs,
                             const Msi::CpuConfig& desired,
                             const Msi::CpuConfig& current) {
    bool success = true;
    const PowerProfileState powerProfiles = readPowerProfiles();
    QString desiredPowerProfile;
    if (powerProfiles.available) {
        for (const Msi::Cpu& cpu : desired.cpus) {
            const QString profile = normalizePowerProfile(cpu.availableGovernor);
            if (!profile.isEmpty() && powerProfiles.profiles.contains(profile)) {
                desiredPowerProfile = profile;
                break;
            }
        }
        if (!desiredPowerProfile.isEmpty() && desiredPowerProfile != powerProfiles.activeProfile) {
            success = writePowerProfile(desiredPowerProfile) && success;
        }
    }

    for (int i = 0; i < desired.cpus.size() && i < cpuDirs.size(); ++i) {
        const Msi::Cpu& cpu = desired.cpus.at(i);
        const bool hasCurrent = i < current.cpus.size();
        const Msi::Cpu* currentCpu = hasCurrent ? &current.cpus.at(i) : nullptr;
        const QString& cpuDir = cpuDirs.at(i);

        auto writeText = [&success](const QString& fileName, const QString& value) {
            QFile file(fileName);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                qWarning() << "Failed to open file for writing:" << fileName;
                success = false;
                return;
            }
            if (file.write(value.toUtf8()) == -1) {
                qWarning() << "Failed to write to file:" << fileName;
                success = false;
            }
        };

        if (!currentCpu || currentCpu->scalingMinFreq != cpu.scalingMinFreq) {
            writeText(cpuPath(cpuDir, scalingFreqMinPath), QString::number(cpu.scalingMinFreq));
        }
        if (!currentCpu || currentCpu->scalingMaxFreq != cpu.scalingMaxFreq) {
            writeText(cpuPath(cpuDir, scalingFreqMaxPath), QString::number(cpu.scalingMaxFreq));
        }
        if (!powerProfiles.available && !cpu.availableGovernor.isEmpty() &&
            (!currentCpu || currentCpu->availableGovernor != cpu.availableGovernor)) {
            writeText(cpuPath(cpuDir, scalingGovernorPath), cpu.availableGovernor);
        }
    }

    return success;
}
