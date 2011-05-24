/*
 * ReactOS Thai Kedmanee (non-ShiftLock) Keyboard layout
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

ROSDATA USHORT scancode_to_vk[] = {
  /* Numbers Row */
  /* - 00 - */
  /* 1 ...         2 ...         3 ...         4 ... */
  VK_EMPTY,     VK_ESCAPE,    '1',          '2',
  '3',          '4',          '5',          '6',
  '7',          '8',          '9',          '0',
  VK_OEM_MINUS, VK_OEM_PLUS,  VK_BACK,
  /* - 0f - */
  /* First Letters Row */
  VK_TAB,       'Q',          'W',          'E',
  'R',          'T',          'Y',          'U',
  'I',          'O',          'P',
  VK_OEM_4,     VK_OEM_6,     VK_RETURN,
  /* - 1d - */
  /* Second Letters Row */
  VK_LCONTROL,
  'A',          'S',          'D',          'F',
  'G',          'H',          'J',          'K',
  'L',          VK_OEM_1,     VK_OEM_7,     VK_OEM_3,
  VK_LSHIFT,    VK_OEM_5,
  /* - 2c - */
  /* Third letters row */
  'Z',          'X',          'C',          'V',
  'B',          'N',          'M',          VK_OEM_COMMA,
  VK_OEM_PERIOD,VK_OEM_2,     VK_RSHIFT | KEXT,
  /* - 37 - */
  /* Bottom Row */
  VK_MULTIPLY,  VK_LMENU,     VK_SPACE,     VK_CAPITAL,

  /* - 3b - */
  /* F-Keys */
  VK_F1, VK_F2, VK_F3, VK_F4, VK_F5, VK_F6,
  VK_F7, VK_F8, VK_F9, VK_F10,
  /* - 45 - */
  /* Locks */
  VK_NUMLOCK | KMEXT,
  VK_SCROLL  | KMULTI,
  /* - 47 - */
  /* Number-Pad */
  VK_HOME   | KNUMS,      VK_UP     | KNUMS,      VK_PRIOR | KNUMS, VK_SUBTRACT,
  VK_LEFT   | KNUMS,      VK_CLEAR  | KNUMS,      VK_RIGHT | KNUMS, VK_ADD,
  VK_END    | KNUMS,      VK_DOWN   | KNUMS,      VK_NEXT  | KNUMS,
  VK_INSERT | KNUMS,      VK_DELETE | KNUMS,
  /* - 54 - */
  /* Presumably PrtSc */
  VK_SNAPSHOT,
  /* - 55 - */
  /* Oddities, and the remaining standard F-Keys */
  VK_EMPTY,     VK_OEM_102,     VK_F11,       VK_F12,
  /* - 59 - */
  VK_CLEAR,	  VK_OEM_WSCTRL, VK_OEM_FINISH, VK_OEM_JUMP, VK_EREOF, /* EREOF */
  VK_OEM_BACKTAB, VK_OEM_AUTO,   VK_EMPTY,      VK_ZOOM,               /* ZOOM */
  VK_HELP,
  /* - 64 - */
  /* Even more F-Keys (for example, NCR keyboards from the early 90's) */
  VK_F13, VK_F14, VK_F15, VK_F16, VK_F17, VK_F18, VK_F19, VK_F20,
  VK_F21, VK_F22, VK_F23,
  /* - 6f - */
  /* Not sure who uses these codes */
  VK_OEM_PA3, VK_EMPTY, VK_OEM_RESET,
  /* - 72 - */
  VK_EMPTY, 0xc1, VK_EMPTY, VK_EMPTY,
  /* - 76 - */
  /* One more f-key */
  VK_F24,
  /* - 77 - */
  VK_EMPTY, VK_EMPTY, VK_EMPTY, VK_EMPTY,
  VK_OEM_PA1, VK_TAB, 0xc2, 0, /* PA1 */
  0,
  /* - 80 - */
  0
};

