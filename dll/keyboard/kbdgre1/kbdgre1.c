/*
 * ReactOS German Extended (E1) Keyboard layout
 * Copyright (C) 2025 ReactOS
 * License: LGPL, see: LGPL.txt
 *
 * Thanks to: http://www.barcodeman.com/altek/mule/scandoc.php
 * and http://win.tue.nl/~aeb/linux/kbd/scancodes-1.html
 */

/*
 * This keyboard layout is Work In Progress!
 *
 * Most dead key translations are still missing.
 */

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winuser.h>
#include <ndk/kbd.h>

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

#define VK_EMPTY  0xff   /* The non-existent VK */

#define KNUMS     KBDNUMPAD|KBDSPECIAL /* Special + number pad */
#define KMEXT     KBDEXT|KBDMULTIVK    /* Multi + ext */

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
  VK_LSHIFT,    VK_OEM_2,
  /* - 2c - */
  /* Third letters row */
  'Y',          'X',          'C',          'V',
  'B',          'N',          'M',          VK_OEM_COMMA,
  VK_OEM_PERIOD,VK_OEM_MINUS,     VK_RSHIFT,
  /* - 37 - */
  /* Bottom Row */
  VK_MULTIPLY,  VK_LMENU,     VK_SPACE,     VK_CAPITAL,

  /* - 3b - */
  /* F-Keys */
  VK_F1, VK_F2, VK_F3, VK_F4, VK_F5, VK_F6,
  VK_F7, VK_F8, VK_F9, VK_F10,
  /* - 45 - */
  /* Locks */
  VK_NUMLOCK | KMEXT,
  VK_SCROLL | KBDMULTIVK,
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
};

ROSDATA VSC_VK extcode0_to_vk[] = {
  { 0x10, VK_MEDIA_PREV_TRACK | KBDEXT },
  { 0x19, VK_MEDIA_NEXT_TRACK | KBDEXT },
  { 0x1D, VK_RCONTROL | KBDEXT },
  { 0x20, VK_VOLUME_MUTE | KBDEXT },
  { 0x21, VK_LAUNCH_APP2 | KBDEXT },
  { 0x22, VK_MEDIA_PLAY_PAUSE | KBDEXT },
  { 0x24, VK_MEDIA_STOP | KBDEXT },
  { 0x2E, VK_VOLUME_DOWN | KBDEXT },
  { 0x30, VK_VOLUME_UP | KBDEXT },
  { 0x32, VK_BROWSER_HOME | KBDEXT },
  { 0x35, VK_DIVIDE | KBDEXT },
  { 0x37, VK_SNAPSHOT | KBDEXT },
  { 0x38, VK_RMENU | KBDEXT },
  { 0x47, VK_HOME | KBDEXT },
  { 0x48, VK_UP | KBDEXT },
  { 0x49, VK_PRIOR | KBDEXT },
  { 0x4B, VK_LEFT | KBDEXT },
  { 0x4D, VK_RIGHT | KBDEXT },
  { 0x4F, VK_END | KBDEXT },
  { 0x50, VK_DOWN | KBDEXT },
  { 0x51, VK_NEXT | KBDEXT },
  { 0x52, VK_INSERT | KBDEXT },
  { 0x53, VK_DELETE | KBDEXT },
  { 0x5B, VK_LWIN | KBDEXT },
  { 0x5C, VK_RWIN | KBDEXT },
  { 0x5D, VK_APPS | KBDEXT },
  { 0x5F, VK_SLEEP | KBDEXT },
  { 0x65, VK_BROWSER_SEARCH | KBDEXT },
  { 0x66, VK_BROWSER_FAVORITES | KBDEXT },
  { 0x67, VK_BROWSER_REFRESH | KBDEXT },
  { 0x68, VK_BROWSER_STOP | KBDEXT },
  { 0x69, VK_BROWSER_FORWARD | KBDEXT },
  { 0x6A, VK_BROWSER_BACK | KBDEXT },
  { 0x6B, VK_LAUNCH_APP1 | KBDEXT },
  { 0x6C, VK_LAUNCH_MAIL | KBDEXT },
  { 0x6D, VK_LAUNCH_MEDIA_SELECT | KBDEXT },
  { 0x1C, VK_RETURN | KBDEXT },
  { 0x46, VK_CANCEL | KBDEXT },
  { 0, 0 },
};

