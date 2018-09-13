//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       update.cpp
//
//  Authors;
//    Jeff Saathoff (jeffreys)
//
//  Notes;
//    SyncMgr integration
//--------------------------------------------------------------------------
#include "pch.h"
#include "msgbox.h"     // CscWin32Message
#include "folder.h"
#include <openfile.h>   // OpenOfflineFile
#include "cscst.h"      // PostToSystray
#include "uihooks.h"    // Self-host notifications
#include "fopendlg.h"   // OpenFilesWarningDialog
#include "statdlg.h"    // ReconnectServers
#include "security.h"

#define RAS_CONNECT_DELAY       (10 * 1000)

// Maximum length of username
#define MAX_USERNAME_CHARS      64

// SYNCTHREADDATA.dwSyncStatus flags
#define SDS_SYNC_OUT                0x00000001  // CSCMergeShare
#define SDS_SYNC_IN_QUICK           0x00000002  // CSCFillSparseFiles(FALSE)
#define SDS_SYNC_IN_FULL            0x00000004  // CSCFillSparseFiles(TRUE)
#define SDS_SYNC_FORCE_INWARD       0x00000008
#define SDS_SYNC_RAS_CONNECTED      0x00000010
#define SDS_SYNC_RESTART_MERGE      0x00000020
#define SDS_SYNC_DELETE_DELETE      0x00000040
#define SDS_SYNC_DELETE_RESTORE     0x00000080
#define SDS_SYNC_AUTOCACHE          0x00000100
#define SDS_SYNC_CONFLICT_KEEPLOCAL 0x00000200
#define SDS_SYNC_CONFLICT_KEEPNET   0x00000400
#define SDS_SYNC_CONFLICT_KEEPBOTH  0x00000800
#define SDS_SYNC_STARTED            0x00010000
#define SDS_SYNC_ERROR              0x00020000
#define SDS_SYNC_CANCELLED          0x00040000
#define SDS_SYNC_FILE_SKIPPED       0x00080000

#define SDS_SYNC_DELETE_CONFLICT_MASK   (SDS_SYNC_DELETE_DELETE | SDS_SYNC_DELETE_RESTORE)
#define SDS_SYNC_FILE_CONFLICT_MASK     (SDS_SYNC_CONFLICT_KEEPLOCAL | SDS_SYNC_CONFLICT_KEEPNET | SDS_SYNC_CONFLICT_KEEPBOTH)


// Sync Flags used internally by CCscUpdate
#define CSC_SYNC_OUT                0x00000001L
#define CSC_SYNC_IN_QUICK           0x00000002L
#define CSC_SYNC_IN_FULL            0x00000004L
#define CSC_SYNC_SETTINGS           0x00000008L
#define CSC_SYNC_MAYBOTHERUSER      0x00000010L
#define CSC_SYNC_NOTIFY_SYSTRAY     0x00000020L
#define CSC_SYNC_LOGOFF             0x00000040L
#define CSC_SYNC_LOGON              0x00000080L
#define CSC_SYNC_IDLE               0x00000100L
#define CSC_SYNC_NONET              0x00000200L
#define CSC_SYNC_PINFILES           0x00000400L
#define CSC_SYNC_PIN_RECURSE        0x00000800L
#define CSC_SYNC_OFWARNINGDONE      0x00001000L
#define CSC_SYNC_CANCELLED          0x00002000L
#define CSC_SYNC_SHOWUI_ALWAYS      0x00004000L
#define CSC_SYNC_IGNORE_ACCESS      0x00008000L
#define CSC_SYNC_SKIP_EFS           0x00010000L
#define CSC_SYNC_EFS_WARNING_SHOWN  0x00020000L
#define CSC_SYNC_RECONNECT          0x00040000L

#define CSC_LOCALLY_MODIFIED    (FLAG_CSC_COPY_STATUS_DATA_LOCALLY_MODIFIED         \
                                    | FLAG_CSC_COPY_STATUS_LOCALLY_DELETED          \
                                    | FLAG_CSC_COPY_STATUS_LOCALLY_CREATED)

HICON g_hCscIcon = NULL;

// Used for marshalling data into the SyncMgr process
typedef struct _CSC_UPDATE_DATA
{
    DWORD dwUpdateFlags;
    DWORD dwFileBufferOffset;
} CSC_UPDATE_DATA, *PCSC_UPDATE_DATA;


LPTSTR GetErrorText(DWORD dwErr)
{
    UINT idString = (UINT)-1;
    LPTSTR pszError = NULL;

    switch (dwErr)
    {
    case ERROR_INVALID_NAME:
        // "Files of this type cannot be made available offline."
        idString = IDS_CACHING_DISALLOWED;
        break;
    }

    if ((UINT)-1 != idString)
    {
        LoadStringAlloc(&pszError, g_hInstance, idString);
    }
    else if (NOERROR != dwErr)
    {
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                      NULL,
                      dwErr,
                      0,
                      (LPTSTR)&pszError,
                      1,
                      NULL);
    }
    return pszError;
}


//*************************************************************
//
//  CscRegisterHandler
//
//  Purpose:    Register/unregister CSC Update handler with SyncMgr
//
//  Parameters: bRegister - TRUE to register, FALSE to unregister
//              punkSyncMgr - (optional) instance of SyncMgr to use
//
//  Return:     HRESULT
//
//*************************************************************
HRESULT
CscRegisterHandler(BOOL bRegister, LPUNKNOWN punkSyncMgr)
{
    HRESULT hr;
    HRESULT hrComInit = E_FAIL;
    ISyncMgrRegister *pSyncRegister = NULL;
    const DWORD dwRegFlags = SYNCMGRREGISTERFLAG_CONNECT | SYNCMGRREGISTERFLAG_PENDINGDISCONNECT;

    static BOOL s_bSyncHandlerRegistered = FALSE;

    TraceEnter(TRACE_UPDATE, "CscRegisterHandler");

    if (bRegister && s_bSyncHandlerRegistered)
        TraceLeaveResult(S_OK); // already registered
    // Note: if bRegister and s_bSyncHandlerRegistered are both FALSE, unregister anyway.

    if (punkSyncMgr)
    {
        hr = punkSyncMgr->QueryInterface(IID_ISyncMgrRegister, (LPVOID*)&pSyncRegister);
    }
    else
    {
        hrComInit = CoInitialize(NULL);
        hr = CoCreateInstance(CLSID_SyncMgr,
                              NULL,
                              CLSCTX_SERVER,
                              IID_ISyncMgrRegister,
                              (LPVOID*)&pSyncRegister);
    }
    FailGracefully(hr, "Unable to get ISyncMgrRegister interface");

    if (bRegister)
        hr = pSyncRegister->RegisterSyncMgrHandler(CLSID_CscUpdateHandler, NULL, dwRegFlags);
    else
        hr = pSyncRegister->UnregisterSyncMgrHandler(CLSID_CscUpdateHandler, dwRegFlags);

    if (SUCCEEDED(hr))
        s_bSyncHandlerRegistered = bRegister;

exit_gracefully:

    DoRelease(pSyncRegister);

    if (SUCCEEDED(hrComInit))
        CoUninitialize();

    TraceLeaveResult(hr);
}


//*************************************************************
//
//  CscUpdateCache
//
//  Purpose:    Invoke SyncMgr to update the CSC cache
//
//  Parameters: pNamelist - list of files passed to the CSC SyncMgr handler
//
//
//  Return:     HRESULT
//
//*************************************************************
HRESULT
CscUpdateCache(DWORD dwUpdateFlags, CscFilenameList *pfnl)
{
    HRESULT hr;
    HRESULT hrComInit = E_FAIL;
    ISyncMgrSynchronizeInvoke *pSyncInvoke = NULL;
    DWORD dwSyncMgrFlags = 0;
    ULONG cbDataLength = sizeof(CSC_UPDATE_DATA);
    PCSC_UPDATE_DATA pUpdateData = NULL;
    PCSC_NAMELIST_HDR pNamelist = NULL;

    TraceEnter(TRACE_UPDATE, "CscUpdateCache");

    hrComInit = CoInitialize(NULL);
    hr = CoCreateInstance(CLSID_SyncMgr,
                          NULL,
                          CLSCTX_SERVER,
                          IID_ISyncMgrSynchronizeInvoke,
                          (LPVOID*)&pSyncInvoke);
    FailGracefully(hr, "Unable to create SyncMgr object");

    if (dwUpdateFlags & CSC_UPDATE_SELECTION)
    {
        if (NULL == pfnl || (0 == (CSC_UPDATE_SHOWUI_ALWAYS & dwUpdateFlags) && 0 == pfnl->GetShareCount()))
            ExitGracefully(hr, E_INVALIDARG, "CSC_UPDATE_SELECTION with no selection");

        pNamelist = pfnl->CreateListBuffer();
        if (!pNamelist)
            ExitGracefully(hr, E_OUTOFMEMORY, "Unable to create namelist buffer");

        cbDataLength += pNamelist->cbSize;
    }

    //
    // Alloc a buffer for the cookie data
    //
    pUpdateData = (PCSC_UPDATE_DATA)LocalAlloc(LPTR, cbDataLength);
    if (!pUpdateData)
        ExitGracefully(hr, E_OUTOFMEMORY, "LocalAlloc failed");

    pUpdateData->dwUpdateFlags = dwUpdateFlags;
    if (pNamelist)
    {
        pUpdateData->dwFileBufferOffset = sizeof(CSC_UPDATE_DATA);
        CopyMemory(ByteOffset(pUpdateData, pUpdateData->dwFileBufferOffset),
                   pNamelist,
                   pNamelist->cbSize);
    }

    if (dwUpdateFlags & CSC_UPDATE_STARTNOW)
        dwSyncMgrFlags |= SYNCMGRINVOKE_STARTSYNC;

    //
    // Start SyncMgr
    //
    hr = pSyncInvoke->UpdateItems(dwSyncMgrFlags,
                                  CLSID_CscUpdateHandler,
                                  cbDataLength,
                                  (LPBYTE)pUpdateData);

exit_gracefully:

    if (pNamelist)
        CscFilenameList::FreeListBuffer(pNamelist);

    if (pUpdateData)
        LocalFree(pUpdateData);

    DoRelease(pSyncInvoke);

    if (SUCCEEDED(hrComInit))
        CoUninitialize();

    TraceLeaveResult(hr);
}


//*************************************************************
//
//  GetNewVersionName
//
//  Purpose:    Create unique names for copies of a file
//
//  Parameters: LPTSTR pszUNCPath - fully qualified UNC name of file
//              LPTSTR pszShare - \\server\share that file lives on
//              LPTSTR pszDrive - drive mapping to use for net operations
//              LPTSTR *ppszNewName - filename for new version returned here (must free)
//
//  Return:     Win32 error code
//
//*************************************************************
DWORD
GetNewVersionName(LPCTSTR pszUNCPath,
                  LPCTSTR pszShare,
                  LPCTSTR pszDrive,
                  LPTSTR *ppszNewName)
{
    DWORD dwErr = NOERROR;
    LPTSTR pszDriveLetterPath = NULL;
    LPTSTR pszPath = NULL;
    LPTSTR pszFile = NULL;
    LPTSTR pszExt = NULL;
    LPTSTR pszWildCardName = NULL;
    TCHAR szUserName[MAX_USERNAME_CHARS];
    ULONG nLength;
    ULONG nMaxVersion = 0;
    ULONG cOlderVersions = 0;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA fd;
    LPTSTR pszT;

    TraceEnter(TRACE_UPDATE, "GetNewVersionName");
    TraceAssert(pszUNCPath != NULL);
    TraceAssert(ppszNewName != NULL);

    *ppszNewName = NULL;

    // 1. Split the path into components.
    // 2. Build wildcard name "X:\dir\foo (johndoe v*).txt"
    // 3. Do a findfirst/findnext loop to get the min & max version #
    //    and count the number of old versions.
    // 4. Increment the max version # and build the new filename as:
    //    "foo (johndoe v<max+1>).txt"

    // Assume that the UNC name contains more than the share
    TraceAssert(!StrCmpNI(pszUNCPath, pszShare, lstrlen(pszShare)));
    TraceAssert(lstrlen(pszUNCPath) > lstrlen(pszShare));

    // Copy the path (without \\server\share)
    if (!LocalAllocString(&pszPath, pszUNCPath + lstrlen(pszShare)))
        ExitGracefully(dwErr, ERROR_OUTOFMEMORY, "LocalAllocString failed");

    // Find the file part of the name
    pszT = PathFindFileName(pszPath);
    if (!pszT)
        ExitGracefully(dwErr, ERROR_INVALID_PARAMETER, "Incomplete path");

    // Copy the filename
    if (!LocalAllocString(&pszFile, pszT))
        ExitGracefully(dwErr, ERROR_OUTOFMEMORY, "LocalAllocString failed");

    // Look for the file extension
    pszT = PathFindExtension(pszFile);
    if (pszT)
    {
        // Copy the extension and truncate the file root at this point
        LocalAllocString(&pszExt, pszT);
        *pszT = TEXT('\0');
    }

    // Truncate the path
    PathRemoveFileSpec(pszPath);

    // Get the user name
    nLength = ARRAYSIZE(szUserName);
    if (!GetUserName(szUserName, &nLength))
        LoadString(g_hInstance, IDS_UNKNOWN_USER, szUserName, ARRAYSIZE(szUserName));

    // Build the wildcard path "X:\dir\foo (johndoe v*).txt"

    nLength = FormatStringID(&pszWildCardName, g_hInstance, IDS_VERSION_FORMAT, pszFile, szUserName, c_szStar, pszExt);
    if (!nLength)
        ExitGracefully(dwErr, GetLastError(), "Unable to format string");

    nLength += lstrlen(pszUNCPath) + lstrlen(szUserName);

    pszDriveLetterPath = (LPTSTR)LocalAlloc(LPTR, MAX(nLength, ULONG(MAX_PATH)) * sizeof(TCHAR));
    if (!pszDriveLetterPath)
        ExitGracefully(dwErr, ERROR_OUTOFMEMORY, "LocalAlloc failed");

    PathCombine(pszDriveLetterPath, pszDrive, pszPath);
    PathAppend(pszDriveLetterPath, pszWildCardName);
    nLength = (ULONG)(StrStr(pszWildCardName, c_szStar) - pszWildCardName); // remember where the '*' is

    // Search for existing versions of the file with this username
    hFind = FindFirstFile(pszDriveLetterPath, &fd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        ULONG nVersion;

        do
        {
            nVersion = StrToLong(&fd.cFileName[nLength]);

            if (nVersion > nMaxVersion)
            {
                nMaxVersion = nVersion;
            }

            cOlderVersions++;
        }
        while (FindNextFile(hFind, &fd));

        FindClose(hFind);
    }

    // Build the new file name to return to the caller.
    // This one is version nMaxVersion+1.
    ULongToString(nMaxVersion+1, pszDriveLetterPath, lstrlen(pszDriveLetterPath));
    nLength = FormatStringID(ppszNewName, g_hInstance, IDS_VERSION_FORMAT, pszFile, szUserName, pszDriveLetterPath, pszExt);
    if (!nLength)
        ExitGracefully(dwErr, GetLastError(), "Unable to format string");

exit_gracefully:

    LocalFreeString(&pszDriveLetterPath);
    LocalFreeString(&pszPath);
    LocalFreeString(&pszFile);
    LocalFreeString(&pszExt);
    LocalFreeString(&pszWildCardName);

    if (NOERROR != dwErr)
    {
        LocalFreeString(ppszNewName);
    }

    TraceLeaveValue(dwErr);
}


//*************************************************************
//
//  ConflictDlgCallback
//
//  Purpose:    Display local or remote file from conflict dialog
//
//  Parameters: hWnd - conflict dialog handle (used as parent for UI)
//              uMsg - one of RFCCM_*
//              wParam - depends on uMsg (unused)
//              lParam - pointer to context data (RFCDLGPARAM)
//
//
//  Return:     TRUE on success, FALSE otherwise
//
//*************************************************************

typedef struct _CONFLICT_DATA
{
    LPCTSTR pszShare;
    LPCTSTR pszDrive;
} CONFLICT_DATA;

BOOL
ConflictDlgCallback(HWND hWnd, UINT uMsg, WPARAM /*wParam*/, LPARAM lParam)
{
    RFCDLGPARAM *pdlgParam = (RFCDLGPARAM*)lParam;
    CONFLICT_DATA cd = {0};
    LPTSTR pszTmpName = NULL;
    ULONG cchShare = 0;
    LPTSTR szFile;
    DWORD dwErr = NOERROR;

    TraceEnter(TRACE_UPDATE, "ConflictDlgCallback");

    if (NULL == pdlgParam)
    {
        TraceAssert(FALSE);
        TraceLeaveValue(FALSE);
    }

    szFile = (LPTSTR)LocalAlloc(LMEM_FIXED,
                                MAX(StringByteSize(pdlgParam->pszLocation)
                                    + StringByteSize(pdlgParam->pszFilename), MAX_PATH_BYTES));
    if (!szFile)
        TraceLeaveValue(FALSE);

    if (pdlgParam->lCallerData)
        cd = *(CONFLICT_DATA*)pdlgParam->lCallerData;
    if (cd.pszShare)
        cchShare = lstrlen(cd.pszShare);

    switch (uMsg)
    {
    case RFCCM_VIEWLOCAL:
        // Build UNC path and view what's in the cache
        PathCombine(szFile, pdlgParam->pszLocation, pdlgParam->pszFilename);
        dwErr = OpenOfflineFile(szFile);
        break;

    case RFCCM_VIEWNETWORK:
        // Build drive letter (non-UNC) path and ShellExecute it
        PathCombine(szFile, cd.pszDrive, pdlgParam->pszLocation + cchShare);
        PathAppend(szFile, pdlgParam->pszFilename);
        {
            SHELLEXECUTEINFO si = {0};
            si.cbSize           = sizeof(si);
            si.fMask            = SEE_MASK_FLAG_NO_UI;
            si.hwnd             = hWnd;
            si.lpFile           = szFile;
            si.nShow            = SW_NORMAL;

            Trace((TEXT("ShellExecuting \"%s\""), szFile));
            if (!ShellExecuteEx(&si))
                dwErr = GetLastError();
        }
        break;
    }

    if (NOERROR != dwErr)
        CscWin32Message(hWnd, dwErr, CSCUI::SEV_ERROR);

    LocalFree(szFile);
    TraceLeaveValue(TRUE);
}

//*************************************************************
//
//  ShowConflictDialog
//
//  Purpose:    Invoke the conflict resolution dialog
//
//  Parameters: hWndParent - dialog parent window
//              pszUNCPath - full UNC of file that conflicts
//              pszNewName - filespec to use for new copy of file (e.g. "foo (johndoe v1).txt"
//              pszShare - "\\server\share"
//              pszDrive - "X:" drive mapping of remote connection
//              pfdLocal - Information about local file
//              pfdRemote - Information about remote file
//
//
//  Return:     HRESULT
//
//*************************************************************

