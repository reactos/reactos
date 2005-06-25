#ifndef _WIN32K_CARET_H
#define _WIN32K_CARET_H

#define IDCARETTIMER (0xffff)

BOOL FASTCALL
IntDestroyCaret(PW32THREAD Win32Thread);

BOOL FASTCALL
IntSetCaretBlinkTime(UINT uMSeconds);

BOOL FASTCALL
IntSetCaretPos(int X, int Y);

BOOL FASTCALL
IntSwitchCaretShowing(PVOID Info);

VOID FASTCALL
IntDrawCaret(HWND hWnd);

#endif /* _WIN32K_CARET_H */

/* EOF */
