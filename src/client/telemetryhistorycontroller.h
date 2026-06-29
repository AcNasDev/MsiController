#pragma once

#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

#include "struct.h"

class ParameterClient;
class ProxyParameter;

class TelemetryHistoryController : public QObject {
    Q_OBJECT
public:
    explicit TelemetryHistoryController(ParameterClient* parameters, QObject* parent = nullptr);

    QVariantList history() const;
    QVariantMap snapshot() const;
    QVariantMap exportHistory(const QString& pathOrUrl) const;

signals:
    void historyChanged();

private:
    ParameterClient* mParameters{nullptr};
    QTimer mTimer;
    QVariantList mHistory;

    ProxyParameter* parameter(Msi::Parametr name) const;
    void sample();
    QVariant valueFor(Msi::Parametr name) const;
    QString normalizeFilePath(const QString& pathOrUrl) const;
};
