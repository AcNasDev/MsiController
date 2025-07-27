#include "curveutils.h"

#include <QPointF>
#include <QVariant>

QVariantList CurveUtils::catmullRomSpline(const QVariantList& xValues, const QVariantList& yValues, int segments) {
    QVariantList result;
    int n = xValues.size();
    for (int i = 0; i < n - 1; ++i) {
        double x0 = i > 0 ? xValues[i - 1].toDouble() : xValues[i].toDouble();
        double x1 = xValues[i].toDouble();
        double x2 = xValues[i + 1].toDouble();
        double x3 = (i < n - 2) ? xValues[i + 2].toDouble() : xValues[i + 1].toDouble();

        double y0 = i > 0 ? yValues[i - 1].toDouble() : yValues[i].toDouble();
        double y1 = yValues[i].toDouble();
        double y2 = yValues[i + 1].toDouble();
        double y3 = (i < n - 2) ? yValues[i + 2].toDouble() : yValues[i + 1].toDouble();

        for (int t = 0; t < segments; ++t) {
            double s = double(t) / segments;
            double x = 0.5 * ((2 * x1) + (-x0 + x2) * s + (2 * x0 - 5 * x1 + 4 * x2 - x3) * s * s +
                              (-x0 + 3 * x1 - 3 * x2 + x3) * s * s * s);
            double y = 0.5 * ((2 * y1) + (-y0 + y2) * s + (2 * y0 - 5 * y1 + 4 * y2 - y3) * s * s +
                              (-y0 + 3 * y1 - 3 * y2 + y3) * s * s * s);
            result << QVariant::fromValue(QPointF(x, y));
        }
    }
    result << QVariant::fromValue(QPointF(xValues[n - 1].toDouble(), yValues[n - 1].toDouble()));
    return result;
}