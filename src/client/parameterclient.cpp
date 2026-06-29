#include "parameterclient.h"

#include <QDBusPendingCallWatcher>
#include <QDateTime>
#include <QSet>
#include <algorithm>

#include "dbusapi.h"
#include "dbuscodec.h"
#include "ecparametersinterface.h"
#include "logging.h"

namespace {
constexpr int writeConfirmRetryMs = 140;
constexpr int writeConfirmTimeoutMs = 2600;
constexpr quint32 cpuFrequencyConfirmToleranceKhz = 25000;
constexpr double gpuPowerConfirmToleranceWatts = 0.75;

qint64 currentTimeMs() {
    return QDateTime::currentMSecsSinceEpoch();
}

bool frequencyClose(quint32 expected, quint32 actual) {
    return qAbs(static_cast<qint64>(expected) - static_cast<qint64>(actual)) <= cpuFrequencyConfirmToleranceKhz;
}

bool powerLimitClose(double expected, double actual) {
    return qAbs(expected - actual) <= gpuPowerConfirmToleranceWatts;
}

QVariantMap gpuDevicesById(const QVariant& value) {
    QVariantMap devicesById;
    for (const QVariant& item : value.toList()) {
        const QVariantMap device = item.toMap();
        const QString id = device.value(QStringLiteral("id")).toString();
        if (!id.isEmpty()) {
            devicesById.insert(id, device);
        }
    }
    return devicesById;
}

bool gpuControlMatches(const QVariant& expectedValue, const QVariant& actualValue) {
    const QVariantMap actualById = gpuDevicesById(actualValue);
    for (const QVariant& item : expectedValue.toList()) {
        const QVariantMap expected = item.toMap();
        const QString id = expected.value(QStringLiteral("id")).toString();
        if (id.isEmpty()) {
            continue;
        }

        const QVariantMap actual = actualById.value(id).toMap();
        if (actual.isEmpty()) {
            return false;
        }

        if (expected.contains(QStringLiteral("performanceLevel")) &&
            actual.contains(QStringLiteral("performanceLevel")) &&
            expected.value(QStringLiteral("performanceLevel")).toString() !=
                actual.value(QStringLiteral("performanceLevel")).toString()) {
            return false;
        }
        if (expected.contains(QStringLiteral("persistenceMode")) &&
            actual.contains(QStringLiteral("persistenceMode")) &&
            expected.value(QStringLiteral("persistenceMode")).toBool() !=
                actual.value(QStringLiteral("persistenceMode")).toBool()) {
            return false;
        }
        if (expected.contains(QStringLiteral("powerLimitWatts")) &&
            actual.contains(QStringLiteral("powerLimitWatts")) &&
            !powerLimitClose(expected.value(QStringLiteral("powerLimitWatts")).toDouble(),
                             actual.value(QStringLiteral("powerLimitWatts")).toDouble())) {
            return false;
        }
    }
    return true;
}
} // namespace

ParameterClient::ParameterClient(const QDBusConnection& connection, QObject* parent) : QObject(parent) {
    mParametersInterface = new ComMsiEcParametersInterface(QString::fromLatin1(MsiDbusApi::serviceName),
                                                           QString::fromLatin1(MsiDbusApi::parametersPath),
                                                           connection,
                                                           this);
    mWriteFlushTimer.setSingleShot(true);
    mWriteFlushTimer.setInterval(40);
    connect(&mWriteFlushTimer, &QTimer::timeout, this, &ParameterClient::flushPendingWrites);

    connect(mParametersInterface,
            &ComMsiEcParametersInterface::parameterChanged,
            this,
            [this](const QDBusVariant& name, const QDBusVariant& value) {
                Msi::Parametr paramName = MsiDbusCodec::unwrap(name).value<Msi::Parametr>();
                QVariant paramValue = MsiDbusCodec::unwrap(value);
                if (hasQueuedWrite(paramName)) {
                    return;
                }
                handleRemoteValue(paramName, paramValue);
            });
    connect(mParametersInterface, &ComMsiEcParametersInterface::availableParametersChanged, this, [this]() {
        clearRuntimeState(true);
        init();
    });

    for (int i = 0; i < static_cast<int>(Msi::Parametr::CountEs); ++i) {
        Msi::Parametr param = static_cast<Msi::Parametr>(i);
        mProxyParameters[param] = new ProxyParameter(this);
        mProxyParameters[param]->setParameter(param);
        mProxyParameters[param]->setIsValid(false);

        connect(mProxyParameters[param], &ProxyParameter::valueEdited, this, [this, param]() {
            queueWrite(param, mProxyParameters[param]->value());
        });
    }
}

