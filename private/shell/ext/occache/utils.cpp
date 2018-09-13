#include "general.h"
#include "ParseInf.h"
#include "resource.h"
#include "FileNode.h"
#include <shlwapi.h>
#include <shlobj.h>


//#define USE_SHORT_PATH_NAME   1

// also defined in \nt\private\inet\urlmon\download\isctrl.cxx
LPCTSTR g_lpszUpdateInfo = TEXT("UpdateInfo");
LPCTSTR g_lpszCookieValue = TEXT("Cookie");
LPCTSTR g_lpszSavedValue = TEXT("LastSpecifiedInterval");

// This is a 'private' entry point into URLMON that we use
// to convert paths from their current, possibly short-file-name
// form to their canonical long-file-name form.

#ifdef UNICODE
#define STR_CDLGETLONGPATHNAME "CDLGetLongPathNameW"

typedef DWORD (WINAPI *CDLGetLongPathNamePtr)(
    LPWSTR lpszLongPath,
    LPCWSTR  lpszShortPath,
    DWORD    cchBuffer
    );

#else // not UNICODE

#define STR_CDLGETLONGPATHNAME "CDLGetLongPathNameA"

typedef DWORD (WINAPI *CDLGetLongPathNamePtr)( 
    LPSTR lpszLong,
    LPCSTR lpszShort,
    DWORD cchBuffer
    );
#endif // else not UNICODE


// given the typelib id, loop through HKEY_CLASSES_ROOT\Interface section and
// remove those entries with "TypeLib" subkey equal to the given type lib id
HRESULT CleanInterfaceEntries(LPCTSTR lpszTypeLibCLSID)
{
    Assert(lpszTypeLibCLSID != NULL);
    if (lpszTypeLibCLSID == NULL || lpszTypeLibCLSID[0] == '\0')
        return HRESULT_FROM_WIN32(ERROR_BAD_ARGUMENTS);

    HRESULT hr = S_OK;
    HKEY hkey = NULL;
    DWORD cStrings = 0;
    LONG lResult = ERROR_SUCCESS, lSize = 0;
    TCHAR szKeyName[OLEUI_CCHKEYMAX];
    TCHAR szTmpID[MAX_PATH];

    // open key HKEY_CLASS_ROOT\Interface
    if (RegOpenKeyEx(
             HKEY_CLASSES_ROOT, 
             HKCR_INTERFACE,
             0,
             KEY_ALL_ACCESS,
             &hkey) == ERROR_SUCCESS)
    {
        // loop through all entries
        while ((lResult = RegEnumKey(
                                hkey,
                                cStrings,
                                szKeyName,
                                OLEUI_CCHKEYMAX)) == ERROR_SUCCESS)
        {
            lSize = MAX_PATH;
            lstrcat(szKeyName, TEXT("\\"));
            lstrcat(szKeyName, HKCR_TYPELIB);

            // if typelib id's match, remove the key
            if ((RegQueryValue(
                       hkey, 
                       szKeyName, 
                       szTmpID, 
                       &lSize) == ERROR_SUCCESS) &&
                (lstrcmpi(szTmpID, lpszTypeLibCLSID) == 0))
            {
                *(ReverseStrchr(szKeyName, '\\')) = '\0';
                DeleteKeyAndSubKeys(hkey, szKeyName);
            }
            else
            {
                cStrings += 1;
            }
        }

        RegCloseKey(hkey);

        Assert(lResult == ERROR_NO_MORE_ITEMS);
        if (lResult != ERROR_NO_MORE_ITEMS)
            hr = HRESULT_FROM_WIN32(lResult);
    }

    return hr;
}
    
