/*
 * PROJECT:     ReactOS Keyboard Layouts
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     NEC PC-9801 Windows 95 Japanese keyboard layout
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winuser.h>
#include <ndk/kbd.h>

/* See also: https://kbdlayout.info/kbdnec95 */

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

#define VK_EMPTY 0xff /* The non-existent VK */
#define KNUMS (KBDNUMPAD | KBDSPECIAL) /* Special + number pad */

#ifndef VK_OEM_NEC_EQUAL
  #define VK_OEM_NEC_EQUAL 0x92
#endif

/* Extra modifier: the KANA key (VK_KANA 0x15) is modifier bit 0x08 */
#define KBDKANA   0x08

/* PC-98 scan code -> VK (0x00 .. 0x7f) */
ROSDATA USHORT scancode_to_vk[] = {
  /* 00 */ VK_ESCAPE,
  /* 01 */ '1',
  /* 02 */ '2',
  /* 03 */ '3',
  /* 04 */ '4',
  /* 05 */ '5',
  /* 06 */ '6',
  /* 07 */ '7',
  /* 08 */ '8',
  /* 09 */ '9',
  /* 0a */ '0',
  /* 0b */ VK_OEM_MINUS,
  /* 0c */ VK_OEM_7,
  /* 0d */ VK_OEM_5,
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
  /* 1d */ 'A',
  /* 1e */ 'S',
  /* 1f */ 'D',
  /* 20 */ 'F',
  /* 21 */ 'G',
  /* 22 */ 'H',
  /* 23 */ 'J',
  /* 24 */ 'K',
  /* 25 */ 'L',
  /* 26 */ VK_OEM_PLUS,
  /* 27 */ VK_OEM_1,
  /* 28 */ VK_OEM_6,
  /* 29 */ 'Z',
  /* 2a */ 'X',
  /* 2b */ 'C',
  /* 2c */ 'V',
  /* 2d */ 'B',
  /* 2e */ 'N',
  /* 2f */ 'M',
  /* 30 */ VK_OEM_COMMA,
  /* 31 */ VK_OEM_PERIOD,
  /* 32 */ VK_OEM_2,
  /* 33 */ VK_OEM_8,
  /* 34 */ VK_SPACE,
  /* 35 */ VK_CONVERT | KBDSPECIAL,
  /* 36 */ VK_NEXT,
  /* 37 */ VK_PRIOR,
  /* 38 */ VK_INSERT,
  /* 39 */ VK_DELETE,
  /* 3a */ VK_UP,
  /* 3b */ VK_LEFT,
  /* 3c */ VK_RIGHT,
  /* 3d */ VK_DOWN,
  /* 3e */ VK_HOME,
  /* 3f */ VK_END,
  /* 40 */ VK_SUBTRACT,
  /* 41 */ VK_DIVIDE,
  /* 42 */ VK_NUMPAD7 | KNUMS,
  /* 43 */ VK_NUMPAD8 | KNUMS,
  /* 44 */ VK_NUMPAD9 | KNUMS,
  /* 45 */ VK_MULTIPLY,
  /* 46 */ VK_NUMPAD4 | KNUMS,
  /* 47 */ VK_NUMPAD5 | KNUMS,
  /* 48 */ VK_NUMPAD6 | KNUMS,
  /* 49 */ VK_ADD,
  /* 4a */ VK_NUMPAD1 | KNUMS,
  /* 4b */ VK_NUMPAD2 | KNUMS,
  /* 4c */ VK_NUMPAD3 | KNUMS,
  /* 4d */ VK_OEM_NEC_EQUAL | KNUMS,
  /* 4e */ VK_NUMPAD0 | KNUMS,
  /* 4f */ VK_SEPARATOR,
  /* 50 */ VK_DECIMAL | KNUMS,
  /* 51 */ VK_NONCONVERT | KBDSPECIAL,
  /* 52 */ VK_F11,
  /* 53 */ VK_F12,
  /* 54 */ VK_F13,
  /* 55 */ VK_F14,
  /* 56 */ VK_F15,
  /* 57 */ VK_EMPTY,
  /* 58 */ VK_EMPTY,
  /* 59 */ VK_EMPTY,
  /* 5a */ VK_EMPTY,
  /* 5b */ VK_EMPTY,
  /* 5c */ VK_RETURN,
  /* 5d */ VK_EMPTY,
  /* 5e */ VK_EMPTY,
  /* 5f */ VK_EMPTY,
  /* 60 */ VK_CANCEL,
  /* 61 */ VK_SNAPSHOT,
  /* 62 */ VK_F1,
  /* 63 */ VK_F2,
  /* 64 */ VK_F3,
  /* 65 */ VK_F4,
  /* 66 */ VK_F5,
  /* 67 */ VK_F6,
  /* 68 */ VK_F7,
  /* 69 */ VK_F8,
  /* 6a */ VK_F9,
  /* 6b */ VK_F10,
  /* 6c */ VK_EMPTY,
  /* 6d */ VK_EMPTY,
  /* 6e */ VK_EMPTY,
  /* 6f */ VK_EMPTY,
  /* 70 */ VK_LSHIFT,
  /* 71 */ VK_CAPITAL,
  /* 72 */ VK_KANA,
  /* 73 */ VK_LMENU,
  /* 74 */ VK_LCONTROL,
  /* 75 */ VK_EMPTY,
  /* 76 */ VK_EMPTY,
  /* 77 */ VK_LWIN,
  /* 78 */ VK_RWIN,
  /* 79 */ VK_APPS,
  /* 7a */ VK_EMPTY,
  /* 7b */ VK_EMPTY,
  /* 7c */ VK_EMPTY,
  /* 7d */ VK_RSHIFT,
  /* 7e */ VK_ABNT_C2,
  /* 7f */ 0xEC,
};

