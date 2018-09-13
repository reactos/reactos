//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       util.cpp
//
//--------------------------------------------------------------------------
#include "pch.h"
#include <shsemip.h>    // ILClone, ILIsEmpty, etc.
#include <mobsyncp.h>
#include <sddl.h>
#include "security.h"
#include "idlhelp.h"
#include "shguidp.h"
#include "folder.h"


//*************************************************************
//
//  GetRemotePath
//
//  Purpose:    Return UNC version of a path
//
//  Parameters: pszInName - initial path
//              ppszOutName - UNC path returned here
//
//
//  Return:     HRESULT
//              S_OK - UNC path returned
//              S_FALSE - drive not connected (UNC not returned)
//              or failure code
//
//  Notes:      The function fails is the path is not a valid
//              network path.  If the path is already UNC,
//              a copy is made without validating the path.
//              *ppszOutName must be LocalFree'd by the caller.
//
//*************************************************************

HRESULT
GetRemotePath(LPCTSTR pszInName, LPTSTR *ppszOutName)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
    DWORD dwErr;

    TraceEnter(TRACE_UTIL, "GetRemotePath");
    TraceAssert(pszInName);
    TraceAssert(ppszOutName);

    *ppszOutName = NULL;

    // Don't bother calling GetFullPathName first, since we always
    // deal with full (complete) paths.

    if (pszInName[1] == TEXT(':'))
    {
        TCHAR szLocalName[3];
        TCHAR szRemoteName[MAX_PATH];
        DWORD dwLen = ARRAYSIZE(szRemoteName);

        szLocalName[0] = pszInName[0];
        szLocalName[1] = pszInName[1];
        szLocalName[2] = TEXT('\0');

        // Call GetDriveType before WNetGetConnection, to avoid loading
        // MPR.DLL until absolutely necessary.
        if (DRIVE_REMOTE != GetDriveType(szLocalName))
            ExitGracefully(hr, S_FALSE, "Drive not connected");

        dwErr = WNetGetConnection(szLocalName, szRemoteName, &dwLen);

        if (NO_ERROR == dwErr)
        {
            hr = S_OK;
            dwLen = lstrlen(szRemoteName);
        }
        else if (ERROR_NOT_CONNECTED == dwErr)
        {
            ExitGracefully(hr, S_FALSE, "Drive not connected");
        }
        else if (ERROR_MORE_DATA != dwErr)
            ExitGracefully(hr, HRESULT_FROM_WIN32(dwErr), "WNetGetConnection failed");
        // if dwErr == ERROR_MORE_DATA, dwLen already has the correct value

        // Skip the drive letter and add the length of the rest of the path
        // (including NULL)
        pszInName += 2;
        dwLen += lstrlen(pszInName) + 1;

        // We should never get incomplete paths, so we should always
        // see a backslash after the "X:".  If this isn't true, then
        // we should call GetFullPathName above.
        TraceAssert(TEXT('\\') == *pszInName);

        // Allocate the return buffer
        *ppszOutName = (LPTSTR)LocalAlloc(LPTR, dwLen * sizeof(TCHAR));
        if (!*ppszOutName)
            ExitGracefully(hr, E_OUTOFMEMORY, "LocalAlloc failed");

        if (ERROR_MORE_DATA == dwErr)
        {
            // Try again with the bigger buffer
            dwErr = WNetGetConnection(szLocalName, *ppszOutName, &dwLen);
            hr = HRESULT_FROM_WIN32(dwErr);
            FailGracefully(hr, "WNetGetConnection failed");
        }
        else
        {
            // WNetGetConnection succeeded on first try. Copy the result.
            lstrcpy(*ppszOutName, szRemoteName);
        }

        // Copy the rest of the path
        lstrcat(*ppszOutName, pszInName);
    }
    else if (PathIsUNC(pszInName))
    {
        // Just copy the path without validating it
        hr = S_OK;
        if (!LocalAllocString(ppszOutName, pszInName))
        {
            dwErr = GetLastError();
            hr = HRESULT_FROM_WIN32(dwErr);
        }
    }

exit_gracefully:

    if (FAILED(hr))
        LocalFreeString(ppszOutName);

    TraceLeaveResult(hr);
}


//*************************************************************
//
//  ULongToString
//
//  Purpose:    TCHAR version of itoa
//
//  Parameters: UINT i - unsigned integer to convert
//              LPTSTR psz - location to store string result
//              UINT cchMax - size of buffer pointed to by psz
//
//
//  Return:     BOOL - FALSE if buffer too small or invalid parameters
//
//*************************************************************
LPTSTR
ULongToString(ULONG i, LPTSTR psz, ULONG cchMax)
{
    TCHAR szTemp[16];   // enough to hold a 32-bit integer (with NULL)
    LPTSTR pszTemp = &szTemp[ARRAYSIZE(szTemp)-1]; // start at the end

    *pszTemp = TEXT('\0');

    do
    {
        *(--pszTemp) = (TCHAR)(TEXT('0') + (i % 10));
        i /= 10;
    }
    while (i != 0);

    return lstrcpyn(psz, pszTemp, cchMax);
}


//*************************************************************
//
//  LocalFreeString
//
//  Purpose:    Free a string allocated with LocalAlloc[String]
//
//  Parameters: LPTSTR *ppsz - location of pointer to string
//
//
//  Return:     void
//
//*************************************************************
VOID
LocalFreeString(LPTSTR *ppsz)
{
    if (ppsz && *ppsz)
    {
        LocalFree(*ppsz);
        *ppsz = NULL;
    }
}


//*************************************************************
//
//  LocalAllocString
//
//  Purpose:    Copy a string into a newly allocated buffer
//
//  Parameters: LPTSTR *ppszDest - location to store string copy
//              LPCTSTR pszSrc - string to copy
//
//
//  Return:     BOOL - FALSE if LocalAlloc fails or invalid parameters
//
//*************************************************************
BOOL
LocalAllocString(LPTSTR *ppszDest, LPCTSTR pszSrc)
{
    ULONG cbString;

    if (!ppszDest)
        return FALSE;

    *ppszDest = NULL;

    if (pszSrc)
    {
        cbString = StringByteSize(pszSrc);

        if (cbString)
        {
            *ppszDest = (LPTSTR)LocalAlloc(LPTR, cbString);

            if (*ppszDest)
            {
                CopyMemory(*ppszDest, pszSrc, cbString);
                return TRUE;
            }
        }
    }

    return FALSE;
}


//*************************************************************
//
//  SizeofStringResource
//
//  Purpose:    Find the length (in chars) of a string resource
//
//  Parameters: HINSTANCE hInstance - module containing the string
//              UINT idStr - ID of string
//
//
//  Return:     UINT - # of chars in string, not including NULL
//
//  Notes:      Based on code from user32.
//
//*************************************************************
UINT
SizeofStringResource(HINSTANCE hInstance,
                     UINT idStr)
{
    UINT cch = 0;
    HRSRC hRes = FindResource(hInstance, (LPTSTR)((LONG)(((USHORT)idStr >> 4) + 1)), RT_STRING);
    if (NULL != hRes)
    {
        HGLOBAL hStringSeg = LoadResource(hInstance, hRes);
        if (NULL != hStringSeg)
        {
            LPWSTR psz = (LPWSTR)LockResource(hStringSeg);
            if (NULL != psz)
            {
                idStr &= 0x0F;
                while(true)
                {
                    cch = *psz++;
                    if (idStr-- == 0)
                        break;
                    psz += cch;
                }
            }
        }
    }
    return cch;
}

//*************************************************************
//
//  LoadStringAlloc
//
//  Purpose:    Loads a string resource into an alloc'd buffer
//
//  Parameters: ppszResult - string resource returned here
//              hInstance - module to load string from
//              idStr - string resource ID
//
//  Return:     same as LoadString
//
//  Notes:      On successful return, the caller must
//              LocalFree *ppszResult
//
//*************************************************************
int
LoadStringAlloc(LPTSTR *ppszResult, HINSTANCE hInstance, UINT idStr)
{
    int nResult = 0;
    UINT cch = SizeofStringResource(hInstance, idStr);
    if (cch)
    {
        cch++; // for NULL
        *ppszResult = (LPTSTR)LocalAlloc(LPTR, cch * sizeof(TCHAR));
        if (*ppszResult)
            nResult = LoadString(hInstance, idStr, *ppszResult, cch);
    }
    return nResult;
}


//*************************************************************
//
//  ShellChangeNotify
//
//  Purpose:    Wrapper for SHChangeNotify
//
//  Parameters: pszPath - path of file that changed
//              bFlush - TRUE forces a flush of the shell's
//                       notify queue.
//
//  Return:     none
//
//  Notes:      SHCNF_PATH doesn't work outside of the shell,
//              so we create a pidl and use SHCNF_IDLIST.
//
//              Force a flush every 8 calls so the shell
//              doesn't start ignoring notifications.
//
//*************************************************************
void
ShellChangeNotify(
    LPCTSTR pszPath,
    WIN32_FIND_DATA *pfd,
    BOOL bFlush,
    LONG nEvent
    )
{
    LPITEMIDLIST pidlFile = NULL;
    LPCVOID pvItem = NULL;
    UINT uFlags = 0;

    static int cNoFlush = 0;

    if (pszPath)
    {
        if ((pfd && SUCCEEDED(SHSimpleIDListFromFindData(pszPath, pfd, &pidlFile)))
            || (pidlFile = ILCreateFromPath(pszPath)))
        {
            uFlags = SHCNF_IDLIST;
            pvItem = pidlFile;
        }
        else
        {
            // ILCreateFromPath sometimes fails when we're in disconnected
            // mode, so try the path instead.
            uFlags = SHCNF_PATH;
            pvItem = pszPath;
        }
        if (0 == nEvent)
            nEvent = SHCNE_UPDATEITEM;
    }
    else
        nEvent = 0;

    if (8 < cNoFlush++)
        bFlush = TRUE;

    if (bFlush)
    {
        uFlags |= (SHCNF_FLUSH | SHCNF_FLUSHNOWAIT);
        cNoFlush = 0;
    }
    SHChangeNotify(nEvent, uFlags, pvItem, NULL);

    if (pidlFile)
        SHFree(pidlFile);
}