// If the OCX file being removed does not exist, then we cannot prompt the control
// to unregister itself.  In this case, we call this function of clean up as many
// registry entries as we could for the control.
HRESULT CleanOrphanedRegistry(
                LPCTSTR szFileName, 
                LPCTSTR szClientClsId,
                LPCTSTR szTypeLibCLSID)
{
    HRESULT hr = S_OK;
    LONG lResult = 0;
    TCHAR szTmpID[MAX_PATH];
    TCHAR szTmpRev[MAX_PATH];
    TCHAR szKeyName[OLEUI_CCHKEYMAX+50];
    HKEY hkey = NULL, hkeyCLSID = NULL;
    int nKeyLen = 0;
    DWORD cStrings = 0;
    long lSize = MAX_PATH;

    Assert(lstrlen(szFileName) > 0);
    Assert(lstrlen(szClientClsId) > 0);

    // Delete CLSID keys
    CatPathStrN( szTmpID, HKCR_CLSID, szClientClsId, MAX_PATH );

    if (DeleteKeyAndSubKeys(HKEY_CLASSES_ROOT, szTmpID) != ERROR_SUCCESS)
        hr = S_FALSE;   // Keep going, but mark that there was a failure

    // Delete TypeLib info
    if (szTypeLibCLSID != NULL && szTypeLibCLSID[0] != '\0')
    {    
        CatPathStrN( szTmpID, HKCR_TYPELIB, szTypeLibCLSID, MAX_PATH);
        if (DeleteKeyAndSubKeys(HKEY_CLASSES_ROOT, szTmpID) != ERROR_SUCCESS)
            hr = S_FALSE;
    }

    // Delete ModuleUsage keys
    // The canonicalizer can fail if the target file isn't there, so in that case, fall back
    // on szFileName, which may well have come in canonical form from the DU file list.
    if ( OCCGetLongPathName(szTmpRev, szFileName, MAX_PATH) == 0 )
        lstrcpy( szTmpRev, szFileName );
    ReverseSlashes(szTmpRev);

    // Guard against the subkey name being empty, as this will cause us to nuke
    // the entire Module Usage subtree, which is a bad thing to do.
    if ( szTmpRev[0] != '\0' )
    {
        CatPathStrN(szTmpID, REGSTR_PATH_MODULE_USAGE, szTmpRev, MAX_PATH);
        if (DeleteKeyAndSubKeys(HKEY_LOCAL_MACHINE, szTmpID) != ERROR_SUCCESS)
            hr = S_FALSE;

        // Delete SharedDLL value
        if (RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    REGSTR_PATH_SHAREDDLLS,
                    0,
                    KEY_ALL_ACCESS,
                    &hkey) == ERROR_SUCCESS)
        {
            hr = (RegDeleteValue(hkey, szFileName) == ERROR_SUCCESS ? hr : S_FALSE);
            RegCloseKey(hkey);
        }
        else
        {
            hr = S_FALSE;
        }
    }

    // loop through entries under HKEY_CLASSES_ROOT to clear entries
    // whose CLSID subsection is equal to the CLSID of the control
    // being removed
    while ((lResult = RegEnumKey(
                HKEY_CLASSES_ROOT,
                cStrings++,
                szKeyName,
                OLEUI_CCHKEYMAX)) == ERROR_SUCCESS)
    {
        lSize = MAX_PATH;
        nKeyLen = lstrlen(szKeyName);
        lstrcat(szKeyName, "\\");
        lstrcat(szKeyName, HKCR_CLSID);
        if ((RegQueryValue(
                HKEY_CLASSES_ROOT, 
                szKeyName, 
                szTmpID, &lSize) == ERROR_SUCCESS) &&    
            (lstrcmpi(szTmpID, szClientClsId) == 0))
        {
            szKeyName[nKeyLen] = '\0';
            DeleteKeyAndSubKeys(HKEY_CLASSES_ROOT, szKeyName);
            lResult = ERROR_NO_MORE_ITEMS;
            break;
        }

    }

    Assert(lResult == ERROR_NO_MORE_ITEMS);
    if (lResult != ERROR_NO_MORE_ITEMS)
        hr = S_FALSE;

    // loop through all HKEY_CLASSES_ROOT\CLSID entries and remove
    // those with InprocServer32 subsection equal to the name of
    // the OCX file being removed
    if (RegOpenKeyEx(
             HKEY_CLASSES_ROOT, 
             HKCR_CLSID,
             0,
             KEY_ALL_ACCESS,
             &hkey) == ERROR_SUCCESS)
    {
        cStrings = 0;
        while ((lResult = RegEnumKey(
                                hkey,
                                cStrings,
                                szKeyName,
                                OLEUI_CCHKEYMAX)) == ERROR_SUCCESS)
        {
            // check InprocServer32
            lSize = MAX_PATH;
            lstrcat(szKeyName, "\\");
            lstrcat(szKeyName, INPROCSERVER32);
            if ((RegQueryValue(
                       hkey, 
                       szKeyName, 
                       szTmpID, 
                       &lSize) == ERROR_SUCCESS) &&
                (lstrcmpi(szTmpID, szFileName) == 0))
            {
                *(ReverseStrchr(szKeyName, '\\')) = '\0';
                DeleteKeyAndSubKeys(hkey, szKeyName);
                continue;
            }

            // check LocalServer32
            *(ReverseStrchr(szKeyName, '\\') + 1) = '\0';
            lstrcat(szKeyName, LOCALSERVER32);
            if ((RegQueryValue(
                       hkey, 
                       szKeyName, 
                       szTmpID, 
                       &lSize) == ERROR_SUCCESS) &&
                (lstrcmpi(szTmpID, szFileName) == 0))
            {
                *(ReverseStrchr(szKeyName, '\\')) = '\0';
                DeleteKeyAndSubKeys(hkey, szKeyName);
                continue;
            }

            // check LocalServerX86
            *(ReverseStrchr(szKeyName, '\\') + 1) = '\0';
            lstrcat(szKeyName, LOCALSERVERX86);
            if ((RegQueryValue(
                       hkey, 
                       szKeyName, 
                       szTmpID, 
                       &lSize) == ERROR_SUCCESS) &&
                (lstrcmpi(szTmpID, szFileName) == 0))
            {
                *(ReverseStrchr(szKeyName, '\\')) = '\0';
                DeleteKeyAndSubKeys(hkey, szKeyName);
                continue;
            }

            // check InProcServerX86
            *(ReverseStrchr(szKeyName, '\\') + 1) = '\0';
            lstrcat(szKeyName, INPROCSERVERX86);
            if ((RegQueryValue(
                       hkey, 
                       szKeyName, 
                       szTmpID, 
                       &lSize) == ERROR_SUCCESS) &&
                (lstrcmpi(szTmpID, szFileName) == 0))
            {
                *(ReverseStrchr(szKeyName, '\\')) = '\0';
                DeleteKeyAndSubKeys(hkey, szKeyName);
                continue;
            }

            cStrings += 1;
        }

        RegCloseKey(hkey);

        Assert(lResult == ERROR_NO_MORE_ITEMS);
        if (lResult != ERROR_NO_MORE_ITEMS)
            hr = S_FALSE;
    }

    return hr;
}

