/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Shell change notification
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include "shelldesktop.h"
#include "shlwapi_undoc.h"
#include <atlsimpcoll.h> // for CSimpleArray
#include <process.h>     // for _beginthreadex
#include <assert.h>      // for assert

WINE_DEFAULT_DEBUG_CHANNEL(shcn);

// TODO: SHCNRF_RecursiveInterrupt

static inline void
NotifyFileSystemChange(LONG wEventId, LPCWSTR path1, LPCWSTR path2)
{
    SHChangeNotify(wEventId | SHCNE_INTERRUPT, SHCNF_PATHW, path1, path2);
}

//////////////////////////////////////////////////////////////////////////////
// DIRLIST --- directory list

// TODO: Share a DIRLIST in multiple Explorer

struct DIRLISTITEM
{
    WCHAR szPath[MAX_PATH];
    DWORD dwFileSize;
    BOOL fDir;

    DIRLISTITEM(LPCWSTR pszPath, DWORD dwSize, BOOL is_dir)
    {
        lstrcpynW(szPath, pszPath, _countof(szPath));
        dwFileSize = dwSize;
        fDir = is_dir;
    }

    BOOL IsEmpty() const
    {
        return szPath[0] == 0;
    }

    BOOL EqualPath(LPCWSTR pszPath) const
    {
        return lstrcmpiW(szPath, pszPath) == 0;
    }
};

class DIRLIST
{
public:
    DIRLIST()
    {
    }

    DIRLIST(LPCWSTR pszDir, BOOL fRecursive)
    {
        GetDirList(pszDir, fRecursive);
    }

    BOOL AddItem(LPCWSTR pszPath, DWORD dwFileSize, BOOL fDir);
    BOOL GetDirList(LPCWSTR pszDir, BOOL fRecursive);
    BOOL Contains(LPCWSTR pszPath, BOOL fDir) const;
    BOOL RenameItem(LPCWSTR pszPath1, LPCWSTR pszPath2, BOOL fDir);
    BOOL DeleteItem(LPCWSTR pszPath, BOOL fDir);
    BOOL GetFirstChange(LPWSTR pszPath) const;

    void RemoveAll()
    {
        m_items.RemoveAll();
    }

protected:
    CSimpleArray<DIRLISTITEM> m_items;
};

BOOL DIRLIST::Contains(LPCWSTR pszPath, BOOL fDir) const
{
    assert(!PathIsRelativeW(pszPath));

    for (INT i = 0; i < m_items.GetSize(); ++i)
    {
        if (m_items[i].IsEmpty() || fDir != m_items[i].fDir)
            continue;

        if (m_items[i].EqualPath(pszPath))
            return TRUE;
    }
    return FALSE;
}

BOOL DIRLIST::AddItem(LPCWSTR pszPath, DWORD dwFileSize, BOOL fDir)
{
    assert(!PathIsRelativeW(pszPath));

    if (dwFileSize == INVALID_FILE_SIZE)
    {
        WIN32_FIND_DATAW find;
        HANDLE hFind = FindFirstFileW(pszPath, &find);
        if (hFind == INVALID_HANDLE_VALUE)
            return FALSE;
        FindClose(hFind);
        dwFileSize = find.nFileSizeLow;
    }

    DIRLISTITEM item(pszPath, dwFileSize, fDir);
    return m_items.Add(item);
}

BOOL DIRLIST::RenameItem(LPCWSTR pszPath1, LPCWSTR pszPath2, BOOL fDir)
{
    assert(!PathIsRelativeW(pszPath1));
    assert(!PathIsRelativeW(pszPath2));

    for (INT i = 0; i < m_items.GetSize(); ++i)
    {
        if (m_items[i].fDir == fDir && m_items[i].EqualPath(pszPath1))
        {
            lstrcpynW(m_items[i].szPath, pszPath2, _countof(m_items[i].szPath));
            return TRUE;
        }
    }
    return FALSE;
}