//*************************************************************
//
//  GetLinkTarget
//
//  Purpose:    Get the path to the target file of a link
//
//  Parameters: pszShortcut - name of link file
//              hwndOwner - passed to GetUIObjectOf
//              ppszTarget - target path returned here
//
//
//  Return:     HRESULT
//              S_OK - target file returned
//              S_FALSE - target not returned
//              or failure code
//
//  Notes:      COM must be initialized before calling.
//              The function fails is the target is a folder.
//              *ppszTarget must be LocalFree'd by the caller.
//
//*************************************************************
HRESULT
GetLinkTarget(LPCTSTR pszShortcut,
              HWND hwndOwner,
              LPTSTR *ppszTarget,
              PDWORD pdwAttr)
{
    HRESULT hr;
    LPPERSISTFILE ppf = NULL;
    IShellLink * psl = NULL;
    TCHAR szTarget[MAX_PATH];
    LPITEMIDLIST pidlTarget = NULL;

    USES_CONVERSION;

    TraceEnter(TRACE_UTIL, "GetLinkTarget");

    if (!pszShortcut || !*pszShortcut || !ppszTarget)
        TraceLeaveResult(E_INVALIDARG);

    *ppszTarget = NULL;

    if (pdwAttr)
        *pdwAttr = 0;

    hr = CoCreateInstance(CLSID_ShellLink,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IPersistFile,
                          (LPVOID*)&ppf);
    FailGracefully(hr, "Could not CCI ShellLink object");
    
    hr = ppf->Load(T2COLE(pszShortcut), 0);
    FailGracefully(hr, "Could not initialize ShellLink");
    
    hr = ppf->QueryInterface(IID_IShellLink, (LPVOID*)&psl);
    FailGracefully(hr, "Could not get IShellLink interface");
    
    // Get the pidl of the target
    hr = psl->GetIDList(&pidlTarget);
    FailGracefully(hr, "Could not get pidl for link target");

    hr = S_FALSE;   // means no target returned

    if (SHGetPathFromIDList(pidlTarget, szTarget))
    {
        // We only want to continue if the target is a file (not a folder)
        // And we don't want the system to put up a dialog if, for example,
        // the shortcut points to a empty floppy drive.
        UINT uErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
        DWORD dwAttr = GetFileAttributes(szTarget);
        SetErrorMode(uErrorMode);

        if (-1 != dwAttr && !(dwAttr & FILE_ATTRIBUTE_DIRECTORY))
        {
            if (pdwAttr)
                *pdwAttr = dwAttr;
            hr = GetRemotePath(szTarget, ppszTarget);
        }
    }

exit_gracefully:

    DoRelease(psl);
    DoRelease(ppf);

    if (pidlTarget)
        SHFree(pidlTarget);

    TraceLeaveResult(hr);
}


//*************************************************************
//
//  _CSCEnumDatabase
//
//  Purpose:    Enumerate CSC database recursively
//
//  Parameters: pszFolder - name of folder to begin enumeration
//                          (can be NULL to enum shares)
//              bRecurse - TRUE to recurse into child folders
//              pfnCB - callback function called once for each child
//              lpContext - extra data passed to callback function
//
//  Return:     One of CSCPROC_RETURN_*
//
//  Notes:      Return CSCPROC_RETURN_SKIP from the callback to prevent
//              recursion into a child folder. CSCPROC_RETURN_ABORT
//              will terminate the entire operation (unwind all recursive
//              calls). CSCPROC_RETURN_CONTINUE will continue normally.
//              Other CSCPROC_RETURN_* values are treated as ABORT.
//
//*************************************************************
#define PATH_BUFFER_SIZE    1024

typedef struct
{
    LPTSTR              szPath;
    int                 cchPathBuffer;
    BOOL                bRecurse;
    PFN_CSCENUMPROC     pfnCB;
    LPARAM              lpContext;
} CSC_ENUM_CONTEXT, *PCSC_ENUM_CONTEXT;

DWORD
_CSCEnumDatabaseInternal(PCSC_ENUM_CONTEXT pContext)
{
    DWORD dwResult = CSCPROC_RETURN_CONTINUE;
    HANDLE hFind;
    DWORD dwStatus = 0;
    DWORD dwPinCount = 0;
    DWORD dwHintFlags = 0;
    LPTSTR pszPath;
    int cchBuffer;
    LPTSTR pszFind = NULL;
    int cchDir = 0;
    WIN32_FIND_DATA fd;

    TraceEnter(TRACE_UTIL, "_CSCEnumDatabaseInternal");
    TraceAssert(pContext);
    TraceAssert(pContext->pfnCB);
    TraceAssert(pContext->szPath);
    TraceAssert(pContext->cchPathBuffer);

    pszPath = pContext->szPath;
    cchBuffer = pContext->cchPathBuffer;

    if (*pszPath)
    {
        PathAddBackslash(pszPath);
        cchDir = lstrlen(pszPath);
        TraceAssert(TEXT('\\') == pszPath[cchDir-1]);
        pszFind = pszPath;
    }

    // skips "." and ".."
    hFind = CacheFindFirst(pszFind,
                           &fd,
                           &dwStatus,
                           &dwPinCount,
                           &dwHintFlags,
                           NULL);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            int cchFile;
            ENUM_REASON eReason = ENUM_REASON_FILE;

            cchFile = lstrlen(fd.cFileName);
            if (cchFile >= cchBuffer - cchDir)
            {
                // Realloc the path buffer
                TraceMsg("Reallocating path buffer");
                cchBuffer += max(PATH_BUFFER_SIZE, cchFile + 1);
                pszPath = (LPTSTR)LocalReAlloc(pContext->szPath,
                                               cchBuffer * sizeof(TCHAR),
                                               LMEM_MOVEABLE);
                if (pszPath)
                {
                    pContext->szPath = pszPath;
                    pContext->cchPathBuffer = cchBuffer;
                }
                else
                {
                    pszPath = pContext->szPath;
                    cchBuffer = pContext->cchPathBuffer;
                    TraceMsg("Unable to reallocate path buffer");
                    Trace((pszPath));
                    Trace((fd.cFileName));
                    continue;
                }
            }

            // Build full path
            lstrcpyn(&pszPath[cchDir],
                     fd.cFileName,
                     cchBuffer - cchDir);
            cchFile = lstrlen(pszPath);

            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || !pszFind)
                eReason = ENUM_REASON_FOLDER_BEGIN;

            // Call the callback
            dwResult = (*pContext->pfnCB)(pszPath,
                                          eReason,
                                          dwStatus,
                                          dwHintFlags,
                                          dwPinCount,
                                          &fd,
                                          pContext->lpContext);

            // Recurse into folders
            if (CSCPROC_RETURN_CONTINUE == dwResult &&
                pContext->bRecurse &&
                ENUM_REASON_FOLDER_BEGIN == eReason)
            {
                dwResult = _CSCEnumDatabaseInternal(pContext);

                // Call the callback again
                pszPath[cchFile] = TEXT('\0');
                dwResult = (*pContext->pfnCB)(pszPath,
                                              ENUM_REASON_FOLDER_END,
                                              0, // dwStatus,       // these have probably changed
                                              0, // dwHintFlags,
                                              0, // dwPinCount,
                                              &fd,
                                              pContext->lpContext);
            }

            if (CSCPROC_RETURN_SKIP == dwResult)
                dwResult = CSCPROC_RETURN_CONTINUE;

            if (CSCPROC_RETURN_CONTINUE != dwResult)
                break;

        } while (CacheFindNext(hFind,
                               &fd,
                               &dwStatus,
                               &dwPinCount,
                               &dwHintFlags,
                               NULL));
        CSCFindClose(hFind);
    }

    TraceLeaveValue(dwResult);
}


DWORD
_CSCEnumDatabase(LPCTSTR pszFolder,
                 BOOL bRecurse,
                 PFN_CSCENUMPROC pfnCB,
                 LPARAM lpContext)
{
    DWORD dwResult = CSCPROC_RETURN_CONTINUE;
    CSC_ENUM_CONTEXT ec;

    TraceEnter(TRACE_UTIL, "_CSCEnumDatabase");
    TraceAssert(pfnCB);

    if (!pfnCB)
        TraceLeaveValue(CSCPROC_RETURN_ABORT);

    // Allocate the single buffer used for the entire enumeration.
    // It will be reallocated later if necessary.
    ec.cchPathBuffer = PATH_BUFFER_SIZE;
    if (pszFolder)
        ec.cchPathBuffer *= ((lstrlen(pszFolder)/PATH_BUFFER_SIZE) + 1);
    ec.szPath = (LPTSTR)LocalAlloc(LMEM_FIXED, ec.cchPathBuffer*sizeof(TCHAR));
    if (!ec.szPath)
        TraceLeaveValue(CSCPROC_RETURN_ABORT);

    ec.szPath[0] = TEXT('\0');

    // Assume pszFolder is valid a directory path or NULL
    if (pszFolder)
        lstrcpyn(ec.szPath, pszFolder, ec.cchPathBuffer);

    ec.bRecurse = bRecurse;
    ec.pfnCB = pfnCB;
    ec.lpContext = lpContext;

    dwResult = _CSCEnumDatabaseInternal(&ec);

    LocalFree(ec.szPath);

    TraceLeaveValue(dwResult);
}


//*************************************************************
//
//  _Win32EnumFolder
//
//  Purpose:    Enumerate a directory recursively
//
//  Parameters: pszFolder - name of folder to begin enumeration
//              bRecurse - TRUE to recurse into child folders
//              pfnCB - callback function called once for each child
//              lpContext - extra data passed to callback function
//
//  Return:     One of CSCPROC_RETURN_*
//
//  Notes:      Same as _CSCEnumDatabase except using FindFirstFile
//              instead of CSCFindFirstFile.
//
//*************************************************************

