#include "ecservice.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDebug>

#include <bitset>

#include "ecadaptor.h"
#include "ioparameter.h"
#include "struct.h"

bool loadEcSysModule();
void registerEsSys(EcService &service);



int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("msi-ec-service");
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

    QObject::connect(&service, &EcService::parameterChanged,
                     [](const QDBusVariant &name, const QDBusVariant &value) {
                         qDebug() << "Parameter changed:" << name.variant().value<Msi::Msg>().variant 
                            << "new value:" << value.variant().value<Msi::Msg>().variant;
                     });

    
    registerMetaType();
    registerEsSys(service);

    return app.exec();
}

QMap<QString, QVariant> getConfig(const QString &firmavareVersion) {
        QString configSelect;
        QSettings settings(":/settings.ini", QSettings::IniFormat);
        for(auto &g : settings.childGroups()) {
            settings.beginGroup(g);
            auto allowedFw{ settings.value("AllowedFw", "") };
            if(allowedFw.canConvert<QString>() && allowedFw.toString() == firmavareVersion) {
                configSelect = g;
                settings.endGroup();
                break;
            } else if(allowedFw.canConvert<QStringList>() && allowedFw.toStringList().contains(firmavareVersion)) {
                auto allowedFwList = allowedFw.toStringList();
                configSelect = g;
                settings.endGroup();
                break;
            }
            settings.endGroup();
        }
        qDebug() << "Config selected:" << configSelect;
        if(configSelect.isEmpty()) {
            qWarning() << "No valid config found for firmware:" << firmavareVersion;
            return {};
        }
        QMap<QString, QVariant> config;
        settings.beginGroup("DEFAULT");
        for(auto &key : settings.childKeys()) {
            config.insert(key, settings.value(key));
        }
        settings.endGroup();
        settings.beginGroup(configSelect);
        for(auto &key : settings.childKeys()) {
            config.insert(key, settings.value(key));
        }
        settings.endGroup();
        qDebug() << "Config loaded:";
        for(auto &key : config.keys()) {
            qDebug() << key + ": " + config.value(key).toString();
        }
        return config;
}

