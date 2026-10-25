#include "main_window.hpp"
#include "startup.hpp"
#include "version.hpp"
#include "app_paths.hpp"

int main(int argc,char** argv){
    QApplication app(argc,argv);QApplication::setApplicationName("Azec");QApplication::setOrganizationName("Azec");QCoreApplication::setApplicationVersion(AZEC_VERSION);QApplication::setStyle("Fusion");app.setWindowIcon(QIcon(":/ui/app-icon.svg"));
    // The system UI face, exactly as the reference renders on Windows.
    QFont font("Segoe UI");font.setPixelSize(13);font.setWeight(QFont::Normal);
    font.setHintingPreference(QFont::PreferNoHinting);
    font.setStyleStrategy(QFont::StyleStrategy(QFont::PreferAntialias|QFont::NoSubpixelAntialias));app.setFont(font);
    if(!azec_paths::initializeStorage()){
        QMessageBox::critical(nullptr,"Azec","Azec could not open its settings folder. Check the folder permissions and try again.");
        return 3;
    }
    const auto configPath=std::filesystem::current_path()/L"r6_config.bin";
    kbtheme::initialize(uiprefs::lightTheme(configPath));
    app.setPalette(kbtheme::palette());
    const auto arguments=app.arguments();const bool selftest=arguments.contains("--ui-self-test"),dryrun=selftest||arguments.contains("--dry-run");QString output,phoneInfo;int duration=0;for(const auto& argument:arguments){if(argument.startsWith("--phone-info="))phoneInfo=argument.mid(13);if(argument.startsWith("--output="))output=argument.mid(9);if(argument.startsWith("--exit-after-ms="))duration=argument.mid(16).toInt();}if(output.isEmpty())output=QDir::currentPath();QDir().mkpath(output);
#ifndef AZEC_UI_TESTS
    if(selftest){
        const auto verifier=QCoreApplication::applicationDirPath()+"/Azec-checks.exe";
        if(!QFile::exists(verifier)){diagnostic("verification","Build with build.cmd test to create the separate verification executable");return 2;}
        return QProcess::execute(verifier,arguments.mid(1));
    }
#endif
    int testResult=0,result=0;
    {
    SingleInstance instance;const auto singleton=instance.acquire();
    if(singleton==SingleInstance::Result::AlreadyRunning)return 0;
    if(singleton==SingleInstance::Result::Unavailable){
        r6::Config probe;probe.load(configPath);
        if(!dryrun)QMessageBox::warning(nullptr,QStringLiteral("Azec"),uiText(UiText::StartupLockFailure,probe.language));
        return 3;
    }
    r6::Engine engine(dryrun);const auto recovery=loadConfiguration(engine,configPath);DesktopTranslator translator([&]{return engine.snapshot().language;});app.installTranslator(&translator);engine.start();MainWindow window(engine,configPath);window.setStartupNotice(recovery.notice,recovery.blockSaving);instance.onActivate(&window);window.show();if(!phoneInfo.isEmpty())window.writePhoneInfo(phoneInfo);
    const auto captureFolder=azec_paths::argument("--capture-ui=");
    if(dryrun&&!selftest&&!captureFolder.isEmpty())QTimer::singleShot(900,&window,[&window,captureFolder]{window.captureDocumentation(captureFolder);});
#ifdef AZEC_UI_TESTS
    if(selftest)QTimer::singleShot(700,&window,[&]{const auto report=window.selfTest(output);QSaveFile file(output+"/ui-verification.json");const auto bytes=QJsonDocument(report).toJson();const bool written=file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size()&&file.commit();if(!written)diagnostic("verification report",file.errorString());testResult=written&&report["passed"].toBool()?0:2;QTimer::singleShot(150,&window,[&]{window.finishVerification();});});else
#endif
    if(duration>0)QTimer::singleShot(duration,&window,[&]{window.closeForTimeout();});
    result=app.exec();engine.stop();
    }
    return selftest?testResult:result;
}
