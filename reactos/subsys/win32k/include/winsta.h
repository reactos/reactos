#ifndef _WIN32K_WINSTA_H
#define _WIN32K_WINSTA_H

#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/ex.h>
#include <internal/ps.h>
#include "msgqueue.h"


#define PROCESS_WINDOW_STATION() \
  ((HWINSTA)(IoGetCurrentProcess()->Win32WindowStation))

#define SET_PROCESS_WINDOW_STATION(WinSta) \
  ((IoGetCurrentProcess()->Win32WindowStation) = (PVOID)(WinSta))
  
WINSTATION_OBJECT *InputWindowStation;
//FAST_MUTEX InputWindowStationLock;

extern HDC ScreenDeviceContext;


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
IntDesktopWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
PDESKTOP_OBJECT FASTCALL
IntGetActiveDesktop(VOID);
PDESKTOP_OBJECT FASTCALL
IntGetDesktopObject ( HDESK hDesk );
PUSER_MESSAGE_QUEUE FASTCALL
IntGetFocusMessageQueue(VOID);
VOID FASTCALL
IntInitializeDesktopGraphics(VOID);
VOID FASTCALL
IntEndDesktopGraphics(VOID);
HDC FASTCALL
IntGetScreenDC(VOID);
VOID STDCALL
IntSetFocusMessageQueue(PUSER_MESSAGE_QUEUE NewQueue);
struct _WINDOW_OBJECT* STDCALL
IntGetCaptureWindow(VOID);
VOID STDCALL
IntSetCaptureWindow(struct _WINDOW_OBJECT* Window);

BOOL FASTCALL
IntGetWindowStationObject(PWINSTATION_OBJECT Object);

#endif /* _WIN32K_WINSTA_H */

/* EOF */
