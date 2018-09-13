/////////////////////////////////////////////////////////////////////////////
// REGINI.CPP
//
// Registry helper functions implementation
//
// History:
//
// Author   Date        Description
// ------   ----        -----------
// jaym     09/18/96    Adapted from ccteng's source
// jaym     04/29/97    Cleanup and simplification
/////////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "regini.h"

/////////////////////////////////////////////////////////////////////////////
// ReadRegValue
/////////////////////////////////////////////////////////////////////////////
DWORD ReadRegValue
(
    HKEY    hKeyRoot,
    LPCTSTR pszSubKey,
    LPCTSTR pszValue,
    BYTE *  pbReturn,
    DWORD * pdwReturnSize
)
{
    HKEY    hKey = NULL;
    DWORD   dwValueType;
    DWORD   dwResult = (DWORD)-1;
    DWORD   retCode;

    for (;;)
    {
        // Attempt to open the key with read access.
        if ((retCode = RegOpenKeyEx(hKeyRoot,
                                    pszSubKey,
                                    0,
                                    KEY_READ,
                                    &hKey)) != ERROR_SUCCESS)
        {
            break;
        }

        if ((retCode = RegQueryValueEx( hKey,
                                        pszValue,
                                        NULL,
                                        &dwValueType,
                                        pbReturn,
                                        pdwReturnSize)) == ERROR_SUCCESS)
        {
            dwResult = dwValueType;
        }

        break;
    }

    if (hKey != NULL)
        RegCloseKey(hKey);

    return dwResult;
}

/////////////////////////////////////////////////////////////////////////////
// WriteRegValue
/////////////////////////////////////////////////////////////////////////////
BOOL WriteRegValue
(
    HKEY    hKeyRoot,
    LPCTSTR pszSubKey,
    LPCTSTR pszValue,
    DWORD   dwValueType,
    BYTE *  pValue,
    DWORD   dwSize
)
{
    HKEY    hKey = NULL;
    DWORD   dwReturn = (DWORD)-1;
    DWORD   retCode;
    DWORD   dwDisposition;
    BOOL    bResult = FALSE;

    for (;;)
    {
        // Attempt to create the key.
        if ((retCode = RegCreateKeyEx(  hKeyRoot,
                                        pszSubKey,
                                        0,
                                        NULL,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_WRITE,
                                        NULL,
                                        &hKey,
                                        &dwDisposition)) != ERROR_SUCCESS)
        {
            break;
        }

        // Now set the key value first we query to see if
        if ((retCode = RegSetValueEx(   hKey,
                                        pszValue,
                                        NULL,
                                        dwValueType,
                                        pValue,
                                        dwSize)) == ERROR_SUCCESS)
        {
            bResult = TRUE;
        }

        break;
    }

    if (hKey != NULL)
        RegCloseKey(hKey);

    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
// ReadRegString
/////////////////////////////////////////////////////////////////////////////
DWORD ReadRegString
( 
    HKEY    hKeyRoot,
    LPCTSTR pszSection, 
    LPCTSTR pszKey, 
    LPTSTR  pszDefault,
    LPTSTR  pszReturn,
    DWORD   cchReturnSize
)
{
    DWORD dwReturnSize = cchReturnSize;
    DWORD dwReturn = ReadRegValue(  hKeyRoot,
                                    pszSection,
                                    pszKey,
                                    (BYTE *)pszReturn,
                                    &dwReturnSize);

    if ( dwReturn > 0 ) {
        // We have a value but is it a string value?
        if ( (REG_SZ == dwReturn) || 
             (REG_MULTI_SZ == dwReturn) || 
             (REG_EXPAND_SZ == dwReturn) ) {
            dwReturn = dwReturnSize;
            goto Done;
        } else {
            // not the correct type - return the default
            goto ReturnDefault;
        }
    } else {
        // could not obtain the key value - return the default
        goto ReturnDefault;
    } 

ReturnDefault:
    // some type of error - we must return the default value
    dwReturn = min( (DWORD)lstrlen(pszDefault), cchReturnSize );
    lstrcpyn(pszReturn, pszDefault, cchReturnSize);//BUGBUG-FIXED-OVERFLOW

Done:
    return dwReturn;
}

/////////////////////////////////////////////////////////////////////////////
// ReadRegDWORD
/////////////////////////////////////////////////////////////////////////////
DWORD ReadRegDWORD
(
    HKEY    hKeyRoot,
    LPCTSTR pszSection,
    LPCTSTR pszKey, 
    DWORD   dwDefault
)
{
    TCHAR   szReturn[MAX_PATH];
    DWORD   dwReturnSize = sizeof(szReturn);
    DWORD   dwReturn = ReadRegValue(hKeyRoot,
                                    pszSection,
                                    pszKey,
                                    (BYTE *)szReturn,
                                    &dwReturnSize);

    if (dwReturn > 0)
    {
        // We have a value but is it a number value?
        if  (
            (REG_DWORD == dwReturn)
            ||
            (REG_DWORD_LITTLE_ENDIAN == dwReturn)
            ||
            (REG_DWORD_BIG_ENDIAN == dwReturn)
            )
        {
            // Return the value as a dword. Convert to little_endian
            // format if the number was in bigendian format.
            dwReturnSize = *((DWORD *) szReturn);

            if (dwReturn == REG_DWORD_BIG_ENDIAN)
                dwReturnSize = MAKELONG(HIWORD(dwReturnSize), LOWORD(dwReturnSize));

            dwReturn = dwReturnSize;
            goto Done;
        }
        else
        {
            // Not the correct type - return the default
            goto ReturnDefault;
        }
    }
    else
    {
        // Could not obtain the key value - return the default
        goto ReturnDefault;
    } 

ReturnDefault:
    // some type of error - we must return the default value
    dwReturn = dwDefault;

Done:
    return dwReturn;
}   
