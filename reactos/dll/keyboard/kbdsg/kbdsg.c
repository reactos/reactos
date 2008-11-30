/*
 * ReactOS German (Switzerland) ASCII Keyboard layout
 * Copyright (C) 2005 ReactOS
 * License: LGPL, see: LGPL.txt
 *
 * Thanks to: http://www.barcodeman.com/altek/mule/scandoc.php
 * and http://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html
 * and http://www.unicode.org/charts/
 */


#include <windows.h>
#include <internal/kbd.h>

#ifdef _M_IA64
#define ROSDATA static __declspec(allocate(".data"))
#else
#ifdef _MSC_VER
#pragma data_seg(".data")
#define ROSDATA static
#else
#define ROSDATA static __attribute__((section(".data")))
#endif
#endif

#define VK_EMPTY 0xff   /* The non-existent VK */
#define KSHIFT   0x001  /* Shift modifier */
#define KCTRL    0x002  /* Ctrl modifier */
#define KALT     0x004  /* Alt modifier */
#define KEXT     0x100  /* Extended key code */
#define KMULTI   0x200  /* Multi-key */
#define KSPEC    0x400  /* Special key */
#define KNUMP    0x800  /* Number-pad */
#define KNUMS    0xc00  /* Special + number pad */
#define KMEXT    0x300  /* Multi + ext */

#define SHFT_INVALID 0x0F

ROSDATA USHORT scancode_to_vk[] = {
  /* Numbers Row */
  /* - 00 - */
  /* 1 ...         2 ...         3 ...         4 ... */
  VK_EMPTY,     VK_ESCAPE,    '1',          '2',
  '3',          '4',          '5',          '6',
  '7',          '8',          '9',          '0',
  VK_OEM_4, VK_OEM_6,  VK_BACK,
  /* - 0f - */
  /* First Letters Row */
  VK_TAB,       'Q',          'W',          'E',
  'R',          'T',          'Z',          'U',
  'I',          'O',          'P',
  VK_OEM_1,     VK_OEM_PLUS,     VK_RETURN,
  /* - 1d - */
  /* Second Letters Row */
  VK_LCONTROL,
  'A',          'S',          'D',          'F',
  'G',          'H',          'J',          'K',
  'L',          VK_OEM_3,     VK_OEM_7,     VK_OEM_5,
  /* - 2c - */
  /* Third letters row */
  VK_LSHIFT,    VK_OEM_2,
  'Y',          'X',          'C',          'V',
  'B',          'N',          'M',          VK_OEM_COMMA,
  VK_OEM_PERIOD, VK_OEM_MINUS,     VK_RSHIFT,
  /* - 37 - */
  /* Bottom Row */
  VK_MULTIPLY,  VK_LMENU,     VK_SPACE,   VK_CAPITAL,

  /* - 3b - */
  /* F-Keys */
  VK_F1, VK_F2, VK_F3, VK_F4, VK_F5, VK_F6,
  VK_F7, VK_F8, VK_F9, VK_F10,
  /* - 45 - */
  /* Locks */
  VK_NUMLOCK | KMEXT,
  VK_SCROLL | KMULTI,
  /* - 47 - */
  /* Number-Pad */
  VK_HOME | KNUMS,      VK_UP | KNUMS,         VK_PRIOR | KNUMS, VK_SUBTRACT,
  VK_LEFT | KNUMS,      VK_CLEAR | KNUMS,      VK_RIGHT | KNUMS, VK_ADD,
  VK_END | KNUMS,       VK_DOWN | KNUMS,       VK_NEXT | KNUMS,
  VK_INSERT | KNUMS,    VK_DELETE | KNUMS,
  /* - 54 - */
  /* Presumably PrtSc */
  VK_SNAPSHOT,
  /* - 55 - */
  /* Oddities, and the remaining standard F-Keys */
  VK_EMPTY,   VK_OEM_102,     VK_F11,       VK_F12,
  /* - 59 - */
  VK_CLEAR,     VK_EMPTY,     VK_EMPTY,     VK_EMPTY,     VK_EMPTY, /* EREOF */
  VK_EMPTY,     VK_EMPTY,     VK_EMPTY,     VK_EMPTY,     VK_EMPTY, /* ZOOM */
  VK_HELP,
  /* - 64 - */
  /* Even more F-Keys (for example, NCR keyboards from the early 90's) */
  VK_F13, VK_F14, VK_F15, VK_F16, VK_F17, VK_F18, VK_F19, VK_F20,
  VK_F21, VK_F22, VK_F23,
  /* - 6f - */
  /* Not sure who uses these codes */
  VK_EMPTY, VK_EMPTY, VK_EMPTY,
  /* - 72 - */
  VK_EMPTY, VK_EMPTY, VK_EMPTY, VK_EMPTY,
  /* - 76 - */
  /* One more f-key */
  VK_F24,
  /* - 77 - */
  VK_EMPTY, VK_EMPTY, VK_EMPTY, VK_EMPTY,
  VK_EMPTY, VK_EMPTY, VK_EMPTY, VK_EMPTY, /* PA1 */
  VK_EMPTY,
  /* - 80 - */
  0
};