ROSDATA VSC_VK extcode0_to_vk[] = {
  { 0x10, VK_MEDIA_PREV_TRACK | KEXT },
  { 0x19, VK_MEDIA_NEXT_TRACK | KEXT },
  { 0x1D, VK_RCONTROL | KEXT },
  { 0x20, VK_VOLUME_MUTE | KEXT },
  { 0x21, VK_LAUNCH_APP2 | KEXT },
  { 0x22, VK_MEDIA_PLAY_PAUSE | KEXT },
  { 0x24, VK_MEDIA_STOP | KEXT },
  { 0x2E, VK_VOLUME_DOWN | KEXT },
  { 0x30, VK_VOLUME_UP | KEXT },
  { 0x32, VK_BROWSER_HOME | KEXT },
  { 0x35, VK_DIVIDE | KEXT },
  { 0x37, VK_SNAPSHOT | KEXT },
  { 0x38, VK_RMENU | KEXT },
  { 0x47, VK_HOME | KEXT },
  { 0x48, VK_UP | KEXT },
  { 0x49, VK_PRIOR | KEXT },
  { 0x4B, VK_LEFT | KEXT },
  { 0x4D, VK_RIGHT | KEXT },
  { 0x4F, VK_END | KEXT },
  { 0x50, VK_DOWN | KEXT },
  { 0x51, VK_NEXT | KEXT },
  { 0x52, VK_INSERT | KEXT },
  { 0x53, VK_DELETE | KEXT },
  { 0x5B, VK_LWIN | KEXT },
  { 0x5C, VK_RWIN | KEXT },
  { 0x5D, VK_APPS | KEXT },
  { 0x5F, VK_SLEEP | KEXT },
  { 0x65, VK_BROWSER_SEARCH | KEXT },
  { 0x66, VK_BROWSER_FAVORITES | KEXT },
  { 0x67, VK_BROWSER_REFRESH | KEXT },
  { 0x68, VK_BROWSER_STOP | KEXT },
  { 0x69, VK_BROWSER_FORWARD | KEXT },
  { 0x6A, VK_BROWSER_BACK | KEXT },
  { 0x6B, VK_LAUNCH_APP1 | KEXT },
  { 0x6C, VK_LAUNCH_MAIL | KEXT },
  { 0x6D, VK_LAUNCH_MEDIA_SELECT | KEXT },
  { 0x1C, VK_RETURN | KEXT },
  { 0x46, VK_CANCEL | KEXT },
  { 0, 0 },
};

ROSDATA VSC_VK extcode1_to_vk[] = {
  { 0x1d, VK_PAUSE},
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
  3,
  { 0, 1, 2, 3 } /* Modifier bit order */
};

#define NOCAPS 0
#define CAPS   KSHIFT /* Caps -> shift */

