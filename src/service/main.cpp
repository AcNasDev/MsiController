#include <QCoreApplication>
#include <QDBusConnection>
#include <QDebug>
#include <QtConcurrent/QtConcurrentRun>
#include <bitset>
#include <limits>
#include <optional>
#include <type_traits>

#include "cpuparameter.h"
#include "ecadaptor.h"
#include "ecservice.h"
#include "ioparameter.h"
#include "struct.h"

bool loadEcSysModule();
void registerEsSys(EcService& service);

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
} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    // app.setApplicationName("msi-ec-service");
    EcService service;
    EcAdaptor ecAdaptor(&service);

    QDBusConnection connection = QDBusConnection::systemBus();
    if (connection.isConnected() && connection.registerObject("/", &service)) {
        if (!connection.registerService("com.msi.ec")) {
            qCritical("Failed to register D-Bus service on system bus.");
            return 1;
        } else {
            qInfo("D-Bus service registered system successfully.");
        }
    } else {
        qCritical("Failed to connect to system D-Bus.");
        return 1;
    }

    QObject::connect(&service, &EcService::parameterChanged, [](const QDBusVariant& name, const QDBusVariant& value) {
        qDebug() << "Parameter changed:" << name.variant().value<Msi::Msg>().variant
                 << "new value:" << value.variant().value<Msi::Msg>().variant;
    });

    registerMetaType();
    registerEsSys(service);

    return app.exec();
}

QMap<QString, QVariant> getConfig(const QString& firmavareVersion) {
    QString configSelect;
    QSettings settings(":/settings.ini", QSettings::IniFormat);
    for (auto& g : settings.childGroups()) {
        settings.beginGroup(g);
        auto allowedFw{settings.value("AllowedFw", "")};
        if (allowedFw.canConvert<QString>() && allowedFw.toString() == firmavareVersion) {
            configSelect = g;
            settings.endGroup();
            break;
        } else if (allowedFw.canConvert<QStringList>() && allowedFw.toStringList().contains(firmavareVersion)) {
            configSelect = g;
            settings.endGroup();
            break;
        }
        settings.endGroup();
    }
    qDebug() << "Config selected:" << configSelect;
    if (configSelect.isEmpty()) {
        qWarning() << "No valid config found for firmware:" << firmavareVersion;
        return {};
    }
    QMap<QString, QVariant> config;
    settings.beginGroup("DEFAULT");
    for (auto& key : settings.childKeys()) {
        config.insert(key, settings.value(key));
    }
    settings.endGroup();
    settings.beginGroup(configSelect);
    for (auto& key : settings.childKeys()) {
        config.insert(key, settings.value(key));
    }
    settings.endGroup();
    qDebug() << "Config loaded:";
    auto keys = config.keys();
    for (const auto& key : std::as_const(keys)) {
        qDebug() << key + ": " + config.value(key).toString();
    }
    return config;
}

