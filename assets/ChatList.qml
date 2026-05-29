// ChatList.qml
import bb.cascades 1.4
import QtQuick 1.0

Page {
    id: chatListPage
    property bool fetchStarted: false
    
    // ── HEADER NATIVE MÀU XANH #2575fc ───────────────────────
    titleBar: TitleBar {
        kind: TitleBarKind.FreeForm
        kindProperties: FreeFormTitleBarKindProperties {
            content: Container {
                background: Color.create("#2575fc")
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                layout: DockLayout {}
                leftPadding: ui.du(2.5)
                rightPadding: ui.du(2.5)
                
                Label {
                    text: "Zalo10"
                    textStyle {
                        color: Color.White
                        base: SystemDefaults.TextStyles.TitleText
                        fontWeight: FontWeight.Bold
                    }
                    verticalAlignment: VerticalAlignment.Center
                    horizontalAlignment: HorizontalAlignment.Left
                }
            }
        }
    }
    
    onCreationCompleted: {
        if (!fetchStarted && zService.loggedIn) {
            fetchStarted = true;
            zService.fetchConversations();
            loadingBar.visible = true;
        }
    }
    
    // ── ACTION BAR DƯỚI ĐÁY CHUẨN BB10 ───────────────────────
    actions: [
        ActionItem {
            title: "Refresh"
            imageSource: "asset:///images/ic_sync.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            onTriggered: {
                chatListPage.fetchStarted = false;
                threadModel.clear();
                zService.fetchConversations();
                loadingBar.visible = true;
                emptyLabel.visible = false;
            }
        }
    ]
    
    content: Container {
        layout: DockLayout {}
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        
        ListView {
            id: chatListView
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            
            dataModel: ArrayDataModel { id: threadModel }
            
            // QUAN TRỌNG: BẮT BUỘC PHẢI CÓ HÀM NÀY ĐỂ RENDER ĐÚNG CUSTOM UI
            function itemType(data, indexPath) {
                return "chatItem";
            }
            
            listItemComponents: [
                ListItemComponent {
                    type: "chatItem" 
                    
                    CustomListItem {
                        id: chatItem
                        dividerVisible: true
                        
                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            verticalAlignment: VerticalAlignment.Fill
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                            leftPadding: ui.du(2.5)
                            rightPadding: ui.du(2.5)
                            topPadding: ui.du(1.5)
                            bottomPadding: ui.du(1.5)
                            
                            // ================= BÊN TRÁI: AVATAR =================
                            Container {
                                verticalAlignment: VerticalAlignment.Center
                                layout: DockLayout {}
                                preferredWidth: ui.du(9.0)
                                preferredHeight: ui.du(9.0)
                                
                                ImageView {
                                    imageSource: {
                                        if (ListItemData.localAvatar && ListItemData.localAvatar.length > 0) {
                                            return "file://" + ListItemData.localAvatar
                                        } else if (ListItemData.avatar && ListItemData.avatar.length > 0) {
                                            return ListItemData.avatar
                                        } else {
                                            return "asset:///images/blank.png"
                                        }
                                    }
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    verticalAlignment: VerticalAlignment.Fill
                                    scalingMethod: ScalingMethod.AspectFill
                                }
                                
                                // Fallback: chữ cái đầu khi không có avatar
                                Container {
                                    visible: {
                                        var hasAvatar = (ListItemData.localAvatar && ListItemData.localAvatar.length > 0) ||
                                        (ListItemData.avatar && ListItemData.avatar.length > 0);
                                        return !hasAvatar;
                                    }
                                    horizontalAlignment: HorizontalAlignment.Center
                                    verticalAlignment: VerticalAlignment.Center
                                    
                                    Label {
                                        text: {
                                            var name = ListItemData.name || ListItemData.displayName || "?";
                                            return name.substring(0, 1).toUpperCase();
                                        }
                                        textStyle {
                                            base: SystemDefaults.TextStyles.HeadingText
                                            color: Color.White
                                            fontWeight: FontWeight.Bold
                                            textAlign: TextAlign.Center
                                        }
                                    }
                                }
                            }
                            
                            // ================= BÊN PHẢI: NAME & MESSAGE =================
                            Container {
                                verticalAlignment: VerticalAlignment.Center
                                leftMargin: ui.du(2.2)
                                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                horizontalAlignment: HorizontalAlignment.Fill
                                
                                Label {
                                    text: ListItemData.name || ListItemData.displayName || "Unknown User"
                                    textStyle {
                                        base: SystemDefaults.TextStyles.TitleText
                                        fontWeight: FontWeight.Bold
                                    }
                                    bottomMargin: ui.du(0.2)
                                }
                                
                                Label {
                                    text: (ListItemData.lastMessage || ListItemData.lastMsg || "No messages yet")
                                    textStyle {
                                        base: SystemDefaults.TextStyles.SubtitleText
                                        color: Color.DarkGray
                                    }
                                    multiline: false
                                }
                            }
                        }
                    }
                }
            ]
        }
        
        ActivityIndicator {
            id: loadingBar
            horizontalAlignment: HorizontalAlignment.Center
            verticalAlignment: VerticalAlignment.Center
            preferredWidth: ui.du(10)
            preferredHeight: ui.du(10)
            visible: false
        }
        
        Label {
            id: emptyLabel
            text: "No conversations found"
            visible: false
            horizontalAlignment: HorizontalAlignment.Center
            verticalAlignment: VerticalAlignment.Center
            textStyle { 
                base: SystemDefaults.TextStyles.BodyText 
            }
        }
    }
    
    attachedObjects: [
        Connections {
            target: zService
            
            onConversationsReady: {
                loadingBar.visible = false;
                if (threads && threads.length > 0) {
                    var firstIsGroup = threads[0].isGroup;
                    if (firstIsGroup) {
                        threadModel.clear();
                    }
                    for (var i = 0; i < threads.length; i++) {
                        var item = threads[i];
                        item.localAvatar = "";
                        threadModel.append(item);
                    }
                    emptyLabel.visible = false;
                } else if (threadModel.size() === 0) {
                    emptyLabel.visible = true;
                }
            }
            
            onAvatarReady: {
                for (var i = 0; i < threadModel.size(); i++) {
                    var d = threadModel.value(i);
                    if (d.threadId === threadId) {
                        d.localAvatar = localPath;
                        threadModel.replace(i, d);
                        break;
                    }
                }
            }
        }
    ]
}