#include "../src/update_check.hpp"
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QEventLoop>
#include <QTimer>
#include <cstdio>

int main(int argc,char** argv){
    QCoreApplication app(argc,argv);QTemporaryDir temporary(QDir::currentPath()+"/download-XXXXXX");
    if(!temporary.isValid())return 2;
    const QString target=temporary.path()+"/Azec.new.exe";
    const QByteArray payload(600000,'a'),hash=QCryptographicHash::hash(payload,QCryptographicHash::Sha256).toHex();
    int failures=0;
    const auto check=[&](const char* name,bool ok){std::printf("%s: %s\n",name,ok?"PASS":"FAIL");if(!ok)++failures;};
    check("sha256 parsing",azec_update::validHash("sha256:"+hash)==hash&&azec_update::validHash("abcd").isEmpty());
    check("checksum sidecar parsing",azec_update::checksum(hash+"  Azec.exe\n")==hash&&azec_update::checksum(hash+"  Other.exe\n").isEmpty());
    auto download=[&](const QByteArray& body,const QByteArray& expected,int status=200,bool truncate=false,const QString& destination=QString()){
        QTcpServer server;if(!server.listen(QHostAddress::LocalHost,0))return false;
        QObject::connect(&server,&QTcpServer::newConnection,&server,[&]{
            auto* socket=server.nextPendingConnection();
            QObject::connect(socket,&QTcpSocket::readyRead,socket,[&,socket]{
                socket->readAll();
                const auto length=truncate?body.size()+100:body.size();
                socket->write("HTTP/1.1 "+QByteArray::number(status)+" Result\r\nContent-Length: "+QByteArray::number(length)+"\r\nConnection: close\r\n\r\n"+body);
                socket->disconnectFromHost();
            });
            QObject::connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
        });
        QNetworkAccessManager network;QEventLoop loop;bool result=false,done=false;
        QTimer timer;timer.setSingleShot(true);QObject::connect(&timer,&QTimer::timeout,&loop,&QEventLoop::quit);timer.start(10000);
        azec_update::downloadVerified(&network,&loop,QUrl("http://127.0.0.1:"+QString::number(server.serverPort())+"/Azec.exe"),
            expected,destination.isEmpty()?target:destination,[&](bool ok){result=ok;done=true;loop.quit();});
        if(!done)loop.exec();
        return done&&result;
    };
    const auto read=[&]{QFile file(target);if(!file.open(QIODevice::ReadOnly))return QByteArray();return file.readAll();};
    check("verified download committed",download(payload,hash)&&read()==payload);
    check("corrupt download rejected and previous file preserved",!download(QByteArray(600000,'b'),hash)&&read()==payload);
    check("truncated response rejected",!download(payload,hash,200,true)&&read()==payload);
    check("http error rejected",!download(payload,hash,500)&&read()==payload);
    check("missing hash rejected",!download(payload,{})&&read()==payload);
    check("short payload rejected",!download("short",QCryptographicHash::hash("short",QCryptographicHash::Sha256).toHex())&&read()==payload);
    check("unwritable destination rejected",!download(payload,hash,200,false,temporary.path()+"/missing/Azec.exe"));
    check("versions compared numerically",azec_update::newer("2026.10.20.10","2026.10.19.2213")&&!azec_update::newer("2026.10.19.9","2026.10.19.10"));
    QJsonObject asset{{"name","Azec.exe"},{"browser_download_url","https://github.com/Azec-Zuckerberg/Azec/releases/download/v2026.10.25.10/Azec.exe"},{"digest","sha256:"+QString::fromLatin1(hash)}};
    QJsonObject release{{"tag_name","v2026.10.25.10"},{"assets",QJsonArray{asset}}};
    const auto parse=[&](const QString& current="2026.10.24.1"){return azec_update::releaseFromJson(QJsonDocument(release).toJson(),current);};
    check("startup accepts the exact public release asset",parse().checked&&parse().downloadable()&&parse().digest==hash);
    check("startup does not downgrade",parse("2026.10.26.1").checked&&!parse("2026.10.26.1").required);
    release["tag_name"]="not-a-version";check("malformed version is not an update",!parse().checked);release["tag_name"]="v2026.10.25.10";
    release["draft"]=true;check("draft is not a release",!parse().checked);release.remove("draft");
    asset["browser_download_url"]="https://example.com/Azec.exe";release["assets"]=QJsonArray{asset};
    check("foreign executable URL cannot be installed",parse().required&&!parse().downloadable());
    asset["browser_download_url"]="https://github.com/Azec-Zuckerberg/Azec/releases/download/v2026.10.25.10/Azec.exe";asset.remove("digest");release["assets"]=QJsonArray{asset};
    check("missing digest keeps the update required without executing",parse().required&&!parse().downloadable());
    if(app.arguments().contains("--live-github")){
        QNetworkAccessManager network;QEventLoop loop;
        auto* reply=network.get(azec_update::request(QUrl(azec_update::latestApi)));
        QObject::connect(reply,&QNetworkReply::finished,&loop,&QEventLoop::quit);loop.exec();
        const bool ok=reply->error()==QNetworkReply::NoError;
        const auto root=QJsonDocument::fromJson(reply->readAll()).object();reply->deleteLater();
        check("packaged HTTPS release metadata",ok&&!root["tag_name"].toString().isEmpty());
        QUrl asset,sidecar;QByteArray digest;
        for(const auto& value:root["assets"].toArray()){
            const auto item=value.toObject();
            if(item["name"].toString()=="Azec.exe"){asset=QUrl(item["browser_download_url"].toString());digest=azec_update::validHash(item["digest"].toString().toLatin1());}
            if(item["name"].toString()=="SHA256SUMS.txt")sidecar=QUrl(item["browser_download_url"].toString());
        }
        if(digest.isEmpty()&&!sidecar.isEmpty()){
            reply=network.get(azec_update::request(sidecar));QObject::connect(reply,&QNetworkReply::finished,&loop,&QEventLoop::quit);loop.exec();
            if(reply->error()==QNetworkReply::NoError)digest=azec_update::checksum(reply->readAll());reply->deleteLater();
        }
        bool verified=false,done=false;
        if(!asset.isEmpty()&&!digest.isEmpty()){
            azec_update::downloadVerified(&network,&loop,asset,digest,temporary.path()+"/published.exe",[&](bool valid){verified=valid;done=true;loop.quit();});
            if(!done)loop.exec();
        }
        check("published EXE downloaded and SHA256 verified",verified);
    }
    return failures?1:0;
}
