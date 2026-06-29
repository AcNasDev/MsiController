#pragma once

#include <QObject>
#include <QVariantMap>

class AppSettingsController : public QObject {
    Q_OBJECT
public:
    explicit AppSettingsController(QObject* parent = nullptr);

    QString status() const;
    QVariantMap snapshot() const;
    QVariantMap exportSettings(const QString& pathOrUrl);
    QVariantMap importSettings(const QString& pathOrUrl);

signals:
    void statusChanged();

private:
    QString mStatus;

    void migrate();
    void setStatus(const QString& status);
    QString normalizeFilePath(const QString& pathOrUrl) const;
};
