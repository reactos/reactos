/*
 *  ReactOS applications
 *  Copyright (C) 2001, 2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: welcome.c,v 1.1 2002/08/23 16:15:09 ekohl Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS welcome/autorun application
 * FILE:        welcome/welcome.c
 * PROGRAMMERS: Eric Kohl (ekohl@rz-online.de)
 */

#include <windows.h>
#include <string.h>
#include <tchar.h>

#include "resource.h"



#define LIGHT_BLUE 0x00F0CAA6
#define DARK_BLUE  0x00996633



/* GLOBALS ******************************************************************/

TCHAR szFrameClass [] = "WelcomeWindowClass";
TCHAR szAppTitle [80];

HINSTANCE hInstance;

HWND hwndMain = 0;
HWND hwndDefaultTopic = 0;

HBITMAP hTitleBitmap = 0;
HDC hdcDisplay=0;
HDC hdcMem = 0;

int nTopic = -1;
int nDefaultTopic = -1;

ULONG ulInnerWidth = 480;
ULONG ulInnerHeight = 360;

HBITMAP hDefaultTopicBitmap;
HBITMAP hTopicBitmap[10];
HWND hwndTopicButton[10];
HWND hwndCloseButton;
HWND hwndCheckButton;


HFONT hfontTopicButton;
HFONT hfontTopicTitle;
HFONT hfontTopicDescription;
HFONT hfontCheckButton;

HFONT hfontBannerTitle;

HBRUSH hbrLightBlue;
HBRUSH hbrDarkBlue;
HBRUSH hbrRightPanel;

RECT rcTitlePanel;
RECT rcLeftPanel;
RECT rcRightPanel;

WNDPROC fnOldBtn;


LRESULT CALLBACK
MainWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


/* FUNCTIONS ****************************************************************/

int WINAPI
WinMain(HINSTANCE hInst,
	HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine,
	int nCmdShow)
{
  WNDCLASSEX wndclass;
  MSG msg;
  int xPos;
  int yPos;
  int xWidth;
  int yHeight;
  RECT rcWindow;
  DWORD dwStyle = WS_OVERLAPPED | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_CAPTION | WS_SYSMENU | WS_VISIBLE;

  hInstance = hInst;

  /* Register the window class */
  wndclass.style = CS_OWNDC;
  wndclass.lpfnWndProc = (WNDPROC)MainWndProc;
  wndclass.cbClsExtra = 0;
  wndclass.cbWndExtra = 0;
  wndclass.hInstance = hInstance;
  wndclass.hIcon = 0;
  wndclass.hCursor = LoadCursor (NULL, IDC_ARROW);
  wndclass.hbrBackground = 0;
  wndclass.lpszMenuName = NULL;
  wndclass.lpszClassName = szFrameClass;

  wndclass.cbSize = sizeof(WNDCLASSEX);
  wndclass.hIconSm = 0;

  RegisterClassEx(&wndclass);

  rcWindow.top = 0;
  rcWindow.bottom = ulInnerHeight - 1;
  rcWindow.left = 0;
  rcWindow.right = ulInnerWidth - 1;

  AdjustWindowRect(&rcWindow,
		    dwStyle,
		    FALSE);
  xWidth = rcWindow.right - rcWindow.left;
  yHeight = rcWindow.bottom - rcWindow.top;

  xPos = (GetSystemMetrics(SM_CXSCREEN) - xWidth) / 2;
  yPos = (GetSystemMetrics(SM_CYSCREEN) - yHeight) / 2;

  rcTitlePanel.top = 0;
  rcTitlePanel.bottom = 93;
  rcTitlePanel.left = 0;
  rcTitlePanel.right = ulInnerWidth - 1;

  rcLeftPanel.top = rcTitlePanel.bottom;
  rcLeftPanel.bottom = ulInnerHeight - 1;
  rcLeftPanel.left = 0;
  rcLeftPanel.right = ulInnerWidth / 3;

  rcRightPanel.top = rcLeftPanel.top;
  rcRightPanel.bottom = rcLeftPanel.bottom;
  rcRightPanel.left = rcLeftPanel.right;
  rcRightPanel.right = ulInnerWidth - 1;

  if (!LoadString(hInstance, (UINT)MAKEINTRESOURCE(IDS_APPTITLE), szAppTitle, 80))
    _tcscpy(szAppTitle, TEXT("ReactOS Welcome"));

  /* Create main window */
  hwndMain = CreateWindow(szFrameClass,
			  szAppTitle,
			  dwStyle,
			  xPos,
			  yPos,
			  xWidth,
			  yHeight,
			  0,
			  0,
			  hInstance,
			  NULL);

  ShowWindow(hwndMain, nCmdShow);
  UpdateWindow(hwndMain);

  while (GetMessage(&msg, NULL, 0, 0) != FALSE)
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

  return(msg.wParam);
}


