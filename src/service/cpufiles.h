#pragma once

#include <QString>
#include <QVector>

#include "struct.h"

namespace CpuFiles {

QVector<QString> discoverCpuDirs();
QString readText(const QString& filePath);
quint32 readCurrentFreq(const QString& cpuDir);
Msi::Cpu readControl(const QString& cpuDir, const Msi::Cpu* fallback = nullptr);
Msi::CpuConfig readControls(const QVector<QString>& cpuDirs, const Msi::CpuConfig& fallback = {});
bool writeControls(const QVector<QString>& cpuDirs, const Msi::CpuConfig& desired, const Msi::CpuConfig& current);

} // namespace CpuFiles
