/*
 *	shell change notification
 *
 * Copyright 2000 Juergen Schmied
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#define BUFFER_SIZE 1024

#include <windef.h>
#include <winbase.h>
#include <shlobj.h>
#include <strsafe.h>
#include <undocshell.h>
#include <shlwapi.h>
#include <wine/debug.h>
#include <wine/list.h>
#include <process.h>
#include <shellutils.h>

#include "pidl.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);
#ifdef __REACTOS__
    #include "../shelldesktop/CChangeNotify.h"
    DWORD WINAPI SHAnsiToUnicode(LPCSTR lpSrcStr, LPWSTR lpDstStr, int iLen);
#endif

static CRITICAL_SECTION SHELL32_ChangenotifyCS;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &SHELL32_ChangenotifyCS,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": SHELL32_ChangenotifyCS") }
};
static CRITICAL_SECTION SHELL32_ChangenotifyCS = { &critsect_debug, -1, 0, 0, 0, 0 };

typedef SHChangeNotifyEntry *LPNOTIFYREGISTER;

/* internal list of notification clients (internal) */
typedef struct _NOTIFICATIONLIST
{
	struct list entry;
	HWND hwnd;		/* window to notify */
	DWORD uMsg;		/* message to send */
	LPNOTIFYREGISTER apidl; /* array of entries to watch*/
	UINT cidl;		/* number of pidls in array */
	LONG wEventMask;	/* subscribed events */
	DWORD dwFlags;		/* client flags */
	ULONG id;
} NOTIFICATIONLIST, *LPNOTIFICATIONLIST;

#ifndef __REACTOS__
static struct list notifications = LIST_INIT( notifications );
static LONG next_id;
#endif  /* ndef __REACTOS__ */

#define SHCNE_NOITEMEVENTS ( \
   SHCNE_ASSOCCHANGED )

#define SHCNE_ONEITEMEVENTS ( \
   SHCNE_ATTRIBUTES | SHCNE_CREATE | SHCNE_DELETE | SHCNE_DRIVEADD | \
   SHCNE_DRIVEADDGUI | SHCNE_DRIVEREMOVED | SHCNE_FREESPACE | \
   SHCNE_MEDIAINSERTED | SHCNE_MEDIAREMOVED | SHCNE_MKDIR | \
   SHCNE_NETSHARE | SHCNE_NETUNSHARE | SHCNE_RMDIR | \
   SHCNE_SERVERDISCONNECT | SHCNE_UPDATEDIR | SHCNE_UPDATEIMAGE )

#define SHCNE_TWOITEMEVENTS ( \
   SHCNE_RENAMEFOLDER | SHCNE_RENAMEITEM | SHCNE_UPDATEITEM )

