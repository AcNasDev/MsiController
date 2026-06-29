#include "deviceprofileservice.h"

#include "supportconfig.h"

DeviceProfileService::DeviceProfileService(QObject* parent) : QObject(parent) {}

void DeviceProfileService::setRepository(SupportConfigRepository* repository) {
    if (mRepository == repository) {
        return;
    }

    if (mRepository) {
        disconnect(mRepository, nullptr, this, nullptr);
    }
    mRepository = repository;
    if (mRepository) {
        connect(mRepository, &SupportConfigRepository::profilesChanged, this, &DeviceProfileService::profilesChanged);
    }
}

void DeviceProfileService::setProfileApplier(std::function<bool(QString*)> applier) {
    mProfileApplier = std::move(applier);
}

bool DeviceProfileService::applySupportProfile(QString* errorMessage) {
    if (!mProfileApplier) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Live profile apply is not available");
        }
        return false;
    }

    const bool ok = mProfileApplier(errorMessage);
    if (ok) {
        emit supportProfileApplied();
        emit activeProfileChanged();
        emit profilesChanged();
    }
    return ok;
}

QVariantList DeviceProfileService::supportedProfiles() const {
    return mRepository ? mRepository->profilesForDbus() : QVariantList{};
}

QVariantMap DeviceProfileService::activeProfile() const {
    return mRepository ? mRepository->activeProfileForDbus() : QVariantMap{};
}

QVariantMap DeviceProfileService::saveProfile(const QVariantMap& profile) {
    QVariantMap result;
    if (!mRepository) {
        result.insert(QStringLiteral("ok"), false);
        result.insert(QStringLiteral("error"), QStringLiteral("Support config repository is not available"));
        return result;
    }

    QString errorMessage;
    const bool saved = mRepository->saveUserProfile(profile, &errorMessage);
    bool applied = false;
    if (saved) {
        QString applyError;
        applied = applySupportProfile(&applyError);
        if (!applied) {
            errorMessage = QStringLiteral("Profile saved, but live apply failed: %1").arg(applyError);
        }
    }
    result.insert(QStringLiteral("ok"), saved && applied);
    result.insert(QStringLiteral("saved"), saved);
    result.insert(QStringLiteral("applied"), applied);
    result.insert(QStringLiteral("error"), errorMessage);
    return result;
}

QVariantMap DeviceProfileService::removeProfile(const QString& profileId) {
    QVariantMap result;
    if (!mRepository) {
        result.insert(QStringLiteral("ok"), false);
        result.insert(QStringLiteral("error"), QStringLiteral("Support config repository is not available"));
        return result;
    }

    QString errorMessage;
    const bool removed = mRepository->removeUserProfile(profileId, &errorMessage);
    bool applied = false;
    if (removed) {
        QString applyError;
        applied = applySupportProfile(&applyError);
        if (!applied) {
            errorMessage = QStringLiteral("Profile removed, but live apply failed: %1").arg(applyError);
        }
    }
    result.insert(QStringLiteral("ok"), removed && applied);
    result.insert(QStringLiteral("removed"), removed);
    result.insert(QStringLiteral("applied"), applied);
    result.insert(QStringLiteral("error"), errorMessage);
    return result;
}
