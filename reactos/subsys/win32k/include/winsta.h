#ifndef __WIN32K_WINSTA_H
#define __WIN32K_WINSTA_H

#define NTOS_KERNEL_MODE
#include <ntos.h>
#include <windows.h>

#define PROCESS_WINDOW_STATION() \
  ((HWINSTA)(IoGetCurrentProcess()->Win32WindowStation))

#define SET_PROCESS_WINDOW_STATION(WinSta) \
  ((IoGetCurrentProcess()->Win32WindowStation) = (PVOID)(WinSta))


NTSTATUS
InitWindowStationImpl(VOID);

NTSTATUS
CleanupWindowStationImpl(VOID);

NTSTATUS
ValidateWindowStationHandle(HWINSTA WindowStation,
			    KPROCESSOR_MODE AccessMode,
			    ACCESS_MASK DesiredAccess,
			    PWINSTATION_OBJECT *Object);

NTSTATUS
ValidateDesktopHandle(HDESK Desktop,
		      KPROCESSOR_MODE AccessMode,
		      ACCESS_MASK DesiredAccess,
		      PDESKTOP_OBJECT *Object);
LRESULT CALLBACK
W32kDesktopWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
PDESKTOP_OBJECT
W32kGetActiveDesktop(VOID);

#endif /* __WIN32K_WINSTA_H */

/* EOF */
