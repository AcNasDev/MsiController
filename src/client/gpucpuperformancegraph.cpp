#include "gpucpuperformancegraph.h"

#include <QSGFlatColorMaterial>
#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGNode>
#include <QTimerEvent>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {
constexpr qreal pi = 3.14159265358979323846;

bool almostEqual(qreal left, qreal right) {
    return std::abs(left - right) <= std::numeric_limits<qreal>::epsilon() * 16.0;
}

qreal clamp01(qreal value) {
    return std::clamp(value, 0.0, 1.0);
}

qreal snapPixel(qreal value) {
    return std::round(value);
}

QSGFlatColorMaterial* materialFor(const QColor& color) {
    auto* material = new QSGFlatColorMaterial;
    material->setColor(color);
    return material;
}

QVector<QPointF> roundedRectPoints(const QRectF& rect, qreal radius, int segments) {
    const qreal r = std::min({std::max<qreal>(0.0, radius), rect.width() / 2.0, rect.height() / 2.0});
    if (r <= 0.0 || segments <= 0) {
        return {rect.topLeft(), rect.bottomLeft(), rect.bottomRight(), rect.topRight()};
    }

    QVector<QPointF> points;
    points.reserve(4 * (segments + 1));
    const QPointF centers[] = {
        QPointF(rect.right() - r, rect.top() + r),
        QPointF(rect.right() - r, rect.bottom() - r),
        QPointF(rect.left() + r, rect.bottom() - r),
        QPointF(rect.left() + r, rect.top() + r),
    };
    const qreal startAngles[] = {-90.0, 0.0, 90.0, 180.0};

    for (int corner = 0; corner < 4; ++corner) {
        for (int step = 0; step <= segments; ++step) {
            const qreal angle = (startAngles[corner] + 90.0 * step / qreal(segments)) * pi / 180.0;
            points.push_back(QPointF(centers[corner].x() + std::cos(angle) * r,
                                     centers[corner].y() + std::sin(angle) * r));
        }
    }
    return points;
}

void appendRoundedRect(QSGGeometry::Point2D* vertices, int& index, const QRectF& rect, qreal radius, int segments) {
    const QVector<QPointF> points = roundedRectPoints(rect, radius, segments);
    const QPointF center = rect.center();
    for (int i = 0; i < points.size(); ++i) {
        const QPointF& a = points[i];
        const QPointF& b = points[(i + 1) % points.size()];
        vertices[index++].set(center.x(), center.y());
        vertices[index++].set(a.x(), a.y());
        vertices[index++].set(b.x(), b.y());
    }
}

QVector<QPointF> topRoundedRectPoints(const QRectF& rect, qreal radius, int segments) {
    const qreal r = std::min({std::max<qreal>(0.0, radius), rect.width() / 2.0, rect.height()});
    if (r <= 0.0 || segments <= 0) {
        return {rect.topLeft(), rect.bottomLeft(), rect.bottomRight(), rect.topRight()};
    }

    QVector<QPointF> points;
    points.reserve(2 * (segments + 1) + 2);
    points.push_back(rect.bottomRight());
    points.push_back(rect.bottomLeft());

    const QPointF leftCenter(rect.left() + r, rect.top() + r);
    for (int step = 0; step <= segments; ++step) {
        const qreal angle = (180.0 + 90.0 * step / qreal(segments)) * pi / 180.0;
        points.push_back(QPointF(leftCenter.x() + std::cos(angle) * r,
                                 leftCenter.y() + std::sin(angle) * r));
    }

    const QPointF rightCenter(rect.right() - r, rect.top() + r);
    for (int step = 0; step <= segments; ++step) {
        const qreal angle = (-90.0 + 90.0 * step / qreal(segments)) * pi / 180.0;
        points.push_back(QPointF(rightCenter.x() + std::cos(angle) * r,
                                 rightCenter.y() + std::sin(angle) * r));
    }

    return points;
}

void appendPolygonFan(QSGGeometry::Point2D* vertices, int& index, const QVector<QPointF>& points, const QPointF& center) {
    for (int i = 0; i < points.size(); ++i) {
        const QPointF& a = points[i];
        const QPointF& b = points[(i + 1) % points.size()];
        vertices[index++].set(center.x(), center.y());
        vertices[index++].set(a.x(), a.y());
        vertices[index++].set(b.x(), b.y());
    }
}
} // namespace

