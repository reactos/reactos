#include <windows.h>
#include "resource.h"

const char titleDrwIco[] = "DrawIcon Output";
const char titleMask[] = "Mask(AND image)";
const char titleXor[] = "XOR(color image)";
const char file[] = "Icon from file:";
const char res[] = "Icon from Resorce:";

HFONT tf;
HINSTANCE hInst;

LRESULT WINAPI MainWndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI 
WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpszCmdLine,
    int nCmdShow)
{
  hInst = hInstance;
  WNDCLASS wc;
  MSG msg;
  HWND hWnd;

  wc.lpszClassName = "IconTestClass";
  wc.lpfnWndProc = MainWndProc;
  wc.style = CS_VREDRAW | CS_HREDRAW;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
  wc.lpszMenuName = NULL;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  if (RegisterClass(&wc) == 0)
  {
    DbgPrint("RegisterClass failed (last error 0x%X)\n", GetLastError());
    return(1);
  }

  hWnd = CreateWindow("IconTestClass",
              "Icon Test",
              WS_OVERLAPPEDWINDOW|WS_HSCROLL|WS_VSCROLL,
              CW_USEDEFAULT,
              CW_USEDEFAULT,
              455,
              320,
              NULL,
              NULL,
              hInstance,
              NULL);
  if (hWnd == NULL)
  {
    DbgPrint("CreateWindow failed (last error 0x%X)\n", GetLastError());
    return(1);
  }
    
  tf = CreateFontA(14,0, 0, TA_BASELINE, FW_NORMAL, FALSE, FALSE, FALSE,
                   ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                   DEFAULT_QUALITY, FIXED_PITCH|FF_DONTCARE, "Timmons");

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
    HICON hIcon;
    HGDIOBJ hOld;
    HDC hMemDC;
    ICONINFO iconinfo;
    HBITMAP hMaskBitmap;
    HBITMAP hColorBitmap;    
        
    switch(msg)
    {
    case WM_PAINT:
      hDC = BeginPaint(hWnd, &ps);
      SelectObject(hDC, tf);
      SetBkMode(hDC, TRANSPARENT);
      
      TextOut(hDC, 160, 10, file, strlen(file));
      TextOut(hDC, 15, 85, titleDrwIco, strlen(titleDrwIco));
      TextOut(hDC, 160, 85, titleMask, strlen(titleMask));
      TextOut(hDC, 300, 85, titleXor, strlen(titleXor));
      
      hIcon = LoadImage(NULL, "icon.ICO", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE|LR_LOADFROMFILE);
      DrawIcon(hDC,50,50,hIcon);

      hMemDC = CreateCompatibleDC(hDC);
      GetIconInfo(hIcon, &iconinfo);
      DestroyIcon(hIcon);

      hOld = SelectObject(hMemDC, iconinfo.hbmMask);
      BitBlt(hDC, 200, 50, 32, 32, hMemDC, 0, 0, SRCCOPY);
      SelectObject(hMemDC, iconinfo.hbmColor);
      BitBlt(hDC, 350, 50, 32, 32, hMemDC, 0, 0, SRCCOPY);

      DeleteObject(iconinfo.hbmMask);
      DeleteObject(iconinfo.hbmColor);

      SelectObject(hMemDC, hOld);
      
      TextOut(hDC, 145, 150, res, strlen(res));
      TextOut(hDC, 15, 225, titleDrwIco, strlen(titleDrwIco));
      TextOut(hDC, 160, 225, titleMask, strlen(titleMask));
      TextOut(hDC, 300, 225, titleXor, strlen(titleXor));
          
      hIcon = LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);          
      DrawIcon(hDC,50,190,hIcon);
          
      GetIconInfo(hIcon, &iconinfo);
      DestroyIcon(hIcon);          
          
      hOld = SelectObject(hMemDC, iconinfo.hbmMask);
      BitBlt(hDC, 200, 190, 32, 32, hMemDC, 0, 0, SRCCOPY);
      SelectObject(hMemDC, iconinfo.hbmColor);
      BitBlt(hDC, 350, 190, 32, 32, hMemDC, 0, 0, SRCCOPY);

      DeleteObject(iconinfo.hbmMask);
      DeleteObject(iconinfo.hbmColor);
      DeleteObject(hMemDC);          
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
