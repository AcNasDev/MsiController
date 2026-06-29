#include "deviceprofileclient.h"

#include <QDBusPendingCallWatcher>

#include "ecprofilesinterface.h"
#include "struct.h"

namespace {
constexpr auto serviceName = "com.msi.ec";
constexpr auto profilesPath = "/Profiles";
} // namespace

DeviceProfileClient::DeviceProfileClient(const QDBusConnection& connection, QObject* parent) : QObject(parent) {
    mProfilesInterface = new ComMsiEcProfilesInterface(QString::fromLatin1(serviceName),
                                                       QString::fromLatin1(profilesPath),
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
            qWarning() << "Failed to fetch supported device profiles:" << watcher->error();
            setStatus(tr("Failed to load profiles"));
            watcher->deleteLater();
            return;
        }

        const auto arguments = watcher->reply().arguments();
        if (!arguments.isEmpty()) {
            mDeviceProfiles = qdbus_cast<Msi::Msg>(arguments.at(0).value<QDBusVariant>().variant()).variant.toList();
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
                mActiveDeviceProfile =
                    qdbus_cast<Msi::Msg>(arguments.at(0).value<QDBusVariant>().variant()).variant.toMap();
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

    auto* watcher = new QDBusPendingCallWatcher(
        mProfilesInterface->saveSupportedDeviceProfile(QDBusVariant(QVariant::fromValue(Msi::Msg(profile)))),
        this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher]() {
        bool ok = false;
        QString errorMessage;
        if (watcher->isError()) {
            errorMessage = watcher->error().message();
        } else {
            const auto arguments = watcher->reply().arguments();
            if (!arguments.isEmpty()) {
                const QVariantMap result =
                    qdbus_cast<Msi::Msg>(arguments.at(0).value<QDBusVariant>().variant()).variant.toMap();
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

    auto* watcher = new QDBusPendingCallWatcher(
        mProfilesInterface->removeSupportedDeviceProfile(QDBusVariant(QVariant::fromValue(Msi::Msg(profileId)))),
        this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher]() {
        bool ok = false;
        QString errorMessage;
        if (watcher->isError()) {
            errorMessage = watcher->error().message();
        } else {
            const auto arguments = watcher->reply().arguments();
            if (!arguments.isEmpty()) {
                const QVariantMap result =
                    qdbus_cast<Msi::Msg>(arguments.at(0).value<QDBusVariant>().variant()).variant.toMap();
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

void DeviceProfileClient::setStatus(const QString& status) {
    if (mStatus == status) {
        return;
    }

    mStatus = status;
    emit statusChanged();
}
