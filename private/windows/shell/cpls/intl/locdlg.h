/*++

Copyright (c) 1994-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    locdlg.h

Abstract:

    This module contains the information for the input locale property
    sheet of the Regional Options applet.

Revision History:

--*/



#ifndef _LOCDLG_H
#define _LOCDLG_H



//
//  Constant Declarations.
//

#define US_LOCALE            MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)

#define IS_FE_LANGUAGE(p)    (((p) == LANG_CHINESE)  ||         \
                              ((p) == LANG_JAPANESE) ||         \
                              ((p) == LANG_KOREAN))

#define IS_DIRECT_SWITCH_HOTKEY(p)                                   \
                             (((p) >= IME_HOTKEY_DSWITCH_FIRST) &&   \
                              ((p) <= IME_HOTKEY_DSWITCH_LAST))

#define HKL_LEN              9           // max # chars in hkl id + null
#define DESC_MAX             MAX_PATH    // max size of a description
#define ALLOCBLOCK           3           // # items added to block for alloc/realloc

#define LIST_MARGIN          2           // for making the list box look good

#define MB_OK_OOPS           (MB_OK    | MB_ICONEXCLAMATION)    // msg box flags
#define MB_YN_OOPS           (MB_YESNO | MB_ICONEXCLAMATION)    // msg box flags

//
//  wStatus bit pile.
//
#define LANG_ACTIVE          0x0001      // language is active
#define LANG_ORIGACTIVE      0x0002      // language was active to start with
#define LANG_CHANGED         0x0004      // user changed status of language
#define ICON_LOADED          0x0010      // icon read in from file
#define LANG_DEFAULT         0x0020      // current language
#define LANG_DEF_CHANGE      0x0040      // language default has changed
#define LANG_IME             0x0080      // IME
#define LANG_HOTKEY          0x0100      // a hotkey has been defined
#define LANG_UPDATE          0x8000      // language needs to be updated

#define HOTKEY_SWITCH_LANG   0x0000      // id to switch between locales

#define MAX(i, j)            (((i) > (j)) ? (i) : (j))

#define LANG_OAC             (LANG_ORIGACTIVE | LANG_ACTIVE | LANG_CHANGED)

//
//  Bits for g_dwChanges.
//
#define CHANGE_SWITCH        0x0001
#define CHANGE_DEFAULT       0x0002
#define CHANGE_CAPSLOCK      0x0004

//
//  For the indicator on the tray.
//
#define IDM_NEWSHELL         249
#define IDM_EXIT             259

#define MOD_VIRTKEY          0x0080

//
// These are according to the US English kbd layout
//
#define VK_OEM_SEMICLN       0xba        //  ;    :
#define VK_OEM_EQUAL         0xbb        //  =    +
#define VK_OEM_SLASH         0xbf        //  /    ?
#define VK_OEM_LBRACKET      0xdb        //  [    {
#define VK_OEM_BSLASH        0xdc        //  \    |
#define VK_OEM_RBRACKET      0xdd        //  ]    }
#define VK_OEM_QUOTE         0xde        //  '    "

//
//  For the hot key switching.
//
#define DIALOG_SWITCH_INPUT_LOCALES     0
#define DIALOG_SWITCH_KEYBOARD_LAYOUT   1
#define DIALOG_SWITCH_IME               2




//
//  Typedef Declarations.
//

typedef struct langnode_s
{
    WORD wStatus;                   // status flags
    UINT iLayout;                   // offset into layout array
    HKL hkl;                        // hkl
    HKL hklUnload;                  // hkl of currently loaded layout
    UINT iLang;                     // offset into lang array
    HANDLE hLangNode;               // handle to free for this structure
    int nIconIME;                   // IME icon
    struct langnode_s *pNext;       // ptr to next langnode
    UINT uModifiers;                // hide Hotkey stuff here
    UINT uVKey;                     //   so we can rebuild the hotkey record
} LANGNODE, *LPLANGNODE;


typedef struct
{
    DWORD dwID;                     // language id
    ATOM atmLanguageName;           // language name - localized
    TCHAR szSymbol[3];              // 2 letter indicator symbol (+ null)
    UINT iUseCount;                 // usage count for this language
    UINT iNumCount;                 // number of links attached
    DWORD dwDefaultLayout;          // default layout id
    LPLANGNODE pNext;               // ptr to lang node structure
} INPUTLANG, *LPINPUTLANG;


