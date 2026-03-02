import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import Labeling 1.0

Window {
    id: root
    width: 1280
    height: 760
    minimumWidth: 1000
    minimumHeight: 620
    visible: true
    title: qsTr("Labeling Tool")

    property color bgColor: "#0f1116"
    property color panelColor: "#1a1f2b"
    property color cardColor: "#222836"
    property color borderColor: "#2f3646"
    property color textColor: "#e6ebf5"
    property color subTextColor: "#9aa7bf"
    property color accentColor: "#4da3ff"
    property bool replaceImagesOnImport: false
    property bool editAnnotationMode: false
    property string pendingClassName: ""

    component DarkButton: Button {
        id: darkButton
        implicitHeight: 30

        contentItem: Text {
            text: darkButton.text
            color: darkButton.enabled ? "#e6ebf5" : "#7f8aa1"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            radius: 6
            border.width: 1
            border.color: !darkButton.enabled
                          ? "#2d3443"
                          : darkButton.checked
                            ? "#7ab4ff"
                            : (darkButton.hovered ? "#5a6478" : root.borderColor)
            color: !darkButton.enabled
                   ? "#252c3a"
                   : darkButton.down
                     ? "#3b4a63"
                     : darkButton.checked
                       ? "#4da3ff"
                       : "#30394b"
        }
    }

    Component.onCompleted: {
        if (control.currentImageIndex >= 0) {
            control.selectImage(control.currentImageIndex)
        }
        pendingClassName = control.currentClassName
        projectNameField.text = control.projectName
    }

    function baseName(path) {
        if (!path)
            return ""
        let normalized = String(path).replace("file://", "")
        let parts = normalized.split("/")
        return parts[parts.length - 1]
    }

    Connections {
        target: control
        function onSelectedAnnotationClassNameChanged() {
            if (control.selectedPolygonIndex >= 0 &&
                    control.selectedAnnotationClassName.length > 0) {
                root.pendingClassName = control.selectedAnnotationClassName
            }
        }
        function onCurrentClassNameChanged() {
            if (control.selectedPolygonIndex < 0) {
                root.pendingClassName = control.currentClassName
            }
        }
        function onProjectChanged() {
            projectNameField.text = control.projectName
        }
    }

    Rectangle {
        anchors.fill: parent
        color: root.bgColor

        Row {
            anchors.fill: parent

            Rectangle {
                id: leftPane
                width: Math.round(parent.width * 0.75)
                height: parent.height
                color: "transparent"

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 16
                    radius: 12
                    color: "#161b26"
                    border.color: root.borderColor
                    border.width: 1

                    ImageDisplay {
                        id: imageBox
                        anchors.fill: parent
                        anchors.margins: 10
                        controller: control
                        editMode: root.editAnnotationMode
                        selectedPolygonIndex: control.selectedPolygonIndex
                        savedPolygons: control.currentPolygons
                        annotationPoints: control.draftPoints
                        onImageClicked: function(x, y) {
                            control.addAnnotationPoint(x, y)
                        }
                        onAnnotationPointEdited: function(polygonIndex, pointIndex, x, y) {
                            control.updateAnnotationPoint(polygonIndex, pointIndex, x, y)
                        }
                        onDraftPointEdited: function(pointIndex, x, y) {
                            control.updateDraftPoint(pointIndex, x, y)
                        }
                        onPolygonSelected: function(polygonIndex) {
                            control.setSelectedPolygonIndex(polygonIndex)
                        }
                    }
                }
            }

            Rectangle {
                id: rightPanel
                width: parent.width - leftPane.width
                height: parent.height
                color: root.panelColor
                border.color: root.borderColor
                border.width: 1
                property int currentTab: 0

                Row {
                    id: rightTabRow
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 12
                    spacing: 8

                    DarkButton {
                        width: Math.floor((rightTabRow.width - rightTabRow.spacing * 3) / 4)
                        text: "Labeling"
                        checkable: true
                        checked: rightPanel.currentTab === 0
                        onClicked: rightPanel.currentTab = 0
                    }
                    DarkButton {
                        width: Math.floor((rightTabRow.width - rightTabRow.spacing * 3) / 4)
                        text: "Training"
                        checkable: true
                        checked: rightPanel.currentTab === 1
                        onClicked: rightPanel.currentTab = 1
                    }
                    DarkButton {
                        width: Math.floor((rightTabRow.width - rightTabRow.spacing * 3) / 4)
                        text: "Inference"
                        checkable: true
                        checked: rightPanel.currentTab === 2
                        onClicked: rightPanel.currentTab = 2
                    }
                    DarkButton {
                        width: Math.floor((rightTabRow.width - rightTabRow.spacing * 3) / 4)
                        text: "Settings"
                        checkable: true
                        checked: rightPanel.currentTab === 3
                        onClicked: rightPanel.currentTab = 3
                    }
                }

                ScrollView {
                    visible: rightPanel.currentTab === 0
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: rightTabRow.bottom
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    anchors.bottomMargin: 12
                    anchors.topMargin: 8

                    Column {
                        width: rightPanel.width - 24
                        spacing: 10

                        // projectCol
                        Rectangle {
                            width: parent.width
                            color: root.cardColor
                            radius: 10
                            border.color: root.borderColor
                            border.width: 1
                            implicitHeight: projectCol.implicitHeight + 20

                            Column {
                                id: projectCol
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.margins: 10
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 8

                                Text { text: "Project"; color: root.subTextColor; font.pixelSize: 13 }

                                TextField {
                                    id: projectNameField
                                    placeholderText: "Preferred project name"
                                    color: root.textColor
                                    placeholderTextColor: "#7885a1"
                                    selectedTextColor: "#ffffff"
                                    selectionColor: "#3a6ea5"
                                    background: Rectangle {
                                        radius: 6
                                        color: "#141a26"
                                        border.color: projectNameField.activeFocus ? root.accentColor : root.borderColor
                                        border.width: 1
                                    }
                                }

                                Row {
                                    id: projectActionRow
                                    width: parent.width
                                    spacing: 8
                                    DarkButton {
                                        width: Math.floor((projectActionRow.width - projectActionRow.spacing * 2) / 3)
                                        text: "Create"
                                        onClicked: control.createNewProject(projectNameField.text)
                                    }
                                    DarkButton {
                                        width: Math.floor((projectActionRow.width - projectActionRow.spacing * 2) / 3)
                                        text: "Load"
                                        onClicked: projectDialog.open()
                                    }
                                    DarkButton {
                                        width: Math.floor((projectActionRow.width - projectActionRow.spacing * 2) / 3)
                                        text: "Save As"
                                        enabled: control.projectFilePath.length > 0
                                        onClicked: {
                                            const currentPath = String(control.projectFilePath)
                                            const slashIndex = currentPath.lastIndexOf("/")
                                            const folder = slashIndex >= 0 ? currentPath.substring(0, slashIndex) : ""
                                            const base = String(control.projectName).trim().length > 0
                                                         ? String(control.projectName).trim()
                                                         : baseName(currentPath).replace(/\.json$/i, "")
                                            const suggested = base + "Copy.json"
                                            const folderUrl = "file://" + (folder.length > 0 ? folder : ".")
                                            saveAsDialog.currentFolder = folderUrl
                                            saveAsDialog.currentFile = folderUrl.replace(/\/$/, "") + "/" + suggested
                                            saveAsDialog.open()
                                        }
                                    }
                                }

                                Text {
                                    text: control.projectFilePath
                                    color: root.subTextColor
                                    font.pixelSize: 11
                                    wrapMode: Text.WrapAnywhere
                                }
                            }
                        }

                        // importCol
                        Rectangle {
                            width: parent.width
                            color: root.cardColor
                            radius: 10
                            border.color: root.borderColor
                            border.width: 1
                            implicitHeight: importCol.implicitHeight + 20

                            Column {
                                id: importCol
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.margins: 10
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 8

                                Text { text: "Import Images"; color: root.subTextColor; font.pixelSize: 13 }

                                Row {
                                    id: importActionRow
                                    width: parent.width
                                    spacing: 8
                                    DarkButton {
                                        width: Math.floor((importActionRow.width - importActionRow.spacing * 2) / 3)
                                        text: "Select Files"
                                        onClicked: fileDialog.open()
                                    }
                                    DarkButton {
                                        width: Math.floor((importActionRow.width - importActionRow.spacing * 2) / 3)
                                        text: "Select Folder"
                                        onClicked: folderDialog.open()
                                    }
                                    CheckBox {
                                        id: replaceImportCheck
                                        width: Math.floor((importActionRow.width - importActionRow.spacing * 2) / 3)
                                        text: "Replace"
                                        checked: root.replaceImagesOnImport
                                        onToggled: root.replaceImagesOnImport = checked

                                        contentItem: Text {
                                            text: replaceImportCheck.text
                                            color: root.textColor
                                            verticalAlignment: Text.AlignVCenter
                                            leftPadding: replaceImportCheck.indicator.width + replaceImportCheck.spacing
                                        }
                                    }
                                }

                                Text {
                                    text: "Images: " + control.imageCount
                                    color: root.textColor
                                    font.pixelSize: 13
                                }
                            }
                        }

                        // navCol
                        Rectangle {
                            width: parent.width
                            color: root.cardColor
                            radius: 10
                            border.color: root.borderColor
                            border.width: 1
                            implicitHeight: navCol.implicitHeight + 20

                            Column {
                                id: navCol
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.margins: 10
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 8

                                Text { text: "Image Navigation"; color: root.subTextColor; font.pixelSize: 13 }

                                Row {
                                    id: navButtonRow
                                    width: parent.width
                                    spacing: 6
                                    property int buttonWidth: Math.max(36, Math.floor((width - spacing * 5) / 6))

                                    DarkButton {
                                        width: navButtonRow.buttonWidth
                                        text: "|<"
                                        enabled: control.imageCount > 0
                                        onClicked: control.firstImage()
                                        ToolTip.visible: hovered
                                        ToolTip.text: "First"
                                    }
                                    DarkButton {
                                        width: navButtonRow.buttonWidth
                                        text: "<<"
                                        enabled: control.imageCount > 0
                                        onClicked: control.previous10Images()
                                        ToolTip.visible: hovered
                                        ToolTip.text: "Prev 10"
                                    }
                                    DarkButton {
                                        width: navButtonRow.buttonWidth
                                        text: "<"
                                        enabled: control.imageCount > 0
                                        onClicked: control.previousImage()
                                        ToolTip.visible: hovered
                                        ToolTip.text: "Prev"
                                    }
                                    DarkButton {
                                        width: navButtonRow.buttonWidth
                                        text: ">"
                                        enabled: control.imageCount > 0
                                        onClicked: control.nextImage()
                                        ToolTip.visible: hovered
                                        ToolTip.text: "Next"
                                    }
                                    DarkButton {
                                        width: navButtonRow.buttonWidth
                                        text: ">>"
                                        enabled: control.imageCount > 0
                                        onClicked: control.next10Images()
                                        ToolTip.visible: hovered
                                        ToolTip.text: "Next 10"
                                    }
                                    DarkButton {
                                        width: navButtonRow.buttonWidth
                                        text: ">|"
                                        enabled: control.imageCount > 0
                                        onClicked: control.lastImage()
                                        ToolTip.visible: hovered
                                        ToolTip.text: "Last"
                                    }
                                }

                                Text {
                                    text: control.imageCount > 0
                                          ? ("Current: " + (control.currentImageIndex + 1) + " / " + control.imageCount)
                                          : "Current: -"
                                    color: root.textColor
                                    font.pixelSize: 13
                                }
                                Text {
                                    text: baseName(control.currentImagePath)
                                    color: root.subTextColor
                                    font.pixelSize: 11
                                    wrapMode: Text.WrapAnywhere
                                }
                            }
                        }

                        // annCol
                        Rectangle {
                            width: parent.width
                            color: root.cardColor
                            radius: 10
                            border.color: root.borderColor
                            border.width: 1
                            implicitHeight: annCol.implicitHeight + 20

                            Column {
                                id: annCol
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.margins: 10
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 8
                                property int metaLabelWidth: 46
                                property int metaSpacing: 8
                                property int metaOptionWidth: Math.max(54, Math.floor((width - metaLabelWidth - metaSpacing * 4) / 4))

                                Text { text: "Annotation"; color: root.subTextColor; font.pixelSize: 13 }

                                Row {
                                    spacing: 12
                                    CheckBox {
                                        id: showLabelCheck
                                        text: "Label"
                                        checked: control.showLabel
                                        onToggled: control.showLabel = checked
                                        contentItem: Text {
                                            text: showLabelCheck.text
                                            color: root.textColor
                                            verticalAlignment: Text.AlignVCenter
                                            leftPadding: showLabelCheck.indicator.width + showLabelCheck.spacing
                                        }
                                    }
                                    CheckBox {
                                        id: showPredictCheck
                                        text: "Predict"
                                        checked: control.showPredict
                                        onToggled: control.showPredict = checked
                                        contentItem: Text {
                                            text: showPredictCheck.text
                                            color: root.textColor
                                            verticalAlignment: Text.AlignVCenter
                                            leftPadding: showPredictCheck.indicator.width + showPredictCheck.spacing
                                        }
                                    }
                                    CheckBox {
                                        id: showGoodCheck
                                        text: "Good"
                                        checked: control.showGood
                                        onToggled: control.showGood = checked
                                        contentItem: Text {
                                            text: showGoodCheck.text
                                            color: root.textColor
                                            verticalAlignment: Text.AlignVCenter
                                            leftPadding: showGoodCheck.indicator.width + showGoodCheck.spacing
                                        }
                                    }
                                }

                                Text {
                                    text: "Draft points: " + control.draftPoints.length
                                    color: root.textColor
                                    font.pixelSize: 13
                                }

                                Text { text: "Class"; color: root.subTextColor; font.pixelSize: 13 }

                                Row {
                                    id: classRow
                                    width: parent.width
                                    spacing: 8
                                    ComboBox {
                                        id: classCombo
                                        width: classRow.width - addClassButton.width - delClassButton.width - updateClassButton.width - (classRow.spacing * 3)
                                        model: control.classNames
                                        editable: true
                                        currentIndex: Math.max(0, control.classNames.indexOf(control.currentClassName))
                                        editText: root.pendingClassName
                                        onActivated: function(index) {
                                            if (index >= 0 && index < control.classNames.length) {
                                                root.pendingClassName = control.classNames[index]
                                                control.setCurrentClassName(control.classNames[index])
                                            }
                                        }
                                        onEditTextChanged: root.pendingClassName = editText
                                    }
                                    DarkButton {
                                        id: addClassButton
                                        width: 34
                                        text: "+"
                                        onClicked: {
                                            const name = classCombo.editText.trim()
                                            if (name.length === 0)
                                                return
                                            if (control.addClassName(name)) {
                                                root.pendingClassName = name
                                            }
                                        }
                                    }
                                    DarkButton {
                                        id: delClassButton
                                        width: 34
                                        text: "-"
                                        onClicked: {
                                            const name = classCombo.editText.trim()
                                            if (name.length === 0)
                                                return
                                            if (control.deleteClassName(name)) {
                                                root.pendingClassName = control.currentClassName
                                            }
                                        }
                                    }
                                    DarkButton {
                                        id: updateClassButton
                                        width: 72
                                        text: "Update"
                                        enabled: control.selectedPolygonIndex >= 0
                                        onClicked: control.updateSelectedAnnotationDetails(classCombo.editText.trim())
                                    }
                                }

                                Row {
                                    id: typeRow
                                    spacing: annCol.metaSpacing
                                    Text {
                                        text: "Type:"
                                        color: root.subTextColor
                                        font.pixelSize: 12
                                        width: annCol.metaLabelWidth
                                        height: 28
                                        horizontalAlignment: Text.AlignRight
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    RadioButton {
                                        id: typeLabelRadio
                                        width: annCol.metaOptionWidth
                                        text: "label"
                                        checked: control.annotationKind === "label"
                                        onClicked: control.annotationKind = "label"
                                        contentItem: Text {
                                            text: typeLabelRadio.text
                                            color: root.textColor
                                            verticalAlignment: Text.AlignVCenter
                                            leftPadding: typeLabelRadio.indicator.width + typeLabelRadio.spacing
                                        }
                                    }
                                    RadioButton {
                                        id: typePredictRadio
                                        width: annCol.metaOptionWidth
                                        text: "predict"
                                        checked: control.annotationKind === "predict"
                                        onClicked: control.annotationKind = "predict"
                                        contentItem: Text {
                                            text: typePredictRadio.text
                                            color: root.textColor
                                            verticalAlignment: Text.AlignVCenter
                                            leftPadding: typePredictRadio.indicator.width + typePredictRadio.spacing
                                        }
                                    }
                                    RadioButton {
                                        id: typeGoodRadio
                                        width: annCol.metaOptionWidth
                                        text: "good"
                                        checked: control.annotationKind === "good"
                                        onClicked: control.annotationKind = "good"
                                        contentItem: Text {
                                            text: typeGoodRadio.text
                                            color: root.textColor
                                            verticalAlignment: Text.AlignVCenter
                                            leftPadding: typeGoodRadio.indicator.width + typeGoodRadio.spacing
                                        }
                                    }
                                    Item { width: annCol.metaOptionWidth; height: 1 }
                                }

                                Row {
                                    spacing: annCol.metaSpacing
                                    Text {
                                        text: "Level:"
                                        color: root.subTextColor
                                        font.pixelSize: 12
                                        width: annCol.metaLabelWidth
                                        height: 28
                                        horizontalAlignment: Text.AlignRight
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    RadioButton {
                                        id: levelSeriousRadio
                                        width: annCol.metaOptionWidth
                                        text: "serious"
                                        checked: control.annotationSeverity === "serious"
                                        onClicked: control.annotationSeverity = "serious"
                                        contentItem: Text {
                                            text: levelSeriousRadio.text
                                            color: root.textColor
                                            verticalAlignment: Text.AlignVCenter
                                            leftPadding: levelSeriousRadio.indicator.width + levelSeriousRadio.spacing
                                        }
                                    }
                                    RadioButton {
                                        id: levelNormalRadio
                                        width: annCol.metaOptionWidth
                                        text: "normal"
                                        checked: control.annotationSeverity === "normal"
                                        onClicked: control.annotationSeverity = "normal"
                                        contentItem: Text {
                                            text: levelNormalRadio.text
                                            color: root.textColor
                                            verticalAlignment: Text.AlignVCenter
                                            leftPadding: levelNormalRadio.indicator.width + levelNormalRadio.spacing
                                        }
                                    }
                                    RadioButton {
                                        id: levelMarginalRadio
                                        width: annCol.metaOptionWidth
                                        text: "marginal"
                                        checked: control.annotationSeverity === "marginal"
                                        onClicked: control.annotationSeverity = "marginal"
                                        contentItem: Text {
                                            text: levelMarginalRadio.text
                                            color: root.textColor
                                            verticalAlignment: Text.AlignVCenter
                                            leftPadding: levelMarginalRadio.indicator.width + levelMarginalRadio.spacing
                                        }
                                    }
                                    Item { width: annCol.metaOptionWidth; height: 1 }
                                }

                                Row {
                                    spacing: annCol.metaSpacing
                                    Text {
                                        text: "Split:"
                                        color: root.subTextColor
                                        font.pixelSize: 12
                                        width: annCol.metaLabelWidth
                                        height: 28
                                        horizontalAlignment: Text.AlignRight
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    RadioButton {
                                        id: splitNoneRadio
                                        width: annCol.metaOptionWidth
                                        text: "none"
                                        checked: control.annotationSplit === "none"
                                        onClicked: control.annotationSplit = "none"
                                        contentItem: Text {
                                            text: splitNoneRadio.text
                                            color: root.textColor
                                            verticalAlignment: Text.AlignVCenter
                                            leftPadding: splitNoneRadio.indicator.width + splitNoneRadio.spacing
                                        }
                                    }
                                    RadioButton {
                                        id: splitTrainRadio
                                        width: annCol.metaOptionWidth
                                        text: "train"
                                        checked: control.annotationSplit === "train"
                                        onClicked: control.annotationSplit = "train"
                                        contentItem: Text {
                                            text: splitTrainRadio.text
                                            color: root.textColor
                                            verticalAlignment: Text.AlignVCenter
                                            leftPadding: splitTrainRadio.indicator.width + splitTrainRadio.spacing
                                        }
                                    }
                                    RadioButton {
                                        id: splitValRadio
                                        width: annCol.metaOptionWidth
                                        text: "val"
                                        checked: control.annotationSplit === "val"
                                        onClicked: control.annotationSplit = "val"
                                        contentItem: Text {
                                            text: splitValRadio.text
                                            color: root.textColor
                                            verticalAlignment: Text.AlignVCenter
                                            leftPadding: splitValRadio.indicator.width + splitValRadio.spacing
                                        }
                                    }
                                    RadioButton {
                                        id: splitTestRadio
                                        width: annCol.metaOptionWidth
                                        text: "test"
                                        checked: control.annotationSplit === "test"
                                        onClicked: control.annotationSplit = "test"
                                        contentItem: Text {
                                            text: splitTestRadio.text
                                            color: root.textColor
                                            verticalAlignment: Text.AlignVCenter
                                            leftPadding: splitTestRadio.indicator.width + splitTestRadio.spacing
                                        }
                                    }
                                }

                                TextArea {
                                    id: remarksField
                                    width: parent.width
                                    text: control.annotationRemarks
                                    placeholderText: "Remarks"
                                    color: root.textColor
                                    placeholderTextColor: "#7885a1"
                                    selectedTextColor: "#ffffff"
                                    selectionColor: "#3a6ea5"
                                    wrapMode: TextEdit.Wrap
                                    implicitHeight: Math.max(72, contentHeight + 16)
                                    background: Rectangle {
                                        radius: 6
                                        color: "#141a26"
                                        border.color: remarksField.activeFocus ? root.accentColor : root.borderColor
                                        border.width: 1
                                    }
                                    onTextChanged: {
                                        if (text !== control.annotationRemarks) {
                                            control.annotationRemarks = text
                                        }
                                    }
                                }

                                Row {
                                    id: annotationActionRow
                                    width: parent.width
                                    spacing: 8
                                    property int buttonWidth: Math.max(44, Math.floor((width - spacing * 5) / 6))
                                    DarkButton {
                                        width: annotationActionRow.buttonWidth
                                        text: "New"
                                        onClicked: control.startNewAnnotation()
                                    }
                                    DarkButton {
                                        width: annotationActionRow.buttonWidth
                                        text: "Undo"
                                        enabled: control.draftPoints.length > 0
                                        onClicked: control.undoLastPoint()
                                    }
                                    DarkButton {
                                        width: annotationActionRow.buttonWidth
                                        text: "Clear"
                                        enabled: control.draftPoints.length > 0
                                        onClicked: control.clearDraftPoints()
                                    }
                                    DarkButton {
                                        width: annotationActionRow.buttonWidth
                                        text: "Save"
                                        enabled: control.draftPoints.length >= 3
                                        onClicked: control.saveCurrentAnnotation()
                                    }
                                    DarkButton {
                                        width: annotationActionRow.buttonWidth
                                        checkable: true
                                        checked: root.editAnnotationMode
                                        text: checked ? "Editing" : "Edit"
                                        enabled: control.selectedPolygonIndex >= 0
                                        onClicked: root.editAnnotationMode = checked
                                    }
                                    DarkButton {
                                        width: annotationActionRow.buttonWidth
                                        text: "Del"
                                        enabled: control.selectedPolygonIndex >= 0 || control.draftPoints.length > 0
                                        onClicked: {
                                            if (control.selectedPolygonIndex >= 0) {
                                                control.deleteSelectedAnnotation()
                                            } else {
                                                control.clearDraftPoints()
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // fileList
                        Rectangle {
                            width: parent.width
                            height: 160
                            color: root.cardColor
                            radius: 10
                            border.color: root.borderColor
                            border.width: 1

                            ListView {
                                anchors.fill: parent
                                anchors.margins: 8
                                model: control.imagePaths
                                clip: true

                                delegate: Rectangle {
                                    required property var modelData
                                    required property int index
                                    width: ListView.view.width
                                    height: 26
                                    color: control.currentImageIndex === index ? "#2c3b56" : "transparent"

                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.left: parent.left
                                        anchors.leftMargin: 6
                                        text: baseName(modelData)
                                        color: root.textColor
                                        font.pixelSize: 12
                                        elide: Text.ElideRight
                                        width: parent.width - 12
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: control.selectImage(index)
                                    }
                                }
                            }
                        }

                        // statusCol
                        Rectangle {
                            width: parent.width
                            color: root.cardColor
                            radius: 10
                            border.color: root.borderColor
                            border.width: 1
                            implicitHeight: statusCol.implicitHeight + 20

                            Column {
                                id: statusCol
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.margins: 10
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 6

                                Text { text: "Status"; color: root.subTextColor; font.pixelSize: 13 }
                                Text {
                                    text: control.status
                                    color: root.textColor
                                    font.pixelSize: 12
                                    wrapMode: Text.WrapAnywhere
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    visible: rightPanel.currentTab === 1
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: rightTabRow.bottom
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    anchors.bottomMargin: 12
                    anchors.topMargin: 8
                    radius: 10
                    color: root.cardColor
                    border.color: root.borderColor
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: "Training tab placeholder"
                        color: root.subTextColor
                        font.pixelSize: 14
                    }
                }

                Rectangle {
                    visible: rightPanel.currentTab === 2
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: rightTabRow.bottom
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    anchors.bottomMargin: 12
                    anchors.topMargin: 8
                    radius: 10
                    color: root.cardColor
                    border.color: root.borderColor
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: "Inference tab placeholder"
                        color: root.subTextColor
                        font.pixelSize: 14
                    }
                }

                Rectangle {
                    visible: rightPanel.currentTab === 3
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: rightTabRow.bottom
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    anchors.bottomMargin: 12
                    anchors.topMargin: 8
                    radius: 10
                    color: root.cardColor
                    border.color: root.borderColor
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: "Settings tab placeholder"
                        color: root.subTextColor
                        font.pixelSize: 14
                    }
                }
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: "Select images"
        fileMode: FileDialog.OpenFiles
        nameFilters: ["Images (*.png *.jpg *.jpeg *.bmp *.tif *.tiff)"]
        onAccepted: control.importImageFileUrls(selectedFiles, root.replaceImagesOnImport)
    }

    FolderDialog {
        id: folderDialog
        title: "Select image folder"
        onAccepted: control.importImageFolder(selectedFolder.toString(), root.replaceImagesOnImport)
    }

    FileDialog {
        id: projectDialog
        title: "Load project"
        fileMode: FileDialog.OpenFile
        nameFilters: ["Project (*.json)"]
        onAccepted: {
            if (selectedFile)
                control.loadProjectFile(selectedFile.toString())
        }
    }

    FileDialog {
        id: saveAsDialog
        title: "Save config as"
        fileMode: FileDialog.SaveFile
        options: FileDialog.DontUseNativeDialog
        nameFilters: ["Project (*.json)"]
        onAccepted: {
            if (selectedFile)
                control.saveProjectAs(selectedFile.toString())
        }
    }
}
