/*----------------------------------------------------------------------------
/ Title;
/   misc.cpp
/
/ Authors;
/   David De Vorchik (daviddv)
/
/ Notes;
/   Misc functions
/----------------------------------------------------------------------------*/
#include "pch.h"
#include "atlbase.h"
#include <advpub.h>         // For REGINSTALL
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ GetKeyForCLSID
/ --------------
/   Given a reference to a CLSID open up the key that represents it.
/
/ In:
/   clsid = clsid reference
/   pSubKey -> name of sub key to be opened
/   phkey = receives the newly opened key
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
EXTERN_C HRESULT GetKeyForCLSID(REFCLSID clsid, LPCTSTR pSubKey, HKEY* phkey)
{
    HRESULT hr;
    TCHAR szBuffer[(MAX_PATH*2)+GUIDSTR_MAX];
    TCHAR szGuid[GUIDSTR_MAX];

    TraceEnter(TRACE_COMMON_MISC, "GetKeyForCLSID");
    TraceGUID("clsid", clsid);
    Trace(TEXT("pSubKey -%s-"), pSubKey ? pSubKey:TEXT("<none>"));

    TraceAssert(phkey);

    // - format the CLSID so we can find it in the registry
    // - then open it (the client is reponsible for closing it)

    *phkey = NULL;              // incase we fail

    if ( 0 == GetStringFromGUID(clsid, szGuid, ARRAYSIZE(szGuid)) )
        ExitGracefully(hr, E_FAIL, "Failed to convert GUID to string");

    wsprintf(szBuffer, TEXT("CLSID\\%s"), szGuid);

    if ( pSubKey )
    {
        StrCat(szBuffer, TEXT("\\"));
        StrCat(szBuffer, pSubKey);
    }

    Trace(TEXT("Trying to open -%s-"), szBuffer);

    if ( ERROR_SUCCESS != RegOpenKeyEx(HKEY_CLASSES_ROOT, szBuffer, NULL, KEY_READ|KEY_WRITE, phkey) )
        ExitGracefully(hr, E_FAIL, "Failed to open key");

    hr = S_OK;                  // success

exit_gracefully:

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ GetRealWindowInfo
/ -----------------
/   Get the window dimensions and client position.
/
/ In:
/   hwnd = window to enquire about
/   pRect -> receives the client position of the window / == NULL
/   pSize -> receives the size of the window  / == NULL
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
EXTERN_C HRESULT GetRealWindowInfo(HWND hwnd, LPRECT pRect, LPSIZE pSize)
{
    HRESULT hr;
    RECT rect;

    TraceEnter(TRACE_COMMON_MISC, "GetRealWindowInfo");

    if ( !GetWindowRect(hwnd, &rect) )
        ExitGracefully(hr, E_FAIL, "Failed to get window rectangles");

    MapWindowPoints(NULL, GetParent(hwnd), (LPPOINT)&rect, 2);
    
    if ( pRect )
        *pRect = rect;

    if ( pSize )
    {
        pSize->cx = rect.right - rect.left;
        pSize->cy = rect.bottom - rect.top;
    }

    hr = S_OK;

exit_gracefully:

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ OffsetWindow
/ ------------
/   Adjust the position of the given window by the given delta.  If the
/   delta is 0,0 then this is a NOP.
/
/ In:
/   hwnd = window to enquire about
/   dx, dy = offset to be applied to the window
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
EXTERN_C VOID OffsetWindow(HWND hwnd, INT dx, INT dy)
{
    RECT rect;

    TraceEnter(TRACE_COMMON_MISC, "OffsetWindow");

    if ( hwnd && (dx || dy) )
    {
        GetWindowRect(hwnd, &rect);
        MapWindowPoints(NULL, GetParent(hwnd), (LPPOINT)&rect, 2);
        SetWindowPos(hwnd, NULL, rect.left + dx, rect.top + dy, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
    }

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ CallRegInstall
/ --------------
/   Call ADVPACK for the given section of our resource based INF>
/
/ In:
/   hInstance = resource instance to get REGINST section from
/   szSection = section name to invoke
/
/ Out:
/   HRESULT:
/----------------------------------------------------------------------------*/
EXTERN_C HRESULT CallRegInstall(HINSTANCE hInstance, LPSTR szSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    TraceEnter(TRACE_COMMON_MISC, "CallRegInstall");

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");

#ifdef UNICODE
        if ( pfnri )
        {
            STRENTRY seReg[] = 
            {
                // These two NT-specific entries must be at the end
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
            };
            STRTABLE stReg = { ARRAYSIZE(seReg), seReg };
            
            hr = pfnri(hInstance, szSection, &stReg);
        }
#else
        if (pfnri)
        {
            hr = pfnri(hInstance, szSection, NULL);
        }

#endif
        FreeLibrary(hinstAdvPack);
    }

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ SetDefButton
/ ------------
/   Jump through hoops, avoid barking dogs and dice with death all to set
/   the default button in a dialog.
/
/ In:
/   hWnd, idButton = button to set
/
/ Out:
/   HRESULT:
/----------------------------------------------------------------------------*/
EXTERN_C VOID SetDefButton(HWND hwndDlg, int idButton)
{
    LRESULT lr;
    LONG style;

    TraceEnter(TRACE_COMMON_MISC, "SetDefButton");

    if (HIWORD(lr = SendMessage(hwndDlg, DM_GETDEFID, 0, 0)) == DC_HASDEFID)
    {
        HWND hwndOldDefButton = GetDlgItem(hwndDlg, LOWORD(lr));

        style = GetWindowLong(hwndOldDefButton, GWL_STYLE) & ~BS_DEFPUSHBUTTON;
        SendMessage (hwndOldDefButton,
                     BM_SETSTYLE,
                     MAKEWPARAM(style, 0),
                     MAKELPARAM(TRUE, 0));
    }

    SendMessage( hwndDlg, DM_SETDEFID, idButton, 0L );
    style = GetWindowLong(GetDlgItem(hwndDlg, idButton), GWL_STYLE)| BS_DEFPUSHBUTTON;
    SendMessage( GetDlgItem(hwndDlg, idButton),
                 BM_SETSTYLE,
                 MAKEWPARAM( style, 0 ),
                 MAKELPARAM( TRUE, 0 ));
    
    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ Data collection functions
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ AllocStorageMedium
/ ------------------
/   Allocate a storage medium (validating the clipboard format as required).
/
/ In:
/   pFmt, pMedium -> describe the allocation
/   cbStruct = size of allocation
/   ppAlloc -> receives a pointer to the allocation / = NULL
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
EXTERN_C HRESULT AllocStorageMedium(FORMATETC* pFmt, STGMEDIUM* pMedium, SIZE_T cbStruct, LPVOID* ppAlloc)
{
    HRESULT hr;

    TraceEnter(TRACE_COMMON_MISC, "AllocStorageMedium");

    TraceAssert(pFmt);
    TraceAssert(pMedium);

    // Validate parameters

    if ( ( cbStruct <= 0 ) || !( pFmt->tymed & TYMED_HGLOBAL ) )
        ExitGracefully(hr, E_INVALIDARG, "Zero size stored medium requested or non HGLOBAL");

    if ( ( pFmt->ptd ) || !( pFmt->dwAspect & DVASPECT_CONTENT) || !( pFmt->lindex == -1 ) )
        ExitGracefully(hr, E_INVALIDARG, "Bad format requested");

    // Allocate the medium via GlobalAlloc

    pMedium->tymed = TYMED_HGLOBAL;
    pMedium->hGlobal = GlobalAlloc(GPTR, cbStruct);
    pMedium->pUnkForRelease = NULL;

    if ( !pMedium->hGlobal )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate StgMedium");
    
    hr = S_OK;                  // success

exit_gracefully:

    if ( ppAlloc )
        *ppAlloc = SUCCEEDED(hr) ? (LPVOID)pMedium->hGlobal:NULL;

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ CopyStorageMedium
/ ------------------
/   Copies a storage medium (and the data in an HGLOBAL).  Only works
/   for TYMED_HGLOBAL mediums...
/
/ In:
/   pMediumDst -> where to copy to...
/   pFmt, pMediumSrc -> describe the source
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
EXTERN_C HRESULT CopyStorageMedium(FORMATETC* pFmt, STGMEDIUM* pMediumDst, STGMEDIUM* pMediumSrc)
{
    HRESULT hr;
    LPVOID pSrc, pDst;
    HGLOBAL hGlobal;
    SIZE_T cbStruct;

    TraceEnter(TRACE_COMMON_MISC, "CopyStorageMedium");

    if ( !(pFmt->tymed & TYMED_HGLOBAL) )
        ExitGracefully(hr, E_INVALIDARG, "Only HGLOBAL mediums suppported to copy");

    // stored in a HGLOBAl, therefore get the size, allocate a new storage
    // object and copy the data away into it.

    cbStruct = GlobalSize((HGLOBAL)pMediumSrc->hGlobal);

    hr = AllocStorageMedium(pFmt, pMediumDst, cbStruct, (LPVOID*)&hGlobal);
    FailGracefully( hr, "Unable to allocated storage medium" );

    *pMediumDst = *pMediumSrc;
    pMediumDst->hGlobal = hGlobal;

    pSrc = GlobalLock(pMediumSrc->hGlobal);
    pDst = GlobalLock(pMediumDst->hGlobal);

    CopyMemory(pDst, pSrc, cbStruct);

    GlobalUnlock(pMediumSrc->hGlobal);
    GlobalUnlock(pMediumDst->hGlobal);

    hr = S_OK;                      // success

exit_gracefully:

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ GetStringFromGUID
/ -----------------
/   Given a GUID convert it to a string.
/
/ In:
/   rGUID = guid to be converted
/   psz, cchMax = buffer to fill
/
/ Out:
/   NUMBER OF characters
/----------------------------------------------------------------------------*/

static const BYTE c_rgbGuidMap[] = { 3, 2, 1, 0, '-', 5, 4, '-', 7, 6, '-', 8, 9, '-', 10, 11, 12, 13, 14, 15 };
static const TCHAR c_szDigits[] = TEXT("0123456789ABCDEF");

EXTERN_C INT GetStringFromGUID(UNALIGNED REFGUID rguid, LPTSTR psz, INT cchMax)
{
    INT i;
    const BYTE* pBytes = (const BYTE*)&rguid;

    if ( cchMax < GUIDSTR_MAX )
        return 0;

#ifdef BIG_ENDIAN
    // This is the slow, but portable version
    wsprintf(psz, TEXT("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"),
                    rguid->Data1, rguid->Data2, rguid->Data3,
                    rguid->Data4[0], rguid->Data4[1],
                    rguid->Data4[2], rguid->Data4[3],
                    rguid->Data4[4], rguid->Data4[5],
                    rguid->Data4[6], rguid->Data4[7]);
#else
    // The following algorithm is faster than the wsprintf.
    *psz++ = TEXT('{');

    for (i = 0; i < SIZEOF(c_rgbGuidMap); i++)
    {
        if (c_rgbGuidMap[i] == TEXT('-'))      // don't TEXT() this line
        {
            *psz++ = TEXT('-');
        }
        else
        {
            // Convert a byte-value into a character representation
            *psz++ = c_szDigits[ (pBytes[c_rgbGuidMap[i]] & 0xF0) >> 4 ];
            *psz++ = c_szDigits[ (pBytes[c_rgbGuidMap[i]] & 0x0F) ];
        }
    }
    *psz++ = TEXT('}');
    *psz   = TEXT('\0');
#endif /* !BIG_ENDIAN */

    return GUIDSTR_MAX;
}


/*-----------------------------------------------------------------------------
/ GetGUIDFromString
/ -----------------
/   Given a string convert it to a GUID.
/
/ In:
/   psz -> string to be parsed
/   rGUID = GUID return into
/
/ Out:
/   BOOL
/----------------------------------------------------------------------------*/

BOOL _HexStringToDWORD(LPCTSTR * ppsz, DWORD * lpValue, int cDigits, TCHAR chDelim)
{
    int ich;
    LPCTSTR psz = *ppsz;
    DWORD Value = 0;
    BOOL fRet = TRUE;

    for (ich = 0; ich < cDigits; ich++)
    {
        TCHAR ch = psz[ich];
        if (InRange(ch, TEXT('0'), TEXT('9')))
        {
            Value = (Value << 4) + ch - TEXT('0');
        }
        else if ( InRange( (ch |= (TEXT('a')-TEXT('A'))), TEXT('a'), TEXT('f')) )
        {
            Value = (Value << 4) + ch - TEXT('a') + 10;
        }
        else
            return(FALSE);
    }

    if (chDelim)
    {
        fRet = (psz[ich++] == chDelim);
    }

    *lpValue = Value;
    *ppsz = psz+ich;

    return fRet;
}

#ifndef UNICODE

EXTERN_C BOOL GetGUIDFromStringW(LPCWSTR psz, GUID* pguid)
{
    USES_CONVERSION;
    return GetGUIDFromString(W2CT(psz), pguid);
}

#endif

EXTERN_C BOOL GetGUIDFromString(LPCTSTR psz, GUID* pguid)
{
    DWORD dw;
    if (*psz++ != TEXT('{') /*}*/ )
        return FALSE;

    if (!_HexStringToDWORD(&psz, &pguid->Data1, SIZEOF(DWORD)*2, TEXT('-')))
        return FALSE;

    if (!_HexStringToDWORD(&psz, &dw, SIZEOF(WORD)*2, TEXT('-')))
        return FALSE;

    pguid->Data2 = (WORD)dw;

    if (!_HexStringToDWORD(&psz, &dw, SIZEOF(WORD)*2, TEXT('-')))
        return FALSE;

    pguid->Data3 = (WORD)dw;

    if (!_HexStringToDWORD(&psz, &dw, SIZEOF(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[0] = (BYTE)dw;

    if (!_HexStringToDWORD(&psz, &dw, SIZEOF(BYTE)*2, TEXT('-')))
        return FALSE;

    pguid->Data4[1] = (BYTE)dw;

    if (!_HexStringToDWORD(&psz, &dw, SIZEOF(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[2] = (BYTE)dw;

    if (!_HexStringToDWORD(&psz, &dw, SIZEOF(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[3] = (BYTE)dw;

    if (!_HexStringToDWORD(&psz, &dw, SIZEOF(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[4] = (BYTE)dw;

    if (!_HexStringToDWORD(&psz, &dw, SIZEOF(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[5] = (BYTE)dw;

    if (!_HexStringToDWORD(&psz, &dw, SIZEOF(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[6] = (BYTE)dw;
    if (!_HexStringToDWORD(&psz, &dw, SIZEOF(BYTE)*2, /*(*/ TEXT('}')))
        return FALSE;

    pguid->Data4[7] = (BYTE)dw;

    return TRUE;
}


//
// Replacement for LoadStringW that will work on the Win95 downlevel client
//

#ifndef UNICODE

EXTERN_C INT MyLoadStringW(HINSTANCE hInstance, UINT uID, LPWSTR pszBuffer, INT cchBuffer)
{
    TCHAR szBuffer[MAX_PATH];
    INT cchResult;

    cchResult = LoadString(hInstance, uID, szBuffer, ARRAYSIZE(szBuffer));
    MultiByteToWideChar(CP_ACP, 0, szBuffer, -1, pszBuffer, cchBuffer);
    
    return cchResult;
}

#endif
