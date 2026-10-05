/*
 * PROJECT:     ReactOS Keyboard Layouts
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Japanese (JIS 106) keyboard layout
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define WIN32_NO_STATUS
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

#ifndef KBDKANA
  #define KBDKANA  0x08  /* Kana modifier bit */
#endif

#define VK_EMPTY   0xff  /* The non-existent VK */

#define VK_PA_FC   0xfc  /* Unnamed VK used by a few rarely used scancodes */
#define VK_PA_F9   0xf9
#define VK_OEM_EC  0xec  /* Unnamed VK used by scancode 0x7f */

#define KNUMS      (KBDNUMPAD | KBDSPECIAL) /* Special + number pad */
#define KMEXT      (KBDEXT | KBDMULTIVK)    /* Multi + ext */

/* Key attributes of the Japanese layout */
#define CAPKANA    (CAPLOK | KANALOK)     /* letters: Caps lock + kana lock */

/* ------------------------------------------------------------------ */
/* Scancode -> virtual key                                            */

ROSDATA USHORT scancode_to_vk[] = {
  /* - 00 - */
  VK_EMPTY,     VK_ESCAPE,    '1',          '2',
  '3',          '4',          '5',          '6',
  '7',          '8',          '9',          '0',
  VK_OEM_MINUS, VK_OEM_7,     VK_BACK,      VK_TAB,
  /* - 10 - */
  /* First Letters Row */
  'Q',          'W',          'E',          'R',
  'T',          'Y',          'U',          'I',
  'O',          'P',          VK_OEM_3,     VK_OEM_4,
  VK_RETURN,
  /* - 1d - */
  /* Second Letters Row */
  VK_LCONTROL,
  'A',          'S',          'D',          'F',
  'G',          'H',          'J',          'K',
  'L',          VK_OEM_PLUS,  VK_OEM_1,
  /* - 29 - Hankaku/Zenkaku */
  VK_DBE_SBCSCHAR | KBDSPECIAL,
  VK_LSHIFT,    VK_OEM_6,
  /* - 2c - */
  /* Third letters row */
  'Z',          'X',          'C',          'V',
  'B',          'N',          'M',          VK_OEM_COMMA,
  VK_OEM_PERIOD,VK_OEM_2,     VK_RSHIFT | KBDEXT,
  /* - 37 - */
  /* Bottom Row */
  VK_MULTIPLY | KBDMULTIVK,
  VK_LMENU,
  VK_SPACE,
  VK_DBE_ALPHANUMERIC | KBDSPECIAL, /* 0x3a: Caps Lock / Eisuu */

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
  VK_SNAPSHOT,
  /* - 55 - */
  /* Oddities, and the remaining standard F-Keys */
  VK_EMPTY,     VK_EMPTY,     VK_F11,       VK_F12,
  /* - 59 - */
  VK_CLEAR,     VK_PA_FC,     VK_PA_FC,     VK_PA_FC,
  VK_PA_F9,     VK_EMPTY,     VK_PA_FC,     VK_EMPTY,
  /* - 61 - */
  VK_EMPTY,     VK_EMPTY,     VK_EMPTY,
  /* - 64 - */
  /* Even more F-Keys */
  VK_F13, VK_F14, VK_F15, VK_F16, VK_F17, VK_F18, VK_F19, VK_F20,
  VK_F21, VK_F22, VK_F23,
  /* - 6f - */
  VK_EMPTY,
  /* - 70 - Katakana/Hiragana/Romaji */
  VK_DBE_HIRAGANA | KBDSPECIAL,
  /* - 71 - */
  VK_EMPTY, VK_EMPTY,
  /* - 73 - Backslash (Ro) key */
  VK_OEM_102,
  /* - 74 - */
  VK_EMPTY, VK_EMPTY,
  /* - 76 - */
  VK_F24,
  /* - 77 - */
  VK_EMPTY, VK_EMPTY,
  /* - 79 - Henkan */
  VK_CONVERT | KBDSPECIAL,
  VK_EMPTY,
  /* - 7b - Muhenkan */
  VK_NONCONVERT | KBDSPECIAL,
  /* - 7c - */
  VK_TAB,
  /* - 7d - Yen key */
  VK_OEM_5,
  /* - 7e - */
  VK_ABNT_C2,
  /* - 7f - */
  VK_OEM_EC,
};

