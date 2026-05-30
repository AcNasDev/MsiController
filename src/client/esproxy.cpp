#include "esproxy.h"

#include "ecinterface.h"

#include <QDateTime>
#include <QSet>
#include <QVector>

#include <algorithm>

namespace {
constexpr int writeConfirmRetryMs = 140;
constexpr int writeConfirmTimeoutMs = 2600;
constexpr quint32 cpuFrequencyConfirmToleranceKhz = 25000;

qint64 currentTimeMs() {
    return QDateTime::currentMSecsSinceEpoch();
}

bool frequencyClose(quint32 expected, quint32 actual) {
    return qAbs(static_cast<qint64>(expected) - static_cast<qint64>(actual)) <= cpuFrequencyConfirmToleranceKhz;
}
} // namespace

EsProxy::EsProxy(QObject* parent) : QObject(parent) {
    registerMetaType();
    QDBusConnection connection{QDBusConnection::systemBus()};
    mEcInterface = new ComMsiEcInterface("com.msi.ec", "/", connection, this);
    mWriteFlushTimer.setSingleShot(true);
    mWriteFlushTimer.setInterval(40);
    connect(&mWriteFlushTimer, &QTimer::timeout, this, &EsProxy::flushPendingWrites);

    mIsConnected = connection.interface()->isServiceRegistered("com.msi.ec");

    // Обработчик изменения параметров
    connect(mEcInterface,
            &ComMsiEcInterface::parameterChanged,
            this,
            [this](const QDBusVariant& name, const QDBusVariant& value) {
                auto msgName = qdbus_cast<Msi::Msg>(name.variant());
                Msi::Parametr paramName = msgName.variant.value<Msi::Parametr>();
                auto msgValue = qdbus_cast<Msi::Msg>(value.variant());
                QVariant paramValue = msgValue.variant;
                if (hasQueuedWrite(paramName))
                    return;
                handleRemoteValue(paramName, paramValue);
            });

    QDBusServiceWatcher* serviceWatcher =
        new QDBusServiceWatcher("com.msi.ec",
                                connection,
                                QDBusServiceWatcher::WatchForUnregistration | QDBusServiceWatcher::WatchForRegistration,
                                this);

    connect(serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString& service) {
        if (service == "com.msi.ec") {
            mIsConnected = false;
            emit connectionChanged(mIsConnected);
            mPendingWrites.clear();
            mInFlightWrites.clear();
            mConfirmingWrites.clear();
            mWriteFlushTimer.stop();
            for (auto& param : mProxyParameters) {
                param->setIsPending(false);
            }
            qWarning() << "EC service disconnected!";
        }
    });

    connect(serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, [this](const QString& service) {
        if (service == "com.msi.ec") {
            mIsConnected = true;
            emit connectionChanged(mIsConnected);
            qInfo() << "EC service reconnected!";
            init();
        }
    });

    for (int i = 0; i < static_cast<int>(Msi::Parametr::CountEs); ++i) {
        Msi::Parametr param = static_cast<Msi::Parametr>(i);
        mProxyParameters[param] = new ProxyParameter(this);
        mProxyParameters[param]->setIsValid(false);

        connect(mProxyParameters[param], &ProxyParameter::valueChangedForEsProxy, this, [this, param]() {
            queueWrite(param, mProxyParameters[param]->value());
        });
    }

    if (mIsConnected) {
        init();
    }
}

