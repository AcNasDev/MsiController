#include "cpufiles.h"

#include <QDebug>
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

QString cpuPath(const QString& cpuDir, const char* filePath) {
    return QString::fromLatin1(rootPath) + cpuDir + QString::fromLatin1(filePath);
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
    for (int i = 0; i < cpuDirs.size(); ++i) {
        const Msi::Cpu* previous = hasFallback ? &fallback.cpus.at(i) : nullptr;
        config.cpus.append(readControl(cpuDirs.at(i), previous));
    }
    return config;
}

bool CpuFiles::writeControls(const QVector<QString>& cpuDirs,
                             const Msi::CpuConfig& desired,
                             const Msi::CpuConfig& current) {
    bool success = true;

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
        if (!cpu.availableGovernor.isEmpty() && (!currentCpu || currentCpu->availableGovernor != cpu.availableGovernor)) {
            writeText(cpuPath(cpuDir, scalingGovernorPath), cpu.availableGovernor);
        }
    }

    return success;
}
