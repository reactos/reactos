#ifndef _WIN32K_PAINTING_H
#define _WIN32K_PAINTING_H

#include <windows.h>
#include <ddk/ntddk.h>
#include <include/class.h>
#include <include/msgqueue.h>
#include <include/window.h>

VOID FASTCALL
IntValidateParent(PWINDOW_OBJECT Child, HRGN ValidRegion);
BOOL FASTCALL
IntRedrawWindow(PWINDOW_OBJECT Wnd, const RECT* UpdateRect, HRGN UpdateRgn, ULONG Flags);
BOOL FASTCALL
IntGetPaintMessage(HWND hWnd, PW32THREAD Thread, MSG *Message, BOOL Remove);
BOOL STDCALL
NtUserValidateRgn(HWND hWnd, HRGN hRgn);

#endif /* _WIN32K_PAINTING_H */
