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
    auto backup=makeBackup(backupConfig,{},true,"original",false);
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

    const auto ui=uiprefs::filePath(path);QJsonObject preferences;
    check(write(ui,{})&&uiprefs::readDocument(ui,preferences)&&preferences.isEmpty()&&
          uiprefs::update(path,[](QJsonObject& root){root["colorTheme"]="rose";root["favorites"]=QJsonArray{"Ace"};}),
          "empty UI preferences recover on the next atomic save");
    check(uiprefs::update(path,[](QJsonObject& root){root["sidebarCollapsed"]=true;})&&
          uiprefs::readDocument(ui,preferences)&&preferences["colorTheme"]=="rose"&&preferences["favorites"].toArray().contains("Ace"),
          "recovered preferences retain other settings on later writes");
    const QByteArray malformed="{broken preferences";
    check(write(ui,malformed)&&!uiprefs::update(path,[](QJsonObject& root){root["colorTheme"]="original";})&&read(ui)==malformed,
          "nonempty malformed preferences remain protected");

    const auto root=std::filesystem::path(fixture.toStdWString())/L"installer";
    const Layout layout{root/L"Programs",root/L"Data",root/L"StartMenu"/L"Azec.lnk"};
    const auto portable=root/L"portable";
    check(write(QString::fromStdWString((portable/L"interception.dll").wstring()),"fixture DLL")&&migrateDriver(layout,"",portable),
          "optional driver migration still succeeds normally");
    check(read(QString::fromStdWString((layout.data/L"interception"/L"interception.dll").wstring()))=="fixture DLL",
          "normal driver migration keeps the original contents");
    const Layout blocked{root/L"BlockedPrograms",root/L"blocked-data",root/L"BlockedStartMenu"};
    check(write(QString::fromStdWString(blocked.data.wstring()),"existing file")&&!migrateDriver(blocked,"",portable)&&
          read(QString::fromStdWString((portable/L"interception.dll").wstring()))=="fixture DLL",
          "driver migration filesystem failure returns without aborting startup or changing the source");
    const auto archive=root/L"azec-payload.zip";const auto archiveName=QString::fromStdWString(archive.wstring());
    check(write(archiveName,"temporary archive"),"create temporary archive cleanup fixture");
    const HANDLE held=CreateFileW(archive.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    const bool retained=held!=INVALID_HANDLE_VALUE&&!removeOptionalFile(archive)&&fs::exists(archive);
    if(held!=INVALID_HANDLE_VALUE)CloseHandle(held);
    check(retained,"locked optional archive is retained without throwing");
    check(removeOptionalFile(archive)&&!fs::exists(archive)&&removeOptionalFile(archive),"optional archive cleanup succeeds when unlocked or already absent");

    // Hold one extra module reference so retry ownership mistakes are observable.
    const auto stub=QCoreApplication::applicationDirPath()+"/Azec-driver-fixture.dll";
    const HMODULE heldModule=LoadLibraryW(reinterpret_cast<LPCWSTR>(stub.utf16()));
    if(!heldModule){check(false,"load test-only driver fixture");return 1;}
    const auto mode=reinterpret_cast<void(*)(int)>(GetProcAddress(heldModule,"fixture_mode"));
    const auto creates=reinterpret_cast<int(*)()>(GetProcAddress(heldModule,"fixture_creates"));
    const auto destroys=reinterpret_cast<int(*)()>(GetProcAddress(heldModule,"fixture_destroys"));
    driver_input::detail::Backend backend;backend.module=LoadLibraryW(reinterpret_cast<LPCWSTR>(stub.utf16()));
    check(!backend.ensure()&&creates()==1,"driver fixture initially fails context creation");
    mode(1);check(!backend.ensure()&&creates()==1,"failed driver probe stays cached until explicitly retried");
    backend.retryInitialization();check(backend.ensure()&&creates()==2,"explicit retry opens a newly available driver context");
    backend.retryInitialization();check(backend.ensure()&&creates()==2,"retry leaves an already working context untouched");
    backend.destroy(backend.context);backend.context=nullptr;mode(2);backend.retryInitialization();
    check(!backend.ensure()&&backend.refused&&destroys()==2,"refused contexts are released before retry");
    mode(1);backend.retryInitialization();check(!backend.refused&&backend.ensure()&&creates()==4,"retry clears refusal and reopens without reloading the DLL");
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
