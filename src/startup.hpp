#pragma once
#include <QtWidgets>
#include <QtNetwork>
#include "engine.hpp"
#include "profile_store.hpp"
#include "diagnostics.hpp"
#include "strings.hpp"
#include <windows.h>

class SingleInstance final : public QObject {
public:
    // Acquired owns the instance, AlreadyRunning defers to a live copy, and
    // Unavailable means the lock cannot be created or reclaimed at all.
    enum class Result { Acquired, AlreadyRunning, Unavailable };
private:
    QLockFile lock_{QDir::current().filePath("azec.lock")};
    QLocalServer server_;
    QString name_="azec-"+QString::fromLatin1(QCryptographicHash::hash(QDir::current().canonicalPath().toCaseFolded().toUtf8(),QCryptographicHash::Sha256).toHex().left(32));
    static bool processRunning(qint64 pid){
        if(pid<=0)return false;
        const HANDLE handle=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,static_cast<DWORD>(pid));
        if(!handle)return GetLastError()==ERROR_ACCESS_DENIED;
        DWORD exit_code=0;const bool running=GetExitCodeProcess(handle,&exit_code)&&exit_code==STILL_ACTIVE;CloseHandle(handle);return running;
    }
    // QLockFile only grows stale while the process it records stays alive, so a
    // truncated or orphaned lock file would otherwise stall startup forever.
    bool ownerIsGone() const {
        qint64 pid=0;QString hostname,appname;
        if(!lock_.getLockInfo(&pid,&hostname,&appname))return true;
        if(!hostname.isEmpty()&&hostname!=QSysInfo::machineHostName())return false;
        return !processRunning(pid);
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
            if(!ownerIsGone())return Result::AlreadyRunning;
            diagnostic("startup lock","Reclaiming a lock file whose owner is gone");
            if(!lock_.removeStaleLockFile()||!lock_.tryLock(0)){diagnostic("startup lock","Unable to reclaim the lock file");return Result::Unavailable;}
        }
        listen();
        return Result::Acquired;
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
    bool focusOnly=true,adsOnly=false;
    engine.edit([&](r6::Config& config){config.language=1;readWeaponProfiles(config,focusOnly,adsOnly,path);});
    engine.game_focus_only=focusOnly;engine.ads_only=adsOnly;
    // The sidecar also carries the output method, so a recovery start honours it.
    engine.output_method=engine.snapshot().output_method?1:0;
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
