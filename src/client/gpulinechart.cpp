#include "gpulinechart.h"

#include <QSGFlatColorMaterial>
#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGNode>
#include <QSGVertexColorMaterial>
#include <QVector2D>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {
bool almostEqual(qreal left, qreal right) {
    return std::abs(left - right) <= std::numeric_limits<qreal>::epsilon() * 16.0;
}

struct StrokeSection {
    QPointF innerLeft;
    QPointF innerRight;
    QPointF outerLeft;
    QPointF outerRight;
};

qreal clamp01(qreal value) {
    return std::clamp(value, 0.0, 1.0);
}

QSGFlatColorMaterial* materialFor(const QColor& color) {
    auto* material = new QSGFlatColorMaterial;
    material->setColor(color);
    return material;
}

void setColoredVertex(QSGGeometry::ColoredPoint2D& vertex, const QPointF& point, const QColor& color, qreal alphaFactor) {
    const qreal alpha = std::clamp<qreal>(color.alphaF() * alphaFactor, 0.0, 1.0);
    vertex.set(float(point.x()),
               float(point.y()),
               uchar(std::clamp<qreal>(color.redF() * alpha, 0.0, 1.0) * 255.0),
               uchar(std::clamp<qreal>(color.greenF() * alpha, 0.0, 1.0) * 255.0),
               uchar(std::clamp<qreal>(color.blueF() * alpha, 0.0, 1.0) * 255.0),
               uchar(alpha * 255.0));
}

void appendColoredTriangle(QSGGeometry::ColoredPoint2D* vertices,
                           int& index,
                           const QPointF& a,
                           qreal alphaA,
                           const QPointF& b,
                           qreal alphaB,
                           const QPointF& c,
                           qreal alphaC,
                           const QColor& color) {
    setColoredVertex(vertices[index++], a, color, alphaA);
    setColoredVertex(vertices[index++], b, color, alphaB);
    setColoredVertex(vertices[index++], c, color, alphaC);
}

QVector2D normalizedDirection(const QPointF& from, const QPointF& to) {
    QVector2D direction(to - from);
    if (direction.lengthSquared() <= 0.0001f) {
        return {};
    }
    return direction.normalized();
}

QPointF miterOffset(const QVector2D& previousDirection, const QVector2D& nextDirection, qreal halfWidth) {
    QVector2D previousNormal(-previousDirection.y(), previousDirection.x());
    QVector2D nextNormal(-nextDirection.y(), nextDirection.x());
    QVector2D miter = previousNormal + nextNormal;
    if (miter.lengthSquared() <= 0.0001f) {
        miter = nextNormal.lengthSquared() > 0.0001f ? nextNormal : previousNormal;
    } else {
        miter.normalize();
    }

    const qreal denominator = QVector2D::dotProduct(miter, nextNormal);
    qreal length = halfWidth;
    if (std::abs(denominator) > 0.2) {
        length = halfWidth / denominator;
    }
    length = std::clamp(length, -halfWidth * 2.0, halfWidth * 2.0);
    return (miter * float(length)).toPointF();
}

QVector<QPointF> removeDuplicatePoints(const QVector<QPointF>& points) {
    QVector<QPointF> result;
    result.reserve(points.size());
    for (const QPointF& point : points) {
        if (result.isEmpty() || QVector2D(point - result.constLast()).lengthSquared() > 0.0001f) {
            result.push_back(point);
        }
    }
    return result;
}

QVector<StrokeSection> strokeSections(const QVector<QPointF>& points, qreal innerHalfWidth, qreal outerHalfWidth) {
    QVector<StrokeSection> result;
    result.reserve(points.size());

    for (int i = 0; i < points.size(); ++i) {
        QVector2D previousDirection;
        QVector2D nextDirection;
        if (i > 0) {
            previousDirection = normalizedDirection(points[i - 1], points[i]);
        }
        if (i + 1 < points.size()) {
            nextDirection = normalizedDirection(points[i], points[i + 1]);
        }
        if (previousDirection.lengthSquared() <= 0.0001f) {
            previousDirection = nextDirection;
        }
        if (nextDirection.lengthSquared() <= 0.0001f) {
            nextDirection = previousDirection;
        }

        const QPointF innerOffset = miterOffset(previousDirection, nextDirection, innerHalfWidth);
        const QPointF outerOffset = miterOffset(previousDirection, nextDirection, outerHalfWidth);
        result.push_back({points[i] + innerOffset, points[i] - innerOffset, points[i] + outerOffset, points[i] - outerOffset});
    }

    return result;
}
} // namespace