GpuCpuPerformanceGraph::GpuCpuPerformanceGraph(QQuickItem* parent)
    : QQuickItem(parent) {
    setFlag(ItemHasContents, true);
    setAntialiasing(true);
}

QVariantList GpuCpuPerformanceGraph::frequencyNorms() const {
    return m_frequencyNorms;
}

void GpuCpuPerformanceGraph::setFrequencyNorms(const QVariantList& values) {
    if (m_frequencyNorms == values) {
        return;
    }
    m_frequencyNorms = values;
    setTargets(m_frequencyTargets, m_displayFrequency, values, true);
}

QVariantList GpuCpuPerformanceGraph::usageNorms() const {
    return m_usageNorms;
}

void GpuCpuPerformanceGraph::setUsageNorms(const QVariantList& values) {
    if (m_usageNorms == values) {
        return;
    }
    m_usageNorms = values;
    setTargets(m_usageTargets, m_displayUsage, values, true);
}

QVariantList GpuCpuPerformanceGraph::limitNorms() const {
    return m_limitNorms;
}

void GpuCpuPerformanceGraph::setLimitNorms(const QVariantList& values) {
    if (m_limitNorms == values) {
        return;
    }
    m_limitNorms = values;
    setTargets(m_limitTargets, m_displayLimit, values, true);
}

QColor GpuCpuPerformanceGraph::trackColor() const {
    return m_trackColor;
}

void GpuCpuPerformanceGraph::setTrackColor(const QColor& color) {
    if (m_trackColor == color) {
        return;
    }
    m_trackColor = color;
    emit colorsChanged();
    scheduleUpdate();
}

QColor GpuCpuPerformanceGraph::frequencyColor() const {
    return m_frequencyColor;
}

void GpuCpuPerformanceGraph::setFrequencyColor(const QColor& color) {
    if (m_frequencyColor == color) {
        return;
    }
    m_frequencyColor = color;
    emit colorsChanged();
    scheduleUpdate();
}

QColor GpuCpuPerformanceGraph::usageColor() const {
    return m_usageColor;
}

void GpuCpuPerformanceGraph::setUsageColor(const QColor& color) {
    if (m_usageColor == color) {
        return;
    }
    m_usageColor = color;
    emit colorsChanged();
    scheduleUpdate();
}

QColor GpuCpuPerformanceGraph::limitColor() const {
    return m_limitColor;
}

void GpuCpuPerformanceGraph::setLimitColor(const QColor& color) {
    if (m_limitColor == color) {
        return;
    }
    m_limitColor = color;
    emit colorsChanged();
    scheduleUpdate();
}

QColor GpuCpuPerformanceGraph::gridColor() const {
    return m_gridColor;
}

void GpuCpuPerformanceGraph::setGridColor(const QColor& color) {
    if (m_gridColor == color) {
        return;
    }
    m_gridColor = color;
    emit colorsChanged();
    scheduleUpdate();
}

int GpuCpuPerformanceGraph::limitHoverIndex() const {
    return m_limitHoverIndex;
}

void GpuCpuPerformanceGraph::setLimitHoverIndex(int index) {
    if (m_limitHoverIndex == index) {
        return;
    }
    m_limitHoverIndex = index;
    emit highlightChanged();
    scheduleUpdate();
}

int GpuCpuPerformanceGraph::editingIndex() const {
    return m_editingIndex;
}

void GpuCpuPerformanceGraph::setEditingIndex(int index) {
    if (m_editingIndex == index) {
        return;
    }
    m_editingIndex = index;
    emit highlightChanged();
    scheduleUpdate();
}

int GpuCpuPerformanceGraph::animationFps() const {
    return m_animationFps;
}

void GpuCpuPerformanceGraph::setAnimationFps(int fps) {
    const int normalized = std::clamp(fps, 12, 60);
    if (m_animationFps == normalized) {
        return;
    }
    m_animationFps = normalized;
    if (m_frameTimer.isActive()) {
        m_frameTimer.stop();
        m_frameClock.restart();
        m_frameTimer.start(1000 / m_animationFps, Qt::PreciseTimer, this);
    }
    emit animationChanged();
}

