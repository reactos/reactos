#include <windows.h>
#include <stdio.h>
#include <string.h>

//HFONT tf;
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

  wc.lpszClassName = "HotkeyTestClass";
  wc.lpfnWndProc = MainWndProc;
  wc.style = CS_VREDRAW | CS_HREDRAW;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(NULL, (LPCTSTR)IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, (LPCTSTR)IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszMenuName = NULL;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  if (RegisterClass(&wc) == 0)
    {
      fprintf(stderr, "RegisterClass failed (last error 0x%lX)\n",
	      GetLastError());
      return(1);
    }

  hWnd = CreateWindow("HotkeyTestClass",
		      "Hotkeys Test",
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


#define CTRLC 1
#define ALTF1 2
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC;

	switch(msg)
	{
	case WM_PAINT:
	  hDC = BeginPaint(hWnd, &ps);
	  TextOut(hDC, 10, 10, "Press Ctrl+C or Ctrl+Alt+F1", 
              strlen("Press Ctrl+C or Ctrl+Alt+F1"));
	  EndPaint(hWnd, &ps);
	  break;
    
    case WM_HOTKEY:
      switch(wParam)
      {
        case CTRLC:
          MessageBox(hWnd, "You just pressed Ctrl+C", "Hotkey", MB_OK | MB_ICONINFORMATION);
          break;
        case ALTF1:
          MessageBox(hWnd, "You just pressed Ctrl+Alt+F1", "Hotkey", MB_OK | MB_ICONINFORMATION);
          break;
      }
      break;

	case WM_DESTROY:
	  UnregisterHotKey(hWnd, CTRLC);
	  UnregisterHotKey(hWnd, ALTF1);
	  PostQuitMessage(0);
	  break;
    
    case WM_CREATE:
      /* Register a Ctrl+Alt+C hotkey*/
      RegisterHotKey(hWnd, CTRLC, MOD_CONTROL, VK_C);
      RegisterHotKey(hWnd, ALTF1, MOD_CONTROL | MOD_ALT, VK_F1);
      break;

	default:
	  return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}
