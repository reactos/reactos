/* Undocumented flags. */
#define SWP_NOCLIENTMOVE          0x0800
#define SWP_NOCLIENTSIZE          0x1000

LRESULT STDCALL
WinPosGetNonClientSize(HWND Wnd, RECT* WindowRect, RECT* ClientRect);
UINT STDCALL
WinPosGetMinMaxInfo(PWINDOW_OBJECT Window, POINT* MaxSize, POINT* MaxPos,
		    POINT* MinTrack, POINT* MaxTrack);
UINT STDCALL
WinPosMinMaximize(PWINDOW_OBJECT WindowObject, UINT ShowFlag, RECT* NewPos);
BOOLEAN STDCALL
WinPosSetWindowPos(HWND Wnd, HWND WndInsertAfter, INT x, INT y, INT cx,
		   INT cy, UINT flags);
BOOLEAN FASTCALL
WinPosShowWindow(HWND Wnd, INT Cmd);
USHORT STDCALL
WinPosWindowFromPoint(PWINDOW_OBJECT ScopeWin, POINT WinPoint, 
		      PWINDOW_OBJECT* Window);
