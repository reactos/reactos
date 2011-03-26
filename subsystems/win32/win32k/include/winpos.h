#pragma once

#define IntPtInWindow(WndObject,x,y) \
  ((x) >= (WndObject)->rcWindow.left && \
   (x) < (WndObject)->rcWindow.right && \
   (y) >= (WndObject)->rcWindow.top && \
   (y) < (WndObject)->rcWindow.bottom && \
   (!(WndObject)->hrgnClip || ((WndObject)->style & WS_MINIMIZE) || \
    NtGdiPtInRegion((WndObject)->hrgnClip, (INT)((x) - (WndObject)->rcWindow.left), \
                    (INT)((y) - (WndObject)->rcWindow.top))))

#define IntPtInRect(lprc,pt) \
    ((pt.x >= (lprc)->left) && (pt.x < (lprc)->right) && (pt.y >= (lprc)->top) && (pt.y < (lprc)->bottom))

UINT
FASTCALL co_WinPosArrangeIconicWindows(PWND parent);
BOOL FASTCALL
IntGetClientOrigin(PWND Window, LPPOINT Point);
LRESULT FASTCALL
co_WinPosGetNonClientSize(PWND Window, RECTL* WindowRect, RECTL* ClientRect);
UINT FASTCALL
co_WinPosGetMinMaxInfo(PWND Window, POINT* MaxSize, POINT* MaxPos,
		    POINT* MinTrack, POINT* MaxTrack);
UINT FASTCALL
co_WinPosMinMaximize(PWND WindowObject, UINT ShowFlag, RECTL* NewPos);
BOOLEAN FASTCALL
co_WinPosSetWindowPos(PWND Wnd, HWND WndInsertAfter, INT x, INT y, INT cx,
		   INT cy, UINT flags);
BOOLEAN FASTCALL
co_WinPosShowWindow(PWND Window, INT Cmd);
void FASTCALL
co_WinPosSendSizeMove(PWND Window);
PWND FASTCALL
co_WinPosWindowFromPoint(PWND ScopeWin, POINT *WinPoint, USHORT* HitTest);
VOID FASTCALL co_WinPosActivateOtherWindow(PWND Window);

VOID FASTCALL WinPosInitInternalPos(PWND WindowObject,
                                    POINT *pt, RECTL *RestoreRect);
