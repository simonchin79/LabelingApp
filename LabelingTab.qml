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
    property bool projectExpanded: true
    property bool importExpanded: true
    property int listMode: 0 // 0=image, 1=current anno, 2=all anno
    property var listRows: []
    property var uiState: control ? control.uiState : ({})
    property var projectData: uiState.project || ({})
    property var imageData: uiState.image || ({})
    property var annotationData: uiState.annotation || ({})
    property var visibilityData: uiState.visibility || ({})
    property string sortKey: "fileName"
    property bool sortAscending: true

    function normalizeSortValue(value) {
        if (value === undefined || value === null)
            return ""
        return String(value).toLowerCase()
    }

    function compareValues(left, right, key) {
        const numericKeys = ["labelCount", "predictCount", "goodCount", "imageIndex", "annotationIndex", "predScore"]
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
            root.listRows = applySort(root.control.listAction("image_summary"))
            return
        }
        let sourceRows = root.listMode === 1
                ? root.control.listAction("annotation_current")
                : root.control.listAction("annotation_all")
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
            const predClass = String(row.predClass || "").toLowerCase()
            const predScore = String(row.predScore !== undefined ? row.predScore : "").toLowerCase()
            if (imageFileName.indexOf(keyword) >= 0
                    || className.indexOf(keyword) >= 0
                    || typeValue === keyword
                    || levelValue === keyword
                    || splitValue === keyword
                    || remarksValue.indexOf(keyword) >= 0
                    || predClass.indexOf(keyword) >= 0
                    || predScore.indexOf(keyword) >= 0)
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
        if (mode !== 0 && ["imageFileName", "className", "type", "level", "split", "predClass", "predScore", "remarks"].indexOf(sortKey) < 0) {
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
        root.pendingClassName = String(annotationData.currentClassName || "")
        projectNameField.text = String(projectData.projectName || "")
        if (visibilityData.projectSectionExpanded !== undefined)
            projectExpanded = Boolean(visibilityData.projectSectionExpanded)
        if (visibilityData.importSectionExpanded !== undefined)
            importExpanded = Boolean(visibilityData.importSectionExpanded)
    }

    Connections {
        target: root.control

        function onUiStateChanged() {
            const anno = root.annotationData
            const project = root.projectData
            const vis = ((root.control.uiState || {}).visibility || {})
            if (anno.selectedPolygonIndex >= 0 && String(anno.selectedAnnotationClassName || "").length > 0) {
                root.pendingClassName = String(anno.selectedAnnotationClassName || "")
            } else if (anno.selectedPolygonIndex < 0) {
                root.pendingClassName = String(anno.currentClassName || "")
            }
            projectNameField.text = String(project.projectName || "")
            if (vis.projectSectionExpanded !== undefined)
                root.projectExpanded = Boolean(vis.projectSectionExpanded)
            if (vis.importSectionExpanded !== undefined)
                root.importExpanded = Boolean(vis.importSectionExpanded)
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

                Button {
                    id: projectToggle
                    width: parent.width
                    text: (root.projectExpanded ? "▼ " : "▶ ") + "Project"
                    onClicked: {
                        root.projectExpanded = !root.projectExpanded
                        control.ioAction("set_section_expanded", { "section": "project", "expanded": root.projectExpanded })
                    }

                    contentItem: Text {
                        text: projectToggle.text
                        color: root.subTextColor
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                    background: Rectangle {
                        radius: 6
                        color: projectToggle.down ? "#30394b" : "#283043"
                        border.color: root.borderColor
                        border.width: 1
                    }
                }

                TextField {
                    id: projectNameField
                    visible: root.projectExpanded
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
                    visible: root.projectExpanded
                    width: parent.width
                    spacing: 8
                    DarkButton {
                        width: Math.floor((projectActionRow.width - projectActionRow.spacing * 2) / 3)
                        text: "Create"
                        onClicked: control.projectAction("create", { "preferredName": projectNameField.text })
                    }
                    DarkButton {
                        width: Math.floor((projectActionRow.width - projectActionRow.spacing * 2) / 3)
                        text: "Load"
                        onClicked: projectDialog.open()
                    }
                    DarkButton {
                        width: Math.floor((projectActionRow.width - projectActionRow.spacing * 2) / 3)
                        text: "Save As"
                        enabled: String(projectData.projectFilePath || "").length > 0
                        onClicked: {
                            const currentPath = String(projectData.projectFilePath)
                            const slashIndex = currentPath.lastIndexOf("/")
                            const folder = slashIndex >= 0 ? currentPath.substring(0, slashIndex) : ""
                            const base = String(projectData.projectName).trim().length > 0
                                       ? String(projectData.projectName).trim()
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
                    visible: root.projectExpanded
                    text: projectData.projectFilePath
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

                Button {
                    id: importToggle
                    width: parent.width
                    text: (root.importExpanded ? "▼ " : "▶ ") + "Import/Export"
                    onClicked: {
                        root.importExpanded = !root.importExpanded
                        control.ioAction("set_section_expanded", { "section": "import", "expanded": root.importExpanded })
                    }

                    contentItem: Text {
                        text: importToggle.text
                        color: root.subTextColor
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                    background: Rectangle {
                        radius: 6
                        color: importToggle.down ? "#30394b" : "#283043"
                        border.color: root.borderColor
                        border.width: 1
                    }
                }

                Row {
                    id: importActionRow
                    visible: root.importExpanded
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
                    visible: root.importExpanded
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
                        enabled: String(visibilityData.ioFolderPath || "").length > 0
                        onClicked: {
                            if (ioFeatureCombo.currentIndex === 0) {
                                control.ioAction("import_classification", { "folderPath": visibilityData.ioFolderPath, "clearExisting": root.replaceImagesOnImport })
                            }
                        }
                    }

                    DarkButton {
                        width: Math.floor((ioActionRow.width - ioActionRow.spacing * 3) * 0.18)
                        text: "Export"
                        enabled: String(visibilityData.ioFolderPath || "").length > 0
                        onClicked: {
                            if (ioFeatureCombo.currentIndex === 0) {
                                control.ioAction("export_classification", { "folderPath": visibilityData.ioFolderPath })
                            }
                        }
                    }
                }

                Text {
                    visible: root.importExpanded
                    text: "I/O Folder: " + (String(visibilityData.ioFolderPath || "").length > 0
                                            ? visibilityData.ioFolderPath
                                            : "-")
                    color: root.subTextColor
                    font.pixelSize: 11
                    wrapMode: Text.WrapAnywhere
                }

                Text {
                    visible: root.importExpanded
                    text: "Images: " + imageData.imageCount
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
                        enabled: imageData.imageCount > 0
                        onClicked: control.imageAction("first")
                        ToolTip.visible: hovered
                        ToolTip.text: "First"
                    }
                    DarkButton {
                        width: navButtonRow.buttonWidth
                        text: "<<"
                        enabled: imageData.imageCount > 0
                        onClicked: control.imageAction("prev10")
                        ToolTip.visible: hovered
                        ToolTip.text: "Prev 10"
                    }
                    DarkButton {
                        width: navButtonRow.buttonWidth
                        text: "<"
                        enabled: imageData.imageCount > 0
                        onClicked: control.imageAction("prev")
                        ToolTip.visible: hovered
                        ToolTip.text: "Prev"
                    }
                    DarkButton {
                        width: navButtonRow.buttonWidth
                        text: ">"
                        enabled: imageData.imageCount > 0
                        onClicked: control.imageAction("next")
                        ToolTip.visible: hovered
                        ToolTip.text: "Next"
                    }
                    DarkButton {
                        width: navButtonRow.buttonWidth
                        text: ">>"
                        enabled: imageData.imageCount > 0
                        onClicked: control.imageAction("next10")
                        ToolTip.visible: hovered
                        ToolTip.text: "Next 10"
                    }
                    DarkButton {
                        width: navButtonRow.buttonWidth
                        text: ">|"
                        enabled: imageData.imageCount > 0
                        onClicked: control.imageAction("last")
                        ToolTip.visible: hovered
                        ToolTip.text: "Last"
                    }
                }

                Text {
                    text: imageData.imageCount > 0
                          ? ("Current: " + (imageData.currentImageIndex + 1) + " / " + imageData.imageCount)
                          : "Current: -"
                    color: root.textColor
                    font.pixelSize: 13
                }
                Text {
                    text: baseName(imageData.currentImagePath)
                    color: root.subTextColor
                    font.pixelSize: 11
                    wrapMode: Text.WrapAnywhere
                }

                Row {
                    id: navListActionRow
                    width: parent.width
                    spacing: 8

                    DarkButton {
                        width: Math.floor((navListActionRow.width - navListActionRow.spacing * 2) / 3)
                        text: "Image List"
                        onClicked: root.openListPopup(0)
                    }
                    DarkButton {
                        width: Math.floor((navListActionRow.width - navListActionRow.spacing * 2) / 3)
                        text: "Anno List"
                        onClicked: root.openListPopup(1)
                    }
                    DarkButton {
                        width: Math.floor((navListActionRow.width - navListActionRow.spacing * 2) / 3)
                        text: "All List"
                        onClicked: root.openListPopup(2)
                    }
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
                        checked: visibilityData.showLabel
                        onToggled: control.ioAction("set_visibility", { "key": "label", "enabled": checked })
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
                        checked: visibilityData.showPredict
                        onToggled: control.ioAction("set_visibility", { "key": "predict", "enabled": checked })
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
                        checked: visibilityData.showGood
                        onToggled: control.ioAction("set_visibility", { "key": "good", "enabled": checked })
                        contentItem: Text {
                            text: showGoodCheck.text
                            color: root.textColor
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: showGoodCheck.indicator.width + showGoodCheck.spacing
                        }
                    }
                }

                Text {
                    text: "Draft points: " + (annotationData.draftPoints || []).length
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
                        model: annotationData.classNames || []
                        editable: true
                        currentIndex: Math.max(0, (annotationData.classNames || []).indexOf(annotationData.currentClassName))
                        editText: root.pendingClassName
                        onActivated: function(index) {
                            if (index >= 0 && index < (annotationData.classNames || []).length) {
                                root.pendingClassName = annotationData.classNames[index]
                                control.annotationAction("set_current_class", { "className": annotationData.classNames[index] })
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
                            if (control.annotationAction("add_class", { "className": name })) {
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
                            if (control.annotationAction("delete_class", { "className": name })) {
                                root.pendingClassName = annotationData.currentClassName
                            }
                        }
                    }
                    DarkButton {
                        id: updateClassButton
                        width: 72
                        text: "Update"
                        enabled: annotationData.selectedPolygonIndex >= 0
                        onClicked: control.annotationAction("update_details", { "className": classCombo.editText.trim() })
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
                        checked: annotationData.annotationKind === "label"
                        onClicked: control.annotationAction("set_kind", { "kind": "label" })
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
                        checked: annotationData.annotationKind === "predict"
                        onClicked: control.annotationAction("set_kind", { "kind": "predict" })
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
                        checked: annotationData.annotationKind === "good"
                        onClicked: control.annotationAction("set_kind", { "kind": "good" })
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
                        checked: annotationData.annotationSeverity === "serious"
                        onClicked: control.annotationAction("set_severity", { "severity": "serious" })
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
                        checked: annotationData.annotationSeverity === "normal"
                        onClicked: control.annotationAction("set_severity", { "severity": "normal" })
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
                        checked: annotationData.annotationSeverity === "marginal"
                        onClicked: control.annotationAction("set_severity", { "severity": "marginal" })
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
                        checked: annotationData.annotationSplit === "none"
                        onClicked: control.annotationAction("set_split", { "split": "none" })
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
                        checked: annotationData.annotationSplit === "train"
                        onClicked: control.annotationAction("set_split", { "split": "train" })
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
                        checked: annotationData.annotationSplit === "val"
                        onClicked: control.annotationAction("set_split", { "split": "val" })
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
                        checked: annotationData.annotationSplit === "test"
                        onClicked: control.annotationAction("set_split", { "split": "test" })
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
                    text: annotationData.annotationRemarks
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
                        if (text !== annotationData.annotationRemarks) {
                            control.annotationAction("set_remarks", { "remarks": text })
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
                        onClicked: control.annotationAction("start_new")
                    }
                    DarkButton {
                        width: annotationActionRow.buttonWidth
                        text: "Undo"
                        enabled: (annotationData.draftPoints || []).length > 0
                        onClicked: control.annotationAction("undo")
                    }
                    DarkButton {
                        width: annotationActionRow.buttonWidth
                        text: "Clear"
                        enabled: (annotationData.draftPoints || []).length > 0
                        onClicked: control.annotationAction("clear")
                    }
                    DarkButton {
                        width: annotationActionRow.buttonWidth
                        text: "Save"
                        enabled: (annotationData.draftPoints || []).length >= 3
                        onClicked: control.annotationAction("save")
                    }
                    DarkButton {
                        width: annotationActionRow.buttonWidth
                        checkable: true
                        checked: root.editAnnotationMode
                        text: checked ? "Editing" : "Edit"
                        enabled: annotationData.selectedPolygonIndex >= 0
                        onClicked: root.editAnnotationMode = checked
                    }
                    DarkButton {
                        width: annotationActionRow.buttonWidth
                        text: "Del"
                        enabled: annotationData.selectedPolygonIndex >= 0 || (annotationData.draftPoints || []).length > 0
                        onClicked: {
                            if (annotationData.selectedPolygonIndex >= 0) {
                                control.annotationAction("delete")
                            } else {
                                control.annotationAction("clear")
                            }
                        }
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
                    text: projectData.status
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
                        width: root.listMode !== 0 ? Math.floor(parent.width * 0.10) : 0
                        label: "Class"
                        sortId: "className"
                        height: parent.height
                    }
                    SortHeader {
                        visible: root.listMode !== 0
                        width: root.listMode !== 0 ? Math.floor(parent.width * 0.08) : 0
                        label: "Type"
                        sortId: "type"
                        height: parent.height
                    }
                    SortHeader {
                        visible: root.listMode !== 0
                        width: root.listMode !== 0 ? Math.floor(parent.width * 0.08) : 0
                        label: "Level"
                        sortId: "level"
                        height: parent.height
                    }
                    SortHeader {
                        visible: root.listMode !== 0
                        width: root.listMode !== 0 ? Math.floor(parent.width * 0.08) : 0
                        label: "Split"
                        sortId: "split"
                        height: parent.height
                    }
                    SortHeader {
                        visible: root.listMode !== 0
                        width: root.listMode !== 0 ? Math.floor(parent.width * 0.10) : 0
                        label: "Pred"
                        sortId: "predClass"
                        height: parent.height
                    }
                    SortHeader {
                        visible: root.listMode !== 0
                        width: root.listMode !== 0 ? Math.floor(parent.width * 0.08) : 0
                        label: "Score"
                        sortId: "predScore"
                        height: parent.height
                    }
                    SortHeader {
                        visible: root.listMode !== 0
                        width: root.listMode !== 0 ? Math.floor(parent.width * 0.20) : 0
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
                                width: root.listMode !== 0 ? Math.floor(parent.width * 0.10) : 0
                                text: root.listMode !== 0 ? (modelData.className || "") : ""
                                color: root.textColor
                                font.pixelSize: 12
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                            Text {
                                visible: root.listMode !== 0
                                width: root.listMode !== 0 ? Math.floor(parent.width * 0.08) : 0
                                text: root.listMode !== 0 ? (modelData.type || "") : ""
                                color: root.textColor
                                font.pixelSize: 12
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                            }
                            Text {
                                visible: root.listMode !== 0
                                width: root.listMode !== 0 ? Math.floor(parent.width * 0.08) : 0
                                text: root.listMode !== 0 ? (modelData.level || "") : ""
                                color: root.textColor
                                font.pixelSize: 12
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                            }
                            Text {
                                visible: root.listMode !== 0
                                width: root.listMode !== 0 ? Math.floor(parent.width * 0.08) : 0
                                text: root.listMode !== 0 ? (modelData.split || "") : ""
                                color: root.textColor
                                font.pixelSize: 12
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                            }
                            Text {
                                visible: root.listMode !== 0
                                width: root.listMode !== 0 ? Math.floor(parent.width * 0.10) : 0
                                text: root.listMode !== 0 ? (modelData.predClass || "") : ""
                                color: root.textColor
                                font.pixelSize: 12
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                            Text {
                                visible: root.listMode !== 0
                                width: root.listMode !== 0 ? Math.floor(parent.width * 0.08) : 0
                                text: {
                                    if (root.listMode === 0 || modelData.predScore === undefined || modelData.predScore === null)
                                        return ""
                                    const s = Number(modelData.predScore)
                                    return isNaN(s) ? "" : s.toFixed(4)
                                }
                                color: root.textColor
                                font.pixelSize: 12
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                            }
                            Text {
                                visible: root.listMode !== 0
                                width: root.listMode !== 0 ? Math.floor(parent.width * 0.20) : 0
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
                                    if (root.control.imageAction("select", { "index": modelData.imageIndex })) {
                                        listPopup.close()
                                    }
                                } else {
                                    if (root.control.annotationAction("select_from_list", { "imageIndex": modelData.imageIndex, "annotationIndex": modelData.annotationIndex })) {
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
        onAccepted: root.control.imageAction("import_files", { "fileList": selectedFiles, "clearExisting": root.replaceImagesOnImport })
    }

    FolderDialog {
        id: folderDialog
        title: "Select image folder"
        onAccepted: root.control.imageAction("import_folder", { "folderPath": selectedFolder.toString(), "clearExisting": root.replaceImagesOnImport })
    }

    FolderDialog {
        id: ioFolderDialog
        title: "Select import/export folder"
        onAccepted: root.control.ioAction("set_folder", { "folderPath": selectedFolder.toString() })
    }

    FileDialog {
        id: projectDialog
        title: "Load project"
        fileMode: FileDialog.OpenFile
        nameFilters: ["Project (*.json)"]
        onAccepted: {
            if (selectedFile)
                root.control.projectAction("load", { "projectPath": selectedFile.toString() })
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
                root.control.projectAction("save_as", { "projectPath": selectedFile.toString() })
        }
    }
}
