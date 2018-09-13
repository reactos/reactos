
//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1991-1993
//
// File: reglist.c
//
// History:
//   5-30-94 KurtE      Created.
//
//---------------------------------------------------------------------------

#include "shellprv.h"
#pragma  hdrstop

//#define PARANOID_VALIDATE_UPDATE

// Define our global states here.  Note: we will do it per process
typedef struct _RLPI    // Registry List Process Info
{
    HDPA    hdpaRLList;             // The dpa of items
    BOOL    fCSInitialized;         // Have we initialized the CS in this process
    BOOL    fListValid;             // Is the list up to date and valid
    CRITICAL_SECTION csRLList;      // The critical section for the process
} RLPI;


RLPI    g_rlpi = {NULL, FALSE, FALSE};

//===========================================================================
// Simple DPA compare function make sure we don't have elsewhere...
//
//===========================================================================

int CALLBACK _CompareStrings(LPVOID sz1, LPVOID sz2, LPARAM lparam)
{
    return lstrcmpi((LPTSTR)sz1, (LPTSTR)sz2);
}

//===========================================================================
// Simple Functions for entering and leaving the Registry list Critical
// section.  This will also make sure that the Critical Section has
// been previously initialized.
//
//===========================================================================
void RLEnterCritical()
{
    if (!g_rlpi.fCSInitialized)
    {
        // Do this under the global critical section.
        ENTERCRITICAL;
        if (!g_rlpi.fCSInitialized)
        {
            g_rlpi.fCSInitialized = TRUE;
            ReinitializeCriticalSection(&g_rlpi.csRLList);
        }
        LEAVECRITICAL;
    }
    EnterCriticalSection(&g_rlpi.csRLList);
}

void RLLeaveCritical()
{
    LeaveCriticalSection(&g_rlpi.csRLList);
}


//===========================================================================
//
// RLEnumRegistry: Enumerate through the registry looking for paths that
//              we may wish to track.  The current ones that we look for
//              include:
//
// HKEY_CLASSES_ROOT\CLSID\???\LocalServer
// HKEY_CLASSES_ROOT\CLSID\???\InProcServer
// HKEY_CLASSES_ROOT\CLSID\???\InProcServer32
// HEKY_CLASSES_ROOT\CLSID\???\DefaultIcon
// HKEY_CLASSES_ROOT\???\protocol\???\server
// HKEY_CLASSES_ROOT\???\shell\???\command
// HKEY_CLASSES_ROOT\???\shell\???\command
// HKEY_LOCAL_MACHINE\Software\Windows\CurrentVersion\App Paths
//              ....\App Paths\Paths
//
//===========================================================================

