#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QMap>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

class EcWritePolicy : public QObject {
    Q_OBJECT
public:
    struct Range {
        int offset{0};
        int length{0};
        QString label;
    };

    explicit EcWritePolicy(QObject* parent = nullptr);

    void setAllowedRanges(const QVector<Range>& ranges);
    QVector<Range> allowedRanges() const;
    QVariantList allowedRangesForDbus() const;
    QVariantList auditLogForDbus() const;
    QVariantMap diagnostics() const;

    bool allows(int offset, int length, QString* errorMessage = nullptr) const;
    void recordWrite(const QString& operation,
                     int offset,
                     const QByteArray& bytes,
                     bool ok,
                     const QString& errorMessage = {});

    static QVector<Range> rangesFromProfileConfig(const QMap<QString, QVariant>& config);

private:
    QVector<Range> mAllowedRanges;
    QVariantList mAuditLog;

    bool unsafeOverrideEnabled() const;
};
