/*
 * PROJECT:     ReactOS Keyboard Layouts
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Canadian Multilingual Keyboard Layout
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winuser.h>
#include <ndk/kbd.h>

/* See also: https://kbdlayout.info/kbdca */

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
  /*28*/ VK_OEM_3, VK_OEM_7, VK_LSHIFT, VK_OEM_5,
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
 * Columns: 0 = Base, 1 = Shift, 2 = AltGr (Ctrl+Alt), 3 = Ctrl, 4 = Shift+Ctrl
 */
ROSDATA MODIFIERS modifier_bits = {
  modifier_keys,
  6,
  {
    0, /* none */
    1, /* Shift */
    3, /* Ctrl */
    4, /* Shift+Ctrl */
    SHFT_INVALID, /* Alt */
    SHFT_INVALID, /* Shift+Alt */
    2 /* Ctrl+Alt (AltGr) */
  }
};

/* ---- VK -> WCHAR ---------------------------------------------------- */
ROSDATA VK_TO_WCHARS2 key_to_chars_2mod[] = {
  { 'Q', CAPLOK, { 'q', 'Q' } },
  { 'W', CAPLOK, { 'w', 'W' } },
  { 'E', CAPLOK, { 'e', 'E' } },
  { 'R', CAPLOK, { 'r', 'R' } },
  { 'T', CAPLOK, { 't', 'T' } },
  { 'Y', CAPLOK, { 'y', 'Y' } },
  { 'U', CAPLOK, { 'u', 'U' } },
  { 'I', CAPLOK, { 'i', 'I' } },
  { 'A', CAPLOK, { 'a', 'A' } },
  { 'S', CAPLOK, { 's', 'S' } },
  { 'D', CAPLOK, { 'd', 'D' } },
  { 'F', CAPLOK, { 'f', 'F' } },
  { 'G', CAPLOK, { 'g', 'G' } },
  { 'H', CAPLOK, { 'h', 'H' } },
  { 'J', CAPLOK, { 'j', 'J' } },
  { 'K', CAPLOK, { 'k', 'K' } },
  { 'L', CAPLOK, { 'l', 'L' } },
  { 'Z', CAPLOK, { 'z', 'Z' } },
  { 'X', CAPLOK, { 'x', 'X' } },
  { 'C', CAPLOK, { 'c', 'C' } },
  { 'V', CAPLOK, { 'v', 'V' } },
  { 'B', CAPLOK, { 'b', 'B' } },
  { 'N', CAPLOK, { 'n', 'N' } },
  { VK_DECIMAL, 0, { '.', '.' } },
  { VK_TAB, 0, { 0x9, 0x9 } },
  { VK_ADD, 0, { '+', '+' } },
  { VK_DIVIDE, 0, { '/', '/' } },
  { VK_MULTIPLY, 0, { '*', '*' } },
  { VK_SUBTRACT, 0, { '-', '-' } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS3 key_to_chars_3mod[] = {
  { VK_OEM_7, 0, { '#', '|', '\\' } },
  { '1', 0, { '1', '!', 0xb1 } },
  { '2', 0, { '2', '"', '@' } },
  { '3', 0, { '3', '/', 0xa3 } },
  { '4', 0, { '4', '$', 0xa2 } },
  { '5', 0, { '5', '%', 0xa4 } },
  { '7', 0, { '7', '&', 0xa6 } },
  { '8', 0, { '8', '*', 0xb2 } },
  { '9', 0, { '9', '(', 0xb3 } },
  { '0', 0, { '0', ')', 0xbc } },
  { VK_OEM_PLUS, 0, { '=', '+', 0xbe } },
  { 'O', CAPLOK, { 'o', 'O', 0xa7 } },
  { 'P', CAPLOK, { 'p', 'P', 0xb6 } },
  { VK_OEM_1, 0, { ';', ':', '~' } },
  { VK_OEM_3, 0, { WCH_DEAD, WCH_DEAD, '{' } },
  { VK_EMPTY, 0, { '`', '`', WCH_NONE } },
  { 'M', CAPLOK, { 'm', 'M', 0xb5 } },
  { VK_OEM_COMMA, 0, { ',', '\'', 0xaf } },
  { VK_OEM_PERIOD, 0, { '.', '.', 0xad } },
  { VK_OEM_2, CAPLOK, { 0xe9, 0xc9, WCH_DEAD } },
  { VK_EMPTY, 0, { WCH_NONE, WCH_NONE, 0xb4 } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] = {
  { VK_OEM_MINUS, 0, { '-', '_', 0xbd, 0x1f } },
  { VK_OEM_4, 0, { WCH_DEAD, WCH_DEAD, '[', 0x1b } },
  { VK_EMPTY, 0, { '^', '^', WCH_NONE, WCH_NONE } },
  { VK_OEM_6, 0, { WCH_DEAD, WCH_DEAD, ']', 0x1d } },
  { VK_EMPTY, 0, { 0xb8, 0xa8, WCH_NONE, WCH_NONE } },
  { VK_OEM_5, 0, { '<', '>', '}', 0x1c } },
  { VK_OEM_102, 0, { 0xab, 0xbb, 0xb0, 0x1c } },
  { VK_BACK, 0, { 0x8, 0x8, WCH_NONE, 0x7f } },
  { VK_ESCAPE, 0, { 0x1b, 0x1b, WCH_NONE, 0x1b } },
  { VK_RETURN, 0, { 0xd, 0xd, WCH_NONE, 0xa } },
  { VK_SPACE, 0, { ' ', ' ', WCH_NONE, ' ' } },
  { VK_CANCEL, 0, { 0x3, 0x3, WCH_NONE, 0x3 } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS5 key_to_chars_5mod[] = {
  { '6', 0, { '6', '?', 0xac, WCH_NONE, 0x1e } },
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
  vk_master(2, key_to_chars_2mod),
  vk_master(1, keypad_numbers),
  { 0, 0, 0 }
};


/* ---- Dead keys ------------------------------------------------------ */
ROSDATA DEADKEY dead_keys[] = {
  /* acute */
  DEADTRANS(0xe1, 'a', 0xb4, 0),
  DEADTRANS(0xe9, 'e', 0xb4, 0),
  DEADTRANS(0xed, 'i', 0xb4, 0),
  DEADTRANS(0xf3, 'o', 0xb4, 0),
  DEADTRANS(0xfa, 'u', 0xb4, 0),
  DEADTRANS(0xfd, 'y', 0xb4, 0),
  DEADTRANS(0xc1, 'A', 0xb4, 0),
  DEADTRANS(0xc9, 'E', 0xb4, 0),
  DEADTRANS(0xcd, 'I', 0xb4, 0),
  DEADTRANS(0xd3, 'O', 0xb4, 0),
  DEADTRANS(0xda, 'U', 0xb4, 0),
  DEADTRANS(0xdd, 'Y', 0xb4, 0),
  DEADTRANS(0xb4, ' ', 0xb4, 0),

  /* grave */
  DEADTRANS(0xe0, 'a', '`', 0),
  DEADTRANS(0xe8, 'e', '`', 0),
  DEADTRANS(0xec, 'i', '`', 0),
  DEADTRANS(0xf2, 'o', '`', 0),
  DEADTRANS(0xf9, 'u', '`', 0),
  DEADTRANS(0xc0, 'A', '`', 0),
  DEADTRANS(0xc8, 'E', '`', 0),
  DEADTRANS(0xcc, 'I', '`', 0),
  DEADTRANS(0xd2, 'O', '`', 0),
  DEADTRANS(0xd9, 'U', '`', 0),
  DEADTRANS('`', ' ', '`', 0),

  /* circumflex */
  DEADTRANS(0xe2, 'a', '^', 0),
  DEADTRANS(0xea, 'e', '^', 0),
  DEADTRANS(0xee, 'i', '^', 0),
  DEADTRANS(0xf4, 'o', '^', 0),
  DEADTRANS(0xfb, 'u', '^', 0),
  DEADTRANS(0xc2, 'A', '^', 0),
  DEADTRANS(0xca, 'E', '^', 0),
  DEADTRANS(0xce, 'I', '^', 0),
  DEADTRANS(0xd4, 'O', '^', 0),
  DEADTRANS(0xdb, 'U', '^', 0),
  DEADTRANS('^', ' ', '^', 0),

  /* cedilla */
  DEADTRANS(0xe7, 'c', 0xb8, 0),
  DEADTRANS(0xc7, 'C', 0xb8, 0),
  DEADTRANS(0xb8, ' ', 0xb8, 0),

  /* diaeresis */
  DEADTRANS(0xe4, 'a', 0xa8, 0),
  DEADTRANS(0xeb, 'e', 0xa8, 0),
  DEADTRANS(0xef, 'i', 0xa8, 0),
  DEADTRANS(0xf6, 'o', 0xa8, 0),
  DEADTRANS(0xfc, 'u', 0xa8, 0),
  DEADTRANS(0xff, 'y', 0xa8, 0),
  DEADTRANS(0xc4, 'A', 0xa8, 0),
  DEADTRANS(0xcb, 'E', 0xa8, 0),
  DEADTRANS(0xcf, 'I', 0xa8, 0),
  DEADTRANS(0xd6, 'O', 0xa8, 0),
  DEADTRANS(0xdc, 'U', 0xa8, 0),
  DEADTRANS(0xa8, ' ', 0xa8, 0),
  { 0, 0, 0 }
};

ROSDATA DEADKEY_LPWSTR dead_key_names[] = {
  L"\x00b4" L"ACCENT AIGU",
  L"\x0060" L"ACCENT GRAVE",
  L"\x005e" L"ACCENT CIRCONFLEXE",
  L"\x00a8" L"TREMA",
  L"\x00b8" L"CEDILLE",
  NULL
};

/* ---- Key names --------------------------------------------------------- */
ROSDATA VSC_LPWSTR key_names[] = {
  { 0x01, L"ECHAP" },
  { 0x0e, L"RET.ARR" },
  { 0x0f, L"TAB" },
  { 0x1c, L"ENTREE" },
  { 0x1d, L"CTRL" },
  { 0x2a, L"MAJ" },
  { 0x36, L"MAJ DROITE" },
  { 0x37, L"* (PAVE NUM.)" },
  { 0x38, L"ALT" },
  { 0x39, L"ESPACE" },
  { 0x3a, L"VERR.MAJ" },
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
  { 0x46, L"DEFIL" },
  { 0x47, L"7 (PAVE NUM.)" },
  { 0x48, L"8 (PAVE NUM.)" },
  { 0x49, L"9 (PAVE NUM.)" },
  { 0x4a, L"- (PAVE NUM.)" },
  { 0x4b, L"4 (PAVE NUM.)" },
  { 0x4c, L"5 (PAVE NUM.)" },
  { 0x4d, L"6 (PAVE NUM.)" },
  { 0x4e, L"+ (PAVE NUM.)" },
  { 0x4f, L"1 (PAVE NUM.)" },
  { 0x50, L"2 (PAVE NUM.)" },
  { 0x51, L"3 (PAVE NUM.)" },
  { 0x52, L"0 (PAVE NUM.)" },
  { 0x53, L". (PAVE NUM.)" },
  { 0x57, L"F11" },
  { 0x58, L"F12" },
  { 0, NULL }
};

ROSDATA VSC_LPWSTR extended_key_names[] = {
  { 0x1c, L"ENTREE (PAVE NUM.)" },
  { 0x1d, L"CTRL DROITE" },
  { 0x35, L"/ (PAVE NUM.)" },
  { 0x37, L"Impr.Ecran" },
  { 0x38, L"ALT DROITE" },
  { 0x45, L"Ver.Num" },
  { 0x46, L"ATTN" },
  { 0x47, L"ORIGINE" },
  { 0x48, L"HAUT" },
  { 0x49, L"PG.PREC" },
  { 0x4b, L"GAUCHE" },
  { 0x4d, L"DROITE" },
  { 0x4f, L"FIN" },
  { 0x50, L"BAS" },
  { 0x51, L"PG.SUIV" },
  { 0x52, L"INS" },
  { 0x53, L"SUPPR" },
  { 0x54, L"<00>" },
  { 0x56, L"AIDE" },
  { 0x5b, L"WINDOWS GAUCHE" },
  { 0x5c, L"WINDOWS DROITE" },
  { 0x5d, L"APPLICATION" },
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
  0x7f,                                   /* bMaxVSCtoVK */
  extcode0_to_vk,
  extcode1_to_vk,
  MAKELONG(KLLF_ALTGR, KBD_VERSION),      /* fLocaleFlags = 0x00010001 */
  0, 0, NULL                              /* No ligatures */
};

PKBDTABLES WINAPI KbdLayerDescriptor(VOID) {
  return &keyboard_layout_table;
}