STDAPI_(BOOL) RLEnumRegistry(HDPA hdpa, PRLCALLBACK pfnrlcb,
                             LPCTSTR pszSource, LPCTSTR pszDest)
{
    HKEY hkeyRoot;
    HKEY hkeySecond;
//    HKEY hkey3;
    TCHAR szRootName[80];
//    TCHAR szSecond[80];
    int  iRootName;
//    int  iSecond;
//    int iThird;
    long cbValue;

    TCHAR szPath[MAX_PATH];

#if 0
    // This code has been disabled because with IE4 our registries have
    // grown so large this is prohibitively expensive to walk everytime
    // a file is being deleted.  If Intel speeds things up more, we will
    // turn this on...

    
    // First look in the CLSID section. I dont think they
    // need to be localized.
    if (RegOpenKey(HKEY_CLASSES_ROOT, c_szCLSID, &hkeyRoot) == ERROR_SUCCESS)
    {
        for (iRootName=0; RegEnumKey(hkeyRoot, iRootName,
                szRootName, ARRAYSIZE(szRootName)) == ERROR_SUCCESS; iRootName++)
        {
            // Now try to enum this key
            if (RegOpenKey(hkeyRoot, szRootName, &hkeySecond) == ERROR_SUCCESS)
            {
                for (iSecond=0; RegEnumKey(hkeySecond, iSecond,
                        szSecond, ARRAYSIZE(szSecond)) == ERROR_SUCCESS; iSecond++)
                {
                    // Now see if this is one of the keys that we know
                    // contain paths of interest.
                    if ((lstrcmpi(szSecond, c_szDefaultIcon)== 0) ||
                        (lstrcmpi(szSecond, TEXT("LocalServer"))==0) ||
                        (lstrcmpi(szSecond, TEXT("LocalServer32"))==0) ||
                        (lstrcmpi(szSecond, TEXT("InprocHandler"))==0) ||
                        (lstrcmpi(szSecond, TEXT("InprocHandler32"))==0) ||
                        (lstrcmpi(szSecond, TEXT("InProcServer"))==0) ||
                        (lstrcmpi(szSecond, TEXT("InProcServer32"))==0))
                    {
                        cbValue = SIZEOF(szPath);
                        if (SHRegQueryValue(hkeySecond, szSecond, szPath,
                                &cbValue) == ERROR_SUCCESS)
                        {
                            pfnrlcb(hdpa, hkeySecond, szSecond, NULL,
                                    szPath, pszSource, pszDest);
                        }
                    }
                }

                RegCloseKey(hkeySecond);
            }
        }

        RegCloseKey(hkeyRoot);
    }
#endif
    


#if 0
    // Now for the typelib section
    if (RegOpenKey(HKEY_CLASSES_ROOT, TEXT("TypeLib"), &hkeyRoot) == ERROR_SUCCESS)
    {
        for (iRootName=0; RegEnumKey(hkeyRoot, iRootName,
                szRootName, ARRAYSIZE(szRootName)) == ERROR_SUCCESS; iRootName++)
        {
            // Now try to enum this key
            if (RegOpenKey(hkeyRoot, szRootName, &hkeySecond) == ERROR_SUCCESS)
            {
                for (iSecond=0; RegEnumKey(hkeySecond, iSecond,
                        szSecond, ARRAYSIZE(szSecond)) == ERROR_SUCCESS; iSecond++)
                {
                    if (RegOpenKey(hkeySecond, szSecond, &hkey3)==ERROR_SUCCESS)
                    {
                        cbValue = SIZEOF(szPath);
                        if (SHRegQueryValue(hkey3, TEXT("HelpDir"), szPath,
                                &cbValue) == ERROR_SUCCESS)
                        {
                            pfnrlcb(hdpa, hkey3, TEXT("HelpDir"), NULL,
                                    szPath,pszSource, pszDest);
                        }

                        for (iThird=0; RegEnumKey(hkey3, iThird,
                                szSecond, ARRAYSIZE(szSecond)) == ERROR_SUCCESS; iThird++)
                        {
                            HKEY hkey4;
                            // See if there is an interesting value under
                            // this one
                            if (RegOpenKey(hkey3, szSecond, &hkey4)==ERROR_SUCCESS)
                            {
                                cbValue = SIZEOF(szPath);
                                if (SHRegQueryValue(hkey4, TEXT("Win16"), szPath,
                                        &cbValue) == ERROR_SUCCESS)
                                {
                                    pfnrlcb(hdpa, hkey4, TEXT("Win16"), NULL,
                                            szPath,pszSource, pszDest);
                                }

                                cbValue = SIZEOF(szPath);
                                if (SHRegQueryValue(hkey4, TEXT("Win32"), szPath,
                                        &cbValue) == ERROR_SUCCESS)
                                {
                                    pfnrlcb(hdpa, hkey4, TEXT("Win32"), NULL, szPath,pszSource, pszDest);
                                }
                                RegCloseKey(hkey4);
                            }
                        }
                        RegCloseKey(hkey3);
                    }
                }

                RegCloseKey(hkeySecond);
            }
        }

        RegCloseKey(hkeyRoot);
    }
#endif


#if 0    
    //
    // Now lets go at the root of the class root.
    //
    for (iRootName=0; RegEnumKey(HKEY_CLASSES_ROOT, iRootName,
            szRootName, ARRAYSIZE(szRootName)) == ERROR_SUCCESS; iRootName++)
    {
        // Now try to enum this key if it is not CLSID
        if ((lstrcmpi(szRootName, c_szCLSID) != 0) &&
                (lstrcmpi(szRootName, TEXT("TypeLib")) != 0) &&
                (RegOpenKey(HKEY_CLASSES_ROOT, szRootName, &hkeySecond) == ERROR_SUCCESS))
        {
            for (iSecond=0; RegEnumKey(hkeySecond, iSecond,
                    szSecond, ARRAYSIZE(szSecond)) == ERROR_SUCCESS; iSecond++)
            {

                static const TCHAR c_szProtocol[] = TEXT("protocol");
                static const TCHAR c_szServer[] = TEXT("Server");
                // Now see if this is one of the keys that we know
                // contain paths of interest.
                if (lstrcmpi(szSecond, c_szDefaultIcon)== 0)
                {
                    cbValue = SIZEOF(szPath);
                    if (SHRegQueryValue(hkeySecond, szSecond, szPath,
                            &cbValue) == ERROR_SUCCESS)
                    {
                        pfnrlcb(hdpa, hkeySecond, szSecond, NULL, szPath,
                                pszSource, pszDest);
                    }
                }
                else if ((lstrcmpi(szSecond, c_szProtocol)==0) ||
                        (lstrcmpi(szSecond, c_szShell)==0))
                {
                    // We need to enum the keys under this one and see
                    // if anything interesting.
                    if (RegOpenKey(hkeySecond, szSecond, &hkey3)==ERROR_SUCCESS)
                    {
                        for (iThird=0; RegEnumKey(hkey3, iThird,
                                szSecond, ARRAYSIZE(szSecond)) == ERROR_SUCCESS; iThird++)
                        {
                            HKEY hkey4;
                            // See if there is an interesting value under
                            // this one
                            if (RegOpenKey(hkey3, szSecond, &hkey4)==ERROR_SUCCESS)
                            {
                                cbValue = SIZEOF(szPath);
                                if (SHRegQueryValue(hkey4, c_szServer, szPath,
                                        &cbValue) == ERROR_SUCCESS)
                                {
                                    pfnrlcb(hdpa, hkey4, c_szServer, NULL,
                                            szPath,pszSource, pszDest);
                                }

                                cbValue = SIZEOF(szPath);
                                if (SHRegQueryValue(hkey4, c_szCommand, szPath,
                                        &cbValue) == ERROR_SUCCESS)
                                {
                                    pfnrlcb(hdpa, hkey4, c_szCommand, NULL,
                                            szPath,pszSource, pszDest);
                                }
                                RegCloseKey(hkey4);
                            }
                        }
                        RegCloseKey(hkey3);
                    }
                }
            }

            RegCloseKey(hkeySecond);
        }
    }
#endif
    
    // First look in the App Paths section
    if (RegOpenKey(HKEY_LOCAL_MACHINE, c_szAppPaths, &hkeyRoot) == ERROR_SUCCESS)
    {
        for (iRootName = 0; 
             RegEnumKey(hkeyRoot, iRootName, szRootName, ARRAYSIZE(szRootName)) == ERROR_SUCCESS; 
             iRootName++)
        {
            // Now see if the app has a qualifid path here.
            cbValue = SIZEOF(szPath);
            if (SHRegQueryValue(hkeyRoot, szRootName, szPath, &cbValue) == ERROR_SUCCESS)
            {
                PathUnquoteSpaces(szPath);
                pfnrlcb(hdpa, hkeyRoot, szRootName, NULL, szPath, pszSource, pszDest);
            }

            // Now try to enum this key  for Path Value.
            if (RegOpenKey(hkeyRoot, szRootName, &hkeySecond) == ERROR_SUCCESS)
            {
                cbValue = SIZEOF(szPath);

                if (SHQueryValueEx(hkeySecond, TEXT("PATH"), NULL, NULL, szPath, &cbValue) == ERROR_SUCCESS)
                {
                    // this is a ";" separated list
                    LPTSTR psz = StrChr(szPath, TEXT(';'));
                    if (psz)
                        *psz = 0;
                    PathUnquoteSpaces(szPath);
                    pfnrlcb(hdpa, hkeySecond, NULL, TEXT("PATH"), szPath, pszSource, pszDest);
                }

                RegCloseKey(hkeySecond);
            }
        }

        RegCloseKey(hkeyRoot);
    }
    return TRUE;
}

