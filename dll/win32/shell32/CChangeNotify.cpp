/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Shell change notification
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "CChangeNotify.h"

static CChangeNotify s_hwndNotif;

EXTERN_C void
DoNotifyFreeSpace(LPCITEMIDLIST pidl1,LPCITEMIDLIST pidl2)
{
    WCHAR path1[MAX_PATH], path2[MAX_PATH];

    if (!SHGetPathFromIDListW(pidl1, path1))
        return;

    path2[0] = 0;
    if (pidl2)
        SHGetPathFromIDListW(pidl2, path2);

    if (path2[0])
        SHChangeNotify(SHCNE_FREESPACE, SHCNF_PATHW, path1, path2);
    else
        SHChangeNotify(SHCNE_FREESPACE, SHCNF_PATHW, path1, NULL);
}

EXTERN_C UINT
DoGetNextRegID(void)
{
    return s_hwndNotif.GetNextRegID();
}

EXTERN_C void
DoRemoveChangeNotifyClientsByProcess(DWORD dwUserID)
{
    DWORD dwOwnerPID;
    GetWindowThreadProcessId(s_hwndNotif, &dwOwnerPID);
    s_hwndNotif.RemoveItemsByProcess(dwOwnerPID, dwUserID);
}

static DWORD WINAPI
DoCreateNotifWindowThreadFunc(LPVOID pData)
{
    if (IsWindow(s_hwndNotif))
        return 0;

    HWND hwndShell = (HWND)pData;
    DWORD exstyle = WS_EX_TOOLWINDOW;
    DWORD style = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    s_hwndNotif.CreateWorker(hwndShell, exstyle, style);
    return 0;
}

static DWORD WINAPI
ChangeNotifThreadFunc(LPVOID args)
{
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!IsWindow(s_hwndNotif))
            break;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

EXTERN_C HWND
DoCreateNotifWindow(HWND hwndShell)
{
    if (!IsWindow(s_hwndNotif))
    {
        SHCreateThread(ChangeNotifThreadFunc, hwndShell, CTF_PROCESS_REF,
                       DoCreateNotifWindowThreadFunc);
    }
    return s_hwndNotif;
}

EXTERN_C HWND
DoGetNotifWindow(BOOL bCreate)
{
    if (s_hwndNotif && IsWindow(s_hwndNotif))
        return s_hwndNotif;

    HWND hwnd, hwndShell = GetShellWindow();
    if (bCreate)
    {
        hwnd = DoCreateNotifWindow(hwndShell);
    }
    else
    {
        hwnd = (HWND)SendMessageW(hwndShell, WM_SHELL_GETNOTIFWND, 0, 0);
    }
    s_hwndNotif.SetHWND(hwnd);

    return hwnd;
}

static LRESULT CALLBACK
OldDeliveryWorkerWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HANDLE hLock;
    LRESULT ret = 0;
    PIDLIST_ABSOLUTE *ppidl = NULL;
    LONG lEvent;
    OLDDELIVERYWORKER *pNotif = (OLDDELIVERYWORKER *)GetWindowLongW(hwnd,0);

    switch (uMsg)
    {
        case WM_NOTIF_BANG:
            if (pNotif)
            {
                hLock = SHChangeNotification_Lock((HANDLE)wParam, lParam, &ppidl, &lEvent);
                if (hLock)
                {
                    ret = SendMessageW(pNotif->hwnd, pNotif->uMsg, (WPARAM)*ppidl, lEvent);
                    SHChangeNotification_Unlock(hLock);
                    return TRUE;
                }
            }
            return FALSE;

        case WM_NCDESTROY:
            SetWindowLongW(hwnd, 0, 0);
            LocalFree(pNotif);
            break;

        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return ret;
}

