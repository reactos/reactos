/*
 *  ReactOS
 *  Copyright (C) 2004 ReactOS Team
 *  Copyright (C) 2004 GkWare e.K.
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
 * PROJECT:         ReactOS System Control Panel
 * FILE:            lib/cpl/system/control.c
 * PURPOSE:         ReactOS System Control Panel
 * PROGRAMMER:      Gero Kuehn (reactos.filter@gkware.com)
 * UPDATE HISTORY:
 *      06-13-2004  Created
 */
#include <windows.h>
#include <commctrl.h>
#include <cpl.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>

#include "resource.h"

//#define CONTROL_DEBUG_ENABLE

#ifdef CONTROL_DEBUG_ENABLE
#define CTL_DEBUG(x) dbgprint x
#else
#define CTL_DEBUG(x)
#endif

#define MYWNDCLASS _T("CTLPANELCLASS")

typedef LONG (CALLBACK *CPLAPPLETFUNC)(HWND hwndCPL, UINT uMsg, LPARAM lParam1, LPARAM lParam2);

typedef struct CPLLISTENTRY
{
	TCHAR pszPath[MAX_PATH];
	HMODULE hDll;
	CPLAPPLETFUNC pFunc;
	CPLINFO CplInfo;
	int nIndex;
} CPLLISTENTRY, *PCPLLISTENTRY;


HWND hListView;
HINSTANCE hInst;
HWND hMainWnd;
DEVMODE pDevMode;

VOID dbgprint(TCHAR *format,...)
{
	TCHAR buf[1000];
	va_list va;

	va_start(va,format);
	_vstprintf(buf,format,va);
	OutputDebugString(buf);
	va_end(va);
}