void EsProxy::init() {
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(mEcInterface->availableParameters(), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher]() {
        if (watcher->isError()) {
            qWarning() << "Failed to fetch available parameters:" << watcher->error();
            watcher->deleteLater();
            return;
        }

        const auto availableArguments = watcher->reply().arguments();
        if (availableArguments.isEmpty()) {
            qWarning() << "Empty available parameters reply";
            watcher->deleteLater();
            return;
        }

        auto args = qdbus_cast<Msi::Msg>(availableArguments.at(0).value<QDBusVariant>().variant())
                        .variant.value<QVariantList>();
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

        // Запрос данных для каждого параметра
        for (auto& name : params) {
            QDBusPendingCall valueCall =
                mEcInterface->availableValues(QDBusVariant(QVariant::fromValue(Msi::Msg(name))));
            QDBusPendingCallWatcher* valueWatcher = new QDBusPendingCallWatcher(valueCall, this);
            connect(valueWatcher, &QDBusPendingCallWatcher::finished, this, [this, name, valueWatcher]() {
                if (valueWatcher->isError()) {
                    qWarning() << "Failed to fetch values for" << name << ":" << valueWatcher->error();
                    valueWatcher->deleteLater();
                    return;
                }
                const auto arguments = valueWatcher->reply().arguments();
                if (arguments.isEmpty()) {
                    qWarning() << "Empty available values reply for" << name;
                    valueWatcher->deleteLater();
                    return;
                }
                auto reply = qdbus_cast<Msi::Msg>(arguments.at(0).value<QDBusVariant>().variant()).variant;
                if (auto it{mProxyParameters.find(name)}; it != mProxyParameters.end()) {
                    it.value()->setAvailableValues(reply);
                }
                valueWatcher->deleteLater();
            });

            // Запрос текущего значения
            QDBusPendingCall readCall = mEcInterface->readParameter(QDBusVariant(QVariant::fromValue(Msi::Msg(name))));
            QDBusPendingCallWatcher* readWatcher = new QDBusPendingCallWatcher(readCall, this);
            connect(readWatcher, &QDBusPendingCallWatcher::finished, this, [this, name, readWatcher]() {
                if (readWatcher->isError()) {
                    qWarning() << "Failed to read" << name << ":" << readWatcher->error();
                    readWatcher->deleteLater();
                    return;
                }
                const auto arguments = readWatcher->reply().arguments();
                if (arguments.isEmpty()) {
                    qWarning() << "Empty read reply for" << name;
                    readWatcher->deleteLater();
                    return;
                }
                if (!hasQueuedWrite(name)) {
                    auto value = qdbus_cast<Msi::Msg>(arguments.at(0).value<QDBusVariant>().variant()).variant;
                    handleRemoteValue(name, value, true);
                }
                readWatcher->deleteLater();
            });
        }
    });
}

bool EsProxy::isConnected() const {
    return mIsConnected;
}

ProxyParameter* EsProxy::getProxyParameter(const Msi::Parametr& name) const {
    if (mProxyParameters.contains(name)) {
        return mProxyParameters[name];
    }
    return nullptr;
}

void EsProxy::setCpuScalingMaxFrequencies(const QVariantList& frequenciesKhz) {
    ProxyParameter* param = cpuControlWriteParameter();
    if (!param || frequenciesKhz.isEmpty())
        return;

    Msi::CpuConfig config = param->value().value<Msi::CpuConfig>();
    if (config.cpus.isEmpty())
        return;

    const int count = std::min<int>(config.cpus.size(), frequenciesKhz.size());
    for (int i = 0; i < count; ++i) {
        bool ok = false;
        const quint32 requested = frequenciesKhz.at(i).toUInt(&ok);
        if (!ok || requested == 0)
            continue;

        Msi::Cpu& cpu = config.cpus[i];
        const quint32 minFreq = cpu.minFreq;
        const quint32 maxFreq = std::max(cpu.maxFreq, minFreq);
        cpu.scalingMaxFreq = std::min(std::max(requested, minFreq), maxFreq);
    }

    param->setValue(QVariant::fromValue(config));
}

void EsProxy::setCpuGovernor(const QString& governor) {
    if (governor.isEmpty())
        return;

    ProxyParameter* param = cpuControlWriteParameter();
    if (!param)
        return;

    Msi::CpuConfig config = param->value().value<Msi::CpuConfig>();
    if (config.cpus.isEmpty())
        return;

    for (Msi::Cpu& cpu : config.cpus)
        cpu.availableGovernor = governor;

    param->setValue(QVariant::fromValue(config));
}