typedef struct
{
    LPTSTR              szPath;
    int                 cchPathBuffer;
    BOOL                bRecurse;
    PFN_WIN32ENUMPROC   pfnCB;
    LPARAM              lpContext;
} W32_ENUM_CONTEXT, *PW32_ENUM_CONTEXT;

DWORD
_Win32EnumFolderInternal(PW32_ENUM_CONTEXT pContext)
{
    DWORD dwResult = CSCPROC_RETURN_CONTINUE;
    HANDLE hFind;
    LPTSTR pszPath;
    int cchBuffer;
    int cchDir = 0;
    WIN32_FIND_DATA fd;

    TraceEnter(TRACE_UTIL, "_Win32EnumFolderInternal");
    TraceAssert(pContext);
    TraceAssert(pContext->pfnCB);
    TraceAssert(pContext->szPath && pContext->szPath[0]);
    TraceAssert(pContext->cchPathBuffer);

    pszPath = pContext->szPath;
    cchBuffer = pContext->cchPathBuffer;

    // Build wildcard path
    PathAddBackslash(pszPath);
    cchDir = lstrlen(pszPath);
    TraceAssert(TEXT('\\') == pszPath[cchDir-1]);
    pszPath[cchDir] = TEXT('*');
    pszPath[cchDir+1] = TEXT('\0');

    hFind = FindFirstFile(pszPath, &fd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            int cchFile;
            ENUM_REASON eReason = ENUM_REASON_FILE;

            // skip "." and ".."
            if (PathIsDotOrDotDot(fd.cFileName))
                continue;

            cchFile = lstrlen(fd.cFileName);
            if (cchFile >= cchBuffer - cchDir)
            {
                // Realloc the path buffer
                TraceMsg("Reallocating path buffer");
                cchBuffer += max(PATH_BUFFER_SIZE, cchFile + 1);
                pszPath = (LPTSTR)LocalReAlloc(pContext->szPath,
                                               cchBuffer * sizeof(TCHAR),
                                               LMEM_MOVEABLE);
                if (pszPath)
                {
                    pContext->szPath = pszPath;
                    pContext->cchPathBuffer = cchBuffer;
                }
                else
                {
                    pszPath = pContext->szPath;
                    cchBuffer = pContext->cchPathBuffer;
                    TraceMsg("Unable to reallocate path buffer");
                    Trace((pszPath));
                    Trace((fd.cFileName));
                    continue;
                }
            }

            // Build full path
            lstrcpyn(&pszPath[cchDir],
                     fd.cFileName,
                     cchBuffer - cchDir);
            cchFile = lstrlen(pszPath);

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                eReason = ENUM_REASON_FOLDER_BEGIN;

            // Call the callback
            dwResult = (*pContext->pfnCB)(pszPath,
                                          eReason,
                                          &fd,
                                          pContext->lpContext);

            // Recurse into folders
            if (CSCPROC_RETURN_CONTINUE == dwResult &&
                pContext->bRecurse &&
                ENUM_REASON_FOLDER_BEGIN == eReason)
            {
                dwResult = _Win32EnumFolderInternal(pContext);

                // Call the callback again
                pszPath[cchFile] = TEXT('\0');
                dwResult = (*pContext->pfnCB)(pszPath,
                                              ENUM_REASON_FOLDER_END,
                                              &fd,
                                              pContext->lpContext);
            }

            if (CSCPROC_RETURN_SKIP == dwResult)
                dwResult = CSCPROC_RETURN_CONTINUE;

            if (CSCPROC_RETURN_CONTINUE != dwResult)
                break;

        } while (FindNextFile(hFind, &fd));

        FindClose(hFind);
    }

    TraceLeaveValue(dwResult);
}


DWORD
_Win32EnumFolder(LPCTSTR pszFolder,
                 BOOL bRecurse,
                 PFN_WIN32ENUMPROC pfnCB,
                 LPARAM lpContext)
{
    DWORD dwResult = CSCPROC_RETURN_CONTINUE;
    W32_ENUM_CONTEXT ec;

    TraceEnter(TRACE_UTIL, "_Win32EnumFolder");
    TraceAssert(pszFolder);
    TraceAssert(pfnCB);

    if (!pszFolder || !*pszFolder || !pfnCB)
        TraceLeaveValue(CSCPROC_RETURN_ABORT);

    // Allocate the single buffer used for the entire enumeration.
    // It will be reallocated later if necessary.
    ec.cchPathBuffer = ((lstrlen(pszFolder)/PATH_BUFFER_SIZE) + 1) * PATH_BUFFER_SIZE;
    ec.szPath = (LPTSTR)LocalAlloc(LMEM_FIXED, ec.cchPathBuffer*sizeof(TCHAR));
    if (!ec.szPath)
        TraceLeaveValue(CSCPROC_RETURN_ABORT);

    ec.szPath[0] = TEXT('\0');

    // Assume pszFolder is valid a directory path
    lstrcpyn(ec.szPath, pszFolder, ec.cchPathBuffer);

    ec.bRecurse = bRecurse;
    ec.pfnCB = pfnCB;
    ec.lpContext = lpContext;

    dwResult = _Win32EnumFolderInternal(&ec);

    LocalFree(ec.szPath);

    TraceLeaveValue(dwResult);
}


//*************************************************************
//
//  CIDArray implementation
//
//*************************************************************

CIDArray::~CIDArray()
{
    DoRelease(m_psf);
    if (m_pIDA)
    {
        GlobalUnlock(m_Medium.hGlobal);
        m_pIDA = NULL;
    }
    ReleaseStgMedium(&m_Medium);
    LocalFreeString(&m_pszPath);
    LocalFreeString(&m_pszAlternatePath);
}