typedef int (WINAPI *PFNSYNCMGRRESOLVECONFLICT)(HWND hWndParent, RFCDLGPARAM *pdlgParam);
TCHAR const c_szSyncMgrDll[]        = TEXT("mobsync.dll");
#ifdef UNICODE
CHAR  const c_szResolveConflict[]   = "SyncMgrResolveConflictW";
#else
CHAR  const c_szResolveConflict[]   = "SyncMgrResolveConflictA";
#endif

BOOL FileHasAssociation(LPCTSTR pszFile)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);
    if (pszFile)
    {
        pszFile = PathFindExtension(pszFile);
        if (pszFile && *pszFile)
        {
            IQueryAssociations *pAssoc = NULL;
            hr = AssocCreate(CLSID_QueryAssociations,
                             IID_IQueryAssociations,
                             (LPVOID*)&pAssoc);
            if (SUCCEEDED(hr))
            {
                hr = pAssoc->Init(ASSOCF_IGNOREBASECLASS, pszFile, NULL, NULL);
                pAssoc->Release();
            }
        }
    }
    return SUCCEEDED(hr);
}

int
ShowConflictDialog(HWND hWndParent,
                   LPCTSTR pszUNCPath,
                   LPCTSTR pszNewName,
                   LPCTSTR pszShare,
                   LPCTSTR pszDrive,
                   LPWIN32_FIND_DATA pfdLocal,
                   LPWIN32_FIND_DATA pfdRemote)
{
    int nResult = 0;
    TCHAR szUser[MAX_USERNAME_CHARS];
    LPTSTR pszPath = NULL;
    LPTSTR pszFile = NULL;
    TCHAR szRemoteDate[MAX_PATH];
    TCHAR szLocalDate[MAX_PATH];
    ULONG nLength;
    SYSTEMTIME st;
    RFCDLGPARAM dp = {0};
    CONFLICT_DATA cd;
    BOOL bLocalIsDir = FALSE;
    BOOL bRemoteIsDir = FALSE;

    static PFNSYNCMGRRESOLVECONFLICT pfnResolveConflict = NULL;

    TraceEnter(TRACE_UPDATE, "ShowConflictDialog");
    TraceAssert(pszUNCPath);

    if (NULL == pfnResolveConflict)
    {
        // The CSC Update handler is loaded by SyncMgr, so assume the SyncMgr
        // dll is already loaded.  We don't want to link to the LIB to keep
        // SyncMgr from loading every time our context menu or icon overlay
        // handler is loaded (for example).
        HMODULE hSyncMgrDll = GetModuleHandle(c_szSyncMgrDll);
        if (NULL != hSyncMgrDll)
            pfnResolveConflict = (PFNSYNCMGRRESOLVECONFLICT)GetProcAddress(hSyncMgrDll,
                                                                           c_szResolveConflict);
        if (NULL == pfnResolveConflict)
            return 0;
    }
    TraceAssert(NULL != pfnResolveConflict);

    szUser[0] = TEXT('\0');
    nLength = ARRAYSIZE(szUser);
    GetUserName(szUser, &nLength);

    szRemoteDate[0] = TEXT('\0');
    if (NULL != pfdRemote)
    {
        DWORD dwFlags = FDTF_DEFAULT;
        SHFormatDateTime(&pfdRemote->ftLastWriteTime, &dwFlags, szRemoteDate, ARRAYSIZE(szRemoteDate));

        if (pfdRemote->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            bRemoteIsDir = TRUE;
    }

    szLocalDate[0] = TEXT('\0');
    if (NULL != pfdLocal)
    {
        DWORD dwFlags = FDTF_DEFAULT;
        SHFormatDateTime(&pfdLocal->ftLastWriteTime, &dwFlags, szLocalDate, ARRAYSIZE(szLocalDate));

        if (pfdLocal->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            bLocalIsDir = TRUE;
    }

    if (!LocalAllocString(&pszPath, pszUNCPath))
        ExitGracefully(nResult, 0, "LocalAllocString failed");
    pszFile = PathFindFileName(pszUNCPath);
    PathRemoveFileSpec(pszPath);

    dp.dwFlags              = RFCF_APPLY_ALL;
    dp.pszFilename          = pszFile;
    dp.pszLocation          = pszPath;
    dp.pszNewName           = pszNewName;
    dp.pszNetworkModifiedBy = NULL;
    dp.pszLocalModifiedBy   = szUser;
    dp.pszNetworkModifiedOn = szRemoteDate;
    dp.pszLocalModifiedOn   = szLocalDate;
    dp.pfnCallBack          = NULL;
    dp.lCallerData          = 0;

    // Only turn on the View buttons (set a callback) if we're
    // dealing with files that have associations.
    if (!(bLocalIsDir || bRemoteIsDir) && FileHasAssociation(pszFile))
    {
        // Save both the share name and drive letter for building paths to view files
        cd.pszShare = pszShare;
        cd.pszDrive = pszDrive;

        dp.pfnCallBack      = ConflictDlgCallback;
        dp.lCallerData      = (LPARAM)&cd;
    }

    nResult = (*pfnResolveConflict)(hWndParent, &dp);

exit_gracefully:

    LocalFreeString(&pszPath);
    // No need to free pszFile

    TraceLeaveValue(nResult);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// SyncMgr integration implementation                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CCscUpdate::CCscUpdate() : m_cRef(1), m_ShareLog(HKEY_CURRENT_USER, c_szCSCShareKey),
  m_pSyncMgrCB(NULL), m_hSyncThreads(NULL),
  m_pFileList(NULL), m_hSyncItems(NULL), m_hwndDlgParent(NULL),
  m_hSyncInProgMutex(NULL), m_pConflictPinList(NULL),
  m_pSilentFolderList(NULL), m_pSpecialFolderList(NULL)
{
    DllAddRef();
    InitializeCriticalSection(&m_csThreadList);
    if (!g_hCscIcon)
        g_hCscIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_CSCUI_ICON));
    m_hSyncMutex = CreateMutex(NULL, FALSE, c_szSyncMutex);
}


CCscUpdate::~CCscUpdate()
{
    TraceEnter(TRACE_UPDATE, "CCscUpdate::~CCscUpdate");

    SyncCompleted();
    TraceAssert(NULL == m_hSyncInProgMutex);

    // We should never get here while a sync thread is still running
    TraceAssert(NULL == m_hSyncThreads || 0 == DPA_GetPtrCount(m_hSyncThreads));
    if (NULL != m_hSyncThreads)
        DPA_Destroy(m_hSyncThreads);
    DeleteCriticalSection(&m_csThreadList);

    if (NULL != m_hSyncItems)
        DSA_Destroy(m_hSyncItems);

    DoRelease(m_pSyncMgrCB);

    delete m_pFileList;
    delete m_pConflictPinList;
    delete m_pSilentFolderList;
    delete m_pSpecialFolderList;

    if (NULL != m_hSyncMutex)
        CloseHandle(m_hSyncMutex);

    DllRelease();
    TraceLeaveVoid();
}


HRESULT WINAPI
CCscUpdate::CreateInstance(REFIID riid, LPVOID *ppv)
{
    HRESULT hr;
    CCscUpdate *pThis;

    TraceEnter(TRACE_UPDATE, "CCscUpdate::CreateInstance");
    TraceAssert(IsCSCEnabled());

    pThis = new CCscUpdate;

    if (pThis)
    {
        hr = pThis->QueryInterface(riid, ppv);
        pThis->Release();
    }
    else
        hr = E_OUTOFMEMORY;

    TraceLeaveResult(hr);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// SyncMgr integration implementation (IUnknown)                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CCscUpdate::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(CCscUpdate, ISyncMgrSynchronize),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CCscUpdate::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CCscUpdate::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Sync Manager integration implementation (ISyncMgrSynchronize)             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CCscUpdate::Initialize(DWORD /*dwReserved*/,
                       DWORD dwSyncFlags,
                       DWORD cbCookie,
                       const BYTE *pCookie)
{
    HRESULT hr = S_OK;
    HKEY hkCSC;
    BOOL bNoNet = TRUE;

    TraceEnter(TRACE_UPDATE, "CCscUpdate::Initialize");
    TraceAssert(IsCSCEnabled());

    if (!(SYNCMGRFLAG_SETTINGS & dwSyncFlags) && ::IsSyncInProgress())
    {
        //
        // We need to guard against running multiple syncs at the 
        // same time.  User notification in the UI is handled where
        // the UI code calls CscUpdate().  This is so that the UI 
        // message contains the proper context with respect to what 
        // the user is doing.
        //
        TraceLeaveResult(E_FAIL);
    }

    m_dwSyncFlags = 0;
    delete m_pFileList;
    m_pFileList = NULL;
    delete m_pConflictPinList;
    m_pConflictPinList = NULL;

    // We used to get the tray status to check for NoNet, but
    // there's a timing problem at logon (the tray window may not
    // be created yet).  So ask RDR instead.  If this call fails,
    // then RDR must be dead, so bNoNet defaults to TRUE.
    CSCIsServerOffline(NULL, &bNoNet);

    switch (dwSyncFlags & SYNCMGRFLAG_EVENTMASK)
    {
    case SYNCMGRFLAG_CONNECT:               // Logon
        if (bNoNet)
            ExitGracefully(hr, E_FAIL, "No Logon sync when no net");
        m_dwSyncFlags = CSC_SYNC_OUT | CSC_SYNC_LOGON | CSC_SYNC_NOTIFY_SYSTRAY; // | CSC_SYNC_RECONNECT;
        break;

    case SYNCMGRFLAG_PENDINGDISCONNECT:     // Logoff
        if (bNoNet)
            ExitGracefully(hr, E_FAIL, "No Logoff sync when no net");
        m_dwSyncFlags = CSC_SYNC_LOGOFF;
        if (CConfig::eSyncFull == CConfig::GetSingleton().SyncAtLogoff())
            m_dwSyncFlags |= CSC_SYNC_OUT | CSC_SYNC_IN_FULL;
        else
            m_dwSyncFlags |= CSC_SYNC_IN_QUICK;
        break;

    case SYNCMGRFLAG_INVOKE:                // CscUpdateCache
        if (pCookie != NULL && cbCookie > 0)
        {
            PCSC_UPDATE_DATA pUpdateData = (PCSC_UPDATE_DATA)pCookie;

            TraceAssert(cbCookie >= sizeof(CSC_UPDATE_DATA));

            DWORD dwUpdateFlags = pUpdateData->dwUpdateFlags;

            if (dwUpdateFlags & CSC_UPDATE_SELECTION)
            {
                TraceAssert(cbCookie > sizeof(CSC_UPDATE_DATA));

                // Create the filelist from the selection provided
                m_pFileList = new CscFilenameList((PCSC_NAMELIST_HDR)ByteOffset(pUpdateData, pUpdateData->dwFileBufferOffset),
                                                  true);

                if (!m_pFileList)
                    ExitGracefully(hr, E_OUTOFMEMORY, "Unable to create CscFilenameList object");

                if (!m_pFileList->IsValid())
                    ExitGracefully(hr, E_FAIL, "Unable to initialize CscFilenameList object");

                if (CSC_UPDATE_SHOWUI_ALWAYS & dwUpdateFlags)
                {
                    m_dwSyncFlags |= CSC_SYNC_SHOWUI_ALWAYS;
                }
                else if (0 == m_pFileList->GetShareCount())
                    ExitGracefully(hr, E_UNEXPECTED, "CSC_UPDATE_SELECTION with no selection");
            }

            if (dwUpdateFlags & CSC_UPDATE_RECONNECT)
            {
                m_dwSyncFlags |= CSC_SYNC_RECONNECT;
            }

            if (dwUpdateFlags & CSC_UPDATE_NOTIFY_DONE)
            {
                //
                // Caller of CscUpdateCache want's systray notification
                // when sync is complete.
                //
                m_dwSyncFlags |= CSC_SYNC_NOTIFY_SYSTRAY;
            }

            if (dwUpdateFlags & CSC_UPDATE_FILL_ALL)
                m_dwSyncFlags |= CSC_SYNC_IN_FULL;
            else if (dwUpdateFlags & CSC_UPDATE_FILL_QUICK)
                m_dwSyncFlags |= CSC_SYNC_IN_QUICK;

            if (dwUpdateFlags & CSC_UPDATE_REINT)
                m_dwSyncFlags |= CSC_SYNC_OUT;

            if (dwUpdateFlags & CSC_UPDATE_PIN_RECURSE)
                m_dwSyncFlags |= CSC_SYNC_PINFILES | CSC_SYNC_PIN_RECURSE | CSC_SYNC_IN_QUICK;
            else if (dwUpdateFlags & CSC_UPDATE_PINFILES)
                m_dwSyncFlags |= CSC_SYNC_PINFILES | CSC_SYNC_IN_QUICK;

            if (dwUpdateFlags & CSC_UPDATE_IGNORE_ACCESS)
                m_dwSyncFlags |= CSC_SYNC_IGNORE_ACCESS;
        }
        break;

    case SYNCMGRFLAG_IDLE:                  // Auto-sync at idle time
        if (bNoNet)
            ExitGracefully(hr, E_FAIL, "No idle sync when no net");
        m_dwSyncFlags = CSC_SYNC_OUT | CSC_SYNC_IN_QUICK | CSC_SYNC_IDLE | CSC_SYNC_NOTIFY_SYSTRAY;
        break;

    case SYNCMGRFLAG_MANUAL:                // Run "mobsync.exe"
        m_dwSyncFlags = CSC_SYNC_OUT | CSC_SYNC_IN_FULL | CSC_SYNC_NOTIFY_SYSTRAY | CSC_SYNC_RECONNECT;
        break;

    case SYNCMGRFLAG_SCHEDULED:             // User scheduled sync
        m_dwSyncFlags = CSC_SYNC_OUT | CSC_SYNC_IN_FULL | CSC_SYNC_NOTIFY_SYSTRAY;
        break;
    }

    if (!(m_dwSyncFlags & CSC_SYNC_PINFILES))
        m_dwSyncFlags |= CSC_SYNC_SKIP_EFS; // skip EFS if not pinning

    if (dwSyncFlags & SYNCMGRFLAG_SETTINGS)
        m_dwSyncFlags |= CSC_SYNC_SETTINGS;

    if (!m_dwSyncFlags)
        ExitGracefully(hr, E_UNEXPECTED, "Nothing to do");

    if (dwSyncFlags & SYNCMGRFLAG_MAYBOTHERUSER)
        m_dwSyncFlags |= CSC_SYNC_MAYBOTHERUSER;

    if (bNoNet)
        m_dwSyncFlags |= CSC_SYNC_NONET;

    GetSilentFolderList();

exit_gracefully:

    TraceLeaveResult(hr);
}


STDMETHODIMP
CCscUpdate::GetHandlerInfo(LPSYNCMGRHANDLERINFO *ppSyncMgrHandlerInfo)
{
    HRESULT hr = S_OK;
    LPSYNCMGRHANDLERINFO pHandlerInfo;

    TraceEnter(TRACE_UPDATE, "CCscUpdate::GetHandlerInfo");

    if (NULL == ppSyncMgrHandlerInfo)
        TraceLeaveResult(E_INVALIDARG);

    *ppSyncMgrHandlerInfo = NULL;

    pHandlerInfo = (LPSYNCMGRHANDLERINFO)CoTaskMemAlloc(sizeof(SYNCMGRHANDLERINFO));
    if (NULL == pHandlerInfo)
        ExitGracefully(hr, E_OUTOFMEMORY, "LocalAlloc failed");

    pHandlerInfo->cbSize = sizeof(SYNCMGRHANDLERINFO);
    pHandlerInfo->hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_CSCUI_ICON));
    pHandlerInfo->SyncMgrHandlerFlags = SYNCMGRHANDLER_HASPROPERTIES | SYNCMGRHANDLER_MAYESTABLISHCONNECTION;
    LoadStringW(g_hInstance,
                IDS_APPLICATION,
                pHandlerInfo->wszHandlerName,
                ARRAYSIZE(pHandlerInfo->wszHandlerName));

    *ppSyncMgrHandlerInfo = pHandlerInfo;

exit_gracefully:

    TraceLeaveResult(hr);
}


STDMETHODIMP
CCscUpdate::EnumSyncMgrItems(LPSYNCMGRENUMITEMS *ppenum)
{
    HRESULT hr;
    PUPDATEENUM pNewEnum;

    TraceEnter(TRACE_UPDATE, "CCscUpdate::EnumSyncMgrItems");

    *ppenum = NULL;

    pNewEnum = new CUpdateEnumerator(this);
    if (pNewEnum)
    {
        hr = pNewEnum->QueryInterface(IID_ISyncMgrEnumItems, (LPVOID*)ppenum);
        pNewEnum->Release();
    }
    else
        hr = E_OUTOFMEMORY;

    TraceLeaveResult(hr);
}


STDMETHODIMP
CCscUpdate::GetItemObject(REFSYNCMGRITEMID /*rItemID*/, REFIID /*riid*/, LPVOID * /*ppv*/)
{
    return E_NOTIMPL;
}


STDMETHODIMP
CCscUpdate::ShowProperties(HWND hWndParent, REFSYNCMGRITEMID rItemID)
{
    CSCEntry *pShareEntry;
    LPCTSTR pszShareName = TEXT("");

    pShareEntry = m_ShareLog.Get(rItemID);

    // We don't enumerate shares to SyncMgr unless a share entry
    // exists in the registry, so m_ShareLog.Get should never fail here.
    if (pShareEntry)
        pszShareName = pShareEntry->Name();

    COfflineFilesFolder::Open();

        // Notify SyncMgr that the ShowProperties is done.
    if (NULL != m_pSyncMgrCB)
        m_pSyncMgrCB->ShowPropertiesCompleted(S_OK);

    return S_OK;
}


STDMETHODIMP
CCscUpdate::SetProgressCallback(LPSYNCMGRSYNCHRONIZECALLBACK pCallback)
{
    TraceEnter(TRACE_UPDATE, "CCscUpdate::SetProgressCallback");

    DoRelease(m_pSyncMgrCB);

    m_pSyncMgrCB = pCallback;

    if (m_pSyncMgrCB)
        m_pSyncMgrCB->AddRef();

    TraceLeaveResult(S_OK);
}


