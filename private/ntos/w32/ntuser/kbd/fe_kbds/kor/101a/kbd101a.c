/***************************************************************************\
* Module Name: kbd101a.c (Type A)
*
* Copyright (c) 1985-92, Microsoft Corporation
*
* Keyboard Type A :  Hangeul Toggle : Right Alt
*                    Junja   Toggle : Left  Alt + '='
*                    Hanja   Toggle : Right Ctrl
\***************************************************************************/

#include <windows.h>
#include <ime.h>
#include "vkoem.h"
#include "kbd101a.h"

#if defined(_M_IA64)
#pragma section(".data")
#define ALLOC_SECTION_LDATA __declspec(allocate(".data"))
#else
#pragma data_seg(".data")
#define ALLOC_SECTION_LDATA
#endif

/***************************************************************************\
* ausVK[] - Virtual Scan Code to Virtual Key conversion table for 101
\***************************************************************************/

static ALLOC_SECTION_LDATA USHORT ausVK[] = {
    T00, T01, T02, T03, T04, T05, T06, T07,
    T08, T09, T0A, T0B, T0C,

    /*
     * '='/'+' key must have KBDSPECIAL bit set (NLS key)
     */
    T0D | KBDSPECIAL,

                                  T0E, T0F,
    T10, T11, T12, T13, T14, T15, T16, T17,
    T18, T19, T1A, T1B, T1C, T1D, T1E, T1F,
    T20, T21, T22, T23, T24, T25, T26, T27,
    T28, T29, T2A, T2B, T2C, T2D, T2E, T2F,
    T30, T31, T32, T33, T34, T35,

    /*
     * Right-hand Shift key must have KBDEXT bit set.
     */
    T36 | KBDEXT,

    /*
     * numpad_* + Shift/Alt -> SnapShot
     */
    T37 | KBDMULTIVK,

    T38, T39, T3A, T3B, T3C, T3D, T3E, T3F,
    T40, T41, T42, T43, T44,

    /*
     * NumLock Key:
     *     KBDEXT     - VK_NUMLOCK is an Extended key
     *     KBDMULTIVK - VK_NUMLOCK or VK_PAUSE (without or with CTRL)
     */
    T45 | KBDEXT | KBDMULTIVK,

    T46 | KBDMULTIVK,

    /*
     * Number Pad keys:
     *     KBDNUMPAD  - digits 0-9 and decimal point.
     *     KBDSPECIAL - require special processing by Windows
     */
    T47 | KBDNUMPAD | KBDSPECIAL,   // Numpad 7 (Home)
    T48 | KBDNUMPAD | KBDSPECIAL,   // Numpad 8 (Up),
    T49 | KBDNUMPAD | KBDSPECIAL,   // Numpad 9 (PgUp),
    T4A,
    T4B | KBDNUMPAD | KBDSPECIAL,   // Numpad 4 (Left),
    T4C | KBDNUMPAD | KBDSPECIAL,   // Numpad 5 (Clear),
    T4D | KBDNUMPAD | KBDSPECIAL,   // Numpad 6 (Right),
    T4E,
    T4F | KBDNUMPAD | KBDSPECIAL,   // Numpad 1 (End),
    T50 | KBDNUMPAD | KBDSPECIAL,   // Numpad 2 (Down),
    T51 | KBDNUMPAD | KBDSPECIAL,   // Numpad 3 (PgDn),
    T52 | KBDNUMPAD | KBDSPECIAL,   // Numpad 0 (Ins),
    T53 | KBDNUMPAD | KBDSPECIAL,   // Numpad . (Del),

    T54, T55, T56, T57, T58, T59, T5A, T5B,
    T5C, T5D, T5E, T5F, T60, T61, T62, T63,
    T64, T65, T66, T67, T68, T69, T6A, T6B,
    T6C, T6D, T6E, T6F, T70, T71, T72, T73,
    T74, T75, T76, T77, T78, T79, T7A, T7B,
    T7C, T7D, T7E, T7F
};

