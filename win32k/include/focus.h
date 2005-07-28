#ifndef _WIN32K_FOCUS_H
#define _WIN32K_FOCUS_H

/*
 * These functions take the window handles from current message queue.
 */
PWINDOW_OBJECT FASTCALL
UserGetCaptureWindow();
PWINDOW_OBJECT FASTCALL
UserGetFocusWindow();



/*
 * These functions take the window handles from current thread queue.
 */
PWINDOW_OBJECT FASTCALL
UserGetThreadFocusWindow();

BOOL FASTCALL
IntMouseActivateWindow(PWINDOW_OBJECT Window);
BOOL FASTCALL
IntSetForegroundWindow(PWINDOW_OBJECT Window);
PWINDOW_OBJECT FASTCALL
IntSetActiveWindow(PWINDOW_OBJECT Window);

#endif /* _WIN32K_FOCUS_H */