GpuLineChart::GpuLineChart(QQuickItem* parent)
    : QQuickItem(parent) {
    setFlag(ItemHasContents, true);
    setAntialiasing(true);
}

QVariantList GpuLineChart::values() const {
    return m_values;
}

void GpuLineChart::setValues(const QVariantList& values) {
    if (m_values == values) {
        return;
    }
    m_values = values;
    emit valuesChanged();
    scheduleUpdate();
}

QVariantList GpuLineChart::xValues() const {
    return m_xValues;
}

void GpuLineChart::setXValues(const QVariantList& values) {
    if (m_xValues == values) {
        return;
    }
    m_xValues = values;
    emit xValuesChanged();
    scheduleUpdate();
}

qreal GpuLineChart::minValue() const {
    return m_minValue;
}

void GpuLineChart::setMinValue(qreal value) {
    if (almostEqual(m_minValue, value)) {
        return;
    }
    m_minValue = value;
    emit rangeChanged();
    scheduleUpdate();
}

qreal GpuLineChart::maxValue() const {
    return m_maxValue;
}

void GpuLineChart::setMaxValue(qreal value) {
    if (almostEqual(m_maxValue, value)) {
        return;
    }
    m_maxValue = value;
    emit rangeChanged();
    scheduleUpdate();
}

qreal GpuLineChart::minX() const {
    return m_minX;
}

void GpuLineChart::setMinX(qreal value) {
    if (almostEqual(m_minX, value)) {
        return;
    }
    m_minX = value;
    emit xRangeChanged();
    scheduleUpdate();
}

qreal GpuLineChart::maxX() const {
    return m_maxX;
}

void GpuLineChart::setMaxX(qreal value) {
    if (almostEqual(m_maxX, value)) {
        return;
    }
    m_maxX = value;
    emit xRangeChanged();
    scheduleUpdate();
}

QColor GpuLineChart::lineColor() const {
    return m_lineColor;
}

void GpuLineChart::setLineColor(const QColor& color) {
    if (m_lineColor == color) {
        return;
    }
    m_lineColor = color;
    emit colorsChanged();
    scheduleUpdate();
}

QColor GpuLineChart::fillColor() const {
    return m_fillColor;
}

void GpuLineChart::setFillColor(const QColor& color) {
    if (m_fillColor == color) {
        return;
    }
    m_fillColor = color;
    emit colorsChanged();
    scheduleUpdate();
}

QColor GpuLineChart::gridColor() const {
    return m_gridColor;
}

void GpuLineChart::setGridColor(const QColor& color) {
    if (m_gridColor == color) {
        return;
    }
    m_gridColor = color;
    emit colorsChanged();
    scheduleUpdate();
}

bool GpuLineChart::fillVisible() const {
    return m_fillVisible;
}

void GpuLineChart::setFillVisible(bool visible) {
    if (m_fillVisible == visible) {
        return;
    }
    m_fillVisible = visible;
    emit appearanceChanged();
    scheduleUpdate();
}

bool GpuLineChart::showGrid() const {
    return m_showGrid;
}

void GpuLineChart::setShowGrid(bool visible) {
    if (m_showGrid == visible) {
        return;
    }
    m_showGrid = visible;
    emit appearanceChanged();
    scheduleUpdate();
}

bool GpuLineChart::extendLastToMaxX() const {
    return m_extendLastToMaxX;
}

void GpuLineChart::setExtendLastToMaxX(bool extend) {
    if (m_extendLastToMaxX == extend) {
        return;
    }
    m_extendLastToMaxX = extend;
    emit appearanceChanged();
    scheduleUpdate();
}

int GpuLineChart::gridRows() const {
    return m_gridRows;
}

void GpuLineChart::setGridRows(int rows) {
    const int normalized = std::max(0, rows);
    if (m_gridRows == normalized) {
        return;
    }
    m_gridRows = normalized;
    emit appearanceChanged();
    scheduleUpdate();
}

