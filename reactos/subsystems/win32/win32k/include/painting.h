#ifndef _WIN32K_PAINTING_H
#define _WIN32K_PAINTING_H

#include <include/class.h>
#include <include/msgqueue.h>
#include <include/window.h>

BOOL FASTCALL
co_UserRedrawWindow(PWINDOW_OBJECT Wnd, const RECTL* UpdateRect, HRGN UpdateRgn, ULONG Flags);
VOID FASTCALL
IntInvalidateWindows(PWINDOW_OBJECT Window, HRGN hRgn, ULONG Flags);
BOOL FASTCALL
IntGetPaintMessage(PWINDOW_OBJECT Window, UINT MsgFilterMin, UINT MsgFilterMax, PTHREADINFO Thread,
                   MSG *Message, BOOL Remove);
INT FASTCALL UserRealizePalette(HDC);
INT FASTCALL co_UserGetUpdateRgn(PWINDOW_OBJECT, HRGN, BOOL);

#endif /* _WIN32K_PAINTING_H */