qreal GpuCpuPerformanceGraph::telemetryEaseMs() const {
    return m_telemetryEaseMs;
}

void GpuCpuPerformanceGraph::setTelemetryEaseMs(qreal value) {
    const qreal normalized = std::max<qreal>(80.0, value);
    if (almostEqual(m_telemetryEaseMs, normalized)) {
        return;
    }
    m_telemetryEaseMs = normalized;
    emit animationChanged();
}

qreal GpuCpuPerformanceGraph::limitEaseMs() const {
    return m_limitEaseMs;
}

void GpuCpuPerformanceGraph::setLimitEaseMs(qreal value) {
    const qreal normalized = std::max<qreal>(40.0, value);
    if (almostEqual(m_limitEaseMs, normalized)) {
        return;
    }
    m_limitEaseMs = normalized;
    emit animationChanged();
}

qreal GpuCpuPerformanceGraph::dragLimitEaseMs() const {
    return m_dragLimitEaseMs;
}

void GpuCpuPerformanceGraph::setDragLimitEaseMs(qreal value) {
    const qreal normalized = std::max<qreal>(16.0, value);
    if (almostEqual(m_dragLimitEaseMs, normalized)) {
        return;
    }
    m_dragLimitEaseMs = normalized;
    emit animationChanged();
}

qreal GpuCpuPerformanceGraph::changeEpsilon() const {
    return m_changeEpsilon;
}

void GpuCpuPerformanceGraph::setChangeEpsilon(qreal value) {
    const qreal normalized = std::max<qreal>(0.0, value);
    if (almostEqual(m_changeEpsilon, normalized)) {
        return;
    }
    m_changeEpsilon = normalized;
    emit animationChanged();
}

qreal GpuCpuPerformanceGraph::leftPadding() const {
    return m_leftPadding;
}

void GpuCpuPerformanceGraph::setLeftPadding(qreal value) {
    if (almostEqual(m_leftPadding, value)) {
        return;
    }
    m_leftPadding = value;
    emit paddingChanged();
    scheduleUpdate();
}

qreal GpuCpuPerformanceGraph::rightPadding() const {
    return m_rightPadding;
}

void GpuCpuPerformanceGraph::setRightPadding(qreal value) {
    if (almostEqual(m_rightPadding, value)) {
        return;
    }
    m_rightPadding = value;
    emit paddingChanged();
    scheduleUpdate();
}

qreal GpuCpuPerformanceGraph::topPadding() const {
    return m_topPadding;
}

void GpuCpuPerformanceGraph::setTopPadding(qreal value) {
    if (almostEqual(m_topPadding, value)) {
        return;
    }
    m_topPadding = value;
    emit paddingChanged();
    scheduleUpdate();
}

qreal GpuCpuPerformanceGraph::bottomPadding() const {
    return m_bottomPadding;
}

void GpuCpuPerformanceGraph::setBottomPadding(qreal value) {
    if (almostEqual(m_bottomPadding, value)) {
        return;
    }
    m_bottomPadding = value;
    emit paddingChanged();
    scheduleUpdate();
}

qreal GpuCpuPerformanceGraph::barRadius() const {
    return m_barRadius;
}

void GpuCpuPerformanceGraph::setBarRadius(qreal value) {
    const qreal normalized = std::max<qreal>(0.0, value);
    if (almostEqual(m_barRadius, normalized)) {
        return;
    }
    m_barRadius = normalized;
    emit appearanceChanged();
    scheduleUpdate();
}

int GpuCpuPerformanceGraph::frameTick() const {
    return m_frameTick;
}

int GpuCpuPerformanceGraph::coreAt(qreal x) const {
    const int count = coreCount();
    const QRectF plot = plotRect();
    if (count <= 0 || x < plot.left() || x > plot.right()) {
        return -1;
    }
    const int index = int((x - plot.left()) / std::max<qreal>(1.0, slotWidth()));
    return std::clamp(index, 0, count - 1);
}

int GpuCpuPerformanceGraph::limitAt(qreal x, qreal y) const {
    const int index = coreAt(x);
    if (index < 0) {
        return -1;
    }

    const qreal bar = barWidth();
    const qreal barLeft = coreCenter(index) - bar / 2.0;
    if (x < barLeft - 8.0 || x > barLeft + bar + 8.0) {
        return -1;
    }

    return std::abs(y - limitY(index)) <= 9.0 ? index : -1;
}

