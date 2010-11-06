#pragma once

#include <include/class.h>
#include <include/msgqueue.h>
#include <include/window.h>

BOOL FASTCALL
co_UserRedrawWindow(PWND Wnd, const RECTL* UpdateRect, HRGN UpdateRgn, ULONG Flags);
VOID FASTCALL
IntInvalidateWindows(PWND Window, HRGN hRgn, ULONG Flags);
BOOL FASTCALL
IntGetPaintMessage(PWND Window, UINT MsgFilterMin, UINT MsgFilterMax, PTHREADINFO Thread,
                   MSG *Message, BOOL Remove);
INT FASTCALL UserRealizePalette(HDC);
INT FASTCALL co_UserGetUpdateRgn(PWND, HRGN, BOOL);
