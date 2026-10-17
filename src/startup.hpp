#pragma once
#include <QtWidgets>
#include <QtNetwork>
#include "engine.hpp"
#include "profile_store.hpp"
#include "diagnostics.hpp"
#include "strings.hpp"

class SingleInstance final : public QObject {
    QLockFile lock_{QDir::current().filePath("azec.lock")};
    QLocalServer server_;
    QString name_="azec-"+QString::fromLatin1(QCryptographicHash::hash(QDir::current().canonicalPath().toCaseFolded().toUtf8(),QCryptographicHash::Sha256).toHex().left(32));
public:
    bool acquire(){
        lock_.setStaleLockTime(0);
        if(!lock_.tryLock(0)){
            if(lock_.error()!=QLockFile::LockFailedError){diagnostic("startup lock","Unable to lock configuration directory");return false;}
            QLocalSocket socket;socket.connectToServer(name_);
            if(socket.waitForConnected(800)){socket.write("show\n");socket.waitForBytesWritten(500);}
            return false;
        }
        QLocalServer::removeServer(name_);
        if(!server_.listen(name_))diagnostic("startup activation",server_.errorString());
        return true;
    }
    void onActivate(QWidget* window){
        connect(&server_,&QLocalServer::newConnection,window,[this,window]{
            while(server_.hasPendingConnections()){
                auto* socket=server_.nextPendingConnection();
                window->showNormal();window->raise();window->activateWindow();
                socket->disconnectFromServer();socket->deleteLater();
            }
        });
    }
};

struct ConfigurationRecovery {QString notice;bool blockSaving=false;};
inline ConfigurationRecovery loadConfiguration(r6::Engine& engine,const std::filesystem::path& path){
    if(engine.load(path))return {};
    engine.edit([&](r6::Config& config){config.language=1;readWeaponProfiles(config,path);});
    const QString filename=QString::fromStdWString(path.wstring());QFile file(filename);
    if(!file.exists())return {};
    if(!file.open(QIODevice::ReadOnly)){
        diagnostic("load binary",file.errorString());
        return {uiText(UiText::ReadSettingsFailure,engine.snapshot().language),true};
    }
    file.close();
    const QString backup=QFileInfo(filename).dir().filePath("r6_config.corrupt-"+QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmss-zzz")+"-"+QUuid::createUuid().toString(QUuid::Id128).left(8)+".bin");
    if(!QFile::rename(filename,backup)){
        diagnostic("recover binary","Unable to preserve rejected configuration; saving blocked");
        return {uiText(UiText::PreserveSettingsFailure,engine.snapshot().language),true};
    }
    diagnostic("recover binary","Rejected configuration preserved as "+QFileInfo(backup).fileName());
    return {uiText(UiText::RecoveredSettings,engine.snapshot().language).arg(QFileInfo(backup).fileName()),false};
}
