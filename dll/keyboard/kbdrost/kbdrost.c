// This file is converted by code7bit.
// code7bit: https://github.com/katahiromz/code7bit
// To revert conversion, please execute "code7bit -r <file>".
/*
 * ReactOS Romanian (Standard) Keyboard layout
 * Copyright (C) 2018 ReactOS
 * Author: Ștefan Fulea (stefan dot fulea at mail dot com)
 * License: LGPL, see: LGPL.txt
 *
 * Thanks to: http://www.barcodeman.com/altek/mule/scandoc.php
 * and http://win.tue.nl/~aeb/linux/kbd/scancodes-1.html
 *
 * Romanian standard layout as defined in SR 13392:2004 by
 * Romanian Standards Association (ASRO), with enacted use in
 * romanian public institutions through Order no. 414/25.09.2006
 * http://legislatie.just.ro/Public/FormaPrintabila/00000G22CFKNTV3ZAR52IRPL5V03Y1CD
 *
 * The combining characters, non-romanian diacritics and other
 * symbols were taken as reference from Microsoft's demo layout:
 * https://docs.microsoft.com/en-us/globalization/keyboards/kbdrost.html
 *
 * Finally, considering the amount of combining characters unused in Romanian
 * but present by default in this language's keyboard layouts, I've took the
 * liberty to add one combining character that it may use but I haven't
 * encountered as of the time of this writing. The combining comma below,
 * provides an alternative way of generating romanian S-comma and T-comma
 * characters and is set as a dead character under letter Q.
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

ROSDATA USHORT scancode_to_vk[] =
{
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
  VK_OEM_PERIOD,VK_OEM_2,     VK_RSHIFT,

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
  VK_EMPTY,     VK_OEM_102,     VK_F11,       VK_F12,

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

ROSDATA VSC_VK extcode0_to_vk[] =
{
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

ROSDATA VSC_VK extcode1_to_vk[] =
{
  { 0x1d, VK_PAUSE },
  { 0, 0 }
};

ROSDATA VK_TO_BIT modifier_keys[] =
{
  { VK_SHIFT,   KBDSHIFT },
  { VK_CONTROL, KBDCTRL },
  { VK_MENU,    KBDALT },
  { 0,          0 }
};

ROSDATA MODIFIERS modifier_bits =
{
  modifier_keys,
   7,
   { 0, 1, 2, SHFT_INVALID, SHFT_INVALID, SHFT_INVALID, 3, 4 }
   /* Modifiers order: NONE, SHIFT, CTRL, ALTGR, SHIFT-ALTGR */
};

ROSDATA VK_TO_WCHARS2 key_to_chars_2mod[] =
{
  /* The alphabet */
  { 'A', CAPLOK, {'a', 'A'} },
  { 'B', CAPLOK, {'b', 'B'} },
  { 'C', CAPLOK, {'c', 'C'} },
  { 'F', CAPLOK, {'f', 'F'} },
  { 'G', CAPLOK, {'g', 'G'} },
  { 'H', CAPLOK, {'h', 'H'} },
  { 'I', CAPLOK, {'i', 'I'} },
  { 'J', CAPLOK, {'j', 'J'} },
  { 'K', CAPLOK, {'k', 'K'} },
  { 'M', CAPLOK, {'m', 'M'} },
  { 'N', CAPLOK, {'n', 'N'} },
  { 'O', CAPLOK, {'o', 'O'} },
  { 'R', CAPLOK, {'r', 'R'} },
  { 'T', CAPLOK, {'t', 'T'} },
  { 'U', CAPLOK, {'u', 'U'} },
  { 'V', CAPLOK, {'v', 'V'} },
  { 'W', CAPLOK, {'w', 'W'} },
  { 'X', CAPLOK, {'x', 'X'} },
  { 'Y', CAPLOK, {'y', 'Y'} },
  { 'Z', CAPLOK, {'z', 'Z'} },

  /* Specials */
  { VK_OEM_2, 0, {'/', '?'} },

  /* Keys that do not have shift states */
  { VK_TAB,      0, {'\t','\t'} },
  { VK_ADD,      0, {'+', '+'} },
  { VK_SUBTRACT, 0, {'-', '-'} },
  { VK_MULTIPLY, 0, {'*', '*'} },
  { VK_DIVIDE,   0, {'/', '/'} },

  { 0, 0 }
};