ROSDATA VSC_VK extcode0_to_vk[] = {
  { 0x10, VK_MEDIA_PREV_TRACK | KBDEXT },
  { 0x19, VK_MEDIA_NEXT_TRACK | KBDEXT },
  { 0x1C, VK_RETURN | KBDEXT },
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
  { 0x46, VK_CANCEL | KBDEXT },
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
  { 0, 0 },
};

ROSDATA VSC_VK extcode1_to_vk[] = {
  { 0x1d, VK_PAUSE },
  { 0, 0 },
};

/* ------------------------------------------------------------------ */
/* Modifiers                                                          */

ROSDATA VK_TO_BIT modifier_keys[] = {
  { VK_SHIFT,   KBDSHIFT },
  { VK_CONTROL, KBDCTRL },
  { VK_MENU,    KBDALT },
  { 0,          0 }
};

/*
 * ModNumber[] is indexed by the modifier bit combination
 * (Shift=1, Ctrl=2, Alt=4, Kana=8) and gives the column number
 * in the VK_TO_WCHARS tables. 0x0f (SHFT_INVALID) means "no such state".
 */
ROSDATA MODIFIERS modifier_bits = {
  modifier_keys,
  11,
  {
      0,            // 0000: none
      1,            // 0001: Shift
      4,            // 0010: Ctrl
      6,            // 0011: Shift+Ctrl
      SHFT_INVALID, // 0100: Alt
      SHFT_INVALID, // 0101: Shift+Alt
      SHFT_INVALID, // 0110: Ctrl+Alt
      SHFT_INVALID, // 0111: Shift+Ctrl+Alt
      2,            // 1000: Kana
      3,            // 1001: Kana+Shift
      5,            // 1010: Kana+Ctrl
      7,            // 1011: Kana+Shift+Ctrl
  }
};

/* ------------------------------------------------------------------ */
/* VK -> WCHAR tables                                                  */

