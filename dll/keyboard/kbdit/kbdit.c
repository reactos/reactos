/*
 * ReactOS Italian Keyboard layout
 * Copyright (C) 2007 ReactOS
 * License: LGPL, see: LGPL.txt
 *
 */

#define WIN32_NO_STATUS
#include <stdarg.h>
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

#define KNUMS     KBDNUMPAD|KBDSPECIAL /* Special + number pad */
#define KMEXT     KBDEXT|KBDMULTIVK    /* Multi + ext */

ROSDATA USHORT scancode_to_vk[] = {
/* 00 */  VK_EMPTY,
/* 01 */  VK_ESCAPE,
/* 02 */  '1',
/* 03 */  '2',
/* 04 */  '3',
/* 05 */  '4',
/* 06 */  '5',
/* 07 */  '6',
/* 08 */  '7',
/* 09 */  '8',
/* 0a */  '9',
/* 0b */  '0',
/* 0c */  VK_OEM_4,
/* 0d */  VK_OEM_6,
/* 0e */  VK_BACK,
/* 0f */  VK_TAB,
/* 10 */  'Q',
/* 11 */  'W',
/* 12 */  'E',
/* 13 */  'R',
/* 14 */  'T',
/* 15 */  'Y',
/* 16 */  'U',
/* 17 */  'I',
/* 18 */  'O',
/* 19 */  'P',
/* 1a */  VK_OEM_1,
/* 1b */  VK_OEM_PLUS,
/* 1c */  VK_RETURN,
/* 1d */  VK_LCONTROL,
/* 1e */  'A',
/* 1f */  'S',
/* 20 */  'D',
/* 21 */  'F',
/* 22 */  'G',
/* 23 */  'H',
/* 24 */  'J',
/* 25 */  'K',
/* 26 */  'L',
/* 27 */  VK_OEM_3,
/* 28 */  VK_OEM_7,
/* 29 */  VK_OEM_2,
/* 2a */  VK_LSHIFT,
/* 2b */  VK_OEM_5,
/* 2c */  'Z',
/* 2d */  'X',
/* 2e */  'C',
/* 2f */  'V',
/* 30 */  'B',
/* 31 */  'N',
/* 32 */  'M',
/* 33 */  VK_OEM_COMMA,
/* 34 */  VK_OEM_PERIOD,
/* 35 */  VK_OEM_MINUS,
/* 36 */  VK_RSHIFT,
/* 37 */  VK_MULTIPLY,
/* 38 */  VK_LMENU,
/* 39 */  VK_SPACE,
/* 3a */  VK_CAPITAL,
/* 3b */  VK_F1,
/* 3c */  VK_F2,
/* 3d */  VK_F3,
/* 3e */  VK_F4,
/* 3f */  VK_F5,
/* 40 */  VK_F6,
/* 41 */  VK_F7,
/* 42 */  VK_F8,
/* 43 */  VK_F9,
/* 44 */  VK_F10,
/* 45 */  VK_NUMLOCK | KMEXT,
/* 46 */  VK_SCROLL | KBDMULTIVK,
/* 47 */  VK_HOME | KNUMS,
/* 48 */  VK_UP | KNUMS,
/* 49 */  VK_PRIOR | KNUMS,
/* 4a */  VK_SUBTRACT,
/* 4b */  VK_LEFT | KNUMS,
/* 4c */  VK_CLEAR | KNUMS,
/* 4d */  VK_RIGHT | KNUMS,
/* 4e */  VK_ADD,
/* 4f */  VK_END | KNUMS,
/* 50 */  VK_DOWN | KNUMS,
/* 51 */  VK_NEXT | KNUMS,
/* 52 */  VK_INSERT | KNUMS,
/* 53 */  VK_DELETE | KNUMS,
/* 54 */  VK_SNAPSHOT,
/* 55 */  VK_EMPTY,
/* 56 */  VK_OEM_102,
/* 57 */  VK_F11,
/* 58 */  VK_F12,
/* 59 */  VK_EMPTY,
/* 5a */  VK_CLEAR,
/* 5b */  VK_EMPTY,
/* 5c */  VK_EMPTY,
/* 5d */  VK_EMPTY,
/* 5e */  VK_EMPTY, /* EREOF */
/* 5f */  VK_EMPTY,
/* 60 */  VK_EMPTY,
/* 61 */  VK_EMPTY,
/* 62 */  VK_EMPTY,
/* 63 */  VK_EMPTY, /* ZOOM */
/* 64 */  VK_HELP,
/* 65 */  VK_F13,
/* 66 */  VK_F14,
/* 67 */  VK_F15,
/* 68 */  VK_F16,
/* 69 */  VK_F17,
/* 6a */  VK_F18,
/* 6b */  VK_F19,
/* 6c */  VK_F20,
/* 6d */  VK_F21,
/* 6e */  VK_F22,
/* 6f */  VK_F23,
/* 70 */  VK_EMPTY,
/* 71 */  VK_EMPTY,
/* 72 */  VK_EMPTY,
/* 73 */  VK_EMPTY,
/* 74 */  VK_EMPTY,
/* 75 */  VK_EMPTY,
/* 76 */  VK_EMPTY,
/* 77 */  VK_F24,
/* 78 */  VK_EMPTY,
/* 79 */  VK_EMPTY,
/* 7a */  VK_EMPTY,
/* 7b */  VK_EMPTY,
/* 7c */  VK_EMPTY,
/* 7d */  VK_EMPTY,
/* 7e */  VK_EMPTY,
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
  { 0, 0 },
};

