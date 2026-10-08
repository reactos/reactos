/*
 * PROJECT:     ReactOS Keyboard Layouts
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Faroese (fo-FO) keyboard layout
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winuser.h>
#include <ndk/kbd.h>

/* See also: https://kbdlayout.info/kbdfo */

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

#define VK_EMPTY 0xff /* The non-existent VK */
#define KNUMS    (KBDNUMPAD | KBDSPECIAL) /* Special + number pad */
#define KMEXT    (KBDEXT | KBDMULTIVK)    /* Multi + ext */
#define DEADTRANS(ch, accent, comp, flags) { MAKELONG(ch, accent), comp, flags }

ROSDATA USHORT scancode_to_vk[] = {
  /* 00 */ VK_EMPTY,
  /* 01 */ VK_ESCAPE,
  /* 02 */ '1',
  /* 03 */ '2',
  /* 04 */ '3',
  /* 05 */ '4',
  /* 06 */ '5',
  /* 07 */ '6',
  /* 08 */ '7',
  /* 09 */ '8',
  /* 0a */ '9',
  /* 0b */ '0',
  /* 0c */ VK_OEM_PLUS,
  /* 0d */ VK_OEM_4,
  /* 0e */ VK_BACK,
  /* 0f */ VK_TAB,
  /* 10 */ 'Q',
  /* 11 */ 'W',
  /* 12 */ 'E',
  /* 13 */ 'R',
  /* 14 */ 'T',
  /* 15 */ 'Y',
  /* 16 */ 'U',
  /* 17 */ 'I',
  /* 18 */ 'O',
  /* 19 */ 'P',
  /* 1a */ VK_OEM_6,
  /* 1b */ VK_OEM_1,
  /* 1c */ VK_RETURN,
  /* 1d */ VK_LCONTROL,
  /* 1e */ 'A',
  /* 1f */ 'S',
  /* 20 */ 'D',
  /* 21 */ 'F',
  /* 22 */ 'G',
  /* 23 */ 'H',
  /* 24 */ 'J',
  /* 25 */ 'K',
  /* 26 */ 'L',
  /* 27 */ VK_OEM_3,
  /* 28 */ VK_OEM_7,
  /* 29 */ VK_OEM_5,
  /* 2a */ VK_LSHIFT,
  /* 2b */ VK_OEM_2,
  /* 2c */ 'Z',
  /* 2d */ 'X',
  /* 2e */ 'C',
  /* 2f */ 'V',
  /* 30 */ 'B',
  /* 31 */ 'N',
  /* 32 */ 'M',
  /* 33 */ VK_OEM_COMMA,
  /* 34 */ VK_OEM_PERIOD,
  /* 35 */ VK_OEM_MINUS,
  /* 36 */ VK_RSHIFT | KBDEXT,
  /* 37 */ VK_MULTIPLY | KBDMULTIVK,
  /* 38 */ VK_LMENU,
  /* 39 */ VK_SPACE,
  /* 3a */ VK_CAPITAL,
  /* 3b */ VK_F1,
  /* 3c */ VK_F2,
  /* 3d */ VK_F3,
  /* 3e */ VK_F4,
  /* 3f */ VK_F5,
  /* 40 */ VK_F6,
  /* 41 */ VK_F7,
  /* 42 */ VK_F8,
  /* 43 */ VK_F9,
  /* 44 */ VK_F10,
  /* 45 */ VK_NUMLOCK | KBDEXT | KBDMULTIVK,
  /* 46 */ VK_SCROLL | KBDMULTIVK,
  /* 47 */ VK_HOME | KBDSPECIAL | KBDNUMPAD,
  /* 48 */ VK_UP | KBDSPECIAL | KBDNUMPAD,
  /* 49 */ VK_PRIOR | KBDSPECIAL | KBDNUMPAD,
  /* 4a */ VK_SUBTRACT,
  /* 4b */ VK_LEFT | KBDSPECIAL | KBDNUMPAD,
  /* 4c */ VK_CLEAR | KBDSPECIAL | KBDNUMPAD,
  /* 4d */ VK_RIGHT | KBDSPECIAL | KBDNUMPAD,
  /* 4e */ VK_ADD,
  /* 4f */ VK_END | KBDSPECIAL | KBDNUMPAD,
  /* 50 */ VK_DOWN | KBDSPECIAL | KBDNUMPAD,
  /* 51 */ VK_NEXT | KBDSPECIAL | KBDNUMPAD,
  /* 52 */ VK_INSERT | KBDSPECIAL | KBDNUMPAD,
  /* 53 */ VK_DELETE | KBDSPECIAL | KBDNUMPAD,
  /* 54 */ VK_SNAPSHOT,
  /* 55 */ VK_EMPTY,
  /* 56 */ VK_OEM_102,
  /* 57 */ VK_F11,
  /* 58 */ VK_F12,
  /* 59 */ VK_CLEAR,
  /* 5a */ VK_OEM_WSCTRL,
  /* 5b */ VK_OEM_FINISH,
  /* 5c */ VK_OEM_JUMP,
  /* 5d */ VK_EREOF,
  /* 5e */ VK_OEM_BACKTAB,
  /* 5f */ VK_OEM_AUTO,
  /* 60 */ VK_EMPTY,
  /* 61 */ VK_EMPTY,
  /* 62 */ VK_ZOOM,
  /* 63 */ VK_HELP,
  /* 64 */ VK_F13,
  /* 65 */ VK_F14,
  /* 66 */ VK_F15,
  /* 67 */ VK_F16,
  /* 68 */ VK_F17,
  /* 69 */ VK_F18,
  /* 6a */ VK_F19,
  /* 6b */ VK_F20,
  /* 6c */ VK_F21,
  /* 6d */ VK_F22,
  /* 6e */ VK_F23,
  /* 6f */ VK_OEM_PA3,
  /* 70 */ VK_EMPTY,
  /* 71 */ VK_OEM_RESET,
  /* 72 */ VK_EMPTY,
  /* 73 */ VK_ABNT_C1,
  /* 74 */ VK_EMPTY,
  /* 75 */ VK_EMPTY,
  /* 76 */ VK_F24,
  /* 77 */ VK_EMPTY,
  /* 78 */ VK_EMPTY,
  /* 79 */ VK_EMPTY,
  /* 7a */ VK_EMPTY,
  /* 7b */ VK_OEM_PA1,
  /* 7c */ VK_TAB,
  /* 7d */ VK_EMPTY,
  /* 7e */ VK_ABNT_C2,
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
  { 0, 0 },
};

