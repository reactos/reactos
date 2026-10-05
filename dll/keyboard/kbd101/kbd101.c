/*
 * PROJECT:     ReactOS Keyboard Layouts
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     (Japanese retro) 101 keyboard layout
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */
/* This keyboard is too old and has limited functionality and also called "English" keyboard
   in Japan. Actually, it was an English keyboard with kana mode. */
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

#define VK_EMPTY  0xff   /* The non-existent VK */

#ifndef KBDKANA
  #define KBDKANA   0x08   /* Kana modifier bit */
#endif

#define KNUMS (KBDNUMPAD | KBDSPECIAL) /* Special + number pad */
#define KMEXT (KBDEXT | KBDMULTIVK)    /* Multi + ext */

/* ------------------------------------------------------------------ */
/* Scancode -> virtual key                                            */

ROSDATA USHORT scancode_to_vk[] = {
  /* 00 */ VK_EMPTY,
  /* 01 */ VK_ESCAPE,
  /* 02 */ '1',
  /* 03 */ '2',
  /* 04 */ '3',
  /* 05 */ '4',
  /* 06 */ '5',
  /* 07 */ '6',
  /* 08 */ '7',
  /* 09 */ '8',
  /* 0a */ '9',
  /* 0b */ '0',
  /* 0c */ VK_OEM_MINUS,
  /* 0d */ VK_OEM_PLUS,
  /* 0e */ VK_BACK,
  /* 0f */ VK_TAB,
  /* 10 */ 'Q',
  /* 11 */ 'W',
  /* 12 */ 'E',
  /* 13 */ 'R',
  /* 14 */ 'T',
  /* 15 */ 'Y',
  /* 16 */ 'U',
  /* 17 */ 'I',
  /* 18 */ 'O',
  /* 19 */ 'P',
  /* 1a */ VK_OEM_4,
  /* 1b */ VK_OEM_6,
  /* 1c */ VK_RETURN,
  /* 1d */ VK_LCONTROL,
  /* 1e */ 'A',
  /* 1f */ 'S',
  /* 20 */ 'D',
  /* 21 */ 'F',
  /* 22 */ 'G',
  /* 23 */ 'H',
  /* 24 */ 'J',
  /* 25 */ 'K',
  /* 26 */ 'L',
  /* 27 */ VK_OEM_1,
  /* 28 */ VK_OEM_7,
  /* 29 */ VK_OEM_3 | KBDSPECIAL,
  /* 2a */ VK_LSHIFT,
  /* 2b */ VK_OEM_5,
  /* 2c */ 'Z',
  /* 2d */ 'X',
  /* 2e */ 'C',
  /* 2f */ 'V',
  /* 30 */ 'B',
  /* 31 */ 'N',
  /* 32 */ 'M',
  /* 33 */ VK_OEM_COMMA,
  /* 34 */ VK_OEM_PERIOD,
  /* 35 */ VK_OEM_2,
  /* 36 */ VK_RSHIFT | KBDEXT,
  /* 37 */ VK_MULTIPLY | KBDMULTIVK,
  /* 38 */ VK_LMENU,
  /* 39 */ VK_SPACE,
  /* 3a */ VK_CAPITAL | KBDSPECIAL,
  /* 3b */ VK_F1,
  /* 3c */ VK_F2,
  /* 3d */ VK_F3,
  /* 3e */ VK_F4,
  /* 3f */ VK_F5,
  /* 40 */ VK_F6,
  /* 41 */ VK_F7,
  /* 42 */ VK_F8,
  /* 43 */ VK_F9,
  /* 44 */ VK_F10,
  /* 45 */ VK_NUMLOCK | KBDEXT | KBDMULTIVK,
  /* 46 */ VK_SCROLL | KBDMULTIVK,
  /* 47 */ VK_HOME | KBDSPECIAL | KBDNUMPAD,
  /* 48 */ VK_UP | KBDSPECIAL | KBDNUMPAD,
  /* 49 */ VK_PRIOR | KBDSPECIAL | KBDNUMPAD,
  /* 4a */ VK_SUBTRACT,
  /* 4b */ VK_LEFT | KBDSPECIAL | KBDNUMPAD,
  /* 4c */ VK_CLEAR | KBDSPECIAL | KBDNUMPAD,
  /* 4d */ VK_RIGHT | KBDSPECIAL | KBDNUMPAD,
  /* 4e */ VK_ADD,
  /* 4f */ VK_END | KBDSPECIAL | KBDNUMPAD,
  /* 50 */ VK_DOWN | KBDSPECIAL | KBDNUMPAD,
  /* 51 */ VK_NEXT | KBDSPECIAL | KBDNUMPAD,
  /* 52 */ VK_INSERT | KBDSPECIAL | KBDNUMPAD,
  /* 53 */ VK_DELETE | KBDSPECIAL | KBDNUMPAD,
  /* 54 */ VK_SNAPSHOT,
  /* 55 */ VK_EMPTY,
  /* 56 */ VK_OEM_102,
  /* 57 */ VK_F11,
  /* 58 */ VK_F12,
  /* 59 */ VK_CLEAR,
  /* 5a */ VK_OEM_WSCTRL,
  /* 5b */ VK_OEM_FINISH,
  /* 5c */ VK_OEM_JUMP,
  /* 5d */ VK_EREOF,
  /* 5e */ VK_OEM_BACKTAB,
  /* 5f */ VK_OEM_AUTO,
  /* 60 */ VK_EMPTY,
  /* 61 */ VK_EMPTY,
  /* 62 */ VK_ZOOM,
  /* 63 */ VK_HELP,
  /* 64 */ VK_F13,
  /* 65 */ VK_F14,
  /* 66 */ VK_F15,
  /* 67 */ VK_F16,
  /* 68 */ VK_F17,
  /* 69 */ VK_F18,
  /* 6a */ VK_F19,
  /* 6b */ VK_F20,
  /* 6c */ VK_F21,
  /* 6d */ VK_F22,
  /* 6e */ VK_F23,
  /* 6f */ VK_OEM_PA3,
  /* 70 */ VK_EMPTY,
  /* 71 */ VK_OEM_RESET,
  /* 72 */ VK_EMPTY,
  /* 73 */ VK_ABNT_C1,
  /* 74 */ VK_EMPTY,
  /* 75 */ VK_EMPTY,
  /* 76 */ VK_F24,
  /* 77 */ VK_EMPTY,
  /* 78 */ VK_EMPTY,
  /* 79 */ VK_EMPTY,
  /* 7a */ VK_EMPTY,
  /* 7b */ VK_OEM_PA1,
  /* 7c */ VK_TAB,
  /* 7d */ VK_EMPTY,
  /* 7e */ VK_ABNT_C2,
  /* 7f */ VK_OEM_PA2,
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
  { 0x1D, VK_PAUSE },
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
 * Modifier bits -> column number of the VK_TO_WCHARS tables.
 * wMaxModBits = KBDSHIFT|KBDCTRL|KBDKANA = 11.
 *
 *   column 0: base          column 4: Ctrl
 *   column 1: Shift         column 5: Kana+Ctrl
 *   column 2: Kana          column 6: Ctrl+Shift
 *   column 3: Kana+Shift    column 7: Kana+Ctrl+Shift
 */
ROSDATA MODIFIERS modifier_bits = {
  modifier_keys,
  11,
  {
      0,            /* 0x0  none            */
      1,            /* 0x1  Shift           */
      4,            /* 0x2  Ctrl            */
      6,            /* 0x3  Shift+Ctrl      */
      SHFT_INVALID, /* 0x4  Alt             */
      SHFT_INVALID, /* 0x5  Alt+Shift       */
      SHFT_INVALID, /* 0x6  Alt+Ctrl        */
      SHFT_INVALID, /* 0x7  Alt+Shift+Ctrl  */
      2,            /* 0x8  Kana            */
      3,            /* 0x9  Kana+Shift      */
      5,            /* 0xa  Kana+Ctrl       */
      7,            /* 0xb  Kana+Shift+Ctrl */
  }
};

/* ------------------------------------------------------------------ */
/* Virtual key -> character                                           */

/* Normal, Shift, Kana, Kana+Shift, Ctrl, Kana+Ctrl */
ROSDATA VK_TO_WCHARS6 key_to_chars_6mod[] = {
  { VK_BACK,    0,       {0x0008, 0x0008, 0x0008, 0x0008, 0x007f, 0x007f} },
  { VK_CANCEL,  0,       {0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003} },
  { VK_ESCAPE,  0,       {0x001b, 0x001b, 0x001b, 0x001b, 0x001b, 0x001b} },
  { VK_OEM_4,   KANALOK, {'[', '{', 0xff9e, 0xff62, 0x001b, 0x001b} },  /* kana: ﾞ ｢ */
  { VK_OEM_5,   KANALOK, {'\\', '|', 0xff91, 0xff91, 0x001c, 0x001c} }, /* kana: ﾑ ﾑ */
  { VK_OEM_102, KANALOK, {'\\', '|', 0xff91, 0xff91, 0x001c, 0x001c} }, /* kana: ﾑ ﾑ */
  { VK_OEM_6,   KANALOK, {']', '}', 0xff9f, 0xff63, 0x001d, 0x001d} },  /* kana: ﾟ ｣ */
  { VK_RETURN,  0,       {0x000d, 0x000d, 0x000d, 0x000d, 0x000a, 0x000a} },
  { VK_SPACE,   0,       {' ', ' ', ' ', ' ', ' ', ' '} },
  { 0, 0 }
};

/* Normal, Shift, Kana, Kana+Shift, Ctrl, Kana+Ctrl, Ctrl+Shift, Kana+Ctrl+Shift */
ROSDATA VK_TO_WCHARS8 key_to_chars_8mod[] = {
  { '2',           KANALOK, {'2', '@', 0xff8c, 0xff8c, WCH_NONE, WCH_NONE, 0x0000, 0x0000} }, /* kana: ﾌ ﾌ */
  { '6',           KANALOK, {'6', '^', 0xff75, 0xff6b, WCH_NONE, WCH_NONE, 0x001e, 0x001e} }, /* kana: ｵ ｫ */
  { VK_OEM_MINUS,  KANALOK, {'-', '_', 0xff8e, 0xff70, WCH_NONE, WCH_NONE, 0x001f, 0x001f} }, /* kana: ﾎ ｰ */
  { 0, 0 }
};

/* Normal, Shift, Kana, Kana+Shift */
ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] = {
  { '0',           KANALOK,        {'0', ')', 0xff9c, 0xff66} },  /* kana: ﾜ ｦ */
  { '1',           KANALOK,        {'1', '!', 0xff87, 0xff87} },  /* kana: ﾇ ﾇ */
  { '3',           KANALOK,        {'3', '#', 0xff71, 0xff67} },  /* kana: ｱ ｧ */
  { '4',           KANALOK,        {'4', '$', 0xff73, 0xff69} },  /* kana: ｳ ｩ */
  { '5',           KANALOK,        {'5', '%', 0xff74, 0xff6a} },  /* kana: ｴ ｪ */
  { '7',           KANALOK,        {'7', '&', 0xff94, 0xff6c} },  /* kana: ﾔ ｬ */
  { '8',           KANALOK,        {'8', '*', 0xff95, 0xff6d} },  /* kana: ﾕ ｭ */
  { '9',           KANALOK,        {'9', '(', 0xff96, 0xff6e} },  /* kana: ﾖ ｮ */
  { 'A',           CAPLOK|KANALOK, {'a', 'A', 0xff81, 0xff81} },  /* kana: ﾁ ﾁ */
  { 'B',           CAPLOK|KANALOK, {'b', 'B', 0xff7a, 0xff7a} },  /* kana: ｺ ｺ */
  { 'C',           CAPLOK|KANALOK, {'c', 'C', 0xff7f, 0xff7f} },  /* kana: ｿ ｿ */
  { 'D',           CAPLOK|KANALOK, {'d', 'D', 0xff7c, 0xff7c} },  /* kana: ｼ ｼ */
  { 'E',           CAPLOK|KANALOK, {'e', 'E', 0xff72, 0xff68} },  /* kana: ｲ ｨ */
  { 'F',           CAPLOK|KANALOK, {'f', 'F', 0xff8a, 0xff8a} },  /* kana: ﾊ ﾊ */
  { 'G',           CAPLOK|KANALOK, {'g', 'G', 0xff77, 0xff77} },  /* kana: ｷ ｷ */
  { 'H',           CAPLOK|KANALOK, {'h', 'H', 0xff78, 0xff78} },  /* kana: ｸ ｸ */
  { 'I',           CAPLOK|KANALOK, {'i', 'I', 0xff86, 0xff86} },  /* kana: ﾆ ﾆ */
  { 'J',           CAPLOK|KANALOK, {'j', 'J', 0xff8f, 0xff8f} },  /* kana: ﾏ ﾏ */
  { 'K',           CAPLOK|KANALOK, {'k', 'K', 0xff89, 0xff89} },  /* kana: ﾉ ﾉ */
  { 'L',           CAPLOK|KANALOK, {'l', 'L', 0xff98, 0xff98} },  /* kana: ﾘ ﾘ */
  { 'M',           CAPLOK|KANALOK, {'m', 'M', 0xff93, 0xff93} },  /* kana: ﾓ ﾓ */
  { 'N',           CAPLOK|KANALOK, {'n', 'N', 0xff90, 0xff90} },  /* kana: ﾐ ﾐ */
  { 'O',           CAPLOK|KANALOK, {'o', 'O', 0xff97, 0xff97} },  /* kana: ﾗ ﾗ */
  { 'P',           CAPLOK|KANALOK, {'p', 'P', 0xff7e, 0xff7e} },  /* kana: ｾ ｾ */
  { 'Q',           CAPLOK|KANALOK, {'q', 'Q', 0xff80, 0xff80} },  /* kana: ﾀ ﾀ */
  { 'R',           CAPLOK|KANALOK, {'r', 'R', 0xff7d, 0xff7d} },  /* kana: ｽ ｽ */
  { 'S',           CAPLOK|KANALOK, {'s', 'S', 0xff84, 0xff84} },  /* kana: ﾄ ﾄ */
  { 'T',           CAPLOK|KANALOK, {'t', 'T', 0xff76, 0xff76} },  /* kana: ｶ ｶ */
  { 'U',           CAPLOK|KANALOK, {'u', 'U', 0xff85, 0xff85} },  /* kana: ﾅ ﾅ */
  { 'V',           CAPLOK|KANALOK, {'v', 'V', 0xff8b, 0xff8b} },  /* kana: ﾋ ﾋ */
  { 'W',           CAPLOK|KANALOK, {'w', 'W', 0xff83, 0xff83} },  /* kana: ﾃ ﾃ */
  { 'X',           CAPLOK|KANALOK, {'x', 'X', 0xff7b, 0xff7b} },  /* kana: ｻ ｻ */
  { 'Y',           CAPLOK|KANALOK, {'y', 'Y', 0xff9d, 0xff9d} },  /* kana: ﾝ ﾝ */
  { 'Z',           CAPLOK|KANALOK, {'z', 'Z', 0xff82, 0xff6f} },  /* kana: ﾂ ｯ */
  { VK_OEM_1,      KANALOK,        {';', ':', 0xff9a, 0xff9a} },  /* kana: ﾚ ﾚ */
  { VK_OEM_2,      KANALOK,        {'/', '?', 0xff92, 0xff65} },  /* kana: ﾒ ･ */
  { VK_OEM_3,      KANALOK,        {'`', '~', 0xff9b, 0xff9b} },  /* kana: ﾛ ﾛ */
  { VK_OEM_7,      KANALOK,        {'\'', '"', 0xff79, 0xff79} },  /* kana: ｹ ｹ */
  { VK_OEM_8,      0,              {WCH_NONE, WCH_NONE, WCH_NONE, WCH_NONE} },
  { VK_OEM_COMMA,  KANALOK,        {',', '<', 0xff88, 0xff64} },  /* kana: ﾈ ､ */
  { VK_OEM_PERIOD, KANALOK,        {'.', '>', 0xff99, 0xff61} },  /* kana: ﾙ ｡ */
  { VK_OEM_PLUS,   KANALOK,        {'=', '+', 0xff8d, 0xff8d} },  /* kana: ﾍ ﾍ */
  { VK_TAB,        0,              {0x0009, 0x0009, 0x0009, 0x0009} },
  { VK_ADD,        0,              {'+', '+', '+', '+'} },
  { VK_DECIMAL,    0,              {'.', '.', '.', '.'} },
  { VK_DIVIDE,     0,              {'/', '/', '/', '/'} },
  { VK_MULTIPLY,   0,              {'*', '*', '*', '*'} },
  { VK_SUBTRACT,   0,              {'-', '-', '-', '-'} },
  { 0, 0 }
};

