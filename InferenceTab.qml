import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ScrollView {
    id: root
    required property var control
    property color cardColor: "#222836"
    property color borderColor: "#2f3646"
    property color textColor: "#e6ebf5"
    property color subTextColor: "#9aa7bf"
    property color accentColor: "#4da3ff"
    property var uiState: control ? control.uiState : ({})
    property var inferenceData: uiState.inference || ({})
    property var projectData: uiState.project || ({})
    property bool settingsExpanded: true

    function saveSetting(setting, value) {
        if (!root.control)
            return
        root.control.inferenceAction("set_setting", { "setting": setting, "value": value })
    }

    Component.onCompleted: {
        if (inferenceData.settingsExpanded !== undefined)
            settingsExpanded = Boolean(inferenceData.settingsExpanded)
    }

    onSettingsExpandedChanged: {
        root.saveSetting("uiSettingsExpanded", settingsExpanded)
    }

    Connections {
        target: root.control
        function onUiStateChanged() {
            const d = ((root.control.uiState || {}).inference || {})
            if (d.settingsExpanded !== undefined) {
                const v = Boolean(d.settingsExpanded)
                if (root.settingsExpanded !== v)
                    root.settingsExpanded = v
            }
        }
    }

    component DarkButton: Button {
        id: darkButton
        implicitHeight: 30
        contentItem: Text {
            text: darkButton.text
            color: darkButton.enabled ? root.textColor : "#7f8aa1"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            radius: 6
            border.width: 1
            border.color: !darkButton.enabled ? "#2d3443" : (darkButton.hovered ? "#5a6478" : root.borderColor)
            color: !darkButton.enabled ? "#252c3a" : (darkButton.down ? "#3b4a63" : "#30394b")
        }
    }

    component DarkField: TextField {
        id: field
        color: root.textColor
        placeholderTextColor: "#7885a1"
        selectedTextColor: "#ffffff"
        selectionColor: "#3a6ea5"
        background: Rectangle {
            radius: 6
            color: "#141a26"
            border.color: field.activeFocus ? root.accentColor : root.borderColor
            border.width: 1
        }
    }

    FileDialog {
        id: pythonFileDialog
        title: "Select Python Script"
        nameFilters: ["Python script (*.py)", "All files (*)"]
        onAccepted: {
            const path = selectedFile.toString().replace("file://", "")
            pythonPathField.text = path
            root.saveSetting("pythonFilePath", path)
        }
    }

    FileDialog {
        id: modelFileDialog
        title: "Select Best Model File"
        nameFilters: ["PyTorch model (*.pth *.pt)", "All files (*)"]
        onAccepted: {
            const path = selectedFile.toString().replace("file://", "")
            modelPathField.text = path
            root.saveSetting("bestModelPath", path)
        }
    }

    FolderDialog {
        id: dockerMountFolderDialog
        title: "Select Docker Host Mount Path"
        onAccepted: {
            const path = selectedFolder.toString().replace("file://", "")
            dockerHostMountField.text = path
            root.saveSetting("dockerHostMountPath", path)
        }
    }

    Column {
        width: root.width - 24
        spacing: 10

        Rectangle {
            width: parent.width
            color: root.cardColor
            radius: 10
            border.color: root.borderColor
            border.width: 1
            implicitHeight: settingsGrid.implicitHeight + 20

            Column {
                id: settingsGrid
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 10
                spacing: 8

                Button {
                    id: settingsToggle
                    width: parent.width
                    text: (root.settingsExpanded ? "▼ " : "▶ ") + "Inference Settings"
                    onClicked: root.settingsExpanded = !root.settingsExpanded

                    contentItem: Text {
                        text: settingsToggle.text
                        color: root.subTextColor
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                    background: Rectangle {
                        radius: 6
                        color: settingsToggle.down ? "#30394b" : "#283043"
                        border.color: root.borderColor
                        border.width: 1
                    }
                }

                RowLayout {
                    width: parent.width
                    spacing: 8
                    visible: root.settingsExpanded
                    Text { Layout.preferredWidth: 120; text: "Inference Type"; color: root.subTextColor }
                    ComboBox {
                        Layout.fillWidth: true
                        model: ["classification", "segmentation", "detection", "ocr"]
                        currentIndex: Math.max(0, model.indexOf(String(root.inferenceData.taskType || "classification")))
                        onActivated: root.saveSetting("taskType", currentText)
                    }
                }

                RowLayout {
                    width: parent.width
                    spacing: 8
                    visible: root.settingsExpanded
                    Text { Layout.preferredWidth: 120; text: "Python Script"; color: root.subTextColor }
                    DarkField {
                        id: pythonPathField
                        Layout.fillWidth: true
                        text: String(root.inferenceData.pythonFilePath || "")
                        onEditingFinished: root.saveSetting("pythonFilePath", text)
                    }
                    DarkButton { Layout.preferredWidth: 72; text: "Browse"; onClicked: pythonFileDialog.open() }
                }

                RowLayout {
                    width: parent.width
                    spacing: 8
                    visible: root.settingsExpanded
                    Text { Layout.preferredWidth: 120; text: "Best Model"; color: root.subTextColor }
                    DarkField {
                        id: modelPathField
                        Layout.fillWidth: true
                        text: String(root.inferenceData.bestModelPath || "")
                        onEditingFinished: root.saveSetting("bestModelPath", text)
                    }
                    DarkButton { Layout.preferredWidth: 72; text: "Browse"; onClicked: modelFileDialog.open() }
                }

                RowLayout {
                    width: parent.width
                    spacing: 8
                    visible: root.settingsExpanded
                    Text { Layout.preferredWidth: 120; text: "Docker Image"; color: root.subTextColor }
                    DarkField {
                        Layout.fillWidth: true
                        text: String(root.inferenceData.dockerImage || "")
                        onEditingFinished: root.saveSetting("dockerImage", text)
                    }
                }

                RowLayout {
                    width: parent.width
                    spacing: 8
                    visible: root.settingsExpanded
                    Text { Layout.preferredWidth: 120; text: "Host Mount"; color: root.subTextColor }
                    DarkField {
                        id: dockerHostMountField
                        Layout.fillWidth: true
                        text: String(root.inferenceData.dockerHostMountPath || "")
                        onEditingFinished: root.saveSetting("dockerHostMountPath", text)
                    }
                    DarkButton { Layout.preferredWidth: 72; text: "Browse"; onClicked: dockerMountFolderDialog.open() }
                }

                RowLayout {
                    width: parent.width
                    spacing: 8
                    visible: root.settingsExpanded
                    Text { Layout.preferredWidth: 120; text: "Container WD"; color: root.subTextColor }
                    DarkField {
                        Layout.fillWidth: true
                        text: String(root.inferenceData.dockerContainerWorkDir || "/workspace")
                        onEditingFinished: root.saveSetting("dockerContainerWorkDir", text)
                    }
                }

                Text {
                    width: parent.width
                    color: root.subTextColor
                    font.pixelSize: 12
                    text: "Project: " + String(root.projectData.projectFilePath || "")
                    elide: Text.ElideMiddle
                    visible: root.settingsExpanded
                }
            }
        }

        Rectangle {
            width: parent.width
            color: root.cardColor
            radius: 10
            border.color: root.borderColor
            border.width: 1
            implicitHeight: 130

            Column {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                Text {
                    text: "Run Inference"
                    color: root.subTextColor
                    font.pixelSize: 13
                }

                RowLayout {
                    width: parent.width
                    spacing: 14
                    CheckBox {
                        text: "Train"
                        checked: Boolean(root.inferenceData.useTrain)
                        onClicked: root.saveSetting("useTrain", checked)
                        contentItem: Text {
                            text: parent.text
                            color: root.textColor
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: parent.indicator.width + parent.spacing
                        }
                    }
                    CheckBox {
                        text: "Val"
                        checked: Boolean(root.inferenceData.useVal)
                        onClicked: root.saveSetting("useVal", checked)
                        contentItem: Text {
                            text: parent.text
                            color: root.textColor
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: parent.indicator.width + parent.spacing
                        }
                    }
                    CheckBox {
                        text: "Test"
                        checked: Boolean(root.inferenceData.useTest)
                        onClicked: root.saveSetting("useTest", checked)
                        contentItem: Text {
                            text: parent.text
                            color: root.textColor
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: parent.indicator.width + parent.spacing
                        }
                    }
                }

                RowLayout {
                    width: parent.width
                    spacing: 8
                    DarkButton {
                        text: "Start"
                        Layout.preferredWidth: 100
                        enabled: !Boolean(root.inferenceData.running)
                        onClicked: root.control.inferenceAction("start_inference", {
                            "projectFilePath": String(root.projectData.projectFilePath || "")
                        })
                    }
                    DarkButton {
                        text: "Stop"
                        Layout.preferredWidth: 100
                        enabled: Boolean(root.inferenceData.running)
                        onClicked: root.control.inferenceAction("stop_inference")
                    }
                    Text {
                        Layout.fillWidth: true
                        text: String(root.inferenceData.status || "Idle")
                        color: root.textColor
                        elide: Text.ElideRight
                    }
                }

                Item {
                    width: parent.width
                    height: 26

                    ProgressBar {
                        anchors.fill: parent
                        from: 0
                        to: 100
                        value: Number(root.inferenceData.progress || 0)
                        background: Rectangle {
                            radius: 3
                            color: "#141a26"
                            border.color: root.borderColor
                            border.width: 1
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        text: Math.round(Number(root.inferenceData.progress || 0)) + "%"
                        color: "#ffffff"
                        font.pixelSize: 12
                        font.bold: true
                        z: 2
                    }
                }
            }
        }

        ScrollView {
            width: parent.width
            height: 220
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            TextArea {
                id: inferenceLog
                width: parent.width
                readOnly: true
                wrapMode: TextArea.Wrap
                text: String(root.inferenceData.logText || "")
                color: root.textColor
                onTextChanged: cursorPosition = length
                background: Rectangle {
                    radius: 6
                    color: "#141a26"
                    border.color: root.borderColor
                    border.width: 1
                }
            }
        }
    }
}
