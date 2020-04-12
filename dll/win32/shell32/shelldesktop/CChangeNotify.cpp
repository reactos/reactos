/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Shell change notification
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include "shelldesktop.h"
#include "shlwapi_undoc.h"
#include <atlsimpcoll.h>

WINE_DEFAULT_DEBUG_CHANNEL(shcn);

/////////////////////////////////////////////////////////////////////////////
// CChangeNotify is a delivery worker window that is managed by CDesktopBrowser.
// The process of CChangeNotify is same as the process of CDesktopBrowser.
// The caller process of SHChangeNotify function might be different from the
// process of CChangeNotify.
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// The shared memory block can be allocated by shlwapi!SHAllocShared function.
//
// HANDLE SHAllocShared(LPCVOID lpData, DWORD dwSize, DWORD dwProcessId);
// LPVOID SHLockShared(HANDLE hData, DWORD dwProcessId);
// LPVOID SHLockSharedEx(HANDLE hData, DWORD dwProcessId, BOOL bWriteAccess);
// BOOL SHUnlockShared(LPVOID lpData);
// BOOL SHFreeShared(HANDLE hData, DWORD dwProcessId);
//
// The shared memory block is managed by the pair of a HANDLE value and an owner PID.
// If the pair is known, it can be accessed by SHLockShared(Ex) function
// from another process.
/////////////////////////////////////////////////////////////////////////////

// This function requests creation of the new delivery worker if necessary
// and returns the window handle of the new delivery worker with cached.
EXTERN_C HWND
DoGetNewDeliveryWorker(BOOL bCreate)
{
    static HWND s_hwndNewWorker = NULL;

    // use cache if any
    if (s_hwndNewWorker && IsWindow(s_hwndNewWorker))
        return s_hwndNewWorker;

    // get the shell window
    HWND hwndShell = GetShellWindow();
    if (hwndShell == NULL)
    {
        TRACE("GetShellWindow() returned NULL\n");
        return NULL;
    }

    // Request delivery worker to the shell window. See also CDesktopBrowser.
    HWND hwndWorker = (HWND)SendMessageW(hwndShell, WM_SHELL_GETWORKERWND, bCreate, 0);
    if (!IsWindow(hwndWorker))
    {
        ERR("Unable to get worker window\n");
        hwndWorker = NULL;
    }

    // save and return
    s_hwndNewWorker = hwndWorker;
    return hwndWorker;
}

/*static*/ LRESULT CALLBACK
CWorker::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CWorker *pThis = (CWorker *)GetWindowLongPtrW(hwnd, 0);
    if (pThis)
    {
        LRESULT lResult = 0;
        pThis->ProcessWindowMessage(hwnd, uMsg, wParam, lParam, lResult, 0);
        return lResult;
    }
    return 0;
}

BOOL CWorker::CreateWorker(HWND hwndParent, DWORD dwExStyle, DWORD dwStyle)
{
    // See also: shlwapi!SHCreateWorkerWindowW
    if (::IsWindow(m_hWnd))
        ::DestroyWindow(m_hWnd);
    m_hWnd = SHCreateWorkerWindowW(WindowProc, hwndParent, dwExStyle, dwStyle,
                                   NULL, (LONG_PTR)this);
    return m_hWnd != NULL;
}

//////////////////////////////////////////////////////////////////////////////
// CChangeNotifyImpl realizes "pimpl idiom" to hide implementation from the
// header.

struct CChangeNotifyImpl
{
    // notification target item
    struct ITEM
    {
        UINT nRegID;        // The registration ID.
        DWORD dwUserPID;    // The user PID; that is the process ID of the target window.
        HANDLE hRegEntry;   // The registration entry.
        HWND hwndOldWorker; // The old delivery worker (if any).
    };

    CSimpleArray<ITEM> m_items;

