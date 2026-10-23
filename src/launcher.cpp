#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shobjidl.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cwctype>
#include "version.hpp"

// Embedded builds install per user. Unembedded development/portable launchers
// still use runtime/ and profiles beside the launcher.
namespace {
namespace fs=std::filesystem;
const wchar_t* profiles[]={L"r6_config.bin",L"azec_weapons.json",L"azec_ui.json"};
void fail(const std::wstring& message){MessageBoxW(nullptr,message.c_str(),L"Azec",MB_OK|MB_ICONERROR|MB_SETFOREGROUND);}
std::wstring wide(const std::string& text){return {text.begin(),text.end()};}
fs::path modulePath(){wchar_t path[32768]{};if(!GetModuleFileNameW(nullptr,path,32768))throw std::runtime_error("executable path");return path;}
fs::path knownFolder(REFKNOWNFOLDERID id){
    PWSTR value=nullptr;if(FAILED(SHGetKnownFolderPath(id,0,nullptr,&value)))throw std::runtime_error("Windows application folder");
    fs::path path(value);CoTaskMemFree(value);return path;
}
std::wstring lower(std::wstring value){std::transform(value.begin(),value.end(),value.begin(),[](wchar_t c){return wchar_t(towlower(c));});return value;}
bool samePath(const fs::path& a,const fs::path& b){return lower(fs::absolute(a).lexically_normal().wstring())==lower(fs::absolute(b).lexically_normal().wstring());}
std::wstring quote(const std::wstring& value){
    std::wstring result=L"\"";size_t slashes=0;
    for(auto c:value){if(c==L'\\'){++slashes;continue;}result.append(slashes*(c==L'"'?2:1),L'\\');slashes=0;if(c==L'"')result+=L'\\';result+=c;}
    result.append(slashes*2,L'\\');return result+L'"';
}
std::wstring commandArguments(const std::vector<std::wstring>& args){std::wstring result;for(const auto& arg:args){if(!result.empty())result+=L' ';result+=quote(arg);}return result;}
bool startProcess(const fs::path& executable,const std::vector<std::wstring>& args,const fs::path& working){
    auto command=quote(executable.wstring())+L" "+commandArguments(args);
    STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION process{};
    if(!CreateProcessW(executable.c_str(),command.data(),nullptr,nullptr,FALSE,0,nullptr,working.c_str(),&startup,&process))return false;
    CloseHandle(process.hThread);CloseHandle(process.hProcess);return true;
}
std::string readText(const fs::path& path){std::ifstream file(path,std::ios::binary);std::string value(512,'\0');file.read(value.data(),value.size());value.resize(size_t(file.gcount()));return value;}
void writeText(const fs::path& path,const std::string& text){
    std::ofstream file(path,std::ios::binary|std::ios::trunc);file.write(text.data(),text.size());file.close();if(!file)throw std::runtime_error("write installation marker");
}
bool processRunning(DWORD pid){
    if(!pid)return false;HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);
    if(!process)return GetLastError()==ERROR_ACCESS_DENIED;
    DWORD code=0;const bool running=GetExitCodeProcess(process,&code)&&code==STILL_ACTIVE;CloseHandle(process);return running;
}
bool activeProfile(const fs::path& folder){
    const auto text=readText(folder/L"azec.lock");if(text.empty())return false;
    try{return processRunning(std::stoul(text));}catch(...){return false;}
}
void waitForApp(DWORD pid){
    if(!pid)return;HANDLE process=OpenProcess(SYNCHRONIZE,FALSE,pid);
    if(!process){if(GetLastError()==ERROR_INVALID_PARAMETER)return;throw std::runtime_error("wait for Azec");}
    const auto result=WaitForSingleObject(process,60000);CloseHandle(process);
    if(result!=WAIT_OBJECT_0)throw std::runtime_error("Azec is still running");
}
struct Layout {fs::path program,data,shortcut;};
Layout installedLayout(bool dryRun){
    // Redirect the ENTIRE installation only for isolated dry-run checks.
    wchar_t testRoot[32768]{};
    if(dryRun&&GetEnvironmentVariableW(L"AZEC_INSTALL_TEST_ROOT",testRoot,32768)){
        const fs::path root=fs::absolute(testRoot);
        return {root/L"Programs"/L"Azec",root/L"Data"/L"Azec",root/L"StartMenu"/L"Azec.lnk"};
    }
    return {knownFolder(FOLDERID_UserProgramFiles)/L"Azec",knownFolder(FOLDERID_LocalAppData)/L"Azec",knownFolder(FOLDERID_Programs)/L"Azec.lnk"};
}
class InstallLock {
    HANDLE handle_=nullptr;
public:
    explicit InstallLock(const fs::path& root){
        unsigned long long hash=14695981039346656037ull;for(auto c:lower(root.wstring())){hash^=c;hash*=1099511628211ull;}
        const auto name=L"Local\\Azec-install-"+std::to_wstring(hash);handle_=CreateMutexW(nullptr,FALSE,name.c_str());
        if(!handle_)throw std::runtime_error("installation lock");
        const auto result=WaitForSingleObject(handle_,60000);
        if(result!=WAIT_OBJECT_0&&result!=WAIT_ABANDONED){CloseHandle(handle_);handle_=nullptr;throw std::runtime_error("installation busy");}
    }
    ~InstallLock(){if(handle_){ReleaseMutex(handle_);CloseHandle(handle_);}}
};
bool newer(const std::string& left,const std::string& right){
    size_t a=0,b=0;
    while(a<left.size()||b<right.size()){
        unsigned long long x=0,y=0;
        while(a<left.size()&&left[a]!='.')x=x*10+unsigned(left[a++]-'0');
        while(b<right.size()&&right[b]!='.')y=y*10+unsigned(right[b++]-'0');
        if(x!=y)return x>y;if(a<left.size())++a;if(b<right.size())++b;
    }
    return false;
}
bool validVersion(const std::string& text){return !text.empty()&&text.size()<64&&std::all_of(text.begin(),text.end(),[](char c){return (c>='0'&&c<='9')||c=='.';});}
bool completePayload(const fs::path& folder){
    for(const auto* name:{L"runtime/Azec.exe",L"runtime/Qt6Core.dll",L"runtime/Qt6Gui.dll",L"runtime/Qt6Widgets.dll",L"runtime/Qt6Network.dll",L"runtime/Qt6Svg.dll",L"runtime/platforms/qwindows.dll",L"runtime/tls/qschannelbackend.dll",L"runtime/vcruntime140.dll",L"runtime/vcruntime140_1.dll",L"runtime/msvcp140.dll",L"runtime/msvcp140_1.dll",L"runtime/msvcp140_2.dll"})
        if(!fs::is_regular_file(folder/name))return false;
    return fs::is_directory(folder/L"licenses");
}
bool removeOptionalFile(const fs::path& path){
    std::error_code error;fs::remove(path,error);
    if(error)OutputDebugStringW(L"Azec: optional temporary-file cleanup skipped.\n");
    return !error;
}
void extractPayload(const fs::path& folder){
    fs::create_directories(folder);const auto archive=folder/L"azec-payload.zip";
    const HRSRC resource=FindResourceW(nullptr,L"AZEC_PAYLOAD",RT_RCDATA);
    const HGLOBAL loaded=resource?LoadResource(nullptr,resource):nullptr;
    const void* data=loaded?LockResource(loaded):nullptr;const DWORD size=resource?SizeofResource(nullptr,resource):0;
    if(!data||!size)throw std::runtime_error("embedded payload");
    {std::ofstream file(archive,std::ios::binary);file.write(static_cast<const char*>(data),size);file.close();if(!file)throw std::runtime_error("write payload");}
    wchar_t system[32768]{};GetSystemDirectoryW(system,32768);const auto tar=fs::path(system)/L"tar.exe";
    auto command=quote(tar.wstring())+L" -xf "+quote(archive.wstring())+L" -C "+quote(folder.wstring());
    STARTUPINFOW startup{};startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESHOWWINDOW;startup.wShowWindow=SW_HIDE;PROCESS_INFORMATION process{};
    if(!CreateProcessW(tar.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,folder.c_str(),&startup,&process))throw std::runtime_error("unpack payload");
    const auto result=WaitForSingleObject(process.hProcess,180000);DWORD code=1;
    if(result==WAIT_OBJECT_0)GetExitCodeProcess(process.hProcess,&code);else{TerminateProcess(process.hProcess,1);WaitForSingleObject(process.hProcess,5000);}
    CloseHandle(process.hThread);CloseHandle(process.hProcess);removeOptionalFile(archive);
    if(code||!completePayload(folder))throw std::runtime_error("incomplete payload");
    writeText(folder/L"runtime"/L"version.txt",AZEC_VERSION);
}
void preparePayload(const fs::path& program){
    const auto target=program/L"versions"/wide(AZEC_VERSION);
    if(readText(target/L"runtime"/L"version.txt")==AZEC_VERSION&&completePayload(target))return;
    const auto stage=program/L"versions"/(wide(AZEC_VERSION)+L".staging-"+std::to_wstring(GetCurrentProcessId()));
    try{
        extractPayload(stage);
        if(fs::exists(target))fs::rename(target,target.wstring()+L".incomplete-"+std::to_wstring(GetCurrentProcessId()));
        fs::rename(stage,target);
    }catch(...){std::error_code ignored;fs::remove_all(stage,ignored);throw;}
}
void migrateProfiles(const fs::path& source,const fs::path& destination){
    fs::create_directories(destination);
    if(fs::exists(destination/L"storage-initialized.txt"))return;
    // Never mix profiles from two installations or overwrite an existing set.
    bool existing=false,found=false;
    for(const auto* name:profiles){existing|=fs::exists(destination/name);found|=fs::exists(source/name);}
    if(!existing&&found&&!samePath(source,destination)){
        if(activeProfile(source))throw std::runtime_error("Close the portable copy before importing its profiles");
        std::vector<fs::path> copied;
        try{
            for(const auto* name:profiles)if(fs::exists(source/name)){fs::copy_file(source/name,destination/name);copied.push_back(destination/name);}
            writeText(destination/L"storage-initialized.txt","Azec profiles imported; originals preserved.\n");
        }catch(...){for(const auto& path:copied){std::error_code ignored;fs::remove(path,ignored);}throw;}
    }else writeText(destination/L"storage-initialized.txt","Azec user settings.\n");
}
bool migrateDriver(const Layout& layout,const std::string& installed,const fs::path& portable){
    try{
        const auto target=layout.data/L"interception"/L"interception.dll";
        if(fs::exists(target))return true;
        auto source=validVersion(installed)?layout.program/L"versions"/wide(installed)/L"runtime"/L"interception.dll":fs::path();
        if(!fs::is_regular_file(source))source=portable/L"runtime"/L"interception.dll";
        if(!fs::is_regular_file(source))source=portable/L"interception.dll";
        if(!fs::is_regular_file(source))return true;
        fs::create_directories(target.parent_path());
        fs::copy_file(source,target);
        return true;
    }catch(const fs::filesystem_error& error){
        OutputDebugStringW(L"Azec: optional driver migration skipped.\n");
        OutputDebugStringA(error.what());return false;
    }
}
void createShortcut(const Layout& layout){
    fs::create_directories(layout.shortcut.parent_path());IShellLinkW* link=nullptr;
    if(FAILED(CoCreateInstance(CLSID_ShellLink,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&link))))throw std::runtime_error("Start menu shortcut");
    const auto launcher=layout.program/L"Azec.exe";
    link->SetPath(launcher.c_str());link->SetWorkingDirectory(layout.data.c_str());link->SetDescription(L"Azec");link->SetIconLocation(launcher.c_str(),0);
    IPersistFile* file=nullptr;HRESULT result=link->QueryInterface(IID_PPV_ARGS(&file));
    if(SUCCEEDED(result)){result=file->Save(layout.shortcut.c_str(),TRUE);file->Release();}link->Release();
    if(FAILED(result))throw std::runtime_error("save Start menu shortcut");
}
void replaceLauncher(const fs::path& source,const fs::path& target){
    if(samePath(source,target))return;
    const fs::path pending=target.wstring()+L".installing";
    if(!CopyFileW(source.c_str(),pending.c_str(),FALSE))throw std::runtime_error("stage launcher");
    for(int attempt=0;attempt<40;++attempt){
        if(MoveFileExW(pending.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return;
        Sleep(100);
    }
    throw std::runtime_error("replace launcher");
}
void pruneVersions(const Layout& layout,const std::string& previous){
    if(activeProfile(layout.data))return;
    for(const auto& entry:fs::directory_iterator(layout.program/L"versions")){
        const auto name=entry.path().filename().string();if(!validVersion(name)||name==AZEC_VERSION||name==previous||newer(name,AZEC_VERSION))continue;
        std::error_code ignored;fs::remove_all(entry.path(),ignored);
    }
}
}

int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int){
    try{
        int argc=0;LPWSTR* argv=CommandLineToArgvW(GetCommandLineW(),&argc);if(!argv)throw std::runtime_error("arguments");
        std::vector<std::wstring> args;for(int i=1;i<argc;++i)args.emplace_back(argv[i]);LocalFree(argv);
        const bool dryRun=std::find(args.begin(),args.end(),L"--dry-run")!=args.end();
        const auto self=modulePath();fs::path migrationSource=self.parent_path(),legacyTarget;
        const auto update=std::find(args.begin(),args.end(),L"--apply-update");
        if(update!=args.end()){
            const auto offset=size_t(update-args.begin());if(offset+2>=args.size())throw std::runtime_error("update arguments");
            legacyTarget=args[offset+1];const auto pid=std::stoul(args[offset+2]);migrationSource=legacyTarget.parent_path();
            args.erase(args.begin()+offset,args.begin()+offset+3);waitForApp(pid);
        }
        if(!FindResourceW(nullptr,L"AZEC_PAYLOAD",RT_RCDATA)){
            const auto app=self.parent_path()/L"runtime"/L"Azec.exe";
            if(!fs::exists(app)||!startProcess(app,args,self.parent_path()))throw std::runtime_error("Keep the portable runtime folder beside Azec.exe");
            return 0;
        }
        const auto layout=installedLayout(dryRun);const auto launcher=layout.program/L"Azec.exe";
        {
            InstallLock lock(layout.program);fs::create_directories(layout.program);
            const auto installed=readText(layout.program/L"version.txt");
            if(validVersion(installed)&&newer(installed,AZEC_VERSION)){
                if(samePath(self,launcher)||!fs::exists(launcher))
                    throw std::runtime_error("A newer Azec installation exists. Download the latest Azec.exe to repair it; older copies cannot replace it.");
                if(!startProcess(launcher,args,layout.program))throw std::runtime_error("start installed Azec");return 0;
            }
            migrateDriver(layout,installed,migrationSource);
            preparePayload(layout.program);migrateProfiles(migrationSource,layout.data);replaceLauncher(self,launcher);
            if(installed!=AZEC_VERSION&&validVersion(installed))writeText(layout.program/L"previous-version.txt",installed);
            writeText(layout.program/L"version.txt",AZEC_VERSION);
            const auto com=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);if(FAILED(com))throw std::runtime_error("Windows shortcuts");
            try{createShortcut(layout);}catch(...){CoUninitialize();throw;}CoUninitialize();
            // An old portable updater targets its original launcher. Replace it
            // too, so its next launch redirects to the installed application.
            if(!legacyTarget.empty()&&!samePath(legacyTarget,launcher))replaceLauncher(self,legacyTarget);
            pruneVersions(layout,readText(layout.program/L"previous-version.txt"));
        }
        args.push_back(L"--data-dir="+layout.data.wstring());args.push_back(L"--azec-launcher="+launcher.wstring());
        const auto application=layout.program/L"versions"/wide(AZEC_VERSION)/L"runtime"/L"Azec.exe";
        if(!startProcess(application,args,layout.data))throw std::runtime_error("start Azec");
        return 0;
    }catch(const std::exception& error){
        fail(L"Azec could not finish starting.\n\n"+wide(error.what())+L"\n\nYour saved profiles have not been replaced. Close Azec and try again.");return 1;
    }
}
