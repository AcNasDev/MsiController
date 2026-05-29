#pragma once

#include <QAbstractListModel>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QStringList>
#include <QTimer>
#include <QVariant>

#include "proxyparameter.h"
#include "struct.h"

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
    ComMsiEcInterface* mEcInterface{nullptr};
    bool mIsConnected{false};
    QMap<Msi::Parametr, ProxyParameter*> mProxyParameters;
    QMap<Msi::Parametr, QVariant> mPendingWrites;
    QMap<Msi::Parametr, QVariant> mInFlightWrites;
    QSet<Msi::Parametr> mConfirmingWrites;
    QTimer mWriteFlushTimer;

    void init();
    void applyRemoteValue(Msi::Parametr param, const QVariant& value, bool markValid = false);
    void queueWrite(Msi::Parametr param, const QVariant& value);
    void flushPendingWrites();
    void refreshParameter(Msi::Parametr param);
    bool hasQueuedWrite(Msi::Parametr param) const;
    bool hasWriteInProgress(Msi::Parametr param) const;
};