    BOOL AddItem(UINT nRegID, DWORD dwUserPID, HANDLE hRegEntry, HWND hwndOldWorker)
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
                m_items[i].hwndOldWorker = hwndOldWorker;
                return TRUE;
            }
        }

        // no empty room found
        ITEM item = { nRegID, dwUserPID, hRegEntry, hwndOldWorker };
        m_items.Add(item);
        return TRUE;
    }

    void DestroyItem(ITEM& item, DWORD dwOwnerPID, HWND *phwndOldWorker)
    {
        // destroy old worker if any and if first time
        if (item.hwndOldWorker && item.hwndOldWorker != *phwndOldWorker)
        {
            DestroyWindow(item.hwndOldWorker);
            *phwndOldWorker = item.hwndOldWorker;
        }

        SHFreeShared(item.hRegEntry, dwOwnerPID);
        item.nRegID = INVALID_REG_ID;
        item.dwUserPID = 0;
        item.hRegEntry = NULL;
        item.hwndOldWorker = NULL;
    }

    BOOL RemoveItemsByRegID(UINT nRegID, DWORD dwOwnerPID)
    {
        BOOL bFound = FALSE;
        HWND hwndOldWorker = NULL;
        for (INT i = 0; i < m_items.GetSize(); ++i)
        {
            if (m_items[i].nRegID == nRegID)
            {
                bFound = TRUE;
                DestroyItem(m_items[i], dwOwnerPID, &hwndOldWorker);
            }
        }
        return bFound;
    }

    void RemoveItemsByProcess(DWORD dwOwnerPID, DWORD dwUserPID)
    {
        HWND hwndOldWorker = NULL;
        for (INT i = 0; i < m_items.GetSize(); ++i)
        {
            if (m_items[i].dwUserPID == dwUserPID)
            {
                DestroyItem(m_items[i], dwOwnerPID, &hwndOldWorker);
            }
        }
    }
};

//////////////////////////////////////////////////////////////////////////////
// CChangeNotify
//
// CChangeNotify class is a class for the "new delivery worker" window.
// New delivery worker is created and used at shell window.
// See also CDesktopBrowser.

CChangeNotify::CChangeNotify()
    : m_nNextRegID(INVALID_REG_ID)
    , m_pimpl(new CChangeNotifyImpl)
{
}

CChangeNotify::~CChangeNotify()
{
    delete m_pimpl;
}

BOOL CChangeNotify::AddItem(UINT nRegID, DWORD dwUserPID, HANDLE hRegEntry, HWND hwndOldWorker)
{
    return m_pimpl->AddItem(nRegID, dwUserPID, hRegEntry, hwndOldWorker);
}

BOOL CChangeNotify::RemoveItemsByRegID(UINT nRegID, DWORD dwOwnerPID)
{
    return m_pimpl->RemoveItemsByRegID(nRegID, dwOwnerPID);
}

void CChangeNotify::RemoveItemsByProcess(DWORD dwOwnerPID, DWORD dwUserPID)
{
    m_pimpl->RemoveItemsByProcess(dwOwnerPID, dwUserPID);
}

// Message WM_WORKER_REGISTER: Register the registration entry.
//   wParam: The handle of registration entry.
//   lParam: The owner PID of registration entry.
//   return: TRUE if successful.
LRESULT CChangeNotify::OnRegister(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TRACE("OnReg(%p, %u, %p, %p)\n", m_hWnd, uMsg, wParam, lParam);

    // lock the registration entry
    HANDLE hRegEntry = (HANDLE)wParam;
    DWORD dwOwnerPID = (DWORD)lParam;
    LPREGENTRY pRegEntry = (LPREGENTRY)SHLockSharedEx(hRegEntry, dwOwnerPID, TRUE);
    if (pRegEntry == NULL || pRegEntry->dwMagic != REGENTRY_MAGIC)
    {
        ERR("pRegEntry is invalid\n");
        return FALSE;
    }

    // update registration ID if necessary
    if (pRegEntry->nRegID == INVALID_REG_ID)
        pRegEntry->nRegID = GetNextRegID();

    TRACE("pRegEntry->nRegID: %u\n", pRegEntry->nRegID);

    // get the user PID; that is the process ID of the target window
    DWORD dwUserPID;
    GetWindowThreadProcessId(pRegEntry->hwnd, &dwUserPID);

    // get old worker if any
    HWND hwndOldWorker = pRegEntry->hwndOldWorker;

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
    return AddItem(m_nNextRegID, dwUserPID, hNewEntry, hwndOldWorker);
}

