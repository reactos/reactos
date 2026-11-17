/*
 * PROJECT:     ReactOS Keyboard Layouts
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Korean (Hangul) keyboard layout stub driver
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <windef.h>
#include <winuser.h>
#include "kbd.h"

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

#ifndef VK_JUNJA
  #define VK_JUNJA 0x17
#endif

#ifndef ALLOC_SECTION_LDATA
  #define ALLOC_SECTION_LDATA
#endif

/* Scancode -> virtual key (one-byte scancodes, 0x00 .. 0x7F) */
ROSDATA ALLOC_SECTION_LDATA USHORT ausVK[] =
{
  /* 00 */ 0xFF,
  /* 01 */ VK_ESCAPE,
  /* 02 */ '1',
  /* 03 */ '2',
  /* 04 */ '3',
  /* 05 */ '4',
  /* 06 */ '5',
  /* 07 */ '6',
  /* 08 */ '7',
  /* 09 */ '8',
  /* 0A */ '9',
  /* 0B */ '0',
  /* 0C */ VK_OEM_MINUS,
  /* 0D */ VK_OEM_PLUS | KBDSPECIAL,
  /* 0E */ VK_BACK,
  /* 0F */ VK_TAB,
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
  /* 1A */ VK_OEM_4,
  /* 1B */ VK_OEM_6,
  /* 1C */ VK_RETURN,
  /* 1D */ VK_LCONTROL,
  /* 1E */ 'A',
  /* 1F */ 'S',
  /* 20 */ 'D',
  /* 21 */ 'F',
  /* 22 */ 'G',
  /* 23 */ 'H',
  /* 24 */ 'J',
  /* 25 */ 'K',
  /* 26 */ 'L',
  /* 27 */ VK_OEM_1,
  /* 28 */ VK_OEM_7,
  /* 29 */ VK_OEM_3,
  /* 2A */ VK_LSHIFT,
  /* 2B */ VK_OEM_5,
  /* 2C */ 'Z',
  /* 2D */ 'X',
  /* 2E */ 'C',
  /* 2F */ 'V',
  /* 30 */ 'B',
  /* 31 */ 'N',
  /* 32 */ 'M',
  /* 33 */ VK_OEM_COMMA,
  /* 34 */ VK_OEM_PERIOD,
  /* 35 */ VK_OEM_2,
  /* 36 */ VK_RSHIFT | KBDEXT,
  /* 37 */ VK_MULTIPLY | KBDMULTIVK,
  /* 38 */ VK_LMENU,
  /* 39 */ VK_SPACE,
  /* 3A */ VK_CAPITAL,
  /* 3B */ VK_F1,
  /* 3C */ VK_F2,
  /* 3D */ VK_F3,
  /* 3E */ VK_F4,
  /* 3F */ VK_F5,
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
  /* 4A */ VK_SUBTRACT,
  /* 4B */ VK_LEFT | KBDSPECIAL | KBDNUMPAD,
  /* 4C */ VK_CLEAR | KBDSPECIAL | KBDNUMPAD,
  /* 4D */ VK_RIGHT | KBDSPECIAL | KBDNUMPAD,
  /* 4E */ VK_ADD,
  /* 4F */ VK_END | KBDSPECIAL | KBDNUMPAD,
  /* 50 */ VK_DOWN | KBDSPECIAL | KBDNUMPAD,
  /* 51 */ VK_NEXT | KBDSPECIAL | KBDNUMPAD,
  /* 52 */ VK_INSERT | KBDSPECIAL | KBDNUMPAD,
  /* 53 */ VK_DELETE | KBDSPECIAL | KBDNUMPAD,
  /* 54 */ VK_SNAPSHOT,
  /* 55 */ 0xFF,
  /* 56 */ VK_OEM_102,
  /* 57 */ VK_F11,
  /* 58 */ VK_F12,
  /* 59 */ VK_CLEAR,
  /* 5A */ 0xEE,
  /* 5B */ 0xF1,
  /* 5C */ 0xEA,
  /* 5D */ 0xF9,
  /* 5E */ VK_OEM_BACKTAB,
  /* 5F */ VK_OEM_AUTO,
  /* 60 */ 0xFF,
  /* 61 */ 0xFB,
  /* 62 */ 0x2F,
  /* 63 */ 0xFF,
  /* 64 */ VK_F13,
  /* 65 */ VK_F14,
  /* 66 */ VK_F15,
  /* 67 */ VK_F16,
  /* 68 */ VK_F17,
  /* 69 */ VK_F18,
  /* 6A */ VK_F19,
  /* 6B */ VK_F20,
  /* 6C */ VK_F21,
  /* 6D */ VK_F22,
  /* 6E */ VK_F23,
  /* 6F */ 0xED,
  /* 70 */ 0xFF,
  /* 71 */ 0xE9,
  /* 72 */ 0xFF,
  /* 73 */ VK_ABNT_C1,
  /* 74 */ 0xFF,
  /* 75 */ 0xFF,
  /* 76 */ VK_F24,
  /* 77 */ 0xFF,
  /* 78 */ 0xFF,
  /* 79 */ 0xFF,
  /* 7A */ 0xFF,
  /* 7B */ 0xEB,
  /* 7C */ VK_TAB,
  /* 7D */ 0xFF,
  /* 7E */ VK_ABNT_C2,
  /* 7F */ 0xEC,
};

