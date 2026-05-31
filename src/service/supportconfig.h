#pragma once

#include <QMap>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantList>

class SupportConfigRepository : public QObject {
    Q_OBJECT
public:
    explicit SupportConfigRepository(QObject* parent = nullptr);

    bool reload(QString* errorMessage = nullptr);
    QMap<QString, QVariant> configForFirmware(const QString& firmwareVersion);
    QVariantList profilesForDbus() const;
    QVariantMap activeProfileForDbus() const;

    bool saveUserProfile(const QVariantMap& profile, QString* errorMessage = nullptr);
    bool removeUserProfile(const QString& profileId, QString* errorMessage = nullptr);

signals:
    void profilesChanged();

private:
    struct Profile {
        QString id;
        QString source;
        QVariantMap values;
    };

    QString builtinPath() const;
    QString userPath() const;
    bool loadFile(const QString& path,
                  const QString& source,
                  QList<Profile>* profiles,
                  QVariantMap* defaults,
                  QString* errorMessage) const;
    bool writeUserProfiles(QString* errorMessage) const;
    QVariantMap profileForDbus(const Profile& profile) const;
    static QStringList firmwareList(const QVariantMap& values);
    static QString normalizeId(const QString& id);
    static QVariantMap sanitizeValues(const QVariantMap& values);

    QVariantMap mDefaults;
    QList<Profile> mBuiltinProfiles;
    QList<Profile> mUserProfiles;
    QString mActiveFirmware;
    QString mActiveProfileId;
    QString mActiveProfileSource;
};
