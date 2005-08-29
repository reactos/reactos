#ifndef _WIN32K_WINSTA_H
#define _WIN32K_WINSTA_H

#include "ntuser.h"  //FIXME: handle.h
#include "msgqueue.h"

#define WINSTA_ROOT_NAME	L"\\Windows\\WindowStations"
#define WINSTA_ROOT_NAME_LENGTH	23

/* Window Station Status Flags */
#define WSS_LOCKED	(1)
#define WSS_NOINTERACTIVE	(2)

typedef enum
{
    wmCenter = 0,
    wmTile,
    wmStretch
} WALLPAPER_MODE;

typedef struct _WINSTATION_OBJECT
{
    PVOID SharedHeap; /* points to kmode memory! */

    CSHORT Type;
    CSHORT Size;
    KSPIN_LOCK Lock;
    UNICODE_STRING Name;
    /* list of desktops of this winstation */
    LIST_ENTRY DesktopListHead;
    PRTL_ATOM_TABLE AtomTable;
 
    HANDLE SystemMenuTemplate;
    PVOID SystemCursor;
    UINT CaretBlinkRate;
    HANDLE ShellWindow;
    HANDLE ShellListView;

    /* Wallpaper */
    HANDLE hbmWallpaper;
    ULONG cxWallpaper, cyWallpaper;
    WALLPAPER_MODE WallpaperMode;

    ULONG Flags;
    /* FIXME: Clipboard */
    LIST_ENTRY HotKeyListHead;
    FAST_MUTEX HotKeyListLock;
} WINSTATION_OBJECT, *PWINSTATION_OBJECT;



#endif /* _WIN32K_WINSTA_H */

/* EOF */
