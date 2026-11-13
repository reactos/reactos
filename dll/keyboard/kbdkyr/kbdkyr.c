/*
 * PROJECT:     ReactOS Keyboard Layouts
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Kyrgyz (Cyrillic) keyboard layout
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winuser.h>
#include <ndk/kbd.h>

/* See also: https://kbdlayout.info/kbdkyr */

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
/* Modifiers: Shift / Ctrl / Alt  (AltGr = Ctrl+Alt)                  */
/*                                                                    */
/*   bits  : 0 none, 1 Shift, 2 Ctrl, 3 Shift+Ctrl, 4 Alt,           */
/*           5 Shift+Alt, 6 Ctrl+Alt (AltGr), 7 Shift+Ctrl+Alt       */
/*   column: 0 = Normal, 1 = Shift, 2 = Ctrl, 3 = Shift+Ctrl,        */
/*           4 = AltGr, 5 = Shift+AltGr                               */
/* ------------------------------------------------------------------ */
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
      0,            /* NONE              */
      1,            /* SHIFT             */
      2,            /* CTRL              */
      3,            /* SHIFT+CTRL        */
      SHFT_INVALID, /* ALT               */
      SHFT_INVALID, /* SHIFT+ALT         */
      4,            /* CTRL+ALT (AltGr)  */
      5             /* SHIFT+CTRL+ALT    */
  }
};

/* ------------------------------------------------------------------ */
/* VK -> WCHAR tables                                                 */
/* ------------------------------------------------------------------ */

/* Normal, Shift */
ROSDATA VK_TO_WCHARS2 key_to_chars_2mod[] = {
  { VK_OEM_3, CAPLOK, {0x0451 /* SMALL IO */, 0x0401 /* CAPITAL IO */} },
  { '1', 0, {'1', '!'} },
  { '3', 0, {'3', 0x2116 /* NUMERO SIGN */} },
  { '4', 0, {'4', ';'} },
  { '5', 0, {'5', '%'} },
  { '7', 0, {'7', '?'} },
  { '8', 0, {'8', '*'} },
  { '9', 0, {'9', '('} },
  { '0', 0, {'0', ')'} },
  { VK_OEM_PLUS, 0, {'=', '+'} },
  { 'Q', CAPLOK, {0x0439 /* SMALL SHORT I */, 0x0419 /* CAPITAL SHORT I */} },
  { 'W', CAPLOK, {0x0446 /* SMALL TSE */, 0x0426 /* CAPITAL TSE */} },
  { 'R', CAPLOK, {0x043A /* SMALL KA */, 0x041A /* CAPITAL KA */} },
  { 'T', CAPLOK, {0x0435 /* SMALL IE */, 0x0415 /* CAPITAL IE */} },
  { 'U', CAPLOK, {0x0433 /* SMALL GHE */, 0x0413 /* CAPITAL GHE */} },
  { 'I', CAPLOK, {0x0448 /* SMALL SHA */, 0x0428 /* CAPITAL SHA */} },
  { 'O', CAPLOK, {0x0449 /* SMALL SHCHA */, 0x0429 /* CAPITAL SHCHA */} },
  { 'P', CAPLOK, {0x0437 /* SMALL ZE */, 0x0417 /* CAPITAL ZE */} },
  { VK_OEM_4, CAPLOK, {0x0445 /* SMALL HA */, 0x0425 /* CAPITAL HA */} },
  { VK_OEM_6, CAPLOK, {0x044A /* SMALL HARD SIGN */, 0x042A /* CAPITAL HARD SIGN */} },
  { 'A', CAPLOK, {0x0444 /* SMALL EF */, 0x0424 /* CAPITAL EF */} },
  { 'S', CAPLOK, {0x044B /* SMALL YERU */, 0x042B /* CAPITAL YERU */} },
  { 'D', CAPLOK, {0x0432 /* SMALL VE */, 0x0412 /* CAPITAL VE */} },
  { 'F', CAPLOK, {0x0430 /* SMALL A */, 0x0410 /* CAPITAL A */} },
  { 'G', CAPLOK, {0x043F /* SMALL PE */, 0x041F /* CAPITAL PE */} },
  { 'H', CAPLOK, {0x0440 /* SMALL ER */, 0x0420 /* CAPITAL ER */} },
  { 'K', CAPLOK, {0x043B /* SMALL EL */, 0x041B /* CAPITAL EL */} },
  { 'L', CAPLOK, {0x0434 /* SMALL DE */, 0x0414 /* CAPITAL DE */} },
  { VK_OEM_1, CAPLOK, {0x0436 /* SMALL ZHE */, 0x0416 /* CAPITAL ZHE */} },
  { VK_OEM_7, CAPLOK, {0x044D /* SMALL E */, 0x042D /* CAPITAL E */} },
  { 'Z', CAPLOK, {0x044F /* SMALL YA */, 0x042F /* CAPITAL YA */} },
  { 'X', CAPLOK, {0x0447 /* SMALL CHE */, 0x0427 /* CAPITAL CHE */} },
  { 'C', CAPLOK, {0x0441 /* SMALL ES */, 0x0421 /* CAPITAL ES */} },
  { 'V', CAPLOK, {0x043C /* SMALL EM */, 0x041C /* CAPITAL EM */} },
  { 'B', CAPLOK, {0x0438 /* SMALL I */, 0x0418 /* CAPITAL I */} },
  { 'N', CAPLOK, {0x0442 /* SMALL TE */, 0x0422 /* CAPITAL TE */} },
  { 'M', CAPLOK, {0x044C /* SMALL SOFT SIGN */, 0x042C /* CAPITAL SOFT SIGN */} },
  { VK_OEM_COMMA, CAPLOK, {0x0431 /* SMALL BE */, 0x0411 /* CAPITAL BE */} },
  { VK_OEM_PERIOD, CAPLOK, {0x044E /* SMALL YU */, 0x042E /* CAPITAL YU */} },
  { VK_OEM_2, 0, {'.', ','} },
  { VK_DECIMAL, 0, {',', ','} },
  { VK_TAB, 0, {0x09, 0x09} },
  { VK_ADD, 0, {'+', '+'} },
  { VK_DIVIDE, 0, {'/', '/'} },
  { VK_MULTIPLY, 0, {'*', '*'} },
  { VK_SUBTRACT, 0, {'-', '-'} },
  { 0, 0 }
};

