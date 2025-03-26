/*
 * ReactOS Icelandic Keyboard layout
 * Copyright (C) 2008 ReactOS
 * Author: Dmitry Chapyshev
 * License: LGPL, see: LGPL.txt
 *
 * Thanks to: http://www.barcodeman.com/altek/mule/scandoc.php
 * and http://win.tue.nl/~aeb/linux/kbd/scancodes-1.html
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
  VK_OEM_MINUS, VK_OEM_PLUS,  VK_BACK,
  /* - 0f - */
  /* First Letters Row */
  VK_TAB,       'Q',          'W',          'E',
  'R',          'T',          'Y',          'U',
  'I',          'O',          'P',
  VK_OEM_4,     VK_OEM_6,     VK_RETURN,
  /* - 1d - */
  /* Second Letters Row */
  VK_LCONTROL,
  'A',          'S',          'D',          'F',
  'G',          'H',          'J',          'K',
  'L',          VK_OEM_1,     VK_OEM_7,     VK_OEM_3,
  VK_LSHIFT,    VK_OEM_5,
  /* - 2c - */
  /* Third letters row */
  'Z',          'X',          'C',          'V',
  'B',          'N',          'M',          VK_OEM_COMMA,
  VK_OEM_PERIOD,VK_OEM_2,     VK_RSHIFT | KBDEXT,
  /* - 37 - */
  /* Bottom Row */
  0x26a,  VK_LMENU,     VK_SPACE,     VK_CAPITAL,

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
  VK_EMPTY,     VK_OEM_102,     VK_F11,       VK_F12,
  /* - 59 - */
  VK_CLEAR,     VK_OEM_WSCTRL,VK_OEM_FINISH,VK_OEM_JUMP,  VK_EREOF, /* EREOF */
  VK_OEM_BACKTAB,    VK_OEM_AUTO,  VK_EMPTY,    VK_ZOOM,            /* ZOOM */
  VK_HELP,
  /* - 64 - */
  /* Even more F-Keys (for example, NCR keyboards from the early 90's) */
  VK_F13, VK_F14, VK_F15, VK_F16, VK_F17, VK_F18, VK_F19, VK_F20,
  VK_F21, VK_F22, VK_F23,
  /* - 6f - */
  /* Not sure who uses these codes */
  VK_OEM_PA3, VK_EMPTY, VK_OEM_RESET,
  /* - 72 - */
  VK_EMPTY, 0xc1, VK_EMPTY, VK_EMPTY,
  /* - 76 - */
  /* One more f-key */
  VK_F24,
  /* - 77 - */
  VK_EMPTY, VK_EMPTY, VK_EMPTY, VK_EMPTY,
  VK_OEM_PA1, VK_TAB, 0xc2, VK_EMPTY, /* PA1 */
  VK_EMPTY,
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
  6,
  { 0, 1, 3, 4, SHFT_INVALID, SHFT_INVALID, 2 }
};

