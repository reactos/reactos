/*
 * PROJECT:     ReactOS Keyboard Layouts
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     NEC PC-9801 Japanese keyboard layout
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

#define VK_EMPTY  0xff   /* The non-existent VK */

#define KNUMS  (KBDNUMPAD | KBDSPECIAL) /* Special + number pad */
#define KMEXT  (KBDEXT | KBDMULTIVK)    /* Multi + ext */

#ifndef VK_OEM_NEC_EQUAL
  #define VK_OEM_NEC_EQUAL  0x92  /* '=' key on the NEC numpad */
#endif

/* Kana modifier (selected by the Kana lock) */
#ifndef KBDKANA
  #define KBDKANA  0x08
#endif

/* ------------------------------------------------------------------ */
/* Scan code -> virtual key                                           */
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
  /* 0d */ VK_OEM_7,
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
  /* 1a */ VK_OEM_3,
  /* 1b */ VK_OEM_4,
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
  /* 27 */ VK_OEM_PLUS,
  /* 28 */ VK_OEM_1,
  /* 29 */ VK_DBE_SBCSCHAR | KBDSPECIAL,
  /* 2a */ VK_LSHIFT,
  /* 2b */ VK_OEM_6,
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
  /* 37 */ VK_MULTIPLY,
  /* 38 */ VK_LMENU,
  /* 39 */ VK_SPACE,
  /* 3a */ VK_CAPITAL,
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
  /* 45 */ VK_NUMLOCK | KMEXT,
  /* 46 */ VK_SCROLL,
  /* 47 */ VK_HOME | KNUMS,
  /* 48 */ VK_UP | KNUMS,
  /* 49 */ VK_PRIOR | KNUMS,
  /* 4a */ VK_SUBTRACT,
  /* 4b */ VK_LEFT | KNUMS,
  /* 4c */ VK_CLEAR | KNUMS,
  /* 4d */ VK_RIGHT | KNUMS,
  /* 4e */ VK_ADD,
  /* 4f */ VK_END | KNUMS,
  /* 50 */ VK_DOWN | KNUMS,
  /* 51 */ VK_NEXT | KNUMS,
  /* 52 */ VK_INSERT | KNUMS,
  /* 53 */ VK_DELETE | KNUMS,
  /* 54 */ VK_SNAPSHOT,
  /* 55 */ VK_EMPTY,
  /* 56 */ VK_EMPTY,
  /* 57 */ VK_F11,
  /* 58 */ VK_F12,
  /* 59 */ VK_OEM_NEC_EQUAL | KBDSPECIAL,
  /* 5a */ VK_DBE_DETERMINESTRING,
  /* 5b */ VK_EMPTY,
  /* 5c */ VK_SEPARATOR,
  /* 5d */ VK_F13,
  /* 5e */ VK_F14,
  /* 5f */ VK_F15,
  /* 60 */ VK_EMPTY,
  /* 61 */ VK_EMPTY,
  /* 62 */ VK_EMPTY,
  /* 63 */ VK_EMPTY,
  /* 64 */ VK_EMPTY,
  /* 65 */ VK_EMPTY,
  /* 66 */ VK_EMPTY,
  /* 67 */ VK_EMPTY,
  /* 68 */ VK_EMPTY,
  /* 69 */ VK_EMPTY,
  /* 6a */ VK_EMPTY,
  /* 6b */ VK_EMPTY,
  /* 6c */ VK_EMPTY,
  /* 6d */ VK_EMPTY,
  /* 6e */ VK_EMPTY,
  /* 6f */ VK_EMPTY,
  /* 70 */ VK_KANA,
  /* 71 */ VK_EMPTY,
  /* 72 */ VK_EMPTY,
  /* 73 */ VK_OEM_8,
  /* 74 */ VK_EMPTY,
  /* 75 */ VK_EMPTY,
  /* 76 */ VK_EMPTY,
  /* 77 */ VK_EMPTY,
  /* 78 */ VK_EMPTY,
  /* 79 */ VK_CONVERT | KBDSPECIAL,
  /* 7a */ VK_EMPTY,
  /* 7b */ VK_NONCONVERT | KBDSPECIAL,
  /* 7c */ VK_TAB,
  /* 7d */ VK_OEM_5,
  /* 7e */ VK_ABNT_C2,
  /* 7f */ 0xec,
};

