import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Window 2.12

Window {
    visible: true
    width: 640
    height: 480
    title: qsTr("Panel")

    Label {
        id: labelY1
        text: "y:"

        font.pointSize: 15
        anchors.top: labelKDelta.bottom

        Label {
            id: labelY1Value
            text: "y value"

            anchors.left: parent.right
            anchors.leftMargin: 10

            Connections {
                target: appCore

                function onY1Changed(value, objectName) { 
                    labelY1.text = objectName + " : ";
                    updateLabelValue(labelY1Value, value);
                }
            }
        }
    }

    Label {
        id: labelY2
        text: "y:"

        font.pointSize: 15

        anchors.top: labelY1.bottom

        Label {
            id: labelY2Value
            text: "y value"

            anchors.left: parent.right
            anchors.leftMargin: 10

            Connections {
                target: appCore

                function onY2Changed(value, objectName) { 
                    labelY2.text = objectName + " : ";
                    updateLabelValue(labelY2Value, value);
                }
            }
        }
    }

    Label {
        id: labelError
        text: "delta * k - y : "

        font.pointSize: 15

        anchors.top: labelY2.bottom

        Label {
            id: labelErrorValue
            text: ""

            anchors.left: parent.right
            anchors.leftMargin: 10

            Connections {
                target: appCore

                function onErrorChanged(value) { 
                    updateLabelValue(labelErrorValue, value, 10);
                }
            }
        }
    }

    Label {
        id: labelTime
        text: "time:"

        font.pointSize: 15

        

        Label {
            id: labelTimeValue
            text: "time value"

            anchors.left: parent.right
            anchors.leftMargin: 10

            Connections {
                target: appCore

                function onTimeChanged(value) { 
                    updateLabelValue(labelTimeValue, value);
                }
            }
        }
    }

    Label {
        id: labelDelta
        text: "delta:"

        font.pointSize: 15

        anchors.top: labelTime.bottom

        Label {
            id: labelDeltaValue
            text: "delta value"

            anchors.left: parent.right
            anchors.leftMargin: 10

            Connections {
                target: appCore

                function onDeltaChanged(value) { 
                    updateLabelValue(labelDeltaValue, value);
                }
            }
        }
    }

    Label {
        id: labelKDelta
        text: "k-delta:"

        font.pointSize: 15

        anchors.top: labelDelta.bottom

        Label {
            id: labelKDeltaValue
            text: "k delta value"

            anchors.left: parent.right
            anchors.leftMargin: 10

            Connections {
                target: appCore

                function onKDeltaChanged(value) { 
                    updateLabelValue(labelKDeltaValue, value);
                }
            }
        }
    }

    TextField {
        id: textFieldK
        placeholderText: qsTr("Enter K")
        text: ""

        anchors.top: labelError.bottom

        onTextChanged: {
            appCore.setK(text);}
    }

    Button {
        id: buttonReset
        text: "reset"

        anchors.left: textFieldK.right
        anchors.top: labelError.bottom
        anchors.leftMargin: 10

        onPressed: {
            appCore.setResetOnPressed();
        }

        onReleased: {
            appCore.setResetOnReleased();
        }
    }

    function updateLabelValue(labelId, value, numSigns = 2) {
        labelId.text = value.toFixed(numSigns);
    }
}
