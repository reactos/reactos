// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  HOTKEY.CPP
//
//  This knows how to talk to COMCTL32's HOTKEY control.
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "client.h"
#include "hotkey.h"


#define NOTOOLBAR
#define NOUPDOWN
#define NOMENUHELP
#define NOTRACKBAR
#define NODRAGLIST
#define NOPROGRESS
#define NOSTATUSBAR
#define NOHEADER
#define NOLISTVIEW
#define NOTREEVIEW
#define NOTABCONTROL
#define NOANIMATE
#include <commctrl.h>




// --------------------------------------------------------------------------
//
//  CreateHotKeyClient()
//
//  Called by CreateClientObject() and Clone()
//
// --------------------------------------------------------------------------
HRESULT CreateHotKeyClient(HWND hwnd, long idChildCur, REFIID riid,
    void** ppvHotKey)
{
    CHotKey32* photkey;
    HRESULT    hr;

    InitPv(ppvHotKey);

    photkey = new CHotKey32(hwnd, idChildCur);
    if (!photkey)
        return(E_OUTOFMEMORY);

    hr = photkey->QueryInterface(riid, ppvHotKey);
    if (!SUCCEEDED(hr))
        delete photkey;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CHotKey32::CHotKey32()
//
// --------------------------------------------------------------------------
CHotKey32::CHotKey32(HWND hwnd, long idChildCur)
{
    Initialize(hwnd, idChildCur);
    m_fUseLabel = TRUE;
}



// --------------------------------------------------------------------------
//
//  CHotKey32::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CHotKey32::get_accRole(VARIANT varChild, VARIANT* pvarRole)
{
    InitPvar(pvarRole);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;
    pvarRole->lVal = ROLE_SYSTEM_HOTKEYFIELD;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CHotKey32::get_accValue()
//
//  The value of the hotkey field control is the currently typed-in contents.
//
// --------------------------------------------------------------------------
STDMETHODIMP CHotKey32::get_accValue(VARIANT varChild, BSTR* pszValue)
{
    DWORD   dwHotKey;
    long    lScan;
    LPTSTR  lpszMods;
    TCHAR   szModifiers[32];
    TCHAR   szKey[32];
    TCHAR   szResult[64];

    InitPv(pszValue);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    // Get the hotkey and turn into a string.
    dwHotKey = SendMessageINT(m_hwnd, HKM_GETHOTKEY, 0, 0);
    if (!dwHotKey)
        return(HrCreateString(STR_HOTKEY_NONE, pszValue));

    //
    // The HIBYTE of the LOWORD is the HOTKEYF_ flags.
    // The LOBYTE of the LOWORD is the VK_.
    //

    // Get the VK_ name.
    lScan = ((LONG)MapVirtualKey(LOBYTE(dwHotKey), 0) << 16);
    if (HIBYTE(dwHotKey) & HOTKEYF_EXT)
        lScan |= 0x01000000L;

    if (!GetKeyNameText(lScan, szKey, ARRAYSIZE(szKey)))
        return(S_FALSE);

    //
    // Make a string of the modifiers.  Do it in the order the shell does.
    // Namely, Ctrl + Shift + Alt + key.
    //
    lpszMods = szModifiers;

    if (HIBYTE(LOWORD(dwHotKey)) & HOTKEYF_CONTROL)
    {
        LoadString(hinstResDll, STR_CONTROL, szResult, ARRAYSIZE(szResult));
        lstrcpy(lpszMods, szResult);
        lpszMods += lstrlen(lpszMods);
        *lpszMods = '+';
        lpszMods++;
        *lpszMods = 0;
    }

    if (HIBYTE(LOWORD(dwHotKey)) & HOTKEYF_SHIFT)
    {
        LoadString(hinstResDll, STR_SHIFT, szResult, ARRAYSIZE(szResult));
        lstrcpy(lpszMods, szResult);
        lpszMods += lstrlen(lpszMods);
        *lpszMods = '+';
        lpszMods++;
        *lpszMods = 0;
    }

    if (HIBYTE(LOWORD(dwHotKey)) & HOTKEYF_ALT)
    {
        LoadString(hinstResDll, STR_ALT, szResult, ARRAYSIZE(szResult));
        lstrcpy(lpszMods, szResult);
        lpszMods += lstrlen(lpszMods);
        *lpszMods = '+';
        lpszMods++;
        *lpszMods = 0;
    }

    if (lpszMods == szModifiers)
        lstrcpy(szResult, szKey);
    else
    {
        lstrcpy(szResult, szModifiers);
        lpszMods = szResult + lstrlen(szResult);
        lstrcpy(lpszMods, szKey);
    }

    *pszValue = TCharSysAllocString(szResult);
    if (! *pszValue)
        return(E_OUTOFMEMORY);

    return(S_OK);
}