ROSDATA VSC_VK extcode0_to_vk[] = {
  { 0x1C, VK_RETURN | KBDEXT },
  { 0x1D, VK_RCONTROL | KBDEXT },
  { 0x35, VK_DIVIDE | KBDEXT },
  { 0x37, VK_SNAPSHOT | KBDEXT },
  { 0x38, VK_EMPTY | KBDEXT },
  { 0x46, VK_CANCEL | KBDEXT },
  { 0x47, VK_HOME | KBDEXT | KBDSPECIAL },
  { 0x48, VK_UP | KBDEXT },
  { 0x49, VK_PRIOR | KBDEXT },
  { 0x4B, VK_LEFT | KBDEXT },
  { 0x4D, VK_RIGHT | KBDEXT },
  { 0x4F, VK_END | KBDEXT | KBDSPECIAL },
  { 0x50, VK_DOWN | KBDEXT },
  { 0x51, VK_NEXT | KBDEXT },
  { 0x52, VK_INSERT | KBDEXT },
  { 0x53, VK_DELETE | KBDEXT },
  { 0x5B, VK_LWIN | KBDEXT },
  { 0x5C, VK_RWIN | KBDEXT },
  { 0x5D, VK_APPS | KBDEXT },
  { 0, 0 },
};

ROSDATA VSC_VK extcode1_to_vk[] = {
  { 0x1D, VK_PAUSE },
  { 0, 0 },
};

ROSDATA VK_TO_BIT modifier_keys[] = {
  { VK_SHIFT,   KBDSHIFT },
  { VK_CONTROL, KBDCTRL },
  { VK_MENU,    KBDALT },
  { 0,          0 }
};

/*
 * ModNumber[] is indexed by (Kana<<3 | Alt<<2 | Ctrl<<1 | Shift)
 * and yields the column in the VK_TO_WCHARS tables:
 *   0 Normal       1 Shift        2 Kana        3 Kana+Shift
 *   4 Ctrl         5 Kana+Ctrl    6 Shift+Ctrl  7 Kana+Shift+Ctrl
 */
ROSDATA MODIFIERS modifier_bits = {
  modifier_keys,
  11,
  {
    0,            /* -                  */
    1,            /* Shift              */
    4,            /* Ctrl               */
    6,            /* Shift+Ctrl         */
    SHFT_INVALID, /* Alt                */
    SHFT_INVALID, /* Shift+Alt          */
    SHFT_INVALID, /* Ctrl+Alt           */
    SHFT_INVALID, /* Shift+Ctrl+Alt     */
    2,            /* Kana               */
    3,            /* Kana+Shift         */
    5,            /* Kana+Ctrl          */
    7             /* Kana+Shift+Ctrl    */
  }
};

/* Normal, Shift, Kana, Kana+Shift, Ctrl, Kana+Ctrl */
ROSDATA VK_TO_WCHARS6 key_to_chars_6mod[] = {
  { VK_BACK       , 0              , { 0x0008, 0x0008, 0x0008, 0x0008, 0x007f, 0x007f } },
  { VK_CANCEL     , 0              , { 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003 } },
  { VK_ESCAPE     , 0              , { 0x001b, 0x001b, 0x001b, 0x001b, 0x001b, 0x001b } },
  { VK_OEM_3      , KANALOK        , { '@', '~', 0xff9e, 0xff9e, 0x0000, 0x0000 } },
  { VK_OEM_4      , KANALOK        , { '[', '{', 0xff9f, 0xff62, 0x001b, 0x001b } },
  { VK_OEM_5      , KANALOK        , { '\\', '|', 0xff70, 0xff70, 0x001c, 0x001c } },
  { VK_OEM_6      , KANALOK        , { ']', '}', 0xff91, 0xff63, 0x001d, 0x001d } },
  { VK_OEM_7      , KANALOK        , { '^', '`', 0xff8d, 0xff8d, 0x001e, 0x001e } },
  { VK_OEM_8      , KANALOK        , { WCH_NONE, '_', 0xff9b, 0xff9b, 0x001f, 0x001f } },
  { VK_RETURN     , 0              , { 0x000d, 0x000d, 0x000d, 0x000d, 0x000a, 0x000a } },
  { VK_SPACE      , 0              , { ' ', ' ', ' ', ' ', ' ', ' ' } },
  { 0, 0 }
};

/* Normal, Shift, Kana, Kana+Shift, Ctrl, Kana+Ctrl, Shift+Ctrl, Kana+Shift+Ctrl */
ROSDATA VK_TO_WCHARS8 key_to_chars_8mod[] = {
  { '2'           , KANALOK        , { '2', '"', 0xff8c, 0xff8c, WCH_NONE, WCH_NONE, 0x0000, 0x0000 } },
  { '6'           , KANALOK        , { '6', '&', 0xff75, 0xff6b, WCH_NONE, WCH_NONE, 0x001e, 0x001e } },
  { VK_OEM_MINUS  , KANALOK        , { '-', '=', 0xff8e, 0xff8e, WCH_NONE, WCH_NONE, 0x001f, 0x001f } },
  { 0, 0 }
};

