/*
 * PROJECT:     ReactOS Keyboard Layouts
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     North Sami (Sami Extended Norway) keyboard layout
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winuser.h>
#include <ndk/kbd.h>

/* See also: https://kbdlayout.info/kbdsmsno */

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
  /* 0C */ VK_OEM_PLUS,  VK_OEM_4,     VK_BACK,      VK_TAB,
  /* 10 */ 'Q',          'W',          'E',          'R',
  /* 14 */ 'T',          'Y',          'U',          'I',
  /* 18 */ 'O',          'P',          VK_OEM_6,     VK_OEM_1,
  /* 1C */ VK_RETURN,    VK_LCONTROL,  'A',          'S',
  /* 20 */ 'D',          'F',          'G',          'H',
  /* 24 */ 'J',          'K',          'L',          VK_OEM_3,
  /* 28 */ VK_OEM_7,     VK_OEM_5,     VK_LSHIFT,    VK_OEM_2,
  /* 2C */ 'Z',          'X',          'C',          'V',
  /* 30 */ 'B',          'N',          'M',          VK_OEM_COMMA,
  /* 34 */ VK_OEM_PERIOD,VK_OEM_MINUS, VK_RSHIFT | KBDEXT,
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
/*   column: 0 = Normal, 1 = Shift, 2 = Ctrl, 3 = AltGr,              */
/*           4 = Shift+AltGr                                          */
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
      SHFT_INVALID, /* SHIFT+CTRL        */
      SHFT_INVALID, /* ALT               */
      SHFT_INVALID, /* SHIFT+ALT         */
      3,            /* CTRL+ALT (AltGr)  */
      4             /* SHIFT+CTRL+ALT    */
  }
};

/* ------------------------------------------------------------------ */
/* VK -> WCHAR tables                                                 */
/* ------------------------------------------------------------------ */

/* Normal, Shift */
ROSDATA VK_TO_WCHARS2 key_to_chars_2mod[] = {
  { VK_TAB, 0, {0x09, 0x09} },
  { VK_ADD, 0, {'+', '+'} },
  { VK_DIVIDE, 0, {'/', '/'} },
  { VK_MULTIPLY, 0, {'*', '*'} },
  { VK_SUBTRACT, 0, {'-', '-'} },
  { 0, 0 }
};

/* Normal, Shift, Ctrl */
ROSDATA VK_TO_WCHARS3 key_to_chars_3mod[] = {
  { VK_BACK, 0, {0x08, 0x08, 0x7f} },
  { VK_ESCAPE, 0, {0x1b, 0x1b, 0x1b} },
  { VK_RETURN, 0, {0x0d, 0x0d, 0x0a} },
  { VK_CANCEL, 0, {0x03, 0x03, 0x03} },
  { 0, 0 }
};

/* Normal, Shift, Ctrl, AltGr, Shift+AltGr
 * VK_EMPTY rows hold the dead-key characters that follow a WCH_DEAD entry. */
