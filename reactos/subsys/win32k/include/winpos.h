LRESULT
WinPosGetNonClientSize(HWND Wnd, RECT* WindowRect, RECT* ClientRect);
VOID
WinPosGetMinMaxInfo(PWINDOW_OBJECT Window, POINT* MaxSize, POINT* MaxPos,
		    POINT* MinTrack, POINT* MaxTrack);
VOID
WinPosMinMaximize(PWINDOW_OBJECT WindowObject, UINT ShowFlag, RECT* NewPos);
BOOLEAN
WinPosSetWindowPos(HWND Wnd, HWND WndInsertAfter, INT x, INT y, INT cx,
		   INT cy, UINT flags);
BOOLEAN
WinPosShowWindow(HWND Wnd, INT Cmd);
