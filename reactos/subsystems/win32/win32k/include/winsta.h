#pragma once

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
    PVOID SharedHeap; /* Points to kmode memory! */

    CSHORT Type;
    CSHORT Size;
    KSPIN_LOCK Lock;
    UNICODE_STRING Name;
    LIST_ENTRY DesktopListHead;
    PRTL_ATOM_TABLE AtomTable;
    HANDLE SystemMenuTemplate;
    UINT CaretBlinkRate;
    HANDLE ShellWindow;
    HANDLE ShellListView;

    /* Effects */
    BOOL FontSmoothing; /* Enable */
    UINT FontSmoothingType; /* 1:Standard,2:ClearType */
    /* FIXME: Big Icons (SPI_GETICONMETRICS?) */
    BOOL DropShadow;
    BOOL DragFullWindows;
    BOOL FlatMenu;

    /* ScreenSaver */
    BOOL ScreenSaverRunning;
    UINT ScreenSaverTimeOut;
   /* Should this be on each desktop ? */
    BOOL ScreenSaverActive;

    /* Wallpaper */
    HANDLE hbmWallpaper;
    ULONG cxWallpaper, cyWallpaper;
    WALLPAPER_MODE WallpaperMode;

    ULONG Flags;
    struct _DESKTOP* ActiveDesktop;

    PTHREADINFO    ptiClipLock;
    PTHREADINFO    ptiDrawingClipboard;
    PWND           spwndClipOpen;
    PWND           spwndClipViewer;
    PWND           spwndClipOwner;
    PCLIP          pClipBase;     // Not a clip object.
    DWORD          cNumClipFormats;
    INT            iClipSerialNumber;
    INT            iClipSequenceNumber;
    INT            fClipboardChanged : 1;
    INT            fInDelayedRendering : 1;

} WINSTATION_OBJECT, *PWINSTATION_OBJECT;

extern WINSTATION_OBJECT *InputWindowStation;
extern PPROCESSINFO LogonProcess;
extern HWND hwndSAS;

INIT_FUNCTION
NTSTATUS
NTAPI
InitWindowStationImpl(VOID);

VOID APIENTRY
IntWinStaObjectDelete(PWIN32_DELETEMETHOD_PARAMETERS Parameters);

NTSTATUS
APIENTRY
IntWinStaObjectParse(PWIN32_PARSEMETHOD_PARAMETERS Parameters);

NTSTATUS NTAPI 
IntWinstaOkToClose(PWIN32_OKAYTOCLOSEMETHOD_PARAMETERS Parameters);

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

BOOL FASTCALL
UserSetProcessWindowStation(HWINSTA hWindowStation);

/* EOF */
