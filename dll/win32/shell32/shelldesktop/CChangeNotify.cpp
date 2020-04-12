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
// The shared memory block can be allocated by SHAllocShared function.
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
DoGetNewDeliveryWorker(void)
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
    HWND hwndWorker = (HWND)SendMessageW(hwndShell, WM_GETDELIWORKERWND, 0, 0);
    if (!IsWindow(hwndWorker))
    {
        ERR("Unable to get notification window\n");
        hwndWorker = NULL;
    }

    // save and return
    s_hwndNewWorker = hwndWorker;
    return hwndWorker;
}

// This function creates a registration entry in SHChangeNotifyRegister function.
EXTERN_C HANDLE
DoCreateRegEntry(ULONG nRegID, HWND hwnd, UINT wMsg, INT fSources, LONG fEvents,
                 LONG fRecursive, LPCITEMIDLIST pidl, DWORD dwOwnerPID,
                 HWND hwndOldWorker)
{
    // pidl has variable length. To store it into the registration entry,
    // we have to consider the length of pidl.
    DWORD cbPidl = ILGetSize(pidl);
    DWORD ibPidl = DWORD_ALIGNMENT(sizeof(REGENTRY));
    DWORD cbSize = ibPidl + cbPidl;
    HANDLE hShared = SHAllocShared(NULL, cbSize, dwOwnerPID);
    if (!hShared)
    {
        ERR("Out of memory\n");
        return NULL;
    }

    // create the registration entry and lock it
    LPREGENTRY pShared = (LPREGENTRY)SHLockSharedEx(hShared, dwOwnerPID, TRUE);
    if (pShared == NULL)
    {
        ERR("SHLockSharedEx failed\n");
        SHFreeShared(hShared, dwOwnerPID);
        return NULL;
    }

    // populate the registration entry
    pShared->dwMagic = REGENTRY_MAGIC;
    pShared->cbSize = cbSize;
    pShared->nRegID = nRegID;
    pShared->hwnd = hwnd;
    pShared->uMsg = wMsg;
    pShared->fSources = fSources;
    pShared->fEvents = fEvents;
    pShared->fRecursive = fRecursive;
    pShared->hwndOldWorker = hwndOldWorker;
    pShared->ibPidl = 0;
    if (pidl)
    {
        pShared->ibPidl = ibPidl;
        memcpy((LPBYTE)pShared + ibPidl, pidl, cbPidl);
    }

    // unlock and return
    SHUnlockShared(pShared);
    return hShared;
}

