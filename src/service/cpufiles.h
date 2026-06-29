#pragma once

#include <QString>
#include <QVector>

#include "struct.h"

class SystemAccess;

namespace CpuFiles {

struct CoreStat {
    quint64 idle{0};
    quint64 total{0};
};

class CpuBackend {
public:
    virtual ~CpuBackend() = default;

    virtual QVector<QString> discoverCpuDirs() const = 0;
    virtual QString readText(const QString& filePath) const = 0;
    virtual quint32 readCurrentFreq(const QString& cpuDir) const = 0;
    virtual Msi::Cpu readControl(const QString& cpuDir, const Msi::Cpu* fallback = nullptr) const = 0;
    virtual Msi::CpuConfig readControls(const QVector<QString>& cpuDirs, const Msi::CpuConfig& fallback = {}) const = 0;
    virtual bool writeControls(const QVector<QString>& cpuDirs,
                               const Msi::CpuConfig& desired,
                               const Msi::CpuConfig& current) const = 0;
    virtual QVector<CoreStat> readCoreStats() const = 0;
};

class LinuxCpuBackend : public CpuBackend {
public:
    explicit LinuxCpuBackend(SystemAccess* systemAccess = nullptr);

    QVector<QString> discoverCpuDirs() const override;
    QString readText(const QString& filePath) const override;
    quint32 readCurrentFreq(const QString& cpuDir) const override;
    Msi::Cpu readControl(const QString& cpuDir, const Msi::Cpu* fallback = nullptr) const override;
    Msi::CpuConfig readControls(const QVector<QString>& cpuDirs, const Msi::CpuConfig& fallback = {}) const override;
    bool writeControls(const QVector<QString>& cpuDirs,
                       const Msi::CpuConfig& desired,
                       const Msi::CpuConfig& current) const override;
    QVector<CoreStat> readCoreStats() const override;

private:
    SystemAccess* mSystemAccess{nullptr};
};

CpuBackend& defaultBackend();
QVector<QString> discoverCpuDirs();
QString readText(const QString& filePath);
quint32 readCurrentFreq(const QString& cpuDir);
Msi::Cpu readControl(const QString& cpuDir, const Msi::Cpu* fallback = nullptr);
Msi::CpuConfig readControls(const QVector<QString>& cpuDirs, const Msi::CpuConfig& fallback = {});
bool writeControls(const QVector<QString>& cpuDirs, const Msi::CpuConfig& desired, const Msi::CpuConfig& current);
QVector<CoreStat> readCoreStats();

} // namespace CpuFiles
