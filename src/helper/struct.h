#pragma once

#include <QDBusArgument>
#include <QDBusMetaType>
#include <QMetaEnum>
#include <QObject>

namespace Msi {
Q_NAMESPACE

enum class Parametr : quint8 {
    CpuTempEc = 0,
    GpuTempEc,
    BatteryChargeEc,
    BatteryThresholdEc,
    BatteryChargingStatusEc,
    KeyboardBacklightModeEc,
    KeyboardBacklightEc,
    UsbPowerShareEc,
    CoolerBoostEc,
    WebCamEc,
    FnSuperSwapEc,
    FanCpuEc,
    FanGpuEc,
    FanSetSpeedCpu1Ec,
    FanSetSpeedCpu2Ec,
    FanSetSpeedCpu3Ec,
    FanSetSpeedCpu4Ec,
    FanSetSpeedCpu5Ec,
    FanSetSpeedCpu6Ec,
    FanSetSpeedCpu7Ec,
    FanSetTempCpu1Ec,
    FanSetTempCpu2Ec,
    FanSetTempCpu3Ec,
    FanSetTempCpu4Ec,
    FanSetTempCpu5Ec,
    FanSetTempCpu6Ec,
    FanSetSpeedGpu1Ec,
    FanSetSpeedGpu2Ec,
    FanSetSpeedGpu3Ec,
    FanSetSpeedGpu4Ec,
    FanSetSpeedGpu5Ec,
    FanSetSpeedGpu6Ec,
    FanSetSpeedGpu7Ec,
    FanSetTempGpu1Ec,
    FanSetTempGpu2Ec,
    FanSetTempGpu3Ec,
    FanSetTempGpu4Ec,
    FanSetTempGpu5Ec,
    FanSetTempGpu6Ec,
    ShiftModeEc,
    FanModeEc,
    FirmwareVersionEc,
    FirmwareReleaseDateEc,
    WebCamBlockEc,
    SuperBatteryEc,
    MicMuteEc,
    MuteLedEc,
    FirmwareReleaseTimeEc,
    CpuConfig,
    CountEs
};
Q_ENUM_NS(Parametr)

struct Range {
    Q_GADGET
    Q_PROPERTY(int min MEMBER min)
    Q_PROPERTY(int max MEMBER max)
public:
    int min;
    int max;
    friend QDataStream& operator<<(QDataStream& out, const Msi::Range& range) {
        out << range.min << range.max;
        return out;
    }

    friend QDataStream& operator>>(QDataStream& in, Msi::Range& range) {
        in >> range.min >> range.max;
        return in;
    }
};

enum class ChargingStatus : quint8 {
    BatteryCharging = 0,
    BatteryDischarging,
    BatteryNotCharging,
    BatteryFullyCharged,
    BatteryFullyChargedNoPower
};
Q_ENUM_NS(ChargingStatus)

enum class KeyboardBacklight : quint8 { Off = 0, Low, Mid, High };
Q_ENUM_NS(KeyboardBacklight)

enum class FnSuperSwap : quint8 { Right = 0, Left };
Q_ENUM_NS(FnSuperSwap)

enum class ShiftMode : quint8 { Turbo = 0, Comfort, Eco, Sport };
Q_ENUM_NS(ShiftMode)

enum class FanMode : quint8 { Auto = 0, Silent, Basic, Advanced };
Q_ENUM_NS(FanMode)

enum class Enable : quint8 { Off = 0, On };
Q_ENUM_NS(Enable)

struct CpuControl {
    Q_GADGET
    Q_PROPERTY(quint32 minFreq MEMBER minFreq)
    Q_PROPERTY(quint32 maxFreq MEMBER maxFreq)
    Q_PROPERTY(quint32 scalingMaxFreq MEMBER scalingMaxFreq)
    Q_PROPERTY(quint32 scalingMinFreq MEMBER scalingMinFreq)
    Q_PROPERTY(quint32 currentFreq MEMBER currentFreq)
    Q_PROPERTY(QString availableGovernor MEMBER availableGovernor)
public:
    quint32 minFreq{0};
    quint32 maxFreq{0};
    quint32 scalingMaxFreq{0};
    quint32 scalingMinFreq{0};
    quint32 currentFreq{0};
    QString availableGovernor;
    friend QDataStream& operator<<(QDataStream& out, const Msi::CpuControl& cpu) {
        out << cpu.minFreq << cpu.maxFreq << cpu.currentFreq << cpu.availableGovernor << cpu.scalingMaxFreq
            << cpu.scalingMinFreq;
        return out;
    }
    friend QDataStream& operator>>(QDataStream& in, Msi::CpuControl& cpu) {
        in >> cpu.minFreq >> cpu.maxFreq >> cpu.currentFreq >> cpu.availableGovernor >> cpu.scalingMaxFreq >>
            cpu.scalingMinFreq;
        return in;
    }
    bool operator==(const CpuControl& other) const {
        return minFreq == other.minFreq && maxFreq == other.maxFreq && currentFreq == other.currentFreq &&
               availableGovernor == other.availableGovernor && scalingMaxFreq == other.scalingMaxFreq &&
               scalingMinFreq == other.scalingMinFreq;
    }
};
struct Cpu : public CpuControl {
    Q_GADGET
    Q_PROPERTY(double usage MEMBER usage)
    Q_PROPERTY(QStringList availableGovernors MEMBER availableGovernors)
public:
    double usage{0.0};
    QStringList availableGovernors;
    bool operator==(const Cpu& other) const {
        return static_cast<const CpuControl&>(*this) == static_cast<const CpuControl&>(other) && usage == other.usage &&
               availableGovernors == other.availableGovernors;
    }
    friend QDataStream& operator<<(QDataStream& out, const Msi::Cpu& cpu) {
        out << static_cast<const Msi::CpuControl&>(cpu) << cpu.usage << cpu.availableGovernors;
        return out;
    }
    friend QDataStream& operator>>(QDataStream& in, Msi::Cpu& cpu) {
        in >> static_cast<Msi::CpuControl&>(cpu) >> cpu.usage >> cpu.availableGovernors;
        return in;
    }
};

struct CpuConfig {
    Q_GADGET
    Q_PROPERTY(QList<Cpu> cpus MEMBER cpus)
public:
    QList<Cpu> cpus;
    bool operator==(const CpuConfig& other) const { return cpus == other.cpus; }
    friend QDataStream& operator<<(QDataStream& out, const Msi::CpuConfig& config) {
        out << config.cpus;
        return out;
    }
    friend QDataStream& operator>>(QDataStream& in, Msi::CpuConfig& config) {
        in >> config.cpus;
        return in;
    }
};

struct Msg {
    Q_GADGET
    Q_PROPERTY(QVariant variant MEMBER variant)
public:
    Msg() = default;
    template <typename T>
    Msg(const T& v) : variant{QVariant::fromValue(v)} {}
    QVariant variant;
    friend QDataStream& operator<<(QDataStream& out, const Msi::Msg& msg) {
        out << msg.variant;
        return out;
    }