int GpuLineChart::gridColumns() const {
    return m_gridColumns;
}

void GpuLineChart::setGridColumns(int columns) {
    const int normalized = std::max(0, columns);
    if (m_gridColumns == normalized) {
        return;
    }
    m_gridColumns = normalized;
    emit appearanceChanged();
    scheduleUpdate();
}

int GpuLineChart::sampleCapacity() const {
    return m_sampleCapacity;
}

void GpuLineChart::setSampleCapacity(int capacity) {
    const int normalized = std::max(0, capacity);
    if (m_sampleCapacity == normalized) {
        return;
    }
    m_sampleCapacity = normalized;
    emit appearanceChanged();
    scheduleUpdate();
}

bool GpuLineChart::smooth() const {
    return m_smooth;
}

void GpuLineChart::setSmooth(bool smooth) {
    if (m_smooth == smooth) {
        return;
    }
    m_smooth = smooth;
    emit appearanceChanged();
    scheduleUpdate();
}

int GpuLineChart::smoothSteps() const {
    return m_smoothSteps;
}

void GpuLineChart::setSmoothSteps(int steps) {
    const int normalized = std::clamp(steps, 1, 16);
    if (m_smoothSteps == normalized) {
        return;
    }
    m_smoothSteps = normalized;
    emit appearanceChanged();
    scheduleUpdate();
}

qreal GpuLineChart::lineWidth() const {
    return m_lineWidth;
}

void GpuLineChart::setLineWidth(qreal width) {
    const qreal normalized = std::max(1.0, width);
    if (almostEqual(m_lineWidth, normalized)) {
        return;
    }
    m_lineWidth = normalized;
    emit appearanceChanged();
    scheduleUpdate();
}

qreal GpuLineChart::leftPadding() const {
    return m_leftPadding;
}

void GpuLineChart::setLeftPadding(qreal value) {
    if (almostEqual(m_leftPadding, value)) {
        return;
    }
    m_leftPadding = value;
    emit paddingChanged();
    scheduleUpdate();
}

qreal GpuLineChart::rightPadding() const {
    return m_rightPadding;
}

void GpuLineChart::setRightPadding(qreal value) {
    if (almostEqual(m_rightPadding, value)) {
        return;
    }
    m_rightPadding = value;
    emit paddingChanged();
    scheduleUpdate();
}

qreal GpuLineChart::topPadding() const {
    return m_topPadding;
}

void GpuLineChart::setTopPadding(qreal value) {
    if (almostEqual(m_topPadding, value)) {
        return;
    }
    m_topPadding = value;
    emit paddingChanged();
    scheduleUpdate();
}

qreal GpuLineChart::bottomPadding() const {
    return m_bottomPadding;
}

void GpuLineChart::setBottomPadding(qreal value) {
    if (almostEqual(m_bottomPadding, value)) {
        return;
    }
    m_bottomPadding = value;
    emit paddingChanged();
    scheduleUpdate();
}

QSGNode* GpuLineChart::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
    delete oldNode;

    auto* root = new QSGNode;
    const QRectF plot = plotRect();
    if (plot.width() <= 0.0 || plot.height() <= 0.0) {
        return root;
    }

    if (auto* grid = createGridNode(plot)) {
        root->appendChildNode(grid);
    }

    const QVector<QPointF> points = chartPoints(plot);
    if (points.isEmpty()) {
        return root;
    }

    if (auto* fill = createFillNode(points, plot.bottom())) {
        root->appendChildNode(fill);
    }

    if (auto* line = createLineNode(points)) {
        root->appendChildNode(line);
    }

    return root;
}

void GpuLineChart::componentComplete() {
    QQuickItem::componentComplete();
    scheduleUpdate();
}

void GpuLineChart::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) {
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    scheduleUpdate();
}

QRectF GpuLineChart::plotRect() const {
    const qreal left = std::max<qreal>(0.0, m_leftPadding);
    const qreal top = std::max<qreal>(0.0, m_topPadding);
    const qreal right = std::max<qreal>(0.0, m_rightPadding);
    const qreal bottom = std::max<qreal>(0.0, m_bottomPadding);
    return QRectF(left,
                  top,
                  std::max<qreal>(1.0, width() - left - right),
                  std::max<qreal>(1.0, height() - top - bottom));
}