// Get from a abbreviated filename its full, long name
// eg. from C:\DOC\MyMath~1.txt to C:\DOC\MyMathFile.txt
// lpszShortFileName must has in it both the file name and its full path
// if bToUpper is TRUE, the name returned will be in uppercase
HRESULT ConvertToLongFileName(
                LPTSTR lpszShortFileName,
                BOOL bToUpper /* = FALSE */)
{
    Assert(lpszShortFileName != NULL);
    if (lpszShortFileName == NULL)
        return HRESULT_FROM_WIN32(ERROR_BAD_ARGUMENTS);

    HRESULT hr = S_OK;
    WIN32_FIND_DATA filedata;
    TCHAR *pEndPath = NULL;
    HANDLE h = FindFirstFile(lpszShortFileName, &filedata);

    if (h != INVALID_HANDLE_VALUE)
    {
        FindClose(h);

        // separate filename from path
        pEndPath = ReverseStrchr(lpszShortFileName, '\\');
        if (pEndPath != NULL)
        {
            *(++pEndPath) = '\0';
            lstrcat(pEndPath, filedata.cFileName);
        }
        else
        {
            lstrcpy(lpszShortFileName, filedata.cFileName);
        }

        // to upper case if requested
        if (bToUpper)
            CharUpper(lpszShortFileName);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}

//=--------------------------------------------------------------------------=
// DeleteKeyAndSubKeys
//=--------------------------------------------------------------------------=
// delete's a key and all of it's subkeys.
//
// Parameters:
//    HKEY                - [in] delete the descendant specified
//    LPSTR               - [in] i'm the descendant specified
//
// Output:
//    LONG                - ERROR_SUCCESS if successful
//                        - else, a nonzero error code defined in WINERROR.H
//
// Notes:
//    - I don't feel too bad about implementing this recursively, since the
//      depth isn't likely to get all the great.
//    - Manually removing subkeys is needed for NT.  Win95 does that 
//      automatically
//
// This code was stolen from the ActiveX framework (util.cpp).
LONG DeleteKeyAndSubKeys(HKEY hkIn, LPCTSTR pszSubKey)
{
    HKEY  hk;
    TCHAR szTmp[MAX_PATH];
    DWORD dwTmpSize;
    LONG  lResult;

    lResult = RegOpenKeyEx(hkIn, pszSubKey, 0, KEY_ALL_ACCESS, &hk);
    if (lResult != ERROR_SUCCESS)
        return lResult;

    // loop through all subkeys, blowing them away.
    for (/* DWORD c = 0 */; lResult == ERROR_SUCCESS; /* c++ */)
    {
        dwTmpSize = MAX_PATH;
        lResult = RegEnumKeyEx(hk, 0, szTmp, &dwTmpSize, 0, NULL, NULL, NULL);
        if (lResult == ERROR_NO_MORE_ITEMS)
            break;
        lResult = DeleteKeyAndSubKeys(hk, szTmp);
    }

    // there are no subkeys left, [or we'll just generate an error and return FALSE].
    // let's go blow this dude away.
    //
    dwTmpSize = MAX_PATH;
    Assert(RegEnumKeyEx(hk, 0, szTmp, &dwTmpSize, 0, NULL, NULL, NULL) == ERROR_NO_MORE_ITEMS);
    RegCloseKey(hk);

    lResult = RegDeleteKey(hkIn, pszSubKey);

    return lResult;
}

// return TRUE if file szFileName exists, FALSE otherwise
BOOL FileExist(LPCTSTR lpszFileName)
{
   DWORD dwErrMode;
   BOOL fResult;

   dwErrMode = SetErrorMode(SEM_FAILCRITICALERRORS);

   fResult = ((UINT)GetFileAttributes(lpszFileName) != (UINT)-1);

   SetErrorMode(dwErrMode);

   return fResult;
}

// given a flag, return the appropriate directory
// possible flags are:
//      GD_WINDOWDIR    : return WINDOWS directory
//      GD_SYSTEMDIR    : return SYSTEM directory
//      GD_CONTAINERDIR : return directory of app used to view control (ie IE)
//      GD_CACHEDIR     : return OCX cache directory, read from registry
//      GD_CONFLICTDIR  : return OCX conflict directory, read from registry
//      GD_EXTRACTDIR   : require an extra parameter szOCXFullName, 
//                        extract and return its path 
HRESULT GetDirectory(
                UINT nDirType, 
                LPTSTR szDirBuffer, 
                int nBufSize,
                LPCTSTR szOCXFullName /* = NULL */)
{
    LONG lResult = 0;
    TCHAR *pCh = NULL, *pszKeyName = NULL;
    HRESULT hr = S_OK;
    HKEY hkeyIntSetting = NULL;
    unsigned long ulSize = nBufSize;

    switch (nDirType)
    {

    case GD_WINDOWSDIR:
        if (GetWindowsDirectory(szDirBuffer, nBufSize) == 0)
            hr = HRESULT_FROM_WIN32(GetLastError());
        break;

    case GD_SYSTEMDIR:
        if (GetSystemDirectory(szDirBuffer, nBufSize) == 0)
            hr = HRESULT_FROM_WIN32(GetLastError());
        break;

    case GD_EXTRACTDIR:
        if (szOCXFullName == NULL)
        {
            hr = HRESULT_FROM_WIN32(ERROR_BAD_ARGUMENTS);
            break;
        }
        lstrcpy(szDirBuffer, szOCXFullName);
        pCh = ReverseStrchr(szDirBuffer, '\\');
        Assert (pCh != NULL);
        if (pCh == NULL)
            hr = HRESULT_FROM_WIN32(ERROR_BAD_ARGUMENTS);
        else
            pCh[0] = '\0';
        break;

    case GD_CONTAINERDIR:
        pszKeyName = new TCHAR[MAX_PATH];
        Assert(pszKeyName != NULL);
        if (pszKeyName == NULL)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        CatPathStrN(pszKeyName, REGSTR_PATH_IE, CONTAINER_APP, MAX_PATH);
        if ((lResult = RegOpenKeyEx(
                            HKEY_LOCAL_MACHINE,
                            pszKeyName,
                            0x0,
                            KEY_READ,
                            &hkeyIntSetting)) == ERROR_SUCCESS)
        {
            lResult = RegQueryValueEx(
                                hkeyIntSetting,
                                VALUE_PATH,
                                NULL,
                                NULL,
                                (unsigned char*)szDirBuffer,
                                &ulSize);
        }
        if (lResult != ERROR_SUCCESS)
            hr = HRESULT_FROM_WIN32(lResult);
        else
            szDirBuffer[lstrlen(szDirBuffer)-1] = '\0';  // take away the ending ';'
        delete pszKeyName;
        break;

    case GD_CACHEDIR:
    case GD_CONFLICTDIR:
        if ((lResult = RegOpenKeyEx(
                            HKEY_LOCAL_MACHINE,
                            REGSTR_PATH_IE_SETTINGS,
                            0x0,
                            KEY_READ,
                            &hkeyIntSetting)) == ERROR_SUCCESS)
        {
            lResult = RegQueryValueEx(
                                hkeyIntSetting,
                                VALUE_ACTIVEXCACHE,
                                NULL,
                                NULL,
                                (unsigned char*)szDirBuffer,
                                &ulSize);
        }

        hr = (lResult == ERROR_SUCCESS ? S_OK : HRESULT_FROM_WIN32(lResult));

        // if looking for cache dir, append "\\CONFLICT"
        if (SUCCEEDED(hr) && nDirType == GD_CONFLICTDIR)
            lstrcat(szDirBuffer, DEFAULT_CONFLICT);

        break;

    default:
        Assert(FALSE);
        hr = E_UNEXPECTED;
    }

    if (hkeyIntSetting != NULL)
        RegCloseKey(hkeyIntSetting);

    if (FAILED(hr))
        szDirBuffer[0] = '\0';

    return hr;
}

// retrieve file size for file szFile.  Size returne in pSize.
HRESULT GetSizeOfFile(LPCTSTR lpszFile, LPDWORD lpSize)
{
    HANDLE hFile = NULL;
    WIN32_FIND_DATA fileData;

    *lpSize = 0;
    Assert(lpszFile != NULL);

    hFile = FindFirstFile(lpszFile, &fileData);
    if (hFile == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(GetLastError());

    FindClose(hFile);
    *lpSize = fileData.nFileSizeLow;

    // Get cluster size to calculate the real # of bytes
    // taken up by the file

    DWORD dwSectorPerCluster, dwBytePerSector;
    DWORD dwFreeCluster, dwTotalCluster;
    TCHAR szRoot[4];
    lstrcpyn(szRoot, lpszFile, 4);

    if (!GetDiskFreeSpace(
                    szRoot, &dwSectorPerCluster, &dwBytePerSector, 
                    &dwFreeCluster, &dwTotalCluster))
        return HRESULT_FROM_WIN32(GetLastError());

    DWORD dwClusterSize =  dwSectorPerCluster * dwBytePerSector;
    *lpSize = ((*lpSize/dwClusterSize) * dwClusterSize +
                        (*lpSize % dwClusterSize ? dwClusterSize : 0));

    return S_OK;
}

// Return S_OK is lpszCLSID is in ModuleUsage section of lpszFileName.
// Return E_... otherwise.
// lpszCLSID can be NULL, in this case it does not search for the CLSID.
// If lpszOwner is not NULL, it must point to a buffer which will be
// used to store the owner of the ModuleUsage section for lpszFileName
// dwOwnerSize is the size of the buffer pointed to by lpszOwner .
HRESULT LookUpModuleUsage(
                      LPCTSTR lpszFileName,
                      LPCTSTR lpszCLSID,
                      LPTSTR lpszOwner /* = NULL */, 
                      DWORD dwOwnerSize /* = 0 */)
{
    HKEY hkey = NULL, hkeyMod = NULL;
    HRESULT hr = S_OK;
    TCHAR szBuf[MAX_PATH];
    LONG lResult = ERROR_SUCCESS;

    if ((lResult = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE, 
                        REGSTR_PATH_MODULE_USAGE,
                        0, 
                        KEY_READ, 
                        &hkeyMod)) != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(lResult);
        goto EXITLOOKUPMODULEUSAGE;
    }


    if ( OCCGetLongPathName(szBuf, lpszFileName, MAX_PATH) == 0 )
        lstrcpyn( szBuf, lpszFileName, MAX_PATH );
    szBuf[256] = '\0'; // truncate if longer than 255 ude to win95 registry bug


    lResult = RegOpenKeyEx(
                        hkeyMod, 
                        szBuf,
                        0, 
                        KEY_READ, 
                        &hkey);
    if (lResult != ERROR_SUCCESS)
    {
        ReverseSlashes(szBuf);
        lResult = RegOpenKeyEx(hkeyMod, szBuf, 0, KEY_READ, &hkey);
        if (lResult != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(lResult);
            goto EXITLOOKUPMODULEUSAGE;
        }
    }

    // Get owner if requested
    if (lpszOwner != NULL)
    {
        DWORD dwSize = dwOwnerSize;
        lResult = RegQueryValueEx(
                            hkey,
                            VALUE_OWNER,
                            NULL,
                            NULL,
                            (unsigned char*)lpszOwner,
                            &dwSize);
        if (lResult != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(lResult);
            lpszOwner[0] = '\0';
            goto EXITLOOKUPMODULEUSAGE;
        }
    }

    // see if lpszCLSID is a client of this module usage section
    if (lpszCLSID != NULL)
    {
        lResult = RegQueryValueEx(
                            hkey,
                            lpszCLSID,
                            NULL,
                            NULL,
                            NULL,
                            NULL);
        if (lResult != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(lResult);
            goto EXITLOOKUPMODULEUSAGE;
        }
    }

EXITLOOKUPMODULEUSAGE:

    if (hkey)
        RegCloseKey(hkey);

    if (hkeyMod)
        RegCloseKey(hkeyMod);

    return hr;
}

// ReverseSlashes() takes a string, that's assumed to be pointing to a
// valid string and is null-terminated, and reverses all forward slashes
// to backslashes and all backslashes to forward slashes.
void ReverseSlashes(LPTSTR pszStr)
{
    while (*pszStr)
    {
        if (*pszStr == '\\')  *pszStr = '/';
        else if (*pszStr == '/')  *pszStr = '\\';
        pszStr++;
    }
}

// find the last occurance of ch in string szString
TCHAR* ReverseStrchr(LPCTSTR szString, TCHAR ch)
{
    if (szString == NULL || szString[0] == '\0')
        return NULL;
    TCHAR *pCh = (TCHAR*)(szString + lstrlen(szString));
    for (;pCh != szString && *pCh != ch; pCh--);
    return (*pCh == ch ? pCh : NULL);
}


// If lpszGUID is an owner of the module lpszFileName in Module Usage,
// remove it, updating the .Owner as necessary. If we remove an owner,
// then decrement the SharedDlls count. Never drop the SharedDlls count
// below 1 if the owner is 'Unknown Owner'.
// If modules usage drops to zero, remove MU. If SharedDlls count drops
// to zero, remove that value.
// Return the resulting owner count.
 
DWORD SubtractModuleOwner( LPCTSTR lpszFileName, LPCTSTR lpszGUID )
{
    LONG cRef = 0;
    HRESULT hr = S_OK;
    LONG lResult;
    HKEY hkeyMU = NULL;
    HKEY hkeyMod = NULL;
    TCHAR szBuf[MAX_PATH];
    TCHAR szOwner[MAX_PATH];
    DWORD dwSize = MAX_PATH;
    BOOL bHasUnknownOwner;
    BOOL bGUIDIsOwner;

    Assert(lpszFileName != NULL);
    Assert(lpszGUID != NULL);

    // Get the current ref count, passing -1 to set is a get. Go figure.
    hr = SetSharedDllsCount( lpszFileName, -1, &cRef );
    if ( FAILED(hr) )
        return 1; // in event of failure, say something safe

    // check if Usage section is present for this dll
    // open the file's section we are concerned with

    if ((lResult = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE, 
                        REGSTR_PATH_MODULE_USAGE,
                        0, 
                        KEY_ALL_ACCESS, 
                        &hkeyMU)) != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(lResult);
        goto ExitSubtractModuleOwner;
    }
    

    if ( OCCGetLongPathName(szBuf, lpszFileName, MAX_PATH) == 0 )
        lstrcpyn( szBuf, lpszFileName, MAX_PATH );

    szBuf[256] = '\0'; // truncate if longer than 255 ude to win95 registry bug
    ReverseSlashes(szBuf);

    // open section for szFileName under ModuleUsage
    if ((lResult = RegOpenKeyEx(
                            hkeyMU, 
                            szBuf,
                            0, 
                            KEY_ALL_ACCESS,
                            &hkeyMod)) != ERROR_SUCCESS)
    {
        goto ExitSubtractModuleOwner;
    }

    dwSize = MAX_PATH;
    if ((lResult = RegQueryValueEx(
                            hkeyMod,
                            VALUE_OWNER,
                            NULL,
                            NULL,
                            (unsigned char*)szOwner,
                            &dwSize)) != ERROR_SUCCESS)
    {
        goto ExitSubtractModuleOwner;
    }

    bHasUnknownOwner = lstrcmp( szOwner, MODULE_UNKNOWN_OWNER ) == 0;

    bGUIDIsOwner = lstrcmp( szOwner, lpszGUID ) == 0;

    // remove the owner value entry, if any.
    lResult = RegDeleteValue(hkeyMod, lpszGUID);
    // if this worked, then we'll need to drop the SharedDlls count,
    // being careful not to let it fall below 1 if bHasUnknownOwner
    if ( lResult == ERROR_SUCCESS )
    {
        if ( !bHasUnknownOwner || cRef > 1 )
            SetSharedDllsCount( lpszFileName, --cRef, NULL );

        if ( cRef > 0 && bGUIDIsOwner )
        {
            DWORD dwEnumIndex; 
            // lpszGUID was the .Owner, now that it's gone, replace it
            // with another owner
            for ( dwEnumIndex = 0, dwSize = MAX_PATH;
                  RegEnumValue(hkeyMod, dwEnumIndex, (char *)szOwner,
                               &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS;
                  dwEnumIndex++, dwSize = MAX_PATH )
            {
                if (szOwner[0] != '.')
                {
                    lResult = RegSetValueEx( hkeyMod,VALUE_OWNER, 0,
                                             REG_SZ, (LPBYTE)szOwner,
                                             (lstrlen( szOwner ) + 1) * sizeof(TCHAR) );
                    break; // we've done our job
                }
            } // for find a new owner
        } // if there are still owners, but we've nuked the owner of record.
        else if ( cRef == 0 )
        {
            // that was the last ref, so nuke the MU entry
            RegCloseKey( hkeyMod );
            hkeyMod = NULL;
            RegDeleteKey( hkeyMU, szBuf ); // note - we assume this key has no subkeys.

            // Take out the shared DLL's value
            HKEY hkey;

            lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE, 
                                    REGSTR_PATH_SHAREDDLLS, 0, KEY_ALL_ACCESS,
                                    &hkey);
            if ( lResult == ERROR_SUCCESS )
            {
                ReverseSlashes(szBuf); // revert to file sys
                lResult = RegDeleteValue( hkey, szBuf );
                RegCloseKey( hkey );
            } // if opened SharedDlls
        } // else last reference
    } // if removed an owner

ExitSubtractModuleOwner:

    if (hkeyMU)
        RegCloseKey(hkeyMU);

    if (hkeyMod)
        RegCloseKey(hkeyMod);

    return cRef;
}

