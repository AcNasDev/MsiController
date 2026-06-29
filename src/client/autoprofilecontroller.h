#pragma once

#include <QObject>
#include <QTimer>
#include <optional>

#include "struct.h"

class ParameterClient;
class ProxyParameter;

class AutoProfileController : public QObject {
    Q_OBJECT
public:
    explicit AutoProfileController(ParameterClient* parameters, QObject* parent = nullptr);

    bool isEnabled() const;
    QString status() const;
    void setEnabled(bool enabled);

signals:
    void enabledChanged();
    void statusChanged();

private:
    ParameterClient* mParameters{nullptr};
    QTimer mTimer;
    bool mEnabled{false};
    QString mStatus;

    void evaluate();
    void setStatus(const QString& status);
    ProxyParameter* parameter(Msi::Parametr name) const;
    std::optional<Msi::ShiftMode> desiredMode(QString* reason) const;
    bool shiftModeAvailable(Msi::ShiftMode mode) const;
};
