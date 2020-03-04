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
#define SHARE_MUTEX_NAME L"Shell32ShareMutex"
#define LINKHUB_GROW 5

typedef struct ITEM
{
    BOOL fRecursive;
    WCHAR szPath[MAX_PATH];
} ITEM, *LPITEM;

typedef struct BLOCK
{
    HWND hwnd;
    INT fSources;
    LONG fEvents;
    UINT uMsg;
    INT cItems;
    ITEM Items[1];
} BLOCK, *LPBLOCK;

typedef struct LINK
{
    DWORD dwUserPID;
    DWORD dwOwnerPID;
    UINT nRegID;
    HANDLE hBlock;
} LINK, *LPLINK;

typedef struct LINKHUB
{
    INT nCapacity;
    INT nCount;
    LINK Links[1];
} LINKHUB, *LPLINKHUB;

/* shared data section */

#ifdef _MSC_VER
    #define SHELL32SHARE
#else
    #define SHELL32SHARE __attribute__((section(".shared"), shared))
#endif
#ifdef _MSC_VER
    #pragma data_seg(".shared")
#endif
static HANDLE s_hLinkHub SHELL32SHARE = NULL;
static DWORD s_dwLinkOwnerPID SHELL32SHARE = 0;
static UINT s_nNextRegID SHELL32SHARE = 0;
#ifdef _MSC_VER
    #pragma data_seg()
    #pragma comment(linker, "/SECTION:.shared,RWS")
#endif

static BOOL
IsProcessRunning(DWORD pid)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, TRUE, pid);
    if (hProcess)
        CloseHandle(hProcess);
    return hProcess != NULL;
}

static DWORD
LinkHub_GetSize(INT nCount)
{
    return sizeof(LINKHUB) + (nCount - 1) * sizeof(LINK);
}

static LPLINKHUB
LinkHub_Lock(HANDLE hLinkHub, DWORD dwOwnerPID)
{
    if (!hLinkHub || !dwOwnerPID || !IsProcessRunning(dwOwnerPID))
        return NULL;

    return (LPLINKHUB)SHLockShared(hLinkHub, dwOwnerPID);
}

static void
LinkHub_Unlock(LPLINKHUB pLinkHub)
{
    if (!pLinkHub)
        return;

    SHUnlockShared(pLinkHub);
}

static DWORD
Block_GetSize(INT nEntries)
{
    return sizeof(BLOCK) + (nEntries - 1) * sizeof(ITEM);
}

static BLOCK *
Block_Lock(HANDLE hShare, DWORD pid)
{
    if (!hShare || !pid || !IsProcessRunning(pid))
        return NULL;

    return (LPBLOCK)SHLockShared(hShare, pid);
}

static void
Block_Unlock(LPVOID block)
{
    if (!block)
        return;

    SHUnlockShared(block);
}

static void
Block_Destroy(HANDLE hShare, DWORD pid)
{
    if (!hShare || !pid || !IsProcessRunning(pid))
        return;

    SHFreeShared(hShare, pid);
}

static BOOL
LinkHub_RemoveByRegID(LPLINKHUB pLinkHub, UINT nRegID)
{
    BOOL ret = FALSE;
    INT iLink;
    LPLINK pLink;
    for (iLink = 0; iLink < pLinkHub->nCapacity; ++iLink)
    {
        pLink = &pLinkHub->Links[iLink];
        if (!pLink->nRegID)
            continue;

        if (pLink->nRegID == nRegID)
        {
            Block_Destroy(pLink->hBlock, pLink->dwOwnerPID);
            ZeroMemory(pLink, sizeof(*pLink));
            pLinkHub->nCount--;
            ret = TRUE;
            break;
        }
    }
    return ret;
}

static BOOL
DoRemoveBlockByRegID(UINT nRegID)
{
    BOOL ret = FALSE;
    LPLINKHUB pLinkHub = LinkHub_Lock(s_hLinkHub, s_dwLinkOwnerPID);
    if (pLinkHub)
    {
        ret = LinkHub_RemoveByRegID(pLinkHub, nRegID);
        LinkHub_Unlock(pLinkHub);
    }
    return ret;
}

