#ifndef _WIN32K_WINSTA_H
#define _WIN32K_WINSTA_H

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
    LIST_ENTRY DesktopListHead;
    PRTL_ATOM_TABLE AtomTable;
    PVOID HandleTable;
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
    struct _DESKTOP_OBJECT* ActiveDesktop;
    /* FIXME: Clipboard */
    LIST_ENTRY HotKeyListHead;
    FAST_MUTEX HotKeyListLock;
} WINSTATION_OBJECT, *PWINSTATION_OBJECT;

extern WINSTATION_OBJECT *InputWindowStation;
extern PW32PROCESS LogonProcess;

NTSTATUS FASTCALL
InitWindowStationImpl(VOID);

NTSTATUS FASTCALL
CleanupWindowStationImpl(VOID);

NTSTATUS
STDCALL
IntWinStaObjectOpen(OB_OPEN_REASON Reason,
                    PVOID ObjectBody,
                    PEPROCESS Process,
                    ULONG HandleCount,
                    ACCESS_MASK GrantedAccess);

VOID STDCALL
IntWinStaObjectDelete(PVOID DeletedObject);

PVOID STDCALL
IntWinStaObjectFind(PVOID Object,
                    PWSTR Name,
                    ULONG Attributes);

NTSTATUS
STDCALL
IntWinStaObjectParse(PVOID Object,
                     PVOID *NextObject,
                     PUNICODE_STRING FullPath,
                     PWSTR *Path,
                     ULONG Attributes);

NTSTATUS FASTCALL
IntValidateWindowStationHandle(
   HWINSTA WindowStation,
   KPROCESSOR_MODE AccessMode,
   ACCESS_MASK DesiredAccess,
   PWINSTATION_OBJECT *Object);

BOOL FASTCALL
IntGetWindowStationObject(PWINSTATION_OBJECT Object);

BOOL FASTCALL
IntInitializeDesktopGraphics(VOID);

VOID FASTCALL
IntEndDesktopGraphics(VOID);

BOOL FASTCALL
IntGetFullWindowStationName(
   OUT PUNICODE_STRING FullName,
   IN PUNICODE_STRING WinStaName,
   IN OPTIONAL PUNICODE_STRING DesktopName);

PWINSTATION_OBJECT FASTCALL IntGetWinStaObj(VOID);

#endif /* _WIN32K_WINSTA_H */

/* EOF */
