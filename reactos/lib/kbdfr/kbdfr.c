/*
 * ReactOS FRASCII Keyboard layout
 * Copyright (C) 2003 ReactOS
 * License: LGPL, see: LGPL.txt
 * autor: Jean-Michel Gay 2003
 * 
 */

#include <windows.h>
#include <internal/kbd.h>

#ifdef _M_IA64
#define ROSDATA static __declspec(allocate(".data"))
#else
#pragma data_seg(".data")
#define ROSDATA static
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

/* Thanks to http://asp.flaaten.dk/pforum/keycode/keycode.htm */
#ifndef VK_OEM_1
#define VK_OEM_1  0xba
#endif
#ifndef VK_OEM_PLUS
#define VK_OEM_PLUS 0xbb
#endif
#ifndef VK_OEM_COMMA
#define VK_OEM_COMMA 0xbc
#endif
#ifndef VK_OEM_MINUS
#define VK_OEM_MINUS 0xbd
#endif
#ifndef VK_OEM_PERIOD
#define VK_OEM_PERIOD 0xbe
#endif
#ifndef VK_OEM_2
#define VK_OEM_2 0xbf
#endif
#ifndef VK_OEM_3
#define VK_OEM_3 0xc0
#endif
#ifndef VK_OEM_4
#define VK_OEM_4 0xdb
#endif
#ifndef VK_OEM_5
#define VK_OEM_5 0xdc
#endif
#ifndef VK_OEM_6
#define VK_OEM_6 0xdd
#endif
#ifndef VK_OEM_7
#define VK_OEM_7 0xde
#endif
#ifndef VK_OEM_8
#define VK_OEM_8 0xdf
#endif
#ifndef VK_OEM_102
#define VK_OEM_102 0xe2
#endif
#ifndef VK_ZOOM
#define VK_ZOOM 0xfb
#endif

ROSDATA USHORT scancode_to_vk[] = {
  /* Numbers Row */
  /* - 00 - */
  /* 1 ...         2 ...         3 ...         4 ... */     
  VK_EMPTY,     VK_ESCAPE,    '1',          '2',
  '3',          '4',          '5',          '6',
  '7',          '8',          '9',          '0',
  VK_OEM_4, VK_OEM_PLUS,  VK_BACK,
  /* - 0f - */
  /* First Letters Row */
  VK_TAB,       'A',          'Z',          'E',
  'R',          'T',          'Y',          'U',
  'I',          'O',          'P',          
  VK_OEM_6,     VK_OEM_1,     VK_RETURN,
  /* - 1d - */
  /* Second Letters Row */
  VK_LCONTROL,  
  'Q',          'S',          'D',          'F',
  'G',          'H',          'J',          'K',
  'L',          'M' , VK_OEM_3,     VK_OEM_7, 
  VK_LSHIFT,    VK_OEM_5,
  /* - 2c - */
  /* Third letters row */
  'W',          'X',          'C',          'V',
  'B',          'N',          VK_OEM_COMMA,
  VK_OEM_PERIOD,VK_OEM_2,    VK_OEM_8,  VK_RSHIFT,
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
  VK_SCROLL | KMULTI,    
  /* - 47 - */
  /* Number-Pad */
  VK_HOME | KNUMS,      VK_UP | KNUMS,         VK_PRIOR | KNUMS, VK_SUBTRACT,
  VK_LEFT | KNUMS,      VK_CLEAR | KNUMS,      VK_RIGHT | KNUMS, VK_ADD,
  VK_END | KNUMS,       VK_DOWN | KNUMS,       VK_NEXT | KNUMS,
  VK_INSERT | KNUMS,    VK_DELETE | KNUMS,
  /* - 54 - */
  /* Presumably PrtSc */
  VK_SNAPSHOT,
  /* - 55 - */
  /* Oddities, and the remaining standard F-Keys */
  VK_EMPTY,     VK_OEM_102,     VK_F11,       VK_F12,
  /* - 59 - */
  VK_CLEAR,     VK_EMPTY,     VK_EMPTY,     VK_EMPTY,     VK_EMPTY, /* EREOF */
  VK_EMPTY,     VK_EMPTY,     VK_EMPTY,     VK_EMPTY,     VK_ZOOM, /* ZOOM */
  VK_HELP,      
  /* - 64 - */
  /* Even more F-Keys (for example, NCR keyboards from the early 90's) */
  VK_F13, VK_F14, VK_F15, VK_F16, VK_F17, VK_F18, VK_F19, VK_F20,
  VK_F21, VK_F22, VK_F23, 
  /* - 6f - */
  /* Not sure who uses these codes */
  VK_EMPTY, VK_EMPTY, VK_EMPTY,
  /* - 72 - */
  VK_EMPTY, VK_EMPTY, VK_EMPTY, VK_EMPTY,
  /* - 76 - */
  /* One more f-key */
  VK_F24,
  /* - 77 - */
  VK_EMPTY, VK_EMPTY, VK_EMPTY, VK_EMPTY,
  VK_EMPTY, VK_EMPTY, VK_EMPTY, VK_EMPTY, /* PA1 */
  VK_EMPTY,
  /* - 80 - */
  0
};

