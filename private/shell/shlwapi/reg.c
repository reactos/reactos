#include "priv.h"
#include <regapix.h>


// These private helper functions below call ANSI versions of the 
// registry functions so they will run on Win95.



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
Purpose: Helper function to delete a key that has no subkeys and
         no values.  Otherwise does nothing.  Mimics what RegDeleteKey
         does on NT.

Returns:
Cond:    --
*/
DWORD
DeleteEmptyKey(
    IN  HKEY    hkey,
    IN  LPCSTR  pszSubKey)
{
    DWORD dwRet;
    HKEY hkeyNew;

    dwRet = RegOpenKeyExA(hkey, pszSubKey, 0, KEY_READ | KEY_SET_VALUE, 
                          &hkeyNew);
    if (NO_ERROR == dwRet)
    {
        DWORD ckeys;
        DWORD cvalues;

        // Are there any subkeys or values?

        if (NO_ERROR == RegQueryInfoKey(hkeyNew, NULL, NULL, NULL, &ckeys,
                                        NULL, NULL, &cvalues, NULL, NULL,
                                        NULL, NULL) &&
            0 == cvalues && 0 == ckeys)
            {
            // No; delete the subkey
            dwRet = RegDeleteKeyA(hkey, pszSubKey);
            }
        RegCloseKey(hkeyNew);
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
Purpose: Gets a registry value.  This opens and closes the
         key in which the value resides.  

         Perf:  if your code involves setting/getting a series
         of values in the same key, it is better to open
         the key once and set/get the values with the regular
         Win32 registry functions, rather than using this 
         function repeatedly.

Returns:
Cond:    --
*/
STDAPI_(DWORD)
SHGetValueA(
    IN  HKEY    hkey,
    IN  LPCSTR  pszSubKey,          OPTIONAL
    IN  LPCSTR  pszValue,           OPTIONAL
    OUT LPDWORD pdwType,            OPTIONAL
    OUT LPVOID  pvData,             OPTIONAL
    OUT LPDWORD pcbData)            OPTIONAL
{
    DWORD dwRet;
    HKEY hkClose = NULL;

    if (pvData)
    {
        RIPMSG(pcbData != NULL, "SHGetValueA: caller passed pvData output buffer but not size of buffer (pcbData)!");
    }

    if (pszSubKey && *pszSubKey)
    {
        dwRet = RegOpenKeyExA(hkey, pszSubKey, 0, KEY_QUERY_VALUE, &hkClose);
        hkey = hkClose;
        ASSERT(NO_ERROR == dwRet || !hkey);
    }
        
    if (hkey)
    {
        dwRet = SHQueryValueExA(hkey, pszValue, NULL, pdwType, pvData, 
                                 pcbData);

        if (hkClose)
            RegCloseKey(hkClose);
    }
    else if (pcbData)
        *pcbData = 0;

    return dwRet;
}


/*----------------------------------------------------------
Purpose: Gets a registry value.  This opens and closes the
         key in which the value resides.  

         On Win95, this function thunks and calls the ansi
         version.  On NT, this function calls the unicode
         registry APIs.

         Perf:  if your code involves setting/getting a series
         of values in the same key, it is better to open
         the key once and set/get the values with the regular
         Win32 registry functions, rather than using this 
         function repeatedly.

Returns:
Cond:    --
*/
STDAPI_(DWORD)
SHGetValueW(
    IN  HKEY    hkey,
    IN  LPCWSTR pwszSubKey,         OPTIONAL
    IN  LPCWSTR pwszValue,          OPTIONAL
    OUT LPDWORD pdwType,            OPTIONAL
    OUT LPVOID  pvData,             OPTIONAL
    OUT LPDWORD pcbData)            OPTIONAL
{
    DWORD dwRet;

    if (pvData)
    {
        RIPMSG(pcbData != NULL, "SHGetValueW: caller passed pvData output buffer but not size of buffer (pcbData)!");
    }

    if (g_bRunningOnNT)
    {
        HKEY hkClose = NULL;

        if (pwszSubKey && *pwszSubKey)
        {
            dwRet = RegOpenKeyExW(hkey, pwszSubKey, 0, KEY_QUERY_VALUE, &hkClose);
            hkey = hkClose;
            ASSERT(NO_ERROR == dwRet || !hkey);

        }
            
        if (hkey)
        {
            dwRet = SHQueryValueExW(hkey, pwszValue, NULL, pdwType, pvData, 
                                     pcbData);
            if (hkClose)
                RegCloseKey(hkClose);
        }
        else if (pcbData)
            *pcbData = 0;
    }
    else
    {
        CHAR szSubKey[MAX_PATH];
        CHAR szValue[MAX_PATH];
        LPSTR pszSubKey = NULL;
        LPSTR pszValue = NULL;
        DWORD dwType;
        DWORD cbSizeSav = 0;

        // Thunk the values
        if (pwszSubKey)
        {
            WideCharToMultiByte(CP_ACP, 0, pwszSubKey, -1, szSubKey, SIZECHARS(szSubKey), NULL, NULL);
            pszSubKey = szSubKey;    
        }
        
        if (pwszValue)
        {
            WideCharToMultiByte(CP_ACP, 0, pwszValue, -1, szValue, SIZECHARS(szValue), NULL, NULL);
            pszValue = szValue;
        }

        if (pcbData)
            cbSizeSav = *pcbData;       // Save this size for later

        dwRet = SHGetValueA(hkey, pszSubKey, pszValue, &dwType, pvData, pcbData);

        if (pdwType)
            *pdwType = dwType;

        if (NO_ERROR == dwRet)
        {
            dwRet = RegData_AtoW(pvData, cbSizeSav, dwType, pcbData);   // Thunk data from ANSI->UNICODE if needed.
        }
    }
    return dwRet;
}


/*----------------------------------------------------------
Purpose: Sets a registry value.  This opens and closes the
         key in which the value resides.  

         Perf:  if your code involves setting/getting a series
         of values in the same key, it is better to open
         the key once and set/get the values with the regular
         Win32 registry functions, rather than using this 
         function repeatedly.

Returns:
Cond:    --
*/
STDAPI_(DWORD)
SHSetValueA(
    IN  HKEY    hkey,
    IN OPTIONAL LPCSTR  pszSubKey,
    IN  LPCSTR  pszValue,
    IN  DWORD   dwType,
    IN  LPCVOID pvData,
    IN  DWORD   cbData)
{
    DWORD dwRet = NO_ERROR;
    HKEY hkeyNew;
    DWORD dwDisp;

    if (pszSubKey && pszSubKey[0])
        dwRet = RegCreateKeyExA(hkey, pszSubKey, 0, "", 0, KEY_SET_VALUE, NULL, &hkeyNew, &dwDisp);
    else
        hkeyNew = hkey;

    if (NO_ERROR == dwRet)
    {
        dwRet = RegSetValueExA(hkeyNew, pszValue, 0, dwType, pvData, cbData);

        if (hkeyNew != hkey)
            RegCloseKey(hkeyNew);
    }
    return dwRet;
}


/*----------------------------------------------------------
Purpose: Sets a registry value.  This opens and closes the
         key in which the value resides.  

         On Win95, this function thunks and calls the ansi
         version.  On NT, this function calls the unicode
         registry APIs directly.

         Perf:  if your code involves setting/getting a series
         of values in the same key, it is better to open
         the key once and set/get the values with the regular
         Win32 registry functions, rather than using this 
         function repeatedly.

Returns:
Cond:    --
*/
STDAPI_(DWORD)
SHSetValueW(
    IN  HKEY    hkey,
    IN OPTIONAL LPCWSTR pwszSubKey,
    IN  LPCWSTR pwszValue,
    IN  DWORD   dwType,
    IN  LPCVOID pvData,
    IN  DWORD   cbData)
{
    DWORD dwRet = NO_ERROR;
    HKEY hkeyNew;
    DWORD dwDisp;

    if (pwszSubKey && pwszSubKey[0])
    {
        if (g_bRunningOnNT)
        {
            dwRet = RegCreateKeyExW(hkey, pwszSubKey, 0, TEXTW(""), 0, KEY_SET_VALUE, 
                                    NULL, &hkeyNew, &dwDisp);
        }
        else
        {
            CHAR szSubKey[MAX_PATH];
            LPSTR pszSubKey = NULL;

            if (pwszSubKey)
            {
                WideCharToMultiByte(CP_ACP, 0, pwszSubKey, -1, szSubKey, SIZECHARS(szSubKey), NULL, NULL);
                pszSubKey = szSubKey;    
            }
        
            dwRet = RegCreateKeyExA(hkey, pszSubKey, 0, "", REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, 
                                    NULL, &hkeyNew, &dwDisp);
        }
    }
    else
        hkeyNew = hkey;

    if (NO_ERROR == dwRet)
    {
        // RegSetValueExW is not supported on Win95 but we have a thunking function.
        dwRet = RegSetValueExW(hkeyNew, pwszValue, 0, dwType, pvData, cbData);

        if (hkeyNew != hkey)
            RegCloseKey(hkeyNew);
    }

    return dwRet;

}


/*----------------------------------------------------------
Purpose: Deletes a registry value.  This opens and closes the
         key in which the value resides.  

         Perf:  if your code involves setting/getting a series
         of values in the same key, it is better to open
         the key once and set/get the values with the regular
         Win32 registry functions, rather than using this 
         function repeatedly.

Returns:
Cond:    --
*/
STDAPI_(DWORD)
SHDeleteValueA(
    IN  HKEY    hkey,
    IN  LPCSTR  pszSubKey,
    IN  LPCSTR  pszValue)
{
    DWORD dwRet;
    HKEY hkeyNew;

    dwRet = RegOpenKeyExA(hkey, pszSubKey, 0, KEY_SET_VALUE, &hkeyNew);
    if (NO_ERROR == dwRet)
    {
        dwRet = RegDeleteValueA(hkeyNew, pszValue);
        RegCloseKey(hkeyNew);
    }
    return dwRet;
}


/*----------------------------------------------------------
Purpose: Deletes a registry value.  This opens and closes the
         key in which the value resides.  

         On Win95, this function thunks and calls the ansi
         version.  On NT, this function calls the unicode
         registry APIs directly.

         Perf:  if your code involves setting/getting a series
         of values in the same key, it is better to open
         the key once and set/get the values with the regular
         Win32 registry functions, rather than using this 
         function repeatedly.

Returns:
Cond:    --
*/
STDAPI_(DWORD)
SHDeleteValueW(
    IN  HKEY    hkey,
    IN  LPCWSTR pwszSubKey,
    IN  LPCWSTR pwszValue)
{
    DWORD dwRet;
    HKEY hkeyNew;

    if (g_bRunningOnNT)
    {
        dwRet = RegOpenKeyExW(hkey, pwszSubKey, 0, KEY_SET_VALUE, &hkeyNew);
        if (NO_ERROR == dwRet)
        {
            dwRet = RegDeleteValueW(hkeyNew, pwszValue);
            RegCloseKey(hkeyNew);
        }
    }
    else
    {
        CHAR szSubKey[MAX_PATH];
        CHAR szValue[MAX_PATH];
        LPSTR pszSubKey = NULL;
        LPSTR pszValue = NULL;

        if (pwszSubKey)
        {
            WideCharToMultiByte(CP_ACP, 0, pwszSubKey, -1, szSubKey, SIZECHARS(szSubKey), NULL, NULL);
            pszSubKey = szSubKey;    
        }
        
        if (pwszValue)
        {
            WideCharToMultiByte(CP_ACP, 0, pwszValue, -1, szValue, SIZECHARS(szValue), NULL, NULL);
            pszValue = szValue;
        }

        dwRet = SHDeleteValueA(hkey, pszSubKey, pszValue);
    }

    return dwRet;
}

// purpose: recursively copy subkeys and values of hkeySrc\pszSrcSubKey to hkeyDest
// e.g. hkeyExplorer = HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\
//      SHCopyKey(HKEY_CURRENT_USER, "Software\\Classes\\", hkeyExplorer, 0)
// results in
//      ...\\CurrentVersion\\Explorer\\
//                                     Appid
//                                     CLSID\\
//                                            {xxxx yyyyy ...}
//                                     Interface
//                                     ...
// TO DO: currently we are not copying the ACL's but in the future we should do that
// upon request that's what fReserved is for
// NOTE that there is no hkeyDest, pszDestSubKey pair like src one, because in case
// pszDestSubKey did not exist we would have to create it and deal with Class name
// which would just clober the parameter list
STDAPI_(DWORD) SHCopyKeyA(HKEY hkeySrc, LPCSTR pszSrcSubKey, HKEY hkeyDest, DWORD fReserved)
{
    HKEY hkeyFrom;
    DWORD dwRet;
    
    if (pszSrcSubKey)
        dwRet = RegOpenKeyExA(hkeySrc, pszSrcSubKey, 0, MAXIMUM_ALLOWED, &hkeyFrom);
    else if (hkeySrc)    
    {
        dwRet = ERROR_SUCCESS;
        hkeyFrom = hkeySrc;
    }
    else
        dwRet = ERROR_INVALID_PARAMETER;

    if (dwRet == ERROR_SUCCESS)
    {
        DWORD dwIndex;
        DWORD cchValueSize;
        DWORD cchClassSize;
        DWORD dwType;
        CHAR  szValue[MAX_PATH]; //NOTE:szValue is also used to store subkey name when enumerating keys
        CHAR  szClass[MAX_PATH];
                
        cchValueSize = ARRAYSIZE(szValue);
        cchClassSize = ARRAYSIZE(szClass);
        for (dwIndex=0; 
             dwRet == ERROR_SUCCESS && (dwRet = RegEnumKeyExA(hkeyFrom, dwIndex, szValue, &cchValueSize, NULL, szClass, &cchClassSize, NULL)) == ERROR_SUCCESS; 
             dwIndex++, cchValueSize = ARRAYSIZE(szValue), cchClassSize = ARRAYSIZE(szClass))
        {
            HKEY  hkeyTo;
            DWORD dwDisp;

            // create new key
            dwRet = RegCreateKeyExA(hkeyDest, szValue, 0, szClass, REG_OPTION_NON_VOLATILE, MAXIMUM_ALLOWED, NULL, &hkeyTo, &dwDisp);
            if (dwRet != ERROR_SUCCESS)
                break;

            dwRet = SHCopyKeyA(hkeyFrom, szValue, hkeyTo, fReserved); //if not error_success we break out
            RegCloseKey(hkeyTo);
        }

        // copied all the sub keys, now copy all the values
        if (dwRet == ERROR_NO_MORE_ITEMS)
        {
            DWORD  cb;
            DWORD  cbBufferSize;
            LPBYTE lpbyBuffer;
            
            // get the max value size
            if ((dwRet = RegQueryInfoKey(hkeyFrom, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &cb, NULL, NULL)) == ERROR_SUCCESS)
            {
                // allocate buffer
                cb++; // add 1 just in case of a string
                lpbyBuffer = (LPBYTE)LocalAlloc(LPTR, cb);
                if (lpbyBuffer)
                    cbBufferSize = cb;
                else
                    dwRet = ERROR_OUTOFMEMORY;
                    
                cchValueSize = ARRAYSIZE(szValue);
                for (dwIndex=0;
                     dwRet == ERROR_SUCCESS && (dwRet = RegEnumValueA(hkeyFrom, dwIndex, szValue, &cchValueSize, NULL, &dwType, lpbyBuffer, &cb)) == ERROR_SUCCESS;
                     dwIndex++, cchValueSize = ARRAYSIZE(szValue), cb = cbBufferSize)
                {
                    // cb has the size of the value so use it rather than cbBufferSize which is just max size
                    dwRet = RegSetValueExA(hkeyDest, szValue, 0, dwType, lpbyBuffer, cb);
                    if (dwRet != ERROR_SUCCESS)
                        break;
                }

                if (lpbyBuffer != NULL)
                    LocalFree(lpbyBuffer);
            }
        }
    
        if (dwRet == ERROR_NO_MORE_ITEMS)
            dwRet = ERROR_SUCCESS;

        if (pszSrcSubKey)
            RegCloseKey(hkeyFrom);
    }

    return dwRet;
}

STDAPI_(DWORD) SHCopyKeyW(HKEY hkeySrc, LPCWSTR pwszSrcSubKey, HKEY hkeyDest, DWORD fReserved)
{
    CHAR sz[MAX_PATH];
    if (pwszSrcSubKey)
        WideCharToMultiByte(CP_ACP, 0, pwszSrcSubKey, -1, sz, SIZECHARS(sz), NULL, NULL);
    return SHCopyKeyA(hkeySrc, pwszSrcSubKey ? sz : NULL, hkeyDest, fReserved);
}


/*----------------------------------------------------------
Purpose: Delete a key only if there are no subkeys or values.
         It comes close to mimicking the behavior of RegDeleteKey 
         as it works on NT, except the NT version ignores values.

*/
STDAPI_(DWORD)
SHDeleteEmptyKeyA(
    IN  HKEY    hkey,
    IN  LPCSTR  pszSubKey)
{
    return DeleteEmptyKey(hkey, pszSubKey);
}


/*----------------------------------------------------------
Purpose: Delete a key only if there are no subkeys or values.
         It comes close to mimicking the behavior of RegDeleteKey 
         as it works on NT, except the NT version ignores values.

*/
STDAPI_(DWORD)
SHDeleteEmptyKeyW(
    IN  HKEY    hkey,
    IN  LPCWSTR pwszSubKey)
{
    CHAR sz[MAX_PATH];

    WideCharToMultiByte(CP_ACP, 0, pwszSubKey, -1, sz, SIZECHARS(sz), NULL, NULL);
    return DeleteEmptyKey(hkey, sz);
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

    if (g_bRunningOnNT)
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

    if (g_bRunningOnNT)
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
    DWORD cbSize = 0;
    DWORD dwType;
    LPSTR lpsz;


    if (pvData)
    {
        RIPMSG(pcbData != NULL, "SHQueryValuegExW: caller passed pvData output buffer but not size of buffer (pcbData)!");
    }

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
                if (g_bRunningOnNT && (cbSize > 0))
                {
                    SHTruncateString(lpsz, *pcbData);
                    cbSize = lstrlen(lpsz) + 1;
                }
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
    DWORD cbSize = 0;
    DWORD dwType;
    LPWSTR lpsz;


    if (pvData)
    {
        RIPMSG(pcbData != NULL, "SHQueryValueExW: caller passed pvData output buffer but not size of buffer (pcbData)!");
    }

    if ( !g_bRunningOnNT )
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
                        lstrcpynW(pvData, lpsz, *pcbData / sizeof(WCHAR));
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

/*----------------------------------------------------------
Purpose: Behaves just like RegEnumKeyExA, except it does not let
         you look at the class and timestamp of the sub-key. Written
         to provide equivalent for SHEnumKeyExW which is useful on
         Win95.

Returns: 
Cond:    --
*/

STDAPI_(LONG)
SHEnumKeyExA
(
    IN HKEY         hkey,
    IN DWORD        dwIndex,        
    OUT LPSTR       pszName,
    IN OUT LPDWORD  pcchName
)
{
    return RegEnumKeyExA(hkey, dwIndex, pszName, pcchName, NULL, NULL, NULL, NULL);
}          
        
/*----------------------------------------------------------
Purpose: Behaves just like RegEnumKeyExW, except it does not let
         you look at the class and timestamp of the sub-key. 
         Wide char version supported under Win95.

Returns: 
Cond:    --
*/

STDAPI_(LONG)
SHEnumKeyExW
(
    IN HKEY         hkey,
    IN DWORD        dwIndex,        
    OUT LPWSTR      pszName,
    IN OUT LPDWORD  pcchName
)
{
    LONG lRet = NO_ERROR;

    if ( !g_bRunningOnNT )
    {
        // We are reading in sub-key names. regapix.h suggests MAXIMUM_SUB_KEY_LENGTH = 256, 
        // so MAX_PATH should be enough. Is this the official limit.
        CHAR sz[MAX_PATH];  
        DWORD dwSize = sizeof(sz);

        lRet = RegEnumKeyExA(hkey, dwIndex, sz, &dwSize, NULL, NULL, NULL, NULL);
        if (NO_ERROR != lRet)
        {
            ASSERT(ERROR_MORE_DATA != lRet);
        }
        else 
        {
            int cch = MultiByteToWideChar(CP_ACP, 0, sz, -1, NULL, 0);

            if (NULL == pcchName)
            {
                lRet = ERROR_INVALID_PARAMETER;
            }
            else if (NULL == pszName || *pcchName < (DWORD)cch )
            {
                *pcchName = cch;
                lRet = (NULL == pszName) ? ERROR_SUCCESS : ERROR_MORE_DATA ;
            }
            else 
            {
                // Not supposed to include null terminator in count if successful.
                *pcchName = cch - 1;
                MultiByteToWideChar(CP_ACP, 0, sz, -1, pszName, cch);
                ASSERT(NO_ERROR == lRet);
            } 
        }
        
    }
    else
    {
        lRet = RegEnumKeyExW(hkey, dwIndex, pszName, pcchName, NULL, NULL, NULL, NULL);
    }    
    
    return lRet;
}        

/*----------------------------------------------------------
Purpose: Behaves just like RegEnumValueA. Written to provide 
         equivalent for SHEnumKeyExW which is useful on Win95.
         Environment vars in a string are NOT expanded. 

Returns: 
Cond:    --
*/

STDAPI_(LONG)
SHEnumValueA
(
    IN HKEY         hkey,
    IN DWORD        dwIndex,        
    OUT LPSTR       pszValueName,   OPTIONAL
    IN OUT LPDWORD  pcchValueName,   OPTIONAL
    OUT LPDWORD     pdwType,        OPTIONAL
    OUT LPVOID      pvData,         OPTIONAL
    IN OUT LPDWORD  pcbData         OPTIONAL
)
{
    return RegEnumValueA(hkey, dwIndex, pszValueName, pcchValueName, NULL, pdwType, pvData, pcbData);
}


/*----------------------------------------------------------
Purpose: Behaves just like RegEnumValueW. Wide char version
         works on Win95.
         Environment vars in a string are NOT expanded. 
Returns: 
Cond:    --
*/

STDAPI_(LONG)
SHEnumValueW
(
    IN HKEY         hkey,
    IN DWORD        dwIndex,        
    OUT LPWSTR      pszValueName,   OPTIONAL       
    IN OUT LPDWORD  pcchValueName,  OPTIONAL
    OUT LPDWORD     pdwType,        OPTIONAL
    OUT LPVOID      pvData,         OPTIONAL
    IN OUT LPDWORD  pcbData         OPTIONAL
)
{
    LONG lRet;

    if ( !g_bRunningOnNT )
    {
        CHAR sz[MAX_PATH]; // This should be sufficient to read the value name.
        DWORD dwSize = sizeof(sz);
        DWORD dwType;
        DWORD cbSizeSav = 0;

        if (pcbData)
            cbSizeSav = *pcbData;

        lRet = RegEnumValueA(hkey, dwIndex, sz, &dwSize, NULL, &dwType, pvData, pcbData); 

        if (pdwType)
            *pdwType = dwType;

        if (NO_ERROR == lRet)
        {
            // Convert the ValueName to unicode.

            if (pcchValueName)
            {
                int cch = MultiByteToWideChar(CP_ACP, 0, sz, -1, NULL, 0);

                if (NULL == pszValueName || *pcchValueName < (DWORD)cch )
                {
                    *pcchValueName = cch;
                    lRet = (NULL == pszValueName) ? ERROR_SUCCESS : ERROR_MORE_DATA ;
                }
                else 
                {
                    // Not supposed to include null terminator in count if returning
                    // back name succesfully. 
                    *pcchValueName = cch - 1;
                    // cch should be sufficient
                    MultiByteToWideChar(CP_ACP, 0, sz, -1, pszValueName, cch);
                }

            }

            // Next convert the data to unicode if it is a string. 
            if ((NOERROR == lRet) && pcbData)
                lRet = RegData_AtoW(pvData, cbSizeSav, dwType, pcbData);
        }
    }
    else
    {
        lRet = RegEnumValueW(hkey, dwIndex, pszValueName, pcchValueName, NULL, pdwType, pvData, pcbData);
    }
    
    return lRet;                                    
}

/*----------------------------------------------------------
Purpose: Behaves just like RegQueryInfoKeyA. Written to provide
         equivalent for W version. 
Returns: 
Cond:    --
*/

STDAPI_(LONG)
SHQueryInfoKeyA
(
    IN  HKEY        hkey,
    OUT LPDWORD     pcSubKeys,             OPTIONAL
    OUT LPDWORD     pcchMaxSubKeyLen,      OPTIONAL
    OUT LPDWORD     pcValues,              OPTIONAL
    OUT LPDWORD     pcchMaxValueNameLen    OPTIONAL
)
{
    return RegQueryInfoKeyA(hkey, NULL, NULL, NULL, pcSubKeys, pcchMaxSubKeyLen, 
                    NULL, pcValues, pcchMaxValueNameLen, NULL, NULL, NULL);
}                     


/*----------------------------------------------------------
Purpose: Behaves just like RegQueryInfoKeyW. Works on Win95.
Returns: 
Cond:    --
*/

STDAPI_(LONG)
SHQueryInfoKeyW
(
    IN  HKEY        hkey,
    OUT LPDWORD     pcSubKeys,             OPTIONAL
    OUT LPDWORD     pcchMaxSubKeyLen,       OPTIONAL
    OUT LPDWORD     pcValues,              OPTIONAL
    OUT LPDWORD     pcchMaxValueNameLen     OPTIONAL
)
{
    LONG lRet;

    if (!g_bRunningOnNT )
    {
        lRet = RegQueryInfoKeyA(hkey, NULL, NULL, NULL, pcSubKeys, pcchMaxSubKeyLen,
                    NULL, pcValues, pcchMaxValueNameLen, NULL, NULL, NULL);

        if (NO_ERROR == lRet)                
        {
            // *pcchMaxSubKeyLen has the number of single byte chars reqd for the
            // KeyName. We return back the same value which is an upper limit on 
            // the number of Unicode characters needed. We will ask for more
            // Unicode chars than required if the string has DBCS characters.
            // Without knowing the actual string this is the best guess we can take.       
        }
    }
    else
    {
        lRet = RegQueryInfoKeyW(hkey, NULL, NULL, NULL, pcSubKeys, pcchMaxSubKeyLen, 
                        NULL, pcValues, pcchMaxValueNameLen, NULL, NULL, NULL);
    }

    return lRet;
}


/*----------------------------------------------------------*\
        USER SPECIFC SETTINGS

  DESCRIPTION:
    These functions will be used to query User Specific settings
    correctly.  The installer needs to populate HKLM
    with User specific settings, because that's the only part
    of the registry that is shared between all users.  Code will
    then read values from HKCU, and if that's empty, it
    will look in HKLM.  The only exception is that if
    TRUE is passed in for the fIgnore parameter, then the HKLM version
    will be used instead of HKCU.  This is the way that an admin can
    specify that they doesn't want users to be able to use their
    User Specific values (HKCU).
\*----------------------------------------------------------*/

typedef struct tagUSKEY
{
    HKEY    hkeyCurrentUser;
    HKEY    hkeyCurrentUserRelative;
    HKEY    hkeyLocalMachine;
    HKEY    hkeyLocalMachineRelative;
    CHAR    szSubPath[MAXIMUM_SUB_KEY_LENGTH];
    REGSAM  samDesired;
} USKEY;

typedef USKEY * PUSKEY;
typedef PUSKEY * PPUSKEY;

#define IS_HUSKEY_VALID(pUSKey)    (((pUSKey) && IS_VALID_WRITE_PTR((pUSKey), USKEY) && ((pUSKey)->hkeyCurrentUser || (pUSKey)->hkeyLocalMachine)))


// Private Helper Function
// Bring the out of date key up to date.
LONG PrivFullOpen(PUSKEY pUSKey)
{
    LONG       lRet         = ERROR_SUCCESS;
    HKEY       *phkey       = NULL;
    HKEY       *phkeyRel    = NULL;

    ASSERT(IS_HUSKEY_VALID(pUSKey));        // Will always be true, but assert against maintainence mistakes

    if (!pUSKey->hkeyCurrentUser)           // Do we need to open HKCU?
    {
        phkey = &(pUSKey->hkeyCurrentUser);
        phkeyRel = &(pUSKey->hkeyCurrentUserRelative);
    }
    if (!pUSKey->hkeyLocalMachine)          // Do we need to open HKLM?
    {
        phkey = &(pUSKey->hkeyLocalMachine);
        phkeyRel = &(pUSKey->hkeyLocalMachineRelative);
    }

    if ((phkeyRel) && (*phkeyRel))
    {
        ASSERT(phkey);        // Will always be true, but assert against maintainence mistakes

        lRet = RegOpenKeyExA(*phkeyRel, pUSKey->szSubPath, 0, pUSKey->samDesired, phkey);

        // If we need to bring the out of date key, up to date, we need to free the old one.
        if ((HKEY_CURRENT_USER != *phkeyRel) && (HKEY_LOCAL_MACHINE != *phkeyRel))
            RegCloseKey(*phkeyRel);
        *phkeyRel = NULL;
        pUSKey->szSubPath[0] = '\0';
    }
    return lRet;
}



// Private Helper Function
// Bring the out of date key up to date.
LONG PrivFullCreate(PUSKEY pUSKey)
{
    LONG       lRet         = ERROR_SUCCESS;
    HKEY       *phkey       = NULL;
    HKEY       *phkeyRel    = NULL;

    ASSERT(IS_HUSKEY_VALID(pUSKey));        // Will always be true, but assert against maintainence mistakes

    if (!pUSKey->hkeyCurrentUser)           // Do we need to open HKCU?
    {
        phkey = &(pUSKey->hkeyCurrentUser);
        phkeyRel = &(pUSKey->hkeyCurrentUserRelative);
    }
    if (!pUSKey->hkeyLocalMachine)          // Do we need to open HKLM?
    {
        phkey = &(pUSKey->hkeyLocalMachine);
        phkeyRel = &(pUSKey->hkeyLocalMachineRelative);
    }

    if ((phkeyRel) && (*phkeyRel))
    {
        ASSERT(phkey);        // Will always be true, but assert against maintainence mistakes

        lRet = RegCreateKeyExA(*phkeyRel, pUSKey->szSubPath, 0, NULL, REG_OPTION_NON_VOLATILE, pUSKey->samDesired, NULL, phkey, NULL);

        // If we need to bring the out of date key, up to date, we need to free the old one.
        if ((HKEY_CURRENT_USER != *phkeyRel) && (HKEY_LOCAL_MACHINE != *phkeyRel))
            RegCloseKey(*phkeyRel);
        *phkeyRel = NULL;
        pUSKey->szSubPath[0] = '\0';
    }
    return lRet;
}


// Private Helper Function
// Create one of the keys (Called for both HKLM and HKCU)
LONG PrivCreateKey(LPHKEY lphkey, LPHKEY lphkeyRelative, LPCSTR lpSubPath, REGSAM samDesired)
{
    LONG    lRet = ERROR_SUCCESS;

    if (*lphkeyRelative)
    {
        lRet = RegCreateKeyExA(*lphkeyRelative, lpSubPath, 0, NULL, REG_OPTION_NON_VOLATILE, samDesired, NULL, lphkey, NULL);
        *lphkeyRelative = NULL;
    }
    else
    {
        // If the relative key == NULL, then we don't have enough of the path to
        // create this key.
        return(ERROR_INVALID_PARAMETER);
    }
    return(lRet);
}



// Private Helper Function
// Query for the specific value.
LONG PrivRegQueryValue(
    IN  PUSKEY          pUSKey,
    IN  HKEY            *phkey,
    IN  LPCWSTR         pwzValue,           // May have been an ANSI String type case.  Use fWideChar to determine if so.
    IN  BOOL            fWideChar,
    OUT LPDWORD         pdwType,            OPTIONAL
    OUT LPVOID          pvData,             OPTIONAL
    OUT LPDWORD         pcbData)            OPTIONAL
{
    LONG       lRet       = ERROR_SUCCESS;

    ASSERT(IS_HUSKEY_VALID(pUSKey));        // Will always be true, but assert against maintainence mistakes

    // It may be necessary to open the key
    if (NULL == *phkey)
        lRet = PrivFullOpen(pUSKey);

    if ((ERROR_SUCCESS == lRet) && (*phkey))
    {
        if (fWideChar)
            lRet = SHQueryValueExW(*phkey, pwzValue, NULL, pdwType, pvData, pcbData);
        else
            lRet = SHQueryValueExA(*phkey, (LPCSTR)pwzValue, NULL, pdwType, pvData, pcbData);
    }
    else
        lRet = ERROR_INVALID_PARAMETER;

    return lRet;
}




// Private Helper Function
// Query for the specific value.
LONG PrivRegWriteValue(
    IN  PUSKEY          pUSKey,
    IN  HKEY            *phkey,
    IN  LPCWSTR         pwzValue,           // May have been an ANSI String type case.  Use fWideChar to determine if so.
    IN  BOOL            bWideChar,
    IN  BOOL            bForceWrite,
    IN  DWORD           dwType,             OPTIONAL
    IN  LPCVOID         pvData,             OPTIONAL
    IN  DWORD           cbData)             OPTIONAL
{
    LONG       lRet       = ERROR_SUCCESS;

    ASSERT(IS_HUSKEY_VALID(pUSKey));        // Will always be true, but assert against maintainence mistakes

    // It may be necessary to open the key
    if (NULL == *phkey)
        lRet = PrivFullCreate(pUSKey);

    // Check if the caller only want's to write value if it's empty
    if (!bForceWrite)
    {   // Yes we need to check before we write.

        if (bWideChar)
            bForceWrite = !(ERROR_SUCCESS == SHQueryValueExW(*phkey, pwzValue, NULL, NULL, NULL, NULL));
        else
            bForceWrite = !(ERROR_SUCCESS == SHQueryValueExA(*phkey, (LPCSTR)pwzValue, NULL, NULL, NULL, NULL));
    }

    if ((ERROR_SUCCESS == lRet) && (*phkey) && bForceWrite)
    {
        if (bWideChar)
            // RegSetValueExW is not supported on Win95 but we have a thunking function.
            lRet = RegSetValueExW(*phkey, pwzValue, 0, dwType, pvData, cbData);
        else
            lRet = RegSetValueExA(*phkey, (LPCSTR)pwzValue, 0, dwType, pvData, cbData);
    }

    return lRet;
}

// Private helper function
// Enum sub-keys of a key.
LONG PrivRegEnumKey(
    IN      PUSKEY          pUSKey,
    IN      HKEY            *phkey,
    IN      DWORD           dwIndex,
    IN      LPWSTR          pwzName,           // May have been an ANSI String type case.  Use fWideChar to determine if so.
    IN      BOOL            fWideChar,
    IN OUT  LPDWORD         pcchName
)
{
    LONG lRet       = ERROR_SUCCESS;

    ASSERT(IS_HUSKEY_VALID(pUSKey));    

    // It may be necessary to open the key
    if (NULL == *phkey)
        lRet = PrivFullOpen(pUSKey);

    if ((ERROR_SUCCESS == lRet) && (*phkey))
    {
        if (fWideChar)
            lRet = SHEnumKeyExW(*phkey, dwIndex, pwzName, pcchName);
        else
            lRet = SHEnumKeyExA(*phkey, dwIndex, (LPSTR)pwzName, pcchName);
    }
    else
        lRet = ERROR_INVALID_PARAMETER;

    return lRet;
}


// Private helper function
// Enum values of a key.
LONG PrivRegEnumValue(
    IN      PUSKEY          pUSKey,
    IN      HKEY            *phkey,
    IN      DWORD           dwIndex,
    IN      LPWSTR          pwzValueName,       // May have been an ANSI String type case.  Use fWideChar to determine if so.
    IN      BOOL            fWideChar,
    IN OUT  LPDWORD         pcchValueName,
    OUT     LPDWORD         pdwType,            OPTIONAL
    OUT     LPVOID          pvData,             OPTIONAL
    IN OUT  LPDWORD         pcbData             OPTIONAL
)
{
    LONG lRet       = ERROR_SUCCESS;

    ASSERT(IS_HUSKEY_VALID(pUSKey));    

    // It may be necessary to open the key
    if (NULL == *phkey)
        lRet = PrivFullOpen(pUSKey);

    if ((ERROR_SUCCESS == lRet) && (*phkey))
    {
        if (fWideChar)
            lRet = SHEnumValueW(*phkey, dwIndex, pwzValueName, pcchValueName, pdwType, pvData, pcbData);
        else
            lRet = SHEnumValueA(*phkey, dwIndex, (LPSTR)pwzValueName, pcchValueName, pdwType, pvData, pcbData);
    }
    else
        lRet = ERROR_INVALID_PARAMETER;

    return lRet;
}

// Query the Key information.
LONG PrivRegQueryInfoKey(
    IN  PUSKEY      pUSKey,
    IN  HKEY        *phkey,
    IN  BOOL        fWideChar,
    OUT LPDWORD     pcSubKeys,             OPTIONAL
    OUT LPDWORD     pcchMaxSubKeyLen,      OPTIONAL
    OUT LPDWORD     pcValues,              OPTIONAL
    OUT LPDWORD     pcchMaxValueNameLen    OPTIONAL
)
{
    LONG lRet       = ERROR_SUCCESS;

    ASSERT(IS_HUSKEY_VALID(pUSKey));

    if (NULL == *phkey)
        lRet = PrivFullOpen(pUSKey);

    if ((ERROR_SUCCESS == lRet) && (*phkey))
    {
        if (fWideChar)
            lRet = SHQueryInfoKeyW(*phkey, pcSubKeys, pcchMaxSubKeyLen, pcValues, pcchMaxValueNameLen);
        else
            lRet = SHQueryInfoKeyA(*phkey, pcSubKeys, pcchMaxSubKeyLen, pcValues, pcchMaxValueNameLen);
    }
    else
        lRet = ERROR_INVALID_PARAMETER;

    return lRet;
}

/*----------------------------------------------------------
Purpose: Create or open a user specifc registry key (HUSKEY).  

Description: This function will:
    1. Allocate a new USKEY structure.
    2. Initialize the structure.
    3. Create/Open HKLM if that flag is set.
    4. Create/Open HKCU if that flag is set.

    Note that there is no difference between FORCE and
    don't force in the dwFlags parameter.

    The hUSKeyRelative parameter should have also been opened by
    a call to SHRegCreateUSKey.  If SHRegOpenUSKey was called,
    it could have returned ERROR_SUCCESS but still be invalid
    for calling this function.  This will occur if: 1) the parameter
    fIgnoreHKCU was FALSE, 2) it was a relative open, 3) the
    HKCU branch could not be opened because it didn't exist, and
    4) HKLM opened successfully.  This situation renders the
    HUSKEY valid for reading but not writing.

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegCreateUSKeyA(
    IN  LPCSTR          pszPath,         
    IN  REGSAM          samDesired,	// security access mask 
    IN  HUSKEY          hUSKeyRelative,	       OPTIONAL
    OUT PHUSKEY         phUSKey,
    IN  DWORD           dwFlags)     // Indicates whether to create/open HKCU, HKLM, or both
{
    PUSKEY      pUSKeyRelative      = (PUSKEY) hUSKeyRelative;
    PPUSKEY     ppUSKey             = (PPUSKEY) phUSKey;
    PUSKEY      pUSKey;
    LONG        lRet                = ERROR_SUCCESS;
    CHAR        szTempPath[MAXIMUM_SUB_KEY_LENGTH]  = "\0";
    LPCSTR      lpszHKLMPath        = szTempPath;
    LPCSTR      lpszHKCUPath        = szTempPath;

    ASSERT(ppUSKey);
    // The following are invalid parameters...
    // 1. ppUSKey cannot be NULL
    // 2. If this is a relative open, pUSKeyRelative needs to be a valid HUSKEY.
    // 3. The user needs to have specified one of the following: SHREGSET_HKCU, SHREGSET_FORCE_HKCU, SHREGSET_HKLM, SHREGSET_FORCE_HKLM.
    if ((!ppUSKey) ||                                                   // 1.
        (pUSKeyRelative && FALSE == IS_HUSKEY_VALID(pUSKeyRelative)) || // 2.
        !(dwFlags & (SHREGSET_HKCU | SHREGSET_FORCE_HKCU  | SHREGSET_HKLM | SHREGSET_FORCE_HKLM))) // 3.
    {
        return ERROR_INVALID_PARAMETER;
    }

    // The temp path will be used when bringing the keys
    // up todate that was out of date in the Relative key.
    if (pUSKeyRelative)
    {
        lstrcpyA(szTempPath, pUSKeyRelative->szSubPath);
        
        PathAddBackslashA(szTempPath);
    }

    StrNCatA(szTempPath, pszPath, ARRAYSIZE(szTempPath) - lstrlenA(szTempPath) - 1);

    /////  1. Allocate a new USKEY structure.
    pUSKey = *ppUSKey = (PUSKEY)LocalAlloc(LPTR, SIZEOF(USKEY));
    if (!pUSKey)
        return ERROR_NOT_ENOUGH_MEMORY;

    /////  2. Initialize the structure.
    if (!pUSKeyRelative)
    {
        // Init a new (non-relative) open.
        pUSKey->hkeyLocalMachineRelative    = HKEY_LOCAL_MACHINE;
        pUSKey->hkeyCurrentUserRelative     = HKEY_CURRENT_USER;
    }
    else
    {
        // Init a new (relative) open.
        *pUSKey = *pUSKeyRelative;

        if (pUSKey->hkeyLocalMachine)
        {
            pUSKey->hkeyLocalMachineRelative = pUSKey->hkeyLocalMachine;
            pUSKey->hkeyLocalMachine = NULL;
            lpszHKLMPath = pszPath;

            // This key is up to date in the Relative Key.  If the
            // user doesn't want it to be up todate in the new key,
            // we don't need the path from the Relative key.
            if (!(dwFlags & (SHREGSET_HKLM | SHREGSET_FORCE_HKLM)))
                *(pUSKey->szSubPath) = '\0';
        }
        // We need to copy the key if:
        // 1. It will not be created in this call, and
        // 2. The relative key is not HKEY_LOCAL_MACHINE.
        if (!(dwFlags & (SHREGSET_HKLM | SHREGSET_FORCE_HKLM)) &&
            (pUSKey->hkeyLocalMachineRelative != HKEY_LOCAL_MACHINE))
        {
            // Make a duplicate of this key.
            lRet = RegOpenKeyExA(pUSKey->hkeyLocalMachineRelative, NULL, 0, pUSKey->samDesired, &(pUSKey->hkeyLocalMachineRelative));
        }

        if (pUSKey->hkeyCurrentUser)
        {
            pUSKey->hkeyCurrentUserRelative = pUSKey->hkeyCurrentUser;
            pUSKey->hkeyCurrentUser = NULL;
            lpszHKCUPath = pszPath;

            // This key is up to date in the Relative Key.  If the
            // user doesn't want it to be up todate in the new key,
            // we don't need the path from the Relative key.
            if (!(dwFlags & (SHREGSET_HKCU | SHREGSET_FORCE_HKCU)))
                *(pUSKey->szSubPath) = '\0';
        }
        // We need to copy the key if:
        // 1. It will not be created in this call, and
        // 2. The relative key is not HKEY_CURRENT_USER.
        if (!(dwFlags & (SHREGSET_HKCU | SHREGSET_FORCE_HKCU)) &&
            (pUSKey->hkeyCurrentUserRelative != HKEY_CURRENT_USER))
        {
            // Make a duplicate of this key.
            lRet = RegOpenKeyExA(pUSKey->hkeyCurrentUserRelative, NULL, 0, pUSKey->samDesired, &(pUSKey->hkeyCurrentUserRelative));
        }
    }
    pUSKey->samDesired = samDesired;


    /////  3. Create/Open HKLM if that flag is set or fill in the structure as appropriate.
    if ((ERROR_SUCCESS == lRet) && (dwFlags & (SHREGSET_HKLM | SHREGSET_FORCE_HKLM)))
        lRet = PrivCreateKey(&(pUSKey->hkeyLocalMachine), &(pUSKey->hkeyLocalMachineRelative), lpszHKLMPath, pUSKey->samDesired);

    /////  4. Create/Open HKCU if that flag is set or fill in the structure as appropriate.
    if ((ERROR_SUCCESS == lRet) && (dwFlags & (SHREGSET_HKCU | SHREGSET_FORCE_HKCU)))
        lRet = PrivCreateKey(&(pUSKey->hkeyCurrentUser), &(pUSKey->hkeyCurrentUserRelative), lpszHKCUPath, pUSKey->samDesired);

    if ((dwFlags & (SHREGSET_HKCU | SHREGSET_FORCE_HKCU)) &&
        (dwFlags & (SHREGSET_HKLM | SHREGSET_FORCE_HKLM))) 
    {
        // The caller wanted both to be opened.
        *(pUSKey->szSubPath) = '\0';       // Both paths are open so Delta Path is empty.
    }
    else
    {
        // One of the paths is not open so set the Delta Path in case it needs to be opened later.
        if (*(pUSKey->szSubPath))
            PathAddBackslashA(pUSKey->szSubPath);

        StrNCatA(pUSKey->szSubPath, pszPath, ARRAYSIZE(pUSKey->szSubPath) - lstrlenA(pUSKey->szSubPath) - 1);
    }

    // Free the memory if we are not successful.
    if (ERROR_SUCCESS != lRet)
    {
        if (pUSKey->hkeyCurrentUser)
            RegCloseKey(pUSKey->hkeyCurrentUser);
        if (pUSKey->hkeyCurrentUserRelative && pUSKey->hkeyCurrentUserRelative != HKEY_CURRENT_USER)
            RegCloseKey(pUSKey->hkeyCurrentUserRelative);
        if (pUSKey->hkeyLocalMachine)
            RegCloseKey(pUSKey->hkeyLocalMachine);
        if (pUSKey->hkeyLocalMachineRelative && pUSKey->hkeyLocalMachineRelative != HKEY_LOCAL_MACHINE)
            RegCloseKey(pUSKey->hkeyLocalMachineRelative);
        LocalFree((HLOCAL)pUSKey);
        *ppUSKey = NULL;
    }

    return lRet;
}







/*----------------------------------------------------------
Purpose: Create or open a user specifc registry key (HUSKEY).  

Description: This function will:
    1. Allocate a new USKEY structure.
    2. Initialize the structure.
    3. Create/Open HKLM if that flag is set.
    4. Create/Open HKCU if that flag is set.

    Note that there is no difference between FORCE and
    don't force in the dwFlags parameter.

    The hUSKeyRelative parameter should have also been opened by
    a call to SHRegCreateUSKey.  If SHRegOpenUSKey was called,
    it could have returned ERROR_SUCCESS but still be invalid
    for calling this function.  This will occur if: 1) the parameter
    fIgnoreHKCU was FALSE, 2) it was a relative open, 3) the
    HKCU branch could not be opened because it didn't exist, and
    4) HKLM opened successfully.  This situation renders the
    HUSKEY valid for reading but not writing.

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegCreateUSKeyW(
    IN  LPCWSTR         pwzPath,
    IN  REGSAM          samDesired,	// security access mask 
    IN  HUSKEY          hUSKeyRelative,	       OPTIONAL
    OUT PHUSKEY         phUSKey,
    IN  DWORD           dwFlags)     // Indicates whether to create/open HKCU, HKLM, or both
{
    CHAR   szNewPath[MAXIMUM_SUB_KEY_LENGTH];

    // Thunk Path to Wide chars.
    if (FALSE == WideCharToMultiByte(CP_ACP, 0, pwzPath, -1, szNewPath, SIZEOF(szNewPath), NULL, 0))
        return GetLastError();

    return SHRegCreateUSKeyA(szNewPath, samDesired, hUSKeyRelative, phUSKey, dwFlags);
}



/*----------------------------------------------------------
Purpose: Open a user specifc registry key (HUSKEY).  

Description: This function will:
    1. Allocate a new USKEY structure.
    2. Initialize the structure.
    3. Determine which key (HKLM or HKCU) will be the one brought up to date.
    4. Open the key that is going to be brought up to date.

    If #4 Succeeded:
    5a. Copy the handle of the out of date key, so it can be opened later if needed.

    If #4 Failed:
    5b. The other key will now be the one brought up to date, as long as it is HKLM.
    6b. Tag the out of date as INVALID. (Key == NULL; RelKey == NULL)

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegOpenUSKeyA(
    IN  LPCSTR          pszPath,         
    IN  REGSAM          samDesired,	// security access mask 
    IN  HUSKEY          hUSKeyRelative,	       OPTIONAL
    OUT PHUSKEY         phUSKey,     
    IN  BOOL            fIgnoreHKCU)           
{
    PUSKEY      pUSKeyRelative      = (PUSKEY) hUSKeyRelative;
    PPUSKEY     ppUSKey             = (PPUSKEY) phUSKey;
    PUSKEY      pUSKey;
    LONG        lRet               = ERROR_SUCCESS;
    HKEY        * phkeyMaster;
    HKEY        * phkeyRelMaster;
    HKEY        * phkeyOld;
    HKEY        * phkeyRelOld;

    ASSERT(ppUSKey);

    // The following are invalid parameters...
    // 1. ppUSKey cannot be NULL
    // 2. If this is a relative open, pUSKeyRelative needs to be a valid HUSKEY.
    if ((!ppUSKey) ||                                                   // 1.
        (pUSKeyRelative && FALSE == IS_HUSKEY_VALID(pUSKeyRelative)))   // 2.
    {
        return ERROR_INVALID_PARAMETER;
    }


    /////  1. Allocate a new USKEY structure.
    pUSKey = *ppUSKey = (PUSKEY)LocalAlloc(LPTR, SIZEOF(USKEY));
    if (!pUSKey)
        return ERROR_NOT_ENOUGH_MEMORY;

    /////  2. Initialize the structure.
    if (!pUSKeyRelative)
    {
        // Init a new (non-relative) open.
        pUSKey->hkeyLocalMachineRelative    = HKEY_LOCAL_MACHINE;
        pUSKey->hkeyCurrentUserRelative     = HKEY_CURRENT_USER;
    }
    else
    {
        // Init a new (relative) open.
        *pUSKey = *pUSKeyRelative;
    }
    pUSKey->samDesired = samDesired;


    /////  3. Determine which key (HKLM or HKCU) will be the one brought up to date.
    // The HUSKY struct will contain 4 HKEYs. HKCU, HKCU Relative, HKLM, and HKLM Relative.
    // For efficiency, only one key will be up to date (HKCU or HKLM).  The one that
    // is out of date will be NULL to indicate out of date.  The relative key for the
    // out of date key, will be the last opened key.  The string will be the delta between
    // the last open key and the current open level.

    // We will determine which key will be the new valid key (Master).
    if (FALSE == fIgnoreHKCU)
    {
        phkeyMaster     = &(pUSKey->hkeyCurrentUser);
        phkeyRelMaster  = &(pUSKey->hkeyCurrentUserRelative);
        phkeyOld        = &(pUSKey->hkeyLocalMachine);
        phkeyRelOld     = &(pUSKey->hkeyLocalMachineRelative);
    }
    else
    {
        phkeyMaster     = &(pUSKey->hkeyLocalMachine);
        phkeyRelMaster  = &(pUSKey->hkeyLocalMachineRelative);
        phkeyOld        = &(pUSKey->hkeyCurrentUser);
        phkeyRelOld     = &(pUSKey->hkeyCurrentUserRelative);
    }

    // Add the new Path to the Total path.
    if ('\0' != *(pUSKey->szSubPath)) 
        PathAddBackslashA(pUSKey->szSubPath);  // Add separator \ if reqd. 

    StrNCatA(pUSKey->szSubPath, pszPath, ARRAYSIZE(pUSKey->szSubPath) - lstrlenA(pUSKey->szSubPath) - 1);

    /////  4. Open the key that is going to be brought up to date.
    if (*phkeyMaster)
    {
        // Masterkey is already up to date, so just do the relative open and add the string to szSubPath
        // It's safe to write write (*phkeyMaster) because it will be freed by the HUSKEY used for the
        // relative open.
        lRet = RegOpenKeyExA(*phkeyMaster, pszPath, 0, pUSKey->samDesired, phkeyMaster);
    }
    else
    {

        // Open Masterkey with the full path (pUSKey->szSubPath + pszPath)
        if (*phkeyRelMaster)
            lRet = RegOpenKeyExA(*phkeyRelMaster, pUSKey->szSubPath, 0, pUSKey->samDesired, phkeyMaster);
        else
            lRet = ERROR_FILE_NOT_FOUND;

        lstrcpyA(pUSKey->szSubPath, pszPath);
        *phkeyRelMaster = NULL;
    }

    /////  Did #4 Succeeded?
    if (ERROR_FILE_NOT_FOUND == lRet)
    {
        /////  #4 Failed, Now we can try to open HKLM if the previous attempt was to open HKCU.
        if (!fIgnoreHKCU)
        {
            if (*phkeyRelOld)       // Can HKLM be opened?
            {
                ASSERT(*phkeyOld == NULL);       // *phkeyOld should never have a value if *phkeyRelOld does.

                /////  5b. The other key will now be the one brought up to date, as long as it is HKLM.
                lRet = RegOpenKeyExA(*phkeyRelOld, pUSKey->szSubPath, 0, pUSKey->samDesired, phkeyOld);
                *phkeyRelOld = NULL;
            }
            else if (*phkeyOld)       // Can HKLM be opened?
            {
                /////  5b. Attempt to bring the other key up to date.
                lRet = RegOpenKeyExA(*phkeyOld, pUSKey->szSubPath, 0, pUSKey->samDesired, phkeyOld);
            }
        }
        else
        {
            *phkeyOld = NULL;            // Tag this as INVALID
            *phkeyRelOld = NULL;         // Tag this as INVALID
        }

        /////  6b. Tag the out of date as INVALID. (Key == NULL; RelKey == NULL)
        *phkeyMaster = NULL;            // Tag this as INVALID
        *phkeyRelMaster = NULL;         // Tag this as INVALID
    }
    else
    {
        /////  #4 Succeeded:
        /////  5a. Does the out of date key need to be copied?
        if (*phkeyOld)
        {
            // Copy the handle of the out of date key, so it can be opened later if needed.
            // We can be assured that any NON-Relative HKEY will not be HKEY_LOCAL_MACHINE or HKEY_CURRENT_USER
            ASSERT(*phkeyOld != HKEY_LOCAL_MACHINE && *phkeyOld != HKEY_CURRENT_USER);       // But let's assert anyway.

            RegOpenKeyExA(*phkeyOld, NULL, 0, pUSKey->samDesired, phkeyOld);
        }
        else
        {
            if ((*phkeyRelOld) && (*phkeyRelOld != HKEY_LOCAL_MACHINE) && (*phkeyRelOld != HKEY_CURRENT_USER))
            {
                // Copy the handle of the out of date key, so it can be opened later if needed.
                lRet = RegOpenKeyExA(*phkeyRelOld, NULL, 0, pUSKey->samDesired, phkeyRelOld);
            }
        }

        if (*phkeyOld)
        {
            *phkeyRelOld = *phkeyOld;
            *phkeyOld = NULL;        // Mark this key as being out of date.
        }
    }

    // Free the memory if we are not successful.
    if (ERROR_SUCCESS != lRet)
    {
        pUSKey->hkeyCurrentUser     = NULL;     // Mark invalid.
        pUSKey->hkeyLocalMachine    = NULL;
        LocalFree((HLOCAL)pUSKey);
        *ppUSKey = NULL;
    }

    return lRet;
}







/*----------------------------------------------------------
Purpose: Open a user specifc registry key (HUSKEY).  

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegOpenUSKeyW(
    IN  LPCWSTR         pwzPath,         
    IN  REGSAM          samDesired,	// security access mask 
    IN  HUSKEY          hUSKeyRelative,	       OPTIONAL
    OUT PHUSKEY         phUSKey,     
    IN  BOOL            fIgnoreHKCU)           
{
    CHAR   szNewPath[MAXIMUM_SUB_KEY_LENGTH];

    // Thunk Path to Wide chars.
    if (FALSE == WideCharToMultiByte(CP_ACP, 0, pwzPath, -1, szNewPath, SIZEOF(szNewPath), NULL, 0))
        return GetLastError();

    return SHRegOpenUSKeyA(szNewPath, samDesired, hUSKeyRelative, phUSKey, fIgnoreHKCU);
}



/*----------------------------------------------------------
Purpose: Query a user specific registry entry for it's value.  
         This will NOT
         open and close the keys in which the value resides. 
         The caller needs to do this and it should be done
         when several keys will be queried for a perf increase.
         Callers that only call this once, will probably want
         to call SHGetUSValue().

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegQueryUSValueA(
    IN  HUSKEY          hUSKey,
    IN  LPCSTR          pszValue,           
    OUT LPDWORD         pdwType,            OPTIONAL
    OUT LPVOID          pvData,             OPTIONAL
    OUT LPDWORD         pcbData,            OPTIONAL
    IN  BOOL            fIgnoreHKCU,
    IN  LPVOID          pvDefaultData,      OPTIONAL
    IN  DWORD           dwDefaultDataSize)  OPTIONAL
{
    PUSKEY      pUSKey      = (PUSKEY) hUSKey;
    LONG        lRet        = ERROR_SUCCESS;
    DWORD       dwSize      = (pcbData ? *pcbData : 0);

    if (FALSE == IS_HUSKEY_VALID(pUSKey))
        return ERROR_INVALID_PARAMETER;

    if (!fIgnoreHKCU)
    {
        lRet = PrivRegQueryValue(pUSKey, &(pUSKey->hkeyCurrentUser), (LPWSTR)pszValue, 
                                    FALSE, pdwType, pvData, pcbData);
    }
    if (fIgnoreHKCU || ERROR_SUCCESS != lRet)
    {
        if (pcbData)
            *pcbData = dwSize;  // We may need to reset if previous open failed.

        lRet = PrivRegQueryValue(pUSKey, &(pUSKey->hkeyLocalMachine), (LPWSTR)pszValue, 
                                    FALSE, pdwType, pvData, pcbData);
    }

    // if fail, use default value.
    if ((ERROR_SUCCESS != lRet) && (pvDefaultData) && (dwDefaultDataSize) && 
        (pvData) && (dwSize >= dwDefaultDataSize))
    {
        MoveMemory(pvData, pvDefaultData, dwDefaultDataSize);
        if (pcbData)
            *pcbData = dwDefaultDataSize;
        lRet = ERROR_SUCCESS;       // Call will now use a default value.
    }

    return lRet;
}



/*----------------------------------------------------------
Purpose: Query a user specific registry entry for it's value.    
         This will NOT
         open and close the keys in which the value resides. 
         The caller needs to do this and it should be done
         when several keys will be queried for a perf increase.
         Callers that only call this once, will probably want
         to call SHGetUSValue().

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegQueryUSValueW(
    IN  HUSKEY          hUSKey,
    IN  LPCWSTR         pwzValue,           
    OUT LPDWORD         pdwType,            OPTIONAL
    OUT LPVOID          pvData,             OPTIONAL
    OUT LPDWORD         pcbData,            OPTIONAL
    IN  BOOL            fIgnoreHKCU,
    IN  LPVOID          pvDefaultData,      OPTIONAL
    IN  DWORD           dwDefaultDataSize)  OPTIONAL
{
    PUSKEY      pUSKey      = (PUSKEY) hUSKey;
    LONG        lRet;
    DWORD       dwSize      = (pcbData ? *pcbData : 0);

    if (FALSE == IS_HUSKEY_VALID(pUSKey))
        return ERROR_INVALID_PARAMETER;

    if (!fIgnoreHKCU)
    {
        lRet = PrivRegQueryValue(pUSKey, &(pUSKey->hkeyCurrentUser), pwzValue, 
                                    TRUE, pdwType, pvData, pcbData);
    }
    if (fIgnoreHKCU || ERROR_SUCCESS != lRet)
    {
        if (pcbData)
            *pcbData = dwSize;  // We may need to reset if previous open failed.
        lRet = PrivRegQueryValue(pUSKey, &(pUSKey->hkeyLocalMachine), pwzValue, 
                                    TRUE, pdwType, pvData, pcbData);
    }

    // if fail, use default value.
    if ((ERROR_SUCCESS != lRet) && (pvDefaultData) && (dwDefaultDataSize) && 
        (pvData) && (dwSize >= dwDefaultDataSize))
    {
        MoveMemory(pvData, pvDefaultData, dwDefaultDataSize);
        if (pcbData)
            *pcbData = dwDefaultDataSize;
        lRet = ERROR_SUCCESS;       // Call will now use a default value.
    }

    return lRet;
}






/*----------------------------------------------------------
Purpose: Write a user specific registry entry.  

Parameters:
  hUSKey - Needs to have been open with KEY_SET_VALUE permissions.
           KEY_QUERY_VALUE also needs to have been used if this is
           not a force write.
  pszValue - Registry Key value to write to.
  dwType - Type for the new registry key.
  pvData - Pointer to data to store
  cbData - Size of data to store.  
  dwFlags - Flags to determine if the registry entry should be written to
            HKLM, HKCU, or both.  Also determines if these are force or
            non-force writes. (non-force means it will only write the value
            if it's empty)  Using FORCE is faster than non-force.

Decription:
    This function will write the value to the
    registry in either the HKLM or HKCU branches depending
    on the flags set in the dwFlags parameter.
    
Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegWriteUSValueA(
    IN  HUSKEY          hUSKey,
    IN  LPCSTR          pszValue,           
    IN  DWORD           dwType,
    IN  LPCVOID         pvData,
    IN  DWORD           cbData,
    IN  DWORD           dwFlags)
{
    PUSKEY      pUSKey      = (PUSKEY) hUSKey;
    LONG        lRet        = ERROR_SUCCESS;

    if (FALSE == IS_HUSKEY_VALID(pUSKey))
        return ERROR_INVALID_PARAMETER;

    // Assert if: 1) This is not a force open, and 2) they key was not
    // opened with KEY_QUERY_VALUE permissions.
    if (!(dwFlags & (SHREGSET_FORCE_HKCU|SHREGSET_FORCE_HKLM)) && !(pUSKey->samDesired & KEY_QUERY_VALUE))
    {
        ASSERT(NULL);   // ERROR_INVALID_PARAMETER
        return(ERROR_INVALID_PARAMETER);
    }

    if (dwFlags & (SHREGSET_HKCU | SHREGSET_FORCE_HKCU))
    {
        lRet = PrivRegWriteValue(pUSKey, &(pUSKey->hkeyCurrentUser), (LPWSTR)pszValue, 
            FALSE, dwFlags & SHREGSET_FORCE_HKCU, dwType, pvData, cbData);
    }
    if ((dwFlags & (SHREGSET_HKLM | SHREGSET_FORCE_HKLM)) && (ERROR_SUCCESS == lRet))
    {
        lRet = PrivRegWriteValue(pUSKey, &(pUSKey->hkeyLocalMachine), (LPWSTR)pszValue, 
            FALSE, dwFlags & SHREGSET_FORCE_HKLM, dwType, pvData, cbData);
    }

    return lRet;
}



/*----------------------------------------------------------
Purpose: Write a user specific registry entry.  

Parameters:
  hUSKey - Needs to have been open with KEY_SET_VALUE permissions.
           KEY_QUERY_VALUE also needs to have been used if this is
           not a force write.
  pszValue - Registry Key value to write to.
  dwType - Type for the new registry key.
  pvData - Pointer to data to store
  cbData - Size of data to store.  
  dwFlags - Flags to determine if the registry entry should be written to
            HKLM, HKCU, or both.  Also determines if these are force or
            non-force writes. (non-force means it will only write the value
            if it's empty)  Using FORCE is faster than non-force.

Decription:
    This function will write the value to the
    registry in either the HKLM or HKCU branches depending
    on the flags set in the dwFlags parameter.
    
Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegWriteUSValueW(
    IN  HUSKEY          hUSKey,
    IN  LPCWSTR         pwzValue,           
    IN  DWORD           dwType,
    IN  LPCVOID         pvData,
    IN  DWORD           cbData,
    IN  DWORD           dwFlags)
{
    PUSKEY      pUSKey      = (PUSKEY) hUSKey;
    LONG        lRet        = ERROR_SUCCESS;

    if (FALSE == IS_HUSKEY_VALID(pUSKey))
        return ERROR_INVALID_PARAMETER;

    // Assert if: 1) This is not a force open, and 2) they key was not
    // opened with access permissions.
    if (!(dwFlags & (SHREGSET_FORCE_HKCU|SHREGSET_FORCE_HKLM)) && !(pUSKey->samDesired & KEY_QUERY_VALUE))
    {
        ASSERT(NULL);   // ERROR_INVALID_PARAMETER
        return(ERROR_INVALID_PARAMETER);
    }

    if (dwFlags & (SHREGSET_HKCU | SHREGSET_FORCE_HKCU))
    {
        lRet = PrivRegWriteValue(pUSKey, &(pUSKey->hkeyCurrentUser), (LPWSTR)pwzValue, 
                                    TRUE, dwFlags & SHREGSET_FORCE_HKCU, dwType, pvData, cbData);
    }
    if (dwFlags & (SHREGSET_HKLM | SHREGSET_FORCE_HKLM))
    {
        lRet = PrivRegWriteValue(pUSKey, &(pUSKey->hkeyLocalMachine), (LPWSTR)pwzValue, 
                                    TRUE, dwFlags & SHREGSET_FORCE_HKLM, dwType, pvData, cbData);
    }

    return lRet;
}





/*----------------------------------------------------------
Purpose: Deletes a registry value.  This will delete HKLM,
         HKCU, or both depending on the hkey parameter. 

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegDeleteUSValueA(
    IN  HUSKEY          hUSKey,
    IN  LPCSTR          pszValue,           
    IN  SHREGDEL_FLAGS  delRegFlags)               // (HKLM, HKCU, or (HKLM | HKCU))
{
    PUSKEY      pUSKey      = (PUSKEY) hUSKey;
    LONG        lRet        = ERROR_INVALID_PARAMETER;

    if (FALSE == IS_HUSKEY_VALID(pUSKey))
        return ERROR_INVALID_PARAMETER;

    if (SHREGDEL_DEFAULT == delRegFlags)        // Delete whatever keys are open
    {
        if (!pUSKey->hkeyCurrentUser)  // Attempt to open HKCU if not currently open
            lRet = PrivFullOpen(pUSKey);

        if (pUSKey->hkeyCurrentUser)
            delRegFlags = SHREGDEL_HKCU;
        else
        {
            // We prefer to delete HKCU, but we got here, so we will delete HKLM
            // if it is open.
            if (pUSKey->hkeyLocalMachine)
                delRegFlags = SHREGDEL_HKLM;
        }
    }

    if (IsFlagSet(delRegFlags, SHREGDEL_HKCU))        // Check if the call wants to delete the HKLM value.
    {
        if (!pUSKey->hkeyCurrentUser)
            PrivFullOpen(pUSKey);
        if (pUSKey->hkeyCurrentUser)
        {
            lRet = RegDeleteValueA(pUSKey->hkeyCurrentUser, pszValue);
            if (ERROR_FILE_NOT_FOUND == lRet)
                delRegFlags = SHREGDEL_HKLM;        // Delete the HKLM value if the HKCU value wasn't found.
        }
    }

    if (IsFlagSet(delRegFlags, SHREGDEL_HKLM))        // Check if the call wants to delete the HKLM value.
    {
        if (!pUSKey->hkeyLocalMachine)
            PrivFullOpen(pUSKey);
        if (pUSKey->hkeyLocalMachine)
            lRet = RegDeleteValueA(pUSKey->hkeyLocalMachine, pszValue);
    }

    return lRet;
}




/*----------------------------------------------------------
Purpose: Deletes a registry value.  This will delete HKLM,
         HKCU, or both depending on the hkey parameter. 

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegDeleteUSValueW(
    IN  HUSKEY          hUSKey,
    IN  LPCWSTR         pwzValue,           
    IN  SHREGDEL_FLAGS  delRegFlags)               // (HKLM, HKCU, or (HKLM | HKCU))
{
    CHAR   szNewPath[MAXIMUM_VALUE_NAME_LENGTH];

    // Thunk Path to Wide chars.
    if (FALSE == WideCharToMultiByte(CP_ACP, 0, pwzValue, -1, szNewPath, SIZEOF(szNewPath), NULL, 0))
        return GetLastError();

    return SHRegDeleteUSValueA(hUSKey, szNewPath, delRegFlags);
}


/*----------------------------------------------------------
Purpose: Deletes a registry sub-key if empty.  This will delete HKLM,
         HKCU, or both depending on the delRegFlags parameter. 

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegDeleteEmptyUSKeyA(
    IN  HUSKEY          hUSKey,
    IN  LPCSTR          pszSubKey,           
    IN  SHREGDEL_FLAGS  delRegFlags)               // (HKLM, HKCU, or (HKLM | HKCU))
{
    PUSKEY      pUSKey      = (PUSKEY) hUSKey;
    LONG        lRet        = ERROR_INVALID_PARAMETER;

    if (FALSE == IS_HUSKEY_VALID(pUSKey))
        return ERROR_INVALID_PARAMETER;

    if (SHREGDEL_DEFAULT == delRegFlags)        // Delete whatever keys are open
    {
        if (!pUSKey->hkeyCurrentUser)  // Attempt to open HKCU if not currently open
            lRet = PrivFullOpen(pUSKey);

        if (pUSKey->hkeyCurrentUser)
            delRegFlags = SHREGDEL_HKCU;
        else
        {
            // We prefer to delete HKCU, but we got here, so we will delete HKLM
            // if it is open.
            if (pUSKey->hkeyLocalMachine)
                delRegFlags = SHREGDEL_HKLM;
        }
    }

    if (IsFlagSet(delRegFlags, SHREGDEL_HKCU))        // Check if the call wants to delete the HKLM key.
    {
        if (!pUSKey->hkeyCurrentUser)
            PrivFullOpen(pUSKey);
        if (pUSKey->hkeyCurrentUser)
        {
            lRet = SHDeleteEmptyKeyA(pUSKey->hkeyCurrentUser, pszSubKey);
            if (ERROR_FILE_NOT_FOUND == lRet)
                delRegFlags = SHREGDEL_HKLM;        // Delete the HKLM key if the HKCU key wasn't found.
        }
    }

    if (IsFlagSet(delRegFlags, SHREGDEL_HKLM))        // Check if the call wants to delete the HKLM key.
    {
        if (!pUSKey->hkeyLocalMachine)
            PrivFullOpen(pUSKey);
        if (pUSKey->hkeyLocalMachine)
            lRet = SHDeleteEmptyKeyA(pUSKey->hkeyLocalMachine, pszSubKey);
    }

    return lRet;
}




/*----------------------------------------------------------
Purpose: Deletes a registry key if empty.  This will delete HKLM,
         HKCU, or both depending on the delRegFlags parameter. 

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegDeleteEmptyUSKeyW(
    IN  HUSKEY          hUSKey,
    IN  LPCWSTR         pwzSubKey,           
    IN  SHREGDEL_FLAGS  delRegFlags)               // (HKLM, HKCU, or (HKLM | HKCU))
{
    CHAR   szNewPath[MAXIMUM_SUB_KEY_LENGTH];

    // Thunk Path to Wide chars.
    if (FALSE == WideCharToMultiByte(CP_ACP, 0, pwzSubKey, -1, szNewPath, SIZEOF(szNewPath), NULL, 0))
        return GetLastError();

    return SHRegDeleteEmptyUSKeyA(hUSKey, szNewPath, delRegFlags);
}


/*----------------------------------------------------------
Purpose: Enumerates sub-keys under a given HUSKEY.
         
         SHREGENUM_FLAGS specifies how to do the enumeration.
         SHREGENUM_DEFAULT - Will look in HKCU followed by HKLM if not found.
         SHREGENUM_HKCU - Enumerates HKCU only.
         SHREGENUM_HKLM = Enumerates HKLM only.
         SHREGENUM_BOTH - This is supposed to do a union of the HKLM and HKCU subkeys. 

Returns: LONG containing success or error code.
Cond:    --
*/

STDAPI_(LONG)
SHRegEnumUSKeyA(
    IN  HUSKEY          hUSKey,
    IN  DWORD           dwIndex,
    OUT LPSTR           pszName,
    IN  LPDWORD         pcchName,           
    IN  SHREGENUM_FLAGS enumRegFlags)               // (HKLM, HKCU, or (HKLM | HKCU))
{
    PUSKEY      pUSKey      = (PUSKEY) hUSKey;
    LONG        lRet        = ERROR_INVALID_PARAMETER;

    if (FALSE == IS_HUSKEY_VALID(pUSKey))
        return ERROR_INVALID_PARAMETER;

    if (SHREGENUM_BOTH == enumRegFlags)             
    {
        // This is not supported yet. 
        ASSERT(FALSE);
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    if (SHREGENUM_HKCU != enumRegFlags && SHREGENUM_HKLM != enumRegFlags && SHREGENUM_DEFAULT != enumRegFlags)
    {
        // check your arguments.
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    // Default is to try HKCU first.
    if (SHREGENUM_HKCU == enumRegFlags || SHREGENUM_DEFAULT == enumRegFlags)
    {
        lRet = PrivRegEnumKey(pUSKey, &(pUSKey->hkeyCurrentUser), dwIndex,
                                (LPWSTR)pszName, FALSE, pcchName);
    }

    if ((SHREGENUM_HKLM == enumRegFlags) || 
        ((SHREGENUM_DEFAULT == enumRegFlags) && ((ERROR_SUCCESS != lRet) && (ERROR_MORE_DATA != lRet) && (ERROR_NO_MORE_ITEMS != lRet))))
    {
        lRet = PrivRegEnumKey(pUSKey, &(pUSKey->hkeyLocalMachine), dwIndex,
                                (LPWSTR)pszName, FALSE, pcchName);
    }

    return lRet;
}


/*----------------------------------------------------------
Purpose: Enumerates sub-keys under a given HUSKEY.
         
         SHREGENUM_FLAGS specifies how to do the enumeration.
         SHREGENUM_DEFAULT - Will look in HKCU followed by HKLM if not found.
         SHREGENUM_HKCU - Enumerates HKCU only.
         SHREGENUM_HKLM = Enumerates HKLM only.
         SHREGENUM_BOTH - This is supposed to do a union of the HKLM and HKCU subkeys. 

Returns: LONG containing success or error code.
Cond:    --
*/

STDAPI_(LONG)
SHRegEnumUSKeyW(
    IN  HUSKEY          hUSKey,
    IN  DWORD           dwIndex,
    OUT LPWSTR          pszName,
    IN  LPDWORD         pcchName,           
    IN  SHREGENUM_FLAGS enumRegFlags)               // (HKLM, HKCU, or (HKLM | HKCU))
{
    PUSKEY      pUSKey      = (PUSKEY) hUSKey;
    LONG        lRet        = ERROR_INVALID_PARAMETER;

    if (FALSE == IS_HUSKEY_VALID(pUSKey))
        return ERROR_INVALID_PARAMETER;

    if (SHREGENUM_BOTH == enumRegFlags)             
    {
        // This is not supported yet. 
        ASSERT(FALSE);
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    if (SHREGENUM_HKCU != enumRegFlags && SHREGENUM_HKLM != enumRegFlags && SHREGENUM_DEFAULT != enumRegFlags)
    {
        // check your arguments.
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    // Default is to try HKCU first.
    if (SHREGENUM_HKCU == enumRegFlags || SHREGENUM_DEFAULT == enumRegFlags)
    {
        lRet = PrivRegEnumKey(pUSKey, &(pUSKey->hkeyCurrentUser), dwIndex,
                                pszName, TRUE, pcchName);
    }

    if ((SHREGENUM_HKLM == enumRegFlags) || 
        ((SHREGENUM_DEFAULT == enumRegFlags) && ((ERROR_SUCCESS != lRet) && (ERROR_MORE_DATA != lRet) && (ERROR_NO_MORE_ITEMS != lRet))))
    {
        lRet = PrivRegEnumKey(pUSKey, &(pUSKey->hkeyLocalMachine), dwIndex,
                                pszName, TRUE, pcchName);
    }

    return lRet;
}


/*----------------------------------------------------------
Purpose: Enumerates Values under a given HUSKEY.
         
         SHREGENUM_FLAGS specifies how to do the enumeration.
         SHREGENUM_DEFAULT - Will look in HKCU followed by HKLM if not found.
         SHREGENUM_HKCU - Enumerates HKCU only.
         SHREGENUM_HKLM = Enumerates HKLM only.
         SHREGENUM_BOTH - This is supposed to do a union of the HKLM and HKCU subkeys. 

Returns: LONG containing success or error code.
Cond:    --
*/

STDAPI_(LONG)
SHRegEnumUSValueA(
    IN  HUSKEY          hUSKey,
    IN  DWORD           dwIndex,
    OUT LPSTR           pszValueName,       
    IN  LPDWORD         pcchValueNameLen,
    OUT LPDWORD         pdwType,            OPTIONAL
    OUT LPVOID          pvData,             OPTIONAL
    OUT LPDWORD         pcbData,            OPTIONAL
    IN  SHREGENUM_FLAGS enumRegFlags)               // (HKLM, HKCU, or (HKLM | HKCU))
{
    PUSKEY      pUSKey      = (PUSKEY) hUSKey;
    LONG        lRet        = ERROR_INVALID_PARAMETER;

    if (FALSE == IS_HUSKEY_VALID(pUSKey))
        return ERROR_INVALID_PARAMETER;

    if (SHREGENUM_BOTH == enumRegFlags)             
    {
        // This is not supported yet. 
        ASSERT(FALSE);
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    if (SHREGENUM_HKCU != enumRegFlags && SHREGENUM_HKLM != enumRegFlags && SHREGENUM_DEFAULT != enumRegFlags)
    {
        // check your arguments.
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    // Default is to try HKCU first.
    if (SHREGENUM_HKCU == enumRegFlags || SHREGENUM_DEFAULT == enumRegFlags)
    {
        lRet = PrivRegEnumValue(pUSKey, &(pUSKey->hkeyCurrentUser), dwIndex,
                                (LPWSTR)pszValueName, FALSE, pcchValueNameLen, pdwType, pvData, pcbData);
    }

    if ((SHREGENUM_HKLM == enumRegFlags) || 
        ((SHREGENUM_DEFAULT == enumRegFlags) && ((ERROR_SUCCESS != lRet) && (ERROR_MORE_DATA != lRet) && (ERROR_NO_MORE_ITEMS != lRet))))
    {
        lRet = PrivRegEnumValue(pUSKey, &(pUSKey->hkeyLocalMachine), dwIndex,
                                (LPWSTR)pszValueName, FALSE, pcchValueNameLen, pdwType, pvData, pcbData);
    }

    return lRet;
}


/*----------------------------------------------------------
Purpose: Enumerates Values under a given HUSKEY.
         
         SHREGENUM_FLAGS specifies how to do the enumeration.
         SHREGENUM_DEFAULT - Will look in HKCU followed by HKLM if not found.
         SHREGENUM_HKCU - Enumerates HKCU only.
         SHREGENUM_HKLM = Enumerates HKLM only.
         SHREGENUM_BOTH - This is supposed to do a union of the HKLM and HKCU subkeys. 

Returns: LONG containing success or error code.
Cond:    --
*/

STDAPI_(LONG)
SHRegEnumUSValueW(
    IN  HUSKEY          hUSKey,
    IN  DWORD           dwIndex,
    OUT LPWSTR          pszValueName,       
    IN  LPDWORD         pcchValueNameLen,   
    OUT LPDWORD         pdwType,            OPTIONAL
    OUT LPVOID          pvData,             OPTIONAL
    OUT LPDWORD         pcbData,            OPTIONAL
    IN  SHREGENUM_FLAGS enumRegFlags)               // (HKLM, HKCU, or (HKLM | HKCU))
{
    PUSKEY      pUSKey      = (PUSKEY) hUSKey;
    LONG        lRet        = ERROR_INVALID_PARAMETER;

    if (FALSE == IS_HUSKEY_VALID(pUSKey))
        return ERROR_INVALID_PARAMETER;

    if (SHREGENUM_BOTH == enumRegFlags)             
    {
        // This is not supported yet. 
        ASSERT(FALSE);
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    if (SHREGENUM_HKCU != enumRegFlags && SHREGENUM_HKLM != enumRegFlags && SHREGENUM_DEFAULT != enumRegFlags)
    {
        // check your arguments.
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    // Default is to try HKCU first.
    if (SHREGENUM_HKCU == enumRegFlags || SHREGENUM_DEFAULT == enumRegFlags)
    {
        lRet = PrivRegEnumValue(pUSKey, &(pUSKey->hkeyCurrentUser), dwIndex,
                                pszValueName, TRUE, pcchValueNameLen, pdwType, pvData, pcbData);
    }

    if ((SHREGENUM_HKLM == enumRegFlags) || 
        ((SHREGENUM_DEFAULT == enumRegFlags) && ((ERROR_SUCCESS != lRet) && (ERROR_MORE_DATA != lRet) && (ERROR_NO_MORE_ITEMS != lRet))))
    {
        lRet = PrivRegEnumValue(pUSKey, &(pUSKey->hkeyLocalMachine), dwIndex,
                                pszValueName, TRUE, pcchValueNameLen, pdwType, pvData, pcbData);
    }

    return lRet;
}


/*----------------------------------------------------------
Purpose: Gets Info about a HUSKEY.
         Re-uses same flags as enumeration functions. 
         Look at SHRegEnumKeyExA for an explanation of the flags.

Returns: LONG containing success or error code.
Cond:    --
*/

STDAPI_(LONG)
SHRegQueryInfoUSKeyA
(
    IN  HUSKEY              hUSKey,
    OUT LPDWORD             pcSubKeys,             OPTIONAL
    OUT LPDWORD             pcchMaxSubKeyLen,      OPTIONAL
    OUT LPDWORD             pcValues,              OPTIONAL
    OUT LPDWORD             pcchMaxValueNameLen,   OPTIONAL
    IN SHREGENUM_FLAGS      enumRegFlags
)
{
    PUSKEY      pUSKey      = (PUSKEY) hUSKey;
    LONG        lRet        = ERROR_INVALID_PARAMETER;

    if (FALSE == IS_HUSKEY_VALID(pUSKey))
        return ERROR_INVALID_PARAMETER;

    if (SHREGENUM_BOTH == enumRegFlags)             
    {
        // This is not supported yet. 
        ASSERT(FALSE);
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    if (SHREGENUM_HKCU != enumRegFlags && SHREGENUM_HKLM != enumRegFlags && SHREGENUM_DEFAULT != enumRegFlags)
    {
        // check your arguments.
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    // Default is to try HKCU first.
    if (SHREGENUM_HKCU == enumRegFlags || SHREGENUM_DEFAULT == enumRegFlags)
    {
        lRet = PrivRegQueryInfoKey(pUSKey, &(pUSKey->hkeyCurrentUser), FALSE,
                                pcSubKeys, pcchMaxSubKeyLen, pcValues, pcchMaxValueNameLen);
    }

    if ((SHREGENUM_HKLM == enumRegFlags) || 
        ((SHREGENUM_DEFAULT == enumRegFlags) && ((ERROR_SUCCESS != lRet) && (ERROR_MORE_DATA != lRet))))
    {
        lRet = PrivRegQueryInfoKey(pUSKey, &(pUSKey->hkeyLocalMachine), FALSE,
                                pcSubKeys, pcchMaxSubKeyLen, pcValues, pcchMaxValueNameLen);
    }

    return lRet;
}


/*----------------------------------------------------------
Purpose: Gets Info about a HUSKEY.
         Re-uses same flags as enumeration functions. 
         Look at SHRegEnumKeyExA for an explanation of the flags.

Returns: LONG containing success or error code.
Cond:    --
*/

STDAPI_(LONG)
SHRegQueryInfoUSKeyW
(
    IN  HUSKEY              hUSKey,
    OUT LPDWORD             pcSubKeys,             OPTIONAL
    OUT LPDWORD             pcchMaxSubKeyLen,      OPTIONAL
    OUT LPDWORD             pcValues,              OPTIONAL
    OUT LPDWORD             pcchMaxValueNameLen,    OPTIONAL
    IN SHREGENUM_FLAGS      enumRegFlags
)
{
    PUSKEY      pUSKey      = (PUSKEY) hUSKey;
    LONG        lRet        = ERROR_INVALID_PARAMETER;

    if (FALSE == IS_HUSKEY_VALID(pUSKey))
        return ERROR_INVALID_PARAMETER;

    if (SHREGENUM_BOTH == enumRegFlags)             
    {
        // This is not supported yet. 
        ASSERT(FALSE);
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    if (SHREGENUM_HKCU != enumRegFlags && SHREGENUM_HKLM != enumRegFlags && SHREGENUM_DEFAULT != enumRegFlags)
    {
        // check your arguments.
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    // Default is to try HKCU first.
    if (SHREGENUM_HKCU == enumRegFlags || SHREGENUM_DEFAULT == enumRegFlags)
    {
        lRet = PrivRegQueryInfoKey(pUSKey, &(pUSKey->hkeyCurrentUser), TRUE,
                                pcSubKeys, pcchMaxSubKeyLen, pcValues, pcchMaxValueNameLen);
    }

    if ((SHREGENUM_HKLM == enumRegFlags) || 
        ((SHREGENUM_DEFAULT == enumRegFlags) && ((ERROR_SUCCESS != lRet) && (ERROR_MORE_DATA != lRet))))
    {
        lRet = PrivRegQueryInfoKey(pUSKey, &(pUSKey->hkeyLocalMachine), TRUE,
                                pcSubKeys, pcchMaxSubKeyLen, pcValues, pcchMaxValueNameLen);
    }

    return lRet;
}

/*----------------------------------------------------------
Purpose: Closes a HUSKEY (Handle to a User Specifc registry key).  

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegCloseUSKey(
    OUT HUSKEY  hUSKey)	
{
    PUSKEY  pUSKey = (PUSKEY) hUSKey;
    LONG    lRet   = ERROR_SUCCESS;

    ASSERT(pUSKey);
    if (FALSE == IS_HUSKEY_VALID(pUSKey))
        return ERROR_INVALID_PARAMETER;

    if (pUSKey->hkeyLocalMachine)
    {
        lRet = RegCloseKey(pUSKey->hkeyLocalMachine);
        pUSKey->hkeyLocalMachine = NULL;             // Used to indicate that it's invalid.
    }
    if (pUSKey->hkeyLocalMachineRelative && HKEY_LOCAL_MACHINE != pUSKey->hkeyLocalMachineRelative)
    {
        lRet = RegCloseKey(pUSKey->hkeyLocalMachineRelative);
    }

    if (pUSKey->hkeyCurrentUser)
    {
        lRet = RegCloseKey(pUSKey->hkeyCurrentUser);
        pUSKey->hkeyCurrentUser = NULL;             // Used to indicate that it's invalid.
    }
    if (pUSKey->hkeyCurrentUserRelative && HKEY_CURRENT_USER != pUSKey->hkeyCurrentUserRelative)
    {
        lRet = RegCloseKey(pUSKey->hkeyCurrentUserRelative);
    }

    LocalFree((HLOCAL)pUSKey);
    return lRet;
}



/*----------------------------------------------------------
Purpose: Gets a registry value that is User Specifc.  
         This opens and closes the key in which the value resides.  

         Perf:  if your code involves setting/getting a series
         of values in the same key, it is better to open
         the key once and then call SHRegQueryUSValue
         rather than using this function repeatedly.

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegGetUSValueA(
    IN  LPCSTR  pszSubKey,          
    IN  LPCSTR  pszValue,           
    OUT LPDWORD pdwType,            OPTIONAL
    OUT LPVOID  pvData,             OPTIONAL
    OUT LPDWORD pcbData,            OPTIONAL
    IN  BOOL    fIgnoreHKCU,
    IN  LPVOID  pvDefaultData,      OPTIONAL
    IN  DWORD   dwDefaultDataSize)
{
    LONG    lRet;
    HUSKEY  hUSkeys;
    DWORD   dwInitialSize = (pcbData ? *pcbData : 0);

    lRet = SHRegOpenUSKeyA(pszSubKey, KEY_QUERY_VALUE, NULL, &hUSkeys, fIgnoreHKCU);
    if (ERROR_SUCCESS == lRet)
    {
        lRet = SHRegQueryUSValueA(hUSkeys, pszValue, pdwType, pvData, pcbData, fIgnoreHKCU, pvDefaultData, dwDefaultDataSize);
        SHRegCloseUSKey(hUSkeys);
    }
    else
    {
        // if fail, use default value as long as dwDefaultDataSize isn't 0. (So we return the error)
        if ((pvDefaultData) && (dwDefaultDataSize) && (pvData) && (dwInitialSize >= dwDefaultDataSize))
        {
            MoveMemory(pvData, pvDefaultData, dwDefaultDataSize);
            if (pcbData)
                *pcbData = dwDefaultDataSize;
            lRet = ERROR_SUCCESS;       // Call will now use a default value.
        }
    }

    return lRet;
}




/*----------------------------------------------------------
Purpose: Gets a registry value that is User Specifc.  
         This opens and closes the key in which the value resides.  

         Perf:  if your code involves setting/getting a series
         of values in the same key, it is better to open
         the key once and then call SHRegQueryUSValue
         rather than using this function repeatedly.

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegGetUSValueW(
    IN  LPCWSTR pwzSubKey,          
    IN  LPCWSTR pwzValue,           
    OUT LPDWORD pdwType,            OPTIONAL
    OUT LPVOID  pvData,             OPTIONAL
    OUT LPDWORD pcbData,            OPTIONAL
    IN  BOOL    fIgnoreHKCU,
    IN  LPVOID  pvDefaultData,      OPTIONAL
    IN  DWORD   dwDefaultDataSize)
{
    LONG    lRet;
    HUSKEY  hUSkeys;
    DWORD   dwInitialSize = (pcbData ? *pcbData : 0);

    lRet = SHRegOpenUSKeyW(pwzSubKey, KEY_QUERY_VALUE, NULL, &hUSkeys, fIgnoreHKCU);
    if (ERROR_SUCCESS == lRet)
    {
        lRet = SHRegQueryUSValueW(hUSkeys, pwzValue, pdwType, pvData, pcbData, fIgnoreHKCU, pvDefaultData, dwDefaultDataSize);
        SHRegCloseUSKey(hUSkeys);
    }
    else
    {
        // if fail, use default value as long as dwDefaultDataSize isn't 0. (So we return the error)
        if ((pvDefaultData) && (dwDefaultDataSize) && (pvData) && (dwInitialSize >= dwDefaultDataSize))
        {
            // if fail, use default value.
            MoveMemory(pvData, pvDefaultData, dwDefaultDataSize);
            if (pcbData)
                *pcbData = dwDefaultDataSize;
            lRet = ERROR_SUCCESS;       // Call will now use a default value.
        }
    }

    return lRet;
}




/*----------------------------------------------------------
Purpose: Sets a registry value that is User Specifc.  
         This opens and closes the key in which the value resides.  

         Perf:  if your code involves setting a series
         of values in the same key, it is better to open
         the key once and then call SHRegWriteUSValue
         rather than using this function repeatedly.

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegSetUSValueA(
    IN  LPCSTR          pszSubKey,          
    IN  LPCSTR          pszValue,           
    IN  DWORD           dwType,
    IN  LPCVOID         pvData,         OPTIONAL
    IN  DWORD           cbData,         OPTIONAL
    IN  DWORD           dwFlags)        OPTIONAL
{
    LONG    lRet;
    HUSKEY  hUSkeys;

    lRet = SHRegCreateUSKeyA(pszSubKey, KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &hUSkeys, dwFlags);
    if (ERROR_SUCCESS == lRet)
    {
        lRet = SHRegWriteUSValueA(hUSkeys, pszValue, dwType, pvData, cbData, dwFlags);
        SHRegCloseUSKey(hUSkeys);
    }

    return lRet;
}




/*----------------------------------------------------------
Purpose: Sets a registry value that is User Specifc.  
         This opens and closes the key in which the value resides.  

         Perf:  if your code involves setting a series
         of values in the same key, it is better to open
         the key once and then call SHRegWriteUSValue
         rather than using this function repeatedly.

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegSetUSValueW(
    IN  LPCWSTR         pwzSubKey,          
    IN  LPCWSTR         pwzValue,           
    IN  DWORD           dwType,         OPTIONAL
    IN  LPCVOID         pvData,         OPTIONAL
    IN  DWORD           cbData,         OPTIONAL
    IN  DWORD           dwFlags)        OPTIONAL
{
    LONG    lRet;
    HUSKEY  hUSkeys;

    lRet = SHRegCreateUSKeyW(pwzSubKey, KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &hUSkeys, dwFlags);
    if (ERROR_SUCCESS == lRet)
    {
        lRet = SHRegWriteUSValueW(hUSkeys, pwzValue, dwType, pvData, cbData, dwFlags);
        SHRegCloseUSKey(hUSkeys);
    }

    return lRet;
}




/*----------------------------------------------------------
Purpose: Gets a BOOL Setting from the registry.  The default
         parameter will be used if it's not found in the registry.  

Cond:    --
*/
#define BOOLSETTING_BOOL_TRUE1W   L"YES"
#define BOOLSETTING_BOOL_TRUE1A   "YES"
#define BOOLSETTING_BOOL_TRUE2W   L"TRUE"
#define BOOLSETTING_BOOL_TRUE2A   "TRUE"
#define BOOLSETTING_BOOL_FALSE1W   L"NO"
#define BOOLSETTING_BOOL_FALSE1A   "NO"
#define BOOLSETTING_BOOL_FALSE2W   L"FALSE"
#define BOOLSETTING_BOOL_FALSE2A   "FALSE"

STDAPI_(BOOL)
SHRegGetBoolUSValueW(
    IN  LPCWSTR         pwzSubKey,          
    IN  LPCWSTR         pwzValue,           
    IN  BOOL            fIgnoreHKCU,
    IN  BOOL            fDefault)
{
    LONG lRet;
    WCHAR szData[MAX_PATH];
    DWORD dwType;
    DWORD dwSize = SIZEOF(szData);
    LPCWSTR pszDefault = fDefault ? BOOLSETTING_BOOL_TRUE1W : BOOLSETTING_BOOL_FALSE1W;
    DWORD dwDefaultSize = (fDefault ? SIZEOF(BOOLSETTING_BOOL_TRUE1W) : SIZEOF(BOOLSETTING_BOOL_FALSE1W)) + SIZEOF(WCHAR);

    lRet = SHRegGetUSValueW(pwzSubKey, pwzValue, &dwType, (LPVOID) szData, &dwSize, fIgnoreHKCU, (LPVOID) pszDefault, dwDefaultSize);
    if (ERROR_SUCCESS == lRet)
    {
        if (dwType == REG_BINARY || dwType == REG_DWORD)
        {
            fDefault = (*((DWORD*)szData) != 0);
        }
        else
        {
            if ((0 == lstrcmpiW(BOOLSETTING_BOOL_TRUE1W, szData)) || 
                (0 == lstrcmpiW(BOOLSETTING_BOOL_TRUE2W, szData)))
            {
                fDefault = TRUE;        // We read TRUE from the registry.
            }
            else if ((0 == lstrcmpiW(BOOLSETTING_BOOL_FALSE1W, szData)) || 
                (0 == lstrcmpiW(BOOLSETTING_BOOL_FALSE2W, szData)))
            {
                fDefault = FALSE;        // We read TRUE from the registry.
            }
        }
    }

    return fDefault;
}




/*----------------------------------------------------------
Purpose: Gets a BOOL Setting from the registry.  The default
         parameter will be used if it's not found in the registry.  

Cond:    --
*/

STDAPI_(BOOL)
SHRegGetBoolUSValueA(
    IN  LPCSTR          pszSubKey,          
    IN  LPCSTR          pszValue,           
    IN  BOOL            fIgnoreHKCU,
    IN  BOOL            fDefault)
{
    LONG lRet;
    CHAR szData[MAX_PATH];
    DWORD dwType;
    DWORD dwSize = SIZEOF(szData);
    LPCSTR pszDefault = fDefault ? BOOLSETTING_BOOL_TRUE1A : BOOLSETTING_BOOL_FALSE1A;
    DWORD dwDefaultSize = (fDefault ? SIZEOF(BOOLSETTING_BOOL_TRUE1A) : SIZEOF(BOOLSETTING_BOOL_FALSE1A)) + SIZEOF(CHAR);

    lRet = SHRegGetUSValueA(pszSubKey, pszValue, &dwType, (LPVOID) szData, &dwSize, fIgnoreHKCU, (LPVOID) pszDefault, dwDefaultSize);
    if (ERROR_SUCCESS == lRet)
    {
        if (dwType == REG_BINARY || dwType == REG_DWORD)
        {
            fDefault = (*((DWORD*)szData) != 0);
        }
        else
        {
            if ((0 == lstrcmpiA(BOOLSETTING_BOOL_TRUE1A, szData)) || 
                (0 == lstrcmpiA(BOOLSETTING_BOOL_TRUE2A, szData)))
            {
                fDefault = TRUE;        // We read TRUE from the registry.
            }
            else if ((0 == lstrcmpiA(BOOLSETTING_BOOL_FALSE1A, szData)) || 
                (0 == lstrcmpiA(BOOLSETTING_BOOL_FALSE2A, szData)))
            {
                fDefault = FALSE;        // We read TRUE from the registry.
            }
        }
    }

    return fDefault;
}

STDAPI_(DWORD)
SHGetValueGoodBootA(HKEY hkeyParent, LPCSTR pcszSubKey,
                                   LPCSTR pcszValue, PDWORD pdwValueType,
                                   PBYTE pbyteBuf, PDWORD pdwcbBufLen)
{
    if (GetSystemMetrics(SM_CLEANBOOT))
        return ERROR_GEN_FAILURE;

    return SHGetValueA(hkeyParent, pcszSubKey, pcszValue, pdwValueType, pbyteBuf, pdwcbBufLen);
}

STDAPI_(DWORD)
SHGetValueGoodBootW(HKEY hkeyParent, LPCWSTR pcwzSubKey,
                                   LPCWSTR pcwzValue, PDWORD pdwValueType,
                                   PBYTE pbyteBuf, PDWORD pdwcbBufLen)
{
    if (GetSystemMetrics(SM_CLEANBOOT))
        return ERROR_GEN_FAILURE;

    return SHGetValueW(hkeyParent, pcwzSubKey, pcwzValue, pdwValueType, pbyteBuf, pdwcbBufLen);
}


/*----------------------------------------------------------
Purpose: Given a CLSID open and return that key from HKCR, or
         the user local version.

Cond:    --
*/

LWSTDAPI SHRegGetCLSIDKeyW(UNALIGNED REFGUID rguid, LPCWSTR pszSubKey, BOOL fUserSpecific, BOOL fCreate, HKEY *phkey)
{
    HKEY    hkeyRef;
    WCHAR   szThisCLSID[GUIDSTR_MAX];
    WCHAR   szPath[GUIDSTR_MAX+MAX_PATH+1];   // room for clsid + extra
    ULONG   err;

    SHStringFromGUIDW(rguid, szThisCLSID, ARRAYSIZE(szThisCLSID));

    if (fUserSpecific)
    {
        hkeyRef = HKEY_CURRENT_USER;
        StrCpyW(szPath, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\");
    }
    else
    {
        hkeyRef = HKEY_CLASSES_ROOT;
        StrCpyW(szPath, L"CLSID");
        StrCatW(szPath, L"\\");
    }
    StrCatW(szPath, szThisCLSID);
    if (pszSubKey)
    {
        StrCatW(szPath, L"\\");
        StrCatBuffW(szPath, pszSubKey, ARRAYSIZE(szPath));
    }

    if (fCreate)
        err = RegCreateKeyWrapW(hkeyRef, szPath, phkey);
    else
        err = RegOpenKeyExWrapW(hkeyRef, szPath, 0, MAXIMUM_ALLOWED, phkey);
    return HRESULT_FROM_WIN32(err);
}


LWSTDAPI SHRegGetCLSIDKeyA(UNALIGNED REFGUID rguid, LPCSTR pszSubKey, BOOL fUserSpecific, BOOL fCreate, HKEY *phkey)
{
    HKEY   hkeyRef;
    CHAR   szThisCLSID[GUIDSTR_MAX];
    CHAR   szPath[GUIDSTR_MAX+MAX_PATH+1];   // room for clsid + extra
    ULONG  err;

    SHStringFromGUIDA(rguid, szThisCLSID, ARRAYSIZE(szThisCLSID));

    if (fUserSpecific)
    {
        hkeyRef = HKEY_CURRENT_USER;
        StrCpyA(szPath, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\");
    }
    else
    {
        hkeyRef = HKEY_CLASSES_ROOT;
        StrCpyA(szPath, "CLSID\\");
    }
    StrCatA(szPath, szThisCLSID);
    if (pszSubKey)
    {
        StrCatA(szPath, "\\");
        StrCatBuffA(szPath, pszSubKey, ARRAYSIZE(szPath));
    }

    if (fCreate)
        err = RegCreateKeyA(hkeyRef, szPath, phkey);
    else
        err = RegOpenKeyExA(hkeyRef, szPath, 0, MAXIMUM_ALLOWED, phkey);
    return HRESULT_FROM_WIN32(err);
}

/*----------------------------------------------------------
Purpose: Duplicate an hkey if an object wants to keep one open

//***   Reg_DupKey -- duplicate registry key (upping refcnt)
// NOTES
//  BUGBUG gotta fix this logic (now that i understand how bogus it is).
//
//  what we're trying to do is dup the handle.  sounds easy.  it isn't.
// here's the deal.
//  1- RegOpenKeyEx(hkey, NULL, ..., &hkey2) is spec'ed as giving back the
//  same handle.  on win95 it ups the refcnt (good!).
//  2- but on winNT there is no refcnt associated w/ it.  so it gives back
//  the same handle but now *any* close will make *all* of the 'pseudo-dup'ed
//  handles invalid.
//  3- (on winNT) if we add MAXIMUM_ALLOWED, we're asking for a new SAM.
//  but the SAM is associated w/ the handle, so the only (or rather, closest)
//  way to do that is to give a new handle.  (presumably this only works
//  if we're not dup'ing a handle that's already MAXIMUM_ALLOWED).
//  4- (on winNT) but wait!  if we open HKEY_CURRENT_USER, we *always* get
//  back 0x80000001 (or somesuch).  but closes on that are ignored, so all
//  works.
//
//  so what we probably should do is:
//  - win95: just do #1, w/ default security.  win95 will give us the same
//  handle w/ an *upped* refcnt and we'll be fine.
//  - winNT: do a DuplicateHandle.  this will correctly give us a *new*
//  handle and we'll be fine.
//
*/
HKEY SHRegDuplicateHKey(HKEY hkey)
{
    HKEY hkeyDup = NULL; // in case incoming hkey is invalid

    // NULL returns key to same place and ups refcnt
    RegOpenKeyExA(hkey, NULL, 0, MAXIMUM_ALLOWED, &hkeyDup);

#ifndef UNIX
    // IEUNIX : We also implement ref counting.
    // This is a bogus assert on Win95, since they have reference counting on these hkeys...
    if (g_bRunningOnNT)
    {
         ASSERT(hkeyDup != hkey ||
                hkey == HKEY_CURRENT_USER ||
                hkey == HKEY_CLASSES_ROOT ||
                hkey == HKEY_LOCAL_MACHINE);
    }
#endif

    return hkeyDup;
}

/*----------------------------------------------------------
Purpose: Read a string value from the registry and convert it
         to an integer.

*/
LWSTDAPI_(int) SHRegGetIntW(HKEY hk, LPCWSTR szKey, int nDefault)
{
    DWORD cb;
    WCHAR ach[20];

    if (hk == NULL)
        return nDefault;

    ach[0] = 0;
    cb = SIZEOF(ach);
    SHQueryValueExW(hk, szKey, NULL, NULL, (LPBYTE)ach, &cb);

    if (ach[0] >= L'0' && ach[0] <= L'9')
        return StrToIntW(ach);
    else
        return nDefault;
}


//  99/01/19 vtan: These two functions have been added to help with
//  getting and setting Win32 file paths in the registry.

DWORD   SHRegGetPathImplementation (HKEY hKey, LPCWSTR pcwszSubKey, LPCWSTR pcwszValue, LPWSTR pwszPath, DWORD dwFlags)

//  RegGetPathImplementation: Unicode implementation of function.
//  Returns an expanded file path from the registry.

//  Parameters:

//             hKey - an open HKEY or registry root key
//      pcwszSubKey - subkey in registry or NULL/zero length string
//       pcwszValue - value name in registry
//         pwszPath - string to place path in (assumed size of MAX_PATH chars)
//          dwFlags - unused / future expansion

//  Return value:

//      Returns Win32 error code from ADVAPI32.DLL function calls.

{
    DWORD   dwResult;
    HKEY    hKeyToUse;

    // Do we need to open a registry key or were we told to use the given HKEY.

    if ((pcwszSubKey == NULL) || (*pcwszSubKey == TEXT('\0')))
    {
        hKeyToUse = hKey;
        dwResult = ERROR_SUCCESS;
    }
    else
    {
        dwResult = RegOpenKeyExW(hKey, pcwszSubKey, 0, KEY_QUERY_VALUE, &hKeyToUse);
    }
    if (dwResult == ERROR_SUCCESS)
    {
        DWORD   dwType, dwPathSize;

        dwPathSize = MAX_PATH * sizeof(WCHAR);
        dwResult = RegQueryValueExW(hKeyToUse, pcwszValue, 0, &dwType, (LPBYTE)pwszPath, &dwPathSize);

        // It is INCORRECT to use this function for data other than
        // REG_SZ / REG_EXPAND_SZ. Use the regular APIs in other cases.

#ifdef  DEBUG
        if ((dwType != REG_SZ) && (dwType != REG_EXPAND_SZ))
            TraceMsg(TF_WARNING, "RegGetPathImplementation expects REG_SZ or REG_EXPAND_SZ");
#endif

        // If the path needs expanding (environment variable) then
        // expand it and make sure the result fits in MAX_PATH chars.

        if ((dwResult == ERROR_SUCCESS) && (dwType == REG_EXPAND_SZ))
        {
            DWORD   dwResult;
            WCHAR   wszTemp[MAX_PATH];

            dwResult = ExpandEnvironmentStringsW(pwszPath, wszTemp, ARRAYSIZE(wszTemp));
            if (dwResult == 0)
                dwResult = GetLastError();
            else
            {
                if (dwResult > MAX_PATH)
                {
                    wszTemp[MAX_PATH - 1] = L'\0';
                    dwResult = ERROR_MORE_DATA;
                }
                lstrcpyW(pwszPath, wszTemp);
            }
        }
        if (hKeyToUse != hKey)
            TW32(RegCloseKey(hKeyToUse));
    }
    return(dwResult);
}

DWORD   SHRegSetPathImplementation (HKEY hKey, LPCWSTR pcwszSubKey, LPCWSTR pcwszValue, LPCWSTR pcwszPath, DWORD dwFlags)

//  RegSetPathImplementation: Unicode implementation of function.
//  Stores a file path in the registry but looks for a match with
//  certain environment variables first. This is a FIXED list.

//  Parameters:

//             hKey - an open HKEY or registry root key
//      pcwszSubKey - subkey in registry or NULL/zero length string
//       pcwszValue - value name in registry
//        pcwszPath - Win32 file path to write
//          dwFlags - unused / future expansion

//  Return value:

//      Returns Win32 error code from ADVAPI32.DLL function calls.

{
    DWORD   dwResult;
    HKEY    hKeyToUse;

    if ((pcwszSubKey == NULL) || (*pcwszSubKey == TEXT('\0')))
    {
        hKeyToUse = hKey;
        dwResult = ERROR_SUCCESS;
    }
    else
    {
        dwResult = RegOpenKeyExW(hKey, pcwszSubKey, 0, KEY_SET_VALUE, &hKeyToUse);
    }
    if (dwResult == ERROR_SUCCESS)
    {
        DWORD           dwType;
        const WCHAR     *pcwszDataToWrite;
        WCHAR           wszTemp[MAX_PATH];

        //  Match        %USERPROFILE% - x:\WINNT\Profiles\<user>
        //                             - x:\Documents And Settings\<user>
        //          %ALLUSERSPROFILES% - x:\WINNT\Profiles\<user>
        //                             - x:\Documents And Settings\<user>
        //              %ProgramFiles% - x:\Program Files
        //                %SystemRoot% - x:\WINNT

        //  %ALLUSERSPROFILE% and %ProgramFiles% are dubious and can be
        //  removed.

        //  WARNING: DO NOT CHANGE THE MATCH ORDER OF %USERPROFILE% AND
        //  %SystemRoot%

        //  If %SystemRoot% is matched first then %USERPROFILE% will
        //  NEVER be matched if inside x:\WINNT\ 

        if (PathUnExpandEnvStringsW(pcwszPath, wszTemp, ARRAYSIZE(wszTemp)))
        {
            dwType = REG_EXPAND_SZ;
            pcwszDataToWrite = wszTemp;
        }
        else
        {
            dwType = REG_SZ;
            pcwszDataToWrite = pcwszPath;
        }
        dwResult = RegSetValueExW(hKeyToUse, pcwszValue, 0, dwType, (LPBYTE)pcwszDataToWrite, (lstrlenW(pcwszDataToWrite) + 1) * sizeof(WCHAR));
        if (hKeyToUse != hKey)
            TW32(RegCloseKey(hKeyToUse));
    }
    return(dwResult);
}

DWORD   SHRegGetPathA (HKEY hKey, LPCSTR pcszSubKey, LPCSTR pcszValue, LPSTR pszPath, DWORD dwFlags)

{
    DWORD   dwResult;
    WCHAR   wszSubKey[MAX_PATH], wszValue[MAX_PATH], wszPath[MAX_PATH];

    SHAnsiToUnicode(pcszSubKey, wszSubKey, ARRAYSIZE(wszSubKey));
    SHAnsiToUnicode(pcszValue, wszValue, ARRAYSIZE(wszValue));
    dwResult = SHRegGetPathImplementation(hKey, wszSubKey, wszValue, wszPath, dwFlags);
    SHUnicodeToAnsi(wszPath, pszPath, MAX_PATH);
    return(dwResult);
}

DWORD   SHRegGetPathW (HKEY hKey, LPCWSTR pcszSubKey, LPCWSTR pcszValue, LPWSTR pszPath, DWORD dwFlags)

{
    return(SHRegGetPathImplementation(hKey, pcszSubKey, pcszValue, pszPath, dwFlags));
}

DWORD   SHRegSetPathA (HKEY hKey, LPCSTR pcszSubKey, LPCSTR pcszValue, LPCSTR pcszPath, DWORD dwFlags)

{
    WCHAR   wszSubKey[MAX_PATH], wszValue[MAX_PATH], wszPath[MAX_PATH];

    SHAnsiToUnicode(pcszSubKey, wszSubKey, ARRAYSIZE(wszSubKey));
    SHAnsiToUnicode(pcszValue, wszValue, ARRAYSIZE(wszValue));
    SHAnsiToUnicode(pcszPath, wszPath, ARRAYSIZE(wszPath));
    return(SHRegSetPathImplementation(hKey, wszSubKey, wszValue, wszPath, dwFlags));
}

DWORD   SHRegSetPathW (HKEY hKey, LPCWSTR pcszSubKey, LPCWSTR pcszValue, LPCWSTR pcszPath, DWORD dwFlags)

{
    return(SHRegSetPathImplementation(hKey, pcszSubKey, pcszValue, pcszPath, dwFlags));
}

BOOL Reg_GetCommand(HKEY hkey, LPCWSTR pszKey, LPCWSTR pszValue, LPWSTR pszCommand)
{
    WCHAR szKey[1024];
    LONG cbSize = sizeof(szKey);
    int iLen;

    StrCpyNW(szKey, pszKey, ARRAYSIZE(szKey));
    iLen = lstrlenW(szKey);
    pszCommand[0] = 0;

    // a trailing backslash means no value key
    if (szKey[iLen-1] == L'\\' ||
        (pszValue && !pszValue[0])) {

        if (!pszValue)
            szKey[iLen-1] = 0;

        RegQueryValueWrapW(hkey, szKey, pszCommand, &cbSize);

    } else {

        if (!pszValue)
            pszValue = PathFindFileNameW(szKey);

        ASSERT(pszValue);
        if (!pszValue)
            return FALSE;

        PathRemoveFileSpecW(szKey);
        SHGetValueGoodBootW(hkey, szKey, pszValue, NULL, (LPBYTE)pszCommand, (DWORD*)&cbSize);
    }

    if (pszCommand[0]) {
        LPWSTR pszNextKey;

        // see if it's a registry spec
        if (!StrCmpNIW(pszCommand, L"HKCU:", 5)) {
            hkey = HKEY_CURRENT_USER;
            pszNextKey = pszCommand + 5;

        } else if (!StrCmpNIW(pszCommand, L"HKLM:", 5)) {
            hkey = HKEY_LOCAL_MACHINE;
            pszNextKey = pszCommand + 5;

        } else if (!StrCmpNIW(pszCommand, L"HKCR:", 5)) {
            hkey = HKEY_CLASSES_ROOT;
            pszNextKey = pszCommand + 5;
        } else {

            return (BOOL)pszCommand[0];
        }

        StrCpyNW(szKey, pszNextKey, ARRAYSIZE(szKey));
        return (Reg_GetCommand(hkey, szKey, NULL, pszCommand));
    }

    return (BOOL)pszCommand[0];
}

LPWSTR _PathGetArgs(LPCWSTR pszPath)
{
        BOOL fInQuotes = FALSE;

        if (!pszPath)
                return NULL;

        while (*pszPath)
        {
                if (*pszPath == L'"')
                        fInQuotes = !fInQuotes;
                else if (!fInQuotes && *pszPath == L' ')
                        return (LPWSTR)pszPath+1;
                pszPath = CharNextW(pszPath);
        }

        return (LPWSTR)pszPath;
}

void _PathRemoveArgs(LPWSTR pszPath)
{
    LPWSTR pArgs = PathGetArgsW(pszPath);
    if (*pArgs)
        *(pArgs - 1) = L'\0';   // clobber the ' '
    // Handle trailing space.
    else
    {
        pArgs = CharPrevW(pszPath, pArgs);
        if (*pArgs == L' ')
            *pArgs = L'\0';
    }
}

#define FillExecInfo(_info, _hwnd, _verb, _file, _params, _dir, _show) \
        (_info).hwnd            = _hwnd;        \
        (_info).lpVerb          = _verb;        \
        (_info).lpFile          = _file;        \
        (_info).lpParameters    = _params;      \
        (_info).lpDirectory     = _dir;         \
        (_info).nShow           = _show;        \
        (_info).fMask           = 0;            \
        (_info).cbSize          = sizeof(SHELLEXECUTEINFOW);

HRESULT RunRegCommand(HWND hwnd, HKEY hkey, LPCWSTR pszKey)
{
    HRESULT hr = E_FAIL;

#ifdef UNIX
//IEUNIX: OE Perf improvement - Steven Yao
    WCHAR szBinaryDir[MAX_PATH];
    WCHAR szOS[11];
    INT nOsLen = 10;    // assume max os name 10
    INT nEnvLen = MAX_PATH -lstrlen(TEXT("/ie/oecontacts")) - nOsLen - 1;
    INT nRetLen;
#endif

    WCHAR szCommand[1024];
    if (Reg_GetCommand(hkey, pszKey, L"", szCommand)) 
    {
        LPWSTR pszArgs;
        SHELLEXECUTEINFOW ExecInfo;
        WCHAR szExpCommand[1024];

        SHExpandEnvironmentStringsW(szCommand, szExpCommand, ARRAYSIZE(szExpCommand));

        // Long filenames _should_ be surrounded by quote marks. However, some aren't.
        // This causes problems because the registry entry might be of the form 
        // (c:\program files\Windows Messaging\[...]) instead of 
        // ("c:\program files\Windows Messaging\[...]"). Compare this with 
        // a reg value with (rundll32 C:\progra~1\etc)
        // We end up parsing attempting to run C:\program, which of course doesn't exist.

        // This is a hack for the benefit lame-O OSR2, which turns szExpCommand
        // into a null string, rather than letting it be, if it can't be shortened.
#ifndef UNIX
//IEUNIX - This returns full path which would messup the Unix Perf-Workaround
        GetShortPathNameW(szExpCommand, szExpCommand, ARRAYSIZE(szExpCommand));
#endif
        if ((*szExpCommand==L'\0') && (*szCommand!=L'\0'))
        {
            SHExpandEnvironmentStringsW(szCommand, szExpCommand, ARRAYSIZE(szExpCommand));
        }
        pszArgs = _PathGetArgs(szExpCommand);
        _PathRemoveArgs(szExpCommand);
        PathUnquoteSpacesW(szExpCommand);
#ifdef UNIX
//IEUNIX: OE Perf improvement - Steven Yao
        szBinaryDir[0] = L'\0';
        szOS[0] = L'\0';
        if ((_wcsicmp(szExpCommand, L"oexpress") == 0) ||
           (_wcsicmp(szExpCommand, L"oecontacts") == 0))
        {
            if ((nRetLen = GetEnvironmentVariableW(L"MWDEV", szBinaryDir, nEnvLen))
                && (nRetLen <= nEnvLen)) {
                if ((nRetLen = GetEnvironmentVariableW(L"MWOS"), szOS, nOsLen))
                  && (nRetLen <= nOsLen)) {
                       wcscat(szBinaryDir, L"/ie/");
                       wcscat(szBinaryDir, szOS);
                }
                else {
                   #ifdef ux10
                   wcscat(szBinaryDir, L"/ie/ux10");
                   #endif
                }
            }
        }
        FillExecInfo(ExecInfo, hwnd, NULL, szExpCommand, pszArgs, szBinaryDir, SW_SHOWNORMAL);
#else
        FillExecInfo(ExecInfo, hwnd, NULL, szExpCommand, pszArgs, NULL, SW_SHOWNORMAL);
#endif //UNIX
        hr = ShellExecuteExWrapW(&ExecInfo) ? S_OK : E_FAIL;
    }

    return hr;
}

HRESULT RunIndirectRegCommand(HWND hwnd, HKEY hkey, LPCWSTR pszKey, LPCWSTR pszVerb)
{
    HRESULT hr = E_FAIL;
    WCHAR szDefApp[80];
    LONG cbSize = sizeof(szDefApp);

    if (RegQueryValueWrapW(hkey, pszKey, szDefApp, &cbSize) == ERROR_SUCCESS) 
    {
        WCHAR szFullKey[256];

        // tack on shell\%verb%\command
        wnsprintfW(szFullKey, ARRAYSIZE(szFullKey), L"%s\\%s\\shell\\%s\\command", pszKey, szDefApp, pszVerb);
        hr = RunRegCommand(hwnd, hkey, szFullKey);
    }

    return hr;
}

HRESULT SHRunIndirectRegClientCommand(HWND hwnd, LPCWSTR pszClient)
{
    WCHAR szKey[80];

    wnsprintfW(szKey, ARRAYSIZE(szKey), L"Software\\Clients\\%s", pszClient);
    return RunIndirectRegCommand(hwnd, HKEY_LOCAL_MACHINE, szKey, L"Open");
}
