#pragma once
#include <QTimer>
#include <QVariant>

#include "parameter.h"

class CpuParameter : public Parameter {
    Q_OBJECT
public:
    explicit CpuParameter(const QVariant& name, QObject* parent = nullptr);
    ~CpuParameter() override = default;

protected:
    QVariant readValue() const override;
    bool writeValue(const QVariant& value) override;

private:
    struct CpuCoreStat {
        quint64 idle{0};
        quint64 total{0};
    };
    QVariant mValue;
    QVector<QString> mCpuDirs;
    QVector<CpuCoreStat> mCpuCoreStatsPrev;
    QVector<CpuCoreStat> mCpuCoreStatsCur;
    QTimer mTimer;

    QVector<CpuCoreStat> readCoreStats() const;
    void updateConfig();
    void writeToFile(const QString& fileName, const QString& value);
};