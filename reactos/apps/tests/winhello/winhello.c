#include <windows.h>

LRESULT WINAPI MainWndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpszCmdLine,
  int nCmdShow)
{
  WNDCLASS wc;
  MSG msg;
  HWND hWnd;

  wc.lpszClassName = "HelloClass";
  wc.lpfnWndProc = MainWndProc;
  wc.style = CS_VREDRAW | CS_HREDRAW;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
  wc.lpszMenuName = NULL;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  RegisterClass(&wc);

  hWnd = CreateWindow
  ( "HelloClass",
    "Hello World",
    WS_OVERLAPPEDWINDOW|WS_HSCROLL|WS_VSCROLL,
    0,
    0,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    NULL,
    NULL,
    hInstance,
    NULL
  );

  ShowWindow(hWnd, nCmdShow);

  while(GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return msg.wParam;
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT ps;
  HDC hDC;
   
  switch(msg)
  {
    case WM_PAINT:
      hDC = BeginPaint(hWnd, &ps);
      TextOut(hDC, 10, 10, "Hello World from ReactOS!", strlen("Hello World from ReactOS!"));
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
