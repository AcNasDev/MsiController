#include "ecmemoryclient.h"

#include "dbusapi.h"
#include "dbuscodec.h"
#include "ecmemoryinterface.h"

EcMemoryClient::EcMemoryClient(const QDBusConnection& connection, QObject* parent) : QObject(parent) {
    mMemoryInterface = new ComMsiEcMemoryInterface(QString::fromLatin1(MsiDbusApi::serviceName),
                                                   QString::fromLatin1(MsiDbusApi::memoryPath),
                                                   connection,
                                                   this);
}

void EcMemoryClient::setConnected(bool connected) {
    mIsConnected = connected;
}

QVariantMap EcMemoryClient::readEcMemory(int offset, int length) const {
    if (!mIsConnected || !mMemoryInterface) {
        return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), tr("Service is disconnected")}};
    }

    auto reply = mMemoryInterface->readEcMemory(MsiDbusCodec::wrap(offset), MsiDbusCodec::wrap(length));
    return ecMemoryReplyToMap(reply);
}

QVariantMap EcMemoryClient::writeEcMemory(int offset, const QVariantList& bytes) {
    if (!mIsConnected || !mMemoryInterface) {
        return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), tr("Service is disconnected")}};
    }

    auto reply = mMemoryInterface->writeEcMemory(MsiDbusCodec::wrap(offset), MsiDbusCodec::wrap(bytes));
    return ecMemoryReplyToMap(reply);
}

QVariantMap EcMemoryClient::writeEcMemoryBits(int offset, int mask, int value) {
    if (!mIsConnected || !mMemoryInterface) {
        return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), tr("Service is disconnected")}};
    }

    auto reply = mMemoryInterface->writeEcMemoryBits(MsiDbusCodec::wrap(offset),
                                                     MsiDbusCodec::wrap(mask),
                                                     MsiDbusCodec::wrap(value));
    return ecMemoryReplyToMap(reply);
}

QVariantMap EcMemoryClient::ecMemoryReplyToMap(QDBusPendingReply<QDBusVariant>& reply) const {
    reply.waitForFinished();
    if (reply.isError()) {
        return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), reply.error().message()}};
    }

    return MsiDbusCodec::unwrap(reply.argumentAt<0>()).toMap();
}
