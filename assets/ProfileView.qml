// ProfileView.qml
import bb.cascades 1.4
import QtQuick 1.0

Page {
    id: profilePage

    property string contactId:    ""
    property string contactName:  ""
    property string avatarPath:   ""
    property string bgAvatarPath: ""
    property string avatarUrl:    ""  // fallback nếu chưa download
    property string bgAvatarUrl:  ""

    titleBar: TitleBar {
        title: profilePage.contactName
        kind: TitleBarKind.Default
        appearance: TitleBarAppearance.Default
        dismissAction: ActionItem {
            title: "Back"
            onTriggered: { navigationPane.pop() }
        }
    }

    actions: [
        ActionItem {
            title: "Nhắn tin"
            imageSource: "asset:///images/ic_bbm.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            onTriggered: {
                var chatPage = chatDef.createObject()
                if (!chatPage) return
                chatPage.threadId   = profilePage.contactId
                chatPage.threadName = profilePage.contactName
                chatPage.isGroup    = false
                navigationPane.push(chatPage)
            }
        }
    ]

    ScrollView {
        scrollViewProperties {
            scrollMode: ScrollMode.Vertical
        }
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill

        Container {
            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
            horizontalAlignment: HorizontalAlignment.Fill

            // ── Ảnh nền ──────────────────────────────────────────
            Container {
                preferredHeight: ui.du(28)
                horizontalAlignment: HorizontalAlignment.Fill
                layout: DockLayout {}
                background: Color.create("#1a1a2e")

                // Bg avatar
                ImageView {
                    id: bgImage
                    imageSource: {
                        if (profilePage.bgAvatarPath.length > 0) return profilePage.bgAvatarPath
                        return "asset:///images/blank.png"
                    }
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Fill
                    scalingMethod: ScalingMethod.AspectFill
                }

                // Gradient overlay phía dưới để chữ dễ đọc
                Container {
                    verticalAlignment: VerticalAlignment.Bottom
                    horizontalAlignment: HorizontalAlignment.Fill
                    preferredHeight: ui.du(14)
                    background: Color.create("#99000000")
                }

                // Avatar tròn đè lên góc dưới-giữa
                Container {
                    verticalAlignment: VerticalAlignment.Bottom
                    horizontalAlignment: HorizontalAlignment.Center
                    bottomPadding: ui.du(1)
                    layout: DockLayout {}

                    Container {
                        preferredWidth: ui.du(16)
                        preferredHeight: ui.du(16)
                        background: Color.create("#2575fc")
                        layout: DockLayout {}

                        ImageView {
                            id: avatarImage
                            imageSource: {
                                if (profilePage.avatarPath.length > 0) return profilePage.avatarPath
                                return "asset:///images/blank.png"
                            }
                            horizontalAlignment: HorizontalAlignment.Fill
                            verticalAlignment: VerticalAlignment.Fill
                            scalingMethod: ScalingMethod.AspectFill
                        }
                    }
                }
            }

            // ── Tên ──────────────────────────────────────────────
            Container {
                topPadding: ui.du(2)
                bottomPadding: ui.du(1)
                horizontalAlignment: HorizontalAlignment.Center

                Label {
                    text: profilePage.contactName
                    textStyle {
                        base: SystemDefaults.TextStyles.BigText
                        fontWeight: FontWeight.Bold
                        textAlign: TextAlign.Center
                    }
                    horizontalAlignment: HorizontalAlignment.Center
                    multiline: false
                }
            }

            // ── Divider ───────────────────────────────────────────
            Divider { topMargin: ui.du(1); bottomMargin: ui.du(1) }

            // ── Nút hành động ─────────────────────────────────────
            Container {
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                horizontalAlignment: HorizontalAlignment.Center
                topPadding: ui.du(1)
                bottomPadding: ui.du(2)

                Button {
                    text: "Nhắn tin"
                    preferredWidth: ui.du(26)
                    onClicked: {
                        var chatPage = chatDef.createObject()
                        if (!chatPage) return
                        chatPage.threadId   = profilePage.contactId
                        chatPage.threadName = profilePage.contactName
                        chatPage.isGroup    = false
                        navigationPane.push(chatPage)
                    }
                }
            }

            // ── Thông tin thêm (placeholder cho sau) ─────────────
            Container {
                leftPadding: ui.du(3)
                rightPadding: ui.du(3)
                topPadding: ui.du(1)

                Label {
                    text: "Zalo ID"
                    textStyle {
                        base: SystemDefaults.TextStyles.SmallText
                        color: Color.Gray
                    }
                }
                Label {
                    text: profilePage.contactId
                    textStyle {
                        base: SystemDefaults.TextStyles.BodyText
                    }
                    topMargin: ui.du(0.3)
                    bottomMargin: ui.du(2)
                }
            }
        }
    }

    // Lắng nghe avatarReady để update ảnh nếu chưa load kịp
    attachedObjects: [
        ComponentDefinition {
            id: chatDef
            source: "asset:///ChatView.qml"
        },
        Connections {
            target: zService
            onAvatarReady: {
                if (threadId === profilePage.contactId && profilePage.avatarPath.length === 0)
                    profilePage.avatarPath = localPath
                if (threadId === ("bg_" + profilePage.contactId) && profilePage.bgAvatarPath.length === 0)
                    profilePage.bgAvatarPath = localPath
            }
        }
    ]

    // navigationPane trỏ vào NavigationPane cha
    property variant navigationPane: parent && parent.parent ? parent.parent : null
}