static ALLOC_SECTION_LDATA VSC_VK aE0VscToVk[] = {
        { 0x10, X10 | KBDEXT              },  // Speedracer: Previous Track
        { 0x19, X19 | KBDEXT              },  // Speedracer: Next Track
        { 0x1C, X1C | KBDEXT              },  // Numpad Enter
        { 0x1D, X1D | KBDEXT | KBDSPECIAL },  // RControl // Hanja NLS key
        { 0x20, X20 | KBDEXT              },  // Speedracer: Volume Mute
        { 0x21, X21 | KBDEXT              },  // Speedracer: Launch App 2
        { 0x22, X22 | KBDEXT              },  // Speedracer: Media Play/Pause
        { 0x24, X24 | KBDEXT              },  // Speedracer: Media Stop
        { 0x2E, X2E | KBDEXT              },  // Speedracer: Volume Down
        { 0x30, X30 | KBDEXT              },  // Speedracer: Volume Up
        { 0x32, X32 | KBDEXT              },  // Speedracer: Browser Home
        { 0x35, X35 | KBDEXT              },  // Numpad Divide
        { 0x37, X37 | KBDEXT              },  // Snapshot
        { 0x38, X38 | KBDEXT | KBDSPECIAL },  // RMenu    // Hangeul NLS key
        { 0x46, X46 | KBDEXT              },  // Break (Ctrl + Pause)
        { 0x47, X47 | KBDEXT              },  // Home
        { 0x48, X48 | KBDEXT              },  // Up
        { 0x49, X49 | KBDEXT              },  // Prior
        { 0x4B, X4B | KBDEXT              },  // Left
        { 0x4D, X4D | KBDEXT              },  // Right
        { 0x4F, X4F | KBDEXT              },  // End
        { 0x50, X50 | KBDEXT              },  // Down
        { 0x51, X51 | KBDEXT              },  // Next
        { 0x52, X52 | KBDEXT              },  // Insert
        { 0x53, X53 | KBDEXT              },  // Delete
        { 0x5B, X5B | KBDEXT              },  // Left Win
        { 0x5C, X5C | KBDEXT              },  // Right Win
        { 0x5D, X5D | KBDEXT              },  // Applications
        { 0x5F, X5F | KBDEXT              },  // Speedracer: Sleep
        { 0x65, X65 | KBDEXT              },  // Speedracer: Browser Search
        { 0x66, X66 | KBDEXT              },  // Speedracer: Browser Favorites
        { 0x67, X67 | KBDEXT              },  // Speedracer: Browser Refresh
        { 0x68, X68 | KBDEXT              },  // Speedracer: Browser Stop
        { 0x69, X69 | KBDEXT              },  // Speedracer: Browser Forward
        { 0x6A, X6A | KBDEXT              },  // Speedracer: Browser Back
        { 0x6B, X6B | KBDEXT              },  // Speedracer: Launch App 1
        { 0x6C, X6C | KBDEXT              },  // Speedracer: Launch Mail
        { 0x6D, X6D | KBDEXT              },  // Speedracer: Launch Media Selector
        { 0xF1, XF1 | KBDEXT | KBDSPECIAL },  // Hanja
        { 0xF2, XF2 | KBDEXT | KBDSPECIAL },  // Hangeul
        { 0,      0                       }
};

static ALLOC_SECTION_LDATA VSC_VK aE1VscToVk[] = {
        { 0x1D, Y1D                       },  // Pause
        { 0   ,   0                       }
};

/***************************************************************************\
* aVkToBits[]  - map Virtual Keys to Modifier Bits
*
* See kbd.h for a full description.
*
* US Keyboard has only three shifter keys:
*     SHIFT (L & R) affects alphabnumeric keys,
*     CTRL  (L & R) is used to generate control characters
*     ALT   (L & R) used for generating characters by number with numpad
\***************************************************************************/

static ALLOC_SECTION_LDATA VK_TO_BIT aVkToBits[] = {
    { VK_SHIFT,   KBDSHIFT },
    { VK_CONTROL, KBDCTRL  },
    { VK_MENU,    KBDALT   },
    { 0,          0        }
};

/***************************************************************************\
* aModification[]  - map character modifier bits to modification number
*
* See kbd.h for a full description.
*
\***************************************************************************/

static ALLOC_SECTION_LDATA MODIFIERS CharModifiers = {
    &aVkToBits[0],
    3,
    {
    //  Modification# //  Keys Pressed  : Explanation
    //  ============= // ============== : =============================
        0,            //                : unshifted characters
        1,            //          SHIFT : capitals, ~!@#$%^&*()_+{}:"<>? etc.
        2,            //     CTRL       : control characters
        3,            //     CTRL SHIFT :
                      // ALT            : invalid
                      // ALT      SHIFT : invalid
                      // ALT CTRL       : invalid
                      // ALT CTRL SHIFT : invalid
    }
};

