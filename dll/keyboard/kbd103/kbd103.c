/*
 * PROJECT:     ReactOS Keyboard Layouts
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Korean (Hangul) 103-key keyboard layout
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winuser.h>
#include <ndk/kbd.h>

/* See also: https://kbdlayout.info/kbd103 */

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

#define VK_EMPTY 0xff   /* The non-existent VK */

#define KNUMS  (KBDNUMPAD | KBDSPECIAL) /* Special + number pad */
#define KMEXT  (KBDEXT | KBDMULTIVK)    /* Multi + ext */

/* ------------------------------------------------------------------ */
/* Scan code -> virtual key (0x00 - 0x7f)                             */

ROSDATA USHORT scancode_to_vk[] = {
  /* 00 */ VK_EMPTY,
  /* 01 */ VK_ESCAPE,
  /* 02 - 0b : number row */
  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
  /* 0c */ VK_OEM_MINUS,
  /* 0d */ VK_OEM_PLUS | KBDSPECIAL,
  /* 0e */ VK_BACK,
  /* 0f */ VK_TAB,
  /* 10 - 19 : first letter row */
  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
  /* 1a */ VK_OEM_4,
  /* 1b */ VK_OEM_6,
  /* 1c */ VK_RETURN,
  /* 1d */ VK_LCONTROL,
  /* 1e - 26 : second letter row */
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
  /* 27 */ VK_OEM_1,
  /* 28 */ VK_OEM_7,
  /* 29 */ VK_OEM_3,
  /* 2a */ VK_LSHIFT,
  /* 2b */ VK_OEM_5,
  /* 2c - 32 : third letter row */
  'Z', 'X', 'C', 'V', 'B', 'N', 'M',
  /* 33 */ VK_OEM_COMMA,
  /* 34 */ VK_OEM_PERIOD,
  /* 35 */ VK_OEM_2,
  /* 36 */ VK_RSHIFT | KBDEXT,
  /* 37 */ VK_MULTIPLY | KBDMULTIVK,
  /* 38 */ VK_LMENU,
  /* 39 */ VK_SPACE,
  /* 3a */ VK_CAPITAL,
  /* 3b - 44 : F1 - F10 */
  VK_F1, VK_F2, VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9, VK_F10,
  /* 45 */ VK_NUMLOCK | KMEXT,
  /* 46 */ VK_SCROLL | KBDMULTIVK,
  /* 47 - 53 : number pad */
  VK_HOME | KNUMS,   VK_UP | KNUMS,    VK_PRIOR | KNUMS, VK_SUBTRACT,
  VK_LEFT | KNUMS,   VK_CLEAR | KNUMS, VK_RIGHT | KNUMS, VK_ADD,
  VK_END | KNUMS,    VK_DOWN | KNUMS,  VK_NEXT | KNUMS,
  VK_INSERT | KNUMS, VK_DELETE | KNUMS,
  /* 54 */ VK_SNAPSHOT,
  /* 55 */ VK_EMPTY,
  /* 56 */ VK_OEM_102,
  /* 57 */ VK_F11,
  /* 58 */ VK_F12,
  /* 59 */ VK_CLEAR,
  /* 5a */ 0xee,        /* VK_OEM_WSCTRL */
  /* 5b */ 0xf1,        /* VK_OEM_FINISH */
  /* 5c */ 0xea,        /* VK_OEM_JUMP */
  /* 5d */ 0xf9,        /* VK_EREOF */
  /* 5e */ 0xf5,        /* VK_OEM_BACKTAB */
  /* 5f */ 0xf3,        /* VK_OEM_AUTO */
  /* 60 */ VK_EMPTY,
  /* 61 */ 0xfb,        /* VK_ZOOM */
  /* 62 */ VK_HELP,
  /* 63 */ VK_EMPTY,
  /* 64 - 6e : F13 - F23 */
  VK_F13, VK_F14, VK_F15, VK_F16, VK_F17, VK_F18, VK_F19, VK_F20,
  VK_F21, VK_F22, VK_F23,
  /* 6f */ 0xed,        /* VK_OEM_PA3 */
  /* 70 */ VK_EMPTY,
  /* 71 */ 0xe9,        /* VK_OEM_RESET */
  /* 72 */ VK_EMPTY,
  /* 73 */ VK_ABNT_C1,
  /* 74 */ VK_EMPTY,
  /* 75 */ VK_EMPTY,
  /* 76 */ VK_F24,
  /* 77 - 7a */ VK_EMPTY, VK_EMPTY, VK_EMPTY, VK_EMPTY,
  /* 7b */ 0xeb,        /* VK_OEM_PA1 */
  /* 7c */ VK_TAB,
  /* 7d */ VK_EMPTY,
  /* 7e */ VK_ABNT_C2,
  /* 7f */ 0xec,        /* VK_OEM_PA2 */
};

