#include "behaviorprofilecontroller.h"

#include <QSettings>
#include <algorithm>

#include "autoprofilecontroller.h"
#include "parameterclient.h"
#include "proxyparameter.h"

namespace {
constexpr auto settingsGroup = "BehaviorProfile";
constexpr auto activeKey = "Active";

template <typename T>
bool availableContains(const QVariant& available, T value) {
    if (!available.isValid()) {
        return true;
    }
    const auto list = available.value<QList<T>>();
    if (!list.isEmpty()) {
        return list.contains(value);
    }
    const auto vector = available.value<QVector<T>>();
    return vector.isEmpty() || vector.contains(value);
}
} // namespace

BehaviorProfileController::BehaviorProfileController(ParameterClient* parameters,
                                                     AutoProfileController* autoProfile,
                                                     QObject* parent)
    : QObject(parent), mParameters(parameters), mAutoProfile(autoProfile) {
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(settingsGroup));
    mActiveProfile = settings.value(QString::fromLatin1(activeKey), QStringLiteral("balanced")).toString();
    settings.endGroup();
    setStatus(tr("Behavior profile ready"));
}

QVariantList BehaviorProfileController::profiles() const {
    QVariantList result;
    for (const ProfileDefinition& profile : definitions()) {
        QVariantMap item;
        item.insert(QStringLiteral("id"), profile.id);
        item.insert(QStringLiteral("title"), profile.title);
        item.insert(QStringLiteral("subtitle"), profile.subtitle);
        result.append(item);
    }
    return result;
}

QString BehaviorProfileController::activeProfile() const {
    return mActiveProfile;
}

QString BehaviorProfileController::status() const {
    return mStatus;
}

void BehaviorProfileController::applyProfile(const QString& id) {
    const QVector<ProfileDefinition> availableProfiles = definitions();
    const auto profileIt = std::find_if(availableProfiles.cbegin(),
                                        availableProfiles.cend(),
                                        [&id](const ProfileDefinition& p) { return p.id == id; });
    if (profileIt == availableProfiles.cend()) {
        setStatus(tr("Unknown behavior profile"));
        return;
    }

    if (mAutoProfile) {
        mAutoProfile->setEnabled(profileIt->enableAutoProfile);
    }

    const bool shiftOk = applyFirstAvailableShiftMode(profileIt->shiftModes);
    const bool fanOk = applyFirstAvailableFanMode(profileIt->fanModes);
    if (!shiftOk && !fanOk && !profileIt->enableAutoProfile) {
        setStatus(tr("No matching firmware controls for this profile"));
        return;
    }

    if (mActiveProfile != profileIt->id) {
        mActiveProfile = profileIt->id;
        QSettings settings;
        settings.beginGroup(QString::fromLatin1(settingsGroup));
        settings.setValue(QString::fromLatin1(activeKey), mActiveProfile);
        settings.endGroup();
        emit activeProfileChanged();
    }
    setStatus(tr("%1 profile applied").arg(profileIt->title));
}

QVector<BehaviorProfileController::ProfileDefinition> BehaviorProfileController::definitions() const {
    return {
        {QStringLiteral("silent"),
         tr("Silent"),
         tr("Battery-friendly cooling"),
         {Msi::ShiftMode::Eco},
         {Msi::FanMode::Silent, Msi::FanMode::Auto},
         false},
        {QStringLiteral("balanced"),
         tr("Balanced"),
         tr("Firmware-managed daily mode"),
         {Msi::ShiftMode::Comfort, Msi::ShiftMode::Sport},
         {Msi::FanMode::Auto, Msi::FanMode::Basic},
         false},
        {QStringLiteral("performance"),
         tr("Performance"),
         tr("Higher thermal headroom"),
         {Msi::ShiftMode::Turbo, Msi::ShiftMode::Sport},
         {Msi::FanMode::Advanced, Msi::FanMode::Basic},
         false},
        {QStringLiteral("adaptive"), tr("Adaptive"), tr("Automatic battery/thermal switching"), {}, {}, true},
    };
}

ProxyParameter* BehaviorProfileController::parameter(Msi::Parametr name) const {
    return mParameters ? mParameters->getProxyParameter(name) : nullptr;
}

void BehaviorProfileController::setStatus(const QString& status) {
    if (mStatus == status) {
        return;
    }
    mStatus = status;
    emit statusChanged();
}

bool BehaviorProfileController::applyFirstAvailableShiftMode(const QVector<Msi::ShiftMode>& modes) {
    ProxyParameter* shift = parameter(Msi::Parametr::ShiftModeEc);
    if (!shift || !shift->isValid() || modes.isEmpty()) {
        return false;
    }
    for (Msi::ShiftMode mode : modes) {
        if (availableContains(shift->availableValues(), mode)) {
            shift->setValue(QVariant::fromValue(mode));
            return true;
        }
    }
    return false;
}

bool BehaviorProfileController::applyFirstAvailableFanMode(const QVector<Msi::FanMode>& modes) {
    ProxyParameter* fan = parameter(Msi::Parametr::FanModeEc);
    if (!fan || !fan->isValid() || modes.isEmpty()) {
        return false;
    }
    for (Msi::FanMode mode : modes) {
        if (availableContains(fan->availableValues(), mode)) {
            fan->setValue(QVariant::fromValue(mode));
            return true;
        }
    }
    return false;
}
