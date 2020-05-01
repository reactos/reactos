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

//////////////////////////////////////////////////////////////////////////////
// DIRLIST --- directory list

struct DIRLIST
{
    DWORD m_count;
    LPWSTR m_items[ANYSIZE_ARRAY];

    static DIRLIST *
    AddItem(DIRLIST *pList, LPCWSTR pszItem, BOOL fDir);

    static DIRLIST *
    GetDirList(DIRLIST *pList, LPCWSTR pszDir, BOOL fRecursive);

    ~DIRLIST();

    BOOL Contains(LPCWSTR pszPath, BOOL fDir) const;

    void RenameItem(LPCWSTR pszItem1, LPCWSTR pszItem2, BOOL fDir);
    void DeleteItem(LPCWSTR pszItem, BOOL fDir);

protected:
    DIRLIST()
    {
    }
};

DIRLIST::~DIRLIST()
{
    if (this)
    {
        for (DWORD i = 0; i < m_count; ++i)
        {
            free(m_items[i]);
        }
        free(this);
    }
}

BOOL DIRLIST::Contains(LPCWSTR pszPath, BOOL fDir) const
{
    if (this == NULL)
        return FALSE;

    for (DWORD i = 0; i < m_count; ++i)
    {
        if (m_items[i] == NULL)
            continue;

        if (fDir)
        {
            if (m_items[i][0] != L'|')
                continue;
        }
        else
        {
            if (m_items[i][0] != L'>')
                continue;
        }

        if (lstrcmpiW(&m_items[i][1], pszPath) == 0)
            return TRUE;
    }

    return FALSE;
}

/*static*/ DIRLIST *
DIRLIST::AddItem(DIRLIST *pList, LPCWSTR pszItem, BOOL fDir)
{
    SIZE_T cbDirList = sizeof(DIRLIST);
    if (pList)
        cbDirList += pList->m_count * sizeof(LPWSTR);

    DIRLIST *pNewList = (DIRLIST *)realloc(pList, cbDirList);
    if (pNewList == NULL)
        return pList;

    WCHAR szPath[MAX_PATH + 1];
    szPath[0] = fDir ? L'|' : L'>';
    lstrcpynW(&szPath[1], pszItem, _countof(szPath) - 1);

    pNewList->m_items[pNewList->m_count] = _wcsdup(szPath);
    pNewList->m_count++;
    return pNewList;
}

void DIRLIST::RenameItem(LPCWSTR pszItem1, LPCWSTR pszItem2, BOOL fDir)
{
    if (this == NULL)
        return;

    WCHAR szPath[MAX_PATH + 1];
    for (DWORD i = 0; i < m_count; ++i)
    {
        if (m_items[i] && lstrcmpiW(&m_items[i][1], pszItem1) == 0)
        {
            szPath[0] = fDir ? L'|' : L'>';
            lstrcpynW(&szPath[1], pszItem2, _countof(szPath) - 1);

            free(m_items[i]);
            m_items[i] = _wcsdup(szPath);
            return;
        }
    }
}

void DIRLIST::DeleteItem(LPCWSTR pszItem, BOOL fDir)
{
    if (this == NULL)
        return;

    for (DWORD i = 0; i < m_count; ++i)
    {
        if (m_items[i] && lstrcmpiW(&m_items[i][1], pszItem) == 0)
        {
            free(m_items[i]);
            m_items[i] = NULL;
            return;
        }
    }
}

