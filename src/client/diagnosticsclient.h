#pragma once

#include <QDBusConnection>
#include <QObject>
#include <QTimer>
#include <QVariantMap>

class ComMsiEcHealthInterface;

class DiagnosticsClient : public QObject {
    Q_OBJECT
public:
    explicit DiagnosticsClient(const QDBusConnection& connection, QObject* parent = nullptr);

    void setConnected(bool connected);
    QVariantMap diagnostics() const;
    QString status() const;
    bool restartRequired() const;

    void refresh();
    QVariantMap saveSupportBundle(const QString& pathOrUrl, const QVariantMap& clientData = {});

signals:
    void diagnosticsChanged();
    void statusChanged();

private:
    ComMsiEcHealthInterface* mHealthInterface{nullptr};
    QTimer mRefreshTimer;
    bool mIsConnected{false};
    QVariantMap mDiagnostics;
    QString mStatus;

    void setStatus(const QString& status);
    QString normalizeOutputPath(const QString& pathOrUrl) const;
};
