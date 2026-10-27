// Exercise launcher helpers without starting the installer, app or input workers.
#define wWinMain unusedLauncherEntry
#include "../src/launcher.cpp"
#undef wWinMain
#include "../src/profile_transfer.hpp"
#include "../src/profile_store.hpp"
#include "../src/ui_preferences.hpp"
#include "../src/driver_input.hpp"
#include "../src/driver_download.hpp"
#include "../src/engine.hpp"
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
    const auto path=std::filesystem::path(fixture.toStdWString())/L"r6_config.bin";

    r6::Config live,backupConfig;
    backupConfig.operators[0].vertical=17;
    for(int id=71;id<78;++id){
        live.operators[id].vertical=float(id-50);live.operators[id].rapid=id%2;
        live.secondary[id].vertical=float(id-30);live.secondary[id].rapid=(id+1)%2;
        live.secondary_initialized[id]=true;live.secondary_enabled[id]=id%2;
    }
    auto backup=makeBackup(backupConfig,{},true,false);
    auto records=backup["operators"].toArray();QJsonArray oldRecords;
    for(int id=70;id>=0;--id)oldRecords.append(records[id]);
    auto oldBackup=backup;oldBackup["operators"]=oldRecords;
    r6::Config imported;QSet<QString> favorites;BackupOptions options;
    bool parsed=parseBackup(oldBackup,imported,favorites,options,&live),preserved=parsed;
    for(int id=71;id<78;++id)preserved&=imported.operators[id].vertical==live.operators[id].vertical&&
        imported.secondary[id].vertical==live.secondary[id].vertical&&imported.operators[id].rapid==live.operators[id].rapid&&
        imported.secondary[id].rapid==live.secondary[id].rapid&&imported.secondary_enabled[id]==live.secondary_enabled[id]&&imported.secondary_initialized[id];
    check(preserved&&imported.operators[0].vertical==17,"old shuffled backup replaces named profiles and preserves both missing weapon profiles");
    auto legacySettings=oldBackup["settings"].toObject();legacySettings.remove("perProfileRapid");legacySettings["rapidActive"]=false;oldBackup["settings"]=legacySettings;
    check(parseBackup(oldBackup,imported,favorites,options,&live)&&imported.operators[71].rapid&&imported.secondary[72].rapid,
          "legacy rapid migration does not overwrite omitted operators");
    r6::Engine engine(true),restored(true);engine.edit([&](r6::Config& config){config=imported;});
    check(engine.save(path)&&restored.load(path)&&restored.snapshot().operators[77].vertical==live.operators[77].vertical&&
          restored.snapshot().secondary[77].vertical==live.secondary[77].vertical&&QFileInfo(QString::fromStdWString(path.wstring())).size()==3734,
          "preserved profiles survive save and reload without extending the binary");
    check(parseBackup(backup,imported,favorites,options,&live)&&imported.operators[71].vertical==10&&!imported.secondary_initialized[71],
          "full backups still replace operators they contain");
    const auto before=imported.bytes();auto invalid=backup;invalid["version"]=99;
    check(!parseBackup(invalid,imported,favorites,options,&live)&&imported.bytes()==before,"invalid import leaves the destination unchanged");

    {
        auto library=live;library.active_profile_set="saved";
        library.profile_sets.push_back(profilesets::capture(library,"saved","Saved profiles"));
        auto legacy=backup;legacy.remove("profileSets");legacy.remove("activeProfileSet");
        check(parseBackup(legacy,imported,favorites,options,&library)&&imported.active_profile_set=="saved"&&imported.profile_sets.size()==1,
              "backup without a library preserves the current active library identity");
        const auto libraryPath=path.parent_path()/L"library"/L"r6_config.bin";
        QDir().mkpath(QString::fromStdWString(libraryPath.parent_path().wstring()));
        imported.operators[0].vertical=29;imported.secondary[0].vertical=39;imported.secondary_initialized[0]=true;
        engine.edit([&](r6::Config& config){config=imported;});
        const bool roundtrip=engine.save(libraryPath)&&restored.load(libraryPath);const auto saved=restored.snapshot();
        check(roundtrip&&saved.active_profile_set=="saved"&&saved.profile_sets.size()==1&&saved.profile_sets[0].primary[0].vertical==29&&saved.profile_sets[0].secondary[0].vertical==39,
              "edits after an older import survive in both weapons of the active library set");
        auto stale=makeBackup(library,{},true,false);stale["activeProfileSet"]="missing";
        check(parseBackup(stale,imported,favorites,options,&library)&&imported.active_profile_set.empty()&&imported.profile_sets.size()==1&&imported.profile_sets[0].id=="saved",
              "backup with a stale library reference preserves profiles and repairs selection");
        stale["profileSets"]="invalid";const auto retained=imported.bytes();
        check(!parseBackup(stale,imported,favorites,options,&library)&&imported.bytes()==retained&&imported.profile_sets.size()==1,
              "malformed backup libraries remain rejected without modifying imported profiles");
    }
    {
        const auto rapidPath=path.parent_path()/L"rapid-recovery"/L"r6_config.bin";
        QDir().mkpath(QString::fromStdWString(rapidPath.parent_path().wstring()));
        r6::Config rapid;rapid.operators[0].rapid=1;rapid.operators[4].rapid=1;
        const auto preservesRapid=[&]{const auto config=restored.snapshot();return config.operators[0].rapid&&config.operators[4].rapid&&!config.operators[1].rapid;};
        check(rapid.save(rapidPath)&&restored.load(rapidPath)&&preservesRapid(),"missing sidecar preserves binary rapid flags");
        const auto sidecar=weaponFile(rapidPath);
        check(write(sidecar,"{broken")&&restored.load(rapidPath)&&preservesRapid(),"damaged sidecar preserves binary rapid flags");
        check(QFile::remove(sidecar)&&persistSelection(rapidPath,4)&&restored.load(rapidPath)&&preservesRapid()&&restored.snapshot().selected==4,
              "selection-only sidecar preserves binary rapid flags");
        check(QFile::remove(sidecar)&&persistHotkeys(rapidPath,VK_HOME,VK_F7,VK_END,'3','4')&&restored.load(rapidPath)&&preservesRapid(),
              "shortcut-only sidecar preserves binary rapid flags");
        check(restored.save(rapidPath)&&restored.load(rapidPath)&&preservesRapid(),"recovered rapid flags survive a normal save and restart");
        check(write(sidecar,"{\"version\":1,\"rapidActive\":false}")&&restored.load(rapidPath)&&!restored.snapshot().operators[0].rapid,
              "explicit legacy rapid-off still migrates");
        check(write(sidecar,"{\"version\":1,\"rapidActive\":true}")&&restored.load(rapidPath)&&restored.snapshot().operators[1].rapid&&restored.snapshot().secondary[1].rapid,
              "explicit legacy rapid-on still migrates to both weapons");
    }

    const auto ui=uiprefs::filePath(path);QJsonObject preferences;
    check(write(ui,{})&&uiprefs::readDocument(ui,preferences)&&preferences.isEmpty()&&
          uiprefs::update(path,[](QJsonObject& root){root["colorTheme"]="rose";root["favorites"]=QJsonArray{"Ace"};}),
          "empty UI preferences recover on the next atomic save");
    check(uiprefs::update(path,[](QJsonObject& root){root["theme"]="light";})&&
          uiprefs::readDocument(ui,preferences)&&!preferences.contains("colorTheme")&&preferences["favorites"].toArray().contains("Ace")&&preferences["theme"]=="light",
          "recovered preferences retain other settings and drop the obsolete theme key");
    const QByteArray malformed="{broken preferences";
    check(write(ui,malformed)&&!uiprefs::update(path,[](QJsonObject& root){root["colorTheme"]="original";})&&read(ui)==malformed,
          "nonempty malformed preferences remain protected");

    const auto root=std::filesystem::path(fixture.toStdWString())/L"installer";
    const Layout layout{root/L"Programs",root/L"Data",root/L"StartMenu"/L"Azec.lnk"};
    const auto portable=root/L"portable";
    check(write(QString::fromStdWString((portable/L"interception.dll").wstring()),"fixture DLL")&&!migrateDriver(layout,"",portable),
          "optional driver migration rejects an unverified DLL");
    check(!fs::exists(layout.data/L"interception"/L"interception.dll"),
          "rejected driver migration leaves no persistent DLL");
    const Layout blocked{root/L"BlockedPrograms",root/L"blocked-data",root/L"BlockedStartMenu"};
    check(write(QString::fromStdWString(blocked.data.wstring()),"existing file")&&!migrateDriver(blocked,"",portable)&&
          read(QString::fromStdWString(blocked.data.wstring()))=="existing file"&&
          read(QString::fromStdWString((portable/L"interception.dll").wstring()))=="fixture DLL",
          "rejected optional migration leaves existing storage and the source unchanged");
    const auto archive=root/L"azec-payload.zip";const auto archiveName=QString::fromStdWString(archive.wstring());
    check(write(archiveName,"temporary archive"),"create temporary archive cleanup fixture");
    const HANDLE held=CreateFileW(archive.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    const bool retained=held!=INVALID_HANDLE_VALUE&&!removeOptionalFile(archive)&&fs::exists(archive);
    if(held!=INVALID_HANDLE_VALUE)CloseHandle(held);
    check(retained,"locked optional archive is retained without throwing");
    check(removeOptionalFile(archive)&&!fs::exists(archive)&&removeOptionalFile(archive),"optional archive cleanup succeeds when unlocked or already absent");

    {
        physical_mouse::Buttons physical;
        physical.event(1,RI_MOUSE_LEFT_BUTTON_DOWN,0);const auto pressed=physical.snapshot();
        physical.event(1,RI_MOUSE_LEFT_BUTTON_UP,physical_mouse::generated_marker);
        physical.event(1,RI_MOUSE_LEFT_BUTTON_DOWN,physical_mouse::generated_marker);
        check(pressed.down&&pressed.presses==1&&physical.snapshot().down&&physical.snapshot().presses==1,
              "generated clicks neither release nor retrigger the physical hold");
        physical.event(1,RI_MOUSE_LEFT_BUTTON_UP,0);physical.event(2,RI_MOUSE_LEFT_BUTTON_DOWN,0);physical.remove(2);
        check(!physical.snapshot().down&&physical.snapshot().presses==2,"physical release and mouse removal end the hold");
        physical.event(1,RI_MOUSE_LEFT_BUTTON_DOWN|RI_MOUSE_LEFT_BUTTON_UP,0);
        check(!physical.snapshot().down&&physical.snapshot().presses==3,"a complete quick physical tap retains one press edge");
        physical.event(1,RI_MOUSE_LEFT_BUTTON_DOWN,0);const auto serial=physical.snapshot().presses;physical.releaseAll();
        check(!physical.snapshot().down&&physical.snapshot().presses==serial,"focus/gate resets release the physical latch without inventing a press");
        r6::RapidSequence doubleClick;
        doubleClick.step(0,false,0,true,r6::RapidMode::DoubleClick,0,50);
        const int first=doubleClick.step(1,true,1,true,r6::RapidMode::DoubleClick,0,50);
        const int second=doubleClick.step(6,false,1,true,r6::RapidMode::DoubleClick,0,50);
        const int release=doubleClick.step(31,false,1,true,r6::RapidMode::DoubleClick,0,50);
        check(first==0&&second==1&&release==0&&doubleClick.step(100,true,1,true,r6::RapidMode::DoubleClick,0,50)==-1,
              "double click adds exactly one complete click per physical press and never repeats from a held button");
        check(doubleClick.step(101,true,2,true,r6::RapidMode::DoubleClick,0,50)==0&&
              doubleClick.step(106,false,2,true,r6::RapidMode::DoubleClick,0,50)==1,
              "the next physical press starts a fresh double click");
        check(doubleClick.step(107,false,2,false,r6::RapidMode::DoubleClick,0,50)==0&&
              doubleClick.step(1000,false,2,false,r6::RapidMode::DoubleClick,0,50)==-1,
              "permission loss cancels a pending double click and releases its generated down");
        r6::RapidSequence hold;hold.step(0,false,0,true,r6::RapidMode::Hold,0,50);
        check(hold.step(1,true,1,true,r6::RapidMode::Hold,0,50)==0&&
              hold.step(2,false,1,true,r6::RapidMode::Hold,0,50)==-1&&
              hold.step(100,false,1,true,r6::RapidMode::Hold,0,50)==-1,
              "Hold release cancels the next down before the pulse begins");
        check(hold.step(101,true,2,true,r6::RapidMode::Hold,0,50)==0&&hold.step(106,true,2,true,r6::RapidMode::Hold,0,50)==1&&
              hold.step(107,false,2,true,r6::RapidMode::Hold,0,50)==0&&hold.step(1000,false,2,true,r6::RapidMode::Hold,0,50)==-1,
              "Hold release ends an active pulse immediately without queued repeat clicks");
        check(hold.step(1001,true,3,true,r6::RapidMode::Hold,0,50)==0&&hold.step(1006,true,3,true,r6::RapidMode::Hold,0,50)==1&&
              hold.step(1031,true,3,true,r6::RapidMode::Hold,0,50)==0&&hold.step(1080,true,3,true,r6::RapidMode::Hold,0,50)==-1&&
              hold.step(1081,true,3,true,r6::RapidMode::Hold,0,50)==0,
              "Hold continues repeating after earlier taps and respects the configured shot delay");
        check(hold.step(1082,false,3,true,r6::RapidMode::Hold,0,50)==-1&&
              hold.step(2000,false,3,true,r6::RapidMode::Hold,0,50)==-1,
              "releasing during a repeat delay cancels the scheduled pulse");
        r6::Config modeConfig;const auto binary=modeConfig.bytes();modeConfig.rapid_mode=r6::RapidMode::Hold;
        check(modeConfig.bytes()==binary&&binary.size()==3734,"rapid mode does not alter the compatible binary layout");
        const auto modePath=path.parent_path()/L"mode"/L"r6_config.bin";QDir().mkpath(QString::fromStdWString(modePath.parent_path().wstring()));
        r6::Engine modes(true),loadedModes(true);modes.edit([](r6::Config& c){c.rapid_mode=r6::RapidMode::Hold;c.secondary[0].rapid=true;c.secondary_initialized[0]=true;});
        check(modes.save(modePath)&&loadedModes.load(modePath)&&loadedModes.snapshot().rapid_mode==r6::RapidMode::Hold&&loadedModes.snapshot().secondary[0].rapid,
              "saved mode and independent secondary rapid fire survive restart");
        auto modeBackup=makeBackup(modes.snapshot(),{},true,false);r6::Config modeImported;QSet<QString> modeFavorites;BackupOptions modeOptions;
        check(parseBackup(modeBackup,modeImported,modeFavorites,modeOptions)&&modeImported.rapid_mode==r6::RapidMode::Hold,
              "backup export and import retain the selected rapid mode");
        auto settings=modeBackup["settings"].toObject();settings.remove("rapidMode");modeBackup["settings"]=settings;
        check(parseBackup(modeBackup,modeImported,modeFavorites,modeOptions)&&modeImported.rapid_mode==r6::RapidMode::DoubleClick,
              "older backups default to Double click");
        settings["rapidMode"]=2;modeBackup["settings"]=settings;
        check(!parseBackup(modeBackup,modeImported,modeFavorites,modeOptions),"unknown backup modes are rejected before applying profiles");
        const auto redacted=errorReport("TEST-REPORT","Example",QDir::homePath()+"/private https://example.test/?token=secret token=secret code=123456");
        check(!redacted.contains("secret")&&!redacted.contains("123456")&&redacted.contains(AZEC_VERSION),"copied error reports redact paths, links and credential-like fields");
    }

    // Hold one extra module reference so retry ownership mistakes are observable.
    const auto stub=QCoreApplication::applicationDirPath()+"/Azec-driver-fixture.dll";
    const HMODULE heldModule=LoadLibraryW(reinterpret_cast<LPCWSTR>(stub.utf16()));
    if(!heldModule){check(false,"load test-only driver fixture");return 1;}
    const auto mode=reinterpret_cast<void(*)(int)>(GetProcAddress(heldModule,"fixture_mode"));
    const auto creates=reinterpret_cast<int(*)()>(GetProcAddress(heldModule,"fixture_creates"));
    const auto destroys=reinterpret_cast<int(*)()>(GetProcAddress(heldModule,"fixture_destroys"));
    const auto lastDevice=reinterpret_cast<int(*)()>(GetProcAddress(heldModule,"fixture_last_device"));
    const auto sends=reinterpret_cast<int(*)()>(GetProcAddress(heldModule,"fixture_sends"));
    const auto mouseRange=reinterpret_cast<int(*)(int)>(GetProcAddress(heldModule,"interception_is_mouse"));
    driver_input::detail::Backend backend;backend.module=LoadLibraryW(reinterpret_cast<LPCWSTR>(stub.utf16()));
    check(!backend.ensure()&&creates()==1,"driver fixture initially fails context creation");
    check(backend.failure_code=="DRV-CONTEXT-ACCESS"&&backend.failure_detail.contains("context_win32=5"),"context report captures access denial before diagnostic calls");
    mode(1);check(!backend.ensure()&&creates()==1,"failed driver probe stays cached until explicitly retried");
    backend.retryInitialization();check(backend.ensure()&&creates()==2,"explicit retry opens a newly available driver context");
    check(mouseRange(11)&&backend.device==12,"friend's 12,13,16 mouse layout skips empty slot 11 despite its valid mouse number");
    backend.retryInitialization();check(backend.ensure()&&creates()==2,"retry leaves an already working context untouched");
    backend.destroy(backend.context);backend.context=nullptr;mode(2);backend.retryInitialization();
    check(!backend.ensure()&&backend.refused&&destroys()==2,"refused contexts are released before retry");
    mode(1);backend.retryInitialization();check(!backend.refused&&backend.ensure()&&creates()==4,"retry clears refusal and reopens without reloading the DLL");
    driver_input::MouseStroke movement{};movement.x=40;
    SetLastError(ERROR_INVALID_PARAMETER);
    check(!backend.stroke(movement)&&backend.failure_code=="DRV-SEND-MOVE"&&backend.failure_detail.contains("send_win32=21")&&
          backend.failure_detail.contains("selected_mouse=12")&&backend.failure_detail.contains("hardware_id_slots=12,13,16"),
          "send report captures return value, fresh Windows error and selected populated slot");
    const auto logSize=QFileInfo("azec.log").size();
    check(!backend.stroke(movement)&&QFileInfo("azec.log").size()==logSize,"identical worker failures do not flood the log");
    movement.state=driver_input::left_button_down;
    check(!backend.stroke(movement)&&backend.failure_code=="DRV-SEND-BUTTON","button and movement failures have different codes");
    mode(3);check(backend.stroke(movement)&&lastDevice()==12,"movement and button output use the selected connected mouse");
    movement.state=0;check(backend.stroke(movement)&&lastDevice()==12,"recoil movement uses the same connected mouse as button output");
    backend.destroy(backend.context);backend.context=nullptr;backend.retryInitialization();mode(4);
    const int beforeNoMouse=sends();
    check(!backend.ensure()&&!backend.context&&backend.failure_code=="DRV-MOUSE-NOT-FOUND"&&
          backend.failure_detail.contains("hardware_id_slots=none")&&!backend.stroke(movement)&&sends()==beforeNoMouse,
          "no connected mouse closes the context, reports a distinct failure and never sends to an empty slot");
    mode(5);backend.retryInitialization();
    check(backend.ensure()&&backend.device==20&&backend.stroke(movement)&&lastDevice()==20,
          "explicit retry discovers a connected mouse in the last slot");
    backend.destroy(backend.context);backend.context=nullptr;backend.retryInitialization();mode(6);
    check(backend.ensure()&&backend.device==11&&backend.stroke(movement)&&lastDevice()==11,
          "PCs with a real mouse in slot 11 retain their original device");
    const auto redacted=errorReport("TEST-REPORT","Example",QDir::homePath()+"/private https://example.test/?token=secret token=secret code=123456");
    check(!redacted.contains(QDir::homePath())&&!redacted.contains("secret")&&!redacted.contains("123456")&&redacted.contains(AZEC_VERSION),
          "shareable reports contain the build version and redact home paths, links and credentials");
    backend.destroy(backend.context);FreeLibrary(backend.module);FreeLibrary(heldModule);

    QNetworkAccessManager network;
    struct Result {QByteArray bytes;QString error;int callbacks=0;qint64 elapsed=0;};
    const auto download=[&](const QByteArray& scenario,driver_setup::DownloadLimits limits){
        Result result;QTcpServer server;QEventLoop loop;QElapsedTimer elapsed;elapsed.start();
        if(!server.listen(QHostAddress::LocalHost,0)){result.error="Fixture server failed";return result;}
        QObject::connect(&server,&QTcpServer::newConnection,&server,[&]{
            auto* socket=server.nextPendingConnection();
            QObject::connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
            QObject::connect(socket,&QTcpSocket::readyRead,socket,[socket,scenario]{
                socket->readAll();if(socket->property("replied").toBool())return;socket->setProperty("replied",true);
                if(scenario=="stall"||scenario=="trickle"){
                    socket->write("HTTP/1.1 200 OK\r\nContent-Length: 1000\r\n\r\na");
                    if(scenario=="trickle"){
                        auto* timer=new QTimer(socket);timer->setInterval(20);
                        QObject::connect(timer,&QTimer::timeout,socket,[socket]{socket->write("a");});timer->start();
                    }return;
                }
                if(scenario=="chunked")socket->write("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n1001\r\n"+QByteArray(4097,'a')+"\r\n0\r\n\r\n");
                else{
                    const QByteArray payload(scenario=="oversize"?4097:4096,'a');
                    socket->write(QByteArray("HTTP/1.1 ")+(scenario=="status"?"404 Not Found":"200 OK")+"\r\nContent-Length: "+QByteArray::number(payload.size())+"\r\n\r\n"+payload);
                }
                socket->disconnectFromHost();
            });
        });
        QTimer guard;guard.setSingleShot(true);QObject::connect(&guard,&QTimer::timeout,&loop,&QEventLoop::quit);guard.start(2500);
        driver_setup::downloadArchive(&network,&loop,QUrl("http://127.0.0.1:"+QString::number(server.serverPort())),[&](QByteArray bytes,QString error){
            result.bytes=std::move(bytes);result.error=std::move(error);++result.callbacks;loop.quit();
        },limits);
        loop.exec();result.elapsed=elapsed.elapsed();
        for(auto* socket:server.findChildren<QTcpSocket*>())socket->abort();
        return result;
    };
    driver_setup::DownloadLimits limits;limits.bytes=4096;limits.inactivityMs=500;limits.totalMs=1500;
    auto result=download("valid",limits);check(result.callbacks==1&&result.error.isEmpty()&&result.bytes.size()==4096,"driver download accepts a complete response at the size limit");
    result=download("oversize",limits);check(result.callbacks==1&&result.bytes.isEmpty()&&result.error.contains("size limit"),"driver download rejects oversized Content-Length");
    result=download("chunked",limits);check(result.callbacks==1&&result.bytes.isEmpty()&&result.error.contains("size limit"),"driver download caps streamed bytes without Content-Length");
    result=download("status",limits);check(result.callbacks==1&&result.bytes.isEmpty()&&!result.error.isEmpty(),"driver download rejects HTTP failure responses");
    limits.inactivityMs=60;result=download("stall",limits);check(result.callbacks==1&&result.bytes.isEmpty()&&!result.error.isEmpty()&&result.elapsed<1500,"stalled driver download fails before the total deadline");
    limits.inactivityMs=500;limits.totalMs=140;result=download("trickle",limits);check(result.callbacks==1&&result.bytes.isEmpty()&&result.error.contains("timed out"),"total deadline stops a continuously trickling driver download");
    limits.totalMs=1500;result=download("valid",limits);check(result.callbacks==1&&result.error.isEmpty()&&result.bytes.size()==4096,"a failed download does not prevent a subsequent successful retry");
    std::cout<<checks<<" reliability checks, "<<failures<<" failures"<<std::endl;return failures?1:0;
}
