#include "priv.h"

// Build this file only on Win9X or NT4
#ifdef DOWNLEVEL_PLATFORM

__inline LPWSTR StrCpyNXW(LPWSTR psz1, LPCWSTR psz2, int cchMax)
{
    ASSERT(psz1);
    ASSERT(psz2);

    if (0 < cchMax)
    {
        // Leave room for the null terminator
        while (0 < --cchMax)
        {
            if ( !(*psz1++ = *psz2++) ) {
                --psz1;
                break;
            }
        }

        if (0 == cchMax)
            *psz1 = L'\0';

        ASSERT(*psz1 == 0);
    }

    return psz1;
}

LPWSTR StrCpyNW(LPWSTR psz1, LPCWSTR psz2, int cchMax)
{
    StrCpyNXW(psz1, psz2, cchMax);
    return psz1;
}

LPWSTR StrCpyW(LPWSTR psz1, LPCWSTR psz2)
{
    LPWSTR psz = psz1;

    ASSERT(psz1);
    ASSERT(psz2);

    while (*psz1++ = *psz2++)
        ;

    return psz;
}

STDAPI_(DWORD)
RegData_AtoW(
    IN    LPCVOID pvData,
    IN    DWORD   dwSize,
    IN    DWORD   dwType,
    /*INOUT*/ LPDWORD pcbData
)
{
    DWORD dwRet = NO_ERROR;

    if (REG_SZ == dwType || REG_EXPAND_SZ == dwType || REG_MULTI_SZ == dwType)
    {
        DWORD cbData = pcbData ? *pcbData : -1;
        if (pvData)
        {
            // Allocate a temporary buffer to work with
            int cch = MultiByteToWideChar(CP_ACP, 0, (LPSTR)pvData, cbData, NULL, 0);
            LPWSTR pwszT = (LPWSTR)LocalAlloc(LPTR, CbFromCchW(cch));

            if (!pwszT)
                dwRet = ERROR_NOT_ENOUGH_MEMORY;
            else
            {
                if (CbFromCchW(cch) > dwSize)
                    dwRet = ERROR_MORE_DATA;
                else
                {
                    // Convert it to the temporary buffer
                    MultiByteToWideChar(CP_ACP, 0, (LPSTR)pvData, cbData, pwszT, cch);

                    // Copy it back to the output buffer
                    StrCpyW((LPWSTR)pvData, pwszT);
                }
                LocalFree(pwszT);
            }

            // If string data, make room for unicode 
            if (pcbData)
            {
                (*pcbData) = cch * sizeof(WCHAR);
            }
        }
        else if (pcbData)
        {
            // We don't have the data so guess (actual value may be less)
            // bugbug: Does this need to be exact?  For now this seem sufficient. (Stevepro)
            (*pcbData) *= sizeof(WCHAR);
        }
    }
    return dwRet;
}


/*----------------------------------------------------------
Purpose: Recursively delete the key, including all child values
         and keys.  Mimics what RegDeleteKey does in Win95.

Returns: 
Cond:    --
*/
DWORD
DeleteKeyRecursively(
    IN HKEY   hkey, 
    IN LPCSTR pszSubKey)
{
    DWORD dwRet;
    HKEY hkSubKey;

    // Open the subkey so we can enumerate any children
    dwRet = RegOpenKeyExA(hkey, pszSubKey, 0, MAXIMUM_ALLOWED, &hkSubKey);
    if (ERROR_SUCCESS == dwRet)
    {
        DWORD   dwIndex;
        CHAR    szSubKeyName[MAX_PATH + 1];
        DWORD   cchSubKeyName = ARRAYSIZE(szSubKeyName);
        CHAR    szClass[MAX_PATH];
        DWORD   cbClass = ARRAYSIZE(szClass);

        // I can't just call RegEnumKey with an ever-increasing index, because
        // I'm deleting the subkeys as I go, which alters the indices of the
        // remaining subkeys in an implementation-dependent way.  In order to
        // be safe, I have to count backwards while deleting the subkeys.

        // Find out how many subkeys there are
        dwRet = RegQueryInfoKeyA(hkSubKey,
                                 szClass,
                                 &cbClass,
                                 NULL,
                                 &dwIndex, // The # of subkeys -- all we need
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL);

        if (NO_ERROR == dwRet)
        {
            // dwIndex is now the count of subkeys, but it needs to be
            // zero-based for RegEnumKey, so I'll pre-decrement, rather
            // than post-decrement.
            while (ERROR_SUCCESS == RegEnumKeyA(hkSubKey, --dwIndex, szSubKeyName, cchSubKeyName))
            {
                DeleteKeyRecursively(hkSubKey, szSubKeyName);
            }
        }

        RegCloseKey(hkSubKey);

        dwRet = RegDeleteKeyA(hkey, pszSubKey);
    }

    return dwRet;
}

