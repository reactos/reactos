#include <windows.h>
#include <stdio.h>

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

  wc.lpszClassName = "ClipClass";
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

  hWnd = CreateWindow("ClipClass",
		      "Line clipping test",
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

  ShowWindow(hWnd, nCmdShow);

  while(GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return msg.wParam;
}

static void
DrawLines(HDC hDC)
{
  static struct
    {
    int fromx;
    int fromy;
    int tox;
    int toy;
    }
  points[ ] =
    {
      {  50,  99, 125,  99 },
      { 160,  99, 190,  99 },
      { 300,  99, 225,  99 },
      {  50, 100, 125, 100 },
      { 160, 100, 190, 100 },
      { 300, 100, 225, 100 },
      {  50, 125, 300, 125 },
      {  50, 149, 125, 149 },
      { 160, 149, 190, 149 },
      { 300, 149, 225, 149 },
      {  50, 150, 125, 150 },
      { 160, 150, 190, 150 },
      { 300, 150, 225, 150 },
      { 160, 249, 190, 249 },
      { 160, 250, 190, 250 },
      { 149,  50, 149, 125 },
      { 149, 160, 149, 190 },
      { 149, 300, 149, 225 },
      { 150,  50, 150, 125 },
      { 150, 160, 150, 190 },
      { 150, 300, 150, 225 },
      { 199,  50, 199, 125 },
      { 199, 160, 199, 190 },
      { 199, 300, 199, 225 },
      { 200,  50, 200, 125 },
      { 200, 160, 200, 190 },
      { 200, 300, 200, 225 },
      { 175,  50, 175, 300 },
      {  50,  55, 300, 290 },
      { 300, 295,  50,  60 },
      {  50, 290, 300,  55 },
      { 300,  60,  50, 295 },
      {  55,  50, 290, 300 },
      { 295, 300,  60,  50 },
      {  55, 300, 290,  50 },
      { 295,  50,  60, 300 }
    };
  int i;

  for (i = 0; i < sizeof(points) / sizeof(points[0]); i++)
    {
      MoveToEx(hDC, points[i].fromx, points[i].fromy, NULL);
      LineTo(hDC, points[i].tox, points[i].toy);
    }
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC;
	RECT clr;
	HRGN ClipRgn, ExcludeRgn;
	RECT Rect;

	switch(msg)
	{
	case WM_PAINT:
	  GetClientRect(hWnd, &clr);
	  ClipRgn = CreateRectRgnIndirect(&clr);
	  hDC = BeginPaint(hWnd, &ps);
	  Rect.left = 100;
	  Rect.top = 100;
	  Rect.right = 250;
	  Rect.bottom = 150;
	  FillRect(hDC, &Rect, CreateSolidBrush(RGB(0xFF, 0x00, 0x00)));
	  ExcludeRgn = CreateRectRgnIndirect(&Rect);
	  CombineRgn(ClipRgn, ClipRgn, ExcludeRgn, RGN_DIFF);
	  DeleteObject(ExcludeRgn);
	  Rect.left = 150;
	  Rect.top = 150;
	  Rect.right = 200;
	  Rect.bottom = 250;
	  FillRect(hDC, &Rect, CreateSolidBrush(RGB(0xFF, 0x00, 0x00)));
	  SelectObject(hDC, CreatePen(PS_SOLID, 0, RGB(0xFF, 0xFF, 0x00)));
	  DrawLines(hDC);
	  SelectObject(hDC, CreatePen(PS_SOLID, 0, RGB(0x00, 0x00, 0xFF)));
	  ExcludeRgn = CreateRectRgnIndirect(&Rect);
	  CombineRgn(ClipRgn, ClipRgn, ExcludeRgn, RGN_DIFF);
	  DeleteObject(ExcludeRgn);
	  SelectClipRgn(hDC, ClipRgn);
	  DrawLines(hDC);
	  EndPaint(hWnd, &ps);
	  break;

	case WM_DESTROY:
	  PostQuitMessage(0);
	  break;

	default:
	  return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}
