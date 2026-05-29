#include "ZaloService.hpp"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QByteArray>
#include <QScriptEngine>
#include <QScriptValue>
#include <QUuid>
#include <QCryptographicHash>
#include <QDateTime>
#include <QRegExp>
#include <QStringList>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSslConfiguration>

#include <openssl/aes.h>
#include <string.h>

const char *ZaloService::USER_AGENT =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36";

const char *ZaloService::AES_FIXED_KEY = "3FC4F0D2AB50057BCE0D90D9187A22B1";

static QVariantMap jsonToMap(const QByteArray &raw)
{
    QByteArray trimmed = raw.trimmed();
    // Chặn lỗi HTML làm treo QScriptEngine
    if (trimmed.isEmpty() || trimmed.startsWith("<")) return QVariantMap();
    QScriptEngine eng;
    QString src = "(" + QString::fromUtf8(trimmed) + ")";
    QScriptValue val = eng.evaluate(src);
    return val.toVariant().toMap();
}

static QVariantList jsonToList(const QByteArray &raw)
{
    QByteArray trimmed = raw.trimmed();
    // Chặn lỗi HTML làm treo QScriptEngine
    if (trimmed.isEmpty() || trimmed.startsWith("<")) return QVariantList();
    QScriptEngine eng;
    QString src = "(" + QString::fromUtf8(trimmed) + ")";
    QScriptValue val = eng.evaluate(src);
    return val.toVariant().toList();
}

static QByteArray mapToJson(const QVariantMap &map)
{
    QScriptEngine eng;
    QScriptValue obj = eng.newObject();
    for (QVariantMap::const_iterator it = map.constBegin(); it != map.constEnd(); ++it) {
        QVariant v = it.value();
        switch (v.type()) {
        case QVariant::String:   obj.setProperty(it.key(), v.toString()); break;
        case QVariant::Int:
        case QVariant::LongLong: obj.setProperty(it.key(), (double)v.toLongLong()); break;
        case QVariant::Bool:     obj.setProperty(it.key(), (bool)v.toBool()); break;
        case QVariant::Double:   obj.setProperty(it.key(), (double)v.toDouble()); break;
        default:                 obj.setProperty(it.key(), v.toString()); break;
        }
    }
    QScriptValue jsonStringify = eng.evaluate("JSON.stringify");
    QScriptValue result = jsonStringify.call(QScriptValue(), QScriptValueList() << obj);
    return result.toString().toUtf8();
}

ZaloService::ZaloService(QObject *parent)
    : QObject(parent), m_manager(new QNetworkAccessManager(this)), m_qrExpireTimer(new QTimer(this)), m_listenTimer(new QTimer(this)), m_userAgent(USER_AGENT), m_language("vi"), m_loggedIn(false), m_qrCancelled(false)
{
    m_qrExpireTimer->setSingleShot(true);
    connect(m_qrExpireTimer, SIGNAL(timeout()), this, SLOT(onQRExpired()));
    connect(m_listenTimer,   SIGNAL(timeout()), this, SLOT(onListenTimer()));
    qsrand((uint)QDateTime::currentMSecsSinceEpoch()); // seed random
    qDebug() << "[Zalo] ===== BUILD v20 - Full Fixes Applied ======";
}

ZaloService::~ZaloService() {}

void ZaloService::startQRLogin()
{
    m_qrCancelled = false;
    m_loggedIn    = false;
    m_cookies.clear();
    m_uid.clear();
    m_displayName.clear();
    m_secretKey.clear();
    m_imei = generateIMEI();
    qDebug() << "[Zalo] startQRLogin IMEI:" << m_imei;
    step1_loadLoginPage();
}

void ZaloService::retryQRLogin() { startQRLogin(); }

void ZaloService::cancelQRLogin()
{
    m_qrCancelled = true;
    m_qrExpireTimer->stop();
}

void ZaloService::logout()
{
    m_loggedIn = false;
    m_listenTimer->stop();
    m_cookies.clear();
    m_secretKey.clear();
    m_uid.clear();
    m_displayName.clear();
    emit loggedInChanged();
}

void ZaloService::loginWithCookie(const QString &zpsid, const QString &zpwSek, const QString &imei, const QString &ua, const QString &token)
{
    m_qrCancelled = true;
    m_qrExpireTimer->stop();
    m_cookies.clear();
    m_cookies["zpsid"]   = zpsid.trimmed();
    m_cookies["zpw_sek"] = zpwSek.trimmed();

    if (!ua.trimmed().isEmpty()) m_userAgent = ua.trimmed();
    else m_userAgent = USER_AGENT;

    m_imei = imei.trimmed().isEmpty() ? generateIMEI() : imei.trimmed();
    qDebug() << "[Zalo] loginWithCookie IMEI:" << m_imei << "UA:" << m_userAgent;

    if (!token.trimmed().isEmpty()) {
        qDebug() << "[Zalo] Co Token ngoai. Se dung lam secretKey sau cookieStep1.";
        m_externalToken = token.trimmed();
    } else {
        m_externalToken.clear();
    }

    cookieStep1_getZaloLoginInfo();
}

void ZaloService::cookieStep1_getZaloLoginInfo()
{
    qDebug() << "[Zalo] cookieStep1";
    QVariantMap data;
    data["computer_name"] = QString("Web");
    data["imei"]          = m_imei;
    data["language"]      = m_language;
    data["ts"]            = QString::number(QDateTime::currentMSecsSinceEpoch());

    EncryptedParams ep = buildEncryptedParams(data);
    m_pendingEncryptKey = ep.encryptKey;

    QVariantMap paramsForSign;
    paramsForSign["zcid"]           = ep.zcid;
    paramsForSign["zcid_ext"]       = ep.zcid_ext;
    paramsForSign["enc_ver"]        = ep.enc_ver;
    paramsForSign["params"]         = ep.encryptedData;
    paramsForSign["type"]           = QString::number(API_TYPE);
    paramsForSign["client_version"] = QString::number(API_VERSION);
    paramsForSign["nretry"]         = QString("0");

    QVariantMap params = paramsForSign;
    params["signkey"] = buildSignKey("getlogininfo", paramsForSign);
    params["imei"]    = m_imei;

    qDebug() << "[Zalo] cookieStep1 signkey:" << params["signkey"].toString();

    QString urlStr = buildRawUrl("https://wpa.chat.zalo.me/api/login/getLoginInfo", params);
    QNetworkRequest req = buildRequest(urlStr, "https://chat.zalo.me/");
    req.setRawHeader("zpw_ver",  QByteArray::number(API_VERSION));
    req.setRawHeader("zpw_type", QByteArray::number(API_TYPE));

    QNetworkReply *reply = m_manager->get(req);
    connect(reply, SIGNAL(finished()), this, SLOT(onCookieStep1Done()));
}

void ZaloService::onCookieStep1Done()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    if (reply->error() != QNetworkReply::NoError) {
        emit loginFailed(reply->errorString());
        reply->deleteLater();
        return;
    }
    QByteArray raw = reply->readAll();
    reply->deleteLater();

    qDebug() << "[Zalo] cookieStep1 raw response:" << raw.left(400);
    qDebug() << "[Zalo] cookieStep1 pendingEncryptKey:" << m_pendingEncryptKey;
    qDebug() << "[Zalo] cookieStep1 cookies:" << m_cookies;

    QVariantMap root = jsonToMap(raw);
    int errCode = root["error_code"].toInt();
    qDebug() << "[Zalo] cookieStep1 error_code:" << errCode << "msg:" << root["error_message"].toString();

    QVariantMap info;
    QVariant dataVariant = root["data"];
    if (dataVariant.type() == QVariant::Map) {
        info = dataVariant.toMap();
    } else {
        QString encData = dataVariant.toString();
        qDebug() << "[Zalo] cookieStep1 encData (first60):" << encData.left(60);
        if (errCode != 0) {
            emit loginFailed(QString("Cookie loi %1 - %2").arg(errCode).arg(root["error_message"].toString()));
            return;
        }
        if (encData.isEmpty()) {
            emit loginFailed("Cookie: response data rong");
            return;
        }
        QString decrypted = aesDecryptBase64_256(m_pendingEncryptKey, encData);
        qDebug() << "[Zalo] cookieStep1 decrypted (first100):" << decrypted.left(100);
        QVariantMap root2 = jsonToMap(decrypted.toUtf8());
        info = root2["data"].toMap();
        if (info.isEmpty()) info = root2;
    }

    m_secretKey   = info["zpw_enk"].toString();
    m_uid         = info["uid"].toString();
    m_displayName = info["display_name"].toString();

    if (!m_externalToken.isEmpty()) {
        qDebug() << "[Zalo] Ghi de secretKey bang externalToken.";
        m_secretKey = m_externalToken;
        m_externalToken.clear();
    }

    QVariantMap svcMap  = info["zpw_service_map_v3"].toMap();
    QVariantList chatA  = svcMap["chat"].toList();
    QVariantList groupA = svcMap["group"].toList();
    if (!chatA.isEmpty())  m_chatServiceUrl  = chatA[0].toString();
    if (!groupA.isEmpty()) m_groupServiceUrl = groupA[0].toString();

    if (m_uid.isEmpty()) {
        emit loginFailed("Cookie het han hoac sai - Lay lai ZPSID/ZPW_SEK moi");
        return;
    }

    cookieStep2_getServerInfo(m_pendingEncryptKey);
}