/*----------------------------------------------------------
Purpose: Recursively delete the key, including all child values
         and keys.

Returns: 
Cond:    --
*/
STDAPI_(DWORD)
SHDeleteKeyA(
    IN HKEY   hkey, 
    IN LPCSTR pszSubKey)
{
    DWORD dwRet;

    if (g_bRunOnNT)
    {
        dwRet = DeleteKeyRecursively(hkey, pszSubKey);
    }
    else
    {
        // On Win95, RegDeleteKey does what we want
        dwRet = RegDeleteKeyA(hkey, pszSubKey);
    }

    return dwRet;
}


/*----------------------------------------------------------
Purpose: Recursively delete the key, including all child values
         and keys.

Returns: 
Cond:    --
*/
STDAPI_(DWORD)
SHDeleteKeyW(
    IN HKEY    hkey, 
    IN LPCWSTR pwszSubKey)
{
    DWORD dwRet;
    CHAR sz[MAX_PATH];

    WideCharToMultiByte(CP_ACP, 0, pwszSubKey, -1, sz, SIZECHARS(sz), NULL, NULL);

    if (g_bRunOnNT)
    {
        dwRet = DeleteKeyRecursively(hkey, sz);
    }
    else
    {
        // On Win95, RegDeleteKey does what we want
        dwRet = RegDeleteKeyA(hkey, sz);
    }

    return dwRet;
}


/*----------------------------------------------------------
Purpose: Behaves just like RegQueryValueEx, except if the
         data type is REG_EXPAND_SZ, then this goes ahead
         and expands out the string.  *pdwType will always
         be massaged to REG_SZ if this happens.

Returns: 
Cond:    --
*/
STDAPI_(DWORD)
SHQueryValueExA(
    IN     HKEY    hkey,
    IN     LPCSTR  pszValue,
    IN     LPDWORD lpReserved,
    OUT    LPDWORD pdwType,
    OUT    LPVOID  pvData,
    IN OUT LPDWORD pcbData)
{
    DWORD dwRet;
    DWORD cbSize;
    DWORD dwType;
    LPSTR lpsz;

    // Trying to get back data

    if (pcbData)
        cbSize = *pcbData;     // Size of output buffer
        
    dwRet = RegQueryValueExA(hkey, pszValue, lpReserved, &dwType,
                             pvData, &cbSize);

    // Normally, we'd be done with this.  But do some extra work
    // if this is an expandable string (something that has system
    // variables in it), or if we need to pad the buffer.

    if (NO_ERROR == dwRet)
    {
        // Note: on Win95, RegSetValueEx will always write the 
        // full string out, including the null terminator.  On NT,
        // it won't unless the write length was specified.  
        // Hence, we have the following check.

        // Pad the buffer, in case the string didn't have a null
        // terminator when it was stored?
        if (REG_SZ == dwType)
        {
            // Yes
            if (pvData && cbSize < *pcbData) 
            {
                LPSTR lpszData = pvData;
                lpszData[cbSize] = '\0';
            }
        }
        // Expand the string?
        else if (REG_EXPAND_SZ == dwType)
        {
            // Yes

            // Use a temporary buffer to expand
            if (pvData)
            {
                lpsz = (LPSTR)LocalAlloc(LPTR, *pcbData);    
                if ( !lpsz )
                    return ERROR_OUTOFMEMORY;

                cbSize = ExpandEnvironmentStringsA(pvData, lpsz, *pcbData);

                // BUGBUG:: NT screws up the cbSize returned...
                if (cbSize > 0)
                    cbSize = lstrlenA(lpsz) + 1;
                if (cbSize > 0 && cbSize <= *pcbData) 
                    lstrcpynA(pvData, lpsz, *pcbData);
                else
                    dwRet = GetLastError();
            }
            else
            {
                //
                // Find out the length of the expanded string
                // we have to call in and actually get the data to do this
                //
                CHAR szBuff[1];

                lpsz = (LPSTR)LocalAlloc(LPTR, cbSize);
                if (!lpsz)
                    return ERROR_OUTOFMEMORY;

                dwRet = RegQueryValueExA(hkey, pszValue, lpReserved, NULL,
                                         (LPBYTE)lpsz, &cbSize);

                if (NO_ERROR == dwRet)
                {
                    //  dummy buffer required...
                    DWORD cbExpand = ExpandEnvironmentStringsA(lpsz, szBuff, ARRAYSIZE(szBuff));
                    cbSize = max(cbExpand, cbSize);


                }

            }

            LocalFree(lpsz);

            // Massage dwType so that callers always see REG_SZ
            dwType = REG_SZ;
        }
    }

    if (pdwType)
        *pdwType = dwType;

    if (pcbData)
        *pcbData = cbSize;

    return dwRet;
}