VOID PopulateCPLList(HWND hLisCtrl)
{
	WIN32_FIND_DATA fd;
	HANDLE hFind;
	TCHAR pszSearchPath[MAX_PATH];
	HIMAGELIST hImgListSmall;
	HIMAGELIST hImgListLarge;
	int ColorDepth;
	HMODULE hDll;
	CPLAPPLETFUNC pFunc;
	TCHAR pszPath[MAX_PATH];

	/* Icon drawing mode */
	pDevMode.dmSize = sizeof(DEVMODE);
	pDevMode.dmDriverExtra = 0;

	EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&pDevMode);
	switch (pDevMode.dmBitsPerPel)
	{
		case 32: ColorDepth = ILC_COLOR32; break;
		case 24: ColorDepth = ILC_COLOR24; break;
		case 16: ColorDepth = ILC_COLOR16; break;
		case  8: ColorDepth = ILC_COLOR8;  break;
		case  4: ColorDepth = ILC_COLOR4;  break;
		default: ColorDepth = ILC_COLOR;  break;
	}

	hImgListSmall = ImageList_Create(16,16,ColorDepth | ILC_MASK,5,5);
	hImgListLarge = ImageList_Create(32,32,ColorDepth | ILC_MASK,5,5);

	GetSystemDirectory(pszSearchPath,MAX_PATH);
	_tcscat(pszSearchPath,_T("\\*.cpl"));

	hFind = FindFirstFile(pszSearchPath,&fd);
	while (hFind != INVALID_HANDLE_VALUE)
	{
		PCPLLISTENTRY pEntry;
		CTL_DEBUG((_T("Found %s\r\n"), fd.cFileName));

		_tcscpy(pszPath, pszSearchPath);
		*_tcsrchr(pszPath, '\\')=0;
		_tcscat(pszPath, _T("\\"));
		_tcscat(pszPath, fd.cFileName);

		hDll = LoadLibrary(pszPath);
		CTL_DEBUG((_T("Handle %08X\r\n"), hDll));

		pFunc = (CPLAPPLETFUNC)GetProcAddress(hDll, "CPlApplet");
		CTL_DEBUG((_T("CPLFunc %08X\r\n"), pFunc));

		if (pFunc && pFunc(hLisCtrl, CPL_INIT, 0, 0))
		{
			UINT i, uPanelCount;

			uPanelCount = (UINT)pFunc(hLisCtrl, CPL_GETCOUNT, 0, 0);
			for (i = 0; i < uPanelCount; i++)
			{
				HICON hIcon;
				TCHAR Name[MAX_PATH];
				int index;

				pEntry = (PCPLLISTENTRY)malloc(sizeof(CPLLISTENTRY));
				if (pEntry == NULL)
					return;

				memset(pEntry, 0, sizeof(CPLLISTENTRY));
				pEntry->hDll = hDll;
				pEntry->pFunc = pFunc;
				_tcscpy(pEntry->pszPath, pszPath);

				pEntry->pFunc(hLisCtrl, CPL_INQUIRE, (LPARAM)i, (LPARAM)&pEntry->CplInfo);
				hIcon = LoadImage(pEntry->hDll,MAKEINTRESOURCE(pEntry->CplInfo.idIcon),IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
				index = ImageList_AddIcon(hImgListSmall,hIcon);
				DestroyIcon(hIcon);
				hIcon = LoadImage(pEntry->hDll,MAKEINTRESOURCE(pEntry->CplInfo.idIcon),IMAGE_ICON,32,32,LR_DEFAULTCOLOR);
				ImageList_AddIcon(hImgListLarge,hIcon);
				DestroyIcon(hIcon);

				if (LoadString(pEntry->hDll, pEntry->CplInfo.idName, Name, MAX_PATH))
				{
					LV_ITEM lvi;

					memset(&lvi,0x00,sizeof(lvi));
					lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
					lvi.pszText = Name;
					lvi.state = 0;
					lvi.iImage = index;
					lvi.lParam = (LPARAM)pEntry;
					pEntry->nIndex = ListView_InsertItem(hLisCtrl,&lvi);

					if (LoadString(pEntry->hDll, pEntry->CplInfo.idInfo, Name, MAX_PATH))
						ListView_SetItemText(hLisCtrl, pEntry->nIndex, 1, Name);
				}
			}
		}

		if (!FindNextFile(hFind,&fd))
			hFind = INVALID_HANDLE_VALUE;
	}

	(void)ListView_SetImageList(hLisCtrl,hImgListSmall,LVSIL_SMALL);
	(void)ListView_SetImageList(hLisCtrl,hImgListLarge,LVSIL_NORMAL);
}

