#include <windows.h>
#include <stdio.h>
#include <string.h>

//HFONT tf;
HENHMETAFILE EnhMetafile;
SIZE EnhMetafileSize;
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
  ENHMETAHEADER emh;

  EnhMetafile = GetEnhMetaFile("test.emf");
  if(!EnhMetafile)
  {
    fprintf(stderr, "GetEnhMetaFile failed (last error 0x%lX)\n",
        GetLastError());
    return(1);
  }
  GetEnhMetaFileHeader(EnhMetafile, sizeof(ENHMETAHEADER), &emh);
  EnhMetafileSize.cx = emh.rclBounds.right - emh.rclBounds.left;
  EnhMetafileSize.cy = emh.rclBounds.bottom - emh.rclBounds.top;

  wc.lpszClassName = "EnhMetaFileClass";
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
      DeleteEnhMetaFile(EnhMetafile);
      fprintf(stderr, "RegisterClass failed (last error 0x%lX)\n",
	      GetLastError());
      return(1);
    }

  hWnd = CreateWindow("EnhMetaFileClass",
		      "Enhanced Metafile test",
		      WS_OVERLAPPEDWINDOW,
		      0,
		      0,
		      EnhMetafileSize.cx + (2 * GetSystemMetrics(SM_CXSIZEFRAME)) + 2,
		      EnhMetafileSize.cy + (2 * GetSystemMetrics(SM_CYSIZEFRAME)) + GetSystemMetrics(SM_CYCAPTION) + 2,
		      NULL,
		      NULL,
		      hInstance,
		      NULL);
  if (hWnd == NULL)
    {
      DeleteEnhMetaFile(EnhMetafile);
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

  DeleteEnhMetaFile(EnhMetafile);

  //DeleteObject(tf);
  UnregisterClass("EnhMetaFileClass", hInstance);

  return msg.wParam;
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{

	case WM_PAINT:
	{
	  PAINTSTRUCT ps;
	  RECT rc;
      HDC hDC;
      int bk;

	  GetClientRect(hWnd, &rc);
	  hDC = BeginPaint(hWnd, &ps);
	  rc.left = (rc.right / 2) - (EnhMetafileSize.cx / 2);
	  rc.top = (rc.bottom / 2) - (EnhMetafileSize.cy / 2);
	  rc.right = rc.left + EnhMetafileSize.cx;
	  rc.bottom = rc.top + EnhMetafileSize.cy;
	  bk = SetBkMode(hDC, TRANSPARENT);
	  Rectangle(hDC, rc.left - 1, rc.top - 1, rc.right + 1, rc.bottom + 1);
	  SetBkMode(hDC, bk);
	  PlayEnhMetaFile(hDC, EnhMetafile, &rc);
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