ProxyParameter* EsProxy::cpuControlWriteParameter() const {
    if (auto it = mProxyParameters.find(Msi::Parametr::CpuControlConfig); it != mProxyParameters.end()) {
        const Msi::CpuConfig config = it.value()->value().value<Msi::CpuConfig>();
        if (it.value()->isValid() && !config.cpus.isEmpty())
            return it.value();
    }

    if (auto it = mProxyParameters.find(Msi::Parametr::CpuConfig); it != mProxyParameters.end()) {
        const Msi::CpuConfig config = it.value()->value().value<Msi::CpuConfig>();
        if (it.value()->isValid() && !config.cpus.isEmpty())
            return it.value();
    }

    return nullptr;
}

void EsProxy::applyRemoteValue(Msi::Parametr param, const QVariant& value, bool markValid) {
    if (auto it = mProxyParameters.find(param); it != mProxyParameters.end()) {
        it.value()->setBlockSignalsForEsProxy(true);
        it.value()->setValue(value);
        it.value()->setBlockSignalsForEsProxy(false);
        if (markValid) {
            it.value()->setIsValid(true);
        }
    }
}

void EsProxy::handleRemoteValue(Msi::Parametr param, const QVariant& value, bool markValid) {
    if (hasQueuedWrite(param))
        return;

    if (auto confirmation = mConfirmingWrites.find(param); confirmation != mConfirmingWrites.end()) {
        const bool confirmed = confirmationMatches(param, confirmation.value().expectedValue, value);
        const bool expired = confirmationExpired(confirmation.value());
        if (!confirmed && !expired) {
            if (markValid) {
                if (auto proxyParam = mProxyParameters.find(param); proxyParam != mProxyParameters.end())
                    proxyParam.value()->setIsValid(true);
            }
            scheduleConfirmationRefresh(param);
            return;
        }

        mConfirmingWrites.erase(confirmation);
        applyRemoteValue(param, value, markValid);
        if (auto proxyParam = mProxyParameters.find(param); proxyParam != mProxyParameters.end())
            proxyParam.value()->setIsPending(false);
        return;
    }

    applyRemoteValue(param, value, markValid);
}

