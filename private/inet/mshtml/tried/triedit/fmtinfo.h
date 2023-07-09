// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved

#ifndef __FMTINFO_H__
#define __FMTINFO_H__


#define RGB_BLACK       RGB(0x00, 0x00, 0x00)
#define RGB_WHITE       RGB(0xFF, 0xFF, 0xFF)
#define RGB_RED         RGB(0xFF, 0x00, 0x00)
#define RGB_GREEN       RGB(0x00, 0xFF, 0x00)
#define RGB_BLUE        RGB(0x00, 0x00, 0xFF)
#define RGB_YELLOW      RGB(0xFF, 0xFF, 0x00)
#define RGB_MAGENTA     RGB(0xFF, 0x00, 0xFF)
#define RGB_CYAN        RGB(0x00, 0xFF, 0xFF)
#define RGB_LIGHTGRAY   RGB(0xC0, 0xC0, 0xC0)
#define RGB_GRAY        RGB(0x80, 0x80, 0x80)
#define RGB_DARKRED     RGB(0x80, 0x00, 0x00)
#define RGB_DARKGREEN   RGB(0x00, 0x80, 0x00)
#define RGB_DARKBLUE    RGB(0x00, 0x00, 0x80)
#define RGB_LIGHTBROWN  RGB(0x80, 0x80, 0x00)
#define RGB_DARKMAGENTA RGB(0x80, 0x00, 0x80)
#define RGB_DARKCYAN    RGB(0x00, 0x80, 0x80)

// IMPORTANT: These macros depend heavily on the order of things in colors.cpp.
//    1) The order of colors in window must be Source Text, Text Selection, Text Highlight.
#define AUTO_TEXT           { TRUE, TRUE, FALSE, FALSE, FALSE, COLOR_WINDOWTEXT },  { TRUE, TRUE, FALSE, FALSE, FALSE, COLOR_WINDOW }
#define AUTO_SELECTION      { TRUE, FALSE, FALSE, TRUE, FALSE, 0 },                 { TRUE, FALSE, FALSE, TRUE, FALSE, 0 }
#define AUTO_HIGHLIGHT      { TRUE, TRUE, FALSE, FALSE, FALSE, COLOR_HIGHLIGHTTEXT },   { TRUE, TRUE, FALSE, FALSE, FALSE, COLOR_HIGHLIGHT }

#define AUTO_REF(n)         { TRUE, FALSE, FALSE, FALSE, FALSE, n },    { TRUE, FALSE, FALSE, FALSE, FALSE, n }
#define AUTO_REF_SRC(n)     { TRUE, FALSE, TRUE, FALSE, FALSE, n },     { TRUE, FALSE, TRUE, FALSE, FALSE, n }

#define BACKAUTO_TEXT           { FALSE, TRUE, FALSE, FALSE, FALSE, COLOR_WINDOWTEXT }, { TRUE, TRUE, FALSE, FALSE, FALSE, COLOR_WINDOW }
#define BACKAUTO_SELECTION      { FALSE, FALSE, FALSE, TRUE, FALSE, 0 },                    { TRUE, FALSE, FALSE, TRUE, FALSE, 0 }
#define BACKAUTO_HIGHLIGHT      { FALSE, TRUE, FALSE, FALSE, FALSE, COLOR_HIGHLIGHTTEXT },  { TRUE, TRUE, FALSE, FALSE, FALSE, COLOR_HIGHLIGHT }

#define BACKAUTO_REF(n)         { FALSE, FALSE, FALSE, FALSE, FALSE, n },   { TRUE, FALSE, FALSE, FALSE, FALSE, n }
#define BACKAUTO_REF_SRC(n)     { FALSE, FALSE, TRUE, FALSE, FALSE, n },    { TRUE, FALSE, TRUE, FALSE, FALSE, n }

#define NOTAUTO_TEXT        { FALSE, TRUE, FALSE, FALSE, FALSE, COLOR_WINDOWTEXT }, { FALSE, TRUE, FALSE, FALSE, FALSE, COLOR_WINDOW }
#define NOTAUTO_SELECTION   { FALSE, FALSE, FALSE, TRUE, FALSE, 0 },    { FALSE, FALSE, FALSE, TRUE, FALSE, 0 }
#define NOTAUTO_HIGHLIGHT   { FALSE, TRUE, FALSE, FALSE, FALSE, COLOR_HIGHLIGHTTEXT },  { FALSE, TRUE, FALSE, FALSE, FALSE, COLOR_HIGHLIGHT }

#define NOTAUTO_REF(n)      { FALSE, FALSE, FALSE, FALSE, FALSE, n },       { FALSE, FALSE, FALSE, FALSE, FALSE, n }
#define NOTAUTO_REF_SRC(n)  { FALSE, FALSE, TRUE, FALSE, FALSE, n },        { FALSE, FALSE, TRUE, FALSE, FALSE, n }

struct AUTO_COLOR
{
    WORD    bOn:1;      // Is auto color being used now?
    WORD    bSys:1;     // Get the color from the system(1) or from a window(0)?
    WORD    bSrc:1;     // If bSys == 0, use this window(0) or the Source Window(1)?
    WORD    bRev:1;     // If from this window, reverse fore/background(1)?
    WORD    bUpd:1;     // Used by UpdateAutoColors().
    WORD    index:5;    // Index into element list(bSys==0) or COLOR_* value (bSys==1).
};



#endif /* __FMTINFO_H__ */

