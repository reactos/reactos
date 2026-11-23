/*
 * PROJECT:     ReactOS Keyboard Layouts
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Greek Polytonic Keyboard Layout
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winuser.h>
#include <ndk/kbd.h>

/* See also: https://kbdlayout.info/kbdhept */

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

#define VK_EMPTY 0xff
#define KNUMS    (KBDNUMPAD | KBDSPECIAL)
#define KMEXT    (KBDEXT | KBDMULTIVK)

/* Dead key composition: ch = result, base = character typed after the dead key */
#ifndef DEADTRANS
  #define DEADTRANS(ch, base, accent, flags) { MAKELONG(base, accent), ch, flags }
#endif

/* ---- scancode -> VK (bMaxVSCtoVK = 0x7f) ---------------------------- */
ROSDATA USHORT scancode_to_vk[0x7f] = {
  /*00*/ VK_EMPTY, VK_ESCAPE, '1', '2',
  /*04*/ '3', '4', '5', '6',
  /*08*/ '7', '8', '9', '0',
  /*0c*/ VK_OEM_MINUS, VK_OEM_PLUS, VK_BACK, VK_TAB,
  /*10*/ 'Q', 'W', 'E', 'R',
  /*14*/ 'T', 'Y', 'U', 'I',
  /*18*/ 'O', 'P', VK_OEM_4, VK_OEM_6,
  /*1c*/ VK_RETURN, VK_LCONTROL, 'A', 'S',
  /*20*/ 'D', 'F', 'G', 'H',
  /*24*/ 'J', 'K', 'L', VK_OEM_1,
  /*28*/ VK_OEM_7, VK_OEM_3, VK_LSHIFT, VK_OEM_5,
  /*2c*/ 'Z', 'X', 'C', 'V',
  /*30*/ 'B', 'N', 'M', VK_OEM_COMMA,
  /*34*/ VK_OEM_PERIOD, VK_OEM_2, VK_RSHIFT | KBDEXT, VK_MULTIPLY | KBDMULTIVK,
  /*38*/ VK_LMENU, VK_SPACE, VK_CAPITAL, VK_F1,
  /*3c*/ VK_F2, VK_F3, VK_F4, VK_F5,
  /*40*/ VK_F6, VK_F7, VK_F8, VK_F9,
  /*44*/ VK_F10, VK_NUMLOCK | KMEXT, VK_SCROLL | KBDMULTIVK, VK_HOME | KNUMS,
  /*48*/ VK_UP | KNUMS, VK_PRIOR | KNUMS, VK_SUBTRACT, VK_LEFT | KNUMS,
  /*4c*/ VK_CLEAR | KNUMS, VK_RIGHT | KNUMS, VK_ADD, VK_END | KNUMS,
  /*50*/ VK_DOWN | KNUMS, VK_NEXT | KNUMS, VK_INSERT | KNUMS, VK_DELETE | KNUMS,
  /*54*/ VK_SNAPSHOT, VK_EMPTY, VK_OEM_102, VK_F11,
  /*58*/ VK_F12, VK_CLEAR, VK_OEM_WSCTRL, VK_OEM_FINISH,
  /*5c*/ VK_OEM_JUMP, VK_EREOF, VK_OEM_BACKTAB, VK_OEM_AUTO,
  /*60*/ VK_EMPTY, VK_EMPTY, VK_ZOOM, VK_HELP,
  /*64*/ VK_F13, VK_F14, VK_F15, VK_F16,
  /*68*/ VK_F17, VK_F18, VK_F19, VK_F20,
  /*6c*/ VK_F21, VK_F22, VK_F23, VK_OEM_PA3,
  /*70*/ VK_EMPTY, VK_OEM_RESET, VK_EMPTY, VK_ABNT_C1,
  /*74*/ VK_EMPTY, VK_EMPTY, VK_F24, VK_EMPTY,
  /*78*/ VK_EMPTY, VK_EMPTY, VK_EMPTY, VK_OEM_PA1,
  /*7c*/ VK_TAB, VK_EMPTY, VK_ABNT_C2
};

