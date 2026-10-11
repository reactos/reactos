/*
 * PROJECT:     ReactOS Keyboard Layouts
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Japanese NEC PC-9801 Windows XP keyboard layout
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winuser.h>
#include <ndk/kbd.h>

/* See also: https://kbdlayout.info/kbdnecnt */

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

#define KBDKANA    0x08
#define VK_EMPTY   0xff
#define VK_OEM_FJ_JISHO 0x92
#define KNUMS      (KBDNUMPAD | KBDSPECIAL)
#define CAPKANA    (CAPLOK | KANALOK)

ROSDATA USHORT scancode_to_vk[] = {
  /* 00 */
  VK_EMPTY, VK_ESCAPE, '1', '2',
  '3', '4', '5', '6',
  '7', '8', '9', '0',
  VK_OEM_MINUS, VK_OEM_PLUS, VK_BACK, VK_TAB,
  /* 10 */
  'Q', 'W', 'E', 'R',
  'T', 'Y', 'U', 'I',
  'O', 'P', VK_OEM_4, VK_OEM_6,
  VK_RETURN, VK_LCONTROL, 'A', 'S',
  /* 20 */
  'D', 'F', 'G', 'H',
  'J', 'K', 'L', VK_OEM_1,
  VK_OEM_7, VK_OEM_3, VK_LSHIFT, VK_OEM_5,
  'Z', 'X', 'C', 'V',
  /* 30 */
  'B', 'N', 'M', VK_OEM_COMMA,
  VK_OEM_PERIOD, VK_OEM_2, VK_RSHIFT | KBDEXT, VK_MULTIPLY,
  VK_LMENU, VK_SPACE, VK_CAPITAL, VK_F1,
  VK_F2, VK_F3, VK_F4, VK_F5,
  /* 40 */
  VK_F6, VK_F7,
  VK_F8, VK_F9,
  VK_F10, VK_NUMLOCK | KBDEXT | KBDMULTIVK,
  VK_SCROLL, VK_HOME | KBDSPECIAL | KBDNUMPAD,
  VK_UP | KBDSPECIAL | KBDNUMPAD, VK_PRIOR | KBDSPECIAL | KBDNUMPAD,
  VK_SUBTRACT, VK_LEFT | KBDSPECIAL | KBDNUMPAD,
  VK_CLEAR | KBDSPECIAL | KBDNUMPAD, VK_RIGHT | KBDSPECIAL | KBDNUMPAD,
  VK_ADD, VK_END | KBDSPECIAL | KBDNUMPAD,
  /* 50 */
  VK_DOWN | KBDSPECIAL | KBDNUMPAD, VK_NEXT | KBDSPECIAL | KBDNUMPAD,
  VK_INSERT | KBDSPECIAL | KBDNUMPAD, VK_DELETE | KBDSPECIAL | KBDNUMPAD,
  VK_SNAPSHOT, VK_OEM_8,
  VK_EMPTY, VK_F11,
  VK_F12, VK_OEM_FJ_JISHO | KBDSPECIAL,
  VK_NONCONVERT | KBDSPECIAL, VK_DBE_DETERMINESTRING,
  VK_SEPARATOR, VK_F13,
  VK_F14, VK_F15,
};

ROSDATA VSC_VK extcode0_to_vk[] = {
  { 0x1C, VK_RETURN | KBDEXT },
  { 0x1D, VK_KANA | KBDEXT | KBDSPECIAL },
  { 0x35, VK_DIVIDE | KBDEXT },
  { 0x37, VK_SNAPSHOT | KBDEXT },
  { 0x38, VK_KANJI | KBDEXT | KBDSPECIAL },
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
  { 0, 0 }
};

ROSDATA VSC_VK extcode1_to_vk[] = {
  { 0x1d, VK_PAUSE },
  { 0, 0 }
};

ROSDATA VK_TO_BIT modifier_keys[] = {
  { VK_SHIFT,   KBDSHIFT },
  { VK_CONTROL, KBDCTRL },
  { VK_MENU,    KBDALT },
  { VK_KANA,    KBDKANA },
  { 0,          0 }
};