void ParameterClient::setConnected(bool connected) {
    if (mIsConnected == connected) {
        return;
    }

    mIsConnected = connected;
    if (!mIsConnected) {
        clearRuntimeState(true);
        return;
    }
    init();
}

ProxyParameter* ParameterClient::getProxyParameter(const Msi::Parametr& name) const {
    return mProxyParameters.value(name, nullptr);
}

void ParameterClient::init() {
    if (!mIsConnected) {
        return;
    }

    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(mParametersInterface->availableParameters(), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher]() {
        if (watcher->isError()) {
            qCWarning(msiClientLog) << "Failed to fetch available parameters:" << watcher->error();
            watcher->deleteLater();
            return;
        }

        const auto availableArguments = watcher->reply().arguments();
        if (availableArguments.isEmpty()) {
            qCWarning(msiClientLog) << "Empty available parameters reply";
            watcher->deleteLater();
            return;
        }

        auto args = MsiDbusCodec::unwrapReplyArgument(availableArguments.at(0)).value<QVariantList>();
        QVector<Msi::Parametr> params;
        for (auto& a : args) {
            if (auto it{mProxyParameters.find(a.value<Msi::Parametr>())}; it != mProxyParameters.end()) {
                params.append(a.value<Msi::Parametr>());
            }
        }
        for (auto it = mProxyParameters.begin(); it != mProxyParameters.end(); ++it) {
            if (!params.contains(it.key())) {
                it.value()->setIsValid(false);
                it.value()->setIsPending(false);
            }
        }
        watcher->deleteLater();

        for (auto& name : params) {
            QDBusPendingCall valueCall = mParametersInterface->availableValues(MsiDbusCodec::wrapValue(name));
            QDBusPendingCallWatcher* valueWatcher = new QDBusPendingCallWatcher(valueCall, this);
            connect(valueWatcher, &QDBusPendingCallWatcher::finished, this, [this, name, valueWatcher]() {
                if (valueWatcher->isError()) {
                    qCWarning(msiClientLog) << "Failed to fetch values for" << name << ":" << valueWatcher->error();
                    valueWatcher->deleteLater();
                    return;
                }
                const auto arguments = valueWatcher->reply().arguments();
                if (arguments.isEmpty()) {
                    qCWarning(msiClientLog) << "Empty available values reply for" << name;
                    valueWatcher->deleteLater();
                    return;
                }
                auto reply = MsiDbusCodec::unwrapReplyArgument(arguments.at(0));
                if (auto it{mProxyParameters.find(name)}; it != mProxyParameters.end()) {
                    it.value()->setAvailableValues(reply);
                }
                valueWatcher->deleteLater();
            });

            QDBusPendingCall readCall = mParametersInterface->readParameter(MsiDbusCodec::wrapValue(name));
            QDBusPendingCallWatcher* readWatcher = new QDBusPendingCallWatcher(readCall, this);
            connect(readWatcher, &QDBusPendingCallWatcher::finished, this, [this, name, readWatcher]() {
                if (readWatcher->isError()) {
                    qCWarning(msiClientLog) << "Failed to read" << name << ":" << readWatcher->error();
                    readWatcher->deleteLater();
                    return;
                }
                const auto arguments = readWatcher->reply().arguments();
                if (arguments.isEmpty()) {
                    qCWarning(msiClientLog) << "Empty read reply for" << name;
                    readWatcher->deleteLater();
                    return;
                }
                if (!hasQueuedWrite(name)) {
                    auto value = MsiDbusCodec::unwrapReplyArgument(arguments.at(0));
                    handleRemoteValue(name, value, true);
                }
                readWatcher->deleteLater();
            });
        }
    });
}

