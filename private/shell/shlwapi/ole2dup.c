//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1991-1993
//
// File: ole2dup.c
//
//  This file contains all the duplicated code from OLE 2.0 DLLs to avoid
// any link to their DLLs from the shell. If we decided to have links to
// them, we need to delete these files.
//
// History:
//  04-16-97 AndyP      moved parts to shlwapi (from shell32)
//  12-29-92 SatoNa     Created.
//
//---------------------------------------------------------------------------

#include "priv.h"

//
// SHStringFromGUIDA
//
// converts GUID into (...) form without leading identifier; returns
// amount of data copied to lpsz if successful; 0 if buffer too small.
//

// An endian-dependant map of what bytes go where in the GUID
// text representation.
//
// Do NOT use the TEXT() macro in GuidMap... they're intended to be bytes
//

static const BYTE c_rgbGuidMap[] = { 3, 2, 1, 0, '-', 5, 4, '-', 7, 6, '-',
                                     8, 9, '-', 10, 11, 12, 13, 14, 15 };

static const CHAR c_szDigitsA[] = "0123456789ABCDEF";
static const WCHAR c_szDigitsW[] = TEXTW("0123456789ABCDEF");

STDAPI_(int) 
SHStringFromGUIDA(
    UNALIGNED REFGUID rguid, 
    LPSTR   psz, 
    int     cchMax)
{
    int i;
    const BYTE * pBytes = (const BYTE *) rguid;

    if (cchMax < GUIDSTR_MAX)
        return 0;

#ifdef BIG_ENDIAN
    // This is the slow, but portable version
    wnsprintf(psz, cchMax,"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            rguid->Data1, rguid->Data2, rguid->Data3,
            rguid->Data4[0], rguid->Data4[1],
            rguid->Data4[2], rguid->Data4[3],
            rguid->Data4[4], rguid->Data4[5],
            rguid->Data4[6], rguid->Data4[7]);
#else
    // The following algorithm is faster than the wsprintf.
    *psz++ = '{';

    for (i = 0; i < SIZEOF(c_rgbGuidMap); i++)
    {
        if (c_rgbGuidMap[i] == '-')      // don't TEXT() this line
        {
            *psz++ = '-';
        }
        else
        {
            // Convert a byte-value into a character representation
            *psz++ = c_szDigitsA[ (pBytes[c_rgbGuidMap[i]] & 0xF0) >> 4 ];
            *psz++ = c_szDigitsA[ (pBytes[c_rgbGuidMap[i]] & 0x0F) ];
        }
    }
    *psz++ = '}';
    *psz   = '\0';
#endif /* !BIG_ENDIAN */

    return GUIDSTR_MAX;
}


STDAPI_(int) 
SHStringFromGUIDW(
    UNALIGNED REFGUID rguid, 
    LPWSTR  psz, 
    int     cchMax)
{
    int i;
    const BYTE * pBytes = (const BYTE *) rguid;

    if (cchMax < GUIDSTR_MAX)
        return 0;

#ifdef BIG_ENDIAN
    // This is the slow, but portable version
    wnsprintfW(psz, cchMax, L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            rguid->Data1, rguid->Data2, rguid->Data3,
            rguid->Data4[0], rguid->Data4[1],
            rguid->Data4[2], rguid->Data4[3],
            rguid->Data4[4], rguid->Data4[5],
            rguid->Data4[6], rguid->Data4[7]);
#else
    // The following algorithm is faster than the wsprintf.
    *psz++ = TEXTW('{');

    for (i = 0; i < SIZEOF(c_rgbGuidMap); i++)
    {
        if (c_rgbGuidMap[i] == '-')      // don't TEXT() this line
        {
            *psz++ = TEXTW('-');
        }
        else
        {
            // Convert a byte-value into a character representation
            *psz++ = c_szDigitsW[ (pBytes[c_rgbGuidMap[i]] & 0xF0) >> 4 ];
            *psz++ = c_szDigitsW[ (pBytes[c_rgbGuidMap[i]] & 0x0F) ];
        }
    }
    *psz++ = TEXTW('}');
    *psz   = TEXTW('\0');
#endif /* !BIG_ENDIAN */

    return GUIDSTR_MAX;
}

