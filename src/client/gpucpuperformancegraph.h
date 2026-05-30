#pragma once

#include <QBasicTimer>
#include <QColor>
#include <QElapsedTimer>
#include <QQuickItem>
#include <QVariantList>
#include <QVector>

class QSGGeometryNode;

class GpuCpuPerformanceGraph : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QVariantList frequencyNorms READ frequencyNorms WRITE setFrequencyNorms NOTIFY dataChanged)
    Q_PROPERTY(QVariantList usageNorms READ usageNorms WRITE setUsageNorms NOTIFY dataChanged)
    Q_PROPERTY(QVariantList limitNorms READ limitNorms WRITE setLimitNorms NOTIFY dataChanged)
    Q_PROPERTY(QColor trackColor READ trackColor WRITE setTrackColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor frequencyColor READ frequencyColor WRITE setFrequencyColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor usageColor READ usageColor WRITE setUsageColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor limitColor READ limitColor WRITE setLimitColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor gridColor READ gridColor WRITE setGridColor NOTIFY colorsChanged)
    Q_PROPERTY(int limitHoverIndex READ limitHoverIndex WRITE setLimitHoverIndex NOTIFY highlightChanged)
    Q_PROPERTY(int editingIndex READ editingIndex WRITE setEditingIndex NOTIFY highlightChanged)
    Q_PROPERTY(int animationFps READ animationFps WRITE setAnimationFps NOTIFY animationChanged)
    Q_PROPERTY(qreal telemetryEaseMs READ telemetryEaseMs WRITE setTelemetryEaseMs NOTIFY animationChanged)
    Q_PROPERTY(qreal limitEaseMs READ limitEaseMs WRITE setLimitEaseMs NOTIFY animationChanged)
    Q_PROPERTY(qreal dragLimitEaseMs READ dragLimitEaseMs WRITE setDragLimitEaseMs NOTIFY animationChanged)
    Q_PROPERTY(qreal changeEpsilon READ changeEpsilon WRITE setChangeEpsilon NOTIFY animationChanged)
    Q_PROPERTY(qreal leftPadding READ leftPadding WRITE setLeftPadding NOTIFY paddingChanged)
    Q_PROPERTY(qreal rightPadding READ rightPadding WRITE setRightPadding NOTIFY paddingChanged)
    Q_PROPERTY(qreal topPadding READ topPadding WRITE setTopPadding NOTIFY paddingChanged)
    Q_PROPERTY(qreal bottomPadding READ bottomPadding WRITE setBottomPadding NOTIFY paddingChanged)
    Q_PROPERTY(qreal barRadius READ barRadius WRITE setBarRadius NOTIFY appearanceChanged)
    Q_PROPERTY(int frameTick READ frameTick NOTIFY frameTickChanged)

