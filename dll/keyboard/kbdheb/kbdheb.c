/*
 * ReactOS Hebrew Keyboard layout
 * Copyright (C) 2005 - 2013 ReactOS
 * License: LGPL, see: LGPL.txt
 * Authors: Royce Mitchell III
 *          Baruch Rutman 2013
 *
 * Thanks to: http://www.barcodeman.com/altek/mule/scandoc.php
 * and http://win.tue.nl/~aeb/linux/kbd/scancodes-1.html
 */

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winuser.h>
#include <ndk/kbd.h>

#ifdef _MSC_VER
#ifndef WINAPI
#define WINAPI __stdcall
#endif
#endif//_MSC_VER

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

enum
{
	// vowels, etc
	SHEVA = 0x5B0,
	HATAF_SEGOL,
	HATAF_PATAH,
	HATAF_QAMATS,
	HIRIQ,
	TSERE,
	SEGOL,
	PATAH,
	QAMATS,
	HOLAM,
	QUBUTS = 0x5BB,
	DAGESH,
	MAPIQ = DAGESH,
	METEG,
	MAQAF,
	RAFE,
	PASEQ,
	SHIN_DOT,
	SIN_DOT,
	SOF_PASUQ,
	YIDDISH_DOUBLE_VAV = 0x5F0,
	YIDDISH_VAV_YUD,
	YIDDISH_DOUBLE_YUD,
	GERESH,
	GERSHAYIM,

	// consonants
	ALEF = 0x5D0,
	BET,
	GIMEL,
	DALET,
	HEY,
	VAV,
	ZAYIN,
	HET,
	TET,
	YUD,
	KAF_SOFIT,
	KAF,
	LAMED,
	MEM_SOFIT,
	MEM,
	NUN_SOFIT,
	NUN,
	SAMEKH,
	AYIN,
	PEY_SOFIT,
	PEY,
	TSADI_SOFIT,
	TSADI,
	QOF,
	RESH,
	SHIN,
	TAV,

    // symbols
    SHEQEL = 0x20AA
};

ROSDATA USHORT scancode_to_vk[] =
{
  /* Numbers Row */

  /* - 00 - */
  VK_EMPTY,     // 00: ?
  VK_ESCAPE,    // 01: esc
  '1',          // 02: '1'
  '2',          // 03: '2'
  '3',          // 04: '3'
  '4',          // 05: '4'
  '5',          // 06: '5'
  '6',          // 07: '6'
  '7',          // 08: '7'
  '8',          // 09: '8'
  '9',          // 0A: '9'
  '0',          // 0B: '0'
  VK_OEM_MINUS, // 0C: '-'
  VK_OEM_PLUS,  // OD: '+'
  VK_BACK,      // OE: backspace

  /* - 0f - */
  /* First Letters Row */
  VK_TAB,       // 0F: tab
  VK_OEM_2,     // 10: 'Q'
  VK_OEM_7,     // 11: 'W'
  'E',          // 12: 'E'
  'R',          // 13: 'R'
  'T',          // 14: 'T'
  'Y',          // 15: 'Y'
  'U',          // 16: 'U'
  'I',          // 17: 'I'
  'O',          // 18: 'O'
  'P',          // 19: 'P'
  VK_OEM_4,     // 1A: '{'
  VK_OEM_6,     // 1B: '}'
  VK_RETURN,    // 1C: enter

  /* - 1d - */
  /* Second Letters Row */
  VK_LCONTROL,  // 1D: lctrl
  'A',          // 1E: 'A'
  'S',          // 1F: 'S'
  'D',          // 20: 'D'
  'F',          // 21: 'F'
  'G',          // 22: 'G'
  'H',          // 23: 'H'
  'J',          // 24: 'J'
  'K',          // 25: 'K'
  'L',          // 26: 'L'
  VK_OEM_1,     // 27: ';'
  VK_OEM_COMMA, // 28: '\''
  VK_OEM_3,     // 29: ?
  VK_LSHIFT,    // 2A: lshift
  VK_OEM_5,     // 2B: ?

  /* - 2c - */
  /* Third letters row */
  'Z',          // 2C: 'Z'
  'X',          // 2D: 'X'
  'C',          // 2E: 'C'
  'V',          // 2F: 'V'
  'B',          // 30: 'B'
  'N',          // 31: 'N'
  'M',          // 32: 'M'
  'Q',          // 33: ','
  'W',          // 34: '.'
  VK_OEM_PERIOD,// 35: '/'
  VK_RSHIFT,    // 36: rshift
  /* - 37 - */
  /* Bottom Row */
  VK_MULTIPLY,  // 37: ?
  VK_LMENU,     // 38: lalt
  VK_SPACE,     // 39: space
  VK_CAPITAL,   // 3A: CAPLOK lock?

  /* - 3b - */
  /* F-Keys */
  VK_F1, VK_F2, VK_F3, VK_F4, VK_F5, VK_F6,
  VK_F7, VK_F8, VK_F9, VK_F10,
  /* - 45 - */
  /* Locks */
  VK_NUMLOCK | KMEXT,
  VK_SCROLL | KBDMULTIVK,
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
  VK_EMPTY,     VK_EMPTY,     VK_F11,       VK_F12,
  /* - 59 - */
  VK_CLEAR,     VK_EMPTY,     VK_EMPTY,     VK_EMPTY,     VK_EMPTY, /* EREOF */
  VK_EMPTY,     VK_EMPTY,     VK_EMPTY,     VK_EMPTY,     VK_EMPTY, /* ZOOM */
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
  3,
  { 0, 1, 2, 3 } /* Modifier bit order, NONE, SHIFT, CTRL, ALT */
};

