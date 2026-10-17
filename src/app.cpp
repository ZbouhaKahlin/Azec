#include "main_window.hpp"
#include "startup.hpp"

int main(int argc,char** argv){
    QApplication app(argc,argv);QApplication::setApplicationName("Azec");QApplication::setOrganizationName("Azec");QApplication::setStyle("Fusion");app.setWindowIcon(QIcon(":/ui/app-icon.svg"));
    for(const auto* weight:{"Regular","Medium","SemiBold","Bold"})QFontDatabase::addApplicationFont(QString(":/fonts/Geist-%1.ttf").arg(weight));
    QFontDatabase::addApplicationFont(":/fonts/Fredoka.ttf");
    QFontDatabase::addApplicationFont(":/fonts/PressStart2P-Regular.ttf");
    QFont font("Geist");font.setPixelSize(13);font.setWeight(QFont::Normal);font.setStyleStrategy(QFont::PreferAntialias);app.setFont(font);
    QPalette palette;palette.setColor(QPalette::Window,QColor("#FFFAEE"));palette.setColor(QPalette::WindowText,QColor("#31204C"));palette.setColor(QPalette::Base,QColor("#FFFFFF"));palette.setColor(QPalette::AlternateBase,QColor("#FFFAEE"));palette.setColor(QPalette::Text,QColor("#31204C"));palette.setColor(QPalette::Button,QColor("#FFFFFF"));palette.setColor(QPalette::ButtonText,QColor("#31204C"));palette.setColor(QPalette::Highlight,QColor("#7248D7"));palette.setColor(QPalette::HighlightedText,QColor("#FFFFFF"));palette.setColor(QPalette::PlaceholderText,QColor("#8A7EA0"));palette.setColor(QPalette::Disabled,QPalette::Text,QColor("#A79BB8"));app.setPalette(palette);
    const auto arguments=app.arguments();const bool selftest=arguments.contains("--ui-self-test"),dryrun=selftest||arguments.contains("--dry-run");QString output,phoneInfo;int duration=0;for(const auto& argument:arguments){if(argument.startsWith("--phone-info="))phoneInfo=argument.mid(13);if(argument.startsWith("--output="))output=argument.mid(9);if(argument.startsWith("--exit-after-ms="))duration=argument.mid(16).toInt();}if(output.isEmpty())output=QDir::currentPath();QDir().mkpath(output);
#ifndef AZEC_UI_TESTS
    if(selftest){
        const auto verifier=QCoreApplication::applicationDirPath()+"/Azec-checks.exe";
        if(!QFile::exists(verifier)){diagnostic("verification","Build with build.cmd test to create the separate verification executable");return 2;}
        return QProcess::execute(verifier,arguments.mid(1));
    }
#endif
    SingleInstance instance;if(!instance.acquire())return 0;
    r6::Engine engine(dryrun);const auto configPath=std::filesystem::current_path()/L"r6_config.bin";const auto recovery=loadConfiguration(engine,configPath);DesktopTranslator translator([&]{return engine.snapshot().language;});app.installTranslator(&translator);engine.start();MainWindow window(engine,configPath);window.setStartupNotice(recovery.notice,recovery.blockSaving);instance.onActivate(&window);window.show();if(!phoneInfo.isEmpty())window.writePhoneInfo(phoneInfo);int testResult=0;
#ifdef AZEC_UI_TESTS
    if(selftest)QTimer::singleShot(700,&window,[&]{const auto report=window.selfTest(output);QFile file(output+"/ui-verification.json");file.open(QIODevice::WriteOnly);file.write(QJsonDocument(report).toJson());testResult=report["passed"].toBool()?0:2;QTimer::singleShot(150,&window,[&]{window.finishVerification();});});else
#endif
    if(duration>0)QTimer::singleShot(duration,&window,[&]{window.closeForTimeout();});
    const int result=app.exec();engine.stop();return selftest?testResult:result;
}