LRESULT CALLBACK
ButtonSubclassWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  LONG i;

  if (uMsg == WM_MOUSEMOVE)
    {
      i = GetWindowLong(hWnd, GWL_ID);
      if (nTopic != i)
	{
	  nTopic = i;
	  SetFocus(hWnd);
	  InvalidateRect(hwndMain, NULL, TRUE);
	}
    }

  return(CallWindowProc(fnOldBtn, hWnd, uMsg, wParam, lParam));
}


static BOOL
RunApplication(int nTopic)
{
  PROCESS_INFORMATION ProcessInfo;
  STARTUPINFO StartupInfo;
  CHAR AppName[256];
  CHAR CurrentDir[256];
  int nLength;

  InvalidateRect(hwndMain, NULL, TRUE);

  GetCurrentDirectory(256, CurrentDir);

  nLength = LoadString(hInstance, IDS_TOPICACTION0 + nTopic, AppName, 256);
  if (nLength == 0)
    return(FALSE);

  if (stricmp(AppName, "explorer.exe") == 0)
    {
      strcat(AppName, " ");
      strcat(AppName, CurrentDir);
    }

  memset(&StartupInfo, 0, sizeof(STARTUPINFO));
  StartupInfo.cb = sizeof(STARTUPINFO);
  StartupInfo.lpTitle = "Test";
  StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
  StartupInfo.wShowWindow = SW_SHOWNORMAL;

  CreateProcess(NULL, AppName, NULL, NULL, FALSE, CREATE_NEW_CONSOLE,NULL,
		CurrentDir,
		&StartupInfo,
		&ProcessInfo);

  CloseHandle(ProcessInfo.hProcess);
  CloseHandle(ProcessInfo.hThread);

  return(TRUE);
}

static VOID
SubclassButton(HWND hWnd)
{
  fnOldBtn = (WNDPROC)SetWindowLong(hWnd, GWL_WNDPROC, (LPARAM)ButtonSubclassWndProc);
}


static DWORD
GetButtonHeight(HDC hDC,
		HFONT hFont,
		PSZ szText,
		DWORD dwWidth)
{
  HFONT hOldFont;
  RECT rect;

  rect.left = 0;
  rect.right = dwWidth - 20;
  rect.top = 0;
  rect.bottom = 25;

  hOldFont = SelectObject(hDC, hFont);
  DrawText(hDC, szText, -1, &rect, DT_TOP | DT_CALCRECT | DT_WORDBREAK);
  SelectObject(hDC, hOldFont);

  return(rect.bottom-rect.top + 14);
}


