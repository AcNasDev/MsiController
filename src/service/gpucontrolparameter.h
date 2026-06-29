#pragma once

#include <QTimer>
#include <QVariant>
#include <QVariantList>

#include "parameter.h"

class SystemAccess;

class GpuControlBackend {
public:
    virtual ~GpuControlBackend() = default;

    virtual QVariantList readDevices() const = 0;
    virtual bool writeDevices(const QVariantList& desiredDevices) const = 0;
};

class LinuxGpuControlBackend : public GpuControlBackend {
public:
    explicit LinuxGpuControlBackend(SystemAccess* systemAccess = nullptr);

    QVariantList readDevices() const override;
    bool writeDevices(const QVariantList& desiredDevices) const override;

private:
    SystemAccess* mSystemAccess{nullptr};
};

GpuControlBackend& defaultGpuControlBackend();

class GpuControlParameter : public Parameter {
    Q_OBJECT
public:
    explicit GpuControlParameter(const QVariant& name, QObject* parent = nullptr, GpuControlBackend* backend = nullptr);
    ~GpuControlParameter() override = default;

protected:
    QVariant readValue() const override;
    bool writeValue(const QVariant& value) override;

private:
    QVariantList mDevices;
    QTimer mTimer;
    GpuControlBackend* mBackend{nullptr};

    void updateConfig();
};