/* Normal, Shift, Ctrl */
ROSDATA VK_TO_WCHARS3 key_to_chars_3mod[] = {
  { VK_OEM_5, 0, {'\\', '/', 0x1c} },
  { VK_OEM_102, 0, {'\\', '/', 0x1c} },
  { VK_BACK, 0, {0x08, 0x08, 0x7f} },
  { VK_ESCAPE, 0, {0x1b, 0x1b, 0x1b} },
  { VK_RETURN, 0, {0x0d, 0x0d, 0x0a} },
  { VK_SPACE, 0, {' ', ' ', ' '} },
  { VK_CANCEL, 0, {0x03, 0x03, 0x03} },
  { 0, 0 }
};

/* Normal, Shift, Ctrl, Ctrl+Shift */
ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] = {
  { '2', 0, {'2', '\"', WCH_NONE, 0x00} },
  { '6', 0, {'6', ':', WCH_NONE, 0x1e} },
  { VK_OEM_MINUS, 0, {'-', '_', WCH_NONE, 0x1f} },
  { 0, 0 }
};

/* Normal, Shift, Ctrl, Ctrl+Shift, AltGr, Shift+AltGr */
ROSDATA VK_TO_WCHARS6 key_to_chars_6mod[] = {
  { 'E', CAPLOK, {0x0443 /* SMALL U */, 0x0423 /* CAPITAL U */, WCH_NONE, WCH_NONE, 0x04AF /* SMALL STRAIGHT U */, 0x04AE /* CAPITAL STRAIGHT U */} },
  { 'Y', CAPLOK, {0x043D /* SMALL EN */, 0x041D /* CAPITAL EN */, WCH_NONE, WCH_NONE, 0x04A3 /* SMALL EN WITH DESCENDER */, 0x04A2 /* CAPITAL EN WITH DESCENDER */} },
  { 'J', CAPLOK, {0x043E /* SMALL O */, 0x041E /* CAPITAL O */, WCH_NONE, WCH_NONE, 0x04E9 /* SMALL BARRED O */, 0x04E8 /* CAPITAL BARRED O */} },
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

/* Order in the binary: 3, 4, 6, 2, 1 */
ROSDATA VK_TO_WCHAR_TABLE vk_to_wchar_master_table[] = {
  vk_master(3, key_to_chars_3mod),
  vk_master(4, key_to_chars_4mod),
  vk_master(6, key_to_chars_6mod),
  vk_master(2, key_to_chars_2mod),
  vk_master(1, keypad_numbers),
  { 0, 0, 0 }
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
  MAKELONG(KLLF_ALTGR, KBD_VERSION),
  0,                                 /* nLgMax: no ligatures */
  0,                                 /* cbLgEntry */
  NULL                               /* no ligature table */
};

PKBDTABLES WINAPI KbdLayerDescriptor(VOID) {
  return &keyboard_layout_table;
}
