#pragma once
#include <QtCore>
#include <windows.h>
#include <iterator>
#include <vector>
#include "app_paths.hpp"
#include "driver_trust.hpp"

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
// Hashes of the x64 DLL and installer inside the pinned, verified v1.0.1 ZIP.
// Never learn these values from a local file or an unsigned sidecar.
inline QByteArray dllSha256(){return QByteArray::fromHex(driver_trust::dllSha256Hex);}
inline QByteArray installerSha256(){return QByteArray::fromHex("e137863a79da797f08e7a137280ff2a123809044a888fd75ce9c973198915abe");}
inline bool verifyDll(const QByteArray& data){
    return !data.isEmpty()&&QCryptographicHash::hash(data,QCryptographicHash::Sha256)==dllSha256();
}

// Keep the file immutable from hashing through LoadLibrary. Pin every directory
// in its resolved local path too, so a directory rename or junction swap cannot
// redirect the loader after verification. Failure always leaves output disabled.
class VerifiedFile final {
    HANDLE file_=INVALID_HANDLE_VALUE;
    std::vector<HANDLE> directories_;
    QString path_;
public:
    VerifiedFile()=default;
    VerifiedFile(const VerifiedFile&)=delete;
    VerifiedFile& operator=(const VerifiedFile&)=delete;
    ~VerifiedFile(){
        if(file_!=INVALID_HANDLE_VALUE)CloseHandle(file_);
        for(auto it=directories_.rbegin();it!=directories_.rend();++it)CloseHandle(*it);
    }
    bool open(const QString& source,const QByteArray& expected){
        if(file_!=INVALID_HANDLE_VALUE)return false;
        const auto native=QDir::toNativeSeparators(QFileInfo(source).absoluteFilePath());
        file_=CreateFileW(reinterpret_cast<LPCWSTR>(native.utf16()),GENERIC_READ,FILE_SHARE_READ,
                          nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file_==INVALID_HANDLE_VALUE)return false;
        wchar_t resolved[32768]{};
        const auto length=GetFinalPathNameByHandleW(file_,resolved,DWORD(std::size(resolved)),FILE_NAME_NORMALIZED|VOLUME_NAME_DOS);
        if(!length||length>=std::size(resolved))return false;
        path_=QString::fromWCharArray(resolved,int(length));
        // Qt's native-separator conversion removes the extended-path prefix.
        // Keep it intact for the drive check and directory guards below.
        auto local=path_;
        local.replace(QLatin1Char('\\'),QLatin1Char('/'));
        // Only local drive paths are supported for executable driver files.
        if(!local.startsWith("//?/")||local.size()<7||!local[4].isLetter()||local.mid(5,2)!=":/")return false;
        const auto parent=local.left(qMax(qsizetype(7),local.lastIndexOf('/')));
        for(int end=7;end<=parent.size();){
            const auto directory=QDir::toNativeSeparators(parent.left(end));
            const HANDLE handle=CreateFileW(reinterpret_cast<LPCWSTR>(directory.utf16()),FILE_READ_ATTRIBUTES,
                FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
            if(handle==INVALID_HANDLE_VALUE)return false;
            directories_.push_back(handle);
            BY_HANDLE_FILE_INFORMATION info{};
            if(!GetFileInformationByHandle(handle,&info)||!(info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)||
               (info.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT))return false;
            if(end==parent.size())break;
            const auto next=parent.indexOf('/',end+1);end=next<0?int(parent.size()):int(next);
        }
        // The resolved path must still name the exact open file after the
        // directory guards were acquired, including when a rename raced open().
        const HANDLE same=CreateFileW(reinterpret_cast<LPCWSTR>(path_.utf16()),GENERIC_READ,FILE_SHARE_READ,
                                     nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(same==INVALID_HANDLE_VALUE)return false;
        BY_HANDLE_FILE_INFORMATION original{},reopened{};
        const bool identical=GetFileInformationByHandle(file_,&original)&&GetFileInformationByHandle(same,&reopened)&&
            original.dwVolumeSerialNumber==reopened.dwVolumeSerialNumber&&original.nFileIndexHigh==reopened.nFileIndexHigh&&
            original.nFileIndexLow==reopened.nFileIndexLow;
        CloseHandle(same);
        LARGE_INTEGER size{};
        if(!identical||!GetFileSizeEx(file_,&size)||size.QuadPart<=0||size.QuadPart>16*1024*1024)return false;
        QCryptographicHash hash(QCryptographicHash::Sha256);char buffer[64*1024];DWORD count=0;qint64 read=0;
        do{
            if(!ReadFile(file_,buffer,sizeof(buffer),&count,nullptr))return false;
            hash.addData(QByteArrayView(buffer,count));read+=count;
        }while(count);
        return read==size.QuadPart&&hash.result()==expected;
    }
    const QString& path() const{return path_;}
};

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
    {VerifiedFile file;if(file.open(direct,dllSha256()))return direct;}
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
    if(input.error()!=QFile::NoError||bytes.size()!=input.size()||!verifyDll(bytes)){
        if(error)*error=QStringLiteral("Interception DLL checksum mismatch; use Download and place");
        return false;
    }
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
    VerifiedFile installer;
    return installer.open(directory.filePath(QStringLiteral("command line installer/install-interception.exe")),installerSha256())?folder:QString();
}

inline QString installCommand(const QString& folder){
    if(folder.isEmpty())return {};
    return QLatin1Char('"')+QDir::toNativeSeparators(QDir(folder).filePath(QStringLiteral("install-interception.exe")))+QStringLiteral("\" /install");
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
