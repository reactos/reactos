#ifndef __WIN32K_PAINTING_H
#define __WIN32K_PAINTING_H

#include <windows.h>
#include <ddk/ntddk.h>
#include <include/class.h>
#include <include/msgqueue.h>

/* PaintRedrawWindow() control flags */
#define RDW_EX_USEHRGN		0x0001
#define RDW_EX_DELETEHRGN	0x0002
#define RDW_EX_XYWINDOW		0x0004
#define RDW_EX_TOPFRAME		0x0010
#define RDW_EX_DELAY_NCPAINT    0x0020

HWND STDCALL
PaintingFindWinToRepaint(HWND hWnd, PW32THREAD Thread);
BOOL STDCALL
PaintHaveToDelayNCPaint(PWINDOW_OBJECT Window, ULONG Flags);
HRGN STDCALL
PaintUpdateNCRegion(PWINDOW_OBJECT Window, HRGN hRgn, ULONG Flags);
#endif /* __WIN32K_PAINTING_H */
