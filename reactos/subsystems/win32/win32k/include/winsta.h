#ifndef _WIN32K_WINSTA_H
#define _WIN32K_WINSTA_H

#include "window.h"
#include "clipboard.h"

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
    HANDLE SystemMenuTemplate;
    PVOID SystemCursor;
    UINT CaretBlinkRate;
    HANDLE ShellWindow;
    HANDLE ShellListView;

    /* Effects */
    BOOL FontSmoothing; /* enable */
    UINT FontSmoothingType; /* 1:Standard,2:ClearType */
    /* FIXME: Big Icons (SPI_GETICONMETRICS?) */
    BOOL DropShadow;
    BOOL DragFullWindows;
    BOOL FlatMenu;

    /* ScreenSaver */
    BOOL ScreenSaverRunning;
    UINT  ScreenSaverTimeOut;
   /* Should this be on each desktop ? */
    BOOL ScreenSaverActive;

    /* Wallpaper */
    HANDLE hbmWallpaper;
    ULONG cxWallpaper, cyWallpaper;
    WALLPAPER_MODE WallpaperMode;

    ULONG Flags;
    struct _DESKTOP* ActiveDesktop;

    PCLIPBOARDSYSTEM Clipboard;
    DWORD           ClipboardSequenceNumber;

} WINSTATION_OBJECT, *PWINSTATION_OBJECT;

extern WINSTATION_OBJECT *InputWindowStation;
extern PW32PROCESS LogonProcess;

NTSTATUS FASTCALL
InitWindowStationImpl(VOID);

NTSTATUS FASTCALL
CleanupWindowStationImpl(VOID);

NTSTATUS
APIENTRY
IntWinStaObjectOpen(PWIN32_OPENMETHOD_PARAMETERS Parameters);

VOID APIENTRY
IntWinStaObjectDelete(PWIN32_DELETEMETHOD_PARAMETERS Parameters);

NTSTATUS
APIENTRY
IntWinStaObjectParse(PWIN32_PARSEMETHOD_PARAMETERS Parameters);

NTSTATUS FASTCALL
IntValidateWindowStationHandle(
   HWINSTA WindowStation,
   KPROCESSOR_MODE AccessMode,
   ACCESS_MASK DesiredAccess,
   PWINSTATION_OBJECT *Object);

BOOL FASTCALL
IntGetWindowStationObject(PWINSTATION_OBJECT Object);

BOOL FASTCALL
co_IntInitializeDesktopGraphics(VOID);

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