qreal GpuCpuPerformanceGraph::limitNormFromY(qreal y) const {
    const QRectF plot = plotRect();
    return 1.0 - clamp01((y - plot.top()) / std::max<qreal>(1.0, plot.height()));
}

qreal GpuCpuPerformanceGraph::limitY(int index) const {
    const QRectF plot = plotRect();
    return plot.bottom() - plot.height() * displayLimitNorm(index);
}

qreal GpuCpuPerformanceGraph::coreCenter(int index) const {
    return plotRect().left() + slotWidth() * index + slotWidth() / 2.0;
}

qreal GpuCpuPerformanceGraph::slotWidth() const {
    const int count = coreCount();
    return count > 0 ? plotRect().width() / count : 0.0;
}

qreal GpuCpuPerformanceGraph::barWidth() const {
    return std::max<qreal>(8.0, std::min<qreal>(28.0, slotWidth() * 0.52));
}

qreal GpuCpuPerformanceGraph::plotBottom() const {
    return plotRect().bottom();
}

qreal GpuCpuPerformanceGraph::displayFrequencyNorm(int index) const {
    return normalizedAt(m_displayFrequency, index);
}

qreal GpuCpuPerformanceGraph::displayUsageNorm(int index) const {
    return normalizedAt(m_displayUsage, index);
}

qreal GpuCpuPerformanceGraph::displayLimitNorm(int index) const {
    return normalizedAt(m_displayLimit, index, normalizedAt(m_limitTargets, index));
}

void GpuCpuPerformanceGraph::componentComplete() {
    QQuickItem::componentComplete();
    scheduleUpdate();
}

QSGNode* GpuCpuPerformanceGraph::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
    delete oldNode;

    auto* root = new QSGNode;
    const QRectF plot = plotRect();
    const int count = coreCount();
    if (count <= 0 || plot.width() <= 0.0 || plot.height() <= 0.0) {
        return root;
    }

    if (auto* grid = createGridNode(plot)) {
        root->appendChildNode(grid);
    }

    QVector<QRectF> tracks;
    QVector<QRectF> frequencies;
    QVector<QRectF> usages;
    QVector<QRectF> limits;
    tracks.reserve(count);
    frequencies.reserve(count);
    usages.reserve(count);
    limits.reserve(count);

    const qreal bar = barWidth();
    const qreal plotTop = snapPixel(plot.top());
    const qreal plotBottom = snapPixel(plot.bottom());
    const qreal plotHeight = std::max<qreal>(1.0, plotBottom - plotTop);
    for (int i = 0; i < count; ++i) {
        const qreal left = snapPixel(coreCenter(i) - bar / 2.0);
        const qreal right = std::max(left + 1.0, snapPixel(coreCenter(i) + bar / 2.0));
        const qreal snappedBar = right - left;
        const QRectF track(left, plotTop, snappedBar, plotHeight);
        tracks.push_back(track);

        const qreal freqHeight = std::max<qreal>(2.0, plotHeight * displayFrequencyNorm(i));
        frequencies.push_back(QRectF(left, plotBottom - freqHeight, snappedBar, freqHeight));

        const qreal usageWidth = std::max<qreal>(3.0, snapPixel(snappedBar * 0.58));
        const qreal usageLeft = snapPixel(left + (snappedBar - usageWidth) / 2.0);
        const qreal usageHeight = std::max<qreal>(2.0, plotHeight * displayUsageNorm(i));
        usages.push_back(QRectF(usageLeft, plotBottom - usageHeight, usageWidth, usageHeight));

        const qreal lineHeight = (m_limitHoverIndex == i || m_editingIndex == i) ? 3.0 : 2.0;
        limits.push_back(QRectF(left - 5.0, limitY(i) - lineHeight / 2.0, snappedBar + 10.0, lineHeight));
    }

    if (auto* node = createRoundedRectsNode(tracks, m_trackColor, m_barRadius)) {
        root->appendChildNode(node);
    }
    if (auto* node = createTopRoundedRectsNode(frequencies, m_frequencyColor, m_barRadius)) {
        root->appendChildNode(node);
    }
    if (auto* node = createTopRoundedRectsNode(usages, m_usageColor, std::max<qreal>(2.0, m_barRadius - 1.0))) {
        root->appendChildNode(node);
    }
    if (auto* node = createRoundedRectsNode(limits, m_limitColor, 2.0)) {
        root->appendChildNode(node);
    }

    return root;
}

