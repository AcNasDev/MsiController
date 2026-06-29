#include "deviceprofileclient.h"

#include <QDBusPendingCallWatcher>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

#include "dbusapi.h"
#include "dbuscodec.h"
#include "ecprofilesinterface.h"
#include "logging.h"
#include "struct.h"

namespace {
QVariantMap resultMap(bool ok, const QString& error = {}, const QVariantMap& extra = {}) {
    QVariantMap result = extra;
    result.insert(QStringLiteral("ok"), ok);
    result.insert(QStringLiteral("error"), error);
    return result;
}
} // namespace

DeviceProfileClient::DeviceProfileClient(const QDBusConnection& connection, QObject* parent) : QObject(parent) {
    mProfilesInterface = new ComMsiEcProfilesInterface(QString::fromLatin1(MsiDbusApi::serviceName),
                                                       QString::fromLatin1(MsiDbusApi::profilesPath),
                                                       connection,
                                                       this);
    connect(mProfilesInterface,
            &ComMsiEcProfilesInterface::supportedDeviceProfilesChanged,
            this,
            &DeviceProfileClient::refreshDeviceProfiles);
    connect(mProfilesInterface,
            &ComMsiEcProfilesInterface::activeDeviceProfileChanged,
            this,
            &DeviceProfileClient::refreshActiveDeviceProfile);
}

void DeviceProfileClient::setConnected(bool connected) {
    if (mIsConnected == connected) {
        return;
    }

    mIsConnected = connected;
    if (!mIsConnected) {
        mDeviceProfiles.clear();
        mActiveDeviceProfile.clear();
        emit deviceProfilesChanged();
        emit activeDeviceProfileChanged();
        return;
    }

    refreshDeviceProfiles();
    refreshActiveDeviceProfile();
}

QVariantList DeviceProfileClient::deviceProfiles() const {
    return mDeviceProfiles;
}

QVariantMap DeviceProfileClient::activeDeviceProfile() const {
    return mActiveDeviceProfile;
}

QString DeviceProfileClient::status() const {
    return mStatus;
}

void DeviceProfileClient::refreshDeviceProfiles() {
    if (!mIsConnected || !mProfilesInterface) {
        return;
    }

    auto* watcher = new QDBusPendingCallWatcher(mProfilesInterface->supportedDeviceProfiles(), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher]() {
        if (watcher->isError()) {
            qCWarning(msiClientLog) << "Failed to fetch supported device profiles:" << watcher->error();
            setStatus(tr("Failed to load profiles"));
            watcher->deleteLater();
            return;
        }

        const auto arguments = watcher->reply().arguments();
        if (!arguments.isEmpty()) {
            mDeviceProfiles = MsiDbusCodec::unwrapReplyArgument(arguments.at(0)).toList();
            emit deviceProfilesChanged();
        }
        watcher->deleteLater();
    });
}

void DeviceProfileClient::refreshActiveDeviceProfile() {
    if (!mIsConnected || !mProfilesInterface) {
        return;
    }

    auto* watcher = new QDBusPendingCallWatcher(mProfilesInterface->activeDeviceProfile(), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher]() {
        if (!watcher->isError()) {
            const auto arguments = watcher->reply().arguments();
            if (!arguments.isEmpty()) {
                mActiveDeviceProfile = MsiDbusCodec::unwrapReplyArgument(arguments.at(0)).toMap();
                emit activeDeviceProfileChanged();
            }
        }
        watcher->deleteLater();
    });
}

void DeviceProfileClient::saveDeviceProfile(const QVariantMap& profile) {
    if (!mIsConnected || !mProfilesInterface) {
        setStatus(tr("Service is disconnected"));
        return;
    }

    auto* watcher =
        new QDBusPendingCallWatcher(mProfilesInterface->saveSupportedDeviceProfile(MsiDbusCodec::wrap(profile)), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher]() {
        bool ok = false;
        QString errorMessage;
        if (watcher->isError()) {
            errorMessage = watcher->error().message();
        } else {
            const auto arguments = watcher->reply().arguments();
            if (!arguments.isEmpty()) {
                const QVariantMap result = MsiDbusCodec::unwrapReplyArgument(arguments.at(0)).toMap();
                ok = result.value(QStringLiteral("ok")).toBool();
                errorMessage = result.value(QStringLiteral("error")).toString();
            }
        }

        setStatus(ok ? tr("Profile saved and applied") : errorMessage);
        refreshDeviceProfiles();
        refreshActiveDeviceProfile();
        watcher->deleteLater();
    });
}

void DeviceProfileClient::removeDeviceProfile(const QString& profileId) {
    if (!mIsConnected || !mProfilesInterface) {
        setStatus(tr("Service is disconnected"));
        return;
    }

    auto* watcher =
        new QDBusPendingCallWatcher(mProfilesInterface->removeSupportedDeviceProfile(MsiDbusCodec::wrap(profileId)),
                                    this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher]() {
        bool ok = false;
        QString errorMessage;
        if (watcher->isError()) {
            errorMessage = watcher->error().message();
        } else {
            const auto arguments = watcher->reply().arguments();
            if (!arguments.isEmpty()) {
                const QVariantMap result = MsiDbusCodec::unwrapReplyArgument(arguments.at(0)).toMap();
                ok = result.value(QStringLiteral("ok")).toBool();
                errorMessage = result.value(QStringLiteral("error")).toString();
            }
        }

        setStatus(ok ? tr("Profile removed and applied") : errorMessage);
        refreshDeviceProfiles();
        refreshActiveDeviceProfile();
        watcher->deleteLater();
    });
}

QVariantMap DeviceProfileClient::importDeviceProfile(const QString& pathOrUrl) {
    const QString path = normalizeFilePath(pathOrUrl);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        const QString error = file.errorString();
        setStatus(error);
        return resultMap(false, error);
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        const QString error = tr("Invalid profile JSON: %1").arg(parseError.errorString());
        setStatus(error);
        return resultMap(false, error);
    }

    QVariantMap profile = document.object().toVariantMap();
    if (profile.contains(QStringLiteral("profile"))) {
        profile = profile.value(QStringLiteral("profile")).toMap();
    }
    if (!profile.contains(QStringLiteral("id")) || !profile.contains(QStringLiteral("values"))) {
        const QString error = tr("Profile JSON must contain id and values");
        setStatus(error);
        return resultMap(false, error);
    }

    saveDeviceProfile(profile);
    return resultMap(true, {}, {{QStringLiteral("id"), profile.value(QStringLiteral("id"))}});
}

QVariantMap DeviceProfileClient::exportDeviceProfile(const QVariantMap& profile, const QString& pathOrUrl) {
    const QString path = normalizeFilePath(pathOrUrl);
    if (profile.isEmpty()) {
        const QString error = tr("No profile selected");
        setStatus(error);
        return resultMap(false, error);
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        const QString error = file.errorString();
        setStatus(error);
        return resultMap(false, error);
    }

    file.write(QJsonDocument::fromVariant(profile).toJson(QJsonDocument::Indented));
    setStatus(tr("Profile exported"));
    return resultMap(true, {}, {{QStringLiteral("path"), path}});
}

void DeviceProfileClient::setStatus(const QString& status) {
    if (mStatus == status) {
        return;
    }

    mStatus = status;
    emit statusChanged();
}

QString DeviceProfileClient::normalizeFilePath(const QString& pathOrUrl) const {
    const QUrl url(pathOrUrl);
    if (url.isValid() && url.isLocalFile()) {
        return url.toLocalFile();
    }
    return pathOrUrl;
}
