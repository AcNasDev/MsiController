#include "ecwritepolicy.h"

#include <QSet>
#include <QtGlobal>
#include <optional>

namespace {
constexpr int maxAuditEntries = 100;

QString hexByte(int value) {
    return QStringLiteral("0x%1").arg(value, 2, 16, QLatin1Char('0'));
}

QVariantList bytesForDbus(const QByteArray& bytes) {
    QVariantList result;
    result.reserve(bytes.size());
    for (char byte : bytes) {
        result.append(static_cast<int>(static_cast<quint8>(byte)));
    }
    return result;
}

std::optional<int> hexConfigValue(const QMap<QString, QVariant>& config, const QString& key) {
    const QString raw = config.value(key).toString().trimmed();
    if (raw.isEmpty()) {
        return std::nullopt;
    }

    bool ok = false;
    const int value = raw.toInt(&ok, 16);
    return ok ? std::optional<int>{value} : std::nullopt;
}

void appendSingleByteRange(QVector<EcWritePolicy::Range>* ranges,
                           const QMap<QString, QVariant>& config,
                           const QString& key) {
    if (const auto offset = hexConfigValue(config, key)) {
        ranges->append(EcWritePolicy::Range{*offset, 1, key});
    }
}

void appendRange(QVector<EcWritePolicy::Range>* ranges, int offset, int length, const QString& label) {
    ranges->append(EcWritePolicy::Range{offset, length, label});
}
} // namespace

EcWritePolicy::EcWritePolicy(QObject* parent) : QObject(parent) {}

void EcWritePolicy::setAllowedRanges(const QVector<Range>& ranges) {
    mAllowedRanges = ranges;
}

QVector<EcWritePolicy::Range> EcWritePolicy::allowedRanges() const {
    return mAllowedRanges;
}

QVariantList EcWritePolicy::allowedRangesForDbus() const {
    QVariantList result;
    result.reserve(mAllowedRanges.size());
    for (const Range& range : mAllowedRanges) {
        QVariantMap item;
        item.insert(QStringLiteral("offset"), range.offset);
        item.insert(QStringLiteral("length"), range.length);
        item.insert(QStringLiteral("end"), range.offset + qMax(0, range.length - 1));
        item.insert(QStringLiteral("label"), range.label);
        item.insert(QStringLiteral("display"),
                    QStringLiteral("%1..%2 %3")
                        .arg(hexByte(range.offset), hexByte(range.offset + qMax(0, range.length - 1)), range.label));
        result.append(item);
    }
    return result;
}

QVariantList EcWritePolicy::auditLogForDbus() const {
    return mAuditLog;
}

QVariantMap EcWritePolicy::diagnostics() const {
    QVariantMap result;
    result.insert(QStringLiteral("unsafeOverride"), unsafeOverrideEnabled());
    result.insert(QStringLiteral("allowedRangeCount"), mAllowedRanges.size());
    result.insert(QStringLiteral("allowedRanges"), allowedRangesForDbus());
    result.insert(QStringLiteral("auditLog"), mAuditLog);
    return result;
}

bool EcWritePolicy::allows(int offset, int length, QString* errorMessage) const {
    if (unsafeOverrideEnabled()) {
        return true;
    }
    if (length <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Write length must be greater than 0");
        }
        return false;
    }

    for (const Range& range : mAllowedRanges) {
        if (offset >= range.offset && offset + length <= range.offset + range.length) {
            return true;
        }
    }

    if (errorMessage) {
        *errorMessage = QStringLiteral("Raw EC write to %1..%2 is outside the active safety allowlist")
                            .arg(hexByte(offset), hexByte(offset + length - 1));
    }
    return false;
}

void EcWritePolicy::recordWrite(const QString& operation,
                                int offset,
                                const QByteArray& bytes,
                                bool ok,
                                const QString& errorMessage) {
    QVariantMap item;
    item.insert(QStringLiteral("time"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    item.insert(QStringLiteral("operation"), operation);
    item.insert(QStringLiteral("offset"), offset);
    item.insert(QStringLiteral("length"), bytes.size());
    item.insert(QStringLiteral("bytes"), bytesForDbus(bytes));
    item.insert(QStringLiteral("ok"), ok);
    item.insert(QStringLiteral("error"), errorMessage);
    mAuditLog.append(item);
    while (mAuditLog.size() > maxAuditEntries) {
        mAuditLog.removeFirst();
    }
}

QVector<EcWritePolicy::Range> EcWritePolicy::rangesFromProfileConfig(const QMap<QString, QVariant>& config) {
    QVector<Range> ranges;
    for (const QString& key : {QStringLiteral("BatteryThresholdEc"),
                               QStringLiteral("KeyboardBacklightModeEc"),
                               QStringLiteral("KeyboardBacklightEc"),
                               QStringLiteral("UsbPowerShareEc"),
                               QStringLiteral("CoolerBoostEc"),
                               QStringLiteral("WebCamEc"),
                               QStringLiteral("WebCamBlockEc"),
                               QStringLiteral("FnSuperSwapEc"),
                               QStringLiteral("FanModeEc"),
                               QStringLiteral("ShiftModeEc"),
                               QStringLiteral("SuperBatteryEc"),
                               QStringLiteral("MicMuteEc"),
                               QStringLiteral("MuteLedEc")}) {
        appendSingleByteRange(&ranges, config, key);
    }

    appendRange(&ranges, 0x6a, 6, QStringLiteral("CPU fan curve temperatures"));
    appendRange(&ranges, 0x72, 7, QStringLiteral("CPU fan curve speeds"));
    appendRange(&ranges, 0x82, 6, QStringLiteral("GPU fan curve temperatures"));
    appendRange(&ranges, 0x8a, 7, QStringLiteral("GPU fan curve speeds"));

    QSet<QString> seen;
    QVector<Range> unique;
    unique.reserve(ranges.size());
    for (const Range& range : std::as_const(ranges)) {
        const QString key = QStringLiteral("%1:%2:%3").arg(range.offset).arg(range.length).arg(range.label);
        if (seen.contains(key)) {
            continue;
        }
        seen.insert(key);
        unique.append(range);
    }
    return unique;
}

bool EcWritePolicy::unsafeOverrideEnabled() const {
    const QString value = qEnvironmentVariable("MSICONTROLLER_ALLOW_UNSAFE_EC_WRITES").trimmed().toLower();
    return value == QStringLiteral("1") || value == QStringLiteral("true") || value == QStringLiteral("yes");
}
