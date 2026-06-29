#include "telemetryhistorycontroller.h"

#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QSaveFile>
#include <QUrl>

#include "parameterclient.h"
#include "proxyparameter.h"

namespace {
constexpr int sampleIntervalMs = 5000;
constexpr int maxSamples = 360;

QVariantMap resultMap(bool ok, const QString& error = {}, const QVariantMap& extra = {}) {
    QVariantMap result = extra;
    result.insert(QStringLiteral("ok"), ok);
    result.insert(QStringLiteral("error"), error);
    return result;
}
} // namespace

TelemetryHistoryController::TelemetryHistoryController(ParameterClient* parameters, QObject* parent)
    : QObject(parent), mParameters(parameters) {
    mTimer.setInterval(sampleIntervalMs);
    connect(&mTimer, &QTimer::timeout, this, &TelemetryHistoryController::sample);
    mTimer.start();

    QTimer::singleShot(0, this, &TelemetryHistoryController::sample);
}

QVariantList TelemetryHistoryController::history() const {
    return mHistory;
}

QVariantMap TelemetryHistoryController::snapshot() const {
    QVariantMap result;
    result.insert(QStringLiteral("sampleIntervalMs"), sampleIntervalMs);
    result.insert(QStringLiteral("maxSamples"), maxSamples);
    result.insert(QStringLiteral("samples"), mHistory);
    return result;
}

QVariantMap TelemetryHistoryController::exportHistory(const QString& pathOrUrl) const {
    const QString path = normalizeFilePath(pathOrUrl);
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return resultMap(false, file.errorString());
    }

    file.write(QJsonDocument::fromVariant(snapshot()).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        return resultMap(false, file.errorString());
    }
    return resultMap(true, {}, {{QStringLiteral("path"), path}});
}

ProxyParameter* TelemetryHistoryController::parameter(Msi::Parametr name) const {
    return mParameters ? mParameters->getProxyParameter(name) : nullptr;
}

void TelemetryHistoryController::sample() {
    QVariantMap item;
    item.insert(QStringLiteral("time"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    item.insert(QStringLiteral("cpuTemp"), valueFor(Msi::Parametr::CpuTempEc));
    item.insert(QStringLiteral("gpuTemp"), valueFor(Msi::Parametr::GpuTempEc));
    item.insert(QStringLiteral("cpuFan"), valueFor(Msi::Parametr::FanCpuEc));
    item.insert(QStringLiteral("gpuFan"), valueFor(Msi::Parametr::FanGpuEc));
    item.insert(QStringLiteral("battery"), valueFor(Msi::Parametr::BatteryChargeEc));

    bool hasValue = false;
    for (auto it = item.cbegin(); it != item.cend(); ++it) {
        if (it.key() != QStringLiteral("time") && it.value().isValid()) {
            hasValue = true;
            break;
        }
    }
    if (!hasValue) {
        return;
    }

    mHistory.append(item);
    while (mHistory.size() > maxSamples) {
        mHistory.removeFirst();
    }
    emit historyChanged();
}

QVariant TelemetryHistoryController::valueFor(Msi::Parametr name) const {
    ProxyParameter* proxy = parameter(name);
    return proxy && proxy->isValid() ? proxy->value() : QVariant{};
}

QString TelemetryHistoryController::normalizeFilePath(const QString& pathOrUrl) const {
    const QUrl url(pathOrUrl);
    if (url.isValid() && url.isLocalFile()) {
        return url.toLocalFile();
    }
    return pathOrUrl;
}
