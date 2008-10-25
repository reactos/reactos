/*
 *  ReactOS applications
 *  Copyright (C) 2001, 2002, 2003 ReactOS Team
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
/* $Id$
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS welcome/autorun application
 * FILE:        subsys/system/welcome/welcome.c
 * PROGRAMMERS: Eric Kohl
 *              Casper S. Hornstrup (chorns@users.sourceforge.net)
 *
 * NOTE:
 *   This utility can be customized by modifying the resources.
 *   Please do NOT change the source code in order to customize this
 *   utility but change the resources!
 */

#include "../../../include/reactos/version.h"
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <tchar.h>

#include "resource.h"



#define LIGHT_BLUE 0x00F7EFD6
#define DARK_BLUE  0x008C7B6B

#define TITLE_WIDTH  480
#define TITLE_HEIGHT  93


/* GLOBALS ******************************************************************/

TCHAR szFrameClass [] = TEXT("WelcomeWindowClass");
TCHAR szAppTitle [80];

HINSTANCE hInstance;

HWND hwndMain = 0;
HWND hwndDefaultTopic = 0;

HDC hdcMem = 0;

int nTopic = -1;
int nDefaultTopic = -1;

ULONG ulInnerWidth = TITLE_WIDTH;
ULONG ulInnerHeight = (TITLE_WIDTH * 3) / 4;
ULONG ulTitleHeight = TITLE_HEIGHT + 3;

HBITMAP hTitleBitmap = 0;
HBITMAP hDefaultTopicBitmap = 0;
HBITMAP hTopicBitmap[10];
HWND hwndTopicButton[10];
HWND hwndCloseButton;
HWND hwndCheckButton;

HFONT hfontTopicButton;
HFONT hfontTopicTitle;
HFONT hfontTopicDescription;
HFONT hfontCheckButton;

HBRUSH hbrLightBlue;
HBRUSH hbrDarkBlue;
HBRUSH hbrRightPanel;

RECT rcTitlePanel;
RECT rcLeftPanel;
RECT rcRightPanel;

WNDPROC fnOldBtn;


INT_PTR CALLBACK
MainWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


/* FUNCTIONS ****************************************************************/

int WINAPI
_tWinMain(HINSTANCE hInst,
	HINSTANCE hPrevInstance,
	LPTSTR lpszCmdLine,
	int nCmdShow)
{
  WNDCLASSEX wndclass;
  MSG msg;
  int xPos;
  int yPos;
  int xWidth;
  int yHeight;
  RECT rcWindow;
  HICON hMainIcon;
  HMENU hSystemMenu;
  DWORD dwStyle = WS_OVERLAPPED | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
                  WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
  BITMAP BitmapInfo;

  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpszCmdLine);

  hInstance = hInst;

  /* Load icons */
  hMainIcon = LoadIcon (hInstance, MAKEINTRESOURCE(IDI_MAIN));

  /* Register the window class */
  wndclass.style = CS_HREDRAW | CS_VREDRAW;
  wndclass.lpfnWndProc = (WNDPROC)MainWndProc;
  wndclass.cbClsExtra = 0;
  wndclass.cbWndExtra = 0;
  wndclass.hInstance = hInstance;
  wndclass.hIcon = hMainIcon;
  wndclass.hCursor = LoadCursor (NULL, IDC_ARROW);
  wndclass.hbrBackground = 0;
  wndclass.lpszMenuName = NULL;
  wndclass.lpszClassName = szFrameClass;

  wndclass.cbSize = sizeof(WNDCLASSEX);
  wndclass.hIconSm = 0;

  RegisterClassEx(&wndclass);

  hTitleBitmap = LoadBitmap (hInstance, MAKEINTRESOURCE(IDB_TITLEBITMAP));
  if (hTitleBitmap != NULL)
    {
      GetObject(hTitleBitmap, sizeof(BITMAP), &BitmapInfo);
      ulInnerWidth = BitmapInfo.bmWidth;
      ulInnerHeight = (ulInnerWidth * 3) / 4;
      ulTitleHeight = BitmapInfo.bmHeight + 3;
      DeleteObject(hTitleBitmap);
    }
  ulInnerHeight -= GetSystemMetrics(SM_CYCAPTION);

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
  rcTitlePanel.bottom = ulTitleHeight;
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

  hSystemMenu = GetSystemMenu(hwndMain, FALSE);
  if(hSystemMenu)
  {
    RemoveMenu(hSystemMenu, SC_SIZE, MF_BYCOMMAND);
    RemoveMenu(hSystemMenu, SC_MAXIMIZE, MF_BYCOMMAND);
  }

  ShowWindow(hwndMain, nCmdShow);
  UpdateWindow(hwndMain);

  while (GetMessage(&msg, NULL, 0, 0) != FALSE)
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

  return(msg.wParam);
}


