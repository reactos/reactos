/*
 * ReactOS Esperanto Keyboard layout
 * Copyright (C) 2003-2016 ReactOS
 * License: LGPL, see: LGPL.txt
 *
 * Thanks to: http://www.barcodeman.com/altek/mule/scandoc.php
 * and http://win.tue.nl/~aeb/linux/kbd/scancodes-1.html
 *
 * Esperanto layout recreated from the X.Org version
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
  VK_OEM_PERIOD,VK_OEM_2,     VK_RSHIFT,
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
  VK_EMPTY,     VK_OEM_102,     VK_F11,       VK_F12,
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
  { 0x1d, VK_PAUSE },
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
  7,
  {
      0,            // NONE
      1,            // SHIFT
      2,            // CTRL
      3,            // SHIFT+CTRL
      0,            // ALT (not used)
      0,            // SHIFT-ALT (not used)
      4,            // CTRL-ALT
      5,            // SHIFT-CTRL-ALT
  }
};

ROSDATA VK_TO_WCHARS2 key_to_chars_2mod[] = {
  /* Normal vs Shifted */
  /* The alphabet */
  { 'B',         CAPLOK,   {'b', 'B'} },
  { 'C',         CAPLOK,   {'c', 'C'} },
  { 'E',         CAPLOK,   {'e', 'E'} },
  { 'G',         CAPLOK,   {'g', 'G'} },
  { 'H',         CAPLOK,   {'h', 'H'} },
  { 'I',         CAPLOK,   {'i', 'I'} },
  { 'J',         CAPLOK,   {'j', 'J'} },
  { 'K',         CAPLOK,   {'k', 'K'} },
  { 'L',         CAPLOK,   {'l', 'L'} },
  { 'R',         CAPLOK,   {'r', 'R'} },
  { 'T',         CAPLOK,   {'t', 'T'} },
  { 'U',         CAPLOK,   {'u', 'U'} },
  { 'V',         CAPLOK,   {'v', 'V'} },
  { 'Z',         CAPLOK,   {'z', 'Z'} },
  /* The numbers */
  { '1',         0, {'1', '!'} },
  { '3',         0, {'3', '#'} },
  { '7',         0, {'7', '&'} },
  { '8',         0, {'8', '*'} },
  { '9',         0, {'9', '('} },
  { '0',         0, {'0', ')'} },

  /* Specials */
  /* Ctrl-_ generates US */
  { VK_OEM_PLUS,   0, {'=', '+'} },
  { VK_OEM_1,      0, {';', ':'} },
  { VK_OEM_7,      0, {'\'','\"'} },
  { VK_OEM_3,      0, {'`', '~'} },
  { VK_OEM_COMMA,  0, {',', '<'} },
  { VK_OEM_PERIOD, 0, {'.', '>'} },
  { VK_OEM_2,      0, {'/', '?'} },
  /* Keys that do not have shift states */
  { VK_TAB,      0, {'\t','\t'} },
  { VK_ADD,      0, {'+', '+'} },
  { VK_SUBTRACT, 0, {'-', '-'} },
  { VK_DECIMAL,  0, {'.', '.'} },
  { VK_MULTIPLY, 0, {'*', '*'} },
  { VK_DIVIDE,   0, {'/', '/'} },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS3 key_to_chars_3mod[] = {
  /* Normal, Shifted, Ctrl */
  /* Legacy (telnet-style) ascii escapes */
  { VK_OEM_5,   0, {'\\','|', 0x1c /* FS */} },
  { VK_OEM_102, 0, {'\\','|', 0x1c /* FS */} },
  { VK_BACK,    0, {0x8, 0x8, 0x7F } },
  { VK_ESCAPE,  0, {0x1b,0x1b,0x1b } },
  { VK_RETURN,  0, {'\r','\r','\n'} },
  { VK_SPACE,   0, {' ', ' ', ' '} },
  { VK_CANCEL,  0, {0x3, 0x3, 0x3} },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] = {
  /* Normal, Shifted, Ctrl, C-S-x */
  /* Legacy Ascii generators */
  { '2',          0, {'2', '@', WCH_NONE, 0} },
  { '6',          0, {'6', '^', WCH_NONE, 0x1e /* RS */} },
  { VK_OEM_MINUS, 0, {'-', '_', WCH_NONE, 0x1f /* US */} },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS5 key_to_chars_5mod[] = {
  /* Normal, Shifted, Ctrl, C-S-x, AltGr */
  { 'A', 1, {'a', 'A', WCH_NONE, WCH_NONE, 0x2018 /* ‘ */ } },
  { 'S', 1, {'s', 'S', WCH_NONE, WCH_NONE, 0x2019 /* ’ */ } },
  { 'D', 1, {'d', 'D', WCH_NONE, WCH_NONE, 0x201c /* “ */ } },
  { 'F', 1, {'f', 'F', WCH_NONE, WCH_NONE, 0x201d /* ” */ } },
  { 'M', 1, {'m', 'M', WCH_NONE, WCH_NONE, 0x2014 /* — */ } },
  { 'N', 1, {'n', 'N', WCH_NONE, WCH_NONE, 0x2013 /* – */ } },
  { 'O', 1, {'o', 'O', WCH_NONE, WCH_NONE, '{' } },
  { 'P', 1, {'p', 'P', WCH_NONE, WCH_NONE, '}' } },
  { '5', 0, {'5', '%', WCH_NONE, WCH_NONE, 0x20ac /* € */ } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS6 key_to_chars_6mod[] = {
  /* Normal, Shifted, Ctrl, C-S-x, AltGr, Shift-AltGr */
  { 'Q',      5, {0x015d, 0x015c, WCH_NONE, WCH_NONE, 'q', 'Q' /* ŝŜ */ } },
  { 'W',      5, {0x011d, 0x011c, WCH_NONE, WCH_NONE, 'w', 'W' /* ĝĜ */ } },
  { 'X',      5, {0x0109, 0x0108, WCH_NONE, WCH_NONE, 'x', 'X' /* ĉĈ */ } },
  { 'Y',      5, {0x016d, 0x016c, WCH_NONE, WCH_NONE, 'y', 'Y' /* ŭŬ */ } },
  { VK_OEM_4, 1, {0x0135, 0x0134, 0x1b, WCH_NONE, '[', '{', /* ĵĴ, ESC */ } },
  { VK_OEM_6, 1, {0x0125, 0x0124, 0x1d, WCH_NONE, ']', '}', /* ĥĤ, GS */ } },
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
  { 0, 0 }
};

#define vk_master(n, x) { (PVK_TO_WCHARS1)x, n, sizeof(x[0]) }

ROSDATA VK_TO_WCHAR_TABLE vk_to_wchar_master_table[] = {
  vk_master(1, keypad_numbers),
  vk_master(2, key_to_chars_2mod),
  vk_master(3, key_to_chars_3mod),
  vk_master(4, key_to_chars_4mod),
  vk_master(5, key_to_chars_5mod),
  vk_master(6, key_to_chars_6mod),
  { 0, 0, 0 }
};

ROSDATA VSC_LPWSTR key_names[] = {
  { 0x01, L"Eskapklavo" },
  { 0x0e, L"Retropaŝo" },
  { 0x0f, L"Tabo" },
  { 0x1c, L"Tirilo" },
  { 0x1d, L"Stirklavo" },
  { 0x2a, L"Majuskliga" },
  { 0x36, L"Dekstra majuskliga" },
  { 0x37, L"Nom *" },
  { 0x38, L"Alt" },
  { 0x39, L"Spaco" },
  { 0x3a, L"Majusklo ŝlosi" },
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
  { 0x45, L"Paŭzo" },
  { 0x46, L"Rulumi ŝlosi" },
  { 0x47, L"Nom 7" },
  { 0x48, L"Nom 8" },
  { 0x49, L"Nom 9" },
  { 0x4a, L"Nom -" },
  { 0x4b, L"Nom 4" },
  { 0x4c, L"Nom 5" },
  { 0x4d, L"Nom 6" },
  { 0x4e, L"Nom +" },
  { 0x4f, L"Nom 1" },
  { 0x50, L"Nom 2" },
  { 0x51, L"Nom 3" },
  { 0x52, L"Nom 0" },
  { 0x53, L"Nom forviŝi" },
  { 0x54, L"Sistemo peto" },
  { 0x56, L"\\" },
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
  { 0x1c, L"Nom Tirilo" },
  { 0x1d, L"Dekstra Stirklavo" },
  { 0x35, L"Nom /" },
  { 0x37, L"Presi Ekrano" },
  { 0x38, L"Dekstra Alt" },
  { 0x45, L"Nom Ŝlosi" },
  { 0x46, L"Paŭzi" },
  { 0x47, L"Hejmo" },
  { 0x48, L"Supren" },
  { 0x49, L"Paĝo Supren" },
  { 0x4a, L"-" },
  { 0x4b, L"Maldekstra" },
  { 0x4d, L"Dekstra" },
  { 0x4f, L"Fino" },
  { 0x50, L"Doĝn" },
  { 0x51, L"Paĝo Doĝn" },
  { 0x52, L"Enigaĵo" },
  { 0x53, L"Forviŝi" },
  { 0x54, L"<00>" },
  { 0x56, L"Helpo" },
  { 0x5b, L"Maldesktra Windows" },
  { 0x5c, L"Desktra Windows" },
  { 0x5d, L"Apliko" },
  { 0, NULL },
};

/* Finally, the master table */
ROSDATA KBDTABLES keyboard_layout_table = {
  /* modifier assignments */
  &modifier_bits,

  /* character from vk tables */
  vk_to_wchar_master_table,

  /* diacritical marks -- currently implemented by wine code */
  /* Esperanto doesn't have any, anyway */
  NULL,

  /* Key names */
  (VSC_LPWSTR *)key_names,
  (VSC_LPWSTR *)extended_key_names,
  NULL, /* Dead key names */

  /* scan code to virtual key maps */
  scancode_to_vk,
  sizeof(scancode_to_vk) / sizeof(scancode_to_vk[0]),
  extcode0_to_vk,
  extcode1_to_vk,

  MAKELONG(KLLF_ALTGR, 1), /* Version 1.0 */

  /* Ligatures -- Esperanto doesn't have any */
  0,
  0,
  NULL
};

PKBDTABLES WINAPI KbdLayerDescriptor(VOID) {
  return &keyboard_layout_table;
}
