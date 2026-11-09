/*
 * PROJECT:     ReactOS Keyboard Layouts
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Latvia-QWERTY Keyboard Layout
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winuser.h>
#include <ndk/kbd.h>

/* See also: https://kbdlayout.info/kbdlv1 */

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

/* Modifier bits -> column number (SHFT_INVALID = no such combination) */
ROSDATA MODIFIERS modifier_bits = {
  modifier_keys,
  7,
  {
    0, /* 0000: none */
    1, /* 0001: Shift */
    4, /* 0010: Ctrl */
    5, /* 0011: Shift+Ctrl */
    SHFT_INVALID, /* 0100: Alt */
    SHFT_INVALID, /* 0101: Shift+Alt */
    2, /* 0110: Ctrl+Alt */
    3  /* 0111: Shift+Ctrl+Alt */
  }
};

/* ---- VK -> WCHAR ---------------------------------------------------- */
ROSDATA VK_TO_WCHARS3 key_to_chars_3mod[] = {
  { VK_OEM_3, 0, { '`', WCH_DEAD, 0xad } },
  { VK_EMPTY, 0, { WCH_NONE, '~', WCH_NONE } },
  { '1', 0, { '1', '!', 0xa0 } },
  { '2', 0, { '2', '@', 0xab } },
  { '3', 0, { '3', '#', 0xbb } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] = {
  { '4', 0, { '4', '$', 0x20ac, 0xa7 } },
  { '5', 0, { '5', '%', WCH_NONE, 0xb0 } },
  { '7', 0, { '7', '&', WCH_NONE, 0xb1 } },
  { '8', 0, { '8', '*', WCH_NONE, 0xd7 } },
  { 'E', CAPLOK, { 'e', 'E', 0x0113, 0x0112 } },
  { 'R', CAPLOK, { 'r', 'R', 0x0157, 0x0156 } },
  { 'U', CAPLOK, { 'u', 'U', 0x016b, 0x016a } },
  { 'I', CAPLOK, { 'i', 'I', 0x012b, 0x012a } },
  { 'O', CAPLOK, { 'o', 'O', 0xf5, 0xd5 } },
  { 'A', CAPLOK, { 'a', 'A', 0x0101, 0x0100 } },
  { 'S', CAPLOK, { 's', 'S', 0x0161, 0x0160 } },
  { 'G', CAPLOK, { 'g', 'G', 0x0123, 0x0122 } },
  { 'K', CAPLOK, { 'k', 'K', 0x0137, 0x0136 } },
  { 'L', CAPLOK, { 'l', 'L', 0x013c, 0x013b } },
  { VK_OEM_7, 0, { '\'', '"', WCH_DEAD, WCH_DEAD } },
  { VK_EMPTY, 0, { WCH_NONE, WCH_NONE, 0xb4, 0xa8 } },
  { 'Z', CAPLOK, { 'z', 'Z', 0x017e, 0x017d } },
  { 'C', CAPLOK, { 'c', 'C', 0x010d, 0x010c } },
  { 'N', CAPLOK, { 'n', 'N', 0x0146, 0x0145 } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS5 key_to_chars_5mod[] = {
  { VK_OEM_4, 0, { '[', '{', WCH_NONE, WCH_NONE, 0x1b } },
  { VK_OEM_6, 0, { ']', '}', WCH_NONE, WCH_NONE, 0x1d } },
  { VK_OEM_5, 0, { WCH_DEAD, '|', WCH_NONE, WCH_NONE, 0x1c } },
  { VK_EMPTY, 0, { 0xb0, WCH_NONE, WCH_NONE, WCH_NONE, WCH_NONE } },
  { VK_OEM_102, 0, { '\\', '|', WCH_NONE, WCH_NONE, 0x1c } },
  { VK_BACK, 0, { 0x08, 0x08, WCH_NONE, WCH_NONE, 0x7f } },
  { VK_ESCAPE, 0, { 0x1b, 0x1b, WCH_NONE, WCH_NONE, 0x1b } },
  { VK_RETURN, 0, { '\r', '\r', WCH_NONE, WCH_NONE, '\n' } },
  { VK_SPACE, 0, { ' ', ' ', WCH_NONE, WCH_NONE, ' ' } },
  { VK_CANCEL, 0, { 0x03, 0x03, WCH_NONE, WCH_NONE, 0x03 } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS6 key_to_chars_6mod[] = {
  { '6', 0, { '6', '^', 0x2019, WCH_NONE, WCH_NONE, 0x1e } },
  { VK_OEM_MINUS, 0, { '-', '_', 0x2013, 0x2014, WCH_NONE, 0x1f } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS2 key_to_chars_2mod[] = {
  { '9', 0, { '9', '(' } },
  { '0', 0, { '0', ')' } },
  { VK_OEM_PLUS, 0, { '=', '+' } },
  { 'Q', CAPLOK, { 'q', 'Q' } },
  { 'W', CAPLOK, { 'w', 'W' } },
  { 'T', CAPLOK, { 't', 'T' } },
  { 'Y', CAPLOK, { 'y', 'Y' } },
  { 'P', CAPLOK, { 'p', 'P' } },
  { 'D', CAPLOK, { 'd', 'D' } },
  { 'F', CAPLOK, { 'f', 'F' } },
  { 'H', CAPLOK, { 'h', 'H' } },
  { 'J', CAPLOK, { 'j', 'J' } },
  { VK_OEM_1, 0, { ';', ':' } },
  { 'X', CAPLOK, { 'x', 'X' } },
  { 'V', CAPLOK, { 'v', 'V' } },
  { 'B', CAPLOK, { 'b', 'B' } },
  { 'M', CAPLOK, { 'm', 'M' } },
  { VK_OEM_COMMA, 0, { ',', '<' } },
  { VK_OEM_PERIOD, 0, { '.', '>' } },
  { VK_OEM_2, 0, { '/', '?' } },
  { VK_DECIMAL, 0, { '.', '.' } },
  { VK_TAB, 0, { '\t', '\t' } },
  { VK_ADD, 0, { '+', '+' } },
  { VK_DIVIDE, 0, { '/', '/' } },
  { VK_MULTIPLY, 0, { '*', '*' } },
  { VK_SUBTRACT, 0, { '-', '-' } },
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
  DEADTRANS(0x0144, 'n', 0xb4, 0),
  DEADTRANS(0x0107, 'c', 0xb4, 0),
  DEADTRANS(0xf3, 'o', 0xb4, 0),
  DEADTRANS(0xe9, 'e', 0xb4, 0),
  DEADTRANS(0x015b, 's', 0xb4, 0),
  DEADTRANS(0x017a, 'z', 0xb4, 0),
  DEADTRANS(0x0143, 'N', 0xb4, 0),
  DEADTRANS(0x0106, 'C', 0xb4, 0),
  DEADTRANS(0xd3, 'O', 0xb4, 0),
  DEADTRANS(0xc9, 'E', 0xb4, 0),
  DEADTRANS(0x015a, 'S', 0xb4, 0),
  DEADTRANS(0x0179, 'Z', 0xb4, 0),
  DEADTRANS(0xb4, ' ', 0xb4, 0),
  DEADTRANS(0xe4, 'a', 0xa8, 0),
  DEADTRANS(0xfc, 'u', 0xa8, 0),
  DEADTRANS(0xf6, 'o', 0xa8, 0),
  DEADTRANS(0xc4, 'A', 0xa8, 0),
  DEADTRANS(0xdc, 'U', 0xa8, 0),
  DEADTRANS(0xd6, 'O', 0xa8, 0),
  DEADTRANS(0xa8, ' ', 0xa8, 0),
  DEADTRANS(0xe5, 'a', 0xb0, 0),
  DEADTRANS(0xc5, 'A', 0xb0, 0),
  DEADTRANS(0x0117, 'e', 0xb0, 0),
  DEADTRANS(0x0116, 'E', 0xb0, 0),
  DEADTRANS(0x017c, 'z', 0xb0, 0),
  DEADTRANS(0x017b, 'Z', 0xb0, 0),
  DEADTRANS(0x0121, 'g', 0xb0, 0),
  DEADTRANS(0xb0, ' ', 0xb0, 0),
  DEADTRANS(0xf5, 'o', '~', 0),
  DEADTRANS(0xd5, 'O', '~', 0),
  DEADTRANS('~', ' ', '~', 0),
  { 0, 0, 0 }
};

ROSDATA DEADKEY_LPWSTR dead_key_names[] = {
  L"\x00b4" L"ACUTE",
  L"\x00a8" L"DIAERESIS",
  L"\x00b0" L"RING",
  L"\x007e" L"TILDE",
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
  0x7f,                       /* bMaxVSCtoVK */
  extcode0_to_vk,
  extcode1_to_vk,
  MAKELONG(KLLF_ALTGR, KBD_VERSION),                          /* fLocaleFlags */
  0, 0, NULL                  /* No ligatures */
};

PKBDTABLES WINAPI KbdLayerDescriptor(VOID) {
  return &keyboard_layout_table;
}
