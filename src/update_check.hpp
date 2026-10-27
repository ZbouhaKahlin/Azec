#pragma once
// Update check against the public source-and-releases repository. Anonymous HTTPS only:
// no account, no identifiers, no telemetry. Downloads are verified before use.
#include <QString>
#include <QStringList>
#include <QtGlobal>
#include <QtNetwork>
#include <QSaveFile>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <functional>
#include <memory>

namespace azec_update{
inline constexpr const char* releasesRepo="Azec-Zuckerberg/Azec";
inline constexpr const char* releasePage="https://github.com/Azec-Zuckerberg/Azec/releases/latest";
inline constexpr const char* latestApi="https://api.github.com/repos/Azec-Zuckerberg/Azec/releases/latest";
inline constexpr const char* assetName="Azec.exe";

inline QByteArray validHash(QByteArray hash){
    hash=hash.trimmed().toLower();if(hash.startsWith("sha256:"))hash.remove(0,7);
    if(hash.size()!=64)return {};
    for(auto c:hash)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return {};
    return hash;
}
inline QByteArray checksum(const QByteArray& document){
    for(auto line:document.split('\n')){
        line=line.trimmed();const auto hash=validHash(line.left(64));
        const auto name=line.mid(64).trimmed();
        if(!hash.isEmpty()&&(name=="Azec.exe"||name=="*Azec.exe"))return hash;
    }
    return {};
}
inline QNetworkRequest request(const QUrl& url){
    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::UserAgentHeader,QString("Azec/")+QCoreApplication::applicationVersion());
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(30000);return request;
}
// Stream to an atomic temporary file. No executable is exposed to the updater
// until the complete response matches the release's SHA-256.
inline void downloadVerified(QNetworkAccessManager* network,QObject* owner,const QUrl& url,
                             const QByteArray& expected,const QString& target,std::function<void(bool)> finished,
                             std::function<void(qint64,qint64)> progress={}){
    struct Download {
        QSaveFile file;
        QCryptographicHash hash{QCryptographicHash::Sha256};
        qint64 size=0;bool ok=true;
        explicit Download(const QString& path):file(path){}
    };
    const auto digest=validHash(expected);
    auto state=std::make_shared<Download>(target);
    if(digest.isEmpty()||target.isEmpty()||!state->file.open(QIODevice::WriteOnly)){finished(false);return;}
    auto* reply=network->get(request(url));reply->setReadBufferSize(1024*1024);
    if(progress)QObject::connect(reply,&QNetworkReply::downloadProgress,owner,std::move(progress));
    const auto consume=[reply,state]{
        const auto bytes=reply->readAll();state->size+=bytes.size();
        if(state->size>128*1024*1024||state->file.write(bytes)!=bytes.size()){state->ok=false;reply->abort();return;}
        state->hash.addData(bytes);
    };
    QObject::connect(reply,&QNetworkReply::readyRead,owner,consume);
    QObject::connect(reply,&QNetworkReply::finished,owner,[reply,state,digest,consume,finished]{
        if(state->ok)consume();
        const bool ok=state->ok&&reply->error()==QNetworkReply::NoError&&
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200&&
            state->size>=500000&&state->hash.result().toHex()==digest&&state->file.commit();
        if(!ok)state->file.cancelWriting();
        reply->deleteLater();finished(ok);
    });
}

inline QList<int> versionParts(QString value){
    if(value.startsWith('v')||value.startsWith('V'))value.remove(0,1);
    QList<int> parts;
    for(const auto& piece:value.split('.',Qt::SkipEmptyParts))parts.append(piece.toInt());
    return parts;
}

// True when candidate is strictly newer than current. Both are dotted numbers.
inline bool newer(const QString& candidate,const QString& current){
    const auto left=versionParts(candidate),right=versionParts(current);
    for(int index=0;index<qMax(left.size(),right.size());++index){
        const int a=index<left.size()?left[index]:0,b=index<right.size()?right[index]:0;
        if(a!=b)return a>b;
    }
    return false;
}

struct Release {
    bool checked=false,required=false;
    QString version;QUrl asset;QByteArray digest;
    bool downloadable() const{return required&&!asset.isEmpty()&&!digest.isEmpty();}
};
inline bool numericVersion(QString value){
    if(value.startsWith('v')||value.startsWith('V'))value.remove(0,1);
    const auto parts=value.split('.');if(parts.size()<2||parts.size()>6)return false;
    for(const auto& part:parts){bool ok=false;part.toInt(&ok);if(!ok||part.isEmpty())return false;for(auto c:part)if(c<'0'||c>'9')return false;}
    return true;
}
inline Release releaseFromJson(const QByteArray& bytes,const QString& current){
    const auto root=QJsonDocument::fromJson(bytes).object();const auto tag=root["tag_name"].toString();
    if(!numericVersion(tag)||root["draft"].toBool()||root["prerelease"].toBool())return {};
    Release result;result.checked=true;result.version=tag;
    if(result.version.startsWith('v')||result.version.startsWith('V'))result.version.remove(0,1);
    result.required=newer(result.version,current);if(!result.required)return result;
    // A known newer release stays mandatory even if its asset metadata is broken.
    const QString path="/"+QString(releasesRepo)+"/releases/download/"+tag+"/"+assetName;
    for(const auto& value:root["assets"].toArray()){
        const auto asset=value.toObject();if(asset["name"].toString()!=assetName)continue;
        const QUrl url(asset["browser_download_url"].toString());
        if(url.scheme()!="https"||url.host()!="github.com"||url.path()!=path||!url.userInfo().isEmpty()||
           (url.port()!=-1&&url.port()!=443)||url.hasQuery()||url.hasFragment())return result;
        result.asset=url;result.digest=validHash(asset["digest"].toString().toLatin1());break;
    }
    return result;
}
inline void checkLatest(QNetworkAccessManager* network,QObject* owner,const QString& current,
                        std::function<void(Release)> finished,const QUrl& endpoint=QUrl(latestApi)){
    auto query=request(endpoint);query.setRawHeader("Accept","application/vnd.github+json");query.setTransferTimeout(8000);
    auto* reply=network->get(query);reply->setReadBufferSize(256*1024);
    auto bytes=std::make_shared<QByteArray>();auto tooLarge=std::make_shared<bool>(false);
    const auto consume=[reply,bytes,tooLarge]{bytes->append(reply->readAll());if(bytes->size()>1024*1024){*tooLarge=true;reply->abort();}};
    QObject::connect(reply,&QNetworkReply::readyRead,owner,consume);
    QTimer::singleShot(8000,reply,[reply]{if(!reply->isFinished())reply->abort();});
    QObject::connect(reply,&QNetworkReply::finished,owner,[reply,current,finished,consume,bytes,tooLarge]{
        if(!*tooLarge)consume();
        const bool ok=!*tooLarge&&reply->error()==QNetworkReply::NoError&&reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200;
        const auto result=ok?releaseFromJson(*bytes,current):Release{};reply->deleteLater();finished(result);
    });
}
}