void ZaloService::cookieStep2_getServerInfo(const QString &encryptKey)
{
    Q_UNUSED(encryptKey);
    QVariantMap params;
    params["imei"]           = m_imei;
    params["type"]           = QString::number(API_TYPE);
    params["client_version"] = QString::number(API_VERSION);
    params["computer_name"]  = QString("Web");
    params["signkey"]        = buildSignKey("getserverinfo", params);

    QString urlStr = buildRawUrl("https://wpa.chat.zalo.me/api/login/getServerInfo", params);
    QNetworkReply *reply = m_manager->get(buildRequest(urlStr, "https://chat.zalo.me/"));
    connect(reply, SIGNAL(finished()), this, SLOT(onCookieStep2Done()));
}

void ZaloService::onCookieStep2Done()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) reply->deleteLater();

    m_loggedIn = true;
    emit loggedInChanged();
    emit loginSuccess(m_uid, m_displayName);
    m_listenTimer->start(8000);
}

QByteArray ZaloService::buildFormBody(const QList<QPair<QString,QString> > &fields)
{
    QStringList parts;
    for (int i = 0; i < fields.size(); ++i) {
        QString k = QString::fromUtf8(QUrl::toPercentEncoding(fields[i].first));
        QString v = QString::fromUtf8(QUrl::toPercentEncoding(fields[i].second));
        parts << k + "=" + v;
    }
    return parts.join("&").toUtf8();
}

void ZaloService::step1_loadLoginPage()
{
    qDebug() << "[Zalo] Step1: Load Login Page";
    QNetworkReply *reply = m_manager->get(buildRequest("https://id.zalo.me/account?continue=https%3A%2F%2Fchat.zalo.me%2F", "https://chat.zalo.me/"));
    connect(reply, SIGNAL(finished()), this, SLOT(onStep1Done()));
}

void ZaloService::onStep1Done()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    if (reply->error() != QNetworkReply::NoError) {
        emit loginFailed("Loi mang: " + reply->errorString());
        reply->deleteLater();
        return;
    }
    parseCookiesFromReply(reply);
    QString html = QString::fromUtf8(reply->readAll());
    reply->deleteLater();

    QStringList rxPatterns;
    rxPatterns << "stc-zlogin\\.zdn\\.vn/main-([\\d.]+)\\.js"
               << "main[.-]([\\w.-]+)\\.js"
               << "chunkMain[.-]([\\w.-]+)\\.js";
    m_loginVersion.clear();
    for (int pi = 0; pi < rxPatterns.size(); ++pi) {
        QRegExp rx2(rxPatterns[pi]);
        if (rx2.indexIn(html) >= 0) {
            m_loginVersion = rx2.cap(1);
            break;
        }
    }
    if (m_loginVersion.isEmpty()) m_loginVersion = "5.6.1";

    qDebug() << "[Zalo] Login Version:" << m_loginVersion;
    step2_getLoginInfo();
}

void ZaloService::step2_getLoginInfo()
{
    qDebug() << "[Zalo] Step2: getLoginInfo";
    QNetworkRequest req = buildRequest("https://id.zalo.me/account/logininfo", "https://id.zalo.me/account?continue=https%3A%2F%2Fzalo.me%2Fpc");
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QList<QPair<QString,QString> > f;
    f << QPair<QString,QString>("continue", "https://zalo.me/pc") << QPair<QString,QString>("v", m_loginVersion);
    QNetworkReply *reply = m_manager->post(req, buildFormBody(f));
    connect(reply, SIGNAL(finished()), this, SLOT(onStep2Done()));
}

void ZaloService::onStep2Done()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) { parseCookiesFromReply(reply); reply->deleteLater(); }
    step3_verifyClient();
}

void ZaloService::step3_verifyClient()
{
    qDebug() << "[Zalo] Step3: verifyClient";
    QNetworkRequest req = buildRequest("https://id.zalo.me/account/verify-client", "https://id.zalo.me/account?continue=https%3A%2F%2Fzalo.me%2Fpc");
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QList<QPair<QString,QString> > f;
    f << QPair<QString,QString>("type", "device") << QPair<QString,QString>("continue", "https://zalo.me/pc") << QPair<QString,QString>("v", m_loginVersion);
    QNetworkReply *reply = m_manager->post(req, buildFormBody(f));
    connect(reply, SIGNAL(finished()), this, SLOT(onStep3Done()));
}

void ZaloService::onStep3Done()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) { parseCookiesFromReply(reply); reply->deleteLater(); }
    step4_generateQR();
}

void ZaloService::step4_generateQR()
{
    qDebug() << "[Zalo] Step4: generateQR";
    QNetworkRequest req = buildRequest("https://id.zalo.me/account/authen/qr/generate", "https://id.zalo.me/account?continue=https%3A%2F%2Fzalo.me%2Fpc");
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QList<QPair<QString,QString> > f;
    f << QPair<QString,QString>("continue", "https://zalo.me/pc") << QPair<QString,QString>("v", m_loginVersion);
    QNetworkReply *reply = m_manager->post(req, buildFormBody(f));
    connect(reply, SIGNAL(finished()), this, SLOT(onStep4Done()));
}

void ZaloService::onStep4Done()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    if (reply->error() != QNetworkReply::NoError) {
        emit loginFailed(reply->errorString());
        reply->deleteLater();
        return;
    }
    parseCookiesFromReply(reply);
    QByteArray raw = reply->readAll();
    reply->deleteLater();

    QVariantMap root = jsonToMap(raw);
    QVariantMap data = root["data"].toMap();
    m_qrCode = data["code"].toString();

    qDebug() << "[Zalo] Step4: QR code =" << m_qrCode.left(30) << "...";
    qDebug() << "[Zalo] Step4: image field =" << data["image"].toString().left(60);

    if (m_qrCode.isEmpty()) {
        emit loginFailed("Khong nhan duoc ma QR tu Zalo. Thu lai.");
        return;
    }

    QString imageB64 = data["image"].toString();
    int comma = imageB64.indexOf(',');
    if (comma >= 0) imageB64 = imageB64.mid(comma + 1);

    if (!imageB64.isEmpty()) {
        QByteArray imgData = QByteArray::fromBase64(imageB64.toUtf8());
        QString tempPath = QDir::tempPath() + "/qr.png";
        QFile imgFile(tempPath);
        if (imgFile.open(QIODevice::WriteOnly)) {
            imgFile.write(imgData);
            imgFile.close();
        }
        emit qrCodeReady("file://" + tempPath, m_qrCode);
    } else {
        QString encoded = QString::fromUtf8(QUrl::toPercentEncoding(m_qrCode));
        QString qrApiUrl = "https://quickchart.io/qr?text=" + encoded + "&size=300&margin=2";
        qDebug() << "[Zalo] Step4: Fetching QR image from quickchart.io...";
        QNetworkRequest qrReq = buildRequest(qrApiUrl, "");
        QNetworkReply *qrReply = m_manager->get(qrReq);
        connect(qrReply, SIGNAL(finished()), this, SLOT(onQRImageFetched()));
    }
    m_qrExpireTimer->start(100000);
    step5_waitingScan();
}

