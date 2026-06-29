#include "ecparameterfactory.h"

#include <QDebug>
#include <limits>
#include <optional>
#include <type_traits>

#include "ecservice.h"
#include "fantargetcontroller.h"
#include "ioparameter.h"
#include "softwareparameter.h"
#include "struct.h"

namespace {
template <typename T>
std::optional<T> parseHexValue(const QString& rawValue, const QString& key) {
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T>);

    bool ok = false;
    const qulonglong parsedValue = rawValue.trimmed().toULongLong(&ok, 16);
    if (!ok || parsedValue > std::numeric_limits<T>::max()) {
        qWarning() << "Invalid config value for" << key << ":" << rawValue;
        return std::nullopt;
    }
    return static_cast<T>(parsedValue);
}

template <typename T>
std::optional<T> configHexValue(const QMap<QString, QVariant>& config, const QString& key) {
    const auto it = config.constFind(key);
    if (it == config.cend()) {
        return std::nullopt;
    }
    return parseHexValue<T>(it.value().toString(), key);
}

QStringList configStringList(const QMap<QString, QVariant>& config, const QString& key) {
    const QVariant value = config.value(key);
    QStringList result = value.toStringList();
    if (!result.isEmpty()) {
        return result;
    }

    const QVariantList list = value.toList();
    for (const QVariant& item : list) {
        const QString text = item.toString().trimmed();
        if (!text.isEmpty()) {
            result.append(text);
        }
    }
    if (!result.isEmpty()) {
        return result;
    }

    const QString text = value.toString();
    for (const QString& item : text.split(',', Qt::SkipEmptyParts)) {
        const QString trimmed = item.trimmed();
        if (!trimmed.isEmpty()) {
            result.append(trimmed);
        }
    }
    return result;
}

void registerSimpleRangeParameter(EcService& service,
                                  IOBuffer* ioBuffer,
                                  const QMap<QString, QVariant>& config,
                                  const QString& key,
                                  Msi::Parametr parameter,
                                  const Msi::Range& range) {
    if (const auto address = configHexValue<quint16>(config, key)) {
        service.registerParameter(new IOParameter<quint8>(ioBuffer,
                                                          *address,
                                                          QVariant::fromValue(parameter),
                                                          QVariant::fromValue(range),
                                                          true));
    }
}

void registerEnableParameter(EcService& service,
                             IOBuffer* ioBuffer,
                             const QMap<QString, QVariant>& config,
                             const QString& addressKey,
                             Msi::Parametr parameter,
                             const QString& maskKey,
                             quint8 offValue,
                             quint8 onValue) {
    if (const auto address = configHexValue<quint16>(config, addressKey)) {
        const auto mask = configHexValue<quint8>(config, maskKey).value_or(0xff);
        auto p{
            new IOParameter<Msi::Enable>(ioBuffer,
                                         *address,
                                         QVariant::fromValue(parameter),
                                         QVariant::fromValue(QVector<Msi::Enable>{Msi::Enable::Off, Msi::Enable::On}),
                                         false,
                                         mask)};
        p->setEnumHash({{Msi::Enable::Off, offValue}, {Msi::Enable::On, onValue == 0xff ? mask : onValue}});
        service.registerParameter(p);
    }
}

void registerFanCurveParameters(EcService& service, IOBuffer* ioBuffer) {
    for (int i{0}; i < 7; ++i) {
        service.registerParameter(new IOParameter<quint8>(
            ioBuffer,
            static_cast<quint16>(0x72 + i),
            QVariant::fromValue(static_cast<Msi::Parametr>(static_cast<int>(Msi::Parametr::FanSetSpeedCpu1Ec) + i)),
            QVariant::fromValue(Msi::Range{0, 150}),
            false));
        service.registerParameter(new IOParameter<quint8>(
            ioBuffer,
            static_cast<quint16>(0x8A + i),
            QVariant::fromValue(static_cast<Msi::Parametr>(static_cast<int>(Msi::Parametr::FanSetSpeedGpu1Ec) + i)),
            QVariant::fromValue(Msi::Range{0, 150}),
            false));
    }

    for (int i{0}; i < 6; ++i) {
        service.registerParameter(new IOParameter<quint8>(
            ioBuffer,
            static_cast<quint16>(0x6A + i),
            QVariant::fromValue(static_cast<Msi::Parametr>(static_cast<int>(Msi::Parametr::FanSetTempCpu1Ec) + i)),
            QVariant::fromValue(Msi::Range{0, 100}),
            false));
        service.registerParameter(new IOParameter<quint8>(
            ioBuffer,
            static_cast<quint16>(0x82 + i),
            QVariant::fromValue(static_cast<Msi::Parametr>(static_cast<int>(Msi::Parametr::FanSetTempGpu1Ec) + i)),
            QVariant::fromValue(Msi::Range{0, 100}),
            false));
    }
}

