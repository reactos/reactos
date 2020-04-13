/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Shell change notification
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shcn);

CRITICAL_SECTION SHELL32_ChangenotifyCS;

// This function requests creation of the server window if it doesn't exist yet
static HWND
GetNotificationServer(BOOL bCreate)
{
    static HWND s_hwndServer = NULL;

    // use cache if any
    if (s_hwndServer && IsWindow(s_hwndServer))
        return s_hwndServer;

    // get the shell window
    HWND hwndShell = GetShellWindow();
    if (hwndShell == NULL)
    {
        TRACE("GetShellWindow() returned NULL\n");
        return NULL;
    }

    // Get the window of the notification server that runs in explorer
    HWND hwndServer = (HWND)SendMessageW(hwndShell, WM_DESKTOP_GET_CNOTIFY_SERVER, bCreate, 0);
    if (!IsWindow(hwndServer))
    {
        ERR("Unable to get server window\n");
        hwndServer = NULL;
    }

    // save and return
    s_hwndServer = hwndServer;
    return hwndServer;
}

// This function will be called from DllMain.DLL_PROCESS_ATTACH.
EXTERN_C void InitChangeNotifications(void)
{
    InitializeCriticalSection(&SHELL32_ChangenotifyCS);
}

// This function will be called from DllMain.DLL_PROCESS_DETACH.
EXTERN_C void FreeChangeNotifications(void)
{
    HWND hwndServer = GetNotificationServer(FALSE);
    if (hwndServer)
        SendMessageW(hwndServer, CN_UNREGISTER_PROCESS, GetCurrentProcessId(), 0);
    DeleteCriticalSection(&SHELL32_ChangenotifyCS);
}

//////////////////////////////////////////////////////////////////////////////////////
// There are two delivery methods: "old delivery method" and "new delivery method".
//
// The old delivery method creates a broker window in the caller process
// for message trampoline. The old delivery method is slow and deprecated.
//
// The new delivery method is enabled by SHCNRF_NewDelivery flag.
// With the new delivery method the server directly sends the delivery message.

typedef CWinTraits <
    WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
    WS_EX_TOOLWINDOW
> CBrokerTraits;

// This class brokers all notifications that don't have the SHCNRF_NewDelivery flag
class CChangeNotifyBroker :
    public CWindowImpl<CChangeNotifyBroker, CWindow, CBrokerTraits>
{
public:
    CChangeNotifyBroker(HWND hwndClient, UINT uMsg) :
        m_hwndClient(hwndClient), m_uMsg(uMsg)
    {
    }

    // Message handlers
    LRESULT OnBrokerNotification(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        return BrokerNotification((HANDLE)wParam, (DWORD)lParam);
    }

    void OnFinalMessage(HWND)
    {
        // The server will destroy this window.
        // After the window gets destroyed we can delete this broker here.
        delete this;
    }

    DECLARE_WND_CLASS_EX(L"WorkerW", 0, 0)

    BEGIN_MSG_MAP(CChangeNotifyBroker)
        MESSAGE_HANDLER(WM_BROKER_NOTIFICATION, OnBrokerNotification)
    END_MSG_MAP()

private:
    HWND m_hwndClient;
    UINT m_uMsg;

    BOOL BrokerNotification(HANDLE hTicket, DWORD dwOwnerPID)
    {
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
        TRACE("broker notifying: %p, 0x%x, %p, 0x%lx\n",
              m_hwndClient, m_uMsg, ppidl, lEvent);
        SendMessageW(m_hwndClient, m_uMsg, (WPARAM)ppidl, lEvent);

        // unlock the ticket
        SHChangeNotification_Unlock(hLock);
        return TRUE;
    }
};

// This function creates a notification broker for old method. Used in SHChangeNotifyRegister.
static HWND
CreateNotificationBroker(HWND hwnd, UINT wMsg)
{
    // Create a new broker. It will be freed when the window gets destroyed
    CChangeNotifyBroker* pBroker = new CChangeNotifyBroker(hwnd, wMsg);
    if (pBroker == NULL)
    {
        ERR("Out of memory\n");
        return NULL;
    }

    HWND hwndBroker = pBroker->Create(0);
    if (hwndBroker == NULL)
    {
        ERR("hwndBroker == NULL\n");
        delete pBroker;
    }

    return hwndBroker;
}

