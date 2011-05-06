#include <windows.h>
#include <stdio.h>

LRESULT WINAPI UnicodeWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT WINAPI UnicodeSubclassProc(HWND, UINT, WPARAM, LPARAM);
LRESULT WINAPI AnsiSubclassProc(HWND, UINT, WPARAM, LPARAM);

static WNDPROC SavedWndProcW;
static WNDPROC SavedWndProcA;

int WINAPI
WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine,
	int nCmdShow)
{
  WNDCLASSW wc;
  HWND hWnd;
  WCHAR WindowTextW[256];
  char WindowTextA[256];

  wc.lpszClassName = L"UnicodeClass";
  wc.lpfnWndProc = UnicodeWndProc;
  wc.style = 0;
  wc.hInstance = hInstance;
  wc.hIcon = NULL;
  wc.hCursor = NULL;
  wc.hbrBackground = NULL;
  wc.lpszMenuName = NULL;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  if (RegisterClassW(&wc) == 0)
    {
      fprintf(stderr, "RegisterClassW failed (last error 0x%lu)\n",
	      GetLastError());
      return 1;
    }
  printf("Unicode class registered, WndProc = 0x%p\n", wc.lpfnWndProc);

  hWnd = CreateWindowA("UnicodeClass",
		       "Unicode Window",
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
      fprintf(stderr, "CreateWindowA failed (last error 0x%lu)\n",
	      GetLastError());
      return 1;
    }

  printf("Window created, IsWindowUnicode returns %s\n", IsWindowUnicode(hWnd) ? "TRUE" : "FALSE");

  printf("Calling GetWindowTextW\n");
  if (! GetWindowTextW(hWnd, WindowTextW, sizeof(WindowTextW) / sizeof(WindowTextW[0])))
    {
      fprintf(stderr, "GetWindowTextW failed (last error 0x%lu)\n", GetLastError());
      return 1;
    }
  printf("GetWindowTextW returned Unicode string \"%S\"\n", WindowTextW);

  printf("Calling GetWindowTextA\n");
  if (! GetWindowTextA(hWnd, WindowTextA, sizeof(WindowTextA) / sizeof(WindowTextA[0])))
    {
      fprintf(stderr, "GetWindowTextA failed (last error 0x%lu)\n", GetLastError());
      return 1;
    }
  printf("GetWindowTextA returned Ansi string \"%s\"\n", WindowTextA);
  printf("\n");

  SavedWndProcW = (WNDPROC) GetWindowLongW(hWnd, GWL_WNDPROC);
  printf("GetWindowLongW returned 0x%p\n", SavedWndProcW);
  SavedWndProcA = (WNDPROC) GetWindowLongA(hWnd, GWL_WNDPROC);
  printf("GetWindowLongA returned 0x%p\n", SavedWndProcA);
  printf("\n");

  printf("Subclassing window using SetWindowLongW, new WndProc 0x%p\n", UnicodeSubclassProc);
  SetWindowLongW(hWnd, GWL_WNDPROC, (LONG) UnicodeSubclassProc);
  printf("After subclass, IsWindowUnicode %s, WndProcA 0x%lx, WndProcW 0x%lx\n",
         IsWindowUnicode(hWnd) ? "TRUE" : "FALSE", GetWindowLongA(hWnd, GWL_WNDPROC),
         GetWindowLongW(hWnd, GWL_WNDPROC));

  printf("Calling GetWindowTextW\n");
  if (! GetWindowTextW(hWnd, WindowTextW, sizeof(WindowTextW) / sizeof(WindowTextW[0])))
    {
      fprintf(stderr, "GetWindowTextW failed (last error 0x%lu)\n", GetLastError());
      return 1;
    }
  printf("GetWindowTextW returned Unicode string \"%S\"\n", WindowTextW);
  printf("\n");

  printf("Subclassing window using SetWindowLongA, new WndProc 0x%p\n", AnsiSubclassProc);
  SetWindowLongA(hWnd, GWL_WNDPROC, (LONG) AnsiSubclassProc);
  printf("After subclass, IsWindowUnicode %s, WndProcA 0x%lx, WndProcW 0x%lx\n",
         IsWindowUnicode(hWnd) ? "TRUE" : "FALSE", GetWindowLongA(hWnd, GWL_WNDPROC),
         GetWindowLongW(hWnd, GWL_WNDPROC));

  printf("Calling GetWindowTextW\n");
  if (! GetWindowTextW(hWnd, WindowTextW, sizeof(WindowTextW) / sizeof(WindowTextW[0])))
    {
      fprintf(stderr, "GetWindowTextW failed (last error 0x%lu)\n", GetLastError());
      return 1;
    }
  printf("GetWindowTextW returned Unicode string \"%S\"\n", WindowTextW);

  DestroyWindow(hWnd);

  return 0;
}

LRESULT CALLBACK UnicodeWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  LRESULT Result;

  switch(msg)
    {
      case WM_GETTEXT:
        printf("UnicodeWndProc calling DefWindowProcW\n");
        Result = DefWindowProcW(hWnd, msg, wParam, lParam);
        printf("UnicodeWndProc Unicode window text \"%S\"\n", (LPWSTR) lParam);
        break;
      default:
        Result = DefWindowProcW(hWnd, msg, wParam, lParam);
        break;
    }

  return Result;
}

LRESULT CALLBACK UnicodeSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  LRESULT Result;

  switch(msg)
    {
      case WM_GETTEXT:
        printf("UnicodeSubclassProc calling SavedWindowProc\n");
        Result = CallWindowProcW(SavedWndProcW, hWnd, msg, wParam, lParam);
        printf("UnicodeSubclassProc Unicode window text \"%S\"\n", (LPWSTR) lParam);
        break;
      default:
        Result = CallWindowProcW(SavedWndProcW, hWnd, msg, wParam, lParam);
        break;
    }

  return Result;
}

LRESULT CALLBACK AnsiSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  LRESULT Result;

  switch(msg)
    {
      case WM_GETTEXT:
        printf("AnsiSubclassProc calling SavedWindowProcA\n");
        Result = CallWindowProcA(SavedWndProcA, hWnd, msg, wParam, lParam);
        printf("AnsiSubclassProc Ansi window text \"%s\"\n", (LPSTR) lParam);
        break;
      default:
        Result = CallWindowProcA(SavedWndProcA, hWnd, msg, wParam, lParam);
        break;
    }

  return Result;
}
