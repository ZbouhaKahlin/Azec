#pragma once
#include <QtGui>
#include <cmath>
#include "theme.hpp"

// Bundled, unmodified country flag SVGs from flag-icons. No emoji or hand-drawn emblems.
// See licenses/flag-icons for the pinned source and MIT attribution.
inline QString languageFlagResource(int language){
    static const char* countries[]={"br","us","fr","es","de","sa","il","cn","jp"};
    return QString(":/flags/%1.svg").arg(QLatin1String(countries[language>=0&&language<9?language:1]));
}
inline QIcon languageFlag(int language){return QIcon(languageFlagResource(language));}
struct LanguageEntry{const char* label;int value;};
inline const QList<LanguageEntry>& languageEntries(){
    static const QList<LanguageEntry> entries={{"English (US)",1},{"Português (Brasil)",0},{"Français",2},{"Español",3},{"Deutsch",4},{"العربية",5},{"עברית",6},{"简体中文",7},{"日本語",8}};
    return entries;
}
// The reveal flag for the Free Palestine surprise popup.
inline QPixmap palestineFlagPixmap(int width,int height){
    return QIcon(":/flags/ps.svg").pixmap(QSize(width,height),2.0);
}