// This function creates a delivery ticket for shell change nofitication.
// Used in SHChangeNotify.
static HANDLE
CreateNotificationParam(LONG wEventId, UINT uFlags, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2,
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
// The handbag is created in SHChangeNotification_Lock and used in OnBrokerNotification.
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
        SHUnlockShared(pTicket);
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
CreateRegistrationParam(ULONG nRegID, HWND hwnd, UINT wMsg, INT fSources, LONG fEvents,
                        LONG fRecursive, LPCITEMIDLIST pidl, DWORD dwOwnerPID,
                        HWND hwndBroker)
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
    pRegEntry->hwndBroker = hwndBroker;
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
// It creates a delivery ticket and send CN_DELIVER_NOTIFICATION message to
// transport the change.
static void
CreateNotificationParamAndSend(LONG wEventId, UINT uFlags, LPITEMIDLIST pidl1, LPITEMIDLIST pidl2,
                               DWORD dwTick)
{
    // get server window
    HWND hwndServer = GetNotificationServer(FALSE);
    if (hwndServer == NULL)
        return;

    // the ticket owner is the process of the notification server
    DWORD pid;
    GetWindowThreadProcessId(hwndServer, &pid);

    // create a delivery ticket
    HANDLE hTicket = CreateNotificationParam(wEventId, uFlags, pidl1, pidl2, pid, dwTick);
    if (hTicket == NULL)
        return;

    TRACE("hTicket: %p, 0x%lx\n", hTicket, pid);

    // send the ticket by using CN_DELIVER_NOTIFICATION
    if ((uFlags & (SHCNF_FLUSH | SHCNF_FLUSHNOWAIT)) == SHCNF_FLUSH)
        SendMessageW(hwndServer, CN_DELIVER_NOTIFICATION, (WPARAM)hTicket, pid);
    else
        SendNotifyMessageW(hwndServer, CN_DELIVER_NOTIFICATION, (WPARAM)hTicket, pid);
}

/*************************************************************************
 * SHChangeNotifyRegister           [SHELL32.2]
 */
EXTERN_C ULONG WINAPI
SHChangeNotifyRegister(HWND hwnd, INT fSources, LONG wEventMask, UINT uMsg,
                       INT cItems, SHChangeNotifyEntry *lpItems)
{
    HWND hwndServer, hwndBroker = NULL;
    HANDLE hRegEntry;
    INT iItem;
    ULONG nRegID = INVALID_REG_ID;
    DWORD dwOwnerPID;
    LPREGENTRY pRegEntry;

    TRACE("(%p,0x%08x,0x%08x,0x%08x,%d,%p)\n",
          hwnd, fSources, wEventMask, uMsg, cItems, lpItems);

    // sanity check
    if (wEventMask == 0 || cItems <= 0 || cItems > 0x7FFF || lpItems == NULL ||
        hwnd == NULL || !IsWindow(hwnd))
    {
        return INVALID_REG_ID;
    }

    // request the window of the server
    hwndServer = GetNotificationServer(TRUE);
    if (hwndServer == NULL)
        return INVALID_REG_ID;

    // disable new delivery method in specific condition
    if ((fSources & SHCNRF_RecursiveInterrupt) != 0 &&
        (fSources & SHCNRF_InterruptLevel) == 0)
    {
        fSources &= ~SHCNRF_NewDelivery;
    }

    // if it is old delivery method, then create a broker window
    if ((fSources & SHCNRF_NewDelivery) == 0)
    {
        hwndBroker = hwnd = CreateNotificationBroker(hwnd, uMsg);
        uMsg = WM_BROKER_NOTIFICATION;
    }

    // The owner PID is the process ID of the server
    GetWindowThreadProcessId(hwndServer, &dwOwnerPID);

    EnterCriticalSection(&SHELL32_ChangenotifyCS);
    for (iItem = 0; iItem < cItems; ++iItem)
    {
        // create a registration entry
        hRegEntry = CreateRegistrationParam(nRegID, hwnd, uMsg, fSources, wEventMask,
                                            lpItems[iItem].fRecursive, lpItems[iItem].pidl,
                                            dwOwnerPID, hwndBroker);
        if (hRegEntry)
        {
            TRACE("CN_REGISTER: hwnd:%p, hRegEntry:%p, pid:0x%lx\n",
                  hwndServer, hRegEntry, dwOwnerPID);

            // send CN_REGISTER to the server
            SendMessageW(hwndServer, CN_REGISTER, (WPARAM)hRegEntry, dwOwnerPID);

            // update nRegID
            pRegEntry = (LPREGENTRY)SHLockSharedEx(hRegEntry, dwOwnerPID, FALSE);
            if (pRegEntry)
            {
                nRegID = pRegEntry->nRegID;
                SHUnlockShared(pRegEntry);
            }

            // free registration entry
            SHFreeShared(hRegEntry, dwOwnerPID);
        }

        // if failed, then destroy the broker
        if (nRegID == INVALID_REG_ID && (fSources & SHCNRF_NewDelivery) == 0)
        {
            ERR("Delivery failed\n");
            DestroyWindow(hwndBroker);
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

    // get the server window
    HWND hwndServer = GetNotificationServer(FALSE);
    if (hwndServer == NULL)
        return FALSE;

    // send CN_UNREGISTER message and try to unregister
    BOOL ret = (BOOL)SendMessageW(hwndServer, CN_UNREGISTER, hNotify, 0);
    if (!ret)
        ERR("CN_UNREGISTER failed\n");

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
        CreateNotificationParamAndSend(wEventId, uFlags, pidl1, pidl2, dwTick);
    }

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
