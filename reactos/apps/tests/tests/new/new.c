#include <windows.h>

HANDLE ghInstance;


LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ChildProc (HWND, UINT, WPARAM, LPARAM);

int PASCAL WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
		    LPSTR lpszCmdParam, int nCmdShow)
    {
    char szAppName[] = "ClassLook" ;
    HWND        hwnd ;
    MSG		msg ;
    WNDCLASS    wndclass ;

    ghInstance = hInstance;
    if (!hPrevInstance)
	 {
	 wndclass.style		= CS_HREDRAW | CS_VREDRAW ;
	 wndclass.lpfnWndProc	= WndProc ;
	 wndclass.cbClsExtra	= 0 ;
	 wndclass.cbWndExtra	= 0 ;
	 wndclass.hInstance	= hInstance ;
	 wndclass.hIcon		= LoadIcon (NULL, IDI_APPLICATION) ;
	 wndclass.hCursor	= LoadCursor (NULL, IDC_ARROW) ;
	 wndclass.hbrBackground	= GetStockObject (WHITE_BRUSH) ;
	 wndclass.lpszMenuName	= NULL ;
	 wndclass.lpszClassName	= szAppName ;

	 RegisterClass (&wndclass) ;
	 }

    hwnd = CreateWindow (szAppName,	/* window class name */
		  szAppName,		/* window caption */
		  WS_OVERLAPPEDWINDOW,	/* window style */
		  CW_USEDEFAULT,	/* initial x position */
		  CW_USEDEFAULT,	/* initial y position */
		  600,	/* initial x size */
		  400,	/* initial y size */
		  NULL,			/* parent window handle */
		  NULL,			/* window menu handle */
		  hInstance,		/* program instance handle */
		  NULL) ;		/* creation parameters */

    ShowWindow (hwnd, nCmdShow) ;
    UpdateWindow (hwnd) ;

    while (GetMessage (&msg, NULL, 0, 0))
	 {
	 TranslateMessage (&msg) ;
	 DispatchMessage (&msg) ;
	 }
    return msg.wParam ;
    }

LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
    HDC		hdc ;
    PAINTSTRUCT	ps ;
    RECT	rect ;
    WNDCLASS    wndclass ;
    char clsName[] = "SecondClass";

    static HWND	hChild;

    switch (message)
	 {
	 case WM_CREATE :
	     wndclass.style		= CS_PARENTDC | CS_HREDRAW | CS_VREDRAW;
    	     wndclass.lpfnWndProc	= ChildProc ;
    	     wndclass.cbClsExtra	= 0 ;
    	     wndclass.cbWndExtra	= 0 ;
    	     wndclass.hInstance		= ghInstance ;
    	     wndclass.hIcon		= NULL ;
    	     wndclass.hCursor		= LoadCursor (NULL, IDC_CROSS) ;
    	     wndclass.hbrBackground	= GetStockObject (LTGRAY_BRUSH) ;
    	     wndclass.lpszMenuName	= NULL ;
    	     wndclass.lpszClassName	= clsName;

	     RegisterClass (&wndclass);
              
             hChild = CreateWindow(clsName,"Child Window",
                 WS_CHILD | WS_VISIBLE | WS_BORDER,
                 10, 10, 580, 380, hwnd, NULL, ghInstance, NULL);
             ShowWindow(hChild, SW_SHOW);
	 case WM_PAINT :
	      hdc = BeginPaint (hwnd, &ps) ;

	      GetClientRect (hwnd, &rect) ;

	      DrawText (hdc, "Hello, Windows!", -1, &rect,
			DT_SINGLELINE | DT_CENTER | DT_VCENTER) ;

	      EndPaint (hwnd, &ps);
	      return 0 ;

	 case WM_DESTROY :
	      PostQuitMessage (0) ;
	      return 0 ;
	 }
    return DefWindowProc (hwnd, message, wParam, lParam) ;
    }

LRESULT CALLBACK ChildProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC			hDC;
    PAINTSTRUCT		ps;
    WNDCLASS		wndClass;
    char		*classes[]={"EDIT","BUTTON","LISTBOX","STATIC","SCROLLBAR","COMBOBOX","COMBOLBOX", NULL};
    char**		curr;
    char		buf[256];
    RECT	rect ;
    int i;

    switch (message) {
        case WM_PAINT:
            curr = classes;
            i=0;
            hDC = BeginPaint(hwnd, &ps);
            SelectObject(hDC,GetStockObject(ANSI_FIXED_FONT));
            while (*curr) {
              wsprintf(buf,"%12s:",*curr);
              GetClassInfo(NULL, *curr, &wndClass);
              if(wndClass.style&CS_VREDRAW)  lstrcat(buf," | CS_VREDRAW");
              if(wndClass.style&CS_HREDRAW)  lstrcat(buf," | CS_HREDRAW" );
              if(wndClass.style&CS_KEYCVTWINDOW) lstrcat(buf," | CS_KEYCVTWINDOW" );
              if(wndClass.style&CS_DBLCLKS) lstrcat(buf," | CS_DBLCLKS" );
              if(wndClass.style&CS_OWNDC) lstrcat(buf," | CS_OWNDC" );
              if(wndClass.style&CS_CLASSDC) lstrcat(buf," | CS_CLASSDC" );
              if(wndClass.style&CS_PARENTDC) lstrcat(buf," | CS_PARENTDC" );
              if(wndClass.style&CS_NOKEYCVT) lstrcat(buf," | CS_NOKEYCVT" );
              if(wndClass.style&CS_NOCLOSE) lstrcat(buf," | CS_NOCLOSE" );
              if(wndClass.style&CS_SAVEBITS) lstrcat(buf," | CS_SAVEBITS" );
              if(wndClass.style&CS_GLOBALCLASS) lstrcat(buf," | CS_GLOBALCLASS");
	      GetClientRect (hwnd, &rect) ;
	      TextOut (hDC, 5,20+i,buf,lstrlen(buf)) ;
              i += 15;
              curr++;
            }
/*            EndPaint(hwnd, &ps);
            break;
            hDC = BeginPaint(hwnd, &ps);
*/
            MoveToEx(hDC, 0, 0, NULL);
            LineTo(hDC, 500, 500);
            EndPaint(hwnd, &ps);
            break;
        default:
            return DefWindowProc (hwnd, message, wParam, lParam) ;
    }
    return (0L);
}

