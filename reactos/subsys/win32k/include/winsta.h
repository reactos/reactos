#ifndef __WIN32K_WINSTA_H
#define __WIN32K_WINSTA_H

#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/ex.h>
#include <internal/ps.h>

#define PROCESS_WINDOW_STATION() \
  ((HWINSTA)(IoGetCurrentProcess()->Win32WindowStation))

#define SET_PROCESS_WINDOW_STATION(WinSta) \
  ((IoGetCurrentProcess()->Win32WindowStation) = (PVOID)(WinSta))


NTSTATUS FASTCALL
InitWindowStationImpl(VOID);

NTSTATUS FASTCALL
CleanupWindowStationImpl(VOID);

NTSTATUS STDCALL
ValidateWindowStationHandle(HWINSTA WindowStation,
			    KPROCESSOR_MODE AccessMode,
			    ACCESS_MASK DesiredAccess,
			    PWINSTATION_OBJECT *Object);

NTSTATUS STDCALL
ValidateDesktopHandle(HDESK Desktop,
		      KPROCESSOR_MODE AccessMode,
		      ACCESS_MASK DesiredAccess,
		      PDESKTOP_OBJECT *Object);
LRESULT CALLBACK
W32kDesktopWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
PDESKTOP_OBJECT FASTCALL
W32kGetActiveDesktop(VOID);
VOID FASTCALL
W32kInitializeDesktopGraphics(VOID);
VOID FASTCALL
W32kEndDesktopGraphics(VOID);
HDC FASTCALL
W32kGetScreenDC(VOID);

#endif /* __WIN32K_WINSTA_H */

/* EOF */