STDMETHODIMP
CCscUpdate::PrepareForSync(ULONG cNumItems,
                           SYNCMGRITEMID *pItemID,
                           HWND /*hWndParent*/,
                           DWORD /*dwReserved*/)
{
    HRESULT hr = S_OK;

    TraceEnter(TRACE_UPDATE, "CCscUpdate::PrepareForSync");
    TraceAssert(0 != cNumItems);
    TraceAssert(NULL != pItemID);

    //
    // Copy the list of item ID's
    //
    if (NULL == m_hSyncItems)
    {
        m_hSyncItems = DSA_Create(sizeof(SYNCMGRITEMID), 4);
        if (NULL == m_hSyncItems)
            ExitGracefully(hr, E_OUTOFMEMORY, "Unable to create DSA for SYNCMGRITEMID list");
    }
    else
        DSA_DeleteAllItems(m_hSyncItems);

    while (cNumItems--)
        DSA_AppendItem(m_hSyncItems, pItemID++);

exit_gracefully:

    // ISyncMgrSynchronize::PrepareForSync is now an asynchronous call
    // so we could create another thread to do the work and return from
    // this call immediately.  However, since all we do is copy the list
    // of Item IDs, let's do it here and call
    // m_pSyncMgrCB->PrepareForSyncCompleted before returning.

    if (NULL != m_pSyncMgrCB)
        m_pSyncMgrCB->PrepareForSyncCompleted(hr);

    TraceLeaveResult(hr);
}



STDMETHODIMP
CCscUpdate::Synchronize(HWND hWndParent)
{
    HRESULT hr = E_FAIL;
    ULONG cItems = 0;
    BOOL bConnectionEstablished = FALSE;

    TraceEnter(TRACE_UPDATE, "CCscUpdate::Synchronize");

    if (NULL != m_hSyncItems)
        cItems = DSA_GetItemCount(m_hSyncItems);

    //
    // Don't want systray UI updates while syncing.
    // Whenever the systray UI is updated, the code checks first
    // for this global mutex object.  If it's non-signaled, the
    // systray knows there's a sync in progress and the UI isn't
    // updated.
    //
    TraceAssert(NULL == m_hSyncInProgMutex);
    m_hSyncInProgMutex = CreateMutex(NULL, TRUE, c_szSyncInProgMutex);

    if (0 == cItems)
    {
        ExitGracefully(hr, E_UNEXPECTED, "Nothing to synchronize");
    }
    else if (1 == cItems)
    {
        SYNCMGRITEMID *pItemID = (SYNCMGRITEMID*)DSA_GetItemPtr(m_hSyncItems, 0);
        if (NULL != pItemID && IsEqualGUID(GUID_CscNullSyncItem, *pItemID))
        {
            //
            // A single item in the DSA and it's our "null sync" GUID.
            // This means we really have nothing to sync but the invoker 
            // of the sync wants to see some SyncMgr progress UI.  In 
            // this scenario the update item enumerator already enumerated
            // the "null sync" item.  Here we set this single item's progress
            // UI info to 100% complete and skip any sync activity.
            //
            SYNCMGRPROGRESSITEM spi = {0};
            spi.mask = SYNCMGRPROGRESSITEM_STATUSTYPE |
                       SYNCMGRPROGRESSITEM_STATUSTEXT |
                       SYNCMGRPROGRESSITEM_PROGVALUE | 
                       SYNCMGRPROGRESSITEM_MAXVALUE;

            spi.cbSize        = sizeof(spi);
            spi.dwStatusType  = SYNCMGRSTATUS_SUCCEEDED;
            spi.lpcStatusText = L" ";
            spi.iProgValue    = 1;
            spi.iMaxValue     = 1;
            m_pSyncMgrCB->Progress(GUID_CscNullSyncItem, &spi);
            m_pSyncMgrCB->SynchronizeCompleted(S_OK);

            ExitGracefully(hr, NOERROR, "Nothing to sync.  Progress UI displayed");
        }
    }

    m_hwndDlgParent = hWndParent;

    // We can pin autocached files without a net (no sync required);
    // otherwise we need to establish a RAS connection to do anything.
    if ((m_dwSyncFlags & CSC_SYNC_NONET) && !(m_dwSyncFlags & CSC_SYNC_PINFILES))
    {
        hr = m_pSyncMgrCB->EstablishConnection(NULL, 0);
        FailGracefully(hr, "Unable to establish RAS connection");

        bConnectionEstablished = TRUE;
    }

    // For each share, kick off a thread to do the work
    while (cItems > 0)
    {
        SYNCMGRITEMID *pItemID;
        CSCEntry *pShareEntry;

        --cItems;
        pItemID = (SYNCMGRITEMID*)DSA_GetItemPtr(m_hSyncItems, cItems);

        pShareEntry = m_ShareLog.Get(*pItemID);

        // We don't enumerate shares to SyncMgr unless a share entry
        // exists in the registry, so m_ShareLog.Get should never fail here.
        if (NULL == pShareEntry)
            ExitGracefully(hr, E_UNEXPECTED, "No share entry");

        hr = SynchronizeShare(pItemID, pShareEntry->Name(), bConnectionEstablished);
        DSA_DeleteItem(m_hSyncItems, cItems);
        FailGracefully(hr, "Unable to create sync thread");
    }

    TraceAssert(0 == DSA_GetItemCount(m_hSyncItems));

exit_gracefully:

    if (FAILED(hr))
        SetItemStatus(GUID_NULL, SYNCMGRSTATUS_STOPPED);

    TraceLeaveResult(hr);
}


STDMETHODIMP
CCscUpdate::SetItemStatus(REFSYNCMGRITEMID rItemID,
                          DWORD dwSyncMgrStatus)
{
    HRESULT hr = E_FAIL;
    ULONG cItems;
    BOOL bAllItems;

    TraceEnter(TRACE_UPDATE, "CCscUpdate::SetItemStatus");

    if (SYNCMGRSTATUS_SKIPPED != dwSyncMgrStatus && SYNCMGRSTATUS_STOPPED != dwSyncMgrStatus)
        TraceLeaveResult(E_NOTIMPL);

    bAllItems = FALSE;
    if (SYNCMGRSTATUS_STOPPED == dwSyncMgrStatus)
    {
        bAllItems = TRUE;
        m_dwSyncFlags |= CSC_SYNC_CANCELLED;
    }

    // SetItemStatus can be called between PrepareForSync and Synchronize, in
    // in which case the correct thing to do is remove the item from m_hSyncItems.
    if (NULL != m_hSyncItems)
    {
        cItems = DSA_GetItemCount(m_hSyncItems);

        while (cItems > 0)
        {
            SYNCMGRITEMID *pItemID;

            --cItems;
            pItemID = (SYNCMGRITEMID*)DSA_GetItemPtr(m_hSyncItems, cItems);

            if (bAllItems || IsEqualGUID(rItemID, *pItemID))
            {
                // Remove the item from the list of items to sync
                DSA_DeleteItem(m_hSyncItems, cItems);
                if (!bAllItems)
                    ExitGracefully(hr, S_OK, "Skipping item");
            }
        }
    }

    // Lookup the thread for the item ID and set its status
    // to cause it to terminate.
    hr = SetSyncThreadStatus(SyncStop, bAllItems ? GUID_NULL : rItemID);

exit_gracefully:

    TraceLeaveResult(hr);
}


STDMETHODIMP
CCscUpdate::ShowError(HWND /*hWndParent*/ , REFSYNCMGRERRORID /*ErrorID*/)
{
    return E_NOTIMPL;
}


HRESULT
CCscUpdate::SynchronizeShare(SYNCMGRITEMID *pItemID, LPCTSTR pszShareName, BOOL bRasConnected)
{
    HRESULT hr = S_OK;
    DWORD dwThreadID;
    PSYNCTHREADDATA pThreadData;
    ULONG cbShareName = 0;

    TraceEnter(TRACE_UPDATE, "CCscUpdate::SynchronizeShare");
    TraceAssert(NULL != pItemID);
    TraceAssert(NULL != pszShareName);
    TraceAssert(*pszShareName);

    EnterCriticalSection(&m_csThreadList);
    if (NULL == m_hSyncThreads)
        m_hSyncThreads = DPA_Create(4);
    LeaveCriticalSection(&m_csThreadList);

    if (NULL == m_hSyncThreads)
        ExitGracefully(hr, E_OUTOFMEMORY, "Unable to create DPA for threads");

    cbShareName = StringByteSize(pszShareName);
    pThreadData = (PSYNCTHREADDATA)LocalAlloc(LPTR, sizeof(SYNCTHREADDATA) + cbShareName);

    if (!pThreadData)
        ExitGracefully(hr, E_OUTOFMEMORY, "LocalAlloc failed");

    pThreadData->pThis = this;
    pThreadData->ItemID = *pItemID;
    pThreadData->pszShareName = (LPTSTR)(pThreadData + 1);
    CopyMemory(pThreadData->pszShareName, pszShareName, cbShareName);

    //
    // If we established a RAS connection, then it will go away
    // right after the sync completes, so there's no point trying
    // to reconnect.  That is, only check CSC_SYNC_RECONNECT and
    // add the share to the reconnect list if we aren't doing RAS.
    //
    if (bRasConnected)
    {
        pThreadData->dwSyncStatus |= SDS_SYNC_RAS_CONNECTED;
    }
    else if (m_dwSyncFlags & CSC_SYNC_RECONNECT)
    {
        CscFilenameList::HSHARE hShare;
        m_ReconnectList.AddShare(pszShareName, &hShare);
    }

    pThreadData->hThread = CreateThread(NULL,
                                        0,
                                        _SyncThread,
                                        pThreadData,
                                        CREATE_SUSPENDED,
                                        &dwThreadID);

    if (NULL != pThreadData->hThread)
    {
        EnterCriticalSection(&m_csThreadList);
        DPA_AppendPtr(m_hSyncThreads, pThreadData);
        LeaveCriticalSection(&m_csThreadList);

        ResumeThread(pThreadData->hThread);
    }
    else
    {
        DWORD dwErr = GetLastError();

        LocalFree(pThreadData);

        LPTSTR pszErr = GetErrorText(GetLastError());
        LogError(*pItemID,
                 SYNCMGRLOGLEVEL_ERROR,
                 IDS_FILL_SPARSE_FILES_ERROR,
                 pszShareName,
                 pszErr);
        LocalFreeString(&pszErr);
        hr = HRESULT_FROM_WIN32(dwErr);
    }

exit_gracefully:

    TraceLeaveResult(hr);
}


void
CCscUpdate::SetLastSyncTime(LPCTSTR pszShareName)
{
    HKEY hKey = NULL;

    hKey = m_ShareLog.OpenKey(pszShareName, KEY_SET_VALUE);
    if (hKey)
    {
        FILETIME ft = {0};
        GetSystemTimeAsFileTime(&ft);
        RegSetValueEx(hKey, c_szLastSync, 0, REG_BINARY, (LPBYTE)&ft, sizeof(ft));
        RegCloseKey(hKey);
    }
}


DWORD
CCscUpdate::GetLastSyncTime(LPCTSTR pszShareName, LPFILETIME pft)
{
    DWORD dwResult = ERROR_PATH_NOT_FOUND;
    HKEY hKey = NULL;

    hKey = m_ShareLog.OpenKey(pszShareName, KEY_QUERY_VALUE);
    if (hKey)
    {
        DWORD dwSize = sizeof(*pft);
        dwResult = RegQueryValueEx(hKey, c_szLastSync, NULL, NULL, (LPBYTE)pft, &dwSize);
        RegCloseKey(hKey);
    }
    return dwResult;
}


void
CCscUpdate::SyncThreadCompleted(PSYNCTHREADDATA pSyncData)
{
    int iThread;

    TraceEnter(TRACE_UPDATE, "CCscUpdate::SyncThreadCompleted");
    TraceAssert(NULL != pSyncData);
    TraceAssert(NULL != m_hSyncThreads);

    EnterCriticalSection(&m_csThreadList);

    iThread = DPA_GetPtrIndex(m_hSyncThreads, pSyncData);
    TraceAssert(-1 != iThread);

    DPA_DeletePtr(m_hSyncThreads, iThread);
    CloseHandle(pSyncData->hThread);
    pSyncData->hThread = NULL;

    iThread = DPA_GetPtrCount(m_hSyncThreads);

    LeaveCriticalSection(&m_csThreadList);

    if (0 == iThread)
    {
        SyncCompleted();
    }

    TraceLeaveVoid();
}


void
CCscUpdate::SyncCompleted(void)
{
    if ((m_dwSyncFlags & CSC_SYNC_RECONNECT) &&
        !(m_dwSyncFlags & CSC_SYNC_CANCELLED))
    {
        m_dwSyncFlags &= ~CSC_SYNC_RECONNECT;
        ReconnectServers(&m_ReconnectList, FALSE, FALSE);
    }

    if (NULL != m_hSyncInProgMutex)
    {
        // We're not syncing so reset the global event
        ReleaseMutex(m_hSyncInProgMutex);
        CloseHandle(m_hSyncInProgMutex);
        m_hSyncInProgMutex = NULL;
    }

    if (m_dwSyncFlags & CSC_SYNC_NOTIFY_SYSTRAY)
    {
        // Notify systray that we're done
        PostToSystray(CSCWM_DONESYNCING, 0, 0);
        m_dwSyncFlags &= ~CSC_SYNC_NOTIFY_SYSTRAY;
    }

    // Notify SyncMgr that the sync is done
    if (NULL != m_pSyncMgrCB)
    {
        m_pSyncMgrCB->SynchronizeCompleted(S_OK);
    }
}


UINT
GetErrorFormat(DWORD dwErr, BOOL bMerging = FALSE)
{
    UINT idString = 0;

    // BUGBUG these are all just initial guesses.  Not sure
    // which error codes we'll get from CSC.

    switch (dwErr)
    {
    case ERROR_DISK_FULL:
        // "The server disk is full."
        // "The local disk is full."
        idString = bMerging ? IDS_SERVER_FULL_ERROR : IDS_LOCAL_DISK_FULL_ERROR;
        break;

    case ERROR_LOCK_VIOLATION:
    case ERROR_SHARING_VIOLATION:
    case ERROR_OPEN_FILES:
    case ERROR_ACTIVE_CONNECTIONS:
    case ERROR_DEVICE_IN_USE:
        // "'%1' is in use on %2"
        idString = IDS_FILE_OPEN_ERROR;
        break;

    case ERROR_BAD_NETPATH:
    case ERROR_DEV_NOT_EXIST:
    case ERROR_NETNAME_DELETED:
    case ERROR_BAD_NET_NAME:
    case ERROR_SHARING_PAUSED:
    case ERROR_REQ_NOT_ACCEP:
    case ERROR_REDIR_PAUSED:
    case ERROR_BAD_DEVICE:
    case ERROR_CONNECTION_UNAVAIL:
    case ERROR_NO_NET_OR_BAD_PATH:
    case ERROR_NO_NETWORK:
    case ERROR_CONNECTION_REFUSED:
    case ERROR_GRACEFUL_DISCONNECT:
    case ERROR_NETWORK_UNREACHABLE:
    case ERROR_HOST_UNREACHABLE:
    case ERROR_PROTOCOL_UNREACHABLE:
    case ERROR_PORT_UNREACHABLE:
    case ERROR_LOGON_FAILURE:
        // "Unable to connect to '%1.'  %2"
        idString = IDS_SHARE_CONNECT_ERROR;
        break;

    case ERROR_OPEN_FAILED:
    case ERROR_UNEXP_NET_ERR:
    case ERROR_NETWORK_BUSY:
    case ERROR_BAD_NET_RESP:
        // "Unable to access '%1' on %2.  %3"
        idString = IDS_NET_ERROR;
        break;

    case ERROR_ACCESS_DENIED:
    case ERROR_NETWORK_ACCESS_DENIED:
        // "Access to '%1' is denied on %2"
        idString = IDS_ACCESS_ERROR;
        break;

    case ERROR_BAD_FORMAT:
        // "The Offline Files cache is corrupt.  Restart the computer to correct the cache."
        idString = IDS_CACHE_CORRUPT;
        break;

    default:
        // "Error accessing '%1' on %2.  %3"
        idString = IDS_UNKNOWN_SYNC_ERROR;
        break;
    }

    return idString;
}


HRESULT
CCscUpdate::LogError(REFSYNCMGRITEMID rItemID,
                     LPCTSTR pszText,
                     DWORD dwLogLevel,
                     REFSYNCMGRERRORID ErrorID)
{
    HRESULT hr;
    SYNCMGRLOGERRORINFO slei;

    USES_CONVERSION;

    TraceEnter(TRACE_UPDATE, "CCscUpdate::LogError");

    if (NULL == m_pSyncMgrCB)
        TraceLeaveResult(E_UNEXPECTED);

    slei.cbSize = sizeof(slei);
    slei.mask   = SYNCMGRLOGERROR_ITEMID | SYNCMGRLOGERROR_ERRORID;
    slei.ItemID = rItemID;
    slei.ErrorID = ErrorID;

    // if we have a jumptext associated with this item then
    // set the enable jumptext flag
    if (ErrorID != GUID_NULL)
    {
        slei.mask |= SYNCMGRLOGERROR_ERRORFLAGS;
        slei.dwSyncMgrErrorFlags = SYNCMGRERRORFLAG_ENABLEJUMPTEXT;
    }

    Trace((pszText));
    hr = m_pSyncMgrCB->LogError(dwLogLevel, T2CW(pszText), &slei);

    TraceLeaveResult(hr);
}

DWORD
CCscUpdate::LogError(REFSYNCMGRITEMID rItemID,
                     DWORD dwLogLevel,
                     UINT nFormatID,
                     ...)
{
    LPTSTR pszError = NULL;
    va_list args;
    va_start(args, nFormatID);
    if (vFormatStringID(&pszError, g_hInstance, nFormatID, &args))
    {
        LogError(rItemID, pszError, dwLogLevel);
        LocalFree(pszError);
    }
    va_end(args);
    return 0;
}


DWORD
CCscUpdate::LogError(REFSYNCMGRITEMID rItemID,
                     UINT nFormatID,
                     LPCTSTR pszName,
                     DWORD dwErr,
                     DWORD dwLogLevel)
{
    //
    // Break the filename into "file" and "path" components
    //
    TCHAR szPath[MAX_PATH] = TEXT("\\");
    LPCTSTR pszFile = NULL;
    if (pszName)
    {
        pszFile = PathFindFileName(pszName);
        lstrcpyn(szPath, pszName, min(ARRAYSIZE(szPath),(int)(pszFile-pszName)));
    }

    //
    // Get the system error text and format the error
    //
    LPTSTR pszErr = GetErrorText(dwErr);
    LogError(rItemID,
             dwLogLevel,
             nFormatID,
             pszFile,
             szPath,
             pszErr);
    LocalFreeString(&pszErr);

    return 0;
}


