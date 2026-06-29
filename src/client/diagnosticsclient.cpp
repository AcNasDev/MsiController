#include "diagnosticsclient.h"

#include <QDBusPendingCallWatcher>
#include <QFile>
#include <QJsonDocument>
#include <QUrl>

#include "dbusapi.h"
#include "dbuscodec.h"
#include "echealthinterface.h"

namespace {
QString appVersion() {
    return QString(CMAKE_TOOLS_GIT_TAG_MAJOR) + QStringLiteral(".") + QString(CMAKE_TOOLS_GIT_TAG_MINOR) +
           QStringLiteral(".") + QString(CMAKE_TOOLS_GIT_TAG_PATCH);
}
} // namespace

DiagnosticsClient::DiagnosticsClient(const QDBusConnection& connection, QObject* parent) : QObject(parent) {
    mHealthInterface = new ComMsiEcHealthInterface(QString::fromLatin1(MsiDbusApi::serviceName),
                                                   QString::fromLatin1(MsiDbusApi::healthPath),
                                                   connection,
                                                   this);
    mRefreshTimer.setInterval(10000);
    connect(&mRefreshTimer, &QTimer::timeout, this, &DiagnosticsClient::refresh);
}

void DiagnosticsClient::setConnected(bool connected) {
    if (mIsConnected == connected) {
        return;
    }

    mIsConnected = connected;
    if (!mIsConnected) {
        mRefreshTimer.stop();
        mDiagnostics.clear();
        setStatus(tr("Service is disconnected"));
        emit diagnosticsChanged();
        return;
    }

    refresh();
    mRefreshTimer.start();
}

QVariantMap DiagnosticsClient::diagnostics() const {
    return mDiagnostics;
}

QString DiagnosticsClient::status() const {
    return mStatus;
}

bool DiagnosticsClient::restartRequired() const {
    if (mDiagnostics.isEmpty()) {
        return false;
    }

    const int apiVersion = mDiagnostics.value(QStringLiteral("apiVersion")).toInt();
    const QString serviceVersion = mDiagnostics.value(QStringLiteral("serviceVersion")).toString();
    return apiVersion != MsiDbusApi::currentApiVersion || (!serviceVersion.isEmpty() && serviceVersion != appVersion());
}

void DiagnosticsClient::refresh() {
    if (!mIsConnected || !mHealthInterface) {
        return;
    }

    auto* watcher = new QDBusPendingCallWatcher(mHealthInterface->diagnostics(), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher]() {
        if (watcher->isError()) {
            setStatus(tr("Diagnostics are unavailable: %1").arg(watcher->error().message()));
            watcher->deleteLater();
            return;
        }

        const auto arguments = watcher->reply().arguments();
        if (arguments.isEmpty()) {
            setStatus(tr("Diagnostics reply is empty"));
            watcher->deleteLater();
            return;
        }

        mDiagnostics = MsiDbusCodec::unwrapReplyArgument(arguments.at(0)).toMap();
        setStatus(restartRequired() ? tr("Client and service versions differ") : tr("Diagnostics refreshed"));
        emit diagnosticsChanged();
        watcher->deleteLater();
    });
}

QVariantMap DiagnosticsClient::saveSupportBundle(const QString& pathOrUrl, const QVariantMap& clientData) {
    QVariantMap result;
    if (!mIsConnected || !mHealthInterface) {
        result.insert(QStringLiteral("ok"), false);
        result.insert(QStringLiteral("error"), tr("Service is disconnected"));
        return result;
    }

    auto reply = mHealthInterface->supportBundle();
    reply.waitForFinished();
    if (reply.isError()) {
        result.insert(QStringLiteral("ok"), false);
        result.insert(QStringLiteral("error"), reply.error().message());
        return result;
    }

    const QString outputPath = normalizeOutputPath(pathOrUrl);
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        result.insert(QStringLiteral("ok"), false);
        result.insert(QStringLiteral("error"), file.errorString());
        return result;
    }

    QVariantMap bundle = MsiDbusCodec::unwrap(reply.argumentAt<0>()).toMap();
    if (!clientData.isEmpty()) {
        bundle.insert(QStringLiteral("client"), clientData);
    }
    file.write(QJsonDocument::fromVariant(bundle).toJson(QJsonDocument::Indented));
    result.insert(QStringLiteral("ok"), true);
    result.insert(QStringLiteral("path"), outputPath);
    return result;
}

void DiagnosticsClient::setStatus(const QString& status) {
    if (mStatus == status) {
        return;
    }

    mStatus = status;
    emit statusChanged();
}

QString DiagnosticsClient::normalizeOutputPath(const QString& pathOrUrl) const {
    const QUrl url(pathOrUrl);
    if (url.isValid() && url.isLocalFile()) {
        return url.toLocalFile();
    }
    return pathOrUrl;
}
