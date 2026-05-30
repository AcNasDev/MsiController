#pragma once

#include <QColor>
#include <QPointF>
#include <QQuickItem>
#include <QVariantList>
#include <QVector>

class QSGGeometryNode;

class GpuLineChart : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QVariantList values READ values WRITE setValues NOTIFY valuesChanged)
    Q_PROPERTY(QVariantList xValues READ xValues WRITE setXValues NOTIFY xValuesChanged)
    Q_PROPERTY(qreal minValue READ minValue WRITE setMinValue NOTIFY rangeChanged)
    Q_PROPERTY(qreal maxValue READ maxValue WRITE setMaxValue NOTIFY rangeChanged)
    Q_PROPERTY(qreal minX READ minX WRITE setMinX NOTIFY xRangeChanged)
    Q_PROPERTY(qreal maxX READ maxX WRITE setMaxX NOTIFY xRangeChanged)
    Q_PROPERTY(QColor lineColor READ lineColor WRITE setLineColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor gridColor READ gridColor WRITE setGridColor NOTIFY colorsChanged)
    Q_PROPERTY(bool fillVisible READ fillVisible WRITE setFillVisible NOTIFY appearanceChanged)
    Q_PROPERTY(bool showGrid READ showGrid WRITE setShowGrid NOTIFY appearanceChanged)
    Q_PROPERTY(bool extendLastToMaxX READ extendLastToMaxX WRITE setExtendLastToMaxX NOTIFY appearanceChanged)
    Q_PROPERTY(int gridRows READ gridRows WRITE setGridRows NOTIFY appearanceChanged)
    Q_PROPERTY(int gridColumns READ gridColumns WRITE setGridColumns NOTIFY appearanceChanged)
    Q_PROPERTY(int sampleCapacity READ sampleCapacity WRITE setSampleCapacity NOTIFY appearanceChanged)
    Q_PROPERTY(bool smooth READ smooth WRITE setSmooth NOTIFY appearanceChanged)
    Q_PROPERTY(int smoothSteps READ smoothSteps WRITE setSmoothSteps NOTIFY appearanceChanged)
    Q_PROPERTY(qreal lineWidth READ lineWidth WRITE setLineWidth NOTIFY appearanceChanged)
    Q_PROPERTY(qreal leftPadding READ leftPadding WRITE setLeftPadding NOTIFY paddingChanged)
    Q_PROPERTY(qreal rightPadding READ rightPadding WRITE setRightPadding NOTIFY paddingChanged)
    Q_PROPERTY(qreal topPadding READ topPadding WRITE setTopPadding NOTIFY paddingChanged)
    Q_PROPERTY(qreal bottomPadding READ bottomPadding WRITE setBottomPadding NOTIFY paddingChanged)

public:
    explicit GpuLineChart(QQuickItem* parent = nullptr);

    QVariantList values() const;
    void setValues(const QVariantList& values);

    QVariantList xValues() const;
    void setXValues(const QVariantList& values);

    qreal minValue() const;
    void setMinValue(qreal value);

    qreal maxValue() const;
    void setMaxValue(qreal value);

    qreal minX() const;
    void setMinX(qreal value);

    qreal maxX() const;
    void setMaxX(qreal value);

    QColor lineColor() const;
    void setLineColor(const QColor& color);

    QColor fillColor() const;
    void setFillColor(const QColor& color);

    QColor gridColor() const;
    void setGridColor(const QColor& color);

    bool fillVisible() const;
    void setFillVisible(bool visible);

    bool showGrid() const;
    void setShowGrid(bool visible);

    bool extendLastToMaxX() const;
    void setExtendLastToMaxX(bool extend);

    int gridRows() const;
    void setGridRows(int rows);

    int gridColumns() const;
    void setGridColumns(int columns);

    int sampleCapacity() const;
    void setSampleCapacity(int capacity);

    bool smooth() const;
    void setSmooth(bool smooth);

    int smoothSteps() const;
    void setSmoothSteps(int steps);

    qreal lineWidth() const;
    void setLineWidth(qreal width);

    qreal leftPadding() const;
    void setLeftPadding(qreal value);

    qreal rightPadding() const;
    void setRightPadding(qreal value);

    qreal topPadding() const;
    void setTopPadding(qreal value);

    qreal bottomPadding() const;
    void setBottomPadding(qreal value);

signals:
    void valuesChanged();
    void xValuesChanged();
    void rangeChanged();
    void xRangeChanged();
    void colorsChanged();
    void appearanceChanged();
    void paddingChanged();

protected:
    void componentComplete() override;
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

private:
    QRectF plotRect() const;
    QVector<qreal> numericValues(const QVariantList& values) const;
    QVector<QPointF> chartPoints(const QRectF& plot) const;
    QVector<QPointF> smoothPoints(const QVector<QPointF>& points, const QRectF& plot) const;
    QSGGeometryNode* createGridNode(const QRectF& plot) const;
    QSGGeometryNode* createFillNode(const QVector<QPointF>& points, qreal bottom) const;
    QSGGeometryNode* createLineNode(const QVector<QPointF>& points) const;
    void scheduleUpdate();

    QVariantList m_values;
    QVariantList m_xValues;
    qreal m_minValue = 0.0;
    qreal m_maxValue = 100.0;
    qreal m_minX = 0.0;
    qreal m_maxX = 100.0;
    QColor m_lineColor = QColor("#3fa7ff");
    QColor m_fillColor = QColor(63, 167, 255, 40);
    QColor m_gridColor = QColor(50, 57, 70);
    bool m_fillVisible = true;
    bool m_showGrid = true;
    bool m_extendLastToMaxX = false;
    int m_gridRows = 4;
    int m_gridColumns = 0;
    int m_sampleCapacity = 0;
    bool m_smooth = true;
    int m_smoothSteps = 16;
    qreal m_lineWidth = 2.0;
    qreal m_leftPadding = 0.0;
    qreal m_rightPadding = 0.0;
    qreal m_topPadding = 0.0;
    qreal m_bottomPadding = 0.0;
};
