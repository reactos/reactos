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
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windef.h>
#include <winbase.h>
#include <shlobj.h>
#include <undocshell.h>
#include <shlwapi.h>
#include <wine/debug.h>
#include <wine/list.h>

#include "pidl.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

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

static struct list notifications = LIST_INIT( notifications );
static LONG next_id;

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

void InitChangeNotifications(void)
{
}

void FreeChangeNotifications(void)
{
    LPNOTIFICATIONLIST ptr, next;

    TRACE("\n");

    EnterCriticalSection(&SHELL32_ChangenotifyCS);

    LIST_FOR_EACH_ENTRY_SAFE( ptr, next, &notifications, NOTIFICATIONLIST, entry )
        DeleteNode( ptr );

    LeaveCriticalSection(&SHELL32_ChangenotifyCS);

    DeleteCriticalSection(&SHELL32_ChangenotifyCS);
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
}

/*************************************************************************
 * SHChangeNotifyDeregister			[SHELL32.4]
 */
BOOL WINAPI SHChangeNotifyDeregister(ULONG hNotify)
{
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

static BOOL should_notify( LPCITEMIDLIST changed, LPCITEMIDLIST watched, BOOL sub )
{
    TRACE("%p %p %d\n", changed, watched, sub );
    if ( !watched )
        return FALSE;
    if (ILIsEqual( watched, changed ) )
        return TRUE;
    if( sub && ILIsParent( watched, changed, FALSE ) )
        return TRUE;
#ifdef __REACTOS__
    if (sub && _ILIsDesktop(watched)) {
        LPITEMIDLIST deskpidl;
        WCHAR wszPath[MAX_PATH];
        SHGetSpecialFolderPathW(0, wszPath, CSIDL_DESKTOPDIRECTORY, FALSE);
        deskpidl = SHSimpleIDListFromPathW(wszPath);
        if (ILIsParent(deskpidl, changed, TRUE))
        {
            ILFree(deskpidl);
            return TRUE;
        }
        ILFree(deskpidl);
        SHGetSpecialFolderPathW(0, wszPath, CSIDL_COMMON_DESKTOPDIRECTORY, FALSE);
        deskpidl = SHSimpleIDListFromPathW(wszPath);
        if (ILIsParent(deskpidl, changed, TRUE))
        {
            ILFree(deskpidl);
            return TRUE;
        }
        ILFree(deskpidl);
    }
#endif
    return FALSE;
}

/*************************************************************************
 * SHChangeNotify				[SHELL32.@]
 */
void WINAPI SHChangeNotify(LONG wEventId, UINT uFlags, LPCVOID dwItem1, LPCVOID dwItem2)
{
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
          ( wEventId & ~SHCNE_NOITEMEVENTS ) ) ||
        ( ( wEventId & SHCNE_ONEITEMEVENTS ) && 
          ( wEventId & ~SHCNE_ONEITEMEVENTS ) ) ||
        ( ( wEventId & SHCNE_TWOITEMEVENTS ) && 
          ( wEventId & ~SHCNE_TWOITEMEVENTS ) ) )
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
#ifdef __REACTOS__
        if (wEventId & (SHCNE_MKDIR | SHCNE_RMDIR | SHCNE_UPDATEDIR | SHCNE_RENAMEFOLDER))
        {
            /*
             * The last items in the ID are currently files. So we chop off the last
             * entry, and create a new one using a find data struct.
             */
            if (dwItem1 && Pidls[0]){
                WIN32_FIND_DATAW wfd;
                LPITEMIDLIST oldpidl, newpidl;
                LPWSTR p = PathFindFileNameW((LPCWSTR)dwItem1);
                ILRemoveLastID(Pidls[0]);
                lstrcpynW(&wfd.cFileName[0], p, MAX_PATH);
                wfd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
                newpidl = _ILCreateFromFindDataW(&wfd);
                oldpidl = ILClone(Pidls[0]);
                ILFree(Pidls[0]);
                Pidls[0] = ILCombine(oldpidl, newpidl);
                ILFree(newpidl);
                ILFree(oldpidl);
            }
            if (dwItem2 && Pidls[1]){
                WIN32_FIND_DATAW wfd;
                LPITEMIDLIST oldpidl, newpidl;
                LPWSTR p = PathFindFileNameW((LPCWSTR)dwItem2);
                ILRemoveLastID(Pidls[1]);
                lstrcpynW(&wfd.cFileName[0], p, MAX_PATH);
                wfd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
                newpidl = _ILCreateFromFindDataW(&wfd);
                oldpidl = ILClone(Pidls[0]);
                ILFree(Pidls[1]);
                Pidls[1] = ILCombine(oldpidl, newpidl);
                ILFree(newpidl);
                ILFree(oldpidl);
            }
        }
#endif
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

#ifndef __REACTOS__
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
}

/*************************************************************************
 * SHChangeNotification_Unlock			[SHELL32.645]
 */
BOOL WINAPI SHChangeNotification_Unlock ( HANDLE hLock)
{
    TRACE("%p\n", hLock);
    return SHUnlockShared(hLock);
}

/*************************************************************************
 * NTSHChangeNotifyDeregister			[SHELL32.641]
 */
DWORD WINAPI NTSHChangeNotifyDeregister(ULONG x1)
{
    FIXME("(0x%08x):semi stub.\n",x1);

    return SHChangeNotifyDeregister( x1 );
}