BOOL
MakeDriveLetterPath(LPCTSTR pszUNC,
                    LPCTSTR pszShare,
                    LPCTSTR pszDrive,
                    LPTSTR *ppszResult)
{
    BOOL bResult = FALSE;
    ULONG cchShare;

    if (!pszUNC || !pszShare || !ppszResult)
        return FALSE;

    *ppszResult = NULL;

    cchShare = lstrlen(pszShare);

    // If the path is on the share, use the drive letter instead
    if (pszDrive && *pszDrive &&
        CSTR_EQUAL == CompareString(LOCALE_USER_DEFAULT,
                                    NORM_IGNORECASE,
                                    pszUNC,
                                    cchShare,
                                    pszShare,
                                    cchShare))
    {
        *ppszResult = (LPTSTR)LocalAlloc(LPTR, MAX(StringByteSize(pszUNC), MAX_PATH_BYTES));
        if (*ppszResult)
        {
            PathCombine(*ppszResult, pszDrive, pszUNC + cchShare);
            bResult = TRUE;
        }
    }
    return bResult;
}


DWORD
CCscUpdate::CopyLocalFileWithDriveMapping(LPCTSTR pszSrc,
                                          LPCTSTR pszDst,
                                          LPCTSTR pszShare,
                                          LPCTSTR pszDrive,
                                          BOOL    bDirectory)
{
    DWORD dwErr = NOERROR;
    LPTSTR szDst = NULL;

    if (!pszSrc || !pszDst || !pszShare)
        return ERROR_INVALID_PARAMETER;

    // If the destination is on the share, use the drive letter instead
    if (MakeDriveLetterPath(pszDst, pszShare, pszDrive, &szDst))
        pszDst = szDst;

    if (bDirectory)
    {
        // We don't need to copy the directory contents here, just create
        // the tree structure on the server.
        if (!CreateDirectory(pszDst, NULL))
        {
            dwErr = GetLastError();
            if (ERROR_ALREADY_EXISTS == dwErr)
                dwErr = NOERROR;
        }
    }
    else
    {
        LPTSTR pszTmpName = NULL;

        if (!CSCCopyReplica(pszSrc, &pszTmpName) ||
            !MoveFileEx(pszTmpName,
                        pszDst,
                        MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        {
            dwErr = GetLastError();
        }

        if (NULL != pszTmpName)
        {
            DeleteFile(pszTmpName);
            LocalFree(pszTmpName);
        }
    }

    if (ERROR_PATH_NOT_FOUND == dwErr)
    {
        // The parent directory doesn't exist, create it now.
        TCHAR szParent[MAX_PATH];
        lstrcpyn(szParent, pszDst, ARRAYSIZE(szParent));
        PathRemoveFileSpec(szParent);
        dwErr = CopyLocalFileWithDriveMapping(pszSrc, szParent, pszShare, NULL, TRUE);

        // If that worked, retry the original operation.
        if (NOERROR == dwErr)
            dwErr = CopyLocalFileWithDriveMapping(pszSrc, pszDst, pszShare, NULL, bDirectory);
    }

    LocalFreeString(&szDst);

    return dwErr;
}


BOOL
HandleConflictLocally(PSYNCTHREADDATA   pSyncData,
                      LPCTSTR           pszPath,
                      DWORD             dwCscStatus,
                      DWORD             dwLocalAttr,
                      DWORD             dwRemoteAttr = 0)
{
    BOOL bResult = FALSE;
    LPTSTR szParent = NULL;

    // If it's super-hidden or not modified locally, we can always
    // handle the conflict locally.
    if (!(dwCscStatus & CSC_LOCALLY_MODIFIED) || IsHiddenSystem(dwLocalAttr) || IsHiddenSystem(dwRemoteAttr))
        return TRUE;

    // If we're dealing with 2 folders, the worst that happens is that the
    // underlying files/folders get merged.
    if ((FILE_ATTRIBUTE_DIRECTORY & dwLocalAttr) && (FILE_ATTRIBUTE_DIRECTORY & dwRemoteAttr))
        return TRUE;

    //
    // Next, check whether the parent path is super-hidden.
    //
    // For example, recycle bin makes super-hidden folders and puts
    // metadata files in them.
    //
    // Do this on the server, since CSC has exclusive access to the database
    // while merging, causing GetFileAttributes to fail with Access Denied.
    //
    if (MakeDriveLetterPath(pszPath, pSyncData->pszShareName, pSyncData->szDrive, &szParent))
    {
        do
        {
            PathRemoveFileSpec(szParent);
            dwRemoteAttr = GetFileAttributes(szParent);

            if ((DWORD)-1 == dwRemoteAttr)
            {
                // Path doesn't exist, access denied, etc.
                break;
            }

            if (IsHiddenSystem(dwRemoteAttr))
            {
                bResult = TRUE;
                break;
            }
        }
        while (!PathIsRoot(szParent));
    }

    LocalFreeString(&szParent);

    return bResult;
}

DWORD
CCscUpdate::HandleFileConflict(PSYNCTHREADDATA     pSyncData,
                               LPCTSTR             pszName,
                               DWORD               dwStatus,
                               DWORD               dwHintFlags,
                               LPWIN32_FIND_DATA   pFind32)
{
    DWORD dwResult = CSCPROC_RETURN_CONTINUE;
    DWORD dwErr = NOERROR;
    int nErrorResolution = RFC_KEEPBOTH;
    LPTSTR pszNewName = NULL;
    LPTSTR szFullPath = NULL;
    BOOL bApplyToAll = FALSE;

    TraceEnter(TRACE_UPDATE, "CCscUpdate::HandleFileConflict");
    Trace((TEXT("File conflict: %s"), pszName));
    TraceAssert(pSyncData->dwSyncStatus & SDS_SYNC_OUT);

    szFullPath = (LPTSTR)LocalAlloc(LPTR, StringByteSize(pszName) + MAX_PATH*sizeof(TCHAR));
    if (!szFullPath)
    {
        dwErr = ERROR_OUTOFMEMORY;
        ExitGracefully(dwResult, CSCPROC_RETURN_SKIP, "LocalAlloc failed");
    }

    HANDLE hFind;
    WIN32_FIND_DATA fdRemote;

    PathCombine(szFullPath, pSyncData->szDrive, pszName + lstrlen(pSyncData->pszShareName));
    hFind = FindFirstFile(szFullPath, &fdRemote);

    // Does the net version still exist?
    if (hFind == INVALID_HANDLE_VALUE)
        ExitGracefully(dwResult, HandleDeleteConflict(pSyncData, pszName, dwStatus, dwHintFlags, pFind32), "Net file deleted");

    // Still exists, continue
    FindClose(hFind);

    // If only the attributes or file times were modified locally,
    // or if the file is hidden+system, keep the server copy and
    // don't bother the user.  (e.g. desktop.ini)
    if (HandleConflictLocally(pSyncData, pszName, dwStatus, pFind32->dwFileAttributes, fdRemote.dwFileAttributes))
    {
        ExitGracefully(dwResult, CSCPROC_RETURN_FORCE_INWARD, "Ignoring conflict");
    }
    else if (IsSilentFolder(pszName))
    {
        // It's in a per-user shell special folder. Last writer wins.
        if (CompareFileTime(&pFind32->ftLastWriteTime, &fdRemote.ftLastWriteTime) < 0)
        {
            ExitGracefully(dwResult, CSCPROC_RETURN_FORCE_INWARD, "Handling special folder conflict - server copy wins");
        }
        else
        {
            ExitGracefully(dwResult, CSCPROC_RETURN_FORCE_OUTWARD, "Handling special folder conflict - local copy wins");
        }
    }

    dwErr = GetNewVersionName(pszName,
                              pSyncData->pszShareName,
                              pSyncData->szDrive,
                              &pszNewName);
    if (NOERROR != dwErr)
    {
        ExitGracefully(dwResult, CSCPROC_RETURN_SKIP, "GetNewVersionName failed");
    }

    switch (SDS_SYNC_FILE_CONFLICT_MASK & pSyncData->dwSyncStatus)
    {
    case 0:
        if (CSC_SYNC_MAYBOTHERUSER & m_dwSyncFlags)
        {
            nErrorResolution = ShowConflictDialog(m_hwndDlgParent,
                                                  pszName,
                                                  pszNewName,
                                                  pSyncData->pszShareName,
                                                  pSyncData->szDrive,
                                                  pFind32,
                                                  &fdRemote);
            if (RFC_APPLY_TO_ALL & nErrorResolution)
            {
                bApplyToAll = TRUE;
                nErrorResolution &= ~RFC_APPLY_TO_ALL;
            }
        }
        break;

    case SDS_SYNC_CONFLICT_KEEPLOCAL:
        nErrorResolution = RFC_KEEPLOCAL;
        break;

    case SDS_SYNC_CONFLICT_KEEPNET:
        nErrorResolution = RFC_KEEPNETWORK;
        break;

    case SDS_SYNC_CONFLICT_KEEPBOTH:
        nErrorResolution = RFC_KEEPBOTH;
        break;
    }

    // Self-host notification callback
    CSCUI_NOTIFYHOOK((CSCH_UpdateConflict, TEXT("Update conflict: %1, resolution %2!d!"), pszName, nErrorResolution));

    switch (nErrorResolution)
    {
    default:
    case RFC_KEEPBOTH:
        if (bApplyToAll)
            pSyncData->dwSyncStatus |= SDS_SYNC_CONFLICT_KEEPBOTH;
        lstrcpy(szFullPath, pszName);
        PathRemoveFileSpec(szFullPath);
        if (FILE_ATTRIBUTE_DIRECTORY & pFind32->dwFileAttributes)
        {
            // Rename the local version in the cache and merge again.
            lstrcpyn(pFind32->cFileName, pszNewName, ARRAYSIZE(pFind32->cFileName));
            if (!CSCDoLocalRenameEx(pszName, szFullPath, pFind32, TRUE, TRUE))
            {
                dwErr = GetLastError();
                ExitGracefully(dwResult, CSCPROC_RETURN_SKIP, "CSCDoLocalRenameEx failed");
            }
            // Because CSCDoLocalRenameEx and CSCMergeShare are separate operations,
            // we have to abort the current merge operation and start over.
            // Otherwise, the current merge operation fails due to the "left
            // hand not knowing what the right hande is doing".
            Trace((TEXT("Restarting merge on: %s"), pSyncData->pszShareName));
            pSyncData->dwSyncStatus |= SDS_SYNC_RESTART_MERGE;
            dwResult = CSCPROC_RETURN_ABORT;
        }
        else
        {
            // Note that CSCDoLocalRenameEx would work for files also, but we
            // prefer to avoid restarting CSCMergeShare so do these ourselves.
            PathAppend(szFullPath, pszNewName);
            dwErr = CopyLocalFileWithDriveMapping(pszName,
                                                  szFullPath,
                                                  pSyncData->pszShareName,
                                                  pSyncData->szDrive);
            if (NOERROR != dwErr)
                ExitGracefully(dwResult, CSCPROC_RETURN_SKIP, "CopyLocalFileWithDriveMapping failed");

            // If the original file was pinned, we want to pin the copy also.
            // Unfortunately, we can't reliably pin during a merge, so we have
            // to remember these in a list and pin them later.
            if (dwHintFlags & FLAG_CSC_HINT_PIN_USER)
            {
                if (!m_pConflictPinList)
                    m_pConflictPinList = new CscFilenameList;
                if (m_pConflictPinList)
                {
                    m_pConflictPinList->AddFile(szFullPath,
                                                !!(pFind32->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
                }
            }

            // Tell CSCMergeShare to copy the server copy to the cache
            // (with the old name).  This clears the dirty cache.
            dwResult = CSCPROC_RETURN_FORCE_INWARD;
        }
        break;

    case RFC_KEEPNETWORK:
        // Tell CSCMergeShare to copy the server copy to the cache
        dwResult = CSCPROC_RETURN_FORCE_INWARD;
        if (bApplyToAll)
            pSyncData->dwSyncStatus |= SDS_SYNC_CONFLICT_KEEPNET;
        break;

    case RFC_KEEPLOCAL:
        // Tell CSCMergeShare to push the local copy to the server
        dwResult = CSCPROC_RETURN_FORCE_OUTWARD;
        if (bApplyToAll)
            pSyncData->dwSyncStatus |= SDS_SYNC_CONFLICT_KEEPLOCAL;
        break;

    case RFC_CANCEL:
        TraceMsg("HandleFileConflict: Cancelling sync - user bailed");
        SetItemStatus(GUID_NULL, SYNCMGRSTATUS_STOPPED);
        dwResult = CSCPROC_RETURN_ABORT;
        break;
    }

exit_gracefully:

    if (CSCPROC_RETURN_FORCE_INWARD == dwResult)
    {
        // CSCMergeShare truncates (makes sparse) the
        // file if we return this.  We'd like to fill
        // it during this sync.
        pSyncData->cFilesToSync++;
        pSyncData->dwSyncStatus |= SDS_SYNC_FORCE_INWARD;
    }

    if (NOERROR != dwErr)
    {
        pszName += lstrlen(pSyncData->pszShareName);
        if (*pszName == TEXT('\\'))
            pszName++;
        LogError(pSyncData->ItemID,
                 IDS_NAME_CONFLICT_ERROR,
                 pszName,
                 dwErr);
        pSyncData->dwSyncStatus |= SDS_SYNC_ERROR;
    }

    LocalFreeString(&szFullPath);
    LocalFreeString(&pszNewName);

    TraceLeaveResult(dwResult);
}


// Returns values for the Resolve Delete Conflict dialog
#define RDC_CANCEL      0x00
#define RDC_DELETE      0x01
#define RDC_RESTORE     0x02
#define RDC_APPLY_ALL   0x04
#define RDC_DELETE_ALL  (RDC_APPLY_ALL | RDC_DELETE)
#define RDC_RESTORE_ALL (RDC_APPLY_ALL | RDC_RESTORE)

TCHAR const c_szDeleteSelection[]   = TEXT("DeleteConflictSelection");

INT_PTR CALLBACK
DeleteConflictProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int nResult;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            TCHAR szShare[MAX_PATH];
            LPCTSTR pszPath = (LPCTSTR)lParam;
            LPTSTR pszT = NULL;

            szShare[0] = TEXT('\0');
            lstrcpyn(szShare, pszPath, ARRAYSIZE(szShare));
            PathStripToRoot(szShare);

            // Format the file name
            PathSetDlgItemPath(hDlg, IDC_FILENAME, pszPath);

            // Build the "Do this for all on <this share>" string
            FormatStringID(&pszT, g_hInstance, IDS_FMT_DELETE_APPLY_ALL, szShare);
            if (pszT)
            {
                SetDlgItemText(hDlg, IDC_APPLY_TO_ALL, pszT);
                LocalFreeString(&pszT);
            }
            // else default text is OK (no share name)

            // Select whatever the user chose last time, default is "restore"
            DWORD dwPrevSelection = RDC_RESTORE;
            DWORD dwType;
            DWORD cbData = sizeof(dwPrevSelection);
            SHGetValue(HKEY_CURRENT_USER,
                       c_szCSCKey,
                       c_szDeleteSelection,
                       &dwType,
                       &dwPrevSelection,
                       &cbData);
            dwPrevSelection = (RDC_DELETE == dwPrevSelection ? IDC_DELETE_LOCAL : IDC_KEEP_LOCAL);
            CheckRadioButton(hDlg, IDC_KEEP_LOCAL, IDC_DELETE_LOCAL, dwPrevSelection);

            // Get the file-type icon
            pszT = PathFindExtension(pszPath);
            if (pszT)
            {
                SHFILEINFO sfi = {0};
                SHGetFileInfo(pszT, 0, &sfi, sizeof(sfi), SHGFI_ICON);
                if (sfi.hIcon)
                {
                    SendDlgItemMessage(hDlg,
                                       IDC_DLGTYPEICON,
                                       STM_SETICON,
                                       (WPARAM)sfi.hIcon,
                                       0L);
                }
            }
        }
        return TRUE;

    case WM_COMMAND:
        nResult = -1;
        switch (LOWORD(wParam))
        {
        case IDCANCEL:
            nResult = RDC_CANCEL;
            break;

        case IDOK:
            if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_DELETE_LOCAL))
                nResult = RDC_DELETE;
            else
                nResult = RDC_RESTORE;
            // Remember the selection for next time
            SHSetValue(HKEY_CURRENT_USER,
                       c_szCSCKey,
                       c_szDeleteSelection,
                       REG_DWORD,
                       &nResult,
                       sizeof(nResult));
            if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_APPLY_TO_ALL))
                nResult |= RDC_APPLY_ALL;
            break;
        }
        if (-1 != nResult)
        {
            EndDialog(hDlg, nResult);
            return TRUE;
        }
        break;
    }
    return FALSE;
}


BOOL CALLBACK
ConflictPurgeCallback(LPCWSTR /*pszFile*/, LPARAM lParam)
{
    PSYNCTHREADDATA pSyncData = (PSYNCTHREADDATA)lParam;
    return !(SDS_SYNC_CANCELLED & pSyncData->dwSyncStatus);
}


DWORD
CCscUpdate::HandleDeleteConflict(PSYNCTHREADDATA    pSyncData,
                                 LPCTSTR            pszName,
                                 DWORD              dwStatus,
                                 DWORD              dwHintFlags,
                                 LPWIN32_FIND_DATA  pFind32)
{
    DWORD dwResult = CSCPROC_RETURN_CONTINUE;
    int nErrorResolution = RDC_DELETE;  // default action
    BOOL bDirectory = (FILE_ATTRIBUTE_DIRECTORY & pFind32->dwFileAttributes);

    TraceEnter(TRACE_UPDATE, "CCscUpdate::HandleDeleteConflict");
    Trace((TEXT("Net file deleted: %s"), pszName));

    //
    // We already know that the net file was deleted, or HandleDeleteConflict
    // wouldn't be called.  If the local copy was also deleted, then there
    // isn't really a conflict and we can continue without prompting.
    //
    // If the file isn't pinned, handle the conflict silently if
    // only attributes changed or it's super-hidden.
    //
    // Finally, if the file lives in certain special folder locations,
    // such as AppData, handle the conflict silently.
    //
    // If we get past all that, ask the user what to do, but only bother
    // the user as a last resort.
    //
    if ( !(dwStatus & FLAG_CSC_COPY_STATUS_LOCALLY_DELETED)
         && ((dwHintFlags & (FLAG_CSC_HINT_PIN_USER | FLAG_CSC_HINT_PIN_ADMIN)) ||
             !HandleConflictLocally(pSyncData, pszName, dwStatus, pFind32->dwFileAttributes))
         && !IsSilentFolder(pszName)
        )
    {
        // The file is either pinned or modified locally, so
        // default action is now "restore".
        nErrorResolution = RDC_RESTORE;

        switch (SDS_SYNC_DELETE_CONFLICT_MASK & pSyncData->dwSyncStatus)
        {
        case 0:
            if (CSC_SYNC_MAYBOTHERUSER & m_dwSyncFlags)
            {
                int idDialog = (bDirectory ? IDD_FOLDER_CONFLICT_DELETE : IDD_FILE_CONFLICT_DELETE);
                nErrorResolution = (int)DialogBoxParam(g_hInstance,
                                                       MAKEINTRESOURCE(idDialog),
                                                       m_hwndDlgParent,
                                                       DeleteConflictProc,
                                                       (LPARAM)pszName);
                if (RDC_DELETE_ALL == nErrorResolution)
                {
                    pSyncData->dwSyncStatus |= SDS_SYNC_DELETE_DELETE;
                    nErrorResolution = RDC_DELETE;
                }
                else if (RDC_RESTORE_ALL == nErrorResolution)
                {
                    pSyncData->dwSyncStatus |= SDS_SYNC_DELETE_RESTORE;
                    nErrorResolution = RDC_RESTORE;
                }
            }
            break;

        case SDS_SYNC_DELETE_DELETE:
            nErrorResolution = RDC_DELETE;
            break;

        case SDS_SYNC_DELETE_RESTORE:
            nErrorResolution = RDC_RESTORE;
            break;
        }

        // Self-host notification callback
        CSCUI_NOTIFYHOOK((CSCH_DeleteConflict, TEXT("Delete conflict: %1, resolution %2!d!"), pszName, nErrorResolution));
    }

    switch (nErrorResolution)
    {
    default:
    case RDC_RESTORE:
        Trace((TEXT("HandleDeleteConflict: restoring %s"), pszName));
        // Tell CSCMergeShare to push the local copy to the server
        dwResult = CSCPROC_RETURN_FORCE_OUTWARD;
        break;

    case RDC_DELETE:
        Trace((TEXT("HandleDeleteConflict: deleting %s"), pszName));
        if (bDirectory)
        {
            // Deep delete
            CSCUIRemoveFolderFromCache(pszName, 0, ConflictPurgeCallback, (LPARAM)pSyncData);
        }
        else
        {
            CscDelete(pszName);
        }
        dwResult = CSCPROC_RETURN_SKIP;
        break;

    case RDC_CANCEL:
        TraceMsg("HandleDeleteConflict: Cancelling sync - user bailed");
        SetItemStatus(GUID_NULL, SYNCMGRSTATUS_STOPPED);
        dwResult = CSCPROC_RETURN_ABORT;
        break;
    }

    TraceLeaveResult(dwResult);
}

