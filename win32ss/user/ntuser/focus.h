#pragma once

extern PUSER_MESSAGE_QUEUE gpqForeground;
extern PUSER_MESSAGE_QUEUE gpqForegroundPrev;
extern PTHREADINFO ptiLastInput;

/*
 * These functions take the window handles from current message queue.
 */
HWND FASTCALL IntGetCaptureWindow(VOID);
HWND FASTCALL co_UserSetCapture(HWND hWnd);
BOOL FASTCALL IntReleaseCapture(VOID);

/*
 * These functions take the window handles from current thread queue.
 */
HWND FASTCALL IntGetThreadFocusWindow(VOID);
HWND APIENTRY IntGetCapture(VOID);
HWND FASTCALL UserGetActiveWindow(VOID);
BOOL FASTCALL co_IntMouseActivateWindow(PWND Window);
BOOL FASTCALL co_IntSetForegroundWindow(PWND Window);
BOOL FASTCALL co_IntSetForegroundWindowMouse(PWND Window);
BOOL FASTCALL co_IntSetActiveWindow(PWND,BOOL,BOOL,BOOL);
BOOL FASTCALL IntUserSetActiveWindow(PWND,BOOL,BOOL,BOOL);
BOOL FASTCALL UserSetActiveWindow(PWND Wnd);
BOOL FASTCALL IntLockSetForegroundWindow(UINT uLockCode);
BOOL FASTCALL IntAllowSetForegroundWindow(DWORD dwProcessId);
VOID FASTCALL IntActivateWindow(PWND,PTHREADINFO,HANDLE,DWORD);
BOOL FASTCALL IntDeactivateWindow(PTHREADINFO,HANDLE);
BOOL FASTCALL co_IntSetForegroundMessageQueue(PWND,PTHREADINFO,BOOL,DWORD );
VOID FASTCALL UpdateShellHook(PWND);
BOOL FASTCALL IntCheckFullscreen(PWND Window);
