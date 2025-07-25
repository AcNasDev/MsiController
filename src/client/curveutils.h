#pragma once

#include <QObject>
#include <QVector>
#include <QPointF>
#include <QVariant>

class CurveUtils : public QObject {
    Q_OBJECT
public:
    Q_INVOKABLE QVariantList catmullRomSpline(const QVariantList &xValues, const QVariantList &yValues, int segments);
};