#include <stdio.h>
#include <windows.h>
/* Win32 counterpart for CalcChildScroll16 is not implemented */
/* even in MS Visual C++ */
// #include "windef.h"
// #include "wingdi.h"
/*#include <wine/winuser16.h>*/

void Write (HDC dc, int x, int y, char *s)
{
    SetBkMode(dc, TRANSPARENT);
    TextOut (dc, x, y, s, strlen (s));
}

LRESULT CALLBACK WndProc (HWND wnd, UINT msg, WPARAM w, LPARAM l)
{
    static short xChar, yChar;
    static RECT  rectHola;
    static char* strHola = "Hola";
    HDC dc;
    PAINTSTRUCT ps;
    TEXTMETRIC tm;

    switch (msg){
    case WM_CREATE:
	dc = GetDC (wnd);
	GetTextMetrics (dc, &tm);
	xChar = tm.tmAveCharWidth;
	yChar = tm.tmHeight;
	GetTextExtentPoint32( dc, strHola, strlen(strHola), ((LPSIZE)&rectHola) + 1 );
	OffsetRect( &rectHola, xChar, yChar );
	ReleaseDC (wnd, dc);
	break;

    case WM_HSCROLL:
    case WM_VSCROLL:
	InvalidateRect(wnd, &rectHola, TRUE );
//        ScrollChildren(wnd, msg, w, l);
        return 0;

    case WM_PAINT:
	dc = BeginPaint (wnd, &ps);
	Write (dc, xChar, yChar, strHola);
	EndPaint (wnd, &ps);
	break;

    case WM_DESTROY:
	PostQuitMessage (0);
	break;

    default:
	return DefWindowProc (wnd, msg, w, l);
    }
    return 0l;
}

LRESULT CALLBACK WndProc2 (HWND wnd, UINT msg, WPARAM w, LPARAM l)
{
    static short xChar, yChar;
    static RECT  rectInfo;
    char buf[128];
    HDC dc;
    PAINTSTRUCT ps;
    TEXTMETRIC tm;

    switch (msg){
    case WM_CREATE:
	dc = GetDC (wnd);
	GetTextMetrics (dc, &tm);
	xChar = tm.tmAveCharWidth;
	yChar = tm.tmHeight;
	ReleaseDC (wnd, dc);
	break;

    case WM_PAINT:
	dc = BeginPaint (wnd, &ps);
        sprintf(buf,"ps.rcPaint = {left = %d, top = %d, right = %d, bottom = %d}",
                ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right,ps.rcPaint.bottom);
	rectInfo.left = rectInfo.top = 0;
	GetTextExtentPoint32 (dc, buf, strlen(buf), ((LPSIZE)&rectInfo) + 1 );
	OffsetRect (&rectInfo, xChar, yChar );
	Write (dc, xChar, yChar, buf);
	EndPaint (wnd, &ps);
	break;

    case WM_MOVE:
    case WM_SIZE:
	InvalidateRect( wnd, &rectInfo, TRUE );
	/*CalcChildScroll16( (UINT16)GetParent(wnd), SB_BOTH );*/
	break;

    case WM_DESTROY:
	PostQuitMessage (0);
	break;

    default:
	return DefWindowProc (wnd, msg, w, l);
    }
    return 0l;
}

int PASCAL WinMain (HINSTANCE inst, HINSTANCE prev, LPSTR cmdline, int show)
{
    HWND     wnd,wnd2;
    MSG      msg;
    WNDCLASS class;
    char className[] = "class";  /* To make sure className >= 0x10000 */
    char class2Name[] = "class2";
    char winName[] = "Test app";

    if (!prev){
	class.style = CS_HREDRAW | CS_VREDRAW;
	class.lpfnWndProc = WndProc;
	class.cbClsExtra = 0;
	class.cbWndExtra = 0;
	class.hInstance  = inst;
	class.hIcon      = LoadIcon (0, IDI_APPLICATION);
	class.hCursor    = LoadCursor (0, IDC_ARROW);
	class.hbrBackground = GetStockObject (WHITE_BRUSH);
	class.lpszMenuName = NULL;
	class.lpszClassName = className;
        if (!RegisterClass (&class))
	    return FALSE;
    }

    wnd = CreateWindow (className, winName, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL,
			CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0, 
			0, inst, 0);

    if (!prev){
        class.lpfnWndProc = WndProc2;
	class.lpszClassName = class2Name;
	class.hbrBackground = GetStockObject(GRAY_BRUSH);
        if (!RegisterClass (&class))
	    return FALSE;
    }

    wnd2= CreateWindow (class2Name,"Child window", WS_CAPTION | WS_CHILD | WS_THICKFRAME, 
                        50, 50, 350, 100, wnd, 0, inst, 0);

    ShowWindow (wnd, show);
    UpdateWindow (wnd);
    ShowWindow (wnd2, show);
    UpdateWindow (wnd2);

    while (GetMessage (&msg, 0, 0, 0)){
	TranslateMessage (&msg);
	DispatchMessage (&msg);
    }
    return 0;
}
