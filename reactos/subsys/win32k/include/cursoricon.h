#ifndef _WIN32K_CURSORICON_H
#define _WIN32K_CURSORICON_H

#define MAXCURICONHANDLES 4096

typedef struct _CURICON_OBJECT
{
  HANDLE Self;
  LIST_ENTRY ListEntry;
  PW32PROCESS Process;
  HMODULE hModule;
  HRSRC hRsrc;
  HRSRC hGroupRsrc;
  SIZE Size;
  BYTE Shadow;
  ICONINFO IconInfo;
} CURICON_OBJECT, *PCURICON_OBJECT;

typedef struct _CURSORCLIP_INFO
{
  BOOL IsClipped;
  UINT Left;
  UINT Top;
  UINT Right;
  UINT Bottom;
} CURSORCLIP_INFO, *PCURSORCLIP_INFO;

typedef struct _SYSTEM_CURSORINFO
{
  BOOL Enabled;
  BOOL SwapButtons;
  UINT ButtonsDown;
  LONG x, y;
  BOOL SafetySwitch;
  UINT SafetyRemoveCount;
  LONG PointerRectLeft;
  LONG PointerRectTop;
  LONG PointerRectRight;
  LONG PointerRectBottom;
  FAST_MUTEX CursorMutex;
  CURSORCLIP_INFO CursorClipInfo;
  PCURICON_OBJECT CurrentCursorObject;
  BYTE ShowingCursor;
  UINT DblClickSpeed;
  UINT DblClickWidth;
  UINT DblClickHeight;
  DWORD LastBtnDown;
  LONG LastBtnDownX;
  LONG LastBtnDownY;
  HANDLE LastClkWnd;
} SYSTEM_CURSORINFO, *PSYSTEM_CURSORINFO;

HCURSOR FASTCALL IntSetCursor(PWINSTATION_OBJECT WinStaObject, PCURICON_OBJECT NewCursor, BOOL ForceChange);
BOOL FASTCALL IntSetupCurIconHandles(PWINSTATION_OBJECT WinStaObject);
PCURICON_OBJECT FASTCALL IntGetCurIconObject(PWINSTATION_OBJECT WinStaObject, HANDLE Handle);
PCURICON_OBJECT FASTCALL IntCreateCurIconHandle(PWINSTATION_OBJECT WinStaObject);
VOID FASTCALL IntCleanupCurIcons(struct _EPROCESS *Process, PW32PROCESS Win32Process);

#define IntGetSysCursorInfo(WinStaObj) \
  (PSYSTEM_CURSORINFO)((WinStaObj)->SystemCursor)

#define IntReleaseCurIconObject(CurIconObj) \
  ObmDereferenceObject(CurIconObj)

#define IntLockProcessCursorIcons(W32Process) \
  ExAcquireFastMutex(&W32Process->CursorIconListLock)

#define IntUnLockProcessCursorIcons(W32Process) \
  ExReleaseFastMutex(&W32Process->CursorIconListLock)

#endif /* _WIN32K_CURSORICON_H */

/* EOF */

