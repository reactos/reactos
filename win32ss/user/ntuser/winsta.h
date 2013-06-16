#pragma once

#define WINSTA_OBJ_DIR L"\\Windows\\WindowStations"
#define SESSION_DIR L"\\Sessions"

/* Window Station Status Flags */
#define WSS_LOCKED	(1)
#define WSS_NOINTERACTIVE	(2)

typedef struct _WINSTATION_OBJECT
{
    DWORD dwSessionId;

    KSPIN_LOCK Lock;
    UNICODE_STRING Name;
    LIST_ENTRY DesktopListHead;
    PRTL_ATOM_TABLE AtomTable;
    HANDLE SystemMenuTemplate;
    HANDLE ShellWindow;
    HANDLE ShellListView;

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
extern UNICODE_STRING gustrWindowStationsDir;

#define WINSTA_READ       STANDARD_RIGHTS_READ     | \
                          WINSTA_ENUMDESKTOPS      | \
                          WINSTA_ENUMERATE         | \
                          WINSTA_READATTRIBUTES    | \
                          WINSTA_READSCREEN

#define WINSTA_WRITE      STANDARD_RIGHTS_WRITE    | \
                          WINSTA_ACCESSCLIPBOARD   | \
                          WINSTA_CREATEDESKTOP     | \
                          WINSTA_WRITEATTRIBUTES

#define WINSTA_EXECUTE    STANDARD_RIGHTS_EXECUTE  | \
                          WINSTA_ACCESSGLOBALATOMS | \
                          WINSTA_EXITWINDOWS

#define WINSTA_ACCESS_ALL STANDARD_RIGHTS_REQUIRED | \
                          WINSTA_ACCESSCLIPBOARD   | \
                          WINSTA_ACCESSGLOBALATOMS | \
                          WINSTA_CREATEDESKTOP     | \
                          WINSTA_ENUMDESKTOPS      | \
                          WINSTA_ENUMERATE         | \
                          WINSTA_EXITWINDOWS       | \
                          WINSTA_READATTRIBUTES    | \
                          WINSTA_READSCREEN        | \
                          WINSTA_WRITEATTRIBUTES

INIT_FUNCTION
NTSTATUS
NTAPI
InitWindowStationImpl(VOID);
NTSTATUS NTAPI UserCreateWinstaDirectory();

VOID APIENTRY IntWinStaObjectDelete(PWIN32_DELETEMETHOD_PARAMETERS Parameters);
NTSTATUS APIENTRY IntWinStaObjectParse(PWIN32_PARSEMETHOD_PARAMETERS Parameters);
NTSTATUS NTAPI IntWinstaOkToClose(PWIN32_OKAYTOCLOSEMETHOD_PARAMETERS Parameters);

NTSTATUS FASTCALL
IntValidateWindowStationHandle(
   HWINSTA WindowStation,
   KPROCESSOR_MODE AccessMode,
   ACCESS_MASK DesiredAccess,
   PWINSTATION_OBJECT *Object);
BOOL FASTCALL UserSetProcessWindowStation(HWINSTA hWindowStation);

BOOL FASTCALL co_IntInitializeDesktopGraphics(VOID);
VOID FASTCALL IntEndDesktopGraphics(VOID);
/* EOF */
