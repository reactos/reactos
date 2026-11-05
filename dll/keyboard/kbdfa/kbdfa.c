/*
 * PROJECT:     ReactOS Keyboard Layouts
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Persian keyboard layout
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winuser.h>
#include <ndk/kbd.h>

/* See also: https://kbdlayout.info/kbdfa */

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
  /* 0C */ VK_OEM_MINUS, VK_OEM_PLUS,  VK_BACK,      VK_TAB,
  /* 10 */ 'Q',          'W',          'E',          'R',
  /* 14 */ 'T',          'Y',          'U',          'I',
  /* 18 */ 'O',          'P',          VK_OEM_4,     VK_OEM_6,
  /* 1C */ VK_RETURN,    VK_LCONTROL,  'A',          'S',
  /* 20 */ 'D',          'F',          'G',          'H',
  /* 24 */ 'J',          'K',          'L',          VK_OEM_1,
  /* 28 */ VK_OEM_7,     VK_OEM_3,     VK_LSHIFT,    VK_OEM_5,
  /* 2C */ 'Z',          'X',          'C',          'V',
  /* 30 */ 'B',          'N',          'M',          VK_OEM_COMMA,
  /* 34 */ VK_OEM_PERIOD,VK_OEM_2,     VK_RSHIFT | KBDEXT,
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
/* Modifiers: Shift / Ctrl / Alt.  No AltGr -> max 3 states used      */
/* ------------------------------------------------------------------ */
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
      0,   /* NONE       */
      1,   /* SHIFT      */
      2,   /* CTRL       */
      3    /* SHIFT+CTRL */
  }
};

/* ------------------------------------------------------------------ */
/* VK -> WCHAR tables.  No key uses CAPLOK (Persian has no case).     */
/* ------------------------------------------------------------------ */

/* Normal, Shift */
ROSDATA VK_TO_WCHARS2 key_to_chars_2mod[] = {
  { VK_OEM_3,      0, {0x00F7, 0x00D7} },  /* DIVISION SIGN, MULTIPLICATION SIGN */
  { '5',           0, {'5', '%'} },
  { '7',           0, {'7', '&'} },
  { '8',           0, {'8', '*'} },
  { '9',           0, {'9', ')'} },
  { VK_OEM_PLUS,   0, {'=', '+'} },
  { 'Q',           0, {0x0636, 0x064B} },  /* DAD      , FATHATAN  */
  { 'W',           0, {0x0635, 0x064C} },  /* SAD      , DAMMATAN  */
  { 'E',           0, {0x062B, 0x064D} },  /* THEH     , KASRATAN  */
  { 'R',           0, {0x0642, WCH_LGTR} },/* QAF      , ligature "RIAL" (see below) */
  { 'T',           0, {0x0641, 0x060C} },  /* FEH      , ARABIC COMMA */
  { 'Y',           0, {0x063A, 0x061B} },  /* GHAIN    , ARABIC SEMICOLON */
  { 'U',           0, {0x0639, ','} },     /* AIN      */
  { 'I',           0, {0x0647, ']'} },     /* HEH      */
  { 'O',           0, {0x062E, '['} },     /* KHAH     */
  { 'P',           0, {0x062D, '\\'} },    /* HAH      */
  { 'A',           0, {0x0634, 0x064E} },  /* SHEEN    , FATHA     */
  { 'S',           0, {0x0633, 0x064F} },  /* SEEN     , DAMMA     */
  { 'D',           0, {0x06CC, 0x0650} },  /* FARSI YEH, KASRA     */
  { 'F',           0, {0x0628, 0x0651} },  /* BEH      , SHADDA    */
  { 'G',           0, {0x0644, 0x06C0} },  /* LAM      , HEH WITH YEH ABOVE */
  { 'H',           0, {0x0627, 0x0622} },  /* ALEF     , ALEF WITH MADDA */
  { 'J',           0, {0x062A, 0x0640} },  /* TEH      , TATWEEL   */
  { 'K',           0, {0x0646, 0x00AB} },  /* NOON     , LEFT GUILLEMET */
  { 'L',           0, {0x0645, 0x00BB} },  /* MEEM     , RIGHT GUILLEMET */
  { VK_OEM_1,      0, {0x06A9, ':'} },     /* KEHEH    */
  { VK_OEM_7,      0, {0x06AF, '\"'} },    /* GAF      */
  { 'Z',           0, {0x0638, 0x0629} },  /* ZAH      , TEH MARBUTA */
  { 'X',           0, {0x0637, 0x064A} },  /* TAH      , ARABIC YEH */
  { 'C',           0, {0x0632, 0x0698} },  /* ZAIN     , JEH       */
  { 'V',           0, {0x0631, 0x0624} },  /* REH      , WAW WITH HAMZA */
  { 'B',           0, {0x0630, 0x0625} },  /* THAL     , ALEF WITH HAMZA BELOW */
  { 'N',           0, {0x062F, 0x0623} },  /* DAL      , ALEF WITH HAMZA ABOVE */
  { 'M',           0, {0x0626, 0x0621} },  /* YEH WITH HAMZA, HAMZA */
  { VK_OEM_COMMA,  0, {0x0648, '<'} },     /* WAW      */
  { VK_OEM_PERIOD, 0, {'.', '>'} },
  { VK_OEM_2,      0, {'/', 0x061F} },     /* ARABIC QUESTION MARK */
  { VK_OEM_102,    0, {0x067E, '|'} },     /* PEH      */
  { VK_DECIMAL,    0, {'.', '.'} },
  { VK_TAB,        0, {'\t', '\t'} },
  { VK_ADD,        0, {'+', '+'} },
  { VK_DIVIDE,     0, {'/', '/'} },
  { VK_MULTIPLY,   0, {'*', '*'} },
  { VK_SUBTRACT,   0, {'-', '-'} },
  { 0, 0 }
};