LRESULT CALLBACK MyWindowProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	TCHAR szBuf[1024];

	switch (uMsg)
	{
	case WM_CREATE:
		{
			RECT rect;
			LV_COLUMN column;

			GetClientRect(hWnd,&rect);
			hListView = CreateWindow(WC_LISTVIEW,_T(""),LVS_REPORT | LVS_ALIGNLEFT | LVS_SORTASCENDING | LVS_AUTOARRANGE | LVS_SINGLESEL | WS_VISIBLE | WS_CHILD | WS_TABSTOP,0,0,rect.right ,rect.bottom,hWnd,NULL,hInst,0);
			CTL_DEBUG((_T("Listview Window %08X\r\n"),hListView));

			memset(&column,0x00,sizeof(column));
			column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_TEXT;
			column.fmt = LVCFMT_LEFT;
			column.cx = (rect.right - rect.left) / 3;
			column.iSubItem = 0;
			LoadString(hInst, IDS_NAME, szBuf, sizeof(szBuf) / sizeof(TCHAR));
			column.pszText = szBuf;
			(void)ListView_InsertColumn(hListView,0,&column);
			column.cx = (rect.right - rect.left) - ((rect.right - rect.left) / 3) - 1;
			column.iSubItem = 1;
			LoadString(hInst, IDS_COMMENT, szBuf, sizeof(szBuf) / sizeof(TCHAR));
			column.pszText = szBuf;
			(void)ListView_InsertColumn(hListView,1,&column);
			PopulateCPLList(hListView);
			(void)ListView_SetColumnWidth(hListView,2,LVSCW_AUTOSIZE_USEHEADER);
			(void)ListView_Update(hListView,0);

			SetFocus(hListView);
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_SIZE:
		{
			RECT rect;

			GetClientRect(hWnd,&rect);
			MoveWindow(hListView,0,0,rect.right,rect.bottom,TRUE);
		}
		break;

	case WM_NOTIFY:
		{
			NMHDR *phdr;
			phdr = (NMHDR*)lParam;
			switch(phdr->code)
			{
			case NM_RETURN:
			case NM_DBLCLK:
				{
					int nSelect;
					LV_ITEM lvi;
					PCPLLISTENTRY pEntry;

					nSelect=SendMessage(hListView,LVM_GETNEXTITEM,(WPARAM)-1,LVNI_FOCUSED);

					if (nSelect==-1)
					{
						/* no items */
						LoadString(hInst, IDS_NO_ITEMS, szBuf, sizeof(szBuf) / sizeof(TCHAR));
						MessageBox(hWnd,(LPCTSTR)szBuf,NULL,MB_OK|MB_ICONINFORMATION);
						break;
					}

					CTL_DEBUG((_T("Select %d\r\n"),nSelect));
					memset(&lvi,0x00,sizeof(lvi));
					lvi.iItem = nSelect;
					lvi.mask = LVIF_PARAM;
					(void)ListView_GetItem(hListView,&lvi);
					pEntry = (PCPLLISTENTRY)lvi.lParam;
					CTL_DEBUG((_T("Listview DblClk Entry %08X\r\n"),pEntry));
					if (pEntry)
					{
						CTL_DEBUG((_T("Listview DblClk Entry Func %08X\r\n"),pEntry->pFunc));
					}

					if (pEntry && pEntry->pFunc)
						pEntry->pFunc(hListView,CPL_DBLCLK,pEntry->CplInfo.lData,0);
				}
			}
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDM_LARGEICONS:
			SetWindowLong(hListView,GWL_STYLE,LVS_ICON | LVS_ALIGNLEFT | LVS_AUTOARRANGE | LVS_SINGLESEL   | WS_VISIBLE | WS_CHILD|WS_BORDER|WS_TABSTOP);
			break;
		case IDM_SMALLICONS:
			SetWindowLong(hListView,GWL_STYLE,LVS_SMALLICON | LVS_ALIGNLEFT | LVS_AUTOARRANGE | LVS_SINGLESEL   | WS_VISIBLE | WS_CHILD|WS_BORDER|WS_TABSTOP);
			break;
		case IDM_LIST:
			SetWindowLong(hListView,GWL_STYLE,LVS_LIST | LVS_ALIGNLEFT | LVS_AUTOARRANGE | LVS_SINGLESEL   | WS_VISIBLE | WS_CHILD|WS_BORDER|WS_TABSTOP);
			break;
		case IDM_DETAILS:
			SetWindowLong(hListView,GWL_STYLE,LVS_REPORT | LVS_ALIGNLEFT | LVS_AUTOARRANGE | LVS_SINGLESEL   | WS_VISIBLE | WS_CHILD|WS_BORDER|WS_TABSTOP);
			break;
		case IDM_CLOSE:
			DestroyWindow(hWnd);
			break;
		case IDM_ABOUT:
			{
				TCHAR Title[256];
				
				LoadString(hInst, IDS_ABOUT, szBuf, sizeof(szBuf) / sizeof(TCHAR));
				LoadString(hInst, IDS_ABOUT_TITLE, Title, sizeof(Title) / sizeof(TCHAR));
				
				MessageBox(hWnd,(LPCTSTR)szBuf,(LPCTSTR)Title,MB_OK | MB_ICONINFORMATION);
			}
			break;
		}
		break;

	default:
		return DefWindowProc(hWnd,uMsg,wParam,lParam);
	}

	return 0;
}