HRESULT
CIDArray::Initialize(LPDATAOBJECT pdobj)
{
    HRESULT hr;
    FORMATETC fe = { g_cfShellIDList, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    IShellFolder *psfDesktop = NULL;
    TCHAR szItem[MAX_PATH] = {0};
    DWORD dwLen;
    LPCITEMIDLIST pidlFolder;

    TraceEnter(TRACE_UTIL, "CIDArray::Initialize");

    if (m_pIDA)
        ExitGracefully(hr, E_UNEXPECTED, "Already initialized");

    if (!pdobj)
        ExitGracefully(hr, E_INVALIDARG, "No data object");

    hr = pdobj->GetData(&fe, &m_Medium);
    FailGracefully(hr, "Can't get ID List format from data object");

    m_pIDA = (LPIDA)GlobalLock(m_Medium.hGlobal);
    if (m_pIDA == NULL)
        ExitGracefully(hr, E_FAIL, "No ID Array");

    if (m_pIDA->cidl < 1)
        ExitGracefully(hr, E_FAIL, "No items");

    m_bSrcIsOfflineFilesFolder = IsFromOfflineFilesFolder(m_pIDA);

    hr = SHGetDesktopFolder(&psfDesktop);
    FailGracefully(hr, "Unable to bind to desktop folder");

    pidlFolder = (LPCITEMIDLIST)ByteOffset(m_pIDA, m_pIDA->aoffset[0]);

    // Bind to the parent folder
    if (m_bSrcIsOfflineFilesFolder)
    {
        hr = COfflineFilesFolder::GetFolder(&m_psf);
    }
    else if (ILIsEmpty(pidlFolder))
    {
        // The parent is the desktop
        m_psf = psfDesktop;
        m_psf->AddRef();
    }
    else
    {
        hr = psfDesktop->BindToObject(pidlFolder,
                                      NULL,
                                      IID_IShellFolder,
                                      (PVOID*)&m_psf);
        FailGracefully(hr, "Unable to bind to folder");
    }
    
    IShellLink *psl;
    if (SUCCEEDED(m_psf->QueryInterface(IID_IShellLink, (void **)&psl)))
    {
        //  this is a folder shortcut
        hr = psl->GetPath(szItem, ARRAYSIZE(szItem), NULL, 0);
        psl->Release();
    }
    else
    {
        //
        // If the source is the Offline Files folder we leave the folder path
        // blank.  All items are full UNC paths.
        //
        if (!m_bSrcIsOfflineFilesFolder)
        {
            // Get the parent folder path
            hr = GetItemName(psfDesktop,
                             pidlFolder,
                             szItem,
                             ARRAYSIZE(szItem));
        }
    }
    
    FailGracefully(hr, "Unable to get folder path");

    if (!PathIsUNC(szItem)
        && TEXT(':') == szItem[1]
        && TEXT(':') != szItem[0])
    {
        LPTSTR pszUNC = NULL;
        GetRemotePath(szItem, &pszUNC);
        if (pszUNC)
        {
            lstrcpyn(szItem, pszUNC, ARRAYSIZE(szItem));
            LocalFree(pszUNC);
        }
    }

    dwLen = lstrlen(szItem);

    // Allocate a buffer with enough room for the parent path,
    // a backslash, a MAX_PATH child path, and a NULL terminator.
    dwLen += MAX_PATH + 2;
    m_pszPath = (LPTSTR)LocalAlloc(LPTR, dwLen * sizeof(TCHAR));
    if (!m_pszPath)
        ExitGracefully(hr, E_OUTOFMEMORY, "LocalAlloc failed");

    lstrcpyn(m_pszPath, szItem, dwLen);
    PathAddBackslash(m_pszPath);
    m_cchFolder = lstrlen(m_pszPath);

    // If the parent is "My Computer", then m_pszPath will be empty here
    // and the item paths will be drive roots (e.g. "X:\").

    // If the parent is "Offline Files", then m_pszPath will be empty here
    // and the item paths will be fully qualified.

exit_gracefully:

    DoRelease(psfDesktop);

    TraceLeaveResult(hr);
}

HRESULT
CIDArray::GetItemAttributes(UINT iItem, PDWORD pdwAttr)
{
    LPCITEMIDLIST pidl;

    if (!m_pIDA || !m_psf)
        return E_UNEXPECTED;

    if (0 == iItem || iItem > m_pIDA->cidl)
        return E_INVALIDARG;

    pidl = (LPCITEMIDLIST)ByteOffset(m_pIDA, m_pIDA->aoffset[iItem]);

    return m_psf->GetAttributesOf(1, &pidl, pdwAttr);
}


LPCTSTR
CIDArray::GetItemPath(UINT iItem)
{
    TCHAR szItem[MAX_PATH];
    LPTSTR pszItem;

    TraceEnter(TRACE_UTIL, "CIDArray::GetItemPath");

    if (!m_pIDA || iItem > m_pIDA->cidl || !m_psf)
    {
        TraceMsg("CIDArray not initialized, or invalid index");
        TraceLeaveValue(NULL);
    }

    if (0 == iItem)
    {
        m_pszPath[m_cchFolder] = TEXT('\0');
        TraceLeaveValue(m_pszPath);
    }

    if (FAILED(GetItemName(m_psf,
                           (LPCITEMIDLIST)ByteOffset(m_pIDA, m_pIDA->aoffset[iItem]),
                           szItem,
                           ARRAYSIZE(szItem),
                           (enum tagSHGDN)(SHGDN_INFOLDER | SHGDN_FORPARSING))))
    {
        TraceMsg("Unable to get item name");
        TraceLeaveValue(NULL);
    }

    if (m_bSrcIsOfflineFilesFolder)
    {
        //
        // Offline Files item paths are fully-qualified UNC paths.  No need
        // to modify.  Use as-is.
        //
        lstrcpyn(&m_pszPath[0],
                 szItem,
                 (((ULONG)LocalSize(m_pszPath))/sizeof(TCHAR)));
    }
    else
    {
        //
        // Build the full path
        //
        pszItem = szItem;
        if (!PathIsRelative(szItem))
        {
            if (CSTR_EQUAL == CompareString(LOCALE_USER_DEFAULT,
                                            NORM_IGNORECASE,
                                            m_pszPath,
                                            m_cchFolder,
                                            szItem,
                                            m_cchFolder))
            {
                // When looking at shares on a server, szItem includes the server
                // name as well as the share name, so handle this differently.
                // I.e. skip the server name and just copy the share name.
                pszItem = &szItem[m_cchFolder];
            }
            else if (m_cchFolder)
            {
                // The item has a fully qualified path, and the folder
                // path is nonempty.  This can happen, for example, when
                // clicking on the MyDocs icon on the desktop.
                // Since the folder path is nonempty, return a different
                // buffer with the item path.
                if (!m_pszAlternatePath)
                    m_pszAlternatePath = (LPTSTR)LocalAlloc(LPTR, MAX_PATH * SIZEOF(TCHAR));

                if (m_pszAlternatePath)
                {
                    lstrcpyn(m_pszAlternatePath, szItem, MAX_PATH);
                    TraceLeaveValue(m_pszAlternatePath);
                }
            }
        }
        lstrcpyn(&m_pszPath[m_cchFolder],
                 pszItem,
                 (((ULONG)LocalSize(m_pszPath))/sizeof(TCHAR)) - m_cchFolder);
    }

    TraceLeaveValue(m_pszPath);
}



bool
CIDArray::IsFromOfflineFilesFolder(
    LPIDA pida
    )
{
    bool bResult = false;
    if (NULL != pida && 0 < pida->cidl)
    {
        LPITEMIDLIST pidlOfflineFiles;
        if (SUCCEEDED(COfflineFilesFolder::CreateIDList(&pidlOfflineFiles)))
        {
            LPCITEMIDLIST pidlDataRoot = (LPCITEMIDLIST)ByteOffset(pida, pida->aoffset[0]);
            if (ILIsEqual(pidlOfflineFiles, pidlDataRoot))
            {
                bResult = true;
            }
            ILFree(pidlOfflineFiles);
        }
    }
    return bResult;
}



#define CSIDL_INVALID   (-1)

//
// Order these with the most-frequently accessed folders first.
// This will prevent unnecessary CLSID string creation in GetItemName.
//
const struct
{
    LPCGUID pClsid;
    int     csidl;

} c_aSpecialFolders[] =
{
    { &CLSID_MyDocuments,        CSIDL_PERSONAL },
    { &CLSID_MyComputer,         CSIDL_INVALID  },
    { &CLSID_NetworkPlaces,      CSIDL_NETHOOD  },
    { &CLSID_OfflineFilesFolder, CSIDL_INVALID  }
};

HRESULT
CIDArray::GetItemName(LPSHELLFOLDER psf,
                      LPCITEMIDLIST pidl,
                      LPTSTR pszName,
                      UINT cchName,
                      SHGNO uFlags)
{
    STRRET str;

    if (NULL == psf)
        return E_INVALIDARG;

    HRESULT hr = psf->GetDisplayNameOf(pidl, uFlags, &str);

    if (SUCCEEDED(hr))
    {
        StrRetToBuf(&str, pidl, pszName, cchName);

        if ((SHGDN_FORPARSING & uFlags)
            && TEXT(':') == pszName[0]
            && TEXT(':') == pszName[1])
        {
            TCHAR szClsid[] = TEXT("::{00000000-0000-0000-0000-000000000000}");
            // It's a special folder path, see if it's one we recognize
            for (int i = 0; i < ARRAYSIZE(c_aSpecialFolders); i++)
            {
                StringFromGUID2(*(c_aSpecialFolders[i].pClsid), &szClsid[2], ARRAYSIZE(szClsid) - 2);
                if (!lstrcmpi(pszName, szClsid))
                {
                    // Get the correct folder path
                    if (CSIDL_INVALID == c_aSpecialFolders[i].csidl)
                        pszName[0] = 0;
                    else
                        SHGetFolderPath(NULL, c_aSpecialFolders[i].csidl | CSIDL_FLAG_DONT_VERIFY, NULL, 0, pszName);
                    break;
                }
            }
        }
    }

    return hr;
}


//*************************************************************
//
//  CCscFileHandle non-inline member functions.
//
//*************************************************************
CCscFindHandle& 
CCscFindHandle::operator = (
    const CCscFindHandle& rhs
    )
{
    if (this != &rhs)
    {
        Attach(rhs.Detach());
    }
    return *this;
}


void 
CCscFindHandle::Close(
    void
    )
{ 
    if (m_bOwns && INVALID_HANDLE_VALUE != m_handle)
    { 
        CSCFindClose(m_handle); 
    }
    m_bOwns  = false;
    m_handle = INVALID_HANDLE_VALUE;
}


//*************************************************************
//
//  String formatting functions
//
//*************************************************************

DWORD
FormatStringID(LPTSTR *ppszResult, HINSTANCE hInstance, UINT idStr, ...)
{
    DWORD dwResult;
    va_list args;
    va_start(args, idStr);
    dwResult = vFormatStringID(ppszResult, hInstance, idStr, &args);
    va_end(args);
    return dwResult;
}

DWORD
FormatString(LPTSTR *ppszResult, LPCTSTR pszFormat, ...)
{
    DWORD dwResult;
    va_list args;
    va_start(args, pszFormat);
    dwResult = vFormatString(ppszResult, pszFormat, &args);
    va_end(args);
    return dwResult;
}

DWORD
vFormatStringID(LPTSTR *ppszResult, HINSTANCE hInstance, UINT idStr, va_list *pargs)
{
    DWORD dwResult = 0;
    LPTSTR pszFormat = NULL;
    if (LoadStringAlloc(&pszFormat, hInstance, idStr))
    {
        dwResult = vFormatString(ppszResult, pszFormat, pargs);
        LocalFree(pszFormat);
    }
    return dwResult;
}

DWORD
vFormatString(LPTSTR *ppszResult, LPCTSTR pszFormat, va_list *pargs)
{
    return FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                         pszFormat,
                         0,
                         0,
                         (LPTSTR)ppszResult,
                         1,
                         pargs);
}


//
// Determine if a folder path is "." or "..".
//
bool
PathIsDotOrDotDot(
    LPCTSTR pszPath
    )
{
    if (TEXT('.') == *pszPath++)
    {
        if (TEXT('\0') == *pszPath || (TEXT('.') == *pszPath && TEXT('\0') == *(pszPath + 1)))
            return true;
    }
    return false;
}

//
// Center a window in it's parent.
// If hwndParent is NULL, the window's parent is used.
// If hwndParent is not NULL, hwnd is centered in it.
// If hwndParent is NULL and hwnd doesn't have a parent, it is centered
// on the desktop.
//
void
CenterWindow(
    HWND hwnd, 
    HWND hwndParent
    )
{
    RECT rcScreen;

    if (NULL != hwnd)
    {
        rcScreen.left   = rcScreen.top = 0;
        rcScreen.right  = GetSystemMetrics(SM_CXSCREEN);
        rcScreen.bottom = GetSystemMetrics(SM_CYSCREEN);

        if (NULL == hwndParent)
        {
            hwndParent = GetParent(hwnd);
            if (NULL == hwndParent)
                hwndParent = GetDesktopWindow();
        }

        RECT rcWnd;
        RECT rcParent;

        GetWindowRect(hwnd, &rcWnd);
        GetWindowRect(hwndParent, &rcParent);

        INT cxWnd    = rcWnd.right  - rcWnd.left;
        INT cyWnd    = rcWnd.bottom - rcWnd.top;
        INT cxParent = rcParent.right  - rcParent.left;
        INT cyParent = rcParent.bottom - rcParent.top;
        POINT ptParentCtr;

        ptParentCtr.x = rcParent.left + (cxParent / 2);
        ptParentCtr.y = rcParent.top  + (cyParent / 2);

        if ((ptParentCtr.x + (cxWnd / 2)) > rcScreen.right)
        {
            //
            // Window would run off the right edge of the screen.
            //
            rcWnd.left = rcScreen.right - cxWnd;
        }
        else if ((ptParentCtr.x - (cxWnd / 2)) < rcScreen.left)
        {
            //
            // Window would run off the left edge of the screen.
            //
            rcWnd.left = rcScreen.left;
        }
        else
        {
            rcWnd.left = ptParentCtr.x - (cxWnd / 2);
        }

        if ((ptParentCtr.y + (cyWnd / 2)) > rcScreen.bottom)
        {
            //
            // Window would run off the bottom edge of the screen.
            //
            rcWnd.top = rcScreen.bottom - cyWnd;
        }
        else if ((ptParentCtr.y - (cyWnd / 2)) < rcScreen.top)
        {
            //
            // Window would run off the top edge of the screen.
            //
            rcWnd.top = rcScreen.top;
        }
        else
        {
            rcWnd.top = ptParentCtr.y - (cyWnd / 2);
        }

        MoveWindow(hwnd, rcWnd.left, rcWnd.top, cxWnd, cyWnd, TRUE);
    }
}

