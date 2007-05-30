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

#include <stdarg.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"
#include "wingdi.h"
#include "shell32_main.h"

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
	struct _NOTIFICATIONLIST *next;
	struct _NOTIFICATIONLIST *prev;
	HWND hwnd;		/* window to notify */
	DWORD uMsg;		/* message to send */
	LPNOTIFYREGISTER apidl; /* array of entries to watch*/
	UINT cidl;		/* number of pidls in array */
	LONG wEventMask;	/* subscribed events */
	LONG wSignalledEvent;   /* event that occurred */
	DWORD dwFlags;		/* client flags */
	LPCITEMIDLIST pidlSignaled; /*pidl of the path that caused the signal*/
    
} NOTIFICATIONLIST, *LPNOTIFICATIONLIST;

static NOTIFICATIONLIST *head, *tail;

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

static const char * NodeName(LPNOTIFICATIONLIST item)
{
    const char *str;
    WCHAR path[MAX_PATH];

    if(SHGetPathFromIDListW(item->apidl[0].pidl, path ))
        str = wine_dbg_sprintf("%s", debugstr_w(path));
    else
        str = wine_dbg_sprintf("<not a disk file>" );
    return str;
}

static void AddNode(LPNOTIFICATIONLIST item)
{
    TRACE("item %p\n", item );

    /* link items */
    item->prev = tail;
    item->next = NULL;
    if( tail )
        tail->next = item;
    else
        head = item;
    tail = item;
}

static LPNOTIFICATIONLIST FindNode( HANDLE hitem )
{
    LPNOTIFICATIONLIST ptr;
    for( ptr = head; ptr; ptr = ptr->next )
        if( ptr == (LPNOTIFICATIONLIST) hitem )
            return ptr;
    return NULL;
}

static void DeleteNode(LPNOTIFICATIONLIST item)
{
    UINT i;

    TRACE("item=%p prev=%p next=%p\n", item, item->prev, item->next);

    /* remove item from list */
    if( item->prev )
        item->prev->next = item->next;
    else
        head = item->next;
    if( item->next )
        item->next->prev = item->prev;
    else
        tail = item->prev;

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
    TRACE("\n");

    EnterCriticalSection(&SHELL32_ChangenotifyCS);

    while( head )
        DeleteNode( head );

    LeaveCriticalSection(&SHELL32_ChangenotifyCS);

    // DeleteCriticalSection(&SHELL32_ChangenotifyCS); // static
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

    item->next = NULL;
    item->prev = NULL;
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
    item->wSignalledEvent = 0;
    item->dwFlags = fSources;

    TRACE("new node: %s\n", NodeName( item ));

    EnterCriticalSection(&SHELL32_ChangenotifyCS);

    AddNode(item);

    LeaveCriticalSection(&SHELL32_ChangenotifyCS);

    return (ULONG)item;
}

/*************************************************************************
 * SHChangeNotifyDeregister			[SHELL32.4]
 */
BOOL WINAPI SHChangeNotifyDeregister(ULONG hNotify)
{
    LPNOTIFICATIONLIST node;

    TRACE("(0x%08x)\n", hNotify);

    EnterCriticalSection(&SHELL32_ChangenotifyCS);

    node = FindNode((HANDLE)hNotify);
    if( node )
        DeleteNode(node);

    LeaveCriticalSection(&SHELL32_ChangenotifyCS);

    return node?TRUE:FALSE;
}

/*************************************************************************
 * SHChangeNotifyUpdateEntryList       		[SHELL32.5]
 */
BOOL WINAPI SHChangeNotifyUpdateEntryList(DWORD unknown1, DWORD unknown2,
			      DWORD unknown3, DWORD unknown4)
{
    FIXME("(0x%08x, 0x%08x, 0x%08x, 0x%08x)\n",
          unknown1, unknown2, unknown3, unknown4);

    return -1;
}

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

/*************************************************************************
 * SHChangeNotify				[SHELL32.@]
 */