static LRESULT
OnCreate(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  char szText[80];
  int i,nLength;
  DWORD dwTop;
  DWORD dwHeight = 0;

  hbrLightBlue = CreateSolidBrush (LIGHT_BLUE);
  hbrDarkBlue = CreateSolidBrush(DARK_BLUE);
  hbrRightPanel = CreateSolidBrush (0x00FFFFFF);

  /* Banner fonts */
  hfontBannerTitle = CreateFont(-30,0,0,0,FW_BOLD,
				FALSE,FALSE,FALSE,ANSI_CHARSET,
			       OUT_DEFAULT_PRECIS,
			       CLIP_DEFAULT_PRECIS,
			       DEFAULT_QUALITY,
			       FF_DONTCARE,
			       "Arial");

  hfontTopicTitle = CreateFont(-18,0,0,0,FW_NORMAL,
			       FALSE,FALSE,FALSE,ANSI_CHARSET,
			       OUT_DEFAULT_PRECIS,
			       CLIP_DEFAULT_PRECIS,
			       DEFAULT_QUALITY,
			       FF_DONTCARE,
			       "Arial");


  hfontTopicDescription = CreateFont(-11,0,0,0,FW_THIN,
				     FALSE,FALSE,FALSE,ANSI_CHARSET,
				     OUT_DEFAULT_PRECIS,
				     CLIP_DEFAULT_PRECIS,
				     DEFAULT_QUALITY,
				     FF_DONTCARE,
				     "Arial");

  hfontTopicButton = CreateFont(-11,0,0,0,FW_BOLD,
				FALSE,FALSE,FALSE,ANSI_CHARSET,
				OUT_DEFAULT_PRECIS,
				CLIP_DEFAULT_PRECIS,
				DEFAULT_QUALITY,
				FF_DONTCARE,
				"Arial");

  /* Load bitmaps */
  hTitleBitmap = LoadBitmap (hInstance, MAKEINTRESOURCE(IDB_TITLEBITMAP));
  hDefaultTopicBitmap = LoadBitmap (hInstance, MAKEINTRESOURCE(IDB_DEFAULTTOPICBITMAP));
  for (i=0;i < 10; i++)
    {
      hTopicBitmap[i] = LoadBitmap (hInstance, MAKEINTRESOURCE(IDB_TOPICBITMAP0+i));
    }

  hdcDisplay = CreateDC ("DISPLAY", NULL, NULL, NULL);
  hdcMem = CreateCompatibleDC (hdcDisplay);

  /* load and create buttons */
  dwTop = rcLeftPanel.top;
  for (i=0;i < 10; i++)
    {
      nLength = LoadString(hInstance, IDS_TOPICBUTTON0+i,szText,80);
      if (nLength > 0)
	{
	  dwHeight = GetButtonHeight(hdcMem,
				     hfontTopicButton,
				     szText,
				     rcLeftPanel.right - rcLeftPanel.left);

	  hwndTopicButton[i] = CreateWindow("BUTTON",
					    szText,
					    WS_CHILDWINDOW | WS_VISIBLE | WS_TABSTOP | BS_MULTILINE | BS_OWNERDRAW,
					    rcLeftPanel.left,
					    dwTop,
					    rcLeftPanel.right - rcLeftPanel.left,
					    dwHeight,
					    hWnd,
					    (HMENU)i,
					    hInstance,
					    NULL);
	  hwndDefaultTopic = hwndTopicButton[i];
	  nDefaultTopic = i;
	  SubclassButton(hwndTopicButton[i]);
	  SendMessage(hwndTopicButton[i], WM_SETFONT, (WPARAM)hfontTopicButton, MAKELPARAM(TRUE,0));
	}
      else
	{
	  hwndTopicButton[i] = 0;
	}

      dwTop += dwHeight;
    }

  /* Create exit button */
  nLength = LoadString(hInstance, IDS_CLOSETEXT, szText, 80);
  if (nLength > 0)
    {
      hwndCloseButton = CreateWindow("BUTTON",
				     szText,
				     WS_VISIBLE | WS_CHILD | BS_FLAT,
				     rcRightPanel.right - 10 - 57,
				     rcRightPanel.bottom - 10 - 21,
				     57,
				     21,
				     hWnd,
				     (HMENU)IDC_CLOSEBUTTON,
				     hInstance,
				     NULL);
      hwndDefaultTopic = 0;
      nDefaultTopic = -1;
      SendMessage(hwndCloseButton, WM_SETFONT, (WPARAM)hfontTopicButton, MAKELPARAM(TRUE,0));
    }
  else
    {
      hwndCloseButton = 0;
    }

  /* Create checkbox */
  nLength = LoadString(hInstance, IDS_CHECKTEXT,szText,80);
  if (nLength > 0)
    {
      hfontCheckButton = CreateFont(-10,0,0,0,FW_THIN,FALSE,FALSE,FALSE,ANSI_CHARSET,
				    OUT_DEFAULT_PRECIS,
				    CLIP_DEFAULT_PRECIS,
				    DEFAULT_QUALITY,
				    FF_DONTCARE,
				    "Tahoma");

      hwndCheckButton = CreateWindow("BUTTON",
				     szText,
				     WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
				     rcLeftPanel.left + 8,
				     rcLeftPanel.bottom - 8 - 13,
				     rcLeftPanel.right - rcLeftPanel.left - 16,
				     13,
				     hWnd,
				     (HMENU)IDC_CHECKBUTTON,
				     hInstance,
				     NULL);
      SendMessage(hwndCheckButton, WM_SETFONT, (WPARAM)hfontCheckButton, MAKELPARAM(TRUE,0));
    }
  else
    {
      hwndCheckButton = 0;
      hfontCheckButton = 0;
    }

  return 0;
}


