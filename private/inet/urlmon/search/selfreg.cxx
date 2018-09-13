//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       selfreg.cxx
//
//  Contents:   Taken from Office96
//              Source file for the common self registration code used by all the
//              sub projects of Sweeper project. They are
//              UrlMon
//              UrlMnPrx
//
//  Exports:    HrDllRegisterServer()
//              HrDllUnregisterServer()
//
//  Classes:
//
//  Functions:
//
//  History:    5-03-96   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <urlint.h>
#include "selfreg.hxx"

HINSTANCE g_hinstDll = NULL;
PFNLOADSTRING g_pfnLoadString = NULL;

//+---------------------------------------------------------------------------
//
//  Function:   GetDllFullPath
//
//  Synopsis:
//
//  Arguments:  [lpszExeName] --
//              [cch] --
//
//  Returns:
//
//  History:         95    OfficeXX                 created
//              5-03-96   JohannP (Johann Posch)   port urlmon
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL GetDllFullPath( LPSTR lpszExeName, DWORD cch )
{
    if ( NULL == g_hinstDll )
    {
        UrlMkAssert(( FALSE && "NULL hInst"));
        return FALSE;
    }

    *lpszExeName = NULL;

    if ( GetModuleFileName( g_hinstDll, lpszExeName, cch ) == 0)
    {
        UrlMkAssert(( FALSE && "GetModuleFileName Failed"));
        return FALSE;
    }

    return TRUE;
}


inline BOOL IsASeparator( char ch )
{
    return (ch == '\\' || ch == '/' || ch == ':');
}

//+---------------------------------------------------------------------------
//
//  Function:   ParseAFileName
//
//  Synopsis:
//
//  Arguments:  [szFileName] --
//              [piRetLen] --
//
//  Returns:
//
//  History:         95    OfficeXX                 created
//              5-03-96   JohannP (Johann Posch)   port urlmon
//
//  Notes:
//
//----------------------------------------------------------------------------
LPSTR ParseAFileName( LPSTR szFileName, int *piRetLen)
{
    LPSTR pszFile;

    // Start at the end of the filename.
    pszFile = szFileName + ( lstrlen(szFileName) - 1 );

    // Back up to a '\\' or beginning or something!! We just want a file
    // name!. Whatever comes first.
    while ( pszFile > szFileName && !IsASeparator(*pszFile ) )
        pszFile = CharPrev(szFileName, pszFile);

    if ( pszFile != szFileName )
        pszFile = CharNext(pszFile);

    if ( piRetLen )
        *piRetLen = lstrlen(pszFile);

    return pszFile;
}


