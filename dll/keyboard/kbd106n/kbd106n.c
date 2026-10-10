/*
 * PROJECT:     ReactOS Keyboard Layouts
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Japanese (106 variant) keyboard layout
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winuser.h>
#include <ndk/kbd.h>

/* See also: https://kbdlayout.info/kbd106n */

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

#define VK_EMPTY  0xff
#define VK_OEM_RESERVED_EC 0xec
#define KBDKANA   0x08 /* Kana modifier bit */
#define KNUMS     (KBDNUMPAD | KBDSPECIAL)
#define KMEXT     (KBDEXT | KBDMULTIVK)

ROSDATA USHORT scancode_to_vk[] = {
  /* 00 */ VK_EMPTY, VK_ESCAPE, '1', '2',
  /* 04 */ '3', '4', '5', '6',
  /* 08 */ '7', '8', '9', '0',
  /* 0C */ VK_OEM_MINUS, VK_OEM_7, VK_BACK, VK_TAB,
  /* 10 */ 'Q', 'W', 'E', 'R',
  /* 14 */ 'T', 'Y', 'U', 'I',
  /* 18 */ 'O', 'P', VK_OEM_3, VK_OEM_4,
  /* 1C */ VK_RETURN, VK_LCONTROL, 'A', 'S',
  /* 20 */ 'D', 'F', 'G', 'H',
  /* 24 */ 'J', 'K', 'L', VK_OEM_PLUS,
  /* 28 */ VK_OEM_1, VK_DBE_SBCSCHAR | KBDSPECIAL, VK_LSHIFT, VK_OEM_6,
  /* 2C */ 'Z', 'X', 'C', 'V',
  /* 30 */ 'B', 'N', 'M', VK_OEM_COMMA,
  /* 34 */ VK_OEM_PERIOD, VK_OEM_2, VK_RSHIFT | KBDEXT, VK_MULTIPLY | KBDMULTIVK,
  /* 38 */ VK_LMENU, VK_SPACE, VK_DBE_ALPHANUMERIC | KBDSPECIAL, VK_F1,
  /* 3C */ VK_F2, VK_F3, VK_F4, VK_F5,
  /* 40 */ VK_F6, VK_F7, VK_F8, VK_F9,
  /* 44 */ VK_F10, VK_NUMLOCK | KBDEXT | KBDMULTIVK, VK_SCROLL | KBDMULTIVK, VK_HOME | KBDSPECIAL | KBDNUMPAD,
  /* 48 */ VK_UP | KBDSPECIAL | KBDNUMPAD, VK_PRIOR | KBDSPECIAL | KBDNUMPAD, VK_SUBTRACT, VK_LEFT | KBDSPECIAL | KBDNUMPAD,
  /* 4C */ VK_CLEAR | KBDSPECIAL | KBDNUMPAD, VK_RIGHT | KBDSPECIAL | KBDNUMPAD, VK_ADD, VK_END | KBDSPECIAL | KBDNUMPAD,
  /* 50 */ VK_DOWN | KBDSPECIAL | KBDNUMPAD, VK_NEXT | KBDSPECIAL | KBDNUMPAD, VK_INSERT | KBDSPECIAL | KBDNUMPAD, VK_DELETE | KBDSPECIAL | KBDNUMPAD,
  /* 54 */ VK_SNAPSHOT, VK_EMPTY, VK_EMPTY, VK_F11,
  /* 58 */ VK_F12, VK_CLEAR, VK_NONAME, VK_NONAME,
  /* 5C */ VK_NONAME, VK_EREOF, VK_EMPTY, VK_NONAME,
  /* 60 */ VK_EMPTY, VK_EMPTY, VK_EMPTY, VK_EMPTY,
  /* 64 */ VK_F13, VK_F14, VK_F15, VK_F16,
  /* 68 */ VK_F17, VK_F18, VK_F19, VK_F20,
  /* 6C */ VK_F21, VK_F22, VK_F23, VK_EMPTY,
  /* 70 */ VK_DBE_HIRAGANA | KBDSPECIAL, VK_EMPTY, VK_EMPTY, VK_OEM_102,
  /* 74 */ VK_EMPTY, VK_EMPTY, VK_F24, VK_EMPTY,
  /* 78 */ VK_EMPTY, VK_CONVERT | KBDSPECIAL, VK_EMPTY, VK_NONCONVERT | KBDSPECIAL,
  /* 7C */ VK_TAB, VK_OEM_5, VK_ABNT_C2, VK_OEM_RESERVED_EC,
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

ROSDATA VK_TO_BIT modifier_keys[] = {
  { VK_SHIFT,   KBDSHIFT },
  { VK_CONTROL, KBDCTRL },
  { VK_MENU,    KBDALT },
  { 0,          0 }
};

/* 0=Normal 1=Shift 2=Kana 3=Kana+Shift 4=Ctrl 5=Kana+Ctrl 6=Ctrl+Shift 7=Kana+Ctrl+Shift */
ROSDATA MODIFIERS modifier_bits = {
  modifier_keys,
  11, /* wMaxModBits */
  {
    0,           /* 0000 */
    1,           /* Shift */
    4,           /* Ctrl */
    6,           /* Ctrl+Shift */
    SHFT_INVALID, SHFT_INVALID, SHFT_INVALID, SHFT_INVALID, /* Alt-ish */
    2,           /* Kana */
    3,           /* Kana+Shift */
    5,           /* Kana+Ctrl */
    7            /* Kana+Ctrl+Shift */
  }
};

ROSDATA VK_TO_WCHARS6 key_to_chars_6mod[] = {
  { VK_BACK, 0, {0x0008, 0x0008, 0x0008, 0x0008, 0x007F, 0x007F} },
  { VK_CANCEL, 0, {0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003} },
  { VK_ESCAPE, 0, {0x001B, 0x001B, 0x001B, 0x001B, 0x001B, 0x001B} },
  { VK_OEM_4, KANALOK, {'[', '{', 0xFF9F, 0xFF62, 0x001B, 0x001B} },
  { VK_OEM_5, KANALOK, {'\\', '|', 0xFF70, 0xFF70, 0x001C, 0x001C} },
  { VK_OEM_102, KANALOK, {'\\', '_', 0xFF9B, 0xFF9B, 0x001C, 0x001C} },
  { VK_OEM_6, KANALOK, {']', '}', 0xFF91, 0xFF63, 0x001D, 0x001D} },
  { VK_RETURN, 0, {0x000D, 0x000D, 0x000D, 0x000D, 0x000A, 0x000A} },
  { VK_SPACE, 0, {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020} },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS8 key_to_chars_8mod[] = {
  { '2', KANALOK, {'2', '"', 0xFF8C, 0xFF8C, WCH_NONE, WCH_NONE, 0x0000, 0x0000} },
  { '6', KANALOK, {'6', '&', 0xFF75, 0xFF6B, WCH_NONE, WCH_NONE, 0x001E, 0x001E} },
  { VK_OEM_MINUS, KANALOK, {'-', '=', 0xFF8E, 0xFF8E, WCH_NONE, WCH_NONE, 0x001F, 0x001F} },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] = {
  { '0', KANALOK, {'0', WCH_NONE, 0xFF9C, 0xFF66} },
  { '1', KANALOK, {'1', '!', 0xFF87, 0xFF87} },
  { '3', KANALOK, {'3', '#', 0xFF71, 0xFF67} },
  { '4', KANALOK, {'4', '$', 0xFF73, 0xFF69} },
  { '5', KANALOK, {'5', '%', 0xFF74, 0xFF6A} },
  { '7', KANALOK, {'7', '\'', 0xFF94, 0xFF6C} },
  { '8', KANALOK, {'8', '(', 0xFF95, 0xFF6D} },
  { '9', KANALOK, {'9', ')', 0xFF96, 0xFF6E} },
  { 'A', CAPLOK|KANALOK, {'a', 'A', 0xFF81, 0xFF81} },
  { 'B', CAPLOK|KANALOK, {'b', 'B', 0xFF7A, 0xFF7A} },
  { 'C', CAPLOK|KANALOK, {'c', 'C', 0xFF7F, 0xFF7F} },
  { 'D', CAPLOK|KANALOK, {'d', 'D', 0xFF7C, 0xFF7C} },
  { 'E', CAPLOK|KANALOK, {'e', 'E', 0xFF72, 0xFF68} },
  { 'F', CAPLOK|KANALOK, {'f', 'F', 0xFF8A, 0xFF8A} },
  { 'G', CAPLOK|KANALOK, {'g', 'G', 0xFF77, 0xFF77} },
  { 'H', CAPLOK|KANALOK, {'h', 'H', 0xFF78, 0xFF78} },
  { 'I', CAPLOK|KANALOK, {'i', 'I', 0xFF86, 0xFF86} },
  { 'J', CAPLOK|KANALOK, {'j', 'J', 0xFF8F, 0xFF8F} },
  { 'K', CAPLOK|KANALOK, {'k', 'K', 0xFF89, 0xFF89} },
  { 'L', CAPLOK|KANALOK, {'l', 'L', 0xFF98, 0xFF98} },
  { 'M', CAPLOK|KANALOK, {'m', 'M', 0xFF93, 0xFF93} },
  { 'N', CAPLOK|KANALOK, {'n', 'N', 0xFF90, 0xFF90} },
  { 'O', CAPLOK|KANALOK, {'o', 'O', 0xFF97, 0xFF97} },
  { 'P', CAPLOK|KANALOK, {'p', 'P', 0xFF7E, 0xFF7E} },
  { 'Q', CAPLOK|KANALOK, {'q', 'Q', 0xFF80, 0xFF80} },
  { 'R', CAPLOK|KANALOK, {'r', 'R', 0xFF7D, 0xFF7D} },
  { 'S', CAPLOK|KANALOK, {'s', 'S', 0xFF84, 0xFF84} },
  { 'T', CAPLOK|KANALOK, {'t', 'T', 0xFF76, 0xFF76} },
  { 'U', CAPLOK|KANALOK, {'u', 'U', 0xFF85, 0xFF85} },
  { 'V', CAPLOK|KANALOK, {'v', 'V', 0xFF8B, 0xFF8B} },
  { 'W', CAPLOK|KANALOK, {'w', 'W', 0xFF83, 0xFF83} },
  { 'X', CAPLOK|KANALOK, {'x', 'X', 0xFF7B, 0xFF7B} },
  { 'Y', CAPLOK|KANALOK, {'y', 'Y', 0xFF9D, 0xFF9D} },
  { 'Z', CAPLOK|KANALOK, {'z', 'Z', 0xFF82, 0xFF6F} },
  { VK_OEM_1, KANALOK, {':', '*', 0xFF79, 0xFF79} },
  { VK_OEM_2, KANALOK, {'/', '?', 0xFF92, 0xFF65} },
  { VK_OEM_3, KANALOK, {'@', '`', 0xFF9E, 0xFF9E} },
  { VK_OEM_7, KANALOK, {'^', '~', 0xFF8D, 0xFF8D} },
  { VK_OEM_8, 0, {',', ',', ',', ','} },
  { VK_OEM_COMMA, KANALOK, {',', '<', 0xFF88, 0xFF64} },
  { VK_OEM_PERIOD, KANALOK, {'.', '>', 0xFF99, 0xFF61} },
  { VK_OEM_PLUS, KANALOK, {';', '+', 0xFF9A, 0xFF9A} },
  { VK_TAB, 0, {0x0009, 0x0009, 0x0009, 0x0009} },
  { VK_ADD, 0, {'+', '+', '+', '+'} },
  { VK_DECIMAL, 0, {'.', '.', '.', '.'} },
  { VK_DIVIDE, 0, {'/', '/', '/', '/'} },
  { VK_MULTIPLY, 0, {'*', '*', '*', '*'} },
  { VK_SUBTRACT, 0, {'-', '-', '-', '-'} },
  { 0, 0 }
};

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

ROSDATA VSC_LPWSTR key_names[] = {
  { 0x01, L"Esc" },
  { 0x0E, L"Backspace" },
  { 0x0F, L"Tab" },
  { 0x1C, L"Enter" },
  { 0x1D, L"Ctrl" },
  { 0x29, L"\x534A\u89D2/\u5168\u89D2" }, /* Hankaku/Zenkaku */
  { 0x2A, L"Shift" },
  { 0x36, L"Right Shift" },
  { 0x37, L"Num *" },
  { 0x38, L"Alt" },
  { 0x39, L"Space" },
  { 0x3A, L"Caps Lock" },
  { 0x3B, L"F1" },
  { 0x3C, L"F2" },
  { 0x3D, L"F3" },
  { 0x3E, L"F4" },
  { 0x3F, L"F5" },
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
  { 0x4A, L"Num -" },
  { 0x4B, L"Num 4" },
  { 0x4C, L"Num 5" },
  { 0x4D, L"Num 6" },
  { 0x4E, L"Num +" },
  { 0x4F, L"Num 1" },
  { 0x50, L"Num 2" },
  { 0x51, L"Num 3" },
  { 0x52, L"Num 0" },
  { 0x53, L"Num Del" },
  { 0x54, L"Sys Req" },
  { 0x57, L"F11" },
  { 0x58, L"F12" },
  { 0x70, L"\u3072\u3089\u304C\u306A" }, /* Hiragana */
  { 0x79, L"\u5909\u63DB" }, /* Convert */
  { 0x7B, L"\u7121\u5909\u63DB" }, /* Non Convert */
  { 0x7C, L"F13" },
  { 0, NULL },
};

ROSDATA VSC_LPWSTR extended_key_names[] = {
  { 0x1C, L"Num Enter" },
  { 0x1D, L"Right Control" },
  { 0x35, L"Num /" },
  { 0x37, L"Prnt Scrn" },
  { 0x38, L"Right Alt" },
  { 0x45, L"Num Lock" },
  { 0x46, L"Break" },
  { 0x47, L"Home" },
  { 0x48, L"Up" },
  { 0x49, L"Page Up" },
  { 0x4B, L"Left" },
  { 0x4D, L"Right" },
  { 0x4F, L"End" },
  { 0x50, L"Down" },
  { 0x51, L"Page Down" },
  { 0x52, L"Insert" },
  { 0x53, L"Delete" },
  { 0x5B, L"Left Windows" },
  { 0x5C, L"Right Windows" },
  { 0x5D, L"Application" },
  { 0, NULL },
};

ROSDATA KBDTABLES_FE keyboard_layout_table = {
  {
    &modifier_bits,
    vk_to_wchar_master_table,
    NULL,                         /* No dead key  */
    key_names,
    extended_key_names,
    NULL,                         /* No dead key names */
    scancode_to_vk,
    RTL_NUMBER_OF(scancode_to_vk),/* 0x80 */
    extcode0_to_vk,
    extcode1_to_vk,
    0,                            /* fLocaleFlags (No KLLF_ALTGR etc.) */
    0, 0, NULL                    /* No ligatures */
  },
  7,   /* dwType : 106 keyboard */
  2    /* dwSubType */
};

PKBDTABLES WINAPI KbdLayerDescriptor(VOID) {
  return (PKBDTABLES)&keyboard_layout_table;
}

/* ---- NLS Table ---- */
#define PROC(i,p) { i, p }
#define NLSNO  PROC(KBDNLS_NOEVENT,0)

ROSDATA VK_F vk_to_function[] = {
  /* CapsLock/Alphanumeric */
  { VK_DBE_ALPHANUMERIC, KBDNLS_TYPE_TOGGLE, KBDNLS_INDEX_NORMAL, KBDNLS_INDEX_ALT,
    { PROC(KBDNLS_ALPHANUM,0), PROC(KBDNLS_SEND_PARAM_VK,0x14), PROC(KBDNLS_SEND_PARAM_VK,0x15),
      PROC(KBDNLS_ALPHANUM,0), PROC(KBDNLS_ALPHANUM,0), PROC(KBDNLS_ALPHANUM,0),
      PROC(KBDNLS_CODEINPUT,0), PROC(KBDNLS_CODEINPUT,0) },
    { PROC(KBDNLS_SEND_PARAM_VK,0x14), PROC(KBDNLS_SEND_PARAM_VK,0x14),
      NLSNO, NLSNO, NLSNO, NLSNO, NLSNO, NLSNO } },
  /* Hiragana */
  { VK_DBE_HIRAGANA, KBDNLS_TYPE_NORMAL, KBDNLS_INDEX_NORMAL, 0,
    { PROC(KBDNLS_HIRAGANA,0), PROC(KBDNLS_KATAKANA,0), PROC(KBDNLS_HIRAGANA,0), PROC(KBDNLS_KATAKANA,0),
      PROC(KBDNLS_ROMAN,0), PROC(KBDNLS_ROMAN,0), PROC(KBDNLS_ROMAN,0), NLSNO },
    { {0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0} } },
  /* Hankaku/Zenkaku */
  { VK_DBE_SBCSCHAR, KBDNLS_TYPE_NORMAL, KBDNLS_INDEX_NORMAL, 0,
    { PROC(KBDNLS_SBCSDBCS,0), PROC(KBDNLS_SBCSDBCS,0), PROC(KBDNLS_SBCSDBCS,0), PROC(KBDNLS_SBCSDBCS,0),
      PROC(KBDNLS_SEND_PARAM_VK,0x19), PROC(KBDNLS_SBCSDBCS,0),
      PROC(KBDNLS_SEND_PARAM_VK,0xF8), PROC(KBDNLS_SEND_PARAM_VK,0xF8) },
    { {0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0} } },
  /* Non Convert (VK_NONCONVERT) */
  { VK_NONCONVERT, KBDNLS_TYPE_NORMAL, KBDNLS_INDEX_NORMAL, 0,
    { PROC(KBDNLS_SEND_BASE_VK,0), PROC(KBDNLS_SEND_BASE_VK,0), PROC(KBDNLS_SEND_BASE_VK,0), PROC(KBDNLS_SEND_BASE_VK,0),
      PROC(KBDNLS_SEND_BASE_VK,0), PROC(KBDNLS_SEND_BASE_VK,0),
      PROC(KBDNLS_SEND_PARAM_VK,0xF7), PROC(KBDNLS_SEND_PARAM_VK,0xF7) },
    { {0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0} } },
};

ROSDATA KBDNLSTABLES NlsTables = {
  0,   /* OEMIdentifier */
  0,   /* LayoutInformation */
  4,   /* NumOfVkToF */
  vk_to_function,
  0,   /* NumOfMouseVKey */
  NULL
};

PKBDNLSTABLES WINAPI KbdNlsLayerDescriptor(VOID) {
  return &NlsTables;
}