static UINT
LinkHub_Add(LPLINKHUB pLinkHub, DWORD pid, LPBLOCK block)
{
    INT iLink;
    LPLINK pLink;
    for (iLink = 0; iLink < pLinkHub->nCapacity; ++iLink)
    {
        pLink = &pLinkHub->Links[iLink];
        if (pLink->nRegID != 0)
            continue;

        pLink->hBlock = SHAllocShared(block, Block_GetSize(block->cItems), pid);
        if (pLink->hBlock)
        {
            pLink->dwUserPID = pLink->dwOwnerPID = pid;
            pLink->nRegID = ++s_nNextRegID;
            pLinkHub->nCount++;
            return pLink->nRegID;
        }
    }
    return 0;
}

static UINT
LinkHub_GrowAdd(LPLINKHUB pOldLinkHub, INT nNewCapacity, DWORD pid, LPBLOCK block)
{
    UINT nRegID = 0;
    DWORD cbOldLinkHub = LinkHub_GetSize(pOldLinkHub->nCapacity);
    DWORD cbNewLinkHub = LinkHub_GetSize(nNewCapacity);
    HANDLE hNewLinkHub;
    LPLINKHUB pNewLinkHub;

    hNewLinkHub = SHAllocShared(NULL, cbNewLinkHub, pid);
    if (!hNewLinkHub)
    {
        ERR("!hNewLinkHub\n");
        return nRegID;
    }

    pNewLinkHub = LinkHub_Lock(hNewLinkHub, pid);
    if (!pNewLinkHub)
    {
        ERR("!pNewLinkHub");
        return nRegID;
    }

    CopyMemory(pNewLinkHub, pOldLinkHub, cbOldLinkHub);
    pNewLinkHub->nCapacity = nNewCapacity;

    if (block)
        nRegID = LinkHub_Add(pNewLinkHub, pid, block);

    SHUnlockShared(pNewLinkHub);

    s_hLinkHub = hNewLinkHub;
    s_dwLinkOwnerPID = pid;

    return nRegID;
}

static HANDLE
LinkHub_Create(DWORD pid)
{
    LINKHUB LinkHub = { 1 };
    return SHAllocShared(&LinkHub, sizeof(LinkHub), pid);
}

static UINT
DoAddBlock(DWORD pid, LPBLOCK block)
{
    UINT nRegID = 0;
    HANDLE hOldLinkHub;
    DWORD dwOldOwnerPID;
    HANDLE hLinkHub;
    LPLINKHUB pLinkHub;

    if (!s_hLinkHub)
    {
        hLinkHub = LinkHub_Create(pid);
        if (!hLinkHub)
        {
            ERR("!hLinkHub\n");
            return nRegID;
        }

        s_hLinkHub = hLinkHub;
        s_dwLinkOwnerPID = pid;
    }

    pLinkHub = LinkHub_Lock(s_hLinkHub, s_dwLinkOwnerPID);
    if (!pLinkHub)
    {
        ERR("!hLinkHub\n");
        return nRegID;
    }

    nRegID = LinkHub_Add(pLinkHub, pid, block);
    if (nRegID)
    {
        LinkHub_Unlock(pLinkHub);
        return nRegID;
    }

    hOldLinkHub = s_hLinkHub;
    dwOldOwnerPID = s_dwLinkOwnerPID;

    nRegID = LinkHub_GrowAdd(pLinkHub, pLinkHub->nCapacity + LINKHUB_GROW, pid, block);
    LinkHub_Unlock(pLinkHub);

    SHFreeShared(hOldLinkHub, dwOldOwnerPID);
    return nRegID;
}

static void
LinkHub_MoveOwnership(LPLINKHUB pLinkHub, DWORD dwFromPID, DWORD dwToPID)
{
    INT iLink;
    LPLINK pLink;
    LPBLOCK block;
    DWORD cbBlock;
    HANDLE hBlock;
    for (iLink = 0; iLink < pLinkHub->nCapacity; ++iLink)
    {
        pLink = &pLinkHub->Links[iLink];
        if (pLink->nRegID == 0)
            continue;

        if (pLink->dwOwnerPID != dwFromPID)
            continue;

        block = Block_Lock(pLink->hBlock, pLink->dwOwnerPID);
        if (!block)
            continue;

        cbBlock = Block_GetSize(block->cItems);
        hBlock = SHAllocShared(block, cbBlock, dwToPID);
        Block_Unlock(block);

        if (hBlock)
        {
            Block_Destroy(pLink->hBlock, pLink->dwOwnerPID);

            pLink->hBlock = hBlock;
            pLink->dwOwnerPID = dwToPID;
        }
    }
}