void GpuCpuPerformanceGraph::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) {
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    scheduleUpdate();
}

void GpuCpuPerformanceGraph::itemChange(ItemChange change, const ItemChangeData& value) {
    QQuickItem::itemChange(change, value);
    if (change == ItemVisibleHasChanged && !value.boolValue) {
        stopAnimation();
    }
}

void GpuCpuPerformanceGraph::timerEvent(QTimerEvent* event) {
    if (event->timerId() == m_frameTimer.timerId()) {
        onFrame();
        return;
    }
    QQuickItem::timerEvent(event);
}

QRectF GpuCpuPerformanceGraph::plotRect() const {
    const qreal left = std::max<qreal>(0.0, m_leftPadding);
    const qreal top = std::max<qreal>(0.0, m_topPadding);
    const qreal right = std::max<qreal>(0.0, m_rightPadding);
    const qreal bottom = std::max<qreal>(0.0, m_bottomPadding);
    return QRectF(left,
                  top,
                  std::max<qreal>(1.0, width() - left - right),
                  std::max<qreal>(1.0, height() - top - bottom));
}

QVector<qreal> GpuCpuPerformanceGraph::numericValues(const QVariantList& values) const {
    QVector<qreal> result;
    result.reserve(values.size());
    for (const QVariant& value : values) {
        bool ok = false;
        const qreal number = value.toReal(&ok);
        result.push_back(ok && std::isfinite(number) ? clamp01(number) : 0.0);
    }
    return result;
}

void GpuCpuPerformanceGraph::setTargets(QVector<qreal>& targets,
                                        QVector<qreal>& display,
                                        const QVariantList& values,
                                        bool emitDataChanged) {
    const QVector<qreal> next = numericValues(values);
    const bool sizeChanged = targets.size() != next.size();
    bool meaningfulChange = sizeChanged;
    for (int i = 0; i < targets.size() && i < next.size(); ++i) {
        if (std::abs(targets[i] - next[i]) >= m_changeEpsilon) {
            meaningfulChange = true;
            break;
        }
    }

    targets = next;
    if (sizeChanged || display.size() != next.size()) {
        display = next;
        scheduleUpdate();
    } else if (meaningfulChange) {
        startAnimation();
    }

    if (emitDataChanged) {
        emit dataChanged();
    }
}

bool GpuCpuPerformanceGraph::advanceVector(QVector<qreal>& display,
                                           const QVector<qreal>& targets,
                                           qreal easeMs,
                                           qreal dtMs) {
    if (display.size() != targets.size()) {
        display = targets;
        return true;
    }

    const qreal factor = 1.0 - std::exp(-dtMs / std::max<qreal>(16.0, easeMs) * 4.0);
    bool changed = false;
    for (int i = 0; i < display.size(); ++i) {
        const qreal delta = targets[i] - display[i];
        if (std::abs(delta) <= 0.001) {
            if (!almostEqual(display[i], targets[i])) {
                display[i] = targets[i];
                changed = true;
            }
            continue;
        }

        display[i] = clamp01(display[i] + delta * factor);
        changed = true;
    }
    return changed;
}

void GpuCpuPerformanceGraph::startAnimation() {
    if (!isVisible() || !isComponentComplete()) {
        return;
    }

    if (!m_frameTimer.isActive()) {
        m_frameClock.restart();
        m_frameTimer.start(1000 / m_animationFps, Qt::PreciseTimer, this);
    }
}

void GpuCpuPerformanceGraph::stopAnimation() {
    if (m_frameTimer.isActive()) {
        m_frameTimer.stop();
    }
}