/* Scancodes with the E0 prefix */
ROSDATA ALLOC_SECTION_LDATA VSC_VK aE0VscToVk[] =
{
  { 0x10, 0xB1 | KBDEXT },
  { 0x19, 0xB0 | KBDEXT },
  { 0x1C, VK_RETURN | KBDEXT },
  { 0x1D, VK_HANJA | KBDEXT | KBDSPECIAL },
  { 0x20, 0xAD | KBDEXT },
  { 0x21, 0xB7 | KBDEXT },
  { 0x22, 0xB3 | KBDEXT },
  { 0x24, 0xB2 | KBDEXT },
  { 0x2E, 0xAE | KBDEXT },
  { 0x30, 0xAF | KBDEXT },
  { 0x32, 0xAC | KBDEXT },
  { 0x35, VK_DIVIDE | KBDEXT },
  { 0x37, VK_SNAPSHOT | KBDEXT },
  { 0x38, VK_HANGUL | KBDEXT | KBDSPECIAL },
  { 0x46, VK_CANCEL | KBDEXT },
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
  { 0x5F, 0x5F | KBDEXT },
  { 0x65, 0xAA | KBDEXT },
  { 0x66, 0xAB | KBDEXT },
  { 0x67, 0xA8 | KBDEXT },
  { 0x68, 0xA9 | KBDEXT },
  { 0x69, 0xA7 | KBDEXT },
  { 0x6A, 0xA6 | KBDEXT },
  { 0x6B, 0xB6 | KBDEXT },
  { 0x6C, 0xB4 | KBDEXT },
  { 0x6D, 0xB5 | KBDEXT },
  { 0xF1, VK_HANJA | KBDEXT | KBDSPECIAL },
  { 0xF2, VK_HANGUL | KBDEXT | KBDSPECIAL },
  { 0, 0 }
};

/* Scancodes with the E1 prefix */
ROSDATA ALLOC_SECTION_LDATA VSC_VK aE1VscToVk[] =
{
  { 0x1D, VK_PAUSE },
  { 0, 0 }
};

ROSDATA ALLOC_SECTION_LDATA VK_TO_BIT aVkToBits[] =
{
  { VK_SHIFT,   KBDSHIFT },
  { VK_CONTROL, KBDCTRL  },
  { VK_MENU,    KBDALT   },
  { 0,          0        }
};

/* Only Shift/Ctrl combinations (0..3) are valid; Alt does not select a column. */
ROSDATA ALLOC_SECTION_LDATA MODIFIERS CharModifiers =
{
  &aVkToBits[0],
  3,
  { 0, 1, 2, 3 }
};

