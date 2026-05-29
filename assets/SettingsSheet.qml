import bb.cascades 1.4
import bb.system 1.0
import QtQuick 1.0

Sheet {
    id: settingsSheetRoot

    Page {
        titleBar: TitleBar {
            title: "Settings"
            dismissAction: ActionItem {
                title: "Close"
                onTriggered: { settingsSheetRoot.close() }
            }
        }

        ScrollView {
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                topPadding: 30; leftPadding: 30; rightPadding: 30; bottomPadding: 60

                Label {
                    text: "Appearance"
                    textStyle.base: SystemDefaults.TextStyles.TitleText
                }

                Container {
                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                    topMargin: 20
                    Label {
                        text: "Dark Theme"
                        layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                        verticalAlignment: VerticalAlignment.Center
                    }
                    ToggleButton { id: darkToggle; checked: false }
                }

                Label {
                    text: "Theme change requires app restart."
                    multiline: true
                    textStyle.color: Color.Gray
                    topMargin: 6
                }

                Button {
                    text: "Restart to Apply Theme"
                    horizontalAlignment: HorizontalAlignment.Fill
                    topMargin: 10
                    onClicked: { app.minimizeApp() }
                }

                Divider { topMargin: 30; bottomMargin: 20 }

                Label {
                    text: "Account"
                    textStyle.base: SystemDefaults.TextStyles.TitleText
                }

                Button {
                    text: "Log Out"
                    horizontalAlignment: HorizontalAlignment.Fill
                    topMargin: 20
                    onClicked: { logoutDialog.show() }
                }

                attachedObjects: [
                    SystemDialog {
                        id: logoutDialog
                        title: "Log Out"
                        body: "Are you sure you want to log out?"
                        confirmButton.label: "Log Out"
                        cancelButton.label: "Cancel"
                        onFinished: {
                            if (result === SystemUiResult.ConfirmButtonSelection) {
                                zService.logout();
                                settingsSheetRoot.close();
                                loginSheet.open();
                            }
                        }
                    }
                ]
            }
        }
    }
}
