#pragma once

#include <QObject>
#include <QVariantList>

#include "struct.h"

class AutoProfileController;
class ParameterClient;
class ProxyParameter;

class BehaviorProfileController : public QObject {
    Q_OBJECT
public:
    BehaviorProfileController(ParameterClient* parameters,
                              AutoProfileController* autoProfile,
                              QObject* parent = nullptr);

    QVariantList profiles() const;
    QString activeProfile() const;
    QString status() const;
    void applyProfile(const QString& id);

signals:
    void profilesChanged();
    void activeProfileChanged();
    void statusChanged();

private:
    struct ProfileDefinition {
        QString id;
        QString title;
        QString subtitle;
        QVector<Msi::ShiftMode> shiftModes;
        QVector<Msi::FanMode> fanModes;
        bool enableAutoProfile{false};
    };

    ParameterClient* mParameters{nullptr};
    AutoProfileController* mAutoProfile{nullptr};
    QString mActiveProfile;
    QString mStatus;

    QVector<ProfileDefinition> definitions() const;
    ProxyParameter* parameter(Msi::Parametr name) const;
    void setStatus(const QString& status);
    bool applyFirstAvailableShiftMode(const QVector<Msi::ShiftMode>& modes);
    bool applyFirstAvailableFanMode(const QVector<Msi::FanMode>& modes);
};