INT_PTR CALLBACK
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
	  InvalidateRect(hwndMain, &rcRightPanel, TRUE);
	}
    }

  return(CallWindowProc(fnOldBtn, hWnd, uMsg, wParam, lParam));
}


static BOOL
RunApplication(int nTopic)
{
  PROCESS_INFORMATION ProcessInfo;
  STARTUPINFO StartupInfo;
  TCHAR AppName[256];
  TCHAR CurrentDir[256];
  int nLength;

  InvalidateRect(hwndMain, NULL, TRUE);

  GetCurrentDirectory(256, CurrentDir);

  nLength = LoadString(hInstance, IDS_TOPICACTION0 + nTopic, AppName, 256);
  if (nLength == 0)
    return TRUE;

  if (!_tcsicmp(AppName, TEXT("<exit>")))
    return FALSE;

  if (_tcsicmp(AppName, TEXT("explorer.exe")) == 0)
    {
      _tcscat(AppName, TEXT(" "));
      _tcscat(AppName, CurrentDir);
    }

  memset(&StartupInfo, 0, sizeof(STARTUPINFO));
  StartupInfo.cb = sizeof(STARTUPINFO);
  StartupInfo.lpTitle = TEXT("Test");
  StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
  StartupInfo.wShowWindow = SW_SHOWNORMAL;

  CreateProcess(NULL, AppName, NULL, NULL, FALSE, CREATE_NEW_CONSOLE,NULL,
		CurrentDir,
		&StartupInfo,
		&ProcessInfo);

  CloseHandle(ProcessInfo.hProcess);
  CloseHandle(ProcessInfo.hThread);

  return TRUE;
}


static VOID
SubclassButton(HWND hWnd)
{
  fnOldBtn = (WNDPROC)SetWindowLongPtr(hWnd, GWL_WNDPROC, (DWORD_PTR)ButtonSubclassWndProc);
}


static DWORD
GetButtonHeight(HDC hDC,
		HFONT hFont,
		LPCTSTR szText,
		DWORD dwWidth)
{
  HFONT hOldFont;
  RECT rect;

  rect.left = 0;
  rect.right = dwWidth - 20;
  rect.top = 0;
  rect.bottom = 25;

  hOldFont = (HFONT) SelectObject(hDC, hFont);
  DrawText(hDC, szText, -1, &rect, DT_TOP | DT_CALCRECT | DT_WORDBREAK);
  SelectObject(hDC, hOldFont);

  return(rect.bottom-rect.top + 14);
}


