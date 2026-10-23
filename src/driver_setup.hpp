#pragma once
#include <QtCore>

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
    const auto base=local.isEmpty()?QDir::tempPath():local+QStringLiteral("/Azec");
    return base+QStringLiteral("/interception");
}
inline QString dllTarget(){
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("interception.dll"));
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

// Places a noob would leave an extracted download.
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
    if(QFileInfo::exists(target)&&QFileInfo(target).size()==QFileInfo(source).size())return true;
    if(QFileInfo::exists(target)&&!QFile::remove(target)){
        if(error)*error=QStringLiteral("could not replace ")+target;
        return false;
    }
    if(!QFile::copy(source,target)){
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

inline bool extract(const QString& archive,const QString& destination,QString* error=nullptr){
    if(!QDir().mkpath(destination)){
        if(error)*error=QStringLiteral("could not create ")+destination;
        return false;
    }
    QProcess tar;
    tar.setProgram(QStringLiteral("tar"));
    tar.setArguments({QStringLiteral("-xf"),archive,QStringLiteral("-C"),destination});
    tar.start();
    if(tar.waitForStarted(5000)&&tar.waitForFinished(120000)&&tar.exitCode()==0)return true;
    QProcess powershell;
    powershell.setProgram(QStringLiteral("powershell"));
    powershell.setArguments({QStringLiteral("-NoProfile"),QStringLiteral("-NonInteractive"),QStringLiteral("-Command"),
        QStringLiteral("Expand-Archive -LiteralPath '")+archive+QStringLiteral("' -DestinationPath '")+destination+QStringLiteral("' -Force")});
    powershell.start();
    if(powershell.waitForStarted(5000)&&powershell.waitForFinished(120000)&&powershell.exitCode()==0)return true;
    if(error)*error=QStringLiteral("extraction failed");
    return false;
}
} // namespace driver_setup