DWORD
CCscUpdate::CscCallback(PSYNCTHREADDATA     pSyncData,
                        LPCTSTR             pszName,
                        DWORD               dwStatus,
                        DWORD               dwHintFlags,
                        DWORD               dwPinCount,
                        LPWIN32_FIND_DATA   pFind32,
                        DWORD               dwReason,
                        DWORD               dwParam1,
                        DWORD               dwParam2)
{
    DWORD dwResult = CSCPROC_RETURN_CONTINUE;
    SYNCMGRPROGRESSITEM spi = { sizeof(spi), 0 };

    TraceEnter(TRACE_UPDATE, "CCscUpdate::CscCallback");
    TraceAssert(pSyncData != NULL);
    TraceAssert(pSyncData->pThis == this);

    // Check for Cancel
    if (pSyncData->dwSyncStatus & SDS_SYNC_CANCELLED)
    {
        TraceMsg("Cancelling sync operation");
        TraceLeaveValue(CSCPROC_RETURN_ABORT);
    }

    switch (dwReason)
    {
    case CSCPROC_REASON_BEGIN:
        // First thing to do is determine if this is for the entire share
        // or an individual file in the share.
        if (!(pSyncData->dwSyncStatus & SDS_SYNC_STARTED))
        {
            // SHARE BEGIN
            pSyncData->dwSyncStatus |= SDS_SYNC_STARTED;

            TraceAssert(!lstrcmpi(pszName, pSyncData->pszShareName));
            Trace((TEXT("Share begin: %s"), pszName));

            if (pSyncData->dwSyncStatus & SDS_SYNC_OUT)
            {
                // Save the drive letter to use for net operations
                Trace((TEXT("Drive %s"), pFind32->cFileName));
                lstrcpyn(pSyncData->szDrive, pFind32->cFileName, ARRAYSIZE(pSyncData->szDrive));
            }
            else
            {
                pSyncData->szDrive[0] = TEXT('\0');
            }

            // Remember whether it's an autocache share or not
            switch (dwStatus & FLAG_CSC_SHARE_STATUS_CACHING_MASK)
            {
            case FLAG_CSC_SHARE_STATUS_AUTO_REINT:
            case FLAG_CSC_SHARE_STATUS_VDO:
                pSyncData->dwSyncStatus |= SDS_SYNC_AUTOCACHE;
                break;
            }
        }
        else
        {
            // FILE BEGIN
            BOOL bSkipFile = FALSE;

            TraceAssert(lstrlen(pszName) > lstrlen(pSyncData->pszShareName));

            if (!(pFind32->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                // If we're updating a file selection and this file
                // isn't part of the selection, skip it.
                if (m_pFileList && !m_pFileList->FileExists(pszName, false))
                {
                    bSkipFile = TRUE;
                }
                else if (!(pSyncData->dwSyncStatus & (SDS_SYNC_AUTOCACHE | SDS_SYNC_OUT)) &&
                         !(dwHintFlags & (FLAG_CSC_HINT_PIN_USER | FLAG_CSC_HINT_PIN_ADMIN)) &&
                         !IsSpecialFolder(pszName))
                {
                    // Skip autocached files when filling on a
                    // non-autocache share. Raid #341786
                    bSkipFile = TRUE;
                }
                else if (!(pSyncData->dwSyncStatus & CSC_SYNC_IGNORE_ACCESS))
                {
                    // dwReserved0 is the current user's access mask
                    // dwReserved1 is the Guest access mask
                    DWORD dwCurrentAccess = pFind32->dwReserved0 | pFind32->dwReserved1;
                    if (pSyncData->dwSyncStatus & SDS_SYNC_OUT)
                    {
                        //
                        // If the current user doesn't have sufficient access
                        // to merge offline changes, then don't bother trying.
                        // (It must be some other user's file.)
                        //

                        // Have the attributes changed offline?
                        if (FLAG_CSC_COPY_STATUS_ATTRIB_LOCALLY_MODIFIED & dwStatus)
                        {
                            // Yes. Continue if the current user has
                            // write-attribute access.
                            bSkipFile = !(dwCurrentAccess & FILE_WRITE_ATTRIBUTES);
                        }

                        // Have the contents changed offline?
                        if (!bSkipFile &&
                            ((FLAG_CSC_COPY_STATUS_DATA_LOCALLY_MODIFIED
                              | FLAG_CSC_COPY_STATUS_LOCALLY_CREATED
                              | FLAG_CSC_COPY_STATUS_LOCALLY_DELETED) & dwStatus))
                        {
                            // Yes. Continue if the current user has
                            // write-data access.
                            bSkipFile = !(dwCurrentAccess & FILE_WRITE_DATA);
                        }
                    }
                    else
                    {
                        //
                        // We're filling. Continue if the current user has
                        // read-data access, otherwise skip.
                        //
                        bSkipFile = !(dwCurrentAccess & FILE_READ_DATA);
                    }
                }
            }
            else if (!(pSyncData->dwSyncStatus & SDS_SYNC_OUT))
            {
                // It's a directory and we're in CSCFillSparseFiles.
                //
                // Note that we never skip directories when merging (we may be
                // interested in a file further down the tree) although we
                // can skip directories when filling.

                // If it's not in the file selection, skip it.
                if (m_pFileList && !m_pFileList->FileExists(pszName, false))
                {
                    bSkipFile = TRUE;
                }
            }

            if (bSkipFile)
            {
                Trace((TEXT("Skipping: %s"), pszName));
                dwResult = CSCPROC_RETURN_SKIP;
                pSyncData->dwSyncStatus |= SDS_SYNC_FILE_SKIPPED;
                break;
            }

            Trace((TEXT("File begin: %s"), pszName));

            //
            // Since we sometimes don't skip directories, even when it turns
            // out they have nothing that the current user is interested in,
            // don't display directory names in SyncMgr.
            //
            // If we sync a file farther down the tree, we will display the
            // filename and the intervening directory names will be visible
            // at that time.
            //
            if (!(pFind32->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                USES_CONVERSION;

                // Tell SyncMgr what we're doing
                spi.mask = SYNCMGRPROGRESSITEM_STATUSTEXT;
                spi.lpcStatusText = T2CW(pszName + lstrlen(pSyncData->pszShareName) + 1);
                NotifySyncMgr(pSyncData, &spi);
            }

            // dwParam1 is non-zero when there is a conflict, i.e. both the
            // local and remote versions of the file have been modified.
            if (dwParam1)
            {
                if (dwParam2)  // indicates server file deleted
                {
                    Trace((TEXT("Delete conflict: %d"), dwParam2));
                    dwResult = HandleDeleteConflict(pSyncData, pszName, dwStatus, dwHintFlags, pFind32);
                }
                else
                {
                    Trace((TEXT("Update conflict: %d"), dwParam1));
                    dwResult = HandleFileConflict(pSyncData, pszName, dwStatus, dwHintFlags, pFind32);
                }
            }
        }
        break;

    case CSCPROC_REASON_END:
        // dwParam2 == error code (winerror.h)
        if (3000 <= dwParam2 && dwParam2 <= 3200)
        {
            // Private error codes used in cscdll
            Trace((TEXT("CSC error: %d"), dwParam2));
            dwParam2 = NOERROR;
        }
        else if (ERROR_OPERATION_ABORTED == dwParam2)
        {
            // We returned CSCPROC_RETURN_ABORT for some reason.
            // Whatever it was, we already reported it.
            dwParam2 = NOERROR;
            dwResult = CSCPROC_RETURN_ABORT;
        }
        if (lstrlen(pszName) == lstrlen(pSyncData->pszShareName))
        {
            // SHARE END
            TraceAssert(!lstrcmpi(pszName, pSyncData->pszShareName));
            Trace((TEXT("Share end: %s"), pszName));

            pSyncData->dwSyncStatus &= ~SDS_SYNC_STARTED;
        }
        else
        {
            BOOL bUpdateProgress = FALSE;

            // FILE END
            if (!(pSyncData->dwSyncStatus & SDS_SYNC_FILE_SKIPPED))
            {
                Trace((TEXT("File end: %s"), pszName));

                bUpdateProgress = TRUE;

                // BUGBUG special case errors
                switch (dwParam2)
                {
                case ERROR_ACCESS_DENIED:
                    if (FILE_ATTRIBUTE_DIRECTORY & pFind32->dwFileAttributes)
                    {
                        // 317751 directories are not per-user, so if a
                        // different user syncs, we can hit this.  Don't want
                        // to show an error message unless we are ignoring
                        // access (when the user explicitly selected something
                        // to pin/sync).
                        //
                        // 394362 BrianV hit this running as an admin, so don't
                        // show this error for admins either.
                        //
                        if (!(pSyncData->dwSyncStatus & CSC_SYNC_IGNORE_ACCESS))
                        {
                            TraceMsg("Suppressing ERROR_ACCESS_DENIED on folder");
                            dwParam2 = NOERROR;
                        }
                    }
                    break;

                case ERROR_GEN_FAILURE:
                    TraceMsg("Received ERROR_GEN_FAILURE from cscdll");
                    if (dwStatus & FLAG_CSC_COPY_STATUS_FILE_IN_USE)
                        dwParam2 = ERROR_OPEN_FILES;
                    break;

                case ERROR_FILE_NOT_FOUND:
                case ERROR_PATH_NOT_FOUND:
                    // We either handle the error here or the user is
                    // prompted, so no need for another error message.
                    dwParam2 = NOERROR;
                    // If this is an autocache file and has not been modified
                    // offline, nuke it now.  Otherwise, prompt for action.
                    if (CSCPROC_RETURN_FORCE_OUTWARD == HandleDeleteConflict(pSyncData,
                                                                             pszName,
                                                                             dwStatus,
                                                                             dwHintFlags,
                                                                             pFind32))
                    {
                        dwParam2 = CopyLocalFileWithDriveMapping(pszName,
                                                                 pszName,
                                                                 pSyncData->pszShareName,
                                                                 pSyncData->szDrive,
                                                                 (FILE_ATTRIBUTE_DIRECTORY & pFind32->dwFileAttributes));
                    }
                    break;

                case ERROR_DISK_FULL:
                    // There's no point continuing
                    dwResult = CSCPROC_RETURN_ABORT;
                    break;

                default:
                    // nothing
                    break;
                }
            }
            else
            {
                pSyncData->dwSyncStatus &= ~SDS_SYNC_FILE_SKIPPED;
                dwParam2 = NOERROR;

                // If doing full sync, then we count progress for skipped
                // files as well. Not true for quick fill or merge.
                if (pSyncData->dwSyncStatus & SDS_SYNC_IN_FULL)
                    bUpdateProgress = TRUE;
            }

            // Update progress in SyncMgr
            if (bUpdateProgress)
            {
                pSyncData->cFilesDone++;

                spi.mask = SYNCMGRPROGRESSITEM_PROGVALUE;
                spi.iProgValue = min(pSyncData->cFilesDone, pSyncData->cFilesToSync - 1);
                Trace((TEXT("%d of %d files done"), spi.iProgValue, pSyncData->cFilesToSync));
                NotifySyncMgr(pSyncData, &spi);
            }
        }
        if (dwParam2 != NOERROR)
        {
            UINT idsError = GetErrorFormat(dwParam2, boolify(pSyncData->dwSyncStatus & SDS_SYNC_OUT));
            if (IDS_SHARE_CONNECT_ERROR == idsError)
            {
                LPTSTR pszErr = GetErrorText(dwParam2);
                //
                // Special-case the "can't connect to share" error.
                // Display only the share name in the error message
                // and abort the synchronization of this share.  
                //
                LogError(pSyncData->ItemID,
                         SYNCMGRLOGLEVEL_ERROR,
                         idsError,
                         pSyncData->pszShareName,
                         pszErr ? pszErr : TEXT(""));

                LocalFreeString(&pszErr);
                dwResult = CSCPROC_RETURN_ABORT;
            }
            else
            {
                LogError(pSyncData->ItemID,
                         idsError,
                         pszName,
                         dwParam2);
            }
            pSyncData->dwSyncStatus |= SDS_SYNC_ERROR;
        }
        break;
    }

    // Check for Cancel
    if (pSyncData->dwSyncStatus & SDS_SYNC_CANCELLED)
    {
        TraceMsg("Cancelling sync operation");
        dwResult = CSCPROC_RETURN_ABORT;
    }
    
    TraceLeaveValue(dwResult);
}


void
CCscUpdate::NotifySyncMgr(PSYNCTHREADDATA pSyncData, LPSYNCMGRPROGRESSITEM pspi)
{
    LPSYNCMGRSYNCHRONIZECALLBACK pSyncMgr = pSyncData->pThis->m_pSyncMgrCB;

    if (pSyncMgr)
    {
        HRESULT hr = pSyncMgr->Progress(pSyncData->ItemID, pspi);

        if (hr == S_SYNCMGR_CANCELITEM || hr == S_SYNCMGR_CANCELALL)
            pSyncData->dwSyncStatus |= SDS_SYNC_CANCELLED;
    }
}


DWORD WINAPI
CCscUpdate::_CscCallback(LPCTSTR             pszName,
                         DWORD               dwStatus,
                         DWORD               dwHintFlags,
                         DWORD               dwPinCount,
                         LPWIN32_FIND_DATA   pFind32,
                         DWORD               dwReason,
                         DWORD               dwParam1,
                         DWORD               dwParam2,
                         DWORD_PTR           dwContext)
{
    DWORD dwResult = CSCPROC_RETURN_ABORT;
    PSYNCTHREADDATA pSyncData = (PSYNCTHREADDATA)dwContext;

    if (pSyncData != NULL && pSyncData->pThis != NULL)
        dwResult = pSyncData->pThis->CscCallback(pSyncData,
                                                 pszName,
                                                 dwStatus,
                                                 dwHintFlags,
                                                 dwPinCount,
                                                 pFind32,
                                                 dwReason,
                                                 dwParam1,
                                                 dwParam2);
    return dwResult;
}


BOOL
CCscUpdate::PinLinkTarget(LPCTSTR pszName, PSYNCTHREADDATA pSyncData)
{
    BOOL bResult = FALSE;
    LPTSTR pszTarget = NULL;
    DWORD dwAttr = 0;

    TraceEnter(TRACE_SHELLEX, "PinLinkTarget");

    GetLinkTarget(pszName, NULL, &pszTarget, &dwAttr);
    if (pszTarget)
    {
        TraceAssert(!(dwAttr & FILE_ATTRIBUTE_DIRECTORY));

        // Check for EFS
        if ((FILE_ATTRIBUTE_ENCRYPTED & dwAttr) && SkipEFSPin(pSyncData, pszTarget))
            ExitGracefully(bResult, FALSE, "Skipping EFS link target");

        if (!(pSyncData->dwSyncStatus & SDS_SYNC_CANCELLED) &&
            CSCPinFile(pszTarget,
                       FLAG_CSC_HINT_PIN_USER | FLAG_CSC_HINT_PIN_INHERIT_USER,
                       NULL,
                       NULL,
                       NULL))
        {
            WIN32_FIND_DATA fd = {0};
            LPCTSTR pszT = PathFindFileName(pszTarget);
            fd.dwFileAttributes = dwAttr;
            lstrcpyn(fd.cFileName, pszT ? pszT : pszTarget, ARRAYSIZE(fd.cFileName));

            ShellChangeNotify(pszTarget, &fd, FALSE);

            bResult = TRUE;

            if (FILE_ATTRIBUTE_ENCRYPTED & dwAttr)
            {
                LogError(pSyncData->ItemID,
                         IDS_PIN_ENCRYPT_WARNING,
                         pszTarget,
                         NOERROR,
                         SYNCMGRLOGLEVEL_WARNING);
            }
        }
    }

exit_gracefully:

    LocalFreeString(&pszTarget);
    TraceLeaveValue(bResult);
}




DWORD WINAPI
CCscUpdate::_PinNewFilesW32Callback(LPCTSTR             pszName,
                                    ENUM_REASON         eReason,
                                    LPWIN32_FIND_DATA   pFind32,
                                    LPARAM              lpContext)
{
    PSYNCTHREADDATA pSyncData = (PSYNCTHREADDATA)lpContext;
    DWORD dwHintFlags = 0;
    DWORD dwErr = NOERROR;

    // This callback is used when enumerating a pinned folder looking
    // for new files on the server.  Since the parent folder is pinned,
    // any files in it that aren't pinned get pinned here.

    TraceEnter(TRACE_UPDATE, "CCscUpdate::_PinNewFilesW32Callback");
    TraceAssert(pSyncData != NULL);

    // Check for Cancel
    if (pSyncData->dwSyncStatus & SDS_SYNC_CANCELLED)
    {
        TraceMsg("Cancelling sync operation");
        TraceLeaveValue(CSCPROC_RETURN_ABORT);
    }

    // Always ignore folder_end and ignore folder_begin if we
    // aren't doing a recursive pin operation.
    if (eReason == ENUM_REASON_FOLDER_END ||
        (eReason == ENUM_REASON_FOLDER_BEGIN && !(pSyncData->pThis->m_dwSyncFlags & CSC_SYNC_PIN_RECURSE)))
    {
        TraceLeaveValue(CSCPROC_RETURN_SKIP);
    }

    // At this point, we either have 1) a file or 2) folder_begin + recurse,
    // so pin anything that isn't pinned.

    // Is this file already pinned?
    if (!CSCQueryFileStatus(pszName, NULL, NULL, &dwHintFlags))
        dwErr = GetLastError();

    if (ERROR_FILE_NOT_FOUND == dwErr ||
        (NOERROR == dwErr && !(dwHintFlags & (FLAG_CSC_HINT_PIN_USER | FLAG_CSC_HINT_PIN_ADMIN))))
    {
        // Check for EFS
        BOOL bIsEFSFile = (FILE_ATTRIBUTE_ENCRYPTED & pFind32->dwFileAttributes) &&
                            !(FILE_ATTRIBUTE_DIRECTORY & pFind32->dwFileAttributes);

        if (bIsEFSFile && pSyncData->pThis->SkipEFSPin(pSyncData, pszName))
            TraceLeaveResult(CSCPROC_RETURN_SKIP);

        if (pSyncData->dwSyncStatus & SDS_SYNC_CANCELLED)
            TraceLeaveValue(CSCPROC_RETURN_ABORT);

        // Pin it now.
        dwHintFlags |= FLAG_CSC_HINT_PIN_USER | FLAG_CSC_HINT_PIN_INHERIT_USER;
        
        if (CSCPinFile(pszName, dwHintFlags, NULL, NULL, NULL))
        {
            
            pSyncData->cFilesToSync++;
            ShellChangeNotify(pszName, pFind32, FALSE);

            if (bIsEFSFile)
            {
                pSyncData->pThis->LogError(pSyncData->ItemID,
                                           IDS_PIN_ENCRYPT_WARNING,
                                           pszName,
                                           NOERROR,
                                           SYNCMGRLOGLEVEL_WARNING);
            }

            // If this is a link file, pin the target (if appropriate)
            LPTSTR pszExtn = PathFindExtension(pszName);
            if (pszExtn && !lstrcmpi(pszExtn, c_szLNK))
            {
                if (pSyncData->pThis->PinLinkTarget(pszName, pSyncData))
                    pSyncData->cFilesToSync++;
            }
        }
        else
        {
            DWORD dwError = GetLastError();
            UINT idsError = GetErrorFormat(dwError);
            if (IDS_SHARE_CONNECT_ERROR == idsError)
            {
                LPTSTR pszErr = GetErrorText(dwError);
                //
                // Special-case the "can't connect to share" error.
                // Display only the share name in the error message
                // and abort the pinning of this share.  
                //
                pSyncData->pThis->LogError(pSyncData->ItemID,
                                           SYNCMGRLOGLEVEL_ERROR,
                                           idsError,
                                           pSyncData->pszShareName,
                                           pszErr ? pszErr : TEXT(""));

                LocalFreeString(&pszErr);
                pSyncData->dwSyncStatus |= SDS_SYNC_CANCELLED;
            }
            else
            {
                pSyncData->pThis->LogError(pSyncData->ItemID,
                                           IDS_PIN_FILE_ERROR,
                                           pszName,
                                           dwError);
            }
            pSyncData->dwSyncStatus |= SDS_SYNC_ERROR;
        }

        LPTSTR pszScanMsg = NULL;
        SYNCMGRPROGRESSITEM spi;
        spi.cbSize = sizeof(spi);
        spi.mask = SYNCMGRPROGRESSITEM_STATUSTEXT;
        spi.lpcStatusText = L" ";

        // Skip the share name
        TraceAssert(PathIsPrefix(pSyncData->pszShareName, pszName));
        pszName += lstrlen(pSyncData->pszShareName);
        if (*pszName == TEXT('\\'))
            pszName++;

        LPCTSTR pszFile = PathFindFileName(pszName);
        TCHAR szPath[MAX_PATH] = TEXT("\\");

        if (*pszName)
            lstrcpyn(szPath, pszName, min(ARRAYSIZE(szPath),(int)(pszFile-pszName)));

        // If we still have a name, build a string like
        // "scanning: dir\foo.txt" to display in SyncMgr
        if (FormatStringID(&pszScanMsg, g_hInstance, IDS_NEW_SCAN, pszFile, szPath))
        {
            USES_CONVERSION;
            spi.lpcStatusText = T2CW(pszScanMsg);
        }

        NotifySyncMgr(pSyncData, &spi);

        LocalFreeString(&pszScanMsg);
    }
    else if ((dwHintFlags & FLAG_CSC_HINT_PIN_USER) &&
             (pSyncData->pThis->m_dwSyncFlags & CSC_SYNC_PINFILES))
    {
        // FLAG_CSC_HINT_PIN_USER being set implies that CSCQueryFileStatus
        // succeeded above.

        // The item was already pinned.  Save it in the undo exclusion list.
        if (!pSyncData->pUndoExclusionList)
            pSyncData->pUndoExclusionList = new CscFilenameList;

        if (pSyncData->pUndoExclusionList)
            pSyncData->pUndoExclusionList->AddFile(pszName);
    }

    TraceLeaveValue(CSCPROC_RETURN_CONTINUE);
}


DWORD WINAPI
CCscUpdate::_PinNewFilesCSCCallback(LPCTSTR             pszName,
                                    ENUM_REASON         eReason,
                                    DWORD               /*dwStatus*/,
                                    DWORD               dwHintFlags,
                                    DWORD               /*dwPinCount*/,
                                    LPWIN32_FIND_DATA   /*pFind32*/,
                                    LPARAM              lpContext)
{
    PSYNCTHREADDATA pSyncData = (PSYNCTHREADDATA)lpContext;
    PCSCUPDATE pThis;

    // This callback is used when enumerating the CSC database looking
    // for pinned folders, with the intention of pinning new files
    // in those folders on the server.

    TraceEnter(TRACE_UPDATE, "CCscUpdate::_PinNewFilesCSCCallback");
    TraceAssert(pSyncData != NULL);
    TraceAssert(pSyncData->pThis != NULL);

    pThis = pSyncData->pThis;

    // Check for Cancel
    if (pSyncData->dwSyncStatus & SDS_SYNC_CANCELLED)
    {
        TraceMsg("Cancelling sync operation");
        TraceLeaveValue(CSCPROC_RETURN_ABORT);
    }

    // If this isn't a directory with the user hint flag, keep looking.
    if (eReason != ENUM_REASON_FOLDER_BEGIN ||
        !(dwHintFlags & FLAG_CSC_HINT_PIN_USER))
    {
        TraceLeaveValue(CSCPROC_RETURN_CONTINUE);
    }

    // If we have a file list and this directory isn't in the list,
    // continue without doing anything here.
    if (pSyncData->pThis->m_pFileList &&
        !pSyncData->pThis->m_pFileList->FileExists(pszName, false))
    {
        TraceLeaveValue(CSCPROC_RETURN_CONTINUE);
    }

    // Ok, we've found a directory with the user hint flag set. Walk
    // this directory on the server, pinning any files that aren't pinned.
    _Win32EnumFolder(pszName,
                     FALSE,
                     _PinNewFilesW32Callback,
                     (LPARAM)pSyncData);

    TraceLeaveValue(CSCPROC_RETURN_CONTINUE);
}


DWORD WINAPI
CCscUpdate::_SyncThread(LPVOID pThreadData)
{
    // Make sure the DLL stays loaded while this thread is running
    HINSTANCE hInstThisDll = LoadLibrary(c_szDllName);

    PSYNCTHREADDATA pSyncData = (PSYNCTHREADDATA)pThreadData;
    PCSCUPDATE pThis;
    HRESULT hrComInit = E_FAIL;
    SYNCMGRPROGRESSITEM spi = {0};
    DWORD dwErr = NOERROR;
    CSCSHARESTATS shareStats;
    CSCGETSTATSINFO si = { SSEF_NONE,
                           SSUF_NONE,
                           false,      // No access info reqd (faster).
                           false };     
    ULONG cDirtyFiles = 0;
    ULONG cStaleFiles = 0;
    DWORD dwShareStatus = 0;
    BOOL bShareOnline = FALSE;
    DWORD dwConnectionSpeed = 0;

    if (!pSyncData)
        return ERROR_INVALID_PARAMETER;

    pThis = pSyncData->pThis;
    if (!pThis || !pSyncData->pszShareName || !*pSyncData->pszShareName)
    {
        LocalFree(pSyncData);
        return ERROR_INVALID_PARAMETER;
    }

    // Make sure the object stays alive as long as we need it
    pThis->AddRef();

    spi.cbSize = sizeof(spi);

    TraceEnter(TRACE_UPDATE, "CCscUpdate::_SyncThread");

    hrComInit = CoInitialize(NULL);

    if (pSyncData->dwSyncStatus & SDS_SYNC_CANCELLED)
        ExitGracefully(dwErr, NOERROR, "Cancelling sync operation");

    // Figure out how many files need updating
    pSyncData->cFilesDone = 0;
    pSyncData->cFilesToSync = 0;
    _GetShareStatisticsForUser(pSyncData->pszShareName, &si, &shareStats);

    // Get share status
    CSCQueryFileStatus(pSyncData->pszShareName, &dwShareStatus, NULL, NULL);

    // The root of a special folder is pinned with a pin count, but
    // not the user-hint flag, so _GetShareStats doesn't count it.
    // We need to count this for some of the checks below.
    // (If the share is manual-cache, these look exactly like the 
    // Siemens scenario in 341786, but we want to sync them.)
    if (shareStats.cTotal && pThis->IsSpecialFolderShare(pSyncData->pszShareName))
    {
        shareStats.cPinned++;
    }

    if (pThis->m_dwSyncFlags & CSC_SYNC_OUT)
    {
        cDirtyFiles = shareStats.cModified;

        //
        // Force the merge code if there are open files, so we are
        // sure to do the open file warning. The danger here is that we
        // don't warn because the share with open files has nothing dirty,
        // but we merge changes on another share and then transition online.
        // We don't want to transition online without warning about open files.
        //
        if (0 == cDirtyFiles)
        {
            if ((FLAG_CSC_SHARE_STATUS_DISCONNECTED_OP & dwShareStatus) &&
                (FLAG_CSC_SHARE_STATUS_FILES_OPEN & dwShareStatus))
            {
                cDirtyFiles++;
            }
        }
    }

    if (pThis->m_dwSyncFlags & CSC_SYNC_IN_FULL)
    {
        // For full inward sync, always set cStaleFiles to at least 1 to force
        // a call to CSCFillSparseFiles.
        // Also, we get callbacks for each file and folder, even if they
        // are not sparse or stale, so go with cTotal here to make
        // the progress bar look right.
        cStaleFiles = max(shareStats.cTotal, 1);
    }
    else if (pThis->m_dwSyncFlags & CSC_SYNC_IN_QUICK)
    {
        cStaleFiles = shareStats.cSparse;

        // If we're pinning, then it's possible that nothing is sparse yet,
        // but we'll need to call CSCFillSparseFiles.
        if (pThis->m_dwSyncFlags & CSC_SYNC_PINFILES)
            pSyncData->dwSyncStatus |= SDS_SYNC_FORCE_INWARD;
    }

    // Self-host notification callback
    CSCUI_NOTIFYHOOK((CSCH_SyncShare, TEXT("Sync: %1, Dirty: %2!d!, Stale: %3!d!"), pSyncData->pszShareName, cDirtyFiles, cStaleFiles));

    if (dwShareStatus & FLAG_CSC_SHARE_STATUS_DISCONNECTED_OP)
    {
        // Can't call CSCFillSparseFiles when disconnected (it just fails)
        cStaleFiles = 0;
    }
    else if ((dwShareStatus & FLAG_CSC_SHARE_STATUS_CACHING_MASK) == FLAG_CSC_SHARE_STATUS_MANUAL_REINT
             && 0 == shareStats.cPinned
             && !(pThis->m_dwSyncFlags & CSC_SYNC_PINFILES))
    {
        // On a manual share, if nothing is pinned (and we aren't pinning)
        // then we prefer not to call CSCFillSparseFiles on the share.
        // Raid #341786
        Trace((TEXT("Manual cache share '%s' has only autocached files"), pSyncData->pszShareName));
        cStaleFiles = 0;
    }

    pSyncData->cFilesToSync = cDirtyFiles + cStaleFiles;

    //
    // At this point, if pSyncData->cFilesToSync is nonzero, then we are doing
    // a sync, and will be calling CSCBeginSynchronization to connect to the
    // share (with prompt for credentials if necessary).
    //
    // If SDS_SYNC_FORCE_INWARD is on, then we are pinning files.  We will only
    // call CSCFillSparseFiles is the server is in connected mode, and we will
    // only call CSCBeginSynchronization if pSyncData->cFilesToSync is nonzero
    // (to pin something, you must already have a connection to the share).
    //

    if (0 == pSyncData->cFilesToSync && !(pSyncData->dwSyncStatus & SDS_SYNC_FORCE_INWARD))
        ExitGracefully(dwErr, NOERROR, "Nothing to synchronize");

    // Tell SyncMgr how many files we're updating
    spi.mask = SYNCMGRPROGRESSITEM_STATUSTYPE
                | SYNCMGRPROGRESSITEM_PROGVALUE | SYNCMGRPROGRESSITEM_MAXVALUE;
    spi.dwStatusType = SYNCMGRSTATUS_UPDATING;
    spi.iProgValue = 0;
    spi.iMaxValue = pSyncData->cFilesToSync;
    Trace((TEXT("%d files to sync on %s"), spi.iMaxValue, pSyncData->pszShareName));
    NotifySyncMgr(pSyncData, &spi);

#if 1
    // BUGBUG RAS isn't notifying RDR before returning success, so
    // do this bogus delay. Remove this when RAS is fixed.
    if (pSyncData->dwSyncStatus & SDS_SYNC_RAS_CONNECTED)
    {
        //
        // We've just established a RAS connection, but there's a timing
        // problem with RDR. Wait until we can connect to the share, but
        // don't wait longer than RAS_CONNECT_DELAY.
        //
        Trace((TEXT("RAS delay for %s"), pSyncData->pszShareName));
        DWORD dwDelayTime = GetTickCount() + RAS_CONNECT_DELAY;
#ifdef DEBUG
        int cAttempts = 0;
#endif
        while (!bShareOnline &&
               GetTickCount() < dwDelayTime &&
               !(pSyncData->dwSyncStatus & SDS_SYNC_CANCELLED))
        {
            WaitForSingleObject(pThis->m_hSyncMutex, INFINITE);
#ifdef DEBUG
            cAttempts++;
#endif
            bShareOnline = CSCBeginSynchronization(pSyncData->pszShareName,
                                                   &dwConnectionSpeed,
                                                   &pSyncData->dwCscContext);
            ReleaseMutex(pThis->m_hSyncMutex);
        }
        Trace((TEXT("%s %s after %d attempts (%dms)"),
              pSyncData->pszShareName,
              (bShareOnline ? TEXT("connected") : TEXT("not reachable")),
              cAttempts,
              GetTickCount() - (dwDelayTime - RAS_CONNECT_DELAY)));
    }
    else
#endif  // RAS delay
    if (pSyncData->cFilesToSync)
    {
        //
        // CSCBeginSynchronization makes a net connection to the share
        // using the "interactive" flag. This causes a credential popup
        // if the current user doesn't have access to the share.
        // Use the sync mutex to avoid multiple concurrent popups.
        //
        WaitForSingleObject(pThis->m_hSyncMutex, INFINITE);
        bShareOnline = CSCBeginSynchronization(pSyncData->pszShareName,
                                               &dwConnectionSpeed,
                                               &pSyncData->dwCscContext);
        ReleaseMutex(pThis->m_hSyncMutex);
    }

    if (pSyncData->cFilesToSync && !bShareOnline)
    {
        // The share isn't reachable, so there's no point in continuing.
        dwErr = GetLastError();

        if (ERROR_CANCELLED == dwErr)
        {
            // The user cancelled the credential popup
            pSyncData->dwSyncStatus |= SDS_SYNC_CANCELLED;
            ExitGracefully(dwErr, NOERROR, "User cancelled sync");
        }

        LPTSTR pszErr = GetErrorText(dwErr);
        pThis->LogError(pSyncData->ItemID,
                        SYNCMGRLOGLEVEL_ERROR,
                        IDS_SHARE_CONNECT_ERROR,
                        pSyncData->pszShareName,
                        pszErr);
        LocalFreeString(&pszErr);
        ExitGracefully(dwErr, dwErr, "Share not reachable");
    }

    if (pSyncData->dwSyncStatus & SDS_SYNC_CANCELLED)
        ExitGracefully(dwErr, NOERROR, "Cancelling sync operation");

    // Note the time of this sync
    pThis->SetLastSyncTime(pSyncData->pszShareName);

    // Merge
    if (0 != cDirtyFiles)
    {
        dwErr = pThis->MergeShare(pSyncData);
        if (NOERROR != dwErr)
        {
            LPTSTR pszErr = GetErrorText(dwErr);
            pThis->LogError(pSyncData->ItemID,
                            SYNCMGRLOGLEVEL_ERROR,
                            IDS_MERGE_SHARE_ERROR,
                            pSyncData->pszShareName,
                            pszErr);
            LocalFreeString(&pszErr);
            ExitGracefully(dwErr, dwErr, "Aborting due to merge error");
        }
    }

    if (pSyncData->dwSyncStatus & SDS_SYNC_CANCELLED)
        ExitGracefully(dwErr, NOERROR, "Cancelling sync operation");

    // Fill
    if (0 != cStaleFiles || (pSyncData->dwSyncStatus & SDS_SYNC_FORCE_INWARD))
    {
        dwErr = pThis->FillShare(pSyncData, shareStats.cPinned, dwConnectionSpeed);
    }

exit_gracefully:

    // If we called CSCBeginSynchronization and it succeeded,
    // we need to call CSCEndSynchronization.
    if (bShareOnline)
        CSCEndSynchronization(pSyncData->pszShareName, pSyncData->dwCscContext);

    // Tell SyncMgr that we're done (succeeded, failed, or stopped)
    spi.mask = SYNCMGRPROGRESSITEM_STATUSTYPE | SYNCMGRPROGRESSITEM_STATUSTEXT
        | SYNCMGRPROGRESSITEM_PROGVALUE | SYNCMGRPROGRESSITEM_MAXVALUE;
    spi.dwStatusType = SYNCMGRSTATUS_SUCCEEDED; // Assume success
    if (pSyncData->dwSyncStatus & SDS_SYNC_CANCELLED)
        spi.dwStatusType = SYNCMGRSTATUS_STOPPED;
    if (NOERROR != dwErr || (pSyncData->dwSyncStatus & SDS_SYNC_ERROR))
        spi.dwStatusType = SYNCMGRSTATUS_FAILED;
    spi.lpcStatusText = L" ";
    spi.iProgValue = spi.iMaxValue = pSyncData->cFilesToSync; // This tells syncmgr that the item is done

    NotifySyncMgr(pSyncData, &spi);

    if ((pSyncData->dwSyncStatus & SDS_SYNC_CANCELLED)
        && (pThis->m_dwSyncFlags & CSC_SYNC_PINFILES))
    {
        // We cancelled a pin operation, roll back to the previous state
        CscUnpinFileList(pThis->m_pFileList,
                        (pThis->m_dwSyncFlags & CSC_SYNC_PIN_RECURSE),
                        pSyncData->pszShareName,
                        _UndoProgress,
                        (LPARAM)pSyncData);
    }

    // Tell the Update Handler that this thread is exiting
    // This may use OLE to notify SyncMgr that the sync is done,
    // so do this before CoUninitialize.
    Trace((TEXT("%s finished"), pSyncData->pszShareName));
    pThis->SyncThreadCompleted(pSyncData);

    if (SUCCEEDED(hrComInit))
        CoUninitialize();

    delete pSyncData->pUndoExclusionList;
    LocalFree(pSyncData);

    // Release our ref on the object
    // (also release our ref on the DLL)
    pThis->Release();

    TraceLeave();
    FreeLibraryAndExitThread(hInstThisDll, dwErr);
    return 0;
}

DWORD
CCscUpdate::MergeShare(PSYNCTHREADDATA pSyncData)
{
    DWORD dwErr = NOERROR;
    BOOL bMergeResult = TRUE;

    TraceEnter(TRACE_UPDATE, "CCscUpdate::MergeShare");

    // CSCMergeShare fails if another thread (or process) is
    // currently merging.  This is because CSCMergeShare uses
    // a drive letter connection to the share to bypass CSC,
    // and we don't want to use up all of the drive letters.
    // So let's protect the call to CSCMergeShare with a mutex
    // (rather than dealing with failure and retrying, etc.)

    WaitForSingleObject(m_hSyncMutex, INFINITE);

    if (pSyncData->dwSyncStatus & SDS_SYNC_CANCELLED)
        ExitGracefully(dwErr, NOERROR, "Merge cancelled");

    //
    // It would be nice to skip the open file warning if we knew
    // that the open files were on a "silent folder".  The best
    // we can do, though, is detect that the open files are on
    // the same share as a silent folder.  There's no guarantee
    // that the open files are not from a different folder on
    // the same share, so we have to show the warning.
    //
    //if (!IsSilentShare(pSyncData->pszShareName))
    {
        DWORD dwShareStatus = 0;
        CSCQueryFileStatus(pSyncData->pszShareName, &dwShareStatus, NULL, NULL);
        if (FLAG_CSC_SHARE_STATUS_FILES_OPEN & dwShareStatus)
        {
            if (CSC_SYNC_MAYBOTHERUSER & m_dwSyncFlags)
            {
                // Only show this warning once per sync (not per thread)
                if (!(CSC_SYNC_OFWARNINGDONE & m_dwSyncFlags))
                {
                    m_dwSyncFlags |= CSC_SYNC_OFWARNINGDONE;
                    if (IDOK != OpenFilesWarningDialog())
                    {
                        TraceMsg("Cancelling sync - user bailed at open file warning");
                        SetItemStatus(GUID_NULL, SYNCMGRSTATUS_STOPPED);
                    }
                }
                // else we already put up the warning on another thread.  If the
                // user cancelled, SDS_SYNC_CANCELLED will be set.
            }
            else
            {
                // Don't merge, but continue otherwise.
                LogError(pSyncData->ItemID,
                         SYNCMGRLOGLEVEL_WARNING,
                         IDS_OPENFILE_MERGE_WARNING,
                         pSyncData->pszShareName);
                ExitGracefully(dwErr, NOERROR, "Skipping merge due to open files");
            }
        }
    }

    pSyncData->dwSyncStatus = SDS_SYNC_OUT;

    //
    // Conflict resolution may require stopping and restarting CSCMergeShare,
    // so do this in a loop
    //
    while (!(pSyncData->dwSyncStatus & SDS_SYNC_CANCELLED))
    {
        Trace((TEXT("Calling CSCMergeShare(%s)"), pSyncData->pszShareName));

        bMergeResult = CSCMergeShare(pSyncData->pszShareName,
                                     CCscUpdate::_CscCallback,
                                     (DWORD_PTR)pSyncData);

        Trace((TEXT("CSCMergeShare(%s) returned"), pSyncData->pszShareName));

        // Do we need to merge again?
        if (!(SDS_SYNC_RESTART_MERGE & pSyncData->dwSyncStatus))
            break;

        pSyncData->dwSyncStatus &= ~SDS_SYNC_RESTART_MERGE;
    }

    if (!(pSyncData->dwSyncStatus & SDS_SYNC_CANCELLED) && !bMergeResult)
    {
        dwErr = GetLastError();
        if (ERROR_OPERATION_ABORTED == dwErr)
            dwErr = NOERROR;
    }

exit_gracefully:

    ReleaseMutex(m_hSyncMutex);

    TraceLeaveValue(dwErr);
}

DWORD
CCscUpdate::FillShare(PSYNCTHREADDATA pSyncData, int cPinned, DWORD dwConnectionSpeed)
{
    DWORD dwErr = NOERROR;
    DWORD dwShareStatus = 0;
    DWORD dwShareHints = 0;

    TraceEnter(TRACE_UPDATE, "CCscUpdate::FillShare");

    CSCQueryFileStatus(pSyncData->pszShareName, &dwShareStatus, NULL, &dwShareHints);

    if (m_dwSyncFlags & CSC_SYNC_IN_FULL)
    {
        pSyncData->dwSyncStatus = SDS_SYNC_IN_FULL;

        Trace((TEXT("Full sync at %d00 bps"), dwConnectionSpeed));

        //
        // Check the server for new files that should be pinned.
        //
        // We can't do this when disconnected.  Also, this is
        // time consuming, so don't do it on a slow connection.
        //
        if (!(FLAG_CSC_SHARE_STATUS_DISCONNECTED_OP & dwShareStatus)
            && cPinned && !_PathIsSlow(dwConnectionSpeed))
        {
            //
            // Look for pinned folders on this share by enumerating
            // in the CSC database.  Go out to the server only if/when
            // we find a pinned folder.
            //
            TraceMsg("Running FrankAr code");
            //
            if (CConfig::GetSingleton().AlwaysPinSubFolders())
            {
                //
                // If the "AlwaysPinSubFolders" policy is set, we 
                // do a recursive pin.  This will cause any content 
                // (including folders) of a pinned folder to become pinned.
                //
                pSyncData->pThis->m_dwSyncFlags |= CSC_SYNC_PIN_RECURSE;
            }

            // First check the root folder
            if (_PinNewFilesCSCCallback(pSyncData->pszShareName,
                                        ENUM_REASON_FOLDER_BEGIN,
                                        0,
                                        dwShareHints,
                                        0,
                                        NULL,
                                        (LPARAM)pSyncData) == CSCPROC_RETURN_CONTINUE)
            {
                _CSCEnumDatabase(pSyncData->pszShareName,
                                 TRUE,
                                 _PinNewFilesCSCCallback,
                                 (LPARAM)pSyncData);
            }

            TraceMsg("FrankAr code complete");
        }
    }
    else
    {
        pSyncData->dwSyncStatus = SDS_SYNC_IN_QUICK;

        if (m_dwSyncFlags & CSC_SYNC_PINFILES)
        {
            //
            // Enumerate the file list and pin everything, checking with
            // SyncMgr periodically.
            //
            PinFiles(pSyncData);
        }
    }

    if (m_pConflictPinList)
    {
        // Make sure that any files we created because of merge
        // conflicts are pinned.
        PinFiles(pSyncData, TRUE);
    }

    // Can't fill when disconnected
    if (!(FLAG_CSC_SHARE_STATUS_DISCONNECTED_OP & dwShareStatus))
    {
        // Clear the status text and update the max count in case we
        // pinned somthing above
        SYNCMGRPROGRESSITEM spi;
        spi.cbSize = sizeof(spi);
        spi.mask = SYNCMGRPROGRESSITEM_STATUSTEXT | SYNCMGRPROGRESSITEM_MAXVALUE;
        spi.lpcStatusText = L" ";
        spi.iMaxValue = pSyncData->cFilesToSync;
        Trace((TEXT("%d files to sync on %s"), spi.iMaxValue, pSyncData->pszShareName));
        NotifySyncMgr(pSyncData, &spi);

        if (!(pSyncData->dwSyncStatus & SDS_SYNC_CANCELLED))
        {
            Trace((TEXT("Calling CSCFillSparseFiles(%s, %s)"), pSyncData->pszShareName, (pSyncData->dwSyncStatus & SDS_SYNC_IN_FULL) ? TEXT("full") : TEXT("quick")));
            if (!CSCFillSparseFiles(pSyncData->pszShareName,
                                    !!(pSyncData->dwSyncStatus & SDS_SYNC_IN_FULL),
                                    CCscUpdate::_CscCallback,
                                    (DWORD_PTR)pSyncData))
            {
                dwErr = GetLastError();
                if (ERROR_OPERATION_ABORTED == dwErr)
                    dwErr = NOERROR;
            }
            Trace((TEXT("CSCFillSparseFiles(%s) complete"), pSyncData->pszShareName));
        }
    }
    else
    {
        Trace((TEXT("Skipping CSCFillSparseFiles(%s) - server is offline"), pSyncData->pszShareName));
    }

    TraceLeaveValue(dwErr);
}

void
CCscUpdate::PinFiles(PSYNCTHREADDATA pSyncData, BOOL bConflictPinList)
{
    CscFilenameList *pfnl;
    CscFilenameList::HSHARE hShare;
    LPCTSTR pszFile;
    BOOL bRecurse;

    TraceEnter(TRACE_UPDATE, "CCscUpdate::PinFiles");
    TraceAssert((m_dwSyncFlags & CSC_SYNC_PINFILES) || bConflictPinList);

    pfnl = m_pFileList;
    bRecurse = m_dwSyncFlags & CSC_SYNC_PIN_RECURSE;

    if (bConflictPinList)
    {
        pfnl = m_pConflictPinList;
        bRecurse = FALSE;
    }

    if (!pfnl ||
        !pfnl->GetShareHandle(pSyncData->pszShareName, &hShare))
    {
        TraceLeaveVoid();
    }

    CscFilenameList::FileIter fi = pfnl->CreateFileIterator(hShare);

    // Iterate over the filenames associated with the share.
    while (pszFile = fi.Next())
    {
        TCHAR szFullPath[MAX_PATH];
        TCHAR szRelativePath[MAX_PATH];
        WIN32_FIND_DATA fd;
        ULONG cchFile = lstrlen(pszFile) + 1; // include NULL

        // Check for Cancel
        if (pSyncData->dwSyncStatus & SDS_SYNC_CANCELLED)
            break;

        ZeroMemory(&fd, sizeof(fd));

        // Directories have a trailing "\*"
        if (StrChr(pszFile, TEXT('*')))
        {
            // It's a directory. Trim off the "\*"
            cchFile -= 2;

            // When pinning at the share level, pszFile points to "*"
            // and cchFile is now zero.
        }

        szRelativePath[0] = TEXT('\0');
        lstrcpyn(szRelativePath, pszFile, min(cchFile, ARRAYSIZE(szRelativePath)));

        // Build the full path
        PathCombine(szFullPath, pSyncData->pszShareName, szRelativePath);

        // Get attributes and test for existence
        fd.dwFileAttributes = GetFileAttributes(szFullPath);
        if ((DWORD)-1 == fd.dwFileAttributes)
            continue;

        pszFile = PathFindFileName(szFullPath);
        lstrcpyn(fd.cFileName, pszFile ? pszFile : szFullPath, ARRAYSIZE(fd.cFileName));

        // Check for EFS
        BOOL bIsEFSFile;
        bIsEFSFile = (FILE_ATTRIBUTE_ENCRYPTED & fd.dwFileAttributes) &&
                        !(FILE_ATTRIBUTE_DIRECTORY & fd.dwFileAttributes);

        if (bIsEFSFile && SkipEFSPin(pSyncData, szFullPath))
            continue;

        if (pSyncData->dwSyncStatus & SDS_SYNC_CANCELLED)
            break;

        // Pin it
        if (CSCPinFile(szFullPath,
                       FLAG_CSC_HINT_PIN_USER | FLAG_CSC_HINT_PIN_INHERIT_USER,
                       NULL,
                       NULL,
                       NULL))
        {
            if (bConflictPinList && m_pFileList)
                m_pFileList->AddFile(szFullPath, !!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
            pSyncData->cFilesToSync++;
            ShellChangeNotify(szFullPath, &fd, FALSE);
            if (bIsEFSFile)
            {
                LogError(pSyncData->ItemID,
                         IDS_PIN_ENCRYPT_WARNING,
                         szFullPath,
                         NOERROR,
                         SYNCMGRLOGLEVEL_WARNING);
            }
        }
        else
        {
            DWORD dwError = GetLastError();
            UINT idsError = GetErrorFormat(dwError);
            if (IDS_SHARE_CONNECT_ERROR == idsError)
            {
                LPTSTR pszErr = GetErrorText(dwError);
                //
                // Special-case the "can't connect to share" error.
                // Display only the share name in the error message
                // and abort the pinning of this share.  
                //
                LogError(pSyncData->ItemID,
                         SYNCMGRLOGLEVEL_ERROR,
                         idsError,
                         pSyncData->pszShareName,
                         pszErr ? pszErr : TEXT(""));

                LocalFreeString(&pszErr);
                pSyncData->dwSyncStatus |= SDS_SYNC_CANCELLED;
            }
            else
            {
                LogError(pSyncData->ItemID,
                         IDS_PIN_FILE_ERROR,
                         szFullPath,
                         dwError);
            }
            pSyncData->dwSyncStatus |= SDS_SYNC_ERROR;
        }

        // If it's a directory, pin its contents
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            _Win32EnumFolder(szFullPath,
                             bRecurse,
                             CCscUpdate::_PinNewFilesW32Callback,
                             (LPARAM)pSyncData);
        }
    }

    // Flush the shell notify queue
    ShellChangeNotify(NULL, TRUE);
    TraceLeaveVoid();
}


void
CCscUpdate::NotifyUndo(PSYNCTHREADDATA pSyncData, LPCTSTR pszName)
{
    LPTSTR pszMsg;
    SYNCMGRPROGRESSITEM spi;
    spi.cbSize = sizeof(spi);
    spi.mask = SYNCMGRPROGRESSITEM_STATUSTEXT;

    spi.lpcStatusText = L" ";

    // Skip the share name
    if (PathIsPrefix(pSyncData->pszShareName, pszName))
    {
        pszName += lstrlen(pSyncData->pszShareName);
        if (*pszName == TEXT('\\'))
            pszName++;
    }

    LPCTSTR pszFile = PathFindFileName(pszName);
    TCHAR szPath[MAX_PATH] = TEXT("\\");

    if (*pszName)
        lstrcpyn(szPath, pszName, min(ARRAYSIZE(szPath),(int)(pszFile-pszName)));

    // If we still have a name, build a string like
    // "undo: dir\foo.txt" to display in SyncMgr
    if (FormatStringID(&pszMsg, g_hInstance, IDS_UNDO_SCAN, pszFile, szPath))
    {
        USES_CONVERSION;
        spi.lpcStatusText = T2CW(pszMsg);
    }

    NotifySyncMgr(pSyncData, &spi);

    LocalFreeString(&pszMsg);
}


DWORD WINAPI
CCscUpdate::_UndoProgress(LPCTSTR pszItem, LPARAM lpContext)
{
    PSYNCTHREADDATA pSyncData = reinterpret_cast<PSYNCTHREADDATA>(lpContext);

    if (pSyncData->pUndoExclusionList &&
        pSyncData->pUndoExclusionList->FileExists(pszItem))
    {
        return CSCPROC_RETURN_SKIP;
    }

    // Update SyncMgr
    pSyncData->pThis->NotifyUndo(pSyncData, pszItem);

    return CSCPROC_RETURN_CONTINUE;
}

#define PINEFS_SKIP     0
#define PINEFS_PIN      1
#define PINEFS_CANCEL   2

INT_PTR CALLBACK
ConfirmEFSPinProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR nResult = 0;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            LPTSTR pszMsg = NULL;
            LPCTSTR pszFile = PathFindFileName((LPCTSTR)lParam);
            FormatStringID(&pszMsg, g_hInstance, IDS_FMT_PIN_EFS_MSG, pszFile);
            if (pszMsg)
            {
                SetDlgItemText(hDlg, IDC_EFS_MSG, pszMsg);
                LocalFree(pszMsg);
            }
            else
                EndDialog(hDlg, PINEFS_SKIP);
            CheckDlgButton(hDlg, IDC_SKIP_EFS, BST_CHECKED);
        }
        nResult = TRUE;
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            EndDialog(hDlg, BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_SKIP_EFS) ? PINEFS_SKIP : PINEFS_PIN);
            nResult = TRUE;
            break;

        case IDCANCEL:
            EndDialog(hDlg, PINEFS_CANCEL);
            nResult = TRUE;
            break;
        }
        break;
    }

    return nResult;
}