// Set manually the count in SharedDlls.
// If dwCount is < 0, nothing is set.
// If pdwOldCount is non-null, the old count is returned
HRESULT SetSharedDllsCount(
                    LPCTSTR lpszFileName, 
                    LONG cRef, 
                    LONG *pcRefOld /* = NULL */)
{
    HRESULT hr = S_OK;
    LONG lResult = ERROR_SUCCESS;
    DWORD dwSize = 0;
    HKEY hkey = NULL;

    Assert(lpszFileName != NULL);
    if (lpszFileName == NULL)
    {
        hr = HRESULT_FROM_WIN32(ERROR_BAD_ARGUMENTS);
        goto EXITSETSHAREDDLLSCOUNT;
    }

    if (cRef < 0 && pcRefOld == NULL)
    {
        goto EXITSETSHAREDDLLSCOUNT;
    }

    // open HKLM, Microsoft\Windows\CurrentVersion\SharedDlls
    lResult = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE, 
                    REGSTR_PATH_SHAREDDLLS, 0, KEY_ALL_ACCESS,
                    &hkey);

    if (lResult != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(lResult);
        goto EXITSETSHAREDDLLSCOUNT;
    }

    // if pdwOldCount is not NULL, save the old count in it
    if (pcRefOld != NULL)
    {
        dwSize = sizeof(DWORD);
        lResult = RegQueryValueEx(
                    hkey,
                    lpszFileName,
                    0,
                    NULL,
                    (unsigned char*)pcRefOld,
                    &dwSize);
        if (lResult != ERROR_SUCCESS)
        {
            *pcRefOld = 0;
            hr = S_FALSE;
            goto EXITSETSHAREDDLLSCOUNT;
        }
    }

    // if dwCount >= 0, set it as the new count
    if (cRef >= 0)
    {
        lResult = RegSetValueEx(
                     hkey, 
                     lpszFileName, 
                     0, 
                     REG_DWORD, 
                     (unsigned char*)&cRef, 
                     sizeof(DWORD)); 
        if (lResult != ERROR_SUCCESS)
        {
            hr = S_FALSE;
            goto EXITSETSHAREDDLLSCOUNT;
        }
    }