void registerFanMode(EcService& service, IOBuffer* ioBuffer, const QMap<QString, QVariant>& config) {
    if (!config.contains("FanModeEc") || !config.contains("FanModeAvailable")) {
        return;
    }

    QStringList modes{configStringList(config, "FanModeAvailable")};
    QHash<Msi::FanMode, quint8> fanModeMap;
    QVector<Msi::FanMode> fanModeVector;
    for (auto& mode : modes) {
        QStringList parts = mode.split(':');
        if (parts.size() == 2) {
            Msi::FanMode fanMode = QVariant("Msi::FanMode::" + parts[0]).value<Msi::FanMode>();
            if (const auto value = parseHexValue<quint8>(parts[1], "FanModeAvailable")) {
                fanModeMap.insert(fanMode, *value);
                fanModeVector.append(fanMode);
            }
        }
    }
    if (const auto address = configHexValue<quint16>(config, "FanModeEc")) {
        auto p{new IOParameter<Msi::FanMode>(ioBuffer,
                                             *address,
                                             QVariant::fromValue(Msi::Parametr::FanModeEc),
                                             QVariant::fromValue(fanModeVector),
                                             false)};
        p->setEnumHash(fanModeMap);
        service.registerParameter(p);
    }
}

void registerShiftMode(EcService& service, IOBuffer* ioBuffer, const QMap<QString, QVariant>& config) {
    if (!config.contains("ShiftModeEc") || !config.contains("ShiftModeAvailable")) {
        return;
    }

    QStringList modes{configStringList(config, "ShiftModeAvailable")};
    QHash<Msi::ShiftMode, quint8> shiftModeMap;
    QVector<Msi::ShiftMode> shiftModeVector;
    for (auto& mode : modes) {
        QStringList parts = mode.split(':');
        if (parts.size() == 2) {
            Msi::ShiftMode shiftMode = QVariant("Msi::ShiftMode::" + parts[0]).value<Msi::ShiftMode>();
            if (const auto value = parseHexValue<quint8>(parts[1], "ShiftModeAvailable")) {
                shiftModeMap.insert(shiftMode, *value);
                shiftModeVector.append(shiftMode);
            }
        }
    }
    if (const auto address = configHexValue<quint16>(config, "ShiftModeEc")) {
        auto p{new IOParameter<Msi::ShiftMode>(ioBuffer,
                                               *address,
                                               QVariant::fromValue(Msi::Parametr::ShiftModeEc),
                                               QVariant::fromValue(shiftModeVector),
                                               false)};
        p->setEnumHash(shiftModeMap);
        service.registerParameter(p);
    }
}
} // namespace

EcParameterFactory::EcParameterFactory(IOBuffer* ioBuffer) : mIoBuffer(ioBuffer) {}

void EcParameterFactory::registerFirmwareParameters(EcService& service) const {
    service.registerParameter(new IOParameterString(mIoBuffer,
                                                    0xA0,
                                                    QVariant::fromValue(Msi::Parametr::FirmwareVersionEc),
                                                    QVariant(QString("")),
                                                    true,
                                                    12));
    service.registerParameter(new IOParameterString(mIoBuffer,
                                                    0xAC,
                                                    QVariant::fromValue(Msi::Parametr::FirmwareReleaseDateEc),
                                                    QVariant(QString("")),
                                                    true,
                                                    8));
    service.registerParameter(new IOParameterString(mIoBuffer,
                                                    0xAC + 8,
                                                    QVariant::fromValue(Msi::Parametr::FirmwareReleaseTimeEc),
                                                    QVariant(QString("")),
                                                    true,
                                                    8));
}