static void
LinkHub_RemoveByProcess(LPLINKHUB pLinkHub, DWORD pid)
{
    INT iLink;
    LPLINK pLink;
    for (iLink = 0; iLink < pLinkHub->nCapacity; ++iLink)
    {
        pLink = &pLinkHub->Links[iLink];
        if (pLink->nRegID == 0)
            continue;

        if (pLink->dwUserPID == pid)
        {
            Block_Destroy(pLink->hBlock, pLink->dwOwnerPID);
            ZeroMemory(pLink, sizeof(*pLink));
            pLinkHub->nCount--;
        }
    }
}

static DWORD
LinkHub_GetAnotherPID(LPLINKHUB pLinkHub, DWORD pid)
{
    INT iLink;
    LPLINK pLink;
    for (iLink = 0; iLink < pLinkHub->nCapacity; ++iLink)
    {
        pLink = &pLinkHub->Links[iLink];
        if (pLink->nRegID == 0)
            continue;

        if (pLink->dwOwnerPID != pid)
        {
            return pLink->dwOwnerPID;
        }
    }
    return 0;
}

static void
DoRemoveBlockByProcess(DWORD pid)
{
    LPLINKHUB pLinkHub = LinkHub_Lock(s_hLinkHub, s_dwLinkOwnerPID);
    if (pLinkHub)
    {
        DWORD dwToPID = LinkHub_GetAnotherPID(pLinkHub, pid);
        if (!dwToPID)
        {
            LinkHub_Unlock(pLinkHub);
            return;
        }

        LinkHub_RemoveByProcess(pLinkHub, pid);
        LinkHub_MoveOwnership(pLinkHub, pid, dwToPID);

        if (pid == s_dwLinkOwnerPID)
        {
            HANDLE hOldLinkHub = s_hLinkHub;
            DWORD dwOldOwnerPID = s_dwLinkOwnerPID;

            LinkHub_GrowAdd(pLinkHub, pLinkHub->nCapacity, dwToPID, NULL);
            LinkHub_Unlock(pLinkHub);

            SHFreeShared(hOldLinkHub, dwOldOwnerPID);
        }
        else
        {
            LinkHub_Unlock(pLinkHub);
        }
    }
}
#else
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
#endif

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
    HANDLE hMutex = CreateMutexW(NULL, TRUE, SHARE_MUTEX_NAME);
    DoRemoveBlockByProcess(GetCurrentProcessId());
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
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
    HANDLE hMutex;
    DWORD pid;
    UINT nRegID = 0;
    DWORD cbBlock;
    LPBLOCK lpBlock;
    INT iEntries;
    LPITEM pItem;

    TRACE("(%p,0x%08x,0x%08x,0x%08x,%d,%p) item=%p\n",

    if (cItems <= 0 || !lpItems || !IsWindow(hwnd))
    {
        ERR("Sanity check\n");
        return nRegID;
    }

    GetWindowThreadProcessId(hwnd, &pid);
    if (pid != GetCurrentProcessId())
    {
        ERR("Sanity check 2\n");
        return nRegID;
    }

    hMutex = CreateMutexW(NULL, TRUE, SHARE_MUTEX_NAME);
    if (cItems == 1)
    {
        ITEM item = { lpItems->fRecursive };
        BLOCK block = { hwnd, fSources, wEventMask, uMsg, 1 };

        SHGetPathFromIDListW(lpItems->pidl, item.szPath);
        block.Items[0] = item;

        nRegID = DoAddBlock(GetCurrentProcessId(), &block);
    }
    else
    {
        cbBlock = Block_GetSize(cItems);
        lpBlock = (LPBLOCK)CoTaskMemAlloc(cbBlock);
        if (lpBlock)
        {
            for (iEntries = 0; iEntries < cItems; ++iEntries)
            {
                pItem = &lpBlock->Items[iEntries];
                pItem->fRecursive = lpItems[iEntries].fRecursive;
                SHGetPathFromIDListW(lpItems[iEntries].pidl, pItem->szPath);
            }
            nRegID = DoAddBlock(GetCurrentProcessId(), lpBlock);
            CoTaskMemFree(lpBlock);
        }
    }
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);

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
    HANDLE hMutex = CreateMutexW(NULL, TRUE, SHARE_MUTEX_NAME);
    BOOL ret = DoRemoveBlockByRegID(hNotify);
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
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

