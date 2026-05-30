#pragma once

#include "parameter.h"

class SoftwareParameter : public Parameter {
public:
    explicit SoftwareParameter(const QVariant& name,
                               const QVariant& available,
                               const QVariant& defaultValue,
                               QObject* parent = nullptr);
    ~SoftwareParameter() override = default;

protected:
    QVariant readValue() const override;
    bool writeValue(const QVariant& value) override;

private:
    QVariant mValue;
};