BOOL
CCscUpdate::SkipEFSPin(PSYNCTHREADDATA pSyncData, LPCTSTR pszItem)
{
    BOOL bSkip = FALSE;
    int iResult = PINEFS_PIN;

    if ((CSC_SYNC_SKIP_EFS & m_dwSyncFlags) ||
        !(CSC_SYNC_MAYBOTHERUSER & m_dwSyncFlags))
    {
        iResult = PINEFS_SKIP;
    }
    else if (!(CSC_SYNC_EFS_WARNING_SHOWN & m_dwSyncFlags))
    {
        EnterCriticalSection(&m_csThreadList);

        // Another thread may have come through here while we were
        // waiting for the critical section, so recheck any flags
        // that are modified here.
        //
        // We could take the critical section at the top of the function and
        // only check flags once, but this way we avoid extra blocking.

        if (CSC_SYNC_SKIP_EFS & m_dwSyncFlags)
        {
            iResult = PINEFS_SKIP;
        }
        else if (!(CSC_SYNC_EFS_WARNING_SHOWN & m_dwSyncFlags))
        {
            m_dwSyncFlags |= CSC_SYNC_EFS_WARNING_SHOWN;

            // Suspend the other sync threads
            SetSyncThreadStatus(SyncPause, pSyncData->ItemID);

            iResult = (int)DialogBoxParam(g_hInstance,
                                          MAKEINTRESOURCE(IDD_CONFIRM_PIN_EFS),
                                          m_hwndDlgParent,
                                          ConfirmEFSPinProc,
                                          (LPARAM)pszItem);
            if (PINEFS_SKIP == iResult)
                m_dwSyncFlags |= CSC_SYNC_SKIP_EFS;

            // Resume syncing
            SetSyncThreadStatus(SyncResume, pSyncData->ItemID);
        }

        LeaveCriticalSection(&m_csThreadList);
    }

    switch (iResult)
    {
    default:
    case PINEFS_SKIP:
        bSkip = TRUE;
        break;

    case PINEFS_PIN:
        // continue
        break;

    case PINEFS_CANCEL:
        // stop all threads
        SetItemStatus(GUID_NULL, SYNCMGRSTATUS_STOPPED);
        break;
    }

    return bSkip;
}

