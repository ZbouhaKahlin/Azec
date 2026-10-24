#pragma once
#include <QtCore>
#include <filesystem>
#include <functional>
#include "diagnostics.hpp"

// Local appearance preferences never change the binary profile format.
namespace uiprefs {
inline QString filePath(const std::filesystem::path& configuration){return QString::fromStdWString((configuration.parent_path()/L"azec_ui.json").wstring());}
inline bool readDocument(const QString& path,QJsonObject& root){
    QFile file(path);if(!file.exists()){root={};return true;}
    if(!file.open(QIODevice::ReadOnly)||file.size()>1024*1024)return false;
    const auto bytes=file.readAll();if(file.error()!=QFile::NoError)return false;
    // A successfully read empty file contains no preferences to preserve.
    // Recover it like a missing file; nonempty malformed files still fail closed.
    if(bytes.isEmpty()){root={};return true;}
    QJsonParseError error;const auto document=QJsonDocument::fromJson(bytes,&error);
    if(error.error!=QJsonParseError::NoError||!document.isObject())return false;
    root=document.object();return !root.contains("version")||root["version"].toInt(-1)==1;
}

inline bool sidebarCollapsed(const std::filesystem::path& configuration){QJsonObject root;return readDocument(filePath(configuration),root)&&root["sidebarCollapsed"].toBool(false);}
inline bool update(const std::filesystem::path& configuration,const std::function<void(QJsonObject&)>& change){
    QJsonObject root;const auto path=filePath(configuration);
    if(!readDocument(path,root)){diagnostic("save UI preferences","Unreadable or invalid file; existing preferences retained");return false;}
    change(root);root.remove("interface");root.remove("colorTheme");root["version"]=1;const auto bytes=QJsonDocument(root).toJson();QSaveFile file(path);
    const bool ok=file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size()&&file.commit();
    if(!ok)diagnostic("save UI preferences",file.errorString());return ok;
}

}
