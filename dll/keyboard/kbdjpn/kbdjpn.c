/*
 * PROJECT:     ReactOS Keyboard Layouts
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Japanese keyboard layout stub driver
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define WIN32_NO_STATUS
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

/* Scan code -> virtual key (bMaxVSCtoVK = 128) */
ROSDATA USHORT ausVK[] =
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
    /* 0D */ VK_OEM_PLUS,
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
    /* 29 */ VK_OEM_3 | KBDSPECIAL,
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
    /* 3A */ VK_CAPITAL | KBDSPECIAL,
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
    /* 5B */ VK_DBE_KATAKANA,
    /* 5C */ 0xEA,
    /* 5D */ VK_DBE_FLUSHSTRING,
    /* 5E */ VK_DBE_ROMAN,
    /* 5F */ VK_DBE_SBCSCHAR,
    /* 60 */ 0xFF,
    /* 61 */ 0xFF,
    /* 62 */ VK_DBE_NOCODEINPUT,
    /* 63 */ 0x2F,
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

ROSDATA VSC_VK aE0VscToVk[] =
{
    { 0x10, 0xB1 | KBDEXT },
    { 0x19, 0xB0 | KBDEXT },
    { 0x1C, VK_RETURN | KBDEXT },
    { 0x1D, VK_RCONTROL | KBDEXT },
    { 0x20, 0xAD | KBDEXT },
    { 0x21, 0xB7 | KBDEXT },
    { 0x22, 0xB3 | KBDEXT },
    { 0x24, 0xB2 | KBDEXT },
    { 0x2E, 0xAE | KBDEXT },
    { 0x30, 0xAF | KBDEXT },
    { 0x32, 0xAC | KBDEXT },
    { 0x35, VK_DIVIDE | KBDEXT },
    { 0x37, VK_SNAPSHOT | KBDEXT },
    { 0x38, VK_RMENU | KBDEXT },
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
    { 0, 0 }
};

ROSDATA VSC_VK aE1VscToVk[] =
{
    { 0x1D, 0x13 },
    { 0, 0 }
};

ROSDATA VK_TO_BIT aVkToBits[] =
{
    { VK_SHIFT,   KBDSHIFT },
    { VK_CONTROL, KBDCTRL },
    { VK_MENU,    KBDALT },
    { VK_KANA,    0x08 },      /* KANA modifier bit */
    { 0,          0 }
};

/* ModNumber[] is indexed by modifier bits: Kana|Alt|Ctrl|Shift */
ROSDATA MODIFIERS CharModifiers =
{
    &aVkToBits[0],
    11,
    {
        0,            /* none */
        1,            /* Shift */
        4,            /* Ctrl */
        6,            /* Shift+Ctrl */
        SHFT_INVALID, /* Alt */
        SHFT_INVALID, /* Shift+Alt */
        SHFT_INVALID, /* Ctrl+Alt */
        SHFT_INVALID, /* Shift+Ctrl+Alt */
        2,            /* Kana */
        3,            /* Kana+Shift */
        5,            /* Kana+Ctrl */
        7             /* Kana+Shift+Ctrl */
    }
};

ROSDATA VK_TO_WCHARS6 aVkToWch6[] =
{
    { VK_BACK, 0, { 0x0008, 0x0008, 0x0008, 0x0008, 0x007F, 0x007F } },
    { VK_CANCEL, 0, { 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003 } },
    { VK_ESCAPE, 0, { 0x001B, 0x001B, 0x001B, 0x001B, 0x001B, 0x001B } },
    { VK_OEM_4, KANALOK, { '[', '{', 0xFF9E, 0xFF62, 0x001B, 0x001B } },
    { VK_OEM_5, KANALOK, { '\\', '|', 0xFF91, 0xFF91, 0x001C, 0x001C } },
    { VK_OEM_102, KANALOK, { '\\', '|', 0xFF91, 0xFF91, 0x001C, 0x001C } },
    { VK_OEM_6, KANALOK, { ']', '}', 0xFF9F, 0xFF63, 0x001D, 0x001D } },
    { VK_RETURN, 0, { 0x000D, 0x000D, 0x000D, 0x000D, 0x000A, 0x000A } },
    { VK_SPACE, 0, { ' ', ' ', ' ', ' ', ' ', ' ' } },
    { 0, 0, { 0, 0, 0, 0, 0, 0 } }
};

