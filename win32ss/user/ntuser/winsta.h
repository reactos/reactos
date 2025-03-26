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

// See also: https://reactos.org/wiki/Techwiki:Win32k/WINDOWSTATION
typedef struct _WINSTATION_OBJECT
{
    DWORD dwSessionId;

    LIST_ENTRY DesktopListHead;
    PRTL_ATOM_TABLE AtomTable;

    ULONG          Flags;
    struct tagKL*  spklList;
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

    /* ReactOS-specific */
    struct _DESKTOP* ActiveDesktop;
    HANDLE         ShellWindow;
    HANDLE         ShellListView;
} WINSTATION_OBJECT, *PWINSTATION_OBJECT;

#ifndef _WIN64
C_ASSERT(offsetof(WINSTATION_OBJECT, Flags) == 0x10);
C_ASSERT(offsetof(WINSTATION_OBJECT, spklList) == 0x14);
C_ASSERT(offsetof(WINSTATION_OBJECT, ptiClipLock) == 0x18);
C_ASSERT(offsetof(WINSTATION_OBJECT, ptiDrawingClipboard) == 0x1c);
C_ASSERT(offsetof(WINSTATION_OBJECT, spwndClipOpen) == 0x20);
C_ASSERT(offsetof(WINSTATION_OBJECT, spwndClipViewer) == 0x24);
C_ASSERT(offsetof(WINSTATION_OBJECT, spwndClipOwner) == 0x28);
#endif

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

PWINSTATION_OBJECT FASTCALL IntGetProcessWindowStation(HWINSTA *phWinSta OPTIONAL);
BOOL FASTCALL UserSetProcessWindowStation(HWINSTA hWindowStation);

BOOL FASTCALL co_IntInitializeDesktopGraphics(VOID);
VOID FASTCALL IntEndDesktopGraphics(VOID);
BOOL FASTCALL CheckWinstaAttributeAccess(ACCESS_MASK);

/* EOF */
