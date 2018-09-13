/*++

Copyright (c) 1994-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    locdlg.c

Abstract:

    This module implements the input locale property sheet for the Regional
    Settings applet.

Revision History:

--*/



//
//  Include Files.
//

#include "intl.h"
#include <windowsx.h>
#include <regstr.h>
#include <setupapi.h>
#include <immp.h>
#include <help.h>
#include "locdlg.h"


//
//  Context Help Ids.
//

static int aLocaleHelpIds[] =
{
    IDC_KBDL_LOCALE,           IDH_KEYB_INPUT_LIST,
    IDC_KBDL_LAYOUT_TEXT,      IDH_KEYB_INPUT_LIST,
    IDC_KBDL_LOCALE_LIST,      IDH_KEYB_INPUT_LIST,
    IDC_KBDL_ADD,              IDH_KEYB_INPUT_ADD,
    IDC_KBDL_EDIT,             IDH_KEYB_INPUT_PROP,
    IDC_KBDL_DELETE,           IDH_KEYB_INPUT_DEL,
    IDC_KBDL_DISABLED,         NO_HELP,
    IDC_KBDL_DISABLED_2,       NO_HELP,
    IDC_KBDL_CAPSLOCK_FRAME,   IDH_KEYB_CAPSLOCK_LAYOUT,
    IDC_KBDL_CAPSLOCK,         IDH_KEYB_CAPSLOCK_LAYOUT,
    IDC_KBDL_SHIFTLOCK,        IDH_KEYB_CAPSLOCK_LAYOUT,
    IDC_KBDL_INPUT_FRAME,      IDH_COMM_GROUPBOX,
    IDC_KBDL_SET_DEFAULT,      IDH_KEYB_INPUT_DEFAULT,
    IDC_KBDL_INDICATOR,        IDH_KEYB_INPUT_INDICATOR,
    IDC_KBDL_ONSCRNKBD,        IDH_KEYB_INPUT_ONSCRN_KEYB,
    IDC_KBDL_IME_SETTINGS,     IDH_KEYB_IME_SETTINGS,
    IDC_KBDL_HOTKEY_FRAME,     NO_HELP,
    IDC_KBDL_HOTKEY_LIST,      IDH_KEYB_HOTKEY_LIST,
    IDC_KBDL_HOTKEY,           IDH_KEYB_HOTKEY_LIST,
    IDC_KBDL_HOTKEY_SEQUENCE,  IDH_KEYB_HOTKEY_LIST,
    IDC_KBDL_CHANGE_HOTKEY,    IDH_KEYB_CHANGE_HOTKEY,

    0, 0
};


static int aLocaleAddHelpIds[] =
{
    IDC_KBDLA_LOCALE,          IDH_KEYB_INPUT_LANG,
    IDC_KBDLA_LAYOUT,          IDH_KEYB_INPUT_PROP_KEYLAY,

    0, 0
};


static int aLocalePropHelpIds[] =
{
    IDC_KBDLE_LOCALE_TXT,      IDH_KEYB_INPUT_PROP_LANG,
    IDC_KBDLE_LOCALE,          IDH_KEYB_INPUT_PROP_LANG,
    IDC_KBDLE_LAYOUT,          IDH_KEYB_INPUT_PROP_KEYLAY,

    0, 0
};


static int aLocaleHotkeyHelpIds[] =
{
    IDC_KBDLH_LAYOUT_TEXT,     NO_HELP,
    IDC_KBDLH_ENABLE,          IDH_KEYB_CHANGE_KEY,
    IDC_KBDLH_CTRL,            IDH_KEYB_CHANGE_KEY,
    IDC_KBDLH_L_ALT,           IDH_KEYB_CHANGE_KEY,
    IDC_KBDLH_SHIFT,           IDH_KEYB_CHANGE_KEY,
    IDC_KBDLH_PLUS,            IDH_KEYB_CHANGE_KEY,
    IDC_KBDLH_KEY_COMBO,       IDH_KEYB_CHANGE_KEY,
    IDC_KBDLH_VLINE,           IDH_KEYB_CHANGE_KEY,
    IDC_KBDLH_GRAVE,           IDH_KEYB_CHANGE_KEY,

    0, 0
};




//
//  Global Variables.
//

static BOOL g_bSetupCase = FALSE;
int g_nDefaultCheck;

TCHAR szPropHwnd[] = TEXT("PROP_HWND");
TCHAR szPropIdx[]  = TEXT("PROP_IDX");

static BOOL g_bGetSwitchLangHotKey = TRUE;




//
//  External Routines.
//

extern void Region_RebootTheSystem();

extern BOOL Region_OpenIntlInfFile(HINF *phInf);

extern BOOL Region_CloseInfFile(HINF *phInf);

extern BOOL Region_ReadDefaultLayoutFromInf(
    LPTSTR pszLocale,
    LPDWORD pdwLocale,
    LPDWORD pdwLayout,
    LPDWORD pdwLocale2,
    LPDWORD pdwLayout2,
    HINF hIntlInf);





////////////////////////////////////////////////////////////////////////////
//
//  Locale_ErrorMsg
//
//  Sound a beep and put up the given error message.
//
////////////////////////////////////////////////////////////////////////////