ROSDATA VK_TO_WCHARS8 aVkToWch8[] =
{
    { '2', KANALOK, { '2', '@', 0xFF8C, 0xFF8C, WCH_NONE, WCH_NONE, 0x0000, 0x0000 } },
    { '6', KANALOK, { '6', '^', 0xFF75, 0xFF6B, WCH_NONE, WCH_NONE, 0x001E, 0x001E } },
    { VK_OEM_MINUS, KANALOK, { '-', '_', 0xFF8E, 0xFF70, WCH_NONE, WCH_NONE, 0x001F, 0x001F } },
    { 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } }
};

ROSDATA VK_TO_WCHARS4 aVkToWch4[] =
{
    { '0', KANALOK, { '0', ')', 0xFF9C, 0xFF66 } },
    { '1', KANALOK, { '1', '!', 0xFF87, 0xFF87 } },
    { '3', KANALOK, { '3', '#', 0xFF71, 0xFF67 } },
    { '4', KANALOK, { '4', '$', 0xFF73, 0xFF69 } },
    { '5', KANALOK, { '5', '%', 0xFF74, 0xFF6A } },
    { '7', KANALOK, { '7', '&', 0xFF94, 0xFF6C } },
    { '8', KANALOK, { '8', '*', 0xFF95, 0xFF6D } },
    { '9', KANALOK, { '9', '(', 0xFF96, 0xFF6E } },
    { 'A', CAPLOK|KANALOK, { 'a', 'A', 0xFF81, 0xFF81 } },
    { 'B', CAPLOK|KANALOK, { 'b', 'B', 0xFF7A, 0xFF7A } },
    { 'C', CAPLOK|KANALOK, { 'c', 'C', 0xFF7F, 0xFF7F } },
    { 'D', CAPLOK|KANALOK, { 'd', 'D', 0xFF7C, 0xFF7C } },
    { 'E', CAPLOK|KANALOK, { 'e', 'E', 0xFF72, 0xFF68 } },
    { 'F', CAPLOK|KANALOK, { 'f', 'F', 0xFF8A, 0xFF8A } },
    { 'G', CAPLOK|KANALOK, { 'g', 'G', 0xFF77, 0xFF77 } },
    { 'H', CAPLOK|KANALOK, { 'h', 'H', 0xFF78, 0xFF78 } },
    { 'I', CAPLOK|KANALOK, { 'i', 'I', 0xFF86, 0xFF86 } },
    { 'J', CAPLOK|KANALOK, { 'j', 'J', 0xFF8F, 0xFF8F } },
    { 'K', CAPLOK|KANALOK, { 'k', 'K', 0xFF89, 0xFF89 } },
    { 'L', CAPLOK|KANALOK, { 'l', 'L', 0xFF98, 0xFF98 } },
    { 'M', CAPLOK|KANALOK, { 'm', 'M', 0xFF93, 0xFF93 } },
    { 'N', CAPLOK|KANALOK, { 'n', 'N', 0xFF90, 0xFF90 } },
    { 'O', CAPLOK|KANALOK, { 'o', 'O', 0xFF97, 0xFF97 } },
    { 'P', CAPLOK|KANALOK, { 'p', 'P', 0xFF7E, 0xFF7E } },
    { 'Q', CAPLOK|KANALOK, { 'q', 'Q', 0xFF80, 0xFF80 } },
    { 'R', CAPLOK|KANALOK, { 'r', 'R', 0xFF7D, 0xFF7D } },
    { 'S', CAPLOK|KANALOK, { 's', 'S', 0xFF84, 0xFF84 } },
    { 'T', CAPLOK|KANALOK, { 't', 'T', 0xFF76, 0xFF76 } },
    { 'U', CAPLOK|KANALOK, { 'u', 'U', 0xFF85, 0xFF85 } },
    { 'V', CAPLOK|KANALOK, { 'v', 'V', 0xFF8B, 0xFF8B } },
    { 'W', CAPLOK|KANALOK, { 'w', 'W', 0xFF83, 0xFF83 } },
    { 'X', CAPLOK|KANALOK, { 'x', 'X', 0xFF7B, 0xFF7B } },
    { 'Y', CAPLOK|KANALOK, { 'y', 'Y', 0xFF9D, 0xFF9D } },
    { 'Z', CAPLOK|KANALOK, { 'z', 'Z', 0xFF82, 0xFF6F } },
    { VK_OEM_1, KANALOK, { ';', ':', 0xFF9A, 0xFF9A } },
    { VK_OEM_2, KANALOK, { '/', '?', 0xFF92, 0xFF65 } },
    { VK_OEM_3, KANALOK, { '`', '~', 0xFF9B, 0xFF9B } },
    { VK_OEM_7, KANALOK, { '\'', '"', 0xFF79, 0xFF79 } },
    { VK_OEM_8, 0, { WCH_NONE, WCH_NONE, WCH_NONE, WCH_NONE } },
    { VK_OEM_COMMA, KANALOK, { ',', '<', 0xFF88, 0xFF64 } },
    { VK_OEM_PERIOD, KANALOK, { '.', '>', 0xFF99, 0xFF61 } },
    { VK_OEM_PLUS, KANALOK, { '=', '+', 0xFF8D, 0xFF8D } },
    { VK_TAB, 0, { 0x0009, 0x0009, 0x0009, 0x0009 } },
    { VK_ADD, 0, { '+', '+', '+', '+' } },
    { VK_DECIMAL, 0, { '.', '.', '.', '.' } },
    { VK_DIVIDE, 0, { '/', '/', '/', '/' } },
    { VK_MULTIPLY, 0, { '*', '*', '*', '*' } },
    { VK_SUBTRACT, 0, { '-', '-', '-', '-' } },
    { 0, 0, { 0, 0, 0, 0 } }
};