// this makes sure the DLL for the given clsid stays in memory
// this is needed because we violate COM rules and hold apparment objects
// across the lifetime of appartment threads. these objects really need
// to be free threaded (we have always treated them as such)
//
//  Look in the registry and suck out the name of the DLL who owns
//  the CLSID.  We must suck the DLL name as unicode in case the
//  DLL name contains unicode characters.
//
STDAPI_(HINSTANCE) SHPinDllOfCLSID(const CLSID *pclsid)
{
    HKEY hk;
    DWORD dwSize;
    HINSTANCE hinst = NULL;
    TCHAR szClass[GUIDSTR_MAX + 64];    // CLSID\{...}\InProcServer32
    WCHAR szDllPath[MAX_PATH];

    lstrcpy(szClass, TEXT("CLSID\\"));
    SHStringFromGUID(pclsid, szClass+6, GUIDSTR_MAX); // 6 = strlen("CLSID\\")
    lstrcat(szClass, TEXT("\\InProcServer32"));

    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szClass, 0, KEY_QUERY_VALUE, &hk)
                == ERROR_SUCCESS) {

        // Explicitly read as unicode.  SHQueryValueEx handles REG_EXPAND_SZ
        dwSize = SIZEOF(szDllPath);
        if (SHQueryValueExW(hk, 0, 0, 0, szDllPath, &dwSize) == ERROR_SUCCESS) {
            hinst = LoadLibraryExWrapW(szDllPath, NULL, 0);
        }

        RegCloseKey(hk);
    }

    return hinst;
}

// scan psz for a number of hex digits (at most 8); update psz, return
// value in Value; check for chDelim; return TRUE for success.
BOOL HexStringToDword(LPCTSTR * ppsz, DWORD * lpValue, int cDigits, TCHAR chDelim)
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

// parse above format; return TRUE if succesful; always writes over *pguid.
STDAPI_(BOOL) GUIDFromString(LPCTSTR psz, GUID *pguid)
{
    DWORD dw;
    if (*psz++ != TEXT('{') /*}*/ )
        return FALSE;

    if (!HexStringToDword(&psz, &pguid->Data1, SIZEOF(DWORD)*2, TEXT('-')))
        return FALSE;

    if (!HexStringToDword(&psz, &dw, SIZEOF(WORD)*2, TEXT('-')))
        return FALSE;

    pguid->Data2 = (WORD)dw;

    if (!HexStringToDword(&psz, &dw, SIZEOF(WORD)*2, TEXT('-')))
        return FALSE;

    pguid->Data3 = (WORD)dw;

    if (!HexStringToDword(&psz, &dw, SIZEOF(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[0] = (BYTE)dw;

    if (!HexStringToDword(&psz, &dw, SIZEOF(BYTE)*2, TEXT('-')))
        return FALSE;

    pguid->Data4[1] = (BYTE)dw;

    if (!HexStringToDword(&psz, &dw, SIZEOF(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[2] = (BYTE)dw;

    if (!HexStringToDword(&psz, &dw, SIZEOF(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[3] = (BYTE)dw;

    if (!HexStringToDword(&psz, &dw, SIZEOF(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[4] = (BYTE)dw;

    if (!HexStringToDword(&psz, &dw, SIZEOF(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[5] = (BYTE)dw;

    if (!HexStringToDword(&psz, &dw, SIZEOF(BYTE)*2, 0))
        return FALSE;

    pguid->Data4[6] = (BYTE)dw;
    if (!HexStringToDword(&psz, &dw, SIZEOF(BYTE)*2, /*(*/ TEXT('}')))
        return FALSE;

    pguid->Data4[7] = (BYTE)dw;

    return TRUE;
}

#ifdef UNICODE

LWSTDAPI_(BOOL) GUIDFromStringA(LPCSTR psz, GUID *pguid)
{
    TCHAR sz[GUIDSTR_MAX];

    SHAnsiToTChar(psz, sz, SIZECHARS(sz));
    return GUIDFromString(sz, pguid);
}

#else

LWSTDAPI_(BOOL) GUIDFromStringW(LPCWSTR psz, GUID *pguid)
{
    TCHAR sz[GUIDSTR_MAX];

    SHUnicodeToAnsi(psz, sz, SIZECHARS(sz));
    return GUIDFromString(sz, pguid);
}

#endif // UNICODE
