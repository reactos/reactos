#ifndef _WIN32K_CARET_H
#define _WIN32K_CARET_H

#define IDCARETTIMER (0xffff)

BOOL FASTCALL
co_IntDestroyCaret(PW32THREAD Win32Thread);

BOOL FASTCALL
IntSetCaretBlinkTime(UINT uMSeconds);

BOOL FASTCALL
co_IntSetCaretPos(int X, int Y);

BOOL FASTCALL
IntSwitchCaretShowing(PVOID Info);

VOID FASTCALL
IntDrawCaret(HWND hWnd);

BOOL FASTCALL co_UserShowCaret(PWINDOW_OBJECT WindowObject);

BOOL FASTCALL co_UserHideCaret(PWINDOW_OBJECT WindowObject);


#endif /* _WIN32K_CARET_H */

/* EOF */