static INT
RunControlPanelWindow(int nCmdShow)
{
  MSG msg;
  WNDCLASS wc;
  TCHAR szBuf[256];

  memset(&wc,0x00,sizeof(wc));
  wc.hIcon = LoadIcon(hInst,MAKEINTRESOURCE(IDI_MAINICON));
  wc.lpszClassName = MYWNDCLASS;
  wc.lpszMenuName = _T("MAINMENU");
  wc.lpfnWndProc = MyWindowProc;
  RegisterClass(&wc);

  InitCommonControls();
  
  LoadString(hInst, IDS_WINDOW_TITLE, szBuf, sizeof(szBuf) / sizeof(TCHAR));
  hMainWnd = CreateWindowEx(WS_EX_CLIENTEDGE,
			    MYWNDCLASS,
			    (LPCTSTR)szBuf,
			    WS_OVERLAPPEDWINDOW,
			    CW_USEDEFAULT,
			    CW_USEDEFAULT,
			    CW_USEDEFAULT,
			    CW_USEDEFAULT,
			    NULL,
			    LoadMenu(hInst, MAKEINTRESOURCE(IDM_MAINMENU)),
			    hInst,
			    0);
  if (!hMainWnd)
    {
      CTL_DEBUG((_T("Unable to create window\r\n")));
      return -1;
    }

  ShowWindow(hMainWnd, nCmdShow);
  while (GetMessage(&msg, 0, 0, 0))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

  return 0;
}


static INT
RunControlPanel(LPCTSTR lpName, UINT uIndex)
{
  CPLINFO CplInfo;
  HMODULE hDll;
  CPLAPPLETFUNC pFunc;
  UINT uPanelCount;

  hDll = LoadLibrary(lpName);
  if (hDll == 0)
    {
      return -1;
    }
  CTL_DEBUG((_T("Handle %08X\r\n"), hDll));

  pFunc = (CPLAPPLETFUNC)GetProcAddress(hDll, "CPlApplet");
  if (pFunc == NULL)
    {
      FreeLibrary(hDll);
      return -1;
    }
  CTL_DEBUG((_T("CPLFunc %08X\r\n"), pFunc));

  if (!pFunc(NULL, CPL_INIT, 0, 0))
    {
      FreeLibrary(hDll);
      return -1;
    }

  uPanelCount = (UINT)pFunc(NULL, CPL_GETCOUNT, 0, 0);
  if (uIndex >= uPanelCount)
    {
      FreeLibrary(hDll);
      return -1;
    }

  pFunc(NULL, CPL_INQUIRE, (LPARAM)uIndex, (LPARAM)&CplInfo);

  pFunc(NULL, CPL_DBLCLK, CplInfo.lData, 0);

  FreeLibrary(hDll);

  return 0;
}

int
_tmain(int argc, const TCHAR *argv[])
{
  STARTUPINFO si;

  si.cb = sizeof(si);
  GetStartupInfo(&si);

  hInst = GetModuleHandle(NULL);

  if (argc <= 1)
    {
      /* No argument on the command line */
      return RunControlPanelWindow(si.wShowWindow);
    }

  if (_tcsicmp(argv[1], _T("desktop")) == 0)
    {
      return RunControlPanel(_T("desk.cpl"), 0);
    }
  else if (_tcsicmp(argv[1], _T("date/time")) == 0)
    {
      return RunControlPanel(_T("timedate.cpl"), 0);
    }
  else if (_tcsicmp(argv[1], _T("international")) == 0)
    {
      return RunControlPanel(_T("intl.cpl"), 0);
    }
  else if (_tcsicmp(argv[1], _T("mouse")) == 0)
    {
      return RunControlPanel(_T("main.cpl"), 0);
    }
  else if (_tcsicmp(argv[1], _T("keyboard")) == 0)
    {
      return RunControlPanel(_T("main.cpl"), 1);
    }

  return 0;
}