EXITSETSHAREDDLLSCOUNT:

    if (hkey != NULL)
        RegCloseKey(hkey);

    return hr;
}

// UnregisterOCX() attempts to unregister a DLL or OCX by calling LoadLibrary
// and then DllUnregisterServer if the LoadLibrary succeeds.  This function
// returns TRUE if the DLL or OCX could be unregistered or if the file isn't
// a loadable module.
HRESULT UnregisterOCX(LPCTSTR pszFile)
{
    HINSTANCE hLib;
    HRESULT hr = S_OK;
    HRESULT (FAR STDAPICALLTYPE * pUnregisterEntry)(void);

    hLib = LoadLibrary(pszFile);

    if (hLib == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        (FARPROC &) pUnregisterEntry = GetProcAddress(
            hLib,
            "DllUnregisterServer"
            );

        if (pUnregisterEntry != NULL)
        {
            hr = (*pUnregisterEntry)();
        }

        FreeLibrary(hLib);
    }

    return hr;
}

// Return S_OK if dll can be removed, or S_FALSE if it cannot.
// Return E_... if an error has occured
HRESULT UpdateSharedDlls(LPCTSTR szFileName, BOOL bUpdate)
{
    HKEY hkeySD = NULL;
    HRESULT hr = S_OK;
    DWORD dwType;
    DWORD dwRef = 1;
    DWORD dwSize = sizeof(DWORD);
    LONG lResult;

    // get the main SHAREDDLLS key ready; this is never freed!
    if ((lResult = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE, REGSTR_PATH_SHAREDDLLS,
                        0, KEY_ALL_ACCESS, &hkeySD)) != ERROR_SUCCESS)
    {
        hkeySD = NULL;
        hr = HRESULT_FROM_WIN32(lResult);
        goto ExitUpdateSharedDlls;
    }

    // now look for szFileName
    lResult = RegQueryValueEx(hkeySD, szFileName, NULL, &dwType, 
                        (unsigned char*)&dwRef, &dwSize);

    if (lResult != ERROR_SUCCESS)
    {
        hr = S_FALSE;
        goto ExitUpdateSharedDlls;
    }

    // decrement reference count by 1.
    //
    // if (count equals to 0) 
    //    if (bUpdate is TRUE)
    //         remove the key from SharedDlls
    //    return S_OK
    // otherwise
    //    if (bUpdate is TRUE)
    //       update the count
    //    return S_FALSE

    if ((--dwRef) > 0)
    {
        hr = S_FALSE;
        if (bUpdate &&
            (lResult = RegSetValueEx(hkeySD, szFileName, 0, REG_DWORD,
                        (unsigned char *)&dwRef, sizeof(DWORD))) != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(lResult);
         }
        goto ExitUpdateSharedDlls;
    }

    Assert(dwRef == 0);

    // remove entry from SharedDlls
    if (bUpdate &&
        (lResult = RegDeleteValue(hkeySD, szFileName)) != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(lResult);
        goto ExitUpdateSharedDlls;
    }
 
ExitUpdateSharedDlls:

    if (hkeySD)
        RegCloseKey(hkeySD);

    return hr;
}

void RemoveList(LPCLSIDLIST_ITEM lpListHead)
{
    LPCLSIDLIST_ITEM lpItemPtr = NULL; 
    while (TRUE)
    {
        lpItemPtr = lpListHead;
        if (lpItemPtr == NULL)
            break;
        lpListHead = lpItemPtr->pNext;
        delete lpItemPtr;
    }
    lpListHead = NULL;
}

BOOL ReadInfFileNameFromRegistry(
                             LPCTSTR lpszCLSID, 
                             LPTSTR lpszInf,
                             LONG nBufLen)
{
    if (lpszCLSID == NULL || lpszInf == NULL)
        return FALSE;

    HKEY hkey = NULL;
    TCHAR szKey[100];
    LONG lResult = ERROR_SUCCESS;

    CatPathStrN( szKey, HKCR_CLSID, lpszCLSID, 100);
    CatPathStrN( szKey, szKey, INFFILE, 100);

    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szKey, 0, KEY_READ, &hkey) != ERROR_SUCCESS)
        return FALSE;

    lResult = RegQueryValue(hkey, NULL, lpszInf, &nBufLen);
    RegCloseKey(hkey);

    if (lResult != ERROR_SUCCESS)
    {
        lpszInf[0] = '\0';
    }

    if (lpszInf[0] == '\0')
        return FALSE;

    return TRUE;
}