ROSDATA VSC_VK extcode1_to_vk[] = {
  { 0x1d, VK_PAUSE },
  { 0, 0 },
};

ROSDATA VK_TO_BIT modifier_keys[] = {
  { VK_SHIFT, KBDSHIFT },
  { VK_CONTROL, KBDCTRL },
  { VK_MENU, KBDALT },
  { 0, 0 }
};

ROSDATA MODIFIERS modifier_bits =
{
  modifier_keys,
  6,
  {
    0,            /* NONE */
    1,            /* SHIFT */
    3,            /* CTRL */
    4,            /* SHIFT+CTRL */
    SHFT_INVALID, /* ALT */
    SHFT_INVALID, /* SHIFT+ALT */
    2,            /* CTRL+ALT (AltGr) */
  }
};

/* Column order: Normal, Shift, AltGr, Ctrl, Shift+Ctrl
 * (see ModNumber in modifier_bits). A row with VK_EMPTY (0xff)
 * following a WCH_DEAD entry gives the dead character itself. */
ROSDATA VK_TO_WCHARS3 key_to_chars_3mod[] = {
  { '2'     , 0,      { '2', '\"', '@' } },
  { '3'     , 0,      { '3', '#', 0xa3 /* £ */ } },
  { '4'     , 0,      { '4', 0xa4 /* ¤ */, '$' } },
  { '5'     , 0,      { '5', '%', 0x20ac /* € */ } },
  { '7'     , 0,      { '7', '/', '{' } },
  { '8'     , 0,      { '8', '(', '[' } },
  { '9'     , 0,      { '9', ')', ']' } },
  { '0'     , 0,      { '0', '=', '}' } },
  { VK_OEM_4, 0,      { WCH_DEAD, WCH_DEAD, '|' } },
  { VK_EMPTY, 0,      { 0xb4 /* ´ */, '`', WCH_NONE } },  /* dead chars of the key above */
  { 'E'     , CAPLOK, { 'e', 'E', 0x20ac /* € */ } },
  { VK_OEM_7, CAPLOK, { 0xf8 /* ø */, 0xd8 /* Ø */, WCH_DEAD } },
  { VK_EMPTY, 0,      { WCH_NONE, WCH_NONE, '^' } },
  { 'M'     , CAPLOK, { 'm', 'M', 0xb5 /* µ */ } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS4 key_to_chars_4mod[] = {
  { VK_OEM_5    , 0,      { 0xbd /* ½ */, 0xa7 /* § */, WCH_NONE, 0x1c } },
  { VK_OEM_6    , CAPLOK, { 0xe5 /* å */, 0xc5 /* Å */, WCH_DEAD, 0x1b } },
  { VK_EMPTY    , 0,      { WCH_NONE, WCH_NONE, 0xa8 /* ¨ */, WCH_NONE } },
  { VK_OEM_1    , CAPLOK, { 0xf0 /* ð */, 0xd0 /* Ð */, WCH_DEAD, 0x1d } },
  { VK_EMPTY    , 0,      { WCH_NONE, WCH_NONE, '~', WCH_NONE } },
  { VK_OEM_MINUS, 0,      { '-', '_', WCH_NONE, 0x1f } },
  { VK_OEM_102  , 0,      { '<', '>', '\\', 0x1c } },
  { VK_BACK     , 0,      { 0x08, 0x08, WCH_NONE, 0x7f } },
  { VK_ESCAPE   , 0,      { 0x1b, 0x1b, WCH_NONE, 0x1b } },
  { VK_RETURN   , 0,      { '\r', '\r', WCH_NONE, '\n' } },
  { VK_SPACE    , 0,      { ' ', ' ', WCH_NONE, ' ' } },
  { VK_CANCEL   , 0,      { 0x03, 0x03, WCH_NONE, 0x03 } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS5 key_to_chars_5mod[] = {
  { '6', 0, { '6', '&', WCH_NONE, WCH_NONE, 0x1e } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS2 key_to_chars_2mod[] = {
  { '1'           , 0,      { '1', '!' } },
  { VK_OEM_PLUS   , 0,      { '+', '?' } },
  { 'Q'           , CAPLOK, { 'q', 'Q' } },
  { 'W'           , CAPLOK, { 'w', 'W' } },
  { 'R'           , CAPLOK, { 'r', 'R' } },
  { 'T'           , CAPLOK, { 't', 'T' } },
  { 'Y'           , CAPLOK, { 'y', 'Y' } },
  { 'U'           , CAPLOK, { 'u', 'U' } },
  { 'I'           , CAPLOK, { 'i', 'I' } },
  { 'O'           , CAPLOK, { 'o', 'O' } },
  { 'P'           , CAPLOK, { 'p', 'P' } },
  { VK_OEM_2      , 0,      { '\'', '*' } },
  { 'A'           , CAPLOK, { 'a', 'A' } },
  { 'S'           , CAPLOK, { 's', 'S' } },
  { 'D'           , CAPLOK, { 'd', 'D' } },
  { 'F'           , CAPLOK, { 'f', 'F' } },
  { 'G'           , CAPLOK, { 'g', 'G' } },
  { 'H'           , CAPLOK, { 'h', 'H' } },
  { 'J'           , CAPLOK, { 'j', 'J' } },
  { 'K'           , CAPLOK, { 'k', 'K' } },
  { 'L'           , CAPLOK, { 'l', 'L' } },
  { VK_OEM_3      , CAPLOK, { 0xe6 /* æ */, 0xc6 /* Æ */ } },
  { 'Z'           , CAPLOK, { 'z', 'Z' } },
  { 'X'           , CAPLOK, { 'x', 'X' } },
  { 'C'           , CAPLOK, { 'c', 'C' } },
  { 'V'           , CAPLOK, { 'v', 'V' } },
  { 'B'           , CAPLOK, { 'b', 'B' } },
  { 'N'           , CAPLOK, { 'n', 'N' } },
  { VK_OEM_COMMA  , 0,      { ',', ';' } },
  { VK_OEM_PERIOD , 0,      { '.', ':' } },
  { VK_DECIMAL    , 0,      { ',', ',' } },
  { VK_TAB        , 0,      { '\t', '\t' } },
  { VK_ADD        , 0,      { '+', '+' } },
  { VK_DIVIDE     , 0,      { '/', '/' } },
  { VK_MULTIPLY   , 0,      { '*', '*' } },
  { VK_SUBTRACT   , 0,      { '-', '-' } },
  { 0, 0 }
};

ROSDATA VK_TO_WCHARS1 key_to_chars_1mod[] = {
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

#define vk_master(n, x) { (PVK_TO_WCHARS1)x, n, sizeof(x[0]) }

ROSDATA VK_TO_WCHAR_TABLE vk_to_wchar_master_table[] = {
  vk_master(3, key_to_chars_3mod),
  vk_master(4, key_to_chars_4mod),
  vk_master(5, key_to_chars_5mod),
  vk_master(2, key_to_chars_2mod),
  vk_master(1, key_to_chars_1mod),
  { 0, 0, 0 }
};

ROSDATA DEADKEY deadkey_table[] = {
  DEADTRANS('a', 0xb4 /* ´ */, 0xe1 /* á */, 0x0000),
  DEADTRANS('e', 0xb4 /* ´ */, 0xe9 /* é */, 0x0000),
  DEADTRANS('i', 0xb4 /* ´ */, 0xed /* í */, 0x0000),
  DEADTRANS('o', 0xb4 /* ´ */, 0xf3 /* ó */, 0x0000),
  DEADTRANS('u', 0xb4 /* ´ */, 0xfa /* ú */, 0x0000),
  DEADTRANS('y', 0xb4 /* ´ */, 0xfd /* ý */, 0x0000),
  DEADTRANS('A', 0xb4 /* ´ */, 0xc1 /* Á */, 0x0000),
  DEADTRANS('E', 0xb4 /* ´ */, 0xc9 /* É */, 0x0000),
  DEADTRANS('I', 0xb4 /* ´ */, 0xcd /* Í */, 0x0000),
  DEADTRANS('O', 0xb4 /* ´ */, 0xd3 /* Ó */, 0x0000),
  DEADTRANS('U', 0xb4 /* ´ */, 0xda /* Ú */, 0x0000),
  DEADTRANS('Y', 0xb4 /* ´ */, 0xdd /* Ý */, 0x0000),
  DEADTRANS(' ', 0xb4 /* ´ */, 0xb4 /* ´ */, 0x0000),
  DEADTRANS('a', '`', 0xe0 /* à */, 0x0000),
  DEADTRANS('e', '`', 0xe8 /* è */, 0x0000),
  DEADTRANS('i', '`', 0xec /* ì */, 0x0000),
  DEADTRANS('o', '`', 0xf2 /* ò */, 0x0000),
  DEADTRANS('u', '`', 0xf9 /* ù */, 0x0000),
  DEADTRANS('A', '`', 0xc0 /* À */, 0x0000),
  DEADTRANS('E', '`', 0xc8 /* È */, 0x0000),
  DEADTRANS('I', '`', 0xcc /* Ì */, 0x0000),
  DEADTRANS('O', '`', 0xd2 /* Ò */, 0x0000),
  DEADTRANS('U', '`', 0xd9 /* Ù */, 0x0000),
  DEADTRANS(' ', '`', '`', 0x0000),
  DEADTRANS('a', 0xa8 /* ¨ */, 0xe4 /* ä */, 0x0000),
  DEADTRANS('e', 0xa8 /* ¨ */, 0xeb /* ë */, 0x0000),
  DEADTRANS('i', 0xa8 /* ¨ */, 0xef /* ï */, 0x0000),
  DEADTRANS('o', 0xa8 /* ¨ */, 0xf6 /* ö */, 0x0000),
  DEADTRANS('u', 0xa8 /* ¨ */, 0xfc /* ü */, 0x0000),
  DEADTRANS('y', 0xa8 /* ¨ */, 0xff /* ÿ */, 0x0000),
  DEADTRANS('A', 0xa8 /* ¨ */, 0xc4 /* Ä */, 0x0000),
  DEADTRANS('E', 0xa8 /* ¨ */, 0xcb /* Ë */, 0x0000),
  DEADTRANS('I', 0xa8 /* ¨ */, 0xcf /* Ï */, 0x0000),
  DEADTRANS('O', 0xa8 /* ¨ */, 0xd6 /* Ö */, 0x0000),
  DEADTRANS('U', 0xa8 /* ¨ */, 0xdc /* Ü */, 0x0000),
  DEADTRANS(' ', 0xa8 /* ¨ */, 0xa8 /* ¨ */, 0x0000),
  DEADTRANS('a', '^', 0xe2 /* â */, 0x0000),
  DEADTRANS('e', '^', 0xea /* ê */, 0x0000),
  DEADTRANS('i', '^', 0xee /* î */, 0x0000),
  DEADTRANS('o', '^', 0xf4 /* ô */, 0x0000),
  DEADTRANS('u', '^', 0xfb /* û */, 0x0000),
  DEADTRANS('A', '^', 0xc2 /* Â */, 0x0000),
  DEADTRANS('E', '^', 0xca /* Ê */, 0x0000),
  DEADTRANS('I', '^', 0xce /* Î */, 0x0000),
  DEADTRANS('O', '^', 0xd4 /* Ô */, 0x0000),
  DEADTRANS('U', '^', 0xdb /* Û */, 0x0000),
  DEADTRANS(' ', '^', '^', 0x0000),
  DEADTRANS('a', '~', 0xe3 /* ã */, 0x0000),
  DEADTRANS('o', '~', 0xf5 /* õ */, 0x0000),
  DEADTRANS('n', '~', 0xf1 /* ñ */, 0x0000),
  DEADTRANS('A', '~', 0xc3 /* Ã */, 0x0000),
  DEADTRANS('O', '~', 0xd5 /* Õ */, 0x0000),
  DEADTRANS('N', '~', 0xd1 /* Ñ */, 0x0000),
  DEADTRANS(' ', '~', '~', 0x0000),
  { 0, 0, 0 }
};

ROSDATA VSC_LPWSTR key_names[] = {
  { 0x01, L"ESC" },
  { 0x0e, L"TILBAGE" },
  { 0x0f, L"TAB" },
  { 0x1c, L"ENTER" },
  { 0x1d, L"CTRL" },
  { 0x2a, L"SKIFT" },
  { 0x36, L"H" L"\x00d8" L"JRE SKIFT" },
  { 0x37, L"NUM *" },
  { 0x38, L"ALT" },
  { 0x39, L"MELLEMRUM" },
  { 0x3a, L"CAPS LOCK" },
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
  { 0x45, L"PAUSE" },
  { 0x46, L"SCROLL LOCK" },
  { 0x47, L"NUM 7" },
  { 0x48, L"NUM 8" },
  { 0x49, L"NUM 9" },
  { 0x4a, L"NUM -" },
  { 0x4b, L"NUM 4" },
  { 0x4c, L"NUM 5" },
  { 0x4d, L"NUM 6" },
  { 0x4e, L"NUM +" },
  { 0x4f, L"NUM 1" },
  { 0x50, L"NUM 2" },
  { 0x51, L"NUM 3" },
  { 0x52, L"NUM 0" },
  { 0x53, L"NUM ," },
  { 0x57, L"F11" },
  { 0x58, L"F12" },
  { 0, NULL },
};

ROSDATA VSC_LPWSTR extended_key_names[] = {
  { 0x1c, L"NUM ENTER" },
  { 0x1d, L"H" L"\x00d8" L"JRE CTRL" },
  { 0x35, L"NUM /" },
  { 0x37, L"PRINTSCRN" },
  { 0x38, L"ALT GR" },
  { 0x45, L"NUM LOCK" },
  { 0x46, L"BREAK" },
  { 0x47, L"HOME" },
  { 0x48, L"PIL OP" },
  { 0x49, L"PGUP" },
  { 0x4b, L"VENSTRE PIL" },
  { 0x4d, L"H" L"\x00d8" L"JRE PIL" },
  { 0x4f, L"END" },
  { 0x50, L"PIL NED" },
  { 0x51, L"PGDN" },
  { 0x52, L"INS" },
  { 0x53, L"DELETE" },
  { 0x54, L"<00>" },
  { 0x56, L"HELP" },
  { 0x5b, L"Venstre Windows" },
  { 0x5c, L"H" L"\x00f8" L"jre Windows" },
  { 0x5d, L"Program" },
  { 0, NULL },
};

ROSDATA DEADKEY_LPWSTR key_names_dead[] = {
  L"\x00b4" L"ACCENT AIGU",
  L"`ACCENT GRAVE",
  L"^CIRKUMFLEKS",
  L"\x00a8" L"TREMA",
  L"~TILDE",
  NULL
};

ROSDATA KBDTABLES keyboard_layout_table = {
  &modifier_bits,
  vk_to_wchar_master_table,
  deadkey_table,
  (VSC_LPWSTR *)key_names,
  (VSC_LPWSTR *)extended_key_names,
  key_names_dead,
  scancode_to_vk,
  RTL_NUMBER_OF(scancode_to_vk),
  extcode0_to_vk,
  extcode1_to_vk,
  MAKELONG(KLLF_ALTGR, KBD_VERSION), /* 0x00010001 */
  0, 0, NULL /* no ligatures */
};

PKBDTABLES WINAPI KbdLayerDescriptor(VOID) {
  return &keyboard_layout_table;
}
