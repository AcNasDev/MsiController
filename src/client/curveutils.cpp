#include "curveutils.h"

#include <algorithm>
#include <QPointF>
#include <QVariant>

QVariantList CurveUtils::catmullRomSpline(const QVariantList& xValues, const QVariantList& yValues, int segments) {
    QVariantList result;
    const qsizetype pointCount = std::min(xValues.size(), yValues.size());
    if (pointCount == 0) {
        return result;
    }

    const int segmentCount = std::max(1, segments);
    for (qsizetype i = 0; i < pointCount - 1; ++i) {
        const double x0 = i > 0 ? xValues[i - 1].toDouble() : xValues[i].toDouble();
        const double x1 = xValues[i].toDouble();
        const double x2 = xValues[i + 1].toDouble();
        const double x3 = (i < pointCount - 2) ? xValues[i + 2].toDouble() : xValues[i + 1].toDouble();

        const double y0 = i > 0 ? yValues[i - 1].toDouble() : yValues[i].toDouble();
        const double y1 = yValues[i].toDouble();
        const double y2 = yValues[i + 1].toDouble();
        const double y3 = (i < pointCount - 2) ? yValues[i + 2].toDouble() : yValues[i + 1].toDouble();

        for (int t = 0; t < segmentCount; ++t) {
            const double s = static_cast<double>(t) / static_cast<double>(segmentCount);
            const double x = 0.5 * ((2.0 * x1) + (-x0 + x2) * s + (2.0 * x0 - 5.0 * x1 + 4.0 * x2 - x3) * s * s +
                                    (-x0 + 3.0 * x1 - 3.0 * x2 + x3) * s * s * s);
            const double y = 0.5 * ((2.0 * y1) + (-y0 + y2) * s + (2.0 * y0 - 5.0 * y1 + 4.0 * y2 - y3) * s * s +
                                    (-y0 + 3.0 * y1 - 3.0 * y2 + y3) * s * s * s);
            result << QVariant::fromValue(QPointF(x, y));
        }
    }
    result << QVariant::fromValue(QPointF(xValues[pointCount - 1].toDouble(), yValues[pointCount - 1].toDouble()));
    return result;
}