/* Number pad: Normal, (none), Kana, (none) */
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

ROSDATA VK_TO_WCHAR_TABLE vk_to_wchar_master_table[] = {
  { (PVK_TO_WCHARS1)key_to_chars_6mod, 6, sizeof(key_to_chars_6mod[0]) },
  { (PVK_TO_WCHARS1)key_to_chars_8mod, 8, sizeof(key_to_chars_8mod[0]) },
  { (PVK_TO_WCHARS1)key_to_chars_4mod, 4, sizeof(key_to_chars_4mod[0]) },
  { (PVK_TO_WCHARS1)keypad_numbers,    4, sizeof(keypad_numbers[0]) },
  { 0, 0, 0 }
};

/* ------------------------------------------------------------------ */
/* Key names                                                          */

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
/* The master table                                                   */

ROSDATA KBDTABLES_FE keyboard_layout_table = {
  {
    /* modifier assignments */
    &modifier_bits,

    /* character from vk tables */
    vk_to_wchar_master_table,

    /* diacritical marks -- none */
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

    0, /* fLocaleFlags: no AltGr, no ShiftLock */

    /* Ligatures -- none */
    0,
    0,
    NULL
  },

  4, /* dwType    */
  0  /* dwSubType */
};

PKBDTABLES WINAPI KbdLayerDescriptor(VOID)
{
  return (PKBDTABLES)&keyboard_layout_table;
}