ROSDATA VK_TO_WCHARS5 key_to_chars_5mod[] = {
  { '1', 0, {'1', '!', WCH_NONE, WCH_NONE, WCH_NONE} },
  { '2', 0, {'2', '\"', WCH_NONE, '@', WCH_NONE} },
  { '3', 0, {'3', '#', WCH_NONE, 0x00A3 /* pound sign */, WCH_NONE} },
  { '4', 0, {'4', 0x00A4 /* currency sign */, WCH_NONE, '$', WCH_NONE} },
  { '5', 0, {'5', '%', WCH_NONE, 0x20AC /* euro sign */, WCH_NONE} },
  { '6', 0, {'6', '&', WCH_NONE, WCH_NONE, WCH_NONE} },
  { '7', 0, {'7', '/', WCH_NONE, '{', WCH_NONE} },
  { '8', 0, {'8', '(', WCH_NONE, '[', WCH_NONE} },
  { '9', 0, {'9', ')', WCH_NONE, ']', WCH_NONE} },
  { '0', 0, {'0', '=', WCH_NONE, '}', WCH_NONE} },
  { VK_OEM_PLUS, 0, {'+', '?', WCH_NONE, WCH_NONE, WCH_NONE} },
  { VK_OEM_4, 0, {'\\', WCH_DEAD, WCH_NONE, WCH_DEAD, WCH_NONE} },
  { VK_EMPTY, 0, {WCH_NONE, '`', WCH_NONE, 0x00B4 /* acute accent */, WCH_NONE} },
  { 'Q', CAPLOK|CAPLOKALTGR, {0x00E1 /* small a with acute */, 0x00C1 /* capital a with acute */, WCH_NONE, 'q', 'Q'} },
  { 'W', CAPLOK|CAPLOKALTGR, {0x0161 /* small s with caron */, 0x0160 /* capital s with caron */, WCH_NONE, 'w', 'W'} },
  { 'E', CAPLOK, {'e', 'E', WCH_NONE, 0x20AC /* euro sign */, WCH_NONE} },
  { 'R', CAPLOK, {'r', 'R', WCH_NONE, WCH_NONE, WCH_NONE} },
  { 'T', CAPLOK, {'t', 'T', WCH_NONE, WCH_NONE, WCH_NONE} },
  { 'Y', CAPLOK|CAPLOKALTGR, {0x0167 /* small t with stroke */, 0x0166 /* capital t with stroke */, WCH_NONE, 'y', 'Y'} },
  { 'U', CAPLOK, {'u', 'U', WCH_NONE, WCH_NONE, WCH_NONE} },
  { 'I', CAPLOK|CAPLOKALTGR, {'i', 'I', WCH_NONE, 0x00EF /* small i with diaeresis */, 0x00CF /* capital i with diaeresis */} },
  { 'O', CAPLOK|CAPLOKALTGR, {'o', 'O', WCH_NONE, 0x00F5 /* small o with tilde */, 0x00D5 /* capital o with tilde */} },
  { 'P', CAPLOK, {'p', 'P', WCH_NONE, WCH_NONE, WCH_NONE} },
  { VK_OEM_6, CAPLOK, {0x00E5 /* small a with ring above */, 0x00C5 /* capital a with ring above */, WCH_NONE, WCH_DEAD, WCH_DEAD} },
  { VK_EMPTY, 0, {WCH_NONE, WCH_NONE, WCH_NONE, 0x00A8 /* diaeresis */, '^'} },
  { VK_OEM_1, CAPLOK, {0x014B /* small eng */, 0x014A /* capital eng */, WCH_NONE, WCH_DEAD, WCH_DEAD} },
  { VK_EMPTY, 0, {WCH_NONE, WCH_NONE, WCH_NONE, '~', 0x02C7 /* caron */} },
  { 'A', CAPLOK|CAPLOKALTGR, {'a', 'A', WCH_NONE, 0x00E2 /* small a with circumflex */, 0x00C2 /* capital a with circumflex */} },
  { 'S', CAPLOK, {'s', 'S', WCH_NONE, WCH_NONE, WCH_NONE} },
  { 'D', CAPLOK, {'d', 'D', WCH_NONE, WCH_NONE, WCH_NONE} },
  { 'F', CAPLOK, {'f', 'F', WCH_NONE, WCH_NONE, WCH_NONE} },
  { 'G', CAPLOK|CAPLOKALTGR, {'g', 'G', WCH_NONE, 0x01E7 /* small g with caron */, 0x01E6 /* capital g with caron */} },
  { 'H', CAPLOK|CAPLOKALTGR, {'h', 'H', WCH_NONE, 0x01E5 /* small g with stroke */, 0x01E4 /* capital g with stroke */} },
  { 'J', CAPLOK, {'j', 'J', WCH_NONE, WCH_NONE, WCH_NONE} },
  { 'K', CAPLOK|CAPLOKALTGR, {'k', 'K', WCH_NONE, 0x01E9 /* small k with caron */, 0x01E8 /* capital k with caron */} },
  { 'L', CAPLOK, {'l', 'L', WCH_NONE, WCH_NONE, WCH_NONE} },
  { VK_OEM_3, CAPLOK|CAPLOKALTGR, {0x00F8 /* small o with stroke */, 0x00D8 /* capital o with stroke */, WCH_NONE, 0x00F6 /* small o with diaeresis */, 0x00D6 /* capital o with diaeresis */} },
  { VK_OEM_7, CAPLOK|CAPLOKALTGR, {0x00E6 /* small ae */, 0x00C6 /* capital ae */, WCH_NONE, 0x00E4 /* small a with diaeresis */, 0x00C4 /* capital a with diaeresis */} },
  { VK_OEM_5, 0, {'|', 0x00A7 /* section sign */, WCH_NONE, WCH_NONE, WCH_NONE} },
  { VK_OEM_2, CAPLOK, {0x0111 /* small d with stroke */, 0x0110 /* capital d with stroke */, WCH_NONE, '\'', '*'} },
  { 'Z', CAPLOK|CAPLOKALTGR, {'z', 'Z', WCH_NONE, 0x0292 /* small ezh */, 0x01B7 /* capital ezh */} },
  { 'X', CAPLOK|CAPLOKALTGR, {0x010D /* small c with caron */, 0x010C /* capital c with caron */, WCH_NONE, 'x', 'X'} },
  { 'C', CAPLOK, {'c', 'C', WCH_NONE, WCH_NONE, WCH_NONE} },
  { 'V', CAPLOK, {'v', 'V', WCH_NONE, WCH_NONE, WCH_NONE} },
  { 'B', CAPLOK, {'b', 'B', WCH_NONE, WCH_NONE, WCH_NONE} },
  { 'N', CAPLOK, {'n', 'N', WCH_NONE, WCH_NONE, WCH_NONE} },
  { 'M', CAPLOK, {'m', 'M', WCH_NONE, 0x00B5 /* micro sign */, WCH_NONE} },
  { VK_OEM_COMMA, 0, {',', ';', WCH_NONE, '<', WCH_NONE} },
  { VK_OEM_PERIOD, 0, {'.', ':', WCH_NONE, '>', WCH_NONE} },
  { VK_OEM_MINUS, 0, {'-', '_', WCH_NONE, WCH_NONE, WCH_NONE} },
  { VK_SPACE, 0, {' ', ' ', ' ', WCH_NONE, WCH_NONE} },
  { VK_OEM_102, CAPLOK|CAPLOKALTGR, {0x017E /* small z with caron */, 0x017D /* capital z with caron */, WCH_NONE, 0x01EF /* small ezh with caron */, 0x01EE /* capital ezh with caron */} },
  { VK_DECIMAL, 0, {',', ',', WCH_NONE, WCH_NONE, WCH_NONE} },
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

/* Order in the binary: 3, 5, 2, 1 */
ROSDATA VK_TO_WCHAR_TABLE vk_to_wchar_master_table[] = {
  vk_master(3, key_to_chars_3mod),
  vk_master(5, key_to_chars_5mod),
  vk_master(2, key_to_chars_2mod),
  vk_master(1, keypad_numbers),
  { 0, 0, 0 }
};

/* ------------------------------------------------------------------ */
/* Dead keys: ` (grave), acute, diaeresis, ^ (circumflex), ~ (tilde), */
/*            caron                                                   */
/* ------------------------------------------------------------------ */
#define DEADTRANS(ch, dead, comp, flags) { MAKELONG(ch, dead), comp, flags }

ROSDATA DEADKEY dead_keys[] = {

  /* dead key '`' */
  DEADTRANS('a', '`', 0x00E0, 0),  /* small a with grave */
  DEADTRANS('A', '`', 0x00C0, 0),  /* capital a with grave */
  DEADTRANS('e', '`', 0x00E8, 0),  /* small e with grave */
  DEADTRANS('E', '`', 0x00C8, 0),  /* capital e with grave */
  DEADTRANS('i', '`', 0x00EC, 0),  /* small i with grave */
  DEADTRANS('I', '`', 0x00CC, 0),  /* capital i with grave */
  DEADTRANS('o', '`', 0x00F2, 0),  /* small o with grave */
  DEADTRANS('O', '`', 0x00D2, 0),  /* capital o with grave */
  DEADTRANS('u', '`', 0x00F9, 0),  /* small u with grave */
  DEADTRANS('U', '`', 0x00D9, 0),  /* capital u with grave */
  DEADTRANS('w', '`', 0x1E81, 0),  /* small w with grave */
  DEADTRANS('W', '`', 0x1E80, 0),  /* capital w with grave */
  DEADTRANS('y', '`', 0x1EF3, 0),  /* small y with grave */
  DEADTRANS('Y', '`', 0x1EF2, 0),  /* capital y with grave */
  DEADTRANS(' ', '`', '`', 0),

  /* dead key 0x00B4 */
  DEADTRANS('a', 0x00B4, 0x00E1, 0),  /* small a with acute */
  DEADTRANS('A', 0x00B4, 0x00C1, 0),  /* capital a with acute */
  DEADTRANS('c', 0x00B4, 0x0107, 0),  /* small c with acute */
  DEADTRANS('C', 0x00B4, 0x0106, 0),  /* capital c with acute */
  DEADTRANS('e', 0x00B4, 0x00E9, 0),  /* small e with acute */
  DEADTRANS('E', 0x00B4, 0x00C9, 0),  /* capital e with acute */
  DEADTRANS('i', 0x00B4, 0x00ED, 0),  /* small i with acute */
  DEADTRANS('I', 0x00B4, 0x00CD, 0),  /* capital i with acute */
  DEADTRANS('l', 0x00B4, 0x013A, 0),  /* small l with acute */
  DEADTRANS('L', 0x00B4, 0x0139, 0),  /* capital l with acute */
  DEADTRANS('n', 0x00B4, 0x0144, 0),  /* small n with acute */
  DEADTRANS('N', 0x00B4, 0x0143, 0),  /* capital n with acute */
  DEADTRANS('o', 0x00B4, 0x00F3, 0),  /* small o with acute */
  DEADTRANS('O', 0x00B4, 0x00D3, 0),  /* capital o with acute */
  DEADTRANS('r', 0x00B4, 0x0155, 0),  /* small r with acute */
  DEADTRANS('R', 0x00B4, 0x0154, 0),  /* capital r with acute */
  DEADTRANS('s', 0x00B4, 0x015B, 0),  /* small s with acute */
  DEADTRANS('S', 0x00B4, 0x015A, 0),  /* capital s with acute */
  DEADTRANS('u', 0x00B4, 0x00FA, 0),  /* small u with acute */
  DEADTRANS('U', 0x00B4, 0x00DA, 0),  /* capital u with acute */
  DEADTRANS('w', 0x00B4, 0x1E83, 0),  /* small w with acute */
  DEADTRANS('W', 0x00B4, 0x1E82, 0),  /* capital w with acute */
  DEADTRANS('y', 0x00B4, 0x00FD, 0),  /* small y with acute */
  DEADTRANS('Y', 0x00B4, 0x00DD, 0),  /* capital y with acute */
  DEADTRANS('z', 0x00B4, 0x017A, 0),  /* small z with acute */
  DEADTRANS('Z', 0x00B4, 0x0179, 0),  /* capital z with acute */
  DEADTRANS(0x00E5, 0x00B4, 0x01FB, 0),  /* small a with ring above and acute */
  DEADTRANS(0x00C5, 0x00B4, 0x01FA, 0),  /* capital a with ring above and acute */
  DEADTRANS(0x00E6, 0x00B4, 0x01FD, 0),  /* small ae with acute */
  DEADTRANS(0x00C6, 0x00B4, 0x01FC, 0),  /* capital ae with acute */
  DEADTRANS(0x00F8, 0x00B4, 0x01FF, 0),  /* small o with stroke and acute */
  DEADTRANS(0x00D8, 0x00B4, 0x01FE, 0),  /* capital o with stroke and acute */
  DEADTRANS(' ', 0x00B4, 0x00B4, 0),  /* acute accent */

  /* dead key 0x00A8 */
  DEADTRANS('a', 0x00A8, 0x00E4, 0),  /* small a with diaeresis */
  DEADTRANS('A', 0x00A8, 0x00C4, 0),  /* capital a with diaeresis */
  DEADTRANS('e', 0x00A8, 0x00EB, 0),  /* small e with diaeresis */
  DEADTRANS('E', 0x00A8, 0x00CB, 0),  /* capital e with diaeresis */
  DEADTRANS('i', 0x00A8, 0x00EF, 0),  /* small i with diaeresis */
  DEADTRANS('I', 0x00A8, 0x00CF, 0),  /* capital i with diaeresis */
  DEADTRANS('o', 0x00A8, 0x00F6, 0),  /* small o with diaeresis */
  DEADTRANS('O', 0x00A8, 0x00D6, 0),  /* capital o with diaeresis */
  DEADTRANS('u', 0x00A8, 0x00FC, 0),  /* small u with diaeresis */
  DEADTRANS('U', 0x00A8, 0x00DC, 0),  /* capital u with diaeresis */
  DEADTRANS('w', 0x00A8, 0x1E85, 0),  /* small w with diaeresis */
  DEADTRANS('W', 0x00A8, 0x1E84, 0),  /* capital w with diaeresis */
  DEADTRANS('y', 0x00A8, 0x00FF, 0),  /* small y with diaeresis */
  DEADTRANS('Y', 0x00A8, 0x0178, 0),  /* capital y with diaeresis */
  DEADTRANS(' ', 0x00A8, 0x00A8, 0),  /* diaeresis */

  /* dead key '^' */
  DEADTRANS('a', '^', 0x00E2, 0),  /* small a with circumflex */
  DEADTRANS('A', '^', 0x00C2, 0),  /* capital a with circumflex */
  DEADTRANS('c', '^', 0x0109, 0),  /* small c with circumflex */
  DEADTRANS('C', '^', 0x0108, 0),  /* capital c with circumflex */
  DEADTRANS('e', '^', 0x00EA, 0),  /* small e with circumflex */
  DEADTRANS('E', '^', 0x00CA, 0),  /* capital e with circumflex */
  DEADTRANS('g', '^', 0x011D, 0),  /* small g with circumflex */
  DEADTRANS('G', '^', 0x011C, 0),  /* capital g with circumflex */
  DEADTRANS('h', '^', 0x0125, 0),  /* small h with circumflex */
  DEADTRANS('H', '^', 0x0124, 0),  /* capital h with circumflex */
  DEADTRANS('i', '^', 0x00EE, 0),  /* small i with circumflex */
  DEADTRANS('I', '^', 0x00CE, 0),  /* capital i with circumflex */
  DEADTRANS('j', '^', 0x0135, 0),  /* small j with circumflex */
  DEADTRANS('J', '^', 0x0134, 0),  /* capital j with circumflex */
  DEADTRANS('o', '^', 0x00F4, 0),  /* small o with circumflex */
  DEADTRANS('O', '^', 0x00D4, 0),  /* capital o with circumflex */
  DEADTRANS('s', '^', 0x015D, 0),  /* small s with circumflex */
  DEADTRANS('S', '^', 0x015C, 0),  /* capital s with circumflex */
  DEADTRANS('u', '^', 0x00FB, 0),  /* small u with circumflex */
  DEADTRANS('U', '^', 0x00DB, 0),  /* capital u with circumflex */
  DEADTRANS('w', '^', 0x0175, 0),  /* small w with circumflex */
  DEADTRANS('W', '^', 0x0174, 0),  /* capital w with circumflex */
  DEADTRANS('y', '^', 0x0177, 0),  /* small y with circumflex */
  DEADTRANS('Y', '^', 0x0176, 0),  /* capital y with circumflex */
  DEADTRANS(' ', '^', '^', 0),

  /* dead key '~' */
  DEADTRANS('a', '~', 0x00E3, 0),  /* small a with tilde */
  DEADTRANS('A', '~', 0x00C3, 0),  /* capital a with tilde */
  DEADTRANS('i', '~', 0x0129, 0),  /* small i with tilde */
  DEADTRANS('I', '~', 0x0128, 0),  /* capital i with tilde */
  DEADTRANS('n', '~', 0x00F1, 0),  /* small n with tilde */
  DEADTRANS('N', '~', 0x00D1, 0),  /* capital n with tilde */
  DEADTRANS('o', '~', 0x00F5, 0),  /* small o with tilde */
  DEADTRANS('O', '~', 0x00D5, 0),  /* capital o with tilde */
  DEADTRANS('u', '~', 0x0169, 0),  /* small u with tilde */
  DEADTRANS('U', '~', 0x0168, 0),  /* capital u with tilde */
  DEADTRANS(' ', '~', '~', 0),

  /* dead key 0x02C7 */
  DEADTRANS('c', 0x02C7, 0x010D, 0),  /* small c with caron */
  DEADTRANS('C', 0x02C7, 0x010C, 0),  /* capital c with caron */
  DEADTRANS('d', 0x02C7, 0x010F, 0),  /* small d with caron */
  DEADTRANS('D', 0x02C7, 0x010E, 0),  /* capital d with caron */
  DEADTRANS('e', 0x02C7, 0x011B, 0),  /* small e with caron */
  DEADTRANS('E', 0x02C7, 0x011A, 0),  /* capital e with caron */
  DEADTRANS('g', 0x02C7, 0x01E7, 0),  /* small g with caron */
  DEADTRANS('G', 0x02C7, 0x01E6, 0),  /* capital g with caron */
  DEADTRANS('h', 0x02C7, 0x021F, 0),  /* small h with caron */
  DEADTRANS('H', 0x02C7, 0x021E, 0),  /* capital h with caron */
  DEADTRANS('k', 0x02C7, 0x01E9, 0),  /* small k with caron */
  DEADTRANS('K', 0x02C7, 0x01E8, 0),  /* capital k with caron */
  DEADTRANS('l', 0x02C7, 0x013E, 0),  /* small l with caron */
  DEADTRANS('L', 0x02C7, 0x013D, 0),  /* capital l with caron */
  DEADTRANS('n', 0x02C7, 0x0148, 0),  /* small n with caron */
  DEADTRANS('N', 0x02C7, 0x0147, 0),  /* capital n with caron */
  DEADTRANS('r', 0x02C7, 0x0159, 0),  /* small r with caron */
  DEADTRANS('R', 0x02C7, 0x0158, 0),  /* capital r with caron */
  DEADTRANS('s', 0x02C7, 0x0161, 0),  /* small s with caron */
  DEADTRANS('S', 0x02C7, 0x0160, 0),  /* capital s with caron */
  DEADTRANS('t', 0x02C7, 0x0165, 0),  /* small t with caron */
  DEADTRANS('T', 0x02C7, 0x0164, 0),  /* capital t with caron */
  DEADTRANS('z', 0x02C7, 0x017E, 0),  /* small z with caron */
  DEADTRANS('Z', 0x02C7, 0x017D, 0),  /* capital z with caron */
  DEADTRANS(0x0292, 0x02C7, 0x01EF, 0),  /* small ezh with caron */
  DEADTRANS(0x01B7, 0x02C7, 0x01EE, 0),  /* capital ezh with caron */
  DEADTRANS(' ', 0x02C7, 0x02C7, 0),  /* caron */
  { 0, 0, 0 }
};

ROSDATA LPWSTR dead_key_names[] = {
  L"`GRAVE ACCENT",
  L"\x00B4" L"ACUTE ACCENT",
  L"\x00A8" L"DIAERESIS",
  L"^CIRCUMFLEX ACCENT",
  L"~TILDE",
  L"\x02C7" L"CARON",
  NULL
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
  { 0x1c, L"Num Enter" },  { 0x1d, L"Right Ctrl" },
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
  dead_keys,
  (VSC_LPWSTR *)key_names,
  (VSC_LPWSTR *)extended_key_names,
  (DEADKEY_LPWSTR *)dead_key_names,
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
