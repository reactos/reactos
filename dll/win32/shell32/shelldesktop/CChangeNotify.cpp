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

static HWND s_hwndNewWorker = NULL;

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

EXTERN_C HWND
DoGetNewDeliveryWorker(void)
{
    if (s_hwndNewWorker && IsWindow(s_hwndNewWorker))
        return s_hwndNewWorker;

    HWND hwndShell = GetShellWindow();
    HWND hwndWorker = (HWND)SendMessageW(hwndShell, WM_GETDELIWORKERWND, 0, 0);
    if (!IsWindow(hwndWorker))
    {
        ERR("Unable to get notification window\n");
        hwndWorker = NULL;
    }

    s_hwndNewWorker = hwndWorker;
    return hwndWorker;
}

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

            ppidl = NULL;
            hLock = SHChangeNotification_Lock(hShared, dwOwnerPID, &ppidl, &lEvent);
            if (!hLock)
            {
                ERR("!hLock\n");
                break;
            }

            TRACE("OldDeliveryWorker notifying: %p, 0x%x, %p, 0x%lx\n",
                  pWorker->hwnd, pWorker->uMsg, ppidl, lEvent);
            SendMessageW(pWorker->hwnd, pWorker->uMsg, (WPARAM)ppidl, lEvent);
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

EXTERN_C HWND
DoHireOldDeliveryWorker(HWND hwnd, UINT wMsg)
{
    LPOLDDELIVERYWORKER pWorker;
    HWND hwndOldWorker;

    pWorker = new OLDDELIVERYWORKER;
    if (!pWorker)
    {
        ERR("Out of memory\n");
        return NULL;
    }

    pWorker->hwnd = hwnd;
    pWorker->uMsg = wMsg;
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

EXTERN_C HANDLE
DoCreateNotifShare(ULONG nRegID, HWND hwnd, UINT wMsg, INT fSources, LONG fEvents,
                   LONG fRecursive, LPCITEMIDLIST pidl, DWORD dwOwnerPID,
                   HWND hwndOldWorker)
{
    DWORD cbPidl = ILGetSize(pidl);
    DWORD ibPidl = DWORD_ALIGNMENT(sizeof(NOTIFSHARE));
    DWORD cbSize = ibPidl + cbPidl;
    HANDLE hShared = SHAllocShared(NULL, cbSize, dwOwnerPID);
    if (!hShared)
    {
        ERR("Out of memory\n");
        return NULL;
    }

    LPNOTIFSHARE pShared = (LPNOTIFSHARE)SHLockSharedEx(hShared, dwOwnerPID, TRUE);
    if (pShared == NULL)
    {
        ERR("SHLockSharedEx failed\n");
        SHFreeShared(hShared, dwOwnerPID);
        return NULL;
    }
    pShared->dwMagic = NOTIFSHARE_MAGIC;
    pShared->cbSize = cbSize;
    pShared->nRegID = nRegID;
    pShared->hwnd = hwnd;
    pShared->hwndOldWorker = hwndOldWorker;
    pShared->uMsg = wMsg;
    pShared->fSources = fSources;
    pShared->fEvents = fEvents;
    pShared->fRecursive = fRecursive;
    pShared->ibPidl = 0;
    if (pidl)
    {
        pShared->ibPidl = ibPidl;
        memcpy((LPBYTE)pShared + ibPidl, pidl, cbPidl);
    }
    SHUnlockShared(pShared);
    return hShared;
}

EXTERN_C HANDLE
DoCreateDeliTicket(LONG wEventId, UINT uFlags, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2,
                   DWORD dwOwnerPID, DWORD dwTick)
{
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

    cbSize = ibOffset2 + cbPidl2;
    hTicket = SHAllocShared(NULL, cbSize, dwOwnerPID);
    if (hTicket == NULL)
    {
        ERR("Out of memory\n");
        return NULL;
    }

    pTicket = (LPDELITICKET)SHLockSharedEx(hTicket, dwOwnerPID, TRUE);
    if (pTicket == NULL)
    {
        ERR("SHLockSharedEx failed\n");
        SHFreeShared(hTicket, dwOwnerPID);
        return NULL;
    }
    pTicket->dwMagic  = DELITICKET_MAGIC;
    pTicket->wEventId = wEventId;
    pTicket->uFlags = uFlags;
    pTicket->ibOffset1 = ibOffset1;
    pTicket->ibOffset2 = ibOffset2;

    if (pidl1)
        memcpy((LPBYTE)pTicket + ibOffset1, pidl1, cbPidl1);
    if (pidl2)
        memcpy((LPBYTE)pTicket + ibOffset2, pidl2, cbPidl2);

    SHUnlockShared(pTicket);
    return hTicket;
}

EXTERN_C LPHANDBAG
DoGetHandbagFromTicket(HANDLE hTicket, DWORD dwOwnerPID)
{
    LPDELITICKET pTicket = (LPDELITICKET)SHLockSharedEx(hTicket, dwOwnerPID, FALSE);
    if (!pTicket || pTicket->dwMagic != DELITICKET_MAGIC)
    {
        ERR("pTicket is invalid\n");
        return NULL;
    }

    LPHANDBAG pHandbag = (LPHANDBAG)LocalAlloc(LMEM_FIXED, sizeof(HANDBAG));
    if (pHandbag == NULL)
    {
        ERR("Out of memory\n");
        SHUnlockShared(pTicket);
        return NULL;
    }
    pHandbag->dwMagic = HANDBAG_MAGIC;
    pHandbag->pTicket = pTicket;

    pHandbag->pidls[0] = pHandbag->pidls[1] = NULL;
    if (pTicket->ibOffset1)
        pHandbag->pidls[0] = (LPITEMIDLIST)((LPBYTE)pTicket + pTicket->ibOffset1);
    if (pTicket->ibOffset2)
        pHandbag->pidls[1] = (LPITEMIDLIST)((LPBYTE)pTicket + pTicket->ibOffset2);

    return pHandbag;
}

EXTERN_C void
DoTransportChange(LONG wEventId, UINT uFlags, LPITEMIDLIST pidl1, LPITEMIDLIST pidl2,
                  DWORD dwTick)
{
    HWND hwndNotif = DoGetNewDeliveryWorker();
    if (!hwndNotif)
        return;

    DWORD pid;
    GetWindowThreadProcessId(hwndNotif, &pid);

    HANDLE hTicket = DoCreateDeliTicket(wEventId, uFlags, pidl1, pidl2, pid, dwTick);
    if (hTicket)
    {
        TRACE("hTicket: %p, 0x%lx\n", hTicket, pid);
        if ((uFlags & (SHCNF_FLUSH | SHCNF_FLUSHNOWAIT)) == SHCNF_FLUSH)
            SendMessageW(hwndNotif, WM_NOTIF_DELIVERY, (WPARAM)hTicket, pid);
        else
            SendNotifyMessageW(hwndNotif, WM_NOTIF_DELIVERY, (WPARAM)hTicket, pid);
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

struct CChangeNotifyImpl
{
    typedef CChangeNotify::ITEM ITEM;
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

    BOOL RemoveItem(UINT nRegID, DWORD dwOwnerPID)
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

BOOL CChangeNotify::RemoveItem(UINT nRegID, DWORD dwOwnerPID)
{
    return m_pimpl->RemoveItem(nRegID, dwOwnerPID);
}

void CChangeNotify::RemoveItemsByProcess(DWORD dwOwnerPID, DWORD dwUserPID)
{
    m_pimpl->RemoveItemsByProcess(dwOwnerPID, dwUserPID);
}

LRESULT CChangeNotify::OnReg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TRACE("OnReg(%p, %u, %p, %p)\n", m_hWnd, uMsg, wParam, lParam);

    HANDLE hShared = (HANDLE)wParam;
    DWORD dwOwnerPID = (DWORD)lParam;

    LPNOTIFSHARE pShared = (LPNOTIFSHARE)SHLockSharedEx(hShared, dwOwnerPID, TRUE);
    if (!pShared || pShared->dwMagic != NOTIFSHARE_MAGIC)
    {
        ERR("pShared is invalid\n");
        return FALSE;
    }

    if (pShared->nRegID == INVALID_REG_ID)
        pShared->nRegID = GetNextRegID();

    TRACE("pShared->nRegID: %u\n", pShared->nRegID);

    DWORD dwUserPID;
    GetWindowThreadProcessId(pShared->hwnd, &dwUserPID);

    HWND hwndOldWorker = pShared->hwndOldWorker;

    HANDLE hNewShared = SHAllocShared(pShared, pShared->cbSize, dwOwnerPID);
    if (!hNewShared)
    {
        ERR("Out of memory\n");
        pShared->nRegID = INVALID_REG_ID;
        SHUnlockShared(pShared);
        return FALSE;
    }

    SHUnlockShared(pShared);

    return AddItem(m_nNextRegID, dwUserPID, hNewShared, hwndOldWorker);
}

LRESULT CChangeNotify::OnUnReg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TRACE("OnUnReg(%p, %u, %p, %p)\n", m_hWnd, uMsg, wParam, lParam);

    UINT nRegID = (UINT)wParam;
    if (nRegID == INVALID_REG_ID)
    {
        ERR("INVALID_REG_ID\n");
        return FALSE;
    }

    DWORD dwOwnerPID;
    GetWindowThreadProcessId(m_hWnd, &dwOwnerPID);
    return RemoveItem(nRegID, dwOwnerPID);
}

LRESULT CChangeNotify::OnDelivery(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TRACE("OnDelivery(%p, %u, %p, %p)\n", m_hWnd, uMsg, wParam, lParam);

    HANDLE hTicket = (HANDLE)wParam;
    DWORD dwOwnerPID = (DWORD)lParam;
    BOOL ret = FALSE;

    LPHANDBAG pHandbag = DoGetHandbagFromTicket(hTicket, dwOwnerPID);
    if (pHandbag && pHandbag->dwMagic == HANDBAG_MAGIC)
    {
        LPDELITICKET pTicket = pHandbag->pTicket;
        if (pTicket && pTicket->dwMagic == DELITICKET_MAGIC)
        {
            ret = DoDelivery(hTicket, dwOwnerPID);
        }
    }

    SHChangeNotification_Unlock(pHandbag);
    SHFreeShared(hTicket, dwOwnerPID);
    return ret;
}

LRESULT CChangeNotify::OnSuspendResume(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TRACE("OnSuspendResume\n");

    // FIXME
    return FALSE;
}

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

BOOL CChangeNotify::DoDelivery(HANDLE hTicket, DWORD dwOwnerPID)
{
    TRACE("DoDelivery(%p, %p, 0x%lx)\n", m_hWnd, hTicket, dwOwnerPID);

    for (INT i = 0; i < m_pimpl->m_items.GetSize(); ++i)
    {
        HANDLE hShare = m_pimpl->m_items[i].hShare;
        if (!m_pimpl->m_items[i].nRegID || !hShare)
            continue;

        LPNOTIFSHARE pShared = (LPNOTIFSHARE)SHLockSharedEx(hShare, dwOwnerPID, FALSE);
        if (!pShared || pShared->dwMagic != NOTIFSHARE_MAGIC)
        {
            ERR("pShared is invalid\n");
            continue;
        }

        LPDELITICKET pTicket = (LPDELITICKET)SHLockSharedEx(hTicket, dwOwnerPID, FALSE);
        if (!pTicket || pTicket->dwMagic != DELITICKET_MAGIC)
        {
            ERR("pTicket is invalid\n");
            SHUnlockShared(pShared);
            continue;
        }
        BOOL bNotify = ShouldNotify(pTicket, pShared);
        SHUnlockShared(pTicket);

        if (bNotify)
        {
            TRACE("Notifying: %p, 0x%x, %p, %lu\n",
                  pShared->hwnd, pShared->uMsg, hTicket, dwOwnerPID);
            SendMessageW(pShared->hwnd, pShared->uMsg, (WPARAM)hTicket, dwOwnerPID);
        }
        SHUnlockShared(pShared);
    }

    return TRUE;
}

BOOL CChangeNotify::ShouldNotify(LPDELITICKET pTicket, LPNOTIFSHARE pShared)
{
    LPITEMIDLIST pidl, pidl1 = NULL, pidl2 = NULL;
    WCHAR szPath[MAX_PATH], szPath1[MAX_PATH], szPath2[MAX_PATH];
    INT cch, cch1, cch2;

    if (!pShared->ibPidl)
        return TRUE;

    pidl = (LPITEMIDLIST)((LPBYTE)pShared + pShared->ibPidl);

    if (pTicket->ibOffset1)
    {
        pidl1 = (LPITEMIDLIST)((LPBYTE)pTicket + pTicket->ibOffset1);
        if (ILIsEqual(pidl, pidl1) || ILIsParent(pidl, pidl1, !pShared->fRecursive))
            return TRUE;
    }

    if (pTicket->ibOffset2)
    {
        pidl2 = (LPITEMIDLIST)((LPBYTE)pTicket + pTicket->ibOffset2);
        if (ILIsEqual(pidl, pidl2) || ILIsParent(pidl, pidl2, !pShared->fRecursive))
            return TRUE;
    }

    if (SHGetPathFromIDListW(pidl, szPath))
    {
        PathAddBackslashW(szPath);
        cch = lstrlenW(szPath);

        if (pidl1 && SHGetPathFromIDListW(pidl1, szPath1))
        {
            PathAddBackslashW(szPath1);
            cch1 = lstrlenW(szPath1);
            if (cch < cch1)
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
            if (cch < cch2)
            {
                szPath2[cch] = 0;
                if (lstrcmpiW(szPath, szPath2) == 0)
                    return TRUE;
            }
        }
    }

    return FALSE;
}
