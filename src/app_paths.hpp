#pragma once
#include <QtCore>

namespace azec_paths {
inline QString argument(const QString& prefix){
    QString value;for(const auto& item:QCoreApplication::arguments())if(item.startsWith(prefix))value=item.mid(prefix.size());return value;
}
// The installed launcher supplies both paths. Direct development/test runs
// deliberately retain their isolated current working directory.
inline bool initializeStorage(){
    const auto path=argument("--data-dir=");
    return path.isEmpty()||(QDir().mkpath(path)&&QDir::setCurrent(path));
}
inline QString launcher(){
    const auto path=argument("--azec-launcher=");
    return path.isEmpty()?QDir::cleanPath(QCoreApplication::applicationDirPath()+"/../Azec.exe"):QDir::cleanPath(path);
}
inline bool installed(){return !argument("--azec-launcher=").isEmpty();}
inline QString pendingUpdate(){
    const auto folder=installed()?QDir::current().filePath("updates"):QFileInfo(launcher()).absolutePath();
    return QDir().mkpath(folder)?QDir(folder).filePath("Azec.new.exe"):QString();
}
inline QStringList updateArguments(){
    QStringList args{"--apply-update",QDir::toNativeSeparators(launcher()),QString::number(QCoreApplication::applicationPid())};
    if(QCoreApplication::arguments().contains("--dry-run")){
        args.append("--dry-run");
        for(const auto& arg:QCoreApplication::arguments())if(arg.startsWith("--exit-after-ms="))args.append(arg);
    }
    return args;
}
}