ROSDATA VSC_VK extcode0_to_vk[] = {
	// FIXME:m qu'est ce que c'est ?
	// What is this?
#if 0
  { 'G', '$' },
  { 'H', '&' },
  //{ 'I', '!' },
  { 'K', '%' },
  { 'M', '\'' },
  { 'O', '#' },
  { 'P', '(' },
  { 'Q', '"' },
  { 'R', '-' },
  { '_', '_' },
  { '[', '[' },
  { ']', ']' },
#endif 
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

typedef struct _mymod {
  PVOID mod_keys;
  WORD maxmod;
  BYTE mod_max[7];
} INTERNAL_KBD_MODIFIERS;

ROSDATA INTERNAL_KBD_MODIFIERS modifier_bits[] = {
  modifier_keys,
  6,
  { 0, 1, 2, 4,15,15,3 } 
  /* new: Modifier bit order, NONE, SHIFT, CTRL, ALT , ? ,? , shift+control*/
  /* old: Modifier bit order, NONE, SHIFT, CTRL, ALT */
};

#define NOCAPS 0
#define CAPS   KSHIFT /* Caps -> shift */

ROSDATA VK_TO_WCHARS2 key_to_chars_2mod[] = {
  /* Normal vs Shifted */
  /* The numbers */
  { '1',         NOCAPS, '&', '1' },
  //{ '2',         NOCAPS, 'é', '2' },
  /* Ctrl-2 generates NUL */
  //{ '3',         NOCAPS, '"', '3' },
  //{ '4',         NOCAPS, '\'', '4' },
  //{ '5',         NOCAPS, '(', '5' },
  //{ '6',         NOCAPS, '-', '6' },
  /* Ctrl-6 generates RS */
  //{ '7',         NOCAPS, 'è', '7' },
  //{ '8',         NOCAPS, '_', '8' },
  //{ '9',         NOCAPS, 'ç', '9' },
  //{ '0',         NOCAPS, 'à', '0' },
  /* First letter row */
  { 'A',         CAPS,   'a', 'A' },
  { 'Z',         CAPS,   'z', 'Z' },
  //{ 'E',         CAPS,   'e', 'E' },
  { 'R',         CAPS,   'r', 'R' },
  { 'T',         CAPS,   't', 'T' },
  { 'Y',         CAPS,   'y', 'Y' },
  { 'U',         CAPS,   'u', 'U' },
  { 'I',         CAPS,   'i', 'I' },
  { 'O',         CAPS,   'o', 'O' },
  { 'P',         CAPS,   'p', 'P' },
  /* Second letter row */
  { 'Q',         CAPS,   'q', 'Q' },
  { 'S',         CAPS,   's', 'S' },
  { 'D',         CAPS,   'd', 'D' },
  { 'F',         CAPS,   'f', 'F' },
  { 'G',         CAPS,   'g', 'G' },
  { 'H',         CAPS,   'h', 'H' },
  { 'J',         CAPS,   'j', 'J' },
  { 'K',         CAPS,   'k', 'K' },
  { 'L',         CAPS,   'l', 'L' },
  { 'M',         CAPS,   'm', 'M' },
  /* Third letter row */
  { 'W',         CAPS,   'w', 'W' },
  { 'X',         CAPS,   'x', 'X' },
  { 'C',         CAPS,   'c', 'C' },
  { 'V',         CAPS,   'v', 'V' },
  { 'B',         CAPS,   'b', 'B' },
  { 'N',         CAPS,   'n', 'N' },

  /* Specials */
  /* Ctrl-_ generates US */
  //{ VK_OEM_1       ,NOCAPS, '$', '£' },
  { VK_OEM_5       ,NOCAPS, '*',L'µ'},
  { VK_OEM_3       ,NOCAPS, L'ù', '%' },
  { VK_OEM_COMMA   ,NOCAPS, ',', '?' },
  { VK_OEM_PERIOD  ,NOCAPS, ';', '.' },
  { VK_OEM_2       ,NOCAPS, ':', '/' },
  { VK_OEM_8       ,NOCAPS, '!', L'§' },
  /* Keys that do not have shift states */
  { VK_TAB     ,NOCAPS, '\t','\t'},
  { VK_ADD     ,NOCAPS, '+', '+' },
  { VK_SUBTRACT,NOCAPS, '-', '-' },
  { VK_MULTIPLY,NOCAPS, '*', '*' },
  { VK_DIVIDE  ,NOCAPS, '/', '/' },
  { VK_ESCAPE  ,NOCAPS, '\x1b','\x1b' },
  { VK_SPACE   ,NOCAPS, ' ', ' ' },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS3 key_to_chars_3mod[] = {
  /* Normal, Shifted, Ctrl */
  /* Legacy (telnet-style) ascii escapes */
  { VK_OEM_102, 0, '<', '>', 0x1c /* FS */ },
  { VK_OEM_6, 0, WCH_DEAD, WCH_DEAD, WCH_NONE },
  { VK_EMPTY, 0, L'^', L'¨', WCH_NONE }, //OEM 6 DEAD
  { VK_OEM_7, 0, L'²','|', 0x1c /* FS */ },
  { VK_RETURN,0, '\r', '\r', '\n' },
  { 0,0 }
};

ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] = {
  /* Normal, shifted, control, Alt+Gr */
  { '2' ,       1, L'é',   '2',      WCH_NONE, WCH_DEAD },
  { VK_EMPTY,   0, WCH_NONE,  WCH_NONE, WCH_NONE, L'~' },
  { '3' ,       0, '"',       '3',      WCH_NONE, '#' },
  { '4' ,       0, '\'',      '4',      WCH_NONE, '{' },
  { '7' ,       1, L'è',   '7',      WCH_NONE, WCH_DEAD },
  { VK_EMPTY,   0, WCH_NONE,  WCH_NONE, WCH_NONE, L'`' },
  { '9' ,       1, L'ç',   '9',      WCH_NONE, L'^' },
  { '0' ,       1, L'à',   '0',      WCH_NONE, '@' },
  { VK_OEM_PLUS,0, '=',       '+',      WCH_NONE, '}' },
  { 'E' ,       1, 'e',       'E',      WCH_NONE, L'€' /* euro */ },
  { VK_OEM_1,   0, '$',       L'£',  WCH_NONE, L'¤' },
  { VK_OEM_4,   0, ')',       '°',   WCH_NONE, ']'  },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS5 key_to_chars_5mod[] = {
  /* x,x,      Normal, Shifted, Ctrl, Alt, C-S-x */
  /* Legacy Ascii generators */
  //{ '2', NOCAPS, '2', '@', WCH_NONE, 0 },
  //{ '6', NOCAPS, '6', '^', WCH_NONE, 0x1e /* RS */ },
  //{ VK_OEM_MINUS, NOCAPS, ')', '°', WCH_NONE, 0x1f /* US */ },
  //{ '5'  | KEXT , NOCAPS, ')', '°', '#' , 0x1f /* US */ },
  { '5' , 1  ,  '(', '5', WCH_NONE , '[' , 0x1b  },
  { '6' , 1  ,  '-', '6', WCH_NONE , '|' , 0x1f  },
  { '8' , 1  ,  '_', '8', WCH_NONE , '\\' , 0x1c  },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS1 keypad_numbers[] = {
  { VK_DECIMAL, 0, '.' },
  { VK_NUMPAD0, 0, '0' },
  { VK_NUMPAD1, 0, '1' },
  { VK_NUMPAD2, 0, '2' },
  { VK_NUMPAD3, 0, '3' },
  { VK_NUMPAD4, 0, '4' },
  { VK_NUMPAD5, 0, '5' },
  { VK_NUMPAD6, 0, '6' },
  { VK_NUMPAD7, 0, '7' },
  { VK_NUMPAD8, 0, '8' },
  { VK_NUMPAD9, 0, '9' },
  { VK_BACK,    0, '\010' },
  { 0,0 }
};

#define vk_master(n,x) { (PVK_TO_WCHARS1)x, n, sizeof(x[0]) }

ROSDATA VK_TO_WCHAR_TABLE vk_to_wchar_master_table[] = {
  vk_master(1,keypad_numbers),
  vk_master(2,key_to_chars_2mod),
  vk_master(3,key_to_chars_3mod),
  vk_master(4,key_to_chars_4mod),
  vk_master(5,key_to_chars_5mod),
  { 0,0,0 }
};

#define DK(l,a) (l | a <<16)
ROSDATA DEADKEY  deadkey[] =
{
	{ DK('a',L'¨'), L'ä' , 0 },
	{ DK('e',L'¨'), L'ë' , 0 },
	{ DK('i',L'¨'), L'ï' , 0 },
	{ DK('o',L'¨'), L'ö' , 0 },
	{ DK('u',L'¨'), L'ü' , 0 },
	{ DK(' ',L'¨'), L'¨' , 0 },
	{ DK('a',L'^'), L'â' , 0 },
	{ DK(L'^',L'e'), L'ê' , 0 },
	{ DK('i',L'^'), L'î' , 0 },
	{ DK('o',L'^'), L'ô' , 0 },
	{ DK('u',L'^'), L'û' , 0 },
	{ DK(' ',L'^'), L'^' , 0 },
	{ DK('a',L'`'), L'à' , 0 },
	{ DK('e',L'`'), L'è' , 0 },
	{ DK('i',L'`'), L'ì' , 0 },
	{ DK('o',L'`'), L'ò' , 0 },
	{ DK('u',L'`'), L'ù' , 0 },
	{ DK(' ',L'`'), L'`' , 0 },
	{ DK('n',L'~'), L'ñ' , 0 },
	{ DK(' ',L'~'), L'~' , 0 },
  { 0,0 ,0,}
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
  (PMODIFIERS)&modifier_bits,
  
  /* character from vk tables */
  vk_to_wchar_master_table,
  
  /* diacritical marks -- currently implemented by wine code */
  &deadkey,

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

  /* Ligatures -- English doesn't have any */
  0,
  0,
  NULL
};

PKBDTABLES STDCALL KbdLayerDescriptor() {
  return &keyboard_layout_table;
}