typedef struct
{
    DWORD dwID;                     // numeric id
    BOOL bInstalled;                // if layout is installed
    UINT iSpecialID;                // special id (0xf001 for dvorak etc)
    ATOM atmLayoutFile;             // layout file name
    ATOM atmLayoutText;             // layout text name
    ATOM atmIMEFile;                // IME file name
} LAYOUT, *LPLAYOUT;

typedef struct
{
    DWORD dwHotKeyID;
    UINT  idHotKeyName;
    DWORD fdwEnable;
    UINT  uModifiers;
    UINT  uVKey;
    HKL   hkl;
    ATOM  atmHotKeyName;
    UINT  idxLayout;
} HOTKEYINFO, *LPHOTKEYINFO;

typedef struct
{
    HWND hwndMain;
    LPLANGNODE pLangNode;
    LPHOTKEYINFO pHotKeyNode;
} INITINFO, *LPINITINFO;

typedef struct
{
    UINT uVirtKeyValue;
    UINT idVirtKeyName;
    ATOM atVirtKeyName;
} VIRTKEYDESC;




//
//  Global Variables.
//

static VIRTKEYDESC g_aVirtKeyDesc[] =
{
    {0,               IDS_VK_NONE,          0},
    {VK_SPACE,        IDS_VK_SPACE,         0},
    {VK_PRIOR,        IDS_VK_PRIOR,         0},
    {VK_NEXT,         IDS_VK_NEXT,          0},
    {VK_END,          IDS_VK_END,           0},
    {VK_HOME,         IDS_VK_HOME,          0},
    {VK_F1,           IDS_VK_F1,            0},
    {VK_F2,           IDS_VK_F2,            0},
    {VK_F3,           IDS_VK_F3,            0},
    {VK_F4,           IDS_VK_F4,            0},
    {VK_F5,           IDS_VK_F5,            0},
    {VK_F6,           IDS_VK_F6,            0},
    {VK_F7,           IDS_VK_F7,            0},
    {VK_F8,           IDS_VK_F8,            0},
    {VK_F9,           IDS_VK_F9,            0},
    {VK_F10,          IDS_VK_F10,           0},
    {VK_F11,          IDS_VK_F11,           0},
    {VK_F12,          IDS_VK_F12,           0},
    {VK_OEM_SEMICLN,  IDS_VK_OEM_SEMICLN,   0},
    {VK_OEM_EQUAL,    IDS_VK_OEM_EQUAL,     0},
    {VK_OEM_COMMA,    IDS_VK_OEM_COMMA,     0},
    {VK_OEM_MINUS,    IDS_VK_OEM_MINUS,     0},
    {VK_OEM_PERIOD,   IDS_VK_OEM_PERIOD,    0},
    {VK_OEM_SLASH,    IDS_VK_OEM_SLASH,     0},
    {VK_OEM_3,        IDS_VK_OEM_3,         0},
    {VK_OEM_LBRACKET, IDS_VK_OEM_LBRACKET,  0},
    {VK_OEM_BSLASH,   IDS_VK_OEM_BSLASH,    0},
    {VK_OEM_RBRACKET, IDS_VK_OEM_RBRACKET,  0},
    {VK_OEM_QUOTE,    IDS_VK_OEM_QUOTE,     0},
    {'A',             IDS_VK_A + 0,         0},
    {'B',             IDS_VK_A + 1,         0},
    {'C',             IDS_VK_A + 2,         0},
    {'D',             IDS_VK_A + 3,         0},
    {'E',             IDS_VK_A + 4,         0},
    {'F',             IDS_VK_A + 5,         0},
    {'G',             IDS_VK_A + 6,         0},
    {'H',             IDS_VK_A + 7,         0},
    {'I',             IDS_VK_A + 8,         0},
    {'J',             IDS_VK_A + 9,         0},
    {'K',             IDS_VK_A + 10,        0},
    {'L',             IDS_VK_A + 11,        0},
    {'M',             IDS_VK_A + 12,        0},
    {'N',             IDS_VK_A + 13,        0},
    {'O',             IDS_VK_A + 14,        0},
    {'P',             IDS_VK_A + 15,        0},
    {'Q',             IDS_VK_A + 16,        0},
    {'R',             IDS_VK_A + 17,        0},
    {'S',             IDS_VK_A + 18,        0},
    {'T',             IDS_VK_A + 19,        0},
    {'U',             IDS_VK_A + 20,        0},
    {'V',             IDS_VK_A + 21,        0},
    {'W',             IDS_VK_A + 22,        0},
    {'X',             IDS_VK_A + 23,        0},
    {'Y',             IDS_VK_A + 24,        0},
    {'Z',             IDS_VK_A + 25,        0},
    {0,               IDS_VK_NONE1,         0},
    {'0',             IDS_VK_0 + 0,         0},
    {'1',             IDS_VK_0 + 1,         0},
    {'2',             IDS_VK_0 + 2,         0},
    {'3',             IDS_VK_0 + 3,         0},
    {'4',             IDS_VK_0 + 4,         0},
    {'5',             IDS_VK_0 + 5,         0},
    {'6',             IDS_VK_0 + 6,         0},
    {'7',             IDS_VK_0 + 7,         0},
    {'8',             IDS_VK_0 + 8,         0},
    {'9',             IDS_VK_0 + 9,         0},
    {'~',             IDS_VK_0 + 10,        0},
    {'`',             IDS_VK_0 + 11,        0},
};


