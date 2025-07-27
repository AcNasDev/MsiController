#pragma once

#include <QObject>
#include <QPointF>
#include <QVariant>
#include <QVector>

class CurveUtils : public QObject {
    Q_OBJECT
public:
    Q_INVOKABLE QVariantList catmullRomSpline(const QVariantList& xValues, const QVariantList& yValues, int segments);
};