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
inline QColor windowGlass(13,13,13,204); // inset-outer: #0d0d0d at 80%, below the content/5 wash
inline QColor windowWash(235,235,235,13);
inline QColor panelGlass(6,6,6,102);     // inset-card: #060606 at 40%, over the already dark shell
inline QColor surface("#202020");        // menus, popovers, inputs (opaque hosts)
inline QColor card(235,235,235,8);       // Settings Group: content/3
inline QColor ink("#EBEBEB");            // content hsl(240 0% 92%)
inline QColor muted(235,235,235,115);          // secondary text
inline QColor purple("#459BF7");         // accent hsl(211 92% 62%)
inline QColor lavender(235,235,235,26);  // selection: content 10%
inline QColor yellow("#E8C547");         // highlight marks
inline QColor lime("#10B981");           // on / success
inline QColor pink("#F43F5E");           // destructive
inline QColor blue("#7DD3FC");           // links
inline QColor danger("#FB7185");         // error ink
inline QColor defaultAccent=purple;
inline QColor outline(235,235,235,10);   // inset-stroke: content/4
inline const QColor chipInk("#171717");

inline void initialize(){
    paper=QColor("#171717");windowGlass=QColor(13,13,13,204);windowWash=QColor(235,235,235,13);panelGlass=QColor(6,6,6,102);surface=QColor("#202020");card=QColor(235,235,235,8);
    ink=QColor("#EBEBEB");muted=QColor(235,235,235,115);purple=defaultAccent=QColor("#459BF7");lavender=QColor(235,235,235,26);
    yellow=QColor("#E8C547");lime=QColor("#10B981");pink=QColor("#F43F5E");blue=QColor("#7DD3FC");danger=QColor("#FB7185");
    outline=QColor(235,235,235,10);
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
