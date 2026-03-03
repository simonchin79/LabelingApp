import QtQuick
import QtQuick.Controls
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
    property var controlObj: control

    Component.onCompleted: {
        if (root.controlObj && root.controlObj.currentImageIndex >= 0)
            root.controlObj.selectImage(root.controlObj.currentImageIndex)
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
                        controller: root.controlObj
                        editMode: rightPanel.editAnnotationMode
                        selectedPolygonIndex: root.controlObj.selectedPolygonIndex
                        savedPolygons: root.controlObj.currentPolygons
                        annotationPoints: root.controlObj.draftPoints

                        onImageClicked: function(x, y) {
                            root.controlObj.addAnnotationPoint(x, y)
                        }
                        onAnnotationPointEdited: function(polygonIndex, pointIndex, x, y) {
                            root.controlObj.updateAnnotationPoint(polygonIndex, pointIndex, x, y)
                        }
                        onDraftPointEdited: function(pointIndex, x, y) {
                            root.controlObj.updateDraftPoint(pointIndex, x, y)
                        }
                        onPolygonSelected: function(polygonIndex) {
                            root.controlObj.setSelectedPolygonIndex(polygonIndex)
                        }
                    }
                }
            }

            RightPanel {
                id: rightPanel
                width: parent.width - leftPane.width
                height: parent.height
                control: root.controlObj
                panelColor: root.panelColor
                cardColor: root.cardColor
                borderColor: root.borderColor
                textColor: root.textColor
                subTextColor: root.subTextColor
                accentColor: root.accentColor
            }
        }
    }
}
