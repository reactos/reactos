//+------------------------------------------------------------------------
//
//  File:       regdbhlp.cxx
//
//  Contents:   Registration helper functions
//
//  History:    20-Oct-94   GaryBu  Created
//
//-------------------------------------------------------------------------

#include "headers.hxx"

//+------------------------------------------------------------------------
//
//  Function:    RegDbDeleteKey
//
//  Synopsis:   Recursively delete registry key given key name.
//
//  Arguments:  [hkeyParent] - Parent of the key to delete.
//              [szDelete] - Name of key to delete.
//
//-------------------------------------------------------------------------
void
RegDbDeleteKey(HKEY hkeyParent, const TCHAR *pszDelete)
{
    DWORD   dwResult;
    HKEY    hkeyDelete;

    dwResult = RegOpenKey(hkeyParent, pszDelete, &hkeyDelete);
    if (dwResult != ERROR_SUCCESS)
        goto Error;
#ifndef _MAC
    // Mac note: RegDeleteKey deletes all subkeys
    {
        TCHAR   szSubKey[256];
        while (RegEnumKey(hkeyDelete, 0, szSubKey, ARRAY_SIZE(szSubKey))
                == ERROR_SUCCESS)
        {
            RegDbDeleteKey(hkeyDelete, szSubKey);
        }
    }
#endif

    Verify(RegCloseKey(hkeyDelete) == ERROR_SUCCESS);

    dwResult = RegDeleteKey(hkeyParent, pszDelete);
Error:
    Assert((dwResult == ERROR_SUCCESS) || (dwResult == ERROR_BADKEY) ||
        (dwResult == ERROR_FILE_NOT_FOUND));

}

//+------------------------------------------------------------------------
//
//  Function:    RegDbOpenCLSIDKey
//
//  Synopsis:   Open HKEY_CLASSES_ROOT\CLISD.
//
//  Arguments:  [phkeyCLISD] - Address at which to return the HKEY
//
//-------------------------------------------------------------------------
HRESULT
RegDbOpenCLSIDKey(HKEY *phkeyCLSID)
{
    return RegOpenKey(HKEY_CLASSES_ROOT, TEXT("CLSID"), phkeyCLSID) ==
            ERROR_SUCCESS ? NOERROR : REGDB_E_READREGDB;
}

//+------------------------------------------------------------------------
//
//  Function:    RegDbDSetValues
//
//  Synopsis:   Add key and values to registry.
//
//              The keys and values are described by a string
//              and an array of arguments for substitution into
//              the string. The string consists of alternating
//              key and value format strings separted by nulls.
//              These format strings are in the format described
//              by the Format function (see CORE\FORMAT.CXX).
//              The string is terminated with two nulls.
//              The first key-value pair is treated as a parent
//              for the remaining keys.
//
//  Arguments:  [hkeyParent] - Add key/value pairs to this key.
//              [szFmt] - String describing values to add to registry.
//                  See description above.
//              [adwArgs] - Values to insert into format string.
//
//-------------------------------------------------------------------------
HRESULT
RegDbSetValues(HKEY hkeyParent, TCHAR *szFmt, DWORD_PTR *adwArgs)
{
    HRESULT hr          = S_OK;
    LONG    lError;
    HKEY    hkeySubkey;
    TCHAR   szKey[128];
    TCHAR   szSubkey[128];
    TCHAR   szValue[MAX_PATH];
    BOOL    fNamedSubkey;
    BOOL    fFirstTime  = TRUE;

    Assert(szFmt && *szFmt);

    hkeySubkey   = NULL;
    fNamedSubkey = FALSE;
    while (*szFmt)
    {
        // First, extract the major key
        // (Only cause of key format error is programmer error.)
        Verify(VFormat(FMT_ARG_ARRAY,
                szKey, ARRAY_SIZE(szKey), szFmt, adwArgs) == S_OK);
        szFmt += _tcslen(szFmt) + 1;

        // Then, write each subkey/value pair
        do
        {
            //  If it is a named subkey (rather than a value on the major key),
            //  format the subkey
            //  (Default values, those associated directly with the major
            //   key, use the name of the major key)
            if (*szFmt)
            {
                fNamedSubkey = TRUE;
                Verify(VFormat(FMT_ARG_ARRAY,
                        szSubkey, ARRAY_SIZE(szSubkey), szFmt, adwArgs) == S_OK);
            }
            szFmt += _tcslen(szFmt) + 1;

            // Format the value
            hr = THR(VFormat(FMT_ARG_ARRAY,
                    szValue, ARRAY_SIZE(szValue), szFmt, adwArgs));
            if (hr)
                goto Cleanup;

            szFmt += _tcslen(szFmt) + 1;

            //  Open (and use) the key if writing a named subkey beneath it
            //  (The key must have already been created...this assumes that
            //   all keys have default values which, when set, will create
            //   the key)
            if (fNamedSubkey && !hkeySubkey)
            {
                if (RegOpenKey(hkeyParent, szKey, &hkeySubkey) != ERROR_SUCCESS)
                {
                    hr = REGDB_E_WRITEREGDB;
                    goto Cleanup;
                }
            }

            // Write the key and value
            if (!fNamedSubkey)
            {
                lError = RegSetValue(hkeyParent, szKey, REG_SZ, szValue,0);
            }
            else
            {
                Assert(hkeySubkey);
                lError = RegSetValueEx(hkeySubkey, szSubkey, 0, REG_SZ,
                                        (CONST BYTE *)szValue,
                                        (sizeof(TCHAR)*_tcslen(szValue))+1);
            }
            if (lError != ERROR_SUCCESS)
            {
                hr = REGDB_E_WRITEREGDB;
                goto Cleanup;
            }

        }
        while (*szFmt);
        szFmt += _tcslen(szFmt) + 1;    // Skip over the subkey/value pair terminator

        // If an HKEY was opened for the key itself, close it before continuing
        if (hkeySubkey)
        {
            Assert(fNamedSubkey);
            Verify(RegCloseKey(hkeySubkey) == ERROR_SUCCESS);
            hkeySubkey   = NULL;
            fNamedSubkey = FALSE;
        }

        // If further major keys exist, write them as children of the first major key
        if (fFirstTime && *szFmt)
        {
            if (RegOpenKey(hkeyParent, szKey, &hkeyParent) != ERROR_SUCCESS)
            {
                hr = REGDB_E_WRITEREGDB;
                goto Cleanup;
            }
            fFirstTime = FALSE;
        }
    }

Cleanup:
    if (hkeySubkey)
    {
        Verify(RegCloseKey(hkeySubkey) == ERROR_SUCCESS);
    }

    if (!fFirstTime)
    {
        Verify(RegCloseKey(hkeyParent) == ERROR_SUCCESS);
    }

    return hr;
}
