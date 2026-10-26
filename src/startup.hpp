#pragma once
#include <QtWidgets>
#include <QtNetwork>
#include "engine.hpp"
#include "profile_store.hpp"
#include "diagnostics.hpp"
#include "strings.hpp"
#include "ui_preferences.hpp"
#include <windows.h>

class SingleInstance final : public QObject {
public:
    // Acquired owns the instance, AlreadyRunning defers to a live copy, and
    // Unavailable means the lock cannot be created or reclaimed at all.
    enum class Result { Acquired, AlreadyRunning, Unavailable };
private:
    QLockFile lock_{QDir::current().filePath("azec.lock")};
    QLocalServer server_;
    QMetaObject::Connection activation_;
    QString name_="azec-"+QString::fromLatin1(QCryptographicHash::hash(QDir::current().canonicalPath().toCaseFolded().toUtf8(),QCryptographicHash::Sha256).toHex().left(32));
    static bool processCouldOwnLock(qint64 pid){
        if(pid<=0)return false;
        const HANDLE handle=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,static_cast<DWORD>(pid));
        if(!handle)return GetLastError()==ERROR_ACCESS_DENIED;
        DWORD exit_code=0;bool owner=true;
        if(GetExitCodeProcess(handle,&exit_code)&&exit_code!=STILL_ACTIVE)owner=false;
        else {
            wchar_t image[32768];DWORD length=DWORD(std::size(image));
            if(QueryFullProcessImageNameW(handle,0,image,&length)){
                const auto name=QFileInfo(QString::fromWCharArray(image,int(length))).fileName();
                owner=name.compare("Azec.exe",Qt::CaseInsensitive)==0
                    ||name.compare("Azec-checks.exe",Qt::CaseInsensitive)==0
                    ||name.compare(QFileInfo(QCoreApplication::applicationFilePath()).fileName(),Qt::CaseInsensitive)==0;
            }
        }
        CloseHandle(handle);return owner;
    }
    void listen(){
        QLocalServer::removeServer(name_);
        if(!server_.listen(name_))diagnostic("startup activation",server_.errorString());
    }
public:
    Result acquire(){
        lock_.setStaleLockTime(0);
        if(!lock_.tryLock(0)){
            if(lock_.error()!=QLockFile::LockFailedError){diagnostic("startup lock","Unable to lock configuration directory");return Result::Unavailable;}
            QLocalSocket socket;socket.connectToServer(name_);
            if(socket.waitForConnected(800)){socket.write("show\n");socket.waitForBytesWritten(500);return Result::AlreadyRunning;}
            qint64 pid=0;QString hostname,appname;
            const bool known=lock_.getLockInfo(&pid,&hostname,&appname);
            // A live same-machine owner answers the activation server above.
            // Beyond it, a lock recorded on another computer is live as far as
            // this rule allows knowing; report it instead of exiting silently.
            const bool localMachine=!known||hostname.isEmpty()
                ||hostname.compare(QSysInfo::machineHostName(),Qt::CaseInsensitive)==0
                ||hostname.compare(QHostInfo::localHostName(),Qt::CaseInsensitive)==0;
            if(!localMachine){
                diagnostic("startup lock","Configuration directory is locked by another computer ("+hostname+")");
                return Result::Unavailable;
            }
            // Inspect the running image, not the name recorded in the lock:
            // a recycled PID still carries the old owner's recorded name.
            // If identity cannot be established, preserve the possible owner.
            if(known&&processCouldOwnLock(pid)){
                diagnostic("startup lock","A live configuration owner could not be activated");
                return Result::Unavailable;
            }
            diagnostic("startup lock","Reclaiming a lock file whose owner is gone");
            if(!lock_.removeStaleLockFile()||!lock_.tryLock(0)){diagnostic("startup lock","Unable to reclaim the lock file");return Result::Unavailable;}
        }
        listen();
        return Result::Acquired;
    }
    void onActivate(QWidget* window){
        disconnect(activation_);
        activation_=connect(&server_,&QLocalServer::newConnection,window,[this,window]{
            while(server_.hasPendingConnections()){
                auto* socket=server_.nextPendingConnection();
                window->showNormal();window->raise();window->activateWindow();
                socket->disconnectFromServer();socket->deleteLater();
            }
        });
    }
};

struct ConfigurationRecovery {QString notice;bool blockSaving=false;};
inline ConfigurationRecovery loadPrimaryConfiguration(r6::Engine& engine,const std::filesystem::path& path,bool& weaponProfilesValid){
    if(engine.load(path,&weaponProfilesValid))return {};
    bool focusOnly=true,adsOnly=false;
    engine.edit([&](r6::Config& config){config.language=1;weaponProfilesValid=readWeaponProfiles(config,focusOnly,adsOnly,path);});
    engine.game_focus_only=focusOnly;engine.ads_only=adsOnly;
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
inline ConfigurationRecovery loadConfiguration(r6::Engine& engine,const std::filesystem::path& path){
    bool weaponProfilesValid=true;
    auto recovery=loadPrimaryConfiguration(engine,path,weaponProfilesValid);
    const int language=engine.snapshot().language;
    const auto preserve=[&](const QString& filename){
        QFile file(filename);QString message;
        if(!file.open(QIODevice::ReadOnly)){
            recovery.blockSaving=true;
            message=uiText(UiText::AdditionalSettingsBlocked,language).arg(QFileInfo(filename).fileName());
        }else{
            file.close();const QFileInfo info(filename);
            const auto backup=info.dir().filePath(info.completeBaseName()+".corrupt-"+
                QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmss-zzz")+"-"+
                QUuid::createUuid().toString(QUuid::Id128).left(8)+"."+info.suffix());
            if(QFile::rename(filename,backup)){
                diagnostic("recover settings","Rejected settings preserved as "+QFileInfo(backup).fileName());
                message=uiText(UiText::AdditionalSettingsRecovered,language).arg(QFileInfo(backup).fileName());
            }else{
                recovery.blockSaving=true;
                message=uiText(UiText::AdditionalSettingsBlocked,language).arg(info.fileName());
            }
        }
        if(!recovery.notice.isEmpty())recovery.notice+="\n\n";
        recovery.notice+=message;
    };
    if(!weaponProfilesValid)preserve(weaponFile(path));
    QJsonObject ui;
    if(!uiprefs::readDocument(uiprefs::filePath(path),ui))preserve(uiprefs::filePath(path));
    return recovery;
}
