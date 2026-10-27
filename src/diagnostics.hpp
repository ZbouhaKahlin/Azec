#pragma once
#include <QtCore>
#include <mutex>
#include "version.hpp"

inline QString diagnosticSafe(QString value){
    value.replace('\\','/');
    const auto home=QDir::fromNativeSeparators(QDir::homePath());
    if(!home.isEmpty())value.replace(home,QStringLiteral("<user>"),Qt::CaseInsensitive);
    value.replace(QRegularExpression(QStringLiteral("https?://[^\\s]+")),QStringLiteral("<link>"));
    value.replace(QRegularExpression(QStringLiteral("(?i)(token|code|authorization)\\s*[:=]\\s*[^\\s;,]+")),QStringLiteral("\\1=<redacted>"));
    return value.left(4096);
}

inline QString errorReport(const QString& code,const QString& message,const QString& detail={}){
    return QStringLiteral("Azec error report\nVersion: ")+AZEC_VERSION+
        "\nUTC: "+QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)+
        "\nSystem: "+QSysInfo::prettyProductName()+" ("+QSysInfo::currentCpuArchitecture()+")"+
        "\nError: "+code+"\nMessage: "+diagnosticSafe(message)+
        (detail.isEmpty()?QString():"\nDetails: "+diagnosticSafe(detail));
}

// Deliberately log only operation names/errors; never requests or pairing secrets.
inline void diagnostic(const QString& operation, const QString& detail) {
    static std::mutex mutex;
    const std::lock_guard guard(mutex);
    const QString path=QDir::current().filePath("azec.log");
    if(QFileInfo(path).size()>256*1024){QFile::remove(path+".1");QFile::rename(path,path+".1");}
    QFile file(path);
    if(file.open(QIODevice::WriteOnly|QIODevice::Append|QIODevice::Text)){
        QString safe=diagnosticSafe(detail);safe.replace('\n',' ');safe.replace('\r',' ');
        file.write((QDateTime::currentDateTimeUtc().toString(Qt::ISODate)+" "+operation+": "+safe+"\n").toUtf8());
    }
}