void ZaloService::onQRImageFetched()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "[Zalo] onQRImageFetched: network error:" << reply->errorString();
        emit qrCodeReady("", m_qrCode);
        reply->deleteLater();
        return;
    }

    QByteArray imgData = reply->readAll();
    reply->deleteLater();

    if (imgData.isEmpty()) {
        qDebug() << "[Zalo] onQRImageFetched: empty response";
        emit qrCodeReady("", m_qrCode);
        return;
    }

    QString tempPath = QDir::tempPath() + "/qr.png";
    QFile imgFile(tempPath);
    if (imgFile.open(QIODevice::WriteOnly)) {
        imgFile.write(imgData);
        imgFile.close();
        qDebug() << "[Zalo] onQRImageFetched: saved QR image to" << tempPath;
        emit qrCodeReady("file://" + tempPath, m_qrCode);
    } else {
        qDebug() << "[Zalo] onQRImageFetched: cannot write file";
        emit qrCodeReady("", m_qrCode);
    }
}

void ZaloService::step5_waitingScan()
{
    if (m_qrCancelled) return;
    qDebug() << "[Zalo] Step5: waitingScan";
    QNetworkRequest req = buildRequest("https://id.zalo.me/account/authen/qr/waiting-scan", "https://id.zalo.me/account?continue=https%3A%2F%2Fchat.zalo.me%2F");
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QList<QPair<QString,QString> > f;
    f << QPair<QString,QString>("code", m_qrCode) << QPair<QString,QString>("continue", "https://chat.zalo.me/") << QPair<QString,QString>("v", m_loginVersion);
    QNetworkReply *reply = m_manager->post(req, buildFormBody(f));
    connect(reply, SIGNAL(finished()), this, SLOT(onStep5Done()));
}

void ZaloService::onStep5Done()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    if (m_qrCancelled) { reply->deleteLater(); return; }
    parseCookiesFromReply(reply);
    QByteArray raw = reply->readAll();
    reply->deleteLater();

    QVariantMap root = jsonToMap(raw);
    int errorCode    = root["error_code"].toInt();

    if (errorCode == 8) {
        step5_waitingScan();
    } else if (errorCode == 0) {
        m_displayName = root["data"].toMap()["display_name"].toString();
        emit qrScanned(m_displayName);
        step6_waitingConfirm();
    } else {
        emit loginFailed(QString("QR scan error: %1").arg(errorCode));
    }
}

void ZaloService::step6_waitingConfirm()
{
    if (m_qrCancelled) return;
    qDebug() << "[Zalo] Step6: waitingConfirm";
    QNetworkRequest req = buildRequest("https://id.zalo.me/account/authen/qr/waiting-confirm", "https://id.zalo.me/account?continue=https%3A%2F%2Fchat.zalo.me%2F");
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QList<QPair<QString,QString> > f;
    f << QPair<QString,QString>("code", m_qrCode) << QPair<QString,QString>("gToken", "") << QPair<QString,QString>("gAction", "CONFIRM_QR") << QPair<QString,QString>("continue", "https://chat.zalo.me/") << QPair<QString,QString>("v", m_loginVersion);
    QNetworkReply *reply = m_manager->post(req, buildFormBody(f));
    connect(reply, SIGNAL(finished()), this, SLOT(onStep6Done()));
}

void ZaloService::onStep6Done()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    if (m_qrCancelled) { reply->deleteLater(); return; }
    parseCookiesFromReply(reply);
    QByteArray raw = reply->readAll();
    reply->deleteLater();

    qDebug() << "[Zalo] Step6 Response:" << raw;

    QVariantMap root = jsonToMap(raw);
    int errorCode    = root["error_code"].toInt();

    if      (errorCode == 8)   { step6_waitingConfirm(); }
    else if (errorCode == -13) { emit loginFailed("Tu choi xac nhan"); }
    else if (errorCode == 0)   { m_qrExpireTimer->stop(); step7_checkSession(); }
    else { emit loginFailed(QString("Confirm error: %1").arg(errorCode)); }
}

void ZaloService::step7_checkSession()
{
    qDebug() << "[Zalo] Step7: checkSession";
    QNetworkReply *reply = m_manager->get(buildRequest("https://id.zalo.me/account/checksession?continue=https%3A%2F%2Fchat.zalo.me%2Findex.html", "https://id.zalo.me/account?continue=https%3A%2F%2Fchat.zalo.me%2F"));
    connect(reply, SIGNAL(finished()), this, SLOT(onStep7Done()));
}

void ZaloService::onStep7Done()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    parseCookiesFromReply(reply);

    QVariant redirectVar = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (redirectVar.isValid()) {
        QUrl redirectUrl = redirectVar.toUrl();
        if (redirectUrl.isRelative()) {
            redirectUrl = reply->url().resolved(redirectUrl);
        }
        qDebug() << "[Zalo] Step7 Dang Redirect de hung Cookie:" << redirectUrl.toString();

        QNetworkReply *redirReply = m_manager->get(buildRequest(redirectUrl.toString(), "https://id.zalo.me/"));
        connect(redirReply, SIGNAL(finished()), this, SLOT(onStep7Done()));
        reply->deleteLater();
        return;
    }

    reply->deleteLater();
    step8_getZaloLoginInfo();
}

void ZaloService::step8_getZaloLoginInfo()
{
    qDebug() << "[Zalo] Step8: getZaloLoginInfo";
    QVariantMap data;
    data["computer_name"] = QString("Web");
    data["imei"]          = m_imei;
    data["language"]      = m_language;
    data["ts"]            = QString::number(QDateTime::currentMSecsSinceEpoch());

    EncryptedParams ep = buildEncryptedParams(data);
    m_pendingEncryptKey = ep.encryptKey;

    QVariantMap paramsForSign;
    paramsForSign["zcid"]           = ep.zcid;
    paramsForSign["zcid_ext"]       = ep.zcid_ext;
    paramsForSign["enc_ver"]        = ep.enc_ver;
    paramsForSign["params"]         = ep.encryptedData;
    paramsForSign["type"]           = QString::number(API_TYPE);
    paramsForSign["client_version"] = QString::number(API_VERSION);
    paramsForSign["nretry"]         = QString("0");

    QVariantMap params = paramsForSign;
    params["signkey"] = buildSignKey("getlogininfo", paramsForSign);
    params["imei"]    = m_imei;

    qDebug() << "[Zalo] step8 signkey:" << params["signkey"].toString();
    qDebug() << "[Zalo] step8 zcid:" << params["zcid"].toString().left(20);
    qDebug() << "[Zalo] step8 zcid_ext:" << params["zcid_ext"].toString();
    qDebug() << "[Zalo] step8 enc_ver:" << params["enc_ver"].toString();
    qDebug() << "[Zalo] step8 params(enc):" << params["params"].toString().left(40);

    QString urlStr = buildRawUrl("https://wpa.chat.zalo.me/api/login/getLoginInfo", params);
    qDebug() << "[Zalo] step8 URL:" << urlStr.left(300);
    QNetworkRequest req = buildRequest(urlStr, "https://chat.zalo.me/");
    req.setRawHeader("zpw_ver",  QByteArray::number(API_VERSION));
    req.setRawHeader("zpw_type", QByteArray::number(API_TYPE));

    QNetworkReply *reply = m_manager->get(req);
    connect(reply, SIGNAL(finished()), this, SLOT(onStep8Done()));
}

