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

// This function handles the case of SHCNE_FREESPACE.
EXTERN_C void
DoNotifyFreeSpace(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    WCHAR path1[MAX_PATH], path2[MAX_PATH];

    path1[0] = 0;
    if (pidl1)
        SHGetPathFromIDListW(pidl1, path1);

    path2[0] = 0;
    if (pidl2)
        SHGetPathFromIDListW(pidl2, path2);

    if (path1[0])
    {
        if (path2[0])
            SHChangeNotify(SHCNE_FREESPACE, SHCNF_PATHW, path1, path2);
        else
            SHChangeNotify(SHCNE_FREESPACE, SHCNF_PATHW, path1, NULL);
    }
    else
    {
        SHChangeNotify(SHCNE_FREESPACE, SHCNF_PATHW, NULL, NULL);
    }
}

// This function creates the new delivery worker if necessary
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

    // request delivery worker to the shell window
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

typedef struct OLDDELIVERYWORKER
{
    HWND hwnd;
    UINT uMsg;
} OLDDELIVERYWORKER, *LPOLDDELIVERYWORKER;


// This is "old delivery worker" window. An old delivery worker will be
// created in the caller process. SHChangeNotification_Lock allocates
// a process-local memory block in response of WM_OLDDELI_HANDOVER, and
// send the pWorker->uMsg message.
static LRESULT CALLBACK
OldDeliveryWorkerWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HANDLE hLock;
    PIDLIST_ABSOLUTE *ppidl;
    LONG lEvent;
    LPOLDDELIVERYWORKER pWorker;
    HANDLE hShared;
    DWORD dwOwnerPID;

    switch (uMsg)
    {
        // Message WM_OLDDELI_HANDOVER:
        //    wParam: the handbag handle.
        //    lParam: the owner PID.
        //    return: TRUE if successful.
        case WM_OLDDELI_HANDOVER:
            hShared = (HANDLE)wParam;
            dwOwnerPID = (DWORD)lParam;
            TRACE("WM_OLDDELI_HANDOVER: hwnd:%p, hShared:%p, pid:0x%lx\n",
                  hwnd, hShared, dwOwnerPID);

            pWorker = (LPOLDDELIVERYWORKER)GetWindowLongPtrW(hwnd, 0);
            if (!pWorker)
            {
                ERR("!pWorker\n");
                break;
            }

            // lock the handbag
            ppidl = NULL;
            hLock = SHChangeNotification_Lock(hShared, dwOwnerPID, &ppidl, &lEvent);
            if (!hLock)
            {
                ERR("!hLock\n");
                break;
            }

            // perform the old delivery
            TRACE("OldDeliveryWorker notifying: %p, 0x%x, %p, 0x%lx\n",
                  pWorker->hwnd, pWorker->uMsg, ppidl, lEvent);
            SendMessageW(pWorker->hwnd, pWorker->uMsg, (WPARAM)ppidl, lEvent);

            // unlock the handbag
            SHChangeNotification_Unlock(hLock);
            return TRUE;

        case WM_NCDESTROY:
            TRACE("WM_NCDESTROY\n");
            pWorker = (LPOLDDELIVERYWORKER)GetWindowLongPtrW(hwnd, 0);
            SetWindowLongW(hwnd, 0, 0);
            delete pWorker;
            break;

        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// This function creates an old delivery worker. Used in SHChangeNotifyRegister.
EXTERN_C HWND
DoHireOldDeliveryWorker(HWND hwnd, UINT wMsg)
{
    LPOLDDELIVERYWORKER pWorker;
    HWND hwndOldWorker;

    // create a memory block for old delivery
    pWorker = new OLDDELIVERYWORKER;
    if (!pWorker)
    {
        ERR("Out of memory\n");
        return NULL;
    }

    // populate the old delivery
    pWorker->hwnd = hwnd;
    pWorker->uMsg = wMsg;

    // create the old delivery worker window
    hwndOldWorker = SHCreateWorkerWindowW(OldDeliveryWorkerWndProc, NULL, 0, 0,
                                          NULL, (LONG_PTR)pWorker);
    if (hwndOldWorker == NULL)
    {
        ERR("hwndOldWorker == NULL\n");
        delete pWorker;
    }

    DWORD pid;
    GetWindowThreadProcessId(hwndOldWorker, &pid);
    TRACE("hwndOldWorker: %p, 0x%lx\n", hwndOldWorker, pid);
    return hwndOldWorker;
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

// This function creates a delivery ticket for shell change nofitication.
// Used in SHChangeNotify.
EXTERN_C HANDLE
DoCreateDeliTicket(LONG wEventId, UINT uFlags, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2,
                   DWORD dwOwnerPID, DWORD dwTick)
{
    // pidl1 and pidl2 have variable length. To store them into the delivery ticket,
    // we have to consider the offsets and the sizes of pidl1 and pidl2.
    LPDELITICKET pTicket;
    HANDLE hTicket = NULL;
    DWORD cbPidl1 = 0, cbPidl2 = 0, ibOffset1 = 0, ibOffset2 = 0, cbSize;

    if (pidl1)
    {
        cbPidl1 = ILGetSize(pidl1);
        ibOffset1 = DWORD_ALIGNMENT(sizeof(DELITICKET));
    }
    if (pidl2)
    {
        cbPidl2 = ILGetSize(pidl2);
        ibOffset2 = DWORD_ALIGNMENT(ibOffset1 + cbPidl1);
    }

    // allocate the delivery ticket
    cbSize = ibOffset2 + cbPidl2;
    hTicket = SHAllocShared(NULL, cbSize, dwOwnerPID);
    if (hTicket == NULL)
    {
        ERR("Out of memory\n");
        return NULL;
    }

    // lock the ticket
    pTicket = (LPDELITICKET)SHLockSharedEx(hTicket, dwOwnerPID, TRUE);
    if (pTicket == NULL)
    {
        ERR("SHLockSharedEx failed\n");
        SHFreeShared(hTicket, dwOwnerPID);
        return NULL;
    }

    // populate the ticket
    pTicket->dwMagic  = DELITICKET_MAGIC;
    pTicket->wEventId = wEventId;
    pTicket->uFlags = uFlags;
    pTicket->ibOffset1 = ibOffset1;
    pTicket->ibOffset2 = ibOffset2;
    if (pidl1)
        memcpy((LPBYTE)pTicket + ibOffset1, pidl1, cbPidl1);
    if (pidl2)
        memcpy((LPBYTE)pTicket + ibOffset2, pidl2, cbPidl2);

    // unlock the ticket and return
    SHUnlockShared(pTicket);
    return hTicket;
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

// This function is the body of SHChangeNotify function.
// It creates a delivery ticket and send WM_NOTIF_DELIVERY message to
// transport the change.
EXTERN_C void
DoTransportChange(LONG wEventId, UINT uFlags, LPITEMIDLIST pidl1, LPITEMIDLIST pidl2,
                  DWORD dwTick)
{
    // get new delivery worker
    HWND hwndWorker = DoGetNewDeliveryWorker();
    if (!hwndWorker)
        return;

    // the ticket owner is the process of new delivery worker.
    DWORD pid;
    GetWindowThreadProcessId(hwndWorker, &pid);

    // create a delivery ticket
    HANDLE hTicket = DoCreateDeliTicket(wEventId, uFlags, pidl1, pidl2, pid, dwTick);
    if (hTicket)
    {
        // send the ticket by using WM_NOTIF_DELIVERY
        TRACE("hTicket: %p, 0x%lx\n", hTicket, pid);
        if ((uFlags & (SHCNF_FLUSH | SHCNF_FLUSHNOWAIT)) == SHCNF_FLUSH)
            SendMessageW(hwndWorker, WM_NOTIF_DELIVERY, (WPARAM)hTicket, pid);
        else
            SendNotifyMessageW(hwndWorker, WM_NOTIF_DELIVERY, (WPARAM)hTicket, pid);
    }
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

// Message WM_NOTIF_REG:
//   wParam: The handle of registration entry.
//   lParam: The owner PID of registration entry.
//   return: TRUE if successful.
LRESULT CChangeNotify::OnReg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
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

// Message WM_NOTIF_UNREG:
//   wParam: The registration ID.
//   lParam: Ignored.
//   return: TRUE if successful.
LRESULT CChangeNotify::OnUnReg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TRACE("OnUnReg(%p, %u, %p, %p)\n", m_hWnd, uMsg, wParam, lParam);

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

// Message WM_NOTIF_DELIVERY:
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

// Message WM_NOTIF_SUSPEND:
//   (specification is unknown)
LRESULT CChangeNotify::OnSuspendResume(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TRACE("OnSuspendResume\n");

    // FIXME
    return FALSE;
}

// Message WM_NOTIF_REMOVEBYPID:
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

        // lock the delivery ticket
        LPDELITICKET pTicket = (LPDELITICKET)SHLockSharedEx(hTicket, dwOwnerPID, FALSE);
        if (!pTicket || pTicket->dwMagic != DELITICKET_MAGIC)
        {
            ERR("pTicket is invalid\n");
            SHUnlockShared(pShared);
            continue;
        }

        // should we notify for it?
        BOOL bNotify = ShouldNotify(pTicket, pShared);

        // unlock the ticket
        SHUnlockShared(pTicket);

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

CRITICAL_SECTION SHELL32_ChangenotifyCS;

// This function will be called from DllMain!DLL_PROCESS_ATTACH.
EXTERN_C void InitChangeNotifications(void)
{
    InitializeCriticalSection(&SHELL32_ChangenotifyCS);
}

// This function will be called from DllMain!DLL_PROCESS_DETACH.
EXTERN_C void FreeChangeNotifications(void)
{
    HWND hwndWorker;
    hwndWorker = DoGetNewDeliveryWorker();
    SendMessageW(hwndWorker, WM_NOTIF_REMOVEBYPID, GetCurrentProcessId(), 0);
    DeleteCriticalSection(&SHELL32_ChangenotifyCS);
}

/*************************************************************************
 * SHChangeNotifyRegister			[SHELL32.2]
 *
 */
EXTERN_C ULONG WINAPI
SHChangeNotifyRegister(HWND hwnd, int fSources, LONG wEventMask, UINT uMsg,
                       int cItems, SHChangeNotifyEntry *lpItems)
{
    HWND hwndWorker, hwndOldWorker = NULL;
    HANDLE hShared;
    INT iItem;
    ULONG nRegID = INVALID_REG_ID;
    DWORD dwOwnerPID;
    LPREGENTRY pShare;

    TRACE("(%p,0x%08x,0x%08x,0x%08x,%d,%p)\n",
          hwnd, fSources, wEventMask, uMsg, cItems, lpItems);

    // sanity check
    if (wEventMask == 0 || cItems <= 0 || cItems > 0x7FFF || lpItems == NULL ||
        !hwnd || !IsWindow(hwnd))
    {
        return INVALID_REG_ID;
    }

    // create and get the new delivery worker window
    hwndWorker = DoGetNewDeliveryWorker();
    if (hwndWorker == NULL)
        return INVALID_REG_ID;

    // if it is old delivery, then create the old delivery worker window
    if ((fSources & SHCNRF_NewDelivery) == 0)
    {
        hwndOldWorker = hwnd = DoHireOldDeliveryWorker(hwnd, uMsg);
        uMsg = WM_OLDDELI_HANDOVER;
    }

    if ((fSources & SHCNRF_RecursiveInterrupt) != 0 &&
        (fSources & SHCNRF_InterruptLevel) == 0)
    {
        fSources &= ~SHCNRF_NewDelivery;
    }

    // The owner PID is the process ID of new delivery worker.
    GetWindowThreadProcessId(hwndWorker, &dwOwnerPID);

    EnterCriticalSection(&SHELL32_ChangenotifyCS);
    for (iItem = 0; iItem < cItems; ++iItem)
    {
        // create a registration entry
        hShared = DoCreateRegEntry(nRegID, hwnd, uMsg, fSources, wEventMask,
                                   lpItems[iItem].fRecursive, lpItems[iItem].pidl,
                                   dwOwnerPID, hwndOldWorker);
        if (hShared)
        {
            TRACE("WM_NOTIF_REG: hwnd:%p, hShared:%p, pid:0x%lx\n", hwndWorker, hShared, dwOwnerPID);
            SendMessageW(hwndWorker, WM_NOTIF_REG, (WPARAM)hShared, dwOwnerPID);

            pShare = (LPREGENTRY)SHLockSharedEx(hShared, dwOwnerPID, FALSE);
            if (pShare)
            {
                nRegID = pShare->nRegID;
                SHUnlockShared(pShare);
            }
            SHFreeShared(hShared, dwOwnerPID);
        }

        if (nRegID == INVALID_REG_ID && (fSources & SHCNRF_NewDelivery) == 0)
        {
            ERR("Old Delivery is failed\n");
            DestroyWindow(hwnd);
            LeaveCriticalSection(&SHELL32_ChangenotifyCS);
            return INVALID_REG_ID;
        }
    }
    LeaveCriticalSection(&SHELL32_ChangenotifyCS);

    return nRegID;
}

/*************************************************************************
 * SHChangeNotifyDeregister			[SHELL32.4]
 */
EXTERN_C BOOL WINAPI
SHChangeNotifyDeregister(ULONG hNotify)
{
    HWND hwndWorker;
    LRESULT ret = 0;
    TRACE("(0x%08x)\n", hNotify);

    // get the new delivery worker window
    hwndWorker = DoGetNewDeliveryWorker();
    if (hwndWorker)
    {
        // send WM_NOTIF_UNREG message and try to unregister
        ret = SendMessageW(hwndWorker, WM_NOTIF_UNREG, hNotify, 0);
        if (!ret)
        {
            ERR("WM_NOTIF_UNREG failed\n");
        }
    }
    return ret;
}

/*************************************************************************
 * SHChangeNotifyUpdateEntryList       		[SHELL32.5]
 */
EXTERN_C BOOL WINAPI
SHChangeNotifyUpdateEntryList(DWORD unknown1, DWORD unknown2,
                              DWORD unknown3, DWORD unknown4)
{
    FIXME("(0x%08x, 0x%08x, 0x%08x, 0x%08x)\n",
          unknown1, unknown2, unknown3, unknown4);

    return TRUE;
}

/* for dumping events */
static LPCSTR DumpEvent(LONG event)
{
    if (event == SHCNE_ALLEVENTS)
        return "SHCNE_ALLEVENTS";
#define DUMPEV(x)  ,( event & SHCNE_##x )? #x " " : ""
    return wine_dbg_sprintf( "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s"
    DUMPEV(RENAMEITEM)
    DUMPEV(CREATE)
    DUMPEV(DELETE)
    DUMPEV(MKDIR)
    DUMPEV(RMDIR)
    DUMPEV(MEDIAINSERTED)
    DUMPEV(MEDIAREMOVED)
    DUMPEV(DRIVEREMOVED)
    DUMPEV(DRIVEADD)
    DUMPEV(NETSHARE)
    DUMPEV(NETUNSHARE)
    DUMPEV(ATTRIBUTES)
    DUMPEV(UPDATEDIR)
    DUMPEV(UPDATEITEM)
    DUMPEV(SERVERDISCONNECT)
    DUMPEV(UPDATEIMAGE)
    DUMPEV(DRIVEADDGUI)
    DUMPEV(RENAMEFOLDER)
    DUMPEV(FREESPACE)
    DUMPEV(EXTENDED_EVENT)
    DUMPEV(ASSOCCHANGED)
    DUMPEV(INTERRUPT)
    );
#undef DUMPEV
}

/*************************************************************************
 * SHChangeNotify				[SHELL32.@]
 */
EXTERN_C void WINAPI
SHChangeNotify(LONG wEventId, UINT uFlags, LPCVOID dwItem1, LPCVOID dwItem2)
{
    LPITEMIDLIST pidl1 = NULL, pidl2 = NULL, pidlTemp1 = NULL, pidlTemp2 = NULL;
    DWORD dwTick = GetTickCount();
    WCHAR szPath1[MAX_PATH], szPath2[MAX_PATH];
    LPWSTR psz1, psz2;
    TRACE("(0x%08x,0x%08x,%p,%p)\n", wEventId, uFlags, dwItem1, dwItem2);

    switch (uFlags & SHCNF_TYPE)
    {
        case SHCNF_IDLIST:
            if (wEventId == SHCNE_FREESPACE)
            {
                DoNotifyFreeSpace((LPCITEMIDLIST)dwItem1, (LPCITEMIDLIST)dwItem2);
                goto Quit;
            }
            pidl1 = (LPITEMIDLIST)dwItem1;
            pidl2 = (LPITEMIDLIST)dwItem2;
            break;

        case SHCNF_PATHA:
            psz1 = psz2 = NULL;
            if (dwItem1)
            {
                MultiByteToWideChar(CP_ACP, 0, (LPCSTR)dwItem1, -1, szPath1, _countof(szPath1));
                psz1 = szPath1;
            }
            if (dwItem2)
            {
                MultiByteToWideChar(CP_ACP, 0, (LPCSTR)dwItem2, -1, szPath2, _countof(szPath2));
                psz2 = szPath2;
            }
            uFlags &= ~SHCNF_TYPE;
            uFlags |= SHCNF_PATHW;
            SHChangeNotify(wEventId, uFlags, psz1, psz2);
            return;

        case SHCNF_PATHW:
            if (wEventId == SHCNE_FREESPACE)
            {
                /* FIXME */
                goto Quit;
            }
            if (dwItem1)
            {
                pidl1 = pidlTemp1 = SHSimpleIDListFromPathW((LPCWSTR)dwItem1);
            }
            if (dwItem2)
            {
                pidl2 = pidlTemp2 = SHSimpleIDListFromPathW((LPCWSTR)dwItem2);
            }
            break;

        case SHCNF_PRINTERA:
        case SHCNF_PRINTERW:
            FIXME("SHChangeNotify with (uFlags & SHCNF_PRINTER)\n");
            return;

        default:
            FIXME("unknown type %08x\n", uFlags & SHCNF_TYPE);
            return;
    }

    if (wEventId == 0 || (wEventId & SHCNE_ASSOCCHANGED) || pidl1 != NULL)
    {
        // transport the change
        TRACE("notifying event %s(%x)\n", DumpEvent(wEventId), wEventId);
        DoTransportChange(wEventId, uFlags, pidl1, pidl2, dwTick);
    }

Quit:
    if (pidlTemp1)
        ILFree(pidlTemp1);
    if (pidlTemp2)
        ILFree(pidlTemp2);
}

/*************************************************************************
 * NTSHChangeNotifyRegister            [SHELL32.640]
 * NOTES
 *   Idlist is an array of structures and Count specifies how many items in the array.
 *   count should always be one when calling SHChangeNotifyRegister, or
 *   SHChangeNotifyDeregister will not work properly.
 */
EXTERN_C ULONG WINAPI
NTSHChangeNotifyRegister(HWND hwnd, int fSources, LONG fEvents, UINT msg,
                         int count, SHChangeNotifyEntry *idlist)
{
    return SHChangeNotifyRegister(hwnd, fSources | SHCNRF_NewDelivery,
                                  fEvents, msg, count, idlist);
}

/*************************************************************************
 * SHChangeNotification_Lock			[SHELL32.644]
 */
EXTERN_C HANDLE WINAPI
SHChangeNotification_Lock(HANDLE hChange, DWORD dwProcessId, LPITEMIDLIST **lppidls,
                          LPLONG lpwEventId)
{
    LPHANDBAG pHandbag;
    TRACE("%p %08x %p %p\n", hChange, dwProcessId, lppidls, lpwEventId);

    // create a handbag from delivery ticket
    pHandbag = DoGetHandbagFromTicket(hChange, dwProcessId);

    // validate the handbag
    if (!pHandbag || pHandbag->dwMagic != HANDBAG_MAGIC)
    {
        ERR("pHandbag is invalid\n");
        return NULL;
    }

    // populate the handbag
    if (lppidls)
        *lppidls = pHandbag->pidls;
    if (lpwEventId)
        *lpwEventId = pHandbag->pTicket->wEventId;

    // return the handbag
    return pHandbag;
}

/*************************************************************************
 * SHChangeNotification_Unlock			[SHELL32.645]
 */
EXTERN_C BOOL WINAPI
SHChangeNotification_Unlock(HANDLE hLock)
{
    LPHANDBAG pHandbag = (LPHANDBAG)hLock;
    BOOL ret;
    TRACE("%p\n", hLock);

    // validate the handbag
    if (!pHandbag || pHandbag->dwMagic != HANDBAG_MAGIC)
    {
        ERR("pHandbag is invalid\n");
        return FALSE;
    }

    // free the handbag
    ret = SHUnlockShared(pHandbag->pTicket);
    LocalFree(hLock);
    return ret;
}

/*************************************************************************
 * NTSHChangeNotifyDeregister			[SHELL32.641]
 */
EXTERN_C DWORD WINAPI
NTSHChangeNotifyDeregister(ULONG x1)
{
    FIXME("(0x%08x):semi stub.\n",x1);

    return SHChangeNotifyDeregister(x1);
}
