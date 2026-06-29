#pragma once

#include <QObject>
#include <QVariant>

class SettingsStore;

class Parameter : public QObject {
    Q_OBJECT
public:
    enum class Persistence { Persistent, Volatile };

    explicit Parameter(const QVariant& name,
                       const QVariant& available,
                       bool isReadOnly,
                       QObject* parent = nullptr,
                       SettingsStore* settingsStore = nullptr,
                       Persistence persistence = Persistence::Persistent);
    ~Parameter() override = default;

    virtual QVariant value() const;
    virtual void setValue(const QVariant& value);
    virtual QVariant name() const;
    virtual QVariant available() const;
    virtual bool isReadOnly() const;
    virtual void update();

signals:
    void valueChanged(const QVariant& newValue);

protected:
    virtual QVariant readValue() const = 0;
    virtual bool writeValue(const QVariant& value) = 0;
    bool publishValue(const QVariant& value);

private:
    QVariant mName;
    QVariant mAvailable;
    bool mIsReadOnly{false};
    QVariant mValue;
    SettingsStore* mSettingsStore{nullptr};
    Persistence mPersistence{Persistence::Persistent};
};
