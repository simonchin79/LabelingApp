import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    required property var control
    property color panelColor: "#1a1f2b"
    property color cardColor: "#222836"
    property color borderColor: "#2f3646"
    property color textColor: "#e6ebf5"
    property color subTextColor: "#9aa7bf"
    property color accentColor: "#4da3ff"
    property alias editAnnotationMode: labelingTab.editAnnotationMode

    color: root.panelColor
    border.color: root.borderColor
    border.width: 1

    TabBar {
        id: rightTabBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 12
        spacing: 6

        background: Rectangle { color: "transparent" }

        Repeater {
            model: ["Labeling", "Training", "Inference", "Settings"]
            TabButton {
                required property int index
                required property string modelData
                text: modelData
                contentItem: Text {
                    text: parent.text
                    color: parent.checked ? "#0f1116" : root.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                background: Rectangle {
                    radius: 6
                    border.width: 1
                    border.color: parent.checked ? "#7ab4ff" : root.borderColor
                    color: parent.checked ? "#7ab4ff" : "#30394b"
                }
            }
        }
    }

    StackLayout {
        currentIndex: rightTabBar.currentIndex
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: rightTabBar.bottom
        anchors.bottom: parent.bottom
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        anchors.bottomMargin: 12
        anchors.topMargin: 8

        LabelingTab {
            id: labelingTab
            Layout.fillWidth: true
            Layout.fillHeight: true
            control: root.control
            cardColor: root.cardColor
            borderColor: root.borderColor
            textColor: root.textColor
            subTextColor: root.subTextColor
            accentColor: root.accentColor
        }

        TrainingTab {
            Layout.fillWidth: true
            Layout.fillHeight: true
            cardColor: root.cardColor
            borderColor: root.borderColor
            subTextColor: root.subTextColor
        }

        InferenceTab {
            Layout.fillWidth: true
            Layout.fillHeight: true
            cardColor: root.cardColor
            borderColor: root.borderColor
            subTextColor: root.subTextColor
        }

        SettingsTab {
            Layout.fillWidth: true
            Layout.fillHeight: true
            cardColor: root.cardColor
            borderColor: root.borderColor
            subTextColor: root.subTextColor
        }
    }
}
