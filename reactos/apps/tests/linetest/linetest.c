#include <windows.h>
#include <stdio.h>

LRESULT WINAPI MainWndProc(HWND, UINT, WPARAM, LPARAM);

const TCHAR* CLASS_NAME = _T("LineTestClass");

int WINAPI 
WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine,
	int nCmdShow)
{
  WNDCLASS wc;
  MSG msg;
  HWND hWnd;

  wc.lpszClassName = CLASS_NAME;
  wc.lpfnWndProc = MainWndProc;
  wc.style = CS_VREDRAW | CS_HREDRAW;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(NULL, (LPCSTR)IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, (LPCSTR)IDC_ARROW);
  wc.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
  wc.lpszMenuName = NULL;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  if (RegisterClass(&wc) == 0)
    {
      fprintf(stderr, "RegisterClass failed (last error 0x%X)\n",
	      GetLastError());
      return(1);
    }

  hWnd = CreateWindow(CLASS_NAME,
		      _T("Line drawing test"),
		      WS_OVERLAPPEDWINDOW,
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
      fprintf(stderr, "CreateWindow failed (last error 0x%X)\n",
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

#define red RGB(255,0,0)
#define grn RGB(0,255,0)
#define blu RGB(0,0,255)
#define blk RGB(0,0,0)

static void
DrawLines(HDC hDC)
{
  static struct
    {
    int fromx;
    int fromy;
    int tox;
    int toy;
    COLORREF clr;
    }
  points[ ] =
    {
      { 10, 10, 19, 10, red },
      { 20, 10, 20, 19, grn },
      { 20, 20, 11, 20, blu },
      { 10, 20, 10, 11, blk },

      { 35, 10, 39, 14, red },
      { 40, 15, 36, 19, grn },
      { 35, 20, 31, 16, blu },
      { 30, 15, 34, 11, blk },

      { 2, 1, 5, 2, red },
      { 6, 2, 5, 5, grn },
      { 5, 6, 2, 5, blu },
      { 1, 5, 2, 2, blk },
    };
  int i;

  for (i = 0; i < sizeof(points) / sizeof(points[0]); i++)
    {
      HPEN hpen, hpenold;
      hpen = CreatePen ( PS_SOLID, 0, points[i].clr );
      hpenold = (HPEN)SelectObject ( hDC, hpen );
      MoveToEx ( hDC, points[i].fromx, points[i].fromy, NULL );
      LineTo ( hDC, points[i].tox, points[i].toy );
      SelectObject ( hDC, hpenold );
      DeleteObject ( hpen );
    }
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT ps;
  HDC hDC;
  RECT clr;
  HBRUSH hbr, hbrold;
  
  switch(msg)
  {
  case WM_PAINT:
    GetClientRect(hWnd, &clr);
    //ClipRgn = CreateRectRgnIndirect(&clr);
    hDC = BeginPaint(hWnd, &ps);
    //Rect.left = 100;
    //Rect.top = 100;
    //Rect.right = 250;
    //Rect.bottom = 150;
    //FillRect(hDC, &Rect, CreateSolidBrush(RGB(0xFF, 0x00, 0x00)));
    //ExcludeRgn = CreateRectRgnIndirect(&Rect);
    //CombineRgn(ClipRgn, ClipRgn, ExcludeRgn, RGN_DIFF);
    //DeleteObject(ExcludeRgn);
    //Rect.left = 150;
    //Rect.top = 150;
    //Rect.right = 200;
    //Rect.bottom = 250;
    hbr = CreateSolidBrush(RGB(255, 255, 255));
    FillRect ( hDC, &clr, hbr );
    DeleteObject ( hbr );
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
