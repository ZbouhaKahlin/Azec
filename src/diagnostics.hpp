#pragma once
#include <QtCore>
#include <mutex>

// Deliberately log only operation names/errors; never requests or pairing secrets.
inline void diagnostic(const QString& operation, const QString& detail) {
    static std::mutex mutex;
    const std::lock_guard guard(mutex);
    const QString path=QDir::current().filePath("azec.log");
    if(QFileInfo(path).size()>256*1024){QFile::remove(path+".1");QFile::rename(path,path+".1");}
    QFile file(path);
    if(file.open(QIODevice::WriteOnly|QIODevice::Append|QIODevice::Text)){
        QString safe=detail;safe.replace('\n',' ');safe.replace('\r',' ');
        file.write((QDateTime::currentDateTimeUtc().toString(Qt::ISODate)+" "+operation+": "+safe.left(512)+"\n").toUtf8());
    }
}