ROSDATA VSC_VK extcode0_to_vk[] = {
  { 0x10, VK_MEDIA_PREV_TRACK | KEXT },
  { 0x19, VK_MEDIA_NEXT_TRACK | KEXT },
  { 0x1D, VK_RCONTROL | KEXT },
  { 0x20, VK_VOLUME_MUTE | KEXT },
  { 0x21, VK_LAUNCH_APP2 | KEXT },
  { 0x22, VK_MEDIA_PLAY_PAUSE | KEXT },
  { 0x24, VK_MEDIA_STOP | KEXT },
  { 0x2E, VK_VOLUME_DOWN | KEXT },
  { 0x30, VK_VOLUME_UP | KEXT },
  { 0x32, VK_BROWSER_HOME | KEXT },
  { 0x35, VK_DIVIDE | KEXT },
  { 0x37, VK_SNAPSHOT | KEXT },
  { 0x38, VK_RMENU | KEXT },
  { 0x47, VK_HOME | KEXT },
  { 0x48, VK_UP | KEXT },
  { 0x49, VK_PRIOR | KEXT },
  { 0x4B, VK_LEFT | KEXT },
  { 0x4D, VK_RIGHT | KEXT },
  { 0x4F, VK_END | KEXT },
  { 0x50, VK_DOWN | KEXT },
  { 0x51, VK_NEXT | KEXT },
  { 0x52, VK_INSERT | KEXT },
  { 0x53, VK_DELETE | KEXT },
  { 0x5B, VK_LWIN | KEXT },
  { 0x5C, VK_RWIN | KEXT },
  { 0x5D, VK_APPS | KEXT },
  { 0x5F, VK_SLEEP | KEXT },
  { 0x65, VK_BROWSER_SEARCH | KEXT },
  { 0x66, VK_BROWSER_FAVORITES | KEXT },
  { 0x67, VK_BROWSER_REFRESH | KEXT },
  { 0x68, VK_BROWSER_STOP | KEXT },
  { 0x69, VK_BROWSER_FORWARD | KEXT },
  { 0x6A, VK_BROWSER_BACK | KEXT },
  { 0x6B, VK_LAUNCH_APP1 | KEXT },
  { 0x6C, VK_LAUNCH_MAIL | KEXT },
  { 0x6D, VK_LAUNCH_MEDIA_SELECT | KEXT },
  { 0x1C, VK_RETURN | KEXT },
  { 0x46, VK_CANCEL | KEXT },
  { 0, 0 },
};

ROSDATA VSC_VK extcode1_to_vk[] = {
  { 0x1d, VK_PAUSE},
  { 0, 0 },
};

ROSDATA VK_TO_BIT modifier_keys[] = {
  { VK_SHIFT,   KSHIFT },
  { VK_CONTROL, KCTRL },
  { VK_MENU,    KALT },
  { 0,          0 }
};

ROSDATA MODIFIERS modifier_bits = {
  modifier_keys,
  6,
  { 0, 1, 3, 4, SHFT_INVALID, SHFT_INVALID, 2 } /* Modifier bit order, NONE, SHIFT, CTRL, ALT, MENU, SHIFT + MENU, CTRL + MENU */
};


/* ############################################ */
/* ############################################ */
/* ############################################ */
/* ############################################ */
/* ############################################ */