QVector<qreal> GpuLineChart::numericValues(const QVariantList& values) const {
    QVector<qreal> result;
    result.reserve(values.size());
    for (const QVariant& value : values) {
        bool ok = false;
        const qreal number = value.toReal(&ok);
        result.push_back(ok && std::isfinite(number) ? number : 0.0);
    }
    return result;
}

QVector<QPointF> GpuLineChart::chartPoints(const QRectF& plot) const {
    const QVector<qreal> yValues = numericValues(m_values);
    if (yValues.isEmpty()) {
        return {};
    }

    const QVector<qreal> xValues = numericValues(m_xValues);
    const bool hasExplicitX = xValues.size() >= yValues.size();
    const qreal ySpan = std::max<qreal>(1.0, m_maxValue - m_minValue);
    const qreal xSpan = std::max<qreal>(1.0, m_maxX - m_minX);
    const int capacity = std::max(m_sampleCapacity, int(yValues.size()));
    QVector<QPointF> basePoints;
    basePoints.reserve(yValues.size());

    for (int i = 0; i < yValues.size(); ++i) {
        const qreal xNorm = hasExplicitX ? clamp01((xValues[i] - m_minX) / xSpan)
                                         : (capacity <= 1 ? 0.0 : qreal(i) / qreal(capacity - 1));
        const qreal yNorm = clamp01((yValues[i] - m_minValue) / ySpan);
        basePoints.push_back(QPointF(plot.left() + xNorm * plot.width(), plot.bottom() - yNorm * plot.height()));
    }

    QVector<QPointF> points = m_smooth ? smoothPoints(basePoints, plot) : basePoints;
    if (m_extendLastToMaxX && !points.isEmpty() && points.constLast().x() < plot.right()) {
        const qreal y = basePoints.isEmpty() ? points.constLast().y() : basePoints.constLast().y();
        points.push_back(QPointF(plot.right(), y));
    }

    return points;
}

QVector<QPointF> GpuLineChart::smoothPoints(const QVector<QPointF>& points, const QRectF& plot) const {
    if (points.size() < 3 || m_smoothSteps <= 1) {
        return points;
    }

    QVector<QPointF> result;
    result.reserve((points.size() - 1) * m_smoothSteps + 1);
    result.push_back(points.first());

    for (int i = 0; i < points.size() - 1; ++i) {
        const QPointF p0 = i > 0 ? points[i - 1] : points[i];
        const QPointF p1 = points[i];
        const QPointF p2 = points[i + 1];
        const QPointF p3 = i + 2 < points.size() ? points[i + 2] : p2;

        for (int step = 1; step <= m_smoothSteps; ++step) {
            const qreal t = qreal(step) / qreal(m_smoothSteps);
            const qreal t2 = t * t;
            const qreal t3 = t2 * t;
            qreal x = 0.5 * ((2.0 * p1.x()) + (-p0.x() + p2.x()) * t +
                             (2.0 * p0.x() - 5.0 * p1.x() + 4.0 * p2.x() - p3.x()) * t2 +
                             (-p0.x() + 3.0 * p1.x() - 3.0 * p2.x() + p3.x()) * t3);
            qreal y = 0.5 * ((2.0 * p1.y()) + (-p0.y() + p2.y()) * t +
                             (2.0 * p0.y() - 5.0 * p1.y() + 4.0 * p2.y() - p3.y()) * t2 +
                             (-p0.y() + 3.0 * p1.y() - 3.0 * p2.y() + p3.y()) * t3);

            x = std::clamp(x, p1.x(), p2.x());
            y = std::clamp(y, plot.top(), plot.bottom());
            result.push_back(QPointF(x, y));
        }
    }

    return result;
}

