#include <windows.h>
#include <stdio.h>
#include <string.h>

LRESULT WINAPI MainWndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI 
WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine,
	int nCmdShow)
{
  WNDCLASS wc;
  MSG msg;
  HWND hWnd;

  wc.lpszClassName = "GradientClass";
  wc.lpfnWndProc = MainWndProc;
  wc.style = CS_VREDRAW | CS_HREDRAW;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(NULL, (LPCTSTR)IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, (LPCTSTR)IDC_ARROW);
  wc.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
  wc.lpszMenuName = NULL;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  if (RegisterClass(&wc) == 0)
    {
      fprintf(stderr, "RegisterClass failed (last error 0x%lX)\n",
	      GetLastError());
      return(1);
    }

  hWnd = CreateWindow("GradientClass",
		      "GradientFill Test",
		      WS_OVERLAPPEDWINDOW|WS_HSCROLL|WS_VSCROLL,
		      0,
		      0,
		      CW_USEDEFAULT,
		      CW_USEDEFAULT,
		      NULL,
		      NULL,
		      hInstance,
		      NULL);
  if (hWnd == NULL)
    {
      fprintf(stderr, "CreateWindow failed (last error 0x%lX)\n",
	      GetLastError());
      return(1);
    }

	//tf = CreateFontA(14, 0, 0, TA_BASELINE, FW_NORMAL, FALSE, FALSE, FALSE,
	//	ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
	//	DEFAULT_QUALITY, FIXED_PITCH|FF_DONTCARE, "Timmons");

  ShowWindow(hWnd, nCmdShow);

  while(GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  //DeleteObject(tf);

  return msg.wParam;
}
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HDC hDC;

	switch(msg)
	{
	case WM_PAINT:
	{
	  PAINTSTRUCT ps;
	  TRIVERTEX        vert [5] ;
          GRADIENT_TRIANGLE    gTRi[3];
          GRADIENT_RECT        gRect[2];
	  hDC = BeginPaint(hWnd, &ps);
	  
	  vert [0] .x       =  0;
	  vert [0] .y       =  0;
	  vert [0] .Red     =  0xff00;
	  vert [0] .Green   =  0xff00;
	  vert [0] .Blue    =  0xff00;
	  vert [0] .Alpha   =  0x0000;

	  vert [1] .x       =  300;
	  vert [1] .y       =  20;
	  vert [1] .Red     =  0x0000;
	  vert [1] .Green   =  0x0000;
	  vert [1] .Blue    =  0xff00;
	  vert [1] .Alpha   =  0x0000;
	  
	  vert [2] .x       =  100;
	  vert [2] .y       =  200; 
	  vert [2] .Red     =  0xff00;
	  vert [2] .Green   =  0x0000;
	  vert [2] .Blue    =  0x0000;
	  vert [2] .Alpha   =  0x0000;

	  vert [3] .x       =  250;
	  vert [3] .y       =  300; 
	  vert [3] .Red     =  0x8000;
	  vert [3] .Green   =  0x8000;
	  vert [3] .Blue    =  0x0000;
	  vert [3] .Alpha   =  0x0000;

	  vert [4] .x       =  325;
	  vert [4] .y       =  300; 
	  vert [4] .Red     =  0x0000;
	  vert [4] .Green   =  0xff00;
	  vert [4] .Blue    =  0x0000;
	  vert [4] .Alpha   =  0x0000;

	  gTRi[0].Vertex1   = 0;
	  gTRi[0].Vertex2   = 1;
	  gTRi[0].Vertex3   = 2;

	  gTRi[1].Vertex1   = 1;
	  gTRi[1].Vertex2   = 2;
	  gTRi[1].Vertex3   = 3;

	  gTRi[2].Vertex1   = 1;
	  gTRi[2].Vertex2   = 3;
	  gTRi[2].Vertex3   = 4;

	  GdiGradientFill(hDC,vert,5,&gTRi,3,GRADIENT_FILL_TRIANGLE);


	  vert [0] .x      = 5;
	  vert [0] .y      = 200;
	  vert [0] .Red    = 0x0000;
	  vert [0] .Green  = 0x0000;
	  vert [0] .Blue   = 0x0000;
	  vert [0] .Alpha  = 0x0000;

	  vert [1] .x      = 90;
	  vert [1] .y      = 240; 
	  vert [1] .Red    = 0x0000;
	  vert [1] .Green  = 0x0000;
	  vert [1] .Blue   = 0xff00;
	  vert [1] .Alpha  = 0x0000;
	  
	  vert [2] .x      = 5;
	  vert [2] .y      = 245;
	  vert [2] .Red    = 0x0000;
	  vert [2] .Green  = 0x0000;
	  vert [2] .Blue   = 0x0000;
	  vert [2] .Alpha  = 0x0000;

	  vert [3] .x      = 90;
	  vert [3] .y      = 300; 
	  vert [3] .Red    = 0x0000;
	  vert [3] .Green  = 0x0000;
	  vert [3] .Blue   = 0xff00;
	  vert [3] .Alpha  = 0x0000;

	  gRect[0].UpperLeft  = 0;
	  gRect[0].LowerRight = 1;
	  
	  gRect[1].UpperLeft  = 2;
	  gRect[1].LowerRight = 3;
	  
	  GdiGradientFill(hDC,vert,4,&gRect[0],1,GRADIENT_FILL_RECT_H);
	  GdiGradientFill(hDC,vert,4,&gRect[1],1,GRADIENT_FILL_RECT_V);

          EndPaint(hWnd, &ps);
	  break;
	}

	case WM_DESTROY:
	  PostQuitMessage(0);
	  break;

	default:
	  return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}
