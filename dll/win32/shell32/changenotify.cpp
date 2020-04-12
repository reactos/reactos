/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Shell change notification
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shcn);

CRITICAL_SECTION SHELL32_ChangenotifyCS;

// This function requests creation of the new delivery worker if necessary
// and returns the window handle of the new delivery worker with cached.
static HWND
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

// This function will be called from DllMain!DLL_PROCESS_ATTACH.
EXTERN_C void InitChangeNotifications(void)
{
    InitializeCriticalSection(&SHELL32_ChangenotifyCS);
}

// This function will be called from DllMain!DLL_PROCESS_DETACH.
EXTERN_C void FreeChangeNotifications(void)
{
    HWND hwndWorker = DoGetNewDeliveryWorker(FALSE);
    if (hwndWorker)
        SendMessageW(hwndWorker, WM_WORKER_REMOVEBYPID, GetCurrentProcessId(), 0);
    DeleteCriticalSection(&SHELL32_ChangenotifyCS);
}

// This function handles the case of SHCNE_FREESPACE.
static void DoNotifyFreeSpace(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
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

//////////////////////////////////////////////////////////////////////////////////////
// There are two delivery methods: "old delivery method" and "new delivery method".
//
// The old delivery method creates an old delivery worker window in the caller process
// for message trampoline. The old delivery method is slow and deprecated.
//
// The new delivery method is enabled by SHCNRF_NewDelivery flag.
// The new delivery method directly sends the delivery ticket.

typedef struct OLDDELIVERY
{
    HWND hwnd;
    UINT uMsg;
} OLDDELIVERY, *LPOLDDELIVERY;

// Message WM_OLDWORKER_HANDOVER: Perform old delivery method.
//    wParam: The handle of delivery ticket.
//    lParam: The owner PID of delivery ticket.
//    return: TRUE if successful.
static LRESULT
OldWorker_OnHandOver(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HANDLE hTicket = (HANDLE)wParam;
    DWORD dwOwnerPID = (DWORD)lParam;

    TRACE("WM_OLDWORKER_HANDOVER: hwnd:%p, hTicket:%p, pid:0x%lx\n",
          hwnd, hTicket, dwOwnerPID);

    // get old worker data
    LPOLDDELIVERY pWorker = (LPOLDDELIVERY)GetWindowLongPtrW(hwnd, 0);
    if (pWorker == NULL)
    {
        ERR("pWorker is NULL\n");
        return FALSE;
    }

    // lock the ticket
    PIDLIST_ABSOLUTE *ppidl = NULL;
    LONG lEvent;
    HANDLE hLock = SHChangeNotification_Lock(hTicket, dwOwnerPID, &ppidl, &lEvent);
    if (hLock == NULL)
    {
        ERR("hLock is NULL\n");
        return FALSE;
    }

    // perform the delivery
    TRACE("OldDeliveryWorker notifying: %p, 0x%x, %p, 0x%lx\n",
          pWorker->hwnd, pWorker->uMsg, ppidl, lEvent);
    SendMessageW(pWorker->hwnd, pWorker->uMsg, (WPARAM)ppidl, lEvent);

    // unlock the ticket
    SHChangeNotification_Unlock(hLock);
    return TRUE;
}

// This is "old delivery worker" window. An old delivery worker will be
// created in the caller process. SHChangeNotification_Lock allocates
// a process-local memory block in response of WM_OLDWORKER_HANDOVER, and
// WM_OLDWORKER_HANDOVER sends the pWorker->uMsg message.
static LRESULT CALLBACK
OldWorkerWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPOLDDELIVERY pWorker;
    switch (uMsg)
    {
        case WM_OLDWORKER_HANDOVER:
            return OldWorker_OnHandOver(hwnd, wParam, lParam);

        case WM_NCDESTROY:
            TRACE("WM_NCDESTROY\n");
            pWorker = (LPOLDDELIVERY)GetWindowLongPtrW(hwnd, 0);
            SetWindowLongW(hwnd, 0, 0);
            delete pWorker;
            break;

        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// This function creates an old delivery worker. Used in SHChangeNotifyRegister.
static HWND
DoCreateOldWorker(HWND hwnd, UINT wMsg)
{
    // create a memory block for old delivery
    LPOLDDELIVERY pWorker = new OLDDELIVERY;
    if (pWorker == NULL)
    {
        ERR("Out of memory\n");
        return NULL;
    }
    // populate the old delivery
    pWorker->hwnd = hwnd;
    pWorker->uMsg = wMsg;

    // create the old delivery worker window
    HWND hwndOldWorker = SHCreateWorkerWindowW(OldWorkerWndProc, NULL, 0, 0,
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

// This function creates a delivery ticket for shell change nofitication.
// Used in SHChangeNotify.
static HANDLE
DoCreateTicket(LONG wEventId, UINT uFlags, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2,
               DWORD dwOwnerPID, DWORD dwTick)
{
    // pidl1 and pidl2 have variable length. To store them into the delivery ticket,
    // we have to consider the offsets and the sizes of pidl1 and pidl2.
    DWORD cbPidl1 = 0, cbPidl2 = 0, ibOffset1 = 0, ibOffset2 = 0;
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
    DWORD cbSize = ibOffset2 + cbPidl2;
    HANDLE hTicket = SHAllocShared(NULL, cbSize, dwOwnerPID);
    if (hTicket == NULL)
    {
        ERR("Out of memory\n");
        return NULL;
    }

    // lock the ticket
    LPDELITICKET pTicket = (LPDELITICKET)SHLockSharedEx(hTicket, dwOwnerPID, TRUE);
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
// The handbag is created in SHChangeNotification_Lock and used in OnTicket.
// hTicket is a ticket handle of a shared memory block and dwOwnerPID is
// the owner PID of the ticket.
static LPHANDBAG
DoGetHandbagFromTicket(HANDLE hTicket, DWORD dwOwnerPID)
{
    // lock and validate the delivery ticket
    LPDELITICKET pTicket = (LPDELITICKET)SHLockSharedEx(hTicket, dwOwnerPID, FALSE);
    if (pTicket == NULL || pTicket->dwMagic != DELITICKET_MAGIC)
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

// This function creates a registration entry in SHChangeNotifyRegister function.
static HANDLE
DoCreateRegEntry(ULONG nRegID, HWND hwnd, UINT wMsg, INT fSources, LONG fEvents,
                 LONG fRecursive, LPCITEMIDLIST pidl, DWORD dwOwnerPID,
                 HWND hwndOldWorker)
{
    // pidl has variable length. To store it into the registration entry,
    // we have to consider the length of pidl.
    DWORD cbPidl = ILGetSize(pidl);
    DWORD ibPidl = DWORD_ALIGNMENT(sizeof(REGENTRY));
    DWORD cbSize = ibPidl + cbPidl;

    // create the registration entry and lock it
    HANDLE hRegEntry = SHAllocShared(NULL, cbSize, dwOwnerPID);
    if (hRegEntry == NULL)
    {
        ERR("Out of memory\n");
        return NULL;
    }
    LPREGENTRY pRegEntry = (LPREGENTRY)SHLockSharedEx(hRegEntry, dwOwnerPID, TRUE);
    if (pRegEntry == NULL)
    {
        ERR("SHLockSharedEx failed\n");
        SHFreeShared(hRegEntry, dwOwnerPID);
        return NULL;
    }

    // populate the registration entry
    pRegEntry->dwMagic = REGENTRY_MAGIC;
    pRegEntry->cbSize = cbSize;
    pRegEntry->nRegID = nRegID;
    pRegEntry->hwnd = hwnd;
    pRegEntry->uMsg = wMsg;
    pRegEntry->fSources = fSources;
    pRegEntry->fEvents = fEvents;
    pRegEntry->fRecursive = fRecursive;
    pRegEntry->hwndOldWorker = hwndOldWorker;
    pRegEntry->ibPidl = 0;
    if (pidl)
    {
        pRegEntry->ibPidl = ibPidl;
        memcpy((LPBYTE)pRegEntry + ibPidl, pidl, cbPidl);
    }

    // unlock and return
    SHUnlockShared(pRegEntry);
    return hRegEntry;
}

// This function is the body of SHChangeNotify function.
// It creates a delivery ticket and send WM_WORKER_TICKET message to
// transport the change.
static void
DoCreateTicketAndSend(LONG wEventId, UINT uFlags, LPITEMIDLIST pidl1, LPITEMIDLIST pidl2,
                      DWORD dwTick)
{
    // get new delivery worker
    HWND hwndWorker = DoGetNewDeliveryWorker(FALSE);
    if (hwndWorker == NULL)
        return;

    // the ticket owner is the process of new delivery worker.
    DWORD pid;
    GetWindowThreadProcessId(hwndWorker, &pid);

    // create a delivery ticket
    HANDLE hTicket = DoCreateTicket(wEventId, uFlags, pidl1, pidl2, pid, dwTick);
    if (hTicket == NULL)
        return;

    TRACE("hTicket: %p, 0x%lx\n", hTicket, pid);

    // send the ticket by using WM_WORKER_TICKET
    if ((uFlags & (SHCNF_FLUSH | SHCNF_FLUSHNOWAIT)) == SHCNF_FLUSH)
        SendMessageW(hwndWorker, WM_WORKER_TICKET, (WPARAM)hTicket, pid);
    else
        SendNotifyMessageW(hwndWorker, WM_WORKER_TICKET, (WPARAM)hTicket, pid);
}

/*************************************************************************
 * SHChangeNotifyRegister           [SHELL32.2]
 */
EXTERN_C ULONG WINAPI
SHChangeNotifyRegister(HWND hwnd, INT fSources, LONG wEventMask, UINT uMsg,
                       INT cItems, SHChangeNotifyEntry *lpItems)
{
    HWND hwndWorker, hwndOldWorker = NULL;
    HANDLE hRegEntry;
    INT iItem;
    ULONG nRegID = INVALID_REG_ID;
    DWORD dwOwnerPID;
    LPREGENTRY pShare;

    TRACE("(%p,0x%08x,0x%08x,0x%08x,%d,%p)\n",
          hwnd, fSources, wEventMask, uMsg, cItems, lpItems);

    // sanity check
    if (wEventMask == 0 || cItems <= 0 || cItems > 0x7FFF || lpItems == NULL ||
        hwnd == NULL || !IsWindow(hwnd))
    {
        return INVALID_REG_ID;
    }

    // request the new delivery worker window
    hwndWorker = DoGetNewDeliveryWorker(TRUE);
    if (hwndWorker == NULL)
        return INVALID_REG_ID;

    // disable new delivery method in specific condition
    if ((fSources & SHCNRF_RecursiveInterrupt) != 0 &&
        (fSources & SHCNRF_InterruptLevel) == 0)
    {
        fSources &= ~SHCNRF_NewDelivery;
    }

    // if it is old delivery method, then create the old delivery worker window
    if ((fSources & SHCNRF_NewDelivery) == 0)
    {
        hwndOldWorker = hwnd = DoCreateOldWorker(hwnd, uMsg);
        uMsg = WM_OLDWORKER_HANDOVER;
    }

    // The owner PID is the process ID of the new delivery worker.
    GetWindowThreadProcessId(hwndWorker, &dwOwnerPID);

    EnterCriticalSection(&SHELL32_ChangenotifyCS);
    for (iItem = 0; iItem < cItems; ++iItem)
    {
        // create a registration entry
        hRegEntry = DoCreateRegEntry(nRegID, hwnd, uMsg, fSources, wEventMask,
                                     lpItems[iItem].fRecursive, lpItems[iItem].pidl,
                                     dwOwnerPID, hwndOldWorker);
        if (hRegEntry)
        {
            TRACE("WM_WORKER_REGISTER: hwnd:%p, hRegEntry:%p, pid:0x%lx\n",
                  hwndWorker, hRegEntry, dwOwnerPID);

            // send WM_WORKER_REGISTER to new delivery worker
            SendMessageW(hwndWorker, WM_WORKER_REGISTER, (WPARAM)hRegEntry, dwOwnerPID);

            // update nRegID
            pShare = (LPREGENTRY)SHLockSharedEx(hRegEntry, dwOwnerPID, FALSE);
            if (pShare)
            {
                nRegID = pShare->nRegID;
                SHUnlockShared(pShare);
            }

            // free registration entry
            SHFreeShared(hRegEntry, dwOwnerPID);
        }

        // if failed, then destroy the old worker.
        if (nRegID == INVALID_REG_ID && (fSources & SHCNRF_NewDelivery) == 0)
        {
            ERR("Old Delivery is failed\n");
            DestroyWindow(hwndOldWorker);
            break;
        }
    }
    LeaveCriticalSection(&SHELL32_ChangenotifyCS);

    return nRegID;
}

/*************************************************************************
 * SHChangeNotifyDeregister         [SHELL32.4]
 */
EXTERN_C BOOL WINAPI
SHChangeNotifyDeregister(ULONG hNotify)
{
    TRACE("(0x%08x)\n", hNotify);

    // get the new delivery worker window
    HWND hwndWorker = DoGetNewDeliveryWorker(FALSE);
    if (hwndWorker == NULL)
        return FALSE;

    // send WM_WORKER_UNREGISTER message and try to unregister
    BOOL ret = (BOOL)SendMessageW(hwndWorker, WM_WORKER_UNREGISTER, hNotify, 0);
    if (!ret)
        ERR("WM_WORKER_UNREGISTER failed\n");

    return ret;
}

/*************************************************************************
 * SHChangeNotifyUpdateEntryList            [SHELL32.5]
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
 * SHChangeNotify               [SHELL32.@]
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
                SHAnsiToUnicode((LPCSTR)dwItem1, szPath1, _countof(szPath1));
                psz1 = szPath1;
            }
            if (dwItem2)
            {
                SHAnsiToUnicode((LPCSTR)dwItem2, szPath2, _countof(szPath2));
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
        TRACE("notifying event %s(%x)\n", DumpEvent(wEventId), wEventId);
        DoCreateTicketAndSend(wEventId, uFlags, pidl1, pidl2, dwTick);
    }

Quit:
    if (pidlTemp1)
        ILFree(pidlTemp1);
    if (pidlTemp2)
        ILFree(pidlTemp2);
}

/*************************************************************************
 * NTSHChangeNotifyRegister            [SHELL32.640]
 */
EXTERN_C ULONG WINAPI
NTSHChangeNotifyRegister(HWND hwnd, INT fSources, LONG fEvents, UINT msg,
                         INT count, SHChangeNotifyEntry *idlist)
{
    return SHChangeNotifyRegister(hwnd, fSources | SHCNRF_NewDelivery,
                                  fEvents, msg, count, idlist);
}

/*************************************************************************
 * SHChangeNotification_Lock            [SHELL32.644]
 */
EXTERN_C HANDLE WINAPI
SHChangeNotification_Lock(HANDLE hTicket, DWORD dwOwnerPID, LPITEMIDLIST **lppidls,
                          LPLONG lpwEventId)
{
    TRACE("%p %08x %p %p\n", hTicket, dwOwnerPID, lppidls, lpwEventId);

    // create a handbag from the ticket
    LPHANDBAG pHandbag = DoGetHandbagFromTicket(hTicket, dwOwnerPID);
    if (pHandbag == NULL || pHandbag->dwMagic != HANDBAG_MAGIC)
    {
        ERR("pHandbag is invalid\n");
        return NULL;
    }

    // populate parameters from the handbag
    if (lppidls)
        *lppidls = pHandbag->pidls;
    if (lpwEventId)
        *lpwEventId = pHandbag->pTicket->wEventId;

    // return the handbag
    return pHandbag;
}

/*************************************************************************
 * SHChangeNotification_Unlock          [SHELL32.645]
 */
EXTERN_C BOOL WINAPI
SHChangeNotification_Unlock(HANDLE hLock)
{
    TRACE("%p\n", hLock);

    // validate the handbag
    LPHANDBAG pHandbag = (LPHANDBAG)hLock;
    if (pHandbag == NULL || pHandbag->dwMagic != HANDBAG_MAGIC)
    {
        ERR("pHandbag is invalid\n");
        return FALSE;
    }

    // free the handbag
    BOOL ret = SHUnlockShared(pHandbag->pTicket);
    LocalFree(hLock);
    return ret;
}

/*************************************************************************
 * NTSHChangeNotifyDeregister           [SHELL32.641]
 */
EXTERN_C DWORD WINAPI
NTSHChangeNotifyDeregister(ULONG hNotify)
{
    FIXME("(0x%08x):semi stub.\n", hNotify);
    return SHChangeNotifyDeregister(hNotify);
}