/* Normal, Shift, Ctrl */
ROSDATA VK_TO_WCHARS3 key_to_chars_3mod[] = {
  { '0',        0, {'0', '(', 0x06C1 /* HEH GOAL */} },
  { VK_OEM_4,   0, {0x062C /* JEEM */,  '}', 0x1b /* ESC */} },
  { VK_OEM_6,   0, {0x0686 /* TCHEH */, '{', 0x1d /* GS  */} },
  { VK_OEM_5,   0, {0x067E /* PEH */,   '|', 0x1c /* FS  */} },
  { VK_BACK,    0, {0x08, 0x08, 0x7F} },
  { VK_ESCAPE,  0, {0x1b, 0x1b, 0x1b} },
  { VK_RETURN,  0, {'\r', '\r', '\n'} },
  { VK_SPACE,   0, {' ', ' ', ' '} },
  { VK_CANCEL,  0, {0x03, 0x03, 0x03} },
  { 0, 0 }
};

/* Normal, Shift, Ctrl, Ctrl+Shift */
ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] = {
  { '1',          0, {'1', '!', WCH_NONE, 0x200D /* ZWJ  */} },
  { '2',          0, {'2', '@', WCH_NONE, 0x200C /* ZWNJ */} },
  { '3',          0, {'3', '#', WCH_NONE, 0x200E /* LRM  */} },
  { '4',          0, {'4', '$', WCH_NONE, 0x200F /* RLM  */} },
  { '6',          0, {'6', '^', WCH_NONE, 0x1e /* RS */} },
  { VK_OEM_MINUS, 0, {'-', '_', WCH_NONE, 0x1f /* US */} },
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

/* Order in the binary: 3, 4, 2, 1 */
ROSDATA VK_TO_WCHAR_TABLE vk_to_wchar_master_table[] = {
  vk_master(3, key_to_chars_3mod),
  vk_master(4, key_to_chars_4mod),
  vk_master(2, key_to_chars_2mod),
  vk_master(1, keypad_numbers),
  { 0, 0, 0 }
};

/* ------------------------------------------------------------------ */
/* Ligature: Shift+R (WCH_LGTR) -> U+0631 U+064A U+0627 U+0644 ("ريال") */
/* ------------------------------------------------------------------ */
ROSDATA LIGATURE4 ligatures[] = {
  { 'R', 1 /* SHIFT */, {0x0631, 0x064A, 0x0627, 0x0644} },
  { 0, 0 }
};

/* ------------------------------------------------------------------ */
/* Key names                                                          */
/* ------------------------------------------------------------------ */
ROSDATA VSC_LPWSTR key_names[] = {
  { 0x01, L"Esc" },        { 0x0e, L"Backspace" },  { 0x0f, L"Tab" },
  { 0x1c, L"Enter" },      { 0x1d, L"Ctrl" },       { 0x2a, L"Shift" },
  { 0x36, L"Right Shift" },{ 0x37, L"Num *" },      { 0x38, L"Alt" },
  { 0x39, L"Space" },      { 0x3a, L"Caps Lock" },
  { 0x3b, L"F1" },  { 0x3c, L"F2" },  { 0x3d, L"F3" },  { 0x3e, L"F4" },
  { 0x3f, L"F5" },  { 0x40, L"F6" },  { 0x41, L"F7" },  { 0x42, L"F8" },
  { 0x43, L"F9" },  { 0x44, L"F10" },
  { 0x45, L"Pause" },      { 0x46, L"Scroll Lock" },
  { 0x47, L"Num 7" },  { 0x48, L"Num 8" },  { 0x49, L"Num 9" },  { 0x4a, L"Num -" },
  { 0x4b, L"Num 4" },  { 0x4c, L"Num 5" },  { 0x4d, L"Num 6" },  { 0x4e, L"Num +" },
  { 0x4f, L"Num 1" },  { 0x50, L"Num 2" },  { 0x51, L"Num 3" },
  { 0x52, L"Num 0" },  { 0x53, L"Num Del" },{ 0x54, L"Sys Req" },
  { 0x57, L"F11" },  { 0x58, L"F12" },
  { 0x7c, L"F13" },  { 0x7d, L"F14" },  { 0x7e, L"F15" },  { 0x7f, L"F16" },
  { 0x80, L"F17" },  { 0x81, L"F18" },  { 0x82, L"F19" },  { 0x83, L"F20" },
  { 0x84, L"F21" },  { 0x85, L"F22" },  { 0x86, L"F23" },  { 0x87, L"F24" },
  { 0, NULL }
};

ROSDATA VSC_LPWSTR extended_key_names[] = {
  { 0x1c, L"Num Enter" },  { 0x1d, L"Right Control" },
  { 0x35, L"Num /" },      { 0x37, L"Prnt Scrn" },   { 0x38, L"Right Alt" },
  { 0x45, L"Num Lock" },   { 0x46, L"Break" },
  { 0x47, L"Home" },       { 0x48, L"Up" },          { 0x49, L"Page Up" },
  { 0x4b, L"Left" },       { 0x4d, L"Right" },
  { 0x4f, L"End" },        { 0x50, L"Down" },        { 0x51, L"Page Down" },
  { 0x52, L"Insert" },     { 0x53, L"Delete" },
  { 0x54, L"<00>" },       { 0x56, L"Help" },
  { 0x5b, L"Left Windows" },{ 0x5c, L"Right Windows" },{ 0x5d, L"Application" },
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
  RTL_NUMBER_OF(scancode_to_vk),     /* 0x7f */
  extcode0_to_vk,
  extcode1_to_vk,
  MAKELONG(0, 1),                    /* fLocaleFlags = 0x00010000 (v1.0, no AltGr) */
  4,                                 /* nLgMax: up to 4 WCHARs per ligature */
  sizeof(LIGATURE4),                 /* cbLgEntry = 12 */
  (PLIGATURE1)ligatures
};

PKBDTABLES WINAPI KbdLayerDescriptor(VOID) {
  return &keyboard_layout_table;
}
