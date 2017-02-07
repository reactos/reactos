#pragma once

BOOL FASTCALL co_IntDestroyCaret(PTHREADINFO Win32Thread);
BOOL FASTCALL IntSetCaretBlinkTime(UINT uMSeconds);
BOOL FASTCALL co_IntSetCaretPos(int X, int Y);
BOOL FASTCALL IntSwitchCaretShowing(PVOID Info);
BOOL FASTCALL co_UserShowCaret(PWND WindowObject);
BOOL FASTCALL co_UserHideCaret(PWND WindowObject);

/* EOF */
