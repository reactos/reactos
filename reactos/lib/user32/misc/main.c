#include <windows.h>
#include <stdio.h>


LRESULT CALLBACK WindowFunc(HWND,UINT,WPARAM, LPARAM);

char szName[] = "Hallo";


int _CRT_fmode = 0;
int _CRT_glob = 0;

int __main(int argc, char **argv)
{
	return 0;
}


int i;



int main(int argc, char **argv)
{
	WIDGETS_Init();
#if 0
	HWND hwnd;
	HWND User32hWnd;
	HMENU hmenu;
	MSG msg;
	WNDCLASSEX wc1;
	HINSTANCE hInst = 0;
	int nWinMode = SW_SHOW;
        unsigned short *test;

        
	HANDLE hMod, hrsrc;
	RECT rect, cl;

	wc1.hInstance = hInst;
	wc1.lpszClassName = szName;
	wc1.lpfnWndProc = WindowFunc;
	wc1.style = 0;
	wc1.cbSize = sizeof(WNDCLASSEX);
	wc1.hIcon = LoadIcon(NULL,IDI_APPLICATION);
	wc1.hIconSm = LoadIcon(NULL,IDI_WINLOGO);;

	wc1.hCursor = NULL;
	wc1.lpszMenuName = NULL;

	wc1.cbClsExtra = 0;
	wc1.cbWndExtra = 0;

	wc1.hbrBackground = GetStockObject(WHITE_BRUSH);


	if ( !RegisterClassEx(&wc1)) return 0;


	hwnd = CreateWindowEx
	(0, szName, "test2", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,
		NULL,NULL,hInst, NULL);


        MessageBox(NULL,"Hallo","Hallo",MB_OK);

	ShowWindow(hwnd,nWinMode);
	UpdateWindow(hwnd);
#endif
	MessageBox(NULL,"xxx","yyyy",MB_OK);
#if 0
	GetWindowRect(hwnd,&rect);
	GetClientRect(hwnd,&cl);

	printf("%d\n",(rect.left - rect.right) - (cl.left - cl.right));
	SetWindowText(hwnd,"Hallo3");

	DrawMenuBar(hwnd);	
//	SendMessage( hwnd, WM_MOVE, 0,MAKELONG(0,0));
//        SendMessage( hwnd, WM_PAINT, GetWindowDC(hwnd),0);
	while(GetMessage(&msg,NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	Sleep(10000);
	return msg.wParam;
#endif
}


	
	
	

LRESULT CALLBACK WindowFunc(HWND hwnd,UINT message,WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}