BOOL WriteInfFileNameToRegistry(LPCTSTR lpszCLSID, LPTSTR lpszInf)
{
    if (lpszCLSID == NULL)
        return FALSE;

    HKEY hkey = NULL;
    LONG lResult = ERROR_SUCCESS;
    TCHAR szKey[100];
    
    CatPathStrN(szKey, HKCR_CLSID, lpszCLSID, 100);
    CatPathStrN(szKey, szKey, INFFILE, 100);

    if (RegCreateKey(HKEY_CLASSES_ROOT, szKey, &hkey) != ERROR_SUCCESS)
        return FALSE;

    lResult = RegSetValue(
                         hkey,
                         NULL,
                         REG_SZ,
                         (lpszInf == NULL ? TEXT("") : lpszInf),
                         (lpszInf == NULL ? 0 : lstrlen(lpszInf)));
    RegCloseKey(hkey);

    return (lResult == ERROR_SUCCESS);
} 

// Define a macro to make life easier
#define QUIT_IF_FAIL if (FAILED(hr)) goto Exit


HRESULT
ExpandVar(
    LPCSTR& pchSrc,          // passed by ref!
    LPSTR& pchOut,          // passed by ref!
    DWORD& cbLen,           // passed by ref!
    DWORD cbBuffer,
    const char * szVars[],
    const char * szValues[])
{
    HRESULT hr = S_FALSE;
    int cbvar = 0;

    Assert (*pchSrc == '%');

    for (int i=0; szVars[i] && (cbvar = lstrlen(szVars[i])) ; i++) { // for each variable

        int cbneed = 0;

        if ( (szValues[i] == NULL) || !(cbneed = lstrlen(szValues[i])))
            continue;

        cbneed++;   // add for nul

        if (0 == strncmp(szVars[i], pchSrc, cbvar)) {

            // found something we can expand

                if ((cbLen + cbneed) >= cbBuffer) {
                    // out of buffer space
                    *pchOut = '\0'; // term
                    hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                    goto Exit;
                }

                lstrcpy(pchOut, szValues[i]);
                cbLen += (cbneed -1); //don't count the nul

                pchSrc += cbvar;        // skip past the var in pchSrc
                pchOut += (cbneed -1);  // skip past dir in pchOut

                hr = S_OK;
                goto Exit;

        }
    }

Exit:

    return hr;
    
}

// from urlmon\download\hooks.cxx (ExpandCommandLine and ExpandVars)
// used to expand variables
HRESULT
ExpandCommandLine(
    LPCSTR szSrc,
    LPSTR szBuf,
    DWORD cbBuffer,
    const char * szVars[],
    const char * szValues[])
{
    Assert(cbBuffer);


    HRESULT hr = S_FALSE;

    LPCSTR pchSrc = szSrc;     // start parsing at begining of cmdline

    LPSTR pchOut = szBuf;       // set at begin of out buffer
    DWORD cbLen = 0;

    while (*pchSrc) {

        // look for match of any of our env vars
        if (*pchSrc == '%') {

            HRESULT hr1 = ExpandVar(pchSrc, pchOut, cbLen, // all passed by ref!
                cbBuffer, szVars, szValues);  

            if (FAILED(hr1)) {
                hr = hr1;
                goto Exit;
            }


            if (hr1 == S_OK) {    // expand var expanded this
                hr = hr1;
                continue;
            }
        }
            
        // copy till the next % or nul
        if ((cbLen + 1) < cbBuffer) {

            *pchOut++ = *pchSrc++;
            cbLen++;

        } else {

            // out of buffer space
            *pchOut = '\0'; // term
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            goto Exit;

        }


    }

    *pchOut = '\0'; // term


Exit:

    return hr;
}
// Find dependent DLLs in ModuleUsage
// given the clsid it will enumerate all the DLLs in the ModuleUsage
// that were used by this clsid
HRESULT FindDLLInModuleUsage(
      LPTSTR lpszFileName,
      LPCTSTR lpszCLSID,
      DWORD &iSubKey)
{
    HKEY hkey = NULL, hkeyMod = NULL;
    HRESULT hr = S_OK;
    TCHAR szBuf[MAX_PATH];
    LONG lResult = ERROR_SUCCESS;

    if (lpszCLSID == NULL) {
        hr = E_INVALIDARG;  // req clsid
        goto Exit;
    }

    // get the main MODULEUSAGE key
    if ((lResult = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE, 
                        REGSTR_PATH_MODULE_USAGE,
                        0, 
                        KEY_READ, 
                        &hkeyMod)) != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(lResult);
        goto Exit;
    }

    while ( ((lResult = RegEnumKey(
                            hkeyMod, 
                            iSubKey++,
                            szBuf, 
                            MAX_PATH)) == ERROR_SUCCESS) ) {

        lResult = RegOpenKeyEx(
                            hkeyMod, 
                            szBuf,
                            0, 
                            KEY_READ, 
                            &hkey);

        if (lResult != ERROR_SUCCESS)
            break;

        // see if lpszCLSID is a client of this module usage section
        lResult = RegQueryValueEx(
                            hkey,
                            lpszCLSID,
                            NULL,
                            NULL,
                            NULL,
                            NULL);
        if (lResult == ERROR_SUCCESS)
        {
            // got the filename, return it
            lstrcpy(lpszFileName, szBuf);
            goto Exit;
        }

        if (hkey) {
            RegCloseKey(hkey);
            hkey = NULL;
        }

    } // while

    if (lResult != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(lResult);
    }

Exit:

    if (hkey)
        RegCloseKey(hkey);

    if (hkeyMod)
        RegCloseKey(hkeyMod);

    return hr;
}

BOOL PatternMatch(LPCTSTR szModName, LPTSTR szSectionName)
{

    LPCTSTR pch = ReverseStrchr(szModName, '/');
    DWORD len = 0;

    if (!pch)
        pch = szModName;
    else
        pch++;

    // pch points at base name of module

    if ((len = lstrlen(pch)) != (DWORD)lstrlen(szSectionName))
        return FALSE;

    LPTSTR pchSecStar = StrChr(szSectionName, '*'); 

    Assert(pchSecStar);

    DWORD cbLen1 = (DWORD) (pchSecStar - szSectionName);

    // compare upto '*'
    if (StrCmpNI(szSectionName, pch, cbLen1) != 0) 
        return FALSE;

    // compare after the 3 stars
    if ( (cbLen1 + 3) < len) // *s not at end
        if (StrCmpNI(pchSecStar+3, pch + (cbLen1+3), len -(cbLen1+3)) != 0) 
            return FALSE;

    // simlar strings but for the stars.

    // modify the szSectionName to hold the value for the stars
    // in-effect this will substitute the original variable with
    // the value that was used when installing the OCX

    lstrcpy(pchSecStar, pch + cbLen1);

    return TRUE;
}