EXTERN_C HWND
DoHireOldDeliveryWorker(HWND hwnd, UINT wMsg)
{
    LPOLDDELIVERYWORKER pWorker;
    HWND hwndWorker;

    pWorker = (LPOLDDELIVERYWORKER)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(OLDDELIVERYWORKER));
    if (!pWorker)
        return NULL;

    pWorker->hwnd = hwnd;
    pWorker->uMsg = wMsg;
    hwndWorker = SHCreateWorkerWindowW(OldDeliveryWorkerWindowProc, 0, 0, 0, 0,
                                       (LONG_PTR)pWorker);
    if (hwndWorker == NULL)
    {
        LocalFree(pWorker);
    }

    return hwndWorker;
}

EXTERN_C HANDLE
DoCreateNotifShare(
    ULONG nRegID,
    HWND hwnd,
    UINT wMsg,
    ULONG fSources,
    LONG fEvents,
    LONG fRecursive,
    LPCITEMIDLIST pidl,
    DWORD dwOwnerPID)
{
    DWORD cbPidl = ILGetSize(pidl);
    DWORD cbSize = sizeof(NOTIFSHARE) + cbPidl;
    HANDLE hShared = SHAllocShared(NULL, cbSize, dwOwnerPID);
    if (!hShared)
        return NULL;

    LPNOTIFSHARE pShared = (LPNOTIFSHARE)SHLockShared(hShared, dwOwnerPID);
    if (pShared == NULL)
    {
        SHFreeShared(hShared, dwOwnerPID);
        return NULL;
    }

    pShared->dwMagic = NOTIFSHARE_MAGIC;
    pShared->cbSize = cbSize;
    pShared->nRegID = nRegID;
    pShared->hwnd = hwnd;
    pShared->uMsg = wMsg;
    pShared->fSources = fSources;
    pShared->fEvents = fEvents;
    pShared->fRecursive = fRecursive;
    pShared->ibPidl = 0;
    if (pidl)
    {
        pShared->ibPidl = DWORD_ALIGNMENT(sizeof(NOTIFSHARE));
        memcpy((LPBYTE)pShared + pShared->ibPidl, pidl, cbPidl);
    }
    SHUnlockShared(pShared);

    return hShared;
}

EXTERN_C HANDLE
DoCreateDelivery(
    LONG wEventId,
    UINT uFlags,
    LPCITEMIDLIST pidl1,
    LPCITEMIDLIST pidl2,
    DWORD dwOwnerPID,
    DWORD dwTick)
{
    LPDELITICKET pTicket;
    HANDLE hShared = NULL;
    DWORD cbPidl1, cbPidl2, ibOffset1, ibOffset2, cbSize;

    ibOffset1 = 0;
    if (pidl1)
    {
        cbPidl1 = ILGetSize(pidl1);
        ibOffset1 = DWORD_ALIGNMENT(sizeof(DELITICKET));
    }

    ibOffset2 = 0;
    if (pidl2)
    {
        cbPidl2 = ILGetSize(pidl2);
        ibOffset2 = DWORD_ALIGNMENT(ibOffset1 + cbPidl1);
    }

    cbSize = ibOffset2 + cbPidl2;

    hShared = SHAllocShared(NULL, cbSize, dwOwnerPID);
    if (hShared == NULL)
        return NULL;

    pTicket = (LPDELITICKET)SHLockShared(hShared, dwOwnerPID);
    if (pTicket == NULL)
    {
        SHFreeShared(hShared,dwOwnerPID);
        return NULL;
    }
    pTicket->dwMagic  = DELIVERY_MAGIC;
    pTicket->wEventId = wEventId;
    pTicket->uFlags = uFlags;
    pTicket->ibOffset1 = ibOffset1;
    pTicket->ibOffset2 = ibOffset2;

    if (pidl1)
        memcpy((LPBYTE)pTicket + ibOffset1, pidl1, cbPidl1);

    if (pidl2)
        memcpy((LPBYTE)pTicket + ibOffset2, pidl1, cbPidl2);

    SHUnlockShared(pTicket);

    return hShared;
}

