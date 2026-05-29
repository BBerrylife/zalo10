// ChatView.qml
import bb.cascades 1.4
import QtQuick 1.0

Page {
    id: chatViewPage

    property string threadId:   ""
    property string threadName: ""
    property bool   isGroup:    false
    property string pendingMsg: ""

    // Native TitleBar - cố định ở trên
    titleBar: TitleBar {
        title: chatViewPage.threadName || "Chat"
        kind: TitleBarKind.Default
        appearance: TitleBarAppearance.Default
    }

    onCreationCompleted: {
        if (threadId !== "")
            zService.fetchMessages(threadId, isGroup);
    }

    content: Container {
        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment:   VerticalAlignment.Fill

        // ── Danh sách tin nhắn (chiếm hết không gian còn lại) ─
        ListView {
            id: msgList
            horizontalAlignment: HorizontalAlignment.Fill
            layoutProperties: StackLayoutProperties { spaceQuota: 1 }
            dataModel: ArrayDataModel { id: msgModel }

            listItemComponents: [
                ListItemComponent {
                    type: "item"

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        leftPadding:   ui.du(2)
                        rightPadding:  ui.du(2)
                        topPadding:    ui.du(0.8)
                        bottomPadding: ui.du(0.8)

                        Container {
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                            horizontalAlignment: (ListItemData.isMine || false)
                                ? HorizontalAlignment.Right
                                : HorizontalAlignment.Left
                            maxWidth: ui.du(55)

                            Label {
                                visible: !(ListItemData.isMine || false)
                                    && (ListItemData.isGroup || false)
                                text: ListItemData.dName || ""
                                textStyle {
                                    base:  SystemDefaults.TextStyles.SmallText
                                    color: Color.create("#0057a8")
                                }
                            }

                            Container {
                                background: (ListItemData.isMine || false)
                                    ? Color.create("#70cbff")
                                    : Color.create("#e8e8e8")
                                leftPadding:   ui.du(2)
                                rightPadding:  ui.du(2)
                                topPadding:    ui.du(1.5)
                                bottomPadding: ui.du(1.5)

                                Label {
                                    text: {
                                        var c = ListItemData.content;
                                        if (typeof c === "string" && c.length > 0) return c;
                                        if (c && c.content) return c.content;
                                        return "[Ảnh/File]";
                                    }
                                    textStyle {
                                        base:  SystemDefaults.TextStyles.BodyText
                                        color: Color.Black
                                    }
                                    multiline: true
                                }
                            }

                            Label {
                                text: {
                                    var ts = ListItemData.ts || 0;
                                    if (!ts) return "";
                                    var d = new Date(ts * 1);
                                    var h = d.getHours();
                                    var m = d.getMinutes();
                                    return h + ":" + (m < 10 ? "0" : "") + m;
                                }
                                textStyle {
                                    base:  SystemDefaults.TextStyles.SmallText
                                    color: Color.create("#999999")
                                }
                                horizontalAlignment: (ListItemData.isMine || false)
                                    ? HorizontalAlignment.Right
                                    : HorizontalAlignment.Left
                            }
                        }
                    }
                }
            ]
        }

        // ── Input bar (cố định ở dưới cùng) ──────────────────
        Container {
            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment:   VerticalAlignment.Bottom
            topPadding:    ui.du(1)
            bottomPadding: ui.du(1)
            leftPadding:   ui.du(2)
            rightPadding:  ui.du(1)

            TextField {
                id: inputField
                hintText: "Nhắn tin..."
                backgroundVisible: true
                verticalAlignment: VerticalAlignment.Center
                layoutProperties: StackLayoutProperties { spaceQuota: 1 }
            }

            Button {
                id: sendBtn
                text: "Gửi"
                preferredWidth: ui.du(14)
                verticalAlignment: VerticalAlignment.Center
                onClicked: {
                    var msg = inputField.text.trim();
                    if (msg !== "") {
                        chatViewPage.pendingMsg = msg;
                        sendBtn.enabled = false;
                        zService.sendMessage(
                            chatViewPage.threadId,
                            msg,
                            chatViewPage.isGroup
                        );
                        inputField.text = "";
                    }
                }
            }
        }
    }

    // Loading overlay - dùng onCreationCompleted để xử lý
    attachedObjects: [
        Connections {
            target: zService

            onMessagesReady: {
                if (tid !== chatViewPage.threadId) return;
                msgModel.clear();
                for (var i = 0; i < messages.length; i++) {
                    msgModel.append(messages[i]);
                }
                msgList.scrollToPosition(ScrollPosition.End, ScrollAnimation.None);
            }

            onMessageSent: {
                if (tid !== chatViewPage.threadId) return;
                sendBtn.enabled = true;
                if (ok && chatViewPage.pendingMsg !== "") {
                    var newMsg = {
                        "content":  chatViewPage.pendingMsg,
                        "isMine":   true,
                        "isGroup":  chatViewPage.isGroup,
                        "dName":    "",
                        "ts":       String(new Date().getTime())
                    };
                    msgModel.append(newMsg);
                    msgList.scrollToPosition(ScrollPosition.End, ScrollAnimation.Smooth);
                    chatViewPage.pendingMsg = "";
                }
            }
        }
    ]
}