ROSDATA VSC_VK extcode0_to_vk[] = {
  { 0x10, VK_MEDIA_PREV_TRACK | KBDEXT },
  { 0x19, VK_MEDIA_NEXT_TRACK | KBDEXT },
  { 0x1d, VK_RCONTROL | KBDEXT },
  { 0x20, VK_VOLUME_MUTE | KBDEXT },
  { 0x21, VK_LAUNCH_APP2 | KBDEXT },
  { 0x22, VK_MEDIA_PLAY_PAUSE | KBDEXT },
  { 0x24, VK_MEDIA_STOP | KBDEXT },
  { 0x2e, VK_VOLUME_DOWN | KBDEXT },
  { 0x30, VK_VOLUME_UP | KBDEXT },
  { 0x32, VK_BROWSER_HOME | KBDEXT },
  { 0x35, VK_DIVIDE | KBDEXT },
  { 0x37, VK_SNAPSHOT | KBDEXT },
  { 0x38, VK_RMENU | KBDEXT },
  { 0x47, VK_HOME | KBDEXT },
  { 0x48, VK_UP | KBDEXT },
  { 0x49, VK_PRIOR | KBDEXT },
  { 0x4b, VK_LEFT | KBDEXT },
  { 0x4d, VK_RIGHT | KBDEXT },
  { 0x4f, VK_END | KBDEXT },
  { 0x50, VK_DOWN | KBDEXT },
  { 0x51, VK_NEXT | KBDEXT },
  { 0x52, VK_INSERT | KBDEXT },
  { 0x53, VK_DELETE | KBDEXT },
  { 0x5b, VK_LWIN | KBDEXT },
  { 0x5c, VK_RWIN | KBDEXT },
  { 0x5d, VK_APPS | KBDEXT },
  { 0x5f, VK_SLEEP | KBDEXT },
  { 0x65, VK_BROWSER_SEARCH | KBDEXT },
  { 0x66, VK_BROWSER_FAVORITES | KBDEXT },
  { 0x67, VK_BROWSER_REFRESH | KBDEXT },
  { 0x68, VK_BROWSER_STOP | KBDEXT },
  { 0x69, VK_BROWSER_FORWARD | KBDEXT },
  { 0x6a, VK_BROWSER_BACK | KBDEXT },
  { 0x6b, VK_LAUNCH_APP1 | KBDEXT },
  { 0x6c, VK_LAUNCH_MAIL | KBDEXT },
  { 0x6d, VK_LAUNCH_MEDIA_SELECT | KBDEXT },
  { 0x1c, VK_RETURN | KBDEXT },
  { 0x46, VK_CANCEL | KBDEXT },
  { 0, 0 }
};

ROSDATA VSC_VK extcode1_to_vk[] = {
  { 0x1d, VK_PAUSE },
  { 0, 0 }
};

/* ---- Modifiers -------------------------------------- */
ROSDATA VK_TO_BIT modifier_keys[] = {
  { VK_SHIFT, KBDSHIFT },
  { VK_CONTROL, KBDCTRL },
  { VK_MENU, KBDALT },
  { 0, 0 }
};

/*
 * Modifier bits -> column number (SHFT_INVALID = no such combination)
 * Columns: 0 = Base, 1 = Shift, 2 = AltGr (Ctrl+Alt), 3 = Shift+AltGr, 4 = Ctrl, 5 = Shift+Ctrl
 */
ROSDATA MODIFIERS modifier_bits = {
  modifier_keys,
  7,
  {
    0, /* none */
    1, /* Shift */
    4, /* Ctrl */
    5, /* Shift+Ctrl */
    SHFT_INVALID, /* Alt */
    SHFT_INVALID, /* Shift+Alt */
    2, /* Ctrl+Alt (AltGr) */
    3 /* Shift+Ctrl+Alt */
  }
};

/* ---- VK -> WCHAR ---------------------------------------------------- */
#define CAPAGR (CAPLOK | CAPLOKALTGR)

