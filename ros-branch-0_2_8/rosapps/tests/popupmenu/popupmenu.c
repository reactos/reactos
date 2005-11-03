#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "resource.h"

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

  wc.lpszClassName = "MenuTestClass";
  wc.lpfnWndProc = MainWndProc;
  wc.style = CS_VREDRAW | CS_HREDRAW;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(NULL, (LPCTSTR)IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, (LPCTSTR)IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszMenuName = (LPCTSTR)IDM_MAINMENU;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  if (RegisterClass(&wc) == 0)
    {
      fprintf(stderr, "RegisterClass failed (last error 0x%lX)\n",
	      GetLastError());
      return(1);
    }

  hWnd = CreateWindow("MenuTestClass",
		      "PopupMenu Test",
		      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
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

  while(GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return msg.wParam;
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch(msg)
  {
    case WM_COMMAND:
    {
      switch(LOWORD(wParam))
      {
        case IDM_EXIT:
          PostQuitMessage(0);
          break;
      }
      break;
    }
    case WM_RBUTTONUP:
    {
      POINT pos;
      HMENU Menu;

      pos.x = LOWORD(lParam);
      pos.y = HIWORD(lParam);
      ClientToScreen(hWnd, &pos);

      if((Menu = GetMenu(hWnd)) && (Menu = GetSubMenu(Menu, 1)))
      {
        TrackPopupMenu(Menu, 0, pos.x, pos.y, 0, hWnd, NULL);
      }
      break;
    }
    case WM_DESTROY:
    {
      PostQuitMessage(0);
      break;
    }
    default:
    {
      return DefWindowProc(hWnd, msg, wParam, lParam);
    }
  }
  return 0;
}