// Message WM_WORKER_UNREGISTER: Unregister registration entries.
//   wParam: The registration ID.
//   lParam: Ignored.
//   return: TRUE if successful.
LRESULT CChangeNotify::OnUnRegister(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
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

// Message WM_WORKER_TICKET: Perform a delivery.
//   wParam: The handle of delivery ticket.
//   lParam: The owner PID of delivery ticket.
//   return: TRUE if necessary.
LRESULT CChangeNotify::OnTicket(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TRACE("OnTicket(%p, %u, %p, %p)\n", m_hWnd, uMsg, wParam, lParam);

    HANDLE hTicket = (HANDLE)wParam;
    DWORD dwOwnerPID = (DWORD)lParam;

    // do delivery
    BOOL ret = DoTicket(hTicket, dwOwnerPID);

    // free the ticket
    SHFreeShared(hTicket, dwOwnerPID);
    return ret;
}

// Message WM_WORKER_SUSPEND: Suspend or resume the change notification.
//   (specification is unknown)
LRESULT CChangeNotify::OnSuspendResume(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TRACE("OnSuspendResume\n");

    // FIXME
    return FALSE;
}

// Message WM_WORKER_REMOVEBYPID: Remove registration entries by PID.
//   wParam: The user PID.
//   lParam: Ignored.
//   return: Zero.
LRESULT CChangeNotify::OnRemoveByPID(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    DWORD dwOwnerPID, dwUserPID = (DWORD)wParam;
    GetWindowThreadProcessId(m_hWnd, &dwOwnerPID);
    RemoveItemsByProcess(dwOwnerPID, dwUserPID);
    return 0;
}

// get next valid registration ID
UINT CChangeNotify::GetNextRegID()
{
    m_nNextRegID++;
    if (m_nNextRegID == INVALID_REG_ID)
        m_nNextRegID++;
    return m_nNextRegID;
}

// This function is called from CChangeNotify::OnTicket.
// The function checks all the registration entries whether the entry
// should be notified.
BOOL CChangeNotify::DoTicket(HANDLE hTicket, DWORD dwOwnerPID)
{
    TRACE("DoTicket(%p, %p, 0x%lx)\n", m_hWnd, hTicket, dwOwnerPID);

    // lock the delivery ticket
    LPDELITICKET pTicket = (LPDELITICKET)SHLockSharedEx(hTicket, dwOwnerPID, FALSE);
    if (pTicket == NULL || pTicket->dwMagic != DELITICKET_MAGIC)
    {
        ERR("pTicket is invalid\n");
        return FALSE;
    }

    // for all items
    for (INT i = 0; i < m_pimpl->m_items.GetSize(); ++i)
    {
        // validate the item
        HANDLE hRegEntry = m_pimpl->m_items[i].hRegEntry;
        if (m_pimpl->m_items[i].nRegID == INVALID_REG_ID || hRegEntry == NULL)
            continue;

        // lock the registration entry
        LPREGENTRY pRegEntry = (LPREGENTRY)SHLockSharedEx(hRegEntry, dwOwnerPID, FALSE);
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
        }

        // unlock the registration entry
        SHUnlockShared(pRegEntry);
    }

    // unlock the ticket
    SHUnlockShared(pTicket);

    return TRUE;
}

BOOL CChangeNotify::ShouldNotify(LPDELITICKET pTicket, LPREGENTRY pRegEntry)
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