HRESULT
CCscUpdate::SetSyncThreadStatus(eSetSyncStatus status, REFGUID rItemID)
{
    // Assume success here.  If we don't find the thread,
    // it's probably already finished.
    HRESULT hr = S_OK;
    BOOL bOneItem;

    TraceEnter(TRACE_UPDATE, "CCscUpdate::SetSyncThreadStatus");

    bOneItem = (SyncStop == status && !IsEqualGUID(rItemID, GUID_NULL));

    EnterCriticalSection(&m_csThreadList);

    if (NULL != m_hSyncThreads)
    {
        int cItems = DPA_GetPtrCount(m_hSyncThreads);
        SYNCMGRPROGRESSITEM spi = {0};
        DWORD (WINAPI *pfnStartStop)(HANDLE);

        pfnStartStop = ResumeThread;

        spi.cbSize        = sizeof(spi);
        spi.mask          = SYNCMGRPROGRESSITEM_STATUSTYPE | SYNCMGRPROGRESSITEM_STATUSTEXT;
        spi.lpcStatusText = L" ";
        spi.dwStatusType  = SYNCMGRSTATUS_UPDATING;
        if (SyncPause == status)
        {
            spi.dwStatusType  = SYNCMGRSTATUS_PAUSED;
            pfnStartStop = SuspendThread;
        }

        while (cItems > 0)
        {
            PSYNCTHREADDATA pSyncData;

            --cItems;
            pSyncData = (PSYNCTHREADDATA)DPA_FastGetPtr(m_hSyncThreads, cItems);
            TraceAssert(NULL != pSyncData);

            if (SyncStop == status)
            {
                // Tell the thread to abort
                if (!bOneItem || IsEqualGUID(rItemID, pSyncData->ItemID))
                {
                    pSyncData->dwSyncStatus |= SDS_SYNC_CANCELLED;
                    if (bOneItem)
                        break;
                }
            }
            else
            {
                // Suspend or resume the thread if it's not the current thread
                if (!IsEqualGUID(rItemID, pSyncData->ItemID))
                    (*pfnStartStop)(pSyncData->hThread);
                m_pSyncMgrCB->Progress(pSyncData->ItemID, &spi);
            }
        }
    }

    LeaveCriticalSection(&m_csThreadList);

    TraceLeaveResult(hr);
}