/* Normal, Shift, Kana, Kana+Shift */
ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] = {
  { '0'           , KANALOK        , { '0', '0', 0xff9c, 0xff66 } },
  { '1'           , KANALOK        , { '1', '!', 0xff87, 0xff87 } },
  { '3'           , KANALOK        , { '3', '#', 0xff71, 0xff67 } },
  { '4'           , KANALOK        , { '4', '$', 0xff73, 0xff69 } },
  { '5'           , KANALOK        , { '5', '%', 0xff74, 0xff6a } },
  { '7'           , KANALOK        , { '7', '\'', 0xff94, 0xff6c } },
  { '8'           , KANALOK        , { '8', '(', 0xff95, 0xff6d } },
  { '9'           , KANALOK        , { '9', ')', 0xff96, 0xff6e } },
  { 'A'           , CAPLOK|KANALOK , { 'a', 'A', 0xff81, 0xff81 } },
  { 'B'           , CAPLOK|KANALOK , { 'b', 'B', 0xff7a, 0xff7a } },
  { 'C'           , CAPLOK|KANALOK , { 'c', 'C', 0xff7f, 0xff7f } },
  { 'D'           , CAPLOK|KANALOK , { 'd', 'D', 0xff7c, 0xff7c } },
  { 'E'           , CAPLOK|KANALOK , { 'e', 'E', 0xff72, 0xff68 } },
  { 'F'           , CAPLOK|KANALOK , { 'f', 'F', 0xff8a, 0xff8a } },
  { 'G'           , CAPLOK|KANALOK , { 'g', 'G', 0xff77, 0xff77 } },
  { 'H'           , CAPLOK|KANALOK , { 'h', 'H', 0xff78, 0xff78 } },
  { 'I'           , CAPLOK|KANALOK , { 'i', 'I', 0xff86, 0xff86 } },
  { 'J'           , CAPLOK|KANALOK , { 'j', 'J', 0xff8f, 0xff8f } },
  { 'K'           , CAPLOK|KANALOK , { 'k', 'K', 0xff89, 0xff89 } },
  { 'L'           , CAPLOK|KANALOK , { 'l', 'L', 0xff98, 0xff98 } },
  { 'M'           , CAPLOK|KANALOK , { 'm', 'M', 0xff93, 0xff93 } },
  { 'N'           , CAPLOK|KANALOK , { 'n', 'N', 0xff90, 0xff90 } },
  { 'O'           , CAPLOK|KANALOK , { 'o', 'O', 0xff97, 0xff97 } },
  { 'P'           , CAPLOK|KANALOK , { 'p', 'P', 0xff7e, 0xff7e } },
  { 'Q'           , CAPLOK|KANALOK , { 'q', 'Q', 0xff80, 0xff80 } },
  { 'R'           , CAPLOK|KANALOK , { 'r', 'R', 0xff7d, 0xff7d } },
  { 'S'           , CAPLOK|KANALOK , { 's', 'S', 0xff84, 0xff84 } },
  { 'T'           , CAPLOK|KANALOK , { 't', 'T', 0xff76, 0xff76 } },
  { 'U'           , CAPLOK|KANALOK , { 'u', 'U', 0xff85, 0xff85 } },
  { 'V'           , CAPLOK|KANALOK , { 'v', 'V', 0xff8b, 0xff8b } },
  { 'W'           , CAPLOK|KANALOK , { 'w', 'W', 0xff83, 0xff83 } },
  { 'X'           , CAPLOK|KANALOK , { 'x', 'X', 0xff7b, 0xff7b } },
  { 'Y'           , CAPLOK|KANALOK , { 'y', 'Y', 0xff9d, 0xff9d } },
  { 'Z'           , CAPLOK|KANALOK , { 'z', 'Z', 0xff82, 0xff6f } },
  { VK_OEM_1      , KANALOK        , { ':', '*', 0xff79, 0xff79 } },
  { VK_OEM_2      , KANALOK        , { '/', '?', 0xff92, 0xff65 } },
  { VK_OEM_COMMA  , KANALOK        , { ',', '<', 0xff88, 0xff64 } },
  { VK_OEM_PERIOD , KANALOK        , { '.', '>', 0xff99, 0xff61 } },
  { VK_OEM_PLUS   , KANALOK        , { ';', '+', 0xff9a, 0xff9a } },
  { VK_TAB        , 0              , { 0x0009, 0x0009, 0x0009, 0x0009 } },
  { VK_ADD        , 0              , { '+', '+', '+', '+' } },
  { VK_DECIMAL    , 0              , { '.', '.', '.', '.' } },
  { VK_DIVIDE     , 0              , { '/', '/', '/', '/' } },
  { VK_MULTIPLY   , 0              , { '*', '*', '*', '*' } },
  { VK_SUBTRACT   , 0              , { '-', '-', '-', '-' } },
  { VK_SEPARATOR  , 0              , { ',', ',', ',', ',' } },
  { VK_OEM_NEC_EQUAL, 0              , { '=', '=', '=', '=' } },
  { 0, 0 }
};