ROSDATA VK_TO_WCHARS6 key_to_chars_6mod[] = {
  { VK_BACK,    0,        {0x08, 0x08, 0x08, 0x08, 0x7f, 0x7f} },
  { VK_CANCEL,  0,        {0x03, 0x03, 0x03, 0x03, 0x03, 0x03} },
  { VK_ESCAPE,  0,        {0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b} },
  { VK_OEM_4,   KANALOK,  {'[',  '{',  0xff9f, 0xff62, 0x1b, 0x1b} },
  { VK_OEM_5,   KANALOK,  {'\\', '|',  0xff70, 0xff70, 0x1c, 0x1c} },
  { VK_OEM_102, KANALOK,  {'\\', '_',  0xff9b, 0xff9b, 0x1c, 0x1c} },
  { VK_OEM_6,   KANALOK,  {']',  '}',  0xff91, 0xff63, 0x1d, 0x1d} },
  { VK_RETURN,  0,        {'\r', '\r', '\r', '\r', '\n', '\n'} },
  { VK_SPACE,   0,        {' ',  ' ',  ' ',  ' ',  ' ',  ' '} },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS8 key_to_chars_8mod[] = {
  { '2',          KANALOK, {'2', '\"', 0xff8c, 0xff8c, WCH_NONE, WCH_NONE, 0x00, 0x00} },
  { '6',          KANALOK, {'6', '&',  0xff75, 0xff6b, WCH_NONE, WCH_NONE, 0x1e, 0x1e} },
  { VK_OEM_MINUS, KANALOK, {'-', '=',  0xff8e, 0xff8e, WCH_NONE, WCH_NONE, 0x1f, 0x1f} },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] = {
  /* The numbers */
  { '0',         KANALOK, {'0', WCH_NONE, 0xff9c, 0xff66} },
  { '1',         KANALOK, {'1', '!',      0xff87, 0xff87} },
  /* '2' and '6' are in the 8-column table */
  { '3',         KANALOK, {'3', '#',      0xff71, 0xff67} },
  { '4',         KANALOK, {'4', '$',      0xff73, 0xff69} },
  { '5',         KANALOK, {'5', '%',      0xff74, 0xff6a} },
  { '7',         KANALOK, {'7', '\'',     0xff94, 0xff6c} },
  { '8',         KANALOK, {'8', '(',      0xff95, 0xff6d} },
  { '9',         KANALOK, {'9', ')',      0xff96, 0xff6e} },
  /* The alphabet */
  { 'A',         CAPKANA, {'a', 'A', 0xff81, 0xff81} },
  { 'B',         CAPKANA, {'b', 'B', 0xff7a, 0xff7a} },
  { 'C',         CAPKANA, {'c', 'C', 0xff7f, 0xff7f} },
  { 'D',         CAPKANA, {'d', 'D', 0xff7c, 0xff7c} },
  { 'E',         CAPKANA, {'e', 'E', 0xff72, 0xff68} },
  { 'F',         CAPKANA, {'f', 'F', 0xff8a, 0xff8a} },
  { 'G',         CAPKANA, {'g', 'G', 0xff77, 0xff77} },
  { 'H',         CAPKANA, {'h', 'H', 0xff78, 0xff78} },
  { 'I',         CAPKANA, {'i', 'I', 0xff86, 0xff86} },
  { 'J',         CAPKANA, {'j', 'J', 0xff8f, 0xff8f} },
  { 'K',         CAPKANA, {'k', 'K', 0xff89, 0xff89} },
  { 'L',         CAPKANA, {'l', 'L', 0xff98, 0xff98} },
  { 'M',         CAPKANA, {'m', 'M', 0xff93, 0xff93} },
  { 'N',         CAPKANA, {'n', 'N', 0xff90, 0xff90} },
  { 'O',         CAPKANA, {'o', 'O', 0xff97, 0xff97} },
  { 'P',         CAPKANA, {'p', 'P', 0xff7e, 0xff7e} },
  { 'Q',         CAPKANA, {'q', 'Q', 0xff80, 0xff80} },
  { 'R',         CAPKANA, {'r', 'R', 0xff7d, 0xff7d} },
  { 'S',         CAPKANA, {'s', 'S', 0xff84, 0xff84} },
  { 'T',         CAPKANA, {'t', 'T', 0xff76, 0xff76} },
  { 'U',         CAPKANA, {'u', 'U', 0xff85, 0xff85} },
  { 'V',         CAPKANA, {'v', 'V', 0xff8b, 0xff8b} },
  { 'W',         CAPKANA, {'w', 'W', 0xff83, 0xff83} },
  { 'X',         CAPKANA, {'x', 'X', 0xff7b, 0xff7b} },
  { 'Y',         CAPKANA, {'y', 'Y', 0xff9d, 0xff9d} },
  { 'Z',         CAPKANA, {'z', 'Z', 0xff82, 0xff6f} },
  /* Specials */
  { VK_OEM_1,      KANALOK, {':', '*', 0xff79, 0xff79} },
  { VK_OEM_2,      KANALOK, {'/', '?', 0xff92, 0xff65} },
  { VK_OEM_3,      KANALOK, {'@', '`', 0xff9e, 0xff9e} },
  { VK_OEM_7,      KANALOK, {'^', '~', 0xff8d, 0xff8d} },
  { VK_OEM_8,      0,       {WCH_NONE, WCH_NONE, WCH_NONE, WCH_NONE} },
  { VK_OEM_COMMA,  KANALOK, {',', '<', 0xff88, 0xff64} },
  { VK_OEM_PERIOD, KANALOK, {'.', '>', 0xff99, 0xff61} },
  { VK_OEM_PLUS,   KANALOK, {';', '+', 0xff9a, 0xff9a} },
  /* Keys that do not have shift states */
  { VK_TAB,      0, {'\t', '\t', '\t', '\t'} },
  { VK_ADD,      0, {'+',  '+',  '+',  '+'} },
  { VK_DECIMAL,  0, {'.',  '.',  '.',  '.'} },
  { VK_DIVIDE,   0, {'/',  '/',  '/',  '/'} },
  { VK_MULTIPLY, 0, {'*',  '*',  '*',  '*'} },
  { VK_SUBTRACT, 0, {'-',  '-',  '-',  '-'} },
  { 0, 0 }
};

/* Number pad (Num Lock on): Normal, Shift(none), Kana, Kana+Shift(none) */
ROSDATA VK_TO_WCHARS4 keypad_numbers[] = {
  { VK_NUMPAD0, 0, {'0', WCH_NONE, '0', WCH_NONE} },
  { VK_NUMPAD1, 0, {'1', WCH_NONE, '1', WCH_NONE} },
  { VK_NUMPAD2, 0, {'2', WCH_NONE, '2', WCH_NONE} },
  { VK_NUMPAD3, 0, {'3', WCH_NONE, '3', WCH_NONE} },
  { VK_NUMPAD4, 0, {'4', WCH_NONE, '4', WCH_NONE} },
  { VK_NUMPAD5, 0, {'5', WCH_NONE, '5', WCH_NONE} },
  { VK_NUMPAD6, 0, {'6', WCH_NONE, '6', WCH_NONE} },
  { VK_NUMPAD7, 0, {'7', WCH_NONE, '7', WCH_NONE} },
  { VK_NUMPAD8, 0, {'8', WCH_NONE, '8', WCH_NONE} },
  { VK_NUMPAD9, 0, {'9', WCH_NONE, '9', WCH_NONE} },
  { 0, 0 }
};

#define vk_master(n, x) { (PVK_TO_WCHARS1)x, n, sizeof(x[0]) }

ROSDATA VK_TO_WCHAR_TABLE vk_to_wchar_master_table[] = {
  vk_master(6, key_to_chars_6mod),
  vk_master(8, key_to_chars_8mod),
  vk_master(4, key_to_chars_4mod),
  vk_master(4, keypad_numbers),
  { 0, 0, 0 }
};

/* ------------------------------------------------------------------ */
/* Key names                                                           */

ROSDATA VSC_LPWSTR key_names[] = {
  { 0x01, L"Esc" },
  { 0x0e, L"Backspace" },
  { 0x0f, L"Tab" },
  { 0x1c, L"Enter" },
  { 0x1d, L"Ctrl" },
  { 0x29, L"\u534A\u89D2/\u5168\u89D2" },  /* Hankaku / Zenkaku */
  { 0x2a, L"Shift" },
  { 0x36, L"Right Shift" },
  { 0x37, L"Num *" },
  { 0x38, L"Alt" },
  { 0x39, L"Space" },
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
  { 0x70, L"\u3072\u3089\u304C\u306A" }, /* Hiragana */
  { 0x79, L"\u5909\u63DB" },             /* Henkan */
  { 0x7b, L"\u7121\u5909\u63DB" },       /* Muhenkan */
  { 0x7c, L"F13" },
  { 0, NULL },
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
  { 0x4d, L"Right" },
  { 0x4f, L"End" },
  { 0x50, L"Down" },
  { 0x51, L"Page Down" },
  { 0x52, L"Insert" },
  { 0x53, L"Delete" },
  { 0x5b, L"Left <ReactOS>" },
  { 0x5c, L"Right <ReactOS>" },
  { 0x5d, L"Application" },
  { 0, NULL },
};

/* ------------------------------------------------------------------ */
/* The master table                                                    */

ROSDATA KBDTABLES_FE keyboard_layout_table = {
  {
    /* modifier assignments */
    &modifier_bits,

    /* character from vk tables */
    vk_to_wchar_master_table,

    /* diacritical marks -- Japanese layout has none */
    NULL,

    /* Key names */
    (VSC_LPWSTR *)key_names,
    (VSC_LPWSTR *)extended_key_names,
    NULL, /* Dead key names */

    /* scan code to virtual key maps */
    scancode_to_vk,
    RTL_NUMBER_OF(scancode_to_vk),
    extcode0_to_vk,
    extcode1_to_vk,

    MAKELONG(0, 0), /* Locale flags: no AltGr / shift lock */

    /* Ligatures -- none */
    0,
    0,
    NULL
  },

  /* Keyboard type / subtype: 7 = Japanese keyboard, 2 = 106/109 key (OADG) */
  7,
  2
};

/* ------------------------------------------------------------------ */
/* Far-East (NLS) tables                                               */

#define NLS_NONE             { KBDNLS_NULL, 0 }

/*
 * Each VK_F entry holds 8 handlers for the normal state and 8 for the
 * alternative (toggled) state.  The slot is picked from the modifier
 * state: Shift, Ctrl and Alt (and combinations) select the column.
 */
ROSDATA VK_F key_vk_to_f[] = {
  /* Alphanumeric / Caps Lock (VK_DBE_ALPHANUMERIC) */
  {
    VK_DBE_ALPHANUMERIC, KBDNLS_TYPE_TOGGLE, KBDNLS_INDEX_NORMAL, 2,
    {
      { KBDNLS_ALPHANUM, 0 },
      { KBDNLS_SEND_PARAM_VK, VK_CAPITAL },
      { KBDNLS_ALPHANUM, 0 },
      { KBDNLS_ALPHANUM, 0 },
      { KBDNLS_ALPHANUM, 0 },
      { KBDNLS_ALPHANUM, 0 },
      { KBDNLS_CODEINPUT, 0 },
      { KBDNLS_CODEINPUT, 0 },
    },
    {
      { KBDNLS_SEND_PARAM_VK, VK_CAPITAL },
      { KBDNLS_SEND_PARAM_VK, VK_CAPITAL },
      { KBDNLS_NOEVENT, 0 },
      { KBDNLS_NOEVENT, 0 },
      { KBDNLS_NOEVENT, 0 },
      { KBDNLS_NOEVENT, 0 },
      { KBDNLS_NOEVENT, 0 },
      { KBDNLS_NOEVENT, 0 },
    }
  },
  /* Hiragana / Katakana / Romaji (VK_DBE_HIRAGANA) */
  {
    VK_DBE_HIRAGANA, KBDNLS_TYPE_TOGGLE, KBDNLS_INDEX_NORMAL, 8,
    {
      { KBDNLS_HIRAGANA, 0 },
      { KBDNLS_KATAKANA, 0 },
      { KBDNLS_HIRAGANA, 0 },
      { KBDNLS_SEND_PARAM_VK, VK_KANA },
      { KBDNLS_ROMAN, 0 },
      { KBDNLS_ROMAN, 0 },
      { KBDNLS_ROMAN, 0 },
      { KBDNLS_NOEVENT, 0 },
    },
    {
      { KBDNLS_SEND_PARAM_VK, VK_KANA },
      { KBDNLS_SEND_PARAM_VK, VK_KANA },
      { KBDNLS_SEND_PARAM_VK, VK_KANA },
      { KBDNLS_SEND_PARAM_VK, VK_KANA },
      { KBDNLS_NOEVENT, 0 },
      { KBDNLS_NOEVENT, 0 },
      { KBDNLS_NOEVENT, 0 },
      { KBDNLS_NOEVENT, 0 },
    }
  },
  /* Hankaku/Zenkaku (VK_DBE_SBCSCHAR) */
  {
    VK_DBE_SBCSCHAR, KBDNLS_TYPE_NORMAL, KBDNLS_INDEX_NORMAL, 0,
    {
      { KBDNLS_SBCSDBCS, 0 },
      { KBDNLS_SBCSDBCS, 0 },
      { KBDNLS_SBCSDBCS, 0 },
      { KBDNLS_SBCSDBCS, 0 },
      { KBDNLS_SEND_PARAM_VK, VK_KANJI },
      { KBDNLS_SBCSDBCS, 0 },
      { KBDNLS_SEND_PARAM_VK, VK_DBE_ENTERIMECONFIGMODE },
      { KBDNLS_SEND_PARAM_VK, VK_DBE_ENTERIMECONFIGMODE },
    },
    { NLS_NONE, NLS_NONE, NLS_NONE, NLS_NONE, NLS_NONE, NLS_NONE, NLS_NONE, NLS_NONE }
  },
  /* Muhenkan (VK_NONCONVERT) */
  {
    VK_NONCONVERT, KBDNLS_TYPE_NORMAL, KBDNLS_INDEX_NORMAL, 0,
    {
      { KBDNLS_SEND_BASE_VK, 0 },
      { KBDNLS_SEND_BASE_VK, 0 },
      { KBDNLS_SEND_BASE_VK, 0 },
      { KBDNLS_SEND_BASE_VK, 0 },
      { KBDNLS_SEND_BASE_VK, 0 },
      { KBDNLS_SEND_BASE_VK, 0 },
      { KBDNLS_SEND_PARAM_VK, VK_DBE_ENTERWORDREGISTERMODE },
      { KBDNLS_SEND_PARAM_VK, VK_DBE_ENTERWORDREGISTERMODE },
    },
    { NLS_NONE, NLS_NONE, NLS_NONE, NLS_NONE, NLS_NONE, NLS_NONE, NLS_NONE, NLS_NONE }
  },
};

ROSDATA KBDNLSTABLES KbdNlsTables = {
  0,                        /* OEMIdentifier */
  0,                        /* LayoutInformation */
  RTL_NUMBER_OF(key_vk_to_f),
  key_vk_to_f,
  0,                        /* NumOfMouseVKey */
  NULL
};

PKBDTABLES WINAPI KbdLayerDescriptor(VOID) {
  return &keyboard_layout_table.Base;
}

PKBDNLSTABLES WINAPI KbdNlsLayerDescriptor(VOID) {
  return &KbdNlsTables;
}