/* ------------------------------------------------------------------ */
/* NLS (Japanese input) tables                                        */

ROSDATA VK_F vk_to_function_table[] = {
  /* CapsLock: toggle type; NLSFEProcSwitch = 0x08 */
  {
    VK_CAPITAL, KBDNLS_TYPE_TOGGLE, KBDNLS_INDEX_NORMAL, 0x08,
    {
      { KBDNLS_SEND_BASE_VK,  0 },
      { KBDNLS_ALPHANUM,      0 },
      { KBDNLS_HIRAGANA,      0 },
      { KBDNLS_SEND_PARAM_VK, VK_KANA },
      { KBDNLS_KATAKANA,      0 },
      { KBDNLS_SEND_BASE_VK,  0 },
      { KBDNLS_SEND_BASE_VK,  0 },
      { KBDNLS_SEND_BASE_VK,  0 },
    },
    {
      { KBDNLS_SEND_PARAM_VK, VK_KANA },
      { KBDNLS_SEND_PARAM_VK, VK_KANA },
      { KBDNLS_SEND_PARAM_VK, VK_KANA },
      { KBDNLS_SEND_PARAM_VK, VK_KANA },
      { KBDNLS_SEND_BASE_VK,  0 },
      { KBDNLS_SEND_BASE_VK,  0 },
      { KBDNLS_SEND_BASE_VK,  0 },
      { KBDNLS_SEND_BASE_VK,  0 },
    }
  },

  /* Grave/Tilde key: normal type, no alternate table */
  {
    VK_OEM_3, KBDNLS_TYPE_NORMAL, KBDNLS_INDEX_NORMAL, 0x00,
    {
      { KBDNLS_SEND_BASE_VK,  0 },
      { KBDNLS_SEND_BASE_VK,  0 },
      { KBDNLS_SBCSDBCS,      0 },
      { KBDNLS_SEND_BASE_VK,  0 },
      { KBDNLS_SEND_PARAM_VK, VK_KANJI },
      { KBDNLS_SEND_BASE_VK,  0 },
      { KBDNLS_SEND_BASE_VK,  0 },
      { KBDNLS_SEND_BASE_VK,  0 },
    },
    {
      { KBDNLS_NULL, 0 }, { KBDNLS_NULL, 0 }, { KBDNLS_NULL, 0 }, { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 }, { KBDNLS_NULL, 0 }, { KBDNLS_NULL, 0 }, { KBDNLS_NULL, 0 },
    }
  },
};

ROSDATA KBDNLSTABLES nls_layout_table = {
  0,                                   /* OEMIdentifier      */
  0,                                   /* LayoutInformation  */
  RTL_NUMBER_OF(vk_to_function_table), /* NumOfVkToF         */
  vk_to_function_table,                /* pVkToF             */
  0,                                   /* NumOfMouseVKey     */
  NULL                                 /* pusMouseVKey       */
};

PKBDNLSTABLES WINAPI KbdNlsLayerDescriptor(VOID)
{
  return &nls_layout_table;
}
