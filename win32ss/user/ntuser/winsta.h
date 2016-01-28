#pragma once

#define WINSTA_OBJ_DIR L"\\Windows\\WindowStations"
#define SESSION_DIR L"\\Sessions"

/* Window Station Status Flags */
#define WSS_LOCKED        (1)
#define WSS_NOINTERACTIVE (2)
#define WSS_NOIO          (4)
#define WSS_SHUTDOWN      (8)
#define WSS_DYING         (16)
#define WSS_REALSHUTDOWN  (32)

typedef struct _WINSTATION_OBJECT
{
    DWORD dwSessionId;

    UNICODE_STRING Name;
    LIST_ENTRY DesktopListHead;
    PRTL_ATOM_TABLE AtomTable;
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

    PWND           spwndClipboardListener;
    LUID           luidEndSession;
    LUID           luidUser;
    PVOID          psidUser;

} WINSTATION_OBJECT, *PWINSTATION_OBJECT;

extern WINSTATION_OBJECT *InputWindowStation;
extern HANDLE gpidLogon;
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

NTSTATUS
NTAPI
UserCreateWinstaDirectory();

NTSTATUS
APIENTRY
IntWinStaObjectDelete(
    _In_ PVOID Parameters);

NTSTATUS
APIENTRY
IntWinStaObjectParse(
    _In_ PVOID Parameters);

NTSTATUS
NTAPI
IntWinstaOkToClose(
    _In_ PVOID Parameters);

NTSTATUS
FASTCALL
IntValidateWindowStationHandle(
   HWINSTA WindowStation,
   KPROCESSOR_MODE AccessMode,
   ACCESS_MASK DesiredAccess,
   PWINSTATION_OBJECT *Object,
   POBJECT_HANDLE_INFORMATION pObjectHandleInfo);

BOOL FASTCALL UserSetProcessWindowStation(HWINSTA hWindowStation);

BOOL FASTCALL co_IntInitializeDesktopGraphics(VOID);
VOID FASTCALL IntEndDesktopGraphics(VOID);
BOOL FASTCALL CheckWinstaAttributeAccess(ACCESS_MASK);
/* EOF */