void ParameterClient::setCpuScalingMaxFrequencies(const QVariantList& frequenciesKhz) {
    ProxyParameter* param = cpuControlWriteParameter();
    if (!param || frequenciesKhz.isEmpty()) {
        return;
    }

    Msi::CpuConfig config = param->value().value<Msi::CpuConfig>();
    if (config.cpus.isEmpty()) {
        return;
    }

    const int count = std::min<int>(config.cpus.size(), frequenciesKhz.size());
    for (int i = 0; i < count; ++i) {
        bool ok = false;
        const quint32 requested = frequenciesKhz.at(i).toUInt(&ok);
        if (!ok || requested == 0) {
            continue;
        }

        Msi::Cpu& cpu = config.cpus[i];
        const quint32 minFreq = cpu.minFreq;
        const quint32 maxFreq = std::max(cpu.maxFreq, minFreq);
        cpu.scalingMaxFreq = std::min(std::max(requested, minFreq), maxFreq);
    }

    param->setValue(QVariant::fromValue(config));
}

void ParameterClient::setCpuGovernor(const QString& governor) {
    if (governor.isEmpty()) {
        return;
    }

    ProxyParameter* param = cpuControlWriteParameter();
    if (!param) {
        return;
    }

    Msi::CpuConfig config = param->value().value<Msi::CpuConfig>();
    if (config.cpus.isEmpty()) {
        return;
    }

    for (Msi::Cpu& cpu : config.cpus) {
        cpu.availableGovernor = governor;
    }

    param->setValue(QVariant::fromValue(config));
}

void ParameterClient::setGpuControlValue(const QString& deviceId, const QString& key, const QVariant& value) {
    if (deviceId.isEmpty() || key.isEmpty()) {
        return;
    }

    auto it = mProxyParameters.find(Msi::Parametr::GpuControlConfig);
    if (it == mProxyParameters.end() || !it.value()->isValid()) {
        return;
    }

    const QVariantList devices = it.value()->value().toList();
    QVariantList updatedDevices;
    updatedDevices.reserve(devices.size());
    bool changed = false;

    for (const QVariant& item : devices) {
        QVariantMap device = item.toMap();
        if (device.value(QStringLiteral("id")).toString() == deviceId && device.value(key) != value) {
            device.insert(key, value);
            changed = true;
        }
        updatedDevices.append(device);
    }

    if (changed) {
        it.value()->setValue(updatedDevices);
    }
}

ProxyParameter* ParameterClient::cpuControlWriteParameter() const {
    if (auto it = mProxyParameters.find(Msi::Parametr::CpuControlConfig); it != mProxyParameters.end()) {
        const Msi::CpuConfig config = it.value()->value().value<Msi::CpuConfig>();
        if (it.value()->isValid() && !config.cpus.isEmpty()) {
            return it.value();
        }
    }

    if (auto it = mProxyParameters.find(Msi::Parametr::CpuConfig); it != mProxyParameters.end()) {
        const Msi::CpuConfig config = it.value()->value().value<Msi::CpuConfig>();
        if (it.value()->isValid() && !config.cpus.isEmpty()) {
            return it.value();
        }
    }

    return nullptr;
}

void ParameterClient::applyRemoteValue(Msi::Parametr param, const QVariant& value, bool markValid) {
    if (auto it = mProxyParameters.find(param); it != mProxyParameters.end()) {
        it.value()->setSuppressWriteSignal(true);
        it.value()->setValue(value);
        it.value()->setSuppressWriteSignal(false);
        if (markValid) {
            it.value()->setIsValid(true);
        }
    }
}

void ParameterClient::handleRemoteValue(Msi::Parametr param, const QVariant& value, bool markValid) {
    if (hasQueuedWrite(param)) {
        return;
    }

    if (auto confirmation = mConfirmingWrites.find(param); confirmation != mConfirmingWrites.end()) {
        const bool confirmed = confirmationMatches(param, confirmation.value().expectedValue, value);
        const bool expired = confirmationExpired(confirmation.value());
        if (!confirmed && !expired) {
            if (markValid) {
                if (auto proxyParam = mProxyParameters.find(param); proxyParam != mProxyParameters.end()) {
                    proxyParam.value()->setIsValid(true);
                }
            }
            scheduleConfirmationRefresh(param);
            return;
        }

        mConfirmingWrites.erase(confirmation);
        applyRemoteValue(param, value, markValid);
        if (auto proxyParam = mProxyParameters.find(param); proxyParam != mProxyParameters.end()) {
            proxyParam.value()->setIsPending(false);
        }
        return;
    }

    applyRemoteValue(param, value, markValid);
}

