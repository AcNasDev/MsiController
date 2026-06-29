#include "autoprofilecontroller.h"

#include <QSettings>

#include "parameterclient.h"
#include "proxyparameter.h"

namespace {
constexpr auto settingsGroup = "AutoProfile";
constexpr auto enabledKey = "Enabled";
constexpr int refreshIntervalMs = 10000;
constexpr int lowBatteryPercent = 35;
constexpr int highTemperatureC = 86;

bool isValid(ProxyParameter* parameter) {
    return parameter && parameter->isValid();
}

int numberValue(ProxyParameter* parameter, int fallback = 0) {
    return isValid(parameter) ? parameter->value().toInt() : fallback;
}

bool shiftAvailable(const QVariant& available, Msi::ShiftMode mode) {
    const auto list = available.value<QList<Msi::ShiftMode>>();
    if (!list.isEmpty()) {
        return list.contains(mode);
    }
    const auto vector = available.value<QVector<Msi::ShiftMode>>();
    return vector.isEmpty() || vector.contains(mode);
}
} // namespace

AutoProfileController::AutoProfileController(ParameterClient* parameters, QObject* parent)
    : QObject(parent), mParameters(parameters) {
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(settingsGroup));
    mEnabled = settings.value(QString::fromLatin1(enabledKey), false).toBool();
    settings.endGroup();

    mTimer.setInterval(refreshIntervalMs);
    connect(&mTimer, &QTimer::timeout, this, &AutoProfileController::evaluate);

    for (const auto name : {Msi::Parametr::BatteryChargeEc,
                            Msi::Parametr::BatteryChargingStatusEc,
                            Msi::Parametr::CpuTempEc,
                            Msi::Parametr::GpuTempEc,
                            Msi::Parametr::ShiftModeEc}) {
        if (ProxyParameter* proxy = parameter(name)) {
            connect(proxy, &ProxyParameter::valueChanged, this, &AutoProfileController::evaluate);
            connect(proxy, &ProxyParameter::isValidChanged, this, &AutoProfileController::evaluate);
        }
    }

    if (mEnabled) {
        mTimer.start();
        QTimer::singleShot(0, this, &AutoProfileController::evaluate);
    } else {
        setStatus(tr("Auto profile is off"));
    }
}

bool AutoProfileController::isEnabled() const {
    return mEnabled;
}

QString AutoProfileController::status() const {
    return mStatus;
}

void AutoProfileController::setEnabled(bool enabled) {
    if (mEnabled == enabled) {
        return;
    }

    mEnabled = enabled;
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(settingsGroup));
    settings.setValue(QString::fromLatin1(enabledKey), mEnabled);
    settings.endGroup();

    if (mEnabled) {
        mTimer.start();
        evaluate();
    } else {
        mTimer.stop();
        setStatus(tr("Auto profile is off"));
    }
    emit enabledChanged();
}

void AutoProfileController::evaluate() {
    if (!mEnabled) {
        return;
    }

    ProxyParameter* shift = parameter(Msi::Parametr::ShiftModeEc);
    if (!isValid(shift)) {
        setStatus(tr("Shift mode is unavailable"));
        return;
    }
    if (shift->isPending()) {
        setStatus(tr("Waiting for shift mode confirmation"));
        return;
    }

    QString reason;
    const auto desired = desiredMode(&reason);
    if (!desired) {
        setStatus(tr("Waiting for telemetry"));
        return;
    }
    if (!shiftModeAvailable(*desired)) {
        setStatus(tr("Required shift mode is unavailable"));
        return;
    }

    if (shift->value().value<Msi::ShiftMode>() != *desired) {
        shift->setValue(QVariant::fromValue(*desired));
    }
    setStatus(reason);
}

void AutoProfileController::setStatus(const QString& status) {
    if (mStatus == status) {
        return;
    }

    mStatus = status;
    emit statusChanged();
}

ProxyParameter* AutoProfileController::parameter(Msi::Parametr name) const {
    return mParameters ? mParameters->getProxyParameter(name) : nullptr;
}

std::optional<Msi::ShiftMode> AutoProfileController::desiredMode(QString* reason) const {
    ProxyParameter* battery = parameter(Msi::Parametr::BatteryChargeEc);
    ProxyParameter* batteryStatus = parameter(Msi::Parametr::BatteryChargingStatusEc);
    ProxyParameter* cpuTemp = parameter(Msi::Parametr::CpuTempEc);
    ProxyParameter* gpuTemp = parameter(Msi::Parametr::GpuTempEc);

    const bool hasBatterySignal = isValid(battery) || isValid(batteryStatus);
    const bool hasThermalSignal = isValid(cpuTemp) || isValid(gpuTemp);
    if (!hasBatterySignal && !hasThermalSignal) {
        return std::nullopt;
    }

    const bool discharging = isValid(batteryStatus) && batteryStatus->value().value<Msi::ChargingStatus>() ==
                                                           Msi::ChargingStatus::BatteryDischarging;
    const int batteryPercent = numberValue(battery, 100);
    if (discharging || batteryPercent <= lowBatteryPercent) {
        if (reason) {
            *reason = tr("Auto profile selected Eco for battery");
        }
        return Msi::ShiftMode::Eco;
    }

    const int hottest = qMax(numberValue(cpuTemp), numberValue(gpuTemp));
    if (hottest >= highTemperatureC) {
        if (reason) {
            *reason = tr("Auto profile selected Turbo for thermals");
        }
        return Msi::ShiftMode::Turbo;
    }

    if (reason) {
        *reason = tr("Auto profile selected Comfort");
    }
    return Msi::ShiftMode::Comfort;
}

bool AutoProfileController::shiftModeAvailable(Msi::ShiftMode mode) const {
    ProxyParameter* shift = parameter(Msi::Parametr::ShiftModeEc);
    if (!isValid(shift)) {
        return false;
    }

    const QVariant available = shift->availableValues();
    return !available.isValid() || shiftAvailable(available, mode);
}