/* ------------------------------------------------------------------ */
/* E0 prefixed scan codes                                             */

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
  /* Korean specific keys (Hanja / Hangeul) */
  { 0xF1, VK_HANJA | KBDEXT | KBDSPECIAL },
  { 0xF2, VK_HANGUL | KBDEXT | KBDSPECIAL },
  { 0, 0 },
};

/* E1 prefixed scan codes */
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

ROSDATA MODIFIERS modifier_bits = {
  modifier_keys,
  3,
  {
    0, // NONE
    1, // SHIFT
    2, // CTRL
    3, // SHIFT+CTRL
  }
};

/* ------------------------------------------------------------------ */
/* VK -> WCHAR tables                                                 */

/* Normal, Shifted, Ctrl */
ROSDATA VK_TO_WCHARS3 key_to_chars_3mod[] = {
  { VK_BACK,    0, {0x08, 0x08, 0x7F} },
  { VK_CANCEL,  0, {0x03, 0x03, 0x03} },
  { VK_ESCAPE,  0, {0x1b, 0x1b, 0x1b} },
  { VK_OEM_4,   0, {'[',  '{',  0x1b} },
  { VK_OEM_5,   0, {'\\', '|',  0x1c} },
  { VK_OEM_102, 0, {'\\', '|',  0x1c} },
  { VK_OEM_6,   0, {']',  '}',  0x1d} },
  { VK_RETURN,  0, {'\r', '\r', '\n'} },
  { VK_SPACE,   0, {' ',  ' ',  ' '} },
  { 0, 0 }
};

/* Normal, Shifted, Ctrl, Ctrl+Shift */
ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] = {
  { '2',          0, {'2', '@', WCH_NONE, 0x00} },
  { '6',          0, {'6', '^', WCH_NONE, 0x1e} },
  { VK_OEM_MINUS, 0, {'-', '_', WCH_NONE, 0x1f} },
  { 0, 0 }
};

/* Normal vs Shifted */
ROSDATA VK_TO_WCHARS2 key_to_chars_2mod[] = {
  /* The numbers (2 and 6 are in the 4-modifier table) */
  { '0',           0, {'0', ')'} },
  { '1',           0, {'1', '!'} },
  { '3',           0, {'3', '#'} },
  { '4',           0, {'4', '$'} },
  { '5',           0, {'5', '%'} },
  { '7',           0, {'7', '&'} },
  { '8',           0, {'8', '*'} },
  { '9',           0, {'9', '('} },
  /* The alphabet */
  { 'A',           CAPLOK, {'a', 'A'} },
  { 'B',           CAPLOK, {'b', 'B'} },
  { 'C',           CAPLOK, {'c', 'C'} },
  { 'D',           CAPLOK, {'d', 'D'} },
  { 'E',           CAPLOK, {'e', 'E'} },
  { 'F',           CAPLOK, {'f', 'F'} },
  { 'G',           CAPLOK, {'g', 'G'} },
  { 'H',           CAPLOK, {'h', 'H'} },
  { 'I',           CAPLOK, {'i', 'I'} },
  { 'J',           CAPLOK, {'j', 'J'} },
  { 'K',           CAPLOK, {'k', 'K'} },
  { 'L',           CAPLOK, {'l', 'L'} },
  { 'M',           CAPLOK, {'m', 'M'} },
  { 'N',           CAPLOK, {'n', 'N'} },
  { 'O',           CAPLOK, {'o', 'O'} },
  { 'P',           CAPLOK, {'p', 'P'} },
  { 'Q',           CAPLOK, {'q', 'Q'} },
  { 'R',           CAPLOK, {'r', 'R'} },
  { 'S',           CAPLOK, {'s', 'S'} },
  { 'T',           CAPLOK, {'t', 'T'} },
  { 'U',           CAPLOK, {'u', 'U'} },
  { 'V',           CAPLOK, {'v', 'V'} },
  { 'W',           CAPLOK, {'w', 'W'} },
  { 'X',           CAPLOK, {'x', 'X'} },
  { 'Y',           CAPLOK, {'y', 'Y'} },
  { 'Z',           CAPLOK, {'z', 'Z'} },
  /* Specials */
  { VK_OEM_1,      0, {';',  ':'} },
  { VK_OEM_2,      0, {'/',  '?'} },
  { VK_OEM_3,      0, {'`',  '~'} },
  { VK_OEM_7,      0, {'\'', '\"'} },
  { 0xdf /* VK_OEM_8 */, 0, {WCH_NONE, WCH_NONE} },
  { VK_OEM_COMMA,  0, {',',  '<'} },
  { VK_OEM_PERIOD, 0, {'.',  '>'} },
  { VK_OEM_PLUS,   0, {'=',  '+'} },
  /* Keys that do not have shift states */
  { VK_TAB,        0, {'\t', '\t'} },
  { VK_ADD,        0, {'+',  '+'} },
  { VK_DECIMAL,    0, {'.',  '.'} },
  { VK_DIVIDE,     0, {'/',  '/'} },
  { VK_MULTIPLY,   0, {'*',  '*'} },
  { VK_SUBTRACT,   0, {'-',  '-'} },
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
  vk_master(3, key_to_chars_3mod),
  vk_master(4, key_to_chars_4mod),
  vk_master(2, key_to_chars_2mod),
  vk_master(1, keypad_numbers),
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
  { 0x5b, L"Left Windows" },
  { 0x5c, L"Right Windows" },
  { 0x5d, L"Application" },
  { 0xf1, L"Hanja" },
  { 0xf2, L"Hangeul" },
  { 0, NULL },
};