void ZaloService::onStep8Done()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "[Zalo Error] Step8 network error:" << reply->errorString();
        emit loginFailed(reply->errorString());
        reply->deleteLater();
        return;
    }

    parseCookiesFromReply(reply);

    QByteArray raw = reply->readAll();
    reply->deleteLater();

    qDebug() << "[Zalo] Step8 raw response:" << raw.left(300);
    qDebug() << "[Zalo] Step8 cookies count:" << m_cookies.size();

    QVariantMap root = jsonToMap(raw);
    int errCode8 = root["error_code"].toInt();
    qDebug() << "[Zalo] Step8 error_code:" << errCode8 << "msg:" << root["error_message"].toString();

    QVariantMap info;
    QVariant dataVar = root["data"];

    if (dataVar.type() == QVariant::Map) {
        info = dataVar.toMap();
    } else {
        QString encData = dataVar.toString();
        qDebug() << "[Zalo] Step8 encrypted data (first60):" << encData.left(60);
        qDebug() << "[Zalo] Step8 pendingEncryptKey:" << m_pendingEncryptKey;
        if (!encData.isEmpty() && !m_pendingEncryptKey.isEmpty()) {
            QString decrypted = aesDecryptBase64_256(m_pendingEncryptKey, encData);
            qDebug() << "[Zalo] Step8 decrypted (first100):" << decrypted.left(100);
            QVariantMap root2 = jsonToMap(decrypted.toUtf8());
            info = root2["data"].toMap();
            if (info.isEmpty()) info = root2;
        }
    }

    if (info.isEmpty()) {
        qDebug() << "[Zalo Error] Du lieu Step 8 rong! Raw:" << raw.left(200);
        emit loginFailed("API tu choi (Loi 18060 / 401)");
        return;
    }

    m_secretKey   = info["zpw_enk"].toString();
    m_uid         = info["uid"].toString();
    m_displayName = info["display_name"].toString();

    qDebug() << "[Zalo] secretKey (first20):" << m_secretKey.left(20);
    qDebug() << "[Zalo] info keys check - zpw_enk empty?:" << info["zpw_enk"].toString().isEmpty()
             << "uid empty?:" << info["uid"].toString().isEmpty();
    qDebug() << "[Zalo] uid:" << m_uid << "name:" << m_displayName;

    QVariantMap svcMap  = info["zpw_service_map_v3"].toMap();
    QVariantList chatA  = svcMap["chat"].toList();
    QVariantList groupA = svcMap["group"].toList();

    m_chatServiceUrl.clear();
    m_groupServiceUrl.clear();

    if (!chatA.isEmpty())  m_chatServiceUrl  = chatA[0].toString();
    if (!groupA.isEmpty()) m_groupServiceUrl = groupA[0].toString();

    QVariantList profileA   = svcMap["profile"].toList();
    QVariantList grpPollA   = svcMap["group_poll"].toList();
    if (!profileA.isEmpty())  m_profileServiceUrl  = profileA[0].toString();
    if (!grpPollA.isEmpty())  m_groupPollServiceUrl = grpPollA[0].toString();

    qDebug() << "[Zalo] chat:"        << m_chatServiceUrl;
    qDebug() << "[Zalo] group:"       << m_groupServiceUrl;
    qDebug() << "[Zalo] profile:"     << m_profileServiceUrl;
    qDebug() << "[Zalo] group_poll:"  << m_groupPollServiceUrl;

    step9_getServerInfo();
}

void ZaloService::step9_getServerInfo()
{
    qDebug() << "[Zalo] Step9: getServerInfo";
    QVariantMap params;
    params["imei"]           = m_imei;
    params["type"]           = QString::number(API_TYPE);
    params["client_version"] = QString::number(API_VERSION);
    params["computer_name"]  = QString("Web");
    params["signkey"]        = buildSignKey("getserverinfo", params);

    QString urlStr = buildRawUrl("https://wpa.chat.zalo.me/api/login/getServerInfo", params);
    QNetworkReply *reply = m_manager->get(buildRequest(urlStr, "https://chat.zalo.me/"));
    connect(reply, SIGNAL(finished()), this, SLOT(onStep9Done()));
}

void ZaloService::onStep9Done()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) { parseCookiesFromReply(reply); reply->deleteLater(); }
    m_loggedIn = true;
    emit loggedInChanged();
    saveSession();
    emit loginSuccess(m_uid, m_displayName);
    m_listenTimer->start(8000);
}

void ZaloService::fetchConversations()
{
    if (!m_loggedIn) return;

    QVariantMap qp;
    qp["zpw_ver"]  = QString::number(API_VERSION);
    qp["zpw_type"] = QString::number(API_TYPE);

    QString base = m_groupPollServiceUrl.isEmpty() ? m_groupServiceUrl : m_groupPollServiceUrl;
    QString urlStr = buildRawUrl(base + "/api/group/getlg/v4", qp);
    qDebug() << "[Zalo] fetchConversations URL:" << urlStr;
    QNetworkReply *reply = m_manager->get(buildRequest(urlStr, "https://chat.zalo.me/"));
    connect(reply, SIGNAL(finished()), this, SLOT(onFetchConvoDone()));

    // ĐÃ BỎ LỆNH fetchFriends() Ở ĐÂY ĐỂ TRÁNH LỖI RATE LIMIT (429)
}

void ZaloService::onFetchConvoDone()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "[Zalo Error] fetchConversations Network Error:" << reply->errorString();
    }

    QByteArray raw = reply->readAll();
    reply->deleteLater();

    if (raw.isEmpty()) {
        emit conversationsReady(QVariantList());
        return;
    }

    qDebug() << "[Zalo] fetchConvo raw (first200):" << raw.left(200);
    QVariantMap root = jsonToMap(raw);
    int ec = root["error_code"].toInt();
    if (ec != 0) {
        qDebug() << "[Zalo Error] fetchConvo error_code:" << ec << root["error_message"].toString();
        emit conversationsReady(QVariantList());
        return;
    }

    QVariantList threads;
    QString dec = aesDecryptBase64(m_secretKey, root["data"].toString());
    qDebug() << "[Zalo] fetchConvo decrypted (first150):" << dec.left(150);

    QVariantMap outer = jsonToMap(dec.toUtf8());
    QVariantMap inner;
    if (outer.contains("data") && outer["data"].type() == QVariant::Map)
        inner = outer["data"].toMap();
    else
        inner = outer;

    QVariantMap gridVerMap = inner["gridVerMap"].toMap();
    qDebug() << "[Zalo] fetchConvo gridVerMap size:" << gridVerMap.size();

    QStringList groupIds = gridVerMap.keys();

    qDebug() << "[Zalo] fetchConvo found" << groupIds.size() << "groups, fetching details...";

    if (!groupIds.isEmpty()) {
        fetchGroupDetails(groupIds);
    } else {
        emit conversationsReady(QVariantList());
    }
}

void ZaloService::fetchGroupDetails(const QStringList &groupIds)
{
    if (groupIds.isEmpty()) return;

    QVariantMap gridVerMapObj;
    for (int i = 0; i < groupIds.size(); ++i)
        gridVerMapObj[groupIds[i]] = 0;
    QString gridVerMapStr = QString::fromUtf8(mapToJson(gridVerMapObj));
    qDebug() << "[Zalo] gridVerMap string (first100):" << gridVerMapStr.left(100);

    QVariantMap inner;
    inner["gridVerMap"] = gridVerMapStr;

    QString encParams = aesEncryptBase64(m_secretKey, QString::fromUtf8(mapToJson(inner)));

    QString urlStr = m_groupServiceUrl + "/api/group/getmg-v2"
        + "?zpw_ver=" + QString::number(API_VERSION)
        + "&zpw_type=" + QString::number(API_TYPE);

    QNetworkRequest req = buildRequest(urlStr, "https://chat.zalo.me/");
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    req.setRawHeader("Origin",  "https://chat.zalo.me");
    req.setRawHeader("Referer", "https://chat.zalo.me/");

    QByteArray body = "params=" + QUrl::toPercentEncoding(encParams);
    qDebug() << "[Zalo] fetchGroupDetails POST" << urlStr;
    qDebug() << "[Zalo] fetchGroupDetails body (first100):" << QString::fromUtf8(body.left(100));

    QNetworkReply *reply = m_manager->post(req, body);
    connect(reply, SIGNAL(finished()), this, SLOT(onGroupDetailsDone()));
}

