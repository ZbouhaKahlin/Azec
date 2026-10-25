// Reuse the actual launcher helpers without running its entry point or any UI.
#define wWinMain unusedLauncherEntry
#include "../src/launcher.cpp"
#undef wWinMain
#include <QCoreApplication>
#include "../src/engine.hpp"
#include "../src/profile_store.hpp"
#include "../src/game_focus.hpp"
#include "../src/lan_address.hpp"
#include "../src/driver_setup.hpp"
#include <iostream>

int main(int argc,char** argv){
    QCoreApplication app(argc,argv);
    if(argc!=3||QString::fromLocal8Bit(argv[2])!="--dry-run")return 2;
    const auto fixture=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath();
    if(!QDir(fixture).exists()||!QDir(fixture).isEmpty()||!QDir::setCurrent(fixture))return 2;
    int checks=0,failures=0;
    const auto check=[&](bool pass,const char* label){++checks;if(!pass)++failures;std::cout<<(pass?"PASS ":"FAIL ")<<label<<std::endl;};
    const auto write=[](const QString& path,const QByteArray& bytes){
        if(!QDir().mkpath(QFileInfo(path).absolutePath()))return false;
        QFile file(path);return file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size();
    };
    const auto read=[](const QString& path){QFile file(path);return file.open(QIODevice::ReadOnly)?file.readAll():QByteArray();};
    const auto lock=[](const QString& path){return CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);};
    check(r6::isGameProcessName("C:\\Games\\RainbowSix.exe")&&r6::isGameProcessName("RainbowSix_DX12.exe")&&
          r6::isGameProcessName("RainbowSix_Vulkan.exe")&&!r6::isGameProcessName("chrome.exe")&&
          r6::isGameProcessName("RainbowSix_browser.exe")&&!r6::isGameProcessName("RainbowSix_BE.exe"),
          "legacy game executable prefix matching excludes the BattlEye launcher");
    const lan::Adapter vpn{QHostAddress("10.1.0.2"),false,true,false,1};
    const lan::Adapter physical{QHostAddress("192.168.8.2"),true,false,false,100};
    check(lan::selectAddress({vpn,physical})=="192.168.8.2"&&lan::selectAddress({physical,vpn})=="192.168.8.2",
          "physical LAN without gateway outranks VPN in either order");
    check(lan::selectAddress({{QHostAddress("169.254.1.2"),true,true,false,1}}).isEmpty(),
          "link-local adapters are not advertised");

    const auto verifiedPath=fixture+QString::fromUtf8("/friend's files \xc3\xa9 $value/verified.bin");
    const QByteArray verifiedBytes("verified file contents");
    const auto verifiedHash=QCryptographicHash::hash(verifiedBytes,QCryptographicHash::Sha256);
    check(write(verifiedPath,verifiedBytes),"create file-verification fixture with a non-ASCII path");
    {
        driver_setup::VerifiedFile file;
        check(file.open(verifiedPath,verifiedHash),
              "resolved Windows drive path accepts a file with the expected checksum");
        const HANDLE replacement=CreateFileW(reinterpret_cast<LPCWSTR>(verifiedPath.utf16()),GENERIC_WRITE,
            FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        check(replacement==INVALID_HANDLE_VALUE,"verified file stays protected against replacement");
        if(replacement!=INVALID_HANDLE_VALUE)CloseHandle(replacement);
    }
    {
        driver_setup::VerifiedFile file;
        check(!file.open(verifiedPath,QByteArray(32,'\0')),"resolved file still rejects a wrong checksum");
    }

    const auto source=fixture+"/manual/library/x64/interception.dll";
    check(write(source,"MZ-first")&&!driver_setup::place(source)&&!QFileInfo::exists(driver_setup::dllTarget()),
          "unverified driver is rejected before persistent storage is created");
    check(write(driver_setup::dllTarget(),"retained driver")&&write(source,"MZ-other")&&
          !driver_setup::place(source)&&read(driver_setup::dllTarget())=="retained driver",
          "unverified replacement preserves existing driver contents");
    const auto dll=read(driver_setup::dllTarget());
    check(!driver_setup::place(fixture+"/missing.dll")&&read(driver_setup::dllTarget())==dll,
          "missing source preserves existing driver DLL");

    wchar_t systemPath[32768]{};GetSystemDirectoryW(systemPath,DWORD(std::size(systemPath)));
    const QDir system(QString::fromWCharArray(systemPath));
    const auto special=fixture+"/friend's files $value";
    const auto archive=special+"/driver's archive.zip";
    check(write(special+"/input/payload.txt","verified archive contents"),"create special-character extraction fixture");
    QProcess pack;pack.setProgram(system.filePath("tar.exe"));
    pack.setArguments({"-a","-cf",archive,"-C",special+"/input","payload.txt"});
    check(driver_setup::runExtractor(pack),"create ZIP with the explicit Windows tar executable");
    check(driver_setup::extract(archive,special+"/tar output")&&read(special+"/tar output/payload.txt")=="verified archive contents",
          "normal extraction accepts apostrophes, spaces, and dollar signs");
    QDir().mkpath(special+"/powershell output");
    check(driver_setup::extractWithPowerShell(system,archive,special+"/powershell output")&&
          read(special+"/powershell output/payload.txt")=="verified archive contents",
          "actual PowerShell fallback preserves special-character paths");
    check(!driver_setup::extractWithPowerShell(system,special+"/missing.zip",special+"/powershell output"),
          "PowerShell extraction failure returns failure");

    const auto path=std::filesystem::path(fixture.toStdWString())/L"r6_config.bin";
    r6::Config library;
    library.profile_sets.push_back(profilesets::capture(library,"saved","Saved profiles"));
    library.active_profile_set="saved";
    check(!profilesets::read(QJsonObject{{"profileSets","invalid"}},library)&&library.profile_sets.size()==1&&library.active_profile_set=="saved",
          "rejected profile library leaves the existing library and selection intact");
    check(!profilesets::read(QJsonObject{{"profileSets",profilesets::json(library)},{"activeProfileSet","missing"}},library)&&library.active_profile_set=="saved",
          "backup validation rejects a stale active set without changing the destination");
    check(profilesets::read(QJsonObject{{"profileSets",profilesets::json(library)},{"activeProfileSet","missing"}},library,true)&&
          library.profile_sets.size()==1&&library.profile_sets.front().id=="saved"&&library.active_profile_set.empty(),
          "stale active set preserves valid library entries without activating another set");
    const auto binary=QString::fromStdWString(path.wstring()),profiles=weaponFile(path),ui=fixture+"/azec_ui.json";
    r6::Engine engine(true);
    const QByteArray originalUi="{\"version\":1,\"favorites\":[\"Ace\"],\"sidebarCollapsed\":true}";
    check(engine.save(path)&&write(ui,originalUi),"create original configuration files");
    const auto oldBinary=read(binary),oldProfiles=read(profiles);
    engine.edit([](r6::Config& config){config.operators[0].vertical=37;config.secondary[0].vertical=29;config.secondary_initialized[0]=true;});
    const QByteArray importedUi="{\"version\":1,\"favorites\":[\"Ash\"],\"colorTheme\":\"rose\",\"sidebarCollapsed\":true}";
    HANDLE held=lock(ui);bool saved=engine.save(path,importedUi);if(held!=INVALID_HANDLE_VALUE)CloseHandle(held);
    check(held!=INVALID_HANDLE_VALUE&&!saved&&read(binary)==oldBinary&&read(profiles)==oldProfiles&&read(ui)==originalUi,
          "failed UI commit rolls back both profile files and retains preferences");
    held=lock(profiles);saved=engine.save(path,importedUi);if(held!=INVALID_HANDLE_VALUE)CloseHandle(held);
    check(held!=INVALID_HANDLE_VALUE&&!saved&&read(binary)==oldBinary&&read(profiles)==oldProfiles&&read(ui)==originalUi,
          "failed sidecar commit rolls back binary and retains imported preferences on disk");
    held=lock(binary);saved=engine.save(path,importedUi);if(held!=INVALID_HANDLE_VALUE)CloseHandle(held);
    check(held!=INVALID_HANDLE_VALUE&&!saved&&read(binary)==oldBinary&&read(profiles)==oldProfiles&&read(ui)==originalUi,
          "failed primary write leaves all saved files unchanged");
    r6::Engine restored(true);
    check(engine.save(path,importedUi)&&read(ui)==importedUi&&restored.load(path)&&
          restored.snapshot().operators[0].vertical==37&&restored.snapshot().secondary[0].vertical==29,
          "successful import transaction persists UI and independent profiles together");

    const auto savedBinary=read(binary),savedProfiles=read(profiles);
    engine.edit([](r6::Config& config){config.operators[0].vertical=49;config.secondary[0].vertical=51;});
    held=lock(binary);saved=persistHotkeys(path,VK_HOME,VK_F7,VK_END,'3','4');if(held!=INVALID_HANDLE_VALUE)CloseHandle(held);
    check(held!=INVALID_HANDLE_VALUE&&!saved&&read(binary)==savedBinary&&read(profiles)==savedProfiles&&read(ui)==importedUi,
          "locked binary rejects shortcut save without changing saved data");
    held=lock(profiles);saved=persistHotkeys(path,VK_HOME,VK_F7,VK_END,'3','4');if(held!=INVALID_HANDLE_VALUE)CloseHandle(held);
    check(held!=INVALID_HANDLE_VALUE&&!saved&&read(binary)==savedBinary&&read(profiles)==savedProfiles&&read(ui)==importedUi,
          "locked sidecar rolls back shortcut binary write and preserves preferences");
    check(persistHotkeys(path,VK_HOME,VK_F7,VK_END,'3','4')&&restored.load(path)&&
          restored.snapshot().menu_key==VK_HOME&&restored.snapshot().recoil_key==VK_F7&&
          restored.snapshot().primary_key=='3'&&restored.snapshot().secondary_key=='4'&&
          restored.snapshot().operators[0].vertical==37&&restored.snapshot().secondary[0].vertical==29&&
          engine.snapshot().operators[0].vertical==49&&engine.snapshot().secondary[0].vertical==51&&read(ui)==importedUi,
          "shortcut save persists only bindings while both pending weapon edits remain in memory");
    const auto hotkeyBinary=read(binary);
    check(write(profiles,"broken sidecar")&&!persistHotkeys(path,VK_INSERT,VK_F6,VK_END,'1','2')&&
          read(binary)==hotkeyBinary&&read(profiles)=="broken sidecar"&&write(profiles,savedProfiles),
          "shortcut save rejects damaged sidecar without overwriting it or the binary");

    const auto root=std::filesystem::path(fixture.toStdWString())/L"installed";
    const auto payload=root/L"payload";
    const wchar_t* required[]={L"Azec.exe",L"Qt6Core.dll",L"Qt6Gui.dll",L"Qt6Widgets.dll",L"Qt6Network.dll",L"Qt6Svg.dll",L"platforms/qwindows.dll",L"tls/qschannelbackend.dll",L"vcruntime140.dll",L"vcruntime140_1.dll",L"msvcp140.dll",L"msvcp140_1.dll",L"msvcp140_2.dll"};
    for(const auto* name:required)write(QString::fromStdWString((payload/L"runtime"/name).wstring()),"fixture");
    fs::create_directories(payload/L"licenses");
    check(completePayload(payload),"complete runtime passes installer checklist");
    for(const auto* name:{L"msvcp140_1.dll",L"msvcp140_2.dll"}){
        fs::remove(payload/L"runtime"/name);check(!completePayload(payload),"missing auxiliary MSVC DLL triggers self-repair");
        write(QString::fromStdWString((payload/L"runtime"/name).wstring()),"fixture");
    }
    const Layout layout{root/L"Programs",root/L"Data",root/L"StartMenu"/L"Azec.lnk"};
    const std::string future="9999.1.1.1",old="1.0.0.0";
    write(QString::fromStdWString((layout.program/L"versions"/wide(old)/L"runtime"/L"interception.dll").wstring()),"retained DLL");
    check(!migrateDriver(layout,old,root/L"portable")&&!fs::exists(layout.data/L"interception"/L"interception.dll"),
          "installer refuses to migrate an unverified DLL from a versioned runtime");
    write(QString::fromStdWString((layout.data/L"interception"/L"interception.dll").wstring()),"persistent fixture");
    fs::create_directories(layout.program/L"versions"/wide(future));
    pruneVersions(layout,"");
    check(fs::exists(layout.program/L"versions"/wide(future))&&!fs::exists(layout.program/L"versions"/wide(old))&&
          read(QString::fromStdWString((layout.data/L"interception"/L"interception.dll").wstring()))=="persistent fixture",
          "cleanup preserves newer runtimes and the persistent driver");
    std::cout<<checks<<" review regression checks, "<<failures<<" failures"<<std::endl;
    return failures?1:0;
}