/* for dumping events */
static const char * DumpEvent( LONG event )
{
    if( event == SHCNE_ALLEVENTS )
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

#ifndef __REACTOS__
static const char * NodeName(const NOTIFICATIONLIST *item)
{
    const char *str;
    WCHAR path[MAX_PATH];

    if(SHGetPathFromIDListW(item->apidl[0].pidl, path ))
        str = wine_dbg_sprintf("%s", debugstr_w(path));
    else
        str = wine_dbg_sprintf("<not a disk file>" );
    return str;
}

static void DeleteNode(LPNOTIFICATIONLIST item)
{
    UINT i;

    TRACE("item=%p\n", item);

    /* remove item from list */
    list_remove( &item->entry );

    /* free the item */
    for (i=0; i<item->cidl; i++)
        SHFree((LPITEMIDLIST)item->apidl[i].pidl);
    SHFree(item->apidl);
    SHFree(item);
}
#endif  /* ndef __REACTOS__ */

void InitChangeNotifications(void)
{
}

void FreeChangeNotifications(void)
{
#ifdef __REACTOS__
    HWND hwndWorker;
    hwndWorker = DoGetNewDeliveryWorker();
    SendMessageW(hwndWorker, WM_NOTIF_REMOVEBYPID, GetCurrentProcessId(), 0);
    DeleteCriticalSection(&SHELL32_ChangenotifyCS);
#else
    LPNOTIFICATIONLIST ptr, next;

    TRACE("\n");

    EnterCriticalSection(&SHELL32_ChangenotifyCS);

    LIST_FOR_EACH_ENTRY_SAFE( ptr, next, &notifications, NOTIFICATIONLIST, entry )
        DeleteNode( ptr );

    LeaveCriticalSection(&SHELL32_ChangenotifyCS);

    DeleteCriticalSection(&SHELL32_ChangenotifyCS);
#endif
}

/*************************************************************************
 * SHChangeNotifyRegister			[SHELL32.2]
 *
 */
ULONG WINAPI
SHChangeNotifyRegister(
    HWND hwnd,
    int fSources,
    LONG wEventMask,
    UINT uMsg,
    int cItems,
    SHChangeNotifyEntry *lpItems)
{
#ifdef __REACTOS__
    HWND hwndNotif;
    HANDLE hShared;
    INT iItem;
    ULONG nRegID = INVALID_REG_ID;
    DWORD dwOwnerPID;
    LPNOTIFSHARE pShare;

    TRACE("(%p,0x%08x,0x%08x,0x%08x,%d,%p)\n",
          hwnd, fSources, wEventMask, uMsg, cItems, lpItems);

    hwndNotif = DoGetNewDeliveryWorker();
    if (hwndNotif == NULL)
        return INVALID_REG_ID;

    if ((fSources & SHCNRF_NewDelivery) == 0)
    {
        hwnd = (HWND)DoHireOldDeliveryWorker(hwnd, uMsg);
        uMsg = WM_OLDDELI_HANDOVER;
    }

    if ((fSources & SHCNRF_RecursiveInterrupt) != 0 &&
        (fSources & SHCNRF_InterruptLevel) == 0)
    {
        fSources &= ~SHCNRF_NewDelivery;
    }

    GetWindowThreadProcessId(hwndNotif, &dwOwnerPID);

    EnterCriticalSection(&SHELL32_ChangenotifyCS);
    for (iItem = 0; iItem < cItems; ++iItem)
    {
        hShared = DoCreateNotifShare(nRegID, hwnd, uMsg, fSources, wEventMask,
                                     lpItems[iItem].fRecursive, lpItems[iItem].pidl,
                                     dwOwnerPID);
        if (hShared)
        {
            TRACE("WM_NOTIF_REG: hwnd:%p, hShared:%p, pid:0x%lx\n", hwndNotif, hShared, dwOwnerPID);
            SendMessageW(hwndNotif, WM_NOTIF_REG, (WPARAM)hShared, dwOwnerPID);

            pShare = (LPNOTIFSHARE)SHLockSharedEx(hShared, dwOwnerPID, FALSE);
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
#else
    LPNOTIFICATIONLIST item;
    int i;

    item = SHAlloc(sizeof(NOTIFICATIONLIST));

    TRACE("(%p,0x%08x,0x%08x,0x%08x,%d,%p) item=%p\n",
	hwnd, fSources, wEventMask, uMsg, cItems, lpItems, item);

    item->cidl = cItems;
    item->apidl = SHAlloc(sizeof(SHChangeNotifyEntry) * cItems);
    for(i=0;i<cItems;i++)
    {
        item->apidl[i].pidl = ILClone(lpItems[i].pidl);
        item->apidl[i].fRecursive = lpItems[i].fRecursive;
    }
    item->hwnd = hwnd;
    item->uMsg = uMsg;
    item->wEventMask = wEventMask;
    item->dwFlags = fSources;
    item->id = InterlockedIncrement( &next_id );

    TRACE("new node: %s\n", NodeName( item ));

    EnterCriticalSection(&SHELL32_ChangenotifyCS);

    list_add_tail( &notifications, &item->entry );

    LeaveCriticalSection(&SHELL32_ChangenotifyCS);

    return item->id;
#endif
}

/*************************************************************************
 * SHChangeNotifyDeregister			[SHELL32.4]
 */
BOOL WINAPI SHChangeNotifyDeregister(ULONG hNotify)
{
#ifdef __REACTOS__
    HWND hwndNotif;
    LRESULT ret = 0;
    TRACE("(0x%08x)\n", hNotify);

    hwndNotif = DoGetNewDeliveryWorker();
    if (hwndNotif)
    {
        ret = SendMessageW(hwndNotif, WM_NOTIF_UNREG, hNotify, 0);
        if (!ret)
        {
            ERR("WM_NOTIF_UNREG failed\n");
        }
    }
    return ret;
#else
    LPNOTIFICATIONLIST node;

    TRACE("(0x%08x)\n", hNotify);

    EnterCriticalSection(&SHELL32_ChangenotifyCS);

    LIST_FOR_EACH_ENTRY( node, &notifications, NOTIFICATIONLIST, entry )
    {
        if (node->id == hNotify)
        {
            DeleteNode( node );
            LeaveCriticalSection(&SHELL32_ChangenotifyCS);
            return TRUE;
        }
    }
    LeaveCriticalSection(&SHELL32_ChangenotifyCS);
    return FALSE;
#endif
}

/*************************************************************************
 * SHChangeNotifyUpdateEntryList       		[SHELL32.5]
 */
BOOL WINAPI SHChangeNotifyUpdateEntryList(DWORD unknown1, DWORD unknown2,
			      DWORD unknown3, DWORD unknown4)
{
    FIXME("(0x%08x, 0x%08x, 0x%08x, 0x%08x)\n",
          unknown1, unknown2, unknown3, unknown4);

    return TRUE;
}

struct new_delivery_notification
{
    LONG event;
    DWORD pidl1_size;
    DWORD pidl2_size;
    LPITEMIDLIST pidls[2];
    BYTE data[1];
};

#ifndef __REACTOS__
static BOOL should_notify( LPCITEMIDLIST changed, LPCITEMIDLIST watched, BOOL sub )
{
    TRACE("%p %p %d\n", changed, watched, sub );
    if ( !watched )
        return FALSE;
    if (ILIsEqual( watched, changed ) )
        return TRUE;
    if( sub && ILIsParent( watched, changed, FALSE ) )
        return TRUE;
    return FALSE;
}
#else   /* def __REACTOS__ */
LPITEMIDLIST PidlFromPathW(LPCWSTR path)
{
#if 1 /* FIXME: This is a workaround to fix display icon. */
    if (PathFileExistsW(path))
        return ILCreateFromPathW(path);
#endif
    return SHSimpleIDListFromPathW(path);
}
#endif  /* def __REACTOS__ */

/*************************************************************************
 * SHChangeNotify				[SHELL32.@]
 */
void WINAPI SHChangeNotify(LONG wEventId, UINT uFlags, LPCVOID dwItem1, LPCVOID dwItem2)
{
#ifdef __REACTOS__
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
                DoNotifyFreeSpace(dwItem1, dwItem2);
                goto Quit;
            }
            pidl1 = (LPITEMIDLIST)dwItem1;
            pidl2 = (LPITEMIDLIST)dwItem2;
            break;

        case SHCNF_PATHA:
            psz1 = psz2 = NULL;
            if (dwItem1)
            {
                SHAnsiToUnicode(dwItem1, szPath1, ARRAYSIZE(szPath1));
                psz1 = szPath1;
            }
            if (dwItem2)
            {
                SHAnsiToUnicode(dwItem2, szPath2, ARRAYSIZE(szPath2));
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
                pidl1 = pidlTemp1 = PidlFromPathW(dwItem1);
            }
            if (dwItem2)
            {
                pidl2 = pidlTemp2 = PidlFromPathW(dwItem2);
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
        DoTransportChange(wEventId, uFlags, pidl1, pidl2, dwTick);
    }

Quit:
    if (pidlTemp1)
        ILFree(pidlTemp1);
    if (pidlTemp2)
        ILFree(pidlTemp2);
#else
    struct notification_recipients {
        struct list entry;
        HWND hwnd;
        DWORD msg;
        DWORD flags;
    } *cur, *next;

    HANDLE shared_data = NULL;
    LPITEMIDLIST Pidls[2];
    LPNOTIFICATIONLIST ptr;
    struct list recipients;

    Pidls[0] = NULL;
    Pidls[1] = NULL;

    TRACE("(0x%08x,0x%08x,%p,%p)\n", wEventId, uFlags, dwItem1, dwItem2);

    if(uFlags & ~(SHCNF_TYPE|SHCNF_FLUSH))
        FIXME("ignoring unsupported flags: %x\n", uFlags);

    if( ( wEventId & SHCNE_NOITEMEVENTS ) && ( dwItem1 || dwItem2 ) )
    {
        TRACE("dwItem1 and dwItem2 are not zero, but should be\n");
        dwItem1 = 0;
        dwItem2 = 0;
        return;
    }
    else if( ( wEventId & SHCNE_ONEITEMEVENTS ) && dwItem2 )
    {
        TRACE("dwItem2 is not zero, but should be\n");
        dwItem2 = 0;
        return;
    }

    if( ( ( wEventId & SHCNE_NOITEMEVENTS ) && 
          ( wEventId & ~(SHCNE_NOITEMEVENTS | SHCNE_INTERRUPT) ) ) ||
        ( ( wEventId & SHCNE_ONEITEMEVENTS ) && 
          ( wEventId & ~(SHCNE_ONEITEMEVENTS | SHCNE_INTERRUPT) ) ) ||
        ( ( wEventId & SHCNE_TWOITEMEVENTS ) && 
          ( wEventId & ~(SHCNE_TWOITEMEVENTS | SHCNE_INTERRUPT) ) ) )
    {
        WARN("mutually incompatible events listed\n");
        return;
    }

    /* convert paths in IDLists*/
    switch (uFlags & SHCNF_TYPE)
    {
    case SHCNF_PATHA:
        if (dwItem1) Pidls[0] = SHSimpleIDListFromPathA(dwItem1); //FIXME
        if (dwItem2) Pidls[1] = SHSimpleIDListFromPathA(dwItem2); //FIXME
        break;
    case SHCNF_PATHW:
        if (dwItem1) Pidls[0] = SHSimpleIDListFromPathW(dwItem1);
        if (dwItem2) Pidls[1] = SHSimpleIDListFromPathW(dwItem2);
        break;
    case SHCNF_IDLIST:
        Pidls[0] = ILClone(dwItem1);
        Pidls[1] = ILClone(dwItem2);
        break;
    case SHCNF_PRINTERA:
    case SHCNF_PRINTERW:
        FIXME("SHChangeNotify with (uFlags & SHCNF_PRINTER)\n");
        return;
    case SHCNF_DWORD:
    default:
        FIXME("unknown type %08x\n", uFlags & SHCNF_TYPE);
        return;
    }

    list_init(&recipients);
    EnterCriticalSection(&SHELL32_ChangenotifyCS);
    LIST_FOR_EACH_ENTRY( ptr, &notifications, NOTIFICATIONLIST, entry )
    {
        struct notification_recipients *item;
        BOOL notify = FALSE;
        DWORD i;

        for( i=0; (i<ptr->cidl) && !notify ; i++ )
        {
            LPCITEMIDLIST pidl = ptr->apidl[i].pidl;
            BOOL subtree = ptr->apidl[i].fRecursive;

            if (wEventId & ptr->wEventMask)
            {
                if( !pidl )          /* all ? */
                    notify = TRUE;
                else if( wEventId & SHCNE_NOITEMEVENTS )
                    notify = TRUE;
                else if( wEventId & ( SHCNE_ONEITEMEVENTS | SHCNE_TWOITEMEVENTS ) )
                    notify = should_notify( Pidls[0], pidl, subtree );
                else if( wEventId & SHCNE_TWOITEMEVENTS )
                    notify = should_notify( Pidls[1], pidl, subtree );
            }
        }

        if( !notify )
            continue;

        item = SHAlloc(sizeof(struct notification_recipients));
        if(!item) {
            ERR("out of memory\n");
            continue;
        }

        item->hwnd = ptr->hwnd;
        item->msg = ptr->uMsg;
        item->flags = ptr->dwFlags;
        list_add_tail(&recipients, &item->entry);
    }
    LeaveCriticalSection(&SHELL32_ChangenotifyCS);

    LIST_FOR_EACH_ENTRY_SAFE(cur, next, &recipients, struct notification_recipients, entry)
    {
        TRACE("notifying %p, event %s(%x)\n", cur->hwnd, DumpEvent(wEventId), wEventId);

        if (cur->flags  & SHCNRF_NewDelivery) {
            if(!shared_data) {
                struct new_delivery_notification *notification;
                UINT size1 = ILGetSize(Pidls[0]), size2 = ILGetSize(Pidls[1]);
                UINT offset = (size1+sizeof(int)-1)/sizeof(int)*sizeof(int);

                notification = SHAlloc(sizeof(struct new_delivery_notification)+offset+size2);
                if(!notification) {
                    ERR("out of memory\n");
                } else {
                    notification->event = wEventId;
                    notification->pidl1_size = size1;
                    notification->pidl2_size = size2;
                    if(size1)
                        memcpy(notification->data, Pidls[0], size1);
                    if(size2)
                        memcpy(notification->data+offset, Pidls[1], size2);

                    shared_data = SHAllocShared(notification,
                        sizeof(struct new_delivery_notification)+size1+size2,
                        GetCurrentProcessId());
                    SHFree(notification);
                }
            }

            if(shared_data)
                SendMessageA(cur->hwnd, cur->msg, (WPARAM)shared_data, GetCurrentProcessId());
            else
                ERR("out of memory\n");
        } else {
            SendMessageA(cur->hwnd, cur->msg, (WPARAM)Pidls, wEventId);
        }

        list_remove(&cur->entry);
        SHFree(cur);
    }
    SHFreeShared(shared_data, GetCurrentProcessId());
    SHFree(Pidls[0]);
    SHFree(Pidls[1]);

    if (wEventId & SHCNE_ASSOCCHANGED)
    {
        static const WCHAR args[] = {' ','-','a',0 };
        TRACE("refreshing file type associations\n");
        run_winemenubuilder( args );
    }
#endif
}

/*************************************************************************
 * NTSHChangeNotifyRegister            [SHELL32.640]
 * NOTES
 *   Idlist is an array of structures and Count specifies how many items in the array.
 *   count should always be one when calling SHChangeNotifyRegister, or
 *   SHChangeNotifyDeregister will not work properly.
 */
EXTERN_C ULONG WINAPI NTSHChangeNotifyRegister(
    HWND hwnd,
    int fSources,
    LONG fEvents,
    UINT msg,
    int count,
    SHChangeNotifyEntry *idlist)
{
    return SHChangeNotifyRegister(hwnd, fSources | SHCNRF_NewDelivery,
                                  fEvents, msg, count, idlist);
}

/*************************************************************************
 * SHChangeNotification_Lock			[SHELL32.644]
 */
HANDLE WINAPI SHChangeNotification_Lock(
	HANDLE hChange,
	DWORD dwProcessId,
	LPITEMIDLIST **lppidls,
	LPLONG lpwEventId)
{
#ifdef __REACTOS__
    LPHANDBAG pHandbag;
    TRACE("%p %08x %p %p\n", hChange, dwProcessId, lppidls, lpwEventId);

    pHandbag = DoGetHandbagFromTicket(hChange, dwProcessId);
    if (!pHandbag || pHandbag->dwMagic != HANDBAG_MAGIC)
    {
        ERR("pHandbag is invalid\n");
        return NULL;
    }

    if (lppidls)
        *lppidls = &pHandbag->pidl1;

    if (lpwEventId)
        *lpwEventId = pHandbag->pTicket->wEventId;

    return pHandbag;
#else
    struct new_delivery_notification *ndn;
    UINT offset;

    TRACE("%p %08x %p %p\n", hChange, dwProcessId, lppidls, lpwEventId);

    ndn = SHLockShared(hChange, dwProcessId);
    if(!ndn) {
        WARN("SHLockShared failed\n");
        return NULL;
    }

    if(lppidls) {
        offset = (ndn->pidl1_size+sizeof(int)-1)/sizeof(int)*sizeof(int);
        ndn->pidls[0] = ndn->pidl1_size ? (LPITEMIDLIST)ndn->data : NULL;
        ndn->pidls[1] = ndn->pidl2_size ? (LPITEMIDLIST)(ndn->data+offset) : NULL;
        *lppidls = ndn->pidls;
    }

    if(lpwEventId)
        *lpwEventId = ndn->event;

    return ndn;
#endif
}

/*************************************************************************
 * SHChangeNotification_Unlock			[SHELL32.645]
 */
BOOL WINAPI SHChangeNotification_Unlock ( HANDLE hLock)
{
#ifdef __REACTOS__
    LPHANDBAG pHandbag = (LPHANDBAG)hLock;
    BOOL ret;
    TRACE("%p\n", hLock);

    if (!pHandbag || pHandbag->dwMagic != HANDBAG_MAGIC)
    {
        ERR("pHandbag is invalid\n");
        return FALSE;
    }

    ret = SHUnlockShared(pHandbag->pTicket);
    LocalFree(hLock);
    return ret;
#else
    TRACE("%p\n", hLock);
    return SHUnlockShared(hLock);
#endif
}

/*************************************************************************
 * NTSHChangeNotifyDeregister			[SHELL32.641]
 */
DWORD WINAPI NTSHChangeNotifyDeregister(ULONG x1)
{
    FIXME("(0x%08x):semi stub.\n",x1);

    return SHChangeNotifyDeregister( x1 );
}
