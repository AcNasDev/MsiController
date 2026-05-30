#pragma once

#include <QTimer>
#include <QVariant>
#include <QVector>

#include "parameter.h"
#include "struct.h"

class CpuControlParameter : public Parameter {
    Q_OBJECT
public:
    explicit CpuControlParameter(const QVariant& name, QObject* parent = nullptr);
    ~CpuControlParameter() override = default;

protected:
    QVariant readValue() const override;
    bool writeValue(const QVariant& value) override;

private:
    QVariant mValue;
    QVector<QString> mCpuDirs;
    QTimer mTimer;

    void updateConfig();
};