#ifdef UNICODE
/*----------------------------------------------------------
Purpose: Behaves just like RegQueryValueEx, except if the
         data type is REG_EXPAND_SZ, then this goes ahead
         and expands out the string.  *pdwType will always
         be massaged to REG_SZ if this happens.

Returns: 
Cond:    --
*/
STDAPI_(DWORD)
SHQueryValueExW(
    IN     HKEY    hkey,
    IN     LPCWSTR pwszValue,
    IN     LPDWORD lpReserved,
    OUT    LPDWORD pdwType,
    OUT    LPVOID  pvData,
    IN OUT LPDWORD pcbData)
{
    DWORD dwRet;
    DWORD cbSize;
    DWORD dwType;
    LPWSTR lpsz;

    if ( !g_bRunOnNT )
    {
        CHAR szValue[MAX_PATH];
        LPSTR pszValue = NULL;
        DWORD dwOriginalSize = 0;

        if (pcbData)
            dwOriginalSize = *pcbData;

        if (pwszValue)
        {
            WideCharToMultiByte(CP_ACP, 0, pwszValue, -1, szValue, SIZECHARS(szValue), NULL, NULL);
            pszValue = szValue;
        }

        if (!pdwType)
            pdwType = &dwType;

        dwRet = SHQueryValueExA(hkey, pszValue, lpReserved, pdwType, pvData, pcbData);

        if (NO_ERROR == dwRet)
            dwRet = RegData_AtoW(pvData, dwOriginalSize, *pdwType, pcbData);   // Thunk data from ANSI->UNICODE if needed.
    }
    else
    {
        // Running on NT
        // Trying to get back data

        if (pcbData)
            cbSize = *pcbData;     // Size of output buffer
            
        dwRet = RegQueryValueExW(hkey, pwszValue, lpReserved, &dwType,
                                 pvData, &cbSize);

        // Normally, we'd be done with this.  But do some extra work
        // if this is an expandable string (something that has system
        // variables in it), or if we need to pad the buffer.

        if (NO_ERROR == dwRet)
        {
            // Note: on Win95, RegSetValueEx will always write the 
            // full string out, including the null terminator.  On NT,
            // it won't unless the write length was specified.  
            // Hence, we have the following check.

            // Pad the buffer, in case the string didn't have a null
            // terminator when it was stored?
            if (REG_SZ == dwType)
            {
                // Yes
                if (pvData && cbSize + sizeof(WCHAR) <= *pcbData) 
                {
                    LPWSTR lpszData = pvData;
                    lpszData[cbSize / sizeof(WCHAR)] = '\0';
                }
            }
            // Expand the string?
            else if (REG_EXPAND_SZ == dwType)
            {
                if (pvData)
                {
                    // Yes

                    // Use a temporary buffer to expand
                    lpsz = (LPWSTR)LocalAlloc(LPTR, *pcbData);    
                    if (!lpsz)
                        return ERROR_OUTOFMEMORY;

                    cbSize = CbFromCchW(ExpandEnvironmentStringsW(pvData, lpsz, *pcbData / sizeof(WCHAR)));
                    if (cbSize > 0 && cbSize <= *pcbData) 
                        StrCpyNW(pvData, lpsz, *pcbData / sizeof(WCHAR));
                    else
                        dwRet = GetLastError();
                }
                else
                {
                    //
                    // Find out the length of the expanded string
                    // we have to call in and actually get the data to do this
                    //
                    WCHAR szBuff[1];

                    // Find out the length of the expanded string
                    //
                    lpsz = (LPWSTR)LocalAlloc(LPTR, cbSize);
                    if (!lpsz)
                        return ERROR_OUTOFMEMORY;

                    dwRet = RegQueryValueExW(hkey, pwszValue, lpReserved, NULL,
                                             (LPBYTE)lpsz, &cbSize);

                    if (NO_ERROR == dwRet)
                    {
                        DWORD cbExpand = CbFromCchW(ExpandEnvironmentStringsW(lpsz, szBuff, 
                                                                      ARRAYSIZE(szBuff)));
                        cbSize = max(cbExpand, cbSize);
                    }
                }

                LocalFree(lpsz);

                // Massage dwType so that callers always see REG_SZ
                dwType = REG_SZ;
            }
        }

        if (pdwType)
            *pdwType = dwType;

        if (pcbData)
            *pcbData = cbSize;
    }

    return dwRet;
}
#endif //UNICODE