/***************************************************************************\
*
* aVkToWch2[]  - Virtual Key to WCHAR translation for 2 shift states
* aVkToWch3[]  - Virtual Key to WCHAR translation for 3 shift states
* aVkToWch4[]  - Virtual Key to WCHAR translation for 4 shift states
*
* Table attributes: Unordered Scan, null-terminated
*
* Search this table for an entry with a matching Virtual Key to find the
* corresponding unshifted and shifted WCHAR characters.
*
* Reserved VirtualKey values (first column)
*     -1            - this line contains dead characters (diacritic)
*     0             - terminator
*
* Reserved Attribute values (second column)
*     CAPLOK        - CapsLock affects this key like Shift
*
* Reserved character values (third through last column)
*     WCH_NONE      - No character
*     WCH_DEAD      - Dead character (diacritic) value is in next line
*
\***************************************************************************/

static ALLOC_SECTION_LDATA VK_TO_WCHARS2 aVkToWch2[] = {
    {'0'          , 0      ,'0'       ,')'       },
    {'1'          , 0      ,'1'       ,'!'       },
    {'3'          , 0      ,'3'       ,'#'       },
    {'4'          , 0      ,'4'       ,'$'       },
    {'5'          , 0      ,'5'       ,'%'       },
    {'7'          , 0      ,'7'       ,'&'       },
    {'8'          , 0      ,'8'       ,'*'       },
    {'9'          , 0      ,'9'       ,'('       },
    {'A'          , CAPLOK ,'a'       ,'A'       },
    {'B'          , CAPLOK ,'b'       ,'B'       },
    {'C'          , CAPLOK ,'c'       ,'C'       },
    {'D'          , CAPLOK ,'d'       ,'D'       },
    {'E'          , CAPLOK ,'e'       ,'E'       },
    {'F'          , CAPLOK ,'f'       ,'F'       },
    {'G'          , CAPLOK ,'g'       ,'G'       },
    {'H'          , CAPLOK ,'h'       ,'H'       },
    {'I'          , CAPLOK ,'i'       ,'I'       },
    {'J'          , CAPLOK ,'j'       ,'J'       },
    {'K'          , CAPLOK ,'k'       ,'K'       },
    {'L'          , CAPLOK ,'l'       ,'L'       },
    {'M'          , CAPLOK ,'m'       ,'M'       },
    {'N'          , CAPLOK ,'n'       ,'N'       },
    {'O'          , CAPLOK ,'o'       ,'O'       },
    {'P'          , CAPLOK ,'p'       ,'P'       },
    {'Q'          , CAPLOK ,'q'       ,'Q'       },
    {'R'          , CAPLOK ,'r'       ,'R'       },
    {'S'          , CAPLOK ,'s'       ,'S'       },
    {'T'          , CAPLOK ,'t'       ,'T'       },
    {'U'          , CAPLOK ,'u'       ,'U'       },
    {'V'          , CAPLOK ,'v'       ,'V'       },
    {'W'          , CAPLOK ,'w'       ,'W'       },
    {'X'          , CAPLOK ,'x'       ,'X'       },
    {'Y'          , CAPLOK ,'y'       ,'Y'       },
    {'Z'          , CAPLOK ,'z'       ,'Z'       },
    {VK_OEM_1     , 0      ,';'       ,':'       },
    {VK_OEM_2     , 0      ,'/'       ,'?'       },
    {VK_OEM_3     , 0      ,'`'       ,'~'       },
    {VK_OEM_7     , 0      ,0x27      ,'"'       },
    {VK_OEM_8     , 0      ,WCH_NONE  ,WCH_NONE  },
    {VK_OEM_COMMA , 0      ,','       ,'<'       },
    {VK_OEM_PERIOD, 0      ,'.'       ,'>'       },
    {VK_OEM_PLUS  , 0      ,'='       ,'+'       },
    {VK_TAB       , 0      ,'\t'      ,'\t'      },
    {VK_ADD       , 0      ,'+'       ,'+'       },
    {VK_DECIMAL   , 0      ,'.'       ,'.'       },
    {VK_DIVIDE    , 0      ,'/'       ,'/'       },
    {VK_MULTIPLY  , 0      ,'*'       ,'*'       },
    {VK_SUBTRACT  , 0      ,'-'       ,'-'       },
    {0            , 0      ,0         ,0         }
};

static ALLOC_SECTION_LDATA VK_TO_WCHARS3 aVkToWch3[] = {
    //                     |          |   SHIFT  |  CONTROL  |
    //                     |          |==========|===========|
    {VK_BACK      , 0      ,'\b'      ,'\b'      , 0x7f      },
    {VK_CANCEL    , 0      ,0x03      ,0x03      , 0x03      },
    {VK_ESCAPE    , 0      ,0x1b      ,0x1b      , 0x1b      },
    {VK_OEM_4     , 0      ,'['       ,'{'       , 0x1b      },
    {VK_OEM_5     , 0      ,'\\'      ,'|'       , 0x1c      },
    {VK_OEM_102   , 0      ,'\\'      ,'|'       , 0x1c      },
    {VK_OEM_6     , 0      ,']'       ,'}'       , 0x1d      },
    {VK_RETURN    , 0      ,'\r'      ,'\r'      , '\n'      },
    {VK_SPACE     , 0      ,' '       ,' '       , 0x20      },
    {0            , 0      ,0         ,0         , 0         }
};

