#include "main_window.hpp"
#include "startup.hpp"

int main(int argc,char** argv){
    QApplication app(argc,argv);QApplication::setApplicationName("Azec");QApplication::setOrganizationName("Azec");QApplication::setStyle("Fusion");app.setWindowIcon(QIcon(":/ui/app-icon.svg"));
    for(const auto* weight:{"Regular","Medium","SemiBold","Bold"})QFontDatabase::addApplicationFont(QString(":/fonts/Geist-%1.ttf").arg(weight));
    QFontDatabase::addApplicationFont(":/fonts/Fredoka.ttf");
    QFontDatabase::addApplicationFont(":/fonts/PressStart2P-Regular.ttf");
    QFont font("Geist");font.setPixelSize(13);font.setWeight(QFont::Normal);font.setStyleStrategy(QFont::PreferAntialias);app.setFont(font);
    const auto configPath=std::filesystem::current_path()/L"r6_config.bin";presentation::active=presentation::read(configPath);
    for(const auto& argument:app.arguments())if(argument.startsWith("--interface="))presentation::active=presentation::fromName(argument.mid(12));
    kbtheme::initialize(kbtheme::readColorTheme(configPath));operatorRowHeight=presentation::isRally()?102:52;
    app.setPalette(kbtheme::palette());
    const auto arguments=app.arguments();const bool selftest=arguments.contains("--ui-self-test"),dryrun=selftest||arguments.contains("--dry-run");QString output,phoneInfo;int duration=0;for(const auto& argument:arguments){if(argument.startsWith("--phone-info="))phoneInfo=argument.mid(13);if(argument.startsWith("--output="))output=argument.mid(9);if(argument.startsWith("--exit-after-ms="))duration=argument.mid(16).toInt();}if(output.isEmpty())output=QDir::currentPath();QDir().mkpath(output);
#ifndef AZEC_UI_TESTS
    if(selftest){
        const auto verifier=QCoreApplication::applicationDirPath()+"/Azec-checks.exe";
        if(!QFile::exists(verifier)){diagnostic("verification","Build with build.cmd test to create the separate verification executable");return 2;}
        return QProcess::execute(verifier,arguments.mid(1));
    }
#endif
    int testResult=0,result=0;bool restart=false;
    {
    SingleInstance instance;if(!instance.acquire())return 0;
    r6::Engine engine(dryrun);const auto recovery=loadConfiguration(engine,configPath);DesktopTranslator translator([&]{return engine.snapshot().language;});app.installTranslator(&translator);engine.start();MainWindow window(engine,configPath);window.setStartupNotice(recovery.notice,recovery.blockSaving);instance.onActivate(&window);window.show();if(!phoneInfo.isEmpty())window.writePhoneInfo(phoneInfo);
#ifdef AZEC_UI_TESTS
    for(const auto& argument:arguments)if(argument.startsWith("--restart-target="))QTimer::singleShot(250,&window,[&,target=argument.mid(17)]{window.verifyRestartTo(target);});
    if(selftest)QTimer::singleShot(700,&window,[&]{const auto report=window.selfTest(output);QFile file(output+"/ui-verification.json");file.open(QIODevice::WriteOnly);file.write(QJsonDocument(report).toJson());testResult=report["passed"].toBool()?0:2;QTimer::singleShot(150,&window,[&]{window.finishVerification();});});else
#endif
    if(duration>0)QTimer::singleShot(duration,&window,[&]{window.closeForTimeout();});
    result=app.exec();engine.stop();restart=window.restartRequested();
    } // Release the window, engine, phone server and instance lock before relaunching.
    if(restart){QStringList forwarded;for(const auto& argument:arguments.mid(1))if(!argument.startsWith("--interface=")&&!argument.startsWith("--restart-target=")&&argument!="--ui-self-test")forwarded<<argument;
        if(dryrun&&!forwarded.contains("--dry-run"))forwarded<<"--dry-run";
        if(!QProcess::startDetached(QCoreApplication::applicationFilePath(),forwarded,QDir::currentPath())){diagnostic("restart","Unable to relaunch Azec");return 1;}}
    return selftest?testResult:result;
}