/*
 *  @doc    INTERNAL
 *
 *  @func   int | SHAnsiToUnicodeNativeCP |
 *
 *          Convert an ANSI string to a UNICODE string via the
 *          specified Windows code page.  If the source string is too large
 *          for the destination buffer, then as many characters as
 *          possible are copied.
 *
 *          The resulting output string is always null-terminated.
 *
 *  @parm   UINT | uiCP |
 *
 *          The code page in which to perform the conversion.
 *          This must be a Windows code page.
 *
 *  @parm   LPCSTR | pszSrc |
 *
 *          Source buffer containing ANSI string to be converted.
 *
 *  @parm   int | cchSrc |
 *
 *          Source buffer length, including terminating null.
 *
 *  @parm   LPWSTR | pwszDst |
 *
 *          Destination buffer to receive converted UNICODE string.
 *
 *  @parm   int | cwchBuf |
 *
 *          Size of the destination buffer in <t WCHAR>s.
 *
 *  @returns
 *
 *          On success, the number of characters copied to the output
 *          buffer is returned, including the terminating null.
 */

int
SHAnsiToUnicodeNativeCP(UINT uiCP,
                        LPCSTR pszSrc, int cchSrc,
                        LPWSTR pwszDst, int cwchBuf)
{
    int cwchRc = 0;             /* Assume failure */

    /*
     *  Checks the caller should've made.
     */
    ASSERT(IS_VALID_STRING_PTRA(pszSrc, -1));
    ASSERT(cchSrc == lstrlenA(pszSrc) + 1);
    ASSERT(IS_VALID_WRITE_BUFFER(pwszDst, WCHAR, cwchBuf));
    ASSERT(pszSrc != NULL);
    ASSERT(uiCP != 1200 && uiCP != 65000 && uiCP != 50000 && uiCP != 65001);
    ASSERT(pwszDst);
    ASSERT(cwchBuf);

    cwchRc = MultiByteToWideChar(uiCP, 0, pszSrc, cchSrc, pwszDst, cwchBuf);
    if (cwchRc) {
        /*
         *  The output buffer was big enough; no double-buffering
         *  needed.
         */
    } else if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        /*
         *  The output buffer wasn't big enough.  Need to double-buffer.
         */

        int cwchNeeded = MultiByteToWideChar(uiCP, 0, pszSrc, cchSrc,
                                             NULL, 0);

        ASSERT(cwchRc == 0);        /* In case we fail later */
        if (cwchNeeded) {
            LPWSTR pwsz = (LPWSTR)LocalAlloc(LMEM_FIXED,
                                             cwchNeeded * SIZEOF(WCHAR));
            if (pwsz) {
                cwchRc = MultiByteToWideChar(uiCP, 0, pszSrc, cchSrc,
                                             pwsz, cwchNeeded);
                if (cwchRc) {
                    StrCpyNW(pwszDst, pwsz, cwchBuf);
                    cwchRc = cwchBuf;
                }
                LocalFree(pwsz);
            }
        }
    } else {
        /* Possibly unsupported code page */
        ASSERT(!"Unexpected error in MultiByteToWideChar");
    }

    return cwchRc;
}