/* Index: Kana(8)|Alt(4)|Ctrl(2)|Shift(1) -> column */
ROSDATA MODIFIERS modifier_bits = {
  modifier_keys,
  11,
  {
      0,            // 0000: none
      1,            // 0001: Shift
      4,            // 0010: Ctrl
      6,            // 0011: Shift+Ctrl
      SHFT_INVALID, // 0100: Alt
      SHFT_INVALID, // 0101
      SHFT_INVALID, // 0110
      SHFT_INVALID, // 0111
      2,            // 1000: Kana
      3,            // 1001: Kana+Shift
      5,            // 1010: Kana+Ctrl
      7,            // 1011: Kana+Shift+Ctrl
  }
};

ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] = {
  { '0', KANALOK, {'0', '0', 0xff9c, 0xff66} },  /* ﾜｦ */
  { '1', KANALOK, {'1', '!', 0xff87, 0xff87} },  /* ﾇﾇ */
  { '3', KANALOK, {'3', '#', 0xff71, 0xff67} },  /* ｱｧ */
  { '4', KANALOK, {'4', '$', 0xff73, 0xff69} },  /* ｳｩ */
  { '5', KANALOK, {'5', '%', 0xff74, 0xff6a} },  /* ｴｪ */
  { '7', KANALOK, {'7', '\'', 0xff94, 0xff6c} },  /* ﾔｬ */
  { '8', KANALOK, {'8', '(', 0xff95, 0xff6d} },  /* ﾕｭ */
  { '9', KANALOK, {'9', ')', 0xff96, 0xff6e} },  /* ﾖｮ */
  { 'A', CAPKANA, {'a', 'A', 0xff81, 0xff81} },  /* ﾁﾁ */
  { 'B', CAPKANA, {'b', 'B', 0xff7a, 0xff7a} },  /* ｺｺ */
  { 'C', CAPKANA, {'c', 'C', 0xff7f, 0xff7f} },  /* ｿｿ */
  { 'D', CAPKANA, {'d', 'D', 0xff7c, 0xff7c} },  /* ｼｼ */
  { 'E', CAPKANA, {'e', 'E', 0xff72, 0xff68} },  /* ｲｨ */
  { 'F', CAPKANA, {'f', 'F', 0xff8a, 0xff8a} },  /* ﾊﾊ */
  { 'G', CAPKANA, {'g', 'G', 0xff77, 0xff77} },  /* ｷｷ */
  { 'H', CAPKANA, {'h', 'H', 0xff78, 0xff78} },  /* ｸｸ */
  { 'I', CAPKANA, {'i', 'I', 0xff86, 0xff86} },  /* ﾆﾆ */
  { 'J', CAPKANA, {'j', 'J', 0xff8f, 0xff8f} },  /* ﾏﾏ */
  { 'K', CAPKANA, {'k', 'K', 0xff89, 0xff89} },  /* ﾉﾉ */
  { 'L', CAPKANA, {'l', 'L', 0xff98, 0xff98} },  /* ﾘﾘ */
  { 'M', CAPKANA, {'m', 'M', 0xff93, 0xff93} },  /* ﾓﾓ */
  { 'N', CAPKANA, {'n', 'N', 0xff90, 0xff90} },  /* ﾐﾐ */
  { 'O', CAPKANA, {'o', 'O', 0xff97, 0xff97} },  /* ﾗﾗ */
  { 'P', CAPKANA, {'p', 'P', 0xff7e, 0xff7e} },  /* ｾｾ */
  { 'Q', CAPKANA, {'q', 'Q', 0xff80, 0xff80} },  /* ﾀﾀ */
  { 'R', CAPKANA, {'r', 'R', 0xff7d, 0xff7d} },  /* ｽｽ */
  { 'S', CAPKANA, {'s', 'S', 0xff84, 0xff84} },  /* ﾄﾄ */
  { 'T', CAPKANA, {'t', 'T', 0xff76, 0xff76} },  /* ｶｶ */
  { 'U', CAPKANA, {'u', 'U', 0xff85, 0xff85} },  /* ﾅﾅ */
  { 'V', CAPKANA, {'v', 'V', 0xff8b, 0xff8b} },  /* ﾋﾋ */
  { 'W', CAPKANA, {'w', 'W', 0xff83, 0xff83} },  /* ﾃﾃ */
  { 'X', CAPKANA, {'x', 'X', 0xff7b, 0xff7b} },  /* ｻｻ */
  { 'Y', CAPKANA, {'y', 'Y', 0xff9d, 0xff9d} },  /* ﾝﾝ */
  { 'Z', CAPKANA, {'z', 'Z', 0xff82, 0xff6f} },  /* ﾂｯ */
  { VK_OEM_1, KANALOK, {':', '*', 0xff79, 0xff79} },  /* ｹｹ */
  { VK_OEM_2, KANALOK, {'/', '?', 0xff92, 0xff65} },  /* ﾒ･ */
  { VK_OEM_COMMA, KANALOK, {',', '<', 0xff88, 0xff64} },  /* ﾈ､ */
  { VK_OEM_PERIOD, KANALOK, {'.', '>', 0xff99, 0xff61} },  /* ﾙ｡ */
  { VK_OEM_PLUS, KANALOK, {';', '+', 0xff9a, 0xff9a} },  /* ﾚﾚ */
  { VK_TAB, 0, {0x09, 0x09, 0x09, 0x09} },
  { VK_ADD, 0, {'+', '+', '+', '+'} },
  { VK_DECIMAL, 0, {'.', '.', '.', '.'} },
  { VK_DIVIDE, 0, {'/', '/', '/', '/'} },
  { VK_MULTIPLY, 0, {'*', '*', '*', '*'} },
  { VK_SUBTRACT, 0, {'-', '-', '-', '-'} },
  { VK_SEPARATOR, 0, {',', ',', ',', ','} },
  { VK_OEM_FJ_JISHO, 0, {'=', '=', '=', '='} },
  { VK_NUMPAD0, 0, {'0', '0', '0', '0'} },
  { VK_NUMPAD1, 0, {'1', '1', '1', '1'} },
  { VK_NUMPAD2, 0, {'2', '2', '2', '2'} },
  { VK_NUMPAD3, 0, {'3', '3', '3', '3'} },
  { VK_NUMPAD4, 0, {'4', '4', '4', '4'} },
  { VK_NUMPAD5, 0, {'5', '5', '5', '5'} },
  { VK_NUMPAD6, 0, {'6', '6', '6', '6'} },
  { VK_NUMPAD7, 0, {'7', '7', '7', '7'} },
  { VK_NUMPAD8, 0, {'8', '8', '8', '8'} },
  { VK_NUMPAD9, 0, {'9', '9', '9', '9'} },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS6 key_to_chars_6mod[] = {
  { VK_BACK, 0, {0x08, 0x08, 0x08, 0x08, 0x007f, 0x007f} },
  { VK_CANCEL, 0, {0x03, 0x03, 0x03, 0x03, 0x03, 0x03} },
  { VK_ESCAPE, 0, {0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b} },
  { VK_OEM_3, KANALOK, {'@', '~', 0xff9e, 0xff9e, 0x00, 0x00} },  /* ﾞﾞ */
  { VK_OEM_4, KANALOK, {'[', '{', 0xff9f, 0xff62, 0x1b, 0x1b} },  /* ﾟ｢ */
  { VK_OEM_4, KANALOK, {'[', '{', 0xff9f, 0xff62, 0x1b, 0x1b} },  /* ﾟ｢ */
  { VK_OEM_5, KANALOK, {'\\', '|', 0xff70, 0xff70, 0x1c, 0x1c} },  /* ｰｰ */
  { VK_OEM_6, KANALOK, {']', '}', 0xff91, 0xff63, 0x1d, 0x1d} },  /* ﾑ｣ */
  { VK_OEM_7, KANALOK, {'^', '`', 0xff8d, 0xff8d, 0x1e, 0x1e} },  /* ﾍﾍ */
  { VK_OEM_8, KANALOK, {WCH_NONE, '_', 0xff9b, 0xff9b, 0x1f, 0x1f} },  /* ﾛﾛ */
  { VK_RETURN, 0, {0x0d, 0x0d, 0x0d, 0x0d, 0x0a, 0x0a} },
  { VK_SPACE, 0, {' ', ' ', ' ', ' ', ' ', ' '} },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS8 key_to_chars_8mod[] = {
  { '2', KANALOK, {'2', '\"', 0xff8c, 0xff8c, WCH_NONE, WCH_NONE, 0x00, 0x00} },  /* ﾌﾌ */
  { '6', KANALOK, {'6', '&', 0xff75, 0xff6b, WCH_NONE, WCH_NONE, 0x1e, 0x1e} },  /* ｵｫ */
  { VK_OEM_MINUS, KANALOK, {'-', '=', 0xff8e, 0xff8e, WCH_NONE, WCH_NONE, 0x1f, 0x1f} },  /* ﾎﾎ */
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
  { 0x01, L"ESC" },
  { 0x0e, L"BS" },
  { 0x0f, L"TAB" },
  { 0x1c, L"RETURN" },
  { 0x1d, L"CTRL" },
  { 0x2a, L"SHIFT" },
  { 0x36, L"SHIFT" },
  { 0x37, L"Num *" },
  { 0x38, L"GRPH" },
  { 0x39, L"SPACE" },
  { 0x3a, L"CAPS" },
  { 0x3b, L"f\uFF651" },  /* f･1 */
  { 0x3c, L"f\uFF652" },  /* f･2 */
  { 0x3d, L"f\uFF653" },  /* f･3 */
  { 0x3e, L"f\uFF654" },  /* f･4 */
  { 0x3f, L"f\uFF655" },  /* f･5 */
  { 0x40, L"f\uFF656" },  /* f･6 */
  { 0x41, L"f\uFF657" },  /* f･7 */
  { 0x42, L"f\uFF658" },  /* f･8 */
  { 0x43, L"f\uFF659" },  /* f･9 */
  { 0x44, L"f\uFF6510" },  /* f･10 */
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
  { 0x53, L"Num ." },
  { 0x54, L"Sys Req" },
  { 0x55, L"" },
  { 0x57, L"f\uFF6511" },  /* f･11 */
  { 0x58, L"f\uFF6512" },  /* f･12 */
  { 0x59, L"Num =" },
  { 0x5a, L"NFER" },
  { 0x5b, L"Convert" },
  { 0x5c, L"Num ," },
  { 0x5d, L"f\uFF6513" },  /* f･13 */
  { 0x5e, L"f\uFF6514" },  /* f･14 */
  { 0x5f, L"f\uFF6515" },  /* f･15 */
  { 0, NULL }
};

ROSDATA VSC_LPWSTR extended_key_names[] = {
  { 0x1c, L"Num Enter" },
  { 0x1d, L"\uFF76\uFF85" },  /* ｶﾅ */
  { 0x35, L"Num /" },
  { 0x37, L"COPY" },
  { 0x38, L"XFER" },
  { 0x45, L"Num Lock" },
  { 0x46, L"STOP" },
  { 0x47, L"CLR" },
  { 0x48, L"UP" },
  { 0x49, L"ROLL DOWN" },
  { 0x4b, L"LEFT" },
  { 0x4d, L"RIGHT" },
  { 0x4f, L"HELP" },
  { 0x50, L"DOWN" },
  { 0x51, L"ROLL UP" },
  { 0x52, L"INS" },
  { 0x53, L"DEL" },
  { 0x54, L"<00>" },
  { 0x56, L"HELP" },
  { 0x5b, L"Left Windows" },
  { 0x5c, L"Right Windows" },
  { 0x5d, L"Application" },
  { 0, NULL }
};

ROSDATA KBDTABLES_FE keyboard_layout_table = {
  {
    &modifier_bits,
    vk_to_wchar_master_table,
    NULL,                       /* no dead keys */
    (VSC_LPWSTR *)key_names,
    (VSC_LPWSTR *)extended_key_names,
    NULL,
    scancode_to_vk,
    RTL_NUMBER_OF(scancode_to_vk),
    extcode0_to_vk,
    extcode1_to_vk,
    MAKELONG(0, 0),
    0, 0, NULL
  },
  7,     /* dwType: Japanese keyboard */
  0x0d02 /* dwSubType: NEC (0x0d) PC-9800 series (0x02) */
};

ROSDATA VK_F key_vk_to_f[] = {
  {
    VK_NONCONVERT, KBDNLS_TYPE_NORMAL, KBDNLS_INDEX_NORMAL, 0,
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
    {
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
    },
  },
  {
    VK_KANJI, KBDNLS_TYPE_NORMAL, KBDNLS_INDEX_NORMAL, 0,
    {
      { KBDNLS_SEND_PARAM_VK, VK_CONVERT },
      { KBDNLS_SEND_PARAM_VK, VK_CONVERT },
      { KBDNLS_SEND_BASE_VK, 0 },
      { KBDNLS_SEND_PARAM_VK, VK_CONVERT },
      { KBDNLS_ROMAN, 0 },
      { KBDNLS_SEND_PARAM_VK, VK_DBE_ENTERIMECONFIGMODE },
      { KBDNLS_SEND_PARAM_VK, VK_DBE_CODEINPUT },
      { KBDNLS_SEND_PARAM_VK, VK_DBE_HIRAGANA },
    },
    {
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
    },
  },
  {
    VK_OEM_FJ_JISHO, KBDNLS_TYPE_NORMAL, KBDNLS_INDEX_NORMAL, 0,
    {
      { KBDNLS_SEND_BASE_VK, 0 },
      { KBDNLS_SEND_BASE_VK, 0 },
      { KBDNLS_SEND_PARAM_VK, VK_SCROLL },
      { KBDNLS_SEND_BASE_VK, 0 },
      { KBDNLS_SEND_BASE_VK, 0 },
      { KBDNLS_SEND_BASE_VK, 0 },
      { KBDNLS_SEND_BASE_VK, 0 },
      { KBDNLS_SEND_BASE_VK, 0 },
    },
    {
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
    },
  },
  {
    VK_HOME, KBDNLS_TYPE_NORMAL, KBDNLS_INDEX_NORMAL, 0,
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
    {
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
    },
  },
  {
    VK_END, KBDNLS_TYPE_NORMAL, KBDNLS_INDEX_NORMAL, 0,
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
    {
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
      { KBDNLS_NULL, 0 },
    },
  },
};

ROSDATA USHORT mouse_vkeys[] = {
  VK_NUMPAD5, VK_NUMPAD9, VK_NUMPAD3, VK_NUMPAD1, VK_NUMPAD7, VK_NUMPAD4, VK_NUMPAD8,
  VK_NUMPAD6, VK_NUMPAD2, VK_NUMPAD0, VK_DECIMAL, VK_MULTIPLY, VK_ADD, VK_SUBTRACT,
  VK_DIVIDE | KBDEXT, VK_HOME | KBDEXT
};

ROSDATA KBDNLSTABLES KbdNlsTables = {
  0x0d,                         /* OEMIdentifier: NEC */
  0x02,                         /* LayoutInformation */
  RTL_NUMBER_OF(key_vk_to_f),
  key_vk_to_f,
  RTL_NUMBER_OF(mouse_vkeys),
  mouse_vkeys
};

PKBDTABLES WINAPI KbdLayerDescriptor(VOID) {
  return &keyboard_layout_table.Base;
}

PKBDNLSTABLES WINAPI KbdNlsLayerDescriptor(VOID) {
  return &KbdNlsTables;
}
