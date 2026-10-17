#pragma once
#include <QtCore>
#include <filesystem>
#include <functional>
#include "diagnostics.hpp"

// Presentation choice is local UI state. It never changes the binary profile format.
namespace presentation {
enum class Interface { Goofy, Rally };
inline Interface active=Interface::Goofy;
inline bool isRally(){return active==Interface::Rally;}
inline QString name(Interface value){return value==Interface::Rally?QStringLiteral("rally"):QStringLiteral("goofy");}
inline Interface fromName(const QString& value){return value==QStringLiteral("rally")?Interface::Rally:Interface::Goofy;}
inline QString filePath(const std::filesystem::path& configuration){return QString::fromStdWString((configuration.parent_path()/L"azec_ui.json").wstring());}
inline bool readDocument(const QString& path,QJsonObject& root){
    QFile file(path);if(!file.exists()){root={};return true;}
    if(!file.open(QIODevice::ReadOnly)||file.size()>1024*1024)return false;
    QJsonParseError error;const auto document=QJsonDocument::fromJson(file.readAll(),&error);
    if(error.error!=QJsonParseError::NoError||!document.isObject())return false;
    root=document.object();return !root.contains("version")||root["version"].toInt(-1)==1;
}
inline Interface read(const std::filesystem::path& configuration){QJsonObject root;if(!readDocument(filePath(configuration),root))return Interface::Goofy;return fromName(root["interface"].toString());}
inline bool update(const std::filesystem::path& configuration,const std::function<void(QJsonObject&)>& change){
    QJsonObject root;const auto path=filePath(configuration);
    if(!readDocument(path,root)){diagnostic("save UI preferences","Unreadable or invalid file; existing preferences retained");return false;}
    change(root);root["version"]=1;const auto bytes=QJsonDocument(root).toJson();QSaveFile file(path);
    const bool ok=file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size()&&file.commit();
    if(!ok)diagnostic("save UI preferences",file.errorString());return ok;
}
inline bool write(const std::filesystem::path& configuration,Interface value){return update(configuration,[=](QJsonObject& root){root["interface"]=name(value);});}
}
