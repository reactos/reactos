/*
 * PROJECT:     ReactOS Keyboard Layouts
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Italian (142) keyboard layout
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winuser.h>
#include <ndk/kbd.h>

/* See also: https://kbdlayout.info/kbdit142 */

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

#define VK_EMPTY  0xff                 /* The non-existent VK */
#define KNUMS     KBDNUMPAD|KBDSPECIAL /* Special + number pad */
#define KMEXT     KBDEXT|KBDMULTIVK    /* Multi + ext */

/* ------------------------------------------------------------------ */
/* Scancode -> virtual key (0x00 .. 0x7e, bMaxVSCtoVK = 0x7f)         */
/* ------------------------------------------------------------------ */
ROSDATA USHORT scancode_to_vk[] = {
  /* 00 */ VK_EMPTY,     VK_ESCAPE,    '1',          '2',
  /* 04 */ '3',          '4',          '5',          '6',
  /* 08 */ '7',          '8',          '9',          '0',
  /* 0C */ VK_OEM_4,     VK_OEM_6,     VK_BACK,      VK_TAB,
  /* 10 */ 'Q',          'W',          'E',          'R',
  /* 14 */ 'T',          'Y',          'U',          'I',
  /* 18 */ 'O',          'P',          VK_OEM_1,     VK_OEM_PLUS,
  /* 1C */ VK_RETURN,    VK_LCONTROL,  'A',          'S',
  /* 20 */ 'D',          'F',          'G',          'H',
  /* 24 */ 'J',          'K',          'L',          VK_OEM_3,
  /* 28 */ VK_OEM_7,     VK_OEM_5,     VK_LSHIFT,    VK_OEM_2,
  /* 2C */ 'Z',          'X',          'C',          'V',
  /* 30 */ 'B',          'N',          'M',          VK_OEM_COMMA,
  /* 34 */ VK_OEM_PERIOD,VK_OEM_MINUS, VK_RSHIFT | KBDEXT,
  /* 37 */ VK_MULTIPLY | KBDMULTIVK,
  /* 38 */ VK_LMENU,     VK_SPACE,     VK_CAPITAL,
  /* 3B */ VK_F1, VK_F2, VK_F3, VK_F4, VK_F5,
  /* 40 */ VK_F6, VK_F7, VK_F8, VK_F9, VK_F10,
  /* 45 */ VK_NUMLOCK | KMEXT,
  /* 46 */ VK_SCROLL | KBDMULTIVK,
  /* 47 */ VK_HOME | KNUMS,  VK_UP | KNUMS,    VK_PRIOR | KNUMS, VK_SUBTRACT,
  /* 4B */ VK_LEFT | KNUMS,  VK_CLEAR | KNUMS, VK_RIGHT | KNUMS, VK_ADD,
  /* 4F */ VK_END | KNUMS,   VK_DOWN | KNUMS,  VK_NEXT | KNUMS,
  /* 52 */ VK_INSERT | KNUMS, VK_DELETE | KNUMS,
  /* 54 */ VK_SNAPSHOT,
  /* 55 */ VK_EMPTY,
  /* 56 */ VK_OEM_102,   VK_F11,       VK_F12,
  /* 59 */ VK_CLEAR,     VK_OEM_WSCTRL, VK_OEM_FINISH, VK_OEM_JUMP,
  /* 5D */ VK_EREOF,     VK_OEM_BACKTAB, VK_OEM_AUTO,
  /* 60 */ VK_EMPTY,     VK_EMPTY,     VK_ZOOM,      VK_HELP,
  /* 64 */ VK_F13, VK_F14, VK_F15, VK_F16, VK_F17, VK_F18, VK_F19, VK_F20,
  /* 6C */ VK_F21, VK_F22, VK_F23,
  /* 6F */ VK_OEM_PA3,   VK_EMPTY,     VK_OEM_RESET, VK_EMPTY,
  /* 73 */ VK_ABNT_C1,   VK_EMPTY,     VK_EMPTY,
  /* 76 */ VK_F24,
  /* 77 */ VK_EMPTY,     VK_EMPTY,     VK_EMPTY,     VK_EMPTY,
  /* 7B */ VK_OEM_PA1,   VK_TAB,       VK_EMPTY,     VK_ABNT_C2,
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
  { 0, 0 }
};

ROSDATA VSC_VK extcode1_to_vk[] = {
  { 0x1D, VK_PAUSE },
  { 0, 0 }
};

/* ------------------------------------------------------------------ */
/* Modifiers: Shift / Ctrl / Alt  (AltGr = Ctrl+Alt)                  */
/*                                                                    */
/*   bits  : 0 none, 1 Shift, 2 Ctrl, 3 Shift+Ctrl, 4 Alt, 5 Shift+Alt, */
/*           6 Ctrl+Alt (AltGr)                                       */
/*   column: 0 = Normal, 1 = Shift, 2 = Ctrl, 3 = AltGr,              */
/*           4 = Shift+Ctrl                                           */
/* ------------------------------------------------------------------ */
ROSDATA VK_TO_BIT modifier_keys[] = {
  { VK_SHIFT,   KBDSHIFT },
  { VK_CONTROL, KBDCTRL },
  { VK_MENU,    KBDALT },
  { 0,          0 }
};