DWORD OCCGetLongPathName( LPTSTR szLong, LPCTSTR szShort, DWORD cchBuffer )
{
    DWORD   dwLen = 0;
    HMODULE hmodUrlMon;
    CDLGetLongPathNamePtr pfnGetLongPathName = NULL;

    hmodUrlMon = LoadLibrary( "URLMON.DLL" );

    // Set up our globals with short and long versions of the base cache path 
    if ( hmodUrlMon != NULL ) {
        pfnGetLongPathName = (CDLGetLongPathNamePtr)GetProcAddress(hmodUrlMon, (LPCSTR)STR_CDLGETLONGPATHNAME );
 
        if ( pfnGetLongPathName != NULL ) {
            dwLen = pfnGetLongPathName( szLong, szShort, cchBuffer );
        }  
        FreeLibrary( hmodUrlMon );
    }

    return dwLen;
}

TCHAR *CatPathStrN( TCHAR *szDst, const TCHAR *szHead, const TCHAR *szTail, int cchDst )
{
    TCHAR *szRet = szDst;
    int cchHead = lstrlen(szHead);
    int cchTail = lstrlen(szTail);

    if ( cchHead + cchTail >= (cchDst - 2) ) {// - 2 for / and null
        Assert(FALSE);
        szRet = NULL;
        *szDst = 0;
    }
    else { // we know the whole thing is safe
        lstrcpy(szDst, szHead);
        lstrcpy(&szDst[cchHead], TEXT("\\"));
        lstrcpy(&szDst[cchHead + 1], szTail);
    }

    return szRet;
}

BOOL IsCanonicalName( LPTSTR szName )
{
    // simple test - if there's a ~ in it, it has a contraction in it
    // and is therefore non-canonical
    for ( ; *szName != '\0' && *szName != '~'; szName++ );
    
    return *szName != '~';
};

struct RegPathName {
    LPTSTR   m_szName;
    LPTSTR   m_szCanonicalName;

    RegPathName(void) : m_szName(NULL), m_szCanonicalName(NULL)
    {};
    ~RegPathName()
    {
         if ( m_szName )
            delete m_szName;

         if ( m_szCanonicalName )
            delete m_szCanonicalName;
    };

    void MakeRegFriendly( LPTSTR szName )
    {
        TCHAR *pch;
        // If szName is going to be a reg key name, we can't have it lookin' like a path
        for ( pch = szName; *pch != '\0'; pch++ )
            if ( *pch == '\\' ) *pch = '/';
    }

    void MakeFileSysFriendly( LPTSTR szName )
    {
        TCHAR *pch;
        // change the slashes back into DOS
        // directory \'s
        for ( pch = szName; *pch != '\0'; pch++ )
            if ( *pch == '/' ) *pch = '\\';
    }

    BOOL FSetCanonicalName(void)
    {
        BOOL fSet = FALSE;
        TCHAR *szT = new TCHAR[MAX_PATH];

        if ( m_szName != NULL && szT != NULL ) {
            LPITEMIDLIST pidl = NULL;
            // WE jump through some hoops to get the all-long
            // name version of szName. First we convert it to
            // an ITEMIDLIST.

            // but first, we must change the slashes back into DOS
            // directory \'s
            MakeFileSysFriendly( m_szName );
            if ( OCCGetLongPathName( szT, m_szName, MAX_PATH ) != 0 ) {
                m_szCanonicalName = szT;
                fSet = TRUE;
            } else
                delete szT;

            // restore m_szName to it's registry-friendly form
            MakeRegFriendly( m_szName );
 
        } // if we can get our temp string

        if ( fSet ) { // whatever its source, our canonical form has reversed slashes
           MakeRegFriendly( m_szCanonicalName );
        }
        return fSet;
    };

    BOOL FSetName( LPTSTR szName, int cchName )
    {
        BOOL fSet = FALSE;

        if ( m_szName != NULL ) {
            delete m_szName;
            m_szName = NULL;
        }

        if ( m_szCanonicalName != NULL ) {
            delete m_szCanonicalName;
            m_szCanonicalName = NULL;
        }

        // we got a short name, so szName is the short name
        m_szName = new TCHAR[cchName + 1];
        if ( m_szName != NULL ) {
            lstrcpy( m_szName, szName );
            fSet = FSetCanonicalName();
        }

        return fSet;
    };
}; 

struct ModuleUsageKeys : public RegPathName {
    ModuleUsageKeys   *m_pmukNext;
    HKEY              m_hkeyShort; // key with the short file name name

    ModuleUsageKeys(void) : m_pmukNext(NULL), m_hkeyShort(NULL) {};
    ~ModuleUsageKeys(void)
    {
        if ( m_hkeyShort )
            RegCloseKey( m_hkeyShort );
    };

    HRESULT MergeMU( HKEY hkeyCanon, HKEY hkeyMU )
    {
        HRESULT hr = E_FAIL;
        DWORD   dwIndex = 0;
        DWORD   cchNameMax;
        DWORD   cbValueMax;

        if ( RegQueryInfoKey( m_hkeyShort,
                              NULL, NULL, NULL, NULL, NULL, NULL,
                              &dwIndex, &cchNameMax, &cbValueMax,
                              NULL, NULL ) == ERROR_SUCCESS ) {
            LPTSTR szName = new TCHAR[cchNameMax + 1];
            LPBYTE lpbValue = new BYTE[cbValueMax];

            if ( szName != NULL && lpbValue != NULL ) {
                // Examine each value.
                for ( dwIndex--, hr = S_OK; (LONG)dwIndex >= 0 && SUCCEEDED(hr); dwIndex-- ) {
                    LONG  lResult;
                    DWORD cchName = cchNameMax + 1;
                    DWORD dwType;
                    DWORD dwSize = cbValueMax;
 
                    // fetch key and value
                    lResult = RegEnumValue( m_hkeyShort,
                                            dwIndex, 
                                            szName, 
                                            &cchName,
                                            0,
                                            &dwType,
                                            lpbValue, 
                                            &dwSize );

                    if ( lResult == ERROR_SUCCESS ) {
                        // Do not replace if the canonical entry already has a
                        // .Owner value that is "Unknown Owner"
                        if ( lstrcmp( szName, ".Owner" ) == 0 ) {
                            TCHAR szCanonValue[MAX_PATH];
                            DWORD dwType;
                            DWORD lcbCanonValue = MAX_PATH;
                            if ( RegQueryValueEx( hkeyCanon, ".Owner", NULL, &dwType,
                                                  (LPBYTE)szCanonValue, &lcbCanonValue ) == ERROR_SUCCESS &&
                                 lstrcmp( szCanonValue, "Unknown Owner" ) == 0 )
                                continue;
                        }

                        // Add the value to the canonical version of the key
                        if ( RegSetValueEx( hkeyCanon, szName, NULL, dwType,
                                            lpbValue, dwSize ) != ERROR_SUCCESS )
                            hr = E_FAIL;
                           
                    } else
                        hr = E_FAIL;
                } // for each value in the non-canoncical key

                // Now we are finished with the non-canonical key
                if ( SUCCEEDED(hr) &&
                     RegDeleteKey( hkeyMU, m_szName ) != ERROR_SUCCESS )
                    hr = E_FAIL;
            } else // couldn't allocate name and data buffers
                hr = E_OUTOFMEMORY;
        } 

        return hr;
    };

