import QtQuick

import QtQuick.Layouts
import QtCharts
import QtQuick.Shapes
import QtQuick.Effects
import QtQuick.Controls.Fusion
import Qt.labs.platform as Platform
import QtQuick.Controls
// import QtQuick.Dialogs

import MsiController 1.0
import Msi 1.0
import MSI.Helpers 1.0


ApplicationWindow {
    id: mainWindow
    width: 800
    height: 600
    minimumWidth: mainContent.implicitWidth
    minimumHeight: mainContent.implicitHeight

    visible: true
    title: "MSI Control Center"

    onClosing: function(close) {
        close.accepted = false;
        mainWindow.hide();
    }
    Platform.SystemTrayIcon {
        visible: true
        icon.source: "qrc:/resources/icon/logo.svg"
        onActivated: {
            mainWindow.show()
            mainWindow.raise()
            mainWindow.requestActivate()
        }
        menu: Platform.Menu {
            Platform.MenuItem {
                text: qsTr("Exit")
                onTriggered: Qt.quit()
            }
        }
    }
    
    EsProxy {
        id: proxy
    }
    Dialog {
        id: aboutDialog
        title: qsTr("About")
        standardButtons: Dialog.Ok
        modal: true
        Label {
            textFormat: Text.RichText
            text: "<b>MSI Control Center</b><br>Version " + appversion + "<br>Author: AcNas<br><a href='https://acnas.net'>https://acnas.net</a><br>Qt version: " + qtversion
            color: palette.text
            wrapMode: Text.WordWrap
            onLinkActivated: Qt.openUrlExternally(link)
        }
    }
    menuBar : MenuBar{
        Menu {
            title: qsTr("File")
            MenuItem {
                text: qsTr("About")
                onTriggered: aboutDialog.open()
            }
            MenuItem {
                text: qsTr("Exit")
                onTriggered: Qt.quit()
            }
        }
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            Image {
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                source: "qrc:/resources/icon/logo.png"
                smooth: true
                antialiasing: true
                layer.enabled: true
                layer.effect: MultiEffect {
                    brightness: 0.05
                    colorization: 1.0
                    colorizationColor: palette.highlight
                }
            }
            Label {
                Layout.alignment: Qt.AlignVCenter
                text: "MSI CONTROL CENTER"
                font.pixelSize: 24
                font.bold: true
                color: Fusion.highlight(palette)
            }
            Item{ Layout.fillWidth: true }
            Rectangle {
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: parent.height / 4
                Layout.preferredHeight: parent.height / 4
                radius: height / 2
                color: proxy.isConnected ? Fusion.highlight(palette) : Fusion.buttonColor(palette, false, true, true)
            }
            Label {
                text: proxy.isConnected ? qsTr("CONNECTED") : qsTr("DISCONNECTED")
            }
            
        }
    }

    footer: ToolBar {
        RowLayout {
            anchors.fill: parent
            Label {
                property var fve : proxy.getProxyParameter(Msi.Parametr.FirmwareVersionEc)
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Firmware Version: ") + (fve.isValid ? fve.value : "N/A")
            }
            Label {
                function formatDate(val) {
                    var s = val.toString();
                    if (s.length !== 8) return val;
                    var month = s.substring(0,2);
                    var day = s.substring(2,4);
                    var year = s.substring(4,8);
                    var dateObj = new Date(year, month - 1, day);
                    return dateObj.toLocaleDateString(Locale.ShortFormat);
                }
                property var frd : proxy.getProxyParameter(Msi.Parametr.FirmwareReleaseDateEc)
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Date: ") + (frd.isValid ? formatDate(frd.value) : "N/A")
            }
            Label {
                function formatTime(val) {
                    var s = val.toString();
                    var parts = s.split(":");
                    if (parts.length !== 3) return null;
                    var hour = parseInt(parts[0]);
                    var min = parseInt(parts[1]);
                    var sec = parseInt(parts[2]);
                    var timeObj = new Date(0, 0, 0, hour, min, sec);
                    return timeObj.toLocaleTimeString(Locale.LongFormat);
                }
                property var frt : proxy.getProxyParameter(Msi.Parametr.FirmwareReleaseTimeEc)
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Time: ") + (frt.isValid ? formatTime(frt.value) : "N/A")
            }
            Item{ Layout.fillWidth: true }
            Label {
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Battery:")
            }
            Item {
                id: batteryIcon
                property var bcec: proxy.getProxyParameter(Msi.Parametr.BatteryChargeEc);
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: parent.height / 2.5
                Layout.preferredHeight: parent.height / 1.5
                Rectangle {
                    anchors.bottom: parent.bottom
                    radius: 0
                    height: parent.height * (parent.bcec.isValid ? (parent.bcec.value) / 100 : 0)
                    width: parent.width
                    color: Fusion.highlight(palette) 
                    Image {
                        id: image
                        property var bcsc: proxy.getProxyParameter(Msi.Parametr.BatteryChargingStatusEc);
                        width: parent.height + 4
                        height: parent.height + 4
                        x: (parent.width - width) / 2
                        y: (parent.height - height) / 2
                        source: {
                            switch (bcsc.value) {
                                case Msi.ChargingStatus.BatteryCharging:
                                    return "qrc:/resources/icon/charging.svg";
                                case Msi.ChargingStatus.BatteryDischarging:
                                    return "qrc:/resources/icon/discharging.svg";
                                case Msi.ChargingStatus.BatteryNotCharging:
                                    return "qrc:/resources/icon/notcharging.svg";
                                case Msi.ChargingStatus.BatteryFullyCharged:
                                    return "qrc:/resources/icon/fullycharged.svg";
                                case Msi.ChargingStatus.BatteryFullyChargedNoPower:
                                    return "qrc:/resources/icon/fullychargednopower.svg";
                                default:
                                    return "qrc:/resources/icon/unknown.svg";
                            }
                        }
                        layer.enabled: true
                        layer.effect: MultiEffect {
                            brightness: .0
                            colorization: 1.0
                            colorizationColor: Qt.darker(palette.accent, 1.8)
                        }
                    }
                }
            }
        }
    }

    ColumnLayout {
        id: mainContent
        anchors.fill: parent
        GridLayout {
            columns: 2
            GraphCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 100
                Layout.preferredHeight: 100
                title: "Temperature"
                visible: proxy.getProxyParameter(Msi.Parametr.CpuTempEc).isValid || 
                        proxy.getProxyParameter(Msi.Parametr.GpuTempEc).isValid
                Sensor {
                    name: "CPU"
                    color: Fusion.highlight(palette)
                    unit: "°C"
                    value: proxy.getProxyParameter(Msi.Parametr.CpuTempEc).value || 0
                    visible: proxy.getProxyParameter(Msi.Parametr.CpuTempEc).isValid
                }
                Sensor {
                    name: "GPU"
                    color: Qt.lighter(Fusion.highlight(palette), 1.5)
                    unit: "°C"
                    value: proxy.getProxyParameter(Msi.Parametr.GpuTempEc).value || 0
                    visible: proxy.getProxyParameter(Msi.Parametr.GpuTempEc).isValid
                }
                min: 0
                max: 100
            }
            GraphCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 100
                Layout.preferredHeight: 100
                title: "Fan Speed"
                visible: proxy.getProxyParameter(Msi.Parametr.FanCpuEc).isValid || 
                        proxy.getProxyParameter(Msi.Parametr.FanGpuEc).isValid
                Sensor {
                    name: "CPU"
                    color: Fusion.highlight(palette)
                    unit: "%"
                    value: proxy.getProxyParameter(Msi.Parametr.FanCpuEc).value || 0
                    visible: proxy.getProxyParameter(Msi.Parametr.FanCpuEc).isValid
                }

                Sensor {
                    name: "GPU"
                    color: Qt.lighter(Fusion.highlight(palette), 1.5)
                    unit: "%"
                    value: proxy.getProxyParameter(Msi.Parametr.FanGpuEc).value || 0
                    visible: proxy.getProxyParameter(Msi.Parametr.FanGpuEc).isValid
                }
                min: 0
                max: 150
            }
            FanCurveCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 100
                Layout.preferredHeight: 100
                id: fanCurveCpu
                property var fssg1: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedCpu1Ec)
                property var fssg2: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedCpu2Ec)
                property var fssg3: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedCpu3Ec)
                property var fssg4: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedCpu4Ec)
                property var fssg5: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedCpu5Ec)
                property var fssg6: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedCpu6Ec)
                property var fssg7: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedCpu7Ec)

                property var fstg1: proxy.getProxyParameter(Msi.Parametr.FanSetTempCpu1Ec)
                property var fstg2: proxy.getProxyParameter(Msi.Parametr.FanSetTempCpu2Ec)
                property var fstg3: proxy.getProxyParameter(Msi.Parametr.FanSetTempCpu3Ec)
                property var fstg4: proxy.getProxyParameter(Msi.Parametr.FanSetTempCpu4Ec)
                property var fstg5: proxy.getProxyParameter(Msi.Parametr.FanSetTempCpu5Ec)
                property var fstg6: proxy.getProxyParameter(Msi.Parametr.FanSetTempCpu6Ec)

                property var isValid: fssg1.isValid && 
                                      fssg2.isValid && 
                                      fssg3.isValid && 
                                      fssg4.isValid && 
                                      fssg5.isValid && 
                                      fssg6.isValid && 
                                      fssg7.isValid && 
                                      fstg1.isValid && 
                                      fstg2.isValid && 
                                      fstg3.isValid && 
                                      fstg4.isValid && 
                                      fstg5.isValid && 
                                      fstg6.isValid

                temp1: 0
                temp2: isValid ? fstg1.value || 0 : 0
                temp3: isValid ? fstg2.value || 0 : 0
                temp4: isValid ? fstg3.value || 0 : 0
                temp5: isValid ? fstg4.value || 0 : 0
                temp6: isValid ? fstg5.value || 0 : 0
                temp7: isValid ? fstg6.value || 0 : 0

                speed1: isValid ? fssg1.value || 0 : 0
                speed2: isValid ? fssg2.value || 0 : 0
                speed3: isValid ? fssg3.value || 0 : 0
                speed4: isValid ? fssg4.value || 0 : 0
                speed5: isValid ? fssg5.value || 0 : 0
                speed6: isValid ? fssg6.value || 0 : 0
                speed7: isValid ? fssg7.value || 0 : 0

                onTemp2Changed: { if(isValid) fstg1.value = temp2; }
                onTemp3Changed: { if(isValid) fstg2.value = temp3; }
                onTemp4Changed: { if(isValid) fstg3.value = temp4; }
                onTemp5Changed: { if(isValid) fstg4.value = temp5; }
                onTemp6Changed: { if(isValid) fstg5.value = temp6; }
                onTemp7Changed: { if(isValid) fstg6.value = temp7; }

                onSpeed1Changed: { if(isValid) fssg1.value = speed1; }
                onSpeed2Changed: { if(isValid) fssg2.value = speed2; }
                onSpeed3Changed: { if(isValid) fssg3.value = speed3; }
                onSpeed4Changed: { if(isValid) fssg4.value = speed4; }
                onSpeed5Changed: { if(isValid) fssg5.value = speed5; }
                onSpeed6Changed: { if(isValid) fssg6.value = speed6; }
                onSpeed7Changed: { if(isValid) fssg7.value = speed7; }

                title: "Fan Curve GPU"
                visible: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedCpu1Ec).isValid
                minTemp: fanCurveGpu.fstg1.isValid ? fanCurveGpu.fstg1.availableValues.min : 0
                maxTemp: fanCurveGpu.fstg1.isValid ? fanCurveGpu.fstg1.availableValues.max : 100
                minSpeed: fanCurveGpu.fssg1.isValid ? fanCurveGpu.fssg1.availableValues.min : 0
                maxSpeed: fanCurveGpu.fssg1.isValid ? fanCurveGpu.fssg1.availableValues.max : 150
            }
            FanCurveCard {
                id: fanCurveGpu
                property var fssg1: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedGpu1Ec)
                property var fssg2: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedGpu2Ec)
                property var fssg3: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedGpu3Ec)
                property var fssg4: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedGpu4Ec)
                property var fssg5: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedGpu5Ec)
                property var fssg6: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedGpu6Ec)
                property var fssg7: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedGpu7Ec)

                property var fstg1: proxy.getProxyParameter(Msi.Parametr.FanSetTempGpu1Ec)
                property var fstg2: proxy.getProxyParameter(Msi.Parametr.FanSetTempGpu2Ec)
                property var fstg3: proxy.getProxyParameter(Msi.Parametr.FanSetTempGpu3Ec)
                property var fstg4: proxy.getProxyParameter(Msi.Parametr.FanSetTempGpu4Ec)
                property var fstg5: proxy.getProxyParameter(Msi.Parametr.FanSetTempGpu5Ec)
                property var fstg6: proxy.getProxyParameter(Msi.Parametr.FanSetTempGpu6Ec)

                property var isValid: fssg1.isValid && 
                                      fssg2.isValid && 
                                      fssg3.isValid && 
                                      fssg4.isValid && 
                                      fssg5.isValid && 
                                      fssg6.isValid && 
                                      fssg7.isValid && 
                                      fstg1.isValid && 
                                      fstg2.isValid && 
                                      fstg3.isValid && 
                                      fstg4.isValid && 
                                      fstg5.isValid && 
                                      fstg6.isValid

                temp1: 0
                temp2: isValid ? fstg1.value || 0 : 0
                temp3: isValid ? fstg2.value || 0 : 0
                temp4: isValid ? fstg3.value || 0 : 0
                temp5: isValid ? fstg4.value || 0 : 0
                temp6: isValid ? fstg5.value || 0 : 0
                temp7: isValid ? fstg6.value || 0 : 0

                speed1: isValid ? fssg1.value || 0 : 0
                speed2: isValid ? fssg2.value || 0 : 0
                speed3: isValid ? fssg3.value || 0 : 0
                speed4: isValid ? fssg4.value || 0 : 0
                speed5: isValid ? fssg5.value || 0 : 0
                speed6: isValid ? fssg6.value || 0 : 0
                speed7: isValid ? fssg7.value || 0 : 0

                onTemp2Changed: { if(isValid) fstg1.value = temp2; }
                onTemp3Changed: { if(isValid) fstg2.value = temp3; }
                onTemp4Changed: { if(isValid) fstg3.value = temp4; }
                onTemp5Changed: { if(isValid) fstg4.value = temp5; }
                onTemp6Changed: { if(isValid) fstg5.value = temp6; }
                onTemp7Changed: { if(isValid) fstg6.value = temp7; }

                onSpeed1Changed: { if(isValid) fssg1.value = speed1; }
                onSpeed2Changed: { if(isValid) fssg2.value = speed2; }
                onSpeed3Changed: { if(isValid) fssg3.value = speed3; }
                onSpeed4Changed: { if(isValid) fssg4.value = speed4; }
                onSpeed5Changed: { if(isValid) fssg5.value = speed5; }
                onSpeed6Changed: { if(isValid) fssg6.value = speed6; }
                onSpeed7Changed: { if(isValid) fssg7.value = speed7; }

                title: "Fan Curve GPU"
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedCpu1Ec).isValid
                minTemp: fanCurveGpu.fstg1.isValid ? fanCurveGpu.fstg1.availableValues.min : 0
                maxTemp: fanCurveGpu.fstg1.isValid ? fanCurveGpu.fstg1.availableValues.max : 100
                minSpeed: fanCurveGpu.fssg1.isValid ? fanCurveGpu.fssg1.availableValues.min : 0
                maxSpeed: fanCurveGpu.fssg1.isValid ? fanCurveGpu.fssg1.availableValues.max : 150
            }
        }
        GridLayout {
            columns: 3
            Switch {
                id: scollerBoost
                Layout.fillWidth: true
                text: "Cooler Boost"
                property var sb : proxy.getProxyParameter(Msi.Parametr.CoolerBoostEc)
                visible: sb.isValid
                    checked: scollerBoost.sb.value || checked
                    onCheckedChanged: {
                        scollerBoost.sb.value = scollerBoost.sb.availableValues[checked ? 1 : 0];
                }
            }
            Switch {
                id: usbPowerShare
                Layout.fillWidth: true
                text: "USB Power"
                property var ps : proxy.getProxyParameter(Msi.Parametr.UsbPowerShareEc)
                visible: ps.isValid
                checked: usbPowerShare.ps.value || checked
                onCheckedChanged: {
                    usbPowerShare.ps.value = usbPowerShare.ps.availableValues[checked ? 1 : 0];
                }
            }
            Switch {
                id: webCam
                Layout.fillWidth: true
                text: "WebCam"
                property var wc : proxy.getProxyParameter(Msi.Parametr.WebCamEc)
                visible: wc.isValid
                checked: webCam.wc.value || checked
                onCheckedChanged: {
                    webCam.wc.value = webCam.wc.availableValues[checked ? 1 : 0];
                }
            }
            Switch {
                Layout.fillWidth: true
                id: fnSuperSwap
                text: "FN ⇄ Meta"
                property var fss : proxy.getProxyParameter(Msi.Parametr.FnSuperSwapEc)
                visible: fss.isValid
                checked: fnSuperSwap.fss.value || checked
                onCheckedChanged: {
                    fnSuperSwap.fss.value = fnSuperSwap.fss.availableValues[checked ? 1 : 0];
                }
            }
            Switch {
                Layout.fillWidth: true
                id: webCamBlock
                text: "WebCam Block"
                property var wcb : proxy.getProxyParameter(Msi.Parametr.WebCamBlockEc)
                visible: wcb.isValid
                checked: webCamBlock.wcb.value || checked
                onCheckedChanged: {
                    webCamBlock.wcb.value = webCamBlock.wcb.availableValues[checked ? 1 : 0];
                }
            }
            Switch {
                Layout.fillWidth: true
                id: superBattery
                text: "Super Battery"
                property var sb : proxy.getProxyParameter(Msi.Parametr.SuperBatteryEc)
                visible: sb.isValid
                checked: superBattery.sb.value || checked
                onCheckedChanged: {
                    superBattery.sb.value = superBattery.sb.availableValues[checked ? 1 : 0];
                }
            }
            Switch {
                Layout.fillWidth: true
                id: micMute
                text: "Mic Mute"
                property var mm : proxy.getProxyParameter(Msi.Parametr.MicMuteEc)
                visible: mm.isValid
                checked: micMute.mm.value || checked
                onCheckedChanged: {
                    micMute.mm.value = micMute.mm.availableValues[checked ? 1 : 0];
                }
            }
            Switch {
                Layout.fillWidth: true
                id: muteLed
                text: "Mute LED"
                property var ml : proxy.getProxyParameter(Msi.Parametr.MuteLedEc)
                visible: ml.isValid
                checked: muteLed.ml.value || checked
                onCheckedChanged: {
                    muteLed.ml.value = muteLed.ml.availableValues[checked ? 1 : 0];
                }
            }
            Switch {
                Layout.fillWidth: true
                id: keyboardBacklightMode
                text: "Key Mode"
                property var kbm : proxy.getProxyParameter(Msi.Parametr.KeyboardBacklightModeEc)
                visible: kbm.isValid
                checked: keyboardBacklightMode.kbm.value || checked
                onCheckedChanged: {
                    keyboardBacklightMode.kbm.value = keyboardBacklightMode.kbm.availableValues[checked ? 1 : 0];
                }
            }

        }
        GridLayout {
            columns: 2
            rows: 2
            property var minimalWidth: 0
            GroupBox {
                id: shiftMode
                title: "Shift Mode"
                property var sm: proxy.getProxyParameter(Msi.Parametr.ShiftModeEc)
                visible: sm.isValid
                Layout.fillWidth: true
                ButtonGroup { 
                    id: shiftModeGroup 
                }
                RowLayout {
                    anchors.centerIn: parent
                    Repeater {
                        model: shiftMode.sm.availableValues
                        RadioButton {
                            text: EnumHelper.enumToString(modelData, "ShiftMode") || "N/A"
                            checked: shiftMode.sm.value === modelData
                            ButtonGroup.group: shiftModeGroup
                            onClicked: {
                                if (shiftMode.sm.isValid)
                                    shiftMode.sm.value = modelData;
                            }
                        }
                    }
                }
            }
            GroupBox {
                Layout.fillWidth: true
                ButtonGroup { 
                    id:fanModeGroup 
                }
                id: fanMode
                title: "Fan Mode"
                property var sm: proxy.getProxyParameter(Msi.Parametr.FanModeEc)
                visible: sm.isValid
                RowLayout {
                    anchors.centerIn: parent
                    Repeater {
                        model: fanMode.sm.availableValues
                        RadioButton {
                            text: EnumHelper.enumToString(modelData, "FanMode") || "N/A"
                            checked: fanMode.sm.value === modelData
                            ButtonGroup.group: fanModeGroup
                            onClicked: {
                                if (fanMode.sm.isValid)
                                    fanMode.sm.value = modelData;
                            }
                        }
                    }
                }
            }

            GroupBox {
                Layout.fillWidth: true
                ButtonGroup { 
                    id: keyboardBacklightGroup 
                }
                id: keyboardBacklight
                title: "Keyboard Backlight"
                property var kb: proxy.getProxyParameter(Msi.Parametr.KeyboardBacklightEc)
                visible: kb.isValid
                RowLayout {
                    anchors.centerIn: parent
                    Repeater {
                        model: keyboardBacklight.kb.availableValues
                        RadioButton {
                            text: EnumHelper.enumToString(modelData, "KeyboardBacklight") || "N/A"
                            checked: keyboardBacklight.kb.value === modelData
                            ButtonGroup.group: keyboardBacklightGroup
                            onClicked: {
                                if (keyboardBacklight.kb.isValid)
                                    keyboardBacklight.kb.value = modelData;
                            }
                        }
                    }
                }
            }
            GroupBox {
                id: batteryThresholdGroup
                Layout.fillWidth: true
                title: "Battery Threshold"
                property var bt: proxy.getProxyParameter(Msi.Parametr.BatteryThresholdEc)
                visible: bt.isValid
                RowLayout {
                    anchors.centerIn: parent
                    Slider {
                        Layout.fillWidth: true
                        id: batteryThresholdSlider
                        from: 0
                        to: 100
                        stepSize: 1
                        value: batteryThresholdGroup.bt.value || 0
                        onValueChanged: {
                            batteryThresholdGroup.bt.value = value;
                        }
                    }
                    Label {
                        Layout.fillWidth: true
                        text: (batteryThresholdGroup.bt.value || 0) + "%"
                    }
                }

            }
        }
    }
}