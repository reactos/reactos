/*
 * X11 keyboard driver
 *
 * Copyright 1993 Bob Amstadt
 * Copyright 1996 Albrecht Kleine
 * Copyright 1997 David Faure
 * Copyright 1998 Morten Welinder
 * Copyright 1998 Ulrich Weigand
 * Copyright 1999 Ove KÃ¥ven
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#ifdef HAVE_X11_XKBLIB_H
#include <X11/XKBlib.h>
#endif

#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "winnls.h"
#include "x11drv.h"
#include "wine/server.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(keyboard);
WINE_DECLARE_DEBUG_CHANNEL(key);

/* key state table bits:
  0x80 -> key is pressed
  0x40 -> key got pressed since last time
  0x01 -> key is toggled
*/
BYTE key_state_table[256];

static BYTE TrackSysKey = 0; /* determine whether ALT key up will cause a WM_SYSKEYUP
                                or a WM_KEYUP message */

static int min_keycode, max_keycode, keysyms_per_keycode;
static WORD keyc2vkey[256], keyc2scan[256];

static int NumLockMask, ScrollLockMask, AltGrMask; /* mask in the XKeyEvent state */

static char KEYBOARD_MapDeadKeysym(KeySym keysym);

/* Keyboard translation tables */
#define MAIN_LEN 49
static const WORD main_key_scan_qwerty[MAIN_LEN] =
{
/* this is my (102-key) keyboard layout, sorry if it doesn't quite match yours */
 /* `    1    2    3    4    5    6    7    8    9    0    -    = */
   0x29,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,
 /* q    w    e    r    t    y    u    i    o    p    [    ] */
   0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,
 /* a    s    d    f    g    h    j    k    l    ;    '    \ */
   0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x2B,
 /* z    x    c    v    b    n    m    ,    .    / */
   0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33,0x34,0x35,
   0x56 /* the 102nd key (actually to the right of l-shift) */
};

static const WORD main_key_scan_abnt_qwerty[MAIN_LEN] =
{
 /* `    1    2    3    4    5    6    7    8    9    0    -    = */
   0x29,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,
 /* q    w    e    r    t    y    u    i    o    p    [    ] */
   0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,
 /* a    s    d    f    g    h    j    k    l    ;    '    \ */
   0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x2B,
 /* \      z    x    c    v    b    n    m    ,    .    / */
   0x5e,0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33,0x34,0x35,
   0x56, /* the 102nd key (actually to the right of l-shift) */
};

static const WORD main_key_scan_dvorak[MAIN_LEN] =
{
 /* `    1    2    3    4    5    6    7    8    9    0    [    ] */
   0x29,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x1A,0x1B,
 /* '    ,    .    p    y    f    g    c    r    l    /    = */
   0x28,0x33,0x34,0x19,0x15,0x21,0x22,0x2E,0x13,0x26,0x35,0x0D,
 /* a    o    e    u    i    d    h    t    n    s    -    \ */
   0x1E,0x18,0x12,0x16,0x17,0x20,0x23,0x14,0x31,0x1F,0x0C,0x2B,
 /* ;    q    j    k    x    b    m    w    v    z */
   0x27,0x10,0x24,0x25,0x2D,0x30,0x32,0x11,0x2F,0x2C,
   0x56 /* the 102nd key (actually to the right of l-shift) */
};

static const WORD main_key_scan_qwerty_jp106[MAIN_LEN] =
{
  /* this is my (106-key) keyboard layout, sorry if it doesn't quite match yours */
 /* 1    2    3    4    5    6    7    8    9    0    -    ^    \ */
   0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x29,
 /* q    w    e    r    t    y    u    i    o    p    @    [ */
   0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,
 /* a    s    d    f    g    h    j    k    l    ;    :    ] */
   0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x2B,
 /* z    x    c    v    b    n    m    ,    .    / */
   0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33,0x34,0x35,
   0x56 /* the 102nd key (actually to the right of l-shift) */
};

static const WORD main_key_scan_qwerty_macjp[MAIN_LEN] =
{
 /* 1    2    3    4    5    6    7    8    9    0    -    ^    \ */
   0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x7d,
 /* q    w    e    r    t    y    u    i    o    p    @    [ */
   0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,
 /* a    s    d    f    g    h    j    k    l    ;    :    ] */
   0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x2B,
 /* z    x    c    v    b    n    m    ,    .    / */
   0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33,0x34,0x35,
   0x73 /* the 102nd key (actually to the right of l-shift) */
};


static const WORD main_key_vkey_qwerty[MAIN_LEN] =
{
/* NOTE: this layout must concur with the scan codes layout above */
   VK_OEM_3,'1','2','3','4','5','6','7','8','9','0',VK_OEM_MINUS,VK_OEM_PLUS,
   'Q','W','E','R','T','Y','U','I','O','P',VK_OEM_4,VK_OEM_6,
   'A','S','D','F','G','H','J','K','L',VK_OEM_1,VK_OEM_7,VK_OEM_5,
   'Z','X','C','V','B','N','M',VK_OEM_COMMA,VK_OEM_PERIOD,VK_OEM_2,
   VK_OEM_102 /* the 102nd key (actually to the right of l-shift) */
};

static const WORD main_key_vkey_qwerty_jp106[MAIN_LEN] =
{
/* NOTE: this layout must concur with the scan codes layout above */
   '1','2','3','4','5','6','7','8','9','0',VK_OEM_MINUS,VK_OEM_PLUS,VK_OEM_3,
   'Q','W','E','R','T','Y','U','I','O','P',VK_OEM_4,VK_OEM_6,
   'A','S','D','F','G','H','J','K','L',VK_OEM_1,VK_OEM_7,VK_OEM_5,
   'Z','X','C','V','B','N','M',VK_OEM_COMMA,VK_OEM_PERIOD,VK_OEM_2,
   VK_OEM_102 /* the 102nd key (actually to the right of l-shift) */
};

static const WORD main_key_vkey_qwerty_macjp[MAIN_LEN] =
{
/* NOTE: this layout must concur with the scan codes layout above */
   '1','2','3','4','5','6','7','8','9','0',VK_OEM_MINUS,VK_OEM_7,VK_OEM_5,
   'Q','W','E','R','T','Y','U','I','O','P',VK_OEM_3,VK_OEM_4,
   'A','S','D','F','G','H','J','K','L',VK_OEM_PLUS,VK_OEM_1,VK_OEM_6,
   'Z','X','C','V','B','N','M',VK_OEM_COMMA,VK_OEM_PERIOD,VK_OEM_2,
   VK_OEM_102 /* the 102nd key (actually to the right of l-shift) */
};

static const WORD main_key_vkey_qwerty_v2[MAIN_LEN] =
{
/* NOTE: this layout must concur with the scan codes layout above */
   VK_OEM_5,'1','2','3','4','5','6','7','8','9','0',VK_OEM_PLUS,VK_OEM_4,
   'Q','W','E','R','T','Y','U','I','O','P',VK_OEM_6,VK_OEM_1,
   'A','S','D','F','G','H','J','K','L',VK_OEM_3,VK_OEM_7,VK_OEM_2,
   'Z','X','C','V','B','N','M',VK_OEM_COMMA,VK_OEM_PERIOD,VK_OEM_MINUS,
   VK_OEM_102 /* the 102nd key (actually to the right of l-shift) */
};

static const WORD main_key_vkey_qwertz[MAIN_LEN] =
{
/* NOTE: this layout must concur with the scan codes layout above */
   VK_OEM_3,'1','2','3','4','5','6','7','8','9','0',VK_OEM_MINUS,VK_OEM_PLUS,
   'Q','W','E','R','T','Z','U','I','O','P',VK_OEM_4,VK_OEM_6,
   'A','S','D','F','G','H','J','K','L',VK_OEM_1,VK_OEM_7,VK_OEM_5,
   'Y','X','C','V','B','N','M',VK_OEM_COMMA,VK_OEM_PERIOD,VK_OEM_2,
   VK_OEM_102 /* the 102nd key (actually to the right of l-shift) */
};

static const WORD main_key_vkey_abnt_qwerty[MAIN_LEN] =
{
/* NOTE: this layout must concur with the scan codes layout above */
   VK_OEM_3,'1','2','3','4','5','6','7','8','9','0',VK_OEM_MINUS,VK_OEM_PLUS,
   'Q','W','E','R','T','Y','U','I','O','P',VK_OEM_4,VK_OEM_6,
   'A','S','D','F','G','H','J','K','L',VK_OEM_1,VK_OEM_8,VK_OEM_5,
   VK_OEM_7,'Z','X','C','V','B','N','M',VK_OEM_COMMA,VK_OEM_PERIOD,VK_OEM_2,
   VK_OEM_102, /* the 102nd key (actually to the right of l-shift) */
};

static const WORD main_key_vkey_azerty[MAIN_LEN] =
{
/* NOTE: this layout must concur with the scan codes layout above */
   VK_OEM_7,'1','2','3','4','5','6','7','8','9','0',VK_OEM_4,VK_OEM_PLUS,
   'A','Z','E','R','T','Y','U','I','O','P',VK_OEM_6,VK_OEM_1,
   'Q','S','D','F','G','H','J','K','L','M',VK_OEM_3,VK_OEM_5,
   'W','X','C','V','B','N',VK_OEM_COMMA,VK_OEM_PERIOD,VK_OEM_2,VK_OEM_8,
   VK_OEM_102 /* the 102nd key (actually to the right of l-shift) */
};

static const WORD main_key_vkey_dvorak[MAIN_LEN] =
{
/* NOTE: this layout must concur with the scan codes layout above */
   VK_OEM_3,'1','2','3','4','5','6','7','8','9','0',VK_OEM_4,VK_OEM_6,
   VK_OEM_7,VK_OEM_COMMA,VK_OEM_PERIOD,'P','Y','F','G','C','R','L',VK_OEM_2,VK_OEM_PLUS,
   'A','O','E','U','I','D','H','T','N','S',VK_OEM_MINUS,VK_OEM_5,
   VK_OEM_1,'Q','J','K','X','B','M','W','V','Z',
   VK_OEM_102 /* the 102nd key (actually to the right of l-shift) */
};

/*** DEFINE YOUR NEW LANGUAGE-SPECIFIC MAPPINGS BELOW, SEE EXISTING TABLES */

/* the VK mappings for the main keyboard will be auto-assigned as before,
   so what we have here is just the character tables */
/* order: Normal, Shift, AltGr, Shift-AltGr */
/* We recommend you write just what is guaranteed to be correct (i.e. what's
   written on the keycaps), not the bunch of special characters behind AltGr
   and Shift-AltGr if it can vary among different X servers */
/* Remember that your 102nd key (to the right of l-shift) should be on a
   separate line, see existing tables */
/* If Wine fails to match your new table, use WINEDEBUG=+key to find out why */
/* Remember to also add your new table to the layout index table far below! */

/*** United States keyboard layout (mostly contributed by Uwe Bonnes) */
static const char main_key_US[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","[{","]}",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL",";:","'\"","\\|",
 "zZ","xX","cC","vV","bB","nN","mM",",<",".>","/?"
};

/*** United States keyboard layout (phantom key version) */
/* (XFree86 reports the <> key even if it's not physically there) */
static const char main_key_US_phantom[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","[{","]}",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL",";:","'\"","\\|",
 "zZ","xX","cC","vV","bB","nN","mM",",<",".>","/?",
 "<>" /* the phantom key */
};

/*** United States keyboard layout (dvorak version) */
static const char main_key_US_dvorak[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","[{","]}",
 "'\"",",<",".>","pP","yY","fF","gG","cC","rR","lL","/?","=+",
 "aA","oO","eE","uU","iI","dD","hH","tT","nN","sS","-_","\\|",
 ";:","qQ","jJ","kK","xX","bB","mM","wW","vV","zZ"
};

/*** British keyboard layout */
static const char main_key_UK[MAIN_LEN][4] =
{
 "`","1!","2\"","3£","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","[{","]}",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL",";:","'@","#~",
 "zZ","xX","cC","vV","bB","nN","mM",",<",".>","/?",
 "\\|"
};

/*** French keyboard layout (setxkbmap fr) */
static const char main_key_FR[MAIN_LEN][4] =
{
 "²","&1","é2","\"3","'4","(5","-6","è7","_8","ç9","à0",")°","=+",
 "aA","zZ","eE","rR","tT","yY","uU","iI","oO","pP","^¨","$£",
 "qQ","sS","dD","fF","gG","hH","jJ","kK","lL","mM","ù%","*µ",
 "wW","xX","cC","vV","bB","nN",",?",";.",":/","!§",
 "<>"
};

/*** Icelandic keyboard layout (setxkbmap is) */
static const char main_key_IS[MAIN_LEN][4] =
{
 "°","1!","2\"","3#","4$","5%","6&","7/","8(","9)","0=","öÖ","-_",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","ðÐ","'?",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","æÆ","´Ä","+*",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","þÞ",
 "<>"
};

/* All german keyb layout tables have the acute/apostrophe symbol next to
 * the BACKSPACE key removed (replaced with NULL which is ignored by the
 * detection code).
 * This was done because the mapping of the acute (and apostrophe) is done
 * differently in various xkb-data/xkeyboard-config versions. Some replace
 * the acute with a normal apostrophe, so that the apostrophe is found twice
 * on the keyboard (one next to BACKSPACE and one next to ENTER).
 * Others put the acute and grave accents on the key left of BACKSPACE.
 * More information on the fd.o bugtracker:
 * https://bugs.freedesktop.org/show_bug.cgi?id=11514
 */

/*** German Logitech Desktop Pro keyboard layout */
static const char main_key_DE_logitech[MAIN_LEN][4] =
{
 "^\xb0","1!","2\"","3\xa7","4$","5%","6&","7/{","8([","9)]","0=}","\xdf?\\","\0`",
 "qQ@","wW","eE","rR","tT","zZ","uU","iI","oO","pP","\xfc\xdc","+*~",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","\xf6\xd6","\xe4\xc4","#'",
 "yY","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>|"
};

/*** German keyboard layout (setxkbmap de) */
static const char main_key_DE[MAIN_LEN][4] =
{
 "^°","1!","2\"","3§","4$","5%","6&","7/","8(","9)","0=","ß?","\0`",
 "qQ","wW","eE","rR","tT","zZ","uU","iI","oO","pP","üÜ","+*",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","öÖ","äÄ","#'",
 "yY","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>|"
};