QSGGeometryNode* GpuLineChart::createGridNode(const QRectF& plot) const {
    if (!m_showGrid || m_gridColor.alpha() == 0 || (m_gridRows <= 0 && m_gridColumns <= 0)) {
        return nullptr;
    }

    const int vertexCount = 2 * (m_gridRows + m_gridColumns);
    auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), vertexCount);
    geometry->setDrawingMode(QSGGeometry::DrawLines);
    geometry->setLineWidth(1.0f);

    auto* vertices = geometry->vertexDataAsPoint2D();
    int index = 0;
    for (int column = 0; column < m_gridColumns; ++column) {
        const qreal x = m_gridColumns <= 1 ? plot.left() : plot.left() + plot.width() * column / qreal(m_gridColumns - 1);
        vertices[index++].set(x, plot.top());
        vertices[index++].set(x, plot.bottom());
    }

    for (int row = 0; row < m_gridRows; ++row) {
        const qreal y = m_gridRows <= 1 ? plot.bottom() : plot.top() + plot.height() * row / qreal(m_gridRows - 1);
        vertices[index++].set(plot.left(), y);
        vertices[index++].set(plot.right(), y);
    }

    auto* node = new QSGGeometryNode;
    node->setGeometry(geometry);
    node->setMaterial(materialFor(m_gridColor));
    node->setFlag(QSGNode::OwnsGeometry);
    node->setFlag(QSGNode::OwnsMaterial);
    return node;
}

QSGGeometryNode* GpuLineChart::createFillNode(const QVector<QPointF>& points, qreal bottom) const {
    if (!m_fillVisible || m_fillColor.alpha() == 0 || points.size() < 2) {
        return nullptr;
    }

    auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), points.size() * 2);
    geometry->setDrawingMode(QSGGeometry::DrawTriangleStrip);

    auto* vertices = geometry->vertexDataAsPoint2D();
    int index = 0;
    for (const QPointF& point : points) {
        vertices[index++].set(point.x(), bottom);
        vertices[index++].set(point.x(), point.y());
    }

    auto* node = new QSGGeometryNode;
    node->setGeometry(geometry);
    node->setMaterial(materialFor(m_fillColor));
    node->setFlag(QSGNode::OwnsGeometry);
    node->setFlag(QSGNode::OwnsMaterial);
    return node;
}

QSGGeometryNode* GpuLineChart::createLineNode(const QVector<QPointF>& points) const {
    if (m_lineColor.alpha() == 0 || points.size() < 2) {
        return nullptr;
    }

    const QVector<QPointF> filteredPoints = removeDuplicatePoints(points);
    if (filteredPoints.size() < 2) {
        return nullptr;
    }

    const qreal innerHalfWidth = std::max<qreal>(0.5, m_lineWidth / 2.0);
    const qreal outerHalfWidth = innerHalfWidth + 0.9;
    const QVector<StrokeSection> sections = strokeSections(filteredPoints, innerHalfWidth, outerHalfWidth);
    const int segmentCount = filteredPoints.size() - 1;

    auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(), segmentCount * 18);
    geometry->setDrawingMode(QSGGeometry::DrawTriangles);

    auto* vertices = geometry->vertexDataAsColoredPoint2D();
    int vertexIndex = 0;
    for (int i = 0; i < segmentCount; ++i) {
        const StrokeSection& start = sections[i];
        const StrokeSection& end = sections[i + 1];

        appendColoredTriangle(vertices, vertexIndex, start.innerLeft, 1.0, start.innerRight, 1.0, end.innerRight, 1.0, m_lineColor);
        appendColoredTriangle(vertices, vertexIndex, start.innerLeft, 1.0, end.innerRight, 1.0, end.innerLeft, 1.0, m_lineColor);

        appendColoredTriangle(vertices, vertexIndex, start.outerLeft, 0.0, start.innerLeft, 1.0, end.innerLeft, 1.0, m_lineColor);
        appendColoredTriangle(vertices, vertexIndex, start.outerLeft, 0.0, end.innerLeft, 1.0, end.outerLeft, 0.0, m_lineColor);

        appendColoredTriangle(vertices, vertexIndex, start.innerRight, 1.0, start.outerRight, 0.0, end.outerRight, 0.0, m_lineColor);
        appendColoredTriangle(vertices, vertexIndex, start.innerRight, 1.0, end.outerRight, 0.0, end.innerRight, 1.0, m_lineColor);
    }

    auto* node = new QSGGeometryNode;
    node->setGeometry(geometry);
    node->setMaterial(new QSGVertexColorMaterial);
    node->setFlag(QSGNode::OwnsGeometry);
    node->setFlag(QSGNode::OwnsMaterial);
    return node;
}

void GpuLineChart::scheduleUpdate() {
    if (isComponentComplete()) {
        update();
    }
}