#define NOCAPS 0
#define CAPS   KSHIFT /* Caps -> shift */

ROSDATA VK_TO_WCHARS2 key_to_chars_2mod[] = {
  { VK_OEM_5, NOCAPS, {0xa7, 0xb0} }, /* § ° */

  /* Normal vs Shifted */
  /* The numbers */
  /* Ctrl-2 generates NUL */
  { 0xff,        NOCAPS, {0xa7, 0x9B} }, /* what is this for? */
  { '4',         NOCAPS, {'4',  0xE7} },
  { '5',         NOCAPS, {'5',  '%'} },
  { '9',         NOCAPS, {'9',  ')'} },
  { '0',         NOCAPS, {'0',  '='} },

  /* First letter row */
  { 'W',         CAPS,   {'w', 'W'} },
  { 'R',         CAPS,   {'r', 'R'} },
  { 'T',         CAPS,   {'t', 'T'} },
  { 'Z',         CAPS,   {'z', 'Z'} },
  { 'U',         CAPS,   {'u', 'U'} },
  { 'I',         CAPS,   {'i', 'I'} },
  { 'O',         CAPS,   {'o', 'O'} },
  { 'P',         CAPS,   {'p', 'P'} },
  /* Second letter row */
  { 'A',         CAPS,   {'a', 'A'} },
  { 'S',         CAPS,   {'s', 'S'} },
  { 'D',         CAPS,   {'d', 'D'} },
  { 'F',         CAPS,   {'f', 'F'} },
  { 'G',         CAPS,   {'g', 'G'} },
  { 'H',         CAPS,   {'h', 'H'} },
  { 'J',         CAPS,   {'j', 'J'} },
  { 'K',         CAPS,   {'k', 'K'} },
  { 'L',         CAPS,   {'l', 'L'} },
  /* Third letter row */
  { 'Y',         CAPS,   {'y', 'Y'} },
  { 'X',         CAPS,   {'x', 'X'} },
  { 'V',         CAPS,   {'v', 'V'} },
  { 'B',         CAPS,   {'b', 'B'} },
  { 'N',         CAPS,   {'n', 'N'} },

  /* Specials */
  { VK_OEM_COMMA,  NOCAPS, {',', ';'} },
  { VK_OEM_PERIOD, NOCAPS, {'.', ':'} },
  { VK_OEM_MINUS,  NOCAPS, {'-', '_'} },
  { VK_DECIMAL,    NOCAPS, {'.', '.'} },
  { VK_TAB,        NOCAPS, {'\t', '\t'} },
  { VK_ADD,        NOCAPS, {'+', '+'} },
  { VK_DIVIDE,     NOCAPS, {0x2f, 0x2f} }, /* '/' */
  { VK_MULTIPLY,   NOCAPS, {'*', '*'} },
  { VK_SUBTRACT,   NOCAPS, {'-', '-'} },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS3 key_to_chars_3mod[] = {
  /* Normal, Shifted, Alt Gr */
  /* Legacy (telnet-style) ascii escapes */
  { VK_OEM_4, NOCAPS, {'\''     , '?'     , WCH_DEAD} },          /* ' ? ´ */
      { 0xff, NOCAPS,  {WCH_NONE, WCH_NONE, 0xb4} },
  { 'Q', CAPS, {'q', 'Q', '@'} },
  { 'C', CAPS, {'c', 'C', 0xa9} },    /* c C Copyright-Sign */
  { 'E', CAPS, {'e', 'E', 0x20ac} }, /* e E € */
  { 'M', CAPS, {'m', 'M', 0xb5} },   /* m M mu-Sign */
  { VK_OEM_102, NOCAPS, {'<', '>', '\\'} },
  { 0,0 }
};

ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] = {
  /* Normal, Shifted, Alt Gr, C-S-x */
  /* Legacy Ascii generators */
  { VK_BACK, NOCAPS, {'\b', '\b', WCH_NONE, 0x7f} },
  { VK_ESCAPE, NOCAPS, {0x1b, 0x1b, WCH_NONE, 0x1b} },
  { VK_RETURN, NOCAPS, {'\r', '\r', WCH_NONE, '\n'} },
  { VK_SPACE, NOCAPS, {' ', ' ', WCH_NONE, ' '} },
  { VK_CANCEL, NOCAPS, {0x03, 0x03, WCH_NONE, 0x03} },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS5 key_to_chars_5mod[] = {
  /* Normal, Shifted, Alt Gr, Ctrl */
  { '1', NOCAPS, {'1', '+',  '|',  WCH_NONE, 0x00} },
  { '2', NOCAPS, {'2', '\"', '@',  WCH_NONE, 0x00} },
  { '3', NOCAPS, {'3', '*',  '#',  WCH_NONE, 0x00} },
  { '6', NOCAPS, {'6', '&',  0xac, WCH_NONE, 0x00} },
  { '7', NOCAPS, {'7', '/',  0xa6, WCH_NONE, 0x00} },
  { '8', NOCAPS, {'8', '(',  0xa2, WCH_NONE, 0x00} },
  { VK_OEM_1, KCTRL, {0xfc, 0xe8, 0x5b, 0Xdc, 0xc8} },                    /*    ü è [ Ü È   */
  { VK_OEM_2, NOCAPS, {0x24, 0xa3, 0x7d, WCH_NONE, 0x00} },                    /*    $ £ }    */
  { VK_OEM_3, KCTRL, {0xf6, 0xe9, WCH_NONE, 0xd6, 0xc9} },                     /* ö é Ö É*/
  { VK_OEM_6, NOCAPS, {WCH_DEAD, WCH_DEAD, WCH_DEAD,  WCH_NONE, 0x00} },       /*    ^ ` ~    */
      { 0xff, NOCAPS, {0x5e    , 0x27    , 0x7e     , WCH_NONE, 0x00} },
  { VK_OEM_7, KCTRL, {0xe4, 0xe0, 0x7b, 0xc4, 0xc0} },   /* ä à { Ä À */
  { VK_OEM_PLUS, NOCAPS, {WCH_DEAD, 0x21    , 0x5D    , WCH_NONE, 0x00} },    /*    ¨ ! ]    */
         { 0xff, NOCAPS, {0xa8    , WCH_NONE, WCH_NONE, WCH_NONE, 0x00} },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS1 keypad_numbers[] = {
  { VK_DECIMAL, 0, {'.'} }, /* I have no idea why this has to be like this. Shouldn't it be a "."? */
  { VK_NUMPAD0, 0, {'0'} },
  { VK_NUMPAD1, 0, {'1'} },
  { VK_NUMPAD2, 0, {'2'} },
  { VK_NUMPAD3, 0, {'3'} },
  { VK_NUMPAD4, 0, {'4'} },
  { VK_NUMPAD5, 0, {'5'} },
  { VK_NUMPAD6, 0, {'6'} },
  { VK_NUMPAD7, 0, {'7'} },
  { VK_NUMPAD8, 0, {'8'} },
  { VK_NUMPAD9, 0, {'9'} },
//  { VK_BACK,    0, '\010' },
  { 0,0 }
};

#define vk_master(n,x) { (PVK_TO_WCHARS1)x, n, sizeof(x[0]) }

ROSDATA VK_TO_WCHAR_TABLE vk_to_wchar_master_table[] = {
  vk_master(3,key_to_chars_3mod),
  vk_master(4,key_to_chars_4mod),
  vk_master(5,key_to_chars_5mod),
  vk_master(2,key_to_chars_2mod),
  vk_master(1,keypad_numbers),
  { 0,0,0 }
};

ROSDATA VSC_LPWSTR key_names[] = {
  { 0x00, L"" },
  { 0x01, L"Esc" },
  { 0x0e, L"R\x00fc" L"ck" },
  { 0x0f, L"Tabulator" },
  { 0x1c, L"Eingabe" },
  { 0x1d, L"Ctrl" },
  { 0x2a, L"Umschalt Links" },
  { 0x36, L"Umschalt Rechts" },
  { 0x37, L" (Zehnertastatur)" },
  { 0x38, L"Alt" },
  { 0x39, L"Leer" },
  { 0x3a, L"Caps Lock" },
  { 0x3b, L"F1" },
  { 0x3c, L"F2" },
  { 0x3d, L"F3" },
  { 0x3e, L"F4" },
  { 0x3f, L"F5" },
  { 0x40, L"F6" },
  { 0x41, L"F7" },
  { 0x42, L"F8" },
  { 0x43, L"F9" },
  { 0x44, L"F10" },
  { 0x45, L"Pause" },
  { 0x46, L"Rollen-Feststell" },
  { 0x47, L"7 (Zehnertastatur)" },
  { 0x48, L"8 (Zehnertastatur)" },
  { 0x49, L"9 (Zehnertastatur)" },
  { 0x4a, L"- (Zehnertastatur)" },
  { 0x4b, L"4 (Zehnertastatur)" },
  { 0x4c, L"5 (Zehnertastatur)" },
  { 0x4d, L"6 (Zehnertastatur)" },
  { 0x4e, L"+ (Zehnertastatur)" },
  { 0x4f, L"1 (Zehnertastatur)" },
  { 0x50, L"2 (Zehnertastatur)" },
  { 0x51, L"3 (Zehnertastatur)" },
  { 0x52, L"0 (Zehnertastatur)" },
  { 0x53, L"Punkt (Zehnertastatur)" },
  { 0x54, L"Sys Req" },
  { 0x57, L"F11" },
  { 0x58, L"F12" },
  { 0x7c, L"F13" },
  { 0x7d, L"F14" },
  { 0x7e, L"F15" },
  { 0x7f, L"F16" },
  { 0x80, L"F17" },
  { 0x81, L"F18" },
  { 0x82, L"F19" },
  { 0x83, L"F20" },
  { 0x84, L"F21" },
  { 0x85, L"F22" },
  { 0x86, L"F23" },
  { 0x87, L"F24" },
  { 0, NULL },
};

ROSDATA VSC_LPWSTR extended_key_names[] = {
  { 0x1c, L"Eingabe (Zehnertastatur" },
  { 0x1d, L"Strg-Rechts" },
  { 0x35, L" (Zehnertastatur)" },
  { 0x37, L"Druck" },
  { 0x38, L"Alt Gr" },
  { 0x45, L"Num-Feststell" },
  { 0x46, L"Untbr" },
  { 0x47, L"Pos1" },
  { 0x48, L"Nach-Oben" },
  { 0x49, L"Bild-Nach-Oben" },
  { 0x4b, L"Nach-Links" },
//{ 0x4c, L"Center" },
  { 0x4d, L"Nach-Rechts" },
  { 0x4f, L"Ende" },
  { 0x50, L"Nach-Unten" },
  { 0x51, L"Bild-Nach-Unten" },
  { 0x52, L"Einfg" },
  { 0x53, L"Entf" },
  { 0x54, L"<ReactOS>" },
  { 0x55, L"Hilfe" },
  { 0x56, L"Linke <ReactOS>" },
  { 0x5b, L"Rechte <ReactOS>" },
  { 0, NULL },
};

ROSDATA DEADKEY_LPWSTR dead_key_names[] = {
    L"\x00b4"	L"Akut",
    L"`"	L"Gravis",
    L"^"	L"Zirkumflex",
    NULL
};

#define DEADTRANS(ch, accent, comp, flags) MAKELONG(ch, accent), comp, flags

ROSDATA DEADKEY dead_key[] = {
  { DEADTRANS(L'a', L'^', 0xe2, 0x00) },/* ^ */
  { DEADTRANS(L'e', L'^', 0xea, 0x00) },
  { DEADTRANS(L'i', L'^', 0xee, 0x00) },
  { DEADTRANS(L'o', L'^', 0xf4, 0x00) },
  { DEADTRANS(L'u', L'^', 0xfb, 0x00) },
  { DEADTRANS(L'A', L'^', 0xc2, 0x00) },
  { DEADTRANS(L'E', L'^', 0xca, 0x00) },
  { DEADTRANS(L'I', L'^', 0xce, 0x00) },
  { DEADTRANS(L'O', L'^', 0xd4, 0x00) },
  { DEADTRANS(L'U', L'^', 0xdb, 0x00) },
  { DEADTRANS(L' ', L'^', L'^', 0x00) },
  { DEADTRANS(L'a', 0xb4, 0xe1, 0x00) }, /* ´ */
  { DEADTRANS(L'e', 0xb4, 0xe9, 0x00) },
  { DEADTRANS(L'i', 0xb4, 0xed, 0x00) },
  { DEADTRANS(L'o', 0xb4, 0xf3, 0x00) },
  { DEADTRANS(L'u', 0xb4, 0xfa, 0x00) },
  { DEADTRANS(L'y', 0xb4, 0xfd, 0x00) },
  { DEADTRANS(L'A', 0xb4, 0xc1, 0x00) },
  { DEADTRANS(L'E', 0xb4, 0xc9, 0x00) },
  { DEADTRANS(L'I', 0xb4, 0xcd, 0x00) },
  { DEADTRANS(L'O', 0xb4, 0xd3, 0x00) },
  { DEADTRANS(L'U', 0xb4, 0xda, 0x00) },
  { DEADTRANS(L'Y', 0xb4, 0xdd, 0x00) },
  { DEADTRANS(L' ', 0xb4, 0xb4, 0x00) },
  { DEADTRANS(L'a', L'`', 0xe0, 0x00) }, /* ` */
  { DEADTRANS(L'e', L'`', 0xe8, 0x00) },
  { DEADTRANS(L'i', L'`', 0xec, 0x00) },
  { DEADTRANS(L'o', L'`', 0xf2, 0x00) },
  { DEADTRANS(L'u', L'`', 0xf9, 0x00) },
  { DEADTRANS(L'A', L'`', 0xc0, 0x00) },
  { DEADTRANS(L'E', L'`', 0xc8, 0x00) },
  { DEADTRANS(L'I', L'`', 0xcc, 0x00) },
  { DEADTRANS(L'O', L'`', 0xd2, 0x00) },
  { DEADTRANS(L'U', L'`', 0xd9, 0x00) },
  { DEADTRANS(L' ', L'`', L'`', 0x00) },
  { DEADTRANS(L' ', 0xa8, 0xa8, 0x00) }, /* ¨ */
  { DEADTRANS(L'a', 0xa8, 0xe4, 0x00) },
  { DEADTRANS(L'e', 0xa8, 0xeb, 0x00) },
  { DEADTRANS(L'i', 0xa8, 0xef, 0x00) },
  { DEADTRANS(L'o', 0xa8, 0xf6, 0x00) },
  { DEADTRANS(L'u', 0xa8, 0xfc, 0x00) },
  { DEADTRANS(L'y', 0xa8, 0xff, 0x00) },
  { DEADTRANS(L'A', 0xa8, 0xc4, 0x00) },
  { DEADTRANS(L'E', 0xa8, 0xcb, 0x00) },
  { DEADTRANS(L'I', 0xa8, 0xcf, 0x00) },
  { DEADTRANS(L'O', 0xa8, 0xd6, 0x00) },
  { DEADTRANS(L'U', 0xa8, 0xdc, 0x00) },
  { DEADTRANS(L' ', 0x7e, 0x7e, 0x00) }, /* ~ */
  { DEADTRANS(L'a', 0x7e, 0xe3, 0x00) },
  { DEADTRANS(L'o', 0x7e, 0xf5, 0x00) },
  { DEADTRANS(L'n', 0x7e, 0xf1, 0x00) },
  { DEADTRANS(L'A', 0x7e, 0xc3, 0x00) },
  { DEADTRANS(L'O', 0x7e, 0xd5, 0x00) },
  { DEADTRANS(L'N', 0x7e, 0xd1, 0x00) },
  { 0, 0 }
};

/* Finally, the master table */
ROSDATA KBDTABLES keyboard_layout_table = {
  /* modifier assignments */
  &modifier_bits,

  /* character from vk tables */
  vk_to_wchar_master_table,

  /* diacritical marks -- currently implemented by wine code */
  dead_key,

  /* Key names */
  (VSC_LPWSTR *)key_names,
  (VSC_LPWSTR *)extended_key_names,
  dead_key_names, /* Dead key names */

  /* scan code to virtual key maps */
  scancode_to_vk,
  sizeof(scancode_to_vk) / sizeof(scancode_to_vk[0]),
  extcode0_to_vk,
  extcode1_to_vk,

  MAKELONG(1,1), /* Version 1.0 */

  /* Ligatures -- German doesn't have any */
  0,
  0,
  NULL
};

PKBDTABLES WINAPI KbdLayerDescriptor(VOID) {
  return &keyboard_layout_table;
}