//+---------------------------------------------------------------------------
//
//  Function:   FRegisterEntries
//
//  Synopsis:   FRegisterEntries: Register a group of reg entries off a base key.
//
//  Arguments:  [hkRoot] --
//              [rgEntries] --
//              [dwEntries] --
//              [pszPath] --
//              [pszBinderName] --
//
//  Returns:
//
//  History:         95    OfficeXX                 created
//              5-03-96   JohannP (Johann Posch)   port urlmon
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL FRegisterEntries(HKEY hkRoot, const REGENTRY rgEntries[],
                    DWORD dwEntries, char *pszPath, char *pszBinderName)
{
    HKEY    hkey = NULL;
    LONG    lRet;
    char    szValue[1024];
    char    szResString[1024];
    char    szKeyName[1024];
    BOOL    fRet = FALSE;
    int         i;

    for (i = 0; i < (int)dwEntries; i++)
    {
        // We work with a copy of the entry, since we might modify it
        REGENTRY reCurrentEntry = rgEntries[i];

        if (reCurrentEntry.iKeyType ==  KEYTYPE_RESID)
        {
            int cch;
            if (g_pfnLoadString == NULL)
                return FALSE;
            cch = g_pfnLoadString(g_hinstDll, (UINT)reCurrentEntry.pszKey, szKeyName,
                    sizeof(szKeyName));
            if (cch > 0 && cch <= 1024)
            {
                reCurrentEntry.pszKey = szKeyName;
            }
            else
            {
                UrlMkAssert(( FALSE && "LoadString Failed ( 1)"));
                continue;
            }
        }

        lRet = RegCreateKey(hkRoot, reCurrentEntry.pszKey, &hkey);

        if (lRet != ERROR_SUCCESS)
        {
            UrlMkAssert(( FALSE && "RegCreateKey Failed ( 1)"));
            continue;
        }

         // If the type is REG_RESID, then pbData holds the resource ID.  We
         // load the resource string, then modify our reCurrentEntry to point
         // to it.

        if (reCurrentEntry.dwType == REG_RESID)
        {
            int cch;
            if (g_pfnLoadString == NULL)
                return FALSE;

            cch = g_pfnLoadString(g_hinstDll, (UINT)reCurrentEntry.pbData, szResString,
                    sizeof(szResString));
            if (cch > 0 && cch <= 1024)
            {
                reCurrentEntry.dwType = REG_SZ;
                reCurrentEntry.pbData = (BYTE*) szResString;
            }
            else
            {
                UrlMkAssert(( FALSE && "LoadString Failed (2)"));
                reCurrentEntry.pbData = NULL;
            }
        }


        // Set the value if there is one
        if (reCurrentEntry.pbData != NULL || reCurrentEntry.dwType != REG_SZ)
        {
            switch (reCurrentEntry.dwType)
            {
                case REG_SZ:
                    // Replace the first %s with the path, and the second
                    // %s with the name of the binder app (may not do anything).
                    if (pszPath != NULL && pszBinderName != NULL)
                    {
                        wsprintf(szValue, (char*)reCurrentEntry.pbData, pszPath,
                            pszBinderName);

                        lRet = RegSetValueEx(hkey, reCurrentEntry.pszValueName, 0,
                            REG_SZ, (BYTE*)szValue, lstrlen(szValue)+1);
#if DBG == 1
                        if ( ERROR_SUCCESS != lRet )
                            UrlMkAssert(( FALSE && "RegSetValueEx Failed ( 1)"));
#endif
                    }
                    break;

                case REG_DWORD:
                    lRet = RegSetValueEx(hkey, reCurrentEntry.pszValueName, 0,
                        REG_DWORD,  (BYTE*)&reCurrentEntry.pbData, sizeof(DWORD));

#if DBG == 1
                    if ( ERROR_SUCCESS != lRet )
                        UrlMkAssert(( FALSE && "RegSetValueEx Failed (2)"));
#endif
                    break;

                default:
                    UrlMkAssert(( FALSE && "Unexpected reg entry type"));
                    // Unexpected type: ignore
                    break;
            }
        }

        // Close the subkey
        RegCloseKey(hkey);
        hkey = NULL;
    }

    fRet = TRUE;

    // Close the base key if it was open
    if (hkey)
        RegCloseKey(hkey);

    return fRet;
}


/*
 * FRegisterEntryGroups: Register several groups of reg entries.
 */
BOOL FRegisterEntryGroups(const REGENTRYGROUP *rgRegEntryGroups,
    char *pszPath, char *pszBinderName)
{
    BOOL fError = FALSE;
    int i;

    // Keep going even if we get some errors
    for (i=0; rgRegEntryGroups[i].hkRoot != NULL; i++)
    {
        if (!FRegisterEntries(rgRegEntryGroups[i].hkRoot, rgRegEntryGroups[i].rgEntries,
            rgRegEntryGroups[i].dwEntries,pszPath, pszBinderName))
        {
            fError = TRUE;
        }
    }

    return !fError;
}