void EcParameterFactory::registerProfileParameters(EcService& service, const QMap<QString, QVariant>& config) const {
    registerSimpleRangeParameter(service, mIoBuffer, config, "CpuTempEc", Msi::Parametr::CpuTempEc, Msi::Range{0, 100});
    registerSimpleRangeParameter(service, mIoBuffer, config, "GpuTempEc", Msi::Parametr::GpuTempEc, Msi::Range{0, 100});
    registerSimpleRangeParameter(service,
                                 mIoBuffer,
                                 config,
                                 "BatteryChargeEc",
                                 Msi::Parametr::BatteryChargeEc,
                                 Msi::Range{0, 100});

    if (const auto address = configHexValue<quint16>(config, "BatteryThresholdEc")) {
        service.registerParameter(new IOParameter<quint8>(mIoBuffer,
                                                          *address,
                                                          QVariant::fromValue(Msi::Parametr::BatteryThresholdEc),
                                                          QVariant::fromValue(QVector<int>{50, 70, 90}),
                                                          false,
                                                          0x7f));
    }
    if (const auto address = configHexValue<quint16>(config, "BatteryChargingStatusEc")) {
        auto p{new IOParameter<Msi::ChargingStatus>(
            mIoBuffer,
            *address,
            QVariant::fromValue(Msi::Parametr::BatteryChargingStatusEc),
            QVariant::fromValue(QVector<Msi::ChargingStatus>{Msi::ChargingStatus::BatteryCharging,
                                                             Msi::ChargingStatus::BatteryDischarging,
                                                             Msi::ChargingStatus::BatteryNotCharging,
                                                             Msi::ChargingStatus::BatteryFullyCharged,
                                                             Msi::ChargingStatus::BatteryFullyChargedNoPower}),
            true)};
        p->setEnumHash({{Msi::ChargingStatus::BatteryCharging, 0x03},
                        {Msi::ChargingStatus::BatteryDischarging, 0x05},
                        {Msi::ChargingStatus::BatteryNotCharging, 0x01},
                        {Msi::ChargingStatus::BatteryFullyCharged, 0x09},
                        {Msi::ChargingStatus::BatteryFullyChargedNoPower, 0x0D}});
        service.registerParameter(p);
    }
    if (const auto address = configHexValue<quint16>(config, "KeyboardBacklightModeEc")) {
        auto p{new IOParameter<Msi::Enable>(mIoBuffer,
                                            *address,
                                            QVariant::fromValue(Msi::Parametr::KeyboardBacklightModeEc),
                                            QVariant(QString("")),
                                            false)};
        p->setEnumHash({{Msi::Enable::Off, 0x08}, {Msi::Enable::On, 0x00}});
        if (config.contains("KeyboardBacklightMode")) {
            QStringList modes = configStringList(config, "KeyboardBacklightMode");
            if (modes.size() == 2) {
                const auto offValue = parseHexValue<quint8>(modes[0], "KeyboardBacklightMode");
                const auto onValue = parseHexValue<quint8>(modes[1], "KeyboardBacklightMode");
                if (offValue && onValue) {
                    p->setEnumHash({{Msi::Enable::Off, *offValue}, {Msi::Enable::On, *onValue}});
                }
            }
        }
        service.registerParameter(p);
    }
    if (const auto address = configHexValue<quint16>(config, "KeyboardBacklightEc")) {
        const auto startState = configHexValue<quint8>(config, "KeyboardBacklightStartState").value_or(0);
        auto p{new IOParameter<Msi::KeyboardBacklight>(
            mIoBuffer,
            *address,
            QVariant::fromValue(Msi::Parametr::KeyboardBacklightEc),
            QVariant::fromValue(QVector<Msi::KeyboardBacklight>{Msi::KeyboardBacklight::Off,
                                                                Msi::KeyboardBacklight::Low,
                                                                Msi::KeyboardBacklight::Mid,
                                                                Msi::KeyboardBacklight::High}),
            false)};
        p->setEnumHash({{Msi::KeyboardBacklight::Off, startState},
                        {Msi::KeyboardBacklight::Low, static_cast<quint8>(startState + 0x01)},
                        {Msi::KeyboardBacklight::Mid, static_cast<quint8>(startState + 0x02)},
                        {Msi::KeyboardBacklight::High, static_cast<quint8>(startState + 0x03)}});
        service.registerParameter(p);
    }

    registerEnableParameter(service,
                            mIoBuffer,
                            config,
                            "UsbPowerShareEc",
                            Msi::Parametr::UsbPowerShareEc,
                            {},
                            0x08,
                            0x28);
    registerEnableParameter(service,
                            mIoBuffer,
                            config,
                            "CoolerBoostEc",
                            Msi::Parametr::CoolerBoostEc,
                            "CoolerBoostMask",
                            0x00,
                            0x80);
    registerEnableParameter(service, mIoBuffer, config, "WebCamEc", Msi::Parametr::WebCamEc, "WebCamMask", 0x00, 0xff);
    registerEnableParameter(service,
                            mIoBuffer,
                            config,
                            "WebCamBlockEc",
                            Msi::Parametr::WebCamBlockEc,
                            "WebCamMask",
                            0x00,
                            0xff);

    if (const auto address = configHexValue<quint16>(config, "FnSuperSwapEc")) {
        const auto mask = configHexValue<quint8>(config, "FnSuperSwapMask").value_or(0xff);
        bool fnWinSwapInvert{config.value("FnWinSwapInvert", false).toBool()};
        auto p{new IOParameter<Msi::FnSuperSwap>(
            mIoBuffer,
            *address,
            QVariant::fromValue(Msi::Parametr::FnSuperSwapEc),
            QVariant::fromValue(QVector<Msi::FnSuperSwap>{Msi::FnSuperSwap::Right, Msi::FnSuperSwap::Left}),
            false,
            mask)};
        if (fnWinSwapInvert) {
            p->setEnumHash({{Msi::FnSuperSwap::Right, 0x10}, {Msi::FnSuperSwap::Left, 0x00}});
        } else {
            p->setEnumHash({{Msi::FnSuperSwap::Right, 0x00}, {Msi::FnSuperSwap::Left, 0x10}});
        }
        service.registerParameter(p);
    }

    registerSimpleRangeParameter(service, mIoBuffer, config, "FanCpuEc", Msi::Parametr::FanCpuEc, Msi::Range{0, 100});
    registerFanMode(service, mIoBuffer, config);
    registerSimpleRangeParameter(service, mIoBuffer, config, "FanGpuEc", Msi::Parametr::FanGpuEc, Msi::Range{0, 100});
    registerFanCurveParameters(service, mIoBuffer);

    service.registerParameter(
        new SoftwareParameter(QVariant::fromValue(Msi::Parametr::FanControlMode),
                              QVariant::fromValue(QList<Msi::FanControlMode>{Msi::FanControlMode::Curve,
                                                                             Msi::FanControlMode::TargetTemperature}),
                              QVariant::fromValue(Msi::FanControlMode::Curve),
                              &service));
    service.registerParameter(new SoftwareParameter(QVariant::fromValue(Msi::Parametr::FanTargetCpuTemp),
                                                    QVariant::fromValue(Msi::Range{50, 95}),
                                                    78,
                                                    &service));
    service.registerParameter(new SoftwareParameter(QVariant::fromValue(Msi::Parametr::FanTargetGpuTemp),
                                                    QVariant::fromValue(Msi::Range{50, 95}),
                                                    76,
                                                    &service));
    service.registerProfileObject(new FanTargetController(&service, &service));

    registerShiftMode(service, mIoBuffer, config);
    registerEnableParameter(service,
                            mIoBuffer,
                            config,
                            "SuperBatteryEc",
                            Msi::Parametr::SuperBatteryEc,
                            "SuperBatteryMask",
                            0x00,
                            0xff);
    registerEnableParameter(service, mIoBuffer, config, "MicMuteEc", Msi::Parametr::MicMuteEc, "LedsMask", 0x00, 0xff);
    registerEnableParameter(service, mIoBuffer, config, "MuteLedEc", Msi::Parametr::MuteLedEc, "LedsMask", 0x00, 0xff);
}