void ParameterClient::queueWrite(Msi::Parametr param, const QVariant& value) {
    if (!mIsConnected) {
        refreshParameter(param);
        return;
    }

    mConfirmingWrites.remove(param);
    mPendingWrites[param] = value;
    if (auto it = mProxyParameters.find(param); it != mProxyParameters.end()) {
        it.value()->setIsPending(true);
    }
    if (!mWriteFlushTimer.isActive()) {
        mWriteFlushTimer.start();
    }
}

void ParameterClient::flushPendingWrites() {
    if (!mIsConnected) {
        return;
    }

    QVariantList updates;
    QVector<Msi::Parametr> batchParams;
    const auto pendingParams = mPendingWrites.keys();
    for (const auto& param : pendingParams) {
        if (mInFlightWrites.contains(param)) {
            continue;
        }

        const QVariant value = mPendingWrites.take(param);
        mInFlightWrites[param] = value;
        batchParams.append(param);
        updates << QVariant::fromValue(param) << value;
    }

    if (batchParams.isEmpty()) {
        return;
    }

    auto* watcher =
        new QDBusPendingCallWatcher(mParametersInterface->writeParameters(MsiDbusCodec::wrap(updates)), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, batchParams, watcher]() {
        QSet<Msi::Parametr> confirmedParams;
        if (watcher->isError()) {
            qCWarning(msiClientLog) << "Failed to write parameter batch:" << watcher->error();
        } else {
            const auto arguments = watcher->reply().arguments();
            if (arguments.isEmpty()) {
                qCWarning(msiClientLog) << "Empty batch write reply";
            } else {
                const auto values = MsiDbusCodec::unwrapReplyArgument(arguments.at(0)).toList();
                if (values.size() % 2 != 0) {
                    qCWarning(msiClientLog) << "Invalid batch write reply size:" << values.size();
                }
                for (qsizetype i = 0; i + 1 < values.size(); i += 2) {
                    const auto param = values.at(i).value<Msi::Parametr>();
                    const QVariant value = values.at(i + 1);
                    confirmedParams.insert(param);
                    const QVariant expectedValue = mInFlightWrites.take(param);
                    if (!hasQueuedWrite(param)) {
                        beginConfirmation(param, expectedValue);
                        handleRemoteValue(param, value, true);
                    }
                }
            }
        }

        for (const auto& param : batchParams) {
            if (confirmedParams.contains(param)) {
                continue;
            }
            const QVariant expectedValue = mInFlightWrites.take(param);
            if (!hasQueuedWrite(param)) {
                beginConfirmation(param, expectedValue);
                refreshParameter(param);
            }
        }

        if (!mPendingWrites.isEmpty()) {
            mWriteFlushTimer.start();
        }
        watcher->deleteLater();
    });
}

void ParameterClient::refreshParameter(Msi::Parametr param) {
    if (!mIsConnected || !mParametersInterface) {
        return;
    }

    auto* watcher =
        new QDBusPendingCallWatcher(mParametersInterface->readParameter(MsiDbusCodec::wrapValue(param)), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, param, watcher]() {
        if (watcher->isError()) {
            qCWarning(msiClientLog) << "Failed to refresh" << param << ":" << watcher->error();
            if (!hasQueuedWrite(param)) {
                if (auto confirmation = mConfirmingWrites.find(param); confirmation != mConfirmingWrites.end()) {
                    if (confirmationExpired(confirmation.value())) {
                        mConfirmingWrites.erase(confirmation);
                    } else {
                        scheduleConfirmationRefresh(param);
                    }
                }
            }
            if (auto it = mProxyParameters.find(param); it != mProxyParameters.end()) {
                it.value()->setIsPending(hasWriteInProgress(param));
            }
            watcher->deleteLater();
            return;
        }

        if (!hasQueuedWrite(param)) {
            const auto arguments = watcher->reply().arguments();
            if (arguments.isEmpty()) {
                qCWarning(msiClientLog) << "Empty refresh reply for" << param;
                if (auto confirmation = mConfirmingWrites.find(param); confirmation != mConfirmingWrites.end()) {
                    if (confirmationExpired(confirmation.value())) {
                        mConfirmingWrites.erase(confirmation);
                    } else {
                        scheduleConfirmationRefresh(param);
                    }
                }
                if (auto it = mProxyParameters.find(param); it != mProxyParameters.end()) {
                    it.value()->setIsPending(hasWriteInProgress(param));
                }
                watcher->deleteLater();
                return;
            }
            auto value = MsiDbusCodec::unwrapReplyArgument(arguments.at(0));
            handleRemoteValue(param, value, true);
        }
        watcher->deleteLater();
    });
}