#ifdef __REACTOS__
typedef struct DELIVERY
{
    LONG event;
    WCHAR szPath1[MAX_PATH];
    WCHAR szPath2[MAX_PATH];
} DELIVERY, *LPDELIVERY;

static BOOL
should_notify(LPCWSTR changed, LPCWSTR watched, BOOL sub)
{
    WCHAR szChanged[MAX_PATH], szWatched[MAX_PATH];
    INT cchChanged;

    if (!watched)
        return FALSE;

    lstrcpynW(szChanged, changed, MAX_PATH);
    if (PathIsDirectoryW(szChanged))
        PathAddBackslashW(szChanged);
    if (PathIsDirectoryW(szWatched))
        PathAddBackslashW(szWatched);

    if (lstrcmpiW(szChanged, szWatched) == 0)
        return TRUE;

    if (sub)
    {
        cchChanged = lstrlenW(szChanged);
        szWatched[cchChanged] = 0;
        if (lstrcmpiW(szChanged, szWatched) == 0)
            return TRUE;
    }

    return FALSE;
}

static void
DoDelivery(LPDELIVERY delivery, LONG wEventId, ULONG uFlags)
{
    LPLINKHUB pLinkHub;
    INT iLink, iItem;
    struct new_delivery_notification *shared_data = NULL;
    LPITEMIDLIST Pidls[2];

    pLinkHub = LinkHub_Lock(s_hLinkHub, s_dwLinkOwnerPID);
    if (!pLinkHub)
        return;

    if (!pLinkHub->nCount)
    {
        LinkHub_Unlock(pLinkHub);
        return;
    }

    Pidls[0] = (delivery->szPath1[0] ? ILCreateFromPathW(delivery->szPath1) : NULL);
    Pidls[1] = (delivery->szPath2[0] ? ILCreateFromPathW(delivery->szPath2) : NULL);

    /* Create shared_data */
    {
        struct new_delivery_notification *notification;
        UINT size1 = ILGetSize(Pidls[0]), size2 = ILGetSize(Pidls[1]);
        UINT offset = (size1 + sizeof(int) - 1) / sizeof(int) * sizeof(int);

        notification = SHAlloc(sizeof(struct new_delivery_notification) + offset + size2);
        if (!notification) {
            ERR("out of memory\n");
        } else {
            notification->event = wEventId;
            notification->pidl1_size = size1;
            notification->pidl2_size = size2;
            if (size1)
                memcpy(notification->data, Pidls[0], size1);
            if (size2)
                memcpy(notification->data+offset, Pidls[1], size2);

            shared_data = SHAllocShared(notification,
                sizeof(struct new_delivery_notification) + size1 + size2,
                GetCurrentProcessId());
            SHFree(notification);
        }
    }

    for (iLink = 0; iLink < pLinkHub->nCapacity; ++iLink)
    {
        LPBLOCK block;
        LPLINK pLink = &pLinkHub->Links[iLink];
        if (pLink->nRegID == 0)
            continue;

        block = Block_Lock(pLink->hBlock, pLink->dwOwnerPID);
        if (!block)
            continue;

        TRACE("notifying %p, event %s(%x)\n", block->hwnd, DumpEvent(block->fEvents), wEventId);

        for (iItem = 0; iItem < block->cItems; ++iItem)
        {
            LPITEM pItem = &block->Items[iItem];
            LPCWSTR pszPath = pItem->szPath;
            BOOL subtree = pItem->fRecursive;
            BOOL notify = FALSE;

            if (wEventId & block->fEvents)
            {
                if (!pszPath[0])
                    notify = TRUE;
                else if (wEventId & SHCNE_NOITEMEVENTS)
                    notify = TRUE;
                else if (wEventId & (SHCNE_ONEITEMEVENTS | SHCNE_TWOITEMEVENTS))
                    notify = should_notify(delivery->szPath1, pszPath, subtree);
                else if( wEventId & SHCNE_TWOITEMEVENTS )
                    notify = should_notify(delivery->szPath2, pszPath, subtree);
            }

            if (!notify)
                continue;

            if ((block->fSources & SHCNRF_NewDelivery))
            {
                if (shared_data)
                    SendMessageA(block->hwnd, block->uMsg, (WPARAM)shared_data, GetCurrentProcessId());
                else
                    ERR("out of memory\n");
            }
            else
            {
                SendMessageA(block->hwnd, block->uMsg, (WPARAM)Pidls, wEventId);
            }
        }

        Block_Unlock(block);
    }

    SHFreeShared(shared_data, GetCurrentProcessId());
    SHFree(Pidls[0]);
    SHFree(Pidls[1]);

    LinkHub_Unlock(pLinkHub);
}
#else
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
#endif