void ZaloService::onGroupDetailsDone()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    QByteArray raw = reply->readAll();
    reply->deleteLater();
    qDebug() << "[Zalo] groupDetails raw (first200):" << raw.left(200);

    QVariantMap root = jsonToMap(raw);
    int ec = root["error_code"].toInt();
    if (ec != 0) {
        qDebug() << "[Zalo Error] groupDetails outer error:" << ec << root["error_message"].toString();
        return;
    }

    QString dec = aesDecryptBase64(m_secretKey, root["data"].toString());
    qDebug() << "[Zalo] groupDetails decrypted (first200):" << dec.left(200);

    QVariantMap outer = jsonToMap(dec.toUtf8());
    int ec2 = outer["error_code"].toInt();
    if (ec2 != 0) {
        qDebug() << "[Zalo Error] groupDetails inner error:" << ec2 << outer["error_message"].toString();
        return;
    }

    QVariantMap inner;
    if (outer.contains("data") && outer["data"].type() == QVariant::Map)
        inner = outer["data"].toMap();
    else
        inner = outer;

    qDebug() << "[Zalo] groupDetails inner keys:" << inner.keys();

    QVariantList threads;

    QVariantMap gridInfoMap = inner["gridInfoMap"].toMap();
    if (!gridInfoMap.isEmpty()) {
        QStringList keys = gridInfoMap.keys();
        for (int i = 0; i < keys.size(); ++i) {
            QVariantMap g = gridInfoMap[keys[i]].toMap();
            QVariantMap t;
            QString gname = g["name"].toString();
            if (gname.isEmpty()) gname = "Nhom " + g["groupId"].toString().right(6);
            t["threadId"] = g["groupId"].toString();
            t["name"]     = gname;
            t["isGroup"]  = true;
            t["avatar"]   = g["avt"].toString();
            t["unread"]   = 0;
            threads.append(t);
        }
    } else {
        QVariantList grids = inner["gridInfos"].toList();
        for (int i = 0; i < grids.size(); ++i) {
            QVariantMap g = grids[i].toMap();
            QVariantMap t;
            t["threadId"] = g["groupId"].toString();
            t["name"]     = g["name"].toString();
            t["isGroup"]  = true;
            t["avatar"]   = g["avt"].toString();
            t["unread"]   = 0;
            threads.append(t);
        }
    }

    qDebug() << "[Zalo] groupDetails found" << threads.size() << "groups with names";
    if (!threads.isEmpty())
        emit conversationsReady(threads);
}

void ZaloService::downloadAvatar(const QString &threadId, const QString &url)
{
    if (m_avatarCache.contains(url)) {
        emit avatarReady(threadId, m_avatarCache[url]);
        return;
    }

    // Nếu đang tải thì đăng ký thêm threadId vào waitlist, không tải lại
    if (m_pendingAvatars.contains(url)) {
        m_pendingAvatarWaiters[url].insert(threadId);
        return;
    }
    m_pendingAvatars.insert(url);
    m_pendingAvatarWaiters[url].clear();
    m_pendingAvatarWaiters[url].insert(threadId);

    QString httpUrl = url;
    if (httpUrl.startsWith("https://"))
        httpUrl = "http://" + httpUrl.mid(8);

    QUrl avatarQUrl(httpUrl);
    QNetworkRequest avatarReq(avatarQUrl);
    avatarReq.setRawHeader("Referer",    "https://chat.zalo.me/");
    avatarReq.setRawHeader("User-Agent", m_userAgent.toUtf8());
    avatarReq.setRawHeader("Accept",     "image/webp,image/apng,image/*,*/*;q=0.8");
    QNetworkReply *reply = m_manager->get(avatarReq);
    reply->setProperty("avatarUrl",      url);
    reply->setProperty("avatarThreadId", threadId);
    connect(reply, SIGNAL(finished()), this, SLOT(onAvatarDownloaded()));
}

void ZaloService::onAvatarDownloaded()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    QString url        = reply->property("avatarUrl").toString();
    QString threadId   = reply->property("avatarThreadId").toString();
    bool hasError      = (reply->error() != QNetworkReply::NoError);
    QByteArray data    = reply->readAll();
    reply->deleteLater();

    // TẢI XONG THÌ GỠ KHỎI HÀNG CHỜ
    QSet<QString> waiters = m_pendingAvatarWaiters.take(url);
    m_pendingAvatars.remove(url);

    if (hasError || data.isEmpty()) {
        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << "[Zalo] avatar download failed for" << threadId
                 << "error:" << reply->errorString()
                 << "HTTP:" << httpStatus;
        return;
    }

    QString fname = "/tmp/avatar_" + md5Hex(url) + ".jpg";
    QFile f(fname);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(data);
        f.close();
    }
    QString localPath = "file://" + fname;
    m_avatarCache[url] = localPath;
    qDebug() << "[Zalo] avatar saved:" << threadId << "->" << fname;
    // Emit cho tất cả caller đang chờ cùng URL này
    foreach (const QString &wid, waiters)
        emit avatarReady(wid, localPath);
    // Đảm bảo emit ít nhất 1 lần với threadId gốc
    if (!waiters.contains(threadId))
        emit avatarReady(threadId, localPath);
}

void ZaloService::fetchFriends()
{
    if (!m_loggedIn) return;

    QVariantMap innerParams;
    innerParams["incInvalid"]  = 1;
    innerParams["page"]        = 1;
    innerParams["count"]       = 20000;
    innerParams["avatar_size"] = 120;
    innerParams["actiontime"]  = 0;
    innerParams["imei"]        = m_imei;

    QString encParams = aesEncryptBase64(m_secretKey, QString::fromUtf8(mapToJson(innerParams)));
    QByteArray body   = "params=" + QUrl::toPercentEncoding(encParams);

    QString urlStr = m_profileServiceUrl + "/api/social/friend/getfriends"
                   + "?zpw_ver=" + QString::number(API_VERSION)
                   + "&zpw_type=" + QString::number(API_TYPE);

    QUrl friendsQUrl(urlStr);
    QNetworkRequest friendsReq(friendsQUrl);
    QSslConfiguration sslConf = friendsReq.sslConfiguration();
    sslConf.setPeerVerifyMode(QSslSocket::VerifyNone);
    friendsReq.setSslConfiguration(sslConf);
    friendsReq.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    friendsReq.setRawHeader("User-Agent", m_userAgent.toUtf8());
    friendsReq.setRawHeader("Cookie",     buildCookieHeader().toUtf8());
    friendsReq.setRawHeader("Referer",    "https://chat.zalo.me/");
    friendsReq.setRawHeader("Accept",     "application/json, text/plain, */*");

    qDebug() << "[Zalo] fetchFriends POST" << urlStr;
    qDebug() << "[Zalo] fetchFriends body len:" << body.size();
    QNetworkReply *reply = m_manager->post(friendsReq, body);
    connect(reply, SIGNAL(finished()), this, SLOT(onFetchFriendsDone()));
}

void ZaloService::onFetchFriendsDone()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    QByteArray raw = reply->readAll();
    reply->deleteLater();

    qDebug() << "[Zalo] fetchFriends raw (first300):" << raw.left(300);
    QVariantMap root = jsonToMap(raw);
    if (root["error_code"].toInt() != 0) {
        qDebug() << "[Zalo Error] fetchFriends:" << root["error_message"].toString();
        return;
    }

    QString dec = aesDecryptBase64(m_secretKey, root["data"].toString());
    qDebug() << "[Zalo] fetchFriends decrypted (first300):" << dec.left(300);

    QVariantList friends;
    QVariantMap outer = jsonToMap(dec.toUtf8());
    if (outer.contains("data") && outer["data"].type() == QVariant::List)
        friends = outer["data"].toList();
    else if (outer.contains("friends") && outer["friends"].type() == QVariant::List)
        friends = outer["friends"].toList();
    else {
        QVariantList arr = jsonToList(dec.toUtf8());
        if (!arr.isEmpty())
            friends = arr;
    }

    qDebug() << "[Zalo] fetchFriends found" << friends.size() << "friends";

    QVariantList threads;
    for (int i = 0; i < friends.size(); ++i) {
        QVariantMap f = friends[i].toMap();
        QString uid  = f["userId"].toString();
        if (uid.isEmpty()) uid = f["uid"].toString();
        QString name = f["zaloName"].toString();
        if (name.isEmpty()) name = f["displayName"].toString();
        if (name.isEmpty()) name = f["username"].toString();
        QString avatarUrl   = f["avatar"].toString();
        QString bgAvatarUrl = f["bgavatar"].toString();
        // Nếu đã có trong cache thì gán luôn, không cần đợi avatarReady
        QString localAvatar   = m_avatarCache.value(avatarUrl,   "");
        QString localBgAvatar = m_avatarCache.value(bgAvatarUrl, "");

        QVariantMap t;
        t["threadId"]      = uid;
        t["name"]          = name;
        t["isGroup"]       = false;
        t["avatar"]        = avatarUrl;
        t["bgavatar"]      = bgAvatarUrl;
        t["localAvatar"]   = localAvatar;
        t["localBgAvatar"] = localBgAvatar;
        t["unread"]        = 0;
        t["lastMessage"]   = "";
        if (!uid.isEmpty() && !name.isEmpty())
            threads.append(t);
    }

    qDebug() << "[Zalo] fetchFriends parsed" << threads.size() << "valid friends";

    if (!threads.isEmpty())
        emit friendsReady(threads);
}