    friend QDataStream& operator>>(QDataStream& in, Msi::Msg& msg) {
        in >> msg.variant;
        return in;
    }

    friend QDBusArgument& operator<<(QDBusArgument& arg, const Msi::Msg& msg) {
        arg.beginStructure();
        QByteArray data;
        QDataStream stream(&data, QIODeviceBase::WriteOnly);
        stream << msg.variant;
        arg << data;
        arg.endStructure();
        return arg;
    }

    friend const QDBusArgument& operator>>(const QDBusArgument& arg, Msi::Msg& msg) {
        arg.beginStructure();
        QByteArray data;
        arg >> data;
        QDataStream stream(data);
        stream >> msg.variant;
        arg.endStructure();
        return arg;
    }
};

} // namespace Msi

Q_DECLARE_METATYPE(Msi::Parametr)
Q_DECLARE_METATYPE(Msi::Range)
Q_DECLARE_METATYPE(Msi::ChargingStatus)
Q_DECLARE_METATYPE(Msi::KeyboardBacklight)
Q_DECLARE_METATYPE(Msi::FnSuperSwap)
Q_DECLARE_METATYPE(Msi::ShiftMode)
Q_DECLARE_METATYPE(Msi::FanMode)
Q_DECLARE_METATYPE(Msi::Enable)
Q_DECLARE_METATYPE(Msi::Msg)
Q_DECLARE_METATYPE(Msi::Cpu)
Q_DECLARE_METATYPE(Msi::CpuConfig)

inline void registerMetaType() {
    qRegisterMetaType<Msi::Parametr>("Msi::Parametr");
    qRegisterMetaType<Msi::Range>("Msi::Range");
    qRegisterMetaType<Msi::ChargingStatus>("Msi::ChargingStatus");
    qRegisterMetaType<Msi::KeyboardBacklight>("Msi::KeyboardBacklight");
    qRegisterMetaType<Msi::FnSuperSwap>("Msi::FnSuperSwap");
    qRegisterMetaType<Msi::ShiftMode>("Msi::ShiftMode");
    qRegisterMetaType<Msi::FanMode>("Msi::FanMode");
    qRegisterMetaType<Msi::Enable>("Msi::Enable");
    qRegisterMetaType<QList<quint8>>("QList<quint8>");
    qRegisterMetaType<QList<Msi::ChargingStatus>>("QList<Msi::ChargingStatus>");
    qRegisterMetaType<QList<Msi::Enable>>("QList<Msi::Enable>");
    qRegisterMetaType<QList<Msi::FnSuperSwap>>("QList<Msi::FnSuperSwap>");
    qRegisterMetaType<QList<Msi::FanMode>>("QList<Msi::FanMode>");
    qRegisterMetaType<QList<Msi::ShiftMode>>("QList<Msi::ShiftMode>");
    qRegisterMetaType<QList<Msi::Msg>>("QList<Msi::Msg>");
    qRegisterMetaType<Msi::Cpu>("Msi::Cpu");
    qRegisterMetaType<Msi::CpuConfig>("Msi::CpuConfig");

    qDBusRegisterMetaType<Msi::Msg>();
}

class EnumHelper : public QObject {
    Q_OBJECT
public:
    explicit EnumHelper(QObject* parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE QString enumToString(int value, const QString& typeName) const {
        // Получаем QMetaObject пространства имён Msi
        const QMetaObject* metaObj = &Msi::staticMetaObject;
        int enumIndex = metaObj->indexOfEnumerator(typeName.toUtf8().constData());
        if (enumIndex != -1) {
            QMetaEnum metaEnum = metaObj->enumerator(enumIndex);
            const char* key = metaEnum.valueToKey(value);
            return key ? QString::fromUtf8(key) : "Unknown";
        }
        return "Unknown";
    }

private:
    // Шаблонная функция для преобразования enum в строку
    template <typename T>
    QString enumToString(int value) const {
        QMetaEnum metaEnum = QMetaEnum::fromType<T>();
        const char* key = metaEnum.valueToKey(value);
        return key ? QString::fromUtf8(key) : "Unknown";
    }
};