//
// We have some extra stuff to pass to the stats callback so we wrap the
// CSCSHARESTATS in a larger structure.
//
typedef struct
{
    CSCSHARESTATS ss;       // The stats data.
    DWORD dwUnityFlagsReq;  // SSUF_XXXX flags set by user (requested).
    DWORD dwUnityFlagsSum;  // SSUF_XXXX flags set during enum (sum total).
    DWORD dwExcludeFlags;   // SSEF_XXXX flags.
    bool bEnumAborted;      // true if unity flags satisfied.

} CSCSHARESTATS_CBKINFO, *PCSCSHARESTATS_CBKINFO;


//
// Called by CSCEnumForStats for each CSC item enumerated.
//
DWORD
_CscShareStatisticsCallback(LPCTSTR             lpszName,
                            DWORD               dwStatus,
                            DWORD               dwHintFlags,
                            DWORD               dwPinCount,
                            WIN32_FIND_DATA    *lpFind32,
                            DWORD               dwReason,
                            DWORD               dwParam1,
                            DWORD               dwParam2,
                            DWORD_PTR           dwContext)
{
    DWORD dwResult = CSCPROC_RETURN_CONTINUE;

    if (CSCPROC_REASON_BEGIN != dwReason &&   // Not "start of data" notification.
        CSCPROC_REASON_END != dwReason &&     // Not "end of data" notification.
        1 != dwParam2)                        // Not "share root" entry.
    {
        PCSCSHARESTATS_CBKINFO pssci = (PCSCSHARESTATS_CBKINFO)(dwContext);
        PCSCSHARESTATS pss = &(pssci->ss);
        const DWORD dwExcludeFlags  = pssci->dwExcludeFlags;
        const DWORD dwUnityFlagsReq = pssci->dwUnityFlagsReq;
        const bool bIsDir           = (0 == dwParam1);
        const bool bAccessUser      = CscAccessUser(dwStatus);
        const bool bAccessGuest     = CscAccessGuest(dwStatus);
        const bool bAccessOther     = CscAccessOther(dwStatus);

        if (0 != dwExcludeFlags)
        {
            //
            // Caller want's to exclude some items from the enumeration.
            // If item is in "excluded" specification, return early.
            //
            if (0 != (dwExcludeFlags & (dwStatus & SSEF_CSCMASK)))
            {
                return dwResult;
            }
            if ((bIsDir && (dwExcludeFlags & SSEF_DIRECTORY)) || 
                (!bIsDir && (dwExcludeFlags & SSEF_FILE)))
            {
                return dwResult;
            }

            const struct
            {
                DWORD fExclude;
                bool bAccess;
                BYTE  fMask;

            } rgExclAccess[] = {{ SSEF_NOACCUSER,  bAccessUser,  0x01 },
                                { SSEF_NOACCGUEST, bAccessGuest, 0x02 },
                                { SSEF_NOACCOTHER, bAccessOther, 0x04 }};

            BYTE fExcludeMask = 0;
            BYTE fNoAccessMask  = 0;
            for (int i = 0; i < ARRAYSIZE(rgExclAccess); i++)
            {
                if (dwExcludeFlags & rgExclAccess[i].fExclude)
                    fExcludeMask |= rgExclAccess[i].fMask;

                if (!rgExclAccess[i].bAccess)
                    fNoAccessMask |= rgExclAccess[i].fMask;
            }

            if (SSEF_NOACCAND & dwExcludeFlags)
            {
                //
                // Treat all access exclusion flags as a single unit.
                //
                if (fExcludeMask == fNoAccessMask)
                    return dwResult;
            }
            else
            {
                //
                // Treat each access flag individually.  Only one specified access
                // condition must be true to exclude this file.
                //
                if (fExcludeMask & fNoAccessMask)
                    return dwResult;
            }
        }

        if (0 == (SSEF_DIRECTORY & dwExcludeFlags) || !bIsDir)
        {
            pss->cTotal++;
            pssci->dwUnityFlagsSum |= SSUF_TOTAL;

            if (0 != (dwHintFlags & (FLAG_CSC_HINT_PIN_USER | FLAG_CSC_HINT_PIN_ADMIN)))
            {
                pss->cPinned++;
                pssci->dwUnityFlagsSum |= SSUF_PINNED;
            }
            if (0 != (dwStatus & FLAG_CSCUI_COPY_STATUS_LOCALLY_DIRTY))
            {
                //
                // If the current user doesn't have sufficient access
                // to merge offline changes, then someone else must have
                // modified the file, so don't count it for this user.
                //
                if (bIsDir || CscCanUserMergeFile(dwStatus))
                {
                    pss->cModified++;
                    pssci->dwUnityFlagsSum |= SSUF_MODIFIED;
                }
            }

            const struct
            {
                DWORD flag;
                int  *pCount;
                bool bAccess;

            } rgUnity[] = {{ SSUF_ACCUSER,  &pss->cAccessUser,  bAccessUser  },
                           { SSUF_ACCGUEST, &pss->cAccessGuest, bAccessGuest },
                           { SSUF_ACCOTHER, &pss->cAccessOther, bAccessOther }};

            DWORD fUnityMask  = 0;
            DWORD fAccessMask = 0;
            for (int i = 0; i < ARRAYSIZE(rgUnity); i++)
            {
                if (dwUnityFlagsReq & rgUnity[i].flag)
                    fUnityMask |= rgUnity[i].flag;

                if (rgUnity[i].bAccess)
                {
                    (*rgUnity[i].pCount)++;
                    fAccessMask |= rgUnity[i].flag;
                }
            }
            if (SSUF_ACCAND & dwUnityFlagsReq)
            {
                //
                // Treat all access unity flags as a single unit.
                // We only signal unity if all of the specified access 
                // unity conditions are true.
                //
                if (fUnityMask == fAccessMask)
                    pssci->dwUnityFlagsSum |= fUnityMask;
            }
            else
            {
                //
                // Treat all access exclusion flags individually.
                //
                if (fUnityMask & fAccessMask)
                {
                    if (SSUF_ACCOR & dwUnityFlagsReq)
                        pssci->dwUnityFlagsSum |= fUnityMask;
                    else
                        pssci->dwUnityFlagsSum |= fAccessMask;
                }
            }

            if (bIsDir)
            {
                pss->cDirs++;
                pssci->dwUnityFlagsSum |= SSUF_DIRS;
            }
            // Note the 'else': don't count dirs in the sparse total
            else if (0 != (dwStatus & FLAG_CSC_COPY_STATUS_SPARSE))
            {
                pss->cSparse++;
                pssci->dwUnityFlagsSum |= SSUF_SPARSE;
            }

            if (0 != dwUnityFlagsReq)
            {
                //
                // Abort enumeration if all of the requested SSUF_XXXX unity flags 
                // have been set.
                //
                if (dwUnityFlagsReq == (dwUnityFlagsReq & pssci->dwUnityFlagsSum))
                {
                   dwResult = CSCPROC_RETURN_ABORT;
                   pssci->bEnumAborted;
                }
            }
        }
    }

    return dwResult;
}

//
// Enumerate all items for a given share and tally up the
// relevant information like file count, pinned count etc.
// Information is returned through *pss.
//
BOOL
_GetShareStatistics(
    LPCTSTR pszShare, 
    PCSCGETSTATSINFO pi,
    PCSCSHARESTATS pss
    )
{
    typedef BOOL (WINAPI * PFNENUMFORSTATS)(LPCTSTR, LPCSCPROC, DWORD_PTR);

    CSCSHARESTATS_CBKINFO ssci;
    BOOL bResult;
    DWORD dwShareStatus = 0;
    PFNENUMFORSTATS pfnEnumForStats = CSCEnumForStats;

    ZeroMemory(&ssci, sizeof(ssci));
    ssci.dwUnityFlagsReq = pi->dwUnityFlags;
    ssci.dwExcludeFlags  = pi->dwExcludeFlags;

    if (pi->bAccessInfo ||
        (pi->dwUnityFlags & (SSUF_ACCUSER | SSUF_ACCGUEST | SSUF_ACCOTHER)) ||
        (pi->dwExcludeFlags & (SSEF_NOACCUSER | SSEF_NOACCGUEST | SSEF_NOACCOTHER)))
    {
        //
        // If the enumeration requires access information, use the "ex" version
        // of the EnumForStats CSC api.  Only use it if necessary because gathering
        // the access information has a perf cost.
        //
        pfnEnumForStats = CSCEnumForStatsEx;
    }

    pi->bEnumAborted = false;

    bResult = (*pfnEnumForStats)(pszShare, _CscShareStatisticsCallback, (DWORD_PTR)&ssci);
    *pss = ssci.ss;

    if (CSCQueryFileStatus(pszShare, &dwShareStatus, NULL, NULL))
    {
        if (FLAG_CSC_SHARE_STATUS_FILES_OPEN & dwShareStatus)
        {
            pss->bOpenFiles = true;
        }
        if (FLAG_CSC_SHARE_STATUS_DISCONNECTED_OP & dwShareStatus)
        {
            pss->bOffline = true;
        }
    }
    pi->bEnumAborted = ssci.bEnumAborted;

    return bResult;
}