ROSDATA MODIFIERS modifier_bits = {
  modifier_keys,
  6,
  {
      0,            /* NONE        */
      1,            /* SHIFT       */
      2,            /* CTRL        */
      4,            /* SHIFT+CTRL  */
      SHFT_INVALID, /* ALT         */
      SHFT_INVALID, /* SHIFT+ALT   */
      3             /* CTRL+ALT (AltGr) */
  }
};

/* ------------------------------------------------------------------ */
/* VK -> WCHAR tables                                                 */
/* ------------------------------------------------------------------ */

/* Normal, Shift */
ROSDATA VK_TO_WCHARS2 key_to_chars_2mod[] = {
  { VK_OEM_5,      0,     {'\\', '|'} },
  { '1',           0,     {'1', '!'} },
  { '2',           0,     {'2', '\"'} },
  { '4',           0,     {'4', '$'} },
  { VK_OEM_4,      0,     {'\'', '?'} },
  { VK_OEM_6,      0,     {0x00EC /* i grave */, '^'} },
  { 'W',           CAPLOK, {'w', 'W'} },
  { 'R',           CAPLOK, {'r', 'R'} },
  { 'T',           CAPLOK, {'t', 'T'} },
  { 'Y',           CAPLOK, {'y', 'Y'} },
  { 'U',           CAPLOK, {'u', 'U'} },
  { 'I',           CAPLOK, {'i', 'I'} },
  { 'O',           CAPLOK, {'o', 'O'} },
  { 'P',           CAPLOK, {'p', 'P'} },
  { 'A',           CAPLOK, {'a', 'A'} },
  { 'S',           CAPLOK, {'s', 'S'} },
  { 'D',           CAPLOK, {'d', 'D'} },
  { 'F',           CAPLOK, {'f', 'F'} },
  { 'G',           CAPLOK, {'g', 'G'} },
  { 'H',           CAPLOK, {'h', 'H'} },
  { 'J',           CAPLOK, {'j', 'J'} },
  { 'K',           CAPLOK, {'k', 'K'} },
  { 'L',           CAPLOK, {'l', 'L'} },
  { VK_OEM_3,      0,     {0x00F2 /* o grave */, 0x00E7 /* c cedilla */} },
  { VK_OEM_7,      0,     {0x00E0 /* a grave */, 0x00B0 /* degree */} },
  { 'Z',           CAPLOK, {'z', 'Z'} },
  { 'X',           CAPLOK, {'x', 'X'} },
  { 'C',           CAPLOK, {'c', 'C'} },
  { 'V',           CAPLOK, {'v', 'V'} },
  { 'B',           CAPLOK, {'b', 'B'} },
  { 'N',           CAPLOK, {'n', 'N'} },
  { 'M',           CAPLOK, {'m', 'M'} },
  { VK_OEM_COMMA,  0,     {',', ';'} },
  { VK_OEM_PERIOD, 0,     {'.', ':'} },
  { VK_DECIMAL,    0,     {',', ','} },
  { VK_TAB,        0,     {'\t', '\t'} },
  { VK_ADD,        0,     {'+', '+'} },
  { VK_DIVIDE,     0,     {'/', '/'} },
  { VK_MULTIPLY,   0,     {'*', '*'} },
  { VK_SUBTRACT,   0,     {'-', '-'} },
  { 0, 0 }
};

/* Normal, Shift, Ctrl */
ROSDATA VK_TO_WCHARS3 key_to_chars_3mod[] = {
  { VK_OEM_1,     0, {0x00E8 /* e grave */, 0x00E9 /* e acute */, 0x1b /* ESC */} },
  { VK_OEM_MINUS, 0, {'-', '_', 0x1f /* US */} },
  { VK_OEM_102,   0, {'<', '>', 0x1c /* FS */} },
  { VK_BACK,      0, {0x08, 0x08, 0x7F} },
  { VK_ESCAPE,    0, {0x1b, 0x1b, 0x1b} },
  { VK_RETURN,    0, {'\r', '\r', '\n'} },
  { VK_SPACE,     0, {' ', ' ', ' '} },
  { VK_CANCEL,    0, {0x03, 0x03, 0x03} },
  { 0, 0 }
};

/* Normal, Shift, Ctrl, AltGr */
ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] = {
  { '3',        0,      {'3', 0x00A3 /* POUND */, WCH_NONE, '#'} },
  { '5',        0,      {'5', '%', WCH_NONE, 0x20AC /* EURO */} },
  { '7',        0,      {'7', '/', WCH_NONE, '{'} },
  { '8',        0,      {'8', '(', WCH_NONE, '['} },
  { '9',        0,      {'9', ')', WCH_NONE, ']'} },
  { '0',        0,      {'0', '=', WCH_NONE, '}'} },
  { 'Q',        CAPLOK, {'q', 'Q', WCH_NONE, '@'} },
  { 'E',        CAPLOK, {'e', 'E', WCH_NONE, 0x20AC /* EURO */} },
  { VK_OEM_PLUS,0,      {'+', '*', 0x1d /* GS */, '~'} },
  { VK_OEM_2,   0,      {0x00F9 /* u grave */, 0x00A7 /* section */, 0x1c /* FS */, '`'} },
  { 0, 0 }
};

