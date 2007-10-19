/*
 * ReactOS Italian Keyboard layout
 * Copyright (C) 2007 ReactOS
 * License: LGPL, see: LGPL.txt
 *
 */

#include <windows.h>
#include <internal/kbd.h>

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
#define KSHIFT   0x001  /* Shift modifier */
#define KCTRL    0x002  /* Ctrl modifier */
#define KALT     0x004  /* Alt modifier */
#define KEXT     0x100  /* Extended key code */
#define KMULTI   0x200  /* Multi-key */
#define KSPEC    0x400  /* Special key */
#define KNUMP    0x800  /* Number-pad */
#define KNUMS    0xc00  /* Special + number pad */
#define KMEXT    0x300  /* Multi + ext */

#define SHFT_INVALID 0x0F

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
/* 46 */  VK_SCROLL | KMULTI,
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
/* 7f */  VK_EMPTY,
/* 80 */  VK_EMPTY,
/* 00 */  0
};

ROSDATA VSC_VK extcode0_to_vk[] = {
  { 0, 0 },
};

ROSDATA VSC_VK extcode1_to_vk[] = {
  { 0, 0 },
};

ROSDATA VK_TO_BIT modifier_keys[] = {
  { VK_SHIFT,   KSHIFT },
  { VK_CONTROL, KCTRL },
  { VK_MENU,    KALT },
  { 0,          0 }
};

ROSDATA MODIFIERS modifier_bits = {
  modifier_keys,
  6,
  {   0,     1,    2,          4,   SHFT_INVALID, SHFT_INVALID, 3  }
};

#define NOCAPS 0
#define CAPS   KSHIFT /* Caps -> shift */

ROSDATA VK_TO_WCHARS2 key_to_chars_2mod[] = {
  /* Normal vs Shifted */
  /* The numbers */
  { '1',         NOCAPS, {'1', '!'} },
  { '2',         NOCAPS, {'2', '"'} },
  { '3',         NOCAPS, {'3', 0x00a3} },
  { '4',         NOCAPS, {'4', '$'} },
  { '5',         NOCAPS, {'5', '%'} },
  { '6',         NOCAPS, {'6', '&'} },
  { '7',         NOCAPS, {'7', '/'} },
  { '8',         NOCAPS, {'8', '('} },
  { '9',         NOCAPS, {'9', ')'} },
  { '0',         NOCAPS, {'0', '='} },

  /* Specials */
  /* Ctrl-_ generates US */
  { VK_OEM_2       ,NOCAPS, {'\\', '|'} },
  { VK_OEM_4       ,NOCAPS, {'\'', '?'} },
  { VK_OEM_102     ,NOCAPS, {'<', '>'} },
  { VK_OEM_COMMA   ,NOCAPS, {',', ';'} },
  { VK_OEM_PERIOD  ,NOCAPS, {'.', ':'} },
  { VK_OEM_MINUS   ,NOCAPS, {'-', '_'} },
  /* Keys that do not have shift states */
  { VK_TAB     ,NOCAPS, {'\t','\t'} },
  { VK_ADD     ,NOCAPS, {'+', '+'} },
  { VK_SUBTRACT,NOCAPS, {'-', '-'} },
  { VK_MULTIPLY,NOCAPS, {'*', '*'} },
  { VK_DIVIDE  ,NOCAPS, {'/', '/'} },
  { VK_ESCAPE  ,NOCAPS, {'\x1b','\x1b'} },
  { VK_SPACE   ,NOCAPS, {' ', ' '} },
  { VK_OEM_5   ,NOCAPS, {0x00f9, 0x00a7} },//ù§
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
  { VK_OEM_6,  NOCAPS, {0x00ec, '^', WCH_NONE, '~' } },
  { VK_OEM_3,  NOCAPS, {0x00f2, 0x00e7, WCH_NONE, '@' } },//òç
  { VK_OEM_7,  NOCAPS, {0x00e0, 0x00b0, WCH_NONE, '#'} },//à°

  { VK_OEM_1,  NOCAPS,   {0x00e8, 0x00e9, '{', '['} },// èé
  { VK_OEM_PLUS,NOCAPS,   {'+', '*', '}', ']'} },

  /* The alphabet */
  { 'A',         CAPS,   {'a', 'A', 0x01, 0x01} },
  { 'B',         CAPS,   {'b', 'B', 0x02, 0x02} },
  { 'C',         CAPS,   {'c', 'C', 0x03, 0x03} },
  { 'D',         CAPS,   {'d', 'D', 0x04, 0x04} },
  { 'E',	 CAPS,   {'e', 'E', 0x05, 0x20AC}  }, // eE€
  { 'F',         CAPS,   {'f', 'F', 0x06, 0x06} },
  { 'G',         CAPS,   {'g', 'G', 0x07, 0x07} },
  { 'H',         CAPS,   {'h', 'H', 0x08, 0x08} },
  { 'I',         CAPS,   {'i', 'I', 0x09, 0x09} },
  { 'J',         CAPS,   {'j', 'J', 0x0a, 0x0a} },
  { 'K',         CAPS,   {'k', 'K', 0x0b, 0x0b} },
  { 'L',         CAPS,   {'l', 'L', 0x0c, 0x0c} },
  { 'M',         CAPS,   {'m', 'M', 0x0d, 0x0d} },
  { 'N',         CAPS,   {'n', 'N', 0x0e, 0x0e} },
  { 'O',         CAPS,   {'o', 'O', 0x0f, 0x0f} },
  { 'P',         CAPS,   {'p', 'P', 0x10, 0x10} },
  { 'Q',         CAPS,   {'q', 'Q', 0x11, 0x11} },
  { 'R',         CAPS,   {'r', 'R', 0x12, 0x12} },
  { 'S',         CAPS,   {'s', 'S', 0x13, 0x13} },
  { 'T',         CAPS,   {'t', 'T', 0x14, 0x14} },
  { 'U',         CAPS,   {'u', 'U', 0x15, 0x15} },
  { 'V',         CAPS,   {'v', 'V', 0x16, 0x16} },
  { 'W',         CAPS,   {'w', 'W', 0x17, 0x17} },
  { 'X',         CAPS,   {'x', 'X', 0x18, 0x18} },
  { 'Y',         CAPS,   {'y', 'Y', 0x19, 0x19} },
  { 'Z',         CAPS,   {'z', 'Z', 0x1a, 0x1a} },

  /* Legacy Ascii generators */
//zz  { VK_OEM_MINUS, NOCAPS, {'\'', '?', WCH_NONE, 0x1f /* US */} },
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
  { 0x1d, L"Right Ctrl" },
  { 0x35, L"Num /" },
  { 0x37, L"Prnt Scrn" },
  { 0x38, L"Right Alt" },
  { 0x45, L"Num Lock" },
  { 0x46, L"Break" },
  { 0x47, L"Home" },
  { 0x48, L"Up" },
  { 0x49, L"Page Up" },
  { 0x4a, L"Left" },
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
  sizeof(scancode_to_vk) / sizeof(scancode_to_vk[0]),
  extcode0_to_vk,
  extcode1_to_vk,

  MAKELONG(0,1), /* Version 1.0 */

  /* Ligatures */
  0,
  0,
  NULL
};

PKBDTABLES STDCALL KbdLayerDescriptor(VOID) {
  return &keyboard_layout_table;
}