void registerEsSys(EcService &service)
{
    QString bufferName{ "/sys/devices/platform/msiec/ec" };
    if(!QFileInfo::exists(bufferName)) {
        bufferName = "/sys/kernel/debug/ec/ec0/io";
        QProcess process;
        process.start("sh", QStringList() << "-c" << "modprobe ec_sys write_support=1 2>&1");
        process.waitForFinished(1000);
        if(!QFileInfo::exists(bufferName)) {
            qWarning() << "EC sysfs path does not exist:" << bufferName;
            return;
        }
    }

    IOBuffer *ioBuffer = new IOBuffer(bufferName, &service);
    bool isIoBufferReady = ioBuffer->buffer().size() > 0;
    qDebug() << "IOBuffer size:" << ioBuffer->buffer().size() << ioBuffer->buffer().mid(0xa0, 12);

    if (isIoBufferReady) {

        service.registerParameter(new IOParameterString(ioBuffer, 0xA0, QVariant::fromValue(Msi::Parametr::FirmwareVersionEc), QVariant(QString("")), true, 12));
        service.registerParameter(new IOParameterString(ioBuffer, 0xAC, QVariant::fromValue(Msi::Parametr::FirmwareReleaseDateEc), QVariant(QString("")), true, 8));
        service.registerParameter(new IOParameterString(ioBuffer, 0xAC + 8, QVariant::fromValue(Msi::Parametr::FirmwareReleaseTimeEc), QVariant(QString("")), true, 8));
        ioBuffer->bufferChanged(ioBuffer->buffer());

        QString currentFw{ service.parameter(QVariant::fromValue(Msi::Parametr::FirmwareVersionEc))->value().toString() };
        qDebug() << "Current firmware version:" << currentFw;
        auto config = getConfig(currentFw);
        if(config.isEmpty()) {
            qWarning() << "No configuration found for firmware version:" << currentFw;
            return;
        }

        if(config.contains("CpuTempEc")) {
            uint address{ config.value("CpuTempEc").toString().toUInt(nullptr, 16) };
            service.registerParameter(new IOParameter<quint8>(ioBuffer, address, QVariant::fromValue(Msi::Parametr::CpuTempEc), QVariant::fromValue(Msi::Range{0, 100}), true));
        }
        if(config.contains("GpuTempEc")) {
            uint address{ config.value("GpuTempEc").toString().toUInt(nullptr, 16) };
            service.registerParameter(new IOParameter<quint8>(ioBuffer, address, QVariant::fromValue(Msi::Parametr::GpuTempEc), QVariant::fromValue(Msi::Range{0, 100}), true));
        }
        if(config.contains("BatteryChargeEc")) {
            uint address{ config.value("BatteryChargeEc").toString().toUInt(nullptr, 16) };
            service.registerParameter(new IOParameter<quint8>(ioBuffer, address, QVariant::fromValue(Msi::Parametr::BatteryChargeEc), QVariant::fromValue(Msi::Range{0, 100}), true));
        }
        if(config.contains("BatteryThresholdEc")) {
            uint address{ config.value("BatteryThresholdEc").toString().toUInt(nullptr, 16) };
            service.registerParameter(new IOParameter<quint8>(ioBuffer, address, QVariant::fromValue(Msi::Parametr::BatteryThresholdEc), QVariant::fromValue(QVector<int>{50, 70, 90}), false, 0x7f));
        }
        if(config.contains("BatteryChargingStatusEc")) {
            uint address{ config.value("BatteryChargingStatusEc").toString().toUInt(nullptr, 16) };
            auto p{ new IOParameter<Msi::ChargingStatus>(ioBuffer, 
                address, 
                QVariant::fromValue(Msi::Parametr::BatteryChargingStatusEc), 
                QVariant::fromValue(QVector<Msi::ChargingStatus>{
                    Msi::ChargingStatus::BatteryCharging, 
                    Msi::ChargingStatus::BatteryDischarging, 
                    Msi::ChargingStatus::BatteryNotCharging, 
                    Msi::ChargingStatus::BatteryFullyCharged, 
                    Msi::ChargingStatus::BatteryFullyChargedNoPower}), 
                    true)};
            p->setEnumHash({
                { Msi::ChargingStatus::BatteryCharging, 0x03 },
                { Msi::ChargingStatus::BatteryDischarging, 0x05 },
                { Msi::ChargingStatus::BatteryNotCharging, 0x01 },
                { Msi::ChargingStatus::BatteryFullyCharged, 0x09 },
                { Msi::ChargingStatus::BatteryFullyChargedNoPower, 0x0D }
            });
            service.registerParameter(p);
        }
        if(config.contains("KeyboardBacklightModeEc")) {
            uint address{ config.value("KeyboardBacklightModeEc").toString().toUInt(nullptr, 16) };
            auto p{ new IOParameter<Msi::Enable>(ioBuffer, address, QVariant::fromValue(Msi::Parametr::KeyboardBacklightModeEc), 
            QVariant(QString("")), false) };
            p->setEnumHash({
                { Msi::Enable::Off, 0x08 },
                { Msi::Enable::On, 0x00 }
            });
            if(config.contains("KeyboardBacklightMode")) {
                QStringList modes = config.value("KeyboardBacklightMode").toStringList();
                if(modes.size() == 2) {
                    p->setEnumHash({
                        { Msi::Enable::Off, modes[0].toUInt(nullptr, 16) },
                        { Msi::Enable::On, modes[1].toUInt(nullptr, 16) }
                    });
                }
            }
            service.registerParameter(p);
        }
        if(config.contains("KeyboardBacklightEc")) {
            uint address{ config.value("KeyboardBacklightEc").toString().toUInt(nullptr, 16) };
            uint startState{ config.value("KeyboardBacklightStartState").toString().toUInt(nullptr, 16) };
            auto p{ new IOParameter<Msi::KeyboardBacklight>(ioBuffer, address, QVariant::fromValue(Msi::Parametr::KeyboardBacklightEc), 
            QVariant::fromValue(QVector<Msi::KeyboardBacklight>{
                Msi::KeyboardBacklight::Off,
                Msi::KeyboardBacklight::Low,
                Msi::KeyboardBacklight::Mid,
                Msi::KeyboardBacklight::High
            }), false) };
            p->setEnumHash({
                { Msi::KeyboardBacklight::Off, startState },
                { Msi::KeyboardBacklight::Low, startState + 0x01 },
                { Msi::KeyboardBacklight::Mid, startState + 0x02 },
                { Msi::KeyboardBacklight::High, startState + 0x03 }
            });
            service.registerParameter(p);
        }
        if(config.contains("UsbPowerShareEc")) {
            uint address{ config.value("UsbPowerShareEc").toString().toUInt(nullptr, 16) };
            auto p{ new IOParameter<Msi::Enable>(ioBuffer, address, 
                QVariant::fromValue(Msi::Parametr::UsbPowerShareEc),
                QVariant::fromValue(QVector<Msi::Enable>{
                    Msi::Enable::Off,
                    Msi::Enable::On
                }), false) };
                p->setEnumHash({
                    { Msi::Enable::Off, 0x08 },
                    { Msi::Enable::On, 0x28 }
                });
            service.registerParameter(p);
        }
        if(config.contains("CoolerBoostEc")) {
            uint address{ config.value("CoolerBoostEc").toString().toUInt(nullptr, 16) };
            uint mask{ config.value("CoolerBoostMask").toString().toUInt(nullptr, 16) };
            auto p{ new IOParameter<Msi::Enable>(ioBuffer, address, QVariant::fromValue(Msi::Parametr::CoolerBoostEc), 
                QVariant::fromValue(QVector<Msi::Enable>{
                    Msi::Enable::Off,
                    Msi::Enable::On
                }), false, mask) };
            p->setEnumHash({
                { Msi::Enable::Off, 0x00 },
                { Msi::Enable::On, 0x80 }
            });
            service.registerParameter(p);
        }
        if(config.contains("WebCamEc")) {
            uint address{ config.value("WebCamEc").toString().toUInt(nullptr, 16) };
            uint mask{ config.value("WebCamMask").toString().toUInt(nullptr, 16) };
            auto p{ new IOParameter<Msi::Enable>(ioBuffer, address, QVariant::fromValue(Msi::Parametr::WebCamEc), 
                QVariant::fromValue(QVector<Msi::Enable>{
                    Msi::Enable::Off,
                    Msi::Enable::On
                }), false, mask) };
            p->setEnumHash({
                { Msi::Enable::Off, 0x00 },
                { Msi::Enable::On, mask }
            });
            service.registerParameter(p);
        }
        if(config.contains("WebCamBlockEc")) {
            uint address{ config.value("WebCamBlockEc").toString().toUInt(nullptr, 16) };
            uint mask{ config.value("WebCamMask").toString().toUInt(nullptr, 16) };
            auto p{ new IOParameter<Msi::Enable>(ioBuffer, address, QVariant::fromValue(Msi::Parametr::WebCamBlockEc), 
                QVariant::fromValue(QVector<Msi::Enable>{
                    Msi::Enable::Off,
                    Msi::Enable::On
                }), false, mask) };
            p->setEnumHash({
                { Msi::Enable::Off, 0x00 },
                { Msi::Enable::On, mask }
            });
            service.registerParameter(p);
        }
        if(config.contains("FnSuperSwapEc")) {
            uint address{ config.value("FnSuperSwapEc").toString().toUInt(nullptr, 16) };
            uint mask{ config.value("FnSuperSwapMask").toString().toUInt(nullptr, 16) };
            bool fnWinSwapInvert{ config.value("FnWinSwapInvert", false).toBool() };
            auto p{ new IOParameter<Msi::FnSuperSwap>(ioBuffer, address, QVariant::fromValue(Msi::Parametr::FnSuperSwapEc), 
                QVariant::fromValue(QVector<Msi::FnSuperSwap>{
                    Msi::FnSuperSwap::Right,
                    Msi::FnSuperSwap::Left
                }), false, mask)};
            if(fnWinSwapInvert) {
                p->setEnumHash({
                    { Msi::FnSuperSwap::Right, 0x10 },
                    { Msi::FnSuperSwap::Left, 0x00 }
                });
            } else {
                p->setEnumHash({
                    { Msi::FnSuperSwap::Right, 0x00 },
                    { Msi::FnSuperSwap::Left, 0x10 }
                });
            }
            service.registerParameter(p);
        }
        // if(quint8 val{ static_cast<quint8>(ioBuffer->buffer()[0xCD]) }; 
        //     val > 0) {
        //     service.registerParameter(new IOParameter<quint16>(ioBuffer, 0xCC, QVariant::fromValue(Msi::Parametr::FanCpuEc), QVariant(), true, 0xFFFF, QDataStream::BigEndian));
        // } else if(quint8 val{ static_cast<quint8>(ioBuffer->buffer()[0xC9]) }; 
        //     val > 0 && val < 50) {
        //     service.registerParameter(new IOParameter<quint16>(ioBuffer, 0xCC, QVariant::fromValue(Msi::Parametr::FanCpuEc), QVariant(), true, 0xFFFF, QDataStream::BigEndian));
        // } else {
        //     service.registerParameter(new IOParameter<quint16>(ioBuffer, 0xC8, QVariant::fromValue(Msi::Parametr::FanCpuEc), QVariant(), true, 0xFFFF, QDataStream::BigEndian));
        // }
        // service.registerParameter(new IOParameter<quint16>(ioBuffer, 0xCA, QVariant::fromValue(Msi::Parametr::FanGpuEc), QVariant(), true, 0xFFFF, QDataStream::BigEndian));

        if(config.contains("FanCpuEc")) {
            uint address{ config.value("FanCpuEc").toString().toUInt(nullptr, 16) };
            service.registerParameter(new IOParameter<quint8>(ioBuffer, address, QVariant::fromValue(Msi::Parametr::FanCpuEc), 
                QVariant::fromValue(Msi::Range{ 0, 100 }), true));
        }
        if(config.contains("FanModeEc") && config.contains("FanModeAvailable")) {
            QStringList modes{ config.value("FanModeAvailable").toStringList() };
            QHash<Msi::FanMode, quint8> fanModeMap;
            QVector<Msi::FanMode> fanModeVector;
            for(auto &mode : modes) {
                QStringList parts = mode.split(':');
                if(parts.size() == 2) {
                    Msi::FanMode fanMode = QVariant("Msi::FanMode::" + parts[0]).value<Msi::FanMode>();
                    uint value{ parts[1].toUInt(nullptr, 16) };
                    fanModeMap.insert(fanMode, value);
                    fanModeVector.append(fanMode);
                }
            }
            uint address{ config.value("FanModeEc").toString().toUInt(nullptr, 16) };
            auto p{ new IOParameter<Msi::FanMode>(ioBuffer, address, QVariant::fromValue(Msi::Parametr::FanModeEc), 
                QVariant::fromValue(fanModeVector), false) };
            p->setEnumHash(fanModeMap);
            service.registerParameter(p);
        }
        if(config.contains("FanGpuEc")) {
            uint address{ config.value("FanGpuEc").toString().toUInt(nullptr, 16) };
            service.registerParameter(new IOParameter<quint8>(ioBuffer, address, QVariant::fromValue(Msi::Parametr::FanGpuEc), 
                QVariant::fromValue(Msi::Range{ 0, 100 }), true));
        }

        for(int i{ 0 }; i < 7; ++i) {
            service.registerParameter(
                new IOParameter<quint8>(ioBuffer, 0x72 + i, 
                    QVariant::fromValue(static_cast<Msi::Parametr>(static_cast<int>(Msi::Parametr::FanSetSpeedCpu1Ec) + i)), 
                    QVariant::fromValue(Msi::Range{0, 150}), false));
            service.registerParameter(
                new IOParameter<quint8>(ioBuffer, 0x8A + i, 
                    QVariant::fromValue(static_cast<Msi::Parametr>(static_cast<int>(Msi::Parametr::FanSetSpeedGpu1Ec) + i)), 
                    QVariant::fromValue(Msi::Range{0, 150}), false));
        }

        for(int i{ 0 }; i < 6; ++i) {
            service.registerParameter(
                new IOParameter<quint8>(ioBuffer, 0x6A + i, 
                    QVariant::fromValue(static_cast<Msi::Parametr>(static_cast<int>(Msi::Parametr::FanSetTempCpu1Ec) + i)), 
                    QVariant::fromValue(Msi::Range{0, 100}), false));
            service.registerParameter(
                new IOParameter<quint8>(ioBuffer, 0x82 + i, 
                    QVariant::fromValue(static_cast<Msi::Parametr>(static_cast<int>(Msi::Parametr::FanSetTempGpu1Ec) + i)), 
                    QVariant::fromValue(Msi::Range{0, 100}), false));
        }

        if(config.contains("ShiftModeEc") && config.contains("ShiftModeAvailable")) {
            QStringList modes{ config.value("ShiftModeAvailable").toStringList() };
            QHash<Msi::ShiftMode, quint8> shiftModeMap;
            QVector<Msi::ShiftMode> shiftModeVector;
            for(auto &mode : modes) {
                QStringList parts = mode.split(':');
                if(parts.size() == 2) {
                    Msi::ShiftMode shiftMode = QVariant("Msi::ShiftMode::" + parts[0]).value<Msi::ShiftMode>();
                    uint value{ parts[1].toUInt(nullptr, 16) };
                    shiftModeMap.insert(shiftMode, value);
                    shiftModeVector.append(shiftMode);
                }
            }
            uint address{ config.value("ShiftModeEc").toString().toUInt(nullptr, 16) };
            auto p{ new IOParameter<Msi::ShiftMode>(ioBuffer, address, QVariant::fromValue(Msi::Parametr::ShiftModeEc), 
                QVariant::fromValue(shiftModeVector), false) };
            p->setEnumHash(shiftModeMap);
            service.registerParameter(p);
        }

        if(config.contains("SuperBatteryEc")) {
            uint address{ config.value("SuperBatteryEc").toString().toUInt(nullptr, 16) };
            uint mask{ config.value("SuperBatteryMask").toString().toUInt(nullptr, 16) };
            auto p{ new IOParameter<Msi::Enable>(ioBuffer, address, QVariant::fromValue(Msi::Parametr::SuperBatteryEc), 
                QVariant::fromValue(QVector<Msi::Enable>{
                    Msi::Enable::Off,
                    Msi::Enable::On
                }), false, mask) };
            p->setEnumHash({
                { Msi::Enable::Off, 0x00 },
                { Msi::Enable::On, mask }
            });
            service.registerParameter(p);
        }

        if(config.contains("MicMuteEc")) {
            uint address{ config.value("MicMuteEc").toString().toUInt(nullptr, 16) };
            uint mask{ config.value("LedsMask").toString().toUInt(nullptr, 16) };
            auto p{ new IOParameter<Msi::Enable>(ioBuffer, address, QVariant::fromValue(Msi::Parametr::MicMuteEc), 
                QVariant::fromValue(QVector<Msi::Enable>{
                    Msi::Enable::Off,
                    Msi::Enable::On
                }), false, mask) };
            p->setEnumHash({
                { Msi::Enable::Off, 0x00 },
                { Msi::Enable::On, mask }
            });
            service.registerParameter(p);
        }
        if(config.contains("MuteLedEc")) {
            uint address{ config.value("MuteLedEc").toString().toUInt(nullptr, 16) };
            uint mask{ config.value("LedsMask").toString().toUInt(nullptr, 16) };
            auto p{ new IOParameter<Msi::Enable>(ioBuffer, address, QVariant::fromValue(Msi::Parametr::MuteLedEc), 
                QVariant::fromValue(QVector<Msi::Enable>{
                    Msi::Enable::Off,
                    Msi::Enable::On
                }), false, mask) };
            p->setEnumHash({
                { Msi::Enable::Off, 0x00 },
                { Msi::Enable::On, mask }
            });
            service.registerParameter(p);
        }
    }
}