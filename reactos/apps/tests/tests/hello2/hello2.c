#include "windows.h"

int PASCAL WinMain (HINSTANCE inst, HINSTANCE prev, LPSTR cmdline, int show)
{
  return MessageBox((HWND)0,
		    (LPSTR)"Hello, hello!",
		    (LPSTR)"Hello Wine Application",
		    (MB_OK | MB_ICONEXCLAMATION));
}
