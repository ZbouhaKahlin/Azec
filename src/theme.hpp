#pragma once
#include <QColor>
#include <QPalette>
#include "ui_preferences.hpp"

// Monocode design tokens (github.com/hardbeat920/monocode, MIT) adapted for
// the desktop window: one dark neutral surface, hairline separators, opacity
// fills instead of opaque cards, a single blue accent, and a translucent
// window over the system blur. There are no palettes to switch; every value
// below is the product look.
namespace kbtheme {
inline QColor paper("#171717");          // hsl(240 0% 9%) — the app surface
inline QColor windowGlass(23,23,23,102); // MonoCode's 40% window wash over system blur
inline QColor surface("#202020");        // menus, popovers, inputs (opaque hosts)
inline QColor card(255,255,255,13);      // content 5% — raised cards
inline QColor ink("#EBEBEB");            // content hsl(240 0% 92%)
inline QColor muted("#A3A3A3");          // secondary text
inline QColor purple("#459BF7");         // accent hsl(211 92% 62%)
inline QColor lavender(235,235,235,26);  // selection: content 10%
inline QColor yellow("#E8C547");         // highlight marks
inline QColor lime("#10B981");           // on / success
inline QColor pink("#F43F5E");           // destructive
inline QColor blue("#7DD3FC");           // links
inline QColor danger("#FB7185");         // error ink
inline QColor defaultAccent=purple;
inline QColor outline(235,235,235,18);   // stroke: content 7% hairline
inline const QColor chipInk("#171717");

inline void initialize(){
    paper=QColor("#171717");windowGlass=QColor(23,23,23,102);surface=QColor("#202020");card=QColor(255,255,255,13);
    ink=QColor("#EBEBEB");muted=QColor("#A3A3A3");purple=defaultAccent=QColor("#459BF7");lavender=QColor(235,235,235,26);
    yellow=QColor("#E8C547");lime=QColor("#10B981");pink=QColor("#F43F5E");blue=QColor("#7DD3FC");danger=QColor("#FB7185");
    outline=QColor(235,235,235,18);
}

// Text that stays readable on a filled surface (the library's 150-luminance rule).
inline QColor readableOn(const QColor& background){

    return (background.red()*299+background.green()*587+background.blue()*114)/1000>150?chipInk:QColor("#FFFFFF");
}
// Stylesheets need the alpha; QColor::name() drops it.
inline QString css(const QColor& color){return color.alpha()==255?color.name():QStringLiteral("rgba(%1,%2,%3,%4)").arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alphaF(),0,'f',3);}
inline QPalette palette(){

    QPalette p;p.setColor(QPalette::Window,paper);p.setColor(QPalette::WindowText,ink);p.setColor(QPalette::Base,surface);p.setColor(QPalette::AlternateBase,card);
    p.setColor(QPalette::Text,ink);p.setColor(QPalette::Button,surface);p.setColor(QPalette::ButtonText,ink);p.setColor(QPalette::Highlight,defaultAccent);p.setColor(QPalette::HighlightedText,readableOn(defaultAccent));p.setColor(QPalette::PlaceholderText,muted);p.setColor(QPalette::Disabled,QPalette::Text,muted);p.setColor(QPalette::Disabled,QPalette::ButtonText,muted);return p;
}
}
