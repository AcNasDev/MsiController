import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts
import QtQuick.Controls.Fusion
import CurveUtils 1.0

GroupBox {
    Control{
        id: control
    }
    id: root
    // property color accentColor: palette.accentColor
    // property color backgroundColor: palette.backgroundColor
    // property color secondaryTextColor: palette.secondaryTextColor

    property string unit: "%"
    property string tempUnit: "°C"
    property var minTemp: 0
    property var maxTemp: 100
    property var minSpeed: 0
    property var maxSpeed: 150
    signal curveChanged()

    // Properties for curve points
    property int temp1: 0
    property int speed1: 30
    property int temp2: 15
    property int speed2: 40
    property int temp3: 30
    property int speed3: 50
    property int temp4: 45
    property int speed4: 60
    property int temp5: 60
    property int speed5: 70
    property int temp6: 75
    property int speed6: 80
    property int temp7: 90
    property int speed7: 100

    onTemp1Changed: updateSeries()
    onSpeed1Changed: updateSeries()
    onTemp2Changed: updateSeries()
    onSpeed2Changed: updateSeries()
    onTemp3Changed: updateSeries()
    onSpeed3Changed: updateSeries()
    onTemp4Changed: updateSeries()
    onSpeed4Changed: updateSeries()
    onTemp5Changed: updateSeries()
    onSpeed5Changed: updateSeries()
    onTemp6Changed: updateSeries()
    onSpeed6Changed: updateSeries()
    onTemp7Changed: updateSeries()
    onSpeed7Changed: updateSeries()

    // Drag state variables
    property int dragIndex: -1
    property bool isDragging: false

CurveUtils {
    id: curveHelper
}

function fillSplineSeries(series, xValues, yValues) {
    series.clear();
    var segments = 8;
    var n = xValues.length;
    for (var i = 0; i < n - 1; i++) {
        var x0 = i > 0 ? xValues[i - 1] : xValues[i];
        var x1 = xValues[i];
        var x2 = xValues[i + 1];
        var x3 = (i < n - 2) ? xValues[i + 2] : xValues[i + 1];

        var y0 = i > 0 ? yValues[i - 1] : yValues[i];
        var y1 = yValues[i];
        var y2 = yValues[i + 1];
        var y3 = (i < n - 2) ? yValues[i + 2] : yValues[i + 1];

        for (var t = 0; t < segments; t++) {
            var s = t / segments;
            // Catmull-Rom interpolation for X and Y
            var x = 0.5 * (
                (2 * x1) +
                (-x0 + x2) * s +
                (2*x0 - 5*x1 + 4*x2 - x3) * s * s +
                (-x0 + 3*x1 - 3*x2 + x3) * s * s * s
            );
            var y = 0.5 * (
                (2 * y1) +
                (-y0 + y2) * s +
                (2*y0 - 5*y1 + 4*y2 - y3) * s * s +
                (-y0 + 3*y1 - 3*y2 + y3) * s * s * s
            );
            series.append(x, y);
        }
    }
    // Добавить последнюю точку
    series.append(xValues[n - 1], yValues[n - 1]);
}
    // Function to update chart series
    function updateSeries() {
        var xValues = [root.minTemp - (root.maxTemp - root.minTemp) * 0.1, temp1, temp2, temp3, temp4, temp5, temp6, temp7, root.maxTemp + (root.maxTemp - root.minTemp) * 0.1];
        var yValues = [speed1, speed1, speed2, speed3, speed4, speed5, speed6, speed7, speed7];
        // fillSplineSeries(lineSeries, xValues, yValues);
        // fillSplineSeries(areaSeries, xValues, yValues);
        var splinePoints = curveHelper.catmullRomSpline(xValues, yValues, 1);
        lineSeries.clear();
        areaSeries.clear();

        for (var i = 0; i < splinePoints.length; ++i) {
            lineSeries.append(splinePoints[i].x, splinePoints[i].y);
            areaSeries.append(splinePoints[i].x, splinePoints[i].y);
        }

        // lineSeries.clear();
        scatterSeries.clear();

        // Add points in fixed order
        // lineSeries.append(minTemp - maxTemp * 0.1, speed1);
        // lineSeries.append(temp1, speed1);
        // lineSeries.append(temp2, speed2);
        // lineSeries.append(temp3, speed3);
        // lineSeries.append(temp4, speed4);
        // lineSeries.append(temp5, speed5);
        // lineSeries.append(temp6, speed6);
        // lineSeries.append(temp7, speed7);
        // lineSeries.append(maxTemp + maxTemp * 0.1, speed7);

        scatterSeries.append(temp1, speed1);
        scatterSeries.append(temp2, speed2);
        scatterSeries.append(temp3, speed3);
        scatterSeries.append(temp4, speed4);
        scatterSeries.append(temp5, speed5);
        scatterSeries.append(temp6, speed6);
        scatterSeries.append(temp7, speed7);
    }

    // Find the closest point to mouse position
    function findClosestPoint(mouseX, mouseY) {
        var points = [
            {x: temp1, y: speed1},
            {x: temp2, y: speed2},
            {x: temp3, y: speed3},
            {x: temp4, y: speed4},
            {x: temp5, y: speed5},
            {x: temp6, y: speed6},
            {x: temp7, y: speed7}
        ];

        var minDist = Number.MAX_VALUE;
        var closestIndex = -1;

        for (var i = 0; i < points.length; i++) {
            var point = points[i];
            var pixelPoint = chartView.mapToPosition(Qt.point(point.x, point.y), scatterSeries);
            var dx = mouseX - pixelPoint.x;
            var dy = mouseY - pixelPoint.y;
            var dist = Math.sqrt(dx * dx + dy * dy);

            if (dist < minDist && dist < 20) {
                minDist = dist;
                closestIndex = i;
            }
        }

        return closestIndex;
    }

    // Initialize chart
    Component.onCompleted: updateSeries()

    ChartView {
        id: chartView
        anchors.fill: parent
        plotArea: Qt.rect(x, y, width, height)
        
        backgroundColor: "transparent"
        plotAreaColor: "transparent"
        legend.visible: false
        antialiasing: true
        margins { left: 0; top: 0; right: 0; bottom: 0 }
        
        // Mouse interaction
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
         onPressed: {
                var closest = findClosestPoint(mouseX, mouseY);
                if (closest >= 0) {
                    dragIndex = closest;
                    isDragging = true;
                }
            }
         onPositionChanged: {
                if (isDragging && dragIndex >= 0) {
                    var valuePoint = chartView.mapToValue(Qt.point(mouseX, mouseY), scatterSeries);
                 // Constrain values within axes limits
                    var newTemp = Math.min(Math.max(Math.max(axisX.min, Math.min(axisX.max, valuePoint.x)), minTemp), maxTemp);
                    var newSpeed = Math.min(Math.max(Math.max(axisY.min, Math.min(axisY.max, valuePoint.y)), minSpeed), maxSpeed);
                    if(dragIndex === 0) {
                        newTemp = minTemp;
                    }
                 // Round to integers
                    newTemp = newTemp;
                    newSpeed = newSpeed;
                 // Constrain temperature by adjacent points
                    if (dragIndex > 0) {
                        var prevTemp = root["temp" + dragIndex];
                        newTemp = Math.max(newTemp, prevTemp);
                    }
                 if (dragIndex < 6) {
                        var nextTemp = root["temp" + (dragIndex + 2)];
                        newTemp = Math.min(newTemp, nextTemp);
                    }
                 // Update the point
                    switch(dragIndex) {
                    case 0: temp1 = newTemp; speed1 = newSpeed; break;
                    case 1: temp2 = newTemp; speed2 = newSpeed; break;
                    case 2: temp3 = newTemp; speed3 = newSpeed; break;
                    case 3: temp4 = newTemp; speed4 = newSpeed; break;
                    case 4: temp5 = newTemp; speed5 = newSpeed; break;
                    case 5: temp6 = newTemp; speed6 = newSpeed; break;
                    case 6: temp7 = newTemp; speed7 = newSpeed; break;
                    }
                 updateSeries();
                    root.curveChanged();
                }
            }
         onReleased: {
                isDragging = false;
                dragIndex = -1;
            }
        }
     ValueAxis {
            id: axisX
            min: root.minTemp - (root.maxTemp - root.minTemp) * 0.1
            max: root.maxTemp + (root.maxTemp - root.minTemp) * 0.1
            labelsVisible: false
            lineVisible: false
            gridVisible: false
        }
     ValueAxis {
            id: axisY
            min: root.minSpeed - (root.maxSpeed - root.minSpeed) * 0.1
            max: root.maxSpeed + (root.maxSpeed - root.minSpeed) * 0.1
            labelsVisible: false
            lineVisible: false
            gridVisible: false
        }
     LineSeries {
            id: lineSeries
            axisX: axisX
            axisY: axisY
            color: Fusion.highlight(control.palette) 
            width: 2
        }
     ScatterSeries {
            id: scatterSeries
            axisX: axisX
            axisY: axisY
            color:  control.palette.base
            borderColor: Fusion.highlight(control.palette) 
            borderWidth: 2
            markerSize: 10
            pointLabelsFormat: "@yPoint" + unit + " @xPoint" + tempUnit
            pointLabelsVisible: true
            pointLabelsColor: control.palette.text
        }
        AreaSeries {
            axisX: axisX
            axisY: axisY
            color: Qt.rgba(scatterSeries.borderColor.r, scatterSeries.borderColor.g, scatterSeries.borderColor.b, 0.2)
            borderColor: "transparent"
            upperSeries: LineSeries {
                id: areaSeries
                axisX: axisX
                axisY: axisY
                // color: palette.mid
                width: 2
            }
        }
    }

    // Functions to get current values
    function getTemperatures() {
        return [temp1, temp2, temp3, temp4, temp5, temp6, temp7];
    }

    function getFanSpeeds() {
        return [speed1, speed2, speed3, speed4, speed5, speed6, speed7];
    }
}