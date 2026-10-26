#pragma once
#include "../src/startup_update.hpp"
#include <QTcpServer>
#include <QTcpSocket>

// Route the real GitHub requests to a local fixture. No downloaded fixture is executed.
class StartupUpdateNetwork final : public QNetworkAccessManager {
    QUrl origin_;
protected:
    QNetworkReply* createRequest(Operation op,const QNetworkRequest& request,QIODevice* outgoing=nullptr) override{
        auto local=request;local.setUrl(origin_.resolved(QUrl(request.url().host()=="api.github.com"?"/latest":"/Azec.exe")));
        return QNetworkAccessManager::createRequest(op,local,outgoing);
    }
public:
    explicit StartupUpdateNetwork(const QUrl& origin):origin_(origin){}
};

class StartupHostEvents final : public QObject {
public:
    int shows=0,hides=0,nativeChanges=0;
    bool eventFilter(QObject*,QEvent* event) override{
        if(event->type()==QEvent::Show)++shows;
        if(event->type()==QEvent::Hide)++hides;
        if(event->type()==QEvent::WinIdChange)++nativeChanges;
        return false;
    }
};

inline QJsonObject startupUpdateChecks(const QString& output,std::function<QJsonObject(QNetworkAccessManager*)> handoff={}){
    using Phase=StartupUpdateWindow::Phase;
    QJsonObject checks;QTcpServer server;
    if(!server.listen(QHostAddress::LocalHost,0))return {{"passed",false},{"error","local fixture server"}};
    const QByteArray payload(600000,'a'),digest=QCryptographicHash::hash(payload,QCryptographicHash::Sha256).toHex();
    QString mode="current";int metadataRequests=0,assetRequests=0;
    const auto metadata=[&]{
        const QString tag=mode=="current"?"v"+QCoreApplication::applicationVersion():"v2099.1.1";
        const QJsonObject asset{{"name","Azec.exe"},{"browser_download_url","https://github.com/Azec-Zuckerberg/Azec-releases/releases/download/"+tag+"/Azec.exe"},
                                {"digest",mode=="missing-hash"?QString():"sha256:"+QString::fromLatin1(digest)}};
        return QJsonDocument(QJsonObject{{"tag_name",tag},{"assets",QJsonArray{asset}}}).toJson(QJsonDocument::Compact);
    };
    QObject::connect(&server,&QTcpServer::newConnection,&server,[&]{
        while(server.hasPendingConnections()){
            auto* socket=server.nextPendingConnection();auto request=std::make_shared<QByteArray>();
            QObject::connect(socket,&QTcpSocket::readyRead,socket,[&,socket,request]{
                request->append(socket->readAll());if(!request->contains("\r\n\r\n")||socket->property("answered").toBool())return;socket->setProperty("answered",true);
                const bool latest=request->startsWith("GET /latest ");if(latest)++metadataRequests;else ++assetRequests;
                const bool unavailable=latest&&mode=="offline";
                const QByteArray body=latest?metadata():mode=="corrupt"?QByteArray(600000,'b'):payload;
                socket->write(QByteArray("HTTP/1.1 ")+(unavailable?"503 Unavailable":"200 OK")+"\r\nContent-Length: "+QByteArray::number(body.size())+"\r\nConnection: close\r\n\r\n");
                if(latest){socket->write(body);socket->disconnectFromHost();}
                else{
                    auto offset=std::make_shared<int>(0);auto* timer=new QTimer(socket);timer->setInterval(40);
                    QObject::connect(timer,&QTimer::timeout,socket,[socket,body,offset,timer]{
                        const auto chunk=body.mid(*offset,60000);*offset+=chunk.size();socket->write(chunk);
                        if(*offset>=body.size()){timer->stop();socket->disconnectFromHost();}
                    });timer->start();
                }
            });QObject::connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
        }
    });
    const auto wait=[](int ms){QEventLoop loop;QTimer::singleShot(ms,&loop,&QEventLoop::quit);loop.exec();};
    const QUrl origin("http://127.0.0.1:"+QString::number(server.serverPort()));
    for(const QString scenario:{"current","offline","available","corrupt","missing-hash","launch-failed"}){
        mode=scenario;StartupUpdateNetwork network(origin);int ready=0,installed=0;bool verified=false,offlineSeen=false,resetSeen=false,downloadSeen=false,compactWindow=true,smooth=true;
        const QString target=output+"/startup-update-"+scenario+".exe";QFile::remove(target);
        if(scenario=="corrupt"){QFile old(target);if(old.open(QIODevice::WriteOnly))old.write("previous verified file");}
        StartupUpdateWindow window(1,target,[&]{++ready;},[&](const QString& path){
            ++installed;QFile file(path);verified=file.open(QIODevice::ReadOnly)&&QCryptographicHash::hash(file.readAll(),QCryptographicHash::Sha256).toHex()==digest;
            return scenario!="launch-failed";
        },&network);
        window.show();window.start();qreal last=window.progress();QElapsedTimer clock;clock.start();
        while(clock.elapsed()<7000&&!ready&&window.phase()!=Phase::Retry&&!(installed&&scenario!="launch-failed")){
            compactWindow&=window.width()<=400&&window.height()<=232;const auto progress=window.progress();smooth&=qAbs(progress-last)<.22;last=progress;
            if(window.phase()==Phase::Offline&&!offlineSeen){offlineSeen=true;window.grab().save(output+"/startup-update-offline.png");}
            if(window.phase()==Phase::Resetting)resetSeen=true;
            if(window.phase()==Phase::Downloading){downloadSeen=true;if(!QFileInfo::exists(output+"/startup-update-downloading.png"))window.grab().save(output+"/startup-update-downloading.png");}
            wait(16);
        }
        checks[scenario+"_stays_compact"]=compactWindow;
        if(scenario=="current"){
            checks["current_version_opens_without_installing"]=ready==1&&installed==0&&window.phase()==Phase::Complete;
            const int requests=metadataRequests;wait(180);checks["checking_stops_after_startup"]=metadataRequests==requests;
        }else if(scenario=="offline")checks["unreachable_server_shows_plain_message_then_opens"]=offlineSeen&&ready==1&&installed==0;
        else if(scenario=="available"){
            checks["download_resets_and_smoothly_tracks_progress"]=resetSeen&&downloadSeen&&smooth;
            checks["only_verified_download_reaches_installer"]=verified&&installed==1&&ready==0&&window.phase()==Phase::Restarting&&qAbs(window.progress()-1)<.001;
            window.grab().save(output+"/startup-update-restarting.png");
        }else{
            checks[scenario+"_does_not_open_the_old_version"]=ready==0&&window.phase()==Phase::Retry;
            checks[scenario+"_does_not_execute_bad_payload"]=scenario=="launch-failed"?installed==1&&verified:installed==0;
            if(scenario=="corrupt"){
                QFile old(target);checks["failed_download_preserves_previous_file"]=old.open(QIODevice::ReadOnly)&&old.readAll()=="previous verified file";old.close();
                mode="offline";window.start();QElapsedTimer retry;retry.start();
                while(retry.elapsed()<3500&&window.phase()!=Phase::Retry&&!ready)wait(16);
                checks["retry_cannot_bypass_a_confirmed_required_update"]=ready==0&&window.phase()==Phase::Retry;
                mode="available";for(auto* button:window.findChildren<QPushButton*>())if(button->text()==uiText(UiText::Retry,1))button->click();
                retry.restart();while(retry.elapsed()<5000&&!installed&&!ready)wait(16);
                checks["retry_can_complete_the_verified_update"]=installed==1&&verified&&ready==0;
            }
        }
        window.close();QFile::remove(target);
    }
    if(handoff){mode="current";StartupUpdateNetwork network(origin);const auto result=handoff(&network);for(auto it=result.begin();it!=result.end();++it)checks[it.key()]=it.value();}
    bool passed=true;for(const auto& check:checks)passed&=check.toBool();
    return {{"passed",passed},{"checks",checks},{"metadataRequests",metadataRequests},{"assetRequests",assetRequests}};
}
