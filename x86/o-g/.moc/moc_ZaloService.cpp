/****************************************************************************
** Meta object code from reading C++ file 'ZaloService.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/ZaloService.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ZaloService.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ZaloService[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      51,   14, // methods
       1,  269, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      12,       // signalCount

 // signals: signature, parameters, type, tag, flags
      13,   12,   12,   12, 0x05,
      39,   31,   12,   12, 0x05,
      76,   60,   12,   12, 0x05,
     123,  106,   12,   12, 0x05,
     164,  152,   12,   12, 0x05,
     183,   12,   12,   12, 0x05,
     203,  195,   12,   12, 0x05,
     244,  236,   12,   12, 0x05,
     279,  271,   12,   12, 0x05,
     324,  306,   12,   12, 0x05,
     377,  360,   12,   12, 0x05,
     422,  403,   12,   12, 0x05,

 // slots: signature, parameters, type, tag, flags
     451,   12,   12,   12, 0x08,
     465,   12,   12,   12, 0x08,
     479,   12,   12,   12, 0x08,
     493,   12,   12,   12, 0x08,
     507,   12,   12,   12, 0x08,
     521,   12,   12,   12, 0x08,
     540,   12,   12,   12, 0x08,
     554,   12,   12,   12, 0x08,
     568,   12,   12,   12, 0x08,
     582,   12,   12,   12, 0x08,
     596,   12,   12,   12, 0x08,
     616,   12,   12,   12, 0x08,
     636,   12,   12,   12, 0x08,
     655,   12,   12,   12, 0x08,
     676,   12,   12,   12, 0x08,
     697,   12,   12,   12, 0x08,
     718,   12,   12,   12, 0x08,
     735,   12,   12,   12, 0x08,
     751,   12,   12,   12, 0x08,
     765,   12,   12,   12, 0x08,
     781,   12,   12,   12, 0x08,
     796,   12,   12,   12, 0x08,

 // methods: signature, parameters, type, tag, flags
     817,   12,   12,   12, 0x02,
     832,   12,   12,   12, 0x02,
     847,   12,   12,   12, 0x02,
     863,   12,   12,   12, 0x02,
     899,  872,   12,   12, 0x02,
     977,  956,   12,   12, 0x22,
    1044, 1026,   12,   12, 0x22,
    1098, 1085,   12,   12, 0x22,
    1136,   12, 1131,   12, 0x02,
    1150,   12,   12,   12, 0x02,
    1164,   12,   12,   12, 0x02,
    1185,   12,   12,   12, 0x02,
    1200,   12,   12,   12, 0x02,
    1224, 1215,   12,   12, 0x02,
    1272, 1255,   12,   12, 0x02,
    1325, 1300,   12,   12, 0x02,
    1372, 1359,   12,   12, 0x02,

 // properties: name, type, flags
    1404, 1131, 0x01495001,

 // properties: notify_signal_id
       0,

       0        // eod
};

static const char qt_meta_stringdata_ZaloService[] = {
    "ZaloService\0\0loggedInChanged()\0message\0"
    "loginFailed(QString)\0uid,displayName\0"
    "loginSuccess(QString,QString)\0"
    "imagePath,qrCode\0qrCodeReady(QString,QString)\0"
    "displayName\0qrScanned(QString)\0"
    "qrExpired()\0threads\0"
    "conversationsReady(QVariantList)\0"
    "friends\0friendsReady(QVariantList)\0"
    "invites\0invitesReady(QVariantList)\0"
    "threadId,messages\0messagesReady(QString,QVariantList)\0"
    "success,threadId\0messageSent(bool,QString)\0"
    "threadId,localPath\0avatarReady(QString,QString)\0"
    "onStep1Done()\0onStep2Done()\0onStep3Done()\0"
    "onStep4Done()\0onStep5Done()\0"
    "onQRImageFetched()\0onStep6Done()\0"
    "onStep7Done()\0onStep8Done()\0onStep9Done()\0"
    "onCookieStep1Done()\0onCookieStep2Done()\0"
    "onFetchConvoDone()\0onFetchFriendsDone()\0"
    "onFetchInvitesDone()\0onGroupDetailsDone()\0"
    "onFetchMsgDone()\0onSendMsgDone()\0"
    "onQRExpired()\0onListenTimer()\0"
    "onListenDone()\0onAvatarDownloaded()\0"
    "startQRLogin()\0retryQRLogin()\0"
    "cancelQRLogin()\0logout()\0"
    "zpsid,zpwSek,imei,ua,token\0"
    "loginWithCookie(QString,QString,QString,QString,QString)\0"
    "zpsid,zpwSek,imei,ua\0"
    "loginWithCookie(QString,QString,QString,QString)\0"
    "zpsid,zpwSek,imei\0"
    "loginWithCookie(QString,QString,QString)\0"
    "zpsid,zpwSek\0loginWithCookie(QString,QString)\0"
    "bool\0loadSession()\0saveSession()\0"
    "fetchConversations()\0fetchFriends()\0"
    "fetchInvites()\0groupIds\0"
    "fetchGroupDetails(QStringList)\0"
    "threadId,isGroup\0fetchMessages(QString,bool)\0"
    "threadId,content,isGroup\0"
    "sendMessage(QString,QString,bool)\0"
    "threadId,url\0downloadAvatar(QString,QString)\0"
    "loggedIn\0"
};

void ZaloService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ZaloService *_t = static_cast<ZaloService *>(_o);
        switch (_id) {
        case 0: _t->loggedInChanged(); break;
        case 1: _t->loginFailed((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->loginSuccess((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 3: _t->qrCodeReady((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 4: _t->qrScanned((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->qrExpired(); break;
        case 6: _t->conversationsReady((*reinterpret_cast< const QVariantList(*)>(_a[1]))); break;
        case 7: _t->friendsReady((*reinterpret_cast< const QVariantList(*)>(_a[1]))); break;
        case 8: _t->invitesReady((*reinterpret_cast< const QVariantList(*)>(_a[1]))); break;
        case 9: _t->messagesReady((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QVariantList(*)>(_a[2]))); break;
        case 10: _t->messageSent((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 11: _t->avatarReady((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 12: _t->onStep1Done(); break;
        case 13: _t->onStep2Done(); break;
        case 14: _t->onStep3Done(); break;
        case 15: _t->onStep4Done(); break;
        case 16: _t->onStep5Done(); break;
        case 17: _t->onQRImageFetched(); break;
        case 18: _t->onStep6Done(); break;
        case 19: _t->onStep7Done(); break;
        case 20: _t->onStep8Done(); break;
        case 21: _t->onStep9Done(); break;
        case 22: _t->onCookieStep1Done(); break;
        case 23: _t->onCookieStep2Done(); break;
        case 24: _t->onFetchConvoDone(); break;
        case 25: _t->onFetchFriendsDone(); break;
        case 26: _t->onFetchInvitesDone(); break;
        case 27: _t->onGroupDetailsDone(); break;
        case 28: _t->onFetchMsgDone(); break;
        case 29: _t->onSendMsgDone(); break;
        case 30: _t->onQRExpired(); break;
        case 31: _t->onListenTimer(); break;
        case 32: _t->onListenDone(); break;
        case 33: _t->onAvatarDownloaded(); break;
        case 34: _t->startQRLogin(); break;
        case 35: _t->retryQRLogin(); break;
        case 36: _t->cancelQRLogin(); break;
        case 37: _t->logout(); break;
        case 38: _t->loginWithCookie((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3])),(*reinterpret_cast< const QString(*)>(_a[4])),(*reinterpret_cast< const QString(*)>(_a[5]))); break;
        case 39: _t->loginWithCookie((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3])),(*reinterpret_cast< const QString(*)>(_a[4]))); break;
        case 40: _t->loginWithCookie((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        case 41: _t->loginWithCookie((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 42: { bool _r = _t->loadSession();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 43: _t->saveSession(); break;
        case 44: _t->fetchConversations(); break;
        case 45: _t->fetchFriends(); break;
        case 46: _t->fetchInvites(); break;
        case 47: _t->fetchGroupDetails((*reinterpret_cast< const QStringList(*)>(_a[1]))); break;
        case 48: _t->fetchMessages((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 49: _t->sendMessage((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3]))); break;
        case 50: _t->downloadAvatar((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ZaloService::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ZaloService::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_ZaloService,
      qt_meta_data_ZaloService, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ZaloService::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ZaloService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ZaloService::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ZaloService))
        return static_cast<void*>(const_cast< ZaloService*>(this));
    return QObject::qt_metacast(_clname);
}

int ZaloService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 51)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 51;
    }
#ifndef QT_NO_PROPERTIES
      else if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = loggedIn(); break;
        }
        _id -= 1;
    } else if (_c == QMetaObject::WriteProperty) {
        _id -= 1;
    } else if (_c == QMetaObject::ResetProperty) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 1;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void ZaloService::loggedInChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void ZaloService::loginFailed(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void ZaloService::loginSuccess(const QString & _t1, const QString & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void ZaloService::qrCodeReady(const QString & _t1, const QString & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void ZaloService::qrScanned(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void ZaloService::qrExpired()
{
    QMetaObject::activate(this, &staticMetaObject, 5, 0);
}

// SIGNAL 6
void ZaloService::conversationsReady(const QVariantList & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void ZaloService::friendsReady(const QVariantList & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void ZaloService::invitesReady(const QVariantList & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void ZaloService::messagesReady(const QString & _t1, const QVariantList & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void ZaloService::messageSent(bool _t1, const QString & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void ZaloService::avatarReady(const QString & _t1, const QString & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}
QT_END_MOC_NAMESPACE