/* Numpad (Normal, -, Ctrl, -) */
ROSDATA VK_TO_WCHARS4 keypad_numbers[] = {
  { VK_NUMPAD0    , 0              , { '0', WCH_NONE, '0', WCH_NONE } },
  { VK_NUMPAD1    , 0              , { '1', WCH_NONE, '1', WCH_NONE } },
  { VK_NUMPAD2    , 0              , { '2', WCH_NONE, '2', WCH_NONE } },
  { VK_NUMPAD3    , 0              , { '3', WCH_NONE, '3', WCH_NONE } },
  { VK_NUMPAD4    , 0              , { '4', WCH_NONE, '4', WCH_NONE } },
  { VK_NUMPAD5    , 0              , { '5', WCH_NONE, '5', WCH_NONE } },
  { VK_NUMPAD6    , 0              , { '6', WCH_NONE, '6', WCH_NONE } },
  { VK_NUMPAD7    , 0              , { '7', WCH_NONE, '7', WCH_NONE } },
  { VK_NUMPAD8    , 0              , { '8', WCH_NONE, '8', WCH_NONE } },
  { VK_NUMPAD9    , 0              , { '9', WCH_NONE, '9', WCH_NONE } },
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

ROSDATA VSC_LPWSTR key_names[] = {
  { 0x01, L"Esc" },
  { 0x0e, L"Backspace" },
  { 0x0f, L"Tab" },
  { 0x1c, L"Enter" },
  { 0x1d, L"Ctrl" },
  { 0x29, L"\x534a\x89d2/\x5168\x89d2" },  /* Hankaku / Zenkaku */
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
  { 0x59, L"Num =" },
  { 0x5c, L"Num ," },
  { 0x5d, L"F13" },
  { 0x5e, L"F14" },
  { 0x5f, L"F15" },
  { 0x70, L"\xff76\xff85" },  /* Kana */
  { 0x79, L"XFER" },
  { 0x7b, L"NFER" },
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
/* NLS tables                                                         */
ROSDATA VK_F vk_to_f[] = {
  { VK_DBE_SBCSCHAR, KBDNLS_TYPE_NORMAL, KBDNLS_INDEX_NORMAL, 0,
    {
      { KBDNLS_SBCSDBCS, 0 },
      { KBDNLS_SBCSDBCS, 0 },
      { KBDNLS_SBCSDBCS, 0 },
      { KBDNLS_SBCSDBCS, 0 },
      { KBDNLS_SBCSDBCS, 0 },
      { KBDNLS_SBCSDBCS, 0 },
      { KBDNLS_SEND_PARAM_VK, VK_DBE_ENTERIMECONFIGMODE },
      { KBDNLS_SEND_PARAM_VK, VK_DBE_ENTERIMECONFIGMODE },
    },
    { { 0 } }  /* alternate table: unused */
  },
  { VK_NONCONVERT, KBDNLS_TYPE_NORMAL, KBDNLS_INDEX_NORMAL, 0,
    {
      { KBDNLS_SEND_BASE_VK, 0 },
      { KBDNLS_SEND_BASE_VK, 0 },
      { KBDNLS_SEND_PARAM_VK, VK_DBE_ALPHANUMERIC },
      { KBDNLS_SBCSDBCS, 0 },
      { KBDNLS_SEND_PARAM_VK, VK_DBE_KATAKANA },
      { KBDNLS_SEND_PARAM_VK, VK_DBE_ENTERWORDREGISTERMODE },
      { KBDNLS_SEND_PARAM_VK, VK_DBE_NOCODEINPUT },
      { KBDNLS_SEND_PARAM_VK, VK_DBE_FLUSHSTRING },
    },
    { { 0 } }  /* alternate table: unused */
  },
  { VK_CONVERT, KBDNLS_TYPE_NORMAL, KBDNLS_INDEX_NORMAL, 0,
    {
      { KBDNLS_SEND_BASE_VK, 0 },
      { KBDNLS_SEND_BASE_VK, 0 },
      { KBDNLS_SEND_PARAM_VK, 0x19 },
      { KBDNLS_SEND_BASE_VK, 0 },
      { KBDNLS_ROMAN, 0 },
      { KBDNLS_SEND_PARAM_VK, VK_DBE_ENTERIMECONFIGMODE },
      { KBDNLS_SEND_PARAM_VK, VK_DBE_CODEINPUT },
      { KBDNLS_SEND_PARAM_VK, VK_DBE_HIRAGANA },
    },
    { { 0 } }  /* alternate table: unused */
  },
  { VK_OEM_NEC_EQUAL, KBDNLS_TYPE_NORMAL, KBDNLS_INDEX_NORMAL, 0,
    {
      { KBDNLS_SEND_BASE_VK, 0 },
      { KBDNLS_SEND_BASE_VK, 0 },
      { KBDNLS_SEND_PARAM_VK, 0x91 },
      { KBDNLS_SEND_BASE_VK, 0 },
      { KBDNLS_SEND_BASE_VK, 0 },
      { KBDNLS_SEND_BASE_VK, 0 },
      { KBDNLS_SEND_BASE_VK, 0 },
      { KBDNLS_SEND_BASE_VK, 0 },
    },
    { { 0 } }  /* alternate table: unused */
  },
  { VK_HOME, KBDNLS_TYPE_NORMAL, KBDNLS_INDEX_NORMAL, 0,
    {
      { KBDNLS_HOME_OR_CLEAR, 0 },
      { KBDNLS_SEND_BASE_VK, 0 },
      { KBDNLS_HOME_OR_CLEAR, 0 },
      { KBDNLS_HOME_OR_CLEAR, 0 },
      { KBDNLS_HOME_OR_CLEAR, 0 },
      { KBDNLS_HOME_OR_CLEAR, 0 },
      { KBDNLS_HOME_OR_CLEAR, 0 },
      { KBDNLS_HOME_OR_CLEAR, 0 },
    },
    { { 0 } }  /* alternate table: unused */
  },
  { VK_END, KBDNLS_TYPE_NORMAL, KBDNLS_INDEX_NORMAL, 0,
    {
      { KBDNLS_HELP_OR_END, 0 },
      { KBDNLS_HELP_OR_END, 0 },
      { KBDNLS_HELP_OR_END, 0 },
      { KBDNLS_HELP_OR_END, 0 },
      { KBDNLS_HELP_OR_END, 0 },
      { KBDNLS_HELP_OR_END, 0 },
      { KBDNLS_HELP_OR_END, 0 },
      { KBDNLS_HELP_OR_END, 0 },
    },
    { { 0 } }  /* alternate table: unused */
  },
};

ROSDATA USHORT mouse_vk[] = {
  VK_CLEAR,
  VK_PRIOR,
  VK_NEXT,
  VK_END,
  VK_HOME,
  VK_LEFT,
  VK_UP,
  VK_RIGHT,
  VK_DOWN,
  VK_INSERT,
  VK_DELETE,
  VK_MULTIPLY,
  VK_ADD,
  VK_SUBTRACT,
  VK_DIVIDE | KBDEXT,
  VK_HOME | KBDEXT
};

/*
 * The DLL stores two KBDNLSTABLES records back to back (default
 * OEMIdentifier 0 and OEMIdentifier 0x0D); KbdNlsLayerDescriptor()
 * returns the first one.
 */
ROSDATA KBDNLSTABLES nls_tables[] = {
  { 0x0000, 2, RTL_NUMBER_OF(vk_to_f), vk_to_f, RTL_NUMBER_OF(mouse_vk), mouse_vk },
  { 0x000d, 2, RTL_NUMBER_OF(vk_to_f), vk_to_f, RTL_NUMBER_OF(mouse_vk), mouse_vk },
};

/* ------------------------------------------------------------------ */
/* Finally, the master table                                          */
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

    0, /* fLocaleFlags (the original DLL stores 0 here) */

    /* Ligatures -- none */
    0,
    0,
    NULL
  },
  7,      /* dwType: Japanese keyboard */
  0x0d02  /* dwSubType: NEC PC-9801 */
};

PKBDTABLES WINAPI KbdLayerDescriptor(VOID)
{
  return &keyboard_layout_table.Base;
}

PKBDNLSTABLES WINAPI KbdNlsLayerDescriptor(VOID)
{
  return &nls_tables[0];
}
