#pragma once
#include <QColor>
#include "interface_preferences.hpp"
#include "../third_party/rally_ui/qt/include/rally_ui.hpp"

// GoofyUI design tokens (github.com/Azec-Zuckerberg/goofy-ui-library) adapted
// for the desktop window: one cream paper canvas, white surfaces, chunky ink
// outlines, flat state colour and hard offset shadows. No gradients, no glow.
// Colour is reserved for live state (the accent, user-selectable) and the
// library's semantic palette.
namespace kbtheme {
inline QColor paper("#FFFAEE");    // canvas
inline QColor surface("#FFFFFF");  // cards, inputs, knobs
inline QColor ink("#31204C");      // text, outlines, shadows
inline QColor muted("#5D4973");    // secondary text
inline QColor purple("#7248D7");   // default accent
inline QColor lavender("#EBDEFF"); // off tracks, selection, dividers
inline QColor hover("#F6F0FF");    // row hover
inline QColor yellow("#FFDD59");   // stickers, numeric chips, header
inline QColor lime("#CCF47F");     // on / success
inline QColor pink("#FFA8C9");     // destructive, notices
inline QColor blue("#BAE7FF");     // stickers
inline QColor danger("#A6284D");   // error ink
inline QColor defaultAccent=purple;

inline void initialize(){
    if(!presentation::isRally())return;
    paper=surface=rally::canvas;ink=rally::text;muted=rally::muted;purple=rally::signal;
    lavender=rally::line;hover=rally::panel;yellow=rally::signal;lime=rally::muted;pink=danger=rally::danger;blue=rally::muted;defaultAccent=rally::signal;
}

// Text that stays readable on a filled surface (the library's 150-luminance rule).
inline QColor readableOn(const QColor& background){
    if(presentation::isRally())return rally::onColor(background);
    return (background.red()*299+background.green()*587+background.blue()*114)/1000>150?ink:QColor("#FFFFFF");
}
}