static LRESULT
OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  if (LOWORD(wParam) == IDC_CLOSEBUTTON)
    {
      DestroyWindow(hWnd);
    }
  else if ((LOWORD(wParam) < 10))
    {
      if (RunApplication(LOWORD(wParam)) == FALSE)
	{
	  DestroyWindow(hWnd);
	}
    }
  return 0;
}


static VOID
PaintBanner(HDC hdc, LPRECT rcPanel)
{
  HBRUSH hOldBrush;
  HFONT hOldFont;
  RECT rcTitle;

#if 0
  /* banner bitmap */
  hOldBitmap = SelectObject(hdcMem, hTitleBitmap);
  BitBlt(hdc,
	 rcPanel->left,
	 rcPanel->top,
	 rcPanel->right,
	 rcPanel->bottom - 3,
	 hdcMem, 0, 0, SRCCOPY);
  SelectObject(hdc, hOldBitmap);
#endif

  /* white background */
  hOldBrush = SelectObject(hdc, hbrRightPanel);
  PatBlt(hdc,
	 rcPanel->left,
	 rcPanel->top,
	 rcPanel->right - rcPanel->left,
	 rcPanel->bottom - 3,
	 PATCOPY);

  hOldFont = SelectObject (hdc, hfontBannerTitle);
  SetTextColor(hdc, 0x00000000);

  rcTitle.top = 5;
  rcTitle.left = 15;
  DrawText(hdc, "ReactOS 0.0.20", -1,
	   &rcTitle, DT_TOP | DT_CALCRECT);

  DrawText(hdc, "ReactOS 0.0.20", -1,
	   &rcTitle, DT_TOP);

  SelectObject(hdc, hOldFont);

  /* dark blue line */
  hOldBrush = SelectObject(hdc, hbrDarkBlue);
  PatBlt(hdc,
	 rcPanel->left,
	 rcPanel->bottom - 3,
	 rcPanel->right - rcPanel->left,
	 3,
	 PATCOPY);

  SelectObject(hdc, hOldBrush);
}


