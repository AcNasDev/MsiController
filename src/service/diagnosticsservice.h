#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class DeviceProfileService;
class EcMemoryService;
class ParameterRegistry;

class DiagnosticsService : public QObject {
    Q_OBJECT
public:
    DiagnosticsService(ParameterRegistry* parameters,
                       EcMemoryService* memory,
                       DeviceProfileService* profiles,
                       QObject* parent = nullptr);

    QVariantMap diagnostics() const;
    QVariantMap supportBundle() const;

private:
    ParameterRegistry* mParameters{nullptr};
    EcMemoryService* mMemory{nullptr};
    DeviceProfileService* mProfiles{nullptr};

    QVariantMap firmwareInfo() const;
    QVariantMap systemInfo() const;
    bool kernelModuleLoaded() const;
};