ROSDATA VK_TO_WCHARS2 key_to_chars_2mod[] = {
  { '7', 0, { '7', '&' } },
  { 'W', 0, { 0x3c2, WCH_DEAD } },
  { VK_EMPTY, 0, { WCH_NONE, 0x385 } },
  { 'T', CAPLOK, { 0x3c4, 0x3a4 } },
  { 'U', CAPLOK, { 0x3b8, 0x398 } },
  { 'I', CAPLOK, { 0x3b9, 0x399 } },
  { 'O', CAPLOK, { 0x3bf, 0x39f } },
  { 'P', CAPLOK, { 0x3c0, 0x3a0 } },
  { 'A', CAPLOK, { 0x3b1, 0x391 } },
  { 'S', CAPLOK, { 0x3c3, 0x3a3 } },
  { 'D', CAPLOK, { 0x3b4, 0x394 } },
  { 'F', CAPLOK, { 0x3c6, 0x3a6 } },
  { 'G', CAPLOK, { 0x3b3, 0x393 } },
  { 'H', CAPLOK, { 0x3b7, 0x397 } },
  { 'J', CAPLOK, { 0x3be, 0x39e } },
  { 'K', CAPLOK, { 0x3ba, 0x39a } },
  { 'L', CAPLOK, { 0x3bb, 0x39b } },
  { 'Z', CAPLOK, { 0x3b6, 0x396 } },
  { 'X', CAPLOK, { 0x3c7, 0x3a7 } },
  { 'V', CAPLOK, { 0x3c9, 0x3a9 } },
  { 'B', CAPLOK, { 0x3b2, 0x392 } },
  { 'N', CAPLOK, { 0x3bd, 0x39d } },
  { 'M', CAPLOK, { 0x3bc, 0x39c } },
  { VK_OEM_COMMA, 0, { ',', '<' } },
  { VK_OEM_PERIOD, 0, { '.', '>' } },
  { VK_DECIMAL, 0, { ',', ',' } },
  { VK_TAB, 0, { 0x9, 0x9 } },
  { VK_ADD, 0, { '+', '+' } },
  { VK_DIVIDE, 0, { '/', '/' } },
  { VK_MULTIPLY, 0, { '*', '*' } },
  { VK_SUBTRACT, 0, { '-', '-' } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS3 key_to_chars_3mod[] = {
  { '1', 0, { '1', '!', 0x3da } },
  { '4', 0, { '4', '$', 0xa3 } },
  { '5', 0, { '5', '%', 0xa7 } },
  { '8', 0, { '8', '*', 0xa4 } },
  { '9', 0, { '9', '(', 0xa6 } },
  { '0', 0, { '0', ')', 0xb0 } },
  { 'Q', 0, { WCH_DEAD, WCH_DEAD, WCH_DEAD } },
  { VK_EMPTY, 0, { ';', ':', 0x1ffd } },
  { 'E', CAPLOK, { 0x3b5, 0x395, 0x20ac } },
  { 'R', CAPLOK, { 0x3c1, 0x3a1, 0xae } },
  { 'Y', CAPLOK, { 0x3c5, 0x3a5, 0xa5 } },
  { VK_OEM_1, 0, { WCH_DEAD, WCH_DEAD, WCH_DEAD } },
  { VK_EMPTY, 0, { 0x384, 0xa8, 0x385 } },
  { 'C', CAPLOK, { 0x3c8, 0x3a8, 0xa9 } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] = {
  { '3', 0, { '3', '#', 0x3e0, 0xb3 } },
  { VK_OEM_PLUS, 0, { WCH_DEAD, WCH_DEAD, WCH_DEAD, WCH_DEAD } },
  { VK_EMPTY, 0, { '=', '+', 0xbd, 0x1fdf } },
  { VK_OEM_2, 0, { WCH_DEAD, WCH_DEAD, WCH_DEAD, WCH_DEAD } },
  { VK_EMPTY, 0, { '/', '?', 0x1fbe, 0x1fde } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS5 key_to_chars_5mod[] = {
  { VK_OEM_4, 0, { WCH_DEAD, WCH_DEAD, WCH_DEAD, WCH_NONE, 0x1b } },
  { VK_EMPTY, 0, { '[', '{', 0xab, WCH_NONE, WCH_NONE } },
  { VK_OEM_6, 0, { WCH_DEAD, WCH_DEAD, WCH_DEAD, 0x387, 0x1d } },
  { VK_EMPTY, 0, { ']', '}', 0xbb, WCH_NONE, WCH_NONE } },
  { VK_OEM_5, 0, { WCH_DEAD, WCH_DEAD, WCH_DEAD, WCH_DEAD, 0x1c } },
  { VK_EMPTY, 0, { '\\', '|', 0xac, 0x1fdd, WCH_NONE } },
  { VK_OEM_7, 0, { WCH_DEAD, WCH_DEAD, WCH_DEAD, WCH_DEAD, 0x1c } },
  { VK_EMPTY, 0, { '\'', '"', 0x1fbf, 0x1ffe, WCH_NONE } },
  { VK_OEM_102, 0, { '<', '>', WCH_NONE, WCH_NONE, 0x1c } },
  { VK_BACK, 0, { 0x8, 0x8, WCH_NONE, WCH_NONE, 0x7f } },
  { VK_ESCAPE, 0, { 0x1b, 0x1b, WCH_NONE, WCH_NONE, 0x1b } },
  { VK_RETURN, 0, { 0xd, 0xd, WCH_NONE, WCH_NONE, 0xa } },
  { VK_SPACE, 0, { ' ', ' ', WCH_NONE, WCH_NONE, ' ' } },
  { VK_CANCEL, 0, { 0x3, 0x3, WCH_NONE, WCH_NONE, 0x3 } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS6 key_to_chars_6mod[] = {
  { VK_OEM_3, 0, { WCH_DEAD, WCH_DEAD, WCH_DEAD, WCH_NONE, WCH_NONE, 0x0 } },
  { VK_EMPTY, 0, { '~', '`', 0x1fc1, WCH_NONE, WCH_NONE, WCH_NONE } },
  { '2', 0, { '2', '@', 0x3de, 0xb2, WCH_NONE, 0x0 } },
  { '6', 0, { '6', '^', 0xb6, WCH_NONE, WCH_NONE, 0x1e } },
  { VK_OEM_MINUS, 0, { WCH_DEAD, WCH_DEAD, 0xb1, WCH_NONE, WCH_NONE, 0x1f } },
  { VK_EMPTY, 0, { '-', '_', WCH_NONE, WCH_NONE, WCH_NONE, WCH_NONE } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS1 keypad_numbers[] = {
  { VK_NUMPAD0, 0, { '0' } },
  { VK_NUMPAD1, 0, { '1' } },
  { VK_NUMPAD2, 0, { '2' } },
  { VK_NUMPAD3, 0, { '3' } },
  { VK_NUMPAD4, 0, { '4' } },
  { VK_NUMPAD5, 0, { '5' } },
  { VK_NUMPAD6, 0, { '6' } },
  { VK_NUMPAD7, 0, { '7' } },
  { VK_NUMPAD8, 0, { '8' } },
  { VK_NUMPAD9, 0, { '9' } },
  { 0, 0 }
};

#define vk_master(n,x) { (PVK_TO_WCHARS1)x, n, sizeof(x[0]) }
ROSDATA VK_TO_WCHAR_TABLE vk_to_wchar_master_table[] = {
  vk_master(3, key_to_chars_3mod),
  vk_master(4, key_to_chars_4mod),
  vk_master(5, key_to_chars_5mod),
  vk_master(6, key_to_chars_6mod),
  vk_master(2, key_to_chars_2mod),
  vk_master(1, keypad_numbers),
  { 0, 0, 0 }
};


/* ---- Dead keys ------------------------------------------------------ */
ROSDATA DEADKEY dead_keys[] = {
  /* dialytika tonos */
  DEADTRANS(0x390, 0x3b9, 0x385, 0),
  DEADTRANS(0x3b0, 0x3c5, 0x385, 0),
  DEADTRANS(0x385, '.', 0x385, 0),
  DEADTRANS(0x385, ' ', 0x385, 0),

  /* tonos */
  DEADTRANS(0x3ac, 0x3b1, 0x384, 0),
  DEADTRANS(0x3ad, 0x3b5, 0x384, 0),
  DEADTRANS(0x3af, 0x3b9, 0x384, 0),
  DEADTRANS(0x3cc, 0x3bf, 0x384, 0),
  DEADTRANS(0x3cd, 0x3c5, 0x384, 0),
  DEADTRANS(0x3ce, 0x3c9, 0x384, 0),
  DEADTRANS(0x3ae, 0x3b7, 0x384, 0),
  DEADTRANS(0x386, 0x391, 0x384, 0),
  DEADTRANS(0x388, 0x395, 0x384, 0),
  DEADTRANS(0x38a, 0x399, 0x384, 0),
  DEADTRANS(0x38c, 0x39f, 0x384, 0),
  DEADTRANS(0x38e, 0x3a5, 0x384, 0),
  DEADTRANS(0x38f, 0x3a9, 0x384, 0),
  DEADTRANS(0x389, 0x397, 0x384, 0),
  DEADTRANS(0x384, '.', 0x384, 0),
  DEADTRANS(0x384, ' ', 0x384, 0),

  /* diaeresis */
  DEADTRANS(0x3ca, 0x3b9, 0xa8, 0),
  DEADTRANS(0x3cb, 0x3c5, 0xa8, 0),
  DEADTRANS(0x3aa, 0x399, 0xa8, 0),
  DEADTRANS(0x3ab, 0x3a5, 0xa8, 0),
  DEADTRANS(0xa8, '.', 0xa8, 0),
  DEADTRANS(0xa8, ' ', 0xa8, 0),

  /* U+003A */
  DEADTRANS(0x3ca, 0x3b9, ':', 0),
  DEADTRANS(0x3cb, 0x3c5, ':', 0),
  DEADTRANS(0x3aa, 0x399, ':', 0),
  DEADTRANS(0x3ab, 0x3a5, ':', 0),
  DEADTRANS(0xa8, '.', ':', 0),
  DEADTRANS(':', ' ', ':', 0),

  /* U+005D */
  DEADTRANS(0x1f70, 0x3b1, ']', 0),
  DEADTRANS(0x1f72, 0x3b5, ']', 0),
  DEADTRANS(0x1f74, 0x3b7, ']', 0),
  DEADTRANS(0x1f76, 0x3b9, ']', 0),
  DEADTRANS(0x1f78, 0x3bf, ']', 0),
  DEADTRANS(0x1f7a, 0x3c5, ']', 0),
  DEADTRANS(0x1f7c, 0x3c9, ']', 0),
  DEADTRANS(0x1fba, 0x391, ']', 0),
  DEADTRANS(0x1fc8, 0x395, ']', 0),
  DEADTRANS(0x1fca, 0x397, ']', 0),
  DEADTRANS(0x1fda, 0x399, ']', 0),
  DEADTRANS(0x1fea, 0x3a5, ']', 0),
  DEADTRANS(0x1ff8, 0x39f, ']', 0),
  DEADTRANS(0x1ffa, 0x3a9, ']', 0),
  DEADTRANS(0x1fef, '.', ']', 0),
  DEADTRANS(']', ' ', ']', 0),

  /* U+007D */
  DEADTRANS(0x387, '.', '}', 0),
  DEADTRANS('}', ' ', '}', 0),

  /* U+00BB */
  DEADTRANS(0x1fb2, 0x3b1, 0xbb, 0),
  DEADTRANS(0x1fc2, 0x3b7, 0xbb, 0),
  DEADTRANS(0x1ff2, 0x3c9, 0xbb, 0),
  DEADTRANS(0xbb, ' ', 0xbb, 0),

  /* U+003B */
  DEADTRANS(0x1f71, 0x3b1, ';', 0),
  DEADTRANS(0x1f73, 0x3b5, ';', 0),
  DEADTRANS(0x1f75, 0x3b7, ';', 0),
  DEADTRANS(0x1f77, 0x3b9, ';', 0),
  DEADTRANS(0x1f79, 0x3bf, ';', 0),
  DEADTRANS(0x1f7b, 0x3c5, ';', 0),
  DEADTRANS(0x1f7d, 0x3c9, ';', 0),
  DEADTRANS(0x1fbb, 0x391, ';', 0),
  DEADTRANS(0x1fc9, 0x395, ';', 0),
  DEADTRANS(0x1fcb, 0x397, ';', 0),
  DEADTRANS(0x1fdb, 0x399, ';', 0),
  DEADTRANS(0x1feb, 0x3a5, ';', 0),
  DEADTRANS(0x1ff9, 0x39f, ';', 0),
  DEADTRANS(0x1ffb, 0x3a9, ';', 0),
  DEADTRANS(0x1ffd, '.', ';', 0),
  DEADTRANS(';', ' ', ';', 0),

  /* U+1FFD */
  DEADTRANS(0x1fb4, 0x3b1, 0x1ffd, 0),
  DEADTRANS(0x1fc4, 0x3b7, 0x1ffd, 0),
  DEADTRANS(0x1ff4, 0x3c9, 0x1ffd, 0),

  /* U+002D */
  DEADTRANS(0x1fb1, 0x3b1, '-', 0),
  DEADTRANS(0x1fb9, 0x391, '-', 0),
  DEADTRANS(0x1fd1, 0x3b9, '-', 0),
  DEADTRANS(0x1fd9, 0x399, '-', 0),
  DEADTRANS(0x1fe1, 0x3c5, '-', 0),
  DEADTRANS(0x1fe9, 0x3a5, '-', 0),
  DEADTRANS(0xaf, '.', '-', 0),
  DEADTRANS('-', ' ', '-', 0),

  /* U+005F */
  DEADTRANS(0x1fb0, 0x3b1, '_', 0),
  DEADTRANS(0x1fb8, 0x391, '_', 0),
  DEADTRANS(0x1fd0, 0x3b9, '_', 0),
  DEADTRANS(0x1fd8, 0x399, '_', 0),
  DEADTRANS(0x1fe0, 0x3c5, '_', 0),
  DEADTRANS(0x1fe8, 0x3a5, '_', 0),
  DEADTRANS(0x2d8, '.', '_', 0),
  DEADTRANS('_', ' ', '_', 0),

  /* grave */
  DEADTRANS(0x1fd2, 0x3b9, '`', 0),
  DEADTRANS(0x1fe2, 0x3c5, '`', 0),
  DEADTRANS(0x1fed, '.', '`', 0),
  DEADTRANS('`', ' ', '`', 0),

  /* tilde */
  DEADTRANS(0x1fd3, 0x3b9, '~', 0),
  DEADTRANS(0x1fe3, 0x3c5, '~', 0),
  DEADTRANS(0x1fee, '.', '~', 0),
  DEADTRANS('~', ' ', '~', 0),

  /* U+1FC1 */
  DEADTRANS(0x1fd7, 0x3b9, 0x1fc1, 0),
  DEADTRANS(0x1fe7, 0x3c5, 0x1fc1, 0),
  DEADTRANS(0x1fc1, '.', 0x1fc1, 0),
  DEADTRANS(0x1fc1, ' ', 0x1fc1, 0),

  /* apostrophe */
  DEADTRANS(0x1f00, 0x3b1, '\'', 0),
  DEADTRANS(0x1f08, 0x391, '\'', 0),
  DEADTRANS(0x1f10, 0x3b5, '\'', 0),
  DEADTRANS(0x1f18, 0x395, '\'', 0),
  DEADTRANS(0x1f20, 0x3b7, '\'', 0),
  DEADTRANS(0x1f28, 0x397, '\'', 0),
  DEADTRANS(0x1f30, 0x3b9, '\'', 0),
  DEADTRANS(0x1f38, 0x399, '\'', 0),
  DEADTRANS(0x1f40, 0x3bf, '\'', 0),
  DEADTRANS(0x1f48, 0x39f, '\'', 0),
  DEADTRANS(0x1f50, 0x3c5, '\'', 0),
  DEADTRANS(0x1f60, 0x3c9, '\'', 0),
  DEADTRANS(0x1f68, 0x3a9, '\'', 0),
  DEADTRANS(0x1fe4, 0x3c1, '\'', 0),
  DEADTRANS(0x1fbf, '.', '\'', 0),
  DEADTRANS('\'', ' ', '\'', 0),

  /* U+005C */
  DEADTRANS(0x1f02, 0x3b1, '\\', 0),
  DEADTRANS(0x1f0a, 0x391, '\\', 0),
  DEADTRANS(0x1f12, 0x3b5, '\\', 0),
  DEADTRANS(0x1f1a, 0x395, '\\', 0),
  DEADTRANS(0x1f22, 0x3b7, '\\', 0),
  DEADTRANS(0x1f2a, 0x397, '\\', 0),
  DEADTRANS(0x1f32, 0x3b9, '\\', 0),
  DEADTRANS(0x1f3a, 0x399, '\\', 0),
  DEADTRANS(0x1f42, 0x3bf, '\\', 0),
  DEADTRANS(0x1f4a, 0x39f, '\\', 0),
  DEADTRANS(0x1f52, 0x3c5, '\\', 0),
  DEADTRANS(0x1f62, 0x3c9, '\\', 0),
  DEADTRANS(0x1f6a, 0x3a9, '\\', 0),
  DEADTRANS(0x1fcd, '.', '\\', 0),
  DEADTRANS('\\', ' ', '\\', 0),

  /* U+00AC */
  DEADTRANS(0x1f82, 0x3b1, 0xac, 0),
  DEADTRANS(0x1f8a, 0x391, 0xac, 0),
  DEADTRANS(0x1f92, 0x3b7, 0xac, 0),
  DEADTRANS(0x1f9a, 0x397, 0xac, 0),
  DEADTRANS(0x1fa2, 0x3c9, 0xac, 0),
  DEADTRANS(0x1faa, 0x3a9, 0xac, 0),
  DEADTRANS(0xac, ' ', 0xac, 0),

  /* U+002F */
  DEADTRANS(0x1f04, 0x3b1, '/', 0),
  DEADTRANS(0x1f0c, 0x391, '/', 0),
  DEADTRANS(0x1f14, 0x3b5, '/', 0),
  DEADTRANS(0x1f1c, 0x395, '/', 0),
  DEADTRANS(0x1f24, 0x3b7, '/', 0),
  DEADTRANS(0x1f2c, 0x397, '/', 0),
  DEADTRANS(0x1f34, 0x3b9, '/', 0),
  DEADTRANS(0x1f3c, 0x399, '/', 0),
  DEADTRANS(0x1f44, 0x3bf, '/', 0),
  DEADTRANS(0x1f4c, 0x39f, '/', 0),
  DEADTRANS(0x1f54, 0x3c5, '/', 0),
  DEADTRANS(0x1f64, 0x3c9, '/', 0),
  DEADTRANS(0x1f6c, 0x3a9, '/', 0),
  DEADTRANS(0x1fce, '.', '/', 0),
  DEADTRANS('/', ' ', '/', 0),

  /* U+1FBE */
  DEADTRANS(0x1f84, 0x3b1, 0x1fbe, 0),
  DEADTRANS(0x1f8c, 0x391, 0x1fbe, 0),
  DEADTRANS(0x1f94, 0x3b7, 0x1fbe, 0),
  DEADTRANS(0x1f9c, 0x397, 0x1fbe, 0),
  DEADTRANS(0x1fa4, 0x3c9, 0x1fbe, 0),
  DEADTRANS(0x1fac, 0x3a9, 0x1fbe, 0),

  /* U+003D */
  DEADTRANS(0x1f06, 0x3b1, '=', 0),
  DEADTRANS(0x1f0e, 0x391, '=', 0),
  DEADTRANS(0x1f26, 0x3b7, '=', 0),
  DEADTRANS(0x1f2e, 0x397, '=', 0),
  DEADTRANS(0x1f36, 0x3b9, '=', 0),
  DEADTRANS(0x1f3e, 0x399, '=', 0),
  DEADTRANS(0x1f56, 0x3c5, '=', 0),
  DEADTRANS(0x1f66, 0x3c9, '=', 0),
  DEADTRANS(0x1f6e, 0x3a9, '=', 0),
  DEADTRANS(0x1fcf, '.', '=', 0),
  DEADTRANS('=', ' ', '=', 0),

  /* U+00BD */
  DEADTRANS(0x1f86, 0x3b1, 0xbd, 0),
  DEADTRANS(0x1f8e, 0x391, 0xbd, 0),
  DEADTRANS(0x1f96, 0x3b7, 0xbd, 0),
  DEADTRANS(0x1f9e, 0x397, 0xbd, 0),
  DEADTRANS(0x1fa6, 0x3c9, 0xbd, 0),
  DEADTRANS(0x1fae, 0x3a9, 0xbd, 0),
  DEADTRANS(0xbd, ' ', 0xbd, 0),

  /* U+1FBF */
  DEADTRANS(0x1f80, 0x3b1, 0x1fbf, 0),
  DEADTRANS(0x1f88, 0x391, 0x1fbf, 0),
  DEADTRANS(0x1f90, 0x3b7, 0x1fbf, 0),
  DEADTRANS(0x1f98, 0x397, 0x1fbf, 0),
  DEADTRANS(0x1fa0, 0x3c9, 0x1fbf, 0),
  DEADTRANS(0x1fa8, 0x3a9, 0x1fbf, 0),

  /* U+0022 */
  DEADTRANS(0x1f01, 0x3b1, '"', 0),
  DEADTRANS(0x1f09, 0x391, '"', 0),
  DEADTRANS(0x1f11, 0x3b5, '"', 0),
  DEADTRANS(0x1f19, 0x395, '"', 0),
  DEADTRANS(0x1f21, 0x3b7, '"', 0),
  DEADTRANS(0x1f29, 0x397, '"', 0),
  DEADTRANS(0x1f31, 0x3b9, '"', 0),
  DEADTRANS(0x1f39, 0x399, '"', 0),
  DEADTRANS(0x1f41, 0x3bf, '"', 0),
  DEADTRANS(0x1f49, 0x39f, '"', 0),
  DEADTRANS(0x1f51, 0x3c5, '"', 0),
  DEADTRANS(0x1f59, 0x3a5, '"', 0),
  DEADTRANS(0x1f61, 0x3c9, '"', 0),
  DEADTRANS(0x1f69, 0x3a9, '"', 0),
  DEADTRANS(0x1fe5, 0x3c1, '"', 0),
  DEADTRANS(0x1fec, 0x3a1, '"', 0),
  DEADTRANS(0x1ffe, '.', '"', 0),
  DEADTRANS('"', ' ', '"', 0),

  /* U+007C */
  DEADTRANS(0x1f03, 0x3b1, '|', 0),
  DEADTRANS(0x1f0b, 0x391, '|', 0),
  DEADTRANS(0x1f13, 0x3b5, '|', 0),
  DEADTRANS(0x1f1b, 0x395, '|', 0),
  DEADTRANS(0x1f23, 0x3b7, '|', 0),
  DEADTRANS(0x1f2b, 0x397, '|', 0),
  DEADTRANS(0x1f33, 0x3b9, '|', 0),
  DEADTRANS(0x1f3b, 0x399, '|', 0),
  DEADTRANS(0x1f43, 0x3bf, '|', 0),
  DEADTRANS(0x1f4b, 0x39f, '|', 0),
  DEADTRANS(0x1f53, 0x3c5, '|', 0),
  DEADTRANS(0x1f5b, 0x3a5, '|', 0),
  DEADTRANS(0x1f63, 0x3c9, '|', 0),
  DEADTRANS(0x1f6b, 0x3a9, '|', 0),
  DEADTRANS(0x1fdd, '.', '|', 0),
  DEADTRANS('|', ' ', '|', 0),

  /* U+1FDD */
  DEADTRANS(0x1f83, 0x3b1, 0x1fdd, 0),
  DEADTRANS(0x1f8b, 0x391, 0x1fdd, 0),
  DEADTRANS(0x1f93, 0x3b7, 0x1fdd, 0),
  DEADTRANS(0x1f9b, 0x397, 0x1fdd, 0),
  DEADTRANS(0x1fa3, 0x3c9, 0x1fdd, 0),
  DEADTRANS(0x1fab, 0x3a9, 0x1fdd, 0),

  /* U+003F */
  DEADTRANS(0x1f05, 0x3b1, '?', 0),
  DEADTRANS(0x1f0d, 0x391, '?', 0),
  DEADTRANS(0x1f15, 0x3b5, '?', 0),
  DEADTRANS(0x1f1d, 0x395, '?', 0),
  DEADTRANS(0x1f25, 0x3b7, '?', 0),
  DEADTRANS(0x1f2d, 0x397, '?', 0),
  DEADTRANS(0x1f35, 0x3b9, '?', 0),
  DEADTRANS(0x1f3d, 0x399, '?', 0),
  DEADTRANS(0x1f45, 0x3bf, '?', 0),
  DEADTRANS(0x1f4d, 0x39f, '?', 0),
  DEADTRANS(0x1f55, 0x3c5, '?', 0),
  DEADTRANS(0x1f5d, 0x3a5, '?', 0),
  DEADTRANS(0x1f65, 0x3c9, '?', 0),
  DEADTRANS(0x1f6d, 0x3a9, '?', 0),
  DEADTRANS(0x1fde, '.', '?', 0),
  DEADTRANS('?', ' ', '?', 0),

  /* U+1FDE */
  DEADTRANS(0x1f85, 0x3b1, 0x1fde, 0),
  DEADTRANS(0x1f8d, 0x391, 0x1fde, 0),
  DEADTRANS(0x1f95, 0x3b7, 0x1fde, 0),
  DEADTRANS(0x1f9d, 0x397, 0x1fde, 0),
  DEADTRANS(0x1fa5, 0x3c9, 0x1fde, 0),
  DEADTRANS(0x1fad, 0x3a9, 0x1fde, 0),

  /* U+002B */
  DEADTRANS(0x1f07, 0x3b1, '+', 0),
  DEADTRANS(0x1f0f, 0x391, '+', 0),
  DEADTRANS(0x1f27, 0x3b7, '+', 0),
  DEADTRANS(0x1f2f, 0x397, '+', 0),
  DEADTRANS(0x1f37, 0x3b9, '+', 0),
  DEADTRANS(0x1f3f, 0x399, '+', 0),
  DEADTRANS(0x1f57, 0x3c5, '+', 0),
  DEADTRANS(0x1f5f, 0x3a5, '+', 0),
  DEADTRANS(0x1f67, 0x3c9, '+', 0),
  DEADTRANS(0x1f6f, 0x3a9, '+', 0),
  DEADTRANS(0x1fdf, '.', '+', 0),
  DEADTRANS('+', ' ', '+', 0),

  /* U+1FDF */
  DEADTRANS(0x1f87, 0x3b1, 0x1fdf, 0),
  DEADTRANS(0x1f8f, 0x391, 0x1fdf, 0),
  DEADTRANS(0x1f97, 0x3b7, 0x1fdf, 0),
  DEADTRANS(0x1f9f, 0x397, 0x1fdf, 0),
  DEADTRANS(0x1fa7, 0x3c9, 0x1fdf, 0),
  DEADTRANS(0x1faf, 0x3a9, 0x1fdf, 0),

  /* U+1FFE */
  DEADTRANS(0x1f81, 0x3b1, 0x1ffe, 0),
  DEADTRANS(0x1f89, 0x391, 0x1ffe, 0),
  DEADTRANS(0x1f91, 0x3b7, 0x1ffe, 0),
  DEADTRANS(0x1f99, 0x397, 0x1ffe, 0),
  DEADTRANS(0x1fa1, 0x3c9, 0x1ffe, 0),
  DEADTRANS(0x1fa9, 0x3a9, 0x1ffe, 0),

  /* U+005B */
  DEADTRANS(0x1fb6, 0x3b1, '[', 0),
  DEADTRANS(0x1fc6, 0x3b7, '[', 0),
  DEADTRANS(0x1fd6, 0x3b9, '[', 0),
  DEADTRANS(0x1fe6, 0x3c5, '[', 0),
  DEADTRANS(0x1ff6, 0x3c9, '[', 0),
  DEADTRANS(0x1fc0, '.', '[', 0),
  DEADTRANS('[', ' ', '[', 0),

  /* U+00AB */
  DEADTRANS(0x1fb7, 0x3b1, 0xab, 0),
  DEADTRANS(0x1fc7, 0x3b7, 0xab, 0),
  DEADTRANS(0x1ff7, 0x3c9, 0xab, 0),
  DEADTRANS(0xab, ' ', 0xab, 0),

  /* U+007B */
  DEADTRANS(0x1fb3, 0x3b1, '{', 0),
  DEADTRANS(0x1fbc, 0x391, '{', 0),
  DEADTRANS(0x1fc3, 0x3b7, '{', 0),
  DEADTRANS(0x1fcc, 0x397, '{', 0),
  DEADTRANS(0x1ff3, 0x3c9, '{', 0),
  DEADTRANS(0x1ffc, 0x3a9, '{', 0),
  DEADTRANS(0x1fbe, '.', '{', 0),
  DEADTRANS('{', ' ', '{', 0),
  { 0, 0, 0 }
};

ROSDATA DEADKEY_LPWSTR dead_key_names[] = {
  L"\x0384" L"TONOS",
  L"\x00a8" L"DIALYTIKA",
  L"\x0385" L"DIALYTIKA TONOS",
  NULL
};

/* ---- Key names --------------------------------------------------------- */
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
  { 0, NULL }
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
  { 0x54, L"<00>" },
  { 0x56, L"Help" },
  { 0x5b, L"Left Windows" },
  { 0x5c, L"Right Windows" },
  { 0x5d, L"Application" },
  { 0, NULL }
};

/* ---- Master table ------------------------------------------------ */
ROSDATA KBDTABLES keyboard_layout_table = {
  &modifier_bits,
  vk_to_wchar_master_table,
  dead_keys,
  key_names,
  extended_key_names,
  dead_key_names,
  scancode_to_vk,
  RTL_NUMBER_OF(scancode_to_vk),
  extcode0_to_vk,
  extcode1_to_vk,
  MAKELONG(KLLF_ALTGR, KBD_VERSION),
  0, 0, NULL                          /* No ligatures */
};

PKBDTABLES WINAPI KbdLayerDescriptor(VOID) {
  return &keyboard_layout_table;
}
