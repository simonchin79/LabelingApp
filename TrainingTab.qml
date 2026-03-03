import QtQuick
import QtQuick.Controls

ScrollView {
    id: root
    property color cardColor: "#222836"
    property color borderColor: "#2f3646"
    property color textColor: "#e6ebf5"
    property color subTextColor: "#9aa7bf"

    Column {
        width: root.width - 24
        spacing: 10

        Rectangle {
            width: parent.width
            color: root.cardColor
            radius: 10
            border.color: root.borderColor
            border.width: 1
            implicitHeight: 64

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                text: "Training"
                color: root.subTextColor
                font.pixelSize: 13
            }
        }

        Rectangle {
            width: parent.width
            color: root.cardColor
            radius: 10
            border.color: root.borderColor
            border.width: 1
            implicitHeight: 150

            Text {
                anchors.centerIn: parent
                text: "Training controls will be added here"
                color: root.textColor
                font.pixelSize: 13
            }
        }

        Rectangle {
            width: parent.width
            color: root.cardColor
            radius: 10
            border.color: root.borderColor
            border.width: 1
            implicitHeight: 120

            Text {
                anchors.centerIn: parent
                text: "Training logs / metrics"
                color: root.textColor
                font.pixelSize: 13
            }
        }
    }
}