ROSDATA ALLOC_SECTION_LDATA VK_TO_WCHARS2 aVkToWch2[] =
{
  { '0', 0, { '0', ')' } },
  { '1', 0, { '1', '!' } },
  { '3', 0, { '3', '#' } },
  { '4', 0, { '4', '$' } },
  { '5', 0, { '5', '%' } },
  { '7', 0, { '7', '&' } },
  { '8', 0, { '8', '*' } },
  { '9', 0, { '9', '(' } },
  { 'A', CAPLOK, { 'a', 'A' } },
  { 'B', CAPLOK, { 'b', 'B' } },
  { 'C', CAPLOK, { 'c', 'C' } },
  { 'D', CAPLOK, { 'd', 'D' } },
  { 'E', CAPLOK, { 'e', 'E' } },
  { 'F', CAPLOK, { 'f', 'F' } },
  { 'G', CAPLOK, { 'g', 'G' } },
  { 'H', CAPLOK, { 'h', 'H' } },
  { 'I', CAPLOK, { 'i', 'I' } },
  { 'J', CAPLOK, { 'j', 'J' } },
  { 'K', CAPLOK, { 'k', 'K' } },
  { 'L', CAPLOK, { 'l', 'L' } },
  { 'M', CAPLOK, { 'm', 'M' } },
  { 'N', CAPLOK, { 'n', 'N' } },
  { 'O', CAPLOK, { 'o', 'O' } },
  { 'P', CAPLOK, { 'p', 'P' } },
  { 'Q', CAPLOK, { 'q', 'Q' } },
  { 'R', CAPLOK, { 'r', 'R' } },
  { 'S', CAPLOK, { 's', 'S' } },
  { 'T', CAPLOK, { 't', 'T' } },
  { 'U', CAPLOK, { 'u', 'U' } },
  { 'V', CAPLOK, { 'v', 'V' } },
  { 'W', CAPLOK, { 'w', 'W' } },
  { 'X', CAPLOK, { 'x', 'X' } },
  { 'Y', CAPLOK, { 'y', 'Y' } },
  { 'Z', CAPLOK, { 'z', 'Z' } },
  { VK_OEM_1, 0, { ';', ':' } },
  { VK_OEM_2, 0, { '/', '?' } },
  { VK_OEM_3, 0, { '`', '~' } },
  { VK_OEM_7, 0, { '\'', '"' } },
  { VK_OEM_8, 0, { WCH_NONE, WCH_NONE } },
  { VK_OEM_COMMA, 0, { ',', '<' } },
  { VK_OEM_PERIOD, 0, { '.', '>' } },
  { VK_OEM_PLUS, 0, { '=', '+' } },
  { VK_TAB, 0, { 0x0009, 0x0009 } },
  { VK_ADD, 0, { '+', '+' } },
  { VK_DECIMAL, 0, { '.', '.' } },
  { VK_DIVIDE, 0, { '/', '/' } },
  { VK_MULTIPLY, 0, { '*', '*' } },
  { VK_SUBTRACT, 0, { '-', '-' } },
  { 0, 0, { 0, 0 } }
};

ROSDATA ALLOC_SECTION_LDATA VK_TO_WCHARS3 aVkToWch3[] =
{
  { VK_BACK, 0, { 0x0008, 0x0008, 0x007F } },
  { VK_CANCEL, 0, { 0x0003, 0x0003, 0x0003 } },
  { VK_ESCAPE, 0, { 0x001B, 0x001B, 0x001B } },
  { VK_OEM_4, 0, { '[', '{', 0x001B } },
  { VK_OEM_5, 0, { '\\', '|', 0x001C } },
  { VK_OEM_102, 0, { '\\', '|', 0x001C } },
  { VK_OEM_6, 0, { ']', '}', 0x001D } },
  { VK_RETURN, 0, { 0x000D, 0x000D, 0x000A } },
  { VK_SPACE, 0, { ' ', ' ', ' ' } },
  { 0, 0, { 0, 0, 0 } }
};

ROSDATA ALLOC_SECTION_LDATA VK_TO_WCHARS4 aVkToWch4[] =
{
  { '2', 0, { '2', '@', WCH_NONE, 0x0000 } },
  { '6', 0, { '6', '^', WCH_NONE, 0x001E } },
  { VK_OEM_MINUS, 0, { '-', '_', WCH_NONE, 0x001F } },
  { 0, 0, { 0, 0, 0, 0 } }
};

ROSDATA ALLOC_SECTION_LDATA VK_TO_WCHARS1 aVkToWch1[] =
{
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
  { 0, 0, { 0 } }
};

ROSDATA ALLOC_SECTION_LDATA VK_TO_WCHAR_TABLE aVkToWcharTable[] =
{
  { (PVK_TO_WCHARS1)aVkToWch3, 3, sizeof(aVkToWch3[0]) },
  { (PVK_TO_WCHARS1)aVkToWch4, 4, sizeof(aVkToWch4[0]) },
  { (PVK_TO_WCHARS1)aVkToWch2, 2, sizeof(aVkToWch2[0]) },
  { (PVK_TO_WCHARS1)aVkToWch1, 1, sizeof(aVkToWch1[0]) },
  { NULL, 0, 0 }
};

