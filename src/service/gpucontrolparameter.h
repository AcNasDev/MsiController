#pragma once

#include <QTimer>
#include <QVariant>
#include <QVariantList>

#include "parameter.h"

class GpuControlParameter : public Parameter {
    Q_OBJECT
public:
    explicit GpuControlParameter(const QVariant& name, QObject* parent = nullptr);
    ~GpuControlParameter() override = default;

    void setValue(const QVariant& value) override;

protected:
    QVariant readValue() const override;
    bool writeValue(const QVariant& value) override;

private:
    QVariantList mDevices;
    QTimer mTimer;

    void updateConfig();
};
