#ifndef __WIN32K_WINSTA_H
#define __WIN32K_WINSTA_H

#include <windows.h>
#include <ddk/ntddk.h>
#include <../ntoskrnl/include/internal/ex.h>

/*
* FIXME: NtCurrentPeb() does not work (returns 0)
#define PROCESS_WINDOW_STATION() \
  (HWINSTA)(NtCurrentPeb()->ProcessWindowStation);

#define SET_PROCESS_WINDOW_STATION(WinSta) \
  (NtCurrentPeb()->ProcessWindowStation = (PVOID)(WinSta));
*/

#define PROCESS_WINDOW_STATION() \
  ((HWINSTA)(((PPEB)PEB_BASE)->ProcessWindowStation))

#define SET_PROCESS_WINDOW_STATION(WinSta) \
  (((PPEB)PEB_BASE)->ProcessWindowStation = (PVOID)(WinSta))


NTSTATUS
InitWindowStationImpl(VOID);

NTSTATUS
CleanupWindowStationImpl(VOID);

NTSTATUS
ValidateWindowStationHandle(
  HWINSTA WindowStation,
  KPROCESSOR_MODE AccessMode,
  ACCESS_MASK DesiredAccess,
  PWINSTATION_OBJECT *Object);

NTSTATUS
ValidateDesktopHandle(
  HDESK Desktop,
  KPROCESSOR_MODE AccessMode,
  ACCESS_MASK DesiredAccess,
  PDESKTOP_OBJECT *Object);

#endif /* __WIN32K_WINSTA_H */

/* EOF */