/* ------------------------------------------------------------------ */
/* Far-East (NLS) tables                                              */

ROSDATA VK_F vk_to_function_table[] = {
  {
    0xBB,                  /* Vk (= VK_OEM_PLUS) */
    KBDNLS_TYPE_NORMAL,    /* NLSFEProcType */
    KBDNLS_INDEX_NORMAL,   /* NLSFEProcCurrent */
    0,                     /* NLSFEProcSwitch */
    {                      /* NLSFEProc[8] */
      { KBDNLS_SEND_BASE_VK,  0    },   /* none */
      { KBDNLS_SEND_BASE_VK,  0    },   /* shift */
      { KBDNLS_SEND_BASE_VK,  0    },   /* ctrl */
      { KBDNLS_SEND_BASE_VK,  0    },   /* shift+ctrl */
      { KBDNLS_SEND_PARAM_VK, 0x17 },   /* alt -> VK_JUNJA */
      { KBDNLS_SEND_BASE_VK,  0    },
      { KBDNLS_SEND_BASE_VK,  0    },
      { KBDNLS_SEND_BASE_VK,  0    },
    },
    { { 0 } }              /* NLSFEProcAlt[8]: all zero */
  },
};

ROSDATA KBDNLSTABLES nls_tables = {
  0,                    /* OEMIdentifier */
  0,                    /* LayoutInformation */
  1,                    /* NumOfVkToF */
  vk_to_function_table, /* pVkToF */
  0,                    /* NumOfMouseVKey */
  NULL                  /* pusMouseVKey */
};

/* ------------------------------------------------------------------ */
/* Finally, the master table                                          */

ROSDATA KBDTABLES_FE keyboard_layout_table = {
  {
    &modifier_bits,           /* modifier assignments */
    vk_to_wchar_master_table, /* character from vk tables */
    NULL,                     /* diacritical marks -- none */

    /* Key names */
    (VSC_LPWSTR *)key_names,
    (VSC_LPWSTR *)extended_key_names,
    NULL, /* Dead key names */

    /* scan code to virtual key maps */
    scancode_to_vk,
    RTL_NUMBER_OF(scancode_to_vk),
    extcode0_to_vk,
    extcode1_to_vk,

    0,    /* fLocaleFlags */

    /* Ligatures -- none */
    0,
    0,
    NULL
  },
  8, /* dwType    : Korean keyboard */
  6  /* dwSubType : 103 keys */
};

PKBDTABLES WINAPI KbdLayerDescriptor(VOID)
{
  return &keyboard_layout_table.Base;
}

PKBDNLSTABLES WINAPI KbdNlsLayerDescriptor(VOID)
{
  return &nls_tables;
}