void
CCscUpdate::GetSilentFolderList(void)
{
    delete m_pSilentFolderList;
    m_pSilentFolderList = new CscFilenameList;

    delete m_pSpecialFolderList;
    m_pSpecialFolderList = new CscFilenameList;

    BuildSilentFolderList(m_pSilentFolderList, m_pSpecialFolderList);

    if (0 == m_pSilentFolderList->GetShareCount())
    {
        delete m_pSilentFolderList;
        m_pSilentFolderList = NULL;
    }

    if (0 == m_pSpecialFolderList->GetShareCount())
    {
        delete m_pSpecialFolderList;
        m_pSpecialFolderList = NULL;
    }
}

void
BuildSilentFolderList(CscFilenameList *pfnlSilentFolders,
                      CscFilenameList *pfnlSpecialFolders)
{
    //
    // We will silently handle sync conflicts in any of the folders
    // below that have a '1' after them.
    //
    // If we get complaints about conflicts in folders that we
    // think we can handle silently and safely, add them.
    //
    // Note that CSIDL_PERSONAL (MyDocs) and CSIDL_MYPICTURES
    // should probably never be silent, since the user
    // interacts with them directly.
    //
    // This list corresponds to the list of shell folders that may
    // be redirected.  See also
    // HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders
    //
    static const int s_csidlFolders[][2] =
    {
        { CSIDL_PROGRAMS,           0 },
        { CSIDL_PERSONAL,           0 },
        { CSIDL_FAVORITES,          0 },
        { CSIDL_STARTUP,            0 },
        { CSIDL_RECENT,             1 },
        { CSIDL_SENDTO,             0 },
        { CSIDL_STARTMENU,          1 },
        { CSIDL_DESKTOPDIRECTORY,   0 },
        { CSIDL_NETHOOD,            0 },
        { CSIDL_TEMPLATES,          0 },
        { CSIDL_APPDATA,            1 },
        { CSIDL_PRINTHOOD,          0 },
        { CSIDL_MYPICTURES,         0 },
        { CSIDL_PROFILE,            1 },
        { CSIDL_ADMINTOOLS,         0 },
    };
    TCHAR szPath[MAX_PATH];

    for (int i = 0; i < ARRAYSIZE(s_csidlFolders); i++)
    {
        if (SHGetSpecialFolderPath(NULL,
                                   szPath,
                                   s_csidlFolders[i][0] | CSIDL_FLAG_DONT_VERIFY,
                                   FALSE))
        {
            // We only want UNC net paths
            LPTSTR pszUNC = NULL;
            GetRemotePath(szPath, &pszUNC);
            if (!pszUNC)
                continue;

            if (s_csidlFolders[i][1])
            {
                if (pfnlSilentFolders)
                    pfnlSilentFolders->AddFile(pszUNC, true);
            }
            else
            {
                if (pfnlSpecialFolders)
                    pfnlSpecialFolders->AddFile(pszUNC, true);
            }

            LocalFreeString(&pszUNC);
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// SyncMgr integration (ISyncMgrEnumItems)                                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CUpdateEnumerator::CUpdateEnumerator(PCSCUPDATE pUpdate)
: m_cRef(1),
  m_pUpdate(pUpdate),
  m_hFind(INVALID_HANDLE_VALUE),
  m_bEnumFileSelection(FALSE),
  m_cCheckedItemsEnumerated(0)
{
    DllAddRef();

    if (m_pUpdate)
    {
        m_pUpdate->AddRef();

        if (m_pUpdate->m_pFileList)
        {
            m_bEnumFileSelection = TRUE;
            m_SelectionIterator = m_pUpdate->m_pFileList->CreateShareIterator();
        }
    }
}

CUpdateEnumerator::~CUpdateEnumerator()
{
    if (m_hFind != INVALID_HANDLE_VALUE)
        CSCFindClose(m_hFind);

    DoRelease(m_pUpdate);
    DllRelease();
}


STDMETHODIMP CUpdateEnumerator::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(CUpdateEnumerator, ISyncMgrEnumItems),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CUpdateEnumerator::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CUpdateEnumerator::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}

STDMETHODIMP
CUpdateEnumerator::Next(ULONG celt, LPSYNCMGRITEM rgelt, PULONG pceltFetched)
{
    HRESULT hr = S_OK;
    ULONG cFetched = 0;
    LPSYNCMGRITEM pItem = rgelt;
    WIN32_FIND_DATA fd = {0};
    DWORD dwShareStatus = 0;
    DWORD dwSyncFlags;
    CscFilenameList::HSHARE hShare;
    LPCTSTR pszShareName = NULL;

    TraceEnter(TRACE_UPDATE, "CUpdateEnumerator::Next");
    TraceAssert(m_pUpdate != NULL);

    if (NULL == rgelt)
        TraceLeaveResult(E_INVALIDARG);

    dwSyncFlags = m_pUpdate->m_dwSyncFlags;

    while (cFetched < celt)
    {
        CSCEntry *pShareEntry;
        CSCSHARESTATS shareStats;
        CSCGETSTATSINFO si = { SSEF_NONE,
                               SSUF_TOTAL | SSUF_PINNED | SSUF_MODIFIED | SSUF_SPARSE | SSUF_DIRS,
                               false,
                               false };     

        if (m_bEnumFileSelection)
        {
            if (!m_SelectionIterator.Next(&hShare))
                break;

            pszShareName = m_pUpdate->m_pFileList->GetShareName(hShare);

            CSCQueryFileStatus(pszShareName, &dwShareStatus, NULL, NULL);
            fd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
            lstrcpyn(fd.cFileName, pszShareName, ARRAYSIZE(fd.cFileName));
        }
        else
        {
            if (m_hFind == INVALID_HANDLE_VALUE)
            {
                m_hFind = CacheFindFirst(NULL, &fd, &dwShareStatus, NULL, NULL, NULL);

                if (m_hFind == INVALID_HANDLE_VALUE)
                {
                    // The database is empty, so there's nothing to enumerate..
                    break;
                }

                pszShareName = fd.cFileName;
            }
            else if (CacheFindNext(m_hFind, &fd, &dwShareStatus, NULL, NULL, NULL))
            {
                pszShareName = fd.cFileName;
            }
            else
                break;
        }
        TraceAssert(pszShareName);

//
// BUGBUG  This was proposed as a fix for part of 383011. However,
// that bug only applies in a multi-user scenario, and if there
// are more than 3 users using the machine, this would cause a
// user whose SID had been expelled from the CSC database to not
// be able to sync a share where they do indeed have access.
//
//        // If the current user has no access to the share, don't enumerate it.
//        if (!(CscAccessUser(dwShareStatus) || CscAccessGuest(dwShareStatus)))
//            continue;

        // Count the # of pinned files, sparse files, etc.
        _GetShareStatisticsForUser(pszShareName, &si, &shareStats);

        // The root of a special folder is pinned with a pin count, but
        // not the user-hint flag, so _GetShareStats doesn't count it.
        // We need to count this for some of the checks below.
        // (If the share is manual-cache, these look exactly like the 
        // Siemens scenario in 341786, but we want to sync them.)
        if (shareStats.cTotal && m_pUpdate->IsSpecialFolderShare(pszShareName))
        {
            shareStats.cPinned++;
        }

        // If we're pinning, then even if nothing is sparse now,
        // there will be sparse files after we pin them.
        if (dwSyncFlags & CSC_SYNC_PINFILES)
        {
            shareStats.cSparse++;
            shareStats.cTotal++;
        }

        // If there's nothing cached on this share, then don't even
        // enumerate it to SyncMgr.  This avoids listing extra junk
        // in SyncMgr.
        if ((0 == shareStats.cTotal) ||
            (shareStats.cTotal == shareStats.cDirs && 0 == shareStats.cPinned))
        {
            // Either there is nothing cached for this share, or the only
            // things found were unpinned dirs (no files, no pinned dirs).
            // The second case can happen if you delete files from the viewer,
            // in which case you think you deleted everything but the viewer
            // doesn't show directories, so they weren't deleted.
            Trace((TEXT("Nothing cached on %s, not enumerating"), pszShareName));
            continue;
        }

        if ((dwShareStatus & FLAG_CSC_SHARE_STATUS_CACHING_MASK) == FLAG_CSC_SHARE_STATUS_NO_CACHING)
        {
            //
            // Don't enumerate "no-cache" shares if there's nothing to merge.
            //
            // These can exist in the cache if the share was previously
            // cacheable, but has since been changed to "no caching".
            //
            // If there's something to merge, we should still sync it to
            // get everything squared away.
            //
            if (!((dwSyncFlags & CSC_SYNC_OUT) && (shareStats.cModified)))
            {
                Trace((TEXT("Not enumerating no-cache share %s"), pszShareName));
                continue;
            }
            Trace((TEXT("Enumerating no-cache share %s with offline changes."), pszShareName));
        }

        pItem->dwFlags = SYNCMGRITEM_HASPROPERTIES;

        if ((dwShareStatus & FLAG_CSC_SHARE_STATUS_CACHING_MASK) == FLAG_CSC_SHARE_STATUS_MANUAL_REINT
             && 0 == shareStats.cPinned
             && !(dwSyncFlags & CSC_SYNC_PINFILES))
        {
            // On a manual share, if nothing is pinned (and we aren't pinning)
            // then we don't want to call CSCFillSparseFiles on the share.
            // Raid #341786
            Trace((TEXT("Manual cache share '%s' has only autocached files"), pszShareName));

            // However, if there is something to merge, then we need to sync.
            if (!((dwSyncFlags & CSC_SYNC_OUT) && shareStats.cModified))
            {
                Trace((TEXT("Not enumerating manual-cache share %s"), pszShareName));
                continue;
            }

            // There is something to merge, so enumerate the share but
            // tell SyncMgr that it's temporary so it doesn't save state
            // for this share.
            pItem->dwFlags |= SYNCMGRITEM_TEMPORARY;
        }

        //
        // In some circumstances, we may want to merge even if there
        // are no modified files, in order to show the open files warning.
        //
        // See comments in CCscUpdate::_SyncThread
        //
        if (0 == shareStats.cModified)
        {
            if ((FLAG_CSC_SHARE_STATUS_DISCONNECTED_OP & dwShareStatus) &&
                (FLAG_CSC_SHARE_STATUS_FILES_OPEN & dwShareStatus))
            {
                shareStats.cModified++;
            }
        }

        // Enumerate this share
        cFetched++;

        // Get existing share entry or create a new one
        pShareEntry = m_pUpdate->m_ShareLog.Add(pszShareName);
        if (!pShareEntry)
            TraceLeaveResult(E_OUTOFMEMORY);

        pItem->cbSize = sizeof(SYNCMGRITEM);
        pItem->ItemID = pShareEntry->Guid();
        pItem->hIcon = g_hCscIcon;
        // BUGBUG SYNCMGRITEM_TEMPORARY causes items to not show up in
        // SyncMgr's logon/logoff settings page.  Raid #237288
        //if (0 == shareStats.cPinned)
        //    pItem->dwFlags |= SYNCMGRITEM_TEMPORARY;
        if (ERROR_SUCCESS == m_pUpdate->GetLastSyncTime(pszShareName, &pItem->ftLastUpdate))
            pItem->dwFlags |= SYNCMGRITEM_LASTUPDATETIME;

        //
        // Determine whether this share needs syncing.
        //
        // At settings time, assume everything needs syncing (check everything)
        //
        // If outbound, shares with modified files are checked
        // If inbound (full), shares with sparse or pinned files are checked
        // If inbound (quick), shares with sparse files are checked
        //
        // Anything else doesn't need to be sync'ed at this time (unchecked)
        //
        pItem->dwItemState = SYNCMGRITEMSTATE_CHECKED;
        if (!(dwSyncFlags & CSC_SYNC_SETTINGS) &&
            !((dwSyncFlags & CSC_SYNC_OUT)      && shareStats.cModified) &&
            !((dwSyncFlags & CSC_SYNC_IN_FULL)  && shareStats.cTotal   ) &&
            !((dwSyncFlags & CSC_SYNC_IN_QUICK) && shareStats.cSparse  ))
        {
            pItem->dwItemState = SYNCMGRITEMSTATE_UNCHECKED;
        }

        // Get friendly share name here
        LPITEMIDLIST pidl = NULL;
        SHFILEINFO sfi = {0};
        if (SUCCEEDED(SHSimpleIDListFromFindData(pszShareName, &fd, &pidl)))
        {
            SHGetFileInfo((LPCTSTR)pidl,
                          0,
                          &sfi,
                          sizeof(sfi),
                          SHGFI_PIDL | SHGFI_DISPLAYNAME);
            SHFree(pidl);
        }
        if (TEXT('\0') != sfi.szDisplayName[0])
            pszShareName = sfi.szDisplayName;

        SHTCharToUnicode((LPTSTR)pszShareName, pItem->wszItemName, ARRAYSIZE(pItem->wszItemName));
        if (SYNCMGRITEMSTATE_CHECKED == pItem->dwItemState)
        {
            m_cCheckedItemsEnumerated++;
            Trace((TEXT("Enumerating %s, checked"), pszShareName));
        }
        else
        {
            Trace((TEXT("Enumerating %s, unchecked"), pszShareName));
        }

        pItem++;
    }


    if (pceltFetched)
        *pceltFetched = cFetched;

    if (cFetched != celt)
        hr = S_FALSE;

    if ((S_FALSE == hr) && 
        0 == m_cCheckedItemsEnumerated &&
        (CSC_SYNC_SHOWUI_ALWAYS & dwSyncFlags))
    {
        //
        // Special-case where we're synching nothing but still
        // want to display SyncMgr progress UI.  We enumerate a 
        // special string rather than a share name for display in
        // the status UI.  Force hr == S_OK so the caller will accept
        // this "dummy" item.  Next() will be called once more but 
        // m_cCheckedItemsEnumerated will be 1 so this block won't be
        // entered and we'll return S_FALSE indicating the end of the
        // enumeration.
        //
        pItem->cbSize      = sizeof(SYNCMGRITEM);
        pItem->hIcon       = g_hCscIcon;
        pItem->dwFlags     = SYNCMGRITEM_HASPROPERTIES;
        pItem->dwItemState = SYNCMGRITEMSTATE_CHECKED;
        pItem->ItemID      = GUID_CscNullSyncItem;

        UINT idString = IDS_NULLSYNC_ITEMNAME;
        if ((CSC_SYNC_OUT & dwSyncFlags) &&
            !((CSC_SYNC_IN_QUICK | CSC_SYNC_IN_FULL) & dwSyncFlags))
        {
            // Use different text if we are only merging
            idString = IDS_NULLMERGE_ITEMNAME;
        }

        LoadStringW(g_hInstance, 
                    idString, 
                    pItem->wszItemName,
                    ARRAYSIZE(pItem->wszItemName));
        m_cCheckedItemsEnumerated = 1;

        TraceMsg("Enumerating NULL item");
        hr = S_OK;
    }

    TraceLeaveResult(hr);
}


STDMETHODIMP
CUpdateEnumerator::Skip(ULONG celt)
{
    return Next(celt, NULL, NULL);
}


STDMETHODIMP
CUpdateEnumerator::Reset()
{
    m_cCheckedItemsEnumerated = 0;
    if (m_bEnumFileSelection)
    {
        m_SelectionIterator.Reset();
    }
    else if (m_hFind != INVALID_HANDLE_VALUE)
    {
        CSCFindClose(m_hFind);
        m_hFind = INVALID_HANDLE_VALUE;
    }
    return S_OK;
}


STDMETHODIMP
CUpdateEnumerator::Clone(LPSYNCMGRENUMITEMS *ppenum)
{
    return E_NOTIMPL;
}