ROSDATA VK_TO_WCHARS4 aVkToWch4Num[] =
{
    { 0x60, 0, { '0', WCH_NONE, '0', WCH_NONE } },
    { 0x61, 0, { '1', WCH_NONE, '1', WCH_NONE } },
    { 0x62, 0, { '2', WCH_NONE, '2', WCH_NONE } },
    { 0x63, 0, { '3', WCH_NONE, '3', WCH_NONE } },
    { 0x64, 0, { '4', WCH_NONE, '4', WCH_NONE } },
    { 0x65, 0, { '5', WCH_NONE, '5', WCH_NONE } },
    { 0x66, 0, { '6', WCH_NONE, '6', WCH_NONE } },
    { 0x67, 0, { '7', WCH_NONE, '7', WCH_NONE } },
    { 0x68, 0, { '8', WCH_NONE, '8', WCH_NONE } },
    { 0x69, 0, { '9', WCH_NONE, '9', WCH_NONE } },
    { 0, 0, { 0, 0, 0, 0 } }
};

ROSDATA VK_TO_WCHAR_TABLE aVkToWcharTable[] =
{
    { (PVK_TO_WCHARS1)aVkToWch6,    6, sizeof(aVkToWch6[0]) },
    { (PVK_TO_WCHARS1)aVkToWch8,    8, sizeof(aVkToWch8[0]) },
    { (PVK_TO_WCHARS1)aVkToWch4,    4, sizeof(aVkToWch4[0]) },
    { (PVK_TO_WCHARS1)aVkToWch4Num, 4, sizeof(aVkToWch4Num[0]) },
    { NULL, 0, 0 }
};

