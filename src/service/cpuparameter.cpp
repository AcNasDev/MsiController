#include "cpuparameter.h"

#include <QDir>
#include <QDebug>
#include <QRegularExpression>
#include <QThread>

#include "struct.h"

namespace {
    static const char* rootPath = "/sys/devices/system/cpu/";
    static const char* scalingCurFreqPath = "/cpufreq/scaling_cur_freq";
    static const char* scalingFreqMinPath = "/cpufreq/scaling_min_freq";
    static const char* scalingFreqMaxPath = "/cpufreq/scaling_max_freq";
    static const char* cpuinfoFreqMaxPath = "/cpufreq/cpuinfo_max_freq";
    static const char* cpuinfoFreqMinPath = "/cpufreq/cpuinfo_min_freq";
    static const char* scalingGovernorPath = "/cpufreq/scaling_governor";
    static const char* availableGovernorsPath = "/cpufreq/scaling_available_governors";
    static const char* statPath = "/proc/stat";
}
CpuParameter::CpuParameter(const QVariant &name, QObject *parent)
    : Parameter(name, QVariant(), false, parent) 
{
    QDir dir(rootPath);
    if (!dir.exists()) {
        qWarning() << "CPU directory does not exist:" << rootPath;
        return;
    }
    static const QRegularExpression cpuRegex("^cpu[0-9]+$");
    QStringList cpuDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    cpuDirs = cpuDirs.filter(cpuRegex);
    std::sort(cpuDirs.begin(), cpuDirs.end(), [&](const QString &a, const QString &b) {
        int numA = QStringView{a}.mid(3).toInt();
        int numB = QStringView{b}.mid(3).toInt();
        return numA < numB;
    });
    mCpuDirs = cpuDirs;
    
    mTimer.start(1000);
    connect(&mTimer, &QTimer::timeout, this, &CpuParameter::updateConfig);
    connect(&mTimer, &QTimer::timeout, this, &CpuParameter::update);
}

QString readFile(const QString &filePath) 
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file:" << filePath;
        return QString();
    }
    return QString::fromUtf8(file.readAll().trimmed());
}

QVector<CpuParameter::CpuCoreStat> CpuParameter::readCoreStats() const
{
    QVector<CpuCoreStat> stats;
    static const QRegularExpression rx("^cpu([0-9]+)\\s+");
    QFile file(statPath);
    QRegularExpressionMatch match;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = QString::fromUtf8(file.readAll());
        QStringList lines = content.split('\n', Qt::SkipEmptyParts);
        for (const QString &strLine : std::as_const(lines)) {
            match = rx.match(strLine.trimmed());
            if (match.hasMatch()) {
                QStringList parts = strLine.simplified().split(' ');
                if (parts.size() < 5) continue;
                quint64 user = parts[1].toULongLong();
                quint64 nice = parts[2].toULongLong();
                quint64 system = parts[3].toULongLong();
                quint64 idle = parts[4].toULongLong();
                quint64 iowait = parts.value(5, "0").toULongLong();
                quint64 irq = parts.value(6, "0").toULongLong();
                quint64 softirq = parts.value(7, "0").toULongLong();
                quint64 steal = parts.value(8, "0").toULongLong();
                CpuCoreStat stat;
                stat.idle = idle + iowait;
                stat.total = user + nice + system + idle + iowait + irq + softirq + steal;
                stats.append(stat);
            }
        }
    } else {
        qWarning() << "Failed to open /proc/stat";
    }
    return stats;
}

void CpuParameter::updateConfig() 
{
    Msi::CpuConfig cpuConfig;
    for (const QString &cpuDir : std::as_const(mCpuDirs)) {
        Msi::Cpu cpu;
        QString fullScalingCurFreqPath = rootPath + cpuDir + scalingCurFreqPath;
        QString fullScalingFreqMinPath = rootPath + cpuDir + scalingFreqMinPath;
        QString fullScalingFreqMaxPath = rootPath + cpuDir + scalingFreqMaxPath;
        QString fullScalingGovernorPath = rootPath + cpuDir + scalingGovernorPath;
        QString fullAvailableGovernorsPath = rootPath + cpuDir + availableGovernorsPath;
        QString fullCpuinfoFreqMaxPath = rootPath + cpuDir + cpuinfoFreqMaxPath;
        QString fullCpuinfoFreqMinPath = rootPath + cpuDir + cpuinfoFreqMinPath;

        uint cpuinfoMinFreq = readFile(fullCpuinfoFreqMinPath).toUInt();
        uint cpuinfoMaxFreq = readFile(fullCpuinfoFreqMaxPath).toUInt();
        uint scalingMinFreq = readFile(fullScalingFreqMinPath).toUInt();
        uint scalingMaxFreq = readFile(fullScalingFreqMaxPath).toUInt();
        uint currentFreq = readFile(fullScalingCurFreqPath).toUInt();
        QString currentGovernor = readFile(fullScalingGovernorPath);
        QStringList availableGovernors = readFile(fullAvailableGovernorsPath).split(QLatin1Char(' '), Qt::SkipEmptyParts);
        cpu.scalingMinFreq = scalingMinFreq;
        cpu.scalingMaxFreq = scalingMaxFreq;
        cpu.minFreq = cpuinfoMinFreq;
        cpu.maxFreq = cpuinfoMaxFreq;
        cpu.currentFreq = currentFreq;
        cpu.availableGovernors = availableGovernors;
        cpu.availableGovernor = currentGovernor;
        cpuConfig.cpus.append(cpu);
    }
    mCpuCoreStatsPrev = mCpuCoreStatsCur;
    mCpuCoreStatsCur = readCoreStats();
    for (int i = 0; i < mCpuCoreStatsCur.size() && i < mCpuCoreStatsPrev.size(); ++i) {
        quint64 idleDiff = mCpuCoreStatsCur[i].idle - mCpuCoreStatsPrev[i].idle;
        quint64 totalDiff = mCpuCoreStatsCur[i].total - mCpuCoreStatsPrev[i].total;
        double usage = totalDiff ? 100 * (1.0 - (double)idleDiff / totalDiff) : 0.0;
        if(i < cpuConfig.cpus.size()) {
            cpuConfig.cpus[i].usage = usage;
        }
    }
    mValue = QVariant::fromValue(cpuConfig);
}

QVariant CpuParameter::readValue() const 
{
    return mValue;
}

void CpuParameter::writeToFile(const QString &fileName, const QString &value) 
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing:" << fileName;
        return;
    }
    if (file.write(value.toUtf8()) == -1) {
        qWarning() << "Failed to write to file:" << fileName;
    }
}

bool CpuParameter::writeValue(const QVariant &value) 
{
    Msi::CpuConfig cpuConfig = value.value<Msi::CpuConfig>();
    for(int i = 0; i < cpuConfig.cpus.size() && i < mCpuDirs.size(); ++i) {
        const Msi::Cpu &cpu = cpuConfig.cpus[i];
        QString cpuDir = mCpuDirs[i];
        QString fullScalingFreqMinPath = rootPath + cpuDir + scalingFreqMinPath;
        QString fullScalingFreqMaxPath = rootPath + cpuDir + scalingFreqMaxPath;
        QString fullScalingGovernorPath = rootPath + cpuDir + scalingGovernorPath;
                
        writeToFile(fullScalingFreqMinPath, QByteArray::number(cpu.scalingMinFreq));
        writeToFile(fullScalingFreqMaxPath, QByteArray::number(cpu.scalingMaxFreq));
        writeToFile(fullScalingGovernorPath, cpu.availableGovernor);
    }
    return true;
}