void GpuCpuPerformanceGraph::onFrame() {
    if (!isVisible()) {
        stopAnimation();
        return;
    }

    const qreal dtMs = std::clamp<qreal>(m_frameClock.restart(), 1.0, 100.0);
    bool changed = false;
    changed = advanceVector(m_displayFrequency, m_frequencyTargets, m_telemetryEaseMs, dtMs) || changed;
    changed = advanceVector(m_displayUsage, m_usageTargets, m_telemetryEaseMs, dtMs) || changed;
    changed = advanceVector(m_displayLimit,
                            m_limitTargets,
                            m_editingIndex >= 0 ? m_dragLimitEaseMs : m_limitEaseMs,
                            dtMs) ||
              changed;

    if (changed) {
        ++m_frameTick;
        emit frameTickChanged();
        update();
    } else {
        stopAnimation();
    }
}

void GpuCpuPerformanceGraph::scheduleUpdate() {
    if (isComponentComplete()) {
        update();
    }
}

int GpuCpuPerformanceGraph::coreCount() const {
    return std::max({m_frequencyTargets.size(), m_usageTargets.size(), m_limitTargets.size()});
}

qreal GpuCpuPerformanceGraph::normalizedAt(const QVector<qreal>& values, int index, qreal fallback) const {
    return index >= 0 && index < values.size() ? clamp01(values[index]) : fallback;
}

QSGGeometryNode* GpuCpuPerformanceGraph::createGridNode(const QRectF& plot) const {
    if (m_gridColor.alpha() == 0) {
        return nullptr;
    }

    constexpr int rowCount = 4;
    auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), rowCount * 2);
    geometry->setDrawingMode(QSGGeometry::DrawLines);
    geometry->setLineWidth(1.0f);

    auto* vertices = geometry->vertexDataAsPoint2D();
    int vertex = 0;
    for (int row = 0; row < rowCount; ++row) {
        const qreal y = plot.top() + plot.height() * row / qreal(rowCount - 1);
        vertices[vertex++].set(plot.left(), y);
        vertices[vertex++].set(plot.right(), y);
    }

    auto* node = new QSGGeometryNode;
    node->setGeometry(geometry);
    node->setMaterial(materialFor(m_gridColor));
    node->setFlag(QSGNode::OwnsGeometry);
    node->setFlag(QSGNode::OwnsMaterial);
    return node;
}

QSGGeometryNode* GpuCpuPerformanceGraph::createRoundedRectsNode(const QVector<QRectF>& rects,
                                                               const QColor& color,
                                                               qreal radius) const {
    if (rects.isEmpty() || color.alpha() == 0) {
        return nullptr;
    }

    constexpr int segments = 8;
    constexpr int pointsPerRect = 4 * (segments + 1);
    constexpr int verticesPerRect = pointsPerRect * 3;
    auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), rects.size() * verticesPerRect);
    geometry->setDrawingMode(QSGGeometry::DrawTriangles);

    auto* vertices = geometry->vertexDataAsPoint2D();
    int vertex = 0;
    for (const QRectF& rect : rects) {
        appendRoundedRect(vertices, vertex, rect, radius, segments);
    }

    auto* node = new QSGGeometryNode;
    node->setGeometry(geometry);
    node->setMaterial(materialFor(color));
    node->setFlag(QSGNode::OwnsGeometry);
    node->setFlag(QSGNode::OwnsMaterial);
    return node;
}

QSGGeometryNode* GpuCpuPerformanceGraph::createTopRoundedRectsNode(const QVector<QRectF>& rects,
                                                                  const QColor& color,
                                                                  qreal radius) const {
    if (rects.isEmpty() || color.alpha() == 0) {
        return nullptr;
    }

    constexpr int segments = 8;
    constexpr int pointsPerRect = 2 * (segments + 1) + 2;
    constexpr int verticesPerRect = pointsPerRect * 3;
    auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), rects.size() * verticesPerRect);
    geometry->setDrawingMode(QSGGeometry::DrawTriangles);

    auto* vertices = geometry->vertexDataAsPoint2D();
    int vertex = 0;
    for (const QRectF& rect : rects) {
        const QVector<QPointF> points = topRoundedRectPoints(rect, radius, segments);
        appendPolygonFan(vertices, vertex, points, rect.center());
    }

    auto* node = new QSGGeometryNode;
    node->setGeometry(geometry);
    node->setMaterial(materialFor(color));
    node->setFlag(QSGNode::OwnsGeometry);
    node->setFlag(QSGNode::OwnsMaterial);
    return node;
}
