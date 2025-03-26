/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Shell change notification
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include "shelldesktop.h"
#include "shlwapi_undoc.h"
#include "CDirectoryWatcher.h"
#include <assert.h>      // for assert

WINE_DEFAULT_DEBUG_CHANNEL(shcn);

//////////////////////////////////////////////////////////////////////////////

// notification target item
struct CWatchItem
{
    UINT nRegID;        // The registration ID.
    DWORD dwUserPID;    // The user PID; that is the process ID of the target window.
    LPREGENTRY pRegEntry;   // The registration entry.
    HWND hwndBroker;    // Client broker window (if any).
    CDirectoryWatcher *pDirWatch; // for filesystem notification
};

//////////////////////////////////////////////////////////////////////////////
// CChangeNotifyServer
//
// CChangeNotifyServer implements a window that handles all shell change notifications.
// It runs in the context of explorer and specifically in the thread of the shell desktop.
// Shell change notification api exported from shell32 forwards all their calls
// to this window where all processing takes place.

class CChangeNotifyServer :
    public CWindowImpl<CChangeNotifyServer, CWindow, CWorkerTraits>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IOleWindow
{
public:
    CChangeNotifyServer();
    virtual ~CChangeNotifyServer();
    HRESULT Initialize();

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow)(HWND *lphwnd) override;
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode) override;

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
    CSimpleArray<CWatchItem*> m_items;

    BOOL AddItem(CWatchItem *pItem);
    BOOL RemoveItemsByRegID(UINT nRegID);
    BOOL RemoveItemsByProcess(DWORD dwUserPID);
    void DestroyItem(CWatchItem *pItem, HWND *phwndBroker);
    void DestroyAllItems();

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

BOOL CChangeNotifyServer::AddItem(CWatchItem *pItem)
{
    // find the empty room
    for (INT i = 0; i < m_items.GetSize(); ++i)
    {
        if (m_items[i] == NULL)
        {
            // found the room, populate it
            m_items[i] = pItem;
            return TRUE;
        }
    }

    // no empty room found
    m_items.Add(pItem);
    return TRUE;
}

void CChangeNotifyServer::DestroyItem(CWatchItem *pItem, HWND *phwndBroker)
{
    assert(pItem);

    // destroy broker if any and if first time
    HWND hwndBroker = pItem->hwndBroker;
    pItem->hwndBroker = NULL;
    if (hwndBroker && hwndBroker != *phwndBroker)
    {
        ::DestroyWindow(hwndBroker);
        *phwndBroker = hwndBroker;
    }

    // request termination of pDirWatch if any
    CDirectoryWatcher *pDirWatch = pItem->pDirWatch;
    pItem->pDirWatch = NULL;
    if (pDirWatch)
        pDirWatch->RequestTermination();

    // free
    SHFree(pItem->pRegEntry);
    delete pItem;
}

void CChangeNotifyServer::DestroyAllItems()
{
    for (INT i = 0; i < m_items.GetSize(); ++i)
    {
        if (m_items[i])
        {
            HWND hwndBroker = NULL;
            DestroyItem(m_items[i], &hwndBroker);
            m_items[i] = NULL;
        }
    }
    m_items.RemoveAll();
}

BOOL CChangeNotifyServer::RemoveItemsByRegID(UINT nRegID)
{
    BOOL bFound = FALSE;
    HWND hwndBroker = NULL;
    assert(nRegID != INVALID_REG_ID);
    for (INT i = 0; i < m_items.GetSize(); ++i)
    {
        if (m_items[i] && m_items[i]->nRegID == nRegID)
        {
            bFound = TRUE;
            DestroyItem(m_items[i], &hwndBroker);
            m_items[i] = NULL;
        }
    }
    return bFound;
}

BOOL CChangeNotifyServer::RemoveItemsByProcess(DWORD dwUserPID)
{
    BOOL bFound = FALSE;
    HWND hwndBroker = NULL;
    assert(dwUserPID != 0);
    for (INT i = 0; i < m_items.GetSize(); ++i)
    {
        if (m_items[i] && m_items[i]->dwUserPID == dwUserPID)
        {
            bFound = TRUE;
            DestroyItem(m_items[i], &hwndBroker);
            m_items[i] = NULL;
        }
    }
    return bFound;
}