EXTERN_C LPCHANGE
DoGetChangeFromTicket(HANDLE hDelivery, DWORD dwOwnerPID)
{
    LPDELITICKET pTicket;
    LPCHANGE pChange;

    pTicket = (LPDELITICKET)SHLockShared(hDelivery, dwOwnerPID);
    if (!pTicket || pTicket->dwMagic != DELIVERY_MAGIC)
        return NULL;

    pChange = (LPCHANGE)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(CHANGE));
    if (pChange == NULL)
    {
        SHUnlockShared(pTicket);
        return NULL;
    }
    pChange->dwMagic = CHANGE_MAGIC;

    pChange->pidl1 = pChange->pidl2 = NULL;
    if (pTicket->ibOffset1)
        pChange->pidl1 = (LPITEMIDLIST)((LPBYTE)pTicket + pTicket->ibOffset1);
    if (pTicket->ibOffset2)
        pChange->pidl2 = (LPITEMIDLIST)((LPBYTE)pTicket + pTicket->ibOffset2);

    pChange->pTicket = pTicket;
    return pChange;
}

EXTERN_C void
DoChangeDelivery(LONG wEventId, UINT uFlags, LPITEMIDLIST pidl1, LPITEMIDLIST pidl2,
                 DWORD dwTick)
{
    HWND hwndNotif = DoGetNotifWindow(FALSE);
    if (!hwndNotif)
        return;

    DWORD pid;
    GetWindowThreadProcessId(hwndNotif, &pid);

    HANDLE hDelivery = DoCreateDelivery(wEventId, uFlags, pidl1, pidl2, pid, dwTick);
    if (hDelivery)
    {
        if ((uFlags & (SHCNF_FLUSH | SHCNF_FLUSHNOWAIT)) == SHCNF_FLUSH)
        {
            SendMessageW(hwndNotif, WM_NOTIF_DELIVERY, (WPARAM)hDelivery, pid);
        }
        else
        {
            SendNotifyMessageW(hwndNotif, WM_NOTIF_DELIVERY, (WPARAM)hDelivery, pid);
        }
    }
}

BOOL CChangeNotify::AddItem(UINT nRegID, DWORD dwUserPID, HANDLE hShare)
{
    for (INT i = 0; i < m_items.GetSize(); ++i)
    {
        if (m_items[i].nRegID == INVALID_REG_ID)
        {
            m_items[i].nRegID = nRegID;
            m_items[i].dwUserPID = dwUserPID;
            m_items[i].hShare = hShare;
            return TRUE;
        }
    }

    ITEM item = { nRegID, dwUserPID, hShare };
    m_items.Add(item);
    return TRUE;
}

BOOL CChangeNotify::RemoveItem(UINT nRegID, DWORD dwOwnerPID)
{
    BOOL bFound = FALSE;

    for (INT i = 0; i < m_items.GetSize(); ++i)
    {
        if (m_items[i].nRegID == nRegID)
        {
            bFound = TRUE;
            SHFreeShared(m_items[i].hShare, dwOwnerPID);
            m_items[i].nRegID = INVALID_REG_ID;
            m_items[i].dwUserPID = 0;
            m_items[i].hShare = NULL;
        }
    }

    return bFound;
}

void CChangeNotify::RemoveItemsByProcess(DWORD dwOwnerPID, DWORD dwUserPID)
{
    for (INT i = 0; i < m_items.GetSize(); ++i)
    {
        if (m_items[i].dwUserPID == dwUserPID)
        {
            SHFreeShared(m_items[i].hShare, dwOwnerPID);
            m_items[i].nRegID = INVALID_REG_ID;
            m_items[i].dwUserPID = 0;
            m_items[i].hShare = NULL;
        }
    }
}

LRESULT CChangeNotify::OnBang(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HANDLE hShared = (HANDLE)wParam;
    DWORD dwOwnerPID = (DWORD)lParam;

    LPNOTIFSHARE pShared = (LPNOTIFSHARE)SHLockShared(hShared, dwOwnerPID);
    if (!pShared || pShared->dwMagic != NOTIFSHARE_MAGIC)
        return FALSE;

    DWORD dwUserPID;
    GetWindowThreadProcessId(pShared->hwnd, &dwUserPID);

    HANDLE hNewShared = SHAllocShared(pShared, pShared->cbSize, dwOwnerPID);
    return AddItem(m_nNextRegID, dwUserPID, hNewShared);
}