BOOL DIRLIST::DeleteItem(LPCWSTR pszPath, BOOL fDir)
{
    assert(!PathIsRelativeW(pszPath));

    for (INT i = 0; i < m_items.GetSize(); ++i)
    {
        if (m_items[i].fDir == fDir && m_items[i].EqualPath(pszPath))
        {
            m_items[i].szPath[0] = 0;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL DIRLIST::GetDirList(LPCWSTR pszDir, BOOL fRecursive)
{
    // get the full path
    WCHAR szPath[MAX_PATH];
    lstrcpynW(szPath, pszDir, _countof(szPath));
    assert(!PathIsRelativeW(szPath));

    // is it a directory?
    if (!PathIsDirectoryW(szPath))
        return FALSE;

    // add the path
    if (!AddItem(szPath, 0, TRUE))
        return FALSE;

    // enumerate the file items to remember
    PathAppendW(szPath, L"*");
    WIN32_FIND_DATAW find;
    HANDLE hFind = FindFirstFileW(szPath, &find);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        ERR("FindFirstFileW failed\n");
        return FALSE;
    }

    do
    {
        // ignore "." and ".."
        if (lstrcmpW(find.cFileName, L".") == 0 ||
            lstrcmpW(find.cFileName, L"..") == 0)
        {
            continue;
        }

        // build a path
        PathRemoveFileSpecW(szPath);
        if (lstrlenW(szPath) + lstrlenW(find.cFileName) + 1 > MAX_PATH)
        {
            ERR("szPath is too long\n");
            continue;
        }
        PathAppendW(szPath, find.cFileName);

        // add the path and do recurse
        if (fRecursive && (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            GetDirList(szPath, fRecursive);
        else
            AddItem(szPath, find.nFileSizeLow, FALSE);
    } while (FindNextFileW(hFind, &find));

    FindClose(hFind);

    return TRUE;
}

BOOL DIRLIST::GetFirstChange(LPWSTR pszPath) const
{
    // validate paths
    for (INT i = 0; i < m_items.GetSize(); ++i)
    {
        if (m_items[i].IsEmpty())
            continue;

        if (m_items[i].fDir) // item is a directory
        {
            if (!PathIsDirectoryW(m_items[i].szPath))
            {
                // mismatched
                lstrcpynW(pszPath, m_items[i].szPath, MAX_PATH);
                return TRUE;
            }
        }
        else // item is a normal file
        {
            if (!PathFileExistsW(m_items[i].szPath) ||
                PathIsDirectoryW(m_items[i].szPath))
            {
                // mismatched
                lstrcpynW(pszPath, m_items[i].szPath, MAX_PATH);
                return TRUE;
            }
        }
    }

    // check sizes
    HANDLE hFind;
    WIN32_FIND_DATAW find;
    for (INT i = 0; i < m_items.GetSize(); ++i)
    {
        if (m_items[i].IsEmpty() || m_items[i].fDir)
            continue;

        // get size
        hFind = FindFirstFileW(m_items[i].szPath, &find);
        FindClose(hFind);

        if (hFind == INVALID_HANDLE_VALUE ||
            find.nFileSizeLow != m_items[i].dwFileSize)
        {
            // different size
            lstrcpynW(pszPath, m_items[i].szPath, MAX_PATH);
            return TRUE;
        }
    }

    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
// DirWatch --- directory watcher using ReadDirectoryChangesW

static HANDLE s_hThreadAPC = NULL;
static BOOL s_fTerminateAllWatches = FALSE;

// NOTE: Regard to asynchronous procedure call (APC), please see:
// https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-sleepex

// The APC thread function for directory watch
static unsigned __stdcall DirWatchThreadFuncAPC(void *)
{
    while (!s_fTerminateAllWatches)
    {
#if 1 // FIXME: This is a HACK
        WaitForSingleObjectEx(GetCurrentThread(), INFINITE, TRUE);
#else
        SleepEx(INFINITE, TRUE);
#endif
    }
    return 0;
}

// the buffer for ReadDirectoryChangesW
#define BUFFER_SIZE 0x1000
static BYTE s_buffer[BUFFER_SIZE];

class DirWatch
{
public:
    HANDLE m_hDir;
    WCHAR m_szDir[MAX_PATH];
    BOOL m_fDeadWatch;
    BOOL m_fRecursive;
    OVERLAPPED m_overlapped; // for async I/O
    DIRLIST m_DirList;

    static DirWatch *Create(LPCWSTR pszDir, BOOL fSubTree = FALSE);
    ~DirWatch();

protected:
    DirWatch(LPCWSTR pszDir, BOOL fSubTree);
};

DirWatch::DirWatch(LPCWSTR pszDir, BOOL fSubTree)
    : m_fDeadWatch(FALSE)
    , m_fRecursive(fSubTree)
    , m_DirList(pszDir, fSubTree)
{
    TRACE("DirWatch::DirWatch: %p, '%S'\n", this, pszDir);

    lstrcpynW(m_szDir, pszDir, MAX_PATH);

    // open the directory to watch changes (for ReadDirectoryChangesW)
    m_hDir = CreateFileW(pszDir, FILE_LIST_DIRECTORY,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL, OPEN_EXISTING,
                         FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                         NULL);
}

/*static*/ DirWatch *DirWatch::Create(LPCWSTR pszDir, BOOL fSubTree)
{
    WCHAR szFullPath[MAX_PATH];
    GetFullPathNameW(pszDir, _countof(szFullPath), szFullPath, NULL);

    DirWatch *pDirWatch = new DirWatch(szFullPath, fSubTree);
    if (pDirWatch->m_hDir == INVALID_HANDLE_VALUE)
    {
        ERR("CreateFileW failed\n");
        delete pDirWatch;
        pDirWatch = NULL;
    }
    return pDirWatch;
}

DirWatch::~DirWatch()
{
    TRACE("DirWatch::~DirWatch: %p, '%S'\n", this, m_szDir);

    if (m_hDir != INVALID_HANDLE_VALUE)
        CloseHandle(m_hDir);
}

static BOOL _BeginRead(DirWatch *pDirWatch);

// The APC procedure to add a DirWatch and start the directory watch
static void NTAPI _AddDirectoryProcAPC(ULONG_PTR Parameter)
{
    DirWatch *pDirWatch = (DirWatch *)Parameter;
    assert(pDirWatch != NULL);

    _BeginRead(pDirWatch);
}

// The APC procedure to request termination of a DirWatch
static void NTAPI _RequestTerminationAPC(ULONG_PTR Parameter)
{
    DirWatch *pDirWatch = (DirWatch *)Parameter;
    assert(pDirWatch != NULL);

    pDirWatch->m_fDeadWatch = TRUE;
    CancelIo(pDirWatch->m_hDir);
}

// The APC procedure to request termination of all the directory watches
static void NTAPI _RequestAllTerminationAPC(ULONG_PTR Parameter)
{
    s_fTerminateAllWatches = TRUE;
    CloseHandle(s_hThreadAPC);
    s_hThreadAPC = NULL;
}

// convert the file action to an event
static DWORD
ConvertActionToEvent(DWORD Action, BOOL fDir)
{
    switch (Action)
    {
        case FILE_ACTION_ADDED:
            return (fDir ? SHCNE_MKDIR : SHCNE_CREATE);
        case FILE_ACTION_REMOVED:
            return (fDir ? SHCNE_RMDIR : SHCNE_DELETE);
        case FILE_ACTION_MODIFIED:
            return (fDir ? SHCNE_UPDATEDIR : SHCNE_UPDATEITEM);
        case FILE_ACTION_RENAMED_OLD_NAME:
            break;
        case FILE_ACTION_RENAMED_NEW_NAME:
            return (fDir ? SHCNE_RENAMEFOLDER : SHCNE_RENAMEITEM);
        default:
            break;
    }
    return 0;
}

// Notify a filesystem notification using pDirWatch.
static void _ProcessNotification(DirWatch *pDirWatch)
{
    PFILE_NOTIFY_INFORMATION pInfo = (PFILE_NOTIFY_INFORMATION)s_buffer;
    WCHAR szName[MAX_PATH], szPath[MAX_PATH], szTempPath[MAX_PATH];
    DWORD dwEvent, cbName;
    BOOL fDir;

    // if the watch is recursive
    if (pDirWatch->m_fRecursive)
    {
        // get the first change
        if (!pDirWatch->m_DirList.GetFirstChange(szPath))
            return;

        // then, notify a SHCNE_UPDATEDIR
        if (lstrcmpiW(pDirWatch->m_szDir, szPath) != 0)
            PathRemoveFileSpecW(szPath);
        NotifyFileSystemChange(SHCNE_UPDATEDIR, szPath, NULL);

        // refresh directory list
        pDirWatch->m_DirList.RemoveAll();
        pDirWatch->m_DirList.GetDirList(pDirWatch->m_szDir, TRUE);
        return;
    }

    // for each entry in s_buffer
    szPath[0] = szTempPath[0] = 0;
    for (;;)
    {
        // get name (relative from pDirWatch->m_szDir)
        cbName = pInfo->FileNameLength;
        if (sizeof(szName) - sizeof(UNICODE_NULL) < cbName)
        {
            ERR("pInfo->FileName is longer than szName\n");
            break;
        }
        // NOTE: FILE_NOTIFY_INFORMATION.FileName is not null-terminated.
        ZeroMemory(szName, sizeof(szName));
        CopyMemory(szName, pInfo->FileName, cbName);

        // get full path
        lstrcpynW(szPath, pDirWatch->m_szDir, _countof(szPath));
        PathAppendW(szPath, szName);

        // convert to long pathname if it contains '~'
        if (StrChrW(szPath, L'~') != NULL)
        {
            GetLongPathNameW(szPath, szName, _countof(szName));
            lstrcpynW(szPath, szName, _countof(szPath));
        }

        // convert action to event
        fDir = PathIsDirectoryW(szPath);
        dwEvent = ConvertActionToEvent(pInfo->Action, fDir);

        // get the directory list of pDirWatch
        DIRLIST& List = pDirWatch->m_DirList;

        // convert SHCNE_DELETE to SHCNE_RMDIR if the path is a directory
        if (!fDir && (dwEvent == SHCNE_DELETE) && List.Contains(szPath, TRUE))
        {
            fDir = TRUE;
            dwEvent = SHCNE_RMDIR;
        }

        // update List
        switch (dwEvent)
        {
            case SHCNE_MKDIR:
                if (!List.AddItem(szPath, 0, TRUE))
                    dwEvent = 0;
                break;
            case SHCNE_CREATE:
                if (!List.AddItem(szPath, INVALID_FILE_SIZE, FALSE))
                    dwEvent = 0;
                break;
            case SHCNE_RENAMEFOLDER:
                if (!List.RenameItem(szTempPath, szPath, TRUE))
                    dwEvent = 0;
                break;
            case SHCNE_RENAMEITEM:
                if (!List.RenameItem(szTempPath, szPath, FALSE))
                    dwEvent = 0;
                break;
            case SHCNE_RMDIR:
                if (!List.DeleteItem(szPath, TRUE))
                    dwEvent = 0;
                break;
            case SHCNE_DELETE:
                if (!List.DeleteItem(szPath, FALSE))
                    dwEvent = 0;
                break;
        }

        if (dwEvent != 0)
        {
            // notify
            if (pInfo->Action == FILE_ACTION_RENAMED_NEW_NAME)
                NotifyFileSystemChange(dwEvent, szTempPath, szPath);
            else
                NotifyFileSystemChange(dwEvent, szPath, NULL);
        }
        else if (pInfo->Action == FILE_ACTION_RENAMED_OLD_NAME)
        {
            // save path for next FILE_ACTION_RENAMED_NEW_NAME
            lstrcpynW(szTempPath, szPath, MAX_PATH);
        }

        if (pInfo->NextEntryOffset == 0)
            break; // there is no next entry

        // go next entry
        pInfo = (PFILE_NOTIFY_INFORMATION)((LPBYTE)pInfo + pInfo->NextEntryOffset);
    }
}

// The completion routine of ReadDirectoryChangesW.
static void CALLBACK
_NotificationCompletion(DWORD dwErrorCode,
                        DWORD dwNumberOfBytesTransfered,
                        LPOVERLAPPED lpOverlapped)
{
    // MSDN: The hEvent member of the OVERLAPPED structure is not used by the
    // system in this case, so you can use it yourself. We do just this, storing
    // a pointer to the working struct in the overlapped structure.
    DirWatch *pDirWatch = (DirWatch *)lpOverlapped->hEvent;
    assert(pDirWatch != NULL);

    // If the FSD doesn't support directory change notifications, there's no
    // no need to retry and requeue notification
    if (dwErrorCode == ERROR_INVALID_FUNCTION)
    {
        ERR("ERROR_INVALID_FUNCTION\n");
        return;
    }

    // Also, if the notify operation was canceled (like, user moved to another
    // directory), then, don't requeue notification.
    if (dwErrorCode == ERROR_OPERATION_ABORTED)
    {
        TRACE("ERROR_OPERATION_ABORTED\n");
        if (pDirWatch->m_fDeadWatch)
            delete pDirWatch;
        return;
    }

    // is this watch dead?
    if (pDirWatch->m_fDeadWatch)
    {
        TRACE("m_fDeadWatch\n");
        delete pDirWatch;
        return;
    }

    // This likely means overflow, so force whole directory refresh.
    if (dwNumberOfBytesTransfered == 0)
    {
        // do notify a SHCNE_UPDATEDIR
        NotifyFileSystemChange(SHCNE_UPDATEDIR, pDirWatch->m_szDir, NULL);
    }
    else
    {
        // do notify
        _ProcessNotification(pDirWatch);
    }

    // restart a watch
    _BeginRead(pDirWatch);
}

// convert events to notification filter
static DWORD
GetFilterFromEvents(DWORD fEvents)
{
    // FIXME
    return (FILE_NOTIFY_CHANGE_FILE_NAME |
            FILE_NOTIFY_CHANGE_DIR_NAME |
            FILE_NOTIFY_CHANGE_CREATION |
            FILE_NOTIFY_CHANGE_SIZE);
}

// Restart a watch by using ReadDirectoryChangesW function
static BOOL _BeginRead(DirWatch *pDirWatch)
{
    assert(pDirWatch != NULL);

    if (pDirWatch->m_fDeadWatch)
    {
        delete pDirWatch;
        return FALSE; // the watch is dead
    }

    // initialize the buffer and the overlapped
    ZeroMemory(s_buffer, sizeof(s_buffer));
    ZeroMemory(&pDirWatch->m_overlapped, sizeof(pDirWatch->m_overlapped));
    pDirWatch->m_overlapped.hEvent = (HANDLE)pDirWatch;

    // start the directory watch
    DWORD dwFilter = GetFilterFromEvents(SHCNE_ALLEVENTS);
    if (!ReadDirectoryChangesW(pDirWatch->m_hDir, s_buffer, sizeof(s_buffer),
                               pDirWatch->m_fRecursive, dwFilter, NULL,
                               &pDirWatch->m_overlapped, _NotificationCompletion))
    {
        ERR("ReadDirectoryChangesW for '%S' failed (error: %ld)\n",
            pDirWatch->m_szDir, GetLastError());
        return FALSE; // failure
    }

    return TRUE; // success
}

// create a DirWatch from a REGENTRY
static DirWatch *
CreateDirWatchFromRegEntry(LPREGENTRY pRegEntry)
{
    if (pRegEntry->ibPidl == 0)
        return NULL;

    // it must be interrupt level if pRegEntry is a filesystem watch
    if (!(pRegEntry->fSources & SHCNRF_InterruptLevel))
        return NULL;

    // get the path
    WCHAR szPath[MAX_PATH];
    LPITEMIDLIST pidl = (LPITEMIDLIST)((LPBYTE)pRegEntry + pRegEntry->ibPidl);
    if (!SHGetPathFromIDListW(pidl, szPath) || !PathIsDirectoryW(szPath))
        return NULL;

    // create a DirWatch
    DirWatch *pDirWatch = DirWatch::Create(szPath, pRegEntry->fRecursive);
    if (pDirWatch == NULL)
        return NULL;

    return pDirWatch;
}

//////////////////////////////////////////////////////////////////////////////

// notification target item
struct ITEM
{
    UINT nRegID;        // The registration ID.
    DWORD dwUserPID;    // The user PID; that is the process ID of the target window.
    HANDLE hRegEntry;   // The registration entry.
    HWND hwndBroker;    // Client broker window (if any).
    DirWatch *pDirWatch; // for filesystem notification (for SHCNRF_InterruptLevel)
};

typedef CWinTraits <
    WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
    WS_EX_TOOLWINDOW
> CChangeNotifyServerTraits;

//////////////////////////////////////////////////////////////////////////////
// CChangeNotifyServer
//
// CChangeNotifyServer implements a window that handles all shell change notifications.
// It runs in the context of explorer and specifically in the thread of the shell desktop.
// Shell change notification api exported from shell32 forwards all their calls
// to this window where all processing takes place.

class CChangeNotifyServer :
    public CWindowImpl<CChangeNotifyServer, CWindow, CChangeNotifyServerTraits>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IOleWindow
{
public:
    CChangeNotifyServer();
    virtual ~CChangeNotifyServer();
    HRESULT Initialize();

    // *** IOleWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *lphwnd);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

    // Message handlers
    LRESULT OnRegister(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnUnRegister(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDeliverNotification(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSuspendResume(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnRemoveByPID(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    DECLARE_NOT_AGGREGATABLE(CChangeNotifyServer)

    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CChangeNotifyServer)
        COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
    END_COM_MAP()

    DECLARE_WND_CLASS_EX(L"WorkerW", 0, 0)

    BEGIN_MSG_MAP(CChangeNotifyServer)
        MESSAGE_HANDLER(CN_REGISTER, OnRegister)
        MESSAGE_HANDLER(CN_UNREGISTER, OnUnRegister)
        MESSAGE_HANDLER(CN_DELIVER_NOTIFICATION, OnDeliverNotification)
        MESSAGE_HANDLER(CN_SUSPEND_RESUME, OnSuspendResume)
        MESSAGE_HANDLER(CN_UNREGISTER_PROCESS, OnRemoveByPID);
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy);
    END_MSG_MAP()

private:
    UINT m_nNextRegID;
    CSimpleArray<ITEM> m_items;

    BOOL AddItem(UINT nRegID, DWORD dwUserPID, HANDLE hRegEntry, HWND hwndBroker,
                 DirWatch *pDirWatch);
    BOOL RemoveItemsByRegID(UINT nRegID, DWORD dwOwnerPID);
    void RemoveItemsByProcess(DWORD dwOwnerPID, DWORD dwUserPID);
    void DestroyItem(ITEM& item, DWORD dwOwnerPID, HWND *phwndBroker);

    UINT GetNextRegID();
    BOOL DeliverNotification(HANDLE hTicket, DWORD dwOwnerPID);
    BOOL ShouldNotify(LPDELITICKET pTicket, LPREGENTRY pRegEntry);
};

CChangeNotifyServer::CChangeNotifyServer()
    : m_nNextRegID(INVALID_REG_ID)
{
}

CChangeNotifyServer::~CChangeNotifyServer()
{
}

BOOL CChangeNotifyServer::AddItem(UINT nRegID, DWORD dwUserPID, HANDLE hRegEntry,
                                  HWND hwndBroker, DirWatch *pDirWatch)
{
    // find the empty room
    for (INT i = 0; i < m_items.GetSize(); ++i)
    {
        if (m_items[i].nRegID == INVALID_REG_ID)
        {
            // found the room, populate it
            m_items[i].nRegID = nRegID;
            m_items[i].dwUserPID = dwUserPID;
            m_items[i].hRegEntry = hRegEntry;
            m_items[i].hwndBroker = hwndBroker;
            m_items[i].pDirWatch = pDirWatch;
            return TRUE;
        }
    }

    // no empty room found
    ITEM item = { nRegID, dwUserPID, hRegEntry, hwndBroker, pDirWatch };
    m_items.Add(item);
    return TRUE;
}

void CChangeNotifyServer::DestroyItem(ITEM& item, DWORD dwOwnerPID, HWND *phwndBroker)
{
    // destroy broker if any and if first time
    HWND hwndBroker = item.hwndBroker;
    item.hwndBroker = NULL;
    if (hwndBroker && hwndBroker != *phwndBroker)
    {
        ::DestroyWindow(hwndBroker);
        *phwndBroker = hwndBroker;
    }

    // request termination of pDirWatch if any
    DirWatch *pDirWatch = item.pDirWatch;
    item.pDirWatch = NULL;
    if (pDirWatch && s_hThreadAPC)
    {
        QueueUserAPC(_RequestTerminationAPC, s_hThreadAPC, (ULONG_PTR)pDirWatch);
    }

    // free
    SHFreeShared(item.hRegEntry, dwOwnerPID);
    item.nRegID = INVALID_REG_ID;
    item.dwUserPID = 0;
    item.hRegEntry = NULL;
    item.hwndBroker = NULL;
    item.pDirWatch = NULL;
}

BOOL CChangeNotifyServer::RemoveItemsByRegID(UINT nRegID, DWORD dwOwnerPID)
{
    BOOL bFound = FALSE;
    HWND hwndBroker = NULL;
    assert(nRegID != INVALID_REG_ID);
    for (INT i = 0; i < m_items.GetSize(); ++i)
    {
        if (m_items[i].nRegID == nRegID)
        {
            bFound = TRUE;
            DestroyItem(m_items[i], dwOwnerPID, &hwndBroker);
        }
    }
    return bFound;
}

void CChangeNotifyServer::RemoveItemsByProcess(DWORD dwOwnerPID, DWORD dwUserPID)
{
    HWND hwndBroker = NULL;
    assert(dwUserPID != 0);
    for (INT i = 0; i < m_items.GetSize(); ++i)
    {
        if (m_items[i].dwUserPID == dwUserPID)
        {
            DestroyItem(m_items[i], dwOwnerPID, &hwndBroker);
        }
    }
}

static BOOL CreateAPCThread(void)
{
    if (s_hThreadAPC != NULL)
        return TRUE;

    unsigned tid;
    s_fTerminateAllWatches = FALSE;
    s_hThreadAPC = (HANDLE)_beginthreadex(NULL, 0, DirWatchThreadFuncAPC, NULL, 0, &tid);
    return s_hThreadAPC != NULL;
}

// Message CN_REGISTER: Register the registration entry.
//   wParam: The handle of registration entry.
//   lParam: The owner PID of registration entry.
//   return: TRUE if successful.
LRESULT CChangeNotifyServer::OnRegister(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TRACE("OnRegister(%p, %u, %p, %p)\n", m_hWnd, uMsg, wParam, lParam);

    // lock the registration entry
    HANDLE hRegEntry = (HANDLE)wParam;
    DWORD dwOwnerPID = (DWORD)lParam;
    LPREGENTRY pRegEntry = (LPREGENTRY)SHLockSharedEx(hRegEntry, dwOwnerPID, TRUE);
    if (pRegEntry == NULL || pRegEntry->dwMagic != REGENTRY_MAGIC)
    {
        ERR("pRegEntry is invalid\n");
        SHUnlockShared(pRegEntry);
        return FALSE;
    }

    // update registration ID if necessary
    if (pRegEntry->nRegID == INVALID_REG_ID)
        pRegEntry->nRegID = GetNextRegID();

    TRACE("pRegEntry->nRegID: %u\n", pRegEntry->nRegID);

    // get the user PID; that is the process ID of the target window
    DWORD dwUserPID;
    GetWindowThreadProcessId(pRegEntry->hwnd, &dwUserPID);

    // get broker if any
    HWND hwndBroker = pRegEntry->hwndBroker;

    // clone the registration entry
    HANDLE hNewEntry = SHAllocShared(pRegEntry, pRegEntry->cbSize, dwOwnerPID);
    if (hNewEntry == NULL)
    {
        ERR("Out of memory\n");
        pRegEntry->nRegID = INVALID_REG_ID;
        SHUnlockShared(pRegEntry);
        return FALSE;
    }

    // create a directory watch if necessary
    DirWatch *pDirWatch = CreateDirWatchFromRegEntry(pRegEntry);
    if (pDirWatch)
    {
        // create an APC thread for directory watching
        if (!CreateAPCThread())
        {
            pRegEntry->nRegID = INVALID_REG_ID;
            SHUnlockShared(pRegEntry);
            delete pDirWatch;
            return FALSE;
        }

        // request adding the watch
        QueueUserAPC(_AddDirectoryProcAPC, s_hThreadAPC, (ULONG_PTR)pDirWatch);
    }

    // unlock the registry entry
    SHUnlockShared(pRegEntry);

    // add an ITEM
    return AddItem(m_nNextRegID, dwUserPID, hNewEntry, hwndBroker, pDirWatch);
}

// Message CN_UNREGISTER: Unregister registration entries.
//   wParam: The registration ID.
//   lParam: Ignored.
//   return: TRUE if successful.
LRESULT CChangeNotifyServer::OnUnRegister(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TRACE("OnUnRegister(%p, %u, %p, %p)\n", m_hWnd, uMsg, wParam, lParam);

    // validate registration ID
    UINT nRegID = (UINT)wParam;
    if (nRegID == INVALID_REG_ID)
    {
        ERR("INVALID_REG_ID\n");
        return FALSE;
    }

    // remove it
    DWORD dwOwnerPID;
    GetWindowThreadProcessId(m_hWnd, &dwOwnerPID);
    return RemoveItemsByRegID(nRegID, dwOwnerPID);
}

// Message CN_DELIVER_NOTIFICATION: Perform a delivery.
//   wParam: The handle of delivery ticket.
//   lParam: The owner PID of delivery ticket.
//   return: TRUE if necessary.
LRESULT CChangeNotifyServer::OnDeliverNotification(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TRACE("OnDeliverNotification(%p, %u, %p, %p)\n", m_hWnd, uMsg, wParam, lParam);

    HANDLE hTicket = (HANDLE)wParam;
    DWORD dwOwnerPID = (DWORD)lParam;

    // do delivery
    BOOL ret = DeliverNotification(hTicket, dwOwnerPID);

    // free the ticket
    SHFreeShared(hTicket, dwOwnerPID);
    return ret;
}

// Message CN_SUSPEND_RESUME: Suspend or resume the change notification.
//   (specification is unknown)
LRESULT CChangeNotifyServer::OnSuspendResume(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TRACE("OnSuspendResume\n");

    // FIXME
    return FALSE;
}

// Message CN_UNREGISTER_PROCESS: Remove registration entries by PID.
//   wParam: The user PID.
//   lParam: Ignored.
//   return: Zero.
LRESULT CChangeNotifyServer::OnRemoveByPID(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    DWORD dwOwnerPID, dwUserPID = (DWORD)wParam;
    GetWindowThreadProcessId(m_hWnd, &dwOwnerPID);
    RemoveItemsByProcess(dwOwnerPID, dwUserPID);
    return 0;
}

LRESULT CChangeNotifyServer::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (s_hThreadAPC)
    {
        // request termination of all directory watches
        QueueUserAPC(_RequestAllTerminationAPC, s_hThreadAPC, (ULONG_PTR)NULL);
    }
    return 0;
}

// get next valid registration ID
UINT CChangeNotifyServer::GetNextRegID()
{
    m_nNextRegID++;
    if (m_nNextRegID == INVALID_REG_ID)
        m_nNextRegID++;
    return m_nNextRegID;
}

// This function is called from CChangeNotifyServer::OnDeliverNotification.
// The function notifies to the registration entries that should be notified.
BOOL CChangeNotifyServer::DeliverNotification(HANDLE hTicket, DWORD dwOwnerPID)
{
    TRACE("DeliverNotification(%p, %p, 0x%lx)\n", m_hWnd, hTicket, dwOwnerPID);

    // lock the delivery ticket
    LPDELITICKET pTicket = (LPDELITICKET)SHLockSharedEx(hTicket, dwOwnerPID, FALSE);
    if (pTicket == NULL || pTicket->dwMagic != DELITICKET_MAGIC)
    {
        ERR("pTicket is invalid\n");
        SHUnlockShared(pTicket);
        return FALSE;
    }

    // for all items
    for (INT i = 0; i < m_items.GetSize(); ++i)
    {
        // validate the item
        if (m_items[i].nRegID == INVALID_REG_ID)
            continue;

        HANDLE hRegEntry = m_items[i].hRegEntry;
        if (hRegEntry == NULL)
            continue;

        // lock the registration entry
        LPREGENTRY pRegEntry = (LPREGENTRY)SHLockSharedEx(hRegEntry, dwOwnerPID, FALSE);
        if (pRegEntry == NULL || pRegEntry->dwMagic != REGENTRY_MAGIC)
        {
            ERR("pRegEntry is invalid\n");
            SHUnlockShared(pRegEntry);
            continue;
        }

        // should we notify for it?
        BOOL bNotify = ShouldNotify(pTicket, pRegEntry);
        if (bNotify)
        {
            // do notify
            TRACE("Notifying: %p, 0x%x, %p, %lu\n",
                  pRegEntry->hwnd, pRegEntry->uMsg, hTicket, dwOwnerPID);
            SendMessageW(pRegEntry->hwnd, pRegEntry->uMsg, (WPARAM)hTicket, dwOwnerPID);
        }

        // unlock the registration entry
        SHUnlockShared(pRegEntry);
    }

    // unlock the ticket
    SHUnlockShared(pTicket);

    return TRUE;
}

BOOL CChangeNotifyServer::ShouldNotify(LPDELITICKET pTicket, LPREGENTRY pRegEntry)
{
    LPITEMIDLIST pidl, pidl1 = NULL, pidl2 = NULL;
    WCHAR szPath[MAX_PATH], szPath1[MAX_PATH], szPath2[MAX_PATH];
    INT cch, cch1, cch2;

    // check fSources
    if (pTicket->uFlags & SHCNE_INTERRUPT)
    {
        if (!(pRegEntry->fSources & SHCNRF_InterruptLevel))
            return FALSE;
    }
    else
    {
        if (!(pRegEntry->fSources & SHCNRF_ShellLevel))
            return FALSE;
    }

    if (pRegEntry->ibPidl == 0)
        return TRUE; // there is no PIDL

    // get the stored pidl
    pidl = (LPITEMIDLIST)((LPBYTE)pRegEntry + pRegEntry->ibPidl);
    if (pidl->mkid.cb == 0 && pRegEntry->fRecursive)
        return TRUE;    // desktop is the root

    // check pidl1
    if (pTicket->ibOffset1)
    {
        pidl1 = (LPITEMIDLIST)((LPBYTE)pTicket + pTicket->ibOffset1);
        if (ILIsEqual(pidl, pidl1) || ILIsParent(pidl, pidl1, !pRegEntry->fRecursive))
            return TRUE;
    }

    // check pidl2
    if (pTicket->ibOffset2)
    {
        pidl2 = (LPITEMIDLIST)((LPBYTE)pTicket + pTicket->ibOffset2);
        if (ILIsEqual(pidl, pidl2) || ILIsParent(pidl, pidl2, !pRegEntry->fRecursive))
            return TRUE;
    }

    // The paths:
    //   "C:\\Path\\To\\File1"
    //   "C:\\Path\\To\\File1Test"
    // should be distinguished in comparison, so we add backslash at last as follows:
    //   "C:\\Path\\To\\File1\\"
    //   "C:\\Path\\To\\File1Test\\"
    if (SHGetPathFromIDListW(pidl, szPath))
    {
        PathAddBackslashW(szPath);
        cch = lstrlenW(szPath);

        if (pidl1 && SHGetPathFromIDListW(pidl1, szPath1))
        {
            PathAddBackslashW(szPath1);
            cch1 = lstrlenW(szPath1);

            // Is szPath1 a subfile or subdirectory of szPath?
            if (cch < cch1 &&
                (pRegEntry->fRecursive ||
                 wcschr(&szPath1[cch], L'\\') == &szPath1[cch1 - 1]))
            {
                szPath1[cch] = 0;
                if (lstrcmpiW(szPath, szPath1) == 0)
                    return TRUE;
            }
        }

        if (pidl2 && SHGetPathFromIDListW(pidl2, szPath2))
        {
            PathAddBackslashW(szPath2);
            cch2 = lstrlenW(szPath2);

            // Is szPath2 a subfile or subdirectory of szPath?
            if (cch < cch2 &&
                (pRegEntry->fRecursive ||
                 wcschr(&szPath2[cch], L'\\') == &szPath2[cch2 - 1]))
            {
                szPath2[cch] = 0;
                if (lstrcmpiW(szPath, szPath2) == 0)
                    return TRUE;
            }
        }
    }

    return FALSE;
}

HRESULT WINAPI CChangeNotifyServer::GetWindow(HWND* phwnd)
{
    if (!phwnd)
        return E_INVALIDARG;
    *phwnd = m_hWnd;
    return S_OK;
}

HRESULT WINAPI CChangeNotifyServer::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

HRESULT CChangeNotifyServer::Initialize()
{
    // This is called by CChangeNotifyServer_CreateInstance right after instantiation.
    // Create the window of the server here.
    Create(0);
    if (!m_hWnd)
        return E_FAIL;
    return S_OK;
}

HRESULT CChangeNotifyServer_CreateInstance(REFIID riid, void **ppv)
{
    return ShellObjectCreatorInit<CChangeNotifyServer>(riid, ppv);
}