void EsProxy::queueWrite(Msi::Parametr param, const QVariant& value) {
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

void EsProxy::flushPendingWrites() {
    if (!mIsConnected)
        return;

    QVariantList updates;
    QVector<Msi::Parametr> batchParams;
    const auto pendingParams = mPendingWrites.keys();
    for (const auto& param : pendingParams) {
        if (mInFlightWrites.contains(param))
            continue;

        const QVariant value = mPendingWrites.take(param);
        mInFlightWrites[param] = value;
        batchParams.append(param);
        updates << QVariant::fromValue(param) << value;
    }

    if (batchParams.isEmpty())
        return;

    auto* watcher = new QDBusPendingCallWatcher(
        mEcInterface->writeParameters(QDBusVariant(QVariant::fromValue(Msi::Msg(updates)))),
        this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, batchParams, watcher]() {
        QSet<Msi::Parametr> confirmedParams;
        if (watcher->isError()) {
            qWarning() << "Failed to write parameter batch:" << watcher->error();
        } else {
            const auto arguments = watcher->reply().arguments();
            if (arguments.isEmpty()) {
                qWarning() << "Empty batch write reply";
            } else {
                const auto values =
                    qdbus_cast<Msi::Msg>(arguments.at(0).value<QDBusVariant>().variant()).variant.toList();
                if (values.size() % 2 != 0) {
                    qWarning() << "Invalid batch write reply size:" << values.size();
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
            if (confirmedParams.contains(param))
                continue;
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

void EsProxy::refreshParameter(Msi::Parametr param) {
    if (!mIsConnected || !mEcInterface)
        return;

    auto* watcher = new QDBusPendingCallWatcher(
        mEcInterface->readParameter(QDBusVariant(QVariant::fromValue(Msi::Msg(param)))),
        this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, param, watcher]() {
        if (watcher->isError()) {
            qWarning() << "Failed to refresh" << param << ":" << watcher->error();
            if (!hasQueuedWrite(param)) {
                if (auto confirmation = mConfirmingWrites.find(param); confirmation != mConfirmingWrites.end()) {
                    if (confirmationExpired(confirmation.value()))
                        mConfirmingWrites.erase(confirmation);
                    else
                        scheduleConfirmationRefresh(param);
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
                qWarning() << "Empty refresh reply for" << param;
                if (auto confirmation = mConfirmingWrites.find(param); confirmation != mConfirmingWrites.end()) {
                    if (confirmationExpired(confirmation.value()))
                        mConfirmingWrites.erase(confirmation);
                    else
                        scheduleConfirmationRefresh(param);
                }
                if (auto it = mProxyParameters.find(param); it != mProxyParameters.end()) {
                    it.value()->setIsPending(hasWriteInProgress(param));
                }
                watcher->deleteLater();
                return;
            }
            auto value = qdbus_cast<Msi::Msg>(arguments.at(0).value<QDBusVariant>().variant()).variant;
            handleRemoteValue(param, value, true);
        }
        watcher->deleteLater();
    });
}

void EsProxy::beginConfirmation(Msi::Parametr param, const QVariant& expectedValue) {
    PendingConfirmation confirmation;
    confirmation.expectedValue = expectedValue;
    confirmation.startedAtMs = currentTimeMs();
    mConfirmingWrites[param] = confirmation;
    if (auto it = mProxyParameters.find(param); it != mProxyParameters.end())
        it.value()->setIsPending(true);
}

void EsProxy::scheduleConfirmationRefresh(Msi::Parametr param) {
    if (!mIsConnected)
        return;

    auto confirmation = mConfirmingWrites.find(param);
    if (confirmation == mConfirmingWrites.end() || confirmation.value().refreshScheduled)
        return;

    confirmation.value().refreshScheduled = true;
    ++confirmation.value().retryCount;
    QTimer::singleShot(writeConfirmRetryMs, this, [this, param]() {
        auto confirmation = mConfirmingWrites.find(param);
        if (confirmation == mConfirmingWrites.end() || hasQueuedWrite(param))
            return;

        confirmation.value().refreshScheduled = false;
        refreshParameter(param);
    });
}

bool EsProxy::confirmationMatches(Msi::Parametr param,
                                  const QVariant& expectedValue,
                                  const QVariant& actualValue) const {
    if (param != Msi::Parametr::CpuConfig && param != Msi::Parametr::CpuControlConfig)
        return expectedValue == actualValue;

    const Msi::CpuConfig expectedConfig = expectedValue.value<Msi::CpuConfig>();
    const Msi::CpuConfig actualConfig = actualValue.value<Msi::CpuConfig>();
    if (expectedConfig.cpus.size() != actualConfig.cpus.size())
        return false;

    for (int i = 0; i < expectedConfig.cpus.size(); ++i) {
        const Msi::Cpu& expectedCpu = expectedConfig.cpus.at(i);
        const Msi::Cpu& actualCpu = actualConfig.cpus.at(i);
        if (!frequencyClose(expectedCpu.scalingMinFreq, actualCpu.scalingMinFreq) ||
            !frequencyClose(expectedCpu.scalingMaxFreq, actualCpu.scalingMaxFreq)) {
            return false;
        }
        if (!expectedCpu.availableGovernor.isEmpty() && expectedCpu.availableGovernor != actualCpu.availableGovernor)
            return false;
    }
    return true;
}

bool EsProxy::confirmationExpired(const PendingConfirmation& confirmation) const {
    return currentTimeMs() - confirmation.startedAtMs >= writeConfirmTimeoutMs;
}

bool EsProxy::hasQueuedWrite(Msi::Parametr param) const {
    return mPendingWrites.contains(param) || mInFlightWrites.contains(param);
}

bool EsProxy::hasWriteInProgress(Msi::Parametr param) const {
    return hasQueuedWrite(param) || mConfirmingWrites.contains(param);
}
