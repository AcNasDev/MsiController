import QtQuick

import QtQuick.Layouts
import QtCharts
import QtQuick.Shapes
import QtQuick.Effects
import QtQuick.Controls.Fusion
import Qt.labs.platform as Platform
import QtCore 6.5 as QtCore
import Qt.labs.settings 1.1
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

    // --- Theme logic ---
    QtCore.Settings {
        id: appSettings
        property string savedTheme: "darkplus"
    }

    property string currentTheme: appSettings.savedTheme
    property var palettes: ({
        darkplus: { // VSCode Dark+
            window: "#1e1e1e",
            windowText: "#d4d4d4",
            base: "#1e1e1e",
            alternateBase: "#252526",
            text: "#d4d4d4",
            button: "#2d2d2d",
            buttonText: "#d4d4d4",
            highlight: "#007acc",
            highlightedText: "#ffffff",
            link: "#3794ff",
            linkVisited: "#c586c0",
            toolTipBase: "#252526",
            toolTipText: "#d4d4d4",
            placeholderText: "#6a9955",
            accent: "#007acc",
            mid: "#333333",
            shadow: "#00000080",
            brightText: "#ffffff",
            light: "#3c3c3c",
            dark: "#1e1e1e"
        },
        lightplus: { // VSCode Light+
            window: "#ffffff",
            windowText: "#333333",
            base: "#ffffff",
            alternateBase: "#f3f3f3",
            text: "#333333",
            button: "#e7e7e7",
            buttonText: "#333333",
            highlight: "#0066b8",
            highlightedText: "#ffffff",
            link: "#006ab1",
            linkVisited: "#a259c4",
            toolTipBase: "#f3f3f3",
            toolTipText: "#333333",
            placeholderText: "#a6a6a6",
            accent: "#0066b8",
            mid: "#cccccc",
            shadow: "#00000020",
            brightText: "#222222",
            light: "#eaeaea",
            dark: "#cccccc"
        },
        solarized_dark: {
            window: "#002b36",
            windowText: "#839496",
            base: "#073642",
            alternateBase: "#002b36",
            text: "#839496",
            button: "#073642",
            buttonText: "#839496",
            highlight: "#268bd2",
            highlightedText: "#fdf6e3",
            link: "#268bd2",
            linkVisited: "#6c71c4",
            toolTipBase: "#073642",
            toolTipText: "#839496",
            placeholderText: "#586e75",
            accent: "#b58900",
            mid: "#586e75",
            shadow: "#00000080",
            brightText: "#fdf6e3",
            light: "#586e75",
            dark: "#002b36"
        },
        solarized_light: {
            window: "#fdf6e3",
            windowText: "#657b83",
            base: "#fdf6e3",
            alternateBase: "#eee8d5",
            text: "#657b83",
            button: "#eee8d5",
            buttonText: "#657b83",
            highlight: "#268bd2",
            highlightedText: "#002b36",
            link: "#268bd2",
            linkVisited: "#6c71c4",
            toolTipBase: "#eee8d5",
            toolTipText: "#657b83",
            placeholderText: "#93a1a1",
            accent: "#b58900",
            mid: "#93a1a1",
            shadow: "#00000020",
            brightText: "#002b36",
            light: "#eee8d5",
            dark: "#93a1a1"
        },
        monokai: {
            window: "#272822",
            windowText: "#f8f8f2",
            base: "#272822",
            alternateBase: "#383830",
            text: "#f8f8f2",
            button: "#49483e",
            buttonText: "#f8f8f2",
            highlight: "#66d9ef",
            highlightedText: "#272822",
            link: "#a6e22e",
            linkVisited: "#f92672",
            toolTipBase: "#49483e",
            toolTipText: "#f8f8f2",
            placeholderText: "#75715e",
            accent: "#fd971f",
            mid: "#75715e",
            shadow: "#00000080",
            brightText: "#f8f8f2",
            light: "#49483e",
            dark: "#272822"
        },
        dracula: {
            window: "#282a36",
            windowText: "#f8f8f2",
            base: "#282a36",
            alternateBase: "#44475a",
            text: "#f8f8f2",
            button: "#44475a",
            buttonText: "#f8f8f2",
            highlight: "#bd93f9",
            highlightedText: "#282a36",
            link: "#8be9fd",
            linkVisited: "#ff79c6",
            toolTipBase: "#44475a",
            toolTipText: "#f8f8f2",
            placeholderText: "#6272a4",
            accent: "#50fa7b",
            mid: "#6272a4",
            shadow: "#00000080",
            brightText: "#f8f8f2",
            light: "#44475a",
            dark: "#282a36"
        },
        darcula: { // JetBrains Darcula
            window: "#2b2b2b",
            windowText: "#a9b7c6",
            base: "#3c3f41",
            alternateBase: "#313335",
            text: "#a9b7c6",
            button: "#4e5254",
            buttonText: "#a9b7c6",
            highlight: "#287bde",
            highlightedText: "#ffffff",
            link: "#287bde",
            linkVisited: "#bc3fbc",
            toolTipBase: "#313335",
            toolTipText: "#a9b7c6",
            placeholderText: "#606366",
            accent: "#ffc66d",
            mid: "#606366",
            shadow: "#00000080",
            brightText: "#ffffff",
            light: "#4e5254",
            dark: "#2b2b2b"
        },
        atomonedark: { // Atom One Dark
            window: "#282c34",
            windowText: "#abb2bf",
            base: "#21252b",
            alternateBase: "#282c34",
            text: "#abb2bf",
            button: "#3e4451",
            buttonText: "#abb2bf",
            highlight: "#61afef",
            highlightedText: "#282c34",
            link: "#56b6c2",
            linkVisited: "#c678dd",
            toolTipBase: "#21252b",
            toolTipText: "#abb2bf",
            placeholderText: "#5c6370",
            accent: "#98c379",
            mid: "#5c6370",
            shadow: "#00000080",
            brightText: "#ffffff",
            light: "#3e4451",
            dark: "#282c34"
        },
        sublime: { // Sublime Text
            window: "#23241f",
            windowText: "#f8f8f2",
            base: "#272822",
            alternateBase: "#383830",
            text: "#f8f8f2",
            button: "#75715e",
            buttonText: "#f8f8f2",
            highlight: "#fd971f",
            highlightedText: "#23241f",
            link: "#a6e22e",
            linkVisited: "#f92672",
            toolTipBase: "#383830",
            toolTipText: "#f8f8f2",
            placeholderText: "#75715e",
            accent: "#66d9ef",
            mid: "#75715e",
            shadow: "#00000080",
            brightText: "#f8f8f2",
            light: "#49483e",
            dark: "#23241f"
        },
        eclipse: { // Eclipse IDE
            window: "#323232",
            windowText: "#dcdcdc",
            base: "#232323",
            alternateBase: "#323232",
            text: "#dcdcdc",
            button: "#464646",
            buttonText: "#dcdcdc",
            highlight: "#6a9fb5",
            highlightedText: "#232323",
            link: "#519aba",
            linkVisited: "#b294bb",
            toolTipBase: "#232323",
            toolTipText: "#dcdcdc",
            placeholderText: "#888888",
            accent: "#b5bd68",
            mid: "#888888",
            shadow: "#00000080",
            brightText: "#ffffff",
            light: "#464646",
            dark: "#232323"
        },
        xcode_light: { // Xcode Light
            window: "#ffffff",
            windowText: "#000000",
            base: "#f7f7f7",
            alternateBase: "#e9e9e9",
            text: "#000000",
            button: "#e9e9e9",
            buttonText: "#000000",
            highlight: "#007aff",
            highlightedText: "#ffffff",
            link: "#007aff",
            linkVisited: "#5856d6",
            toolTipBase: "#f7f7f7",
            toolTipText: "#000000",
            placeholderText: "#b0b0b0",
            accent: "#34c759",
            mid: "#cccccc",
            shadow: "#00000020",
            brightText: "#000000",
            light: "#e9e9e9",
            dark: "#cccccc"
        },
        xcode_dark: { // Xcode Dark
            window: "#1e1e1e",
            windowText: "#d4d4d4",
            base: "#202124",
            alternateBase: "#232326",
            text: "#d4d4d4",
            button: "#232326",
            buttonText: "#d4d4d4",
            highlight: "#0a84ff",
            highlightedText: "#ffffff",
            link: "#0a84ff",
            linkVisited: "#5e5ce6",
            toolTipBase: "#232326",
            toolTipText: "#d4d4d4",
            placeholderText: "#6e6e6e",
            accent: "#30d158",
            mid: "#333333",
            shadow: "#00000080",
            brightText: "#ffffff",
            light: "#232326",
            dark: "#1e1e1e"
        },
        notepadpp: { // Notepad++
            window: "#ffffff",
            windowText: "#000000",
            base: "#f5f5f5",
            alternateBase: "#e0e0e0",
            text: "#000000",
            button: "#e0e0e0",
            buttonText: "#000000",
            highlight: "#3399ff",
            highlightedText: "#ffffff",
            link: "#3399ff",
            linkVisited: "#cc99cc",
            toolTipBase: "#f5f5f5",
            toolTipText: "#000000",
            placeholderText: "#888888",
            accent: "#66cc66",
            mid: "#cccccc",
            shadow: "#00000020",
            brightText: "#000000",
            light: "#e0e0e0",
            dark: "#cccccc"
        },
        highcontrast_dark: { // Windows High Contrast Black
            window: "#000000",
            windowText: "#ffffff",
            base: "#000000",
            alternateBase: "#1a1a1a",
            text: "#ffffff",
            button: "#000000",
            buttonText: "#ffffff",
            highlight: "#ffff00",
            highlightedText: "#000000",
            link: "#00ffff",
            linkVisited: "#ff00ff",
            toolTipBase: "#000000",
            toolTipText: "#ffffff",
            placeholderText: "#ffffff",
            accent: "#ffff00",
            mid: "#333333",
            shadow: "#00000080",
            brightText: "#ffffff",
            light: "#1a1a1a",
            dark: "#000000"
        },
        highcontrast_light: { // Windows High Contrast White
            window: "#ffffff",
            windowText: "#000000",
            base: "#ffffff",
            alternateBase: "#e5e5e5",
            text: "#000000",
            button: "#ffffff",
            buttonText: "#000000",
            highlight: "#0000ff",
            highlightedText: "#ffffff",
            link: "#0000ff",
            linkVisited: "#ff00ff",
            toolTipBase: "#ffffff",
            toolTipText: "#000000",
            placeholderText: "#000000",
            accent: "#0000ff",
            mid: "#cccccc",
            shadow: "#00000020",
            brightText: "#000000",
            light: "#e5e5e5",
            dark: "#cccccc"
        },
        nord_contrast: { // Nord Contrast
            window: "#2e3440",
            windowText: "#eceff4",
            base: "#3b4252",
            alternateBase: "#434c5e",
            text: "#eceff4",
            button: "#4c566a",
            buttonText: "#eceff4",
            highlight: "#88c0d0",
            highlightedText: "#2e3440",
            link: "#8fbcbb",
            linkVisited: "#b48ead",
            toolTipBase: "#434c5e",
            toolTipText: "#eceff4",
            placeholderText: "#d8dee9",
            accent: "#a3be8c",
            mid: "#4c566a",
            shadow: "#00000080",
            brightText: "#eceff4",
            light: "#434c5e",
            dark: "#2e3440"
        },
        solarized_highcontrast: { // Solarized High Contrast
            window: "#002b36",
            windowText: "#ffffff",
            base: "#073642",
            alternateBase: "#002b36",
            text: "#ffffff",
            button: "#073642",
            buttonText: "#ffffff",
            highlight: "#ffb700",
            highlightedText: "#002b36",
            link: "#268bd2",
            linkVisited: "#d33682",
            toolTipBase: "#073642",
            toolTipText: "#ffffff",
            placeholderText: "#b58900",
            accent: "#ffb700",
            mid: "#586e75",
            shadow: "#00000080",
            brightText: "#ffffff",
            light: "#586e75",
            dark: "#002b36"
        }
    })

    function applyPalette(theme) {
        var p = palettes[theme];
        if (!p) return;
        mainWindow.palette.window = p.window
        mainWindow.palette.windowText = p.windowText
        mainWindow.palette.base = p.base
        mainWindow.palette.alternateBase = p.alternateBase
        mainWindow.palette.text = p.text
        mainWindow.palette.button = p.button
        mainWindow.palette.buttonText = p.buttonText
        mainWindow.palette.highlight = p.highlight
        mainWindow.palette.highlightedText = p.highlightedText
        mainWindow.palette.link = p.link
        mainWindow.palette.linkVisited = p.linkVisited
        mainWindow.palette.toolTipBase = p.toolTipBase
        mainWindow.palette.toolTipText = p.toolTipText
        mainWindow.palette.placeholderText = p.placeholderText
        mainWindow.palette.accent = p.accent
        mainWindow.palette.mid = p.mid
        mainWindow.palette.shadow = p.shadow
        mainWindow.palette.brightText = p.brightText
        mainWindow.palette.light = p.light
        mainWindow.palette.dark = p.dark
    }

    palette: Palette {
        property var themePalette: palettes[currentTheme] ? palettes[currentTheme] : palettes["darkplus"]
        window: themePalette.window
        windowText: themePalette.windowText
        base: themePalette.base
        alternateBase: themePalette.alternateBase
        text: themePalette.text
        button: themePalette.button
        buttonText: themePalette.buttonText
        highlight: themePalette.highlight
        highlightedText: themePalette.highlightedText
        link: themePalette.link
        linkVisited: themePalette.linkVisited
        toolTipBase: themePalette.toolTipBase
        toolTipText: themePalette.toolTipText
        placeholderText: themePalette.placeholderText
        accent: themePalette.accent
        mid: themePalette.mid
        shadow: themePalette.shadow
        brightText: themePalette.brightText
        light: themePalette.light
        dark: themePalette.dark
    }

    onCurrentThemeChanged: {
        applyPalette(currentTheme)
        appSettings.savedTheme = currentTheme
    }
    Component.onCompleted: {
        // ensure palette is applied on startup
        applyPalette(currentTheme)
    }

    // icon: "qrc:/resources/icon/logo.png"

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
        Menu {
            title: qsTr("Theme")
            MenuItem {
                text: qsTr("VSCode Dark+")
                checkable: true
                checked: currentTheme === "darkplus"
                onTriggered: currentTheme = "darkplus"
            }
            MenuItem {
                text: qsTr("VSCode Light+")
                checkable: true
                checked: currentTheme === "lightplus"
                onTriggered: currentTheme = "lightplus"
            }
            MenuItem {
                text: qsTr("Solarized Dark")
                checkable: true
                checked: currentTheme === "solarized_dark"
                onTriggered: currentTheme = "solarized_dark"
            }
            MenuItem {
                text: qsTr("Solarized Light")
                checkable: true
                checked: currentTheme === "solarized_light"
                onTriggered: currentTheme = "solarized_light"
            }
            MenuItem {
                text: qsTr("Monokai")
                checkable: true
                checked: currentTheme === "monokai"
                onTriggered: currentTheme = "monokai"
            }
            MenuItem {
                text: qsTr("Dracula")
                checkable: true
                checked: currentTheme === "dracula"
                onTriggered: currentTheme = "dracula"
            }
            MenuItem {
                text: qsTr("Darcula (JetBrains)")
                checkable: true
                checked: currentTheme === "darcula"
                onTriggered: currentTheme = "darcula"
            }
            MenuItem {
                text: qsTr("Atom One Dark")
                checkable: true
                checked: currentTheme === "atomonedark"
                onTriggered: currentTheme = "atomonedark"
            }
            MenuItem {
                text: qsTr("Sublime Text")
                checkable: true
                checked: currentTheme === "sublime"
                onTriggered: currentTheme = "sublime"
            }
            MenuItem {
                text: qsTr("Xcode Light")
                checkable: true
                checked: currentTheme === "xcode_light"
                onTriggered: currentTheme = "xcode_light"
            }
            MenuItem {
                text: qsTr("Xcode Dark")
                checkable: true
                checked: currentTheme === "xcode_dark"
                onTriggered: currentTheme = "xcode_dark"
            }
            MenuItem {
                text: qsTr("Notepad++")
                checkable: true
                checked: currentTheme === "notepadpp"
                onTriggered: currentTheme = "notepadpp"
            }
            MenuItem {
                text: qsTr("High Contrast Dark")
                checkable: true
                checked: currentTheme === "highcontrast_dark"
                onTriggered: currentTheme = "highcontrast_dark"
            }
            MenuItem {
                text: qsTr("High Contrast Light")
                checkable: true
                checked: currentTheme === "highcontrast_light"
                onTriggered: currentTheme = "highcontrast_light"
            }
            MenuItem {
                text: qsTr("Nord Contrast")
                checkable: true
                checked: currentTheme === "nord_contrast"
                onTriggered: currentTheme = "nord_contrast"
            }
            MenuItem {
                text: qsTr("Solarized High Contrast")
                checkable: true
                checked: currentTheme === "solarized_highcontrast"
                onTriggered: currentTheme = "solarized_highcontrast"
            }
        }
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            // Image {
            //     Layout.preferredWidth: 32
            //     Layout.preferredHeight: 32
            //     source: "qrc:/resources/icon/logo.png"
            //     smooth: true
            //     antialiasing: true
            //     layer.enabled: true
            //     layer.effect: MultiEffect {
            //         brightness: 0.05
            //         colorization: 1.0
            //         colorizationColor: palette.highlight
            //     }
            // }
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
        anchors.fill: parent
        TabBar {
            id: bar
            Layout.fillWidth: true
            Layout.fillHeight: true
            TabButton {
                text: qsTr("Msi")
            }
            TabButton {
                text: qsTr("Cpu")
            }
        }
        StackLayout {
            // anchors.fill: parent
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: bar.currentIndex


            ColumnLayout {
                id: mainContent
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
                        checked: sb.value != undefined ? sb.value : false
                        onCheckedChanged: {
                            if (scollerBoost.sb.availableValues != undefined && scollerBoost.sb.availableValues.length > 1) {
                                scollerBoost.sb.value = scollerBoost.sb.availableValues[checked ? 1 : 0];
                            }
                        }
                    }
                    Switch {
                        id: usbPowerShare
                        Layout.fillWidth: true
                        text: "USB Power"
                        property var ps : proxy.getProxyParameter(Msi.Parametr.UsbPowerShareEc)
                        visible: ps.isValid
                        checked: ps.value != undefined ? ps.value : false
                        onCheckedChanged: {
                            if (usbPowerShare.ps.availableValues != undefined && usbPowerShare.ps.availableValues.length > 1)
                                usbPowerShare.ps.value = usbPowerShare.ps.availableValues[checked ? 1 : 0];
                        }
                    }
                    Switch {
                        id: webCam
                        Layout.fillWidth: true
                        text: "WebCam"
                        property var wc : proxy.getProxyParameter(Msi.Parametr.WebCamEc)
                        visible: wc.isValid
                        checked: wc.value != undefined ? wc.value : false
                        onCheckedChanged: {
                            if (webCam.wc.availableValues != undefined && webCam.wc.availableValues.length > 1)
                                webCam.wc.value = webCam.wc.availableValues[checked ? 1 : 0];
                        }
                    }
                    Switch {
                        Layout.fillWidth: true
                        id: fnSuperSwap
                        text: "FN ⇄ Meta"
                        property var fss : proxy.getProxyParameter(Msi.Parametr.FnSuperSwapEc)
                        visible: fss.isValid
                        checked: fss.value != undefined ? fss.value : false
                        onCheckedChanged: {
                            if (fnSuperSwap.fss.availableValues != undefined && fnSuperSwap.fss.availableValues.length > 1)
                                fnSuperSwap.fss.value = fnSuperSwap.fss.availableValues[checked ? 1 : 0];
                        }
                    }
                    Switch {
                        Layout.fillWidth: true
                        id: webCamBlock
                        text: "WebCam Block"
                        property var wcb : proxy.getProxyParameter(Msi.Parametr.WebCamBlockEc)
                        visible: wcb.isValid
                        checked: wcb.value != undefined ? wcb.value : false
                        onCheckedChanged: {
                            if (webCamBlock.wcb.availableValues != undefined && webCamBlock.wcb.availableValues.length > 1)
                                webCamBlock.wcb.value = webCamBlock.wcb.availableValues[checked ? 1 : 0];
                        }
                    }
                    Switch {
                        Layout.fillWidth: true
                        id: superBattery
                        text: "Super Battery"
                        property var sb : proxy.getProxyParameter(Msi.Parametr.SuperBatteryEc)
                        visible: sb.isValid
                        checked: sb.value != undefined ? sb.value : false
                        onCheckedChanged: {
                            if (superBattery.sb.availableValues != undefined && superBattery.sb.availableValues.length > 1)
                                superBattery.sb.value = superBattery.sb.availableValues[checked ? 1 : 0];
                        }
                    }
                    Switch {
                        Layout.fillWidth: true
                        id: micMute
                        text: "Mic Mute"
                        property var mm : proxy.getProxyParameter(Msi.Parametr.MicMuteEc)
                        visible: mm.isValid
                        checked: mm.value != undefined ? mm.value : false
                        onCheckedChanged: {
                            if (micMute.mm.availableValues != undefined && micMute.mm.availableValues.length > 1)
                                micMute.mm.value = micMute.mm.availableValues[checked ? 1 : 0];
                        }
                    }
                    Switch {
                        Layout.fillWidth: true
                        id: muteLed
                        text: "Mute LED"
                        property var ml : proxy.getProxyParameter(Msi.Parametr.MuteLedEc)
                        visible: ml.isValid
                        checked: ml.value != undefined ? ml.value : false
                        onCheckedChanged: {
                            if (muteLed.ml.availableValues != undefined && muteLed.ml.availableValues.length > 1)
                                muteLed.ml.value = muteLed.ml.availableValues[checked ? 1 : 0];
                        }
                    }
                    Switch {
                        Layout.fillWidth: true
                        id: keyboardBacklightMode
                        text: "Key Mode"
                        property var kbm : proxy.getProxyParameter(Msi.Parametr.KeyboardBacklightModeEc)
                        visible: kbm.isValid
                        checked: kbm.value != undefined ? kbm.value : false
                        onCheckedChanged: {
                            if (keyboardBacklightMode.kbm.availableValues != undefined && keyboardBacklightMode.kbm.availableValues.length > 1)
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
                                value: batteryThresholdGroup.bt.value || value
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

            ColumnLayout {
                id: cpuContent
                property var cpuConfig: proxy.getProxyParameter(Msi.Parametr.CpuConfig).value
                property var valueFregs: [];
                property var valueUsage: [];
                property var prevFregs: []
                property var prevUsage: []
                property var rawFregs: []
                property var rawUsage: []
                property var prevMaxFreqScaling: []

                function kalmanFilter(prev, measurement, estimateError, measurementError) {
                    var gain = estimateError / (estimateError + measurementError);
                    return prev + gain * (measurement - prev);
                }
                function arraysEqual(a, b) {
                    if (a === b) return true;
                    if (!a || !b) return false;
                    if (a.length !== b.length) return false;
                    for (var i = 0; i < a.length; ++i) {
                        if (a[i] !== b[i]) return false;
                    }
                    return true;
                }

                function normalized(value, idx) {
                    return (value - cpuConfig.cpus[idx].minFreq) / 
                           (cpuConfig.cpus[idx].maxFreq - cpuConfig.cpus[idx].minFreq);
                }

                function denormalized(value, idx) {
                    return value * (cpuConfig.cpus[idx].maxFreq - cpuConfig.cpus[idx].minFreq) + 
                           cpuConfig.cpus[idx].minFreq;
                }

                onCpuConfigChanged: {
                    if(!cpuConfig) {
                        return;
                    }
                    
                    if(cpuContent.rawFregs.length !== cpuConfig.cpus.length && cpuContent.rawUsage !== cpuConfig.cpus.length) {
                        cpuContent.rawFregs = Array(cpuConfig.cpus.length).fill(0);
                        cpuContent.rawUsage = Array(cpuConfig.cpus.length).fill(0);
                    }
                    var curMaxFreqScaling = Array(cpuContent.cpuConfig.cpus.length).fill(0);
                    
                    if (cpuConfig.cpus.length > 0) {
                        for (var i = 0; i < cpuConfig.cpus.length; i++) {
                            var core = cpuConfig.cpus[i];
                            cpuContent.rawFregs[i] = (core.currentFreq - core.minFreq) / (core.maxFreq - core.minFreq);
                            cpuContent.rawUsage[i] = core.usage / 100.0;
                            curMaxFreqScaling[i] = core.scalingMaxFreq;
                        }
                    }
                    if(!arraysEqual(curMaxFreqScaling, prevMaxFreqScaling)) {
                        prevMaxFreqScaling = curMaxFreqScaling;
                    }
                    if (prevFregs.length !== cpuContent.rawFregs.length)
                        prevFregs = Array(cpuContent.rawFregs.length).fill(0);
                    if (prevUsage.length !== cpuContent.rawUsage.length)
                        prevUsage = Array(cpuContent.rawUsage.length).fill(0);
                }

                Timer {
                    interval: 16
                    running: cpuContent.visible
                    repeat: true
                    onTriggered: {
                        var estimateError = 0.003 * interval;
                        var measurementError = 10.0;
                        for (var i = 0; i < cpuContent.rawFregs.length; i++) {
                            var filteredFreq = cpuContent.kalmanFilter(cpuContent.prevFregs[i], cpuContent.rawFregs[i], estimateError, measurementError);
                            cpuContent.valueFregs[i] = filteredFreq;
                            cpuContent.prevFregs[i] = filteredFreq;

                            var filteredUsage = cpuContent.kalmanFilter(cpuContent.prevUsage[i], cpuContent.rawUsage[i], estimateError, measurementError);
                            cpuContent.valueUsage[i] = filteredUsage;
                            cpuContent.prevUsage[i] = filteredUsage;
                        }
                        cpuContent.valueUsage = cpuContent.valueUsage
                        cpuContent.prevUsage = cpuContent.prevUsage
                        cpuContent.valueFregs = cpuContent.valueFregs;
                        cpuContent.prevFregs = cpuContent.prevFregs;
                    
                    }
                }


                Frame {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: freqGraph.margin
                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter 
                            Layout.fillWidth: true
                            RowLayout {
                                Rectangle {
                                    width: 18; height: 10; radius: 2
                                    color: Fusion.highlight(palette)
                                    opacity: 0.7
                                }
                                Label { text: qsTr("Frequency"); color: palette.text }
                            }
                            RowLayout {
                                Rectangle {
                                    width: 18; height: 10; radius: 2
                                    color: Qt.lighter(Fusion.highlight(palette), 1.5)
                                    opacity: 0.5
                                }
                                Label { text: qsTr("Usage"); color: palette.text }
                            }
                            RowLayout {
                                Rectangle {
                                    width: 18; height: 10; radius: 2
                                    color: Fusion.highlight(palette)
                                    border.color: palette.base
                                    border.width: 2
                                }
                                Label { text: qsTr("Control frequency"); color: palette.text }
                            }
                        }

                        Rectangle {
                            id: freqGraph
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: "transparent"

                            property int barCount: cpuContent.valueFregs.length
                            property real barWidth: barCount > 0 ? width / (barCount * 1.5) : 0
                            property real margin: barCount > 0 ? width / (barCount * 3) : 0

                            property int hoveredIndex: -1
                            property real tooltipX: 0
                            property real tooltipY: 0
                            property var tooltipText: ""

                            Label {
                                id: globalTooltip
                                visible: freqGraph.hoveredIndex >= 0
                                text: freqGraph.hoveredIndex >= 0
                                    ? freqGraph.tooltipText
                                    : ""
                                color: palette.text
                                font.pixelSize: 12
                                background: Rectangle { color: "#222"; radius: 4; opacity: 0.9 }
                                x: Math.max(0, Math.min(freqGraph.tooltipX - width / 2, freqGraph.width - width))
                                y: freqGraph.tooltipY - height - 8
                                z: 1000
                            }


                            Repeater {
                                model: freqGraph.barCount
                                Rectangle {
                                    x: index * (freqGraph.barWidth + freqGraph.margin) + freqGraph.margin / 2
                                    width: freqGraph.barWidth
                                    height: freqGraph.height * cpuContent.valueFregs[index]
                                    y: freqGraph.height - height
                                    color: Fusion.highlight(palette)
                                    radius: 2
                                    opacity: 0.7
                                    MouseArea {
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onPositionChanged: function(mouse) {
                                            freqGraph.hoveredIndex = index;
                                            freqGraph.tooltipX = parent.x + parent.width / 2;
                                            freqGraph.tooltipY = parent.y;
                                            freqGraph.tooltipText = (cpuContent.denormalized(cpuContent.valueFregs[index], index) / 1000000.).toFixed(1) + " GHz";
                                        }
                                        onExited: {
                                            freqGraph.hoveredIndex = -1
                                            parent.opacity = 0.7
                                        }
                                        onEntered: parent.opacity = 1.0
                                    }
                                }
                            }
                            Repeater {
                                model: freqGraph.barCount
                                Rectangle {
                                    x: index * (freqGraph.barWidth + freqGraph.margin) + freqGraph.margin / 2 
                                    width: freqGraph.barWidth 
                                    height: freqGraph.height * cpuContent.valueUsage[index]
                                    y: freqGraph.height - height
                                    color: Qt.lighter(Fusion.highlight(palette), 1.5)
                                    radius: 1
                                    opacity: 0.5

                                    MouseArea {
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onPositionChanged: function(mouse) {
                                            freqGraph.hoveredIndex = index;
                                            freqGraph.tooltipX = parent.x + parent.width / 2;
                                            freqGraph.tooltipY = parent.y;
                                            freqGraph.tooltipText = (cpuContent.valueUsage[index] * 100).toFixed(1) + "%";
                                        }
                                        onExited: {
                                            freqGraph.hoveredIndex = -1
                                            parent.opacity = 0.5
                                        }
                                        onEntered: parent.opacity = 1.0
                                    }
                                }
                            }

                            Repeater {
                                model: freqGraph.barCount
                                Rectangle {
                                    id: controlRect
                                    property int idx: index
                                    property real dragStartY: 0
                                    property real dragStartNorm: 0

                                    x: idx * (freqGraph.barWidth + freqGraph.margin) + freqGraph.margin / 2 - freqGraph.margin / 2
                                    width: freqGraph.barWidth + freqGraph.margin
                                    height: freqGraph.margin * 2
                                    y: freqGraph.height - (freqGraph.height * cpuContent.normalized(cpuContent.prevMaxFreqScaling[idx], idx) + freqGraph.margin)
                                    color: Fusion.highlight(palette)
                                    border.color: palette.base
                                    border.width: 2
                                    radius: 1
                                    MouseArea {
                                        anchors.fill: parent
                                        // hoverEnabled: true
                                        onEntered: parent.opacity = 0.7
                                        onExited: parent.opacity = 1.0
                                        onPressed: function(mouse) {
                                            parent.dragStartY = mouse.y;
                                            parent.dragStartNorm = cpuContent.normalized(cpuContent.prevMaxFreqScaling[parent.idx], parent.idx);
                                            var newY = parent.y + mouse.y - parent.dragStartY + parent.height / 2;
                                            var norm = 1 - Math.max(0, Math.min(newY, freqGraph.height)) / freqGraph.height;
                                            norm = Math.max(0, Math.min(norm, 1));
                                            freqGraph.hoveredIndex = index;
                                            freqGraph.tooltipX = parent.x + parent.width / 2;
                                            freqGraph.tooltipY = parent.y;
                                            freqGraph.tooltipText = (cpuContent.denormalized(norm, parent.idx) / 1000000).toFixed(1) + "GHz";
                                        }
                                        onPositionChanged: function(mouse) {
                                            var newY = parent.y + mouse.y - parent.dragStartY + parent.height / 2;
                                            var norm = 1 - Math.max(0, Math.min(newY, freqGraph.height)) / freqGraph.height;
                                            norm = Math.max(0, Math.min(norm, 1));
                                            cpuContent.prevMaxFreqScaling[parent.idx] = cpuContent.denormalized(norm, parent.idx);
                                            var config = cpuContent.cpuConfig;
                                            config.cpus[parent.idx].scalingMaxFreq = cpuContent.prevMaxFreqScaling[parent.idx];
                                            proxy.getProxyParameter(Msi.Parametr.CpuConfig).value = config;
                                            freqGraph.hoveredIndex = index;
                                            freqGraph.tooltipX = parent.x + parent.width / 2;
                                            freqGraph.tooltipY = parent.y;
                                            freqGraph.tooltipText = (cpuContent.denormalized(norm, parent.idx) / 1000000).toFixed(1) + "GHz";
                                        }
                                        onReleased: {
                                            freqGraph.hoveredIndex = -1;
                                        }
                                    }
                                }
                            }
                            Repeater {
                                model: freqGraph.barCount
                                Label {
                                    text: index + 1
                                    color: palette.text
                                    font.pixelSize: 12
                                    width: freqGraph.barWidth
                                    horizontalAlignment: Text.AlignHCenter
                                    x: index * (freqGraph.barWidth + freqGraph.margin) + freqGraph.margin / 2
                                    y: freqGraph.height - height
                                }
                            }
                        }

                        GridLayout {
                            id: cpuGrid
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            // columns: cpuContent.cpuConfig.cpus.length / 2
                            property var availableGovernors: cpuContent.cpuConfig ? cpuContent.cpuConfig.cpus[0].availableGovernors || [] : [];
                            property var availableGovernor: cpuContent.cpuConfig ? cpuContent.cpuConfig.cpus[0].availableGovernor || "N/A" : "N/A";
                            ButtonGroup {
                                id: availableGovernorGroup
                            }
                            Label {
                                text: qsTr("CPU Governor:")
                                color: palette.text
                            }
                            RowLayout {
                                spacing: 12
                                Repeater {
                                    model: cpuGrid.availableGovernors
                                    RadioButton {
                                        text: cpuGrid.availableGovernors[index] || "N/A"
                                        checked: cpuGrid.availableGovernor === modelData
                                        ButtonGroup.group: availableGovernorGroup
                                            onClicked: {
                                                var config = cpuContent.cpuConfig;
                                                for(var i = 0; i < config.cpus.length; i++) {
                                                    config.cpus[i].availableGovernor = modelData;
                                                }
                                                proxy.getProxyParameter(Msi.Parametr.CpuConfig).value = config;
                                            }
                                    }
                                }
                            }
                        }
                    }
                }   
            }   
        }
    }
}