/* Scancode names */
ROSDATA ALLOC_SECTION_LDATA VSC_LPWSTR aKeyNames[] =
{
  { 0x01, L"Esc" },
  { 0x0E, L"Backspace" },
  { 0x0F, L"Tab" },
  { 0x1C, L"Enter" },
  { 0x1D, L"Ctrl" },
  { 0x2A, L"Shift" },
  { 0x36, L"Right Shift" },
  { 0x37, L"Num *" },
  { 0x38, L"Alt" },
  { 0x39, L"Space" },
  { 0x3A, L"Caps Lock" },
  { 0x3B, L"F1" },
  { 0x3C, L"F2" },
  { 0x3D, L"F3" },
  { 0x3E, L"F4" },
  { 0x3F, L"F5" },
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
  { 0x4A, L"Num -" },
  { 0x4B, L"Num 4" },
  { 0x4C, L"Num 5" },
  { 0x4D, L"Num 6" },
  { 0x4E, L"Num +" },
  { 0x4F, L"Num 1" },
  { 0x50, L"Num 2" },
  { 0x51, L"Num 3" },
  { 0x52, L"Num 0" },
  { 0x53, L"Num Del" },
  { 0x54, L"Sys Req" },
  { 0x57, L"F11" },
  { 0x58, L"F12" },
  { 0x7C, L"F13" },
  { 0x7D, L"F14" },
  { 0x7E, L"F15" },
  { 0x7F, L"F16" },
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

/* Scancode names (extended keys) */
ROSDATA ALLOC_SECTION_LDATA VSC_LPWSTR aKeyNamesExt[] =
{
  { 0x1C, L"Num Enter" },
  { 0x1D, L"Right Control" },
  { 0x35, L"Num /" },
  { 0x37, L"Prnt Scrn" },
  { 0x38, L"Right Alt" },
  { 0x45, L"Num Lock" },
  { 0x46, L"Break" },
  { 0x47, L"Home" },
  { 0x48, L"Up" },
  { 0x49, L"Page Up" },
  { 0x4B, L"Left" },
  { 0x4D, L"Right" },
  { 0x4F, L"End" },
  { 0x50, L"Down" },
  { 0x51, L"Page Down" },
  { 0x52, L"Insert" },
  { 0x53, L"Delete" },
  { 0x5B, L"Left Windows" },
  { 0x5C, L"Right Windows" },
  { 0x5D, L"Application" },
  { 0xF1, L"Hanja" },
  { 0xF2, L"Hangeul" },
  { 0, NULL }
};

/* Hangul keyboard layout (type 8 = Korean) */
ROSDATA ALLOC_SECTION_LDATA KBDTABLES_FE KbdTables =
{
  {
    &CharModifiers,       /* pCharModifiers */
    aVkToWcharTable,      /* pVkToWcharTable */
    NULL,                 /* pDeadKey */
    aKeyNames,            /* pKeyNames */
    aKeyNamesExt,         /* pKeyNamesExt */
    NULL,                 /* pKeyNamesDead */
    ausVK,                /* pusVSCtoVK */
    RTL_NUMBER_OF(ausVK), /* bMaxVSCtoVK */
    aE0VscToVk,           /* pVSCtoVK_E0 */
    aE1VscToVk,           /* pVSCtoVK_E1 */
    0,                    /* fLocaleFlags */
    0, 0,                 /* nLgMaxd, cbLgEntry */
    NULL                  /* pLigature */
  },
  8,                      /* dwType */
  0                       /* dwSubType */
};

/* Far-East (NLS) function table: only VK_OEM_PLUS is remapped */
ROSDATA ALLOC_SECTION_LDATA VK_F aVkToF[] =
{
  {
    VK_OEM_PLUS, KBDNLS_TYPE_NORMAL, KBDNLS_INDEX_NORMAL, 0,
    {   /* NLSFEProc: one entry per modifier state (0..7) */
      { KBDNLS_SEND_BASE_VK,  0 },
      { KBDNLS_SEND_BASE_VK,  0 },
      { KBDNLS_SEND_BASE_VK,  0 },
      { KBDNLS_SEND_BASE_VK,  0 },
      { KBDNLS_SEND_PARAM_VK, VK_JUNJA }, /* Alt */
      { KBDNLS_SEND_BASE_VK,  0 },
      { KBDNLS_SEND_BASE_VK,  0 },
      { KBDNLS_SEND_BASE_VK,  0 }
    },
    { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } }
  }
};

ROSDATA ALLOC_SECTION_LDATA KBDNLSTABLES KbdNlsTables =
{
  0,                     /* OEMIdentifier */
  0,                     /* LayoutInformation */
  RTL_NUMBER_OF(aVkToF), /* NumOfVkToF */
  aVkToF,                /* pVkToF */
  0,                     /* NumOfMouseVKey */
  NULL                   /* pusMouseVKey */
};

/* ------------------------------------------------------------------------
 * KbdLayerDescriptor @1
 * ------------------------------------------------------------------------ */
PKBDTABLES WINAPI
KbdLayerDescriptor(VOID)
{
  return &KbdTables.Base;
}

/* ------------------------------------------------------------------------
 * KbdNlsLayerDescriptor @2
 * ------------------------------------------------------------------------ */
PKBDNLSTABLES WINAPI
KbdNlsLayerDescriptor(VOID)
{
  return &KbdNlsTables;
}
