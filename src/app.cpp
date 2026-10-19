#include "main_window.hpp"
#include "startup.hpp"
#include "version.hpp"
#include "app_paths.hpp"

int main(int argc,char** argv){
    QApplication app(argc,argv);QApplication::setApplicationName("Azec");QApplication::setOrganizationName("Azec");QCoreApplication::setApplicationVersion(AZEC_VERSION);QApplication::setStyle("Fusion");app.setWindowIcon(QIcon(":/ui/app-icon.svg"));
    for(const auto* weight:{"Regular","Medium","SemiBold","Bold"})QFontDatabase::addApplicationFont(QString(":/fonts/Geist-%1.ttf").arg(weight));
    QFontDatabase::addApplicationFont(":/fonts/Fredoka.ttf");
    QFontDatabase::addApplicationFont(":/fonts/PressStart2P-Regular.ttf");
    QFont font("Geist");font.setPixelSize(13);font.setWeight(QFont::Normal);font.setStyleStrategy(QFont::PreferAntialias);app.setFont(font);
    if(!azec_paths::initializeStorage()){
        QMessageBox::critical(nullptr,"Azec","Azec could not open its settings folder. Check the folder permissions and try again.");
        return 3;
    }
    const auto configPath=std::filesystem::current_path()/L"r6_config.bin";
    kbtheme::initialize(kbtheme::readColorTheme(configPath));
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
#ifdef AZEC_UI_TESTS
    if(selftest)QTimer::singleShot(700,&window,[&]{const auto report=window.selfTest(output);QFile file(output+"/ui-verification.json");file.open(QIODevice::WriteOnly);file.write(QJsonDocument(report).toJson());testResult=report["passed"].toBool()?0:2;QTimer::singleShot(150,&window,[&]{window.finishVerification();});});else
#endif
    if(duration>0)QTimer::singleShot(duration,&window,[&]{window.closeForTimeout();});
    result=app.exec();engine.stop();
    }
    return selftest?testResult:result;
}
