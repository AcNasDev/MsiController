#include "fantargetcontroller.h"

#include <QSettings>
#include <QVector>
#include <QtMath>

#include <array>

#include "ecservice.h"
#include "parameter.h"

namespace {
constexpr int updateIntervalMs = 2500;
constexpr int minTargetTemp = 50;
constexpr int maxTargetTemp = 95;

QString backupKey(FanTargetController::FanSide side) {
    return side == FanTargetController::FanSide::Cpu ? QStringLiteral("CpuCurveBackup")
                                                     : QStringLiteral("GpuCurveBackup");
}

Msi::Parametr tempParam(FanTargetController::FanSide side) {
    return side == FanTargetController::FanSide::Cpu ? Msi::Parametr::CpuTempEc : Msi::Parametr::GpuTempEc;
}

Msi::Parametr targetParam(FanTargetController::FanSide side) {
    return side == FanTargetController::FanSide::Cpu ? Msi::Parametr::FanTargetCpuTemp
                                                     : Msi::Parametr::FanTargetGpuTemp;
}

Msi::Parametr speedParam(FanTargetController::FanSide side, int index) {
    const int base = side == FanTargetController::FanSide::Cpu
                         ? static_cast<int>(Msi::Parametr::FanSetSpeedCpu1Ec)
                         : static_cast<int>(Msi::Parametr::FanSetSpeedGpu1Ec);
    return static_cast<Msi::Parametr>(base + index);
}

Msi::Parametr curveTempParam(FanTargetController::FanSide side, int index) {
    const int base = side == FanTargetController::FanSide::Cpu
                         ? static_cast<int>(Msi::Parametr::FanSetTempCpu1Ec)
                         : static_cast<int>(Msi::Parametr::FanSetTempGpu1Ec);
    return static_cast<Msi::Parametr>(base + index);
}

int parameterInt(Parameter* parameter, int fallback = 0) {
    return parameter ? parameter->value().toInt() : fallback;
}

bool isValidCurveList(const QVariantList& values) {
    return values.size() == 13;
}
} // namespace

FanTargetController::FanTargetController(EcService* service, QObject* parent)
    : QObject(parent), mService(service) {
    mTimer.setInterval(updateIntervalMs);
    connect(&mTimer, &QTimer::timeout, this, &FanTargetController::apply);

    if (Parameter* mode = parameter(Msi::Parametr::FanControlMode)) {
        connect(mode, &Parameter::valueChanged, this, [this]() {
            mLastCpuCurve.clear();
            mLastGpuCurve.clear();
            apply();
        });
    }
    for (const auto param :
         {Msi::Parametr::FanTargetCpuTemp, Msi::Parametr::FanTargetGpuTemp, Msi::Parametr::CpuTempEc,
          Msi::Parametr::GpuTempEc}) {
        if (Parameter* p = parameter(param)) {
            connect(p, &Parameter::valueChanged, this, [this]() { apply(); });
        }
    }

    mTimer.start();
    QTimer::singleShot(1500, this, &FanTargetController::apply);
}

Parameter* FanTargetController::parameter(Msi::Parametr param) const {
    return mService ? mService->parameter(QVariant::fromValue(param)) : nullptr;
}

Msi::FanControlMode FanTargetController::controlMode() const {
    Parameter* mode = parameter(Msi::Parametr::FanControlMode);
    if (!mode) {
        return Msi::FanControlMode::Curve;
    }

    const QVariant value = mode->value();
    if (value.canConvert<Msi::FanControlMode>()) {
        return value.value<Msi::FanControlMode>();
    }
    return static_cast<Msi::FanControlMode>(value.toInt());
}

int FanTargetController::targetTemperature(FanSide side) const {
    return qBound(minTargetTemp, parameterInt(parameter(targetParam(side)), 78), maxTargetTemp);
}

int FanTargetController::currentTemperature(FanSide side) const {
    return parameterInt(parameter(tempParam(side)));
}

bool FanTargetController::isTargetModeActive() const {
    return controlMode() == Msi::FanControlMode::TargetTemperature;
}

void FanTargetController::apply() {
    const Msi::FanControlMode mode = controlMode();

    if (!mHasSeenMode) {
        mHasSeenMode = true;
        mLastMode = mode;
        if (mode == Msi::FanControlMode::TargetTemperature && !hasCurveBackup()) {
            saveCurveBackup();
        }
    } else if (mode != mLastMode) {
        if (mode == Msi::FanControlMode::TargetTemperature) {
            saveCurveBackup();
        } else if (mLastMode == Msi::FanControlMode::TargetTemperature) {
            restoreCurveBackup();
        }
        mLastMode = mode;
    }

    if (!isTargetModeActive()) {
        return;
    }

    applyTargetMode();
}

void FanTargetController::applyTargetMode() {
    forceAdvancedFanMode();
    applyCurve(FanSide::Cpu, targetTemperature(FanSide::Cpu), currentTemperature(FanSide::Cpu));
    applyCurve(FanSide::Gpu, targetTemperature(FanSide::Gpu), currentTemperature(FanSide::Gpu));
}