static BOOL g_bAdmin_Privileges = FALSE;

static DWORD g_dwChanges = 0;

static LPINPUTLANG g_lpLang = NULL;
static UINT g_iLangBuff;
static HANDLE g_hLang;
static UINT g_nLangBuffSize;

static LPLAYOUT g_lpLayout = NULL;
static UINT g_iLayoutBuff;
static HANDLE g_hLayout;
static UINT g_nLayoutBuffSize;
static UINT g_iLayoutIME;         // Number of IME keyboard layouts.
static int g_iUsLayout;
static DWORD g_dwAttributes;

static int g_cyText;
static int g_cyListItem;
static int g_cxIcon;
static int g_cyIcon;

static HIMAGELIST g_himIndicators = NULL;

static TCHAR szLocaleInfo[]    = TEXT("SYSTEM\\CurrentControlSet\\Control\\Nls\\Locale");
static TCHAR szLayoutPath[]    = TEXT("SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts");
static TCHAR szLayoutFile[]    = TEXT("layout file");
static TCHAR szLayoutText[]    = TEXT("layout text");
static TCHAR szLayoutID[]      = TEXT("layout id");
static TCHAR szInstalled[]     = TEXT("installed");
static TCHAR szIMEFile[]       = TEXT("IME File");

static TCHAR szKbdLayouts[]    = TEXT("Keyboard Layout");
static TCHAR szPreloadKey[]    = TEXT("Preload");
static TCHAR szSubstKey[]      = TEXT("Substitutes");
static TCHAR szToggleKey[]     = TEXT("Toggle");
static TCHAR szAttributes[]    = TEXT("Attributes");
static TCHAR szKbdPreloadKey[] = TEXT("Keyboard Layout\\Preload");
static TCHAR szKbdSubstKey[]   = TEXT("Keyboard Layout\\Substitutes");
static TCHAR szKbdToggleKey[]  = TEXT("Keyboard Layout\\Toggle");
static TCHAR szInternat[]      = TEXT("internat.exe");
static char  szInternatA[]     = "internat.exe";

static TCHAR szScanCodeKey[]     = TEXT("Keyboard Layout\\IMEtoggle\\scancode");
static TCHAR szValueShiftLeft[]  = TEXT("Shift Left");
static TCHAR szValueShiftRight[] = TEXT("Shift Right");

static TCHAR szIndicator[]     = TEXT("Indicator");

static TCHAR szLoadImmPath[]   = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\IMM");


static HOTKEYINFO g_aDirectSwitchHotKey[IME_HOTKEY_DSWITCH_LAST - IME_HOTKEY_DSWITCH_FIRST + 1];
#define DSWITCH_HOTKEY_SIZE sizeof(g_aDirectSwitchHotKey) / sizeof(HOTKEYINFO)

static HOTKEYINFO g_SwitchLangHotKey;