public:
    explicit GpuCpuPerformanceGraph(QQuickItem* parent = nullptr);

    QVariantList frequencyNorms() const;
    void setFrequencyNorms(const QVariantList& values);

    QVariantList usageNorms() const;
    void setUsageNorms(const QVariantList& values);

    QVariantList limitNorms() const;
    void setLimitNorms(const QVariantList& values);

    QColor trackColor() const;
    void setTrackColor(const QColor& color);

    QColor frequencyColor() const;
    void setFrequencyColor(const QColor& color);

    QColor usageColor() const;
    void setUsageColor(const QColor& color);

    QColor limitColor() const;
    void setLimitColor(const QColor& color);

    QColor gridColor() const;
    void setGridColor(const QColor& color);

    int limitHoverIndex() const;
    void setLimitHoverIndex(int index);

    int editingIndex() const;
    void setEditingIndex(int index);

    int animationFps() const;
    void setAnimationFps(int fps);

    qreal telemetryEaseMs() const;
    void setTelemetryEaseMs(qreal value);

    qreal limitEaseMs() const;
    void setLimitEaseMs(qreal value);

    qreal dragLimitEaseMs() const;
    void setDragLimitEaseMs(qreal value);

    qreal changeEpsilon() const;
    void setChangeEpsilon(qreal value);

    qreal leftPadding() const;
    void setLeftPadding(qreal value);

    qreal rightPadding() const;
    void setRightPadding(qreal value);

    qreal topPadding() const;
    void setTopPadding(qreal value);

    qreal bottomPadding() const;
    void setBottomPadding(qreal value);

    qreal barRadius() const;
    void setBarRadius(qreal value);

    int frameTick() const;

    Q_INVOKABLE int coreAt(qreal x) const;
    Q_INVOKABLE int limitAt(qreal x, qreal y) const;
    Q_INVOKABLE qreal limitNormFromY(qreal y) const;
    Q_INVOKABLE qreal limitY(int index) const;
    Q_INVOKABLE qreal coreCenter(int index) const;
    Q_INVOKABLE qreal slotWidth() const;
    Q_INVOKABLE qreal barWidth() const;
    Q_INVOKABLE qreal plotBottom() const;
    Q_INVOKABLE qreal displayFrequencyNorm(int index) const;
    Q_INVOKABLE qreal displayUsageNorm(int index) const;
    Q_INVOKABLE qreal displayLimitNorm(int index) const;

signals:
    void dataChanged();
    void colorsChanged();
    void highlightChanged();
    void appearanceChanged();
    void animationChanged();
    void paddingChanged();
    void frameTickChanged();

protected:
    void componentComplete() override;
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;
    void itemChange(ItemChange change, const ItemChangeData& value) override;
    void timerEvent(QTimerEvent* event) override;

private:
    QRectF plotRect() const;
    QVector<qreal> numericValues(const QVariantList& values) const;
    void setTargets(QVector<qreal>& targets, QVector<qreal>& display, const QVariantList& values, bool emitDataChanged);
    bool advanceVector(QVector<qreal>& display, const QVector<qreal>& targets, qreal easeMs, qreal dtMs);
    void startAnimation();
    void stopAnimation();
    void onFrame();
    void scheduleUpdate();
    int coreCount() const;
    qreal normalizedAt(const QVector<qreal>& values, int index, qreal fallback = 0.0) const;
    QSGGeometryNode* createGridNode(const QRectF& plot) const;
    QSGGeometryNode* createRoundedRectsNode(const QVector<QRectF>& rects, const QColor& color, qreal radius) const;
    QSGGeometryNode* createTopRoundedRectsNode(const QVector<QRectF>& rects, const QColor& color, qreal radius) const;

    QVariantList m_frequencyNorms;
    QVariantList m_usageNorms;
    QVariantList m_limitNorms;
    QVector<qreal> m_frequencyTargets;
    QVector<qreal> m_usageTargets;
    QVector<qreal> m_limitTargets;
    QVector<qreal> m_displayFrequency;
    QVector<qreal> m_displayUsage;
    QVector<qreal> m_displayLimit;
    QColor m_trackColor = QColor(63, 167, 255, 32);
    QColor m_frequencyColor = QColor(63, 167, 255, 220);
    QColor m_usageColor = QColor(107, 208, 196, 184);
    QColor m_limitColor = QColor(63, 167, 255);
    QColor m_gridColor = QColor(48, 54, 66);
    int m_limitHoverIndex = -1;
    int m_editingIndex = -1;
    int m_animationFps = 30;
    qreal m_telemetryEaseMs = 620.0;
    qreal m_limitEaseMs = 240.0;
    qreal m_dragLimitEaseMs = 45.0;
    qreal m_changeEpsilon = 0.004;
    qreal m_leftPadding = 14.0;
    qreal m_rightPadding = 14.0;
    qreal m_topPadding = 18.0;
    qreal m_bottomPadding = 28.0;
    qreal m_barRadius = 5.0;
    QBasicTimer m_frameTimer;
    QElapsedTimer m_frameClock;
    int m_frameTick = 0;
};