void FanTargetController::applyCurve(FanSide side, int targetTemp, int currentTemp) {
    if (currentTemp <= 0) {
        return;
    }

    const std::array<int, 6> tempOffsets{-24, -16, -10, -5, 0, 5};
    QVariantList curveTemps;
    curveTemps.reserve(static_cast<int>(tempOffsets.size()));

    int previousTemp = 0;
    for (int offset : tempOffsets) {
        int point = qBound(20, targetTemp + offset, 100);
        if (point <= previousTemp) {
            point = previousTemp + 1;
        }
        previousTemp = qMin(point, 100);
        curveTemps.append(previousTemp);
    }

    const int bucket = qBound(-3, qRound((currentTemp - targetTemp) / 3.0), 7);
    const int speedOffset = bucket * 5;
    const std::array<int, 7> baseSpeeds{18, 30, 42, 56, 72, 90, 110};
    QVariantList speeds;
    speeds.reserve(static_cast<int>(baseSpeeds.size()));

    int previousSpeed = 0;
    for (int baseSpeed : baseSpeeds) {
        int speed = qBound(0, baseSpeed + speedOffset, 150);
        if (speed < previousSpeed) {
            speed = previousSpeed;
        }
        previousSpeed = speed;
        speeds.append(speed);
    }

    if (currentTemp >= targetTemp + 8) {
        speeds[5] = qMax(speeds[5].toInt(), 112);
        speeds[6] = qMax(speeds[6].toInt(), 132);
    }

    QVariantList signature;
    signature.reserve(13);
    signature.append(speeds);
    signature.append(curveTemps);
    QVariantList& lastSignature = side == FanSide::Cpu ? mLastCpuCurve : mLastGpuCurve;
    if (signature == lastSignature) {
        return;
    }
    lastSignature = signature;

    for (int i = 0; i < speeds.size(); ++i) {
        if (Parameter* speed = parameter(speedParam(side, i))) {
            speed->setValue(speeds.at(i));
        }
    }
    for (int i = 0; i < curveTemps.size(); ++i) {
        if (Parameter* temp = parameter(curveTempParam(side, i))) {
            temp->setValue(curveTemps.at(i));
        }
    }
}

void FanTargetController::forceAdvancedFanMode() {
    Parameter* fanMode = parameter(Msi::Parametr::FanModeEc);
    if (!fanMode || fanMode->isReadOnly()) {
        return;
    }

    const auto modes = fanMode->available().value<QVector<Msi::FanMode>>();
    if (!modes.contains(Msi::FanMode::Advanced)) {
        return;
    }

    if (fanMode->value().value<Msi::FanMode>() != Msi::FanMode::Advanced) {
        fanMode->setValue(QVariant::fromValue(Msi::FanMode::Advanced));
    }
}

bool FanTargetController::hasCurveBackup() const {
    QSettings settings(QStringLiteral("/etc/MsiController/settings.ini"), QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("FanTargetController"));
    const bool hasBackup = settings.contains(backupKey(FanSide::Cpu)) || settings.contains(backupKey(FanSide::Gpu));
    settings.endGroup();
    return hasBackup;
}

void FanTargetController::saveCurveBackup() {
    QSettings settings(QStringLiteral("/etc/MsiController/settings.ini"), QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("FanTargetController"));
    settings.setValue(backupKey(FanSide::Cpu), readCurve(FanSide::Cpu));
    settings.setValue(backupKey(FanSide::Gpu), readCurve(FanSide::Gpu));

    if (Parameter* fanMode = parameter(Msi::Parametr::FanModeEc)) {
        settings.setValue(QStringLiteral("FanModeBackup"), static_cast<int>(fanMode->value().value<Msi::FanMode>()));
    }
    settings.endGroup();
}

void FanTargetController::restoreCurveBackup() {
    QSettings settings(QStringLiteral("/etc/MsiController/settings.ini"), QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("FanTargetController"));
    const QVariantList cpuCurve = settings.value(backupKey(FanSide::Cpu)).toList();
    const QVariantList gpuCurve = settings.value(backupKey(FanSide::Gpu)).toList();
    const int fanModeBackup = settings.value(QStringLiteral("FanModeBackup"), -1).toInt();
    settings.endGroup();

    writeCurve(FanSide::Cpu, cpuCurve);
    writeCurve(FanSide::Gpu, gpuCurve);
    mLastCpuCurve.clear();
    mLastGpuCurve.clear();

    if (fanModeBackup >= 0) {
        if (Parameter* fanMode = parameter(Msi::Parametr::FanModeEc)) {
            fanMode->setValue(QVariant::fromValue(static_cast<Msi::FanMode>(fanModeBackup)));
        }
    }
}

QVariantList FanTargetController::readCurve(FanSide side) const {
    QVariantList values;
    values.reserve(13);
    for (int i = 0; i < 7; ++i) {
        values.append(parameterInt(parameter(speedParam(side, i))));
    }
    for (int i = 0; i < 6; ++i) {
        values.append(parameterInt(parameter(curveTempParam(side, i))));
    }
    return values;
}

void FanTargetController::writeCurve(FanSide side, const QVariantList& values) {
    if (!isValidCurveList(values)) {
        return;
    }

    for (int i = 0; i < 7; ++i) {
        if (Parameter* speed = parameter(speedParam(side, i))) {
            speed->setValue(values.at(i));
        }
    }
    for (int i = 0; i < 6; ++i) {
        if (Parameter* temp = parameter(curveTempParam(side, i))) {
            temp->setValue(values.at(i + 7));
        }
    }
}