static HOTKEYINFO g_aImeHotKey0404[] =
{
    {IME_ITHOTKEY_RESEND_RESULTSTR,     IDS_RESEND_RESULTSTR_CHT,
        MOD_VIRTKEY|MOD_CONTROL|MOD_ALT|MOD_SHIFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_ITHOTKEY_PREVIOUS_COMPOSITION, IDS_PREVIOUS_COMPOS_CHT,
        MOD_VIRTKEY|MOD_CONTROL|MOD_ALT|MOD_SHIFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_ITHOTKEY_UISTYLE_TOGGLE,       IDS_UISTYLE_TOGGLE_CHT,
        MOD_VIRTKEY|MOD_CONTROL|MOD_ALT|MOD_SHIFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_THOTKEY_IME_NONIME_TOGGLE,     IDS_IME_NONIME_TOGGLE_CHT,
        MOD_LEFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_THOTKEY_SHAPE_TOGGLE,          IDS_SHAPE_TOGGLE_CHT,
        MOD_LEFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_THOTKEY_SYMBOL_TOGGLE,         IDS_SYMBOL_TOGGLE_CHT,
        MOD_VIRTKEY|MOD_CONTROL|MOD_ALT|MOD_SHIFT,
        0, 0, (HKL)NULL, 0, -1},
};

static HOTKEYINFO g_aImeHotKey0804[] =
{

    {IME_CHOTKEY_IME_NONIME_TOGGLE,     IDS_IME_NONIME_TOGGLE_CHS,
        MOD_LEFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_CHOTKEY_SHAPE_TOGGLE,          IDS_SHAPE_TOGGLE_CHS,
        MOD_LEFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_CHOTKEY_SYMBOL_TOGGLE,         IDS_SYMBOL_TOGGLE_CHS,
        MOD_VIRTKEY|MOD_CONTROL|MOD_ALT|MOD_SHIFT,
        0, 0, (HKL)NULL, 0, -1},

};


static HOTKEYINFO g_aImeHotKeyCHxBoth[]=
{

// CHS HOTKEYs,

    {IME_CHOTKEY_IME_NONIME_TOGGLE,     IDS_IME_NONIME_TOGGLE_CHS,
        MOD_LEFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_CHOTKEY_SHAPE_TOGGLE,          IDS_SHAPE_TOGGLE_CHS,
        MOD_LEFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_CHOTKEY_SYMBOL_TOGGLE,         IDS_SYMBOL_TOGGLE_CHS,
        MOD_VIRTKEY|MOD_CONTROL|MOD_ALT|MOD_SHIFT,
        0, 0, (HKL)NULL, 0, -1},

// CHT HOTKEYs,

    {IME_ITHOTKEY_RESEND_RESULTSTR,     IDS_RESEND_RESULTSTR_CHT,
        MOD_VIRTKEY|MOD_CONTROL|MOD_ALT|MOD_SHIFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_ITHOTKEY_PREVIOUS_COMPOSITION, IDS_PREVIOUS_COMPOS_CHT,
        MOD_VIRTKEY|MOD_CONTROL|MOD_ALT|MOD_SHIFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_ITHOTKEY_UISTYLE_TOGGLE,       IDS_UISTYLE_TOGGLE_CHT,
        MOD_VIRTKEY|MOD_CONTROL|MOD_ALT|MOD_SHIFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_THOTKEY_IME_NONIME_TOGGLE,     IDS_IME_NONIME_TOGGLE_CHT,
        MOD_LEFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_THOTKEY_SHAPE_TOGGLE,          IDS_SHAPE_TOGGLE_CHT,
        MOD_LEFT,
        0, 0, (HKL)NULL, 0, -1},
    {IME_THOTKEY_SYMBOL_TOGGLE,         IDS_SYMBOL_TOGGLE_CHT,
        MOD_VIRTKEY|MOD_CONTROL|MOD_ALT|MOD_SHIFT,
        0, 0, (HKL)NULL, 0, -1},
};

//
//  Function Prototypes.
//

INT_PTR CALLBACK
KbdLocaleAddDlg(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

INT_PTR CALLBACK
KbdLocaleEditDlg(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

INT_PTR CALLBACK
KbdLocaleChangeInputLocaleHotkey(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

INT_PTR CALLBACK
KbdLocaleChangeThaiInputLocaleHotkey(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

INT_PTR CALLBACK
KbdLocaleChangeKeyboardLayoutHotkey(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

INT_PTR CALLBACK
KbdLocaleChangeImeHotkey(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);


#endif // _LOCDLG_H
