#pragma once

/*
 * These functions take the window handles from current message queue.
 */
HWND FASTCALL
IntGetCaptureWindow(VOID);
HWND FASTCALL
IntGetFocusWindow(VOID);

/*
 * These functions take the window handles from current thread queue.
 */
HWND FASTCALL
IntGetThreadFocusWindow(VOID);
HWND APIENTRY IntGetCapture(VOID);
HWND FASTCALL UserGetActiveWindow(VOID);

BOOL FASTCALL
co_IntMouseActivateWindow(PWND Window);
BOOL FASTCALL
co_IntSetForegroundWindow(PWND Window);
HWND FASTCALL
co_IntSetActiveWindow(PWND Window);
