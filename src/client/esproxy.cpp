#include "esproxy.h"

#include "ecinterface.h"

#include <QVector>

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
                if (hasWriteInProgress(paramName))
                    return;
                applyRemoteValue(paramName, paramValue);
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
                if (!hasWriteInProgress(name)) {
                    auto value = qdbus_cast<Msi::Msg>(arguments.at(0).value<QDBusVariant>().variant()).variant;
                    applyRemoteValue(name, value, true);
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

void EsProxy::queueWrite(Msi::Parametr param, const QVariant& value) {
    if (!mIsConnected) {
        refreshParameter(param);
        return;
    }

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
                    mInFlightWrites.remove(param);
                    mConfirmingWrites.remove(param);
                    if (!hasQueuedWrite(param)) {
                        applyRemoteValue(param, value, true);
                        if (auto it = mProxyParameters.find(param); it != mProxyParameters.end()) {
                            it.value()->setIsPending(false);
                        }
                    }
                }
            }
        }

        for (const auto& param : batchParams) {
            if (confirmedParams.contains(param))
                continue;
            mInFlightWrites.remove(param);
            mConfirmingWrites.insert(param);
            refreshParameter(param);
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
                mConfirmingWrites.remove(param);
            }
            if (auto it = mProxyParameters.find(param); it != mProxyParameters.end()) {
                it.value()->setIsPending(hasQueuedWrite(param));
            }
            watcher->deleteLater();
            return;
        }

        if (!hasQueuedWrite(param)) {
            const auto arguments = watcher->reply().arguments();
            if (arguments.isEmpty()) {
                qWarning() << "Empty refresh reply for" << param;
                mConfirmingWrites.remove(param);
                if (auto it = mProxyParameters.find(param); it != mProxyParameters.end()) {
                    it.value()->setIsPending(false);
                }
                watcher->deleteLater();
                return;
            }
            auto value = qdbus_cast<Msi::Msg>(arguments.at(0).value<QDBusVariant>().variant()).variant;
            applyRemoteValue(param, value, true);
            mConfirmingWrites.remove(param);
            if (auto it = mProxyParameters.find(param); it != mProxyParameters.end()) {
                it.value()->setIsPending(false);
            }
        }
        watcher->deleteLater();
    });
}

bool EsProxy::hasQueuedWrite(Msi::Parametr param) const {
    return mPendingWrites.contains(param) || mInFlightWrites.contains(param);
}

bool EsProxy::hasWriteInProgress(Msi::Parametr param) const {
    return hasQueuedWrite(param) || mConfirmingWrites.contains(param);
}
