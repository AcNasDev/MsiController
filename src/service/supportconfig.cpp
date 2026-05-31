#include "supportconfig.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <algorithm>

namespace {
constexpr auto builtinConfigPath = ":/supported-devices.json";
constexpr auto userConfigPath = "/etc/MsiController/supported-devices.json";

QVariantMap objectToVariantMap(const QJsonObject& object) {
    return object.toVariantMap();
}

QJsonObject variantMapToObject(const QVariantMap& map) {
    return QJsonObject::fromVariantMap(map);
}
} // namespace

SupportConfigRepository::SupportConfigRepository(QObject* parent) : QObject(parent) {
    QString errorMessage;
    if (!reload(&errorMessage) && !errorMessage.isEmpty()) {
        qWarning() << errorMessage;
    }
}

bool SupportConfigRepository::reload(QString* errorMessage) {
    QVariantMap defaults;
    QList<Profile> builtinProfiles;
    QList<Profile> userProfiles;

    if (!loadFile(builtinPath(), QStringLiteral("builtin"), &builtinProfiles, &defaults, errorMessage)) {
        return false;
    }

    QString userError;
    if (QFileInfo::exists(userPath()) &&
        !loadFile(userPath(), QStringLiteral("user"), &userProfiles, nullptr, &userError)) {
        qWarning() << userError;
    }

    mDefaults = defaults;
    mBuiltinProfiles = builtinProfiles;
    mUserProfiles = userProfiles;
    emit profilesChanged();
    return true;
}

QMap<QString, QVariant> SupportConfigRepository::configForFirmware(const QString& firmwareVersion) {
    auto matchesFirmware = [&firmwareVersion](const Profile& profile) {
        return firmwareList(profile.values).contains(firmwareVersion);
    };

    const Profile* selectedProfile = nullptr;
    for (const Profile& profile : mUserProfiles) {
        if (matchesFirmware(profile)) {
            selectedProfile = &profile;
            break;
        }
    }
    if (!selectedProfile) {
        for (const Profile& profile : mBuiltinProfiles) {
            if (matchesFirmware(profile)) {
                selectedProfile = &profile;
                break;
            }
        }
    }

    if (!selectedProfile) {
        qWarning() << "No valid config found for firmware:" << firmwareVersion;
        return {};
    }

    mActiveFirmware = firmwareVersion;
    mActiveProfileId = selectedProfile->id;
    mActiveProfileSource = selectedProfile->source;

    QMap<QString, QVariant> config;
    for (auto it = mDefaults.cbegin(); it != mDefaults.cend(); ++it) {
        config.insert(it.key(), it.value());
    }
    for (auto it = selectedProfile->values.cbegin(); it != selectedProfile->values.cend(); ++it) {
        config.insert(it.key(), it.value());
    }

    qDebug() << "Config selected:" << selectedProfile->id << "source:" << selectedProfile->source;
    qDebug() << "Config loaded:";
    const auto keys = config.keys();
    for (const auto& key : keys) {
        qDebug() << key + ": " + config.value(key).toString();
    }
    return config;
}

QVariantList SupportConfigRepository::profilesForDbus() const {
    QVariantList profiles;
    profiles.reserve(mBuiltinProfiles.size() + mUserProfiles.size());
    for (const Profile& profile : mBuiltinProfiles) {
        profiles.append(profileForDbus(profile));
    }
    for (const Profile& profile : mUserProfiles) {
        profiles.append(profileForDbus(profile));
    }
    return profiles;
}

QVariantMap SupportConfigRepository::activeProfileForDbus() const {
    QVariantMap profile;
    profile.insert(QStringLiteral("id"), mActiveProfileId);
    profile.insert(QStringLiteral("source"), mActiveProfileSource);
    profile.insert(QStringLiteral("firmware"), mActiveFirmware);
    return profile;
}

bool SupportConfigRepository::saveUserProfile(const QVariantMap& profile, QString* errorMessage) {
    const QString id = normalizeId(profile.value(QStringLiteral("id")).toString());
    if (id.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Profile id is empty");
        return false;
    }

    QVariantMap values = profile.value(QStringLiteral("values")).toMap();
    if (values.isEmpty()) {
        values = profile;
        values.remove(QStringLiteral("id"));
        values.remove(QStringLiteral("source"));
        values.remove(QStringLiteral("builtin"));
        values.remove(QStringLiteral("user"));
    }
    values = sanitizeValues(values);
    if (firmwareList(values).isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Profile has no AllowedFw values");
        return false;
    }

    Profile userProfile{id, QStringLiteral("user"), values};
    bool replaced = false;
    for (Profile& existing : mUserProfiles) {
        if (existing.id == id) {
            existing = userProfile;
            replaced = true;
            break;
        }
    }
    if (!replaced) {
        mUserProfiles.append(userProfile);
    }

    if (!writeUserProfiles(errorMessage)) {
        reload();
        return false;
    }

    emit profilesChanged();
    return true;
}

bool SupportConfigRepository::removeUserProfile(const QString& profileId, QString* errorMessage) {
    const QString id = normalizeId(profileId);
    const qsizetype oldSize = mUserProfiles.size();
    mUserProfiles.erase(std::remove_if(mUserProfiles.begin(),
                                       mUserProfiles.end(),
                                       [&id](const Profile& profile) { return profile.id == id; }),
                        mUserProfiles.end());

    if (mUserProfiles.size() == oldSize) {
        if (errorMessage)
            *errorMessage = QStringLiteral("User profile not found");
        return false;
    }

    if (!writeUserProfiles(errorMessage)) {
        reload();
        return false;
    }

    emit profilesChanged();
    return true;
}

