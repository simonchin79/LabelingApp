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
    property var statsData: trainingData.stats || ({})
    property string currentTaskType: String(trainingData.taskType || "classification")
    property var taskTypes: trainingData.taskTypes || ["classification", "segmentation", "detection", "ocr"]
    property int labelWidth: 92
    property bool pathsExpanded: true
    property bool hyperparamsExpanded: true
    property bool splitExpanded: true

    function saveSetting(setting, value) {
        if (!root.control)
            return
        root.control.trainAction("set_setting", { "setting": setting, "value": value })
    }

    function backboneOptions() {
        if (root.currentTaskType === "classification")
            return ["efficientnet_b0", "resnet18", "mobilenet_v3_large", "mobilenet_v3_small", "mobilenet_v2"]
        if (root.currentTaskType === "segmentation")
            return ["unet", "deeplabv3", "segformer"]
        if (root.currentTaskType === "detection")
            return ["yolov8n", "fasterrcnn_resnet50", "retinanet_resnet50"]
        if (root.currentTaskType === "ocr")
            return ["crnn", "trocr_base", "svtr_tiny"]
        return ["efficientnet_b0"]
    }

    function statText(key, fallback) {
        const v = statsData[key]
        if (v === undefined || v === null || v === "")
            return fallback
        if (typeof v === "number") {
            if (Math.abs(v) >= 1000 || Number.isInteger(v))
                return String(v)
            return Number(v).toFixed(4)
        }
        return String(v)
    }

    function formatSeconds(value) {
        const n = Number(value)
        if (!Number.isFinite(n) || n < 0)
            return "-"
        const sec = Math.floor(n % 60)
        const min = Math.floor((n / 60) % 60)
        const hour = Math.floor(n / 3600)
        function p2(x) { return (x < 10 ? "0" : "") + x }
        if (hour > 0)
            return hour + ":" + p2(min) + ":" + p2(sec)
        return min + ":" + p2(sec)
    }

    Component.onCompleted: {
        if (trainingData.pathsExpanded !== undefined)
            pathsExpanded = Boolean(trainingData.pathsExpanded)
        if (trainingData.hyperparamsExpanded !== undefined)
            hyperparamsExpanded = Boolean(trainingData.hyperparamsExpanded)
        if (trainingData.splitExpanded !== undefined)
            splitExpanded = Boolean(trainingData.splitExpanded)
    }

    onPathsExpandedChanged: {
        root.saveSetting("uiPathsExpanded", pathsExpanded)
    }

    onHyperparamsExpandedChanged: {
        root.saveSetting("uiHyperparamsExpanded", hyperparamsExpanded)
    }
    onSplitExpandedChanged: {
        root.saveSetting("uiSplitExpanded", splitExpanded)
    }

    Connections {
        target: root.control
        function onUiStateChanged() {
            const td = ((root.control.uiState || {}).training || {})
            if (td.pathsExpanded !== undefined) {
                const v = Boolean(td.pathsExpanded)
                if (root.pathsExpanded !== v)
                    root.pathsExpanded = v
            }
            if (td.hyperparamsExpanded !== undefined) {
                const v2 = Boolean(td.hyperparamsExpanded)
                if (root.hyperparamsExpanded !== v2)
                    root.hyperparamsExpanded = v2
            }
            if (td.splitExpanded !== undefined) {
                const v3 = Boolean(td.splitExpanded)
                if (root.splitExpanded !== v3)
                    root.splitExpanded = v3
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
            bestModelPathField.text = path
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
            implicitHeight: settingsWrap.implicitHeight + 20

            Column {
                id: settingsWrap
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 10
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8

                Button {
                    id: pathsToggle
                    width: parent.width
                    text: (root.pathsExpanded ? "▼ " : "▶ ") + "Training Paths (" + root.currentTaskType + ")"
                    onClicked: root.pathsExpanded = !root.pathsExpanded
                    contentItem: Text {
                        text: pathsToggle.text
                        color: root.subTextColor
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: "transparent"
                        border.color: root.borderColor
                        border.width: 1
                        radius: 6
                    }
                }

                Column {
                    width: parent.width
                    spacing: 8
                    visible: root.pathsExpanded

                    RowLayout {
                        width: parent.width
                        spacing: 8
                        Text {
                            Layout.preferredWidth: 120
                            text: "Task Type"
                            color: root.subTextColor
                            verticalAlignment: Text.AlignVCenter
                        }
                        ComboBox {
                            id: taskTypeCombo
                            Layout.fillWidth: true
                            model: root.taskTypes
                            currentIndex: Math.max(0, model.indexOf(root.currentTaskType))
                            onActivated: root.saveSetting("taskType", currentText)
                        }
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

                    RowLayout {
                        width: parent.width
                        spacing: 8
                        Text {
                            Layout.preferredWidth: 120
                            text: "Docker Image"
                            color: root.subTextColor
                            verticalAlignment: Text.AlignVCenter
                        }
                        DarkField {
                            id: dockerImageField
                            Layout.fillWidth: true
                            text: String(root.trainingData.dockerImage || "")
                            placeholderText: "e.g. myorg/cls-train:latest"
                            onEditingFinished: root.saveSetting("dockerImage", text)
                        }
                    }

                    RowLayout {
                        width: parent.width
                        spacing: 8
                        Text {
                            Layout.preferredWidth: 120
                            text: "Host Mount"
                            color: root.subTextColor
                            verticalAlignment: Text.AlignVCenter
                        }
                        DarkField {
                            id: dockerHostMountField
                            Layout.fillWidth: true
                            text: String(root.trainingData.dockerHostMountPath || "")
                            placeholderText: "e.g. /host/project"
                            onEditingFinished: root.saveSetting("dockerHostMountPath", text)
                        }
                        DarkButton {
                            Layout.preferredWidth: 72
                            text: "Browse"
                            onClicked: dockerMountFolderDialog.open()
                        }
                    }

                    RowLayout {
                        width: parent.width
                        spacing: 8
                        Text {
                            Layout.preferredWidth: 120
                            text: "Container WD"
                            color: root.subTextColor
                            verticalAlignment: Text.AlignVCenter
                        }
                        DarkField {
                            id: dockerWorkDirField
                            Layout.fillWidth: true
                            text: String(root.trainingData.dockerContainerWorkDir || "/workspace")
                            placeholderText: "/workspace"
                            onEditingFinished: root.saveSetting("dockerContainerWorkDir", text)
                        }
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

                Button {
                    id: hyperToggle
                    width: parent.width
                    text: (root.hyperparamsExpanded ? "▼ " : "▶ ") + "Common Hyperparameters (" + root.currentTaskType + ")"
                    onClicked: root.hyperparamsExpanded = !root.hyperparamsExpanded
                    contentItem: Text {
                        text: hyperToggle.text
                        color: root.subTextColor
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: "transparent"
                        border.color: root.borderColor
                        border.width: 1
                        radius: 6
                    }
                }

                ColumnLayout {
                    width: parent.width
                    spacing: 8
                    visible: root.hyperparamsExpanded

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text { Layout.preferredWidth: root.labelWidth; text: "Backbone"; color: root.subTextColor; verticalAlignment: Text.AlignVCenter }
                        ComboBox {
                            id: backboneCombo
                            Layout.fillWidth: true
                            model: root.backboneOptions()
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

                Button {
                    id: splitToggle
                    width: parent.width
                    text: (root.splitExpanded ? "▼ " : "▶ ") + "Dataset Split"
                    onClicked: root.splitExpanded = !root.splitExpanded

                    contentItem: Text {
                        text: splitToggle.text
                        color: root.subTextColor
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                    background: Rectangle {
                        radius: 6
                        color: splitToggle.down ? "#30394b" : "#283043"
                        border.color: root.borderColor
                        border.width: 1
                    }
                }

                RowLayout {
                    id: splitModel
                    width: parent.width
                    spacing: 8
                    visible: root.splitExpanded
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

                        RowLayout {
                            Layout.fillWidth: true
                            Text { Layout.preferredWidth: 55; text: "Seed"; color: root.subTextColor }
                            DarkField {
                                id: splitSeedField
                                Layout.preferredWidth: 90
                                inputMethodHints: Qt.ImhDigitsOnly
                                text: String(root.trainingData.splitSeed !== undefined ? root.trainingData.splitSeed : 42)
                                onEditingFinished: root.saveSetting("splitSeed", Number(text))
                            }
                            Item { Layout.fillWidth: true }
                        }
                    }
                }
                RowLayout {
                    id: splitBox
                    width: parent.width
                    spacing: 8
                    visible: root.splitExpanded
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
                    visible: root.splitExpanded
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
                        onClicked: root.control.trainAction("start_train", {
                            "projectFilePath": ((((root.control || {}).uiState || {}).project || {}).projectFilePath || "")
                        })
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

                Item {
                    width: parent.width
                    height: 26

                    ProgressBar {
                        id: trainProgressBar
                        anchors.fill: parent
                        from: 0
                        to: 100
                        value: Number(root.trainingData.progress || 0)
                        background: Rectangle {
                            radius: 3
                            color: "#141a26"
                            border.color: root.borderColor
                            border.width: 1
                        }
                    }

                    Text {
                        id: progressPercentText
                        anchors.centerIn: parent
                        text: Math.round(Number(root.trainingData.progress || 0)) + "%"
                        color: "#ffffff"
                        font.pixelSize: 12
                        font.bold: true
                        z: 2
                    }
                }

                Rectangle {
                    width: parent.width
                    color: "#1b2231"
                    radius: 6
                    border.color: root.borderColor
                    border.width: 1
                    implicitHeight: statsGrid.implicitHeight + 12

                    GridLayout {
                        id: statsGrid
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: 6
                        columns: 4
                        columnSpacing: 8
                        rowSpacing: 4

                        Text { text: "Stats"; color: root.subTextColor; Layout.columnSpan: 4; font.pixelSize: 12 }

                        Text { text: "Epoch"; color: root.subTextColor }
                        Text { text: root.statText("epoch", "-") + "/" + root.statText("total_epochs", "-"); color: root.textColor }
                        Text { text: "Best Val"; color: root.subTextColor }
                        Text { text: root.statText("best_val_acc", "-"); color: root.textColor }

                        Text { text: "Train Loss"; color: root.subTextColor }
                        Text { text: root.statText("train_loss", "-"); color: root.textColor }
                        Text { text: "Train Acc"; color: root.subTextColor }
                        Text { text: root.statText("train_acc", "-"); color: root.textColor }

                        Text { text: "Val Loss"; color: root.subTextColor }
                        Text { text: root.statText("val_loss", "-"); color: root.textColor }
                        Text { text: "Val Acc"; color: root.subTextColor }
                        Text { text: root.statText("val_acc", "-"); color: root.textColor }

                        Text { text: "Test Loss"; color: root.subTextColor }
                        Text { text: root.statText("test_loss", "-"); color: root.textColor }
                        Text { text: "Test Acc"; color: root.subTextColor }
                        Text { text: root.statText("test_acc", "-"); color: root.textColor }

                        Text { text: "Elapsed"; color: root.subTextColor }
                        Text { text: root.formatSeconds(root.statsData.elapsed_sec); color: root.textColor }
                        Text { text: "Epoch Time"; color: root.subTextColor }
                        Text { text: root.formatSeconds(root.statsData.epoch_sec); color: root.textColor }

                        Text { text: "ETA"; color: root.subTextColor }
                        Text { text: root.formatSeconds(root.statsData.eta_sec); color: root.textColor }
                        Item { Layout.columnSpan: 2 }
                    }
                }

                ScrollView {
                    width: parent.width
                    height: 160
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded

                    TextArea {
                        id: trainingLogArea
                        width: parent.width
                        readOnly: true
                        wrapMode: TextArea.Wrap
                        color: root.textColor
                        text: String(root.trainingData.logText || "")
                        onTextChanged: {
                            cursorPosition = length
                        }
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
}
