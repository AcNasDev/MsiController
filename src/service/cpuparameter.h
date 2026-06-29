#pragma once
#include <QTimer>
#include <QVariant>
#include <QVector>

#include "cpufiles.h"
#include "parameter.h"
#include "struct.h"

class CpuParameter : public Parameter {
    Q_OBJECT
public:
    explicit CpuParameter(const QVariant& name, QObject* parent = nullptr, CpuFiles::CpuBackend* backend = nullptr);
    ~CpuParameter() override = default;

protected:
    QVariant readValue() const override;
    bool writeValue(const QVariant& value) override;

private:
    QVariant mValue;
    CpuFiles::CpuBackend* mBackend{nullptr};
    QVector<QString> mCpuDirs;
    QVector<CpuFiles::CoreStat> mCpuCoreStatsPrev;
    QVector<CpuFiles::CoreStat> mCpuCoreStatsCur;
    QTimer mTimer;
    bool mForceControlRefresh{true};
    int mTicksSinceControlRefresh{0};

    void updateConfig();
};
