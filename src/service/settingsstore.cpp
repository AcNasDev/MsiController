#include "settingsstore.h"

#include <QSettings>
#include <utility>

QtSettingsStore::QtSettingsStore(QString filePath) : mFilePath(std::move(filePath)) {}

bool QtSettingsStore::contains(const QString& group, const QString& key) const {
    QSettings settings(mFilePath, QSettings::IniFormat);
    settings.beginGroup(group);
    const bool result = settings.contains(key);
    settings.endGroup();
    return result;
}

QVariant QtSettingsStore::value(const QString& group, const QString& key, const QVariant& fallback) const {
    QSettings settings(mFilePath, QSettings::IniFormat);
    settings.beginGroup(group);
    const QVariant result = settings.value(key, fallback);
    settings.endGroup();
    return result;
}

void QtSettingsStore::setValue(const QString& group, const QString& key, const QVariant& value) {
    QSettings settings(mFilePath, QSettings::IniFormat);
    settings.beginGroup(group);
    settings.setValue(key, value);
    settings.endGroup();
}

SettingsStore& defaultSettingsStore() {
    static QtSettingsStore store;
    return store;
}