// create a CDirectoryWatcher from a REGENTRY
static CDirectoryWatcher *
CreateDirectoryWatcherFromRegEntry(LPREGENTRY pRegEntry)
{
    if (pRegEntry->ibPidl == 0)
        return NULL;

    // get the path
    WCHAR szPath[MAX_PATH];
    LPITEMIDLIST pidl = (LPITEMIDLIST)((LPBYTE)pRegEntry + pRegEntry->ibPidl);
    if (!SHGetPathFromIDListW(pidl, szPath) || !PathIsDirectoryW(szPath))
        return NULL;

    // create a CDirectoryWatcher
    CDirectoryWatcher *pDirectoryWatcher =
        CDirectoryWatcher::Create(pRegEntry->hwnd, szPath, pRegEntry->fRecursive);
    if (pDirectoryWatcher == NULL)
        return NULL;

    return pDirectoryWatcher;
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
    LPREGENTRY pNewEntry = (LPREGENTRY)SHAlloc(pRegEntry->cbSize);
    if (pNewEntry == NULL)
    {
        ERR("Out of memory\n");
        pRegEntry->nRegID = INVALID_REG_ID;
        SHUnlockShared(pRegEntry);
        return FALSE;
    }
    CopyMemory(pNewEntry, pRegEntry, pRegEntry->cbSize);

    // create a directory watch if necessary
    CDirectoryWatcher *pDirWatch = NULL;
    if (pRegEntry->ibPidl && (pRegEntry->fSources & SHCNRF_InterruptLevel))
    {
        pDirWatch = CreateDirectoryWatcherFromRegEntry(pRegEntry);
        if (pDirWatch && !pDirWatch->RequestAddWatcher())
        {
            ERR("RequestAddWatcher failed: %u\n", pRegEntry->nRegID);
            pRegEntry->nRegID = INVALID_REG_ID;
            SHUnlockShared(pRegEntry);
            delete pDirWatch;
            return FALSE;
        }
    }

    // unlock the registry entry
    SHUnlockShared(pRegEntry);

    // add an item
    CWatchItem *pItem = new CWatchItem { m_nNextRegID, dwUserPID, pNewEntry, hwndBroker, pDirWatch };
    return AddItem(pItem);
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
    return RemoveItemsByRegID(nRegID);
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
    DWORD dwUserPID = (DWORD)wParam;
    RemoveItemsByProcess(dwUserPID);
    return 0;
}

LRESULT CChangeNotifyServer::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    DestroyAllItems();
    CDirectoryWatcher::RequestAllWatchersTermination();
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
        if (m_items[i] == NULL)
            continue;

        LPREGENTRY pRegEntry = m_items[i]->pRegEntry;
        if (pRegEntry == NULL || pRegEntry->dwMagic != REGENTRY_MAGIC)
        {
            ERR("pRegEntry is invalid\n");
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
            TRACE("GetLastError(): %ld\n", ::GetLastError());
        }
    }

    // unlock the ticket
    SHUnlockShared(pTicket);

    return TRUE;
}

BOOL CChangeNotifyServer::ShouldNotify(LPDELITICKET pTicket, LPREGENTRY pRegEntry)
{
#define RETURN(x) do { \
    TRACE("ShouldNotify return %d\n", (x)); \
    return (x); \
} while (0)

    if (pTicket->wEventId & SHCNE_INTERRUPT)
    {
        if (!(pRegEntry->fSources & SHCNRF_InterruptLevel))
            RETURN(FALSE);
        if (!pRegEntry->ibPidl)
            RETURN(FALSE);
    }
    else
    {
        if (!(pRegEntry->fSources & SHCNRF_ShellLevel))
            RETURN(FALSE);
    }

    if (!(pTicket->wEventId & pRegEntry->fEvents))
        RETURN(FALSE);

    LPITEMIDLIST pidl = NULL, pidl1 = NULL, pidl2 = NULL;
    if (pRegEntry->ibPidl)
        pidl = (LPITEMIDLIST)((LPBYTE)pRegEntry + pRegEntry->ibPidl);
    if (pTicket->ibOffset1)
        pidl1 = (LPITEMIDLIST)((LPBYTE)pTicket + pTicket->ibOffset1);
    if (pTicket->ibOffset2)
        pidl2 = (LPITEMIDLIST)((LPBYTE)pTicket + pTicket->ibOffset2);

    if (pidl == NULL || (pTicket->wEventId & SHCNE_GLOBALEVENTS))
        RETURN(TRUE);

    if (pRegEntry->fRecursive)
    {
        if (ILIsParent(pidl, pidl1, FALSE) ||
            (pidl2 && ILIsParent(pidl, pidl2, FALSE)))
        {
            RETURN(TRUE);
        }
    }
    else
    {
        if (ILIsEqual(pidl, pidl1) ||
            ILIsParent(pidl, pidl1, TRUE) ||
            (pidl2 && ILIsParent(pidl, pidl2, TRUE)))
        {
            RETURN(TRUE);
        }
    }

    RETURN(FALSE);
#undef RETURN
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
    HWND hwnd = SHCreateDefaultWorkerWindow();
    if (!hwnd)
        return E_FAIL;
    SubclassWindow(hwnd);
    return S_OK;
}

HRESULT CChangeNotifyServer_CreateInstance(REFIID riid, void **ppv)
{
    return ShellObjectCreatorInit<CChangeNotifyServer>(riid, ppv);
}
