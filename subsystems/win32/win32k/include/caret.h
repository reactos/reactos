#ifndef _WIN32K_CARET_H
#define _WIN32K_CARET_H

#include <include/win32.h>
#include <include/window.h>

#define IDCARETTIMER (0xffff)

BOOL FASTCALL
co_IntDestroyCaret(PTHREADINFO Win32Thread);

BOOL FASTCALL
IntSetCaretBlinkTime(UINT uMSeconds);

BOOL FASTCALL
co_IntSetCaretPos(int X, int Y);

BOOL FASTCALL
IntSwitchCaretShowing(PVOID Info);

BOOL FASTCALL co_UserShowCaret(PWINDOW_OBJECT WindowObject);

BOOL FASTCALL co_UserHideCaret(PWINDOW_OBJECT WindowObject);


#endif /* _WIN32K_CARET_H */

/* EOF */