/* No E0 / E1 prefixed scan codes on the PC-98 keyboard: empty tables */
ROSDATA VSC_VK extcode0_to_vk[] = {
  { 0, 0 },
};

ROSDATA VSC_VK extcode1_to_vk[] = {
  { 0, 0 },
};

ROSDATA VK_TO_BIT modifier_keys[] = {
  { VK_SHIFT, KBDSHIFT },
  { VK_CONTROL, KBDCTRL },
  { VK_MENU, KBDALT },
  { VK_KANA, KBDKANA },
  { 0, 0 }
};

ROSDATA MODIFIERS modifier_bits = {
  modifier_keys,
  11,
  {
    0, /* NONE */
    1, /* SHIFT */
    4, /* CTRL */
    6, /* SHIFT+CTRL */
    SHFT_INVALID, /* ALT */
    SHFT_INVALID, /* SHIFT+ALT */
    SHFT_INVALID, /* CTRL+ALT */
    SHFT_INVALID, /* SHIFT+CTRL+ALT */
    2, /* KANA */
    3, /* SHIFT+KANA */
    5, /* CTRL+KANA */
    7, /* SHIFT+CTRL+KANA */
  }
};

ROSDATA VK_TO_WCHARS6 key_to_chars_6mod[] = {
  /* Normal, Shift, Kana, Kana+Shift, Ctrl, Ctrl+Kana */
  { VK_BACK, 0, {0x0008, 0x0008, 0x0008, 0x0008, 0x007f, 0x007f} },
  { VK_CANCEL, 0, {0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003} },
  { VK_ESCAPE, 0, {0x001b, 0x001b, 0x001b, 0x001b, 0x001b, 0x001b} },
  { VK_OEM_3, KANALOK, {'@', '~', 0xff9e, 0xff9e, 0x0000, 0x0000} },
  { VK_OEM_4, KANALOK, {'[', '{', 0xff9f, 0xff62, 0x001b, 0x001b} },
  { VK_OEM_5, KANALOK, {'\\', '|', 0xff70, 0xff70, 0x001c, 0x001c} },
  { VK_OEM_6, KANALOK, {']', '}', 0xff91, 0xff63, 0x001d, 0x001d} },
  { VK_OEM_7, KANALOK, {'^', '`', 0xff8d, 0xff8d, 0x001e, 0x001e} },
  { VK_OEM_8, KANALOK, {WCH_NONE, '_', 0xff9b, 0xff9b, 0x001f, 0x001f} },
  { VK_RETURN, 0, {0x000d, 0x000d, 0x000d, 0x000d, 0x000a, 0x000a} },
  { VK_SPACE, 0, {0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020} },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS8 key_to_chars_8mod[] = {
  /* Normal, Shift, Kana, Kana+Shift, Ctrl, Ctrl+Kana, Ctrl+Shift, Ctrl+Shift+Kana */
  { '2', KANALOK, {'2', '"', 0xff8c, 0xff8c, WCH_NONE, WCH_NONE, 0x0000, 0x0000} },
  { '6', KANALOK, {'6', '&', 0xff75, 0xff6b, WCH_NONE, WCH_NONE, 0x001e, 0x001e} },
  { VK_OEM_MINUS, KANALOK, {'-', '=', 0xff8e, 0xff8e, WCH_NONE, WCH_NONE, 0x001f, 0x001f} },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] = {
  /* Normal, Shift, Kana, Kana+Shift */
  { '0', KANALOK, {'0', '0', 0xff9c, 0xff66} },
  { '1', KANALOK, {'1', '!', 0xff87, 0xff87} },
  { '3', KANALOK, {'3', '#', 0xff71, 0xff67} },
  { '4', KANALOK, {'4', '$', 0xff73, 0xff69} },
  { '5', KANALOK, {'5', '%', 0xff74, 0xff6a} },
  { '7', KANALOK, {'7', '\'', 0xff94, 0xff6c} },
  { '8', KANALOK, {'8', '(', 0xff95, 0xff6d} },
  { '9', KANALOK, {'9', ')', 0xff96, 0xff6e} },
  { 'A', CAPLOK|KANALOK, {'a', 'A', 0xff81, 0xff81} },
  { 'B', CAPLOK|KANALOK, {'b', 'B', 0xff7a, 0xff7a} },
  { 'C', CAPLOK|KANALOK, {'c', 'C', 0xff7f, 0xff7f} },
  { 'D', CAPLOK|KANALOK, {'d', 'D', 0xff7c, 0xff7c} },
  { 'E', CAPLOK|KANALOK, {'e', 'E', 0xff72, 0xff68} },
  { 'F', CAPLOK|KANALOK, {'f', 'F', 0xff8a, 0xff8a} },
  { 'G', CAPLOK|KANALOK, {'g', 'G', 0xff77, 0xff77} },
  { 'H', CAPLOK|KANALOK, {'h', 'H', 0xff78, 0xff78} },
  { 'I', CAPLOK|KANALOK, {'i', 'I', 0xff86, 0xff86} },
  { 'J', CAPLOK|KANALOK, {'j', 'J', 0xff8f, 0xff8f} },
  { 'K', CAPLOK|KANALOK, {'k', 'K', 0xff89, 0xff89} },
  { 'L', CAPLOK|KANALOK, {'l', 'L', 0xff98, 0xff98} },
  { 'M', CAPLOK|KANALOK, {'m', 'M', 0xff93, 0xff93} },
  { 'N', CAPLOK|KANALOK, {'n', 'N', 0xff90, 0xff90} },
  { 'O', CAPLOK|KANALOK, {'o', 'O', 0xff97, 0xff97} },
  { 'P', CAPLOK|KANALOK, {'p', 'P', 0xff7e, 0xff7e} },
  { 'Q', CAPLOK|KANALOK, {'q', 'Q', 0xff80, 0xff80} },
  { 'R', CAPLOK|KANALOK, {'r', 'R', 0xff7d, 0xff7d} },
  { 'S', CAPLOK|KANALOK, {'s', 'S', 0xff84, 0xff84} },
  { 'T', CAPLOK|KANALOK, {'t', 'T', 0xff76, 0xff76} },
  { 'U', CAPLOK|KANALOK, {'u', 'U', 0xff85, 0xff85} },
  { 'V', CAPLOK|KANALOK, {'v', 'V', 0xff8b, 0xff8b} },
  { 'W', CAPLOK|KANALOK, {'w', 'W', 0xff83, 0xff83} },
  { 'X', CAPLOK|KANALOK, {'x', 'X', 0xff7b, 0xff7b} },
  { 'Y', CAPLOK|KANALOK, {'y', 'Y', 0xff9d, 0xff9d} },
  { 'Z', CAPLOK|KANALOK, {'z', 'Z', 0xff82, 0xff6f} },
  { VK_OEM_1, KANALOK, {':', '*', 0xff79, 0xff79} },
  { VK_OEM_2, KANALOK, {'/', '?', 0xff92, 0xff65} },
  { VK_OEM_COMMA, KANALOK, {',', '<', 0xff88, 0xff64} },
  { VK_OEM_PERIOD, KANALOK, {'.', '>', 0xff99, 0xff61} },
  { VK_OEM_PLUS, KANALOK, {';', '+', 0xff9a, 0xff9a} },
  { VK_TAB, 0, {0x0009, 0x0009, 0x0009, 0x0009} },
  { VK_ADD, 0, {'+', '+', '+', '+'} },
  { VK_DECIMAL, 0, {'.', '.', '.', '.'} },
  { VK_DIVIDE, 0, {'/', '/', '/', '/'} },
  { VK_MULTIPLY, 0, {'*', '*', '*', '*'} },
  { VK_SUBTRACT, 0, {'-', '-', '-', '-'} },
  { VK_SEPARATOR, 0, {',', ',', ',', ','} },
  { VK_OEM_NEC_EQUAL, 0, {'=', '=', '=', '='} },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS4 keypad_numbers[] = {
  /* Normal, Shift, Kana, Kana+Shift */
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
  { 0x0e, L"BS" },
  { 0x0f, L"TAB" },
  { 0x1c, L"RETURN" },
  { 0x34, L"SPACE" },
  { 0x35, L"XFER" },
  { 0x36, L"ROLL UP" },
  { 0x37, L"ROLL DOWN" },
  { 0x38, L"INS" },
  { 0x39, L"DEL" },
  { 0x3a, L"UP" },
  { 0x3b, L"LEFT" },
  { 0x3c, L"RIGHT" },
  { 0x3d, L"DOWN" },
  { 0x3e, L"CLR" },
  { 0x3f, L"HELP" },
  { 0x40, L"Num -" },
  { 0x41, L"Num /" },
  { 0x42, L"Num 7" },
  { 0x43, L"Num 8" },
  { 0x44, L"Num 9" },
  { 0x45, L"Num *" },
  { 0x46, L"Num 4" },
  { 0x47, L"Num 5" },
  { 0x48, L"Num 6" },
  { 0x49, L"Num +" },
  { 0x4a, L"Num 1" },
  { 0x4b, L"Num 2" },
  { 0x4c, L"Num 3" },
  { 0x4d, L"Num =" },
  { 0x4e, L"Num 0" },
  { 0x4f, L"Num ," },
  { 0x50, L"Num ." },
  { 0x51, L"NFER" },
  { 0x52, L"f\uff6511" },
  { 0x53, L"f\uff6512" },
  { 0x54, L"f\uff6513" },
  { 0x55, L"f\uff6514" },
  { 0x56, L"f\uff6515" },
  { 0x60, L"STOP" },
  { 0x61, L"COPY" },
  { 0x62, L"f\uff651" },
  { 0x63, L"f\uff652" },
  { 0x64, L"f\uff653" },
  { 0x65, L"f\uff654" },
  { 0x66, L"f\uff655" },
  { 0x67, L"f\uff656" },
  { 0x68, L"f\uff657" },
  { 0x69, L"f\uff658" },
  { 0x6a, L"f\uff659" },
  { 0x6b, L"f\uff6510" },
  { 0x70, L"SHIFT" },
  { 0x71, L"CAPS" },
  { 0x72, L"\uff76\uff85" },
  { 0x73, L"GRPH" },
  { 0x74, L"CTRL" },
  { 0x77, L"Left Windows" },
  { 0x78, L"Right Windows" },
  { 0x79, L"Application" },
  { 0x7d, L"SHIFT" },
  { 0, NULL },
};

ROSDATA VSC_LPWSTR extended_key_names[] = {
  { 0, NULL },
};

/* Finally, the master table (KBDTABLES + FE trailer, cf. KBDTABLES_FE in kbd.h) */
ROSDATA KBDTABLES_FE keyboard_layout_table = {
 {
  &modifier_bits,                 /* modifier assignments */
  vk_to_wchar_master_table,       /* character from vk tables */
  NULL,                           /* no dead keys */
  (VSC_LPWSTR *)key_names,        /* key names */
  (VSC_LPWSTR *)extended_key_names,
  NULL,                           /* dead key names */
  scancode_to_vk,                 /* scan code to virtual key maps */
  RTL_NUMBER_OF(scancode_to_vk),  /* 0x80 */
  extcode0_to_vk,
  extcode1_to_vk,
  0,                              /* fLocaleFlags (binary has 0, i.e. version word 0) */
  0, 0,                           /* no ligatures */
  NULL
 },
  7,                              /* dwType    : Japanese keyboard */
  0x0d02                          /* dwSubType : OEM id 0x0d / layout 2 (NEC PC-98) */
};

PKBDTABLES WINAPI KbdLayerDescriptor(VOID)
{
  return &keyboard_layout_table.Base;
}

/* ------------------------------------------------------------------ */
/* NLS (Far-East) function tables                                     */

#define NLS(i,p) { (i), (p) }
#define NLS_NONE NLS(0,0)

ROSDATA VK_F vk_to_function_table[] = {
  { 0xF3 /*0xF3*/, 1, 1, 0,
    { NLS(8, 0), NLS(8, 0), NLS(8, 0), NLS(8, 0), NLS(8, 0), NLS(8, 0), NLS(3, 248), NLS(3, 248) },
    { NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0) } },
  { 0x1D /*VK_NONCONVERT*/, 1, 1, 0,
    { NLS(2, 0), NLS(2, 0), NLS(3, 240), NLS(8, 0), NLS(3, 241), NLS(3, 247), NLS(3, 251), NLS(3, 249) },
    { NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0) } },
  { 0x1C /*VK_CONVERT*/, 1, 1, 0,
    { NLS(2, 0), NLS(2, 0), NLS(3, 25), NLS(2, 0), NLS(9, 0), NLS(3, 248), NLS(3, 250), NLS(3, 242) },
    { NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0) } },
  { 0x92 /*VK_OEM_NEC_EQUAL*/, 1, 1, 0,
    { NLS(2, 0), NLS(2, 0), NLS(3, 145), NLS(2, 0), NLS(2, 0), NLS(2, 0), NLS(2, 0), NLS(2, 0) },
    { NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0) } },
  { 0x24 /*VK_HOME*/, 1, 1, 0,
    { NLS(12, 0), NLS(2, 0), NLS(12, 0), NLS(12, 0), NLS(12, 0), NLS(12, 0), NLS(12, 0), NLS(12, 0) },
    { NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0) } },
  { 0x23 /*VK_END*/, 1, 1, 0,
    { NLS(11, 0), NLS(11, 0), NLS(11, 0), NLS(11, 0), NLS(11, 0), NLS(11, 0), NLS(11, 0), NLS(11, 0) },
    { NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0), NLS(0, 0) } },
};

ROSDATA USHORT mouse_vkeys[] = {
  0x65, 0x69, 0x63, 0x61, 0x67, 0x64, 0x68, 0x66, 0x62, 0x60, 0x6e, 0x6a, 0x6b, 0x6d, 0x16f, 0x124
};

ROSDATA KBDNLSTABLES nls_layer = {
  0x0d,                          /* OEMIdentifier */
  2,                             /* LayoutInformation */
  RTL_NUMBER_OF(vk_to_function_table),
  vk_to_function_table,
  RTL_NUMBER_OF(mouse_vkeys),
  mouse_vkeys
};

PKBDNLSTABLES WINAPI KbdNlsLayerDescriptor(VOID)
{
  return &nls_layer;
}
