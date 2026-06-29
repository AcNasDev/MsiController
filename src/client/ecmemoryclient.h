#pragma once

#include <QDBusConnection>
#include <QDBusPendingReply>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class ComMsiEcMemoryInterface;

class EcMemoryClient : public QObject {
    Q_OBJECT
public:
    explicit EcMemoryClient(const QDBusConnection& connection, QObject* parent = nullptr);

    void setConnected(bool connected);
    QVariantMap readEcMemory(int offset, int length) const;
    QVariantMap writeEcMemory(int offset, const QVariantList& bytes);
    QVariantMap writeEcMemoryBits(int offset, int mask, int value);

private:
    ComMsiEcMemoryInterface* mMemoryInterface{nullptr};
    bool mIsConnected{false};

    QVariantMap ecMemoryReplyToMap(QDBusPendingReply<QDBusVariant>& reply) const;
};