static ALLOC_SECTION_LDATA VK_TO_WCHARS4 aVkToWch4[] = {
    //                     |          |   SHIFT  |  CONTROL  | SHFT+CTRL |
    //                     |          |==========|===========|===========|
    {'2'          , 0      ,'2'       ,'@'       , WCH_NONE  , 0x00      },
    {'6'          , 0      ,'6'       ,'^'       , WCH_NONE  , 0x1e      },
    {VK_OEM_MINUS , 0      ,'-'       ,'_'       , WCH_NONE  , 0x1f      },
    {0            , 0      ,0         ,0         , 0         , 0         }
};

// Put this last so that VkKeyScan interprets number characters
// as coming from the main section of the kbd (aVkToWch2 and
// aVkToWch4) before considering the numpad (aVkToWch1).

static ALLOC_SECTION_LDATA VK_TO_WCHARS1 aVkToWch1[] = {
    { VK_NUMPAD0   , 0      ,  '0'   },
    { VK_NUMPAD1   , 0      ,  '1'   },
    { VK_NUMPAD2   , 0      ,  '2'   },
    { VK_NUMPAD3   , 0      ,  '3'   },
    { VK_NUMPAD4   , 0      ,  '4'   },
    { VK_NUMPAD5   , 0      ,  '5'   },
    { VK_NUMPAD6   , 0      ,  '6'   },
    { VK_NUMPAD7   , 0      ,  '7'   },
    { VK_NUMPAD8   , 0      ,  '8'   },
    { VK_NUMPAD9   , 0      ,  '9'   },
    { 0            , 0      ,  '\0'  }   //null terminator
};

/***************************************************************************\
* aVkToWcharTable: table of pointers to Character Tables
*
* Describes the character tables and the order they should be searched.
*
* Note: the order determines the behavior of VkKeyScan() : this function
*       takes a character and attempts to find a Virtual Key and character-
*       modifier key combination that produces that character.  The table
*       containing the numeric keypad (aVkToWch1) must appear last so that
*       VkKeyScan('0') will be interpreted as one of keys from the main
*       section, not the numpad.  etc.
\***************************************************************************/

static ALLOC_SECTION_LDATA VK_TO_WCHAR_TABLE aVkToWcharTable[] = {
    {  (PVK_TO_WCHARS1)aVkToWch3, 3, sizeof(aVkToWch3[0]) },
    {  (PVK_TO_WCHARS1)aVkToWch4, 4, sizeof(aVkToWch4[0]) },
    {  (PVK_TO_WCHARS1)aVkToWch2, 2, sizeof(aVkToWch2[0]) },
    {  (PVK_TO_WCHARS1)aVkToWch1, 1, sizeof(aVkToWch1[0]) },  // must come last
    {                       NULL, 0, 0                    }
};

/***************************************************************************\
* aKeyNames[], aKeyNamesExt[]  - Scan Code -> Key Name tables
*
* For the GetKeyNameText() API function
*
* Tables for non-extended and extended (KBDEXT) keys.
* (Keys producing printable characters are named by the character itself)
\***************************************************************************/

static ALLOC_SECTION_LDATA VSC_LPWSTR aKeyNames[] = {
    0x01,    L"Esc",
    0x0e,    L"Backspace",
    0x0f,    L"Tab",
    0x1c,    L"Enter",
    0x1d,    L"Ctrl",
    0x2a,    L"Shift",
    0x36,    L"Right Shift",
    0x37,    L"Num *",
    0x38,    L"Alt",
    0x39,    L"Space",
    0x3a,    L"Caps Lock",
    0x3b,    L"F1",
    0x3c,    L"F2",
    0x3d,    L"F3",
    0x3e,    L"F4",
    0x3f,    L"F5",
    0x40,    L"F6",
    0x41,    L"F7",
    0x42,    L"F8",
    0x43,    L"F9",
    0x44,    L"F10",
    0x45,    L"Pause",
    0x46,    L"Scroll Lock",
    0x47,    L"Num 7",
    0x48,    L"Num 8",
    0x49,    L"Num 9",
    0x4a,    L"Num -",
    0x4b,    L"Num 4",
    0x4c,    L"Num 5",
    0x4d,    L"Num 6",
    0x4e,    L"Num +",
    0x4f,    L"Num 1",
    0x50,    L"Num 2",
    0x51,    L"Num 3",
    0x52,    L"Num 0",
    0x53,    L"Num Del",
    0x54,    L"Sys Req",
    0x57,    L"F11",
    0x58,    L"F12",
    0x7C,    L"F13",
    0x7D,    L"F14",
    0x7E,    L"F15",
    0x7F,    L"F16",
    0x80,    L"F17",
    0x81,    L"F18",
    0x82,    L"F19",
    0x83,    L"F20",
    0x84,    L"F21",
    0x85,    L"F22",
    0x86,    L"F23",
    0x87,    L"F24",
    0   ,    NULL
};

