#pragma once
// Update check against the public releases-only repository. Anonymous HTTPS only:
// no account, no identifiers, no telemetry. Failure is always non-fatal.
#include <QString>
#include <QStringList>
#include <QtGlobal>
#include <QtNetwork>
#include <QSaveFile>
#include <QCryptographicHash>
#include <functional>
#include <memory>

namespace azec_update{
inline constexpr const char* releasesRepo="Azec-Zuckerberg/Azec-releases";
inline constexpr const char* releasePage="https://github.com/Azec-Zuckerberg/Azec-releases/releases/latest";
inline constexpr const char* latestApi="https://api.github.com/repos/Azec-Zuckerberg/Azec-releases/releases/latest";
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
                             const QByteArray& expected,const QString& target,std::function<void(bool)> finished){
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
}