//
// Retrieve the statistics for the entire cache.
// This is a simple wrapper that calls _GetShareStatistics for each share
// in the cache then sums the results for the entire cache.  It accepts
// the same unity and exclusion flags used by _GetShareStatistics.
//
BOOL
_GetCacheStatistics(
    PCSCGETSTATSINFO pi,
    PCSCCACHESTATS pcs
    )
{
    BOOL bResult = TRUE;
    WIN32_FIND_DATA fd;
    CSCSHARESTATS ss;

    ZeroMemory(pcs, sizeof(*pcs));

    pi->bEnumAborted = false;

    CCscFindHandle hFind(CacheFindFirst(NULL, &fd, NULL, NULL, NULL, NULL));
    if (hFind.IsValid())
    {
        do
        {
            pcs->cShares++;
            if (bResult = _GetShareStatistics(fd.cFileName, 
                                              pi,
                                              &ss))
            {
                pcs->cTotal               += ss.cTotal;
                pcs->cPinned              += ss.cPinned;
                pcs->cModified            += ss.cModified;
                pcs->cSparse              += ss.cSparse;
                pcs->cDirs                += ss.cDirs;
                pcs->cAccessUser          += ss.cAccessUser;
                pcs->cAccessGuest         += ss.cAccessGuest;
                pcs->cAccessOther         += ss.cAccessOther;
                pcs->cSharesOffline       += int(ss.bOffline);
                pcs->cSharesWithOpenFiles += int(ss.bOpenFiles);
            }
        }
        while(bResult && !pi->bEnumAborted && CacheFindNext(hFind, &fd, NULL, NULL, NULL, NULL));
    }

    return bResult;
}


//
// Sets the proper exclusion flags to report only on files accessible by the
// logged on user.  Otherwise it's the same as calling _GetShareStatistics.
//
BOOL
_GetShareStatisticsForUser(
    LPCTSTR pszShare, 
    PCSCGETSTATSINFO pi,
    PCSCSHARESTATS pss
    )
{
    pi->dwExcludeFlags |= SSEF_NOACCUSER | SSEF_NOACCGUEST | SSEF_NOACCAND;
    return _GetShareStatistics(pszShare, pi, pss);
}


//
// Sets the proper exclusion flags to report only on files accessible by the
// logged on user.  Otherwise it's the same as calling _GetCacheStatistics.
//
BOOL
_GetCacheStatisticsForUser(
    PCSCGETSTATSINFO pi,
    PCSCCACHESTATS pcs
    )
{
    pi->dwExcludeFlags |= SSEF_NOACCUSER | SSEF_NOACCGUEST | SSEF_NOACCAND;
    return _GetCacheStatistics(pi, pcs);
}


//
// CSCUI version of reboot.  Requires security goo.
// This code was pattered after that found in \shell\shell32\restart.c
// function CommonRestart().
//
DWORD 
CSCUIRebootSystem(
    void
    )
{
    TraceEnter(TRACE_UTIL, "CSCUIRebootSystem");
    DWORD dwOldState, dwStatus, dwSecError;
    DWORD dwRebootError = ERROR_SUCCESS;

    SetLastError(0);           // Be really safe about last error value!
    dwStatus = Security_SetPrivilegeAttrib(SE_SHUTDOWN_NAME,
                                           SE_PRIVILEGE_ENABLED,
                                           &dwOldState);
    dwSecError = GetLastError();  // ERROR_NOT_ALL_ASSIGNED sometimes    

    if (!ExitWindowsEx(EWX_REBOOT, 0))
    {
        dwRebootError = GetLastError();
        Trace((TEXT("Error %d rebooting system"), dwRebootError));
    }
    if (NT_SUCCESS(dwStatus))
    {
        if (ERROR_SUCCESS == dwSecError)
        {
            Security_SetPrivilegeAttrib(SE_SHUTDOWN_NAME, dwOldState, NULL);
        }
        else
        {
            Trace((TEXT("Error %d setting SE_SHUTDOWN_NAME privilege"), dwSecError));
        }
    }
    else
    {
        Trace((TEXT("Error %d setting SE_SHUTDOWN_NAME privilege"), dwStatus));
    }
    TraceLeaveResult(dwRebootError);
}


//
// Retrieve location, size and file/directory count information for the 
// CSC cache.  If CSC is disabled, information is gathered about the
// system volume.  That's where the CSC agent will put the cache when
// one is created.
//
void
GetCscSpaceUsageInfo(
    CSCSPACEUSAGEINFO *psui
    )
{
    ULARGE_INTEGER ulTotalBytes = {0, 0};
    ULARGE_INTEGER ulUsedBytes = {0, 0};

    ZeroMemory(psui, sizeof(*psui));
    CSCGetSpaceUsage(psui->szVolume,
                     ARRAYSIZE(psui->szVolume),
                     &ulTotalBytes.HighPart,
                     &ulTotalBytes.LowPart,
                     &ulUsedBytes.HighPart,
                     &ulUsedBytes.LowPart,
                     &psui->dwNumFilesInCache,
                     &psui->dwNumDirsInCache);

    if (TEXT('\0') == psui->szVolume[0])
    {
        //
        // CSCGetSpaceUsage didn't give us a volume name.  Probably because
        // CSC hasn't been enabled on the system.  Default to the system
        // drive because that's what CSC uses anyway.
        //
        GetSystemDirectory(psui->szVolume, ARRAYSIZE(psui->szVolume));
        psui->dwNumFilesInCache = 0;
        psui->dwNumDirsInCache  = 0;
    }

    PathStripToRoot(psui->szVolume);
    DWORD spc = 0; // Sectors per cluster.
    DWORD bps = 0; // Bytes per sector.
    DWORD fc  = 0; // Free clusters.
    DWORD nc  = 0; // Total clusters.
    GetDiskFreeSpace(psui->szVolume, &spc, &bps, &fc, &nc);

    psui->llBytesOnVolume     = (LONGLONG)nc * (LONGLONG)spc * (LONGLONG)bps;
    psui->llBytesTotalInCache = ulTotalBytes.QuadPart;
    psui->llBytesUsedInCache  = ulUsedBytes.QuadPart;
}



//-----------------------------------------------------------------------------
// This is code taken from shell32's utils.cpp file.
// We need the function SHSimpleIDListFromFindData() but it's not exported
// from shell32.  Therefore, until it is, we just lifted the code.
// [brianau - 9/28/98]
//-----------------------------------------------------------------------------
class CFileSysBindData: public IFileSystemBindData
{ 
public:
    CFileSysBindData();
    
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // IFileSystemBindData
    STDMETHODIMP SetFindData(const WIN32_FIND_DATAW *pfd);
    STDMETHODIMP GetFindData(WIN32_FIND_DATAW *pfd);

private:
    ~CFileSysBindData();
    
    LONG _cRef;
    WIN32_FIND_DATAW _fd;
};


CFileSysBindData::CFileSysBindData() : _cRef(1)
{
    ZeroMemory(&_fd, sizeof(_fd));
}

CFileSysBindData::~CFileSysBindData()
{
}

