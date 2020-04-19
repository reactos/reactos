/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Shell change notification
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include "shelldesktop.h"
#include "shlwapi_undoc.h"
#include <atlsimpcoll.h>
#include <assert.h>

WINE_DEFAULT_DEBUG_CHANNEL(shcn);

// notification target item
struct ITEM
{
    UINT nRegID;        // The registration ID.
    DWORD dwUserPID;    // The user PID; that is the process ID of the target window.
    HANDLE hRegEntry;   // The registration entry.
    HWND hwndBroker;    // Client broker window (if any).
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
    END_MSG_MAP()

private:
    UINT m_nNextRegID;
    CSimpleArray<ITEM> m_items;

    BOOL AddItem(UINT nRegID, DWORD dwUserPID, HANDLE hRegEntry, HWND hwndBroker);
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

BOOL CChangeNotifyServer::AddItem(UINT nRegID, DWORD dwUserPID, HANDLE hRegEntry, HWND hwndBroker)
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
            return TRUE;
        }
    }

    // no empty room found
    ITEM item = { nRegID, dwUserPID, hRegEntry, hwndBroker };
    m_items.Add(item);
    return TRUE;
}

void CChangeNotifyServer::DestroyItem(ITEM& item, DWORD dwOwnerPID, HWND *phwndBroker)
{
    // destroy broker if any and if first time
    if (item.hwndBroker && item.hwndBroker != *phwndBroker)
    {
        ::DestroyWindow(item.hwndBroker);
        *phwndBroker = item.hwndBroker;
    }

    // free
    SHFreeShared(item.hRegEntry, dwOwnerPID);
    item.nRegID = INVALID_REG_ID;
    item.dwUserPID = 0;
    item.hRegEntry = NULL;
    item.hwndBroker = NULL;
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

    // unlock the registry entry
    SHUnlockShared(pRegEntry);

    // add an ITEM
    return AddItem(m_nNextRegID, dwUserPID, hNewEntry, hwndBroker);
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
