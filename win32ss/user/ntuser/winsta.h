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

CODE_SEG("INIT")
NTSTATUS
NTAPI
InitWindowStationImpl(VOID);

NTSTATUS
NTAPI
UserCreateWinstaDirectory(VOID);

NTSTATUS
NTAPI
IntWinStaObjectDelete(
    _In_ PVOID Parameters);

NTSTATUS
NTAPI
IntWinStaObjectParse(
    _In_ PVOID Parameters);

NTSTATUS
NTAPI
IntWinStaOkToClose(
    _In_ PVOID Parameters);

NTSTATUS
FASTCALL
IntValidateWindowStationHandle(
   HWINSTA WindowStation,
   KPROCESSOR_MODE AccessMode,
   ACCESS_MASK DesiredAccess,
   PWINSTATION_OBJECT *Object,
   POBJECT_HANDLE_INFORMATION pObjectHandleInfo);

NTSTATUS
FASTCALL
IntCreateWindowStation(
    OUT HWINSTA* phWinSta,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN KPROCESSOR_MODE AccessMode,
    IN KPROCESSOR_MODE OwnerMode,
    IN ACCESS_MASK dwDesiredAccess,
    DWORD Unknown2,
    DWORD Unknown3,
    DWORD Unknown4,
    DWORD Unknown5,
    DWORD Unknown6);

BOOL FASTCALL UserSetProcessWindowStation(HWINSTA hWindowStation);

BOOL FASTCALL co_IntInitializeDesktopGraphics(VOID);
VOID FASTCALL IntEndDesktopGraphics(VOID);
BOOL FASTCALL CheckWinstaAttributeAccess(ACCESS_MASK);

/* EOF */
