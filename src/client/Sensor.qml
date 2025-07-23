import QtQuick 2.15

QtObject {
    // Маркер для идентификации сенсоров
    readonly property bool isSensor: true
    
    property string id: ""
    property string name: ""
    property real value: 0
    property color color: "white"
    property string unit: ""
    property var visible: true
}