void Locale_ErrorMsg(
    HWND hwnd,
    UINT iErr,
    LPTSTR szValue)
{
    TCHAR sz[DESC_MAX];
    TCHAR szString[DESC_MAX];

    //
    //  Sound a beep.
    //
    MessageBeep(MB_OK);

    //
    //  Put up the appropriate error message box.
    //
    if (LoadString(hInstance, iErr, sz, DESC_MAX))
    {
        //
        //  If the caller wants to display a message with a caller supplied
        //  value string, do it.
        //
        if (szValue)
        {
            wsprintf(szString, sz, szValue);
            MessageBox(hwnd, szString, NULL, MB_OK_OOPS);
        }
        else
        {
            MessageBox(hwnd, sz, NULL, MB_OK_OOPS);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_ApplyError
//
//  Put up the given error message with the language name in it.
//
//  NOTE: This error is NOT fatal - as we could be half way through the
//        list before an error occurs.  The registry will already have
//        some information and we should let them have what comes next
//        as well.
//
////////////////////////////////////////////////////////////////////////////

int Locale_ApplyError(
    HWND hwnd,
    LPLANGNODE pLangNode,
    UINT iErr,
    UINT iStyle)
{
    UINT idxLang, idxLayout;
    TCHAR sz[MAX_PATH];
    TCHAR szTemp[MAX_PATH];
    TCHAR szLangName[MAX_PATH * 2];
    LPTSTR pszLang;

    //
    //  Load in the string for the given string id.
    //
    LoadString(hInstance, iErr, sz, MAX_PATH);

    //
    //  Get the language name to fill into the above string.
    //
    if (pLangNode)
    {
        idxLang = pLangNode->iLang;
        idxLayout = pLangNode->iLayout;
        GetAtomName(g_lpLang[idxLang].atmLanguageName, szLangName, MAX_PATH);
        if (g_lpLang[idxLang].dwID != g_lpLayout[idxLayout].dwID)
        {
            pszLang = szLangName + lstrlen(szLangName);
            pszLang[0] = TEXT(' ');
            pszLang[1] = TEXT('-');
            pszLang[2] = TEXT(' ');
            GetAtomName( g_lpLayout[idxLayout].atmLayoutText,
                         pszLang + 3,
                         MAX_PATH - 3 );
        }
    }
    else
    {
        LoadString(hInstance, IDS_UNKNOWN, szLangName, MAX_PATH);
    }

    //
    //  Put up the error message box.
    //
    wsprintf(szTemp, sz, szLangName);
    return (MessageBox(hwnd, szTemp, NULL, iStyle));
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_FetchIndicator
//
//  Saves the two letter indicator symbol for the given language in the
//  g_lpLang array.
//
////////////////////////////////////////////////////////////////////////////

void Locale_FetchIndicator(
    LPLANGNODE pLangNode)
{
    TCHAR szData[MAX_PATH];
    LPINPUTLANG pInpLang = &g_lpLang[pLangNode->iLang];

    pLangNode->wStatus |= ICON_LOADED;

    //
    //  See if it's an IME.  If so, use the IME icon.
    //
    if (pLangNode->wStatus & LANG_IME)
    {
        TCHAR szFileName[MAX_PATH];
        HICON hIcon = NULL;

        if (g_himIndicators != NULL)
        {
            GetAtomName( g_lpLayout[pLangNode->iLayout].atmIMEFile,
                         szFileName,
                         MAX_PATH );
            ExtractIconEx(szFileName, 0, (HICON *)&hIcon, NULL, 1);

            if (hIcon)
            {
                pLangNode->nIconIME = ImageList_AddIcon( g_himIndicators,
                                                         hIcon );
            }
            else
            {
                pLangNode->nIconIME = -1;
                pLangNode->wStatus &= ~ICON_LOADED;
            }
            DestroyIcon(hIcon);
            if (pLangNode->nIconIME != -1)
            {
                return;
            }
        }
    }
    else
    {
        pLangNode->nIconIME = -1;
    }

    //
    //  Get the indicator by using the first 2 characters of the
    //  abbreviated language name.
    //
    if (GetLocaleInfo( LOWORD(pInpLang->dwID),
                       LOCALE_SABBREVLANGNAME | LOCALE_NOUSEROVERRIDE,
                       szData,
                       MAX_PATH ))
    {
        //
        //  Save the first two characters.
        //
        pInpLang->szSymbol[0] = szData[0];
        pInpLang->szSymbol[1] = szData[1];
        pInpLang->szSymbol[2] = TEXT('\0');
    }
    else
    {
        //
        //  Id wasn't found.  Return question marks.
        //
        pInpLang->szSymbol[0] = TEXT('?');
        pInpLang->szSymbol[1] = TEXT('?');
        pInpLang->szSymbol[2] = TEXT('\0');
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_IsIndicatorPresent
//
////////////////////////////////////////////////////////////////////////////

BOOL Locale_IsIndicatorPresent()
{
    HKEY hkey;
    DWORD dwResultLen;
    ULONG rc;

    if (RegOpenKey( HKEY_CURRENT_USER,
                    REGSTR_PATH_RUN,
                    &hkey ) != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    rc = RegQueryValueEx( hkey,
                          szInternat,
                          NULL,
                          NULL,
                          NULL,
                          &dwResultLen );
    RegCloseKey(hkey);

    return ((rc == ERROR_SUCCESS) ? TRUE : FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_SetSecondaryControls
//
//  Sets the secondary controls to either be enabled or disabled.  When
//  there is only 1 active keyboard layout, then this function will be
//  called to disable these controls.
//
////////////////////////////////////////////////////////////////////////////

void Locale_SetSecondaryControls(
    HWND hwndMain,
    BOOL bOn)
{
    EnableWindow(GetDlgItem(hwndMain, IDC_KBDL_INDICATOR), bOn);
    CheckDlgButton(hwndMain, IDC_KBDL_INDICATOR, bOn);

    EnableWindow(GetDlgItem(hwndMain, IDC_KBDL_DELETE), bOn);

    EnableWindow(GetDlgItem(hwndMain, IDC_KBDL_SET_DEFAULT), bOn);

    bOn = (ListBox_GetCount(GetDlgItem(hwndMain, IDC_KBDL_HOTKEY_LIST)) > 0)
            ? TRUE
            : FALSE;
    EnableWindow(GetDlgItem(hwndMain, IDC_KBDL_CHANGE_HOTKEY), bOn);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_SetDefaultHotKey
//
//  Set the default hotkey for a locale switch.
//
////////////////////////////////////////////////////////////////////////////

void Locale_SetDefaultHotKey(
    HWND hwnd,
    BOOL bAdd)
{
    HWND hwndList = GetDlgItem(hwnd, IDC_KBDL_LOCALE_LIST);
    HWND hwndHotkey = GetDlgItem(hwnd, IDC_KBDL_HOTKEY_LIST);
    LPHOTKEYINFO pHotKeyNode;
    LPLANGNODE pLangNode;
    int nLangs, ctr;
    BOOL fThai = FALSE;

    //
    //  Check if the Thai keyboard layout is loaded.
    //
    nLangs = ListBox_GetCount(hwndList);
    for (ctr = 0; ctr < nLangs; ctr++)
    {
        pLangNode = (LPLANGNODE)ListBox_GetItemData(hwndList, ctr);
        if (PRIMARYLANGID(LOWORD(g_lpLayout[pLangNode->iLayout].dwID)) == LANG_THAI)
        {
            fThai = TRUE;
            break;
        }
    }

    //
    //  Get the current hotkey node for the default locale switch.
    //  It's the first item in the list.
    //
    pHotKeyNode = (LPHOTKEYINFO)ListBox_GetItemData(hwndHotkey, 0);

    //
    //  See if we are adding or deleting a new language-layout combination.
    //
    if (bAdd && (nLangs == 2))
    {
        //
        //  Define default locale switch hotkey as Grave accent if
        //  system default locale is Thai and we have a Thai keyboard
        //  layout loaded.
        //
        if ((PRIMARYLANGID(LANGIDFROMLCID(SysLocaleID)) == LANG_THAI) && fThai)
        {
            pHotKeyNode->uVKey = CHAR_GRAVE;
            pHotKeyNode->uModifiers &= ~(MOD_CONTROL | MOD_ALT | MOD_SHIFT);
            g_dwChanges |= CHANGE_SWITCH;
        }
        else
        {
            //
            //  No Thai locale loaded, so default locale switch hotkey
            //  is Left-Alt + Shift.
            //
            pHotKeyNode->uVKey = 0;
            pHotKeyNode->uModifiers = MOD_ALT | MOD_SHIFT;
            g_dwChanges |= CHANGE_SWITCH;
        }
    }
    else
    {
        //
        //  Deleting a language-layout combination.
        //
        if (nLangs == 1)
        {
            //
            //  Remove the locale hotkey, since it's no longer required.
            //
            pHotKeyNode->uVKey = 0;
            pHotKeyNode->uModifiers &= ~(MOD_CONTROL | MOD_ALT | MOD_SHIFT);
            g_dwChanges |= CHANGE_SWITCH;
        }
        else if (!fThai && (pHotKeyNode->uVKey == CHAR_GRAVE))
        {
            //
            //  Reset the locale switch hotkey from Grave accent to
            //  Left-Alt + Shift.
            //
            pHotKeyNode->uVKey = 0;
            pHotKeyNode->uModifiers = MOD_ALT | MOD_SHIFT;
            g_dwChanges |= CHANGE_SWITCH;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_AddToLinkedList
//
//  Adds an Input Locale to the main g_lpLang array.
//
////////////////////////////////////////////////////////////////////////////

LPLANGNODE Locale_AddToLinkedList(
    UINT idx,
    HKL hkl)
{
    LPINPUTLANG pInpLang = &g_lpLang[idx];
    LPLANGNODE pLangNode;
    LPLANGNODE pTemp;
    HANDLE hLangNode;

    //
    //  Create the new node.
    //
    if (!(hLangNode = GlobalAlloc(GHND, sizeof(LANGNODE))))
    {
        return (NULL);
    }
    pLangNode = GlobalLock(hLangNode);

    //
    //  Fill in the new node with the appropriate info.
    //
    pLangNode->wStatus = 0;
    pLangNode->iLayout = (UINT)(-1);
    pLangNode->hkl = hkl;
    pLangNode->hklUnload = hkl;
    pLangNode->iLang = idx;
    pLangNode->hLangNode = hLangNode;
    pLangNode->pNext = NULL;
    pLangNode->nIconIME = -1;

    //
    //  If an hkl is given, see if it's an IME.  If so, mark the status bit.
    //
    if ((hkl) && ((HIWORD(hkl) & 0xf000) == 0xe000))
    {
        pLangNode->wStatus |= LANG_IME;
    }

    //
    //  Put the new node in the list.
    //
    pTemp = pInpLang->pNext;
    if (pTemp == NULL)
    {
        pInpLang->pNext = pLangNode;
    }
    else
    {
        while (pTemp->pNext != NULL)
        {
            pTemp = pTemp->pNext;
        }
        pTemp->pNext = pLangNode;
    }

    //
    //  Increment the count.
    //
    pInpLang->iNumCount++;

    //
    //  Return the pointer to the new node.
    //
    return (pLangNode);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_RemoveFromLinkedList
//
//  Removes a link from the linked list.
//
////////////////////////////////////////////////////////////////////////////

void Locale_RemoveFromLinkedList(
    LPLANGNODE pLangNode)
{
    LPINPUTLANG pInpLang;
    LPLANGNODE pPrev;
    LPLANGNODE pCur;
    HANDLE hCur;

    pInpLang = &g_lpLang[pLangNode->iLang];

    //
    //  Find the node in the list.
    //
    pPrev = NULL;
    pCur = pInpLang->pNext;

    while (pCur && (pCur != pLangNode))
    {
        pPrev = pCur;
        pCur = pCur->pNext;
    }

    if (pPrev == NULL)
    {
        if (pCur == pLangNode)
        {
            pInpLang->pNext = pCur->pNext;
        }
        else
        {
            pInpLang->pNext = NULL;
        }
    }
    else if (pCur)
    {
        pPrev->pNext = pCur->pNext;
    }

    //
    //  Remove the node from the list.
    //
    if (pCur)
    {
        hCur = pCur->hLangNode;
        GlobalUnlock(hCur);
        GlobalFree(hCur);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_GetImeHotKeyInfo
//
//  Initializes array for CHS/CHT specific IME related hotkey items.
//
////////////////////////////////////////////////////////////////////////////

int Locale_GetImeHotKeyInfo(
    HWND         hwnd,
    LPHOTKEYINFO *aImeHotKey)
{

    HWND       hwndList = GetDlgItem(hwnd, IDC_KBDL_LOCALE_LIST);
    LPLANGNODE pLangNode;
    LANGID     LangID;
    int        nLangs, ctr;
    BOOL       fCHS, fCHT;

    fCHS = fCHT = FALSE;
    nLangs = ctr = 0;

    //
    //  Check if the CHS or CHT layouts are loaded.
    //
    nLangs = ListBox_GetCount(hwndList);

    if ( nLangs == LB_ERR ){
        *aImeHotKey = NULL;
        return (0);
    }

    for (ctr = 0; ctr < nLangs; ctr++)
    {
        pLangNode = (LPLANGNODE)ListBox_GetItemData(hwndList, ctr);

        if ( (LPARAM)pLangNode == LB_ERR ) {
           continue;
        }

        LangID = LOWORD(g_lpLayout[pLangNode->iLayout].dwID);

        if ( PRIMARYLANGID(LangID) == LANG_CHINESE )
        {
           if ( SUBLANGID(LangID) == SUBLANG_CHINESE_SIMPLIFIED)
              fCHS = TRUE;
           else if ( SUBLANGID(LangID) == SUBLANG_CHINESE_TRADITIONAL )
                   fCHT = TRUE;
        }
    }

    if ( (fCHS == TRUE)  && (fCHT == TRUE) )
    {
        // Both CHS and CHT IMEs are Loaded

        *aImeHotKey = g_aImeHotKeyCHxBoth;
        return(sizeof(g_aImeHotKeyCHxBoth) / sizeof(HOTKEYINFO) );
    }
    else
    {
        if ( fCHS == TRUE )
        {
          // only CHS IMEs are loaded

            *aImeHotKey = g_aImeHotKey0804;
            return (sizeof(g_aImeHotKey0804) / sizeof(HOTKEYINFO));
        }

        if ( fCHT == TRUE )
        {

          // Only CHT IMEs are loaded.

            *aImeHotKey = g_aImeHotKey0404;
            return (sizeof(g_aImeHotKey0404) / sizeof(HOTKEYINFO));
        }

    }

    // all other cases, No Chinese IME is loaded.

    *aImeHotKey=NULL;
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_GetHotkeys
//
//  Gets the hotkey keyboard switch value from the registry and then
//  sets the appropriate radio button in the dialog.
//
////////////////////////////////////////////////////////////////////////////

void Locale_GetHotkeys(
    HWND hwnd)
{
    TCHAR sz[10];
    DWORD cb;
    HKEY hkey;
    int ctr1, iLangCount, iCount;
    UINT iIndex;
    TCHAR szLanguage[DESC_MAX];
    TCHAR szLayout[DESC_MAX];
    TCHAR szItem[DESC_MAX];
    TCHAR szAction[DESC_MAX];
    LPLANGNODE pLangNode;
    BOOL bHasIme = FALSE;
    HWND hwndList = GetDlgItem(hwnd, IDC_KBDL_LOCALE_LIST);
    HWND hwndHotkey = GetDlgItem(hwnd, IDC_KBDL_HOTKEY_LIST);
    LPHOTKEYINFO aImeHotKey;

    //
    //  Clear out the hot keys list box.
    //
    ListBox_ResetContent(hwndHotkey);

    //
    //  Get the number of items in the input locales list.
    //
    iLangCount = ListBox_GetCount(hwndList);

    //
    //  Get the hotkey value to switch between locales from the registry.
    //
    if (g_bGetSwitchLangHotKey)
    {
        g_SwitchLangHotKey.dwHotKeyID = HOTKEY_SWITCH_LANG;
        g_SwitchLangHotKey.fdwEnable = MOD_CONTROL | MOD_ALT | MOD_SHIFT;

        LoadString( hInstance,
                    IDS_KBD_SWITCH_LOCALE,
                    szItem,
                    sizeof(szItem) / sizeof(TCHAR) );

        g_SwitchLangHotKey.atmHotKeyName = AddAtom(szItem);

        sz[0] = 0;
        if (RegOpenKey(HKEY_CURRENT_USER, szKbdToggleKey, &hkey) == ERROR_SUCCESS)
        {
            cb = sizeof(sz);
            RegQueryValueEx(hkey, TEXT("Hotkey"), NULL, NULL, (LPBYTE)sz, &cb);
            RegCloseKey(hkey);
        }

        //
        //  Set the modifiers.
        //
        if (sz[1] == 0)
        {
            switch (sz[0])
            {
                case ( TEXT('1') ) :
                {
                    g_SwitchLangHotKey.uModifiers = MOD_ALT | MOD_SHIFT;
                    break;
                }
                case ( TEXT('2') ) :
                {
                    g_SwitchLangHotKey.uModifiers = MOD_CONTROL | MOD_SHIFT;
                    break;
                }
                case ( TEXT('3') ) :
                {
                    g_SwitchLangHotKey.uModifiers = 0;
                    break;
                }
                case ( TEXT('4') ) :
                {
                    g_SwitchLangHotKey.uModifiers = 0;
                    g_SwitchLangHotKey.uVKey = CHAR_GRAVE;
                    break;
                }
            }
        }
        g_bGetSwitchLangHotKey = FALSE;
    }

    iIndex = ListBox_InsertString(hwndHotkey, -1, szItem);
    ListBox_SetItemData(hwndHotkey, iIndex, (LONG_PTR)&g_SwitchLangHotKey);

    //
    //  Determine the hotkey value for direct locale switch.
    //
    //  Query all available direct switch hotkey IDs and put the
    //  corresponding hkl, key, and modifiers information into the array.
    //
    for (ctr1 = 0; ctr1 < DSWITCH_HOTKEY_SIZE; ctr1++)
    {
        BOOL fRet;

        g_aDirectSwitchHotKey[ctr1].dwHotKeyID = IME_HOTKEY_DSWITCH_FIRST + ctr1;
        g_aDirectSwitchHotKey[ctr1].fdwEnable = MOD_VIRTKEY | MOD_CONTROL |
                                                MOD_ALT | MOD_SHIFT |
                                                MOD_LEFT | MOD_RIGHT;
        g_aDirectSwitchHotKey[ctr1].idxLayout = -1;

        fRet = ImmGetHotKey( g_aDirectSwitchHotKey[ctr1].dwHotKeyID,
                             &g_aDirectSwitchHotKey[ctr1].uModifiers,
                             &g_aDirectSwitchHotKey[ctr1].uVKey,
                             &g_aDirectSwitchHotKey[ctr1].hkl );
        if (!fRet)
        {
            g_aDirectSwitchHotKey[ctr1].uModifiers = 0;

            if ((g_aDirectSwitchHotKey[ctr1].fdwEnable & (MOD_LEFT | MOD_RIGHT)) ==
                (MOD_LEFT | MOD_RIGHT))
            {
                g_aDirectSwitchHotKey[ctr1].uModifiers |= MOD_LEFT | MOD_RIGHT;
            }
            g_aDirectSwitchHotKey[ctr1].uVKey = 0;
            g_aDirectSwitchHotKey[ctr1].hkl = (HKL)NULL;
        }
    }

    LoadString( hInstance,
                IDS_KBD_SWITCH_TO,
                szAction,
                sizeof(szAction) / sizeof(TCHAR) );

    //
    //  Try to find either a matching hkl or empty spot in the array
    //  for each of the hkls in the locale list.
    //
    for (ctr1 = 0; ctr1 < iLangCount; ctr1++)
    {
        int ctr2;
        int iEmpty = -1;
        int iMatch = -1;

        pLangNode = (LPLANGNODE)ListBox_GetItemData(hwndList, ctr1);

        for (ctr2 = 0; ctr2 < DSWITCH_HOTKEY_SIZE; ctr2++)
        {
            if (!g_aDirectSwitchHotKey[ctr2].hkl)
            {
                if ((iEmpty == -1) &&
                    (g_aDirectSwitchHotKey[ctr2].idxLayout == -1))
                {
                    //
                    //  Remember the first empty spot.
                    //
                    iEmpty = ctr2;
                }
            }
            else if (g_aDirectSwitchHotKey[ctr2].hkl == pLangNode->hkl)
            {
                //
                //  We found a match.  Remember it.
                //
                iMatch = ctr2;
                break;
            }
        }

        if (iMatch == -1)
        {
            if (iEmpty == -1)
            {
                //
                //  We don't have any spots left.
                //
                continue;
            }
            else
            {
                //
                //  New item.
                //
                iMatch = iEmpty;
                if (pLangNode->hkl)
                {
                    g_aDirectSwitchHotKey[iMatch].hkl = pLangNode->hkl;
                }
                else
                {
                    //
                    //  This must be a newly added layout.  We don't have
                    //  the hkl yet.  Remember the index position of this
                    //  layout - we can get the real hkl when the user
                    //  chooses to apply.
                    //
                    g_aDirectSwitchHotKey[iMatch].idxLayout = ctr1;
                }
            }
        }

        if (pLangNode->wStatus & LANG_IME)
        {
            bHasIme = TRUE;
        }

        if (pLangNode->wStatus & LANG_HOTKEY)
        {
            g_aDirectSwitchHotKey[iMatch].uModifiers = pLangNode->uModifiers;
            g_aDirectSwitchHotKey[iMatch].uVKey = pLangNode->uVKey;
        }

        GetAtomName( g_lpLang[pLangNode->iLang].atmLanguageName,
                     szLanguage,
                     DESC_MAX );

        GetAtomName( g_lpLayout[pLangNode->iLayout].atmLayoutText,
                     szLayout,
                     DESC_MAX );

        lstrcat (szLanguage, TEXT(" - "));
        lstrcat (szLanguage, szLayout);

        wsprintf(szItem, szAction, szLanguage);

        g_aDirectSwitchHotKey[iMatch].atmHotKeyName = AddAtom(szItem);
        iIndex = ListBox_InsertString(hwndHotkey, -1, szItem);

        ListBox_SetItemData(hwndHotkey, iIndex, &g_aDirectSwitchHotKey[iMatch]);
    }

    //
    //  Determine IME specific hotkeys for CHS and CHT locales.
    //
    iCount = bHasIme ? Locale_GetImeHotKeyInfo(hwnd,&aImeHotKey) : 0;

    for (ctr1 = 0; ctr1 < iCount; ctr1++)
    {
        UINT iIndex;
        BOOL bRet;

        LoadString( hInstance,
                    aImeHotKey[ctr1].idHotKeyName,
                    szItem,
                    sizeof(szItem) / sizeof(TCHAR) );

        aImeHotKey[ctr1].atmHotKeyName = AddAtom(szItem);

        iIndex = ListBox_InsertString(hwndHotkey, -1,szItem);

        ListBox_SetItemData(hwndHotkey, iIndex, &aImeHotKey[ctr1]);

        //
        //  Get the hot key value.
        //
        bRet = ImmGetHotKey( aImeHotKey[ctr1].dwHotKeyID,
                             &aImeHotKey[ctr1].uModifiers,
                             &aImeHotKey[ctr1].uVKey,
                             NULL );
        if (!bRet)
        {
            aImeHotKey[ctr1].uModifiers = 0;
            if ((aImeHotKey[ctr1].fdwEnable & (MOD_LEFT | MOD_RIGHT)) ==
                (MOD_LEFT | MOD_RIGHT))
            {
                aImeHotKey[ctr1].uModifiers |= MOD_LEFT | MOD_RIGHT;
            }
            aImeHotKey[ctr1].uVKey = 0;
            aImeHotKey[ctr1].hkl = (HKL)NULL;
        }
    }

    ListBox_SetCurSel(hwndHotkey, 0);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_GetAttributes
//
//  Gets the global layout attributes (eg: CapsLock/ShiftLock value) from
//  the registry and then sets the appropriate radio button in the dialog.
//
////////////////////////////////////////////////////////////////////////////

void Locale_GetAttributes(
    HWND hwnd)
{
    DWORD cb;
    HKEY hkey;

    //
    //  Initialize the global.
    //
    g_dwAttributes = 0;           // KLF_SHIFTLOCK = 0x00010000

    //
    //  Get the Atributes value from the registry.
    //
    if (RegOpenKey(HKEY_CURRENT_USER, szKbdLayouts, &hkey) == ERROR_SUCCESS)
    {
        cb = sizeof(DWORD);
        RegQueryValueEx( hkey,
                         szAttributes,
                         NULL,
                         NULL,
                         (LPBYTE)&g_dwAttributes,
                         &cb );
        RegCloseKey(hkey);
    }

    //
    //  Set the radio buttons appropriately.
    //
    CheckDlgButton( hwnd,
                    IDC_KBDL_SHIFTLOCK,
                    (g_dwAttributes & KLF_SHIFTLOCK)
                      ? BST_CHECKED
                      : BST_UNCHECKED );
    CheckDlgButton( hwnd,
                    IDC_KBDL_CAPSLOCK,
                    (g_dwAttributes & KLF_SHIFTLOCK)
                      ? BST_UNCHECKED
                      : BST_CHECKED);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_AddLanguage
//
//  Adds the new input locale to the list in the property page.
//
////////////////////////////////////////////////////////////////////////////

BOOL Locale_AddLanguage(
    HWND hwndMain,
    LPLANGNODE pLangNode)
{
    HWND hwndLang;
    UINT iCount;

    //
    //  See if the user has Admin privileges.  If not, then don't allow
    //  them to install any NEW layouts.
    //
    if ((!g_bAdmin_Privileges) &&
        (!g_lpLayout[pLangNode->iLayout].bInstalled))
    {
        //
        //  The layout is not currently installed, so don't allow it
        //  to be added.
        //
        Locale_ErrorMsg(hwndMain, IDS_KBD_LAYOUT_FAILED, NULL);
        return (FALSE);
    }

    //
    //  Set the language to active.
    //  Also, set the status to changed so that the layout will be added.
    //
    pLangNode->wStatus |= (LANG_CHANGED | LANG_ACTIVE);

    //
    //  Get the number of items in the input locale list box.
    //
    hwndLang = GetDlgItem(hwndMain, IDC_KBDL_LOCALE_LIST);
    iCount = ListBox_GetCount(hwndLang);

    //
    //  Add the new item data to the list box.
    //
    ListBox_AddItemData(hwndLang, pLangNode);

    //
    //  Get the indicator symbol.
    //
    if (!(pLangNode->wStatus & ICON_LOADED))
    {
        Locale_FetchIndicator(pLangNode);
    }

    //
    //  Get hotkey information.
    //
    Locale_GetHotkeys(hwndMain);

    //
    //  See if the original count (before the addition) was 1.  If so,
    //  enable the secondary controls, since there are now 2 items in
    //  the list box.
    //
    if (iCount == 1)
    {
        Locale_SetSecondaryControls(hwndMain, TRUE);
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_ApplyInputs
//
//  1. make sure we have all the layout files required.
//  2. write the information into the registry
//  3. call Load/UnloadKeyboardLayout where relevant
//
//  Note that this will trash the previous preload and substitutes sections,
//  based on what is actually loaded.  Thus if something was wrong before in
//  the registry, it will be corrected now.
//
////////////////////////////////////////////////////////////////////////////

BOOL Locale_ApplyInputs(
    HWND hwnd)
{
    HKL *pLangs = NULL;
    UINT nLangs, nLocales, iVal, idx, ctr, nHotKeys;
    UINT iPreload = 0;
    LPLANGNODE pLangNode, pTemp;
    LPINPUTLANG pInpLang;
    LPHOTKEYINFO pHotKeyNode;
    DWORD dwID;
    TCHAR sz[DESC_MAX];            // temp - build the name of the reg entry
    TCHAR szPreload10[10];
    TCHAR szTemp[MAX_PATH];
    HWND hwndIndicate;
    HKEY hkeyLayouts;
    HKEY hkeySubst;
    HKEY hkeyPreload;
    HKEY hkeyToggle;
    HKEY hKeyImm;
    HWND hwndList = GetDlgItem(hwnd, IDC_KBDL_LOCALE_LIST);
    HWND hwndHotkey = GetDlgItem(hwnd, IDC_KBDL_HOTKEY_LIST);
    HKL hklDefault = 0;
    HKL hklLoad, hklUnload;
    HCURSOR hcurSave;
    HKEY hkeyScanCode;
    DWORD cb;
    TCHAR szShiftL[8];
    TCHAR szShiftR[8];
    BOOL bDirectSwitch = FALSE;
    BOOL bHasIme = FALSE;
    BOOL bReplaced = FALSE;

    //
    //  See if the pane is disabled.  If so, then there is nothing to
    //  Apply.
    //
    if (!IsWindowEnabled(hwndList))
    {
        return (TRUE);
    }

    //
    //  First make sure we are left with a layout.
    //
    //  This actually shouldn't happen, since the "Remove" button is
    //  disabled when there is only one input locale left in the list.
    //
    nLocales = ListBox_GetCount(hwndList);

    if (nLocales < 1)
    {
        Locale_ErrorMsg(hwnd, IDS_KBD_NEED_LAYOUT, NULL);
        return (FALSE);
    }

    //
    //  Put up the hour glass.
    //
    hcurSave = SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    //  Make sure there are actually changes since the last save when
    //  OK is selected.  If the user hits OK without anything to Apply,
    //  then we should do nothing.
    //
    if (g_dwChanges == 0)
    {
        pLangNode = NULL;
        for (idx = 0; idx < g_iLangBuff; idx++)
        {
            pLangNode = g_lpLang[idx].pNext;
            while (pLangNode != NULL)
            {
                if (pLangNode->wStatus & (LANG_CHANGED | LANG_DEF_CHANGE))
                {
                    break;
                }
                pLangNode = pLangNode->pNext;
            }
            if (pLangNode != NULL)
            {
                break;
            }
        }
        if ((idx == g_iLangBuff) && (pLangNode == NULL))
        {
            SetCursor(hcurSave);
            PropSheet_UnChanged(GetParent(hwnd), hwnd);
            return (TRUE);
        }
    }

    //
    //  Clean up the registry.
    //

    //
    //  For FE languages, there is a keyboard which has a different
    //  scan code for shift keys - eg. NEC PC9801.
    //  We have to keep information about scan codes for shift keys in
    //  the registry under the 'toggle' sub key as named values.
    //
    szShiftL[0] = TEXT('\0');
    szShiftR[0] = TEXT('\0');
    if (RegOpenKey( HKEY_CURRENT_USER,
                    szScanCodeKey,
                    &hkeyScanCode ) == ERROR_SUCCESS)
    {
        cb = sizeof(szShiftL);
        RegQueryValueEx( hkeyScanCode,
                         szValueShiftLeft,
                         NULL,
                         NULL,
                         (LPBYTE)szShiftL,
                         &cb );

        cb = sizeof(szShiftR);
        RegQueryValueEx( hkeyScanCode,
                         szValueShiftRight,
                         NULL,
                         NULL,
                         (LPBYTE)szShiftR,
                         &cb );

        RegCloseKey(hkeyScanCode);
    }

    //
    //  Delete the HKCU\Keyboard Layout key and all subkeys.
    //
    if (RegOpenKeyEx( HKEY_CURRENT_USER,
                      szKbdLayouts,
                      0,
                      KEY_ALL_ACCESS,
                      &hkeyLayouts ) == ERROR_SUCCESS)
    {
        //
        //  Delete the HKCU\Keyboard Layout\Preload, Substitutes, and Toggle
        //  keys in the registry so that the Keyboard Layout section can be
        //  rebuilt.
        //
        RegDeleteKey(hkeyLayouts, szPreloadKey);
        RegDeleteKey(hkeyLayouts, szSubstKey);
        RegDeleteKey(hkeyLayouts, szToggleKey);

        RegCloseKey(hkeyLayouts);

        RegDeleteKey(HKEY_CURRENT_USER, szKbdLayouts);
    }

    //
    //  Create the HKCU\Keyboard Layout key.
    //
    if (RegCreateKey( HKEY_CURRENT_USER,
                      szKbdLayouts,
                      &hkeyLayouts ) == ERROR_SUCCESS)
    {
        //
        //  Create the HKCU\Keyboard Layout\Substitutes key.
        //
        if (RegCreateKey( hkeyLayouts,
                          szSubstKey,
                          &hkeySubst ) == ERROR_SUCCESS)
        {
            //
            //  Create the HKCU\Keyboard Layout\Preload key.
            //
            if (RegCreateKey( hkeyLayouts,
                              szPreloadKey,
                              &hkeyPreload ) == ERROR_SUCCESS)
            {
                //
                //  Initialize the iPreload variable to 1 to show
                //  that the key has been created.
                //
                iPreload = 1;

                //
                //  Create the HKCU\Keyboard Layout\Toggle key.
                //
                RegCreateKey(hkeyLayouts, szToggleKey, &hkeyToggle);
            }
            else
            {
                RegCloseKey(hkeySubst);
            }
        }

        RegSetValueEx( hkeyLayouts,
                       szAttributes,
                       0,
                       REG_DWORD,
                       (LPBYTE)&g_dwAttributes,
                       sizeof(DWORD) );

        RegCloseKey(hkeyLayouts);
    }
    if (!iPreload)
    {
        //
        //  Registry keys could not be created.  Now what?
        //
        MessageBeep(MB_OK);
        SetCursor(hcurSave);
        return (FALSE);
    }

    //
    //  Set all usage counts to zero in the language array.
    //
    for (idx = 0; idx < g_iLangBuff; idx++)
    {
        g_lpLang[idx].iUseCount = 0;
    }

    //
    //  Search through the list to see if any keyboard layouts need to be
    //  unloaded from the system.
    //
    for (idx = 0; idx < g_iLangBuff; idx++)
    {
        pLangNode = g_lpLang[idx].pNext;
        while (pLangNode != NULL)
        {
            if ( (pLangNode->wStatus & LANG_ORIGACTIVE) &&
                 !(pLangNode->wStatus & LANG_ACTIVE) )
            {
                //
                //  Before unloading the hkl, look for the corresponding
                //  hotkey and remove it.
                //
                DWORD dwHotKeyID = 0;

                for (ctr = 0; ctr < DSWITCH_HOTKEY_SIZE; ctr++)
                {
                    if (g_aDirectSwitchHotKey[ctr].hkl == pLangNode->hkl)
                    {
                        //
                        //  Found an hkl match.  Remember the hotkey ID so
                        //  we can delete the hotkey entry later if the
                        //  unload of the hkl succeeds.
                        //
                        dwHotKeyID = g_aDirectSwitchHotKey[ctr].dwHotKeyID;
                        break;
                    }
                }

                //
                //  Started off with this active, deleting it now.
                //  Failure is not fatal.
                //
                if (!UnloadKeyboardLayout(pLangNode->hkl))
                {
                    Locale_ApplyError( hwnd,
                                       pLangNode,
                                       IDS_KBD_UNLOAD_KBD_FAILED,
                                       MB_OK_OOPS );

                    //
                    //  Failed to unload layout, put it back in the list,
                    //  and turn ON the indicator whether it needs it or not.
                    //
                    if (Locale_AddLanguage(hwnd, pLangNode))
                    {
                        CheckDlgButton(hwnd, IDC_KBDL_INDICATOR, TRUE);
                    }

                    pLangNode = pLangNode->pNext;
                }
                else
                {
                    //
                    //  Succeeded, no longer in USER's list.
                    //
                    //  Reset flag, this could be from ApplyInput and we'll
                    //  fail on the OK if we leave it marked as original
                    //  active.
                    //
                    pLangNode->wStatus &= ~(LANG_ORIGACTIVE | LANG_CHANGED);

                    //
                    //  Remove the hotkey entry for this hkl.
                    //
                    if (dwHotKeyID)
                    {
                        ImmSetHotKey(dwHotKeyID, 0, 0, (HKL)NULL);
                    }

                    //
                    //  Remove the link in the language array.
                    //
                    //  NOTE: pLangNode could be null here.
                    //
                    pTemp = pLangNode->pNext;
                    Locale_RemoveFromLinkedList(pLangNode);
                    pLangNode = pTemp;
                }
            }
            else
            {
                pLangNode = pLangNode->pNext;
            }
        }
    }

    //
    //  The order in the registry is based on the order in which they
    //  appear in the list box.
    //
    //  The only exception to this is that the default will be number 1.
    //
    //  If no default is found, the last one in the list will be used as
    //  the default.
    //
    iVal = 2;
    for (ctr = 0; ctr < nLocales; ctr++)
    {
        //
        //  Get the pointer to the lang node from the list box
        //  item data.
        //
        pLangNode = (LPLANGNODE)ListBox_GetItemData(hwndList, ctr);
        pInpLang = &(g_lpLang[pLangNode->iLang]);

        //
        //  Clear the "set hot key" field, since we will be writing to the
        //  registry.
        //
        pLangNode->wStatus &= ~LANG_HOTKEY;

        //
        //  See if it's the default input locale.
        //
        if (pLangNode->wStatus & LANG_DEFAULT)
        {
            //
            //  Default input locale, so the preload value should be
            //  set to 1.
            //
            iPreload = 1;
        }
        else if (ctr == (nLocales - 1))
        {
            //
            //  We're on the last one.  Make sure there was a default.
            //
            iPreload = (iVal <= nLocales) ? iVal : 1;
        }
        else
        {
            //
            //  Set the preload value to the next value.
            //
            iPreload = iVal;
            iVal++;
        }

        //
        //  Store the preload value as a string so that it can be written
        //  into the registry (as a value name).
        //
        wsprintf(sz, TEXT("%d"), iPreload);

        //
        //  Store the locale id as a string so that it can be written
        //  into the registry (as a value).
        //
        if ((HIWORD(g_lpLayout[pLangNode->iLayout].dwID) & 0xf000) == 0xe000)
        {
            pLangNode->wStatus |= LANG_IME;
            wsprintf( szPreload10,
                      TEXT("%8.8lx"),
                      g_lpLayout[pLangNode->iLayout].dwID );
            bHasIme = TRUE;
        }
        else
        {
            pLangNode->wStatus &= ~LANG_IME;
            dwID = pInpLang->dwID;
            idx = pInpLang->iUseCount;
            if ((idx == 0) || (idx > 0xfff))
            {
                idx = 0;
            }
            else
            {
                dwID |= ((DWORD)(0xd000 | ((WORD)(idx - 1))) << 16);
            }
            wsprintf(szPreload10, TEXT("%08.8x"), dwID);
            (pInpLang->iUseCount)++;
        }

        //
        //  Set the new entry in the registry.  It is of the form:
        //
        //  HKCU\Keyboard Layout
        //      Preload:    1 = <locale id>
        //                  2 = <locale id>
        //                      etc...
        //

        RegSetValueEx( hkeyPreload,
                       sz,
                       0,
                       REG_SZ,
                       (LPBYTE)szPreload10,
                       (DWORD)(lstrlen(szPreload10) + 1) * sizeof(TCHAR) );

        //
        //  See if we need to add a substitute for this input locale.
        //
        if (((pInpLang->dwID != g_lpLayout[pLangNode->iLayout].dwID) || idx) &&
            (!(pLangNode->wStatus & LANG_IME)))
        {
            //
            //  Get the layout id as a string so that it can be written
            //  into the registry (as a value).
            //
            wsprintf( szTemp,
                      TEXT("%8.8lx"),
                      g_lpLayout[pLangNode->iLayout].dwID );

            //
            //  Set the new entry in the registry.  It is of the form:
            //
            //  HKCU\Keyboard Layout
            //      Substitutes:    <locale id> = <layout id>
            //                      <locale id> = <layout id>
            //                          etc...
            //
            RegSetValueEx( hkeySubst,
                           szPreload10,
                           0,
                           REG_SZ,
                           (LPBYTE)szTemp,
                           (DWORD)(lstrlen(szTemp) + 1) * sizeof(TCHAR) );
        }

        //
        //  Make sure all of the changes are written to disk.
        //
        RegFlushKey(hkeySubst);
        RegFlushKey(hkeyPreload);
        RegFlushKey(HKEY_CURRENT_USER);

        //
        //  See if the keyboard layout needs to be loaded.
        //
        if (pLangNode->wStatus & (LANG_CHANGED | LANG_DEF_CHANGE))
        {
            //
            //  Load the keyboard layout into the system.
            //
            if (pLangNode->hklUnload)
            {
                hklLoad = LoadKeyboardLayoutEx( pLangNode->hklUnload,
                                                szPreload10,
                                                KLF_SUBSTITUTE_OK |
                                                  KLF_NOTELLSHELL |
                                                  g_dwAttributes );
                if (hklLoad != pLangNode->hklUnload) {
                    bReplaced = TRUE;
                }
            }
            else
            {
                hklLoad = LoadKeyboardLayout( szPreload10,
                                              KLF_SUBSTITUTE_OK |
                                                KLF_NOTELLSHELL |
                                                g_dwAttributes );
            }
            if (hklLoad)
            {
                pLangNode->wStatus &= ~(LANG_CHANGED | LANG_DEF_CHANGE);
                pLangNode->wStatus |= (LANG_ACTIVE | LANG_ORIGACTIVE);

                if (pLangNode->wStatus & LANG_DEFAULT)
                {
                    hklDefault = hklLoad;
                }

                pLangNode->hkl = hklLoad;
                pLangNode->hklUnload = hklLoad;
            }
            else
            {
                Locale_ApplyError( hwnd,
                                   pLangNode,
                                   IDS_KBD_LOAD_KBD_FAILED,
                                   MB_OK_OOPS );
            }
        }
        else if (g_dwChanges & CHANGE_CAPSLOCK)
        {
            ActivateKeyboardLayout(GetKeyboardLayout(0), KLF_RESET | g_dwAttributes);
        }
    }

    //
    //  Close the handles to the registry keys.
    //
    RegCloseKey(hkeySubst);
    RegCloseKey(hkeyPreload);

    //
    //  Make sure the default is set properly.  The layout id for the
    //  current default input locale may have been changed.
    //
    //  NOTE: This should be done before the Unloads occur in case one
    //        of the layouts to unload is the old default layout.
    //
    if (hklDefault != 0)
    {
        if (!SystemParametersInfo( SPI_SETDEFAULTINPUTLANG,
                                   0,
                                   (LPVOID)((LPDWORD)&hklDefault),
                                   0 ))
        {
            //
            //  Failure is not fatal.  The old default language will
            //  still work.
            //
            Locale_ErrorMsg(hwnd, IDS_KBD_NO_DEF_LANG2, NULL);
        }
        else
        {
            //
            //  Activate the new default keyboard layout.
            //

            //
            //  This may be a good idea (the broadcast will do it though).
            //
        //  ActivateKeyboardLayout(hklDefault, KLF_REORDER);

            //
            //  Try to make everything switch to the new default input locale:
            //  if we are in setup  OR
            //  if it is the only one (but not if we just replaced the layout
            //    within the Input Locale without changing the input locale)
            //
            if (g_bSetupCase || ((nLocales == 1) && !bReplaced))
            {
                DWORD dwRecipients = BSM_APPLICATIONS | BSM_ALLDESKTOPS;
                BroadcastSystemMessage( BSF_POSTMESSAGE,
                                        &dwRecipients,
                                        WM_INPUTLANGCHANGEREQUEST,
                                        1,  // IS compatible with system font
                                        (LPARAM)hklDefault );
            }
        }
    }

    //
    //  Handle the task bar indicator option.
    //
    hwndIndicate = FindWindow(szIndicator, NULL);

    if (RegCreateKey( HKEY_CURRENT_USER,
                      REGSTR_PATH_RUN,
                      &hkeySubst ) != ERROR_SUCCESS)
    {
        Locale_ErrorMsg(hwnd, IDS_KBD_LOAD_LINE_BAD, NULL);
        hkeySubst = NULL;
    }

    //
    //  See if the task bar indicator check box is set.
    //
    if (IsDlgButtonChecked(hwnd, IDC_KBDL_INDICATOR))
    {
        //
        //  User wants the indicator.
        //
        //  See if the indicator is already enabled.
        //
        if (hwndIndicate && IsWindow(hwndIndicate))
        {
            SendMessage(hwndIndicate, WM_COMMAND, IDM_NEWSHELL, 0L);
        }
        else
        {
            WinExec(szInternatA, SW_SHOWMINNOACTIVE);
        }

        if (hkeySubst)
        {
            RegSetValueEx( hkeySubst,
                           szInternat,
                           0,
                           REG_SZ,
                           (LPBYTE)szInternat,
                           sizeof(szInternat) );
        }
    }
    else
    {
        //
        //  Either the user doesn't want the indicator or there are less
        //  than two input locales.
        //
        if (hwndIndicate && IsWindow(hwndIndicate))
        {
            //
            //  It's on, turn it off again.
            //
            SendMessage(hwndIndicate, WM_COMMAND, IDM_EXIT, 0L);
        }
        if (hkeySubst)
        {
            //
            //  Clean up the registry.
            //
            RegDeleteValue(hkeySubst, szInternat);
        }
    }
    if (hkeySubst)
    {
        RegCloseKey(hkeySubst);
    }

    //
    //  The first item in the hotkey list should always
    //  be the "switch between locales" option, so get its setting.
    //
    pHotKeyNode = (LPHOTKEYINFO)ListBox_GetItemData(hwndHotkey, 0);

    idx = 3;
    if (pHotKeyNode->uModifiers & MOD_ALT)
    {
        idx = 1;
    }
    else if (pHotKeyNode->uModifiers & MOD_CONTROL)
    {
        idx = 2;
    }
    else if (pHotKeyNode->uVKey == CHAR_GRAVE)
    {
        idx = 4;
    }

    //
    //  Get the toggle hotkey as a string so that it can be written
    //  into the registry (as data).
    //
    wsprintf(szTemp, TEXT("%d"), idx);

    //
    //  Set the new entry in the registry.  It is of the form:
    //
    //  HKCU\Keyboard Layout
    //      Toggle:    Hotkey = <hotkey number>
    //
    if (hkeyToggle)
    {
        RegSetValueEx( hkeyToggle,
                       TEXT("Hotkey"),
                       0,
                       REG_SZ,
                       (LPBYTE)szTemp,
                       (DWORD)(lstrlen(szTemp) + 1) * sizeof(TCHAR) );
        RegCloseKey(hkeyToggle);
    }

    //
    //  Since we updated the registry, we should reread this next time.
    //
    g_bGetSwitchLangHotKey = TRUE;

    //
    //  Set the scan code entries in the registry.
    //
    if (RegCreateKey( HKEY_CURRENT_USER,
                      szScanCodeKey,
                      &hkeyScanCode ) == ERROR_SUCCESS)
    {
        if (szShiftL[0])
        {
            RegSetValueEx( hkeyScanCode,
                           szValueShiftLeft,
                           0,
                           REG_SZ,
                           (LPBYTE)szShiftL,
                           (DWORD)(lstrlen(szShiftL) + 1) * sizeof(TCHAR) );
        }

        if (szShiftR[0])
        {
            RegSetValueEx( hkeyScanCode,
                           szValueShiftRight,
                           0,
                           REG_SZ,
                           (LPBYTE)szShiftR,
                           (DWORD)(lstrlen(szShiftR) + 1) * sizeof(TCHAR) );
        }
        RegCloseKey(hkeyScanCode);
    }

    //
    //  Call SystemParametersInfo to enable the toggle.
    //
    SystemParametersInfo(SPI_SETLANGTOGGLE, 0, NULL, 0);

    //
    //  Set Imm hotkeys.
    //
    //  Get the list of the currently active keyboard layouts from
    //  the system.  We will possibly need to sync up all IMEs with new
    //  hotkeys.
    //
    nLangs = GetKeyboardLayoutList(0, NULL);
    if (nLangs != 0)
    {
        pLangs = (HKL *)LocalAlloc(LPTR, sizeof(DWORD) * nLangs);
        GetKeyboardLayoutList(nLangs, (HKL *)pLangs);
    }

    nHotKeys = ListBox_GetCount(hwndHotkey);
    for (ctr = 1; ctr < nHotKeys; ctr++)
    {
        BOOL bRet;
        UINT uModifiers;
        UINT uVKey;
        HKL  hkl;

        //
        //  Get Hotkey information for each item in the hotkey list.
        //
        pHotKeyNode = (LPHOTKEYINFO)ListBox_GetItemData(hwndHotkey, ctr);

        bRet = ImmGetHotKey(pHotKeyNode->dwHotKeyID, &uModifiers, &uVKey, &hkl);

        if (!bRet &&
            (!pHotKeyNode->uVKey) &&
            ((pHotKeyNode->uModifiers & (MOD_ALT | MOD_CONTROL | MOD_SHIFT))
             != (MOD_ALT | MOD_CONTROL | MOD_SHIFT)))
        {
            //
            //  No such hotkey exists.  User does not specify key and modifier
            //  information either. We can skip this one.
            //
            continue;
        }

        if ((pHotKeyNode->uModifiers == uModifiers) &&
            (pHotKeyNode->uVKey == uVKey))
        {
            //
            //  No change.
            //
            if (IS_DIRECT_SWITCH_HOTKEY(pHotKeyNode->dwHotKeyID))
            {
                bDirectSwitch = TRUE;
            }
            continue;
        }

        if (pHotKeyNode->idxLayout != -1)
        {
            //
            //  We had this layout index remembered because at that time
            //  we did not have a real hkl to work with.  Now it is
            //  time to get the real hkl.
            //
            pLangNode = (LPLANGNODE)ListBox_GetItemData( hwndList,
                                                         pHotKeyNode->idxLayout );
            pHotKeyNode->hkl = pLangNode->hkl;
        }

        if (!bRet && IS_DIRECT_SWITCH_HOTKEY(pHotKeyNode->dwHotKeyID))
        {
            //
            //  New direct switch hotkey ID.  We need to see if the same
            //  hkl is set at another ID.  If so, set the other ID instead
            //  of the one requested.
            //
            DWORD dwHotKeyID;

            //
            //  Loop through all direct switch hotkeys.
            //
            for (dwHotKeyID = IME_HOTKEY_DSWITCH_FIRST;
                 (dwHotKeyID <= IME_HOTKEY_DSWITCH_LAST);
                 dwHotKeyID++)
            {
                if (dwHotKeyID == pHotKeyNode->dwHotKeyID)
                {
                    //
                    //  Skip itself.
                    //
                    continue;
                }

                bRet = ImmGetHotKey(dwHotKeyID, &uModifiers, &uVKey, &hkl);
                if (!bRet)
                {
                    //
                    //  Did not find the hotkey id. Skip.
                    //
                    continue;
                }

                if (hkl == pHotKeyNode->hkl)
                {
                    //
                    //  We found the same hkl already with hotkey
                    //  settings at another ID.  Set hotkey
                    //  ID equal to the one with the same hkl. So later
                    //  we will modify hotkey for the correct hkl.
                    //
                    pHotKeyNode->dwHotKeyID = dwHotKeyID;
                    break;
                }
            }
        }

        //
        //  Set the hotkey value.
        //
        bRet = ImmSetHotKey( pHotKeyNode->dwHotKeyID,
                             pHotKeyNode->uModifiers,
                             pHotKeyNode->uVKey,
                             pHotKeyNode->hkl );

        if (bRet)
        {
            //
            //  Hotkey set successfully. See if user used any direct
            //  switch hot key. We may have to load imm later.
            //
            if (IS_DIRECT_SWITCH_HOTKEY(pHotKeyNode->dwHotKeyID))
            {
                if (pHotKeyNode->uVKey != 0)
                {
                    bDirectSwitch = TRUE;
                }
            }
            else
            {
                //
                //  Must be IME related hotkey.  We need to sync up the
                //  imes so that the new hotkey is effective to all
                //  of them.
                //
                UINT ctr2;

                for (ctr2 = 0; ctr2 < nLangs; ctr2++)
                {
                    if (!ImmIsIME(pLangs[ctr2]))
                    {
                        continue;
                    }

                    ImmEscape( pLangs[ctr],
                               (HIMC)NULL,
                               IME_ESC_SYNC_HOTKEY,
                               &pHotKeyNode->dwHotKeyID );
                }
            }
        }
        else
        {
            //
            //  Failed to set hotkey.  Maybe a duplicate.  Warn user.
            //
            TCHAR szString[DESC_MAX];

            GetAtomName( pHotKeyNode->atmHotKeyName,
                         szString,
                         sizeof(szString) / sizeof(TCHAR) );
            Locale_ErrorMsg(hwnd, IDS_KBD_SET_HOTKEY_ERR, szString);
        }
    }

    //
    //  Turn off the hour glass.
    //
    SetCursor(hcurSave);

    //
    //  Free any allocated memory.
    //
    if (pLangs != NULL)
    {
        LocalFree((HANDLE)pLangs);
    }

    if (bDirectSwitch || bHasIme)
    {
        if (RegOpenKey( HKEY_LOCAL_MACHINE,
                        szLoadImmPath,
                        &hKeyImm ) == ERROR_SUCCESS)
        {
            DWORD dwValue = 1;

            RegSetValueEx( hKeyImm,
                           TEXT("LoadIMM"),
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwValue,
                           sizeof(DWORD) );

            RegCloseKey(hKeyImm);

            if (!g_bSetupCase &&
                !GetSystemMetrics(SM_IMMENABLED) &&
                !GetSystemMetrics(SM_DBCSENABLED))
            {
                //
                //  Imm was not loaded.  Ask user to reboot and let
                //  it be loaded.
                //
                TCHAR szReboot[DESC_MAX];
                TCHAR szTitle[DESC_MAX];

                LoadString(hInstance, IDS_REBOOT_STRING, szReboot, DESC_MAX);
                LoadString(hInstance, IDS_TITLE_STRING, szTitle, DESC_MAX);
                if (MessageBox( hwnd,
                                szReboot,
                                szTitle,
                                MB_YESNO | MB_ICONQUESTION ) == IDYES)
                {
                    Region_RebootTheSystem();
                }
            }
        }
    }

    //
    //  Return success.
    //
    g_dwChanges = 0;
    PropSheet_UnChanged(GetParent(hwnd), hwnd);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_EnablePane
//
//  The controls in "iControl" are the controls that get disabled if the
//  pane can't come up.
//
////////////////////////////////////////////////////////////////////////////

static UINT iLocaleControls[] =
{
    IDC_KBDL_LOCALE,         IDC_KBDL_LAYOUT_TEXT,   IDC_KBDL_LOCALE_LIST,
    IDC_KBDL_ADD,            IDC_KBDL_EDIT,          IDC_KBDL_DELETE,
    IDC_KBDL_INPUT_FRAME,    IDC_KBDL_SET_DEFAULT,
    IDC_KBDL_CAPSLOCK_FRAME, IDC_KBDL_CAPSLOCK,      IDC_KBDL_SHIFTLOCK,
    IDC_KBDL_HOTKEY_FRAME,   IDC_KBDL_IME_SETTINGS,  IDC_KBDL_HOTKEY_LIST,
    IDC_KBDL_CHANGE_HOTKEY,  IDC_KBDL_INDICATOR,
    IDC_KBDL_ONSCRNKBD,      IDC_KBDL_DISABLED,      IDC_KBDL_DISABLED_2,
    IDC_KBDL_HOTKEY,        IDC_KBDL_HOTKEY_SEQUENCE
};
#define NCONTROLS sizeof(iLocaleControls) / sizeof(UINT)


void Locale_EnablePane(
    HWND hwnd,
    BOOL bEnable,
    UINT DisableId)
{
    HWND hwndItem;
    int ctr;

    if (bEnable)
    {
        //
        //  Enable all of the controls except for the "pane disabled"
        //  strings.
        //
        for (ctr = 0; ctr < NCONTROLS; ctr++)
        {
            hwndItem = GetDlgItem(hwnd, iLocaleControls[ctr]);
            ShowWindow(hwndItem, SW_SHOW);
            EnableWindow(hwndItem, TRUE);
        }

        //
        //  Disable the "pane disabled" strings.
        //
        EnableWindow(GetDlgItem(hwnd, IDC_KBDL_DISABLED), FALSE);
        ShowWindow(GetDlgItem(hwnd, IDC_KBDL_DISABLED), SW_HIDE);
        EnableWindow(GetDlgItem(hwnd, IDC_KBDL_DISABLED_2), FALSE);
        ShowWindow(GetDlgItem(hwnd, IDC_KBDL_DISABLED_2), SW_HIDE);
    }
    else
    {
        //
        //  Disable all of the controls except for the "pane disabled"
        //  string.
        //
        for (ctr = 0; ctr < NCONTROLS; ctr++)
        {
            hwndItem = GetDlgItem(hwnd, iLocaleControls[ctr]);
            EnableWindow(hwndItem, FALSE);
            ShowWindow(hwndItem, SW_HIDE);
        }

        hwndItem = GetDlgItem(hwnd, DisableId);
        ShowWindow(hwndItem, SW_SHOW);
        EnableWindow(hwndItem, TRUE);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_FileExists
//
//  Determines if the file exists and is accessible.
//
////////////////////////////////////////////////////////////////////////////

BOOL Locale_FileExists(
    LPTSTR pFileName)
{
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle;
    BOOL bRet;
    UINT OldMode;

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    FindHandle = FindFirstFile(pFileName, &FindData);
    if (FindHandle == INVALID_HANDLE_VALUE)
    {
        bRet = FALSE;
    }
    else
    {
        FindClose(FindHandle);
        bRet = TRUE;
    }

    SetErrorMode(OldMode);

    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_LoadLayouts
//
//  Loads the layouts from the registry.
//
////////////////////////////////////////////////////////////////////////////

BOOL Locale_LoadLayouts(
    HWND hwnd)
{
    HKEY hKey;
    HKEY hkey1;
    DWORD cb;
    DWORD dwIndex;
    LONG dwRetVal;
    DWORD dwValue;
    DWORD dwType;
    TCHAR szValue[MAX_PATH];           // language id (number)
    TCHAR szData[MAX_PATH];            // language name
    TCHAR szSystemDir[MAX_PATH * 2];
    UINT SysDirLen;

    //
    //  Now read all of the layouts from the registry.
    //
    if (RegOpenKey(HKEY_LOCAL_MACHINE, szLayoutPath, &hKey) != ERROR_SUCCESS)
    {
        Locale_EnablePane(hwnd, FALSE, IDC_KBDL_DISABLED);
        return (FALSE);
    }

    dwIndex = 0;
    dwRetVal = RegEnumKey( hKey,
                           dwIndex,
                           szValue,
                           sizeof(szValue) / sizeof(TCHAR) );

    if (dwRetVal != ERROR_SUCCESS)
    {
        Locale_EnablePane(hwnd, FALSE, IDC_KBDL_DISABLED);
        RegCloseKey(hKey);
        return (FALSE);
    }

    g_hLayout = GlobalAlloc(GHND, ALLOCBLOCK * sizeof(LAYOUT));
    g_nLayoutBuffSize = ALLOCBLOCK;
    g_iLayoutBuff = 0;
    g_iLayoutIME = 0;                    // number of IME layouts.
    g_lpLayout = GlobalLock(g_hLayout);

    if (!g_hLayout)
    {
        Locale_EnablePane(hwnd, FALSE, IDC_KBDL_DISABLED);
        RegCloseKey(hKey);
        return (FALSE);
    }

    //
    //  Save the system directory string.
    //
    szSystemDir[0] = 0;
    if (SysDirLen = GetSystemDirectory(szSystemDir, MAX_PATH))
    {
        if (SysDirLen > MAX_PATH)
        {
            SysDirLen = 0;
            szSystemDir[0] = 0;
        }
        else if (szSystemDir[SysDirLen - 1] != TEXT('\\'))
        {
            szSystemDir[SysDirLen] = TEXT('\\');
            szSystemDir[SysDirLen + 1] = 0;
            SysDirLen++;
        }
    }

    do
    {
        //
        //  New layout - get the layout id, the layout file name, and
        //  the layout description string.
        //
        if (g_iLayoutBuff + 1 == g_nLayoutBuffSize)
        {
            HANDLE hTemp;

            GlobalUnlock(g_hLayout);

            g_nLayoutBuffSize += ALLOCBLOCK;
            hTemp = GlobalReAlloc( g_hLayout,
                                   g_nLayoutBuffSize * sizeof(LAYOUT),
                                   GHND );
            if (hTemp == NULL)
            {
                break;
            }

            g_hLayout = hTemp;
            g_lpLayout = GlobalLock(g_hLayout);
        }

        //
        //  Save the layout id.
        //
        g_lpLayout[g_iLayoutBuff].dwID = TransNum(szValue);

        lstrcpy(szData, szLayoutPath);
        lstrcat(szData, TEXT("\\"));
        lstrcat(szData, szValue);

        if (RegOpenKey(HKEY_LOCAL_MACHINE, szData, &hkey1) == ERROR_SUCCESS)
        {
            //
            //  Get the name of the layout file.
            //
            szValue[0] = TEXT('\0');
            cb = sizeof(szValue);
            if ((RegQueryValueEx( hkey1,
                                  szLayoutFile,
                                  NULL,
                                  NULL,
                                  (LPBYTE)szValue,
                                  &cb ) == ERROR_SUCCESS) &&
                (cb > sizeof(TCHAR)))
            {
                g_lpLayout[g_iLayoutBuff].atmLayoutFile = AddAtom(szValue);

                //
                //  See if the layout file exists already.
                //
                lstrcpy(szSystemDir + SysDirLen, szValue);
                g_lpLayout[g_iLayoutBuff].bInstalled = (Locale_FileExists(szSystemDir));

                //
                //  Get the name of the layout.
                //
                szValue[0] = TEXT('\0');
                cb = sizeof(szValue);
                g_lpLayout[g_iLayoutBuff].iSpecialID = 0;
                if (RegQueryValueEx( hkey1,
                                     szLayoutText,
                                     NULL,
                                     NULL,
                                     (LPBYTE)szValue,
                                     &cb ) == ERROR_SUCCESS)
                {
                    g_lpLayout[g_iLayoutBuff].atmLayoutText = AddAtom(szValue);

                    //
                    //  See if it's an IME or a special id.
                    //
                    szValue[0] = TEXT('\0');
                    cb = sizeof(szValue);
                    if ((HIWORD(g_lpLayout[g_iLayoutBuff].dwID) & 0xf000) == 0xe000)
                    {
                        //
                        //  Get the name of the IME file.
                        //
                        if (RegQueryValueEx( hkey1,
                                             szIMEFile,
                                             NULL,
                                             NULL,
                                             (LPBYTE)szValue,
                                             &cb ) == ERROR_SUCCESS)
                        {
                            g_lpLayout[g_iLayoutBuff].atmIMEFile = AddAtom(szValue);
                            szValue[0] = TEXT('\0');
                            cb = sizeof(szValue);
                            g_iLayoutBuff++;
                            g_iLayoutIME++;   // increment number of IME layouts.
                        }
                    }
                    else
                    {
                        //
                        //  See if this is a special id.
                        //
                        if (RegQueryValueEx( hkey1,
                                             szLayoutID,
                                             NULL,
                                             NULL,
                                             (LPBYTE)szValue,
                                             &cb ) == ERROR_SUCCESS)
                        {
                            //
                            //  This may not exist.
                            //
                            g_lpLayout[g_iLayoutBuff].iSpecialID =
                                (UINT)TransNum(szValue);
                        }
                        g_iLayoutBuff++;
                    }
                }
            }

            RegCloseKey(hkey1);
        }

        dwIndex++;
        szValue[0] = TEXT('\0');
        dwRetVal = RegEnumKey( hKey,
                               dwIndex,
                               szValue,
                               sizeof(szValue) / sizeof(TCHAR) );

    } while (dwRetVal == ERROR_SUCCESS);

    cb = sizeof(DWORD);
    g_dwAttributes = 0;
    if (RegQueryValueEx( hKey,
                         szAttributes,
                         NULL,
                         NULL,
                         (LPBYTE)&g_dwAttributes,
                         &cb ) != ERROR_SUCCESS)
    {
        g_dwAttributes &= 0x00FF0000;
    }

    RegCloseKey(hKey);

    return (g_iLayoutBuff);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_LoadLocales
//
//  Loads the locales from the registry.
//
////////////////////////////////////////////////////////////////////////////

BOOL Locale_LoadLocales(
    HWND hwnd)
{
    HKEY hKey;
    DWORD cchValue, cbData;
    DWORD dwIndex;
    DWORD dwLocale, dwLayout;
    DWORD dwLocale2, dwLayout2;
    LONG dwRetVal;
    UINT ctr1, ctr2 = 0;
    HKL hklSystem;
    TCHAR szValue[MAX_PATH];           // language id (number)
    TCHAR szData[MAX_PATH];            // language name
    HINF hIntlInf;
    BOOL bRet;

    //
    //  Get the keyboard layout handle for the system default input locale.
    //
    if (!SystemParametersInfo(SPI_GETDEFAULTINPUTLANG, 0, (PVOID)&hklSystem, 0))
    {
        Locale_EnablePane(hwnd, FALSE, IDC_KBDL_DISABLED);
        return (FALSE);
    }

    if (!(g_hLang = GlobalAlloc(GHND, ALLOCBLOCK * sizeof(INPUTLANG))))
    {
        Locale_EnablePane(hwnd, FALSE, IDC_KBDL_DISABLED);
        return (FALSE);
    }

    g_nLangBuffSize = ALLOCBLOCK;
    g_iLangBuff = 0;
    g_lpLang = GlobalLock(g_hLang);

    //
    //  Now read all of the locales from the registry.
    //
    if (RegOpenKey(HKEY_LOCAL_MACHINE, szLocaleInfo, &hKey) != ERROR_SUCCESS)
    {
        Locale_EnablePane(hwnd, FALSE, IDC_KBDL_DISABLED);
        return (FALSE);
    }

    dwIndex = 0;
    cchValue = sizeof(szValue) / sizeof(TCHAR);
    cbData = sizeof(szData);
    dwRetVal = RegEnumValue( hKey,
                             dwIndex,
                             szValue,
                             &cchValue,
                             NULL,
                             NULL,
                             (LPBYTE)szData,
                             &cbData );


    if (dwRetVal != ERROR_SUCCESS)
    {
        Locale_EnablePane(hwnd, FALSE, IDC_KBDL_DISABLED);
        RegCloseKey(hKey);
        return (FALSE);
    }

    //
    //  Open the INF file.
    //
    bRet = Region_OpenIntlInfFile(&hIntlInf);

    do
    {
        //
        //  Check for cchValue > 1 - an empty string will be enumerated,
        //  and will come back with cchValue == 1 for the null terminator.
        //  Also, check for cbData > 2 - an empty string will be 2, since
        //  this is the count of bytes.
        //
        if ((cchValue > 1) && (cchValue < HKL_LEN) && (cbData > 2))
        {
            //
            //  New language - get the language name and the language id.
            //
            if ((g_iLangBuff + 1) == g_nLangBuffSize)
            {
                HANDLE hTemp;

                GlobalUnlock(g_hLang);

                g_nLangBuffSize += ALLOCBLOCK;
                hTemp = GlobalReAlloc( g_hLang,
                                       g_nLangBuffSize * sizeof(INPUTLANG),
                                       GHND );
                if (hTemp == NULL)
                {
                    break;
                }

                g_hLang = hTemp;
                g_lpLang = GlobalLock(g_hLang);
            }

            g_lpLang[g_iLangBuff].dwID = TransNum(szValue);
            g_lpLang[g_iLangBuff].iUseCount = 0;
            g_lpLang[g_iLangBuff].iNumCount = 0;
            g_lpLang[g_iLangBuff].pNext = NULL;

            //
            //  Get the default keyboard layout for the language.
            //
            if (bRet && Region_ReadDefaultLayoutFromInf( szValue,
                                                         &dwLocale,
                                                         &dwLayout,
                                                         &dwLocale2,
                                                         &dwLayout2,
                                                         hIntlInf ) == TRUE)
            {
                //
                // The default layout is either the first layout in the inf file line
                // or it's the first layout in the line that has the same language
                // is the locale.
                g_lpLang[g_iLangBuff].dwDefaultLayout = dwLayout2?dwLayout2:dwLayout;
            }

            //
            //  Get the full localized name of the language.
            //
            if (GetLocaleInfo( LOWORD(g_lpLang[g_iLangBuff].dwID),
                               LOCALE_SLANGUAGE,
                               szData,
                               MAX_PATH ))
            {
                g_lpLang[g_iLangBuff].atmLanguageName = AddAtom(szData);
                g_iLangBuff++;
            }
        }

        dwIndex++;
        cchValue = sizeof(szValue) / sizeof(TCHAR);
        szValue[0] = TEXT('\0');
        cbData = sizeof(szData);
        szData[0] = TEXT('\0');
        dwRetVal = RegEnumValue( hKey,
                                 dwIndex,
                                 szValue,
                                 &cchValue,
                                 NULL,
                                 NULL,
                                 (LPBYTE)szData,
                                 &cbData );

    } while (dwRetVal == ERROR_SUCCESS);

    //
    //  If we succeeded in opening the INF file, close it.
    //
    if (bRet)
    {
        Region_CloseInfFile(&hIntlInf);
    }

    RegCloseKey(hKey);
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_GetActiveLocales
//
//  Gets the active locales.
//
////////////////////////////////////////////////////////////////////////////

BOOL Locale_GetActiveLocales(
    HWND hwnd)
{
    HKL *pLangs;
    UINT nLangs, ctr1, ctr2, ctr3, id;
    HWND hwndList = GetDlgItem(hwnd, IDC_KBDL_LOCALE_LIST);
    HKL hklSystem;
    int idxListBox;
    DWORD langLay;
    HANDLE hLangNode;
    LPLANGNODE pLangNode;
    HICON hIcon = NULL;

    //
    //  Initialize US layout option.
    //
    g_iUsLayout = -1;

    //
    //  Get the active keyboard layout list from the system.
    //
    if (!SystemParametersInfo( SPI_GETDEFAULTINPUTLANG,
                               0,
                               (LPVOID)((LPDWORD)&hklSystem),
                               0 ))
    {
        hklSystem = GetKeyboardLayout(0);
    }

    nLangs = GetKeyboardLayoutList(0, NULL);
    if (nLangs == 0)
    {
        Locale_EnablePane(hwnd, FALSE, IDC_KBDL_DISABLED);
        return (FALSE);
    }
    pLangs = (HKL *)LocalAlloc(LPTR, sizeof(DWORD) * nLangs);
    GetKeyboardLayoutList(nLangs, (HKL *)pLangs);

    //
    //  Create the image list for the IME icon.
    //
    g_himIndicators = ImageList_Create( GetSystemMetrics(SM_CXSMICON),
                                      GetSystemMetrics(SM_CYSMICON),
                                      TRUE,
                                      0,
                                      0 );

    //
    //  Load the check mark icon.
    //
    hIcon = LoadImage( hInstance,
                       MAKEINTRESOURCE(IDI_DEFAULT_CHECK),
                       IMAGE_ICON,
                       0,
                       0,
                       LR_DEFAULTCOLOR );
    g_nDefaultCheck = ImageList_AddIcon(g_himIndicators, hIcon);

    //
    //  Find the position of the US layout to use as a default.
    //
    for (ctr1 = 0; ctr1 < g_iLayoutBuff; ctr1++)
    {
        if (g_lpLayout[ctr1].dwID == US_LOCALE)
        {
            g_iUsLayout = ctr1;
            break;
        }
    }
    if (ctr1 == g_iLayoutBuff)
    {
        Locale_EnablePane(hwnd, FALSE, IDC_KBDL_DISABLED);
        return (FALSE);
    }

    //
    //  Get the active keyboard information and put it in the internal
    //  language structure.
    //
    for (ctr2 = 0; ctr2 < nLangs; ctr2++)
    {
        for (ctr1 = 0; ctr1 < g_iLangBuff; ctr1++)
        {
            //
            //  See if there's a match.
            //
            if (LOWORD(pLangs[ctr2]) == LOWORD(g_lpLang[ctr1].dwID))
            {
                //
                //  Found a match.
                //  Create a node for this language.
                //
                pLangNode = Locale_AddToLinkedList(ctr1, pLangs[ctr2]);
                if (!pLangNode)
                {
                    Locale_EnablePane(hwnd, FALSE, IDC_KBDL_DISABLED);
                    return (FALSE);
                }

                //
                //  Add the item data to the list box, mark the
                //  language as original and active, save the pointer
                //  to the match in the layout list, and get the
                //  2 letter indicator symbol.
                //
                idxListBox = ListBox_AddItemData(hwndList, pLangNode);
                if ((HIWORD(pLangs[ctr2]) & 0xf000) == 0xe000)
                {
                    pLangNode->wStatus |= LANG_IME;
                }
                pLangNode->wStatus |= (LANG_ORIGACTIVE | LANG_ACTIVE);
                pLangNode->hkl = pLangs[ctr2];
                pLangNode->hklUnload = pLangs[ctr2];
                Locale_FetchIndicator(pLangNode);

                //
                //  Match the language to the layout.
                //
                pLangNode->iLayout = 0;
                langLay = (DWORD)HIWORD(pLangs[ctr2]);

                if ((HIWORD(pLangs[ctr2]) == 0xffff) ||
                    (HIWORD(pLangs[ctr2]) == 0xfffe))
                {
                    //
                    //  Mark default or previous error as US - this
                    //  means that the layout will be that of the basic
                    //  keyboard driver (the US one).
                    //
                    pLangNode->wStatus |= LANG_CHANGED;
                    pLangNode->iLayout = g_iUsLayout;
                    langLay = 0;
                }
                else if ((HIWORD(pLangs[ctr2]) & 0xf000) == 0xf000)
                {
                    //
                    //  Layout is special, need to search for the ID
                    //  number.
                    //
                    id = HIWORD(pLangs[ctr2]) & 0x0fff;
                    for (ctr3 = 0; ctr3 < g_iLayoutBuff; ctr3++)
                    {
                        if (id == g_lpLayout[ctr3].iSpecialID)
                        {
                            pLangNode->iLayout = ctr3;
                            langLay = 0;
                            break;
                        }
                    }
                    if (langLay)
                    {
                        //
                        //  Didn't find the id, so reset to basic for
                        //  the language.
                        //
                        langLay = (DWORD)LOWORD(pLangs[ctr2]);
                    }
                }

                if (langLay)
                {
                    //
                    //  Search for the id.
                    //
                    for (ctr3 = 0; ctr3 < g_iLayoutBuff; ctr3++)
                    {
                        if (((LOWORD(langLay) & 0xf000) == 0xe000) &&
                            (g_lpLayout[ctr3].dwID) == (DWORD_PTR)(pLangs[ctr2]))
                        {
                            pLangNode->iLayout = ctr3;
                            break;
                        }
                        else
                        {
                            if (langLay == (DWORD)LOWORD(g_lpLayout[ctr3].dwID))
                            {
                                pLangNode->iLayout = ctr3;
                                break;
                            }
                        }
                    }

                    if (ctr3 == g_iLayoutBuff)
                    {
                        //
                        //  Something went wrong or didn't load from
                        //  the registry correctly.
                        //
                        MessageBeep(MB_ICONEXCLAMATION);
                        pLangNode->wStatus |= LANG_CHANGED;
                        pLangNode->iLayout = g_iUsLayout;
                    }
                }

                //
                //  If this is the current language, then it's the default
                //  one.
                //
                if (pLangNode->hkl == hklSystem)
                {
                    TCHAR sz[DESC_MAX];
                    LPINPUTLANG pInpLang = &g_lpLang[ctr1];

                    //
                    //  Found the default.  Set the Default input locale
                    //  text in the property sheet.
                    //
                    if (pLangNode->wStatus & LANG_IME)
                    {
                        GetAtomName( g_lpLayout[pLangNode->iLayout].atmLayoutText,
                                     sz,
                                     DESC_MAX );
                    }
                    else
                    {
                        GetAtomName(pInpLang->atmLanguageName, sz, DESC_MAX);
                    }
                    pLangNode->wStatus |= LANG_DEFAULT;
                    ListBox_SetCurSel( GetDlgItem(hwnd, IDC_KBDL_LOCALE_LIST),
                                       idxListBox );
                }

                //
                //  Break out of inner loop - we've found it.
                //
                break;
            }
        }
    }

    LocalFree((HANDLE)pLangs);
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_InitPropSheet
//
//  Processing for a WM_INITDIALOG message for the Input Locales
//  property sheet.
//
////////////////////////////////////////////////////////////////////////////

void Locale_InitPropSheet(
    HWND hwnd,
    LPPROPSHEETPAGE psp)
{
    HKEY hKey;
    HANDLE hlib;
    HWND hwndList;
    LPLANGNODE pLangNode;
    LANGID LangID;
    UINT iNumLangs, ctr;
    TCHAR szItem[DESC_MAX];
    BOOL bImeSetting = FALSE;

    //
    //  See if there are any other instances of this property page.
    //  If so, disable this page.
    //
    if (g_hMutex && (WaitForSingleObject(g_hMutex, 0) != WAIT_OBJECT_0))
    {
        Locale_EnablePane(hwnd, FALSE, IDC_KBDL_DISABLED_2);
        return;
    }
    else
    {
        Locale_EnablePane(hwnd, TRUE, 0);
    }

    //
    //  Make sure the event is clear.
    //
    if (g_hEvent)
    {
        SetEvent(g_hEvent);
    }

    //
    //  Set the setup special case flag.
    //
    if (psp && psp->lParam)
    {
        g_bSetupCase = TRUE;

        if (psp->lParam == SETUP_SWITCH_I)
        {
            SysLocaleID = RegSysLocaleID;
        }
        psp->lParam = 0;
    }

    //
    //  See if the user has Administrative privileges by checking for
    //  write permission to the registry key.
    //
    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      szLocaleInfo,
                      0L,
                      KEY_WRITE,
                      &hKey ) == ERROR_SUCCESS)
    {
        //
        //  We can write to the HKEY_LOCAL_MACHINE key, so the user
        //  has Admin privileges.
        //
        g_bAdmin_Privileges = TRUE;
        RegCloseKey(hKey);
    }
    else
    {
        //
        //  The user does not have admin privileges.
        //
        g_bAdmin_Privileges = FALSE;
    }

    //
    //  Initialize all of the global variables.
    //
    if ((!Locale_LoadLayouts(hwnd)) ||
        (!Locale_LoadLocales(hwnd)) ||
        (!Locale_GetActiveLocales(hwnd)))
    {
        return;
    }

    g_cxIcon = GetSystemMetrics(SM_CXSMICON);
    g_cyIcon = GetSystemMetrics(SM_CYSMICON);

    //
    //  Get hotkey information.
    //
    Locale_GetHotkeys(hwnd);

    //
    //  Get Attributes information (CapsLock/ShiftLock etc.)
    //
    Locale_GetAttributes(hwnd);

    //
    //  Load virtual key description.
    //
    for (ctr = 0; (ctr < sizeof(g_aVirtKeyDesc) / sizeof(VIRTKEYDESC)); ctr++)
    {
        LoadString( hInstance,
                    g_aVirtKeyDesc[ctr].idVirtKeyName,
                    szItem,
                    sizeof(szItem) / sizeof(TCHAR) );
        g_aVirtKeyDesc[ctr].atVirtKeyName = AddAtom(szItem);
    }
    g_dwChanges = 0;

    //
    //  See how many active keyboard layouts are in the input locale list.
    //
    hwndList = GetDlgItem(hwnd, IDC_KBDL_LOCALE_LIST);
    iNumLangs = ListBox_GetCount(hwndList);

    if (iNumLangs == 1)
    {
        //
        //  Only 1 active keyboard, so disable the secondary controls.
        //
        Locale_SetSecondaryControls(hwnd, FALSE);

        //
        //  Special case some of the FE languages.
        //
        pLangNode = (LPLANGNODE)ListBox_GetItemData(hwndList, 0);
        if ((LPARAM)pLangNode != LB_ERR)
        {
            LangID = LOWORD(pLangNode->hkl);
            if (IS_FE_LANGUAGE(PRIMARYLANGID(LangID)))
            {
                //
                //  Enable the indicator symbol check box and check it.
                //
                EnableWindow(GetDlgItem(hwnd, IDC_KBDL_INDICATOR), TRUE);
                CheckDlgButton( hwnd,
                                IDC_KBDL_INDICATOR,
                                Locale_IsIndicatorPresent() );
            }
        }
        if (pLangNode->wStatus & LANG_IME)
        {
            bImeSetting = TRUE;
        }
    }
    else
    {
        //
        //  Set the indicator symbol check box to the "checked" state
        //  if the check box is enabled.
        //
        CheckDlgButton( hwnd,
                        IDC_KBDL_INDICATOR,
                        Locale_IsIndicatorPresent() );
        for (ctr = 0; ctr < iNumLangs; ctr++)
        {
            pLangNode = (LPLANGNODE)ListBox_GetItemData(hwndList, ctr);
            if (pLangNode->wStatus & LANG_IME)
            {
                bImeSetting = TRUE;
                break;
            }
        }
    }

    //
    //  See if we need to show the IME Settings button.
    //
    if (bImeSetting && !g_bSetupCase)
    {
        ShowWindow(GetDlgItem(hwnd, IDC_KBDL_IME_SETTINGS), SW_SHOW);
    }
    else
    {
        ShowWindow(GetDlgItem(hwnd, IDC_KBDL_IME_SETTINGS), SW_HIDE);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_TranslateHotKey
//
//  Translates hotkey modifiers and values into key names.
//
////////////////////////////////////////////////////////////////////////////

void Locale_TranslateHotKey(
    LPTSTR szString,
    UINT uVKey,
    UINT uModifiers)
{
    UINT ctr;
    TCHAR szBuffer[DESC_MAX];
    BOOL bMod = FALSE;

    szString[0] = 0;

    if (uModifiers & MOD_CONTROL)
    {
        LoadString(hInstance, IDS_KBD_MOD_CONTROL, szBuffer, DESC_MAX);
        lstrcat(szString, szBuffer);
        bMod = TRUE;
    }

    if (uModifiers & MOD_ALT)
    {
        LoadString(hInstance, IDS_KBD_MOD_LEFT_ALT, szBuffer, DESC_MAX);
        lstrcat(szString, szBuffer);
        bMod = TRUE;
    }

    if (uModifiers & MOD_SHIFT)
    {
        LoadString(hInstance, IDS_KBD_MOD_SHIFT, szBuffer, DESC_MAX);

        lstrcat(szString, szBuffer);
        bMod = TRUE;
    }

    if (uVKey == 0)
    {
        if (!bMod)
        {
            GetAtomName( g_aVirtKeyDesc[0].atVirtKeyName,
                         szBuffer,
                         sizeof(szBuffer) / sizeof(TCHAR) );
            lstrcat(szString, szBuffer);
            return;
        }
        else
        {
            //
            //  Only modifiers, remove the "+" at the end.
            //
            szString[lstrlen(szString) - 1] = 0;
            return;
        }
    }

    for (ctr = 0; (ctr < sizeof(g_aVirtKeyDesc) / sizeof(VIRTKEYDESC)); ctr++)
    {
        if (g_aVirtKeyDesc[ctr].uVirtKeyValue == uVKey)
        {
            GetAtomName( g_aVirtKeyDesc[ctr].atVirtKeyName,
                         szBuffer,
                         sizeof(szBuffer) / sizeof(TCHAR) );
            lstrcat(szString, szBuffer);
            return;
        }
    }

    GetAtomName( g_aVirtKeyDesc[0].atVirtKeyName,
                 szBuffer,
                 sizeof(szBuffer) / sizeof(TCHAR) );
    lstrcat(szString, szBuffer);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_HotKeyDrawItem
//
//  Draws the hotkey list box.
//
////////////////////////////////////////////////////////////////////////////

void Locale_HotKeyDrawItem(
    HWND hWnd,
    LPDRAWITEMSTRUCT lpDis)
{
    LPHOTKEYINFO pHotKeyNode;
    COLORREF crBk, crTxt;
    UINT uStrLen, uAlign;
    TCHAR szString[DESC_MAX];
    TCHAR szHotKey[DESC_MAX];
    SIZE Size;
    UINT iMaxChars;
    int iMaxWidth;

    if (!ListBox_GetCount(lpDis->hwndItem))
    {
        return;
    }

    pHotKeyNode = (LPHOTKEYINFO)lpDis->itemData;

    crBk = SetBkColor( lpDis->hDC,
                       (lpDis->itemState & ODS_SELECTED)
                         ? GetSysColor(COLOR_HIGHLIGHT)
                         : GetSysColor(COLOR_WINDOW) );

    crTxt = SetTextColor( lpDis->hDC,
                          (lpDis->itemState & ODS_SELECTED)
                            ? GetSysColor(COLOR_HIGHLIGHTTEXT)
                            : GetSysColor(COLOR_WINDOWTEXT) );

    Locale_TranslateHotKey( szHotKey,
                            pHotKeyNode->uVKey,
                            pHotKeyNode->uModifiers );

    GetTextExtentExPoint( lpDis->hDC,
                          szHotKey,
                          lstrlen(szHotKey),
                          0,
                          NULL,
                          NULL ,
                          &Size );

    iMaxWidth = lpDis->rcItem.right - lpDis->rcItem.left - Size.cx - LIST_MARGIN * 8;

    uStrLen = GetAtomName( pHotKeyNode->atmHotKeyName,
                           szString,
                           sizeof(szString) / sizeof(TCHAR) );

    GetTextExtentExPoint( lpDis->hDC,
                          szString,
                          uStrLen,
                          iMaxWidth,
                          &iMaxChars,
                          NULL ,
                          &Size );

    if (uStrLen > iMaxChars)
    {
        szString[iMaxChars-3] = TEXT('.');
        szString[iMaxChars-2] = TEXT('.');
        szString[iMaxChars-1] = TEXT('.');
        szString[iMaxChars]   = 0;
    }

    ExtTextOut( lpDis->hDC,
                lpDis->rcItem.left + LIST_MARGIN,
                lpDis->rcItem.top + (g_cyListItem - g_cyText) / 2,
                ETO_OPAQUE,
                &lpDis->rcItem,
                szString,
                iMaxChars,
                NULL );

    uAlign = GetTextAlign(lpDis->hDC);

    SetTextAlign(lpDis->hDC, TA_RIGHT);

    ExtTextOut( lpDis->hDC,
                lpDis->rcItem.right - LIST_MARGIN,
                lpDis->rcItem.top + (g_cyListItem - g_cyText) / 2,
                0,
                NULL,
                szHotKey,
                lstrlen(szHotKey),
                NULL );

    SetTextAlign(lpDis->hDC, uAlign);

    SetBkColor(lpDis->hDC, crBk);

    SetTextColor(lpDis->hDC, crTxt);

    if (lpDis->itemState & ODS_FOCUS)
    {
        DrawFocusRect(lpDis->hDC, &lpDis->rcItem);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_DrawItem
//
//  Processing for a WM_DRAWITEM message.
//
////////////////////////////////////////////////////////////////////////////

BOOL Locale_DrawItem(
    HWND hwnd,
    LPDRAWITEMSTRUCT lpdi)
{
    switch (lpdi->CtlID)
    {
#ifdef ON_SCREEN_KEYBOARD
        case ( IDC_KBDL_UP ) :
        case ( IDC_KBDL_DOWN ) :
        {
            UINT wFlags;

            wFlags = ((lpdi->CtlID == IDC_KBDL_UP)
                         ? DFCS_SCROLLUP
                         : DFCS_SCROLLDOWN);

            if (lpdi->itemState & ODS_SELECTED)
            {
                wFlags |= DFCS_PUSHED;
            }
            else if (lpdi->itemState & ODS_DISABLED)
            {
                wFlags |= DFCS_INACTIVE;
            }

            DrawFrameControl(lpdi->hDC, &lpdi->rcItem, DFC_SCROLL, wFlags);
            break;
        }
#endif
        case ( IDC_KBDL_LOCALE_LIST ) :
        {
            LPLANGNODE pLangNode;
            LPINPUTLANG pInpLang;
            TCHAR sz[DESC_MAX];
            UINT len;
            DWORD rgbBk;
            DWORD rgbText;
            UINT oldAlign;
            RECT rc;
            UINT nList;
            int cxDefIcon;

            if ((nList = ListBox_GetCount(lpdi->hwndItem)) == 0)
            {
                break;
            }
            else if (nList == 1)
            {
                cxDefIcon = 0;
            }
            else
            {
                cxDefIcon = g_cxIcon;
            }

            pLangNode = (LPLANGNODE)lpdi->itemData;
            pInpLang = &g_lpLang[pLangNode->iLang];
            rgbBk = SetBkColor( lpdi->hDC,
                                (lpdi->itemState & ODS_SELECTED)
                                    ? GetSysColor(COLOR_HIGHLIGHT)
                                    : GetSysColor(COLOR_WINDOW) );

            rgbText = SetTextColor( lpdi->hDC,
                                    (lpdi->itemState & ODS_SELECTED)
                                        ? GetSysColor(COLOR_HIGHLIGHTTEXT)
                                        : GetSysColor(COLOR_WINDOWTEXT) );

            len = GetAtomName(pInpLang->atmLanguageName, sz, DESC_MAX);

            ExtTextOut( lpdi->hDC,
                        lpdi->rcItem.left + cxDefIcon + g_cxIcon + 3 * LIST_MARGIN + 2,
                        lpdi->rcItem.top + (g_cyListItem - g_cyText) / 2,
                        ETO_OPAQUE,
                        &lpdi->rcItem,
                        sz,
                        len,
                        NULL );

            oldAlign = GetTextAlign(lpdi->hDC);
            SetTextAlign(lpdi->hDC, TA_RIGHT | (oldAlign & ~TA_CENTER));

            len = GetAtomName( g_lpLayout[pLangNode->iLayout].atmLayoutText,
                               sz,
                               DESC_MAX );

            ExtTextOut( lpdi->hDC,
                        lpdi->rcItem.right - LIST_MARGIN,
                        lpdi->rcItem.top + (g_cyListItem - g_cyText) / 2,
                        0,
                        NULL,
                        sz,
                        len,
                        NULL );

            SetTextAlign(lpdi->hDC, oldAlign);

            if (!(pLangNode->wStatus & ICON_LOADED))
            {
                Locale_FetchIndicator(pLangNode);
            }

            if ((g_himIndicators != NULL) &&
                (pLangNode->wStatus & LANG_IME) &&
                (pLangNode->nIconIME != -1))
            {
                ImageList_Draw( g_himIndicators,
                                pLangNode->nIconIME,
                                lpdi->hDC,
                                lpdi->rcItem.left + cxDefIcon + 3 * LIST_MARGIN,
                                lpdi->rcItem.top + LIST_MARGIN,
                                ILD_TRANSPARENT );
            }
            else
            {
                rgbBk = SetBkColor( lpdi->hDC,
                                    (lpdi->itemState & ODS_SELECTED)
                                        ? GetSysColor(COLOR_WINDOW)
                                        : GetSysColor(COLOR_HIGHLIGHT) );

                rgbText = SetTextColor( lpdi->hDC,
                                        (lpdi->itemState & ODS_SELECTED)
                                            ? GetSysColor(COLOR_WINDOWTEXT)
                                            : GetSysColor(COLOR_HIGHLIGHTTEXT) );
                rc.left = lpdi->rcItem.left + cxDefIcon + 3 * LIST_MARGIN;
                rc.right = rc.left + g_cxIcon;
                rc.top = lpdi->rcItem.top + LIST_MARGIN;
                rc.bottom = rc.top + g_cyIcon;
                ExtTextOut( lpdi->hDC,
                            rc.left,
                            rc.top,
                            ETO_OPAQUE,
                            &rc,
                            TEXT(""),
                            0,
                            NULL );
                DrawText( lpdi->hDC,
                          pInpLang->szSymbol,
                          2,
                          &rc,
                          DT_CENTER | DT_VCENTER | DT_SINGLELINE );
            }

            if ((nList > 1) && (pLangNode->wStatus & LANG_DEFAULT))
            {
                ImageList_Draw( g_himIndicators,
                                g_nDefaultCheck,
                                lpdi->hDC,
                                lpdi->rcItem.left + LIST_MARGIN,
                                lpdi->rcItem.top + LIST_MARGIN,
                                ILD_TRANSPARENT );
            }

            SetBkColor(lpdi->hDC, rgbBk);
            SetTextColor(lpdi->hDC, rgbText);

            if (lpdi->itemState & ODS_FOCUS)
            {
                DrawFocusRect(lpdi->hDC, &lpdi->rcItem);
            }

            break;
        }
        case ( IDC_KBDL_HOTKEY_LIST ) :
        {
            Locale_HotKeyDrawItem(hwnd, lpdi);
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_KillPaneDialog
//
//  Processing for a WM_DESTROY message.
//
////////////////////////////////////////////////////////////////////////////

void Locale_KillPaneDialog(
    HWND hwnd)
{
    UINT ctr, iCount;
    HANDLE hCur;
    LPLANGNODE pCur;
    LPHOTKEYINFO aImeHotKey;

    //
    //  Delete all hot key atoms and free up the hotkey arrays.
    //
    if (g_SwitchLangHotKey.atmHotKeyName)
    {
        DeleteAtom(g_SwitchLangHotKey.atmHotKeyName);
    }

    iCount = Locale_GetImeHotKeyInfo(hwnd, &aImeHotKey);
    for (ctr = 0; ctr < iCount; ctr++)
    {
        if (aImeHotKey[ctr].atmHotKeyName)
        {
            DeleteAtom(aImeHotKey[ctr].atmHotKeyName);
        }
    }

    for (ctr = 0; ctr < DSWITCH_HOTKEY_SIZE; ctr++)
    {
        if (g_aDirectSwitchHotKey[ctr].atmHotKeyName)
        {
            DeleteAtom(g_aDirectSwitchHotKey[ctr].atmHotKeyName);
        }
    }

    //
    //  Delete all Language Name atoms and free the g_lpLang array.
    //
    for (ctr = 0; ctr < g_iLangBuff; ctr++)
    {
        if (g_lpLang[ctr].atmLanguageName)
        {
            DeleteAtom(g_lpLang[ctr].atmLanguageName);
        }

        pCur = g_lpLang[ctr].pNext;
        g_lpLang[ctr].pNext = NULL;
        while (pCur)
        {
            hCur = pCur->hLangNode;
            pCur = pCur->pNext;
            GlobalUnlock(hCur);
            GlobalFree(hCur);
        }
    }
    if (g_himIndicators != NULL)
    {
        ImageList_Destroy(g_himIndicators);
    }
    GlobalUnlock(g_hLang);
    GlobalFree(g_hLang);

    //
    //  Delete all layout text and layout file atoms and free the
    //  g_lpLayout array.
    //
    for (ctr = 0; ctr < g_iLayoutBuff; ctr++)
    {
        if (g_lpLayout[ctr].atmLayoutText)
        {
            DeleteAtom(g_lpLayout[ctr].atmLayoutText);
        }
        if (g_lpLayout[ctr].atmLayoutFile)
        {
            DeleteAtom(g_lpLayout[ctr].atmLayoutFile);
        }
        if (g_lpLayout[ctr].atmIMEFile)
        {
            DeleteAtom(g_lpLayout[ctr].atmIMEFile);
        }
    }

    GlobalUnlock(g_hLayout);
    GlobalFree(g_hLayout);

    //
    //  Make sure the mutex is released.
    //
    if (g_hMutex)
    {
        ReleaseMutex(g_hMutex);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_MeasureItem
//
//  Processing for a WM_MEASUREITEM message.
//
////////////////////////////////////////////////////////////////////////////

void Locale_MeasureItem(
    HWND hwnd,
    LPMEASUREITEMSTRUCT lpmi)
{
    HFONT hfont;
    HDC hdc;
    TEXTMETRIC tm;

    switch (lpmi->CtlID)
    {
        case ( IDC_KBDL_LOCALE_LIST ) :
        case ( IDC_KBDL_HOTKEY_LIST ) :
        {
            hfont = (HFONT) SendMessage(hwnd, WM_GETFONT, 0, 0);
            hdc = GetDC(NULL);
            hfont = SelectObject(hdc, hfont);

            GetTextMetrics(hdc, &tm);
            SelectObject(hdc, hfont);
            ReleaseDC(NULL, hdc);

            g_cyText = tm.tmHeight;
            lpmi->itemHeight = g_cyListItem =
                MAX(g_cyText, GetSystemMetrics(SM_CYSMICON)) + 2 * LIST_MARGIN;

            break;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_EnableIMESetting
//
//  Enable or disable IME settings button based on whether or not the
//  current selection is an IME.
//
////////////////////////////////////////////////////////////////////////////

void Locale_EnableIMESetting(
    HWND hwnd)
{
    LPLANGNODE pLangNode;
    BOOL bOn;
    HWND hwndIme = GetDlgItem(hwnd, IDC_KBDL_IME_SETTINGS);
    HWND hwndList = GetDlgItem(hwnd, IDC_KBDL_LOCALE_LIST);
    int idx;

    if ((!g_bSetupCase) &&
        ((idx = ListBox_GetCurSel(hwndList)) != LB_ERR))
    {
        pLangNode = (LPLANGNODE)ListBox_GetItemData(hwndList, idx);
        bOn = (pLangNode->wStatus & LANG_IME) &&
              (pLangNode->wStatus & LANG_ORIGACTIVE) &&
              !(pLangNode->wStatus & LANG_CHANGED);

        EnableWindow(hwndIme, bOn);
        if (bOn && !IsWindowVisible(hwndIme))
        {
            ShowWindow(hwndIme, SW_SHOW);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_CommandAdd
//
//  Invokes the Add dialog.
//
//  Returns 1 if a dialog box was invoked and the dialog returned IDOK.
//  Otherwise, it returns 0.
//
////////////////////////////////////////////////////////////////////////////

int Locale_CommandAdd(
    HWND hwnd,
    LPLANGNODE pLangNode)
{
    HWND hwndList;
    int idxList;
    UINT nList;
    int rc = 0;
    INITINFO InitInfo;

    //
    //  Initialize hwndList and pLangNode.
    //
    if (pLangNode == NULL)
    {
        hwndList = GetDlgItem(hwnd, IDC_KBDL_LOCALE_LIST);
        if (hwndList == NULL)
        {
            return (0);
        }
        idxList = ListBox_GetCurSel(hwndList);
        pLangNode = (LPLANGNODE)ListBox_GetItemData(hwndList, idxList);
    }
    else
    {
        hwndList = GetDlgItem(hwnd, IDC_KBDLA_LOCALE);
        if (hwndList == NULL)
        {
            return (0);
        }
        idxList = ListBox_GetCurSel(hwndList);
    }

    //
    //  Make sure we haven't added all possible combinations to the system.
    //
    nList = ListBox_GetCount(hwndList);

    if (nList == (g_iLangBuff * g_iLayoutBuff))
    {
        //
        //  No languages left!
        //
        Locale_ErrorMsg(hwnd, IDS_KBD_NO_MORE_TO_ADD, NULL);
        return (rc);
    }

    //
    //  Bring up the appropriate dialog box.
    //
    if ((pLangNode != (LPLANGNODE)LB_ERR) && (pLangNode != NULL))
    {
        //
        //  Return value can be 1:IDOK, 2:IDCANCEL or -1:Error (from USER)
        //
        //  If adding a language, it goes at the end of the list, so get
        //  the end and make that the current selection.
        //
        InitInfo.hwndMain = hwnd;
        InitInfo.pLangNode = pLangNode;
        if ((rc = (int)DialogBoxParam( hInstance,
                                  MAKEINTRESOURCE(DLG_KEYBOARD_LOCALE_ADD),
                                  hwnd,
                                  KbdLocaleAddDlg,
                                  (LPARAM)(&InitInfo) )) == IDOK)
        {
            //
            //  Get the number of items in the input locale list and
            //  enable the Properties and Remove push buttons.
            //
            nList = ListBox_GetCount(hwndList) - 1;
            EnableWindow(GetDlgItem(hwnd, IDC_KBDL_EDIT), TRUE);
            EnableWindow(GetDlgItem(hwnd, IDC_KBDL_DELETE), TRUE);

            //
            //  Set the current selection to be the last one in the
            //  list (the one that was just added).
            //
            ListBox_SetCurSel(hwndList, nList);

            pLangNode = (LPLANGNODE)ListBox_GetItemData(hwndList, nList);

            //
            //  Set the default locale switch hotkey.
            //
            Locale_SetDefaultHotKey(hwnd, TRUE);

            //
            //  Turn on ApplyNow button.
            //
            PropSheet_Changed(GetParent(hwnd), hwnd);
        }
        else
        {
            //
            //  Failure, so need to return 0.
            //
            ListBox_SetCurSel(hwndList, idxList);
            rc = 0;
        }
    }
    else
    {
        MessageBeep(MB_ICONEXCLAMATION);
    }

    Locale_EnableIMESetting(hwnd);
    return (rc);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_CommandEdit
//
//  Invokes the Properties dialog.
//
//  Returns 1 if a dialog box was invoked and the dialog returned IDOK.
//  Otherwise, it returns 0.
//
////////////////////////////////////////////////////////////////////////////

int Locale_CommandEdit(
    HWND hwnd,
    LPLANGNODE pLangNode)
{
    HWND hwndList;
    int idxList;
    UINT nList;
    int rc = 0;
    INITINFO InitInfo;

    //
    //  Initialize hwndList and pLangNode.
    //
    if (pLangNode == NULL)
    {
        hwndList = GetDlgItem(hwnd, IDC_KBDL_LOCALE_LIST);
        if (hwndList == NULL)
        {
            return (0);
        }
        idxList = ListBox_GetCurSel(hwndList);
        pLangNode = (LPLANGNODE)ListBox_GetItemData(hwndList, idxList);
    }
    else
    {
        hwndList = GetDlgItem(hwnd, IDC_KBDLA_LOCALE);
        if (hwndList == NULL)
        {
            return (0);
        }
        idxList = ListBox_GetCurSel(hwndList);
    }

    //
    //  Bring up the appropriate dialog box.
    //
    if ((pLangNode != (LPLANGNODE)LB_ERR) && (pLangNode != NULL))
    {
        //
        //  Return value can be 1:IDOK, 2:IDCANCEL or -1:Error (from USER)
        //
        //  If adding a language, it goes at the end of the list, so get
        //  the end and make that the current selection.
        //
        InitInfo.hwndMain = hwnd;
        InitInfo.pLangNode = pLangNode;
        if ((rc = (int)DialogBoxParam( hInstance,
                                  MAKEINTRESOURCE(DLG_KEYBOARD_LOCALE_EDIT),
                                  hwnd,
                                  KbdLocaleEditDlg,
                                  (LPARAM)(&InitInfo) )) == IDOK)
        {
            //
            //  Reset the current selection to be the one that was
            //  previously set.
            //
            ListBox_SetCurSel(hwndList, idxList);

            //
            //  Turn on ApplyNow button.
            //
            PropSheet_Changed(GetParent(hwnd), hwnd);
        }
        else
        {
            //
            //  Failure, so need to return 0.
            //
            ListBox_SetCurSel(hwndList, idxList);
            rc = 0;
        }
    }
    else
    {
        MessageBeep(MB_ICONEXCLAMATION);
    }

    Locale_EnableIMESetting(hwnd);
    return (rc);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_CommandLocaleList
//
//  Handles changes to the input locales list box in the property sheet.
//
////////////////////////////////////////////////////////////////////////////

void Locale_CommandLocaleList(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam)
{
    if ((HIWORD(wParam) == LBN_SELCHANGE) || (HIWORD(wParam) == LBN_SETFOCUS))
    {
        //
        //  User changed selection.  Enable or disable IME Settings button
        //  accordingly.
        //
        Locale_EnableIMESetting(hwnd);
    }
    else if (HIWORD(wParam) == LBN_DBLCLK)
    {
        //
        //  User double clicked on an input locale.  Invoke the Properties
        //  dialog.
        //
        Locale_CommandEdit(hwnd, NULL);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_CommandSetDefault
//
//  Sets the new default when the Set as Default button is pressed.
//
////////////////////////////////////////////////////////////////////////////

void Locale_CommandSetDefault(
    HWND hwnd)
{
    UINT idx;
    int idxList;
    LPLANGNODE pLangNode;
    HWND hwndList = GetDlgItem(hwnd, IDC_KBDL_LOCALE_LIST);
    TCHAR sz[DESC_MAX];

    //
    //  Get the current selection in the list box.
    //
    if ((idxList = ListBox_GetCurSel(hwndList)) == LB_ERR)
    {
        MessageBeep(MB_ICONEXCLAMATION);
        return;
    }

    //
    //  Remove the LANG_DEFAULT flag from the current default.
    //
    pLangNode = NULL;
    for (idx = 0; idx < g_iLangBuff; idx++)
    {
        pLangNode = g_lpLang[idx].pNext;
        while (pLangNode != NULL)
        {
            if (pLangNode->wStatus & LANG_DEFAULT)
            {
                if (pLangNode ==
                     (LPLANGNODE)ListBox_GetItemData(hwndList, idxList))
                {
                    return;
                }
                pLangNode->wStatus &= ~(LANG_DEFAULT | LANG_DEF_CHANGE);
                break;
            }
            pLangNode = pLangNode->pNext;
        }
        if (pLangNode != NULL)
        {
            break;
        }
    }

    //
    //  Mark the current selection as the new default.
    //
    pLangNode = (LPLANGNODE)ListBox_GetItemData(hwndList, idxList);
    pLangNode->wStatus |= (LANG_DEFAULT | LANG_DEF_CHANGE);

    //
    //  Update the "Default input locale" text in the dialog.
    //
    if (pLangNode->wStatus & LANG_IME)
    {
        GetAtomName(g_lpLayout[pLangNode->iLayout].atmLayoutText, sz, DESC_MAX);
    }
    else
    {
        GetAtomName(g_lpLang[pLangNode->iLang].atmLanguageName, sz, DESC_MAX);
    }
    InvalidateRect(hwndList, NULL, FALSE);

    //
    //  Enable the Apply button.
    //
    g_dwChanges |= CHANGE_DEFAULT;
    PropSheet_Changed(GetParent(hwnd), hwnd);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_CommandDelete
//
//  Removes the currently selected input locale from the list.
//
////////////////////////////////////////////////////////////////////////////

void Locale_CommandDelete(
    HWND hwnd)
{
    LPLANGNODE pLangNode;
    int idxList;
    HWND hwndList = GetDlgItem(hwnd, IDC_KBDL_LOCALE_LIST);
    int count;
    LPLANGNODE pLangNodeLeft;
    LANGID LangID;

    //
    //  Get the current selection in the input locale list.
    //
    if ((idxList = ListBox_GetCurSel(hwndList)) == LB_ERR)
    {
        MessageBeep(MB_ICONEXCLAMATION);
        return;
    }

    //
    //  Make sure we're not removing the only entry in the list.
    //
    if (ListBox_GetCount(hwndList) == 1)
    {
        MessageBeep(MB_ICONEXCLAMATION);
        return;
    }

    //
    //  Get the pointer to the lang node from the list box
    //  item data.
    //
    pLangNode = (LPLANGNODE)ListBox_GetItemData(hwndList, idxList);

    //
    //  Set the input locale to be not active and show that its state
    //  has changed.  Also, delete the string from the input locale list
    //  in the property sheet.
    //
    //  Decrement the number of nodes for this input locale.
    //
    pLangNode->wStatus &= ~LANG_ACTIVE;
    pLangNode->wStatus |= LANG_CHANGED;
    ListBox_DeleteString(hwndList, idxList);

    g_lpLang[pLangNode->iLang].iNumCount--;

    //
    //  See how many entries are left in the input locale list box.
    //
    if (count = ListBox_GetCount(hwndList))
    {
        //
        //  Set the new current selection.
        //
        ListBox_SetCurSel(hwndList, (count <= idxList) ? (count - 1) : idxList);

        //
        //  Get hotkey settings again.
        //
        Locale_GetHotkeys(hwnd);

        //
        //  See if there is only one entry left in the list.
        //
        if (count < 2)
        {
            //
            //  Only 1 entry in list.  Disable the secondary controls.
            //
            Locale_SetSecondaryControls(hwnd, FALSE);

            //
            //  Special case some of the FE languages.
            //
            pLangNodeLeft = (LPLANGNODE)ListBox_GetItemData(hwndList, 0);
            LangID = LOWORD(pLangNodeLeft->hkl);
            if (IS_FE_LANGUAGE(PRIMARYLANGID(LangID)))
            {
                EnableWindow(GetDlgItem(hwnd, IDC_KBDL_INDICATOR), TRUE);
                CheckDlgButton( hwnd,
                                IDC_KBDL_INDICATOR,
                                Locale_IsIndicatorPresent() );
            }
        }
    }
    else
    {
        //
        //  No entries left.  Disable the secondary controls.
        //  This should never happen since we check for this above.
        //
        Locale_SetSecondaryControls(hwnd, FALSE);
    }

    //
    //  Update the default locale switch hotkey.
    //
    Locale_SetDefaultHotKey(hwnd, FALSE);

    //
    //  If it was the default input locale, change the default to something
    //  else.
    //
    if (pLangNode->wStatus & LANG_DEFAULT)
    {
//      pLangNode->wStatus &= ~LANG_DEFAULT;
        Locale_CommandSetDefault(hwnd);
    }

    //
    //  If it wasn't originally active, then remove it from the list.
    //  There's nothing more to do with this node.
    //
    if (!(pLangNode->wStatus & LANG_ORIGACTIVE))
    {
        Locale_RemoveFromLinkedList(pLangNode);
    }

    //
    // Move the focus to the Add button if the Remove button
    // is now disabled (so that we don't lose input focus)
    //
    if (!IsWindowEnabled(GetDlgItem(hwnd, IDC_KBDL_DELETE)))
    {
        SetFocus(GetDlgItem(hwnd, IDC_KBDL_ADD));
    }

    //
    //  We may have removed an IME.  Check to see if we need to
    //  enable/disable the IME settings button.
    //
    Locale_EnableIMESetting(hwnd);

    //
    //  Enable the Apply button.
    //
    PropSheet_Changed(GetParent(hwnd), hwnd);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_CommandConfigIME
//
//  Configures the IME.
//
////////////////////////////////////////////////////////////////////////////

void Locale_CommandConfigIME(
    HWND hwnd)
{
    LPLANGNODE pLangNode;
    int idxList;
    HWND hwndList = GetDlgItem(hwnd, IDC_KBDL_LOCALE_LIST);

    if ((idxList = ListBox_GetCurSel(hwndList)) == LB_ERR)
    {
        MessageBeep(MB_ICONEXCLAMATION);
        return;
    }

    pLangNode = (LPLANGNODE)ListBox_GetItemData(hwndList, idxList);
    if ((!(pLangNode->wStatus & LANG_IME)) ||
        (!(pLangNode->wStatus & LANG_ORIGACTIVE)))
    {
        MessageBeep(MB_ICONEXCLAMATION);
        return;
    }
    ImmConfigureIME(pLangNode->hkl, hwnd, IME_CONFIG_GENERAL, NULL);

    ListBox_SetCurSel(hwndList, idxList);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_CommandChangeHotKey
//
//  Brings up change hotkey dialog box.
//
////////////////////////////////////////////////////////////////////////////

void Locale_CommandChangeHotKey(
    HWND hwnd)
{
    HWND hwndHotkey = GetDlgItem(hwnd, IDC_KBDL_HOTKEY_LIST);
    HWND hwndList = GetDlgItem(hwnd, IDC_KBDL_LOCALE_LIST);
    UINT nLangs, ctr;
    int iIndex;
    LPHOTKEYINFO pHotKeyNode;
    LPLANGNODE pLangNode;
    LPLANGNODE pTempLangNode;
    INITINFO InitInfo;
    BOOL fThai = FALSE;

    iIndex = ListBox_GetCurSel(hwndHotkey);
    pHotKeyNode = (LPHOTKEYINFO)ListBox_GetItemData(hwndHotkey, iIndex);

    InitInfo.hwndMain = hwnd;
    InitInfo.pHotKeyNode = pHotKeyNode;

    nLangs = ListBox_GetCount(hwndList);
    for (ctr = 0; ctr < nLangs; ctr++)
    {
        pLangNode = (LPLANGNODE)ListBox_GetItemData(hwndList, ctr);
        if (PRIMARYLANGID(LOWORD(g_lpLayout[pLangNode->iLayout].dwID)) == LANG_THAI)
        {
            fThai = TRUE;
        }
        if (pLangNode->hkl == pHotKeyNode->hkl)
        {
            break;
        }
    }

    while (!fThai && (ctr < nLangs))
    {
        pTempLangNode = (LPLANGNODE)ListBox_GetItemData(hwndList, ctr);
        if (PRIMARYLANGID(LOWORD(g_lpLayout[pTempLangNode->iLayout].dwID)) == LANG_THAI)
        {
            fThai = TRUE;
            break;
        }
        ctr++;
    }

    if (pHotKeyNode->dwHotKeyID == HOTKEY_SWITCH_LANG)
    {
        if ((PRIMARYLANGID(LANGIDFROMLCID(SysLocaleID)) == LANG_THAI) && fThai)
        {
            DialogBoxParam( hInstance,
                            MAKEINTRESOURCE(DLG_KEYBOARD_HOTKEY_INPUT_LOCALE_THAI),
                            hwnd,
                            (DLGPROC)KbdLocaleChangeThaiInputLocaleHotkey,
                            (LPARAM)&InitInfo );
        }
        else
        {
            DialogBoxParam( hInstance,
                            MAKEINTRESOURCE(DLG_KEYBOARD_HOTKEY_INPUT_LOCALE),
                            hwnd,
                            (DLGPROC)KbdLocaleChangeInputLocaleHotkey,
                            (LPARAM)&InitInfo );
        }
    }
    else if (pLangNode->wStatus & LANG_IME)
    {
        DialogBoxParam( hInstance,
                        MAKEINTRESOURCE(DLG_KEYBOARD_HOTKEY_IME),
                        hwnd,
                        (DLGPROC)KbdLocaleChangeImeHotkey,
                        (LPARAM)&InitInfo );
    }
    else
    {
        DialogBoxParam( hInstance,
                        MAKEINTRESOURCE(DLG_KEYBOARD_HOTKEY_KEYBOARD_LAYOUT),
                        hwnd,
                        KbdLocaleChangeKeyboardLayoutHotkey,
                        (LPARAM)&InitInfo );
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_ReloadInfo
//
////////////////////////////////////////////////////////////////////////////

void Locale_ReloadInfo(
    HWND hwnd)
{
    HWND hwndList = GetDlgItem(hwnd, IDC_KBDL_LOCALE_LIST);
    UINT ctr;
    HANDLE hCur;
    LPLANGNODE pCur;

    //
    //  Save the old globals so that they can be deleted.
    //
    LPINPUTLANG lpLang = g_lpLang;
    UINT iLangBuff = g_iLangBuff;
    HANDLE hLang = g_hLang;
    UINT nLangBuffSize = g_nLangBuffSize;

    LPLAYOUT lpLayout = g_lpLayout;
    UINT iLayoutBuff = g_iLayoutBuff;
    HANDLE hLayout = g_hLayout;
    UINT nLayoutBuffSize = g_nLayoutBuffSize;

    //
    //  See if the pane is disabled.  If so, then there is nothing to
    //  update.
    //
    if (!IsWindowEnabled(hwndList))
    {
        return;
    }

    //
    //  Clear out the list box.
    //
    ListBox_ResetContent(hwndList);

    //
    //  Zero out the global variables.
    //
    g_lpLang = NULL;
    g_iLangBuff = 0;
    g_hLang = 0;
    g_nLangBuffSize = 0;

    g_lpLayout = NULL;
    g_iLayoutBuff = 0;
    g_hLayout = 0;
    g_nLayoutBuffSize = 0;
    g_iLayoutIME = 0;
    g_iUsLayout = -1;

    //
    //  Destroy the image list, if it exists.
    //
    if (g_himIndicators != NULL)
    {
        ImageList_Destroy(g_himIndicators);
        g_himIndicators = NULL;
    }

    //
    //  Re-Initialize all of the global variables.
    //
    Locale_LoadLayouts(hwnd);
    Locale_LoadLocales(hwnd);
    Locale_GetActiveLocales(hwnd);
    Locale_GetHotkeys(hwnd);

    //
    //  Delete all Language Name atoms and free the old lpLang array.
    //
    for (ctr = 0; ctr < iLangBuff; ctr++)
    {
        if (lpLang[ctr].atmLanguageName)
        {
            DeleteAtom(lpLang[ctr].atmLanguageName);
        }

        pCur = lpLang[ctr].pNext;
        lpLang[ctr].pNext = NULL;
        while (pCur)
        {
            hCur = pCur->hLangNode;
            pCur = pCur->pNext;
            GlobalUnlock(hCur);
            GlobalFree(hCur);
        }
    }
    GlobalUnlock(hLang);
    GlobalFree(hLang);

    //
    //  Delete all layout text and layout file atoms and free the
    //  old lpLayout array.
    //
    for (ctr = 0; ctr < iLayoutBuff; ctr++)
    {
        if (lpLayout[ctr].atmLayoutText)
        {
            DeleteAtom(lpLayout[ctr].atmLayoutText);
        }
        if (lpLayout[ctr].atmLayoutFile)
        {
            DeleteAtom(lpLayout[ctr].atmLayoutFile);
        }
        if (lpLayout[ctr].atmIMEFile)
        {
            DeleteAtom(lpLayout[ctr].atmIMEFile);
        }
    }
    GlobalUnlock(hLayout);
    GlobalFree(hLayout);

    //
    //  Make sure the event is clear.
    //
    if (g_hEvent)
    {
        SetEvent(g_hEvent);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_UpdateActiveLocales
//
//  Updates the active locales.
//
////////////////////////////////////////////////////////////////////////////

BOOL Locale_UpdateActiveLocales(
    HWND hwnd)
{
    HKL *pLangs;
    UINT nLangs, ctr1, ctr2, ctr3;
    UINT id, iOldLayout;
    HWND hwndList = GetDlgItem(hwnd, IDC_KBDL_LOCALE_LIST);
    HKL hklSystem;
    int idxListBox, idxCurSel = -1;
    DWORD langLay;
    BOOL bApply;
    int iOldCount;
    LPLANGNODE pDefault = NULL;
    LPLANGNODE pLangNode, pLangNodeSel, pTemp;

    //
    //  See if the pane is disabled.  If so, then there is nothing to
    //  update.
    //
    if (!IsWindowEnabled(hwndList))
    {
        return (TRUE);
    }

    //
    //  See if we need to reload the init information.
    //
    if (g_hEvent && (WaitForSingleObject(g_hEvent, 0) != WAIT_OBJECT_0))
    {
        //
        // Commit any changes made to the keyboard list before 
        // wiping out the g_lpLayout list and reloading it
        // from the registry. Language group installation/un-installation
        // will trigger this code, and the following line will prevent wiping
        // out any user selected keyboards.
        //
        Locale_ApplyInputs(hwnd);

        Locale_ReloadInfo(hwnd);
    }

    //
    //  Get the active keyboard layout list from the system.
    //
    if (!SystemParametersInfo( SPI_GETDEFAULTINPUTLANG,
                               0,
                               (LPVOID)((LPDWORD)&hklSystem),
                               0 ))
    {
        hklSystem = GetKeyboardLayout(0);
    }
    nLangs = GetKeyboardLayoutList(0, NULL);
    if (nLangs == 0)
    {
        Locale_EnablePane(hwnd, FALSE, IDC_KBDL_DISABLED);
        return (FALSE);
    }
    pLangs = (HKL *)LocalAlloc(LPTR, sizeof(DWORD) * nLangs);
    GetKeyboardLayoutList(nLangs, (HKL *)pLangs);

    //
    //  Save the old state of the list box and then clear it out.
    //
    iOldCount = ListBox_GetCount(hwndList);
    pLangNodeSel = (LPLANGNODE)ListBox_GetItemData( hwndList,
                                                    ListBox_GetCurSel(hwndList) );
    ListBox_ResetContent(hwndList);

    //
    //  Mark all of the Original & Active entries with the LANG_UPDATE
    //  value so that these can be added to the list if they were deleted
    //  from the original list by another process.
    //
    for (ctr1 = 0; ctr1 < g_iLangBuff; ctr1++)
    {
        pLangNode = g_lpLang[ctr1].pNext;
        while (pLangNode)
        {
            if ((pLangNode->wStatus & LANG_ORIGACTIVE) &&
                (pLangNode->wStatus & LANG_ACTIVE))
            {
                pLangNode->wStatus |= LANG_UPDATE;
            }

            if ((pLangNode->wStatus & LANG_DEFAULT) &&
                (pLangNode->wStatus & LANG_DEF_CHANGE))
            {
                pDefault = pLangNode;
            }

            pLangNode = pLangNode->pNext;
        }
    }

    //
    //  Get the active keyboard information and put it in the internal
    //  language structure.
    //
    for (ctr2 = 0; ctr2 < nLangs; ctr2++)
    {
        for (ctr1 = 0; ctr1 < g_iLangBuff; ctr1++)
        {
            //
            //  See if there's a match.
            //
            if (LOWORD(pLangs[ctr2]) == LOWORD(g_lpLang[ctr1].dwID))
            {
                //
                //  Found a match.
                //  Find the correct entry for the hkl.
                //
                pLangNode = g_lpLang[ctr1].pNext;
                while (pLangNode)
                {
                    if (pLangNode->hkl == pLangs[ctr2])
                    {
                        break;
                    }
                    pLangNode = pLangNode->pNext;
                }
                if (pLangNode == NULL)
                {
                    pLangNode = Locale_AddToLinkedList(ctr1, pLangs[ctr2]);
                    if (!pLangNode)
                    {
                        continue;
                    }
                }

                //
                //  Make sure it wasn't one that was removed by the user
                //  before the Apply button was hit.
                //
                if ((pLangNode->wStatus & LANG_ORIGACTIVE) &&
                    (!(pLangNode->wStatus & LANG_ACTIVE)))
                {
                    if ((pLangNode->hkl == hklSystem) &&
                        (!pDefault || (pLangNode == pDefault)))
                    {
                        //
                        //  Override the user's removal if it is now the
                        //  system default.
                        //
                        pLangNode->wStatus |= LANG_ACTIVE;
                        pLangNode->wStatus &= ~LANG_CHANGED;

                        g_lpLang[ctr1].iNumCount++;
                    }
                    else
                    {
                        //
                        //  Want to break out of the inner loop so that this
                        //  one won't be added to the list.
                        //
                        break;
                    }
                }

                //
                //  Add the item data to the list box, mark the
                //  language as original and active, save the pointer
                //  to the match in the layout list, and get the
                //  2 letter indicator symbol.
                //
                idxListBox = ListBox_AddItemData(hwndList, pLangNode);
                if ((HIWORD(pLangs[ctr2]) & 0xf000) == 0xe000)
                {
                    pLangNode->wStatus |= LANG_IME;
                }
                pLangNode->wStatus |= (LANG_ORIGACTIVE | LANG_ACTIVE);
                pLangNode->wStatus &= ~LANG_UPDATE;
                pLangNode->hkl = pLangs[ctr2];
                pLangNode->hklUnload = pLangs[ctr2];
                Locale_FetchIndicator(pLangNode);

                //
                //  Save the iLayout value to see if it's changed.
                //
                iOldLayout = pLangNode->iLayout;

                //
                //  Match the language to the layout.
                //
                pLangNode->iLayout = 0;
                langLay = (DWORD)HIWORD(pLangs[ctr2]);

                if ((HIWORD(pLangs[ctr2]) == 0xffff) ||
                    (HIWORD(pLangs[ctr2]) == 0xfffe))
                {
                    //
                    //  Mark default or previous error as US - this
                    //  means that the layout will be that of the basic
                    //  keyboard driver (the US one).
                    //
                    pLangNode->wStatus |= LANG_CHANGED;
                    pLangNode->iLayout = g_iUsLayout;
                    langLay = 0;
                }
                else if ((HIWORD(pLangs[ctr2]) & 0xf000) == 0xf000)
                {
                    //
                    //  Layout is special, need to search for the ID
                    //  number.
                    //
                    id = HIWORD(pLangs[ctr2]) & 0x0fff;
                    for (ctr3 = 0; ctr3 < g_iLayoutBuff; ctr3++)
                    {
                        if (id == g_lpLayout[ctr3].iSpecialID)
                        {
                            pLangNode->iLayout = ctr3;
                            langLay = 0;
                            break;
                        }
                    }
                    if (langLay)
                    {
                        //
                        //  Didn't find the id, so reset to basic for
                        //  the language.
                        //
                        langLay = (DWORD)LOWORD(pLangs[ctr2]);
                    }
                }

                if (langLay)
                {
                    //
                    //  Search for the id.
                    //
                    for (ctr3 = 0; ctr3 < g_iLayoutBuff; ctr3++)
                    {
                        if (((LOWORD(langLay) & 0xf000) == 0xe000) &&
                            (g_lpLayout[ctr3].dwID) == (DWORD_PTR)(pLangs[ctr2]))
                        {
                            pLangNode->iLayout = ctr3;
                            break;
                        }
                        else
                        {
                            if (langLay == (DWORD)LOWORD(g_lpLayout[ctr3].dwID))
                            {
                                pLangNode->iLayout = ctr3;
                                break;
                            }
                        }
                    }

                    if (ctr3 == g_iLayoutBuff)
                    {
                        //
                        //  Something went wrong or didn't load from
                        //  the registry correctly.
                        //
                        MessageBeep(MB_ICONEXCLAMATION);
                        pLangNode->wStatus |= LANG_CHANGED;
                        pLangNode->iLayout = g_iUsLayout;
                    }
                }

                //
                //  See if the user changed the layout.
                //
                if ((pLangNode->wStatus & LANG_OAC) == LANG_OAC)
                {
                    if ((pLangNode->iLayout == iOldLayout) ||
                        ((pLangNode->hkl == hklSystem) &&
                         (!(pLangNode->wStatus & LANG_DEFAULT))))
                    {
                        pLangNode->wStatus &= ~LANG_CHANGED;
                    }
                    else
                    {
                        pLangNode->iLayout = iOldLayout;
                    }
                }

                //
                //  If this is the current language, then it's the default
                //  one.
                //
                if ((pLangNode == pDefault) ||
                    ((pLangNode->hkl == hklSystem) && !pDefault))
                {
                    TCHAR sz[DESC_MAX];

                    //
                    //  Found the default.  Set the Default input locale
                    //  text in the property sheet.
                    //
                    if (pLangNode->wStatus & LANG_IME)
                    {
                        GetAtomName( g_lpLayout[pLangNode->iLayout].atmLayoutText,
                                     sz,
                                     DESC_MAX );
                    }
                    else
                    {
                        GetAtomName(g_lpLang[ctr1].atmLanguageName, sz, DESC_MAX);
                    }
                    pLangNode->wStatus |= LANG_DEFAULT;
                    if (pLangNode->hkl == hklSystem)
                    {
                        pLangNode->wStatus &= ~LANG_DEF_CHANGE;
                        g_dwChanges &= ~CHANGE_DEFAULT;
                    }
                    pDefault = pLangNode;

                    //
                    //  Set selection to default if nothing is selected.
                    //
                    if (idxCurSel == -1)
                    {
                        idxCurSel = idxListBox;
                    }
                }

                //
                //  See if this is the previously selected item.
                //
                if (pLangNode == pLangNodeSel)
                {
                    idxCurSel = idxListBox;
                }

                //
                //  Break out of inner loop - we've found it.
                //
                break;
            }
        }
    }

    //
    //  Need to see if any items are marked to be updated, if any were
    //  added to the list before the Apply button was hit, and if
    //  the default has changed.
    //
    bApply = FALSE;
    for (ctr1 = 0; ctr1 < g_iLangBuff; ctr1++)
    {
        g_lpLang[ctr1].iNumCount = 0;
        pLangNode = g_lpLang[ctr1].pNext;
        while (pLangNode)
        {
            //
            //  See if this item is an update item.
            //
            if (pLangNode->wStatus & LANG_UPDATE)
            {
                pLangNode->wStatus = 0;
            }

            //
            //  See if this item needs to be added to the list box.
            //
            else if ((pLangNode->wStatus & LANG_ACTIVE) &&
                     (!(pLangNode->wStatus & LANG_ORIGACTIVE)))
            {
                //
                //  In this case, the Apply button should already be enabled.
                //
                ListBox_AddItemData(hwndList, pLangNode);
                Locale_FetchIndicator(pLangNode);
            }

            //
            //  See if the default has changed.
            //
            if ((pLangNode->wStatus & LANG_DEFAULT) &&
                (pDefault) && (pLangNode != pDefault))
            {
                pLangNode->wStatus &= ~LANG_DEFAULT;
            }

            //
            //  See if the Apply button should be enabled or disabled.
            //
            if (pLangNode->wStatus & LANG_CHANGED)
            {
                bApply = TRUE;
            }

            //
            //  Advance the pointer.
            //
            if (pLangNode->wStatus == 0)
            {
                //
                //  Remove the node - it's no longer needed.
                //
                pTemp = pLangNode;
                pLangNode = pLangNode->pNext;
                Locale_RemoveFromLinkedList(pTemp);
            }
            else
            {
                //
                //  Increment the active count.
                //
                if (pLangNode->wStatus & LANG_ACTIVE)
                {
                    (g_lpLang[ctr1].iNumCount)++;
                }
                pLangNode = pLangNode->pNext;
            }
        }
    }

    //
    //  See if the user specifically changed the "Switch locales" choice
    //  or the "Enable indicator on taskbar" check box, or toggled
    //  CapsLock/ShiftLock.
    //
    if (g_dwChanges != 0)
    {
        bApply = TRUE;
    }

    //
    //  Enable or Disable the Apply button.
    //
    SendMessage( GetParent(hwnd),
                 bApply ? PSM_CHANGED : PSM_UNCHANGED,
                 (WPARAM)hwnd,
                 0L );

    //
    //  Make sure the IME setting is correct.
    //
    ListBox_SetCurSel(hwndList, (idxCurSel == -1) ? 0 : idxCurSel);
    Locale_EnableIMESetting(hwnd);

    //
    //  See if we need to enable the secondary controls.
    //
    if ((iOldCount < 2) && (ListBox_GetCount(hwndList) > 1))
    {
        //
        //  Set the appropriate hot key.
        //  Need to call this before Locale_SetSecondaryControls because
        //  whether or not to enable the change key sequence button depends
        //  on how many hotkeys there are in the list.
        //
        Locale_GetHotkeys(hwnd);

        //
        //  Enable the secondary controls.
        //
        Locale_SetSecondaryControls(hwnd, TRUE);

        //
        //  See if the taskbar indicator should be on or off.
        //
        CheckDlgButton( hwnd,
                        IDC_KBDL_INDICATOR,
                        Locale_IsIndicatorPresent() );
    }

    //
    //  Return success.
    //
    LocalFree((HANDLE)pLangs);
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  LocaleDlgProc
//
//  This is the dialog proc for the Input Locales property sheet.
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK LocaleDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
        case ( WM_DESTROY ) :
        {
            Locale_KillPaneDialog(hDlg);
            break;
        }
        case ( WM_INITDIALOG ) :
        {
            Locale_InitPropSheet(hDlg, (LPPROPSHEETPAGE)lParam);
            break;
        }
        case ( WM_MEASUREITEM ) :
        {
            Locale_MeasureItem(hDlg, (LPMEASUREITEMSTRUCT)lParam);
            break;
        }
        case ( WM_DRAWITEM ) :
        {
            return (Locale_DrawItem(hDlg, (LPDRAWITEMSTRUCT)lParam));
        }
        case ( WM_ACTIVATE ) :
        {
            if (IsWindowEnabled(GetDlgItem(hDlg, IDC_KBDL_DISABLED_2)))
            {
                Locale_InitPropSheet(hDlg, NULL);
            }
            else
            {
                Locale_UpdateActiveLocales(hDlg);
            }

            //
            //  Let the Window Management code also process
            //  the message so it will restore focus to the
            //  correct control.
            //
            return (FALSE);
        }
        case ( WM_NOTIFY ) :
        {
            switch (((NMHDR *)lParam)->code)
            {
                case ( PSN_SETACTIVE ) :
                {
                    if (IsWindowEnabled(GetDlgItem(hDlg, IDC_KBDL_DISABLED_2)))
                    {
                        Locale_InitPropSheet(hDlg, NULL);
                    }
                    else
                    {
                        Locale_UpdateActiveLocales(hDlg);
                    }
                    break;
                }
                case ( PSN_APPLY ) :
                {
                    Locale_ApplyInputs(hDlg);
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        case ( PSM_QUERYSIBLINGS ) :
        {
            Locale_UpdateActiveLocales(hDlg);
            break;
        }
        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
                case ( IDC_KBDL_INDICATOR ) :
                {
                    //
                    //  Care about these for "ApplyNow" only.
                    //
                    g_dwChanges |= CHANGE_SWITCH;
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                    break;
                }
                case ( IDC_KBDL_LOCALE_LIST ) :
                {
                    Locale_CommandLocaleList(hDlg, wParam, lParam);
                    break;
                }
                case ( IDC_KBDL_ADD ) :
                {
                    Locale_CommandAdd(hDlg, NULL);
                    break;
                }
                case ( IDC_KBDL_EDIT ) :
                {
                    Locale_CommandEdit(hDlg, NULL);
                    break;
                }
                case ( IDC_KBDL_DELETE ) :
                {
                    Locale_CommandDelete(hDlg);
                    break;
                }
                case ( IDC_KBDL_SET_DEFAULT ) :
                {
                    Locale_CommandSetDefault(hDlg);
                    break;
                }
                case ( IDC_KBDL_IME_SETTINGS ) :
                {
                    Locale_CommandConfigIME(hDlg);
                    break;
                }
                case ( IDC_KBDL_HOTKEY_LIST ):
                {
                    if (HIWORD(wParam) == LBN_DBLCLK)
                    {
                        //
                        //  User double clicked on a hotkey.  Invoke the
                        //  change hotkey dialog.
                        //
                        Locale_CommandChangeHotKey(hDlg);
                    }
                    break;
                }
                case ( IDC_KBDL_CHANGE_HOTKEY ) :
                {
                    Locale_CommandChangeHotKey(hDlg);
                    break;
                }
                case ( IDC_KBDL_CAPSLOCK ) :
                case ( IDC_KBDL_SHIFTLOCK ) :
                {
                    if (LOWORD(wParam) == IDC_KBDL_SHIFTLOCK)
                    {
                        g_dwAttributes |= KLF_SHIFTLOCK;
                    }
                    else
                    {
                        g_dwAttributes &= ~KLF_SHIFTLOCK;
                    }
                    g_dwChanges |= CHANGE_CAPSLOCK;
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                    break;
                }
                case ( IDOK ) :
                {
                    if (!Locale_ApplyInputs(hDlg))
                    {
                        break;
                    }

                    // fall thru...
                }
                case ( IDCANCEL ) :
                {
                    EndDialog(hDlg, TRUE);
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     NULL,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aLocaleHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :      // right mouse click
        {
            WinHelp( (HWND)wParam,
                     NULL,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aLocaleHelpIds );
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_GetLayoutList
//
//  Fills in the given listbox with the appropriate list of layouts.
//
////////////////////////////////////////////////////////////////////////////

void Locale_GetLayoutList(
    HWND hwndLayout,
    UINT idxLang,
    UINT idxLayout)
{
    UINT ctr;
    UINT idx;
    int idxSel = -1;
    int idxSame = -1;
    int idxOther = -1;
    int idxBase = -1;
    int idxUSA = -1;              // last resort default
    TCHAR sz[DESC_MAX];
    LPLANGNODE pTemp;
    DWORD LangID = g_lpLang[idxLang].dwID;
    DWORD BaseLangID = (LOWORD(LangID) & 0xff) | 0x400;

    //
    //  Reset the contents of the combo box.
    //
    ComboBox_ResetContent(hwndLayout);

    //
    //  Search through all of the layouts.
    //
    for (ctr = 0; ctr < g_iLayoutBuff; ctr++)
    {
        //
        //  Filter out IME layout if it is not under native locale.
        //
        if (((HIWORD(g_lpLayout[ctr].dwID) & 0xf000) == 0xe000) &&
            (LOWORD(g_lpLayout[ctr].dwID) != LOWORD(LangID)))
        {
            continue;
        }

        //
        //  Make sure this layout isn't already used for this input locale.
        //  If it is, then don't display it in the properties dialog.
        //
        if (ctr != idxLayout)
        {
            pTemp = g_lpLang[idxLang].pNext;
            while (pTemp)
            {
                if (pTemp->wStatus & LANG_ACTIVE)
                {
                    if (ctr == pTemp->iLayout)
                    {
                        break;
                    }
                }
                pTemp = pTemp->pNext;
            }
            if (pTemp && (ctr == pTemp->iLayout))
            {
                continue;
            }
        }

        //
        //  Get the layout text.  If it doesn't already exist in the
        //  combo box, then add it to the list of possible layouts.
        //
        GetAtomName(g_lpLayout[ctr].atmLayoutText, sz, DESC_MAX);
        if ((idx = ComboBox_FindStringExact(hwndLayout, 0, sz)) == CB_ERR)
        {
            //
            //  Add the layout string and set the item data to be the
            //  index into the g_lpLayout array.
            //
            idx = ComboBox_AddString(hwndLayout, sz);
            ComboBox_SetItemData(hwndLayout, idx, MAKELONG(ctr, 0));

            //
            //  See if it's the US layout.  If so, save the index.
            //
            if (g_lpLayout[ctr].dwID == US_LOCALE)
            {
                idxUSA = ctr;
            }
        }

        if (idxLayout == -1)
        {
            //
            //  If the caller does not specify a layout, it must be the
            //  Add dialog.  First we want the default layout.  If the
            //  default layout is not an option (eg. it's already used),
            //  then we want any layout that has the same id as the locale
            //  to be the default.
            //
            if (idxSel == -1)
            {
                if (g_lpLayout[ctr].dwID == g_lpLang[idxLang].dwDefaultLayout)
                {
                    idxSel = ctr;
                }
                else if (idxSame == -1)
                {
                    if ((LOWORD(g_lpLayout[ctr].dwID) == LOWORD(LangID)) &&
                        (HIWORD(g_lpLayout[ctr].dwID) == 0))
                    {
                        idxSame = ctr;
                    }
                    else if (idxOther == -1)
                    {
                        if (LOWORD(g_lpLayout[ctr].dwID) == LOWORD(LangID))
                        {
                            idxOther = ctr;
                        }
                        else if ((idxBase == -1) &&
                                 (LOWORD(g_lpLayout[ctr].dwID) == LOWORD(BaseLangID)))
                        {
                            idxBase = ctr;
                        }
                    }
                }
            }
        }
        else if (ctr == idxLayout)
        {
            //
            //  For the properties dialog, we want the one ALREADY associated.
            //
            idxSel = ctr;
        }
    }

    //
    //  If it's the Add dialog, do some extra checking for the layout to use.
    //
    if (idxLayout == -1)
    {
        if (idxSel == -1)
        {
            idxSel = (idxSame != -1)
                         ? idxSame
                         : ((idxOther != -1) ? idxOther : idxBase);
        }
    }

    //
    //  If a default layout was not found, then set it to the US layout.
    //
    if (idxSel == -1)
    {
        idxSel = idxUSA;
    }

    //
    //  Set the current selection.
    //
    if (idxSel == -1)
    {
        //
        //  Simply set the current selection to be the first entry
        //  in the list.
        //
        ComboBox_SetCurSel(hwndLayout, 0);
    }
    else
    {
        //
        //  The combo box is sorted, but we need to know where
        //  g_lpLayout[idxSel] was stored.  So, get the atom again, and
        //  search the list.
        //
        GetAtomName(g_lpLayout[idxSel].atmLayoutText, sz, DESC_MAX);
        idx = ComboBox_FindStringExact(hwndLayout, 0, sz);
        ComboBox_SetCurSel(hwndLayout, idx);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_AddDlgInit
//
//  Processing for a WM_INITDIALOG message for the Add dialog box.
//
////////////////////////////////////////////////////////////////////////////

void Locale_AddDlgInit(
    HWND hwnd,
    LPARAM lParam)
{
    UINT ctr1, SelectedLang = -1;
    HWND hwndLang = GetDlgItem(hwnd, IDC_KBDLA_LOCALE);
    UINT idx;
    TCHAR sz[DESC_MAX];
    HWND hwndList = GetDlgItem((HWND)((INITINFO *)lParam)->hwndMain, IDC_KBDL_LOCALE_LIST);
    LPLANGNODE pLangNode = (LPLANGNODE)(((INITINFO *)lParam)->pLangNode);
    int  nLocales, idxList, IMELayoutExist = 0;
    UINT ctr2, ListCount, DefaultIdx = 0;
    LRESULT LCSelectData = (LONG)-1;
    nLocales = ListBox_GetCount(hwndList);

    //
    //  Get the currently chosen input locale in the parent dialog's
    //  list box.
    //
    if (pLangNode)
    {
        SelectedLang = pLangNode->iLang;
    }

    //
    //  Go through all of the input locales.  Display all of them,
    //  since we can have multiple layouts per locale.
    //
    //  Do NOT go down the links in this case.  We don't want to display
    //  the language choice multiple times.
    //
    for (ctr1 = 0; ctr1 < g_iLangBuff; ctr1++)
    {
        //
        //  If the language does not contain an IME layout, then
        //  compare with layout counts without IME.
        //
        for (ctr2 = 0; ctr2 < g_iLayoutBuff; ctr2++)
        {
            if ((LOWORD(g_lpLayout[ctr2].dwID) == LOWORD(g_lpLang[ctr1].dwID)) &&
                ((HIWORD(g_lpLayout[ctr2].dwID) & 0xf000) == 0xe000))
            {
                IMELayoutExist = 1;
                break;
            }
        }
        if ((!IMELayoutExist) &&
            (g_lpLang[ctr1].iNumCount == (g_iLayoutBuff - g_iLayoutIME)))
        {
            //
            //  No more layouts to be added for this language.
            //
            continue;
        }

        //
        //  Make sure there are layouts to be added for this
        //  input locale.
        //
        if (g_lpLang[ctr1].iNumCount != g_iLayoutBuff)
        {
            //
            //  Get the language name, add the string to the
            //  combo box, and set the index into the g_lpLang
            //  array as the item data.
            //
            GetAtomName(g_lpLang[ctr1].atmLanguageName, sz, DESC_MAX);
            idx = ComboBox_AddString(hwndLang, sz);
            ComboBox_SetItemData(hwndLang, idx, MAKELONG(ctr1, 0));

            //
            //  Save chosen input locale and system default locale.
            //
            if ((LCSelectData == -1) || (ctr1 == SelectedLang))
            {
                if ((ctr1 == SelectedLang) ||
                    (g_lpLang[ctr1].dwID == GetSystemDefaultLCID()))
                {
                    LCSelectData = MAKELONG(ctr1, 0);
                }
            }
        }
    }

    //
    //  Set the current selection to the currently chosen input locale
    //  or the default system locale entry.
    //
    if (LCSelectData != -1)
    {
        ListCount = ComboBox_GetCount(hwndLang);
        for (ctr1 = 0; ctr1 < ListCount; ctr1++)
        {
            if (LCSelectData == ComboBox_GetItemData(hwndLang, ctr1))
            {
                DefaultIdx = ctr1;
                break;
            }
        }
    }
    ComboBox_SetCurSel(hwndLang, DefaultIdx);
    idx = (UINT)ComboBox_GetItemData(hwndLang, DefaultIdx);

    SetProp(hwnd, szPropHwnd, (HANDLE)((LPINITINFO)lParam)->hwndMain);
    SetProp(hwnd, szPropIdx, (HANDLE)idx);

    //
    //  Display the keyboard layout.
    //
    Locale_GetLayoutList(GetDlgItem(hwnd, IDC_KBDLA_LAYOUT), idx, -1);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_AddCommandOK
//
//  Gets the currently selected input locale from the combo box and marks
//  it as active in the g_lpLang list.  It then gets the requested layout
//  and sets that in the list.  It then adds the new input locale string
//  to the input locale list in the property sheet.
//
////////////////////////////////////////////////////////////////////////////

int Locale_AddCommandOK(
    HWND hwnd)
{
    HWND hwndLang = GetDlgItem(hwnd, IDC_KBDLA_LOCALE);
    HWND hwndLayout = GetDlgItem(hwnd, IDC_KBDLA_LAYOUT);
    int idxLang = ComboBox_GetCurSel(hwndLang);
    int idxLayout = ComboBox_GetCurSel(hwndLayout);
    LPLANGNODE pLangNode;

    //
    //  Get the offset for the language to add.
    //
    idxLang = (int)ComboBox_GetItemData(hwndLang, idxLang);

    //
    //  Insert a new language node.
    //
    pLangNode = Locale_AddToLinkedList(idxLang, 0);
    if (!pLangNode)
    {
        return (0);
    }

    //
    //  Get the offset for the chosen layout.
    //
    pLangNode->iLayout = (UINT)ComboBox_GetItemData(hwndLayout, idxLayout);

    //
    //  See if the layout is an IME and mark the status bits accordingly.
    //
    if ((HIWORD(g_lpLayout[pLangNode->iLayout].dwID) & 0xf000) == 0xe000)
    {
        pLangNode->wStatus |= LANG_IME;
    }
    else
    {
        pLangNode->wStatus &= ~LANG_IME;
    }

    //
    //  Add the new language.
    //
    if (!Locale_AddLanguage(GetProp(hwnd, szPropHwnd), pLangNode))
    {
        //
        //  Unable to add the language.  Need to return the user back
        //  to the Add dialog.
        //
        Locale_RemoveFromLinkedList(pLangNode);
        return (0);
    }

    //
    //  Return success.
    //
    return (1);
}


////////////////////////////////////////////////////////////////////////////
//
//  KbdLocaleAddDlg
//
//  This is the dialog proc for the Add button of the Input Locales
//  property sheet.
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK KbdLocaleAddDlg(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            Locale_AddDlgInit(hwnd, lParam);
            break;
        }
        case ( WM_DESTROY ) :
        {
            RemoveProp(hwnd, szPropHwnd);
            RemoveProp(hwnd, szPropIdx);
            break;
        }
        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
                case ( IDOK ) :
                {
                    if (!Locale_AddCommandOK(hwnd))
                    {
                        //
                        //  This means the properties dialog was cancelled.
                        //  The Add dialog should remain active.
                        //
                        break;
                    }

                    // fall thru...
                }
                case ( IDCANCEL ) :
                {
                    EndDialog(hwnd, (wParam == IDOK) ? 1 : 0);
                    break;
                }
                case ( IDC_KBDLA_LOCALE ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        HWND hwndLocale = (HWND)lParam;
                        HWND hwndLayout = GetDlgItem(hwnd, IDC_KBDLA_LAYOUT);
                        int idx;

                        //
                        //  Update the keyboard layout options.
                        //
                        if ((idx = ComboBox_GetCurSel(hwndLocale)) != CB_ERR)
                        {
                            idx = (int)ComboBox_GetItemData(hwndLocale, idx);
                            Locale_GetLayoutList(hwndLayout, idx, -1);
                        }
                    }
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     NULL,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aLocaleAddHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :      // right mouse click
        {
            WinHelp( (HWND)wParam,
                     NULL,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aLocaleAddHelpIds );
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_EditDlgInit
//
//  Processing for a WM_INITDIALOG message for the Edit dialog box.
//
////////////////////////////////////////////////////////////////////////////

void Locale_EditDlgInit(
    HWND hwnd,
    LPARAM lParam)
{
    TCHAR sz[DESC_MAX];
    HWND hwndMain = ((LPINITINFO)lParam)->hwndMain;
    LPLANGNODE pLangNode;

    //
    //  Get the language name for the currently selected input locale
    //  and display it in the dialog.
    //
    pLangNode = ((LPINITINFO)lParam)->pLangNode;
    GetAtomName(g_lpLang[pLangNode->iLang].atmLanguageName, sz, DESC_MAX);
    SetDlgItemText(hwnd, IDC_KBDLE_LOCALE, sz);

    //
    //  Display the list of layouts.
    //
    Locale_GetLayoutList( GetDlgItem(hwnd, IDC_KBDLE_LAYOUT),
                          pLangNode->iLang,
                          pLangNode->iLayout );

    //
    //  Save the info.
    //
    SetProp(hwnd, szPropHwnd, (HANDLE)hwndMain);
    SetProp(hwnd, szPropIdx, (HANDLE)pLangNode);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_EditCommandOK
//
//  Gets the new keyboard layout selection.  If it's different from the
//  current one for that locale, then it updates the g_lpLang array and
//  redraws the input locale combo box in the main property sheet.
//
////////////////////////////////////////////////////////////////////////////

void Locale_EditCommandOK(
    HWND hwnd)
{
    UINT idx;
    UINT idxLay;
    LPLANGNODE pLangNode;
    HWND hwndLay = GetDlgItem(hwnd, IDC_KBDLE_LAYOUT);
    HWND hwndMain;

    //
    //  Get the currently selected layout from the combo box.
    //
    idx = (UINT)ComboBox_GetCurSel(hwndLay);
    idxLay = (UINT)ComboBox_GetItemData(hwndLay, idx);
    pLangNode = (LPLANGNODE)GetProp(hwnd, szPropIdx);

    //
    //  See if the selected layout has changed.
    //
    if ((pLangNode->iLayout == (UINT)(-1)) || (pLangNode->iLayout != idxLay))
    {
        //
        //  Reset the lang node to contain the new layout and show
        //  that there is a change.
        //
        pLangNode->iLayout = idxLay;
        pLangNode->wStatus |= LANG_CHANGED;

        //
        //  See if the new layout is an IME and set the status bits
        //  accordingly.
        //
        if ((HIWORD(g_lpLayout[pLangNode->iLayout].dwID) & 0xf000) == 0xe000)
        {
            pLangNode->wStatus |= LANG_IME;
        }
        else
        {
            pLangNode->wStatus &= ~LANG_IME;
        }

        //
        //  Redraw the input locale list in the property sheet.
        //
        hwndMain = GetProp(hwnd, szPropHwnd);
        InvalidateRect(GetDlgItem(hwndMain, IDC_KBDL_LOCALE_LIST), NULL, FALSE);

        //
        //  Change the hotkey list.
        //
        Locale_GetHotkeys(hwndMain);

        //
        //  Enable/disable the IME setting button if the layout is changed
        //  to/from an IME.
        //
        Locale_EnableIMESetting(hwndMain);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  KbdLocaleEditDlg
//
//  This is the dialog proc for the Properties button of the Input Locales
//  property sheet.
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK KbdLocaleEditDlg(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            Locale_EditDlgInit(hwnd, lParam);
            break;
        }
        case ( WM_DESTROY ) :
        {
            RemoveProp(hwnd, szPropHwnd);
            RemoveProp(hwnd, szPropIdx);
            break;
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     NULL,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aLocalePropHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :
        {
            WinHelp( (HWND)wParam,
                     NULL,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aLocalePropHelpIds );
            break;
        }
        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
                case ( IDOK ) :
                {
                    Locale_EditCommandOK(hwnd);

                    // fall thru...
                }
                case ( IDCANCEL ) :
                {
                    EndDialog(hwnd, (wParam == IDOK) ? 1 : 0);
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_VirtKeyList
//
//  Initializes the virtual key combo box and sets the current selection.
//
////////////////////////////////////////////////////////////////////////////

void Locale_VirtKeyList(
    HWND hwnd,
    UINT uVKey,
    BOOL bDirectSwitch)
{
    UINT  ctr, iStart, iEnd, iIndex;
    UINT  iSel = 0;
    TCHAR szString[DESC_MAX];
    HWND  hwndKey = GetDlgItem(hwnd, IDC_KBDLH_KEY_COMBO);

    //
    //  Look for hot keys for direct switch.
    //
    for (ctr = sizeof(g_aVirtKeyDesc) / sizeof(VIRTKEYDESC) - 1;
         ctr >= 0;
         ctr--)
    {
        if (g_aVirtKeyDesc[ctr].idVirtKeyName == IDS_VK_NONE1)
        {
            //
            //  Found it.  Remove "(None)" from hwndKey list box.
            //
            ctr++;
            break;
        }
    }

    iStart = bDirectSwitch ? ctr : 0;
    iEnd = bDirectSwitch ? sizeof(g_aVirtKeyDesc) / sizeof(VIRTKEYDESC) : ctr;

    ComboBox_ResetContent(hwndKey);

    for (ctr = iStart; ctr < iEnd; ctr++)
    {
        GetAtomName( g_aVirtKeyDesc[ctr].atVirtKeyName,
                     szString,
                     sizeof(szString) / sizeof(TCHAR) );

        iIndex = ComboBox_InsertString(hwndKey, -1, szString);
        ComboBox_SetItemData(hwndKey, iIndex, g_aVirtKeyDesc[ctr].uVirtKeyValue);
        if (g_aVirtKeyDesc[ctr].uVirtKeyValue == uVKey)
        {
            iSel = iIndex;
        }
    }
    ComboBox_SetCurSel(hwndKey, iSel);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_ChangeHotKeyDlgInit
//
//  Initializes the change hotkey dialog box.
//
////////////////////////////////////////////////////////////////////////////

void Locale_ChangeHotKeyDlgInit(
    HWND hwnd,
    LPARAM lParam)
{
    TCHAR szHotKeyName[DESC_MAX];
    LPHOTKEYINFO pHotKeyNode = ((LPINITINFO)lParam)->pHotKeyNode;
    BOOL bCtrl = TRUE;
    BOOL bAlt = TRUE;
    BOOL bGrave = TRUE;

    GetAtomName(pHotKeyNode->atmHotKeyName, szHotKeyName, DESC_MAX);
    SetDlgItemText(hwnd, IDC_KBDLH_LAYOUT_TEXT, szHotKeyName);

    if (pHotKeyNode->uModifiers & MOD_CONTROL)
    {
        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, TRUE);
        bCtrl = FALSE;
    }

    if (pHotKeyNode->uModifiers & MOD_ALT)
    {
        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, TRUE);
        bAlt = FALSE;
    }

    if (GetDlgItem(hwnd, IDC_KBDLH_GRAVE) && (pHotKeyNode->uVKey == CHAR_GRAVE))
    {
        CheckDlgButton(hwnd, IDC_KBDLH_GRAVE, TRUE);
        bGrave = FALSE;
    }

    if (bCtrl && bAlt && bGrave)
    {
        CheckDlgButton(hwnd, IDC_KBDLH_ENABLE, FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_GRAVE), FALSE);
        ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_KBDLH_KEY_COMBO), 0);
        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_KEY_COMBO), FALSE);
    }
    else
    {
        CheckDlgButton(hwnd, IDC_KBDLH_ENABLE, TRUE);
    }

    if (IS_DIRECT_SWITCH_HOTKEY(pHotKeyNode->dwHotKeyID))
    {
        Locale_VirtKeyList(hwnd, pHotKeyNode->uVKey, TRUE);
    }
    else
    {
        Locale_VirtKeyList(hwnd, pHotKeyNode->uVKey, FALSE);
    }

    SetProp(hwnd, szPropHwnd, (HANDLE)((LPINITINFO)lParam)->hwndMain);
    SetProp(hwnd, szPropIdx, (HANDLE)pHotKeyNode);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_ChangeHotKeyCommandOK
//
//  Records hotkey changes made in change hotkey dialog box.
//  Warns if duplicate hotkeys are selected.
//
////////////////////////////////////////////////////////////////////////////

BOOL Locale_ChangeHotKeyCommandOK(
    HWND hwnd)
{
    LPHOTKEYINFO pHotKeyNode, pHotKeyTemp;
    UINT iIndex;
    HWND hwndHotkey, hwndMain, hwndKey;
    UINT uOldVKey, uOldModifiers;
    int ctr;
    int iNumMods = 0;
    int DialogType;

    pHotKeyNode = GetProp(hwnd, szPropIdx);
    hwndMain = GetProp(hwnd, szPropHwnd);
    hwndHotkey = GetDlgItem(hwndMain, IDC_KBDL_HOTKEY_LIST);
    hwndKey = GetDlgItem(hwnd, IDC_KBDLH_KEY_COMBO);

    uOldVKey = pHotKeyNode->uVKey;
    uOldModifiers = pHotKeyNode->uModifiers;

    if (pHotKeyNode->dwHotKeyID == HOTKEY_SWITCH_LANG)
    {
        DialogType = DIALOG_SWITCH_INPUT_LOCALES;
    }
    else if (GetDlgItem(hwnd, IDC_KBDLH_KEY_COMBO) &&
             (GetWindowLong( GetDlgItem(hwnd, IDC_KBDLH_KEY_COMBO),
                             GWL_STYLE) & BS_RADIOBUTTON))
    {
        DialogType = DIALOG_SWITCH_KEYBOARD_LAYOUT;
    }
    else
    {
        DialogType = DIALOG_SWITCH_IME;
    }

    pHotKeyNode->uModifiers &= ~(MOD_CONTROL | MOD_ALT | MOD_SHIFT);
    pHotKeyNode->uVKey = 0;
    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_ENABLE))
    {
        if (IsDlgButtonChecked(hwnd, IDC_KBDLH_CTRL))
        {
            pHotKeyNode->uModifiers |= MOD_CONTROL;
            iNumMods++;
        }

        if (IsDlgButtonChecked(hwnd, IDC_KBDLH_L_ALT))
        {
            pHotKeyNode->uModifiers |= MOD_ALT;
            iNumMods++;
        }

        if (!IsDlgButtonChecked(hwnd, IDC_KBDLH_GRAVE))
        {
            //
            //  Shift key is mandatory.
            //
            pHotKeyNode->uModifiers |= MOD_SHIFT;
            iNumMods++;
        }

        if (IsDlgButtonChecked(hwnd, IDC_KBDLH_GRAVE))
        {
            //
            //  Assign Grave key.
            //
            pHotKeyNode->uVKey = CHAR_GRAVE;
        }
        else if ((iIndex = ComboBox_GetCurSel(hwndKey)) == CB_ERR)
        {
            pHotKeyNode->uVKey = 0;
        }
        else
        {
            pHotKeyNode->uVKey = (UINT)ComboBox_GetItemData(hwndKey, iIndex);
        }
    }

    //
    //  Key sequence with only one modifier and without a key,
    //  or without any modifier is invalid.
    //
    if (((pHotKeyNode->uVKey != 0) && (iNumMods == 0) &&
         (DialogType != DIALOG_SWITCH_INPUT_LOCALES)) ||
        ((pHotKeyNode->uVKey == 0) && (iNumMods != 0) &&
         (DialogType != DIALOG_SWITCH_INPUT_LOCALES)))
    {
        TCHAR szName[DESC_MAX];

        Locale_TranslateHotKey( szName,
                                pHotKeyNode->uVKey,
                                pHotKeyNode->uModifiers );

        Locale_ErrorMsg(hwnd, IDS_KBD_INVALID_HOTKEY, szName);

        pHotKeyNode->uModifiers = uOldModifiers;
        pHotKeyNode->uVKey = uOldVKey;
        return (FALSE);
    }

    //
    //  Do not allow duplicate hot keys.
    //
    for (ctr = 0; ctr < ListBox_GetCount(hwndHotkey); ctr++)
    {
        pHotKeyTemp = (LPHOTKEYINFO)ListBox_GetItemData(hwndHotkey, ctr);
        if ((pHotKeyTemp != pHotKeyNode) &&
            ((pHotKeyNode->uModifiers & (MOD_CONTROL | MOD_ALT | MOD_SHIFT)) ==
             (pHotKeyTemp->uModifiers & (MOD_CONTROL | MOD_ALT | MOD_SHIFT))) &&
            (pHotKeyNode->uVKey == pHotKeyTemp->uVKey) &&
            (iNumMods || pHotKeyNode->uVKey != 0))
        {
            TCHAR szName[DESC_MAX];

            Locale_TranslateHotKey( szName,
                                    pHotKeyNode->uVKey,
                                    pHotKeyNode->uModifiers );
            Locale_ErrorMsg(hwnd, IDS_KBD_CONFLICT_HOTKEY, szName);

            pHotKeyNode->uModifiers = uOldModifiers;
            pHotKeyNode->uVKey = uOldVKey;
            return (FALSE);
        }
    }

    //
    //  Now look for a langnode to stash this in.  If the hotkey
    //  is the switchlang hotkey, we don't need to bother.
    //
    if (pHotKeyNode->dwHotKeyID != HOTKEY_SWITCH_LANG)
    {
        LPLANGNODE pLangNode;
        HWND hwndList = GetDlgItem(hwndMain, IDC_KBDL_LOCALE_LIST);

        if (pHotKeyNode->hkl == 0)
        {
            pLangNode = (LPLANGNODE)ListBox_GetItemData( hwndList,
                                                         pHotKeyNode->idxLayout );
            if ( (LPARAM)pLangNode == LB_ERR )
               pLangNode = NULL;
        }
        else
        {
            int iLangCount = ListBox_GetCount(hwndList);
            int ctr1;
            for (ctr1 = 0; ctr1 < iLangCount; ctr1++)
            {
                pLangNode = (LPLANGNODE)ListBox_GetItemData(hwndList, ctr1);

                if ( (LPARAM)pLangNode != LB_ERR )
                {
                   if (pLangNode->hkl == pHotKeyNode->hkl)
                   {
                      break;
                   }
                }
            }
            if (ctr1 == iLangCount)
            {
                pLangNode = NULL;
            }
        }
        if (pLangNode)
        {
            pLangNode->uModifiers = pHotKeyNode->uModifiers;
            pLangNode->uVKey = pHotKeyNode->uVKey;
            pLangNode->wStatus |= LANG_HOTKEY;
        }
    }

    InvalidateRect(hwndHotkey, NULL, FALSE);

    if ((uOldVKey != pHotKeyNode->uVKey) ||
        (uOldModifiers != pHotKeyNode->uModifiers))
    {
        g_dwChanges |= CHANGE_SWITCH;
    }

    if (g_dwChanges & CHANGE_SWITCH)
    {
        PropSheet_Changed(GetParent(hwndMain), hwndMain);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  KbdLocaleChangeInputLocaleHotkey
//
//  Dlgproc for changing input locale hotkey dialog box.
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK KbdLocaleChangeInputLocaleHotkey(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            Locale_ChangeHotKeyDlgInit(hwnd, lParam);
            break;
        }
        case ( WM_DESTROY ) :
        {
            RemoveProp(hwnd, szPropHwnd);
            RemoveProp(hwnd, szPropIdx);
            break;
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     NULL,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aLocaleHotkeyHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :
        {
            WinHelp( (HWND)wParam,
                     NULL,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aLocaleHotkeyHelpIds );
            break;
        }
        case ( WM_COMMAND ) :
        {
            LPHOTKEYINFO pHotKeyNode = GetProp(hwnd, szPropIdx);

            switch (LOWORD(wParam))
            {
                case ( IDOK ) :
                {
                    if (Locale_ChangeHotKeyCommandOK(hwnd))
                    {
                        EndDialog(hwnd, 1);
                    }
                    break;
                }
                case ( IDCANCEL ) :
                {
                    EndDialog(hwnd, 0);
                    break;
                }
                case ( IDC_KBDLH_ENABLE ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_ENABLE))
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_CHECKED);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL), TRUE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT), TRUE);
                    }
                    else
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_UNCHECKED);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT), FALSE);
                    }
                    break;
                }
                case ( IDC_KBDLH_CTRL ) :
                {
                    break;
                }
                case ( IDC_KBDLH_L_ALT ) :
                {
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  KbdLocaleChangeThaiInputLocaleHotkey
//
//  Dlgproc for changing Thai input locale hotkey dialog box.
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK KbdLocaleChangeThaiInputLocaleHotkey(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            Locale_ChangeHotKeyDlgInit(hwnd, lParam);
            break;
        }
        case ( WM_DESTROY ) :
        {
            RemoveProp(hwnd, szPropHwnd);
            RemoveProp(hwnd, szPropIdx);
            break;
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     NULL,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aLocaleHotkeyHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :
        {
            WinHelp( (HWND)wParam,
                     NULL,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aLocaleHotkeyHelpIds );
            break;
        }
        case ( WM_DRAWITEM ) :
        {
            if (wParam == IDC_KBDLH_VLINE)
            {
                LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
                RECT rect;
                HPEN hPenHilite, hPenShadow;

                GetClientRect(lpdis->hwndItem, &rect);
                hPenHilite = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNHIGHLIGHT));
                hPenShadow = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW));
                SelectObject(lpdis->hDC, hPenShadow);
                MoveToEx(lpdis->hDC, rect.right / 2, 0, NULL);
                LineTo(lpdis->hDC, rect.right / 2, rect.bottom);
                SelectObject(lpdis->hDC, hPenHilite);
                MoveToEx(lpdis->hDC, rect.right / 2 + 1, 0, NULL);
                LineTo(lpdis->hDC, rect.right / 2 + 1, rect.bottom);
                SelectObject(lpdis->hDC, GetStockObject(BLACK_PEN));
                DeleteObject(hPenHilite);
                DeleteObject(hPenShadow);

                return (TRUE);
            }
            return (FALSE);
            break;
        }
        case ( WM_COMMAND ) :
        {
            LPHOTKEYINFO pHotKeyNode = GetProp(hwnd, szPropIdx);

            switch (LOWORD(wParam))
            {
                case ( IDOK ) :
                {
                    if (Locale_ChangeHotKeyCommandOK(hwnd))
                    {
                        EndDialog(hwnd, 1);
                    }
                    break;
                }
                case ( IDCANCEL ) :
                {
                    EndDialog(hwnd, 0);
                    break;
                }
                case ( IDC_KBDLH_ENABLE ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_ENABLE))
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_GRAVE, BST_CHECKED);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL), TRUE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT), TRUE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_GRAVE), TRUE);
                    }
                    else
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_GRAVE, BST_UNCHECKED);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_GRAVE), FALSE);
                    }
                    break;
                }
                case ( IDC_KBDLH_CTRL ) :
                {
                    break;
                }
                case ( IDC_KBDLH_L_ALT ) :
                {
                    break;
                }
                case ( IDC_KBDLH_GRAVE ) :
                {
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  KbdLocaleChangeKeyboardLayoutHotkey
//
//  Dlgproc for changing direct switch keyboard layout hotkey dialog box.
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK KbdLocaleChangeKeyboardLayoutHotkey(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            Locale_ChangeHotKeyDlgInit(hwnd, lParam);
            break;
        }
        case ( WM_DESTROY ) :
        {
            RemoveProp(hwnd, szPropHwnd);
            RemoveProp(hwnd, szPropIdx);
            break;
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     NULL,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aLocaleHotkeyHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :
        {
            WinHelp( (HWND)wParam,
                     NULL,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aLocaleHotkeyHelpIds );
            break;
        }
        case ( WM_COMMAND ) :
        {
            HWND hwndKey = GetDlgItem(hwnd, IDC_KBDLH_KEY_COMBO);
            LPHOTKEYINFO pHotKeyNode = GetProp(hwnd, szPropIdx);

            switch (LOWORD(wParam))
            {
                case ( IDOK ) :
                {
                    if (Locale_ChangeHotKeyCommandOK(hwnd))
                    {
                        EndDialog(hwnd, 1);
                    }
                    break;
                }
                case ( IDCANCEL ) :
                {
                    EndDialog(hwnd, 0);
                    break;
                }
                case ( IDC_KBDLH_ENABLE ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_ENABLE))
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_CHECKED);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL), TRUE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT), TRUE);
                        ComboBox_SetCurSel(hwndKey, 0);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_KEY_COMBO), TRUE);
                    }
                    else
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_UNCHECKED);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT), FALSE);
                        ComboBox_SetCurSel(hwndKey, 0);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_KEY_COMBO), FALSE);
                    }
                    break;
                }
                case ( IDC_KBDLH_CTRL ) :
                {
                    break;
                }
                case ( IDC_KBDLH_L_ALT ) :
                {
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  KbdLocaleChangeImeHotkey
//
//  Dlgproc for changing direct switch IME hotkey dialog box.
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK KbdLocaleChangeImeHotkey(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            Locale_ChangeHotKeyDlgInit(hwnd, lParam);
            break;
        }
        case ( WM_DESTROY ) :
        {
            RemoveProp(hwnd, szPropHwnd);
            RemoveProp(hwnd, szPropIdx);
            break;
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     NULL,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aLocaleHotkeyHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :
        {
            WinHelp( (HWND)wParam,
                     NULL,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aLocaleHotkeyHelpIds );
            break;
        }
        case ( WM_COMMAND ) :
        {
            HWND hwndKey = GetDlgItem(hwnd, IDC_KBDLH_KEY_COMBO);
            LPHOTKEYINFO pHotKeyNode = GetProp(hwnd, szPropIdx);

            switch (LOWORD(wParam))
            {
                case ( IDOK ) :
                {
                    if (Locale_ChangeHotKeyCommandOK(hwnd))
                    {
                        EndDialog(hwnd, 1);
                    }
                    break;
                }
                case ( IDCANCEL ) :
                {
                    EndDialog(hwnd, 0);
                    break;
                }
                case ( IDC_KBDLH_ENABLE ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_ENABLE))
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_CHECKED);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL), TRUE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT), TRUE);
                        ComboBox_SetCurSel(hwndKey, 0);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_KEY_COMBO), TRUE);
                    }
                    else
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_UNCHECKED);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT), FALSE);
                        ComboBox_SetCurSel(hwndKey, 0);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_KEY_COMBO), FALSE);
                    }
                    break;
                }
                case ( IDC_KBDLH_CTRL ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_CTRL))
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_CHECKED);
                        if (!IsDlgButtonChecked(hwnd, IDC_KBDLH_L_ALT))
                        {
                            ComboBox_SetCurSel(hwndKey, 0);
                            EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_KEY_COMBO), FALSE);
                        }
                    }
                    else
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_CHECKED);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_KEY_COMBO), TRUE);
                    }
                    break;
                }
                case ( IDC_KBDLH_L_ALT ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_L_ALT))
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_CHECKED);
                        if (!IsDlgButtonChecked(hwnd, IDC_KBDLH_CTRL))
                        {
                            ComboBox_SetCurSel(hwndKey, 0);
                            EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_KEY_COMBO), FALSE);
                        }
                    }
                    else
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_CHECKED);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_KEY_COMBO), TRUE);
                    }
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}