void ZaloService::fetchInvites()
{
    if (!m_loggedIn) return;

    QString urlStr = m_profileServiceUrl + "/api/friend/getreqs"
                   + "?zpw_ver=" + QString::number(API_VERSION)
                   + "&zpw_type=" + QString::number(API_TYPE);

    QVariantMap innerParams;
    innerParams["uid"]  = m_uid;
    innerParams["imei"] = m_imei;

    QString encParams = aesEncryptBase64(m_secretKey, QString::fromUtf8(mapToJson(innerParams)));
    QByteArray body   = "params=" + QUrl::toPercentEncoding(encParams);

    QUrl invQUrl(urlStr);
    QNetworkRequest invReq(invQUrl);
    QSslConfiguration sc = invReq.sslConfiguration();
    sc.setPeerVerifyMode(QSslSocket::VerifyNone);
    invReq.setSslConfiguration(sc);
    invReq.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    invReq.setRawHeader("User-Agent", m_userAgent.toUtf8());
    invReq.setRawHeader("Cookie",     buildCookieHeader().toUtf8());
    invReq.setRawHeader("Referer",    "https://chat.zalo.me/");

    qDebug() << "[Zalo] fetchInvites POST" << urlStr;
    QNetworkReply *reply = m_manager->post(invReq, body);
    connect(reply, SIGNAL(finished()), this, SLOT(onFetchInvitesDone()));
}

void ZaloService::onFetchInvitesDone()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    QByteArray raw = reply->readAll();
    reply->deleteLater();

    qDebug() << "[Zalo] fetchInvites raw (first200):" << raw.left(200);
    QVariantMap root = jsonToMap(raw);
    if (root["error_code"].toInt() != 0) {
        emit invitesReady(QVariantList());
        return;
    }

    QString dec = aesDecryptBase64(m_secretKey, root["data"].toString());
    QVariantMap outer = jsonToMap(dec.toUtf8());
    QVariantList rawList;
    if (outer.contains("data") && outer["data"].type() == QVariant::List)
        rawList = outer["data"].toList();

    QVariantList invites;
    for (int i = 0; i < rawList.size(); ++i) {
        QVariantMap f = rawList[i].toMap();
        QVariantMap inv;
        inv["uid"]    = f["uid"].toString();
        inv["name"]   = f["zaloName"].toString().isEmpty()
                        ? f["displayName"].toString() : f["zaloName"].toString();
        inv["avatar"] = f["avatar"].toString();
        inv["msg"]    = f["msg"].toString();
        invites.append(inv);
    }
    qDebug() << "[Zalo] fetchInvites found" << invites.size();
    emit invitesReady(invites);
}

void ZaloService::fetchMessages(const QString &threadId, bool isGroup)
{
    if (!m_loggedIn) return;

    QVariantMap innerParams;
    if (isGroup) {
        innerParams["grid"]  = threadId;
        innerParams["count"] = 50;
    } else {
        innerParams["threadId"] = threadId;
        innerParams["count"]    = 50;
        innerParams["lastId"]   = 0;
        innerParams["lastIdClient"] = 0;
        innerParams["sender"]   = 0;
    }

    QString encParams = aesEncryptBase64(m_secretKey, QString::fromUtf8(mapToJson(innerParams)));
    QString base      = isGroup ? m_groupServiceUrl + "/api/group/getgroupmsg"
                                : m_chatServiceUrl  + "/api/message/getmsglist";

    QString urlStr = base + "?zpw_ver=" + QString::number(API_VERSION)
                          + "&zpw_type=" + QString::number(API_TYPE);

    QNetworkRequest req = buildRequest(urlStr, "https://chat.zalo.me/");
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    req.setRawHeader("Origin",  "https://chat.zalo.me");
    req.setRawHeader("Referer", "https://chat.zalo.me/");

    QByteArray body = "params=" + QUrl::toPercentEncoding(encParams);
    qDebug() << "[Zalo] fetchMessages POST" << urlStr << "isGroup:" << isGroup;

    QNetworkReply *reply = m_manager->post(req, body);
    reply->setProperty("threadId", threadId);
    reply->setProperty("isGroup",  isGroup);
    connect(reply, SIGNAL(finished()), this, SLOT(onFetchMsgDone()));
}

void ZaloService::onFetchMsgDone()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    QString tid    = reply->property("threadId").toString();
    QByteArray raw = reply->readAll();
    reply->deleteLater();

    qDebug() << "[Zalo] fetchMessages raw (first200):" << raw.left(200);
    QVariantMap root = jsonToMap(raw);
    if (root["error_code"].toInt() != 0) {
        qDebug() << "[Zalo Error] fetchMessages error:" << root["error_message"].toString();
        emit messagesReady(tid, QVariantList());
        return;
    }

    QVariantList msgs;
    bool isGroup = reply->property("isGroup").toBool();

    QString dec2 = aesDecryptBase64(m_secretKey, root["data"].toString());
    qDebug() << "[Zalo] fetchMessages decrypted (first150):" << dec2.left(150);

    QVariantMap outer2 = jsonToMap(dec2.toUtf8());
    QVariantMap d2;
    if (outer2.contains("data") && outer2["data"].type() == QVariant::Map)
        d2 = outer2["data"].toMap();
    else
        d2 = outer2;

    QVariantList rawMsgs = isGroup ? d2["groupMsgs"].toList() : d2["msgs"].toList();
    if (rawMsgs.isEmpty()) rawMsgs = d2["data"].toList();
    qDebug() << "[Zalo] fetchMessages rawMsgs count:" << rawMsgs.size() << "d2 keys:" << d2.keys();

    for (int i = 0; i < rawMsgs.size(); ++i) {
        QVariantMap m = rawMsgs[i].toMap();
        QVariantMap out;
        out["msgId"]    = m["msgId"].toString();
        out["content"]  = m["content"].toString();
        out["senderId"] = m["uidFrom"].toString();
        out["dName"]    = m["dName"].toString();
        out["ts"]       = m["ts"].toString();
        out["isGroup"]  = isGroup;
        out["isMine"]   = (m["uidFrom"].toString() == m_uid);
        msgs.append(out);
    }
    qDebug() << "[Zalo] fetchMessages found" << msgs.size() << "messages";
    emit messagesReady(tid, msgs);
}

