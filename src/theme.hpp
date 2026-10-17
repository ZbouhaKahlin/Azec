#pragma once
#include <QColor>

// GoofyUI design tokens (github.com/Azec-Zuckerberg/goofy-ui-library) adapted
// for the desktop window: one cream paper canvas, white surfaces, chunky ink
// outlines, flat state colour and hard offset shadows. No gradients, no glow.
// Colour is reserved for live state (the accent, user-selectable) and the
// library's semantic palette.
namespace kbtheme {
inline const QColor paper("#FFFAEE");    // canvas
inline const QColor surface("#FFFFFF");  // cards, inputs, knobs
inline const QColor ink("#31204C");      // text, outlines, shadows
inline const QColor muted("#5D4973");    // secondary text
inline const QColor purple("#7248D7");   // default accent
inline const QColor lavender("#EBDEFF"); // off tracks, selection, dividers
inline const QColor hover("#F6F0FF");    // row hover
inline const QColor yellow("#FFDD59");   // stickers, numeric chips, header
inline const QColor lime("#CCF47F");     // on / success
inline const QColor pink("#FFA8C9");     // destructive, notices
inline const QColor blue("#BAE7FF");     // stickers
inline const QColor danger("#A6284D");   // error ink
inline const QColor defaultAccent=purple;

// Text that stays readable on a filled surface (the library's 150-luminance rule).
inline QColor readableOn(const QColor& background){
    return (background.red()*299+background.green()*587+background.blue()*114)/1000>150?ink:QColor("#FFFFFF");
}
}