void WINAPI SHChangeNotify(LONG wEventId, UINT uFlags, LPCVOID dwItem1, LPCVOID dwItem2)
{
    LPCITEMIDLIST Pidls[2];
    LPNOTIFICATIONLIST ptr;
    UINT typeFlag = uFlags & SHCNF_TYPE;

    Pidls[0] = NULL;
    Pidls[1] = NULL;

    TRACE("(0x%08x,0x%08x,%p,%p):stub.\n", wEventId, uFlags, dwItem1, dwItem2);

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
    switch (typeFlag)
    {
    case SHCNF_PATHA:
        if (dwItem1) Pidls[0] = SHSimpleIDListFromPathA((LPCSTR)dwItem1);
        if (dwItem2) Pidls[1] = SHSimpleIDListFromPathA((LPCSTR)dwItem2);
        break;
    case SHCNF_PATHW:
        if (dwItem1) Pidls[0] = SHSimpleIDListFromPathW((LPCWSTR)dwItem1);
        if (dwItem2) Pidls[1] = SHSimpleIDListFromPathW((LPCWSTR)dwItem2);
        break;
    case SHCNF_IDLIST:
        Pidls[0] = (LPCITEMIDLIST)dwItem1;
        Pidls[1] = (LPCITEMIDLIST)dwItem2;
        break;
    case SHCNF_PRINTERA:
    case SHCNF_PRINTERW:
        FIXME("SHChangeNotify with (uFlags & SHCNF_PRINTER)\n");
        return;
    case SHCNF_DWORD:
    default:
        FIXME("unknown type %08x\n",typeFlag);
        return;
    }

    {
        WCHAR path[MAX_PATH];

        if( Pidls[0] && SHGetPathFromIDListW(Pidls[0], path ))
            TRACE("notify %08x on item1 = %s\n", wEventId, debugstr_w(path));
    
        if( Pidls[1] && SHGetPathFromIDListW(Pidls[1], path ))
            TRACE("notify %08x on item2 = %s\n", wEventId, debugstr_w(path));
    }

    EnterCriticalSection(&SHELL32_ChangenotifyCS);

    /* loop through the list */
    for( ptr = head; ptr; ptr = ptr->next )
    {
        BOOL notify;
        DWORD i;

        notify = FALSE;

        TRACE("trying %p\n", ptr);

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

        ptr->pidlSignaled = ILClone(Pidls[0]);

        TRACE("notifying %s, event %s(%x) before\n", NodeName( ptr ), DumpEvent(
               wEventId ),wEventId );

        ptr->wSignalledEvent |= wEventId;

        if (ptr->dwFlags  & SHCNRF_NewDelivery)
            SendMessageA(ptr->hwnd, ptr->uMsg, (WPARAM) ptr, (LPARAM) GetCurrentProcessId());
        else
            SendMessageA(ptr->hwnd, ptr->uMsg, (WPARAM)Pidls, wEventId);

        TRACE("notifying %s, event %s(%x) after\n", NodeName( ptr ), DumpEvent(
                wEventId ),wEventId );

    }
    TRACE("notify Done\n");
    LeaveCriticalSection(&SHELL32_ChangenotifyCS);
    
    /* if we allocated it, free it. The ANSI flag is also set in its Unicode sibling. */
    if ((typeFlag & SHCNF_PATHA) || (typeFlag & SHCNF_PRINTERA))
    {
        SHFree((LPITEMIDLIST)Pidls[0]);
        SHFree((LPITEMIDLIST)Pidls[1]);
    }
}

/*************************************************************************
 * NTSHChangeNotifyRegister			[SHELL32.640]
 * NOTES
 *   Idlist is an array of structures and Count specifies how many items in the array
 *   (usually just one I think).
 */
DWORD WINAPI NTSHChangeNotifyRegister(
    HWND hwnd,
    LONG events1,
    LONG events2,
    DWORD msg,
    int count,
    SHChangeNotifyEntry *idlist)
{
    FIXME("(%p,0x%08x,0x%08x,0x%08x,0x%08x,%p):semi stub.\n",
		hwnd,events1,events2,msg,count,idlist);

    return (DWORD) SHChangeNotifyRegister(hwnd, events1, events2, msg, count, idlist);
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
    DWORD i;
    LPNOTIFICATIONLIST node;
    LPCITEMIDLIST *idlist;

    TRACE("%p %08x %p %p\n", hChange, dwProcessId, lppidls, lpwEventId);

    /* EnterCriticalSection(&SHELL32_ChangenotifyCS); */

    node = FindNode( hChange );
    if( node )
    {
        idlist = SHAlloc( sizeof(LPCITEMIDLIST *) * node->cidl );
        for(i=0; i<node->cidl; i++)
            idlist[i] = (LPCITEMIDLIST)node->pidlSignaled;
        *lpwEventId = node->wSignalledEvent;
        *lppidls = (LPITEMIDLIST*)idlist;
        node->wSignalledEvent = 0;
    }
    else
        ERR("Couldn't find %p\n", hChange );

    /* LeaveCriticalSection(&SHELL32_ChangenotifyCS); */

    return (HANDLE) node;
}

/*************************************************************************
 * SHChangeNotification_Unlock			[SHELL32.645]
 */
BOOL WINAPI SHChangeNotification_Unlock ( HANDLE hLock)
{
    TRACE("\n");
    return 1;
}

/*************************************************************************
 * NTSHChangeNotifyDeregister			[SHELL32.641]
 */
DWORD WINAPI NTSHChangeNotifyDeregister(ULONG x1)
{
    FIXME("(0x%08x):semi stub.\n",x1);

    return SHChangeNotifyDeregister( x1 );
}