void ZaloService::sendMessage(const QString &threadId, const QString &content, bool isGroup)
{
    if (!m_loggedIn) return;

    QVariantMap msgData;
    msgData["message"]  = content;
    msgData["clientId"] = QString::number(QDateTime::currentMSecsSinceEpoch());
    msgData["imei"]     = m_imei;
    if (isGroup) {
        msgData["visibility"] = 0;
        msgData["grid"]       = threadId;
    } else {
        msgData["toid"] = threadId;
    }

    QString encParams = aesEncryptBase64(m_secretKey, QString::fromUtf8(mapToJson(msgData)));
    QByteArray body   = "params=" + QUrl::toPercentEncoding(encParams);

    QString base = isGroup ? m_groupServiceUrl + "/api/group/sendmsg"
                           : m_chatServiceUrl  + "/api/message/sendmessage";

    QString urlStr = base + "?zpw_ver=" + QString::number(API_VERSION)
                          + "&zpw_type=" + QString::number(API_TYPE);

    QNetworkRequest sendReq = buildRequest(urlStr, "https://chat.zalo.me/");
    sendReq.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    qDebug() << "[Zalo] sendMessage POST" << urlStr << "isGroup:" << isGroup;
    QNetworkReply *reply = m_manager->post(sendReq, body);
    reply->setProperty("threadId", threadId);
    connect(reply, SIGNAL(finished()), this, SLOT(onSendMsgDone()));
}

void ZaloService::onSendMsgDone()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    bool hasError = (reply->error() != QNetworkReply::NoError);
    QString tid   = reply->property("threadId").toString();
    QByteArray raw = reply->readAll();
    reply->deleteLater();
    qDebug() << "[Zalo] sendMessage response:" << raw.left(200);
    emit messageSent(!hasError, tid);
}

void ZaloService::onQRExpired()
{
    if (!m_loggedIn) {
        emit qrExpired();
        step1_loadLoginPage();
    }
}

void ZaloService::onListenTimer()
{
    if (!m_loggedIn) return;
    QVariantMap params;
    params["zpw_ver"]  = QString::number(API_VERSION);
    params["zpw_type"] = QString::number(API_TYPE);

    QString urlStr = buildRawUrl(m_chatServiceUrl + "/api/message/getrecentgroup", params);
    QNetworkReply *reply = m_manager->get(buildRequest(urlStr, "https://chat.zalo.me/"));
    connect(reply, SIGNAL(finished()), this, SLOT(onListenDone()));
}

void ZaloService::onListenDone()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    QByteArray raw = reply->readAll();
    reply->deleteLater();

    QVariantMap root = jsonToMap(raw);
    if (root["error_code"].toInt() != 0) return;

    qDebug() << "[Zalo] listenTimer: poll OK";
}

static QByteArray resolveKeyUtf8(const QString &keyStr)
{
    QByteArray k = keyStr.toUtf8();
    while (k.size() < 32) k.append('\0');
    return k.left(32);
}

static QByteArray resolveKeyBase64(const QString &keyStr)
{
    QByteArray decoded = QByteArray::fromBase64(keyStr.toUtf8());
    int sz = decoded.size();
    if (sz <= 16) { while (decoded.size() < 16) decoded.append('\0'); return decoded.left(16); }
    if (sz <= 24) { while (decoded.size() < 24) decoded.append('\0'); return decoded.left(24); }
    while (decoded.size() < 32) decoded.append('\0');
    return decoded.left(32);
}

static QByteArray resolveKey(const QString &keyStr)
{
    return resolveKeyBase64(keyStr);
}

QString ZaloService::aesEncryptHex(const QString &keyHex32, const QString &plainText)
{
    QByteArray key  = resolveKeyUtf8(keyHex32);
    QByteArray data = plainText.toUtf8();
    unsigned char iv[AES_BLOCK_SIZE];
    memset(iv, 0, AES_BLOCK_SIZE);

    int pad = AES_BLOCK_SIZE - (data.size() % AES_BLOCK_SIZE);
    data.append(QByteArray(pad, (char)pad));

    QByteArray out(data.size(), '\0');
    AES_KEY k;
    AES_set_encrypt_key((const unsigned char*)key.constData(), 256, &k);
    AES_cbc_encrypt((const unsigned char*)data.constData(), (unsigned char*)out.data(), data.size(), &k, iv, AES_ENCRYPT);
    return out.toHex().toUpper();
}

QString ZaloService::aesDecryptBase64_256(const QString &keyStr, const QString &cipherB64)
{
    QByteArray key    = resolveKeyUtf8(keyStr);
    QByteArray cipher = QByteArray::fromBase64(
        QUrl::fromPercentEncoding(cipherB64.toUtf8()).toUtf8());
    if (cipher.isEmpty()) return QString();

    unsigned char iv[AES_BLOCK_SIZE];
    memset(iv, 0, AES_BLOCK_SIZE);

    QByteArray out(cipher.size(), '\0');
    AES_KEY k;
    AES_set_decrypt_key((const unsigned char*)key.constData(), 256, &k);
    AES_cbc_encrypt((const unsigned char*)cipher.constData(),
                    (unsigned char*)out.data(), cipher.size(), &k, iv, AES_DECRYPT);

    if (!out.isEmpty()) {
        int pad = (unsigned char)out[out.size()-1];
        if (pad > 0 && pad <= AES_BLOCK_SIZE) out.chop(pad);
    }
    return QString::fromUtf8(out);
}

QString ZaloService::aesEncryptBase64(const QString &keyStr, const QString &plainText)
{
    QByteArray key  = resolveKeyBase64(keyStr);
    int keyBits = key.size() * 8;
    QByteArray data = plainText.toUtf8();
    unsigned char iv[AES_BLOCK_SIZE];
    memset(iv, 0, AES_BLOCK_SIZE);

    int pad = AES_BLOCK_SIZE - (data.size() % AES_BLOCK_SIZE);
    data.append(QByteArray(pad, (char)pad));

    QByteArray out(data.size(), '\0');
    AES_KEY k;
    AES_set_encrypt_key((const unsigned char*)key.constData(), keyBits, &k);
    AES_cbc_encrypt((const unsigned char*)data.constData(), (unsigned char*)out.data(), data.size(), &k, iv, AES_ENCRYPT);
    return out.toBase64();
}

QString ZaloService::aesEncryptBase64_256(const QString &keyStr, const QString &plainText)
{
    QByteArray key  = resolveKeyUtf8(keyStr);
    qDebug() << "[Zalo] aesEncryptBase64_256(params) keyBytes=" << key.toHex() << "bits=" << (key.size()*8);
    QByteArray data = plainText.toUtf8();
    unsigned char iv[AES_BLOCK_SIZE];
    memset(iv, 0, AES_BLOCK_SIZE);

    int pad = AES_BLOCK_SIZE - (data.size() % AES_BLOCK_SIZE);
    data.append(QByteArray(pad, (char)pad));

    QByteArray out(data.size(), '\0');
    AES_KEY k;
    AES_set_encrypt_key((const unsigned char*)key.constData(), 256, &k);
    AES_cbc_encrypt((const unsigned char*)data.constData(), (unsigned char*)out.data(), data.size(), &k, iv, AES_ENCRYPT);
    return out.toBase64();
}

QString ZaloService::aesDecryptBase64(const QString &keyStr, const QString &cipherB64)
{
    if (cipherB64.isEmpty()) return QString();
    QByteArray key    = resolveKey(keyStr);
    int keyBits = key.size() * 8;
    QString decoded   = QUrl::fromPercentEncoding(cipherB64.toUtf8());
    QByteArray cipher = QByteArray::fromBase64(decoded.toUtf8());
    if (cipher.isEmpty()) return QString();

    unsigned char iv[AES_BLOCK_SIZE];
    memset(iv, 0, AES_BLOCK_SIZE);

    QByteArray out(cipher.size(), '\0');
    AES_KEY k;
    AES_set_decrypt_key((const unsigned char*)key.constData(), keyBits, &k);
    AES_cbc_encrypt((const unsigned char*)cipher.constData(), (unsigned char*)out.data(), cipher.size(), &k, iv, AES_DECRYPT);

    if (!out.isEmpty()) {
        unsigned char pad = (unsigned char)out.at(out.size() - 1);
        if (pad > 0 && pad <= AES_BLOCK_SIZE) out.chop(pad);
    }
    return QString::fromUtf8(out);
}

QString ZaloService::md5Hex(const QString &input)
{
    return QCryptographicHash::hash(input.toUtf8(), QCryptographicHash::Md5).toHex().toLower();
}

QString ZaloService::md5Hex(const QByteArray &input)
{
    return QCryptographicHash::hash(input, QCryptographicHash::Md5).toHex().toLower();
}