QString SupportConfigRepository::builtinPath() const {
    return qEnvironmentVariable("MSICONTROLLER_BUILTIN_DEVICE_CONFIG", QString::fromLatin1(builtinConfigPath));
}

QString SupportConfigRepository::userPath() const {
    return qEnvironmentVariable("MSICONTROLLER_USER_DEVICE_CONFIG", QString::fromLatin1(userConfigPath));
}

bool SupportConfigRepository::loadFile(const QString& path,
                                       const QString& source,
                                       QList<Profile>* profiles,
                                       QVariantMap* defaults,
                                       QString* errorMessage) const {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Unable to open device config %1: %2").arg(path, file.errorString());
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Invalid device config %1: %2").arg(path, parseError.errorString());
        }
        return false;
    }

    const QJsonObject root = document.object();
    if (defaults && root.value(QStringLiteral("defaults")).isObject()) {
        *defaults = objectToVariantMap(root.value(QStringLiteral("defaults")).toObject());
    }

    const QJsonArray profileArray = root.value(QStringLiteral("profiles")).toArray();
    for (const QJsonValue& value : profileArray) {
        if (!value.isObject()) {
            continue;
        }

        const QJsonObject object = value.toObject();
        const QString id = normalizeId(object.value(QStringLiteral("id")).toString());
        const QJsonObject valuesObject = object.value(QStringLiteral("values")).toObject();
        if (id.isEmpty() || valuesObject.isEmpty()) {
            qWarning() << "Skipping invalid device profile in" << path << object;
            continue;
        }
        profiles->append(Profile{id, source, sanitizeValues(objectToVariantMap(valuesObject))});
    }

    return true;
}

bool SupportConfigRepository::writeUserProfiles(QString* errorMessage) const {
    const QFileInfo fileInfo(userPath());
    QDir directory(fileInfo.absolutePath());
    if (!directory.exists() && !directory.mkpath(QStringLiteral("."))) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Unable to create %1").arg(directory.absolutePath());
        return false;
    }

    QJsonArray profiles;
    for (const Profile& profile : mUserProfiles) {
        QJsonObject object;
        object.insert(QStringLiteral("id"), profile.id);
        object.insert(QStringLiteral("values"), variantMapToObject(profile.values));
        profiles.append(object);
    }

    QJsonObject root;
    root.insert(QStringLiteral("schemaVersion"), 1);
    root.insert(QStringLiteral("profiles"), profiles);

    QSaveFile file(userPath());
    if (!file.open(QIODevice::WriteOnly)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Unable to write %1: %2").arg(userPath(), file.errorString());
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Unable to commit %1: %2").arg(userPath(), file.errorString());
        return false;
    }
    return true;
}

QVariantMap SupportConfigRepository::profileForDbus(const Profile& profile) const {
    QVariantMap result;
    result.insert(QStringLiteral("id"), profile.id);
    result.insert(QStringLiteral("source"), profile.source);
    result.insert(QStringLiteral("builtin"), profile.source == QStringLiteral("builtin"));
    result.insert(QStringLiteral("user"), profile.source == QStringLiteral("user"));
    result.insert(QStringLiteral("firmware"), firmwareList(profile.values));
    result.insert(QStringLiteral("values"), profile.values);
    return result;
}

QStringList SupportConfigRepository::firmwareList(const QVariantMap& values) {
    const QVariant value = values.value(QStringLiteral("AllowedFw"));
    if (value.typeId() == QMetaType::QStringList) {
        return value.toStringList();
    }

    if (value.typeId() == QMetaType::QString) {
        const QString firmware = value.toString().trimmed();
        return firmware.isEmpty() ? QStringList{} : QStringList{firmware};
    }

    QStringList firmwares;
    for (const QVariant& item : value.toList()) {
        const QString firmware = item.toString().trimmed();
        if (!firmware.isEmpty()) {
            firmwares.append(firmware);
        }
    }
    const QString singleFirmware = value.toString().trimmed();
    if (firmwares.isEmpty() && !singleFirmware.isEmpty()) {
        firmwares.append(singleFirmware);
    }
    return firmwares;
}

QString SupportConfigRepository::normalizeId(const QString& id) {
    QString normalized = id.trimmed();
    normalized.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_.-]")), QStringLiteral("_"));
    return normalized;
}

QVariantMap SupportConfigRepository::sanitizeValues(const QVariantMap& values) {
    QVariantMap sanitized;
    for (auto it = values.cbegin(); it != values.cend(); ++it) {
        if (it.key().isEmpty()) {
            continue;
        }

        if (it.value().typeId() == QMetaType::QVariantList) {
            QStringList list;
            const QVariantList values = it.value().toList();
            for (const QVariant& item : values) {
                const QString text = item.toString().trimmed();
                if (!text.isEmpty()) {
                    list.append(text);
                }
            }
            sanitized.insert(it.key(), list);
        } else {
            sanitized.insert(it.key(), it.value());
        }
    }
    return sanitized;
}