HRESULT CFileSysBindData::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFileSysBindData, IFileSystemBindData), // IID_IFileSystemBindData
         { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CFileSysBindData::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFileSysBindData::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CFileSysBindData::SetFindData(const WIN32_FIND_DATAW *pfd)
{
    _fd = *pfd;
    return S_OK;
}

HRESULT CFileSysBindData::GetFindData(WIN32_FIND_DATAW *pfd) 
{
    *pfd = _fd;
    return S_OK;
}


HRESULT
SHCreateFileSysBindCtx(
    const WIN32_FIND_DATA *pfd, 
    IBindCtx **ppbc
    )
{
    HRESULT hres;
    IFileSystemBindData *pfsbd = new CFileSysBindData();
    if (pfsbd)
    {
        if (pfd)
        {
            WIN32_FIND_DATAW fdw;
            memcpy(&fdw, pfd, FIELD_OFFSET(WIN32_FIND_DATAW, cFileName));
            SHTCharToUnicode(pfd->cFileName, fdw.cFileName, ARRAYSIZE(fdw.cFileName));
            SHTCharToUnicode(pfd->cAlternateFileName, fdw.cAlternateFileName, ARRAYSIZE(fdw.cAlternateFileName));
            pfsbd->SetFindData(&fdw);
        }

        hres = CreateBindCtx(0, ppbc);
        if (SUCCEEDED(hres))
        {
            BIND_OPTS bo = {sizeof(bo)};  // Requires size filled in.
            bo.grfMode = STGM_CREATE;
            (*ppbc)->SetBindOptions(&bo);
            (*ppbc)->RegisterObjectParam(STR_FILE_SYS_BIND_DATA, pfsbd);
        }
        pfsbd->Release();
    }
    else
    {
        *ppbc = NULL;
        hres = E_OUTOFMEMORY;
    }
    return hres;
}


HRESULT
SHSimpleIDListFromFindData(
    LPCTSTR pszPath, 
    const WIN32_FIND_DATA *pfd, 
    LPITEMIDLIST *ppidl
    )
{
    IShellFolder *psfDesktop;
    HRESULT hres = SHGetDesktopFolder(&psfDesktop);
    if (SUCCEEDED(hres))
    {
        IBindCtx *pbc;
        hres = SHCreateFileSysBindCtx(pfd, &pbc);
        if (SUCCEEDED(hres))
        {
            WCHAR wszPath[MAX_PATH];

            SHTCharToUnicode(pszPath, wszPath, ARRAYSIZE(wszPath));

            hres = psfDesktop->ParseDisplayName(NULL, pbc, wszPath, NULL, ppidl, NULL);
            pbc->Release();
        }
        psfDesktop->Release();
    }

    if (FAILED(hres))
        *ppidl = NULL;
    return hres;
}


//
// Number of times a CSC API will be repeated if it fails.
// In particular, this is used for CSCDelete and CSCFillSparseFiles; both of
// which can fail on one call but succeed the next.  This isn't designed
// behavior but it is reality.  ShishirP knows about it and may be able to
// investigate later. [brianau - 4/2/98]
// 
const int CSC_API_RETRIES = 3;

//
// Occasionally if a call to a CSC API fails with ERROR_ACCESS_DENIED, 
// repeating the call will succeed.
// Here we wrap up the call to CSCDelete so that it is called multiple
// times in the case of these failures.
//
DWORD
CscDelete(
    LPCTSTR pszPath
    )
{
    DWORD dwError = ERROR_SUCCESS;
    int nRetries = CSC_API_RETRIES;
    while(0 < nRetries--)
    {
        if (CSCDelete(pszPath))
            return ERROR_SUCCESS;

        dwError = GetLastError();
        if (ERROR_ACCESS_DENIED != dwError)
            return dwError;
    }
    if (ERROR_SUCCESS == dwError)
    {
        //
        // BUGBUG:  Hack for some CSC APIs returning
        //          ERROR_SUCCESS even though they fail.
        //
        dwError = ERROR_GEN_FAILURE;
    }
    return dwError;
}


void 
EnableDlgItems(
    HWND hwndDlg, 
    const UINT* pCtlIds, 
    int cCtls, 
    bool bEnable
    )
{
    for (int i = 0; i < cCtls; i++)
    {
        EnableWindow(GetDlgItem(hwndDlg, *(pCtlIds + i)), bEnable);
    }
}

void 
ShowDlgItems(
    HWND hwndDlg, 
    const UINT* pCtlIds, 
    int cCtls, 
    bool bShow
    )
{
    const int nCmdShow = bShow ? SW_NORMAL : SW_HIDE;

    for (int i = 0; i < cCtls; i++)
    {
        ShowWindow(GetDlgItem(hwndDlg, *(pCtlIds + i)), nCmdShow);
    }
}


//
// Set/Clear the sync-at-logon-logoff flags for our SyncMgr handler.
// When set, SyncMgr will include Offline Files in any sync activity
// at logon and/or logoff.
//
// dwFlagsRequested - Value of flags bits.  1 == set, 0 == clear.
// dwMask           - Mask describing which flags bits to use.
//
// Both dwMask and dwFlagsRequested may be one of the following:
//
//  0
//  SYNCMGRREGISTER_CONNECT
//  SYNCMGRREGISTER_PENDINGDISCONNECT
//  SYNCMGRREGISTER_CONNECT | SYNCMGRREGISTER_PENDINGDISCONNECT
//
HRESULT
RegisterForSyncAtLogonAndLogoff(
    DWORD dwMask,
    DWORD dwFlagsRequested
    )
{
    CCoInit coinit;
    HRESULT hr = coinit.Result();
    if (SUCCEEDED(hr))
    {
        ISyncMgrRegisterCSC *pSyncRegister = NULL;
        hr = CoCreateInstance(CLSID_SyncMgr,
                              NULL,
                              CLSCTX_SERVER,
                              IID_ISyncMgrRegisterCSC,
                              (LPVOID*)&pSyncRegister);
        if (SUCCEEDED(hr))
        {
            //
            // Re-register the sync mgr handler with the "connect" and "disconnect" 
            // flags set.  Other existing flags are left unmodified.
            //
            DWORD dwFlagsActual;
            hr = pSyncRegister->GetUserRegisterFlags(&dwFlagsActual);
            if (SUCCEEDED(hr))
            {
                const DWORD LOGON   = SYNCMGRREGISTERFLAG_CONNECT;
                const DWORD LOGOFF  = SYNCMGRREGISTERFLAG_PENDINGDISCONNECT;

                if (dwMask & LOGON)
                {
                    if (dwFlagsRequested & LOGON)
                        dwFlagsActual |= LOGON;
                    else
                        dwFlagsActual &= ~LOGON;
                }
                
                if (dwMask & LOGOFF)
                {
                    if (dwFlagsRequested & LOGOFF)
                        dwFlagsActual |= LOGOFF;
                    else
                        dwFlagsActual &= ~LOGOFF;
                }
                
                hr = pSyncRegister->SetUserRegisterFlags(dwMask & (LOGON | LOGOFF), 
                                                         dwFlagsActual);
            }
            pSyncRegister->Release();
        }
    }
    return hr;
}


//
// Determine if we're registered for sync at logon/logoff.
// Returns:
//      S_OK    = We're registered.  Query *pbLogon and *pbLogoff to
//                determine specifics if you're interested.
//      S_FALSE = We're not registered.
//      Other   = Couldn't determine because of some error.
//
HRESULT
IsRegisteredForSyncAtLogonAndLogoff(
    bool *pbLogon,
    bool *pbLogoff
    )
{
    bool bLogon  = false;
    bool bLogoff = false;
    CCoInit coinit;
    HRESULT hr = coinit.Result();
    if (SUCCEEDED(hr))
    {
        ISyncMgrRegisterCSC *pSyncRegister = NULL;
        hr = CoCreateInstance(CLSID_SyncMgr,
                              NULL,
                              CLSCTX_SERVER,
                              IID_ISyncMgrRegisterCSC,
                              (LPVOID*)&pSyncRegister);
        if (SUCCEEDED(hr))
        {
            DWORD dwFlags;
            hr = pSyncRegister->GetUserRegisterFlags(&dwFlags);
            if (SUCCEEDED(hr))
            {
                hr      = S_FALSE;
                bLogon  = (0 != (SYNCMGRREGISTERFLAG_CONNECT & dwFlags));
                bLogoff = (0 != (SYNCMGRREGISTERFLAG_PENDINGDISCONNECT & dwFlags));
                if (bLogon || bLogoff)
                    hr = S_OK;
            }
            pSyncRegister->Release();
        }
    }
    if (NULL != pbLogon)
        *pbLogon = bLogon;
    if (NULL != pbLogoff)
        *pbLogoff = bLogoff;

    return hr;
}

//
// Determine if the parent net share for a given UNC path has an open
// connection on the local machine.
//
// Returns:
//
//      S_OK        = There is an open connection to the share.
//      S_FALSE     = No open connection to the share.
//      other       = Some error code.
//
HRESULT
IsOpenConnectionPathUNC(
    LPCTSTR pszPathUNC
    )
{
    TCHAR szShare[MAX_PATH * 2];
    lstrcpyn(szShare, pszPathUNC, ARRAYSIZE(szShare));
    PathStripToRoot(szShare);
    return PathIsUNCServerShare(szShare) && IsOpenConnectionShare(szShare);
}


//
// Determine if a net share has an open connection on the local machine.
//
// Returns:
//
//      S_OK        = There is an open connection to the share.
//      S_FALSE     = No open connection to the share.
//      other       = Some error code.
//
HRESULT
IsOpenConnectionShare(
    LPCTSTR pszShare
    )
{
    DWORD dwStatus;
    if (CSCQueryFileStatus(pszShare, &dwStatus, NULL, NULL))
    {
        if (FLAG_CSC_SHARE_STATUS_CONNECTED & dwStatus)
            return S_OK;
    }
    return S_FALSE;
}


// With this version of CSCIsCSCEnabled, we can delay all extra dll loads
// (including cscdll.dll) until we actually see a net file/folder.
#include <devioctl.h>
#include <shdcom.h>
static TCHAR const c_szShadowDevice[] = TEXT("\\\\.\\shadow");

BOOL IsCSCEnabled(void)
{
    BOOL bIsCSCEnabled = FALSE;
    SHADOWINFO sSI = {0};
    ULONG ulBytesReturned;

    HANDLE hShadowDB = CreateFile(c_szShadowDevice,
                                  FILE_EXECUTE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  OPEN_EXISTING,
                                  0,
                                  NULL);
    if (INVALID_HANDLE_VALUE == hShadowDB)
        return FALSE;

    sSI.uStatus = SHADOW_SWITCH_SHADOWING;
    sSI.uOp = SHADOW_SWITCH_GET_STATE;

    if (DeviceIoControl(hShadowDB,
                        IOCTL_SWITCHES,
                        (LPVOID)(&sSI),
                        0,
                        NULL,
                        0,
                        &ulBytesReturned,
                        NULL))
    {
        bIsCSCEnabled = (sSI.uStatus & SHADOW_SWITCH_SHADOWING);
    }
    CloseHandle(hShadowDB);

    return bIsCSCEnabled;
}



BOOL
IsSyncInProgress(
    void
    )
{
    BOOL bSyncInProgress = FALSE; // Assume sync NOT in progress.
    
    HANDLE hMutex = OpenMutex(SYNCHRONIZE, FALSE, c_szSyncInProgMutex);
    if (NULL != hMutex)
    {
        switch(WaitForSingleObject(hMutex, 0))
        {
            case WAIT_TIMEOUT:
                //
                // Could NOT get ownership of mutex.  Sync is in progress.
                //
                bSyncInProgress = TRUE;
                break;

            case WAIT_OBJECT_0:
                //
                // Took ownership of mutex.  Sync not in progress.
                //
                ReleaseMutex(hMutex);
                break;

            case WAIT_ABANDONED:
                //
                // Mutex was abandoned by it's owning thread.
                // Probably the result of someone killing mobsync.exe
                // during a sync.  Note that this can happen through a 
                // "Mobsync not responding, kill now?" dialog that is
                // displayed by mobsync if it appears to be hung.
                //
            default:
                break;
        }
        CloseHandle(hMutex);
    }
    return bSyncInProgress;
}

BOOL
IsPurgeInProgress(
    void
    )
{
    BOOL bPurgeInProgress = FALSE; // Assume purge NOT in progress.

    HANDLE hMutex = OpenMutex(SYNCHRONIZE, FALSE, c_szPurgeInProgMutex);
    if (NULL != hMutex)
    {
        switch(WaitForSingleObject(hMutex, 0))
        {
            case WAIT_TIMEOUT:
                //
                // Could NOT get ownership of mutex.  Purge is in progress.
                //
                bPurgeInProgress = TRUE;
                break;

            case WAIT_OBJECT_0:
                //
                // Took ownership of mutex.  Purge not in progress.
                //
                ReleaseMutex(hMutex);
                break;

            case WAIT_ABANDONED:
                //
                // Mutex was abandoned by it's owning thread.
                //
            default:
                break;
        }
        CloseHandle(hMutex);
    }
    return bPurgeInProgress;
}

//
// The desktop is a virtual shell folder that caches it's enumerator.
// We need to invalidate that enumerator before we refresh the desktop
// in cases where the desktop folder is redirected to a network share and
// we have just reconnected that network share.  We invalidate by sending
// a SHCNE_DISKEVENTS notification event to the shell for the desktop IDL.
//
void InvalidateTheDesktop(void)
{
    LPITEMIDLIST pidl;
    if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidl)))
    {
        SHChangeNotify(SHCNE_DISKEVENTS, SHCNF_IDLIST | SHCNF_FLUSH, pidl, NULL);
        ILFree(pidl);
    }
}