ROSDATA VSC_VK extcode1_to_vk[] = {
  { 0x1d, VK_PAUSE },
  { 0, 0 },
};

ROSDATA VK_TO_BIT modifier_keys[] = {
  { VK_SHIFT,   KBDSHIFT },
  { VK_CONTROL, KBDCTRL },
  { VK_MENU,    KBDALT },
  { 0,          0 }
};

ROSDATA MODIFIERS modifier_bits = {
  modifier_keys,
  6,
  { 0, 1, 2, 4, SHFT_INVALID, SHFT_INVALID, 3 }
};

ROSDATA VK_TO_WCHARS2 key_to_chars_2mod[] = {
  /* Normal vs Shifted */
  /* The alphabet */
  { 'A',         CAPLOK,   {'a', 'A'} },
  { 'B',         CAPLOK,   {'b', 'B'} },
  { 'C',         CAPLOK,   {'c', 'C'} },
  { 'D',         CAPLOK,   {'d', 'D'} },
  { 'F',         CAPLOK,   {'f', 'F'} },
  { 'G',         CAPLOK,   {'g', 'G'} },
  { 'H',         CAPLOK,   {'h', 'H'} },
  { 'I',         CAPLOK,   {'i', 'I'} },
  { 'J',         CAPLOK,   {'j', 'J'} },
  { 'K',         CAPLOK,   {'k', 'K'} },
  { 'L',         CAPLOK,   {'l', 'L'} },
  { 'M',         CAPLOK,   {'m', 'M'} },
  { 'N',         CAPLOK,   {'n', 'N'} },
  { 'O',         CAPLOK,   {'o', 'O'} },
  { 'P',         CAPLOK,   {'p', 'P'} },
  { 'Q',         CAPLOK,   {'q', 'Q'} },
  { 'R',         CAPLOK,   {'r', 'R'} },
  { 'S',         CAPLOK,   {'s', 'S'} },
  { 'T',         CAPLOK,   {'t', 'T'} },
  { 'U',         CAPLOK,   {'u', 'U'} },
  { 'V',         CAPLOK,   {'v', 'V'} },
  { 'W',         CAPLOK,   {'w', 'W'} },
  { 'X',         CAPLOK,   {'x', 'X'} },
  { 'Y',         CAPLOK,   {'y', 'Y'} },
  { 'Z',         CAPLOK,   {'z', 'Z'} },

  /* The numbers */
  { '1',         0, {'1', '!'} },
  { '2',         0, {'2', '"'} },
  { '3',         0, {'3', 0x00a3} },
  { '4',         0, {'4', '$'} },
  { '5',         0, {'5', '%'} },
  { '6',         0, {'6', '&'} },
  { '7',         0, {'7', '/'} },
  { '8',         0, {'8', '('} },
  { '9',         0, {'9', ')'} },
  { '0',         0, {'0', '='} },

  /* Specials */
  /* Ctrl-_ generates US */
  { VK_OEM_2       ,0, {'\\', '|'} },
  { VK_OEM_4       ,0, {'\'', '?'} },
  { VK_OEM_102     ,0, {'<', '>'} },
  { VK_OEM_COMMA   ,0, {',', ';'} },
  { VK_OEM_PERIOD  ,0, {'.', ':'} },
  { VK_OEM_MINUS   ,0, {'-', '_'} },
  /* Keys that do not have shift states */
  { VK_TAB     ,0, {'\t','\t'} },
  { VK_ADD     ,0, {'+', '+'} },
  { VK_SUBTRACT,0, {'-', '-'} },
  { VK_MULTIPLY,0, {'*', '*'} },
  { VK_DIVIDE  ,0, {'/', '/'} },
  { VK_ESCAPE  ,0, {0x1b,0x1b} },
  { VK_SPACE   ,0, {' ', ' '} },
  { VK_OEM_5   ,0, {0x00f9, 0x00a7} },//ù§
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS3 key_to_chars_3mod[] = {
  /* Normal, Shifted, Ctrl */
  /* Legacy (telnet-style) ascii escapes */
  { VK_RETURN,0, {'\r', '\r', '\n'} },
  { 0,0 }
};

ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] = {
  /* Normal, Shifted, Ctrl, C-S-x */
  { VK_OEM_6,  0, {0x00ec, '^', WCH_NONE, '~' } },
  { VK_OEM_3,  0, {0x00f2, 0x00e7, WCH_NONE, '@' } },//òç
  { VK_OEM_7,  0, {0x00e0, 0x00b0, WCH_NONE, '#'} },//à°

  { VK_OEM_1,  0,   {0x00e8, 0x00e9, '{', '['} },// èé
  { VK_OEM_PLUS,0,   {'+', '*', '}', ']'} },

  /* The alphabet */
  { 'E',         CAPLOK,   {'e', 'E', 0x05, 0x20AC}  }, // eE€

  /* Legacy Ascii generators */
//zz  { VK_OEM_MINUS, 0, {'\'', '?', WCH_NONE, 0x1f /* US */} },
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
  { VK_DECIMAL, 0, {'.'} },
  { VK_BACK,    0, {'\010'} },
  { 0,0 }
};

