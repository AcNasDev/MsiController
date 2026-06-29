#pragma once

#include <QString>
#include <QVariant>

class SettingsStore {
public:
    virtual ~SettingsStore() = default;

    virtual bool contains(const QString& group, const QString& key) const = 0;
    virtual QVariant value(const QString& group, const QString& key, const QVariant& fallback = {}) const = 0;
    virtual void setValue(const QString& group, const QString& key, const QVariant& value) = 0;
};

class QtSettingsStore : public SettingsStore {
public:
    explicit QtSettingsStore(QString filePath = QStringLiteral("/etc/MsiController/settings.ini"));

    bool contains(const QString& group, const QString& key) const override;
    QVariant value(const QString& group, const QString& key, const QVariant& fallback = {}) const override;
    void setValue(const QString& group, const QString& key, const QVariant& value) override;

private:
    QString mFilePath;
};

SettingsStore& defaultSettingsStore();