//+---------------------------------------------------------------------------
//
//  Function:   FDeleteEntries
//
//  Synopsis:   Delete a group of reg entries off a base key.
//
//  Arguments:  [hkRoot] --
//              [rgEntries] --
//              [dwEntries] --
//
//  Returns:
//
//  History:         95    OfficeXX                 created
//              5-03-96   JohannP (Johann Posch)   port urlmon
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL FDeleteEntries(HKEY hkRoot, const REGENTRY rgEntries[], DWORD dwEntries)
{
    LONG    lRet;
    int         i;
    char    szKeyName[1024];
    PSTR    pKey;

    // Delete in reverse order, to kill children before parent
    for (i = (int)dwEntries - 1; i >= 0; i--)
    {
        pKey = NULL;

        if (rgEntries[i].iKeyType ==  KEYTYPE_RESID)
        {
            int cch;
            cch = g_pfnLoadString(g_hinstDll, (UINT)rgEntries[i].pszKey, szKeyName,
                    sizeof(szKeyName));
            if (cch > 0 && cch <= 1024)
            {
                pKey = szKeyName;
            }
        else
            {
                UrlMkAssert(( FALSE && "LoadString Failed (FDeleteEntries)"));
                continue;
            }
        }
        else
        {
            if ( KEYTYPE_STRING != rgEntries[i].iKeyType )
            {
                UrlMkAssert(( FALSE && "Unknown Key Type"));
                continue;
            }
            pKey = rgEntries[i].pszKey;
        }

        if (pKey != NULL)
        {
            // Delete the current key if it has no subkeys.
            // Ignore the return value.
            lRet = RegDeleteKey(hkRoot, pKey);
        }
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   FDeleteEntryGroups
//
//  Synopsis:   Delete the base keys of all the given groups.

//
//  Arguments:  [rgRegEntryGroups] --
//
//  Returns:
//
//  History:         95    OfficeXX                 created
//              5-03-96   JohannP (Johann Posch)   port urlmon
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL FDeleteEntryGroups(const REGENTRYGROUP *rgRegEntryGroups)
{
    BOOL fError = FALSE;

    // Keep going even if we get some errors
    for (int i=0; rgRegEntryGroups[i].hkRoot != NULL; i++)
    {
        if (!FDeleteEntries(rgRegEntryGroups[i].hkRoot,
            rgRegEntryGroups[i].rgEntries,
            rgRegEntryGroups[i].dwEntries))
        {
            fError = TRUE;
        }
    }

    return !fError;
}


#ifdef NOT_USED
/*
 * FDeleteSubtree - Delete given key and all subkeys
 */
BOOL FDeleteSubtree(HKEY hkRoot, char *pszKey)
{
    HKEY        hkey = NULL;
    LONG        lRet;
    char        szSubKey[MAX_PATH];

    lRet = RegOpenKey(hkRoot, pszKey, &hkey);
    if (lRet != ERROR_SUCCESS)
        goto End;

    // remove all subkeys
    for (;;)
{
        lRet = RegEnumKey(hkey, 0, szSubKey, sizeof szSubKey);

        if (lRet == ERROR_NO_MORE_ITEMS)
            break;

        if (lRet != ERROR_SUCCESS)
            goto End;

        if (!FDeleteSubtree(hkey, szSubKey))
            goto End;
}

End:
    if (hkey != NULL)
        RegCloseKey (hkey);

    lRet = RegDeleteKey(hkRoot, pszKey);

    return (lRet == ERROR_SUCCESS);
}
#endif // NOT_USED



//+---------------------------------------------------------------------------
//
//  Function:   HrDllRegisterServer
//
//  Synopsis:   registers an entrygroup
//
//  Arguments:  [HINSTANCE] --
//              [hinstDll] --
//              [pfnLoadString] --
//              [pszAppName] --
//
//  Returns:
//
//  History:         95    OfficeXX                 created
//              5-03-96   JohannP (Johann Posch)   port urlmon
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT HrDllRegisterServer(const REGENTRYGROUP *rgRegEntryGroups,HINSTANCE hinstDll,
                            PFNLOADSTRING pfnLoadString, char *pszAppName)
{
    // REVIEW: for Windows dll, do we want to register full path?
    BOOL    fRet = TRUE;
    char    szFullPath[MAX_PATH];
    char    szFileName[MAX_PATH];
    char    *pszFileName;

    g_hinstDll = hinstDll;
    if ((g_pfnLoadString = pfnLoadString) == NULL)
        // set the pointer to windows LoadString() api
        g_pfnLoadString = (PFNLOADSTRING) LoadString;

    if (!GetDllFullPath(szFullPath, MAX_PATH))
        return E_FAIL;

    pszFileName = ParseAFileName(szFullPath, NULL);

    if (pszAppName != NULL)
        lstrcpy(szFileName, pszAppName);
    else
        lstrcpy(szFileName, pszFileName);

    // Terminate the path at the file name
    *pszFileName = '\0';
    fRet = FRegisterEntryGroups(rgRegEntryGroups, szFullPath, szFileName);

    g_hinstDll = NULL;
    g_pfnLoadString = NULL;
    return fRet ? NOERROR : E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrDllUnregisterServer
//
//  Synopsis:   unregisters an entrygroup
//
//  Arguments:  [HINSTANCE] --
//              [hinstDll] --
//              [pfnLoadString] --
//
//  Returns:
//
//  History:         95    OfficeXX                 created
//              5-03-96   JohannP (Johann Posch)   port urlmon
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT HrDllUnregisterServer(const REGENTRYGROUP *rgRegEntryGroups,HINSTANCE hinstDll, PFNLOADSTRING pfnLoadString)
{
    g_hinstDll = hinstDll;
    if ((g_pfnLoadString = pfnLoadString) == NULL)
        // set the pointer to windows LoadString() api
        g_pfnLoadString = (PFNLOADSTRING) LoadString;

    FDeleteEntryGroups(rgRegEntryGroups);

    g_hinstDll = NULL;
    g_pfnLoadString = NULL;

    return NOERROR;
}



