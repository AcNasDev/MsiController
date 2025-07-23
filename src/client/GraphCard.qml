import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts
import QtQuick.Controls.Fusion

GroupBox {
    id: graphCard

    property int maxHistoryLength: 60
    property bool showLegend: true
    property var nameAxisX: ""
    property var nameAxisY: ""
    property var min: 0
    property var max: 100

    ObjectModel { id: sensorsContainer }
    default property alias sensors: sensorsContainer.children

    QtObject {
        id: internal
        property var sensorValues: ({})
        property real maxYValue: graphCard.max + (graphCard.max - graphCard.min) * 0.1
        property real minYValue: graphCard.min - (graphCard.max - graphCard.min) * 0.1
        property var seriesMap: ({})
    }

    Component.onCompleted: initializeChart()
    function initializeChart() {
        // Удаляем старые серии
        for (var sensorId in internal.seriesMap) {
            chartView.removeSeries(internal.seriesMap[sensorId])
        }
        
        // Сбрасываем данные
        internal.sensorValues = {}
        internal.seriesMap = {}
        internal.maxYValue = graphCard.max + (graphCard.max - graphCard.min) * 0.1
        internal.minYValue = graphCard.min - (graphCard.max - graphCard.min) * 0.1

        // Создаем новые серии
        for (var i = 0; i < sensorsContainer.children.length; i++) {
            var sensor = sensorsContainer.children[i]
            if (!sensor || !sensor.isSensor) continue
            
            // Используем имя сенсора как идентификатор
            var sensorId = sensor.name
            
            internal.sensorValues[sensorId] = []
            
            var series = chartView.createSeries(
                ChartView.SeriesTypeArea, 
                sensor.name, 
                axisX, 
                axisY
            )
            
            series.color = Qt.rgba(sensor.color.r, sensor.color.g, sensor.color.b, 0.1)
            series.borderColor = sensor.color 
            series.width = 2
            series.pointsVisible = false
            series.opacity = 1
            
            internal.seriesMap[sensorId] = series
            
            // Добавляем начальное значение
            addValue(sensorId, sensor.value)
        }
        
        // Принудительно обновляем оси
        axisY.applyNiceNumbers()
        chartView.update()
    }
    
    function addValue(sensorId, value) {
        if (!internal.sensorValues[sensorId]) {
            console.warn("Sensor not found:", sensorId)
            return;
        }

        var values = internal.sensorValues[sensorId]
        var series = internal.seriesMap[sensorId]

        values.push(value)
        while (values.length > maxHistoryLength) {
            values.shift()
        }

        if (series && series.upperSeries) {
            series.upperSeries.clear();
            var segments = 1;
            for (var i = 0; i < values.length - 1; i++) {
                var p0 = i > 0 ? values[i - 1] : values[i];
                var p1 = values[i];
                var p2 = values[i + 1];
                var p3 = (i < values.length - 2) ? values[i + 2] : values[i + 1];

                for (var t = 0; t < segments; t++) {
                    var s = t / segments;
                    var y = 0.5 * (
                        (2 * p1) +
                        (-p0 + p2) * s +
                        (2*p0 - 5*p1 + 4*p2 - p3) * s * s +
                        (-p0 + 3*p1 - 3*p2 + p3) * s * s * s
                    );
                    var x = i + s;
                    series.upperSeries.append(x, y);
                }
            }
            var lastIdx = values.length - 1;
            series.upperSeries.append(lastIdx, values[lastIdx]);
        } else {
            console.warn("Series not ready:", series);
        }
    }
    
    // Таймер для обновления данных
    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: {
            // Обновляем все сенсоры
            for (var i = 0; i < sensorsContainer.children.length; i++) {
                var sensor = sensorsContainer.children[i]
                if (sensor && sensor.isSensor) {
                    addValue(sensor.name, sensor.value)
                }
            }
        }
    }
    
    ColumnLayout {
        anchors.fill: parent
        RowLayout {
            Layout.fillWidth: true
            visible: showLegend && sensorsContainer.children.length > 0
            Repeater {
                model: sensorsContainer.children
                delegate: Row {
                    visible: modelData.isSensor
                    RowLayout {
                        Layout.fillWidth: true
                        Rectangle {
                            width: nameLabel.implicitHeight
                            height: nameLabel.implicitHeight
                            radius: 2
                            color: modelData.color
                        }
                        Label {
                            id: nameLabel
                            color: modelData.color
                            text: modelData.name + ": " + modelData.value.toFixed(1) + modelData.unit
                        }
                    }

                }
            }
        }
        
        ChartView {
            id: chartView
            Layout.fillWidth: true
            Layout.fillHeight: true
            plotArea: Qt.rect(x, y, width, height)
            
            backgroundColor: "transparent"
            plotAreaColor: "transparent"
            legend.visible: false
            antialiasing: true
            margins { left: 0; top: 0; right: 0; bottom: 0 }
            
            ValueAxis {
                id: axisX
                min: 0
                max: maxHistoryLength - 1
                labelsVisible: false
                lineVisible: false
                gridVisible: false
            }

            ValueAxis {
                id: axisY
                min: internal.minYValue
                max: internal.maxYValue
                labelsVisible: false
                lineVisible: false
                gridVisible: false
            }
        }
    }
}