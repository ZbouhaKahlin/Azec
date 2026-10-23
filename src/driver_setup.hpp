#pragma once
#include <QtCore>
#include <windows.h>
#include <iterator>
#include "app_paths.hpp"

// One-click Interception setup used by the Settings guide.
//
// The release build is pinned: the download is verified against sha256() before
// it is extracted, so a hijacked URL cannot deliver another payload. Extraction
// uses the Windows tar.exe (bsdtar) with a PowerShell fallback; nothing is
// bundled, linked or installed silently.
namespace driver_setup {
inline QString releasePage(){return QStringLiteral("https://github.com/oblitum/Interception/releases/tag/v1.0.1");}
inline QString downloadUrl(){return QStringLiteral("https://github.com/oblitum/Interception/releases/download/v1.0.1/Interception.zip");}
inline QByteArray sha256(){return QByteArray::fromHex("ad038963d6413055765128b0b931f6e765147c9916dba79e65d872b261f9af10");}
inline bool verify(const QByteArray& data){
    return !data.isEmpty()&&QCryptographicHash::hash(data,QCryptographicHash::Sha256)==sha256();
}

// Everything the helper writes lives under the app's local data folder.
inline QString workRoot(){
    const auto local=qEnvironmentVariable("LOCALAPPDATA");
    const auto base=azec_paths::installed()||QCoreApplication::arguments().contains("--dry-run")
        ?QDir::currentPath():(local.isEmpty()?QDir::tempPath()+QStringLiteral("/Azec"):local+QStringLiteral("/Azec"));
    return base+QStringLiteral("/interception");
}
inline QString dllTarget(){
    return QDir(workRoot()).filePath(QStringLiteral("interception.dll"));
}

// Look for library/x64/interception.dll inside an extracted release folder.
inline QString findDll(const QString& root,int depth=4){
    if(root.isEmpty()||depth<0)return {};
    const QDir dir(root);
    if(!dir.exists())return {};
    const auto direct=dir.filePath(QStringLiteral("library/x64/interception.dll"));
    if(QFileInfo::exists(direct))return direct;
    const auto children=dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
    for(const auto& child:children){
        if(child.startsWith(QLatin1Char('.')))continue;
        const auto found=findDll(dir.filePath(child),depth-1);
        if(!found.isEmpty())return found;
    }
    return {};
}

// Common locations for a manually extracted download.
inline QStringList searchRoots(){
    QStringList roots;
    const auto home=QDir::homePath();
    for(const auto folder:{QStringLiteral("Downloads"),QStringLiteral("Desktop"),QStringLiteral("Documents")})
        roots<<home+QLatin1Char('/')+folder;
    roots<<QDir::tempPath()<<QCoreApplication::applicationDirPath()<<QDir::currentPath()<<workRoot();
    return roots;
}

inline QString locate(){
    for(const auto& root:searchRoots()){
        const auto found=findDll(root);
        if(!found.isEmpty())return found;
    }
    return {};
}

inline bool place(const QString& source,QString* error=nullptr){
    const auto target=dllTarget();
    QFile input(source);
    if(!input.open(QIODevice::ReadOnly)||input.size()<=0||input.size()>16*1024*1024){
        if(error)*error=QStringLiteral("could not read ")+source;
        return false;
    }
    const auto bytes=input.readAll();
    if(input.error()!=QFile::NoError||bytes.size()!=input.size())return false;
    input.close();
    QFile existing(target);
    if(existing.open(QIODevice::ReadOnly)&&existing.size()==bytes.size()&&existing.readAll()==bytes)return true;
    existing.close();
    QSaveFile output(target);
    if(!QDir().mkpath(QFileInfo(target).absolutePath())||!output.open(QIODevice::WriteOnly)||
       output.write(bytes)!=bytes.size()||!output.commit()){
        if(error)*error=QStringLiteral("could not copy to ")+target;
        return false;
    }
    return true;
}

// <root>/Interception/library/x64/interception.dll -> the installer folder.
inline QString installerFolder(const QString& dllFile){
    if(dllFile.isEmpty())return {};
    QDir directory=QFileInfo(dllFile).absoluteDir();
    directory.cdUp();directory.cdUp();
    const auto folder=directory.filePath(QStringLiteral("command line installer"));
    return QFileInfo::exists(directory.filePath(QStringLiteral("command line installer/install-interception.exe")))
        ?folder:QString();
}

inline QString installCommand(const QString& folder){
    if(folder.isEmpty())return QStringLiteral("install-interception.exe /install");
    return QStringLiteral("cd \"")+QDir::toNativeSeparators(folder)+QStringLiteral("\"\ninstall-interception.exe /install");
}

// Shared by the normal tar path and the PowerShell fallback. Neither opens a console.
inline bool runExtractor(QProcess& process){
    process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* args){
        args->flags|=CREATE_NO_WINDOW;
    });
    process.start();
    if(process.waitForStarted(5000)&&process.waitForFinished(120000))
        return process.exitStatus()==QProcess::NormalExit&&process.exitCode()==0;
    if(process.state()!=QProcess::NotRunning){process.kill();process.waitForFinished(5000);}
    return false;
}
inline bool extractWithPowerShell(const QDir& system,const QString& archive,const QString& destination){
    QProcess powershell;
    powershell.setProgram(system.filePath(QStringLiteral("WindowsPowerShell/v1.0/powershell.exe")));
    auto environment=QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("AZEC_DRIVER_ARCHIVE"),QDir::toNativeSeparators(QFileInfo(archive).absoluteFilePath()));
    environment.insert(QStringLiteral("AZEC_DRIVER_DESTINATION"),QDir::toNativeSeparators(QFileInfo(destination).absoluteFilePath()));
    powershell.setProcessEnvironment(environment);
    powershell.setArguments({QStringLiteral("-NoProfile"),QStringLiteral("-NonInteractive"),QStringLiteral("-Command"),
        QStringLiteral("Expand-Archive -LiteralPath $env:AZEC_DRIVER_ARCHIVE -DestinationPath $env:AZEC_DRIVER_DESTINATION -Force -ErrorAction Stop")});
    return runExtractor(powershell);
}
inline bool extract(const QString& archive,const QString& destination,QString* error=nullptr){
    if(!QDir().mkpath(destination)){
        if(error)*error=QStringLiteral("could not create ")+destination;
        return false;
    }
    wchar_t directory[32768]{};
    const auto length=GetSystemDirectoryW(directory,DWORD(std::size(directory)));
    if(!length||length>=std::size(directory)){
        if(error)*error=QStringLiteral("could not locate Windows system tools");
        return false;
    }
    const QDir system(QString::fromWCharArray(directory,int(length)));
    QProcess tar;
    tar.setProgram(system.filePath(QStringLiteral("tar.exe")));
    tar.setArguments({QStringLiteral("-xf"),archive,QStringLiteral("-C"),destination});
    if(runExtractor(tar)||extractWithPowerShell(system,archive,destination))return true;
    if(error)*error=QStringLiteral("extraction failed");
    return false;
}
} // namespace driver_setup
