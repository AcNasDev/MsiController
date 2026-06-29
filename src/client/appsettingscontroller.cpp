#include "appsettingscontroller.h"

#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSettings>
#include <QUrl>

namespace {
constexpr int currentSettingsSchemaVersion = 2;
constexpr auto schemaGroup = "SettingsSchema";
constexpr auto schemaVersionKey = "Version";

QString appVersion() {
    return QString(CMAKE_TOOLS_GIT_TAG_MAJOR) + QStringLiteral(".") + QString(CMAKE_TOOLS_GIT_TAG_MINOR) +
           QStringLiteral(".") + QString(CMAKE_TOOLS_GIT_TAG_PATCH);
}

QVariantMap resultMap(bool ok, const QString& error = {}, const QVariantMap& extra = {}) {
    QVariantMap result = extra;
    result.insert(QStringLiteral("ok"), ok);
    result.insert(QStringLiteral("error"), error);
    return result;
}
} // namespace

AppSettingsController::AppSettingsController(QObject* parent) : QObject(parent) {
    migrate();
}

QString AppSettingsController::status() const {
    return mStatus;
}

QVariantMap AppSettingsController::snapshot() const {
    QSettings settings;
    QVariantMap values;
    for (const QString& key : settings.allKeys()) {
        values.insert(key, settings.value(key));
    }

    QVariantMap result;
    result.insert(QStringLiteral("schemaVersion"), currentSettingsSchemaVersion);
    result.insert(QStringLiteral("appVersion"), appVersion());
    result.insert(QStringLiteral("generatedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    result.insert(QStringLiteral("settings"), values);
    return result;
}

QVariantMap AppSettingsController::exportSettings(const QString& pathOrUrl) {
    const QString path = normalizeFilePath(pathOrUrl);
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        const QString error = file.errorString();
        setStatus(error);
        return resultMap(false, error);
    }

    file.write(QJsonDocument::fromVariant(snapshot()).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        const QString error = file.errorString();
        setStatus(error);
        return resultMap(false, error);
    }

    setStatus(tr("Application settings exported"));
    return resultMap(true, {}, {{QStringLiteral("path"), path}});
}

QVariantMap AppSettingsController::importSettings(const QString& pathOrUrl) {
    QFile file(normalizeFilePath(pathOrUrl));
    if (!file.open(QIODevice::ReadOnly)) {
        const QString error = file.errorString();
        setStatus(error);
        return resultMap(false, error);
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        const QString error = tr("Invalid settings JSON: %1").arg(parseError.errorString());
        setStatus(error);
        return resultMap(false, error);
    }

    const QVariantMap root = document.object().toVariantMap();
    const QVariantMap values = root.value(QStringLiteral("settings")).toMap();
    if (values.isEmpty()) {
        const QString error = tr("Settings JSON has no settings object");
        setStatus(error);
        return resultMap(false, error);
    }

    QSettings settings;
    for (auto it = values.cbegin(); it != values.cend(); ++it) {
        settings.setValue(it.key(), it.value());
    }
    settings.sync();
    migrate();
    setStatus(tr("Application settings imported"));
    return resultMap(true, {}, {{QStringLiteral("count"), values.size()}});
}

void AppSettingsController::migrate() {
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(schemaGroup));
    const int previousVersion = settings.value(QString::fromLatin1(schemaVersionKey), 0).toInt();
    if (previousVersion < currentSettingsSchemaVersion) {
        settings.setValue(QString::fromLatin1(schemaVersionKey), currentSettingsSchemaVersion);
        settings.setValue(QStringLiteral("MigratedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    }
    settings.endGroup();

    setStatus(previousVersion < currentSettingsSchemaVersion ? tr("Application settings migrated")
                                                             : tr("Application settings ready"));
}

void AppSettingsController::setStatus(const QString& status) {
    if (mStatus == status) {
        return;
    }
    mStatus = status;
    emit statusChanged();
}

QString AppSettingsController::normalizeFilePath(const QString& pathOrUrl) const {
    const QUrl url(pathOrUrl);
    if (url.isValid() && url.isLocalFile()) {
        return url.toLocalFile();
    }
    return pathOrUrl;
}