//===========================================================================
//
// _RLBuildListCallback: This is the call back called to build the list of
//      of paths.
//
//===========================================================================

BOOL CALLBACK _RLBuildListCallBack(HDPA hdpa, HKEY hkey, LPCTSTR pszKey,
        LPCTSTR pszValueName, LPTSTR pszValue, LPCTSTR pszSource, LPCTSTR pszDest)
{
    int iIndex;

    // Also don't add any relative paths.
    if (PathIsRelative(pszValue) || (lstrlen(pszValue) < 3))
        return TRUE;

    // Don't try UNC names as this can get expensive...
    if (PathIsUNC(pszValue))
        return TRUE;

    // If it is already in our list, we can simply return now..
    if (DPA_Search(hdpa, pszValue, 0, _CompareStrings, 0, DPAS_SORTED) != -1)
        return TRUE;

    // If it is in our old list then
    if (g_rlpi.hdpaRLList && ((iIndex = DPA_Search(g_rlpi.hdpaRLList, pszValue, 0,
            _CompareStrings, 0, DPAS_SORTED)) != -1))

    {
        // Found the item in the old list.
        TraceMsg(TF_REG, "_RLBuildListCallBack: Add from old list %s", pszValue);

        DPA_InsertPtr(hdpa,
                    DPA_Search(hdpa, pszValue, 0,
                    _CompareStrings, 0,
                    DPAS_SORTED|DPAS_INSERTBEFORE),
                    (LPTSTR)DPA_FastGetPtr(g_rlpi.hdpaRLList, iIndex));
        // now remove it from the old list
        DPA_DeletePtr(g_rlpi.hdpaRLList, iIndex);
    }
    else
    {
        // Not in either list.
        // Now see if we can convert the short name to a long name

        TCHAR szLongName[MAX_PATH];
        int cchName;
        LPTSTR psz;

        if (!NT5_GetLongPathName(pszValue, szLongName, ARRAYSIZE(szLongName)))
            szLongName[0] = 0;

        if (lstrcmpi(szLongName, pszValue) == 0)
            szLongName[0] = 0;   // Don't need both strings.

        cchName = lstrlen(pszValue);

        psz = (LPTSTR)LocalAlloc(LPTR,
                (cchName + 1 + lstrlen(szLongName) + 1) * SIZEOF(TCHAR));
        if (psz)
        {
            TraceMsg(TF_REG, "_RLBuildListCallBack: Add %s", pszValue);

            lstrcpy(psz, pszValue);
            lstrcpy(psz + cchName + 1, szLongName);

            return DPA_InsertPtr(hdpa,
                    DPA_Search(hdpa, pszValue, 0,
                    _CompareStrings, 0,
                    DPAS_SORTED|DPAS_INSERTBEFORE),
                    psz);
        }
    }
    return TRUE;
}