/* Normal, Shift, Ctrl, AltGr, Shift+Ctrl */
ROSDATA VK_TO_WCHARS5 key_to_chars_5mod[] = {
  { '6',        0,      {'6', '&', WCH_NONE, WCH_NONE, 0x1e /* RS */} },
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

/* Order in the binary: 3, 4, 5, 2, 1 */
ROSDATA VK_TO_WCHAR_TABLE vk_to_wchar_master_table[] = {
  vk_master(3, key_to_chars_3mod),
  vk_master(4, key_to_chars_4mod),
  vk_master(5, key_to_chars_5mod),
  vk_master(2, key_to_chars_2mod),
  vk_master(1, keypad_numbers),
  { 0, 0, 0 }
};

/* ------------------------------------------------------------------ */
/* Key names (Italian)                                                */
/* ------------------------------------------------------------------ */
ROSDATA VSC_LPWSTR key_names[] = {
  { 0x01, L"ESC" },        { 0x0e, L"BACKSPACE" },  { 0x0f, L"TAB" },
  { 0x1c, L"INVIO" },      { 0x1d, L"CTRL" },       { 0x2a, L"MAIUSC" },
  { 0x36, L"MAIUSC (DESTRA)" }, { 0x37, L"* (Tn)" }, { 0x38, L"ALT" },
  { 0x39, L"BARRA SPAZIATRICE" }, { 0x3a, L"BLOC MAIUS" },
  { 0x3b, L"F1" },  { 0x3c, L"F2" },  { 0x3d, L"F3" },  { 0x3e, L"F4" },
  { 0x3f, L"F5" },  { 0x40, L"F6" },  { 0x41, L"F7" },  { 0x42, L"F8" },
  { 0x43, L"F9" },  { 0x44, L"F10" },
  { 0x45, L"PAUSA" },      { 0x46, L"BLOC SCORR" },
  { 0x47, L"7 (Tn)" },  { 0x48, L"8 (Tn)" },  { 0x49, L"9 (Tn)" },  { 0x4a, L"- (Tn)" },
  { 0x4b, L"4 (Tn)" },  { 0x4c, L"5 (Tn)" },  { 0x4d, L"6 (Tn)" },  { 0x4e, L"PI\x00D9 (Tn)" },
  { 0x4f, L"1 (Tn)" },  { 0x50, L"2 (Tn)" },  { 0x51, L"3 (Tn)" },
  { 0x52, L"0 (Tn)" },  { 0x53, L". (Tn)" },
  { 0x57, L"F11" },  { 0x58, L"F12" },
  { 0, NULL }
};

ROSDATA VSC_LPWSTR extended_key_names[] = {
  { 0x1c, L"INVIO (TN)" },  { 0x1d, L"CTRL (DESTRA)" },
  { 0x35, L"/ (Tn)" },      { 0x37, L"STAMPA" },      { 0x38, L"ALT GR" },
  { 0x45, L"BLOC NUM" },    { 0x46, L"INTERR" },
  { 0x47, L"HOME" },        { 0x48, L"Freccia SU" },  { 0x49, L"PGSU" },
  { 0x4b, L"Freccia SINISTRA" }, { 0x4d, L"Freccia DESTRA" },
  { 0x4f, L"FINE" },        { 0x50, L"Freccia GI\x00D9" }, { 0x51, L"PGGI\x00D9" },
  { 0x52, L"INS" },         { 0x54, L"<00>" },        { 0x56, L"AIUTO" },
  { 0x53, L"CANCELLA" },
  { 0x5b, L"WINDOWS SINISTRO" }, { 0x5c, L"WINDOWS DESTRO" }, { 0x5d, L"APPLICAZIONE" },
  { 0, NULL }
};

/* ------------------------------------------------------------------ */
/* Master table                                                       */
/* ------------------------------------------------------------------ */
ROSDATA KBDTABLES keyboard_layout_table = {
  &modifier_bits,
  vk_to_wchar_master_table,
  NULL,                              /* no dead keys */
  (VSC_LPWSTR *)key_names,
  (VSC_LPWSTR *)extended_key_names,
  NULL,                              /* no dead key names */
  scancode_to_vk,
  RTL_NUMBER_OF(scancode_to_vk),
  extcode0_to_vk,
  extcode1_to_vk,
  MAKELONG(KLLF_ALTGR, KBD_VERSION),
  0,                                 /* nLgMax: no ligatures */
  0,                                 /* cbLgEntry */
  NULL                               /* no ligature table */
};

PKBDTABLES WINAPI KbdLayerDescriptor(VOID) {
  return &keyboard_layout_table;
}
