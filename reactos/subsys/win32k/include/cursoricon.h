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

HCURSOR FASTCALL IntSetCursor(PWINSTATION_OBJECT WinStaObject, PCURICON_OBJECT NewCursor, BOOL ForceChange);
BOOL FASTCALL IntSetupCurIconHandles(PWINSTATION_OBJECT WinStaObject);
PCURICON_OBJECT FASTCALL IntGetCurIconObject(PWINSTATION_OBJECT WinStaObject, HANDLE Handle);
VOID FASTCALL IntReleaseCurIconObject(PCURICON_OBJECT Object);
PCURICON_OBJECT FASTCALL IntCreateCurIconHandle(PWINSTATION_OBJECT WinStaObject);
VOID FASTCALL IntCleanupCurIcons(struct _EPROCESS *Process, PW32PROCESS Win32Process);

#define IntLockProcessCursorIcons(W32Process) \
  ExAcquireFastMutex(&W32Process->CursorIconListLock)

#define IntUnLockProcessCursorIcons(W32Process) \
  ExReleaseFastMutex(&W32Process->CursorIconListLock)

#endif /* _WIN32K_CURSORICON_H */

/* EOF */

