#include <windows.h>

char szAppName[] = "Hello";

long FAR PASCAL WndProc(HWND, UINT, WPARAM, LPARAM);

int PASCAL WinMain (HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR lpszCmdLine,
			 int nCmdShow)
{
	HWND hwnd;
	MSG msg;
	WNDCLASS wndclass;

	if(!hPrevInst) {

		wndclass.style =  CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc = WndProc;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 0;
		wndclass.hInstance = hInstance;
		wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndclass.hbrBackground = GetStockObject(WHITE_BRUSH);
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = szAppName;

		RegisterClass(&wndclass);

					
	}

	hwnd = CreateWindow(szAppName, szAppName,
	 WS_HSCROLL | WS_VSCROLL | WS_OVERLAPPEDWINDOW,
	 CW_USEDEFAULT, CW_USEDEFAULT, 600,
	 400, NULL, NULL, hInstance, NULL);

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);
															
									
	while(GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}					
	return msg.wParam;
}					



long FAR PASCAL WndProc(HWND hwnd, UINT message, WPARAM wParam,
				LPARAM lParam)
{
	HDC hdc;
	RECT rect;
	SIZE size;
	PAINTSTRUCT ps;

	switch(message) {
			
	case WM_PAINT:
	    	hdc = BeginPaint(hwnd, &ps);
		GetClientRect(hwnd, &rect);
		InflateRect(&rect, -10, -10);
		if( !IsRectEmpty( &rect ) )
		{
                    GetTextExtentPoint32(hdc, szAppName, strlen(szAppName), &size);
		    SelectObject(hdc, GetStockObject(LTGRAY_BRUSH));
		    Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
		    rect.left = (rect.right + rect.left - size.cx) / 2;
		    rect.top  = (rect.bottom + rect.top - size.cy) / 2;
		    SetBkMode(hdc, TRANSPARENT);
		    TextOut(hdc, rect.left, rect.top, szAppName, strlen(szAppName) );
		}
		EndPaint(hwnd, &ps);
		return 0;
							
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}												