    HRESULT MergeSharedDlls( HKEY hkeySD )
    {
        HRESULT hr = E_FAIL;
        DWORD dwShortVal = 0;
        DWORD dwCanonicalVal = 0;
        DWORD dwType;
        DWORD dwSize;
        
        // The value names under shared DLLs are raw paths
        MakeFileSysFriendly( m_szName );
        MakeFileSysFriendly( m_szCanonicalName );

        dwSize = sizeof(DWORD);
        if ( RegQueryValueEx( hkeySD, m_szName, NULL,
                              &dwType, (LPBYTE)&dwShortVal, &dwSize ) == ERROR_SUCCESS &&
              dwType == REG_DWORD ) {
            dwCanonicalVal = 0;
            dwSize = sizeof(DWORD);
            // the canonical form may not be there, so we don't care if this
            // fails.
            RegQueryValueEx( hkeySD, m_szCanonicalName, NULL,
                             &dwType, (LPBYTE)&dwCanonicalVal, &dwSize );
            dwCanonicalVal += dwShortVal;
            dwSize = sizeof(DWORD);
            if ( RegSetValueEx( hkeySD, m_szCanonicalName, NULL, REG_DWORD,
                                (LPBYTE)&dwCanonicalVal, dwSize ) == ERROR_SUCCESS ) {
                RegDeleteValue( hkeySD, m_szName );
            }
        } else {
            dwCanonicalVal = 1;
            dwSize = sizeof(DWORD);
            if ( RegSetValueEx( hkeySD, m_szCanonicalName, NULL, REG_DWORD,
                                (LPBYTE)&dwCanonicalVal, dwSize ) == ERROR_SUCCESS )
                hr = S_OK;
        }

        MakeRegFriendly( m_szName );
        MakeRegFriendly( m_szCanonicalName );

        return hr;
    }

    HRESULT CanonicalizeMU( HKEY hkeyMU, HKEY hkeySD )
    {
        HRESULT hr = E_FAIL; 
        HKEY    hkeyCanon;
        LONG    lResult = RegOpenKeyEx( hkeyMU, m_szCanonicalName, 0, KEY_ALL_ACCESS, &hkeyCanon);
           
            
        if ( lResult != ERROR_SUCCESS )
            lResult = RegCreateKey( hkeyMU, 
                                    m_szCanonicalName, 
                                    &hkeyCanon );

        if ( lResult == ERROR_SUCCESS ) {
            hr = MergeMU( hkeyCanon, hkeyMU );
            if ( SUCCEEDED(hr) )
                hr = MergeSharedDlls( hkeySD );
            RegCloseKey( hkeyCanon );
        } else
            hr = E_FAIL;

        return S_OK;
    };
};

// FAddModuleUsageKeys adds a module usage key to the list.

BOOL FAddModuleUsageKeys( ModuleUsageKeys*&pmuk, // head of ModuleUsageKeys list
                         LPTSTR szName,          // name of key value
                         DWORD  cchName,         // length of szName, minus null terminator
                         HKEY  hkeyMU            // hkey of parent
                        )
{
    BOOL fAdd = FALSE;
    ModuleUsageKeys* pmukNew;
    HKEY hkeySub = NULL;
    LRESULT lr;

    pmukNew = new ModuleUsageKeys;
    if ( pmukNew &&
         (lr = RegOpenKeyEx( hkeyMU, szName, 0, KEY_ALL_ACCESS, &hkeySub)) == ERROR_SUCCESS ) {
 
        fAdd = pmukNew->FSetName( szName, cchName );

        if ( fAdd ) {
            // append to head of the list
            pmukNew->m_hkeyShort = hkeySub;
            pmukNew->m_pmukNext = pmuk;
            pmuk = pmukNew;
        }
    }

    if ( !fAdd ) {
        if ( hkeySub )
            RegCloseKey( hkeySub );
        if ( pmukNew != NULL )
            delete pmukNew;
    }

    return fAdd;
}

EXTERN_C HRESULT
CanonicalizeModuleUsage(void)
{
    HKEY hkeyMU = NULL;
    HKEY hkeySD = NULL;
    HRESULT hr = S_OK;
    LONG lResult;

    // get the main SHAREDDLLS key ready; this is never freed!


    if ((lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REGSTR_PATH_SHAREDDLLS,
                        0, KEY_ALL_ACCESS, &hkeySD)) == ERROR_SUCCESS &&
        (lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REGSTR_PATH_MODULE_USAGE,
                        0, KEY_ALL_ACCESS, &hkeyMU)) == ERROR_SUCCESS )
    {
        DWORD          dwIndex = 0;
        ModuleUsageKeys *pmukUpdate = NULL; // records for values we want to update.
        ModuleUsageKeys *pmuk;
        ModuleUsageKeys *pmukNext;
 
        // Examine each value.
        do  {
            TCHAR szName[MAX_PATH];        // Value name
            DWORD cchName = MAX_PATH;
            FILETIME ftT;

            // fetch key and value
            lResult = RegEnumKeyEx( hkeyMU, dwIndex, szName, &cchName,
                                    0, NULL, NULL, &ftT );

             if ( lResult == ERROR_SUCCESS ) {

                if ( !IsCanonicalName( szName ) )
                    if ( !FAddModuleUsageKeys( pmukUpdate, szName, cchName, hkeyMU ) )
                        hr = E_OUTOFMEMORY;
                dwIndex++;
             } else if ( lResult == ERROR_NO_MORE_ITEMS )
                 hr = S_FALSE;
             else
                 hr = E_FAIL;
        } while ( hr == S_OK );

   
        if ( SUCCEEDED(hr) ) {
            hr = S_OK; // don't need S_FALSE any longer
            for ( pmuk = pmukUpdate; pmuk != NULL; pmuk = pmukNext ) {
                HRESULT hr2 = pmuk->CanonicalizeMU( hkeyMU, hkeySD );
                if ( FAILED(hr2) )
                    hr = hr2;
                pmukNext = pmuk->m_pmukNext; 
                delete pmuk;
            } // for 
        } //  if enumeration succeeded
    } // if keys opened

    if (hkeyMU)
        RegCloseKey(hkeyMU);

    if ( hkeySD )
        RegCloseKey( hkeySD );

    return hr;
}