//
// This function returns TRUE if we're running on either a TS server
// or a TS client.
//
BOOL IsWindowsTerminalServer(void)
{
    BOOL bIsTS = TRUE;  // Assume it's a TS system.

    //
    // Is it a TS client session?
    //
    if (!GetSystemMetrics(SM_REMOTESESSION))
    {
        //
        // Nope.  Is it a TS server?
        // This code is from Ara Bernardi (Hydra team).
        //
        OSVERSIONINFOEX osvi;
        DWORDLONG dwMask = 0;

        ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        osvi.wSuiteMask          = VER_SUITE_TERMINAL;

        VER_SET_CONDITION(dwMask, VER_SUITENAME, VER_AND);

        bIsTS = VerifyVersionInfo(&osvi, VER_SUITENAME, dwMask); 
    }
    return bIsTS;
}

//---------------------------------------------------------------
// DataObject helper functions.
// These are roughly taken from similar functions in 
// shell\shell32\datautil.cpp
//---------------------------------------------------------------
HRESULT
DataObject_SetBlob(
    IDataObject *pdtobj,
    CLIPFORMAT cf, 
    LPCVOID pvBlob,
    UINT cbBlob
    )
{
    HRESULT hr = E_OUTOFMEMORY;
    LPVOID pv = GlobalAlloc(GPTR, cbBlob);
    if (pv)
    {
        CopyMemory(pv, pvBlob, cbBlob);
        hr = DataObject_SetGlobal(pdtobj, cf, pv);

        if (FAILED(hr))
            GlobalFree((HGLOBAL)pv);
    }
    return hr;
}

HRESULT
DataObject_GetBlob(
    IDataObject *pdtobj, 
    CLIPFORMAT cf, 
    LPVOID pvBlob, 
    UINT cbBlob
    )
{
    STGMEDIUM medium = {0};
    FORMATETC fmte = {cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    HRESULT hr = pdtobj->GetData(&fmte, &medium);
    if (SUCCEEDED(hr))
    {
        LPVOID pv = GlobalLock(medium.hGlobal);
        if (pv)
        {
            CopyMemory(pvBlob, pv, cbBlob);
            GlobalUnlock(medium.hGlobal);
        }
        else
        {
            hr = E_UNEXPECTED;
        }
        ReleaseStgMedium(&medium);
    }
    return hr;
}


HRESULT
DataObject_SetGlobal(
    IDataObject *pdtobj,
    CLIPFORMAT cf, 
    HGLOBAL hGlobal
    )
{
    FORMATETC fmte = {cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium;

    medium.tymed = TYMED_HGLOBAL;
    medium.hGlobal = hGlobal;
    medium.pUnkForRelease = NULL;

    return pdtobj->SetData(&fmte, &medium, TRUE);
}


HRESULT
DataObject_SetDWORD(
    IDataObject *pdtobj,
    CLIPFORMAT cf, 
    DWORD dw
    )
{
    return DataObject_SetBlob(pdtobj, cf, &dw, sizeof(dw));
}


HRESULT
DataObject_GetDWORD(
    IDataObject *pdtobj, 
    CLIPFORMAT cf, 
    DWORD *pdwOut
    )
{
    return DataObject_GetBlob(pdtobj, cf, pdwOut, sizeof(DWORD));
}
 

HRESULT
SetGetLogicalPerformedDropEffect(
    IDataObject *pdtobj,
    DWORD *pdwEffect,
    bool bSet
    )
{
    HRESULT hr = NOERROR;
    static CLIPFORMAT cf;
    if ((CLIPFORMAT)0 == cf)
        cf = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_LOGICALPERFORMEDDROPEFFECT);

    if (bSet)
    {
        hr = DataObject_SetDWORD(pdtobj, cf, *pdwEffect);
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
        DataObject_GetDWORD(pdtobj, cf, pdwEffect);
    }        
        
    return hr;
}

DWORD 
GetLogicalPerformedDropEffect(
    IDataObject *pdtobj
    )
{
    DWORD dwEffect = DROPEFFECT_NONE;
    SetGetLogicalPerformedDropEffect(pdtobj, &dwEffect, false);
    return dwEffect;
}

HRESULT
SetLogicalPerformedDropEffect(
    IDataObject *pdtobj,
    DWORD dwEffect
    )
{
    return SetGetLogicalPerformedDropEffect(pdtobj, &dwEffect, true);
}


HRESULT
SetGetPreferredDropEffect(
    IDataObject *pdtobj,
    DWORD *pdwEffect,
    bool bSet
    )
{
    HRESULT hr = NOERROR;
    static CLIPFORMAT cf;
    if ((CLIPFORMAT)0 == cf)
        cf = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);

    if (bSet)
    {
        hr = DataObject_SetDWORD(pdtobj, cf, *pdwEffect);
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
        DataObject_GetDWORD(pdtobj, cf, pdwEffect);
    }        
        
    return hr;
}

DWORD 
GetPreferredDropEffect(
    IDataObject *pdtobj
    )
{
    DWORD dwEffect = DROPEFFECT_NONE;
    SetGetPreferredDropEffect(pdtobj, &dwEffect, false);
    return dwEffect;
}

HRESULT
SetPreferredDropEffect(
    IDataObject *pdtobj,
    DWORD dwEffect
    )
{
    return SetGetPreferredDropEffect(pdtobj, &dwEffect, true);
}


//
// Wrap CSCFindFirstFile so we don't enumerate "." or "..".
// Wrapper also helps code readability.
// 
HANDLE 
CacheFindFirst(
    LPCTSTR pszPath, 
    PSID psid,
    WIN32_FIND_DATA *pfd,
    DWORD *pdwStatus,
    DWORD *pdwPinCount,
    DWORD *pdwHintFlags,
    FILETIME *pft
    )
{ 
    HANDLE hFind = CSCFindFirstFileForSid(pszPath, 
                                          psid,
                                          pfd, 
                                          pdwStatus, 
                                          pdwPinCount, 
                                          pdwHintFlags, 
                                          pft); 

    while(INVALID_HANDLE_VALUE != hFind && PathIsDotOrDotDot(pfd->cFileName))
    {
        if (!CSCFindNextFile(hFind, 
                             pfd, 
                             pdwStatus, 
                             pdwPinCount, 
                             pdwHintFlags, 
                             pft))
        {
            CSCFindClose(hFind);
            hFind = INVALID_HANDLE_VALUE;
        }
    }
    return hFind;
}


//
// Wrap CSCFindFirstFile so we don't enumerate "." or "..".
// Wrapper also helps code readability.
//
BOOL 
CacheFindNext(
    HANDLE hFind, 
    WIN32_FIND_DATA *pfd,
    DWORD *pdwStatus,
    DWORD *pdwPinCount,
    DWORD *pdwHintFlags,
    FILETIME *pft
    )
{   
    BOOL bResult = FALSE;
    do
    {
        bResult = CSCFindNextFile(hFind, 
                                  pfd, 
                                  pdwStatus, 
                                  pdwPinCount, 
                                  pdwHintFlags, 
                                  pft); 
    }
    while(bResult && PathIsDotOrDotDot(pfd->cFileName));
    return bResult;
}

//
// If there's a link to the Offline Files folder on the
// user's desktop, delete the link.
//
BOOL
DeleteOfflineFilesFolderLink(
    HWND hwndParent
    )
{    
    BOOL bResult = false;
    TCHAR szLinkPath[MAX_PATH];
    if (SUCCEEDED(COfflineFilesFolder::IsLinkOnDesktop(hwndParent, szLinkPath, ARRAYSIZE(szLinkPath))))
    {
        bResult = DeleteFile(szLinkPath);
    }
    return bResult;
}

//
// This was taken from shell\shell32\util.cpp.
//
BOOL ShowSuperHidden(void)
{
    BOOL bRet = FALSE;

    if (!SHRestricted(REST_DONTSHOWSUPERHIDDEN))
    {
        SHELLSTATE ss;

        SHGetSetSettings(&ss, SSF_SHOWSUPERHIDDEN, FALSE);
        bRet = ss.fShowSuperHidden;
    }
    return bRet;
}


BOOL ShowHidden(void)
{
    SHELLSTATE ss;
    SHGetSetSettings(&ss, SSF_SHOWALLOBJECTS, FALSE);
    return ss.fShowAllObjects;
}


BOOL IsSyncMgrInitialized(void)
{    
    //
    // Is this the first time this user has used run CSCUI?
    //
    DWORD dwValue = 0;
    DWORD cbData  = sizeof(dwValue);
    DWORD dwType;
    SHGetValue(HKEY_CURRENT_USER,
               c_szCSCKey,
               c_szSyncMgrInitialized,
               &dwType,
               (LPVOID)&dwValue,
               &cbData);
    
    return (0 != dwValue);
}


void SetSyncMgrInitialized(void)
{

    //
    // Set the "initialized" flag so our logoff code in cscst.cpp doesn't
    // try to re-register for sync-at-logon/logoff.
    //
    DWORD dwSyncMgrInitialized = 1;
    SHSetValue(HKEY_CURRENT_USER,
               c_szCSCKey,
               c_szSyncMgrInitialized,
               REG_DWORD,
               &dwSyncMgrInitialized,
               sizeof(dwSyncMgrInitialized));
}

