#include "parameter.h"

#include <QCoreApplication>
#include <QTimer>

#include "settingsstore.h"

namespace {
constexpr auto parameterSettingsGroup = "Parameters";
} // namespace

Parameter::Parameter(const QVariant& name,
                     const QVariant& available,
                     bool isReadOnly,
                     QObject* parent,
                     SettingsStore* settingsStore,
                     Persistence persistence)
    : QObject(parent), mName(name), mAvailable(available), mIsReadOnly(isReadOnly),
      mSettingsStore(settingsStore ? settingsStore : &defaultSettingsStore()), mPersistence(persistence) {
    QTimer::singleShot(0, this, [this]() {
        if (mPersistence != Persistence::Persistent || !mSettingsStore) {
            return;
        }
        if (mSettingsStore->contains(QString::fromLatin1(parameterSettingsGroup), mName.toString())) {
            setValue(mSettingsStore->value(QString::fromLatin1(parameterSettingsGroup), mName.toString()));
        }
    });
}

void Parameter::setValue(const QVariant& value) {
    if (value == mValue)
        return;
    if (mIsReadOnly)
        return;

    if (qEnvironmentVariableIsSet("MSICONTROLLER_DEBUG_WRITES")) {
        qDebug() << "Setting value for parameter:" << mName << "to" << value;
    }

    bool success{writeValue(value)};
    if (success) {
        if (mPersistence == Persistence::Persistent && mSettingsStore) {
            mSettingsStore->setValue(QString::fromLatin1(parameterSettingsGroup), mName.toString(), value);
        }
        if (qEnvironmentVariableIsSet("MSICONTROLLER_DEBUG_WRITES")) {
            qDebug() << "Value set for parameter:" << mName;
        }
        update();
    } else {
        qWarning() << "Failed to set value for parameter:" << mName;
    }
}

QVariant Parameter::value() const {
    return mValue;
}

QVariant Parameter::name() const {
    return mName;
}

QVariant Parameter::available() const {
    return mAvailable;
}

bool Parameter::isReadOnly() const {
    return mIsReadOnly;
}

void Parameter::update() {
    auto newValue = readValue();
    publishValue(newValue);
}

bool Parameter::publishValue(const QVariant& value) {
    if (value == mValue)
        return false;

    mValue = value;
    emit valueChanged(mValue);
    return true;
}