ROSDATA VK_TO_WCHARS3 key_to_chars_3mod[] =
{
  /* Legacy (telnet-style) ascii escapes */
  { VK_OEM_102, 0, {'\\','|', 0x1c /* FS */} },
  { VK_BACK,    0, {0x8, 0x8, 0x7F } },
  { VK_ESCAPE,  0, {0x1b,0x1b,0x1b } },
  { VK_RETURN,  0, {'\r','\r','\n'} },
  { VK_SPACE,   0, {' ', ' ', ' '} },
  { VK_CANCEL,  0, {0x3, 0x3, 0x3} },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] =
{
  { '1', 0, {'1', '!', WCH_NONE, 0x0303 /* COMBINING TILDE */} },
  { '2', 0, {'2', '@', WCH_NONE, 0x030C /* COMBINING CARON */} },
  { '3', 0, {'3', '#', WCH_NONE, 0x0302 /* COMBINING CIRCUMFLEX */} },
  { '4', 0, {'4', '$', WCH_NONE, 0x0306 /* COMBINING BREVE */} },
  { '5', 0, {'5', '%', WCH_NONE, 0x030A /* COMBINING RING ABOVE */} },
  { '6', 0, {'6', '^', WCH_NONE, 0x0328 /* COMBINING OGONEK */} },
  { '7', 0, {'7', '&', WCH_NONE, 0x0300 /* COMBINING GRAVE ACCENT */} },
  { '8', 0, {'8', '*', WCH_NONE, 0x0307 /* COMBINING DOT ABOVE */} },
  { '9', 0, {'9', '(', WCH_NONE, 0x0301 /* COMBINING ACUTE ACCENT */} },
  { '0', 0, {'0', ')', WCH_NONE, 0x030B /* COMBINING DOUBLE ACUTE ACCENT */} },
  { 'Q', CAPLOK, {'q', 'Q', WCH_NONE, 0x0326 /* COMBINING COMMA BELOW */} },
  { 'E', CAPLOK, {'e', 'E', WCH_NONE, 0x20AC /* EURO SIGN */} },
  { 'P', CAPLOK, {'p', 'P', WCH_NONE, 0x00A7 /* SECTION SIGN */} },
  { 'S', CAPLOK, {'s', 'S', WCH_NONE, 0x00DF /* LATIN SMALL LETTER SHARP S */} },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS5 key_to_chars_5mod[] =
{
  { VK_OEM_3, 0,  {0x201E /* DOUBLE QUOTE 99 LOW */, 0x201D /* DOUBLE QUOTE 99 HIGH */, WCH_NONE, '`', '~' } },
  { VK_OEM_MINUS, 0, {'-', '_', WCH_NONE, 0x0308 /* COMBINING DIAERESIS */, 0x2013 /* EN DASH */} },
  { VK_OEM_PLUS, 0, {'=', '+', WCH_NONE, 0x0327 /* COMBINING CEDILLA */, 0x00B1 /* PLUS-MINUS SIGN */} },
  { VK_OEM_4, CAPLOK, {0x0103 /* a BREVE */, 0x0102 /* A BREVE */, 0x1b /* ESC */, '[', '{' } },
  { VK_OEM_6, CAPLOK, {0x00ee /* i CIRCUMFLEX */, 0x00ce /* I CIRCUMFLEX */, 0x1d /* GS */, ']', '}' } },
  { 'D', CAPLOK, {'d', 'D', WCH_NONE, 0x0111 /* d WITH STROKE */, 0x0110 /* D WITH STROKE */} },
  { 'L', CAPLOK, {'l', 'L', WCH_NONE, 0x0142 /* l WITH STROKE */, 0x0141 /* L WITH STROKE */} },
  { VK_OEM_1, CAPLOK, {0x0219 /* s COMMA */, 0x0218 /* S COMMA */, WCH_NONE, ';', ':' } },
  { VK_OEM_7, CAPLOK, {0x021B /* t COMMA */, 0x021A /* T COMMA */, WCH_NONE, '\'', '"' } },
  { VK_OEM_5, CAPLOK, {0x00e2 /* a CIRCUMFLEX */, 0x00c2 /* A CIRCUMFLEX */, 0x1c /* FS */, '\\', '|' } },
  { VK_OEM_COMMA, 0, {',', ';', WCH_NONE, '<', 0x00AB /* LEFT-POINTING DOUBLE ANGLE QUOTATION MARK */ } },
  { VK_OEM_PERIOD, 0, {'.', ':', WCH_NONE, '>', 0x00BB /* RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK */ } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS1 keypad_numbers[] =
{
  { VK_NUMPAD0,  0, {'0'} },
  { VK_NUMPAD1,  0, {'1'} },
  { VK_NUMPAD2,  0, {'2'} },
  { VK_NUMPAD3,  0, {'3'} },
  { VK_NUMPAD4,  0, {'4'} },
  { VK_NUMPAD5,  0, {'5'} },
  { VK_NUMPAD6,  0, {'6'} },
  { VK_NUMPAD7,  0, {'7'} },
  { VK_NUMPAD8,  0, {'8'} },
  { VK_NUMPAD9,  0, {'9'} },
  { VK_DECIMAL,  0, {','} },
  { 0, 0 }
};

#define vk_master(n, x) { (PVK_TO_WCHARS1)x, n, sizeof(x[0]) }
ROSDATA VK_TO_WCHAR_TABLE vk_to_wchar_master_table[] =
{
  vk_master(2, key_to_chars_2mod),
  vk_master(3, key_to_chars_3mod),
  vk_master(4, key_to_chars_4mod),
  vk_master(5, key_to_chars_5mod),
  vk_master(1, keypad_numbers),
  { 0, 0, 0 }
};

ROSDATA VSC_LPWSTR key_names[] =
{
  { 0x01, L"Esc" },
  { 0x0e, L"Retro\u0219tergere" },
  { 0x0f, L"Tabulator" },
  { 0x1c, L"Intrare" },
  { 0x1d, L"Control" },
  { 0x2a, L"Schimb" },
  { 0x35, L"/ (num.)" },
  { 0x36, L"Schimb dreapta" },
  { 0x37, L"* (num.)" },
  { 0x38, L"Alt" },
  { 0x39, L"Spa\u021Biu" },
  { 0x3a, L"MAJUSCULE" },
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
  { 0x45, L"Pauz\u0103" },
  { 0x46, L"Fix. derulare" },
  { 0x47, L"7 (num.)" },
  { 0x48, L"8 (num.)" },
  { 0x49, L"9 (num.)" },
  { 0x4a, L"- (num.)" },
  { 0x4b, L"4 (num.)" },
  { 0x4c, L"5 (num.)" },
  { 0x4d, L"6 (num.)" },
  { 0x4e, L"+ (num.)" },
  { 0x4f, L"1 (num.)" },
  { 0x50, L"2 (num.)" },
  { 0x51, L"3 (num.)" },
  { 0x52, L"0 (num.)" },
  { 0x53, L"Sep. zecimal" },
  { 0x54, L"Sys Req" },
  { 0x56, L"\\" },
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

ROSDATA VSC_LPWSTR extended_key_names[] =
{
  { 0x1c, L"Intr. numerice" },
  { 0x1d, L"Control dreapta" },
  { 0x37, L"Imprimare" },
  { 0x38, L"Alt dreapta" },
  { 0x45, L"Fix. numerice" },
  { 0x46, L"\u00CEntrerupere" },
  { 0x47, L"Acas\u0103" },
  { 0x48, L"Sus" },
  { 0x49, L"Pag. sus" },
  { 0x4b, L"St\u00E2nga" },
  { 0x4c, L"Centru" },
  { 0x4d, L"Dreapta" },
  { 0x4f, L"Sf\u00E2r\u0219it" },
  { 0x50, L"Jos" },
  { 0x51, L"Pag. jos" },
  { 0x52, L"Inser\u021Bie" },
  { 0x53, L"\u0218tergere" },
  { 0x54, L"Meniu" },
  { 0x56, L"Ajutor" },
  { 0x5b, L"Meniu st\u00E2nga" },
  { 0x5c, L"Meniu dreapta" },
  { 0, NULL }
};

ROSDATA DEADKEY_LPWSTR dead_key_names[] =
{
    L"\x0300"   L"Accent grav",
    L"\x0301"   L"Accent acut",
    L"\x0302"   L"Circumflex",
    L"\x0303"   L"Tild\u0103",
    L"\x0306"   L"Breve",
    L"\x0307"   L"Suprapunct",
    L"\x0308"   L"Trem\u0103",
    L"\x030A"   L"Supracerc",
    L"\x030B"   L"Accent acut dublu",
    L"\x030C"   L"Caron",
    L"\x0326"   L"Virgul\u0103",
    L"\x0327"   L"Sedil\u0103",
    L"\x0328"   L"Ogonek",
    NULL
};

#define DEADTRANS(ch, accent, comp, flags) MAKELONG(ch, accent), comp, flags
ROSDATA DEADKEY dead_key[] =
{
  /*
   * combining diacritics: tilde, caron, circumflex,
   * breve, ring above, ogonek, grave accent, dot above,
   * acute accent, double acute accent, and comma below
   */
  #include "deadkeys/tilde"
  #include "deadkeys/caron"
  #include "deadkeys/circumflex"
  #include "deadkeys/breve"
  #include "deadkeys/ring_above"
  #include "deadkeys/ogonek"
  #include "deadkeys/grave_acc"
  #include "deadkeys/dot_above"
  #include "deadkeys/acute_acc"
  #include "deadkeys/dbl_acute_acc"
  #include "deadkeys/comma_below"

  { 0, 0 }
};

/* Finally, the master table */
ROSDATA KBDTABLES keyboard_layout_table =
{
  /* modifier assignments */
  &modifier_bits,

  /* character from vk tables */
  vk_to_wchar_master_table,

  /* diacritical marks */
  dead_key,

  /* Key names */
  key_names,
  extended_key_names,
  dead_key_names,

  /* scan code to virtual key maps */
  scancode_to_vk,
  sizeof(scancode_to_vk) / sizeof(scancode_to_vk[0]),
  extcode0_to_vk,
  extcode1_to_vk,

  MAKELONG(KLLF_ALTGR, 1), /* Version 1.0 */

  /* Ligatures (Romanian doesn't have any) */
  0,
  0,
  NULL
};

PKBDTABLES WINAPI KbdLayerDescriptor(VOID)
{
  return &keyboard_layout_table;
}
