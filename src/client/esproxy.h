#pragma once

#include <QObject>
#include <QMap>
#include <QVariant>
#include <QStringList>
#include <QAbstractListModel>

#include "struct.h"
#include "proxyparameter.h"

class ComMsiEcInterface;
class EsProxy : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionChanged)
public:
    explicit EsProxy(QObject* parent = nullptr);
    virtual ~EsProxy() = default;

    Q_INVOKABLE bool isConnected() const;
    Q_INVOKABLE ProxyParameter* getProxyParameter(const Msi::Parametr& name) const;

signals:
    void connectionChanged(bool isConnected);

private:
    ComMsiEcInterface* mEcInterface{ nullptr };
    bool mIsConnected{ false };
    QMap<Msi::Parametr, ProxyParameter*> mProxyParameters;

    void init();

};