static LRESULT
OnPaint(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  HDC hdc;
  PAINTSTRUCT ps;
  HBITMAP hOldBitmap = 0;
  HBRUSH hOldBrush;
  HFONT hOldFont;
  RECT rcTitle, rcDescription;
  TCHAR szTopicTitle[80];
  TCHAR szTopicDesc[256];
  int nLength;
  BITMAP bmpInfo;

  hdc = BeginPaint(hWnd, &ps);

  /* Banner panel */
  PaintBanner(hdc, &rcTitlePanel);

  /* Left panel */
  hOldBrush = SelectObject (hdc, hbrLightBlue);
  PatBlt(hdc,
	 rcLeftPanel.left,
	 rcLeftPanel.top,
	 rcLeftPanel.right - rcLeftPanel.left,
	 rcLeftPanel.bottom - rcLeftPanel.top,
	 PATCOPY);

  /* Right panel */
  SelectObject (hdc, hbrRightPanel);
  PatBlt(hdc,
	 rcRightPanel.left,
	 rcRightPanel.top,
	 rcRightPanel.right - rcRightPanel.left,
	 rcRightPanel.bottom - rcRightPanel.top,
	 PATCOPY);

  /* Draw topic bitmap */
  if ((nTopic == -1) && (hDefaultTopicBitmap != 0))
    {
      GetObject(hDefaultTopicBitmap, sizeof(BITMAP), &bmpInfo);
      hOldBitmap = SelectObject (hdcMem, hDefaultTopicBitmap);
      BitBlt(hdc,
	     rcRightPanel.right - bmpInfo.bmWidth,
	     rcRightPanel.bottom - bmpInfo.bmHeight,
	     bmpInfo.bmWidth,
	     bmpInfo.bmHeight,
	     hdcMem,
	     0,
	     0,
	     SRCCOPY);
    }
  else if (hTopicBitmap[nTopic] != 0)
    {
      GetObject(hTopicBitmap[nTopic], sizeof(BITMAP), &bmpInfo);
      hOldBitmap = SelectObject (hdcMem, hTopicBitmap[nTopic]);
      BitBlt(hdc,
	     rcRightPanel.right - bmpInfo.bmWidth,
	     rcRightPanel.bottom - bmpInfo.bmHeight,
	     bmpInfo.bmWidth,
	     bmpInfo.bmHeight,
	     hdcMem,
	     0,
	     0,
	     SRCCOPY);
    }

  if (nTopic == -1)
    {
      nLength = LoadString(hInstance, IDS_DEFAULTTOPICTITLE, szTopicTitle, 80);
    }
  else
    {
      nLength = LoadString(hInstance, IDS_TOPICTITLE0 + nTopic, szTopicTitle, 80);
      if (nLength == 0)
	nLength = LoadString(hInstance, IDS_DEFAULTTOPICTITLE, szTopicTitle, 80);
    }

  if (nTopic == -1)
    {
      nLength = LoadString(hInstance, IDS_DEFAULTTOPICDESC, szTopicDesc, 256);
    }
  else
    {
      nLength = LoadString(hInstance, IDS_TOPICDESC0 + nTopic, szTopicDesc, 256);
      if (nLength == 0)
	nLength = LoadString(hInstance, IDS_DEFAULTTOPICDESC, szTopicDesc, 256);
    }

  SetBkMode(hdc, TRANSPARENT);

  /* Draw topic title */
  rcTitle.left = rcRightPanel.left + 12;
  rcTitle.right = rcRightPanel.right - 8;
  rcTitle.top = rcRightPanel.top + 8;
  rcTitle.bottom = rcTitle.top + 57;
  hOldFont = SelectObject(hdc, hfontTopicTitle);
  DrawText(hdc, szTopicTitle, -1, &rcTitle, DT_TOP | DT_CALCRECT);

  SetTextColor(hdc, DARK_BLUE);
  DrawText(hdc, szTopicTitle, -1, &rcTitle, DT_TOP);

  /* Draw topic description */
  rcDescription.left = rcRightPanel.left + 12;
  rcDescription.right = rcRightPanel.right - 8;
  rcDescription.top = rcTitle.bottom + 8;
  rcDescription.bottom = rcRightPanel.bottom - 20;

  SelectObject(hdc, hfontTopicDescription);
  SetTextColor(hdc, 0x00000000);
  DrawText(hdc, szTopicDesc, -1, &rcDescription, DT_TOP | DT_WORDBREAK);

  SetBkMode(hdc, OPAQUE);
  SelectObject(hdc, hOldFont);

  SelectObject (hdcMem, hOldBrush);
  SelectObject (hdcMem, hOldBitmap);

  EndPaint(hWnd, &ps);
  return 0;
}


