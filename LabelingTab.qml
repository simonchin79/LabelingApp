import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

ScrollView {
    id: root
    required property var control
    property color cardColor: "#222836"
    property color borderColor: "#2f3646"
    property color textColor: "#e6ebf5"
    property color subTextColor: "#9aa7bf"
    property color accentColor: "#4da3ff"
    property bool replaceImagesOnImport: false
    property bool editAnnotationMode: false
    property string pendingClassName: ""
    property int listMode: 0 // 0=image, 1=current anno, 2=all anno
    property var listRows: []
    property string sortKey: "fileName"
    property bool sortAscending: true

    function normalizeSortValue(value) {
        if (value === undefined || value === null)
            return ""
        return String(value).toLowerCase()
    }

    function compareValues(left, right, key) {
        const numericKeys = ["labelCount", "predictCount", "goodCount", "imageIndex", "annotationIndex"]
        if (numericKeys.indexOf(key) >= 0) {
            const l = Number(left || 0)
            const r = Number(right || 0)
            if (l < r) return -1
            if (l > r) return 1
            return 0
        }
        const ls = normalizeSortValue(left)
        const rs = normalizeSortValue(right)
        if (ls < rs) return -1
        if (ls > rs) return 1
        return 0
    }

    function applySort(rows) {
        const copied = rows.slice(0)
        copied.sort(function(a, b) {
            const result = compareValues(a[sortKey], b[sortKey], sortKey)
            return sortAscending ? result : -result
        })
        return copied
    }

    function toggleSort(key) {
        if (sortKey === key) {
            sortAscending = !sortAscending
        } else {
            sortKey = key
            sortAscending = true
        }
        refreshListRows()
    }

    function sortMarker(key) {
        if (sortKey !== key)
            return ""
        return sortAscending ? " ▲" : " ▼"
    }

    function refreshListRows() {
        if (!root.control)
            return
        if (root.listMode === 0) {
            root.listRows = applySort(root.control.imageSummaryList())
            return
        }
        let sourceRows = root.listMode === 1
                ? root.control.annotationListForCurrentImage()
                : root.control.annotationListForAllImages()
        const keyword = imageFilterField.text.trim().toLowerCase()
        if (keyword.length === 0) {
            root.listRows = applySort(sourceRows)
            return
        }
        let filtered = []
        for (let i = 0; i < sourceRows.length; ++i) {
            const row = sourceRows[i]
            const imageFileName = String(row.imageFileName || "").toLowerCase()
            const className = String(row.className || "").toLowerCase()
            const typeValue = String(row.type || "").toLowerCase()
            const levelValue = String(row.level || "").toLowerCase()
            const splitValue = String(row.split || "").toLowerCase()
            const remarksValue = String(row.remarks || "").toLowerCase()
            if (imageFileName.indexOf(keyword) >= 0
                    || className.indexOf(keyword) >= 0
                    || typeValue === keyword
                    || levelValue === keyword
                    || splitValue === keyword
                    || remarksValue.indexOf(keyword) >= 0)
                filtered.push(sourceRows[i])
        }
        root.listRows = applySort(filtered)
    }

    function openListPopup(mode) {
        root.listMode = mode
        if (mode === 0 && ["fileName", "labelCount", "predictCount", "goodCount"].indexOf(sortKey) < 0) {
            sortKey = "fileName"
            sortAscending = true
        }
        if (mode !== 0 && ["imageFileName", "className", "type", "level", "split", "remarks"].indexOf(sortKey) < 0) {
            sortKey = "imageFileName"
            sortAscending = true
        }
        imageFilterField.text = ""
        refreshListRows()
        listPopup.open()
    }

    function baseName(path) {
        if (!path)
            return ""
        let normalized = String(path).replace("file://", "")
        let parts = normalized.split("/")
        return parts[parts.length - 1]
    }

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

    component SortHeader: Rectangle {
        id: sortHeader
        required property string label
        required property string sortId
        property color textColor: root.subTextColor
        property bool activeSort: root.sortKey === sortHeader.sortId
        radius: 4
        color: mouseArea.pressed ? "#243149" : (mouseArea.containsMouse ? "#202b41" : "transparent")

        Text {
            anchors.fill: parent
            text: sortHeader.label + root.sortMarker(sortHeader.sortId)
            color: sortHeader.activeSort ? root.textColor : sortHeader.textColor
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: root.toggleSort(sortHeader.sortId)
        }
    }

    Component.onCompleted: {
        root.pendingClassName = root.control.currentClassName
        projectNameField.text = root.control.projectName
    }

    Connections {
        target: root.control

        function onSelectedAnnotationClassNameChanged() {
            if (root.control.selectedPolygonIndex >= 0 &&
                    root.control.selectedAnnotationClassName.length > 0) {
                root.pendingClassName = root.control.selectedAnnotationClassName
            }
        }

        function onCurrentClassNameChanged() {
            if (root.control.selectedPolygonIndex < 0) {
                root.pendingClassName = root.control.currentClassName
            }
        }

        function onProjectChanged() {
            projectNameField.text = root.control.projectName
        }
    }

    Column {
        width: root.width - 24
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
        // remainder of labeling panel unchanged

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

                Row {
                    id: ioActionRow
                    width: parent.width
                    spacing: 8

                    ComboBox {
                        id: ioFeatureCombo
                        width: Math.floor((ioActionRow.width - ioActionRow.spacing * 3) * 0.46)
                        model: ["Image Classification"]
                    }

                    DarkButton {
                        width: Math.floor((ioActionRow.width - ioActionRow.spacing * 3) * 0.18)
                        text: "Browse"
                        onClicked: ioFolderDialog.open()
                    }

                    DarkButton {
                        width: Math.floor((ioActionRow.width - ioActionRow.spacing * 3) * 0.18)
                        text: "Import"
                        enabled: control.ioFolderPath.length > 0
                        onClicked: {
                            if (ioFeatureCombo.currentIndex === 0) {
                                control.importClassificationFromFolder(control.ioFolderPath,
                                                                       root.replaceImagesOnImport)
                            }
                        }
                    }

                    DarkButton {
                        width: Math.floor((ioActionRow.width - ioActionRow.spacing * 3) * 0.18)
                        text: "Export"
                        enabled: control.ioFolderPath.length > 0
                        onClicked: {
                            if (ioFeatureCombo.currentIndex === 0) {
                                control.exportClassificationToFolder(control.ioFolderPath)
                            }
                        }
                    }
                }

                Text {
                    text: "I/O Folder: " + (control.ioFolderPath.length > 0
                                            ? control.ioFolderPath
                                            : "-")
                    color: root.subTextColor
                    font.pixelSize: 11
                    wrapMode: Text.WrapAnywhere
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

        // listActions
        Rectangle {
            width: parent.width
            color: root.cardColor
            radius: 10
            border.color: root.borderColor
            border.width: 1
            implicitHeight: listActionCol.implicitHeight + 20

            Column {
                id: listActionCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 10
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8

                Text { text: "Lists"; color: root.subTextColor; font.pixelSize: 13 }

                Row {
                    id: listActionRow
                    width: parent.width
                    spacing: 8

                    DarkButton {
                        width: Math.floor((listActionRow.width - listActionRow.spacing * 2) / 3)
                        text: "Image List"
                        onClicked: root.openListPopup(0)
                    }
                    DarkButton {
                        width: Math.floor((listActionRow.width - listActionRow.spacing * 2) / 3)
                        text: "Anno List"
                        onClicked: root.openListPopup(1)
                    }
                    DarkButton {
                        width: Math.floor((listActionRow.width - listActionRow.spacing * 2) / 3)
                        text: "All List"
                        onClicked: root.openListPopup(2)
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

    Popup {
        id: listPopup
        parent: Overlay.overlay
        modal: true
        focus: true
        Overlay.modal: Rectangle {
            color: "#0a0f18cc"
        }
        x: 0
        y: 0
        width: root.Window.window ? root.Window.window.width : 1280
        height: root.Window.window ? root.Window.window.height : 760
        padding: 0
        onOpened: root.refreshListRows()

        background: Rectangle {
            color: "#101522"
            border.color: root.borderColor
            border.width: 1
        }

        Column {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 10

            Row {
                width: parent.width
                spacing: 8

                Text {
                    width: parent.width - closeListButton.width - parent.spacing
                    text: root.listMode === 0
                          ? "Image List"
                          : (root.listMode === 1 ? "Anno List (Current Image)" : "All List")
                    color: root.textColor
                    font.pixelSize: 20
                    font.bold: true
                    verticalAlignment: Text.AlignVCenter
                }

                DarkButton {
                    id: closeListButton
                    width: 90
                    text: "Close"
                    onClicked: listPopup.close()
                }
            }

            Text {
                width: parent.width
                text: "Showing " + root.listRows.length + " item(s)"
                color: root.subTextColor
                font.pixelSize: 12
            }

            Row {
                visible: root.listMode !== 0
                width: parent.width
                spacing: 8

                TextField {
                    id: imageFilterField
                    width: parent.width
                    placeholderText: "Filter: Image/Class/Remarks contains, or Type/Level/Split exact"
                    color: root.textColor
                    placeholderTextColor: "#7885a1"
                    selectedTextColor: "#ffffff"
                    selectionColor: "#3a6ea5"
                    background: Rectangle {
                        radius: 6
                        color: "#141a26"
                        border.color: imageFilterField.activeFocus ? root.accentColor : root.borderColor
                        border.width: 1
                    }
                    onTextChanged: root.refreshListRows()
                }
            }

            Rectangle {
                width: parent.width
                height: 34
                radius: 6
                color: "#1b2436"
                border.color: root.borderColor
                border.width: 1

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 8

                    SortHeader {
                        width: root.listMode === 0 ? Math.floor(parent.width * 0.55) : Math.floor(parent.width * 0.25)
                        label: "Image"
                        sortId: root.listMode === 0 ? "fileName" : "imageFileName"
                        height: parent.height
                    }
                    SortHeader {
                        visible: root.listMode === 0
                        width: root.listMode === 0 ? Math.floor(parent.width * 0.12) : 0
                        label: "Label"
                        sortId: "labelCount"
                        height: parent.height
                    }
                    SortHeader {
                        visible: root.listMode === 0
                        width: root.listMode === 0 ? Math.floor(parent.width * 0.12) : 0
                        label: "Predict"
                        sortId: "predictCount"
                        height: parent.height
                    }
                    SortHeader {
                        visible: root.listMode === 0
                        width: root.listMode === 0 ? Math.floor(parent.width * 0.12) : 0
                        label: "Good"
                        sortId: "goodCount"
                        height: parent.height
                    }

                    SortHeader {
                        visible: root.listMode !== 0
                        width: root.listMode !== 0 ? Math.floor(parent.width * 0.12) : 0
                        label: "Class"
                        sortId: "className"
                        height: parent.height
                    }
                    SortHeader {
                        visible: root.listMode !== 0
                        width: root.listMode !== 0 ? Math.floor(parent.width * 0.10) : 0
                        label: "Type"
                        sortId: "type"
                        height: parent.height
                    }
                    SortHeader {
                        visible: root.listMode !== 0
                        width: root.listMode !== 0 ? Math.floor(parent.width * 0.10) : 0
                        label: "Level"
                        sortId: "level"
                        height: parent.height
                    }
                    SortHeader {
                        visible: root.listMode !== 0
                        width: root.listMode !== 0 ? Math.floor(parent.width * 0.10) : 0
                        label: "Split"
                        sortId: "split"
                        height: parent.height
                    }
                    SortHeader {
                        visible: root.listMode !== 0
                        width: root.listMode !== 0 ? Math.floor(parent.width * 0.25) : 0
                        label: "Remarks"
                        sortId: "remarks"
                        height: parent.height
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: parent.height - 94 - (root.listMode !== 0 ? 42 : 0)
                radius: 6
                color: "#0f1625"
                border.color: root.borderColor
                border.width: 1

                ListView {
                    anchors.fill: parent
                    anchors.margins: 4
                    model: root.listRows
                    clip: true
                    spacing: 2
                    boundsBehavior: Flickable.StopAtBounds

                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                        contentItem: Rectangle {
                            implicitWidth: 8
                            radius: 4
                            color: "#3a455e"
                        }
                        background: Rectangle {
                            color: "#121a2a"
                            radius: 4
                        }
                    }

                    delegate: Rectangle {
                        required property var modelData
                        required property int index
                        width: ListView.view.width
                        height: 30
                        radius: 4
                        color: index % 2 === 0 ? "#1a2336" : "#141d2f"

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            spacing: 8

                            Text {
                                width: root.listMode === 0 ? Math.floor(parent.width * 0.55) : Math.floor(parent.width * 0.25)
                                text: root.listMode === 0 ? (modelData.fileName || "") : (modelData.imageFileName || "")
                                color: root.textColor
                                font.pixelSize: 12
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                            Text {
                                visible: root.listMode === 0
                                width: root.listMode === 0 ? Math.floor(parent.width * 0.12) : 0
                                text: root.listMode === 0 ? String(modelData.labelCount || 0) : ""
                                color: root.textColor
                                font.pixelSize: 12
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                            }
                            Text {
                                visible: root.listMode === 0
                                width: root.listMode === 0 ? Math.floor(parent.width * 0.12) : 0
                                text: root.listMode === 0 ? String(modelData.predictCount || 0) : ""
                                color: root.textColor
                                font.pixelSize: 12
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                            }
                            Text {
                                visible: root.listMode === 0
                                width: root.listMode === 0 ? Math.floor(parent.width * 0.12) : 0
                                text: root.listMode === 0 ? String(modelData.goodCount || 0) : ""
                                color: root.textColor
                                font.pixelSize: 12
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                            }

                            Text {
                                visible: root.listMode !== 0
                                width: root.listMode !== 0 ? Math.floor(parent.width * 0.12) : 0
                                text: root.listMode !== 0 ? (modelData.className || "") : ""
                                color: root.textColor
                                font.pixelSize: 12
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                            Text {
                                visible: root.listMode !== 0
                                width: root.listMode !== 0 ? Math.floor(parent.width * 0.10) : 0
                                text: root.listMode !== 0 ? (modelData.type || "") : ""
                                color: root.textColor
                                font.pixelSize: 12
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                            }
                            Text {
                                visible: root.listMode !== 0
                                width: root.listMode !== 0 ? Math.floor(parent.width * 0.10) : 0
                                text: root.listMode !== 0 ? (modelData.level || "") : ""
                                color: root.textColor
                                font.pixelSize: 12
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                            }
                            Text {
                                visible: root.listMode !== 0
                                width: root.listMode !== 0 ? Math.floor(parent.width * 0.10) : 0
                                text: root.listMode !== 0 ? (modelData.split || "") : ""
                                color: root.textColor
                                font.pixelSize: 12
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                            }
                            Text {
                                visible: root.listMode !== 0
                                width: root.listMode !== 0 ? Math.floor(parent.width * 0.25) : 0
                                text: root.listMode !== 0 ? (modelData.remarks || "") : ""
                                color: root.textColor
                                font.pixelSize: 12
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled: true
                            onClicked: {
                                if (root.listMode === 0) {
                                    if (root.control.selectImage(modelData.imageIndex)) {
                                        listPopup.close()
                                    }
                                } else {
                                    if (root.control.selectAnnotationFromList(modelData.imageIndex,
                                                                              modelData.annotationIndex)) {
                                        listPopup.close()
                                    }
                                }
                            }
                        }
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
        onAccepted: root.control.importImageFileUrls(selectedFiles, root.replaceImagesOnImport)
    }

    FolderDialog {
        id: folderDialog
        title: "Select image folder"
        onAccepted: root.control.importImageFolder(selectedFolder.toString(), root.replaceImagesOnImport)
    }

    FolderDialog {
        id: ioFolderDialog
        title: "Select import/export folder"
        onAccepted: root.control.ioFolderPath = selectedFolder.toString()
    }

    FileDialog {
        id: projectDialog
        title: "Load project"
        fileMode: FileDialog.OpenFile
        nameFilters: ["Project (*.json)"]
        onAccepted: {
            if (selectedFile)
                root.control.loadProjectFile(selectedFile.toString())
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
                root.control.saveProjectAs(selectedFile.toString())
        }
    }
}