/*************************************************************************
 * SHChangeNotify				[SHELL32.@]
 */
void WINAPI SHChangeNotify(LONG wEventId, UINT uFlags, LPCVOID dwItem1, LPCVOID dwItem2)
{
#ifdef __REACTOS__
    DELIVERY delivery;
    HANDLE hMutex;

    TRACE("(0x%08x,0x%08x,%p,%p)\n", wEventId, uFlags, dwItem1, dwItem2);

    if (uFlags & ~(SHCNF_TYPE | SHCNF_FLUSH))
        FIXME("ignoring unsupported flags: %x\n", uFlags);

    if ((wEventId & SHCNE_NOITEMEVENTS) && (dwItem1 || dwItem2))
    {
        TRACE("dwItem1 and dwItem2 are not zero, but should be\n");
        dwItem1 = 0;
        dwItem2 = 0;
    }
    else if ((wEventId & SHCNE_ONEITEMEVENTS) && dwItem2)
    {
        TRACE("dwItem2 is not zero, but should be\n");
        dwItem2 = 0;
    }

    if (((wEventId & SHCNE_NOITEMEVENTS) && 
         (wEventId & ~(SHCNE_NOITEMEVENTS | SHCNE_INTERRUPT))) ||
        ((wEventId & SHCNE_ONEITEMEVENTS) && 
         (wEventId & ~(SHCNE_ONEITEMEVENTS | SHCNE_INTERRUPT))) ||
        ((wEventId & SHCNE_TWOITEMEVENTS) && 
         (wEventId & ~(SHCNE_TWOITEMEVENTS | SHCNE_INTERRUPT))))
    {
        WARN("mutually incompatible events listed\n");
        return;
    }

    delivery.szPath1[0] = delivery.szPath2[0] = 0;
    switch (uFlags & SHCNF_TYPE)
    {
        case SHCNF_PATHA:
            if (dwItem1)
                MultiByteToWideChar(CP_ACP, 0, dwItem1, -1, delivery.szPath1, MAX_PATH);
            if (dwItem2)
                MultiByteToWideChar(CP_ACP, 0, dwItem2, -1, delivery.szPath2, MAX_PATH);
            break;
        case SHCNF_PATHW:
            if (dwItem1)
                lstrcpynW(delivery.szPath1, dwItem1, MAX_PATH);
            if (dwItem2)
                lstrcpynW(delivery.szPath2, dwItem2, MAX_PATH);
            break;
        case SHCNF_IDLIST:
            if (dwItem1)
                SHGetPathFromIDListW(dwItem1, delivery.szPath1);
            if (dwItem2)
                SHGetPathFromIDListW(dwItem2, delivery.szPath2);
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

    hMutex = CreateMutexW(NULL, TRUE, SHARE_MUTEX_NAME);
    DoDelivery(&delivery, wEventId, uFlags);
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
#else   /* ndef __REACTOS__ */
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
        if (dwItem1) Pidls[0] = SHSimpleIDListFromPathA(dwItem1);
        if (dwItem2) Pidls[1] = SHSimpleIDListFromPathA(dwItem2);
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
#endif  /* ndef __REACTOS__ */
}

/*************************************************************************
 * NTSHChangeNotifyRegister			[SHELL32.640]
 * NOTES
 *   Idlist is an array of structures and Count specifies how many items in the array
 *   (usually just one I think).
 */
ULONG WINAPI NTSHChangeNotifyRegister(
    HWND hwnd,
    int fSources,
    LONG wEventMask,
    UINT uMsg,
    int cItems,
    SHChangeNotifyEntry *lpItems)
{
    return SHChangeNotifyRegister(hwnd, fSources | SHCNRF_NewDelivery,
                                  wEventMask, uMsg, cItems, lpItems);
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
