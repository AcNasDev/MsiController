#include "ecmemoryclient.h"

#include "ecmemoryinterface.h"
#include "struct.h"

namespace {
constexpr auto serviceName = "com.msi.ec";
constexpr auto memoryPath = "/Memory";
} // namespace

EcMemoryClient::EcMemoryClient(const QDBusConnection& connection, QObject* parent) : QObject(parent) {
    mMemoryInterface = new ComMsiEcMemoryInterface(QString::fromLatin1(serviceName),
                                                   QString::fromLatin1(memoryPath),
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

    auto reply = mMemoryInterface->readEcMemory(QDBusVariant(QVariant::fromValue(Msi::Msg(offset))),
                                                QDBusVariant(QVariant::fromValue(Msi::Msg(length))));
    return ecMemoryReplyToMap(reply);
}

QVariantMap EcMemoryClient::writeEcMemory(int offset, const QVariantList& bytes) {
    if (!mIsConnected || !mMemoryInterface) {
        return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), tr("Service is disconnected")}};
    }

    auto reply = mMemoryInterface->writeEcMemory(QDBusVariant(QVariant::fromValue(Msi::Msg(offset))),
                                                 QDBusVariant(QVariant::fromValue(Msi::Msg(bytes))));
    return ecMemoryReplyToMap(reply);
}

QVariantMap EcMemoryClient::writeEcMemoryBits(int offset, int mask, int value) {
    if (!mIsConnected || !mMemoryInterface) {
        return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), tr("Service is disconnected")}};
    }

    auto reply = mMemoryInterface->writeEcMemoryBits(QDBusVariant(QVariant::fromValue(Msi::Msg(offset))),
                                                     QDBusVariant(QVariant::fromValue(Msi::Msg(mask))),
                                                     QDBusVariant(QVariant::fromValue(Msi::Msg(value))));
    return ecMemoryReplyToMap(reply);
}

QVariantMap EcMemoryClient::ecMemoryReplyToMap(QDBusPendingReply<QDBusVariant>& reply) const {
    reply.waitForFinished();
    if (reply.isError()) {
        return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), reply.error().message()}};
    }

    return qdbus_cast<Msi::Msg>(reply.argumentAt<0>().variant()).variant.toMap();
}