ROSDATA VK_TO_WCHARS2 key_to_chars_2mod[] = {
  /* Normal vs Shifted */
  /* The numbers */
  { '1',         0, {'1', '!'} },
  /* Ctrl-2 generates NUL */
  { '3',         0, {'3', '#'} },
  { '4',         0, {'4', '$'} },
  { '5',         0, {'5', '%'} },
  /* Ctrl-6 generates RS */
  { '7',         0, {'7', '&'} },
  { '8',         0, {'8', '*'} },
  { '9',         0, {'9', '('} },
  { '0',         0, {'0', ')'} },

  /* Specials */
  /* Ctrl-_ generates ? */
  { VK_OEM_PLUS,    0,      {'=', '+'} },
  { VK_OEM_1,       0,      {PEY_SOFIT, ':'} },
  { 'Q',            0,      {TAV, '>'} },
  { 'W',            0,      {TSADI_SOFIT, '<'} },
  { VK_OEM_7,       CAPLOK, {'\'','W'} },
  { VK_OEM_3,       0,      {';', '~'} },
  { VK_OEM_COMMA,   0,      {',', '\"'} },
  { VK_OEM_PERIOD,  0,      {'.', '?'} },
  { VK_OEM_2,       CAPLOK, {'/', 'Q'} },
  /* Keys that do not have shift states */
  { VK_TAB,     0, {'\t','\t'} },
  { VK_ADD,     0, {'+', '+'} },
  { VK_SUBTRACT,0, {'-', '-'} },
  { VK_MULTIPLY,0, {'*', '*'} },
  { VK_DIVIDE,  0, {'/', '/'} },
  { VK_ESCAPE,  0, {0x1b,0x1b} },
  { VK_SPACE,   0, {' ', ' '} },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS3 key_to_chars_3mod[] = {
  /* Normal, Shifted, Ctrl */
  /* Legacy (telnet-style) ascii escapes */
  { VK_OEM_4, 0, {'[', '{', 0x1b /* ESC */} },
  { VK_OEM_6, 0, {']', '}', 0x1d /* GS */} },
  { VK_OEM_5, 0, {'\\','|', 0x1c /* FS */} },
  { VK_RETURN,0, {'\r', '\r', '\n'} },
  { 0,0 }
};

ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] = {
  /* Normal, Shifted, Ctrl, C-S-x */

  /* The alphabet */
  { 'A',         CAPLOK, {SHIN, 'A', 0x01, 0x01} },
  { 'B',         CAPLOK, {NUN, 'B', 0x02, 0x02} },
  { 'C',         CAPLOK, {BET, 'C', 0x03, 0x03} },
  { 'D',         CAPLOK, {GIMEL, 'D', 0x04, 0x04} },
  { 'E',         CAPLOK, {QOF, 'E', 0x05, 0x05} },
  { 'F',         CAPLOK, {KAF, 'F', 0x06, 0x06} },
  { 'G',         CAPLOK, {AYIN, 'G', 0x07, 0x07} },
  { 'H',         CAPLOK, {YUD, 'H', 0x08, 0x08} },
  { 'I',         CAPLOK, {NUN_SOFIT, 'I', 0x09, 0x09} },
  { 'J',         CAPLOK, {HET, 'J', 0x0a, 0x0a} },
  { 'K',         CAPLOK, {LAMED, 'K', 0x0b, 0x0b} },
  { 'L',         CAPLOK, {KAF_SOFIT, 'L', 0x0c, 0x0c} },
  { 'M',         CAPLOK, {TSADI, 'M', 0x0d, 0x0d} },
  { 'N',         CAPLOK, {MEM, 'N', 0x0e, 0x0e} },
  { 'O',         CAPLOK, {MEM_SOFIT, 'O', 0x0f, 0x0f} },
  { 'P',         CAPLOK, {PEY, 'P', 0x10, 0x10} },
  { VK_OEM_2,    CAPLOK, {'/', 'Q', 0x11, 0x11} },
  { 'R',         CAPLOK, {RESH, 'R', 0x12, 0x12} },
  { 'S',         CAPLOK, {DALET, 'S', 0x13, 0x13} },
  { 'T',         CAPLOK, {ALEF, 'T', 0x14, 0x14} },
  { 'U',         CAPLOK, {VAV, 'U', 0x15, 0x15} },
  { 'V',         CAPLOK, {HEY, 'V', 0x16, 0x16} },
  { VK_OEM_7,    CAPLOK, {'\'', 'W', 0x17, 0x17} },
  { 'X',         CAPLOK, {SAMEKH, 'X', 0x18, 0x18} },
  { 'Y',         CAPLOK, {TET, 'Y', 0x19, 0x19} },
  { 'Z',         CAPLOK, {ZAYIN, 'Z', 0x1a, 0x1a} },

  /* Legacy Ascii generators */
  { '2', 0, {'2', '@', WCH_NONE, 0} },
  { '6', 0, {'6', '^', WCH_NONE, 0x1e /* RS */} },
  { VK_OEM_MINUS, 0, {'-', '_', WCH_NONE, 0x1f /* US */} },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS1 keypad_numbers[] =
{
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

ROSDATA VK_TO_WCHAR_TABLE vk_to_wchar_master_table[] =
{
  vk_master(1,keypad_numbers),
  vk_master(2,key_to_chars_2mod),
  vk_master(3,key_to_chars_3mod),
  vk_master(4,key_to_chars_4mod),
  { 0,0,0 }
};

ROSDATA VSC_LPWSTR key_names[] =
{
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

ROSDATA VSC_LPWSTR extended_key_names[] =
{
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
ROSDATA KBDTABLES keyboard_layout_table =
{
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

  MAKELONG(0, 1), /* Version 1.0 */

  /* Ligatures -- The Standard Hebrew Keyboard doesn't have any */
  0,
  0,
  NULL
};

PKBDTABLES WINAPI KbdLayerDescriptor(VOID)
{
  return &keyboard_layout_table;
}

