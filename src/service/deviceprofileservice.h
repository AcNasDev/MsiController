#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <functional>

class SupportConfigRepository;

class DeviceProfileService : public QObject {
    Q_OBJECT
public:
    explicit DeviceProfileService(QObject* parent = nullptr);

    void setRepository(SupportConfigRepository* repository);
    void setProfileApplier(std::function<bool(QString*)> applier);
    bool applySupportProfile(QString* errorMessage = nullptr);

    QVariantList supportedProfiles() const;
    QVariantMap activeProfile() const;
    QVariantMap saveProfile(const QVariantMap& profile);
    QVariantMap removeProfile(const QString& profileId);

signals:
    void profilesChanged();
    void activeProfileChanged();
    void supportProfileApplied();

private:
    SupportConfigRepository* mRepository{nullptr};
    std::function<bool(QString*)> mProfileApplier;
};