/*static*/ DIRLIST *
DIRLIST::GetDirList(DIRLIST *pList, LPCWSTR pszDir, BOOL fRecursive)
{
    WCHAR szPath[MAX_PATH];
    GetFullPathNameW(pszDir, _countof(szPath), szPath, NULL);

    if (!PathIsDirectoryW(szPath))
    {
        delete pList;
        return NULL;
    }

    pList = AddItem(pList, szPath, TRUE);

    WIN32_FIND_DATAW find;
    PathAppendW(szPath, L"*");

    HANDLE hFind = FindFirstFileW(szPath, &find);
    if (hFind == INVALID_HANDLE_VALUE)
        return pList;

    do
    {
        if (lstrcmpW(find.cFileName, L".") == 0 ||
            lstrcmpW(find.cFileName, L"..") == 0)
        {
            continue;
        }

        PathRemoveFileSpecW(szPath);
        if (lstrlenW(szPath) + lstrlenW(find.cFileName) + 1 > MAX_PATH)
            continue;
        PathAppendW(szPath, find.cFileName);

        if (fRecursive && (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            pList = GetDirList(pList, szPath, fRecursive);
        }
        else
        {
            pList = AddItem(pList, szPath, FALSE);
        }
    } while (FindNextFileW(hFind, &find));

    FindClose(hFind);

    return pList;
}

//////////////////////////////////////////////////////////////////////////////
// DirWatch --- directory watcher using ReadDirectoryChangesW

static HANDLE s_hThread = NULL;
static BOOL s_fTerminateAll = FALSE;

static unsigned __stdcall DirWatchThreadFuncAPC(void *)
{
    while (!s_fTerminateAll)
    {
        SleepEx(INFINITE, TRUE);
    }
    return 0;
}

#define BUFFER_SIZE 0x1000
static BYTE s_abBuffer[BUFFER_SIZE];

class DirWatch
{
public:
    HANDLE m_hDir;
    WCHAR m_szDir[MAX_PATH];
    BOOL m_fDeadWatch;
    BOOL m_fRecursive;
    OVERLAPPED m_overlapped; // for async I/O
    DIRLIST *m_pDirList;

    static DirWatch *Create(LPCWSTR pszDir, BOOL fSubTree = FALSE)
    {
        DirWatch *pDirWatch = new DirWatch(pszDir, fSubTree);
        if (pDirWatch->m_hDir == INVALID_HANDLE_VALUE)
        {
            delete pDirWatch;
            pDirWatch = NULL;
        }
        return pDirWatch;
    }

    ~DirWatch()
    {
        if (m_hDir != INVALID_HANDLE_VALUE && m_hDir != NULL)
            CloseHandle(m_hDir);

        delete m_pDirList;
    }

protected:
    DirWatch(LPCWSTR pszDir, BOOL fSubTree)
    {
        m_fDeadWatch = FALSE;
        m_fRecursive = fSubTree;

        lstrcpynW(m_szDir, pszDir, MAX_PATH);

        m_hDir = CreateFileW(pszDir, FILE_LIST_DIRECTORY,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             NULL, OPEN_EXISTING,
                             FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                             NULL);
        m_pDirList = NULL;
    }
};

static BOOL _BeginRead(DirWatch *pDirWatch);

static void NTAPI _AddDirectoryProcAPC(ULONG_PTR Parameter)
{
    DirWatch *pDirWatch = (DirWatch *)Parameter;
    assert(pDirWatch != NULL);

    _BeginRead(pDirWatch);
}

static void NTAPI _RequestTerminationAPC(ULONG_PTR Parameter)
{
    DirWatch *pDirWatch = (DirWatch *)Parameter;
    assert(pDirWatch != NULL);

    pDirWatch->m_fDeadWatch = TRUE;
    CancelIo(pDirWatch->m_hDir);
}

static void NTAPI _RequestAllTerminationAPC(ULONG_PTR Parameter)
{
    s_fTerminateAll = TRUE;
}

static DWORD
TranslateActionToEvent(DWORD Action, BOOL fDir)
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

static void _ProcessNotification(DirWatch *pDirWatch)
{
    PFILE_NOTIFY_INFORMATION pInfo = (PFILE_NOTIFY_INFORMATION)s_abBuffer;
    WCHAR szName[MAX_PATH], szPath[MAX_PATH], szTempPath[MAX_PATH];
    DWORD dwEvent, cbName;
    LPWSTR psz1, psz2;
    BOOL fDir;

    szPath[0] = szTempPath[0] = 0;
    for (;;)
    {
        // get name
        // NOTE: FILE_NOTIFY_INFORMATION.FileName is not null-terminated.
        cbName = pInfo->FileNameLength;
        if (sizeof(szName) - sizeof(UNICODE_NULL) < cbName)
        {
            break;
        }
        ZeroMemory(szName, sizeof(szName));
        CopyMemory(szName, pInfo->FileName, cbName);

        if (pDirWatch->m_fRecursive)
        {
            SHChangeNotify(SHCNE_UPDATEDIR | SHCNE_INTERRUPT, SHCNF_PATHW,
                           pDirWatch->m_szDir, NULL);

            if (pInfo->NextEntryOffset == 0)
                break;

            // get next
            pInfo = (PFILE_NOTIFY_INFORMATION)((LPBYTE)pInfo + pInfo->NextEntryOffset);
            continue;
        }

        // get full path
        lstrcpynW(szPath, pDirWatch->m_szDir, _countof(szPath));
        PathAppendW(szPath, szName);

        // convert action to event
        fDir = PathIsRootW(szPath) || PathIsDirectoryW(szPath);
        dwEvent = TranslateActionToEvent(pInfo->Action, fDir);

        DIRLIST*& pList = pDirWatch->m_pDirList;
        if (!fDir && dwEvent == SHCNE_DELETE)
        {
            if (pList && pList->Contains(szPath, TRUE))
            {
                fDir = TRUE;
                dwEvent = SHCNE_RMDIR;
            }
        }

        switch (dwEvent)
        {
            case SHCNE_MKDIR:
                pList = DIRLIST::AddItem(pList, szPath, TRUE);
                break;
            case SHCNE_CREATE:
                pList = DIRLIST::AddItem(pList, szPath, FALSE);
                break;
            case SHCNE_RENAMEFOLDER:
                pList->RenameItem(szTempPath, szPath, TRUE);
                break;
            case SHCNE_RENAMEITEM:
                pList->RenameItem(szTempPath, szPath, FALSE);
                break;
            case SHCNE_RMDIR:
                pList->DeleteItem(szPath, TRUE);
                break;
        }

        if (dwEvent)
        {
            // notify
            if (pInfo->Action == FILE_ACTION_RENAMED_NEW_NAME)
            {
                psz1 = szTempPath;
                psz2 = szPath;
            }
            else
            {
                psz1 = szPath;
                psz2 = NULL;
            }
            SHChangeNotify(dwEvent | SHCNE_INTERRUPT, SHCNF_PATHW, psz1, psz2);
        }
        else
        {
            if (pInfo->Action == FILE_ACTION_RENAMED_OLD_NAME)
            {
                lstrcpynW(szTempPath, szPath, MAX_PATH);
            }
        }

        if (pInfo->NextEntryOffset == 0)
            break;

        // get next
        pInfo = (PFILE_NOTIFY_INFORMATION)((LPBYTE)pInfo + pInfo->NextEntryOffset);
    }
}

static void CALLBACK
_NotificationCompletion(DWORD dwErrorCode,
                        DWORD dwNumberOfBytesTransfered,
                        LPOVERLAPPED lpOverlapped)
{
    DirWatch *pDirWatch = (DirWatch *)lpOverlapped->hEvent;
    assert(pDirWatch != NULL);

    if (dwErrorCode == ERROR_INVALID_FUNCTION)
    {
        return;
    }

    if (dwErrorCode == ERROR_OPERATION_ABORTED ||
        pDirWatch->m_fDeadWatch ||
        dwNumberOfBytesTransfered == 0)
    {
        delete pDirWatch;
        return;
    }

    _ProcessNotification(pDirWatch);

    _BeginRead(pDirWatch);
}

// convert events to notification filter
static DWORD
GetFilterFromEvents(DWORD fEvents)
{
    return (FILE_NOTIFY_CHANGE_FILE_NAME |
            FILE_NOTIFY_CHANGE_DIR_NAME |
            FILE_NOTIFY_CHANGE_CREATION);
}

static BOOL _BeginRead(DirWatch *pDirWatch)
{
    assert(pDirWatch != NULL);

    if (pDirWatch->m_fDeadWatch)
        return FALSE;

    ZeroMemory(&pDirWatch->m_overlapped, sizeof(pDirWatch->m_overlapped));
    pDirWatch->m_overlapped.hEvent = (HANDLE)pDirWatch;

    DWORD dwFilter = GetFilterFromEvents(SHCNE_ALLEVENTS);
    if (!ReadDirectoryChangesW(pDirWatch->m_hDir, s_abBuffer, sizeof(s_abBuffer),
                               pDirWatch->m_fRecursive, dwFilter, NULL,
                               &pDirWatch->m_overlapped, _NotificationCompletion))
    {
        return FALSE;
    }

    return TRUE;
}

static DirWatch *
CreateDirWatch(LPREGENTRY pRegEntry)
{
    if (pRegEntry->ibPidl == 0 || pRegEntry->fEvents == 0)
        return NULL;

    if (!(pRegEntry->fSources & SHCNRF_InterruptLevel))
        return NULL;

    // get the path
    WCHAR szPath[MAX_PATH];
    LPITEMIDLIST pidl = (LPITEMIDLIST)((LPBYTE)pRegEntry + pRegEntry->ibPidl);
    if (!SHGetPathFromIDListW(pidl, szPath) ||
        (!PathIsDirectoryW(szPath) && !PathIsRootW(szPath)))
    {
        return NULL;
    }

    DirWatch *pDirWatch = DirWatch::Create(szPath, FALSE);
    if (pDirWatch == NULL)
        return NULL;

    pDirWatch->m_pDirList = DIRLIST::GetDirList(NULL, szPath, FALSE);

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
    DirWatch *pDirWatch; // for filesystem notification (SHCNRF_InterruptLevel)
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

    DirWatch *pDirWatch = item.pDirWatch;
    item.pDirWatch = NULL;
    if (pDirWatch && s_hThread)
    {
        QueueUserAPC(_RequestTerminationAPC, s_hThread, (ULONG_PTR)pDirWatch);
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

    DirWatch *pDirWatch = CreateDirWatch(pRegEntry);
    if (pDirWatch)
    {
        if (s_hThread == NULL)
        {
            unsigned tid;
            s_fTerminateAll = FALSE;
            s_hThread = (HANDLE)_beginthreadex(NULL, 0, DirWatchThreadFuncAPC, NULL, 0, &tid);
        }
        if (s_hThread == NULL)
        {
            pRegEntry->nRegID = INVALID_REG_ID;
            SHUnlockShared(pRegEntry);
            return FALSE;
        }

        QueueUserAPC(_AddDirectoryProcAPC, s_hThread, (ULONG_PTR)pDirWatch);
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
    if (s_hThread)
    {
        QueueUserAPC(_RequestAllTerminationAPC, s_hThread, (ULONG_PTR)NULL);
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

    if (pRegEntry->ibPidl == 0)
        return TRUE;

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