void ParameterClient::beginConfirmation(Msi::Parametr param, const QVariant& expectedValue) {
    PendingConfirmation confirmation;
    confirmation.expectedValue = expectedValue;
    confirmation.startedAtMs = currentTimeMs();
    mConfirmingWrites[param] = confirmation;
    if (auto it = mProxyParameters.find(param); it != mProxyParameters.end()) {
        it.value()->setIsPending(true);
    }
}

void ParameterClient::scheduleConfirmationRefresh(Msi::Parametr param) {
    if (!mIsConnected) {
        return;
    }

    auto confirmation = mConfirmingWrites.find(param);
    if (confirmation == mConfirmingWrites.end() || confirmation.value().refreshScheduled) {
        return;
    }

    confirmation.value().refreshScheduled = true;
    ++confirmation.value().retryCount;
    QTimer::singleShot(writeConfirmRetryMs, this, [this, param]() {
        auto confirmation = mConfirmingWrites.find(param);
        if (confirmation == mConfirmingWrites.end() || hasQueuedWrite(param)) {
            return;
        }

        confirmation.value().refreshScheduled = false;
        refreshParameter(param);
    });
}

bool ParameterClient::confirmationMatches(Msi::Parametr param,
                                          const QVariant& expectedValue,
                                          const QVariant& actualValue) const {
    if (param == Msi::Parametr::GpuControlConfig) {
        return gpuControlMatches(expectedValue, actualValue);
    }

    if (param != Msi::Parametr::CpuConfig && param != Msi::Parametr::CpuControlConfig) {
        return expectedValue == actualValue;
    }

    const Msi::CpuConfig expectedConfig = expectedValue.value<Msi::CpuConfig>();
    const Msi::CpuConfig actualConfig = actualValue.value<Msi::CpuConfig>();
    if (expectedConfig.cpus.size() != actualConfig.cpus.size()) {
        return false;
    }

    for (int i = 0; i < expectedConfig.cpus.size(); ++i) {
        const Msi::Cpu& expectedCpu = expectedConfig.cpus.at(i);
        const Msi::Cpu& actualCpu = actualConfig.cpus.at(i);
        if (!frequencyClose(expectedCpu.scalingMinFreq, actualCpu.scalingMinFreq) ||
            !frequencyClose(expectedCpu.scalingMaxFreq, actualCpu.scalingMaxFreq)) {
            return false;
        }
        if (!expectedCpu.availableGovernor.isEmpty() && expectedCpu.availableGovernor != actualCpu.availableGovernor) {
            return false;
        }
    }
    return true;
}

bool ParameterClient::confirmationExpired(const PendingConfirmation& confirmation) const {
    return currentTimeMs() - confirmation.startedAtMs >= writeConfirmTimeoutMs;
}

bool ParameterClient::hasQueuedWrite(Msi::Parametr param) const {
    return mPendingWrites.contains(param) || mInFlightWrites.contains(param);
}

bool ParameterClient::hasWriteInProgress(Msi::Parametr param) const {
    return hasQueuedWrite(param) || mConfirmingWrites.contains(param);
}

void ParameterClient::clearRuntimeState(bool markInvalid) {
    mPendingWrites.clear();
    mInFlightWrites.clear();
    mConfirmingWrites.clear();
    mWriteFlushTimer.stop();
    for (auto& param : mProxyParameters) {
        param->setIsPending(false);
        if (markInvalid) {
            param->setIsValid(false);
        }
    }
}
