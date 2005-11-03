// Static Control Test.c

//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>

#ifndef SS_ENDELLIPSIS
#define SS_ENDELLIPSIS	0x00004000L
#endif /* SS_ENDELLIPSIS */


#define nMaxCtrls 32
#define nStaticWidth 384
#define nStaticHeight 18

HWND g_hwnd = NULL;
HINSTANCE g_hInst = 0;
int nNextCtrl = 0;
HWND g_hwndCtrl[nMaxCtrls];

static void CreateStatic ( const char* lpWindowName, DWORD dwStyle )
{
	int n = nNextCtrl++;
	assert ( n < nMaxCtrls );
	g_hwndCtrl[n] = CreateWindow (
		"STATIC", // lpClassName
		lpWindowName, // lpWindowName
		WS_VISIBLE|WS_CHILD|dwStyle, // dwStyle
		n+2, // x
		nStaticHeight*n+1, // y
		nStaticWidth, // nWidth
		nStaticHeight-1, // nHeight
		g_hwnd, // hWndParent
		NULL, // hMenu
		g_hInst, // hInstance
		NULL ); // lParam
}

LRESULT CALLBACK WndProc ( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	int i;
	switch ( msg )
	{
	case WM_CREATE:
		g_hwnd = hwnd;
		for ( i = 0; i < nMaxCtrls; i++ )
			g_hwndCtrl[i] = NULL;

		CreateStatic ( "SS_NOTIFY test (click/double-click here)", SS_NOTIFY );

		CreateStatic ( "SS_ENDELLIPSIS test test test test test test test test test test test", SS_ENDELLIPSIS );

		CreateStatic ( "SS_CENTER test", SS_CENTER );

		CreateStatic ( "SS_RIGHT test", SS_RIGHT );

		CreateStatic ( "SS_BLACKFRAME test:", 0 );
		CreateStatic ( "this text shouldn't be visible!", SS_BLACKFRAME );

		CreateStatic ( "SS_BLACKRECT test:", 0 );
		CreateStatic ( "this text shouldn't be visible!", SS_BLACKRECT );

		CreateStatic ( "SS_ETCHEDFRAME test:", 0 );
		CreateStatic ( "this text shouldn't be visible!", SS_ETCHEDFRAME );

		CreateStatic ( "SS_ETCHEDHORZ test:", 0 );
		CreateStatic ( "this text shouldn't be visible!", SS_ETCHEDHORZ );

		CreateStatic ( "SS_ETCHEDVERT test", 0 );
		CreateStatic ( "this text shouldn't be visible!", SS_ETCHEDVERT );

		CreateStatic ( "SS_GRAYFRAME test", 0 );
		CreateStatic ( "this text shouldn't be visible!", SS_GRAYFRAME );

		CreateStatic ( "SS_GRAYRECT test", 0 );
		CreateStatic ( "this text shouldn't be visible!", SS_GRAYRECT );

		CreateStatic ( "SS_NOPREFIX &test", SS_NOPREFIX );

		CreateStatic ( "SS_OWNERDRAW test", SS_OWNERDRAW );

		CreateStatic ( "SS_SUNKEN test", SS_SUNKEN );

		CreateStatic ( "SS_WHITEFRAME test:", 0 );
		CreateStatic ( "this text shouldn't be visible!", SS_WHITEFRAME );

		CreateStatic ( "SS_WHITERECT test:", 0 );
		CreateStatic ( "this text shouldn't be visible!", SS_WHITERECT );

		//if ( creation fails )
		//	return 0;
		break;

	case WM_COMMAND:
		if ( HIWORD(wParam) == STN_CLICKED )
			SetWindowText ( (HWND)lParam, "SS_NOTIFY:STN_CLICKED!" );
		if ( HIWORD(wParam) == STN_DBLCLK )
			SetWindowText ( (HWND)lParam, "SS_NOTIFY:STN_DBLCLK!" );
		break;

	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT lpDrawItem = (LPDRAWITEMSTRUCT) lParam;
			DrawText ( lpDrawItem->hDC, "SS_DRAWITEM test successful!", 28, &(lpDrawItem->rcItem), 0 );
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc ( hwnd, msg, wParam, lParam );
}

HWND RegisterAndCreateWindow (
	HINSTANCE hInst,
	const char* className,
	const char* title )
{
	WNDCLASSEX wc;
	HWND hwnd;

	g_hInst = hInst;

	wc.cbSize = sizeof (WNDCLASSEX);
	wc.lpfnWndProc = WndProc;   // window procedure: mandatory
	wc.hInstance = hInst;         // owner of the class: mandatory
	wc.lpszClassName = className; // mandatory
	wc.hCursor = LoadCursor ( 0, (LPCTSTR)IDC_ARROW ); // optional
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // optional
	wc.style = 0;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hIcon = 0;
	wc.hIconSm = 0;
	wc.lpszMenuName = 0;
	if ( !RegisterClassEx ( &wc ) )
		return NULL;

	hwnd = CreateWindowEx (
		0, // dwStyleEx
		className, // class name
		title, // window title
		WS_OVERLAPPEDWINDOW, // dwStyle
		CW_USEDEFAULT, // x
		CW_USEDEFAULT, // y
		CW_USEDEFAULT, // width
		CW_USEDEFAULT, // height
		NULL, // hwndParent
		NULL, // hMenu
		hInst, // hInstance
		0 ); // lParam

	if ( !hwnd )
		return NULL;

	ShowWindow ( hwnd, SW_SHOW );
	UpdateWindow ( hwnd );

	return hwnd;
}

int WINAPI WinMain ( HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdParam, int cmdShow )
{
	char className [] = "Static Control Test";
	HWND hwnd;
	MSG msg;
	int status;

	hwnd = RegisterAndCreateWindow ( hInst, className, "Static Control Test" );

	// Message loop
	while ((status = GetMessage (& msg, 0, 0, 0)) != 0)
	{
		if (status == -1)
			return -1;
		DispatchMessage ( &msg );
	}
	return msg.wParam;
}