LRESULT CChangeNotify::OnUnReg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UINT nRegID = (UINT)wParam;
    if (nRegID == INVALID_REG_ID)
        return FALSE;

    DWORD dwOwnerPID;
    GetWindowThreadProcessId(m_hWnd, &dwOwnerPID);

    return RemoveItem(nRegID, dwOwnerPID);
}

LRESULT CChangeNotify::OnDelivery(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HANDLE hDelivery = (HANDLE)wParam;
    DWORD dwOwnerPID = (DWORD)lParam;
    BOOL ret = FALSE;

    LPCHANGE pChange = DoGetChangeFromTicket(hDelivery, dwOwnerPID);
    if (pChange && pChange->dwMagic == CHANGE_MAGIC)
    {
        LPDELITICKET pTicket = pChange->pTicket;
        if (pTicket && pTicket->dwMagic == DELIVERY_MAGIC)
        {
            ret = DoDelivery(hDelivery, pChange);
        }
    }

    SHChangeNotification_Unlock(pChange);
    SHFreeShared(hDelivery, dwOwnerPID);
    return ret;
}

LRESULT CChangeNotify::OnSuspendResume(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // FIXME
    return FALSE;
}

UINT CChangeNotify::GetNextRegID()
{
    m_nNextRegID++;
    if (m_nNextRegID == INVALID_REG_ID)
        m_nNextRegID++;
    return m_nNextRegID;
}

BOOL CChangeNotify::DoDelivery(HANDLE hDelivery, LPCHANGE pChange)
{
    DWORD dwOwnerPID;
    GetWindowThreadProcessId(m_hWnd, &dwOwnerPID);

    for (INT i = 0; i < m_items.GetSize(); ++i)
    {
        HANDLE hShare = m_items[i].hShare;
        if (!m_items[i].nRegID || !hShare)
            continue;

        LPNOTIFSHARE pShared = (LPNOTIFSHARE)SHLockShared(hShare, dwOwnerPID);
        if (!pShared || pShared->dwMagic != NOTIFSHARE_MAGIC)
            continue;

        LPDELITICKET pTicket = (LPDELITICKET)SHLockShared(hDelivery, dwOwnerPID);
        if (!pTicket || pTicket->dwMagic != DELIVERY_MAGIC)
        {
            SHUnlockShared(pShared);
            continue;
        }

        BOOL bNotify = ShouldNotify(pChange, pTicket, pShared);
        SHUnlockShared(pTicket);

        if (bNotify)
        {
            SendNotifyMessageW(pShared->hwnd, pShared->uMsg, (WPARAM)hDelivery, dwOwnerPID);
        }
        SHUnlockShared(pShared);
    }

    return TRUE;
}

BOOL CChangeNotify::ShouldNotify(LPCHANGE pChange, LPDELITICKET pTicket, LPNOTIFSHARE pShared)
{
    LPITEMIDLIST pidl = NULL;
    if (pShared->ibPidl)
        pidl = (LPITEMIDLIST)((LPBYTE)pShared + pShared->ibPidl);

    if (!pidl)
        return FALSE;

    LPITEMIDLIST pidl1 = NULL, pidl2 = NULL;
    if (pTicket->ibOffset1)
        pidl1 = (LPITEMIDLIST)((LPBYTE)pTicket + pTicket->ibOffset1);
    if (pTicket->ibOffset2)
        pidl2 = (LPITEMIDLIST)((LPBYTE)pTicket + pTicket->ibOffset2);

    if (pidl1 && ILIsEqual(pidl, pidl1))
        return TRUE;
    if (pidl2 && ILIsEqual(pidl, pidl2))
        return TRUE;

    if (pShared->fRecursive)
    {
        if (pidl1 && ILIsParent(pidl, pidl1, FALSE))
            return TRUE;
        if (pidl2 && ILIsParent(pidl, pidl2, FALSE))
            return TRUE;
    }

    return FALSE;
}