ROSDATA VK_TO_WCHARS2 key_to_chars_2mod[] = {
  { VK_OEM_3,     0, {'_',    '%'   } },
  { '1',          0, {0x0e45, '+'   } },
  { '2',          0, {'/',    0x0e51} },
  { '3',          0, {'-',    0x0e52} },
  { '4',          0, {0x0e20, 0x0e53} },
  { '5',          0, {0x0e16, 0x0e54} },
  { '6',          0, {0x0e38, 0x0e39} },
  { '7',          0, {0x0e36, 0x0e3f} },
  { '8',          0, {0x0e04, 0x0e55} },
  { '9',          0, {0x0e15, 0x0e56} },
  { '0',          0, {0x0e08, 0x0e57} },
  { VK_OEM_MINUS, 0, {0x0e02, 0x0e58} },
  { VK_OEM_PLUS,  0, {0x0e0a, 0x0e59} },
  { VK_OEM_5,     0, {0x0e03, 0x0e05} },

  { VK_OEM_4,     0, {0x0e1a, 0x0e10} },
  { VK_OEM_6,     0, {0x0e25, ','   } },
  { VK_OEM_1,     0, {0x0e27, 0x0e0b} },
  { VK_OEM_7,     0, {0x0e07, '.'   } },
  { VK_OEM_COMMA, 0, {0x0e21, 0x0e12} },
  { VK_OEM_PERIOD,0, {0x0e43, 0x0e2c} },
  { VK_OEM_2,     0, {0x0e1d, 0x0e26} },

  { VK_TAB,	  0, {'\t', '\t'} },
  { VK_ADD,       0, {'+',  '+' } },
  { VK_SUBTRACT,  0, {'-',  '-' } },
  { VK_MULTIPLY,  0, {'*',  '*' } },
  { VK_DIVIDE,    0, {'/',  '/' } },
  { VK_ESCAPE,    0, {0x1b, 0x1b} },
  { VK_SPACE,     0, {' ',  ' ' } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS3 key_to_chars_3mod[] = {
  { VK_BACK,      0, {'\b', '\b', 0x7f} },
  { VK_RETURN,    0, {'\r', '\r', '\n'} },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] = {
  { 'Q',          0, {0x0e46, 0x0e50, 0x11, 0x11} },
  { 'W',          0, {0x0e44, '\"'  , 0x17, 0x17} },
  { 'E',          0, {0x0e33, 0x0e0e, 0x05, 0x05} },
  { 'R',          0, {0x0e1e, 0x0e11, 0x12, 0x12} },
  { 'T',          0, {0x0e30, 0x0e18, 0x14, 0x14} },
  { 'Y',          0, {0x0e31, 0x0e4d, 0x19, 0x19} },
  { 'U',          0, {0x0e35, 0x0e4a, 0x15, 0x15} },
  { 'I',          0, {0x0e23, 0x0e13, 0x09, 0x09} },
  { 'O',          0, {0x0e19, 0x0e2f, 0x0f, 0x0f} },
  { 'P',          0, {0x0e22, 0x0e0d, 0x10, 0x10} },

  { 'A',          0, {0x0e1f, 0x0e24, 0x01, 0x01} },
  { 'S',          0, {0x0e2b, 0x0e06, 0x13, 0x13} },
  { 'D',          0, {0x0e01, 0x0e0f, 0x04, 0x04} },
  { 'F',          0, {0x0e14, 0x0e42, 0x06, 0x06} },
  { 'G',          0, {0x0e40, 0x0e0c, 0x07, 0x07} },
  { 'H',          0, {0x0e49, 0x0e47, 0x08, 0x08} },
  { 'J',          0, {0x0e48, 0x0e4b, 0x0a, 0x0a} },
  { 'K',          0, {0x0e32, 0x0e29, 0x0b, 0x0b} },
  { 'L',          0, {0x0e2a, 0x0e28, 0x0c, 0x0c} },

  { 'Z',          0, {0x0e1c, '('   , 0x1a, 0x1a} },
  { 'X',          0, {0x0e1b, ')'   , 0x18, 0x18} },
  { 'C',          0, {0x0e41, 0x0e09, 0x03, 0x03} },
  { 'V',          0, {0x0e2d, 0x0e2e, 0x16, 0x16} },
  { 'B',          0, {0x0e34, 0x0e3a, 0x02, 0x02} },
  { 'N',          0, {0x0e37, 0x0e4c, 0x0e, 0x0e} },
  { 'M',          0, {0x0e17, '?'   , 0x0d, 0x0d} },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS1 keypad_numbers[] = {
  { VK_NUMPAD0,   0, {'0'} },
  { VK_NUMPAD1,   0, {'1'} },
  { VK_NUMPAD2,   0, {'2'} },
  { VK_NUMPAD3,   0, {'3'} },
  { VK_NUMPAD4,   0, {'4'} },
  { VK_NUMPAD5,   0, {'5'} },
  { VK_NUMPAD6,   0, {'6'} },
  { VK_NUMPAD7,   0, {'7'} },
  { VK_NUMPAD8,   0, {'8'} },
  { VK_NUMPAD9,   0, {'9'} },
  { VK_DECIMAL,   0, {'.'} },
  { 0, 0 }
};

#define vk_master(n,x) { (PVK_TO_WCHARS1)x, n, sizeof(x[0]) }

ROSDATA VK_TO_WCHAR_TABLE vk_to_wchar_master_table[] = {
  vk_master(1, keypad_numbers),
  vk_master(2, key_to_chars_2mod),
  vk_master(3, key_to_chars_3mod),
  vk_master(4, key_to_chars_4mod),
  { 0, 0, 0 }
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
  { 0x4c, L"Center" },
  { 0x4d, L"Right" },
  { 0x4f, L"End" },
  { 0x50, L"Down" },
  { 0x51, L"Page Down" },
  { 0x52, L"Insert" },
  { 0x53, L"Delete" },
  { 0x54, L"<ReactOS>" },
  { 0x56, L"Help" },
  { 0x5b, L"Left <ReactOS>" },
  { 0x5c, L"Right <ReactOS>" },
  { 0x5d, L"Application" },
  { 0, NULL }
};

/* Finally, the master table */
ROSDATA KBDTABLES keyboard_layout_table = {
  /* modifier assignments */
  &modifier_bits,

  /* character from vk tables */
  vk_to_wchar_master_table,

  /* diacritical marks -- currently implemented by wine code */
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

PKBDTABLES WINAPI KbdLayerDescriptor(VOID) {
  return &keyboard_layout_table;
}

/* EOF */
