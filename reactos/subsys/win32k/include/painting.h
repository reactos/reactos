#ifndef __WIN32K_PAINTING_H
#define __WIN32K_PAINTING_H

#include <windows.h>
#include <ddk/ntddk.h>
#include <include/class.h>
#include <include/msgqueue.h>
#include <include/window.h>

/* PaintRedrawWindow() control flags */
#define RDW_EX_USEHRGN		0x0001
#define RDW_EX_DELETEHRGN	0x0002
#define RDW_EX_XYWINDOW		0x0004
#define RDW_EX_TOPFRAME		0x0010
#define RDW_EX_DELAY_NCPAINT    0x0020

/* Update non-client region flags. */
#define UNC_DELAY_NCPAINT                      (0x00000001)
#define UNC_IN_BEGINPAINT                      (0x00000002)
#define UNC_CHECK                              (0x00000004)
#define UNC_REGION                             (0x00000008)
#define UNC_ENTIRE                             (0x00000010)
#define UNC_UPDATE                             (0x00000020)

HWND STDCALL
PaintingFindWinToRepaint(HWND hWnd, PW32THREAD Thread);
BOOL STDCALL
PaintRedrawWindow(PWINDOW_OBJECT Wnd, const RECT* UpdateRect, HRGN UpdateRgn,
		  ULONG Flags, ULONG ExFlags);
BOOL STDCALL
PaintHaveToDelayNCPaint(PWINDOW_OBJECT Window, ULONG Flags);
HRGN STDCALL
PaintUpdateNCRegion(PWINDOW_OBJECT Window, HRGN hRgn, ULONG Flags);
BOOL STDCALL
NtUserValidateRgn(HWND hWnd, HRGN hRgn);

#endif /* __WIN32K_PAINTING_H */