void registerEsSys(EcService& service) {
    QString bufferName{"/sys/devices/platform/msiec/ec"};
    if (!QFileInfo::exists(bufferName)) {
        bufferName = "/sys/kernel/debug/ec/ec0/io";
        QProcess process;
        process.start("sh", QStringList() << "-c" << "modprobe ec_sys write_support=1 2>&1");
        process.waitForFinished(1000);
        if (!QFileInfo::exists(bufferName)) {
            qWarning() << "EC sysfs path does not exist:" << bufferName;
            return;
        }
    }

    IOBuffer* ioBuffer = new IOBuffer(bufferName, &service);
    bool isIoBufferReady = ioBuffer->buffer().size() > 0;
    qDebug() << "IOBuffer size:" << ioBuffer->buffer().size() << ioBuffer->buffer().mid(0xa0, 12);

    if (isIoBufferReady) {
        service.registerParameter(new IOParameterString(ioBuffer,
                                                        0xA0,
                                                        QVariant::fromValue(Msi::Parametr::FirmwareVersionEc),
                                                        QVariant(QString("")),
                                                        true,
                                                        12));
        service.registerParameter(new IOParameterString(ioBuffer,
                                                        0xAC,
                                                        QVariant::fromValue(Msi::Parametr::FirmwareReleaseDateEc),
                                                        QVariant(QString("")),
                                                        true,
                                                        8));
        service.registerParameter(new IOParameterString(ioBuffer,
                                                        0xAC + 8,
                                                        QVariant::fromValue(Msi::Parametr::FirmwareReleaseTimeEc),
                                                        QVariant(QString("")),
                                                        true,
                                                        8));
        emit ioBuffer->bufferChanged(ioBuffer->buffer());

        QString currentFw{service.parameter(QVariant::fromValue(Msi::Parametr::FirmwareVersionEc))->value().toString()};
        qDebug() << "Current firmware version:" << currentFw;
        auto config = getConfig(currentFw);
        if (config.isEmpty()) {
            qWarning() << "No configuration found for firmware version:" << currentFw;
            return;
        }

        if (const auto address = configHexValue<quint16>(config, "CpuTempEc")) {
            service.registerParameter(new IOParameter<quint8>(ioBuffer,
                                                              *address,
                                                              QVariant::fromValue(Msi::Parametr::CpuTempEc),
                                                              QVariant::fromValue(Msi::Range{0, 100}),
                                                              true));
        }
        if (const auto address = configHexValue<quint16>(config, "GpuTempEc")) {
            service.registerParameter(new IOParameter<quint8>(ioBuffer,
                                                              *address,
                                                              QVariant::fromValue(Msi::Parametr::GpuTempEc),
                                                              QVariant::fromValue(Msi::Range{0, 100}),
                                                              true));
        }
        if (const auto address = configHexValue<quint16>(config, "BatteryChargeEc")) {
            service.registerParameter(new IOParameter<quint8>(ioBuffer,
                                                              *address,
                                                              QVariant::fromValue(Msi::Parametr::BatteryChargeEc),
                                                              QVariant::fromValue(Msi::Range{0, 100}),
                                                              true));
        }
        if (const auto address = configHexValue<quint16>(config, "BatteryThresholdEc")) {
            service.registerParameter(new IOParameter<quint8>(ioBuffer,
                                                              *address,
                                                              QVariant::fromValue(Msi::Parametr::BatteryThresholdEc),
                                                              QVariant::fromValue(QVector<int>{50, 70, 90}),
                                                              false,
                                                              0x7f));
        }
        if (const auto address = configHexValue<quint16>(config, "BatteryChargingStatusEc")) {
            auto p{new IOParameter<Msi::ChargingStatus>(
                ioBuffer,
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
            auto p{new IOParameter<Msi::Enable>(ioBuffer,
                                                *address,
                                                QVariant::fromValue(Msi::Parametr::KeyboardBacklightModeEc),
                                                QVariant(QString("")),
                                                false)};
            p->setEnumHash({{Msi::Enable::Off, 0x08}, {Msi::Enable::On, 0x00}});
            if (config.contains("KeyboardBacklightMode")) {
                QStringList modes = config.value("KeyboardBacklightMode").toStringList();
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
                ioBuffer,
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
        if (const auto address = configHexValue<quint16>(config, "UsbPowerShareEc")) {
            auto p{new IOParameter<Msi::Enable>(
                ioBuffer,
                *address,
                QVariant::fromValue(Msi::Parametr::UsbPowerShareEc),
                QVariant::fromValue(QVector<Msi::Enable>{Msi::Enable::Off, Msi::Enable::On}),
                false)};
            p->setEnumHash({{Msi::Enable::Off, 0x08}, {Msi::Enable::On, 0x28}});
            service.registerParameter(p);
        }
        if (const auto address = configHexValue<quint16>(config, "CoolerBoostEc")) {
            const auto mask = configHexValue<quint8>(config, "CoolerBoostMask").value_or(0xff);
            auto p{new IOParameter<Msi::Enable>(
                ioBuffer,
                *address,
                QVariant::fromValue(Msi::Parametr::CoolerBoostEc),
                QVariant::fromValue(QVector<Msi::Enable>{Msi::Enable::Off, Msi::Enable::On}),
                false,
                mask)};
            p->setEnumHash({{Msi::Enable::Off, 0x00}, {Msi::Enable::On, 0x80}});
            service.registerParameter(p);
        }
        if (const auto address = configHexValue<quint16>(config, "WebCamEc")) {
            const auto mask = configHexValue<quint8>(config, "WebCamMask").value_or(0xff);
            auto p{new IOParameter<Msi::Enable>(
                ioBuffer,
                *address,
                QVariant::fromValue(Msi::Parametr::WebCamEc),
                QVariant::fromValue(QVector<Msi::Enable>{Msi::Enable::Off, Msi::Enable::On}),
                false,
                mask)};
            p->setEnumHash({{Msi::Enable::Off, 0x00}, {Msi::Enable::On, mask}});
            service.registerParameter(p);
        }
        if (const auto address = configHexValue<quint16>(config, "WebCamBlockEc")) {
            const auto mask = configHexValue<quint8>(config, "WebCamMask").value_or(0xff);
            auto p{new IOParameter<Msi::Enable>(
                ioBuffer,
                *address,
                QVariant::fromValue(Msi::Parametr::WebCamBlockEc),
                QVariant::fromValue(QVector<Msi::Enable>{Msi::Enable::Off, Msi::Enable::On}),
                false,
                mask)};
            p->setEnumHash({{Msi::Enable::Off, 0x00}, {Msi::Enable::On, mask}});
            service.registerParameter(p);
        }
        if (const auto address = configHexValue<quint16>(config, "FnSuperSwapEc")) {
            const auto mask = configHexValue<quint8>(config, "FnSuperSwapMask").value_or(0xff);
            bool fnWinSwapInvert{config.value("FnWinSwapInvert", false).toBool()};
            auto p{new IOParameter<Msi::FnSuperSwap>(
                ioBuffer,
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
        // if(quint8 val{ static_cast<quint8>(ioBuffer->buffer()[0xCD]) };
        //     val > 0) {
        //     service.registerParameter(new IOParameter<quint16>(ioBuffer, 0xCC,
        //     QVariant::fromValue(Msi::Parametr::FanCpuEc), QVariant(), true, 0xFFFF, QDataStream::BigEndian));
        // } else if(quint8 val{ static_cast<quint8>(ioBuffer->buffer()[0xC9]) };
        //     val > 0 && val < 50) {
        //     service.registerParameter(new IOParameter<quint16>(ioBuffer, 0xCC,
        //     QVariant::fromValue(Msi::Parametr::FanCpuEc), QVariant(), true, 0xFFFF, QDataStream::BigEndian));
        // } else {
        //     service.registerParameter(new IOParameter<quint16>(ioBuffer, 0xC8,
        //     QVariant::fromValue(Msi::Parametr::FanCpuEc), QVariant(), true, 0xFFFF, QDataStream::BigEndian));
        // }
        // service.registerParameter(new IOParameter<quint16>(ioBuffer, 0xCA,
        // QVariant::fromValue(Msi::Parametr::FanGpuEc), QVariant(), true, 0xFFFF, QDataStream::BigEndian));

        if (const auto address = configHexValue<quint16>(config, "FanCpuEc")) {
            service.registerParameter(new IOParameter<quint8>(ioBuffer,
                                                              *address,
                                                              QVariant::fromValue(Msi::Parametr::FanCpuEc),
                                                              QVariant::fromValue(Msi::Range{0, 100}),
                                                              true));
        }
        if (config.contains("FanModeEc") && config.contains("FanModeAvailable")) {
            QStringList modes{config.value("FanModeAvailable").toStringList()};
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
        if (const auto address = configHexValue<quint16>(config, "FanGpuEc")) {
            service.registerParameter(new IOParameter<quint8>(ioBuffer,
                                                              *address,
                                                              QVariant::fromValue(Msi::Parametr::FanGpuEc),
                                                              QVariant::fromValue(Msi::Range{0, 100}),
                                                              true));
        }

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

        if (config.contains("ShiftModeEc") && config.contains("ShiftModeAvailable")) {
            QStringList modes{config.value("ShiftModeAvailable").toStringList()};
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

        if (const auto address = configHexValue<quint16>(config, "SuperBatteryEc")) {
            const auto mask = configHexValue<quint8>(config, "SuperBatteryMask").value_or(0xff);
            auto p{new IOParameter<Msi::Enable>(
                ioBuffer,
                *address,
                QVariant::fromValue(Msi::Parametr::SuperBatteryEc),
                QVariant::fromValue(QVector<Msi::Enable>{Msi::Enable::Off, Msi::Enable::On}),
                false,
                mask)};
            p->setEnumHash({{Msi::Enable::Off, 0x00}, {Msi::Enable::On, mask}});
            service.registerParameter(p);
        }

        if (const auto address = configHexValue<quint16>(config, "MicMuteEc")) {
            const auto mask = configHexValue<quint8>(config, "LedsMask").value_or(0xff);
            auto p{new IOParameter<Msi::Enable>(
                ioBuffer,
                *address,
                QVariant::fromValue(Msi::Parametr::MicMuteEc),
                QVariant::fromValue(QVector<Msi::Enable>{Msi::Enable::Off, Msi::Enable::On}),
                false,
                mask)};
            p->setEnumHash({{Msi::Enable::Off, 0x00}, {Msi::Enable::On, mask}});
            service.registerParameter(p);
        }
        if (const auto address = configHexValue<quint16>(config, "MuteLedEc")) {
            const auto mask = configHexValue<quint8>(config, "LedsMask").value_or(0xff);
            auto p{new IOParameter<Msi::Enable>(
                ioBuffer,
                *address,
                QVariant::fromValue(Msi::Parametr::MuteLedEc),
                QVariant::fromValue(QVector<Msi::Enable>{Msi::Enable::Off, Msi::Enable::On}),
                false,
                mask)};
            p->setEnumHash({{Msi::Enable::Off, 0x00}, {Msi::Enable::On, mask}});
            service.registerParameter(p);
        }
    }
    service.registerParameter(new CpuParameter(QVariant::fromValue(Msi::Parametr::CpuConfig), &service));
}
