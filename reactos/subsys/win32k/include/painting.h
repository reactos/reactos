#ifndef _WIN32K_PAINTING_H
#define _WIN32K_PAINTING_H

#include <include/class.h>
#include <include/msgqueue.h>
#include <include/window.h>

VOID FASTCALL
IntValidateParent(PWINDOW_OBJECT Child, HRGN ValidRegion);
BOOL FASTCALL
co_UserRedrawWindow(PWINDOW_OBJECT Wnd, const RECT* UpdateRect, HRGN UpdateRgn, ULONG Flags);
BOOL FASTCALL
IntGetPaintMessage(HWND hWnd, UINT MsgFilterMin, UINT MsgFilterMax, PW32THREAD Thread,
                   MSG *Message, BOOL Remove);

#endif /* _WIN32K_PAINTING_H */