QString ZaloService::randomHexString(int len)
{
    QString r;
    while (r.length() < len) r += QString::number((quint32)qrand(), 16);
    return r.left(len);
}

ZaloService::EncryptedParams ZaloService::buildEncryptedParams(const QVariantMap &data)
{
    EncryptedParams ep;
    ep.enc_ver = "v2";
    qDebug() << "[Zalo] buildEncryptedParams enc_ver=v2 API=" << API_VERSION;

    QString ts = data.contains("ts") ? data["ts"].toString()
                                     : QString::number(QDateTime::currentMSecsSinceEpoch());
    QString firstLaunchTime = QString::number(QDateTime::currentMSecsSinceEpoch());

    QString zcidMsg = QString("%1,%2,%3").arg(API_TYPE).arg(m_imei).arg(firstLaunchTime);
    ep.zcid     = aesEncryptHex(AES_FIXED_KEY, zcidMsg);
    ep.zcid_ext = randomHexString(8).toLower();

    QString n = md5Hex(ep.zcid_ext).toUpper();

    QStringList nEven;
    for (int i = 0; i < n.length(); i += 2) nEven << QString(n[i]);

    QStringList zEven, zOdd;
    for (int i = 0; i < ep.zcid.length(); ++i) {
        if (i % 2 == 0) zEven << QString(ep.zcid[i]);
        else             zOdd  << QString(ep.zcid[i]);
    }
    for (int i = 0, j = zOdd.size() - 1; i < j; ++i, --j) zOdd.swap(i, j);

    QStringList nEven8(nEven.mid(0, 8));
    QStringList zEven12(zEven.mid(0, 12));
    QStringList zOdd12(zOdd.mid(0, 12));
    ep.encryptKey = nEven8.join("") + zEven12.join("") + zOdd12.join("");

    ep.encryptedData = aesEncryptBase64_256(ep.encryptKey, QString::fromUtf8(mapToJson(data)));
    qDebug() << "[Zalo] buildEncryptedParams encryptKey=" << ep.encryptKey << "zcid_ext=" << ep.zcid_ext;
    return ep;
}

QString ZaloService::buildSignKey(const QString &type, const QVariantMap &params)
{
    QStringList keys = params.keys();
    keys.sort();
    QString a = "zsecure" + type;
    for (int i = 0; i < keys.size(); ++i) a += params[keys[i]].toString();
    return md5Hex(a);
}

QString ZaloService::generateIMEI()
{
    return generateUUIDv4() + "-" + md5Hex(m_userAgent);
}

QString ZaloService::generateUUIDv4()
{
    return QUuid::createUuid().toString().remove('{').remove('}').toLower();
}

QString ZaloService::buildRawUrl(const QString &base, const QVariantMap &params)
{
    QString safeBase = base.trimmed().isEmpty() ? "https://wpa.chat.zalo.me" : base;

    QStringList qp;
    for (QVariantMap::const_iterator it = params.constBegin(); it != params.constEnd(); ++it) {
        QString k = QString::fromUtf8(QUrl::toPercentEncoding(it.key()));
        QString v = QString::fromUtf8(QUrl::toPercentEncoding(it.value().toString()));
        qp << (k + "=" + v);
    }
    if (qp.isEmpty()) return safeBase;
    return safeBase + "?" + qp.join("&");
}

QNetworkRequest ZaloService::buildRequest(const QString &urlStr, const QString &referer, bool jsonAccept)
{
    QUrl url = QUrl::fromEncoded(urlStr.toUtf8());
    QNetworkRequest req(url);

    QSslConfiguration sslConf = req.sslConfiguration();
    sslConf.setPeerVerifyMode(QSslSocket::VerifyNone);
    req.setSslConfiguration(sslConf);

    req.setRawHeader("User-Agent", m_userAgent.toUtf8());
    req.setRawHeader("Accept", jsonAccept ? "application/json, text/plain, */*" : "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8");
    req.setRawHeader("Accept-Language", "vi-VN,vi;q=0.9,en-US;q=0.8,en;q=0.7");

    if (!referer.isEmpty()) {
        req.setRawHeader("Referer", referer.toUtf8());
        QString origin = referer;
        int slashCount = 0;
        int pos = 0;
        for(int i=0; i<referer.length(); i++) {
            if(referer[i] == '/') slashCount++;
            if(slashCount == 3) { pos = i; break; }
        }
        if(pos > 0) origin = referer.left(pos);
        req.setRawHeader("Origin", origin.toUtf8());
    }

    req.setRawHeader("Connection", "keep-alive");
    req.setRawHeader("sec-fetch-dest", "empty");
    req.setRawHeader("sec-fetch-mode", "cors");
    req.setRawHeader("sec-fetch-site", "same-origin");

    QString ck = buildCookieHeader();
    if (!ck.isEmpty()) {
        req.setRawHeader("Cookie", ck.toUtf8());
    }

    return req;
}

QString ZaloService::buildCookieHeader() const
{
    QStringList p;
    for (QMap<QString,QString>::const_iterator it = m_cookies.constBegin(); it != m_cookies.constEnd(); ++it)
        p << it.key() + "=" + it.value();
    return p.join("; ");
}

void ZaloService::parseCookiesFromReply(QNetworkReply *reply)
{
    foreach (const QByteArray &h, reply->rawHeaderList()) {
        if (h.toLower() == "set-cookie") {
            QList<QByteArray> lines = reply->rawHeader(h).split('\n');
            foreach (const QByteArray &lineBytes, lines) {
                QString line = QString::fromUtf8(lineBytes);
                QString kv   = line.split(";").first().trimmed();
                int eq = kv.indexOf('=');
                if (eq > 0) {
                    m_cookies[kv.left(eq).trimmed()] = kv.mid(eq + 1).trimmed();
                }
            }
        }
    }
}

void ZaloService::saveSession()
{
    QSettings s("BerryLife", "Zalo10");
    s.setValue("uid",        m_uid);
    s.setValue("secretKey",  m_secretKey);
    s.setValue("imei",       m_imei);
    s.setValue("chatUrl",    m_chatServiceUrl);
    s.setValue("groupUrl",   m_groupServiceUrl);
    s.setValue("profileUrl", m_profileServiceUrl);
    s.setValue("grpPollUrl", m_groupPollServiceUrl);

    QVariantMap cookieMap;
    QMapIterator<QString, QString> it(m_cookies);
    while (it.hasNext()) {
        it.next();
        cookieMap[it.key()] = it.value();
    }
    QString cookieJson = mapToJson(cookieMap);
    s.setValue("cookies", cookieJson);
    s.sync();
    qDebug() << "[Zalo] saveSession: saved" << m_cookies.size() << "cookies, uid=" << m_uid;
}

bool ZaloService::loadSession()
{
    QSettings s("BerryLife", "Zalo10");
    QString uid = s.value("uid").toString();
    if (uid.isEmpty()) {
        qDebug() << "[Zalo] loadSession: no saved session";
        return false;
    }

    m_uid              = uid;
    m_secretKey        = s.value("secretKey").toString();
    m_imei             = s.value("imei").toString();
    m_chatServiceUrl   = s.value("chatUrl").toString();
    m_groupServiceUrl  = s.value("groupUrl").toString();
    m_profileServiceUrl= s.value("profileUrl").toString();
    m_groupPollServiceUrl = s.value("grpPollUrl").toString();

    QString cookieJson = s.value("cookies").toString();
    if (!cookieJson.isEmpty()) {
        QVariantMap cookieMap = jsonToMap(cookieJson.toUtf8());
        QMapIterator<QString, QVariant> it(cookieMap);
        while (it.hasNext()) {
            it.next();
            m_cookies[it.key()] = it.value().toString();
        }
    }

    if (m_uid.isEmpty() || m_secretKey.isEmpty() || m_chatServiceUrl.isEmpty()) {
        qDebug() << "[Zalo] loadSession: incomplete session data";
        return false;
    }

    m_loggedIn = true;
    emit loggedInChanged();
    m_listenTimer->start(8000);
    qDebug() << "[Zalo] loadSession: restored session uid=" << m_uid
             << "cookies=" << m_cookies.size();
    emit loginSuccess(m_uid, m_displayName);
    return true;
}
