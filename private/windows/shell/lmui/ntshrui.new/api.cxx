//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       api.cxx
//
//  Contents:   Exported APIs from this DLL
//
//  History:    5-Oct-95 BruceFo  Created
//
//--------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "resource.h"
#include "dllmain.hxx"
#include "shrpage.hxx"
#include "shrinfo.hxx"
#include "cache.hxx"
#include "util.hxx"

//--------------------------------------------------------------------------

static BOOL WINAPI
SharingDialogHelp(
    HWND   hwndParent,
    LPWSTR pszPath          // this is 'new' memory that I take ownership of
    );

//--------------------------------------------------------------------------
// A couple macros to use a stack buffer if smaller than a default size,
// else use a heap buffer.

// Example:
//      LPWSTR pBuffer;
//      DWORD dwBufLen;
//      GET_BUFFER(pBuffer, dwBufLen, WCHAR, wcslen(pszString) + 1, MAX_PATH);
//      if (NULL == pBuffer) { return NULL; } // couldn't get the buffer 
//      ... play with pBuffer
//      FREE_BUFFER(pBuffer);

#define GET_BUFFER(pBuffer, dwBufLen, type, desiredLen, defaultLen) \
    DWORD __desiredLen ## pBuffer = desiredLen;                     \
    type __szTmpBuffer ## pBuffer[defaultLen];                      \
    if (__desiredLen ## pBuffer <= defaultLen)                      \
    {                                                               \
        pBuffer = __szTmpBuffer ## pBuffer;                         \
        dwBufLen = defaultLen;                                      \
    }                                                               \
    else                                                            \
    {                                                               \
        pBuffer = new type[__desiredLen ## pBuffer];                \
        if (NULL != pBuffer)                                        \
        {                                                           \
            dwBufLen = __desiredLen ## pBuffer;                     \
        }                                                           \
    }

#define FREE_BUFFER(pBuffer) \
    if (__szTmpBuffer ## pBuffer != pBuffer) { delete[] pBuffer; }

//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Function:   IsPathSharedW
//
//  Synopsis:   IsPathShared is used by the shell to determine whether to
//              put a "shared folder / hand" icon next to a directory.
//              Different from Windows 95, we don't allow sharing remote
//              directories (e.g., \\brucefo4\c$\foo\bar style paths).
//
//  Arguments:  [lpcszPath] - path to look for
//              [bRefresh]  - TRUE if cache should be refreshed
//
//  Returns:    TRUE if the path is shared, FALSE otherwise
//
//  History:    4-Apr-95    BruceFo  Created
//
//--------------------------------------------------------------------------

BOOL WINAPI
IsPathSharedW(
    LPCWSTR lpcszPath,
    BOOL bRefresh
    )
{
    InterlockedIncrement((long*)&g_NonOLEDLLRefs);

    appDebugOut((DEB_TRACE,"IsPathSharedW(%ws, %d)\n", lpcszPath, bRefresh));

    OneTimeInit();
    BOOL bSuccess = g_ShareCache.IsPathShared(lpcszPath, bRefresh);

    appDebugOut((DEB_TRACE,
        "IsPathShared(%ws, %ws) = %ws\n",
        lpcszPath,
        bRefresh ? L"refresh" : L"no refresh",
        bSuccess ? L"yes" : L"no"));

    InterlockedDecrement((long*)&g_NonOLEDLLRefs);
    return bSuccess;
}


//+-------------------------------------------------------------------------
//
//  Function:   IsPathSharedA
//
//  Synopsis:   See IsPathSharedW
//
//  Arguments:  See IsPathSharedW
//
//  Returns:    See IsPathSharedW
//
//  History:    1-Mar-96    BruceFo  Created
//
//--------------------------------------------------------------------------

BOOL WINAPI
IsPathSharedA(
    LPCSTR lpcszPath,
    BOOL bRefresh
    )
{
    InterlockedIncrement((long*)&g_NonOLEDLLRefs);
    appDebugOut((DEB_TRACE,"IsPathSharedA(%s, %d)\n", lpcszPath, bRefresh));

    if (NULL == lpcszPath)
    {
        return FALSE;   // invalid input!
    }

    LPWSTR pszTmp;
    DWORD dwBufLen;
    GET_BUFFER(pszTmp, dwBufLen, WCHAR, lstrlenA(lpcszPath) + 1, MAX_PATH);
    if (NULL == pszTmp)
    {
        return FALSE;   // didn't get the buffer
    }
    MultiByteToWideChar(CP_ACP, 0, lpcszPath, -1, pszTmp, dwBufLen);
    BOOL bReturn = IsPathSharedW(pszTmp, bRefresh);
    FREE_BUFFER(pszTmp);

    InterlockedDecrement((long*)&g_NonOLEDLLRefs);
    return bReturn;
}

//+-------------------------------------------------------------------------
//
//  Function:   SharingDialog
//
//  Synopsis:   This API brings up the "Sharing" dialog. This entrypoint is
//              only used by the FAX team, as far as I know. Note that the
//              paths passed in are ANSI---that's because that's what they
//              were in Win95 when the API was defined.
//
//              This API, on NT, only works locally. It does not do remote
//              sharing, as Win95 does. Thus, the pszComputerName parameter
//              is ignored.
//
//  Arguments:  hwndParent      -- parent window
//              pszComputerName -- a computer name. This is ignored!
//              pszPath         -- the path to share.
//
//  Returns:    TRUE if everything went OK, FALSE otherwise
//
//  History:    5-Oct-95    BruceFo  Created
//
//--------------------------------------------------------------------------

BOOL WINAPI
SharingDialogW(
    HWND   hwndParent,
    LPWSTR pszComputerName,
    LPWSTR pszPath
    )
{
    InterlockedIncrement((long*)&g_NonOLEDLLRefs);
    appDebugOut((DEB_TRACE,"SharingDialogW(%ws)\n", pszPath));

    // Parameter validation
    if (NULL == pszPath)
    {
        return FALSE;
    }

    if (NULL != pszComputerName)
    {
        appDebugOut((DEB_TRACE,
            "SharingDialog() API called with a computer name which will be ignored\n"));
    }

    // Make sure the DLL is initialized. Note that this loads up the share
    // cache in this address space. Also, the first thing the dialog does
    // is refresh the cache, so we just wasted the work done to create the
    // cache in the init. Oh well...
    OneTimeInit(TRUE);

    PWSTR pszCopy = NewDup(pszPath);
    if (NULL == pszCopy)
    {
        return FALSE;
    }

    BOOL bReturn = SharingDialogHelp(hwndParent, pszCopy);
    InterlockedDecrement((long*)&g_NonOLEDLLRefs);
    return bReturn;
}

//+-------------------------------------------------------------------------
//
//  Function:   SharingDialogA
//
//  Synopsis:   see SharingDialogW
//
//  Arguments:  see SharingDialogW
//
//  Returns:    see SharingDialogW
//
//  History:    1-Mar-96    BruceFo  Created
//
//--------------------------------------------------------------------------

BOOL WINAPI
SharingDialogA(
    HWND  hwndParent,
    LPSTR pszComputerName,
    LPSTR pszPath
    )
{
    InterlockedIncrement((long*)&g_NonOLEDLLRefs);
    appDebugOut((DEB_TRACE,"SharingDialogA(%s)\n", pszPath));

    // Parameter validation
    if (NULL == pszPath)
    {
        return FALSE;
    }

    if (NULL != pszComputerName)
    {
        appDebugOut((DEB_TRACE,
            "SharingDialog() API called with a computer name which will be ignored\n"));
    }

    // Make sure the DLL is initialized. Note that this loads up the share
    // cache in this address space. Also, the first thing the dialog does
    // is refresh the cache, so we just wasted the work done to create the
    // cache in the init. Oh well...
    OneTimeInit(TRUE);

    DWORD dwLen = lstrlenA(pszPath) + 1;
    PWSTR pszUnicodePath = new WCHAR[dwLen];
    if (NULL == pszUnicodePath)
    {
        appDebugOut((DEB_ERROR,"OUT OF MEMORY\n"));
        return FALSE;
    }

    MultiByteToWideChar(CP_ACP, 0, pszPath, -1, pszUnicodePath, dwLen);
    BOOL bReturn = SharingDialogHelp(hwndParent, pszUnicodePath);
    InterlockedDecrement((long*)&g_NonOLEDLLRefs);
    return bReturn;
}


static BOOL WINAPI
SharingDialogHelp(
    HWND   hwndParent,
    LPWSTR pszPath          // this is 'new' memory that I take ownership of
    )
{
    SHARINGPROPSHEETPAGE sp;

    sp.psp.dwSize      = sizeof(sp);
    sp.psp.dwFlags     = PSP_USEREFPARENT;
    sp.psp.hInstance   = g_hInstance;
    sp.psp.pszTemplate = MAKEINTRESOURCE(IDD_SHARE_PROPERTIES);
    sp.psp.hIcon       = NULL;
    sp.psp.pszTitle    = NULL;
    sp.psp.pfnDlgProc  = CSharingPropertyPage::DlgProcPage;
    sp.psp.lParam      = (LPARAM)pszPath;  // transfer ownership
    sp.psp.pfnCallback = NULL;
    sp.psp.pcRefParent = &g_NonOLEDLLRefs;
    sp.bDialog         = TRUE;

    int ret = DialogBoxParam(
                    g_hInstance,
                    MAKEINTRESOURCE(IDD_SHARE_PROPERTIES),
                    hwndParent,
                    CSharingPropertyPage::DlgProcPage,
                    (LPARAM)&sp);
    if (-1 == ret)
    {
        appDebugOut((DEB_ERROR,"DialogBoxParam() error, 0x%08lx\n",GetLastError()));
        delete[] pszPath;
        return FALSE;
    }

    return TRUE;
}


// BUGBUG: there appears to be a bug in the Win95 code where they think
// they're storing and using "\\machine\share", but it appears they are
// actually storing and using "machine\share".

DWORD
CopyShareNameToBuffer(
    IN     CShareInfo* p,
    IN OUT LPWSTR lpszNameBuf,
    IN     DWORD cchNameBufLen
    )
{
    appAssert(NULL != lpszNameBuf);
    appAssert(0 != cchNameBufLen);

    WCHAR szLocalComputer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD nSize = ARRAYLEN(szLocalComputer);
    if (!GetComputerName(szLocalComputer, &nSize))
    {
        return GetLastError();
    }

    /* Two slashes + server name + slash + share name + null terminator. */

    DWORD computerLen = wcslen(szLocalComputer);
    DWORD shareLen    = wcslen(p->GetNetname());
    if (2 + computerLen + 1 + shareLen + 1 <= cchNameBufLen)
    {
        /* Return network resource name as UNC path. */

        lpszNameBuf[0] = L'\\';
        lpszNameBuf[1] = L'\\';
        wcscpy(lpszNameBuf + 2, szLocalComputer);
        *(lpszNameBuf + 2 + computerLen) = L'\\';
        wcscpy(lpszNameBuf + 2 + computerLen + 1, p->GetNetname());
        return ERROR_SUCCESS;
    }
    else
    {
        return ERROR_MORE_DATA;
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   GetNetResourceFromLocalPathW
//
//  Synopsis:   Used by shell link tracking code.
//
//  Arguments:  [lpcszPath]      Path we're concerned about.
//              [lpszNameBuf]    If path is shared, UNC path to share goes here.
//              [cchNameBufLen] length of lpszNameBuf buffer in characters
//              [pdwNetType]     net type of local server, e.g., WNNC_NET_LANMAN
//
//  Returns:    TRUE if path is shared and net resource information
//              returned, else FALSE.
//
//  Notes:      *lpszNameBuf and *pwNetType are only valid if TRUE is returned.
//
//  Example:    If c:\documents is shared as MyDocs on machine Scratch, then
//              calling GetNetResourceFromLocalPath(c:\documents, ...) will
//              set lpszNameBuf to \\Scratch\MyDocs.
//
//  History:    3-Mar-96    BruceFo  Created from Win95 sources
//
//--------------------------------------------------------------------------

BOOL WINAPI
GetNetResourceFromLocalPathW(
    IN     LPCWSTR lpcszPath,
    IN OUT LPWSTR lpszNameBuf,
    IN     DWORD cchNameBufLen,
    OUT    PDWORD pdwNetType
    )
{
    InterlockedIncrement((long*)&g_NonOLEDLLRefs);
    appDebugOut((DEB_TRACE,"GetNetResourceFromLocalPathW(%ws)\n", lpcszPath));

    // do some parameter validation
    if (NULL == lpcszPath || NULL == lpszNameBuf || NULL == pdwNetType || 0 == cchNameBufLen)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        InterlockedDecrement((long*)&g_NonOLEDLLRefs);
        return FALSE;
    }

    OneTimeInit();

    // Parameters seem OK (pointers might still point to bad memory);
    // do the work.

    CShareInfo* pShareList = new CShareInfo();  // dummy head node
    if (NULL == pShareList)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        InterlockedDecrement((long*)&g_NonOLEDLLRefs);
        return FALSE;   // out of memory
    }

    BOOL bReturn = FALSE;
    DWORD dwLastError;
    DWORD cShares;
    HRESULT hr = g_ShareCache.ConstructList(lpcszPath, pShareList, &cShares);
    if (SUCCEEDED(hr))
    {
        // Now, we have a list of (possibly zero) shares. The user is asking for
        // one of them. Give them the first normal, non-special share. If there
        // doesn't exist a non-special share, then give them a special share.

        if (cShares > 0)
        {
            BOOL bFoundOne = FALSE;
            CShareInfo* p;

            for (p = (CShareInfo*) pShareList->Next();
                 p != pShareList;
                 p = (CShareInfo*) p->Next())
            {
                if (p->GetType() == STYPE_DISKTREE)
                {
                    // found a share for this one.
                    bFoundOne = TRUE;
                    break;
                }
            }

            if (!bFoundOne)
            {
                for (p = (CShareInfo*) pShareList->Next();
                     p != pShareList;
                     p = (CShareInfo*) p->Next())
                {
                    if (p->GetType() == (STYPE_SPECIAL | STYPE_DISKTREE))
                    {
                        bFoundOne = TRUE;
                        break;
                    }
                }
            }

            if (bFoundOne)
            {
                dwLastError = CopyShareNameToBuffer(p, lpszNameBuf, cchNameBufLen);
                if (ERROR_SUCCESS == dwLastError)
                {
                    bReturn = TRUE;
                    *pdwNetType = WNNC_NET_LANMAN; // we only support LanMan
                }
            }
            else
            {
                // nothing found!
                dwLastError = ERROR_BAD_NET_NAME;
            }
        }
        else
        {
            dwLastError = ERROR_BAD_NET_NAME;
        }
    }
    else
    {
        dwLastError = ERROR_OUTOFMEMORY;
    }

    DeleteShareInfoList(pShareList, TRUE);

    if (!bReturn)
    {
        SetLastError(dwLastError);
    }

    InterlockedDecrement((long*)&g_NonOLEDLLRefs);
    return bReturn;
}


BOOL WINAPI
GetNetResourceFromLocalPathA(
    IN     LPCSTR lpcszPath,
    IN OUT LPSTR lpszNameBuf,
    IN     DWORD cchNameBufLen,
    OUT    PDWORD pdwNetType
    )
{
    InterlockedIncrement((long*)&g_NonOLEDLLRefs);
    appDebugOut((DEB_TRACE,"GetNetResourceFromLocalPathA(%s)\n", lpcszPath));
    BOOL bReturn = FALSE;
    LPWSTR pszPathTmp, pszNameTmp;
    DWORD dwPathBufLen, dwNameBufLen;
    GET_BUFFER(pszPathTmp, dwPathBufLen, WCHAR, lstrlenA(lpcszPath) + 1, MAX_PATH);
    if (NULL != pszPathTmp)
    {
        MultiByteToWideChar(CP_ACP, 0, lpcszPath, -1, pszPathTmp, dwPathBufLen);

        GET_BUFFER(pszNameTmp, dwNameBufLen, WCHAR, cchNameBufLen, MAX_PATH);
        if (NULL != pszNameTmp)
        {
            // got the buffers, now party...
            bReturn = GetNetResourceFromLocalPathW(pszPathTmp, pszNameTmp, cchNameBufLen, pdwNetType);
            if (bReturn)
            {
                // now convert the return string back
                WideCharToMultiByte(CP_ACP, 0,
                        pszNameTmp, -1,
                        lpszNameBuf, cchNameBufLen,
                        NULL, NULL);
            }

            FREE_BUFFER(pszNameTmp);
        }
        else
        {
            // didn't get the buffer
            SetLastError(ERROR_OUTOFMEMORY);
        }

        FREE_BUFFER(pszPathTmp);
    }
    else
    {
        // didn't get the buffer
        SetLastError(ERROR_OUTOFMEMORY);
    }

    InterlockedDecrement((long*)&g_NonOLEDLLRefs);
    return bReturn;
}


//+-------------------------------------------------------------------------
//
//  Function:   GetLocalPathFromNetResourceW
//
//  Synopsis:   Used by shell link tracking code.
//
//  Arguments:  [lpcszName]     A UNC path we're concerned about.
//              [dwNetType]     net type of local server, e.g., WNNC_NET_LANMAN
//              [lpszLocalPathBuf]   Buffer to place local path of UNC path
//              [cchLocalPathBufLen] length of lpszLocalPathBuf buffer in
//                                   characters
//              [pbIsLocal]     Set to TRUE if lpcszName points to a local
//                              resource.
//
//  Returns:
//
//  Notes:      *lpszLocalPathBuf and *pbIsLocal are only valid if
//              TRUE is returned.
//
//  Example:    If c:\documents is shared as MyDocs on machine Scratch, then
//              calling GetLocalPathFromNetResource(\\Scratch\MyDocs, ...) will
//              set lpszLocalPathBuf to c:\documents.
//
//  History:    3-Mar-96    BruceFo  Created from Win95 sources
//
//--------------------------------------------------------------------------

BOOL WINAPI
GetLocalPathFromNetResourceW(
    IN     LPCWSTR lpcszName,
    IN     DWORD dwNetType,
    IN OUT LPWSTR lpszLocalPathBuf,
    IN     DWORD cchLocalPathBufLen,
    OUT    PBOOL pbIsLocal
    )
{
    InterlockedIncrement((long*)&g_NonOLEDLLRefs);
    appDebugOut((DEB_TRACE,"GetLocalPathFromNetResourceW(%ws)\n", lpcszName));
    OneTimeInit();

    BOOL bReturn = FALSE;
    DWORD dwLastError;

    *pbIsLocal = FALSE;

    if (g_fSharingEnabled)
    {
        if (0 != dwNetType && HIWORD(dwNetType) == HIWORD(WNNC_NET_LANMAN))
        {
            /* Is the network resource name a UNC path on this machine? */

            WCHAR szLocalComputer[MAX_COMPUTERNAME_LENGTH + 1];
            DWORD nSize = ARRAYLEN(szLocalComputer);
            if (!GetComputerName(szLocalComputer, &nSize))
            {
                dwLastError = GetLastError();
            }
            else
            {
                dwLastError = ERROR_BAD_NET_NAME;

                DWORD dwLocalComputerLen = wcslen(szLocalComputer);
                if (   lpcszName[0] == L'\\'
                    && lpcszName[1] == L'\\'
                    && (0 == _wcsnicmp(lpcszName + 2, szLocalComputer, dwLocalComputerLen))
                    )
                {
                    LPCWSTR lpcszSep = &(lpcszName[2 + dwLocalComputerLen]);
                    if (*lpcszSep == L'\\')
                    {
                        *pbIsLocal = TRUE;

                        WCHAR szLocalPath[MAX_PATH];
                        if (g_ShareCache.IsExistingShare(lpcszSep + 1, NULL, szLocalPath))
                        {
                            if (wcslen(szLocalPath) < cchLocalPathBufLen)
                            {
                                wcscpy(lpszLocalPathBuf, szLocalPath);
                                dwLastError = ERROR_SUCCESS;
                                bReturn = TRUE;
                            }
                            else
                            {
                                dwLastError = ERROR_MORE_DATA;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            dwLastError = ERROR_BAD_PROVIDER;
        }
    }
    else
    {
        appDebugOut((DEB_TRACE,"GetLocalPathFromNetResourceW: sharing not enabled\n"));
        dwLastError = ERROR_BAD_NET_NAME;
    }

    if (!bReturn)
    {
        SetLastError(dwLastError);
    }

    InterlockedDecrement((long*)&g_NonOLEDLLRefs);
    return bReturn;
}


BOOL WINAPI
GetLocalPathFromNetResourceA(
    IN     LPCSTR lpcszName,
    IN     DWORD dwNetType,
    IN OUT LPSTR lpszLocalPathBuf,
    IN     DWORD cchLocalPathBufLen,
    OUT    PBOOL pbIsLocal
    )
{
    InterlockedIncrement((long*)&g_NonOLEDLLRefs);
    appDebugOut((DEB_TRACE,"GetLocalPathFromNetResourceA(%s)\n", lpcszName));

    BOOL bReturn = FALSE;
    LPWSTR pszLocalPathTmp, pszNameTmp;
    DWORD dwPathBufLen, dwNameBufLen;
    GET_BUFFER(pszLocalPathTmp, dwPathBufLen, WCHAR, cchLocalPathBufLen, MAX_PATH);
    if (NULL != pszLocalPathTmp)
    {
        GET_BUFFER(pszNameTmp, dwNameBufLen, WCHAR, lstrlenA(lpszLocalPathBuf) + 1, MAX_PATH);
        if (NULL != pszNameTmp)
        {
            MultiByteToWideChar(CP_ACP, 0, lpcszName, -1, pszNameTmp, dwNameBufLen);

            // got the buffers, now party...
            bReturn = GetLocalPathFromNetResourceW(pszNameTmp, dwNetType, pszLocalPathTmp, cchLocalPathBufLen, pbIsLocal);
            if (bReturn)
            {
                // now convert the return string back
                WideCharToMultiByte(CP_ACP, 0,
                        pszLocalPathTmp, -1,
                        lpszLocalPathBuf, cchLocalPathBufLen,
                        NULL, NULL);
            }

            FREE_BUFFER(pszNameTmp);
        }
        else
        {
            // didn't get the buffer
            SetLastError(ERROR_OUTOFMEMORY);
        }

        FREE_BUFFER(pszLocalPathTmp);
    }
    else
    {
        // didn't get the buffer
        SetLastError(ERROR_OUTOFMEMORY);
    }

    InterlockedDecrement((long*)&g_NonOLEDLLRefs);
    return bReturn;
}