// This function creates a "handbag" by using a delivery ticket.
// The handbag is used in SHChangeNotification_Lock and OnDelivery.
// hTicket is a ticket handle of a shared memory block and dwOwnerPID is
// the owner PID of the ticket.
EXTERN_C LPHANDBAG
DoGetHandbagFromTicket(HANDLE hTicket, DWORD dwOwnerPID)
{
    // validate the delivery ticket
    LPDELITICKET pTicket = (LPDELITICKET)SHLockSharedEx(hTicket, dwOwnerPID, FALSE);
    if (!pTicket || pTicket->dwMagic != DELITICKET_MAGIC)
    {
        ERR("pTicket is invalid\n");
        return NULL;
    }

    // allocate the handbag
    LPHANDBAG pHandbag = (LPHANDBAG)LocalAlloc(LMEM_FIXED, sizeof(HANDBAG));
    if (pHandbag == NULL)
    {
        ERR("Out of memory\n");
        SHUnlockShared(pTicket);
        return NULL;
    }

    // populate the handbag
    pHandbag->dwMagic = HANDBAG_MAGIC;
    pHandbag->pTicket = pTicket;

    pHandbag->pidls[0] = pHandbag->pidls[1] = NULL;
    if (pTicket->ibOffset1)
        pHandbag->pidls[0] = (LPITEMIDLIST)((LPBYTE)pTicket + pTicket->ibOffset1);
    if (pTicket->ibOffset2)
        pHandbag->pidls[1] = (LPITEMIDLIST)((LPBYTE)pTicket + pTicket->ibOffset2);

    return pHandbag;
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
    struct ITEM
    {
        UINT nRegID;
        DWORD dwUserPID;
        HANDLE hShare;
        HWND hwndOldWorker;
    };
    CSimpleArray<ITEM> m_items;

    BOOL AddItem(UINT nRegID, DWORD dwUserPID, HANDLE hShare, HWND hwndOldWorker)
    {
        for (INT i = 0; i < m_items.GetSize(); ++i)
        {
            if (m_items[i].nRegID == INVALID_REG_ID)
            {
                m_items[i].nRegID = nRegID;
                m_items[i].dwUserPID = dwUserPID;
                m_items[i].hShare = hShare;
                m_items[i].hwndOldWorker = hwndOldWorker;
                return TRUE;
            }
        }

        ITEM item = { nRegID, dwUserPID, hShare, hwndOldWorker };
        m_items.Add(item);
        return TRUE;
    }

    void DestroyItem(ITEM& item, DWORD dwOwnerPID, HWND *phwndOldWorker)
    {
        if (item.hwndOldWorker && item.hwndOldWorker != *phwndOldWorker)
        {
            DestroyWindow(item.hwndOldWorker);
            *phwndOldWorker = item.hwndOldWorker;
        }

        SHFreeShared(item.hShare, dwOwnerPID);
        item.nRegID = INVALID_REG_ID;
        item.dwUserPID = 0;
        item.hShare = NULL;
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

BOOL CChangeNotify::AddItem(UINT nRegID, DWORD dwUserPID, HANDLE hShare, HWND hwndOldWorker)
{
    return m_pimpl->AddItem(nRegID, dwUserPID, hShare, hwndOldWorker);
}

BOOL CChangeNotify::RemoveItemsByRegID(UINT nRegID, DWORD dwOwnerPID)
{
    return m_pimpl->RemoveItemsByRegID(nRegID, dwOwnerPID);
}

void CChangeNotify::RemoveItemsByProcess(DWORD dwOwnerPID, DWORD dwUserPID)
{
    m_pimpl->RemoveItemsByProcess(dwOwnerPID, dwUserPID);
}

// Message WM_WORKER_REGISTER:
//   wParam: The handle of registration entry.
//   lParam: The owner PID of registration entry.
//   return: TRUE if successful.
LRESULT CChangeNotify::OnRegister(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TRACE("OnReg(%p, %u, %p, %p)\n", m_hWnd, uMsg, wParam, lParam);

    // lock the registration entry
    HANDLE hShared = (HANDLE)wParam;
    DWORD dwOwnerPID = (DWORD)lParam;
    LPREGENTRY pShared = (LPREGENTRY)SHLockSharedEx(hShared, dwOwnerPID, TRUE);
    if (!pShared || pShared->dwMagic != REGENTRY_MAGIC)
    {
        ERR("pShared is invalid\n");
        return FALSE;
    }

    // update registration ID if necessary
    if (pShared->nRegID == INVALID_REG_ID)
        pShared->nRegID = GetNextRegID();

    TRACE("pShared->nRegID: %u\n", pShared->nRegID);

    // get the user PID; that is the process ID of the target window
    DWORD dwUserPID;
    GetWindowThreadProcessId(pShared->hwnd, &dwUserPID);

    // get old worker if any
    HWND hwndOldWorker = pShared->hwndOldWorker;

    // clone the registration entry
    HANDLE hNewShared = SHAllocShared(pShared, pShared->cbSize, dwOwnerPID);
    if (!hNewShared)
    {
        ERR("Out of memory\n");
        pShared->nRegID = INVALID_REG_ID;
        SHUnlockShared(pShared);
        return FALSE;
    }

    // unlock the registry entry
    SHUnlockShared(pShared);

    // add an ITEM
    return AddItem(m_nNextRegID, dwUserPID, hNewShared, hwndOldWorker);
}

// Message WM_WORKER_UNREGISTER:
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

// Message WM_WORKER_DELIVERY:
//   wParam: The handle of delivery ticket.
//   lParam: The owner PID of delivery ticket.
//   return: TRUE if necessary.
LRESULT CChangeNotify::OnDelivery(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TRACE("OnDelivery(%p, %u, %p, %p)\n", m_hWnd, uMsg, wParam, lParam);

    HANDLE hTicket = (HANDLE)wParam;
    DWORD dwOwnerPID = (DWORD)lParam;
    BOOL ret = FALSE;

    // create a handbag from the delivery ticket
    LPHANDBAG pHandbag = DoGetHandbagFromTicket(hTicket, dwOwnerPID);
    if (pHandbag && pHandbag->dwMagic == HANDBAG_MAGIC)
    {
        // validate the handbag
        LPDELITICKET pTicket = pHandbag->pTicket;
        if (pTicket && pTicket->dwMagic == DELITICKET_MAGIC)
        {
            // do delivery
            ret = DoDelivery(hTicket, dwOwnerPID);
        }
    }

    // unlock and free the handbag
    SHChangeNotification_Unlock(pHandbag);
    SHFreeShared(hTicket, dwOwnerPID);
    return ret;
}

// Message WM_WORKER_SUSPEND:
//   (specification is unknown)
LRESULT CChangeNotify::OnSuspendResume(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TRACE("OnSuspendResume\n");

    // FIXME
    return FALSE;
}

// Message WM_WORKER_REMOVEBYPID:
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

UINT CChangeNotify::GetNextRegID()
{
    m_nNextRegID++;
    if (m_nNextRegID == INVALID_REG_ID)
        m_nNextRegID++;
    return m_nNextRegID;
}

// This function is called from CChangeNotify::OnDelivery.
// The function checks all the registration entries whether the entry
// should be notified.
BOOL CChangeNotify::DoDelivery(HANDLE hTicket, DWORD dwOwnerPID)
{
    TRACE("DoDelivery(%p, %p, 0x%lx)\n", m_hWnd, hTicket, dwOwnerPID);

    // lock the delivery ticket
    LPDELITICKET pTicket = (LPDELITICKET)SHLockSharedEx(hTicket, dwOwnerPID, FALSE);
    if (!pTicket || pTicket->dwMagic != DELITICKET_MAGIC)
    {
        ERR("pTicket is invalid\n");
        return FALSE;
    }

    // for all items
    for (INT i = 0; i < m_pimpl->m_items.GetSize(); ++i)
    {
        // validate the item
        HANDLE hShare = m_pimpl->m_items[i].hShare;
        if (m_pimpl->m_items[i].nRegID == INVALID_REG_ID || !hShare)
            continue;

        // lock the registration entry
        LPREGENTRY pShared = (LPREGENTRY)SHLockSharedEx(hShare, dwOwnerPID, FALSE);
        if (!pShared || pShared->dwMagic != REGENTRY_MAGIC)
        {
            ERR("pShared is invalid\n");
            continue;
        }

        // should we notify for it?
        BOOL bNotify = ShouldNotify(pTicket, pShared);
        if (bNotify)
        {
            // do notify
            TRACE("Notifying: %p, 0x%x, %p, %lu\n",
                  pShared->hwnd, pShared->uMsg, hTicket, dwOwnerPID);
            SendMessageW(pShared->hwnd, pShared->uMsg, (WPARAM)hTicket, dwOwnerPID);
        }

        // unlock the registration entry
        SHUnlockShared(pShared);
    }

    // unlock the ticket
    SHUnlockShared(pTicket);

    return TRUE;
}

BOOL CChangeNotify::ShouldNotify(LPDELITICKET pTicket, LPREGENTRY pShared)
{
    LPITEMIDLIST pidl, pidl1 = NULL, pidl2 = NULL;
    WCHAR szPath[MAX_PATH], szPath1[MAX_PATH], szPath2[MAX_PATH];
    INT cch, cch1, cch2;

    if (!pShared->ibPidl)
        return TRUE;

    // get the stored pidl
    pidl = (LPITEMIDLIST)((LPBYTE)pShared + pShared->ibPidl);
    if (pidl->mkid.cb == 0 && pShared->fRecursive)
        return TRUE;    // desktop is the root

    // check pidl1
    if (pTicket->ibOffset1)
    {
        pidl1 = (LPITEMIDLIST)((LPBYTE)pTicket + pTicket->ibOffset1);
        if (ILIsEqual(pidl, pidl1) || ILIsParent(pidl, pidl1, !pShared->fRecursive))
            return TRUE;
    }

    // check pidl2
    if (pTicket->ibOffset2)
    {
        pidl2 = (LPITEMIDLIST)((LPBYTE)pTicket + pTicket->ibOffset2);
        if (ILIsEqual(pidl, pidl2) || ILIsParent(pidl, pidl2, !pShared->fRecursive))
            return TRUE;
    }

    // The paths:
    //   "C:\\Path\\To\\File1"
    //   "C:\\Path\\To\\File1Test"
    // should be distinguished, so we add backslash at last as follows:
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

            // Is szPath1 a subdirectory of szPath?
            if (cch < cch1 &&
                (pShared->fRecursive ||
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

            // Is szPath2 a subdirectory of szPath?
            if (cch < cch2 &&
                (pShared->fRecursive ||
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