static LRESULT
OnCreate(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  TCHAR szText[80];
  int i,nLength;
  HDC ScreenDC;
  DWORD dwTop;
  DWORD dwHeight = 0;

  UNREFERENCED_PARAMETER(wParam);
  UNREFERENCED_PARAMETER(lParam);

  hbrLightBlue = CreateSolidBrush(LIGHT_BLUE);
  hbrDarkBlue = CreateSolidBrush(DARK_BLUE);
  hbrRightPanel = CreateSolidBrush(0x00FFFFFF);

  /* Topic title font */
  hfontTopicTitle = CreateFont(-18,0,0,0,FW_NORMAL,
			       FALSE,FALSE,FALSE,ANSI_CHARSET,
			       OUT_DEFAULT_PRECIS,
			       CLIP_DEFAULT_PRECIS,
			       DEFAULT_QUALITY,
			       FF_DONTCARE,
			       TEXT("Arial"));

  /* Topic description font */
  hfontTopicDescription = CreateFont(-11,0,0,0,FW_THIN,
				     FALSE,FALSE,FALSE,ANSI_CHARSET,
				     OUT_DEFAULT_PRECIS,
				     CLIP_DEFAULT_PRECIS,
				     DEFAULT_QUALITY,
				     FF_DONTCARE,
				     TEXT("Arial"));

  /* Topic button font */
  hfontTopicButton = CreateFont(-11,0,0,0,FW_BOLD,
				FALSE,FALSE,FALSE,ANSI_CHARSET,
				OUT_DEFAULT_PRECIS,
				CLIP_DEFAULT_PRECIS,
				DEFAULT_QUALITY,
				FF_DONTCARE,
				TEXT("Arial"));

  /* Load title bitmap */
  if (hTitleBitmap != 0)
    hTitleBitmap = LoadBitmap (hInstance, MAKEINTRESOURCE(IDB_TITLEBITMAP));

  /* Load topic bitmaps */
  hDefaultTopicBitmap = LoadBitmap (hInstance, MAKEINTRESOURCE(IDB_DEFAULTTOPICBITMAP));
  for (i=0;i < 10; i++)
    {
      hTopicBitmap[i] = LoadBitmap (hInstance, MAKEINTRESOURCE(IDB_TOPICBITMAP0+i));
    }

  ScreenDC = GetWindowDC(hWnd);
  hdcMem = CreateCompatibleDC (ScreenDC);
  ReleaseDC(hWnd, ScreenDC);

  /* load and create buttons */
  dwTop = rcLeftPanel.top;
  for (i = 0; i < 10; i++)
    {
      nLength = LoadString(hInstance, IDS_TOPICBUTTON0 + i, szText, 80);
      if (nLength > 0)
	{
	  dwHeight = GetButtonHeight(hdcMem,
				     hfontTopicButton,
				     szText,
				     rcLeftPanel.right - rcLeftPanel.left);

	  hwndTopicButton[i] = CreateWindow(TEXT("BUTTON"),
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
      hwndCloseButton = CreateWindow(TEXT("BUTTON"),
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
				    TEXT("Tahoma"));

      hwndCheckButton = CreateWindow(TEXT("BUTTON"),
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
  UNREFERENCED_PARAMETER(lParam);

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
  HBITMAP hOldBitmap;
  HBRUSH hOldBrush;

  /* Title bitmap */
  hOldBitmap = (HBITMAP) SelectObject(hdcMem, hTitleBitmap);
  BitBlt(hdc,
	 rcPanel->left,
	 rcPanel->top,
	 rcPanel->right - rcPanel->left,
	 rcPanel->bottom - 3,
	 hdcMem, 0, 0, SRCCOPY);
  SelectObject(hdcMem, hOldBitmap);

  /* Dark blue line */
  hOldBrush = (HBRUSH) SelectObject(hdc, hbrDarkBlue);
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
  HPEN hPen;
  HPEN hOldPen;
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
  TCHAR version[50];

  UNREFERENCED_PARAMETER(wParam);
  UNREFERENCED_PARAMETER(lParam);

  hdc = BeginPaint(hWnd, &ps);

  /* Banner panel */
  PaintBanner(hdc, &rcTitlePanel);

  /* Left panel */
  hOldBrush = (HBRUSH) SelectObject (hdc, hbrLightBlue);
  PatBlt(hdc,
	 rcLeftPanel.left,
	 rcLeftPanel.top,
	 rcLeftPanel.right - rcLeftPanel.left,
	 rcLeftPanel.bottom - rcLeftPanel.top,
	 PATCOPY);
  SelectObject(hdc, hOldBrush);

  /* Right panel */
  hOldBrush = (HBRUSH) SelectObject (hdc, WHITE_BRUSH);
  PatBlt(hdc,
	 rcRightPanel.left,
	 rcRightPanel.top,
	 rcRightPanel.right - rcRightPanel.left,
	 rcRightPanel.bottom - rcRightPanel.top,
	 PATCOPY);
  SelectObject(hdc, hOldBrush);

  /* Draw dark verical line */
  hPen = CreatePen(PS_SOLID, 0, DARK_BLUE);
  hOldPen = (HPEN) SelectObject(hdc, hPen);
  MoveToEx(hdc, rcRightPanel.left, rcRightPanel.top, NULL);
  LineTo(hdc, rcRightPanel.left, rcRightPanel.bottom);
  SelectObject(hdc, hOldPen);
  DeleteObject(hPen);

  /* Draw topic bitmap */
  if ((nTopic == -1) && (hDefaultTopicBitmap != 0))
    {
      GetObject(hDefaultTopicBitmap, sizeof(BITMAP), &bmpInfo);
      hOldBitmap = (HBITMAP) SelectObject (hdcMem, hDefaultTopicBitmap);
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
  else if ((nTopic != -1) && (hTopicBitmap[nTopic] != 0))
    {
      GetObject(hTopicBitmap[nTopic], sizeof(BITMAP), &bmpInfo);
      hOldBitmap = (HBITMAP) SelectObject (hdcMem, hTopicBitmap[nTopic]);
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

  /* Draw version information */
  wsprintf(version, TEXT("ReactOS %d.%d.%d"),
    KERNEL_VERSION_MAJOR,
    KERNEL_VERSION_MINOR,
    KERNEL_VERSION_PATCH_LEVEL);

  rcTitle.left = rcLeftPanel.left + 8;
  rcTitle.right = rcLeftPanel.right - 5;
  rcTitle.top = rcLeftPanel.bottom - 40;
  rcTitle.bottom = rcLeftPanel.bottom - 5;
  hOldFont = (HFONT) SelectObject(hdc, hfontTopicDescription);
  DrawText(hdc, version, -1, &rcTitle, DT_BOTTOM | DT_CALCRECT | DT_SINGLELINE);
  DrawText(hdc, version, -1, &rcTitle, DT_BOTTOM | DT_SINGLELINE);
  SelectObject(hdc, hOldFont);

  /* Draw topic title */
  rcTitle.left = rcRightPanel.left + 12;
  rcTitle.right = rcRightPanel.right - 8;
  rcTitle.top = rcRightPanel.top + 8;
  rcTitle.bottom = rcTitle.top + 57;
  hOldFont = (HFONT) SelectObject(hdc, hfontTopicTitle);
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
  TCHAR szText[80];
  int iBkMode;

  UNREFERENCED_PARAMETER(hWnd);
  UNREFERENCED_PARAMETER(wParam);

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
	hOldBrush = (HBRUSH) SelectObject(lpDis->hDC, hbrRightPanel);
      else
	hOldBrush = (HBRUSH) SelectObject(lpDis->hDC, hbrLightBlue);

      PatBlt(lpDis->hDC,
	     lpDis->rcItem.left,
	     lpDis->rcItem.top,
	     lpDis->rcItem.right,
	     lpDis->rcItem.bottom,
	     PATCOPY);
      SelectObject(lpDis->hDC, hOldBrush);

      hPen = CreatePen(PS_SOLID, 0, DARK_BLUE);
      hOldPen = (HPEN) SelectObject(lpDis->hDC, hPen);
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
  UNREFERENCED_PARAMETER(wParam);
  UNREFERENCED_PARAMETER(lParam);

  if (nTopic != -1)
    {
      nTopic = -1;
      SetFocus(hWnd);
      InvalidateRect(hwndMain, &rcRightPanel, TRUE);
    }

  return 0;
}


static LRESULT
OnCtlColorStatic(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(hWnd);

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
  UNREFERENCED_PARAMETER(hWnd);
  UNREFERENCED_PARAMETER(wParam);
  UNREFERENCED_PARAMETER(lParam);
  nTopic = -1;
  InvalidateRect(hwndMain, &rcRightPanel, TRUE);

  return(0);
}


static LRESULT
OnDestroy(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  int i;

  UNREFERENCED_PARAMETER(hWnd);
  UNREFERENCED_PARAMETER(wParam);
  UNREFERENCED_PARAMETER(lParam);

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

  if (hfontCheckButton != 0)
    DeleteObject(hfontCheckButton);

  DeleteObject(hbrLightBlue);
  DeleteObject(hbrDarkBlue);
  DeleteObject(hbrRightPanel);

  return 0;
}


INT_PTR CALLBACK
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