/*** German keyboard layout without dead keys */
static const char main_key_DE_nodead[MAIN_LEN][4] =
{
 "^°","1!","2\"","3§","4$","5%","6&","7/{","8([","9)]","0=}","ß?\\","",
 "qQ","wW","eE","rR","tT","zZ","uU","iI","oO","pP","üÜ","+*~",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","öÖ","äÄ","#'",
 "yY","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Swiss German keyboard layout (setxkbmap ch -variant de) */
static const char main_key_SG[MAIN_LEN][4] =
{
 "§°","1+","2\"","3*","4ç","5%","6&","7/","8(","9)","0=","'?","^`",
 "qQ","wW","eE","rR","tT","zZ","uU","iI","oO","pP","üè","¨!",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","öé","äà","$£",
 "yY","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Swiss French keyboard layout (setxkbmap ch -variant fr) */
static const char main_key_SF[MAIN_LEN][4] =
{
 "§°","1+","2\"","3*","4ç","5%","6&","7/","8(","9)","0=","'?","^`",
 "qQ","wW","eE","rR","tT","zZ","uU","iI","oO","pP","èü","¨!",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","éö","àä","$£",
 "yY","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Norwegian keyboard layout (contributed by Ove Kåven) */
static const char main_key_NO[MAIN_LEN][4] =
{
 "|§","1!","2\"@","3#£","4¤$","5%","6&","7/{","8([","9)]","0=}","+?","\\`´",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","åÅ","¨^~",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","øØ","æÆ","'*",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Danish keyboard layout (setxkbmap dk) */
static const char main_key_DA[MAIN_LEN][4] =
{
 "½§","1!","2\"","3#","4¤","5%","6&","7/","8(","9)","0=","+?","´`",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","åÅ","¨^",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","æÆ","øØ","'*",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Swedish keyboard layout (setxkbmap se) */
static const char main_key_SE[MAIN_LEN][4] =
{
 "§½","1!","2\"","3#","4¤","5%","6&","7/","8(","9)","0=","+?","´`",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","åÅ","¨^",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","öÖ","äÄ","'*",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Estonian keyboard layout (setxkbmap ee) */
static const char main_key_ET[MAIN_LEN][4] =
{
 "·~","1!","2\"","3#","4¤","5%","6&","7/","8(","9)","0=","+?","´`",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","üÜ","õÕ",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","öÖ","äÄ","'*",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Canadian French keyboard layout (setxkbmap ca_enhanced) */
static const char main_key_CF[MAIN_LEN][4] =
{
 "#|\\","1!±","2\"@","3/£","4$¢","5%¤","6?¬","7&¦","8*²","9(³","0)¼","-_½","=+¾",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO§","pP¶","^^[","¸¨]",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL",";:~","``{","<>}",
 "zZ","xX","cC","vV","bB","nN","mM",",'-",".","éÉ",
 "«»°"
};

/*** Canadian French keyboard layout (setxkbmap ca -variant fr) */
static const char main_key_CA_fr[MAIN_LEN][4] =
{
 "#|","1!","2\"","3/","4$","5%","6?","7&","8*","9(","0)","-_","=+",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","^^","¸¨",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL",";:","``","<>",
 "zZ","xX","cC","vV","bB","nN","mM",",'",".","éÉ",
 "«»"
};

/*** Canadian keyboard layout (setxkbmap ca) */
static const char main_key_CA[MAIN_LEN][4] =
{
 "/\\","1!¹¡","2@²","3#³£","4$¼¤","5%½","6?¾","7&","8*","9(","0)","-_","=+",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oOøØ","pPþÞ","^¨¨","çÇ~",
 "aAæÆ","sSß§","dDðÐ","fF","gG","hH","jJ","kK","lL",";:´","èÈ","àÀ",
 "zZ","xX","cC¢©","vV","bB","nN","mMµº",",'",".\"·÷","éÉ",
 "ùÙ"
};

/*** Portuguese keyboard layout (setxkbmap pt) */
static const char main_key_PT[MAIN_LEN][4] =
{
 "\\|","1!","2\"","3#","4$","5%","6&","7/","8(","9)","0=","'?","«»",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","+*","´`",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","çÇ","ºª","~^",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Italian keyboard layout (setxkbmap it) */
static const char main_key_IT[MAIN_LEN][4] =
{
 "\\|","1!","2\"","3£","4$","5%","6&","7/","8(","9)","0=","'?","ì^",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","èé","+*",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","òç","à°","ù§",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Finnish keyboard layout (setxkbmap fi) */
static const char main_key_FI[MAIN_LEN][4] =
{
 "§½","1!","2\"","3#","4¤","5%","6&","7/","8(","9)","0=","+?","´`",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","åÅ","¨^",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","öÖ","äÄ","'*",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Bulgarian bds keyboard layout */
static const char main_key_BG_bds[MAIN_LEN][4] =
{
 "`~()","1!","2@2?","3#3+","4$4\"","5%","6^6=","7&7:","8*8/","9(","0)","-_-I","=+.V",
 "qQ,û","wWóÓ","eEåÅ","rRèÈ","tTøØ","yYùÙ","uUêÊ","iIñÑ","oOäÄ","pPçÇ","[{öÖ","]};",
 "aAüÜ","sSÿß","dDàÀ","fFîÎ","gGæÆ","hHãÃ","jJòÒ","kKíÍ","lLâÂ",";:ìÌ","'\"÷×","\\|'Û",
 "zZþÞ","xXéÉ","cCúÚ","vVýÝ","bBôÔ","nNõÕ","mMïÏ",",<ðÐ",".>ëË","/?áÁ",
 "<>" /* the phantom key */
};

/*** Bulgarian phonetic keyboard layout */
static const char main_key_BG_phonetic[MAIN_LEN][4] =
{
 "`~÷×","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQÿß","wWâÂ","eEåÅ","rRðÐ","tTòÒ","yYúÚ","uUóÓ","iIèÈ","oOîÎ","pPïÏ","[{øØ","]}ùÙ",
 "aAàÀ","sSñÑ","dDäÄ","fFôÔ","gGãÃ","hHõÕ","jJéÉ","kKêÊ","lLëË",";:","'\"","\\|þÞ",
 "zZçÇ","xXüÜ","cCöÖ","vVæÆ","bBáÁ","nNíÍ","mMìÌ",",<",".>","/?",
 "<>" /* the phantom key */
};

/*** Belarusian standard keyboard layout (contributed by Hleb Valoska) */
/*** It matches belarusian layout for XKB from Alexander Mikhailian    */
static const char main_key_BY[MAIN_LEN][4] =
{
 "`~£³","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQÊê","wWÃã","eEÕõ","rRËë","tTÅå","yYÎî","uUÇç","iIÛû","oO®¾","pPÚú","[{Èè","]}''",
 "aAÆæ","sSÙù","dD×÷","fFÁá","gGÐð","hHÒò","jJÏï","kKÌì","lLÄä",";:Öö","'\"Üü","\\|/|",
 "zZÑñ","xXÞþ","cCÓó","vVÍí","bB¦¶","nNÔô","mMØø",",<Ââ",".>Àà","/?.,", "<>|¦",
};


/*** Russian keyboard layout (contributed by Pavel Roskin) */
static const char main_key_RU[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQÊê","wWÃã","eEÕõ","rRËë","tTÅå","yYÎî","uUÇç","iIÛû","oOÝý","pPÚú","[{Èè","]}ßÿ",
 "aAÆæ","sSÙù","dD×÷","fFÁá","gGÐð","hHÒò","jJÏï","kKÌì","lLÄä",";:Öö","'\"Üü","\\|",
 "zZÑñ","xXÞþ","cCÓó","vVÍí","bBÉé","nNÔô","mMØø",",<Ââ",".>Àà","/?"
};

/*** Russian keyboard layout (phantom key version) */
static const char main_key_RU_phantom[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQÊê","wWÃã","eEÕõ","rRËë","tTÅå","yYÎî","uUÇç","iIÛû","oOÝý","pPÚú","[{Èè","]}ßÿ",
 "aAÆæ","sSÙù","dD×÷","fFÁá","gGÐð","hHÒò","jJÏï","kKÌì","lLÄä",";:Öö","'\"Üü","\\|",
 "zZÑñ","xXÞþ","cCÓó","vVÍí","bBÉé","nNÔô","mMØø",",<Ââ",".>Àà","/?",
 "<>" /* the phantom key */
};

/*** Russian keyboard layout KOI8-R */
static const char main_key_RU_koi8r[MAIN_LEN][4] =
{
 "()","1!","2\"","3/","4$","5:","6,","7.","8;","9?","0%","-_","=+",
 "Êê","Ãã","Õõ","Ëë","Åå","Îî","Çç","Ûû","Ýý","Úú","Èè","ßÿ",
 "Ææ","Ùù","×÷","Áá","Ðð","Òò","Ïï","Ìì","Ää","Öö","Üü","\\|",
 "Ññ","Þþ","Óó","Íí","Éé","Ôô","Øø","Ââ","Àà","/?",
 "<>" /* the phantom key */
};

/*** Russian keyboard layout cp1251 */
static const char main_key_RU_cp1251[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQéÉ","wWöÖ","eEóÓ","rRêÊ","tTåÅ","yYíÍ","uUãÃ","iIøØ","oOùÙ","pPçÇ","[{õÕ","]}úÚ",
 "aAôÔ","sSûÛ","dDâÂ","fFàÀ","gGïÏ","hHðÐ","jJîÎ","kKëË","lLäÄ",";:æÆ","'\"ýÝ","\\|",
 "zZÿß","xX÷×","cCñÑ","vVìÌ","bBèÈ","nNòÒ","mMüÜ",",<áÁ",".>þÞ","/?",
 "<>" /* the phantom key */
};

/*** Russian phonetic keyboard layout */
static const char main_key_RU_phonetic[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQÑñ","wW×÷","eEÅå","rRÒò","tTÔô","yYÙù","uUÕõ","iIÉé","oOÏï","pPÐð","[{Ûû","]}Ýý",
 "aAÁá","sSÓó","dDÄä","fFÆæ","gGÇç","hHÈè","jJÊê","kKËë","lLÌì",";:","'\"","\\|",
 "zZÚú","xXØø","cCÃã","vVÖö","bBÂâ","nNÎî","mMÍí",",<",".>","/?",
 "<>" /* the phantom key */
};

/*** Ukrainian keyboard layout KOI8-U */
static const char main_key_UA[MAIN_LEN][4] =
{
 "`~­½","1!1!","2@2\"","3#3'","4$4*","5%5:","6^6,","7&7.","8*8;","9(9(","0)0)","-_-_","=+=+",
 "qQÊê","wWÃã","eEÕõ","rRËë","tTÅå","yYÎî","uUÇç","iIÛû","oOÝý","pPÚú","[{Èè","]}§·",
 "aAÆæ","sS¦¶","dD×÷","fFÁá","gGÐð","hHÒò","jJÏï","kKÌì","lLÄä",";:Öö","'\"¤´","\\|\\|",
 "zZÑñ","xXÞþ","cCÓó","vVÍí","bBÉé","nNÔô","mMØø",",<Ââ",".>Àà","/?/?",
 "<>" /* the phantom key */
};

/*** Ukrainian keyboard layout KOI8-U by O. Nykyforchyn */
/***  (as it appears on most of keyboards sold today)   */
static const char main_key_UA_std[MAIN_LEN][4] =
{
 "­½","1!","2\"","3'","4;","5%","6:","7?","8*","9(","0)","-_","=+",
 "Êê","Ãã","Õõ","Ëë","Åå","Îî","Çç","Ûû","Ýý","Úú","Èè","§·",
 "Ææ","¦¶","×÷","Áá","Ðð","Òò","Ïï","Ìì","Ää","Öö","¤´","\\/",
 "Ññ","Þþ","Óó","Íí","Éé","Ôô","Øø","Ââ","Àà",".,",
 "<>" /* the phantom key */
};

/*** Russian keyboard layout KOI8-R (pair to the previous) */
static const char main_key_RU_std[MAIN_LEN][4] =
{
 "£³","1!","2\"","3'","4;","5%","6:","7?","8*","9(","0)","-_","=+",
 "Êê","Ãã","Õõ","Ëë","Åå","Îî","Çç","Ûû","Ýý","Úú","Èè","ßÿ",
 "Ææ","Ùù","×÷","Áá","Ðð","Òò","Ïï","Ìì","Ää","Öö","Üü","\\/",
 "Ññ","Þþ","Óó","Íí","Éé","Ôô","Øø","Ââ","Àà",".,",
 "<>" /* the phantom key */
};

/*** Spanish keyboard layout (setxkbmap es) */
static const char main_key_ES[MAIN_LEN][4] =
{
 "ºª","1!","2\"","3·","4$","5%","6&","7/","8(","9)","0=","'?","¡¿",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","`^","+*",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","ñÑ","´¨","çÇ",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Belgian keyboard layout ***/
static const char main_key_BE[MAIN_LEN][4] =
{
 "","&1|","é2@","\"3#","'4","(5","§6^","è7","!8","ç9{","à0}",")°","-_",
 "aA","zZ","eE¤","rR","tT","yY","uU","iI","oO","pP","^¨[","$*]",
 "qQ","sSß","dD","fF","gG","hH","jJ","kK","lL","mM","ù%´","µ£`",
 "wW","xX","cC","vV","bB","nN",",?",";.",":/","=+~",
 "<>\\"
};

/*** Hungarian keyboard layout (setxkbmap hu) */
static const char main_key_HU[MAIN_LEN][4] =
{
 "0§","1'","2\"","3+","4!","5%","6/","7=","8(","9)","öÖ","üÜ","óÓ",
 "qQ","wW","eE","rR","tT","zZ","uU","iI","oO","pP","õÕ","úÚ",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","éÉ","áÁ","ûÛ",
 "yY","xX","cC","vV","bB","nN","mM",",?",".:","-_",
 "íÍ"
};

/*** Polish (programmer's) keyboard layout ***/
static const char main_key_PL[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&§","8*","9(","0)","-_","=+",
 "qQ","wW","eEêÊ","rR","tT","yY","uU","iI","oOóÓ","pP","[{","]}",
 "aA±¡","sS¶¦","dD","fF","gG","hH","jJ","kK","lL³£",";:","'\"","\\|",
 "zZ¿¯","xX¼¬","cCæÆ","vV","bB","nNñÑ","mM",",<",".>","/?",
 "<>|"
};

/*** Slovenian keyboard layout (setxkbmap si) ***/
static const char main_key_SI[MAIN_LEN][4] =
{
 "¸¨","1!","2\"","3#","4$","5%","6&","7/","8(","9)","0=","'?","+*",
 "qQ","wW","eE","rR","tT","zZ","uU","iI","oO","pP","¹©","ðÐ",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","èÈ","æÆ","¾®",
 "yY","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Serbian keyboard layout (setxkbmap sr) ***/
static const char main_key_SR[MAIN_LEN][4] =
{
 "`~","1!","2\"","3#","4$","5%","6&","7/","8(","9)","0=","'?","+*",
 "©¹","ªº","Åå","Òò","Ôô","Úú","Õõ","Éé","Ïï","Ðð","Ûû","[]",
 "Áá","Óó","Ää","Ææ","Çç","Èè","¨¸","Ëë","Ìì","Þþ","«»","-_",
 "¡±","¯¿","Ãã","×÷","Ââ","Îî","Íí",",;",".:","Öö",
 "<>" /* the phantom key */
};

/*** Serbian keyboard layout (setxkbmap us,sr) ***/
static const char main_key_US_SR[MAIN_LEN][4] =
{
 "`~","1!","2@2\"","3#","4$","5%","6^6&","7&7/","8*8(","9(9)","0)0=","-_'?","=++*",
 "qQ©¹","wWªº","eEÅå","rRÒò","tTÔô","yYÚú","uUÕõ","iIÉé","oOÏï","pPÐð","[{Ûû","]}[]",
 "aAÁá","sSÓó","dDÄä","fFÆæ","gGÇç","hHÈè","jJ¨¸","kKËë","lLÌì",";:Þþ","'\"«»","\\|-_",
 "zZ¡±","xX¯¿","cCÃã","vV×÷","bBÂâ","nNÎî","mMÍí",",<,;",".>.:","/?Öö",
 "<>" /* the phantom key */
};

/*** Croatian keyboard layout specific for me <jelly@srk.fer.hr> ***/
static const char main_key_HR_jelly[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","[{¹©","]}ðÐ",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL",";:èÈ","'\"æÆ","\\|¾®",
 "zZ","xX","cC","vV","bB","nN","mM",",<",".>","/?",
 "<>|"
};

/*** Croatian keyboard layout (setxkbmap hr) ***/
static const char main_key_HR[MAIN_LEN][4] =
{
 "¸¨","1!","2\"","3#","4$","5%","6&","7/","8(","9)","0=","'?","+*",
 "qQ","wW","eE","rR","tT","zZ","uU","iI","oO","pP","¹©","ðÐ",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","èÈ","æÆ","¾®",
 "yY","xX","cC","vV","bB","nN","mM",",;",".:","/?",
 "<>"
};

/*** Japanese 106 keyboard layout ***/
static const char main_key_JA_jp106[MAIN_LEN][4] =
{
 "1!","2\"","3#","4$","5%","6&","7'","8(","9)","0~","-=","^~","\\|",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","@`","[{",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL",";+",":*","]}",
 "zZ","xX","cC","vV","bB","nN","mM",",<",".>","/?",
 "\\_",
};

static const char main_key_JA_macjp[MAIN_LEN][4] =
{
 "1!","2\"","3#","4$","5%","6&","7'","8(","9)","0","-=","^~","\\|",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","@`","[{",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL",";+",":*","]}",
 "zZ","xX","cC","vV","bB","nN","mM",",<",".>","/?",
 "__",
};

/*** Japanese pc98x1 keyboard layout ***/
static const char main_key_JA_pc98x1[MAIN_LEN][4] =
{
 "1!","2\"","3#","4$","5%","6&","7'","8(","9)","0","-=","^`","\\|",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","@~","[{",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL",";+",":*","]}",
 "zZ","xX","cC","vV","bB","nN","mM",",<",".>","/?",
 "\\_",
};

/*** Brazilian ABNT-2 keyboard layout (contributed by Raul Gomes Fernandes) */
static const char main_key_PT_br[MAIN_LEN][4] =
{
 "'\"","1!","2@","3#","4$","5%","6¨","7&","8*","9(","0)","-_","=+",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","´`","[{",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","çÇ","~^","]}",
 "\\|","zZ","xX","cC","vV","bB","nN","mM",",<",".>",";:","/?",
};

/*** Brazilian ABNT-2 keyboard layout with <ALT GR> (contributed by Mauro Carvalho Chehab) */
static const char main_key_PT_br_alt_gr[MAIN_LEN][4] =
{
 "'\"","1!9","2@2","3#3","4$#","5%\"","6(,","7&","8*","9(","0)","-_","=+'",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","4`","[{*",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","gG","~^","]}:",
 "\\|","zZ","xX","cC","vV","bB","nN","mM",",<",".>",";:","/?0"
};

/*** US international keyboard layout (contributed by Gustavo Noronha (kov@debian.org)) */
static const char main_key_US_intl[MAIN_LEN][4] =
{
  "`~", "1!", "2@", "3#", "4$", "5%", "6^", "7&", "8*", "9(", "0)", "-_", "=+", "\\|",
  "qQ", "wW", "eE", "rR", "tT", "yY", "uU", "iI", "oO", "pP", "[{", "]}",
  "aA", "sS", "dD", "fF", "gG", "hH", "jJ", "kK", "lL", ";:", "'\"",
  "zZ", "xX", "cC", "vV", "bB", "nN", "mM", ",<", ".>", "/?"
};

/*** Slovak keyboard layout (see cssk_ibm(sk_qwerty) in xkbsel)
  - dead_abovering replaced with degree - no symbol in iso8859-2
  - brokenbar replaced with bar					*/
static const char main_key_SK[MAIN_LEN][4] =
{
 ";0","+1","µ2","¹3","è4","»5","¾6","ý7","á8","í9","é0","=%","'v",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","ú/","ä(",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","ô\"","§!","ò)",
 "zZ","xX","cC","vV","bB","nN","mM",",?",".:","-_",
 "<>"
};

/*** Czech keyboard layout (setxkbmap cz) */
static const char main_key_CZ[MAIN_LEN][4] =
{
 ";","+1","ì2","¹3","è4","ø5","¾6","ý7","á8","í9","é0","=%","´·",
 "qQ","wW","eE","rR","tT","zZ","uU","iI","oO","pP","ú/",")(",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","ù\"","§!","¨'",
 "yY","xX","cC","vV","bB","nN","mM",",?",".:","-_",
 "\\"
};

/*** Czech keyboard layout (setxkbmap cz_qwerty) */
static const char main_key_CZ_qwerty[MAIN_LEN][4] =
{
 ";","+1","ì2","¹3","è4","ø5","¾6","ý7","á8","í9","é0","=%","´·",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","ú/",")(",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","ù\"","§!","¨'",
 "zZ","xX","cC","vV","bB","nN","mM",",?",".:","-_",
 "\\"
};

/*** Slovak and Czech (programmer's) keyboard layout (see cssk_dual(cs_sk_ucw)) */
static const char main_key_SK_prog[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQäÄ","wWìÌ","eEéÉ","rRøØ","tT»«","yYýÝ","uUùÙ","iIíÍ","oOóÓ","pPöÖ","[{","]}",
 "aAáÁ","sS¹©","dDïÏ","fFëË","gGàÀ","hHúÚ","jJüÜ","kKôÔ","lLµ¥",";:","'\"","\\|",
 "zZ¾®","xX¤","cCèÈ","vVçÇ","bB","nNòÒ","mMåÅ",",<",".>","/?",
 "<>"
};

/*** Czech keyboard layout (see cssk_ibm(cs_qwerty) in xkbsel) */
static const char main_key_CS[MAIN_LEN][4] =
{
 ";","+1","ì2","¹3","è4","ø5","¾6","ý7","á8","í9","é0½)","=%","",
 "qQ\\","wW|","eE","rR","tT","yY","uU","iI","oO","pP","ú/[{",")(]}",
 "aA","sSð","dDÐ","fF[","gG]","hH","jJ","kK³","lL£","ù\"$","§!ß","¨'",
 "zZ>","xX#","cC&","vV@","bB{","nN}","mM",",?<",".:>","-_*",
 "<>\\|"
};

/*** Latin American keyboard layout (contributed by Gabriel Orlando Garcia) */
static const char main_key_LA[MAIN_LEN][4] =
{
 "|°","1!","2\"","3#","4$","5%","6&","7/","8(","9)","0=","'?","¿¡",
 "qQ@","wW","eE","rR","tT","yY","uU","iI","oO","pP","´¨","+*",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","ñÑ","{[^","}]",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","-_",
 "<>"
};

/*** Lithuanian keyboard layout (setxkbmap lt) */
static const char main_key_LT_B[MAIN_LEN][4] =
{
 "`~","àÀ","èÈ","æÆ","ëË","áÁ","ðÐ","øØ","ûÛ","¥(","´)","-_","þÞ","\\|",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","[{","]}",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL",";:","'\"",
 "zZ","xX","cC","vV","bB","nN","mM",",<",".>","/?",
 "ª¬"
};

/*** Turkish keyboard Layout */
static const char main_key_TK[MAIN_LEN][4] =
{
"\"é","1!","2'","3^#","4+$","5%","6&","7/{","8([","9)]","0=}","*?\\","-_",
"qQ@","wW","eE","rR","tT","yY","uU","ýIî","oO","pP","ðÐ","üÜ~",
"aAæ","sSß","dD","fF","gG","hH","jJ","kK","lL","þÞ","iÝ",",;`",
"zZ","xX","cC","vV","bB","nN","mM","öÖ","çÇ",".:"
};

/*** Turkish keyboard layout (setxkbmap tr) */
static const char main_key_TR[MAIN_LEN][4] =
{
"\"\\","1!","2'","3^","4+","5%","6&","7/","8(","9)","0=","*?","-_",
"qQ","wW","eE","rR","tT","yY","uU","\xb9I","oO","pP","\xbb\xab","üÜ",
"aA","sS","dD","fF","gG","hH","jJ","kK","lL","\xba\xaa","i\0",",;",
"zZ","xX","cC","vV","bB","nN","mM","öÖ","çÇ",".:",
"<>"
};

/*** Turkish F keyboard layout (setxkbmap trf) */
static const char main_key_TR_F[MAIN_LEN][4] =
{
"+*","1!","2\"","3^#","4$","5%","6&","7'","8(","9)","0=","/?","-_",
"fF","gG","\xbb\xab","\xb9I","oO","dD","rR","nN","hH","pP","qQ","wW",
"uU","i\0","eE","aA","üÜ","tT","kK","mM","lL","yY","\xba\xaa","xX",
"jJ","öÖ","vV","cC","çÇ","zZ","sS","bB",".:",",;",
"<>"
};

/*** Israelian keyboard layout (setxkbmap us,il) */
static const char main_key_IL[MAIN_LEN][4] =
{
 "`~;","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQ/","wW'","eE÷","rRø","tTà","yYè","uUå","iIï","oOí","pPô","[{","]}",
 "aAù","sSã","dDâ","fFë","gGò","hHé","jJç","kKì","lLê",";:ó","\'\",","\\|",
 "zZæ","xXñ","cCá","vVä","bBð","nNî","mMö",",<ú",".>õ","/?.",
 "<>"
};

/*** Israelian phonetic keyboard layout (setxkbmap us,il_phonetic) */
static const char main_key_IL_phonetic[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQ÷","wWå","eEà","rRø","tTú","yYò","uUå","iIé","oOñ","pPô","[{","]}",
 "aAà","sSù","dDã","fFô","gGâ","hHä","jJé","kKë","lLì",";:","'\"","\\|",
 "zZæ","xXç","cCö","vVå","bBá","nNð","mMî",",<",".>","/?",
 "<>"
};

/*** Israelian Saharon keyboard layout (setxkbmap -symbols "us(pc105)+il_saharon") */
static const char main_key_IL_saharon[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQ÷","wWñ","eE","rRø","tTè","yYã","uU","iI","oO","pPô","[{","]}",
 "aAà","sSå","dDì","fFú","gGâ","hHä","jJù","kKë","lLé",";:","'\"","\\|",
 "zZæ","xXç","cCö","vVò","bBá","nNð","mMî",",<",".>","/?",
 "<>"
};

/*** Greek keyboard layout (contributed by Kriton Kyrimis <kyrimis@cti.gr>)
  Greek characters for "wW" and "sS" are omitted to not produce a mismatch
  message since they have different characters in gr and el XFree86 layouts. */
static const char main_key_EL[MAIN_LEN][4] =
{
 "`~","1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+",
 "qQ;:","wW","eEåÅ","rRñÑ","tTôÔ","yYõÕ","uUèÈ","iIéÉ","oOïÏ","pPðÐ","[{","]}",
 "aAáÁ","sS","dDäÄ","fFöÖ","gGãÃ","hHçÇ","jJîÎ","kKêÊ","lLëË",";:´¨","'\"","\\|",
 "zZæÆ","xX÷×","cCøØ","vVùÙ","bBâÂ","nNíÍ","mMìÌ",",<",".>","/?",
 "<>"
};

/*** Thai (Kedmanee) keyboard layout by Supphachoke Suntiwichaya <mrchoke@opentle.org> */
static const char main_key_th[MAIN_LEN][4] =
{
 "`~_%","1!å+","2@/ñ","3#-ò","4$Àó","5%¶ô","6^ØÙ","7&Öß","8*¤õ","9(µö","0)¨÷","-_¢ø","=+ªù",
 "qQæð","wWä\"","eEÓ®","rR¾±","tTÐ¸","yYÑí","uUÕê","iIÃ³","oO¹Ï","pPÂ­","[{º°","]}Å,",
 "aA¿Ä","sSË¦","dD¡¯","fF´â","gGà¬","hHéç","jJèë","kKÒÉ","lLÊÈ",";:Ç«","\'\"§.","\\|£¥",
 "zZ¼(","xX»)","cCá©","vVÍÎ","bBÚ","nN×ì","mM·?",",<Á²",".>ãÌ","/?½Æ"
}; 

/*** VNC keyboard layout */
static const WORD main_key_scan_vnc[MAIN_LEN] =
{
   0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x1A,0x1B,0x27,0x28,0x29,0x33,0x34,0x35,0x2B,
   0x1E,0x30,0x2E,0x20,0x12,0x21,0x22,0x23,0x17,0x24,0x25,0x26,0x32,0x31,0x18,0x19,0x10,0x13,0x1F,0x14,0x16,0x2F,0x11,0x2D,0x15,0x2C,
   0x56
};

static const WORD main_key_vkey_vnc[MAIN_LEN] =
{
   '1','2','3','4','5','6','7','8','9','0',VK_OEM_MINUS,VK_OEM_PLUS,VK_OEM_4,VK_OEM_6,VK_OEM_1,VK_OEM_7,VK_OEM_3,VK_OEM_COMMA,VK_OEM_PERIOD,VK_OEM_2,VK_OEM_5,
   'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
   VK_OEM_102
};

static const char main_key_vnc[MAIN_LEN][4] =
{
 "1!","2@","3#","4$","5%","6^","7&","8*","9(","0)","-_","=+","[{","]}",";:","'\"","`~",",<",".>","/?","\\|",
 "aA","bB","cC","dD","eE","fF","gG","hH","iI","jJ","kK","lL","mM","nN","oO","pP","qQ","rR","sS","tT","uU","vV","wW","xX","yY","zZ"
};

/*** Dutch keyboard layout (setxkbmap nl) ***/
static const char main_key_NL[MAIN_LEN][4] =
{
 "@§","1!","2\"","3#","4$","5%","6&","7_","8(","9)","0'","/?","°~",
 "qQ","wW","eE","rR","tT","yY","uU","iI","oO","pP","¨~","*|",
 "aA","sS","dD","fF","gG","hH","jJ","kK","lL","+±","'`","<>",
 "zZ","xX","cC","vV","bB","nN","mM",",;",".:","-=",
 "[]"
};



/*** Layout table. Add your keyboard mappings to this list */
static const struct {
    LCID lcid; /* input locale identifier, look for LOCALE_ILANGUAGE
                 in the appropriate dlls/kernel/nls/.nls file */
    const char *comment;
    const char (*key)[MAIN_LEN][4];
    const WORD (*scan)[MAIN_LEN]; /* scan codes mapping */
    const WORD (*vkey)[MAIN_LEN]; /* virtual key codes mapping */
} main_key_tab[]={
 {0x0409, "United States keyboard layout", &main_key_US, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0409, "United States keyboard layout (phantom key version)", &main_key_US_phantom, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0409, "United States keyboard layout (dvorak)", &main_key_US_dvorak, &main_key_scan_dvorak, &main_key_vkey_dvorak},
 {0x0409, "United States International keyboard layout", &main_key_US_intl, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0809, "British keyboard layout", &main_key_UK, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0407, "German keyboard layout", &main_key_DE, &main_key_scan_qwerty, &main_key_vkey_qwertz},
 {0x0407, "German keyboard layout without dead keys", &main_key_DE_nodead, &main_key_scan_qwerty, &main_key_vkey_qwertz},
 {0x0407, "German keyboard layout for logitech desktop pro", &main_key_DE_logitech,  &main_key_scan_qwerty, &main_key_vkey_qwertz},
 {0x0807, "Swiss German keyboard layout", &main_key_SG, &main_key_scan_qwerty, &main_key_vkey_qwertz},
 {0x100c, "Swiss French keyboard layout", &main_key_SF, &main_key_scan_qwerty, &main_key_vkey_qwertz},
 {0x041d, "Swedish keyboard layout", &main_key_SE, &main_key_scan_qwerty, &main_key_vkey_qwerty_v2},
 {0x0425, "Estonian keyboard layout", &main_key_ET, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0414, "Norwegian keyboard layout", &main_key_NO, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0406, "Danish keyboard layout", &main_key_DA, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x040c, "French keyboard layout", &main_key_FR, &main_key_scan_qwerty, &main_key_vkey_azerty},
 {0x0c0c, "Canadian French keyboard layout", &main_key_CF, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0c0c, "Canadian French keyboard layout (CA_fr)", &main_key_CA_fr, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0c0c, "Canadian keyboard layout", &main_key_CA, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x080c, "Belgian keyboard layout", &main_key_BE, &main_key_scan_qwerty, &main_key_vkey_azerty},
 {0x0816, "Portuguese keyboard layout", &main_key_PT, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0416, "Brazilian ABNT-2 keyboard layout", &main_key_PT_br, &main_key_scan_abnt_qwerty, &main_key_vkey_abnt_qwerty},
 {0x0416, "Brazilian ABNT-2 keyboard layout ALT GR", &main_key_PT_br_alt_gr,&main_key_scan_abnt_qwerty, &main_key_vkey_abnt_qwerty},
 {0x040b, "Finnish keyboard layout", &main_key_FI, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0402, "Bulgarian bds keyboard layout", &main_key_BG_bds, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0402, "Bulgarian phonetic keyboard layout", &main_key_BG_phonetic, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0423, "Belarusian keyboard layout", &main_key_BY, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0419, "Russian keyboard layout", &main_key_RU, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0419, "Russian keyboard layout (phantom key version)", &main_key_RU_phantom, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0419, "Russian keyboard layout KOI8-R", &main_key_RU_koi8r, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0419, "Russian keyboard layout cp1251", &main_key_RU_cp1251, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0419, "Russian phonetic keyboard layout", &main_key_RU_phonetic, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0422, "Ukrainian keyboard layout KOI8-U", &main_key_UA, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0422, "Ukrainian keyboard layout (standard)", &main_key_UA_std, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0419, "Russian keyboard layout (standard)", &main_key_RU_std, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x040a, "Spanish keyboard layout", &main_key_ES, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0410, "Italian keyboard layout", &main_key_IT, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x040f, "Icelandic keyboard layout", &main_key_IS, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x040e, "Hungarian keyboard layout", &main_key_HU, &main_key_scan_qwerty, &main_key_vkey_qwertz},
 {0x0415, "Polish (programmer's) keyboard layout", &main_key_PL, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0424, "Slovenian keyboard layout", &main_key_SI, &main_key_scan_qwerty, &main_key_vkey_qwertz},
 {0x0c1a, "Serbian keyboard layout sr", &main_key_SR, &main_key_scan_qwerty, &main_key_vkey_qwerty}, /* LANG_SERBIAN,SUBLANG_SERBIAN_CYRILLIC */
 {0x0c1a, "Serbian keyboard layout us,sr", &main_key_US_SR, &main_key_scan_qwerty, &main_key_vkey_qwerty}, /* LANG_SERBIAN,SUBLANG_SERBIAN_CYRILLIC */
 {0x041a, "Croatian keyboard layout", &main_key_HR, &main_key_scan_qwerty, &main_key_vkey_qwertz},
 {0x041a, "Croatian keyboard layout (specific)", &main_key_HR_jelly, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0411, "Japanese 106 keyboard layout", &main_key_JA_jp106, &main_key_scan_qwerty_jp106, &main_key_vkey_qwerty_jp106},
 {0x0411, "Japanese Mac keyboard layout", &main_key_JA_macjp, &main_key_scan_qwerty_macjp, &main_key_vkey_qwerty_macjp},
 {0x0411, "Japanese pc98x1 keyboard layout", &main_key_JA_pc98x1, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x041b, "Slovak keyboard layout", &main_key_SK, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x041b, "Slovak and Czech keyboard layout without dead keys", &main_key_SK_prog, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0405, "Czech keyboard layout", &main_key_CS, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0405, "Czech keyboard layout cz", &main_key_CZ, &main_key_scan_qwerty, &main_key_vkey_qwertz},
 {0x0405, "Czech keyboard layout cz_qwerty", &main_key_CZ_qwerty, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x040a, "Latin American keyboard layout", &main_key_LA, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0427, "Lithuanian (Baltic) keyboard layout", &main_key_LT_B, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x041f, "Turkish keyboard layout", &main_key_TK, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x041f, "Turkish keyboard layout tr", &main_key_TR, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x041f, "Turkish keyboard layout trf", &main_key_TR_F, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x040d, "Israelian keyboard layout", &main_key_IL, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x040d, "Israelian phonetic keyboard layout", &main_key_IL_phonetic, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x040d, "Israelian Saharon keyboard layout", &main_key_IL_saharon, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0409, "VNC keyboard layout", &main_key_vnc, &main_key_scan_vnc, &main_key_vkey_vnc},
 {0x0408, "Greek keyboard layout", &main_key_EL, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x041e, "Thai (Kedmanee)  keyboard layout", &main_key_th, &main_key_scan_qwerty, &main_key_vkey_qwerty},
 {0x0413, "Dutch keyboard layout", &main_key_NL, &main_key_scan_qwerty, &main_key_vkey_qwerty},

 {0, NULL, NULL, NULL, NULL} /* sentinel */
};
static unsigned kbd_layout=0; /* index into above table of layouts */

/* maybe more of these scancodes should be extended? */
                /* extended must be set for ALT_R, CTRL_R,
                   INS, DEL, HOME, END, PAGE_UP, PAGE_DOWN, ARROW keys,
                   keypad / and keypad ENTER (SDK 3.1 Vol.3 p 138) */
                /* FIXME should we set extended bit for NumLock ? My
                 * Windows does ... DF */
                /* Yes, to distinguish based on scan codes, also
                   for PrtScn key ... GA */

static const WORD nonchar_key_vkey[256] =
{
    /* unused */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FF00 */
    /* special keys */
    VK_BACK, VK_TAB, 0, VK_CLEAR, 0, VK_RETURN, 0, 0,           /* FF08 */
    0, 0, 0, VK_PAUSE, VK_SCROLL, 0, 0, 0,                      /* FF10 */
    0, 0, 0, VK_ESCAPE, 0, 0, 0, 0,                             /* FF18 */
    /* unused */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FF20 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FF28 */
    0, VK_HANGUL, 0, 0, VK_HANJA, 0, 0, 0,                      /* FF30 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FF38 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FF40 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FF48 */
    /* cursor keys */
    VK_HOME, VK_LEFT, VK_UP, VK_RIGHT,                          /* FF50 */
    VK_DOWN, VK_PRIOR, VK_NEXT, VK_END,
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FF58 */
    /* misc keys */
    VK_SELECT, VK_SNAPSHOT, VK_EXECUTE, VK_INSERT, 0,0,0, VK_APPS, /* FF60 */
    0, VK_CANCEL, VK_HELP, VK_CANCEL, 0, 0, 0, 0,               /* FF68 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FF70 */
    /* keypad keys */
    0, 0, 0, 0, 0, 0, 0, VK_NUMLOCK,                            /* FF78 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FF80 */
    0, 0, 0, 0, 0, VK_RETURN, 0, 0,                             /* FF88 */
    0, 0, 0, 0, 0, VK_HOME, VK_LEFT, VK_UP,                     /* FF90 */
    VK_RIGHT, VK_DOWN, VK_PRIOR, VK_NEXT,                       /* FF98 */
    VK_END, VK_CLEAR, VK_INSERT, VK_DELETE,
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FFA0 */
    0, 0, VK_MULTIPLY, VK_ADD,                                  /* FFA8 */
    VK_SEPARATOR, VK_SUBTRACT, VK_DECIMAL, VK_DIVIDE,
    VK_NUMPAD0, VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3,             /* FFB0 */
    VK_NUMPAD4, VK_NUMPAD5, VK_NUMPAD6, VK_NUMPAD7,
    VK_NUMPAD8, VK_NUMPAD9, 0, 0, 0, VK_OEM_NEC_EQUAL,          /* FFB8 */
    /* function keys */
    VK_F1, VK_F2,
    VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9, VK_F10,    /* FFC0 */
    VK_F11, VK_F12, VK_F13, VK_F14, VK_F15, VK_F16, 0, 0,       /* FFC8 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FFD0 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FFD8 */
    /* modifier keys */
    0, VK_LSHIFT, VK_RSHIFT, VK_LCONTROL,                       /* FFE0 */
    VK_RCONTROL, VK_CAPITAL, 0, VK_MENU,
    VK_MENU, VK_LMENU, VK_RMENU, VK_LWIN, VK_RWIN, 0, 0, 0,     /* FFE8 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* FFF0 */
    0, 0, 0, 0, 0, 0, 0, VK_DELETE                              /* FFF8 */
};

static const WORD nonchar_key_scan[256] =
{
    /* unused */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FF00 */
    /* special keys */
    0x0E, 0x0F, 0x00, /*?*/ 0, 0x00, 0x1C, 0x00, 0x00,           /* FF08 */
    0x00, 0x00, 0x00, 0x45, 0x46, 0x00, 0x00, 0x00,              /* FF10 */
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,              /* FF18 */
    /* unused */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FF20 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FF28 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FF30 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FF38 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FF40 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FF48 */
    /* cursor keys */
    0x147, 0x14B, 0x148, 0x14D, 0x150, 0x149, 0x151, 0x14F,      /* FF50 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FF58 */
    /* misc keys */
    /*?*/ 0, 0x137, /*?*/ 0, 0x152, 0x00, 0x00, 0x00, 0x15D,     /* FF60 */
    /*?*/ 0, /*?*/ 0, 0x38, 0x146, 0x00, 0x00, 0x00, 0x00,       /* FF68 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FF70 */
    /* keypad keys */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x145,             /* FF78 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FF80 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x11C, 0x00, 0x00,             /* FF88 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x4B, 0x48,              /* FF90 */
    0x4D, 0x50, 0x49, 0x51, 0x4F, 0x4C, 0x52, 0x53,              /* FF98 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FFA0 */
    0x00, 0x00, 0x37, 0x4E, 0x53, 0x4A, 0x53, 0x135,             /* FFA8 */
    0x52, 0x4F, 0x50, 0x51, 0x4B, 0x4C, 0x4D, 0x47,              /* FFB0 */
    0x48, 0x49, 0x00, 0x00, 0x00, 0x00,                          /* FFB8 */
    /* function keys */
    0x3B, 0x3C,
    0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44,              /* FFC0 */
    0x57, 0x58, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FFC8 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FFD0 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FFD8 */
    /* modifier keys */
    0x00, 0x2A, 0x136, 0x1D, 0x11D, 0x3A, 0x00, 0x38,            /* FFE0 */
    0x138, 0x38, 0x138, 0x15b, 0x15c, 0x00, 0x00, 0x00,          /* FFE8 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,              /* FFF0 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x153              /* FFF8 */
};

static const WORD xfree86_vendor_key_vkey[256] =
{
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF00 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF08 */
    0, VK_VOLUME_DOWN, VK_VOLUME_MUTE, VK_VOLUME_UP,            /* 1008FF10 */
    VK_MEDIA_PLAY_PAUSE, VK_MEDIA_STOP,
    VK_MEDIA_PREV_TRACK, VK_MEDIA_NEXT_TRACK,
    0, VK_LAUNCH_MAIL, 0, VK_BROWSER_SEARCH,                    /* 1008FF18 */
    0, 0, 0, VK_BROWSER_HOME,
    0, 0, 0, 0, 0, 0, VK_BROWSER_BACK, VK_BROWSER_FORWARD,      /* 1008FF20 */
    VK_BROWSER_STOP, VK_BROWSER_REFRESH, 0, 0, 0, 0, 0, 0,      /* 1008FF28 */
    VK_BROWSER_FAVORITES, 0, VK_LAUNCH_MEDIA_SELECT, 0,         /* 1008FF30 */
    0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF38 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF40 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF48 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF50 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF58 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF60 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF68 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF70 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF78 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF80 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF88 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF90 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FF98 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FFA0 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FFA8 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FFB0 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FFB8 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FFC0 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FFC8 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FFD0 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FFD8 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FFE0 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FFE8 */
    0, 0, 0, 0, 0, 0, 0, 0,                                     /* 1008FFF0 */
    0, 0, 0, 0, 0, 0, 0, 0                                      /* 1008FFF8 */
};

/* Returns the Windows virtual key code associated with the X event <e> */
/* x11 lock must be held */
static WORD EVENT_event_to_vkey( XIC xic, XKeyEvent *e)
{
    KeySym keysym = 0;
    Status status;
    char buf[24];

    /* Clients should pass only KeyPress events to XmbLookupString */
    if (xic && e->type == KeyPress)
        XmbLookupString(xic, e, buf, sizeof(buf), &keysym, &status);
    else
        XLookupString(e, buf, sizeof(buf), &keysym, NULL);

    if ((e->state & NumLockMask) &&
        (keysym == XK_KP_Separator || keysym == XK_KP_Decimal ||
         (keysym >= XK_KP_0 && keysym <= XK_KP_9)))
        /* Only the Keypad keys 0-9 and . send different keysyms
         * depending on the NumLock state */
        return nonchar_key_vkey[keysym & 0xFF];

    TRACE_(key)("e->keycode = %x\n", e->keycode);

    return keyc2vkey[e->keycode];
}


/***********************************************************************
 *           X11DRV_send_keyboard_input
 */
void X11DRV_send_keyboard_input( WORD wVk, WORD wScan, DWORD event_flags, DWORD time,
                                 DWORD dwExtraInfo, UINT injected_flags )
{
    UINT message;
    KBDLLHOOKSTRUCT hook;
    WORD flags, wVkStripped, wVkL, wVkR, vk_hook = wVk;

    wVk = LOBYTE(wVk);
    flags = LOBYTE(wScan);

    if (event_flags & KEYEVENTF_EXTENDEDKEY) flags |= KF_EXTENDED;
    /* FIXME: set KF_DLGMODE and KF_MENUMODE when needed */

    /* strip left/right for menu, control, shift */
    switch (wVk)
    {
    case VK_MENU:
    case VK_LMENU:
    case VK_RMENU:
        wVk = (event_flags & KEYEVENTF_EXTENDEDKEY) ? VK_RMENU : VK_LMENU;
        wVkStripped = VK_MENU;
        wVkL = VK_LMENU;
        wVkR = VK_RMENU;
        break;
    case VK_CONTROL:
    case VK_LCONTROL:
    case VK_RCONTROL:
        wVk = (event_flags & KEYEVENTF_EXTENDEDKEY) ? VK_RCONTROL : VK_LCONTROL;
        wVkStripped = VK_CONTROL;
        wVkL = VK_LCONTROL;
        wVkR = VK_RCONTROL;
        break;
    case VK_SHIFT:
    case VK_LSHIFT:
    case VK_RSHIFT:
        wVk = (event_flags & KEYEVENTF_EXTENDEDKEY) ? VK_RSHIFT : VK_LSHIFT;
        wVkStripped = VK_SHIFT;
        wVkL = VK_LSHIFT;
        wVkR = VK_RSHIFT;
        break;
    default:
        wVkStripped = wVkL = wVkR = wVk;
    }

    if (event_flags & KEYEVENTF_KEYUP)
    {
        message = WM_KEYUP;
        if ((key_state_table[VK_MENU] & 0x80) &&
            ((wVkStripped == VK_MENU) || (wVkStripped == VK_CONTROL)
             || !(key_state_table[VK_CONTROL] & 0x80)))
        {
            if( TrackSysKey == VK_MENU || /* <ALT>-down/<ALT>-up sequence */
                (wVkStripped != VK_MENU)) /* <ALT>-down...<something else>-up */
                message = WM_SYSKEYUP;
            TrackSysKey = 0;
        }
        flags |= KF_REPEAT | KF_UP;
    }
    else
    {
        message = WM_KEYDOWN;
        if ((key_state_table[VK_MENU] & 0x80 || wVkStripped == VK_MENU) &&
            !(key_state_table[VK_CONTROL] & 0x80 || wVkStripped == VK_CONTROL))
        {
            message = WM_SYSKEYDOWN;
            TrackSysKey = wVkStripped;
        }
        if (key_state_table[wVk] & 0x80) flags |= KF_REPEAT;
    }

    TRACE_(key)(" wParam=%04x, lParam=%08lx, InputKeyState=%x\n",
                wVk, MAKELPARAM( 1, flags ), key_state_table[wVk] );

    /* Hook gets whatever key was sent. */
    hook.vkCode      = vk_hook;
    hook.scanCode    = wScan;
    hook.flags       = (flags >> 8) | injected_flags;
    hook.time        = time;
    hook.dwExtraInfo = dwExtraInfo;
    if (HOOK_CallHooks( WH_KEYBOARD_LL, HC_ACTION, message, (LPARAM)&hook, TRUE )) return;

    if (event_flags & KEYEVENTF_KEYUP)
    {
        key_state_table[wVk] &= ~0x80;
        key_state_table[wVkStripped] = key_state_table[wVkL] | key_state_table[wVkR];
    }
    else
    {
        if (!(key_state_table[wVk] & 0x80)) key_state_table[wVk] ^= 0x01;
        key_state_table[wVk] |= 0xc0;
        key_state_table[wVkStripped] = key_state_table[wVkL] | key_state_table[wVkR];
    }

    if (key_state_table[VK_MENU] & 0x80) flags |= KF_ALTDOWN;

    if (wVkStripped == VK_SHIFT) flags &= ~KF_EXTENDED;

    SERVER_START_REQ( send_hardware_message )
    {
        req->id       = (injected_flags & LLKHF_INJECTED) ? 0 : GetCurrentThreadId();
        req->win      = 0;
        req->msg      = message;
        req->wparam   = wVk;
        req->lparam   = MAKELPARAM( 1 /* repeat count */, flags );
        req->x        = cursor_pos.x;
        req->y        = cursor_pos.y;
        req->time     = time;
        req->info     = dwExtraInfo;
        wine_server_call( req );
    }
    SERVER_END_REQ;
}


/***********************************************************************
 *           KEYBOARD_UpdateOneState
 *
 * Updates internal state for <vkey>, depending on key <state> under X
 *
 */
static inline void KEYBOARD_UpdateOneState ( WORD vkey, WORD scan, int state, DWORD time )
{
    /* Do something if internal table state != X state for keycode */
    if (((key_state_table[vkey & 0xff] & 0x80)!=0) != state)
    {
        DWORD flags = vkey & 0x100 ? KEYEVENTF_EXTENDEDKEY : 0;

        if (!state) flags |= KEYEVENTF_KEYUP;

        TRACE("Adjusting state for vkey %#.2x. State before %#.2x\n",
              vkey, key_state_table[vkey & 0xff]);

        /* Fake key being pressed inside wine */
        X11DRV_send_keyboard_input( vkey & 0xff, scan & 0xff, flags, time, 0, 0 );

        TRACE("State after %#.2x\n", key_state_table[vkey & 0xff]);
    }
}

/***********************************************************************
 *           X11DRV_KeymapNotify
 *
 * Update modifiers state (Ctrl, Alt, Shift) when window is activated.
 *
 * This handles the case where one uses Ctrl+... Alt+... or Shift+.. to switch
 * from wine to another application and back.
 * Toggle keys are handled in HandleEvent.
 */
void X11DRV_KeymapNotify( HWND hwnd, XEvent *event )
{
    int i, j;
    DWORD time = GetCurrentTime();

    /* the minimum keycode is always greater or equal to 8, so we can
     * skip the first 8 values, hence start at 1
     */
    for (i = 1; i < 32; i++)
    {
        for (j = 0; j < 8; j++)
        {
            WORD vkey = keyc2vkey[(i * 8) + j];
            WORD scan = keyc2scan[(i * 8) + j];
            int state = (event->xkeymap.key_vector[i] & (1<<j)) != 0;

            switch(vkey & 0xff)
            {
            case VK_LMENU:
            case VK_RMENU:
            case VK_LCONTROL:
            case VK_RCONTROL:
            case VK_LSHIFT:
            case VK_RSHIFT:
                KEYBOARD_UpdateOneState( vkey, scan, state, time );
                break;
            }
        }
    }
}

static void update_lock_state(BYTE vkey, WORD scan, DWORD time)
{
    DWORD flags = vkey == VK_NUMLOCK ? KEYEVENTF_EXTENDEDKEY : 0;

    if (key_state_table[vkey] & 0x80) flags ^= KEYEVENTF_KEYUP;

    X11DRV_send_keyboard_input( vkey, scan, flags, time, 0, 0 );
    X11DRV_send_keyboard_input( vkey, scan, flags ^ KEYEVENTF_KEYUP, time, 0, 0 );
}

/***********************************************************************
 *           X11DRV_KeyEvent
 *
 * Handle a X key event
 */
void X11DRV_KeyEvent( HWND hwnd, XEvent *xev )
{
    XKeyEvent *event = &xev->xkey;
    char buf[24];
    char *Str = buf;
    KeySym keysym = 0;
    WORD vkey = 0, bScan;
    DWORD dwFlags;
    int ascii_chars;
    XIC xic = X11DRV_get_ic( hwnd );
    DWORD event_time = EVENT_x11_time_to_win32_time(event->time);
    Status status = 0;

    TRACE_(key)("type %d, window %lx, state 0x%04x, keycode 0x%04x\n",
		event->type, event->window, event->state, event->keycode);

    wine_tsx11_lock();
    /* Clients should pass only KeyPress events to XmbLookupString */
    if (xic && event->type == KeyPress)
    {
        ascii_chars = XmbLookupString(xic, event, buf, sizeof(buf), &keysym, &status);
        TRACE("XmbLookupString needs %i byte(s)\n", ascii_chars);
        if (status == XBufferOverflow)
        {
            Str = HeapAlloc(GetProcessHeap(), 0, ascii_chars);
            if (Str == NULL)
            {
                ERR("Failed to allocate memory!\n");
                wine_tsx11_unlock();
                return;
            }
            ascii_chars = XmbLookupString(xic, event, Str, ascii_chars, &keysym, &status);
        }
    }
    else
        ascii_chars = XLookupString(event, buf, sizeof(buf), &keysym, NULL);
    wine_tsx11_unlock();

    TRACE_(key)("nbyte = %d, status 0x%x\n", ascii_chars, status);

    if (status == XLookupChars)
    {
        X11DRV_XIMLookupChars( Str, ascii_chars );
        if (buf != Str)
            HeapFree(GetProcessHeap(), 0, Str);
        return;
    }

    /* If XKB extensions are used, the state mask for AltGr will use the group
       index instead of the modifier mask. The group index is set in bits
       13-14 of the state field in the XKeyEvent structure. So if AltGr is
       pressed, look if the group index is different than 0. From XKB
       extension documentation, the group index for AltGr should be 2
       (event->state = 0x2000). It's probably better to not assume a
       predefined group index and find it dynamically

       Ref: X Keyboard Extension: Library specification (section 14.1.1 and 17.1.1) */
    /* Save also all possible modifier states. */
    AltGrMask = event->state & (0x6000 | Mod1Mask | Mod2Mask | Mod3Mask | Mod4Mask | Mod5Mask);

    if (TRACE_ON(key)){
	const char *ksname;

        wine_tsx11_lock();
        ksname = XKeysymToString(keysym);
        wine_tsx11_unlock();
	if (!ksname)
	  ksname = "No Name";
	TRACE_(key)("%s : keysym=%lX (%s), # of chars=%d / %s\n",
                    (event->type == KeyPress) ? "KeyPress" : "KeyRelease",
                    keysym, ksname, ascii_chars, debugstr_an(Str, ascii_chars));
    }
    if (buf != Str)
        HeapFree(GetProcessHeap(), 0, Str);

    wine_tsx11_lock();
    vkey = EVENT_event_to_vkey(xic,event);
    /* X returns keycode 0 for composed characters */
    if (!vkey && ascii_chars) vkey = VK_NONAME;
    wine_tsx11_unlock();

    TRACE_(key)("keycode 0x%x converted to vkey 0x%x\n",
                event->keycode, vkey);

    if (!vkey) return;

    dwFlags = 0;
    if ( event->type == KeyRelease ) dwFlags |= KEYEVENTF_KEYUP;
    if ( vkey & 0x100 )              dwFlags |= KEYEVENTF_EXTENDEDKEY;


    /* Note: X sets the below states on key down and clears them on key up.
       Windows triggers them on key down. */

    /* Adjust the CAPSLOCK state if it has been changed outside wine */
    if (!(key_state_table[VK_CAPITAL] & 0x01) != !(event->state & LockMask) &&
        vkey != VK_CAPITAL)
    {
        TRACE("Adjusting CapsLock state (%#.2x)\n", key_state_table[VK_CAPITAL]);
        update_lock_state(VK_CAPITAL, 0x3A, event_time);
    }

    /* Adjust the NUMLOCK state if it has been changed outside wine */
    if (!(key_state_table[VK_NUMLOCK] & 0x01) != !(event->state & NumLockMask) &&
        (vkey & 0xff) != VK_NUMLOCK)
    {
        TRACE("Adjusting NumLock state (%#.2x)\n", key_state_table[VK_NUMLOCK]);
        update_lock_state(VK_NUMLOCK, 0x45, event_time);
    }

    /* Adjust the SCROLLLOCK state if it has been changed outside wine */
    if (!(key_state_table[VK_SCROLL] & 0x01) != !(event->state & ScrollLockMask) &&
        vkey != VK_SCROLL)
    {
        TRACE("Adjusting ScrLock state (%#.2x)\n", key_state_table[VK_SCROLL]);
        update_lock_state(VK_SCROLL, 0x46, event_time);
    }

    bScan = keyc2scan[event->keycode] & 0xFF;
    TRACE_(key)("bScan = 0x%02x.\n", bScan);

    X11DRV_send_keyboard_input( vkey & 0xff, bScan, dwFlags, event_time, 0, 0 );
}

/**********************************************************************
 *		X11DRV_KEYBOARD_DetectLayout
 *
 * Called from X11DRV_InitKeyboard
 *  This routine walks through the defined keyboard layouts and selects
 *  whichever matches most closely.
 * X11 lock must be held.
 */
static void
X11DRV_KEYBOARD_DetectLayout( Display *display )
{
  unsigned current, match, mismatch, seq, i, syms;
  int score, keyc, key, pkey, ok;
  KeySym keysym = 0;
  const char (*lkey)[MAIN_LEN][4];
  unsigned max_seq = 0;
  int max_score = 0, ismatch = 0;
  char ckey[256][4];

  syms = keysyms_per_keycode;
  if (syms > 4) {
    WARN("%d keysyms per keycode not supported, set to 4\n", syms);
    syms = 4;
  }

  memset( ckey, 0, sizeof(ckey) );
  for (keyc = min_keycode; keyc <= max_keycode; keyc++) {
      /* get data for keycode from X server */
      for (i = 0; i < syms; i++) {
        if (!(keysym = XKeycodeToKeysym (display, keyc, i))) continue;
	/* Allow both one-byte and two-byte national keysyms */
	if ((keysym < 0x8000) && (keysym != ' '))
        {
#ifdef HAVE_XKB
            if (!use_xkb || !XkbTranslateKeySym(display, &keysym, 0, &ckey[keyc][i], 1, NULL))
#endif
            {
                TRACE("XKB could not translate keysym %ld\n", keysym);
                /* FIXME: query what keysym is used as Mode_switch, fill XKeyEvent
                 * with appropriate ShiftMask and Mode_switch, use XLookupString
                 * to get character in the local encoding.
                 */
                ckey[keyc][i] = keysym & 0xFF;
            }
        }
	else {
	  ckey[keyc][i] = KEYBOARD_MapDeadKeysym(keysym);
	}
      }
  }

  for (current = 0; main_key_tab[current].comment; current++) {
    TRACE("Attempting to match against \"%s\"\n", main_key_tab[current].comment);
    match = 0;
    mismatch = 0;
    score = 0;
    seq = 0;
    lkey = main_key_tab[current].key;
    pkey = -1;
    for (keyc = min_keycode; keyc <= max_keycode; keyc++) {
      if (ckey[keyc][0]) {
	/* search for a match in layout table */
	/* right now, we just find an absolute match for defined positions */
	/* (undefined positions are ignored, so if it's defined as "3#" in */
	/* the table, it's okay that the X server has "3#Â£", for example) */
	/* however, the score will be higher for longer matches */
	for (key = 0; key < MAIN_LEN; key++) {
	  for (ok = 0, i = 0; (ok >= 0) && (i < syms); i++) {
	    if ((*lkey)[key][i] && ((*lkey)[key][i] == ckey[keyc][i]))
	      ok++;
	    if ((*lkey)[key][i] && ((*lkey)[key][i] != ckey[keyc][i]))
	      ok = -1;
	  }
	  if (ok > 0) {
	    score += ok;
	    break;
	  }
	}
	/* count the matches and mismatches */
	if (ok > 0) {
	  match++;
	  /* and how much the keycode order matches */
	  if (key > pkey) seq++;
	  pkey = key;
	} else {
          /* print spaces instead of \0's */
          char str[5];
          for (i = 0; i < 4; i++) str[i] = ckey[keyc][i] ? ckey[keyc][i] : ' ';
          str[4] = 0;
          TRACE_(key)("mismatch for keysym 0x%04lX, keycode %d, got %s\n", keysym, keyc, str );
          mismatch++;
          score -= syms;
	}
      }
    }
    TRACE("matches=%d, mismatches=%d, seq=%d, score=%d\n",
	   match, mismatch, seq, score);
    if ((score > max_score) ||
	((score == max_score) && (seq > max_seq))) {
      /* best match so far */
      kbd_layout = current;
      max_score = score;
      max_seq = seq;
      ismatch = !mismatch;
    }
  }
  /* we're done, report results if necessary */
  if (!ismatch)
    WARN("Using closest match (%s) for scan/virtual codes mapping.\n",
        main_key_tab[kbd_layout].comment);

  TRACE("detected layout is \"%s\"\n", main_key_tab[kbd_layout].comment);
}

static HKL get_locale_kbd_layout(void)
{
    ULONG_PTR layout;
    LANGID langid;

    /* FIXME:
     *
     * layout = main_key_tab[kbd_layout].lcid;
     *
     * Winword uses return value of GetKeyboardLayout as a codepage
     * to translate ANSI keyboard messages to unicode. But we have
     * a problem with it: for instance Polish keyboard layout is
     * identical to the US one, and therefore instead of the Polish
     * locale id we return the US one.
     */

    layout = GetUserDefaultLCID();

    /*
     * Microsoft Office expects this value to be something specific
     * for Japanese and Korean Windows with an IME the value is 0xe001
     * We should probably check to see if an IME exists and if so then
     * set this word properly.
     */
    langid = PRIMARYLANGID(LANGIDFROMLCID(layout));
    if (langid == LANG_CHINESE || langid == LANG_JAPANESE || langid == LANG_KOREAN)
        layout |= 0xe001 << 16; /* IME */
    else
        layout |= layout << 16;

    return (HKL)layout;
}

/***********************************************************************
 *     GetKeyboardLayoutName (X11DRV.@)
 */
BOOL CDECL X11DRV_GetKeyboardLayoutName(LPWSTR name)
{
    static const WCHAR formatW[] = {'%','0','8','l','x',0};
    DWORD layout;
    LANGID langid;

    layout = main_key_tab[kbd_layout].lcid;
    /* see comment for get_locale_kbd_layout */
    langid = PRIMARYLANGID(LANGIDFROMLCID(layout));
    if (langid == LANG_CHINESE || langid == LANG_JAPANESE || langid == LANG_KOREAN)
        layout |= 0xe001 << 16; /* FIXME */

    sprintfW(name, formatW, layout);
    TRACE("returning %s\n", debugstr_w(name));
    return TRUE;
}

static void set_kbd_layout_preload_key(void)
{
    static const WCHAR preload[] =
        {'K','e','y','b','o','a','r','d',' ','L','a','y','o','u','t','\\','P','r','e','l','o','a','d',0};
    static const WCHAR one[] = {'1',0};

    HKEY hkey;
    WCHAR layout[KL_NAMELENGTH];

    if (RegCreateKeyExW(HKEY_CURRENT_USER, preload, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey, NULL))
        return;

    if (!RegQueryValueExW(hkey, one, NULL, NULL, NULL, NULL))
    {
        RegCloseKey(hkey);
        return;
    }
    if (X11DRV_GetKeyboardLayoutName(layout))
        RegSetValueExW(hkey, one, 0, REG_SZ, (const BYTE *)layout, sizeof(layout));

    RegCloseKey(hkey);
}

/**********************************************************************
 *		X11DRV_InitKeyboard
 */
void X11DRV_InitKeyboard( Display *display )
{
    KeySym *ksp;
    XModifierKeymap *mmp;
    KeySym keysym;
    KeyCode *kcp;
    XKeyEvent e2;
    WORD scan, vkey, OEMvkey;
    int keyc, i, keyn, syms;
    char ckey[4]={0,0,0,0};
    const char (*lkey)[MAIN_LEN][4];
    char vkey_used[256] = { 0 };

    set_kbd_layout_preload_key();

    wine_tsx11_lock();
    XDisplayKeycodes(display, &min_keycode, &max_keycode);
    ksp = XGetKeyboardMapping(display, min_keycode,
                              max_keycode + 1 - min_keycode, &keysyms_per_keycode);
    /* We are only interested in keysyms_per_keycode.
       There is no need to hold a local copy of the keysyms table */
    XFree(ksp);

    mmp = XGetModifierMapping(display);
    kcp = mmp->modifiermap;
    for (i = 0; i < 8; i += 1) /* There are 8 modifier keys */
    {
        int j;

        for (j = 0; j < mmp->max_keypermod; j += 1, kcp += 1)
	    if (*kcp)
            {
		int k;

		for (k = 0; k < keysyms_per_keycode; k += 1)
                    if (XKeycodeToKeysym(display, *kcp, k) == XK_Num_Lock)
		    {
                        NumLockMask = 1 << i;
                        TRACE_(key)("NumLockMask is %x\n", NumLockMask);
		    }
                    else if (XKeycodeToKeysym(display, *kcp, k) == XK_Scroll_Lock)
		    {
                        ScrollLockMask = 1 << i;
                        TRACE_(key)("ScrollLockMask is %x\n", ScrollLockMask);
		    }
            }
    }
    XFreeModifiermap(mmp);

    /* Detect the keyboard layout */
    X11DRV_KEYBOARD_DetectLayout( display );
    lkey = main_key_tab[kbd_layout].key;
    syms = (keysyms_per_keycode > 4) ? 4 : keysyms_per_keycode;

    /* Now build two conversion arrays :
     * keycode -> vkey + scancode + extended
     * vkey + extended -> keycode */

    e2.display = display;
    e2.state = 0;

    OEMvkey = VK_OEM_8; /* next is available.  */
    memset(keyc2vkey, 0, sizeof(keyc2vkey));
    for (keyc = min_keycode; keyc <= max_keycode; keyc++)
    {
        char buf[30];
        int have_chars;

        keysym = 0;
        e2.keycode = (KeyCode)keyc;
        have_chars = XLookupString(&e2, buf, sizeof(buf), &keysym, NULL);
        vkey = 0; scan = 0;
        if (keysym)  /* otherwise, keycode not used */
        {
            if ((keysym >> 8) == 0xFF)         /* non-character key */
            {
                vkey = nonchar_key_vkey[keysym & 0xff];
                scan = nonchar_key_scan[keysym & 0xff];
		/* set extended bit when necessary */
		if (scan & 0x100) vkey |= 0x100;
            } else if ((keysym >> 8) == 0x1008FF) { /* XFree86 vendor keys */
                vkey = xfree86_vendor_key_vkey[keysym & 0xff];
                /* All vendor keys are extended with a scan code of 0 per testing on WinXP */
                scan = 0x100;
		vkey |= 0x100;
            } else if (keysym == 0x20) {                 /* Spacebar */
	        vkey = VK_SPACE;
		scan = 0x39;
	    } else if (have_chars) {
	      /* we seem to need to search the layout-dependent scancodes */
	      int maxlen=0,maxval=-1,ok;
	      for (i=0; i<syms; i++) {
		keysym = XKeycodeToKeysym(display, keyc, i);
		if ((keysym<0x8000) && (keysym!=' '))
                {
#ifdef HAVE_XKB
                    if (!use_xkb || !XkbTranslateKeySym(display, &keysym, 0, &ckey[i], 1, NULL))
#endif
                    {
                        /* FIXME: query what keysym is used as Mode_switch, fill XKeyEvent
                         * with appropriate ShiftMask and Mode_switch, use XLookupString
                         * to get character in the local encoding.
                         */
                        ckey[i] = keysym & 0xFF;
                    }
		} else {
		  ckey[i] = KEYBOARD_MapDeadKeysym(keysym);
		}
	      }
	      /* find key with longest match streak */
	      for (keyn=0; keyn<MAIN_LEN; keyn++) {
		for (ok=(*lkey)[keyn][i=0]; ok&&(i<4); i++)
		  if ((*lkey)[keyn][i] && (*lkey)[keyn][i]!=ckey[i]) ok=0;
		if (!ok) i--; /* we overshot */
		if (ok||(i>maxlen)) {
		  maxlen=i; maxval=keyn;
		}
		if (ok) break;
	      }
	      if (maxval>=0) {
		/* got it */
		const WORD (*lscan)[MAIN_LEN] = main_key_tab[kbd_layout].scan;
		const WORD (*lvkey)[MAIN_LEN] = main_key_tab[kbd_layout].vkey;
		scan = (*lscan)[maxval];
		vkey = (*lvkey)[maxval];
	      }
	    }
        }
        TRACE("keycode %04x => vkey %04x\n", e2.keycode, vkey);
        keyc2vkey[e2.keycode] = vkey;
        keyc2scan[e2.keycode] = scan;
        if ((vkey & 0xff) && vkey_used[(vkey & 0xff)])
            WARN("vkey %04x is being used by more than one keycode\n", vkey);
        vkey_used[(vkey & 0xff)] = 1;
    } /* for */

#define VKEY_IF_NOT_USED(vkey) (vkey_used[(vkey)] ? 0 : (vkey_used[(vkey)] = 1, (vkey)))
    for (keyc = min_keycode; keyc <= max_keycode; keyc++)
    {
        vkey = keyc2vkey[keyc] & 0xff;
        if (vkey)
            continue;

        e2.keycode = (KeyCode)keyc;
        keysym = XLookupKeysym(&e2, 0);
        if (!keysym)
           continue;

        /* find a suitable layout-dependent VK code */
        /* (most Winelib apps ought to be able to work without layout tables!) */
        for (i = 0; (i < keysyms_per_keycode) && (!vkey); i++)
        {
            keysym = XLookupKeysym(&e2, i);
            if ((keysym >= XK_0 && keysym <= XK_9)
                || (keysym >= XK_A && keysym <= XK_Z)) {
                vkey = VKEY_IF_NOT_USED(keysym);
            }
        }

        for (i = 0; (i < keysyms_per_keycode) && (!vkey); i++)
        {
            keysym = XLookupKeysym(&e2, i);
            switch (keysym)
            {
            case ';':             vkey = VKEY_IF_NOT_USED(VK_OEM_1); break;
            case '/':             vkey = VKEY_IF_NOT_USED(VK_OEM_2); break;
            case '`':             vkey = VKEY_IF_NOT_USED(VK_OEM_3); break;
            case '[':             vkey = VKEY_IF_NOT_USED(VK_OEM_4); break;
            case '\\':            vkey = VKEY_IF_NOT_USED(VK_OEM_5); break;
            case ']':             vkey = VKEY_IF_NOT_USED(VK_OEM_6); break;
            case '\'':            vkey = VKEY_IF_NOT_USED(VK_OEM_7); break;
            case ',':             vkey = VKEY_IF_NOT_USED(VK_OEM_COMMA); break;
            case '.':             vkey = VKEY_IF_NOT_USED(VK_OEM_PERIOD); break;
            case '-':             vkey = VKEY_IF_NOT_USED(VK_OEM_MINUS); break;
            case '+':             vkey = VKEY_IF_NOT_USED(VK_OEM_PLUS); break;
            }
        }

        if (!vkey)
        {
            /* Others keys: let's assign OEM virtual key codes in the allowed range,
             * that is ([0xba,0xc0], [0xdb,0xe4], 0xe6 (given up) et [0xe9,0xf5]) */
            do
            {
                switch (++OEMvkey)
                {
                case 0xc1 : OEMvkey=0xdb; break;
                case 0xe5 : OEMvkey=0xe9; break;
                case 0xf6 : OEMvkey=0xf5; WARN("No more OEM vkey available!\n");
                }
            } while (OEMvkey < 0xf5 && vkey_used[OEMvkey]);

            vkey = VKEY_IF_NOT_USED(OEMvkey);

            if (TRACE_ON(keyboard))
            {
                TRACE("OEM specific virtual key %X assigned to keycode %X:\n",
                                 OEMvkey, e2.keycode);
                TRACE("(");
                for (i = 0; i < keysyms_per_keycode; i += 1)
                {
                    const char *ksname;

                    keysym = XLookupKeysym(&e2, i);
                    ksname = XKeysymToString(keysym);
                    if (!ksname)
                        ksname = "NoSymbol";
                    TRACE( "%lX (%s) ", keysym, ksname);
                }
                TRACE(")\n");
            }
        }

        if (vkey)
        {
            TRACE("keycode %04x => vkey %04x\n", e2.keycode, vkey);
            keyc2vkey[e2.keycode] = vkey;
        }
    } /* for */
#undef VKEY_IF_NOT_USED

    /* If some keys still lack scancodes, assign some arbitrary ones to them now */
    for (scan = 0x60, keyc = min_keycode; keyc <= max_keycode; keyc++)
      if (keyc2vkey[keyc]&&!keyc2scan[keyc]) {
	const char *ksname;
	keysym = XKeycodeToKeysym(display, keyc, 0);
	ksname = XKeysymToString(keysym);
	if (!ksname) ksname = "NoSymbol";

	/* should make sure the scancode is unassigned here, but >=0x60 currently always is */

	TRACE_(key)("assigning scancode %02x to unidentified keycode %02x (%s)\n",scan,keyc,ksname);
	keyc2scan[keyc]=scan++;
      }

    wine_tsx11_unlock();
}

static BOOL match_x11_keyboard_layout(HKL hkl)
{
    const DWORD isIME = 0xE0000000;
    HKL xHkl = get_locale_kbd_layout();

    /* if the layout is an IME, only match the low word (LCID) */
    if (((ULONG_PTR)hkl & isIME) == isIME)
        return (LOWORD(hkl) == LOWORD(xHkl));
    else
        return (hkl == xHkl);
}

/**********************************************************************
 *		GetAsyncKeyState (X11DRV.@)
 */
SHORT CDECL X11DRV_GetAsyncKeyState(INT key)
{
    SHORT retval;

    /* Photoshop livelocks unless mouse events are included here */
    X11DRV_MsgWaitForMultipleObjectsEx( 0, NULL, 0, QS_KEY | QS_MOUSE, 0 );

    retval = ((key_state_table[key] & 0x40) ? 0x0001 : 0) |
             ((key_state_table[key] & 0x80) ? 0x8000 : 0);
    key_state_table[key] &= ~0x40;
    TRACE_(key)("(%x) -> %x\n", key, retval);
    return retval;
}


/***********************************************************************
 *		GetKeyboardLayout (X11DRV.@)
 */
HKL CDECL X11DRV_GetKeyboardLayout(DWORD dwThreadid)
{
    if (!dwThreadid || dwThreadid == GetCurrentThreadId())
    {
        struct x11drv_thread_data *thread_data = x11drv_thread_data();
        if (thread_data && thread_data->kbd_layout) return thread_data->kbd_layout;
    }
    else
        FIXME("couldn't return keyboard layout for thread %04x\n", dwThreadid);

    return get_locale_kbd_layout();
}


/***********************************************************************
 *		LoadKeyboardLayout (X11DRV.@)
 */
HKL CDECL X11DRV_LoadKeyboardLayout(LPCWSTR name, UINT flags)
{
    FIXME("%s, %04x: stub!\n", debugstr_w(name), flags);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/***********************************************************************
 *		UnloadKeyboardLayout (X11DRV.@)
 */
BOOL CDECL X11DRV_UnloadKeyboardLayout(HKL hkl)
{
    FIXME("%p: stub!\n", hkl);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/***********************************************************************
 *		ActivateKeyboardLayout (X11DRV.@)
 */
HKL CDECL X11DRV_ActivateKeyboardLayout(HKL hkl, UINT flags)
{
    HKL oldHkl = 0;
    struct x11drv_thread_data *thread_data = x11drv_init_thread_data();

    FIXME("%p, %04x: semi-stub!\n", hkl, flags);
    if (flags & KLF_SETFORPROCESS)
    {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        FIXME("KLF_SETFORPROCESS not supported\n");
        return 0;
    }

    if (flags)
        FIXME("flags %x not supported\n",flags);

    if (hkl == (HKL)HKL_NEXT || hkl == (HKL)HKL_PREV)
    {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        FIXME("HKL_NEXT and HKL_PREV not supported\n");
        return 0;
    }

    if (!match_x11_keyboard_layout(hkl))
    {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        FIXME("setting keyboard of different locales not supported\n");
        return 0;
    }

    oldHkl = thread_data->kbd_layout;
    if (!oldHkl) oldHkl = get_locale_kbd_layout();

    thread_data->kbd_layout = hkl;

    return oldHkl;
}


/***********************************************************************
 *           X11DRV_MappingNotify
 */
void X11DRV_MappingNotify( HWND dummy, XEvent *event )
{
    HWND hwnd;

    wine_tsx11_lock();
    XRefreshKeyboardMapping(&event->xmapping);
    wine_tsx11_unlock();
    X11DRV_InitKeyboard( event->xmapping.display );

    hwnd = GetFocus();
    if (!hwnd) hwnd = GetActiveWindow();
    PostMessageW(hwnd, WM_INPUTLANGCHANGEREQUEST,
                 0 /*FIXME*/, (LPARAM)X11DRV_GetKeyboardLayout(0));
}


/***********************************************************************
 *		VkKeyScanEx (X11DRV.@)
 *
 * Note: Windows ignores HKL parameter and uses current active layout instead
 */
SHORT CDECL X11DRV_VkKeyScanEx(WCHAR wChar, HKL hkl)
{
    Display *display = thread_init_display();
    KeyCode keycode;
    KeySym keysym;
    int i, index;
    CHAR cChar;
    SHORT ret;

    /* FIXME: what happens if wChar is not a Latin1 character and CP_UNIXCP
     * is UTF-8 (multibyte encoding)?
     */
    if (!WideCharToMultiByte(CP_UNIXCP, 0, &wChar, 1, &cChar, 1, NULL, NULL))
    {
        WARN("no translation from unicode to CP_UNIXCP for 0x%02x\n", wChar);
        return -1;
    }

    TRACE("wChar 0x%02x -> cChar '%c'\n", wChar, cChar);

    /* char->keysym (same for ANSI chars) */
    keysym = (unsigned char)cChar; /* (!) cChar is signed */
    if (keysym <= 27) keysym += 0xFF00; /* special chars : return, backspace... */

    wine_tsx11_lock();
    keycode = XKeysymToKeycode(display, keysym);  /* keysym -> keycode */
    if (!keycode)
    {
        if (keysym >= 0xFF00) /* Windows returns 0x0240 + cChar in this case */
        {
            ret = 0x0240 + cChar; /* 0x0200 indicates a control character */
            TRACE(" ... returning ctrl char %#.2x\n", ret);
            wine_tsx11_unlock();
            return ret;
        }
        /* It didn't work ... let's try with deadchar code. */
        TRACE("retrying with | 0xFE00\n");
        keycode = XKeysymToKeycode(display, keysym | 0xFE00);
    }
    wine_tsx11_unlock();

    TRACE("'%c'(%#lx, %lu): got keycode %#.2x (%d)\n",
            cChar, keysym, keysym, keycode, keycode);

    /* keycode -> (keyc2vkey) vkey */
    ret = keyc2vkey[keycode];

    if (!keycode || !ret)
    {
        TRACE("keycode for '%c' not found, returning -1\n", cChar);
        return -1;
    }

    index = -1;
    wine_tsx11_lock();
    for (i = 0; i < 4; i++) /* find shift state */
    {
        if (XKeycodeToKeysym(display, keycode, i) == keysym)
        {
            index = i;
            break;
        }
    }
    wine_tsx11_unlock();

    switch (index)
    {
        default:
        case -1:
            WARN("Keysym %lx not found while parsing the keycode table\n", keysym);
            return -1;

        case 0: break;
        case 1: ret += 0x0100; break;
        case 2: ret += 0x0600; break;
        case 3: ret += 0x0700; break;
    }
    /*
      index : 0     adds 0x0000
      index : 1     adds 0x0100 (shift)
      index : ?     adds 0x0200 (ctrl)
      index : 2     adds 0x0600 (ctrl+alt)
      index : 3     adds 0x0700 (ctrl+alt+shift)
     */

    TRACE(" ... returning %#.2x\n", ret);
    return ret;
}

/***********************************************************************
 *		MapVirtualKeyEx (X11DRV.@)
 */
UINT CDECL X11DRV_MapVirtualKeyEx(UINT wCode, UINT wMapType, HKL hkl)
{
    Display *display = thread_init_display();

#define returnMVK(value) do { TRACE("returning 0x%x.\n",value); return value; } while(0)

    TRACE("wCode=0x%x, wMapType=%d, hkl %p\n", wCode, wMapType, hkl);
    if (!match_x11_keyboard_layout(hkl))
        FIXME("keyboard layout %p is not supported\n", hkl);

    switch(wMapType)
    {
        case MAPVK_VK_TO_VSC: /* vkey-code to scan-code */
        case MAPVK_VK_TO_VSC_EX:
        {
            int keyc;

            switch (wCode)
            {
                case VK_SHIFT: wCode = VK_LSHIFT; break;
                case VK_CONTROL: wCode = VK_LCONTROL; break;
                case VK_MENU: wCode = VK_LMENU; break;
            }

            /* let's do vkey -> keycode -> scan */
            for (keyc = min_keycode; keyc <= max_keycode; keyc++)
                if ((keyc2vkey[keyc] & 0xFF) == wCode) break;

            if (keyc > max_keycode)
            {
                TRACE("returning no scan-code.\n");
                return 0;
            }
            returnMVK (keyc2scan[keyc] & 0xFF);
        }
        case MAPVK_VSC_TO_VK: /* scan-code to vkey-code */
        case MAPVK_VSC_TO_VK_EX:
        {
            int keyc;
            UINT vkey = 0;

            /* let's do scan -> keycode -> vkey */
            for (keyc = min_keycode; keyc <= max_keycode; keyc++)
                if ((keyc2scan[keyc] & 0xFF) == (wCode & 0xFF))
                {
                    vkey = keyc2vkey[keyc] & 0xFF;
                    /* Only stop if it's not a numpad vkey; otherwise keep
                       looking for a potential better vkey. */
                    if (vkey && (vkey < VK_NUMPAD0 || VK_DIVIDE < vkey))
                        break;
                }

            if (vkey == 0)
            {
                TRACE("returning no vkey-code.\n");
                return 0;
            }

            if (wMapType == MAPVK_VSC_TO_VK)
                switch (vkey)
                {
                    case VK_LSHIFT:
                    case VK_RSHIFT:
                        vkey = VK_SHIFT; break;
                    case VK_LCONTROL:
                    case VK_RCONTROL:
                        vkey = VK_CONTROL; break;
                    case VK_LMENU:
                    case VK_RMENU:
                        vkey = VK_MENU; break;
                }

            returnMVK (vkey);
        }
		case MAPVK_VK_TO_CHAR: /* vkey-code to unshifted ANSI code */
		{
                        /* we still don't know what "unshifted" means. in windows VK_W (0x57)
                         * returns 0x57, which is upercase 'W'. So we have to return the uppercase
                         * key.. Looks like something is wrong with the MS docs?
                         * This is only true for letters, for example VK_0 returns '0' not ')'.
                         * - hence we use the lock mask to ensure this happens.
                         */
			/* let's do vkey -> keycode -> (XLookupString) ansi char */
			XKeyEvent e;
			KeySym keysym;
			int keyc, len;
			char s[10];

			e.display = display;
			e.state = 0;
			e.keycode = 0;

                        wine_tsx11_lock();

			/* We exit on the first keycode found, to speed up the thing. */
			for (keyc=min_keycode; (keyc<=max_keycode) && (!e.keycode) ; keyc++)
			{ /* Find a keycode that could have generated this virtual key */
			    if  ((keyc2vkey[keyc] & 0xFF) == wCode)
			    { /* We filter the extended bit, we don't know it */
			        e.keycode = keyc; /* Store it temporarily */
				if ((EVENT_event_to_vkey(0,&e) & 0xFF) != wCode) {
				    e.keycode = 0; /* Wrong one (ex: because of the NumLock
					 state), so set it to 0, we'll find another one */
				}
			    }
			}

			if ((wCode>=VK_NUMPAD0) && (wCode<=VK_NUMPAD9))
			  e.keycode = XKeysymToKeycode(e.display, wCode-VK_NUMPAD0+XK_KP_0);

			if (wCode==VK_DECIMAL)
			  e.keycode = XKeysymToKeycode(e.display, XK_KP_Decimal);

			if (!e.keycode)
			{
			  WARN("Unknown virtual key %X !!!\n", wCode);
                          wine_tsx11_unlock();
			  return 0; /* whatever */
			}
			TRACE("Found keycode %d (0x%2X)\n",e.keycode,e.keycode);

                        len = XLookupString(&e, s, sizeof(s), &keysym, NULL);
                        wine_tsx11_unlock();

                        if (len)
                        {
                            WCHAR wch;
                            if (MultiByteToWideChar(CP_UNIXCP, 0, s, len, &wch, 1))
                                returnMVK(toupperW(wch));
                        }
			TRACE("returning no ANSI.\n");
			return 0;
		}
		default: /* reserved */
			FIXME("Unknown wMapType %d !\n", wMapType);
			return 0;
	}
	return 0;
}

/***********************************************************************
 *		GetKeyNameText (X11DRV.@)
 */
INT CDECL X11DRV_GetKeyNameText(LONG lParam, LPWSTR lpBuffer, INT nSize)
{
  Display *display = thread_init_display();
  int vkey, ansi, scanCode;
  KeyCode keyc;
  int keyi;
  KeySym keys;
  char *name;

  scanCode = lParam >> 16;
  scanCode &= 0x1ff;  /* keep "extended-key" flag with code */

  vkey = X11DRV_MapVirtualKeyEx(scanCode, MAPVK_VSC_TO_VK_EX, X11DRV_GetKeyboardLayout(0));

  /*  handle "don't care" bit (0x02000000) */
  if (!(lParam & 0x02000000)) {
    switch (vkey) {
         case VK_RSHIFT:
                          /* R-Shift is "special" - it is an extended key with separate scan code */
                          scanCode |= 0x100;
         case VK_LSHIFT:
                          vkey = VK_SHIFT;
                          break;
       case VK_LCONTROL:
       case VK_RCONTROL:
                          vkey = VK_CONTROL;
                          break;
          case VK_LMENU:
          case VK_RMENU:
                          vkey = VK_MENU;
                          break;
    }
  }

  ansi = X11DRV_MapVirtualKeyEx(vkey, MAPVK_VK_TO_CHAR, X11DRV_GetKeyboardLayout(0));
  TRACE("scan 0x%04x, vkey 0x%04x, ANSI 0x%04x\n", scanCode, vkey, ansi);

  /* first get the name of the "regular" keys which is the Upper case
     value of the keycap imprint.                                     */
  if ( ((ansi >= 0x21) && (ansi <= 0x7e)) &&
       (scanCode != 0x137) &&   /* PrtScn   */
       (scanCode != 0x135) &&   /* numpad / */
       (scanCode != 0x37 ) &&   /* numpad * */
       (scanCode != 0x4a ) &&   /* numpad - */
       (scanCode != 0x4e ) )    /* numpad + */
      {
        if ((nSize >= 2) && lpBuffer)
	{
          *lpBuffer = toupperW((WCHAR)ansi);
          *(lpBuffer+1) = 0;
          return 1;
        }
     else
        return 0;
  }

  /* FIXME: horrible hack to fix function keys. Windows reports scancode
            without "extended-key" flag. However Wine generates scancode
            *with* "extended-key" flag. Seems to occur *only* for the
            function keys. Soooo.. We will leave the table alone and
            fudge the lookup here till the other part is found and fixed!!! */

  if ( ((scanCode >= 0x13b) && (scanCode <= 0x144)) ||
       (scanCode == 0x157) || (scanCode == 0x158))
    scanCode &= 0xff;   /* remove "extended-key" flag for Fx keys */

  /* let's do scancode -> keycode -> keysym -> String */

  for (keyi=min_keycode; keyi<=max_keycode; keyi++)
      if ((keyc2scan[keyi]) == scanCode)
         break;
  if (keyi <= max_keycode)
  {
      wine_tsx11_lock();
      keyc = (KeyCode) keyi;
      keys = XKeycodeToKeysym(display, keyc, 0);
      name = XKeysymToString(keys);
      wine_tsx11_unlock();
      TRACE("found scan=%04x keyc=%04x keysym=%04x string=%s\n",
            scanCode, keyc, (int)keys, name);
      if (lpBuffer && nSize && name)
          return MultiByteToWideChar(CP_UNIXCP, 0, name, -1, lpBuffer, nSize);
  }

  /* Finally issue WARN for unknown keys   */

  WARN("(%08x,%p,%d): unsupported key, vkey=%04x, ansi=%04x\n",lParam,lpBuffer,nSize,vkey,ansi);
  if (lpBuffer && nSize)
    *lpBuffer = 0;
  return 0;
}

/***********************************************************************
 *		X11DRV_KEYBOARD_MapDeadKeysym
 */
static char KEYBOARD_MapDeadKeysym(KeySym keysym)
{
	switch (keysym)
	    {
	/* symbolic ASCII is the same as defined in rfc1345 */
#ifdef XK_dead_tilde
	    case XK_dead_tilde :
#endif
	    case 0x1000FE7E : /* Xfree's XK_Dtilde */
		return '~';	/* '? */
#ifdef XK_dead_acute
	    case XK_dead_acute :
#endif
	    case 0x1000FE27 : /* Xfree's XK_Dacute_accent */
		return 0xb4;	/* '' */
#ifdef XK_dead_circumflex
	    case XK_dead_circumflex:
#endif
	    case 0x1000FE5E : /* Xfree's XK_Dcircumflex_accent */
		return '^';	/* '> */
#ifdef XK_dead_grave
	    case XK_dead_grave :
#endif
	    case 0x1000FE60 : /* Xfree's XK_Dgrave_accent */
		return '`';	/* '! */
#ifdef XK_dead_diaeresis
	    case XK_dead_diaeresis :
#endif
	    case 0x1000FE22 : /* Xfree's XK_Ddiaeresis */
		return 0xa8;	/* ': */
#ifdef XK_dead_cedilla
	    case XK_dead_cedilla :
	        return 0xb8;	/* ', */
#endif
#ifdef XK_dead_macron
	    case XK_dead_macron :
	        return '-';	/* 'm isn't defined on iso-8859-x */
#endif
#ifdef XK_dead_breve
	    case XK_dead_breve :
	        return 0xa2;	/* '( */
#endif
#ifdef XK_dead_abovedot
	    case XK_dead_abovedot :
	        return 0xff;	/* '. */
#endif
#ifdef XK_dead_abovering
	    case XK_dead_abovering :
	        return '0';	/* '0 isn't defined on iso-8859-x */
#endif
#ifdef XK_dead_doubleacute
	    case XK_dead_doubleacute :
	        return 0xbd;	/* '" */
#endif
#ifdef XK_dead_caron
	    case XK_dead_caron :
	        return 0xb7;	/* '< */
#endif
#ifdef XK_dead_ogonek
	    case XK_dead_ogonek :
	        return 0xb2;	/* '; */
#endif
/* FIXME: I don't know this three.
	    case XK_dead_iota :
	        return 'i';
	    case XK_dead_voiced_sound :
	        return 'v';
	    case XK_dead_semivoiced_sound :
	        return 's';
*/
	    }
	TRACE("no character for dead keysym 0x%08lx\n",keysym);
	return 0;
}

/***********************************************************************
 *		ToUnicodeEx (X11DRV.@)
 *
 * The ToUnicode function translates the specified virtual-key code and keyboard
 * state to the corresponding Windows character or characters.
 *
 * If the specified key is a dead key, the return value is negative. Otherwise,
 * it is one of the following values:
 * Value	Meaning
 * 0	The specified virtual key has no translation for the current state of the keyboard.
 * 1	One Windows character was copied to the buffer.
 * 2	Two characters were copied to the buffer. This usually happens when a
 *      dead-key character (accent or diacritic) stored in the keyboard layout cannot
 *      be composed with the specified virtual key to form a single character.
 *
 * FIXME : should do the above (return 2 for non matching deadchar+char combinations)
 *
 */
INT CDECL X11DRV_ToUnicodeEx(UINT virtKey, UINT scanCode, const BYTE *lpKeyState,
                             LPWSTR bufW, int bufW_size, UINT flags, HKL hkl)
{
    Display *display = thread_init_display();
    XKeyEvent e;
    KeySym keysym = 0;
    INT ret;
    int keyc;
    char buf[10];
    char *lpChar = buf;
    HWND focus;
    XIC xic;
    Status status = 0;

    if (scanCode & 0x8000)
    {
        TRACE("Key UP, doing nothing\n" );
        return 0;
    }

    if (!match_x11_keyboard_layout(hkl))
        FIXME("keyboard layout %p is not supported\n", hkl);

    if ((lpKeyState[VK_MENU] & 0x80) && (lpKeyState[VK_CONTROL] & 0x80))
    {
        TRACE("Ctrl+Alt+[key] won't generate a character\n");
        return 0;
    }

    e.display = display;
    e.keycode = 0;
    e.state = 0;
    e.type = KeyPress;

    focus = GetFocus();
    if (focus) focus = GetAncestor( focus, GA_ROOT );
    if (!focus) focus = GetActiveWindow();
    e.window = X11DRV_get_whole_window( focus );
    xic = X11DRV_get_ic( focus );

    if (lpKeyState[VK_SHIFT] & 0x80)
    {
	TRACE("ShiftMask = %04x\n", ShiftMask);
	e.state |= ShiftMask;
    }
    if (lpKeyState[VK_CAPITAL] & 0x01)
    {
	TRACE("LockMask = %04x\n", LockMask);
	e.state |= LockMask;
    }
    if (lpKeyState[VK_CONTROL] & 0x80)
    {
	TRACE("ControlMask = %04x\n", ControlMask);
	e.state |= ControlMask;
    }
    if (lpKeyState[VK_NUMLOCK] & 0x01)
    {
	TRACE("NumLockMask = %04x\n", NumLockMask);
	e.state |= NumLockMask;
    }

    /* Restore saved AltGr state */
    TRACE("AltGrMask = %04x\n", AltGrMask);
    e.state |= AltGrMask;

    TRACE_(key)("(%04X, %04X) : faked state = 0x%04x\n",
		virtKey, scanCode, e.state);
    wine_tsx11_lock();
    /* We exit on the first keycode found, to speed up the thing. */
    for (keyc=min_keycode; (keyc<=max_keycode) && (!e.keycode) ; keyc++)
      { /* Find a keycode that could have generated this virtual key */
          if  ((keyc2vkey[keyc] & 0xFF) == virtKey)
          { /* We filter the extended bit, we don't know it */
              e.keycode = keyc; /* Store it temporarily */
              if ((EVENT_event_to_vkey(xic,&e) & 0xFF) != virtKey) {
                  e.keycode = 0; /* Wrong one (ex: because of the NumLock
                         state), so set it to 0, we'll find another one */
              }
	  }
      }

    if (virtKey >= VK_LEFT && virtKey <= VK_DOWN)
        e.keycode = XKeysymToKeycode(e.display, virtKey - VK_LEFT + XK_Left);

    if ((virtKey>=VK_NUMPAD0) && (virtKey<=VK_NUMPAD9))
        e.keycode = XKeysymToKeycode(e.display, virtKey-VK_NUMPAD0+XK_KP_0);

    if (virtKey==VK_DECIMAL)
        e.keycode = XKeysymToKeycode(e.display, XK_KP_Decimal);

    if (virtKey==VK_SEPARATOR)
        e.keycode = XKeysymToKeycode(e.display, XK_KP_Separator);

    if (!e.keycode && virtKey != VK_NONAME)
      {
	WARN("Unknown virtual key %X !!!\n", virtKey);
        wine_tsx11_unlock();
	return 0;
      }
    else TRACE("Found keycode %d (0x%2X)\n",e.keycode,e.keycode);

    TRACE_(key)("type %d, window %lx, state 0x%04x, keycode 0x%04x\n",
		e.type, e.window, e.state, e.keycode);

    /* Clients should pass only KeyPress events to XmbLookupString,
     * e.type was set to KeyPress above.
     */
    if (xic)
    {
        ret = XmbLookupString(xic, &e, buf, sizeof(buf), &keysym, &status);
        TRACE("XmbLookupString needs %d byte(s)\n", ret);
        if (status == XBufferOverflow)
        {
            lpChar = HeapAlloc(GetProcessHeap(), 0, ret);
            if (lpChar == NULL)
            {
                ERR("Failed to allocate memory!\n");
                wine_tsx11_unlock();
                return 0;
            }
            ret = XmbLookupString(xic, &e, lpChar, ret, &keysym, &status);
        }
    }
    else
        ret = XLookupString(&e, buf, sizeof(buf), &keysym, NULL);
    wine_tsx11_unlock();

    TRACE_(key)("nbyte = %d, status 0x%x\n", ret, status);

    if (TRACE_ON(key))
    {
        const char *ksname;

        wine_tsx11_lock();
        ksname = XKeysymToString(keysym);
        wine_tsx11_unlock();
        if (!ksname) ksname = "No Name";
        TRACE_(key)("%s : keysym=%lX (%s), # of chars=%d / %s\n",
                    (e.type == KeyPress) ? "KeyPress" : "KeyRelease",
                    keysym, ksname, ret, debugstr_an(lpChar, ret));
    }

    if (ret == 0)
    {
	char dead_char;

#ifdef XK_EuroSign
        /* An ugly hack for EuroSign: X can't translate it to a character
           for some locales. */
        if (keysym == XK_EuroSign)
        {
            bufW[0] = 0x20AC;
            ret = 1;
            goto found;
        }
#endif
        /* Special case: X turns shift-tab into ISO_Left_Tab. */
        /* Here we change it back. */
        if (keysym == XK_ISO_Left_Tab && !(e.state & ControlMask))
        {
            bufW[0] = 0x09;
            ret = 1;
            goto found;
        }

	dead_char = KEYBOARD_MapDeadKeysym(keysym);
	if (dead_char)
        {
	    MultiByteToWideChar(CP_UNIXCP, 0, &dead_char, 1, bufW, bufW_size);
	    ret = -1;
            goto found;
        }

        if (keysym >= 0x01000100 && keysym <= 0x0100ffff)
        {
            /* Unicode direct mapping */
            bufW[0] = keysym & 0xffff;
            ret = 1;
            goto found;
        }
        else if ((keysym >> 8) == 0x1008FF) {
            bufW[0] = 0;
            ret = 0;
            goto found;
        }
	else
	    {
	    const char *ksname;

            wine_tsx11_lock();
	    ksname = XKeysymToString(keysym);
            wine_tsx11_unlock();
	    if (!ksname)
		ksname = "No Name";
	    if ((keysym >> 8) != 0xff)
		{
		WARN("no char for keysym %04lX (%s) :\n",
                    keysym, ksname);
		WARN("virtKey=%X, scanCode=%X, keycode=%X, state=%X\n",
                    virtKey, scanCode, e.keycode, e.state);
		}
	    }
	}
    else {  /* ret != 0 */
        /* We have a special case to handle : Shift + arrow, shift + home, ...
           X returns a char for it, but Windows doesn't. Let's eat it. */
        if (!(e.state & NumLockMask)  /* NumLock is off */
            && (e.state & ShiftMask) /* Shift is pressed */
            && (keysym>=XK_KP_0) && (keysym<=XK_KP_9))
        {
            lpChar[0] = 0;
            ret = 0;
        }

        /* more areas where X returns characters but Windows does not
           CTRL + number or CTRL + symbol */
        if (e.state & ControlMask)
        {
            if (((keysym>=33) && (keysym < 'A')) ||
                ((keysym > 'Z') && (keysym < 'a')) ||
                (keysym == XK_Tab))
            {
                lpChar[0] = 0;
                ret = 0;
            }
        }

        /* We have another special case for delete key (XK_Delete) on an
         extended keyboard. X returns a char for it, but Windows doesn't */
        if (keysym == XK_Delete)
        {
            lpChar[0] = 0;
            ret = 0;
        }
	else if((lpKeyState[VK_SHIFT] & 0x80) /* Shift is pressed */
		&& (keysym == XK_KP_Decimal))
        {
            lpChar[0] = 0;
            ret = 0;
        }
	else if((lpKeyState[VK_CONTROL] & 0x80) /* Control is pressed */
		&& (keysym == XK_Return || keysym == XK_KP_Enter))
        {
            lpChar[0] = '\n';
            ret = 1;
        }

        /* Hack to detect an XLookupString hard-coded to Latin1 */
        if (ret == 1 && keysym >= 0x00a0 && keysym <= 0x00ff && (BYTE)lpChar[0] == keysym)
        {
            bufW[0] = (BYTE)lpChar[0];
            goto found;
        }

	/* perform translation to unicode */
	if(ret)
	{
	    TRACE_(key)("Translating char 0x%02x to unicode\n", *(BYTE *)lpChar);
	    ret = MultiByteToWideChar(CP_UNIXCP, 0, lpChar, ret, bufW, bufW_size);
	}
    }

found:
    if (buf != lpChar)
        HeapFree(GetProcessHeap(), 0, lpChar);
    TRACE_(key)("returning %d with %s\n", ret, debugstr_wn(bufW, ret));
    return ret;
}

/***********************************************************************
 *		Beep (X11DRV.@)
 */
void CDECL X11DRV_Beep(void)
{
    wine_tsx11_lock();
    XBell(gdi_display, 0);
    wine_tsx11_unlock();
}
