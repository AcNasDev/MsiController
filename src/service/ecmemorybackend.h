#pragma once

#include <QByteArray>
#include <QMap>
#include <QObject>
#include <QVariantMap>
#include <optional>

class EcMemoryBackend : public QObject {
    Q_OBJECT
public:
    explicit EcMemoryBackend(QObject* parent = nullptr);
    ~EcMemoryBackend() override = default;

    virtual std::optional<QByteArray> readAll(const QMap<uint, QByteArray>& pendingWrites) = 0;
    virtual QString displayName() const = 0;
    virtual bool isSimulator() const;
    virtual QVariantMap diagnostics() const;
};

class FileEcMemoryBackend : public EcMemoryBackend {
    Q_OBJECT
public:
    explicit FileEcMemoryBackend(QString fileName, QObject* parent = nullptr);

    std::optional<QByteArray> readAll(const QMap<uint, QByteArray>& pendingWrites) override;
    QString displayName() const override;
    QVariantMap diagnostics() const override;

private:
    QString mFileName;
};

class SimulatedEcMemoryBackend : public EcMemoryBackend {
    Q_OBJECT
public:
    explicit SimulatedEcMemoryBackend(QObject* parent = nullptr);

    std::optional<QByteArray> readAll(const QMap<uint, QByteArray>& pendingWrites) override;
    QString displayName() const override;
    bool isSimulator() const override;
    QVariantMap diagnostics() const override;

private:
    QByteArray mMemory;
    int mTick{0};

    void applyWrites(const QMap<uint, QByteArray>& pendingWrites);
    void refreshTelemetry();
    void writeAscii(int offset, int length, const QByteArray& text);
    void writeByte(int offset, quint8 value);
};
