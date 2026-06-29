#pragma once

#include <QDBusConnection>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class ComMsiEcProfilesInterface;

class DeviceProfileClient : public QObject {
    Q_OBJECT
public:
    explicit DeviceProfileClient(const QDBusConnection& connection, QObject* parent = nullptr);

    void setConnected(bool connected);
    QVariantList deviceProfiles() const;
    QVariantMap activeDeviceProfile() const;
    QString status() const;

    void refreshDeviceProfiles();
    void refreshActiveDeviceProfile();
    void saveDeviceProfile(const QVariantMap& profile);
    void removeDeviceProfile(const QString& profileId);
    QVariantMap importDeviceProfile(const QString& pathOrUrl);
    QVariantMap exportDeviceProfile(const QVariantMap& profile, const QString& pathOrUrl);

signals:
    void deviceProfilesChanged();
    void activeDeviceProfileChanged();
    void statusChanged();

private:
    ComMsiEcProfilesInterface* mProfilesInterface{nullptr};
    bool mIsConnected{false};
    QVariantList mDeviceProfiles;
    QVariantMap mActiveDeviceProfile;
    QString mStatus;

    void setStatus(const QString& status);
    QString normalizeFilePath(const QString& pathOrUrl) const;
};
