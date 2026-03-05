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
    property var trainingData: uiState.training || ({})
    property int labelWidth: 92

    function saveSetting(setting, value) {
        if (!root.control)
            return
        root.control.trainAction("set_setting", { "setting": setting, "value": value })
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
            bestModelPathField.text = path
            root.saveSetting("bestModelPath", path)
        }
    }

    FileDialog {
        id: predictionFileDialog
        title: "Select Predicted JSON Path"
        fileMode: FileDialog.SaveFile
        nameFilters: ["JSON files (*.json)", "All files (*)"]
        onAccepted: {
            const path = selectedFile.toString().replace("file://", "")
            predictionPathField.text = path
            root.saveSetting("predictionJsonPath", path)
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
            implicitHeight: settingsCol.implicitHeight + 20

            Column {
                id: settingsCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 10
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8

                Text {
                    text: "Training Paths (System Config)"
                    color: root.subTextColor
                    font.pixelSize: 13
                }

                RowLayout {
                    width: parent.width
                    spacing: 8
                    Text {
                        Layout.preferredWidth: 120
                        text: "Python Script"
                        color: root.subTextColor
                        verticalAlignment: Text.AlignVCenter
                    }
                    DarkField {
                        id: pythonPathField
                        Layout.fillWidth: true
                        text: String(root.trainingData.pythonFilePath || "")
                        onEditingFinished: root.saveSetting("pythonFilePath", text)
                    }
                    DarkButton {
                        Layout.preferredWidth: 72
                        text: "Browse"
                        onClicked: pythonFileDialog.open()
                    }
                }

                RowLayout {
                    width: parent.width
                    spacing: 8
                    Text {
                        Layout.preferredWidth: 120
                        text: "Pred JSON"
                        color: root.subTextColor
                        verticalAlignment: Text.AlignVCenter
                    }
                    DarkField {
                        id: predictionPathField
                        Layout.fillWidth: true
                        text: String(root.trainingData.predictionJsonPath || "")
                        onEditingFinished: root.saveSetting("predictionJsonPath", text)
                    }
                    DarkButton {
                        Layout.preferredWidth: 72
                        text: "Browse"
                        onClicked: predictionFileDialog.open()
                    }
                }

                RowLayout {
                    width: parent.width
                    spacing: 8
                    Text {
                        Layout.preferredWidth: 120
                        text: "Best Model"
                        color: root.subTextColor
                        verticalAlignment: Text.AlignVCenter
                    }
                    DarkField {
                        id: bestModelPathField
                        Layout.fillWidth: true
                        text: String(root.trainingData.bestModelPath || "")
                        onEditingFinished: root.saveSetting("bestModelPath", text)
                    }
                    DarkButton {
                        Layout.preferredWidth: 72
                        text: "Browse"
                        onClicked: modelFileDialog.open()
                    }
                }
            }
        }

        Rectangle {
            width: parent.width
            color: root.cardColor
            radius: 10
            border.color: root.borderColor
            border.width: 1
            implicitHeight: hyperCol.implicitHeight + 20

            Column {
                id: hyperCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 10
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8

                Text {
                    text: "Common Hyperparameters (System Defaults)"
                    color: root.subTextColor
                    font.pixelSize: 13
                }

                ColumnLayout {
                    width: parent.width
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text { Layout.preferredWidth: root.labelWidth; text: "Backbone"; color: root.subTextColor; verticalAlignment: Text.AlignVCenter }
                        ComboBox {
                            id: backboneCombo
                            Layout.fillWidth: true
                            model: ["efficientnet_b0", "resnet18", "mobilenet_v3_large", "mobilenet_v3_small", "mobilenet_v2"]
                            currentIndex: Math.max(0, model.indexOf(String(root.trainingData.backbone || "efficientnet_b0")))
                            onActivated: root.saveSetting("backbone", currentText)
                            contentItem: Text {
                                text: backboneCombo.displayText
                                color: "#111111"
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: 8
                                elide: Text.ElideRight
                            }
                            popup: Popup {
                                y: backboneCombo.height
                                width: backboneCombo.width
                                padding: 2
                                contentItem: ListView {
                                    clip: true
                                    implicitHeight: contentHeight
                                    model: backboneCombo.delegateModel
                                    currentIndex: backboneCombo.highlightedIndex
                                }
                                background: Rectangle {
                                    color: "#f0f0f0"
                                    border.color: "#a0a0a0"
                                    radius: 4
                                }
                            }
                            delegate: ItemDelegate {
                                width: backboneCombo.width
                                contentItem: Text {
                                    text: modelData
                                    color: "#111111"
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                }
                                highlighted: backboneCombo.highlightedIndex === index
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text { Layout.preferredWidth: root.labelWidth; text: "Epochs"; color: root.subTextColor; verticalAlignment: Text.AlignVCenter }
                        DarkField {
                            id: epochsField
                            Layout.fillWidth: true
                            inputMethodHints: Qt.ImhDigitsOnly
                            text: String(root.trainingData.epochs || 40)
                            onEditingFinished: root.saveSetting("epochs", Number(text))
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text { Layout.preferredWidth: root.labelWidth; text: "Learning Rate"; color: root.subTextColor; verticalAlignment: Text.AlignVCenter }
                        DarkField {
                            id: lrField
                            Layout.fillWidth: true
                            text: String(root.trainingData.learningRate || 0.001)
                            onEditingFinished: root.saveSetting("learningRate", Number(text))
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text { Layout.preferredWidth: root.labelWidth; text: "Weight Decay"; color: root.subTextColor; verticalAlignment: Text.AlignVCenter }
                        DarkField {
                            id: wdField
                            Layout.fillWidth: true
                            text: String(root.trainingData.weightDecay || 0.0001)
                            onEditingFinished: root.saveSetting("weightDecay", Number(text))
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text { Layout.preferredWidth: root.labelWidth; text: "Batch Size"; color: root.subTextColor; verticalAlignment: Text.AlignVCenter }
                        DarkField {
                            id: batchField
                            Layout.fillWidth: true
                            inputMethodHints: Qt.ImhDigitsOnly
                            text: String(root.trainingData.batchSize || 64)
                            onEditingFinished: root.saveSetting("batchSize", Number(text))
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text { Layout.preferredWidth: root.labelWidth; text: "Workers"; color: root.subTextColor; verticalAlignment: Text.AlignVCenter }
                        DarkField {
                            id: workersField
                            Layout.fillWidth: true
                            inputMethodHints: Qt.ImhDigitsOnly
                            text: String(root.trainingData.numWorkers || 8)
                            onEditingFinished: root.saveSetting("numWorkers", Number(text))
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text { Layout.preferredWidth: root.labelWidth; text: "Input Size"; color: root.subTextColor; verticalAlignment: Text.AlignVCenter }
                        DarkField {
                            id: inputSizeField
                            Layout.fillWidth: true
                            inputMethodHints: Qt.ImhDigitsOnly
                            text: String(root.trainingData.inputSize || 224)
                            onEditingFinished: root.saveSetting("inputSize", Number(text))
                        }
                    }

                    Text {
                        text: "Saved to app/system config"
                        color: root.subTextColor
                        font.pixelSize: 12
                        Layout.fillWidth: true
                    }
                }
            }
        }

        Rectangle {
            width: parent.width
            color: root.cardColor
            radius: 10
            border.color: root.borderColor
            border.width: 1
            implicitHeight: splitCol.implicitHeight + 20

            Column {
                id: splitCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 10
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8

                Text {
                    text: "Dataset Split"
                    color: root.subTextColor
                    font.pixelSize: 13
                }

                RowLayout {
                    id: splitModel
                    width: parent.width
                    spacing: 8
                    property int total: Number((((root.control.uiState || {}).image || {}).imageCount || 0))
                    property int trainPct: Math.max(0, Number(splitTrainField.text || 0))
                    property int valPct: Math.max(0, Number(splitValField.text || 0))
                    property int testPct: Math.max(0, Number(splitTestField.text || 0))
                    property int sumPct: trainPct + valPct + testPct
                    property int trainCount: sumPct > 0 ? Math.floor(total * trainPct / sumPct) : 0
                    property int valCount: sumPct > 0 ? Math.floor(total * valPct / sumPct) : 0
                    property int testCount: Math.max(0, total - trainCount - valCount)

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        RowLayout {
                            Layout.fillWidth: true
                            Text { Layout.preferredWidth: 55; text: "Train"; color: root.subTextColor }
                            DarkField {
                                id: splitTrainField
                                Layout.preferredWidth: 64
                                inputMethodHints: Qt.ImhDigitsOnly
                                text: String(root.trainingData.splitTrainPercent || 60)
                                onEditingFinished: root.saveSetting("splitTrainPercent", Number(text))
                            }
                            Text { text: "%"; color: root.subTextColor }
                            Text {
                                Layout.fillWidth: true
                                text: "Count: " + splitModel.trainCount
                                color: root.subTextColor
                                elide: Text.ElideRight
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { Layout.preferredWidth: 55; text: "Val"; color: root.subTextColor }
                            DarkField {
                                id: splitValField
                                Layout.preferredWidth: 64
                                inputMethodHints: Qt.ImhDigitsOnly
                                text: String(root.trainingData.splitValPercent || 30)
                                onEditingFinished: root.saveSetting("splitValPercent", Number(text))
                            }
                            Text { text: "%"; color: root.subTextColor }
                            Text {
                                Layout.fillWidth: true
                                text: "Count: " + splitModel.valCount
                                color: root.subTextColor
                                elide: Text.ElideRight
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { Layout.preferredWidth: 55; text: "Test"; color: root.subTextColor }
                            DarkField {
                                id: splitTestField
                                Layout.preferredWidth: 64
                                inputMethodHints: Qt.ImhDigitsOnly
                                text: String(root.trainingData.splitTestPercent || 10)
                                onEditingFinished: root.saveSetting("splitTestPercent", Number(text))
                            }
                            Text { text: "%"; color: root.subTextColor }
                            Text {
                                Layout.fillWidth: true
                                text: "Count: " + splitModel.testCount
                                color: root.subTextColor
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
                RowLayout {
                    id: splitBox
                    width: parent.width
                    spacing: 8
                    DarkButton {
                        text: "Apply"
                        Layout.preferredWidth: 76
                        Layout.minimumWidth: 76
                        enabled: (splitModel.sumPct === 100) && (splitModel.total > 0)
                        onClicked: {
                            const total = ((root.control.uiState || {}).image || {}).imageCount || 0
                            root.control.trainAction("split_dataset", { "totalImages": total })
                        }
                    }
                    Item { Layout.fillWidth: true }
                }

                RowLayout {
                    width: parent.width
                    spacing: 8
                    Text {
                        text: "Total: " + splitModel.total
                              + "  Sum%: " + splitModel.sumPct
                              + (splitModel.sumPct === 100 ? "" : " (must be 100)")
                        color: root.subTextColor
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }

        Rectangle {
            width: parent.width
            color: root.cardColor
            radius: 10
            border.color: root.borderColor
            border.width: 1
            implicitHeight: trainRunCol.implicitHeight + 20

            Column {
                id: trainRunCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 10
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8

                Text {
                    text: "Training Run"
                    color: root.subTextColor
                    font.pixelSize: 13
                }

                RowLayout {
                    width: parent.width
                    spacing: 8
                    DarkButton {
                        text: "Start Train"
                        Layout.preferredWidth: 110
                        enabled: !(root.trainingData.running || false)
                        onClicked: root.control.trainAction("start_train", {})
                    }
                    DarkButton {
                        text: "Stop"
                        Layout.preferredWidth: 80
                        enabled: root.trainingData.running || false
                        onClicked: root.control.trainAction("stop_train", {})
                    }
                    Text {
                        text: String(root.trainingData.status || "Idle")
                        color: root.textColor
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                ProgressBar {
                    width: parent.width
                    from: 0
                    to: 100
                    value: Number(root.trainingData.progress || 0)
                }

                TextArea {
                    width: parent.width
                    height: 160
                    readOnly: true
                    wrapMode: TextArea.Wrap
                    color: root.textColor
                    text: String(root.trainingData.logText || "")
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
}