ROSDATA VSC_VK extcode1_to_vk[] = {
  { 0x1d, VK_PAUSE},
  { 0, 0 },
};

ROSDATA VK_TO_BIT modifier_keys[] = {
  { VK_SHIFT,   KBDSHIFT },
  { VK_CONTROL, KBDCTRL },
  { VK_MENU,    KBDALT },
  { 0,          0 }
};

ROSDATA MODIFIERS modifier_bits = {
  modifier_keys,
  8,
  /* Modifier bit order, NONE, SHIFT, CTRL, SHIFT+CTRL, ALT, SHIFT+ALT, CTRL+ALT, SHIFT+CTRL+ALT */
  { 0, 1, 4, 5, SHFT_INVALID, SHFT_INVALID, 2, 3 }
};

ROSDATA VK_TO_WCHARS2 key_to_chars_2mod[] = {
  /* Normal, Shift */
  { VK_DECIMAL,    0,                  {',',      ',' } },
  { VK_TAB,        0,                  {'\t',     '\t'} },
  { VK_ADD,        0,                  {'+',      '+'} },
  { VK_DIVIDE,     0,                  {'/',      '/'} },
  { VK_MULTIPLY,   0,                  {'*',      '*'} },
  { VK_SUBTRACT,   0,                  {'-',      '-'} },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS3 key_to_chars_3mod[] = {
  /* Normal, Shift, Ctrl+Alt */
  { VK_OEM_5,      0,                  {WCH_DEAD, 0x00b0,   0x00d7  } },
  { 0xff,          0,                  {'^',      WCH_NONE, WCH_NONE} },
  { '1',           CAPLOK,             {'1',      '!',      0x2019  } },
  { '3',           CAPLOK,             {'3',      0x00a7,   0x00b3  } },
  { '4',           CAPLOK,             {'4',      '$',      0x2014  } },
  { '5',           CAPLOK,             {'5',      '%',      0x00a1  } },
  { '7',           CAPLOK,             {'7',      '/',      '{'     } },
  { '8',           CAPLOK,             {'8',      '(',      '['     } },
  { '9',           CAPLOK,             {'9',      ')',      ']'     } },
  { '0',           CAPLOK,             {'0',      '=',      '}'     } },
  { VK_OEM_6,      0,                  {WCH_DEAD, WCH_DEAD, WCH_DEAD} },
  { 0xff,          0,                  {0x00b4,   '`',      0x02d9  } },
  { 'Q',           CAPLOK,             {'q',      'Q',      '@'     } },
  { 'W',           CAPLOK,             {'w',      'W',      WCH_DEAD} },
  { 0xff,          0,                  {WCH_NONE, WCH_NONE, 0x00af  } },
  { 'E',           CAPLOK,             {'e',      'E',      0x20ac  } },
  { 'R',           CAPLOK,             {'r',      'R',      WCH_DEAD} },
  { 0xff,          0,                  {WCH_NONE, WCH_NONE, 0x02dd  } },
  { 'T',           CAPLOK,             {'t',      'T',      WCH_DEAD} },
  { 0xff,          0,                  {WCH_NONE, WCH_NONE, 0x02c7  } },
  { 'Z',           CAPLOK,             {'z',      'Z',      WCH_DEAD} },
  { 0xff,          0,                  {WCH_NONE, WCH_NONE, 0x00a8  } },
  { 'U',           CAPLOK,             {'u',      'U',      WCH_DEAD} },
  { 0xff,          0,                  {WCH_NONE, WCH_NONE, 0x02d8  } },
  { 'I',           CAPLOK,             {'i',      'I',      WCH_DEAD} },
  { 0xff,          0,                  {WCH_NONE, WCH_NONE, 0x02dc  } },
  { 'O',           CAPLOK,             {'o',      'O',      WCH_DEAD} },
  { 0xff,          0,                  {WCH_NONE, WCH_NONE, 0x02da  } },
  { 'P',           CAPLOK,             {'p',      'P',      WCH_DEAD} },
  { 0xff,          0,                  {WCH_NONE, WCH_NONE, 0x02c0  } },
  { 'A',           CAPLOK,             {'a',      'A',      0x263a  } },
  { 'S',           CAPLOK,             {'s',      'S',      0x2033  } },
  { 'D',           CAPLOK,             {'d',      'D',      0x2032  } },
  { 'F',           CAPLOK,             {'f',      'F',      WCH_DEAD} },
  { 0xff,          0,                  {WCH_NONE, WCH_NONE, 0x02df  } },
  { 'G',           CAPLOK,             {'g',      'G',      0x1e9e  } },
  { 'H',           CAPLOK,             {'h',      'H',      WCH_DEAD} },
  { 0xff,          0,                  {WCH_NONE, WCH_NONE, 0x02cd  } },
  { 'J',           CAPLOK,             {'j',      'J',      WCH_DEAD} },
  { 0xff,          0,                  {WCH_NONE, WCH_NONE, 0x00b8  } },
  { 'K',           CAPLOK,             {'k',      'K',      WCH_DEAD} },
  { 0xff,          0,                  {WCH_NONE, WCH_NONE, 0x02cf  } },
  { 'L',           CAPLOK,             {'l',      'L',      WCH_DEAD} },
  { 0xff,          0,                  {WCH_NONE, WCH_NONE, 0x02db  } },
  { VK_OEM_3,      CAPLOK,             {0x00f6,   0x00d6,   WCH_DEAD} },
  { 0xff,          0,                  {WCH_NONE, WCH_NONE, 0x02cc  } },
  { VK_OEM_7,      CAPLOK,             {0x00e4,   0x00c4,   WCH_DEAD} },
  { 0xff,          0,                  {WCH_NONE, WCH_NONE, 0x02d7  } },
  { VK_OEM_102,    0,                  {'<',      '>',      '|'     } },
  { 'Y',           CAPLOK,             {'y',      'Y',      0x203a  } },
  { 'X',           CAPLOK,             {'x',      'X',      0x00bb  } },
  { 'C',           CAPLOK,             {'c',      'C',      0x202f  } },
  { 'V',           CAPLOK,             {'v',      'V',      0x20ab  } },
  { 'B',           CAPLOK,             {'b',      'B',      0x2039  } },
  { 'N',           CAPLOK,             {'n',      'N',      0x2013  } },
  { 'M',           CAPLOK,             {'m',      'M',      0x00b5  } },
  { VK_OEM_COMMA,  CAPLOK,             {',',      ';',      0x2011  } },
  { VK_OEM_PERIOD, CAPLOK,             {'.',      ':',      0x00b7  } },
  { VK_SPACE,      0,                  {' ',      ' ',      0x00a0  } },
  { 0,0 }
};

ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] = {
  /* Normal, Shift, Ctrl+Alt, Shift+Ctrl+Alt */
  { VK_OEM_4,      CAPLOK,             {0x00df,   '?',      '\\',     0x1e9e  } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS5 key_to_chars_5mod[] = {
  /* Normal, Shift, Ctrl+Alt, Shift+Ctrl+Alt, Ctrl */
  { '2',           CAPLOK,             {'2',      '\"',     0x00b2,   WCH_NONE, 0x0000  } },
  { '6',           CAPLOK,             {'6',      '&',      0x00bf,   WCH_NONE, 0x001e  } },
  { VK_OEM_1,      CAPLOK,             {0x00fc,   0x00dc,   WCH_DEAD, WCH_NONE, 0x001b  } },
  { 0xff,          0,                  {WCH_NONE, WCH_NONE, 0x02bc,   WCH_NONE, WCH_NONE} },
  { VK_OEM_PLUS,   CAPLOK,             {'+',      '*',      '~',      WCH_NONE, 0x001d  } },
  { VK_OEM_2,      CAPLOK,             {'#',      '\'',     0x2212,   WCH_NONE, 0x001c  } },

  { VK_BACK,       0,                  {'\b',     '\b',     WCH_NONE, WCH_NONE, 0x007f  } },
  { VK_ESCAPE,     0,                  {0x001b,   0x001b,   WCH_NONE, WCH_NONE, 0x001b  } },
  { VK_RETURN,     0,                  {'\r',     '\r',     WCH_NONE, WCH_NONE, '\n'    } },
  { VK_CANCEL,     0,                  {0x0003,   0x0003,   WCH_NONE, WCH_NONE, 0x0003  } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS6 key_to_chars_6mod[] = {
  /* Normal, Shift, Ctrl+Alt, Shift+Ctrl+Alt, Ctrl, Shift+Ctrl */
  { VK_OEM_MINUS,  0,                  {'-',      '_',      0x00ad,   WCH_NONE, WCH_NONE, 0x001f  } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS1 keypad_numbers[] = {
  { VK_DECIMAL, 0, {'.'} },
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
  vk_master(6,key_to_chars_6mod),
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
  { 0x1d, L"Strg" },
  { 0x2a, L"Umschalt" },
  { 0x36, L"Umschalt Rechts" },
  { 0x37, L" (Zehnertastatur)" },
  { 0x38, L"Alt" },
  { 0x39, L"Leer" },
  { 0x3a, L"Feststell" },
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
  { 0x53, L"Komma (Zehnertastatur)" },
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
    L"\x00b4" L"AKUT",
    L"`"      L"GRAVIS",
    L"\x00af" L"MAKRON",
    L"\x02d9" L"\x00dcBERPUNKT",
    L"^"      L"ZIRKUMFLEX",
    L"\x02dd" L"DOPPELAKUT",
    L"\x02c7" L"HATSCHEK",
    L"\x00a8" L"TREMA",
    L"\x02d8" L"BREVE",
    L"\x02dc" L"TILDE",
    L"\x02da" L"RING",
    L"\x02c0" L"HORN",
    L"\x02bc" L"HAKEN",
    L"\x02df" L"EXTRA-WAHLTASTE",
    L"\x02cd" L"UNTERSTRICH",
    L"\x00b8" L"CEDILLA",
    L"\x02cf" L"UNTERKOMMA",
    L"\x02db" L"OGONEK",
    L"\x02cc" L"UNTERPUNKT",
    L"\x02d7" L"QUERSTRICHAKZENT",
    NULL
};

#define DEADTRANS(ch, accent, comp, flags) MAKELONG(ch, accent), comp, flags

ROSDATA DEADKEY dead_key[] = {
  { DEADTRANS(L' ', L'^', L'^',   0x00) }, /* circumflex */
  { DEADTRANS(L'0', L'^', 0x2070, 0x00) },
  { DEADTRANS(L'1', L'^', 0x00b9, 0x00) },
  { DEADTRANS(L'2', L'^', 0x00b2, 0x00) },
  { DEADTRANS(L'3', L'^', 0x00b3, 0x00) },
  { DEADTRANS(L'4', L'^', 0x2074, 0x00) },
  { DEADTRANS(L'5', L'^', 0x2075, 0x00) },
  { DEADTRANS(L'6', L'^', 0x2076, 0x00) },
  { DEADTRANS(L'7', L'^', 0x2077, 0x00) },
  { DEADTRANS(L'8', L'^', 0x2078, 0x00) },
  { DEADTRANS(L'9', L'^', 0x2079, 0x00) },
  { DEADTRANS(L'a', L'^', 0x00e2, 0x00) },
  { DEADTRANS(L'c', L'^', 0x0109, 0x00) },
  { DEADTRANS(L'e', L'^', 0x00ea, 0x00) },
  { DEADTRANS(L'g', L'^', 0x011d, 0x00) },
  { DEADTRANS(L'h', L'^', 0x0125, 0x00) },
  { DEADTRANS(L'i', L'^', 0x00ee, 0x00) },
  { DEADTRANS(L'j', L'^', 0x0135, 0x00) },
  { DEADTRANS(L'o', L'^', 0x00f4, 0x00) },
  { DEADTRANS(L's', L'^', 0x015d, 0x00) },
  { DEADTRANS(L'u', L'^', 0x00fb, 0x00) },
  { DEADTRANS(L'w', L'^', 0x0175, 0x00) },
  { DEADTRANS(L'y', L'^', 0x0177, 0x00) },
  { DEADTRANS(L'z', L'^', 0x1e91, 0x00) },
  { DEADTRANS(L'A', L'^', 0x00c2, 0x00) },
  { DEADTRANS(L'C', L'^', 0x0108, 0x00) },
  { DEADTRANS(L'E', L'^', 0x00ca, 0x00) },
  { DEADTRANS(L'G', L'^', 0x011c, 0x00) },
  { DEADTRANS(L'H', L'^', 0x0124, 0x00) },
  { DEADTRANS(L'I', L'^', 0x00ce, 0x00) },
  { DEADTRANS(L'J', L'^', 0x0134, 0x00) },
  { DEADTRANS(L'O', L'^', 0x00d4, 0x00) },
  { DEADTRANS(L'S', L'^', 0x015c, 0x00) },
  { DEADTRANS(L'U', L'^', 0x00db, 0x00) },
  { DEADTRANS(L'W', L'^', 0x0174, 0x00) },
  { DEADTRANS(L'Y', L'^', 0x0176, 0x00) },
  { DEADTRANS(L'Z', L'^', 0x1e90, 0x00) },
  { DEADTRANS(L'(', L'^', 0x207d, 0x00) },
  { DEADTRANS(L')', L'^', 0x207e, 0x00) },
  { DEADTRANS(L'+', L'^', 0x207a, 0x00) },
  { DEADTRANS(L'-', L'^', 0x207b, 0x00) },
  { DEADTRANS(L'=', L'^', 0x2259, 0x00) },
  { DEADTRANS(L'^', L'^', 0x0302, 0x00) },

  { DEADTRANS(L'a', 0xb4, 0xe1, 0x00) }, /* acute */
  { DEADTRANS(L'c', 0xb4, 0x107, 0x00) },
  { DEADTRANS(L'e', 0xb4, 0xe9, 0x00) },
  { DEADTRANS(L'g', 0xb4, 0x1f5, 0x00) },
  { DEADTRANS(L'i', 0xb4, 0xed, 0x00) },
  { DEADTRANS(L'k', 0xb4, 0x1e31, 0x00) },
  { DEADTRANS(L'l', 0xb4, 0x13a, 0x00) },
  { DEADTRANS(L'm', 0xb4, 0x1e3f, 0x00) },
  { DEADTRANS(L'n', 0xb4, 0x144, 0x00) },
  { DEADTRANS(L'o', 0xb4, 0xf3, 0x00) },
  { DEADTRANS(L'p', 0xb4, 0x1e55, 0x00) },
  { DEADTRANS(L'r', 0xb4, 0x155, 0x00) },
  { DEADTRANS(L's', 0xb4, 0x15b, 0x00) },
  { DEADTRANS(L'u', 0xb4, 0xfa, 0x00) },
  { DEADTRANS(L'w', 0xb4, 0x1e83, 0x00) },
  { DEADTRANS(L'y', 0xb4, 0xfd, 0x00) },
  { DEADTRANS(L'z', 0xb4, 0x17a, 0x00) },
  { DEADTRANS(0xfc, 0xb4, 0x1d8, 0x00) },
  { DEADTRANS(L'A', 0xb4, 0xc1, 0x00) },
  { DEADTRANS(L'C', 0xb4, 0x106, 0x00) },
  { DEADTRANS(L'E', 0xb4, 0xc9, 0x00) },
  { DEADTRANS(L'G', 0xb4, 0x1f4, 0x00) },
  { DEADTRANS(L'I', 0xb4, 0xcd, 0x00) },
  { DEADTRANS(L'K', 0xb4, 0x1e30, 0x00) },
  { DEADTRANS(L'L', 0xb4, 0x139, 0x00) },
  { DEADTRANS(L'M', 0xb4, 0x1e3e, 0x00) },
  { DEADTRANS(L'N', 0xb4, 0x143, 0x00) },
  { DEADTRANS(L'O', 0xb4, 0xd3, 0x00) },
  { DEADTRANS(L'P', 0xb4, 0x1e54, 0x00) },
  { DEADTRANS(L'R', 0xb4, 0x154, 0x00) },
  { DEADTRANS(L'S', 0xb4, 0x15a, 0x00) },
  { DEADTRANS(L'U', 0xb4, 0xda, 0x00) },
  { DEADTRANS(L'W', 0xb4, 0x1e82, 0x00) },
  { DEADTRANS(L'Y', 0xb4, 0xdd, 0x00) },
  { DEADTRANS(L'Z', 0xb4, 0x179, 0x00) },
  { DEADTRANS(0xdc, 0xb4, 0x1d7, 0x00) },
  { DEADTRANS(L' ', 0xb4, 0xb4, 0x00) },
  { DEADTRANS(0xb4, 0xb4, 0x301, 0x00) },

  { DEADTRANS(L'a', L'`', 0xe0, 0x00) }, /* grave */
  { DEADTRANS(L'e', L'`', 0xe8, 0x00) },
  { DEADTRANS(L'i', L'`', 0xec, 0x00) },
  { DEADTRANS(L'n', L'`', 0x1f9, 0x00) },
  { DEADTRANS(L'o', L'`', 0xf2, 0x00) },
  { DEADTRANS(L'u', L'`', 0xf9, 0x00) },
  { DEADTRANS(L'w', L'`', 0x1e81, 0x00) },
  { DEADTRANS(L'y', L'`', 0x1ef3, 0x00) },
  { DEADTRANS(0xfc, L'`', 0x1dc, 0x00) },
  { DEADTRANS(L'A', L'`', 0xc0, 0x00) },
  { DEADTRANS(L'E', L'`', 0xc8, 0x00) },
  { DEADTRANS(L'I', L'`', 0xcc, 0x00) },
  { DEADTRANS(L'N', L'`', 0x1f8, 0x00) },
  { DEADTRANS(L'O', L'`', 0xd2, 0x00) },
  { DEADTRANS(L'U', L'`', 0xd9, 0x00) },
  { DEADTRANS(L'W', L'`', 0x1e80, 0x00) },
  { DEADTRANS(L'Y', L'`', 0x1ef2, 0x00) },
  { DEADTRANS(0xdc, L'`', 0x1db, 0x00) },
  { DEADTRANS(L' ', L'`', L'`', 0x00) },
  { DEADTRANS(L'`', L'`', 0x300, 0x00) },

  { DEADTRANS(L'a', 0x2d9, 0x227, 0x00) },  /* dot above */
  { DEADTRANS(L'b', 0x2d9, 0x1e03, 0x00) },
  { DEADTRANS(L'c', 0x2d9, 0x10b, 0x00) },
  { DEADTRANS(L'd', 0x2d9, 0x1e0b, 0x00) },
  { DEADTRANS(L'e', 0x2d9, 0x117, 0x00) },
  { DEADTRANS(L'f', 0x2d9, 0x1e1f, 0x00) },
  { DEADTRANS(L'g', 0x2d9, 0x121, 0x00) },
  { DEADTRANS(L'h', 0x2d9, 0x1e23, 0x00) },
  { DEADTRANS(L'l', 0x2d9, 0x140, 0x00) },
  { DEADTRANS(L'm', 0x2d9, 0x1e41, 0x00) },
  { DEADTRANS(L'n', 0x2d9, 0x1e45, 0x00) },
  { DEADTRANS(L'o', 0x2d9, 0x22f, 0x00) },
  { DEADTRANS(L'p', 0x2d9, 0x1e57, 0x00) },
  { DEADTRANS(L'r', 0x2d9, 0x1e59, 0x00) },
  { DEADTRANS(L's', 0x2d9, 0x1e61, 0x00) },
  { DEADTRANS(L't', 0x2d9, 0x1e6b, 0x00) },
  { DEADTRANS(L'w', 0x2d9, 0x1e87, 0x00) },
  { DEADTRANS(L'x', 0x2d9, 0x1e8b, 0x00) },
  { DEADTRANS(L'z', 0x2d9, 0x17c, 0x00) },
  { DEADTRANS(L'A', 0x2d9, 0x226, 0x00) },
  { DEADTRANS(L'B', 0x2d9, 0x1e02, 0x00) },
  { DEADTRANS(L'C', 0x2d9, 0x10a, 0x00) },
  { DEADTRANS(L'D', 0x2d9, 0x1e0a, 0x00) },
  { DEADTRANS(L'E', 0x2d9, 0x116, 0x00) },
  { DEADTRANS(L'F', 0x2d9, 0x1e1e, 0x00) },
  { DEADTRANS(L'G', 0x2d9, 0x120, 0x00) },
  { DEADTRANS(L'H', 0x2d9, 0x1e22, 0x00) },
  { DEADTRANS(L'I', 0x2d9, 0x130, 0x00) },
  { DEADTRANS(L'L', 0x2d9, 0x13f, 0x00) },
  { DEADTRANS(L'M', 0x2d9, 0x1e40, 0x00) },
  { DEADTRANS(L'N', 0x2d9, 0x1e44, 0x00) },
  { DEADTRANS(L'O', 0x2d9, 0x22e, 0x00) },
  { DEADTRANS(L'P', 0x2d9, 0x1e56, 0x00) },
  { DEADTRANS(L'R', 0x2d9, 0x1e58, 0x00) },
  { DEADTRANS(L'S', 0x2d9, 0x1e60, 0x00) },
  { DEADTRANS(L'T', 0x2d9, 0x1e6a, 0x00) },
  { DEADTRANS(L'W', 0x2d9, 0x1e86, 0x00) },
  { DEADTRANS(L'X', 0x2d9, 0x1e8a, 0x00) },
  { DEADTRANS(L'Z', 0x2d9, 0x17b, 0x00) },
  { DEADTRANS(L' ', 0x2d9, 0x2d9, 0x00) },
  { DEADTRANS(L'~', 0x2d9, 0x2e1e, 0x00) },
  { DEADTRANS(L'°', 0x2d9, 0x310, 0x00) },
  { DEADTRANS(0x2d9, 0x2d9, 0x307, 0x00) },

  { DEADTRANS(L'a', 0xaf, 0x101, 0x00) }, /* macron */
  { DEADTRANS(L'e', 0xaf, 0x113, 0x00) },
  { DEADTRANS(L'g', 0xaf, 0x1e21, 0x00) },
  { DEADTRANS(L'i', 0xaf, 0x12b, 0x00) },
  { DEADTRANS(L'o', 0xaf, 0x14d, 0x00) },
  { DEADTRANS(L'u', 0xaf, 0x16b, 0x00) },
  { DEADTRANS(L'y', 0xaf, 0x233, 0x00) },
  { DEADTRANS(0xe4, 0xaf, 0x1df, 0x00) }, /* ä */
  { DEADTRANS(0xf6, 0xaf, 0x22b, 0x00) }, /* ö */
  { DEADTRANS(0xfc, 0xaf, 0x1d6, 0x00) }, /* ü */
  { DEADTRANS(L'A', 0xaf, 0x100, 0x00) },
  { DEADTRANS(L'E', 0xaf, 0x112, 0x00) },
  { DEADTRANS(L'G', 0xaf, 0x1e20, 0x00) },
  { DEADTRANS(L'I', 0xaf, 0x12a, 0x00) },
  { DEADTRANS(L'O', 0xaf, 0x14c, 0x00) },
  { DEADTRANS(L'U', 0xaf, 0x16a, 0x00) },
  { DEADTRANS(L'Y', 0xaf, 0x232, 0x00) },
  { DEADTRANS(0xc4, 0xaf, 0x1de, 0x00) }, /* Ä */
  { DEADTRANS(0xd6, 0xaf, 0x22a, 0x00) }, /* Ö */
  { DEADTRANS(0xdc, 0xaf, 0x1d5, 0x00) }, /* Ü */
  { DEADTRANS(L'=', 0xaf, 0x2261, 0x00) },
  { DEADTRANS(L'-', 0xaf, 0x2e40, 0x00) },
  { DEADTRANS(0xaf, 0xaf, 0x304, 0x00) },

  { DEADTRANS(L' ', 0x02dd, 0x02dd, 0x00) }, /* DOPPELAKUT incomplete */

  { DEADTRANS(L' ', 0x02c7, 0x02c7, 0x00) }, /* HATCHEK incomplete */

  { DEADTRANS(L' ', 0x00a8, 0x00a8, 0x00) }, /* TREMA incomplete */

  { DEADTRANS(L' ', 0x02d8, 0x02d8, 0x00) }, /* BREVE incomplete */

  { DEADTRANS(L' ', 0x02dc, 0x02dc, 0x00) }, /* TILDE incomplete */

  { DEADTRANS(L' ', 0x02da, 0x02da, 0x00) }, /* RING incomplete */

  { DEADTRANS(L' ', 0x02c0, 0x02c0, 0x00) }, /* HORN incomplete */

  { DEADTRANS(L' ', 0x02bc, 0x02bc, 0x00) }, /* HAKEN incomplete */

  { DEADTRANS(L' ', 0x02df, 0x02df, 0x00) }, /* EXTRA-WAHLTASTE incomplete */

  { DEADTRANS(L' ', 0x02cd, 0x02cd, 0x00) }, /* UNTERSTRICH incomplete */

  { DEADTRANS(L' ', 0x00b8, 0x00b8, 0x00) }, /* CEDILLA incomplete */

  { DEADTRANS(L' ', 0x02cf, 0x02cf, 0x00) }, /* UNTERKOMMA incomplete */

  { DEADTRANS(L' ', 0x02db, 0x02db, 0x00) }, /* OGONEK incomplete */

  { DEADTRANS(L' ', 0x02cc, 0x02cc, 0x00) }, /* UNTERPUNKT incomplete */

  { DEADTRANS(L' ', 0x02d7, 0x02d7, 0x00) }, /* QUERSTRICHAKZENT incomplete */

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
  RTL_NUMBER_OF(scancode_to_vk),
  extcode0_to_vk,
  extcode1_to_vk,

  MAKELONG(KLLF_ALTGR, 1), /* Version 1.0 */

  /* Ligatures -- German doesn't have any */
  0,
  0,
  NULL
};

PKBDTABLES WINAPI KbdLayerDescriptor(VOID) {
  return &keyboard_layout_table;
}