static LRESULT
OnDrawItem(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  LPDRAWITEMSTRUCT lpDis = (LPDRAWITEMSTRUCT)lParam;
  HPEN hPen, hOldPen;
  HBRUSH hOldBrush;
  CHAR szText[80];
  int iBkMode;

  if (lpDis->hwndItem == hwndCloseButton)
    {
      DrawFrameControl(lpDis->hDC,
		       &lpDis->rcItem,
		       DFC_BUTTON,
		        DFCS_BUTTONPUSH | DFCS_FLAT);
    }
  else
    {
	if (lpDis->CtlID == (ULONG)nTopic)
		hOldBrush = SelectObject(lpDis->hDC, hbrRightPanel);
	else
		hOldBrush = SelectObject(lpDis->hDC, hbrLightBlue);
	PatBlt(lpDis->hDC,
		   lpDis->rcItem.left,
		   lpDis->rcItem.top,
		   lpDis->rcItem.right,
		   lpDis->rcItem.bottom,
		   PATCOPY);
	SelectObject(lpDis->hDC, hOldBrush);

	hPen = CreatePen(PS_SOLID, 0, DARK_BLUE);
	hOldPen = SelectObject(lpDis->hDC, hPen);
	MoveToEx(lpDis->hDC, lpDis->rcItem.left, lpDis->rcItem.bottom-1, NULL);
	LineTo(lpDis->hDC, lpDis->rcItem.right, lpDis->rcItem.bottom-1);
	SelectObject(lpDis->hDC, hOldPen);
	DeleteObject(hPen);

	InflateRect(&lpDis->rcItem, -10, -4);
	OffsetRect(&lpDis->rcItem, 0, 1);
	GetWindowText(lpDis->hwndItem, szText, 80);
	SetTextColor(lpDis->hDC, 0x00000000);
	iBkMode = SetBkMode(lpDis->hDC, TRANSPARENT);
	DrawText(lpDis->hDC, szText, -1, &lpDis->rcItem, DT_TOP | DT_LEFT | DT_WORDBREAK);
	SetBkMode(lpDis->hDC, iBkMode);
  }

  return 0;
}


static LRESULT
OnMouseMove(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  if (nTopic != -1)
    {
      nTopic = -1;
      SetFocus(hWnd);
      InvalidateRect(hwndMain, NULL, TRUE);
    }

  return 0;
}


static LRESULT
OnCtlColorStatic(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  if ((HWND)lParam == hwndCheckButton)
    {
      SetBkColor((HDC)wParam, LIGHT_BLUE);
      return((LRESULT)hbrLightBlue);
    }

  return 0;
}


static LRESULT
OnActivate(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  nTopic = -1;
  InvalidateRect(hwndMain, NULL, TRUE);

  return(0);
}


static LRESULT
OnDestroy(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  int i;

  for (i=0;i<10;i++)
    {
      if (hwndTopicButton[i] != 0)
	DestroyWindow(hwndTopicButton[i]);
    }

  if (hwndCloseButton != 0)
    DestroyWindow(hwndCloseButton);

  if (hwndCheckButton != 0)
    DestroyWindow(hwndCheckButton);

  DeleteDC(hdcMem);
  DeleteDC(hdcDisplay);

  /* delete bitmaps */
  DeleteObject(hDefaultTopicBitmap);
  DeleteObject(hTitleBitmap);
  for (i=0;i<10;i++)
    {
      if (hTopicBitmap[i] != 0)
	DeleteObject(hTopicBitmap[i]);
    }

  DeleteObject(hfontTopicTitle);
  DeleteObject(hfontTopicDescription);
  DeleteObject(hfontTopicButton);

  DeleteObject(hfontBannerTitle);


  if (hfontCheckButton != 0)
    DeleteObject(hfontCheckButton);

  DeleteObject(hbrLightBlue);
  DeleteObject(hbrDarkBlue);
  DeleteObject(hbrRightPanel);

  return 0;
}


LRESULT CALLBACK
MainWndProc(HWND hWnd,
	    UINT uMsg,
	    WPARAM wParam,
	    LPARAM lParam)
{
  switch(uMsg)
    {
      case WM_CREATE:
	return(OnCreate(hWnd, wParam, lParam));

      case WM_COMMAND:
	return(OnCommand(hWnd, wParam, lParam));

      case WM_ACTIVATE:
	return(OnActivate(hWnd, wParam, lParam));

      case WM_PAINT:
	return(OnPaint(hWnd, wParam, lParam));

      case WM_DRAWITEM:
	return(OnDrawItem(hWnd, wParam, lParam));

      case WM_CTLCOLORSTATIC:
	return(OnCtlColorStatic(hWnd, wParam, lParam));

      case WM_MOUSEMOVE:
	return(OnMouseMove(hWnd, wParam, lParam));

      case WM_DESTROY:
	OnDestroy(hWnd, wParam, lParam);
	PostQuitMessage(0);
	return(0);
    }

  return(DefWindowProc(hWnd, uMsg, wParam, lParam));
}

/* EOF */
