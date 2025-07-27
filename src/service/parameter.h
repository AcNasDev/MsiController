#pragma once

#include <QObject>
#include <QVariant>

class Parameter : public QObject {
    Q_OBJECT
public:
    explicit Parameter(const QVariant& name,
                       const QVariant& available,
                       const bool& isReadOnly,
                       QObject* parent = nullptr);
    ~Parameter() = default;

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

private:
    QVariant mName;
    QVariant mAvailable;
    bool mIsReadOnly{false};
    QVariant mValue;
};