//===========================================================================
//
// RLBuildListOfPaths: This function will build the list of items that we
//      will look through to see if the user may have changed the path of
//      of one of the programs that is registered in the registry.
//
//===========================================================================


BOOL WINAPI RLBuildListOfPaths()
{
    BOOL fRet = FALSE;
    HDPA hdpa;

    DEBUG_CODE( DWORD   dwStart = GetCurrentTime(); )

    RLEnterCritical();

    hdpa = DPA_Create(0);
    if (!hdpa)
        goto Error;


    // And initialize the list
    fRet = RLEnumRegistry(hdpa, _RLBuildListCallBack, NULL, NULL);


    // If we had on old list destroy it now.

    if (g_rlpi.hdpaRLList)
    {
        // Walk through all of the items in the list and
        // delete all of the strings.
        int i;
        for (i = DPA_GetPtrCount(g_rlpi.hdpaRLList)-1; i >= 0; i--)
            LocalFree((HLOCAL)DPA_FastGetPtr(g_rlpi.hdpaRLList, i));
        DPA_Destroy(g_rlpi.hdpaRLList);
    }

    g_rlpi.hdpaRLList = hdpa;
    g_rlpi.fListValid = TRUE;     // Say that we are valid...

    DEBUG_CODE( TraceMsg(TF_REG, "RLBuildListOfPaths time: %ld", GetCurrentTime()-dwStart); )

Error:

    RLLeaveCritical();
    return fRet;
}

//===========================================================================
//
// RLTerminate: this function does any cleanup necessary for when a process
//      is no longer going to use the Registry list.
//
//===========================================================================

void WINAPI RLTerminate()
{
    int i;

    if (!g_rlpi.hdpaRLList)
        return;

    RLEnterCritical();
    // Walk through all of the items in the list and
    // delete all of the strings.
    for (i = DPA_GetPtrCount(g_rlpi.hdpaRLList)-1; i >= 0; i--)
        LocalFree((HLOCAL)DPA_FastGetPtr(g_rlpi.hdpaRLList, i));

    DPA_Destroy(g_rlpi.hdpaRLList);
    g_rlpi.hdpaRLList = NULL;
    RLLeaveCritical();
}



//===========================================================================
//
// RLIsPathInList: This function returns TRUE if the path that is passed
//      in is contained in one or more of the paths that we extracted from
//      the registry.
//
//===========================================================================


int WINAPI RLIsPathInList(LPCTSTR pszPath)
{
    int i = -1;
    RLEnterCritical();

    if (!g_rlpi.hdpaRLList || !g_rlpi.fListValid)
        RLBuildListOfPaths();

    if (g_rlpi.hdpaRLList)
    {
        int cchPath = lstrlen(pszPath);

        for (i = DPA_GetPtrCount(g_rlpi.hdpaRLList) - 1; i >= 0; i--)
        {
            LPTSTR psz = DPA_FastGetPtr(g_rlpi.hdpaRLList, i);
            if (PathCommonPrefix(pszPath, psz, NULL) == cchPath)
                break;

            // See if a long file name to check.
            psz += lstrlen(psz) + 1;
            if (*psz && (PathCommonPrefix(pszPath, psz, NULL) == cchPath))
                break;
        }
    }

    RLLeaveCritical();

    return i;   // -1 if none, >= 0 index
}