ROSDATA VK_TO_WCHARS2 key_to_chars_2mod[] = {
  { '1',           0, {'1',     '!'   } },
  { '2',           0, {'2',     '\"'  } },
  { '3',           0, {'3',     '#'   } },
  { '4',           0, {'4',     '$'   } },
  { VK_OEM_4,      0, {'-',     '_'   } },
  { 'W',           CAPLOK,   {'w',     'W'   } },
  { 'R',           CAPLOK,   {'r',     'R'   } },
  { 'T',           CAPLOK,   {'t',     'T'   } },
  { 'Y',           CAPLOK,   {'y',     'Y'   } },
  { 'U',           CAPLOK,   {'u',     'U'   } },
  { 'I',           CAPLOK,   {'i',     'I'   } },
  { 'O',           CAPLOK,   {'o',     'O'   } },
  { 'P',           CAPLOK,   {'p',     'P'   } },
  { 'A',           CAPLOK,   {'a',     'A'   } },
  { 'S',           CAPLOK,   {'s',     'S'   } },
  { 'D',           CAPLOK,   {'d',     'D'   } },
  { 'F',           CAPLOK,   {'f',     'F'   } },
  { 'G',           CAPLOK,   {'g',     'G'   } },
  { 'H',           CAPLOK,   {'h',     'H'   } },
  { 'J',           CAPLOK,   {'j',     'J'   } },
  { 'K',           CAPLOK,   {'k',     'K'   } },
  { 'L',           CAPLOK,   {'l',     'L'   } },
  { VK_OEM_3,      CAPLOK,   {0x00e6,  0x00c6} },
  { 'Z',           CAPLOK,   {'z',     'Z'   } },
  { 'X',           CAPLOK,   {'x',     'X'   } },
  { 'C',           CAPLOK,   {'c',     'C'   } },
  { 'V',           CAPLOK,   {'v',     'V'   } },
  { 'B',           CAPLOK,   {'b',     'B'   } },
  { 'N',           CAPLOK,   {'n',     'N'   } },
  { VK_OEM_COMMA,  0, {',',     ';'   } },
  { VK_OEM_PERIOD, 0, {'.',     ':'   } },
  { VK_DECIMAL,    0, {',',     ','   } },
  { VK_TAB,        0, {'\t',    '\t'  } },
  { VK_ADD,        0, {'+',     '+'   } },
  { VK_DIVIDE,     0, {'/',     '/'   } },
  { VK_MULTIPLY,   0, {'*',     '*'   } },
  { VK_SUBTRACT,   0, {'-',     '-'   } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS3 key_to_chars_3mod[] = {
  /* Normal, Shifted, Ctrl */
  /* Legacy (telnet-style) ascii escapes */
  { '5',         0, {'5',      '%',      0x20ac  } },
  { '7',         0, {'7',      '/',      '{'     } },
  { '8',         0, {'8',      '(',      '['     } },
  { '9',         0, {'9',      ')',      ']'     } },
  { '0',         0, {'0',      '=',      '}'     } },
  { VK_OEM_PLUS, CAPLOK,   {0x00f6,   0x00d6,   '\\'    } },
  { 'Q',         CAPLOK,   {'q',      'Q',      '@'     } },
  { 'E',         CAPLOK,   {'e',      'E',      0x20ac  } },
  { VK_OEM_2,    0, {'+',      '*',      WCH_DEAD} },
  { 0xff,        0, {WCH_NONE, WCH_NONE, '`'     } },
  { VK_OEM_7,    0, {WCH_DEAD, '\'',     WCH_DEAD} },
  { 0xff,        0, {0x00b4,   WCH_NONE, '^'     } },
  { 'M',         CAPLOK,   {'m',      'M',      0x00b5  } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] = {
  /* Normal, Shifted, Ctrl-Alt, Ctrl */
  /* Legacy Ascii generators */
  { VK_OEM_5,     0, {WCH_DEAD, WCH_DEAD, 0x00b0,0x001c     } },
  { 0xff,         0, {0x00b0,   0x00a8,   WCH_NONE, WCH_NONE} },
  { VK_OEM_6,     CAPLOK,   {0x00f0,   0x00d0,   WCH_NONE, 0x001b  } },
  { VK_OEM_1,     0, {'\'',     '?',      '~',      0x001d  } },
  { VK_OEM_MINUS, CAPLOK,   {0x00fe,   0x00de,   WCH_NONE, 0x001f  } },
  { VK_OEM_102,   0, {'<',      '>',      '|',      0x001c  } },
  { VK_BACK,      0, {'\b',     '\b',     WCH_NONE, 0x007f  } },
  { VK_ESCAPE,    0, {0x001b,   0x001b,   WCH_NONE, 0x001b  } },
  { VK_RETURN,    0, {'\r',     '\r',     WCH_NONE, '\n'    } },
  { VK_SPACE,     0, {' ',      ' ',      WCH_NONE, ' '     } },
  { VK_CANCEL,    0, {0x0003,   0x0003,   WCH_NONE, 0x0003  } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS5 key_to_chars_5mod[] = {
  /* Normal, Shifted, Ctrl-Alt, Ctrl, S+Ctrl */
  /* Legacy Ascii generators */
  { '6', 0, {'6', '&', WCH_NONE, WCH_NONE, 0x001e} },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS1 keypad_numbers[] = {
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
  { 0x01, L"Esc" },
  { 0x0e, L"Backspace" },
  { 0x0f, L"Tab" },
  { 0x1c, L"Enter" },
  { 0x1d, L"Ctrl" },
  { 0x2a, L"Shift" },
  { 0x36, L"Right Shift" },
  { 0x37, L"Num *" },
  { 0x38, L"Alt" },
  { 0x39, L"Space" },
  { 0x3a, L"CAPLOK Lock" },
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
  { 0x46, L"Scroll Lock" },
  { 0x47, L"Num 7" },
  { 0x48, L"Num 8" },
  { 0x49, L"Num 9" },
  { 0x4a, L"Num -" },
  { 0x4b, L"Num 4" },
  { 0x4c, L"Num 5" },
  { 0x4d, L"Num 6" },
  { 0x4e, L"Num +" },
  { 0x4f, L"Num 1" },
  { 0x50, L"Num 2" },
  { 0x51, L"Num 3" },
  { 0x52, L"Num 0" },
  { 0x53, L"Num Del" },
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
  { 0, NULL }
};

ROSDATA VSC_LPWSTR extended_key_names[] = {
  { 0x1c, L"Num Enter" },
  { 0x1d, L"Right Control" },
  { 0x35, L"Num /" },
  { 0x37, L"Prnt Scrn" },
  { 0x38, L"Right Alt" },
  { 0x45, L"Num Lock" },
  { 0x46, L"Break" },
  { 0x47, L"Home" },
  { 0x48, L"Up" },
  { 0x49, L"Page Up" },
  { 0x4b, L"Left" },
//{ 0x4c, L"Center" },
  { 0x4d, L"Right" },
  { 0x4f, L"End" },
  { 0x50, L"Down" },
  { 0x51, L"Page Down" },
  { 0x52, L"Insert" },
  { 0x53, L"Delete" },
  { 0x54, L"<ReactOS>" },
  { 0x56, L"Help" },
  { 0x5b, L"Left <ReactOS>" },
  { 0x5c, L"Right <ReactOS>" },
  { 0x5d, L"Application" },
  { 0, NULL }
};

ROSDATA DEADKEY_LPWSTR dead_key_names[] = {
    L"\x00b4"	L"ACUTE",
    L"`"	    L"GRAVE",
    L"^"	    L"CIRCUMFLEX",
    L"\x00a8"	L"UMLAUT",
    L"\x00b0"	L"RING",
    NULL
};

#define DEADTRANS(ch, accent, comp, flags) MAKELONG(ch, accent), comp, flags

ROSDATA DEADKEY dead_key[] = {
  { DEADTRANS(L'a', L'`',   0x00e0, 0x0000) },
  { DEADTRANS(L'e', L'`',   0x00e8, 0x0000) },
  { DEADTRANS(L'i', L'`',   0x00ec, 0x0000) },
  { DEADTRANS(L'o', L'`',   0x00f2, 0x0000) },
  { DEADTRANS(L'u', L'`',   0x00f9, 0x0000) },
  { DEADTRANS(L'A', L'`',   0x00c0, 0x0000) },
  { DEADTRANS(L'E', L'`',   0x00c8, 0x0000) },
  { DEADTRANS(L'I', L'`',   0x00cc, 0x0000) },
  { DEADTRANS(L'O', L'`',   0x00d2, 0x0000) },
  { DEADTRANS(L'U', L'`',   0x00d9, 0x0000) },
  { DEADTRANS(L' ', L'`',   L'`',   0x0000) },

  { DEADTRANS(L'a', 0x00b4, 0x00e1, 0x0000) },
  { DEADTRANS(L'e', 0x00b4, 0x00e9, 0x0000) },
  { DEADTRANS(L'i', 0x00b4, 0x00ed, 0x0000) },
  { DEADTRANS(L'o', 0x00b4, 0x00f3, 0x0000) },
  { DEADTRANS(L'u', 0x00b4, 0x00fa, 0x0000) },
  { DEADTRANS(L'y', 0x00b4, 0x00fd, 0x0000) },
  { DEADTRANS(L'A', 0x00b4, 0x00c1, 0x0000) },
  { DEADTRANS(L'E', 0x00b4, 0x00c9, 0x0000) },
  { DEADTRANS(L'I', 0x00b4, 0x00cd, 0x0000) },
  { DEADTRANS(L'O', 0x00b4, 0x00d3, 0x0000) },
  { DEADTRANS(L'U', 0x00b4, 0x00da, 0x0000) },
  { DEADTRANS(L'Y', 0x00b4, 0x00dd, 0x0000) },
  { DEADTRANS(L' ', 0x00b4, 0x00b4, 0x0000) },

  { DEADTRANS(L'a', L'^',   0x00e2, 0x0000) },
  { DEADTRANS(L'e', L'^',   0x00ea, 0x0000) },
  { DEADTRANS(L'i', L'^',   0x00ee, 0x0000) },
  { DEADTRANS(L'o', L'^',   0x00f4, 0x0000) },
  { DEADTRANS(L'u', L'^',   0x00fb, 0x0000) },
  { DEADTRANS(L'A', L'^',   0x00c2, 0x0000) },
  { DEADTRANS(L'E', L'^',   0x00ca, 0x0000) },
  { DEADTRANS(L'I', L'^',   0x00ce, 0x0000) },
  { DEADTRANS(L'O', L'^',   0x00d4, 0x0000) },
  { DEADTRANS(L'U', L'^',   0x00db, 0x0000) },
  { DEADTRANS(L' ', L'^',   L'^',   0x0000) },

  { DEADTRANS(L'a', 0x00b0, 0x00e5, 0x0000) },
  { DEADTRANS(L'A', 0x00b0, 0x00c5, 0x0000) },
  { DEADTRANS(L' ', 0x00b0, 0x00b0, 0x0000) },

  { DEADTRANS(L'a', 0x00a8, 0x00e4, 0x0000) },
  { DEADTRANS(L'e', 0x00a8, 0x00eb, 0x0000) },
  { DEADTRANS(L'i', 0x00a8, 0x00ef, 0x0000) },
  { DEADTRANS(L'o', 0x00a8, 0x00f6, 0x0000) },
  { DEADTRANS(L'u', 0x00a8, 0x00fc, 0x0000) },
  { DEADTRANS(L'y', 0x00a8, 0x00ff, 0x0000) },
  { DEADTRANS(L'A', 0x00a8, 0x00c4, 0x0000) },
  { DEADTRANS(L'E', 0x00a8, 0x00cb, 0x0000) },
  { DEADTRANS(L'I', 0x00a8, 0x00cf, 0x0000) },
  { DEADTRANS(L'O', 0x00a8, 0x00d6, 0x0000) },
  { DEADTRANS(L'U', 0x00a8, 0x00dc, 0x0000) },
  { DEADTRANS(L' ', 0x00a8, 0x00a8, 0x0000) },
  { 0, 0 }
};

/* Finally, the master table */
ROSDATA KBDTABLES keyboard_layout_table = {
  /* modifier assignments */
  &modifier_bits,

  /* character from vk tables */
  vk_to_wchar_master_table,

  /* diacritical marks */
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

  /* Ligatures -- Icelandic doesn't have any */
  0,
  0,
  NULL
};

PKBDTABLES WINAPI KbdLayerDescriptor(VOID) {
  return &keyboard_layout_table;
}

INT WINAPI
DllMain(
  PVOID hinstDll,
  ULONG dwReason,
  PVOID reserved)
{
  return 1;
}