#define vk_master(n,x) { (PVK_TO_WCHARS1)x, n, sizeof(x[0]) }

ROSDATA VK_TO_WCHAR_TABLE vk_to_wchar_master_table[] = {
  vk_master(1,keypad_numbers),
  vk_master(2,key_to_chars_2mod),
  vk_master(3,key_to_chars_3mod),
  vk_master(4,key_to_chars_4mod),
  { 0,0,0 }
};

ROSDATA VSC_LPWSTR key_names[] = {
  { 0x00, L"" },
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
  { 0x3a, L"CAPLOK Lock" },
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
  { 0x1d, L"Right Ctrl" },
  { 0x35, L"Num /" },
  { 0x37, L"Prnt Scrn" },
  { 0x38, L"Right Alt" },
  { 0x45, L"Num Lock" },
  { 0x46, L"Break" },
  { 0x47, L"Home" },
  { 0x48, L"Up" },
  { 0x49, L"Page Up" },
  { 0x4b, L"Left" },
  { 0x4c, L"Center" },
  { 0x4d, L"Right" },
  { 0x4f, L"End" },
  { 0x50, L"Down" },
  { 0x51, L"Page Down" },
  { 0x52, L"Insert" },
  { 0x53, L"Delete" },
  { 0x54, L"<ReactOS>" },
  { 0x55, L"Help" },
  { 0x56, L"Left Windows" },
  { 0x5b, L"Right Windows" },
  { 0, NULL },
};

/* Finally, the master table */
ROSDATA KBDTABLES keyboard_layout_table = {
  /* modifier assignments */
  &modifier_bits,

  /* character from vk tables */
  vk_to_wchar_master_table,

  /* diacritical marks -- currently implemented by wine code */
  /* doesn't have any, anyway */
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

  MAKELONG(KLLF_ALTGR, 1), /* Version 1.0 */

  /* Ligatures */
  0,
  0,
  NULL
};

PKBDTABLES WINAPI KbdLayerDescriptor(VOID) {
  return &keyboard_layout_table;
}