static ALLOC_SECTION_LDATA VSC_LPWSTR aKeyNamesExt[] = {
    0x1c,    L"Num Enter",
    0x1d,    L"Right Control",
    0x35,    L"Num /",
    0x37,    L"Prnt Scrn",
    0x38,    L"Right Alt",
    0x45,    L"Num Lock",
    0x46,    L"Break",
    0x47,    L"Home",
    0x48,    L"Up",
    0x49,    L"Page Up",
    0x4b,    L"Left",
    0x4d,    L"Right",
    0x4f,    L"End",
    0x50,    L"Down",
    0x51,    L"Page Down",
    0x52,    L"Insert",
    0x53,    L"Delete",
    0x5B,    L"Left Windows",
    0x5C,    L"Right Windows",
    0x5D,    L"Application",
    0xF1,    L"Hanja",
    0xF2,    L"Hangeul",
    0   ,    NULL
};

static ALLOC_SECTION_LDATA KBDTABLES KbdTables = {
    /*
     * Modifier keys
     */
    &CharModifiers,

    /*
     * Characters tables
     */
    aVkToWcharTable,

    /*
     * Diacritics  (none for US English)
     */
    NULL,

    /*
     * Names of Keys  (no dead keys)
     */
    aKeyNames,
    aKeyNamesExt,
    NULL,

    /*
     * Scan codes to Virtual Keys
     */
    ausVK,
    sizeof(ausVK) / sizeof(ausVK[0]),
    aE0VscToVk,
    aE1VscToVk,

    /*
     * No Locale-specific special processing
     */
    0
};

PKBDTABLES KbdLayerDescriptor(VOID)
{
    return &KbdTables;
}

/***********************************************************************\
* VkToFuncTable_101a[]
*
\***********************************************************************/

static ALLOC_SECTION_LDATA VK_F VkToFuncTable_101a[] = {
    {
        VK_OEM_PLUS,                 // Base Vk
        KBDNLS_TYPE_NORMAL,          // NLSFEProcType
        KBDNLS_INDEX_NORMAL,         // NLSFEProcCurrent
        0x0,                         // NLSFEProcSwitch
        {                            // NLSFEProcIndex
            {KBDNLS_SEND_BASE_VK,0},         // Base
            {KBDNLS_SEND_BASE_VK,0},         // Shift
            {KBDNLS_SEND_BASE_VK,0},         // Control
            {KBDNLS_SEND_BASE_VK,0},         // Shift+Control
            {KBDNLS_SEND_PARAM_VK,VK_JUNJA}, // Alt
            {KBDNLS_SEND_BASE_VK,0},         // Shift+Alt
            {KBDNLS_SEND_BASE_VK,0},         // Control+Alt
            {KBDNLS_SEND_BASE_VK,0}          // Shift+Control+Alt
        },
        {                            // NLSFEProcIndexAlt
            {KBDNLS_NULL,0},                 // Base
            {KBDNLS_NULL,0},                 // Shift
            {KBDNLS_NULL,0},                 // Control
            {KBDNLS_NULL,0},                 // Shift+Control
            {KBDNLS_NULL,0},                 // Alt
            {KBDNLS_NULL,0},                 // Shift+Alt
            {KBDNLS_NULL,0},                 // Control+Alt
            {KBDNLS_NULL,0}                  // Shift+Control+Alt
        }
    }
};

/***********************************************************************\
* KbdNlsTables
*
\***********************************************************************/

static ALLOC_SECTION_LDATA KBDNLSTABLES KbdNlsTables = {
    0,                      // OEM ID (0 = Microsoft)
    0,                      // Information
    1,                      // Number of VK_F entry
    VkToFuncTable_101a,     // Pointer to VK_F array
    0,                      // Number of MouseVk entry
    NULL                    // Pointer to MouseVk array
};

PKBDNLSTABLES KbdNlsLayerDescriptor(VOID)
{
    return &KbdNlsTables;
}