ROSDATA VSC_LPWSTR aKeyNames[] =
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

ROSDATA VSC_LPWSTR aKeyNamesExt[] =
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
    { 0, NULL }
};

ROSDATA KBDTABLES_FE KbdTables =
{
    {
        &CharModifiers,
        (PVK_TO_WCHAR_TABLE)aVkToWcharTable,
        NULL,                    /* no dead keys */
        aKeyNames,
        aKeyNamesExt,
        NULL,                    /* no dead key names */
        ausVK,
        RTL_NUMBER_OF(ausVK),    /* 128 */
        aE0VscToVk,
        aE1VscToVk,
        0,                       /* fLocaleFlags */
        0, 0,                    /* nLgMaxd, cbLgEntry */
        NULL                     /* pLigature */
    },
    4,                           /* dwType (IBM enhanced) */
    0                            /* dwSubType */
};

/* NLS function-key table */
ROSDATA VK_F aVkToF[] =
{
    {
        VK_CAPITAL,
        2, 1, 8,
        {   /* NLSFEProc */
            { KBDNLS_SEND_BASE_VK, 0 },
            { KBDNLS_ALPHANUM, 0 },
            { KBDNLS_HIRAGANA, 0 },
            { KBDNLS_SEND_PARAM_VK, 21 },
            { KBDNLS_KATAKANA, 0 },
            { KBDNLS_SEND_BASE_VK, 0 },
            { KBDNLS_SEND_BASE_VK, 0 },
            { KBDNLS_SEND_BASE_VK, 0 },
        },
        {   /* NLSFEProcAlt */
            { KBDNLS_SEND_PARAM_VK, 21 },
            { KBDNLS_SEND_PARAM_VK, 21 },
            { KBDNLS_SEND_PARAM_VK, 21 },
            { KBDNLS_SEND_PARAM_VK, 21 },
            { KBDNLS_SEND_BASE_VK, 0 },
            { KBDNLS_SEND_BASE_VK, 0 },
            { KBDNLS_SEND_BASE_VK, 0 },
            { KBDNLS_SEND_BASE_VK, 0 },
        }
    },
    {
        VK_OEM_3,
        1, 1, 0,
        {   /* NLSFEProc */
            { KBDNLS_SEND_BASE_VK, 0 },
            { KBDNLS_SEND_BASE_VK, 0 },
            { KBDNLS_SBCSDBCS, 0 },
            { KBDNLS_SEND_BASE_VK, 0 },
            { KBDNLS_SEND_PARAM_VK, 25 },
            { KBDNLS_SEND_BASE_VK, 0 },
            { KBDNLS_SEND_BASE_VK, 0 },
            { KBDNLS_SEND_BASE_VK, 0 },
        },
        {   /* NLSFEProcAlt */
            { KBDNLS_NULL, 0 },
            { KBDNLS_NULL, 0 },
            { KBDNLS_NULL, 0 },
            { KBDNLS_NULL, 0 },
            { KBDNLS_NULL, 0 },
            { KBDNLS_NULL, 0 },
            { KBDNLS_NULL, 0 },
            { KBDNLS_NULL, 0 },
        }
    },
};

ROSDATA KBDNLSTABLES KbdNlsTables =
{
    0,                          /* OEMIdentifier */
    0,                          /* LayoutInformation */
    RTL_NUMBER_OF(aVkToF),      /* NumOfVkToF (2) */
    aVkToF,
    0,                          /* NumOfMouseVKey */
    NULL                        /* pusMouseVKey */
};

PKBDTABLES WINAPI KbdLayerDescriptor(VOID)
{
    return &KbdTables.Base;
}

PKBDNLSTABLES WINAPI KbdNlsLayerDescriptor(VOID)
{
    return &KbdNlsTables;
}
