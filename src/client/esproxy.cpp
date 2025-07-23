#include "esproxy.h"
#include "ecinterface.h"

EsProxy::EsProxy(QObject* parent)
    : QObject(parent) {
    registerMetaType();
    QDBusConnection connection{ QDBusConnection::systemBus() };
    mEcInterface = new ComMsiEcInterface("com.msi.ec", "/", connection, this);

    mIsConnected = connection.interface()->isServiceRegistered("com.msi.ec");

    // Обработчик изменения параметров
    connect(mEcInterface, &ComMsiEcInterface::parameterChanged, this, [this](const QDBusVariant &name, const QDBusVariant &value) {
        auto msgName = qdbus_cast<Msi::Msg>(name.variant());
        Msi::Parametr paramName = msgName.variant.value<Msi::Parametr>();
        auto msgValue = qdbus_cast<Msi::Msg>(value.variant());
        QVariant paramValue = msgValue.variant;
        if(auto it = mProxyParameters.find(paramName); it != mProxyParameters.end()) {
            it.value()->setBlockSignalsForEsProxy(true);
            it.value()->setValue(paramValue);
            it.value()->setBlockSignalsForEsProxy(false);
        }
    });

    QDBusServiceWatcher *serviceWatcher = new QDBusServiceWatcher(
        "com.msi.ec",
        connection,
        QDBusServiceWatcher::WatchForUnregistration | QDBusServiceWatcher::WatchForRegistration,
        this
    );

    connect(serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &service){
        if (service == "com.msi.ec") {
            mIsConnected = false;
            emit connectionChanged(mIsConnected);
            qWarning() << "EC service disconnected!";
        }
    });
    
    connect(serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, [this](const QString &service){
        if (service == "com.msi.ec") {
            mIsConnected = true;
            emit connectionChanged(mIsConnected);
            qInfo() << "EC service reconnected!";
            init();
        }
    });


    for(int i = 0; i < static_cast<int>(Msi::Parametr::CountEs); ++i) {
        Msi::Parametr param = static_cast<Msi::Parametr>(i);
        mProxyParameters[param] = new ProxyParameter(this);
        mProxyParameters[param]->setIsValid(false);

        connect(mProxyParameters[param], &ProxyParameter::valueChangedForEsProxy, this, [this, param]() {
            mEcInterface->writeParameter(
                QDBusVariant(QVariant::fromValue(Msi::Msg(param))),
                QDBusVariant(QVariant::fromValue(Msi::Msg(mProxyParameters[param]->value())))
            );
        });
    }


    if(mIsConnected) {
        init();
    }
}

void EsProxy::init() 
{
    for(auto &p : mProxyParameters) {
        p->setIsValid(false);
    }

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(mEcInterface->availableParameters(), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher]() {
        if (watcher->isError()) {
            qWarning() << "Failed to fetch available parameters:" << watcher->error();
            watcher->deleteLater();
            return;
        }
        
        auto args = qdbus_cast<Msi::Msg>(watcher->reply().arguments().at(0).value<QDBusVariant>().variant()).variant.value<QVariantList>();
        QVector<Msi::Parametr> params;
        for(auto &a : args) {
            if(auto it{ mProxyParameters.find(a.value<Msi::Parametr>())}; it != mProxyParameters.end()) {
                params.append(a.value<Msi::Parametr>());
            }
        }
        watcher->deleteLater();

        // Запрос данных для каждого параметра
        for (auto &name : params) {
            QDBusPendingCall valueCall = mEcInterface->availableValues(
                QDBusVariant(QVariant::fromValue(Msi::Msg(name)))
            );
            QDBusPendingCallWatcher *valueWatcher = new QDBusPendingCallWatcher(valueCall, this);
            connect(valueWatcher, &QDBusPendingCallWatcher::finished, this, [this, name, valueWatcher]() {
                if (valueWatcher->isError()) {
                    qWarning() << "Failed to fetch values for" << name << ":" << valueWatcher->error();
                    valueWatcher->deleteLater();
                    return;
                }
                auto reply = qdbus_cast<Msi::Msg>(valueWatcher->reply().arguments().at(0).value<QDBusVariant>().variant()).variant;
                if(auto it{ mProxyParameters.find(name)}; it != mProxyParameters.end()) {
                    it.value()->setAvailableValues(reply);
                }
                valueWatcher->deleteLater();
            });

            // Запрос текущего значения
            QDBusPendingCall readCall = mEcInterface->readParameter(
                QDBusVariant(QVariant::fromValue(Msi::Msg(name)))
            );
            QDBusPendingCallWatcher *readWatcher = new QDBusPendingCallWatcher(readCall, this);
            connect(readWatcher, &QDBusPendingCallWatcher::finished, this, [this, name, readWatcher]() {
                if (readWatcher->isError()) {
                    qWarning() << "Failed to read" << name << ":" << readWatcher->error();
                    readWatcher->deleteLater();
                    return;
                }
                auto value = qdbus_cast<Msi::Msg>(readWatcher->reply().arguments().at(0).value<QDBusVariant>().variant()).variant;
                if(auto it{ mProxyParameters.find(name)}; it != mProxyParameters.end()) {
                    it.value()->setValue(value);
                    it.value()->setIsValid(true);
                }
                readWatcher->deleteLater();
            });
        }
    });
}

bool EsProxy::isConnected() const 
{
    return mIsConnected;
}

ProxyParameter* EsProxy::getProxyParameter(const Msi::Parametr& name) const 
{
    if (mProxyParameters.contains(name)) {
        return mProxyParameters[name];
    }
    return nullptr;
}