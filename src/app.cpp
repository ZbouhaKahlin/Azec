#include "main_window.hpp"
#include "startup.hpp"

int main(int argc,char** argv){
    QApplication app(argc,argv);QApplication::setApplicationName("KingBoss2004");QApplication::setOrganizationName("KingBoss2004");QApplication::setStyle("Fusion");
    for(const auto* weight:{"Regular","Medium","SemiBold","Bold"})QFontDatabase::addApplicationFont(QString(":/fonts/Inter-%1.otf").arg(weight));
    QFont font("Inter");font.setPixelSize(14);font.setWeight(QFont::Normal);font.setStyleStrategy(QFont::PreferAntialias);app.setFont(font);
    QPalette palette;palette.setColor(QPalette::Window,QColor("#141416"));palette.setColor(QPalette::WindowText,QColor("#dedee5"));palette.setColor(QPalette::Base,QColor("#222225"));palette.setColor(QPalette::AlternateBase,QColor("#2a2a30"));palette.setColor(QPalette::Text,QColor("#dedee5"));palette.setColor(QPalette::Button,QColor("#2a2a30"));palette.setColor(QPalette::ButtonText,QColor("#dedee5"));palette.setColor(QPalette::Highlight,QColor("#ff5b63"));palette.setColor(QPalette::HighlightedText,Qt::white);palette.setColor(QPalette::Disabled,QPalette::Text,QColor("#6e6e7a"));app.setPalette(palette);
    const auto arguments=app.arguments();const bool selftest=arguments.contains("--ui-self-test"),dryrun=selftest||arguments.contains("--dry-run");QString output,phoneInfo;int duration=0;for(const auto& argument:arguments){if(argument.startsWith("--phone-info="))phoneInfo=argument.mid(13);if(argument.startsWith("--output="))output=argument.mid(9);if(argument.startsWith("--exit-after-ms="))duration=argument.mid(16).toInt();}if(output.isEmpty())output=QDir::currentPath();QDir().mkpath(output);
#ifndef KINGBOSS_UI_TESTS
    if(selftest){
        const auto verifier=QCoreApplication::applicationDirPath()+"/KingBoss2004-checks.exe";
        if(!QFile::exists(verifier)){diagnostic("verification","Build with build.cmd test to create the separate verification executable");return 2;}
        return QProcess::execute(verifier,arguments.mid(1));
    }
#endif
    SingleInstance instance;if(!instance.acquire())return 0;
    r6::Engine engine(dryrun);const auto configPath=std::filesystem::current_path()/L"r6_config.bin";const auto recovery=loadConfiguration(engine,configPath);DesktopTranslator translator([&]{return engine.snapshot().language;});app.installTranslator(&translator);engine.start();MainWindow window(engine,configPath);window.setStartupNotice(recovery.notice,recovery.blockSaving);instance.onActivate(&window);window.show();if(!phoneInfo.isEmpty())window.writePhoneInfo(phoneInfo);int testResult=0;
#ifdef KINGBOSS_UI_TESTS
    if(selftest)QTimer::singleShot(700,&window,[&]{const auto report=window.selfTest(output);QFile file(output+"/ui-verification.json");file.open(QIODevice::WriteOnly);file.write(QJsonDocument(report).toJson());testResult=report["passed"].toBool()?0:2;QTimer::singleShot(150,&window,[&]{window.finishVerification();});});else
#endif
    if(duration>0)QTimer::singleShot(duration,&window,[&]{window.closeForTimeout();});
    const int result=app.exec();engine.stop();return selftest?testResult:result;
}

