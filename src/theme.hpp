#pragma once
#include <QColor>
#include <QPalette>
#include <array>
#include "ui_preferences.hpp"

// GoofyUI design tokens (github.com/Azec-Zuckerberg/goofy-ui-library) adapted
// for the desktop window: named palettes, chunky ink outlines, flat state
// colour and hard offset shadows. Every palette shares the same Goofy layout.
namespace kbtheme {
inline QColor paper("#FFFAEE");    // canvas
inline QColor surface("#FFFFFF");  // cards, inputs, knobs
inline QColor ink("#31204C");      // text, outlines, shadows
inline QColor muted("#5D4973");    // secondary text
inline QColor purple("#7248D7");   // default accent
inline QColor lavender("#EBDEFF"); // off tracks, selection, dividers
inline QColor hover("#F6F0FF");    // row hover
inline QColor yellow("#FFDD59");   // stickers, numeric chips
inline QColor lime("#CCF47F");     // on / success
inline QColor pink("#FFA8C9");     // destructive, notices
inline QColor blue("#BAE7FF");     // stickers
inline QColor danger("#A6284D");   // error ink
inline QColor defaultAccent=purple;
inline QColor outline=ink,card=paper,header=paper;
inline const QColor chipInk("#31204C");

// Named palettes change materials only; all Goofy themes share one widget tree.
struct ColorTheme {const char *id,*name,*paper,*surface,*card,*accent,*selection,*text,*muted,*outline;};
inline const std::array<ColorTheme,10> colorThemes{{
    {"goofy","Goofy (original)","#FFFAEE","#FFFFFF","#FFFAEE","#7248D7","#EBDEFF","#31204C","#5D4973","#31204C"},
    {"pistachio","Pistachio & coral","#EEF5E9","#FFFFFF","#F7FAF2","#D85947","#F9DDD2","#27372D","#596D5D","#27372D"},
    {"ice-blue","Ice blue & cobalt","#EAF2FF","#FFFFFF","#F5F8FF","#315BDF","#DDE6FF","#182D50","#536987","#182D50"},
    {"lilac","Lilac","#EFE8FB","#FFFFFF","#F8F4FF","#7248D7","#E6DAFF","#2C2046","#75608D","#2C2046"},
    {"butter-ocean","Butter & ocean","#FFF8D9","#FFFDF4","#FFFAE8","#177B86","#D3EEEA","#263D40","#5F756F","#263D40"},
    {"rose","Rose","#FBECEF","#FFFCFD","#FFF5F7","#AF3B64","#F6D4E1","#442536","#886071","#442536"},
    {"midnight","Midnight violet","#171522","#242131","#211F2C","#B89AFF","#47385F","#F2EAFB","#B9ADC8","#100E17"},
    {"forest","Forest & tangerine","#14241E","#21382D","#1D3027","#F7A25B","#3F5743","#F3EED9","#B0C5B4","#0B1711"},
    {"deep-sea","Deep sea & citron","#14242C","#21343E","#1B2E37","#DDE877","#41524A","#F0F4E4","#B0C5CC","#0B171E"},
    {"charcoal","Charcoal & blush","#222122","#323032","#2C2A2C","#F2B3C6","#58414A","#F8EFF2","#C8B7BF","#131113"}
}};
inline QString activeColorTheme="goofy";
inline const ColorTheme& colorTheme(const QString& id){for(const auto& theme:colorThemes)if(id==QLatin1String(theme.id))return theme;return colorThemes.front();}
inline QString readColorTheme(const std::filesystem::path& path){QJsonObject root;if(!uiprefs::readDocument(uiprefs::filePath(path),root))return QStringLiteral("goofy");return QLatin1String(colorTheme(root["colorTheme"].toString()).id);}
inline bool writeColorTheme(const std::filesystem::path& path,const QString& id){return uiprefs::update(path,[&](QJsonObject& root){root["colorTheme"]=QLatin1String(colorTheme(id).id);});}

inline void initialize(const QString& id=QStringLiteral("goofy")){
    const auto& theme=colorTheme(id);activeColorTheme=QLatin1String(theme.id);
    paper=QColor(theme.paper);surface=QColor(theme.surface);card=QColor(theme.card);header=paper;
    purple=defaultAccent=QColor(theme.accent);lavender=QColor(theme.selection);ink=QColor(theme.text);muted=QColor(theme.muted);outline=QColor(theme.outline);
    yellow=QColor("#FFDD59");lime=QColor("#CCF47F");pink=QColor("#FFA8C9");blue=QColor("#BAE7FF");danger=QColor("#A6284D");
    if(ink.lightness()>180)danger=QColor("#FFA8C9");
}

// Text that stays readable on a filled surface (the library's 150-luminance rule).
inline QColor readableOn(const QColor& background){

    return (background.red()*299+background.green()*587+background.blue()*114)/1000>150?chipInk:QColor("#FFFFFF");
}
inline QPalette palette(){

    QPalette p;p.setColor(QPalette::Window,paper);p.setColor(QPalette::WindowText,ink);p.setColor(QPalette::Base,surface);p.setColor(QPalette::AlternateBase,card);
    p.setColor(QPalette::Text,ink);p.setColor(QPalette::Button,surface);p.setColor(QPalette::ButtonText,ink);p.setColor(QPalette::Highlight,defaultAccent);p.setColor(QPalette::HighlightedText,readableOn(defaultAccent));p.setColor(QPalette::PlaceholderText,muted);p.setColor(QPalette::Disabled,QPalette::Text,muted);p.setColor(QPalette::Disabled,QPalette::ButtonText,muted);return p;
}
}
