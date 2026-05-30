#pragma once

#include <QObject>
#include <QTimer>
#include <QVariantList>

#include "struct.h"

class EcService;
class Parameter;

class FanTargetController : public QObject {
    Q_OBJECT
public:
    enum class FanSide { Cpu, Gpu };

    explicit FanTargetController(EcService* service, QObject* parent = nullptr);

private:
    EcService* mService{nullptr};
    QTimer mTimer;
    bool mHasSeenMode{false};
    Msi::FanControlMode mLastMode{Msi::FanControlMode::Curve};
    QVariantList mLastCpuCurve;
    QVariantList mLastGpuCurve;

    Parameter* parameter(Msi::Parametr param) const;
    Msi::FanControlMode controlMode() const;
    int targetTemperature(FanSide side) const;
    int currentTemperature(FanSide side) const;
    bool isTargetModeActive() const;

    void apply();
    void applyTargetMode();
    void applyCurve(FanSide side, int targetTemp, int currentTemp);
    void forceAdvancedFanMode();

    bool hasCurveBackup() const;
    void saveCurveBackup();
    void restoreCurveBackup();
    QVariantList readCurve(FanSide side) const;
    void writeCurve(FanSide side, const QVariantList& values);
};