//===========================================================================
//
// _RLRenameCallback: This is the call back called to build the list of
//      of paths.
//
//===========================================================================

BOOL CALLBACK _RLRenameCallBack(HDPA hdpa, HKEY hkey, LPCTSTR pszKey,
        LPCTSTR pszValueName, LPTSTR pszValue, LPCTSTR pszSource, LPCTSTR pszDest)
{
    int cbMatch = PathCommonPrefix(pszValue, pszSource, NULL);
    if (cbMatch == lstrlen(pszSource))
    {
        TCHAR szPath[MAX_PATH+64];   // Add some slop just in case...
        // Found a match, lets try to rebuild the new line
        lstrcpy(szPath, pszDest);
        lstrcat(szPath, pszValue + cbMatch);

        if (pszValueName)
            RegSetValueEx(hkey, pszValueName, 0, REG_SZ, (BYTE *)szPath, (lstrlen(szPath) + 1) * SIZEOF(TCHAR));
        else
            RegSetValue(hkey, pszKey, REG_SZ, szPath, lstrlen(szPath));
    }

    // Make sure that we have not allready added
    // this path to our list.
    if (DPA_Search(hdpa, pszValue, 0, _CompareStrings, 0, DPAS_SORTED) == -1)
    {
        // One to Add!
        LPTSTR psz = StrDup(pszValue);
        if (psz)
        {
            return DPA_InsertPtr(hdpa,
                    DPA_Search(hdpa, pszValue, 0,
                    _CompareStrings, 0,
                    DPAS_SORTED | DPAS_INSERTBEFORE), psz);
        }
    }
    return TRUE;
}


//===========================================================================
//
// RLFSChanged: This function handles the cases when we are notified of
//      a change to the file system and then we need to see if there are
//      any changes that we need to make to the regisry to handle the
//      changes.
//
//===========================================================================
int WINAPI RLFSChanged(LONG lEvent, LPITEMIDLIST pidl, LPITEMIDLIST pidlExtra)
{
    TCHAR szSrc[MAX_PATH];
    TCHAR szDest[MAX_PATH+8];     // For slop like Quotes...
    int iIndex;
    LPTSTR psz;
    int iRet = -1;
    int i;

    // First see if the operation is something we are interested in.
    if ((lEvent & (SHCNE_RENAMEITEM | SHCNE_RENAMEFOLDER)) == 0)
        return -1; // Nope

    if (!SHGetPathFromIDList(pidl, szSrc))
    {
        // must be a rename of a non-filesys object (such as a printer!)
        return -1;
    }

    SHGetPathFromIDList(pidlExtra, szDest);

    // If either are roots we really can not rename them...
    if (PathIsRoot(szSrc) || PathIsRoot(szDest))
        return -1;

    // ignore if coming from bitbucket or going to ...
    // check bitbucket first.  that's a cheap call
    if ((lEvent & SHCNE_RENAMEITEM) &&
        (IsFileInBitBucket(szSrc) || IsFileInBitBucket(szDest)))
        return -1;

    RLEnterCritical();
    // Now see if the source file is in our list of paths
    iIndex = RLIsPathInList(szSrc);
    if (iIndex != -1)
    {
        // Now make sure we are working with the short name
        // Note we may only be a subpiece of this item.
        // Count how many fields there are in the szSrc Now;
        for (i = 0, psz = szSrc; psz; i++)
        {
            psz = StrChr(psz + 1, TEXT('\\'));
        }
        lstrcpy(szSrc, (LPTSTR)DPA_FastGetPtr(g_rlpi.hdpaRLList, iIndex));

        // Now truncate off stuff that is not from us Go one more then we countd
        // above and if we have a non null value cut it off there.
        for (psz = szSrc; i > 0; i--)
        {
            psz = StrChr(psz+1, TEXT('\\'));
        }
        if (psz)
            *psz = 0;

        // verify that this is a fully qulified path and that it exists
        // before we go and muck with the registry.
        if (!PathIsRelative(szDest) && PathFileExistsAndAttributes(szDest, NULL) && (lstrlen(szDest) >= 3))
        {
            // Yes, so now lets reenum and try to update the paths...
            PathGetShortPath(szDest);        // Convert to a short name...
            RLEnumRegistry(g_rlpi.hdpaRLList, _RLRenameCallBack, szSrc, szDest);

            // We changed something so mark it to be rebuilt
            g_rlpi.fListValid = FALSE;     // Force it to rebuild.
            iRet = 1;
        }
    }
    RLLeaveCritical();

    return iRet;
}
