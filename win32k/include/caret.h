#ifndef _WIN32K_CARET_H
#define _WIN32K_CARET_H

#define IDCARETTIMER (0xffff)

BOOL FASTCALL
UserDestroyCaret(PW32THREAD Win32Thread);

BOOL FASTCALL
UserSetCaretBlinkTime(UINT uMSeconds);

BOOL FASTCALL
UserSwitchCaretShowing(PVOID Info);

VOID FASTCALL
UserDrawCaret(HWND hWnd);

#endif /* _WIN32K_CARET_H */

/* EOF */