int
SHAnsiToUnicodeCP_ACP(LPCSTR pszSrc, LPWSTR pwszDst, int cwchBuf)
{
    int cwchRc = 0;             /* Assume failure */

    ASSERT(IS_VALID_STRING_PTRA(pszSrc, -1));
    ASSERT(IS_VALID_WRITE_BUFFER(pwszDst, WCHAR, cwchBuf));

    /*
     *  Sanity check - NULL source string is treated as a null string.
     */
    if (pszSrc == NULL) {
        pszSrc = "";
    }

    /*
     *  Sanity check - Output buffer must be non-NULL and must be of
     *  nonzero size.
     */
    if (pwszDst && cwchBuf) {

        int cchSrc;

        pwszDst[0] = 0;         /* In case of error */

        cchSrc = lstrlenA(pszSrc) + 1;

        cwchRc = SHAnsiToUnicodeNativeCP(CP_ACP, pszSrc, cchSrc, pwszDst, cwchBuf);
    }

    return cwchRc;
}

/*
 *  @doc    EXTERNAL
 *
 *  @func   int | SHAnsiToUnicode |
 *
 *          Convert an ANSI string to a UNICODE string via the
 *          <c CP_ACP> code page.  If the source string is too large
 *          for the destination buffer, then as many characters as
 *          possible are copied.
 *
 *          The resulting output string is always null-terminated.
 *
 *  @parm   LPCSTR | pszSrc |
 *
 *          Source buffer containing ANSI string to be converted.
 *
 *  @parm   LPWSTR | pwszDst |
 *
 *          Destination buffer to receive converted UNICODE string.
 *
 *  @parm   int | cwchBuf |
 *
 *          Size of the destination buffer in <t WCHAR>s.
 *
 *  @returns
 *
 *          On success, the number of characters copied to the output
 *          buffer is returned, including the terminating null.
 *
 */

int
SHAnsiToUnicode(LPCSTR pszSrc, LPWSTR pwszDst, int cwchBuf)
{
    return SHAnsiToUnicodeCP_ACP(pszSrc, pwszDst, cwchBuf);
}

// dupe a string using the task allocator for returing from a COM interface
// These functions use SHAlloc, so they cannot go into shlwapi.

STDAPI SHStrDupA(LPCSTR psz, WCHAR **ppwsz)
{
    DWORD cch = MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0);
    *ppwsz = (WCHAR *)CoTaskMemAlloc((cch + 1) * SIZEOF(WCHAR));
    if (*ppwsz)
    {
        MultiByteToWideChar(CP_ACP, 0, psz, -1, *ppwsz, cch);
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

// dupe a string using the task allocator for returing from a COM interface

STDAPI SHStrDupW(LPCWSTR psz, WCHAR **ppwsz)
{
    *ppwsz = (WCHAR *)CoTaskMemAlloc((lstrlenW(psz) + 1) * SIZEOF(WCHAR));
    if (*ppwsz)
    {
        StrCpyW(*ppwsz, psz);
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

// Modifies:
//      szRoot
//
// Returns:
//      TRUE if a drive root was found
//      FALSE otherwise
//
STDAPI_(BOOL)
PathStripToRoot(
    LPTSTR szRoot)
{
    while(!PathIsRoot(szRoot))
    {
        if (!PathRemoveFileSpec(szRoot))
        {
            // If we didn't strip anything off,
            // must be current drive
            return(FALSE);
        }
    }

    return(TRUE);
}

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

//
//  Why do we use the unsafe version?
//
//  -   Unsafe is much faster.
//
//  -   The safe version isn't safe after all and serves only to mask
//      existing bugs.  The situation the safe version "saves" is if
//      two threads both try to atomicrelease the same object.  This
//      means that at the same moment, both threads think the object
//      is alive.  Change the timing slightly, and now one thread
//      atomicreleases the object before the other one, so the other
//      thread is now using an object after the first thread already
//      atomicreleased it.  Bug.
//
STDAPI_(void) IUnknown_AtomicRelease(void **ppunk)
{
#if 1 // Unsafe
    if (ppunk && *ppunk) {
        IUnknown* punk = *(IUnknown**)ppunk;
        *ppunk = NULL;
        punk->lpVtbl->Release(punk);
    }
#else // Safe
    if (ppunk) {
        IUnknown* punk = (IUnknown *)InterlockedExchangePointer(ppunk, NULL);
        if (punk) {
            punk->Release();
        }
    }
#endif
}

#endif //DOWNLEVEL_PLATFORM
