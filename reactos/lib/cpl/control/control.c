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
/* $Id: control.c,v 1.4 2004/06/30 12:10:12 ekohl Exp $
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
	HMODULE hDLL;
	CPLAPPLETFUNC pFunc;
	CPLINFO CPLInfo;
	int nIndex;
} CPLLISTENTRY;


HWND hListView;
HINSTANCE hInst;
HWND hMainWnd;

void dbgprint(TCHAR *format,...)
{
	TCHAR buf[1000];
	va_list va;
	va_start(va,format);
	_vstprintf(buf,format,va);
	OutputDebugString(buf);
	va_end(va);
}

void PopulateCPLList(HWND hLisCtrl)
{
	WIN32_FIND_DATA fd;
	HANDLE hFind;
	TCHAR pszSearchPath[MAX_PATH];
	HIMAGELIST hImgListSmall;
	HIMAGELIST hImgListLarge;
	GetSystemDirectory(pszSearchPath,MAX_PATH);
	_tcscat(pszSearchPath,_T("\\*.cpl"));
	hFind = FindFirstFile(pszSearchPath,&fd);
	hImgListSmall = ImageList_Create(16,16,ILC_COLOR | ILC_MASK,256,1000);
	hImgListLarge = ImageList_Create(32,32,ILC_COLOR | ILC_MASK,256,1000);
	while(hFind != INVALID_HANDLE_VALUE)
	{
		CPLLISTENTRY *pEntry;
		CTL_DEBUG((_T("Found %s\r\n"),fd.cFileName));
		pEntry = (CPLLISTENTRY*)malloc(sizeof(CPLLISTENTRY));
		if(!pEntry)
			break;
		_tcscpy(pEntry->pszPath,pszSearchPath);
		*_tcsrchr(pEntry->pszPath,'\\')=0;
		_tcscat(pEntry->pszPath,_T("\\"));
		_tcscat(pEntry->pszPath,fd.cFileName);

		pEntry->hDLL = LoadLibrary(pEntry->pszPath);
		CTL_DEBUG((_T("Handle %08X\r\n"),pEntry->hDLL));
		pEntry->pFunc = (CPLAPPLETFUNC)GetProcAddress(pEntry->hDLL,"CPlApplet");
		CTL_DEBUG((_T("CPLFunc %08X\r\n"),pEntry->pFunc));
		if(pEntry->pFunc && pEntry->pFunc(hLisCtrl,CPL_INIT,0,0))
		{
			int i;
			for(i=0;i<pEntry->pFunc(hLisCtrl,CPL_GETCOUNT,0,0);i++)
			{
				HICON hIcon;
				TCHAR Name[MAX_PATH];
				int index;
				pEntry->pFunc(hLisCtrl,CPL_INQUIRE,0,(LPARAM)&pEntry->CPLInfo);

				hIcon = LoadImage(pEntry->hDLL,MAKEINTRESOURCE(pEntry->CPLInfo.idIcon),IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
				index = ImageList_AddIcon(hImgListSmall,hIcon);
				DestroyIcon(hIcon);
				hIcon = LoadImage(pEntry->hDLL,MAKEINTRESOURCE(pEntry->CPLInfo.idIcon),IMAGE_ICON,32,32,LR_DEFAULTCOLOR);
				ImageList_AddIcon(hImgListLarge,hIcon);
				DestroyIcon(hIcon);

				LoadString(pEntry->hDLL,pEntry->CPLInfo.idName,Name,MAX_PATH);
				if(_tcslen(Name))
				{
				LV_ITEM lvi;

				memset(&lvi,0x00,sizeof(lvi));
				lvi.mask=LVIF_TEXT|LVIF_PARAM|LVIF_STATE|LVIF_IMAGE;
				lvi.pszText = Name;
				lvi.state=0;
				lvi.iImage=index;
				lvi.lParam = (LPARAM)pEntry;
				pEntry->nIndex = ListView_InsertItem(hLisCtrl,&lvi);

				LoadString(pEntry->hDLL,pEntry->CPLInfo.idInfo,Name,MAX_PATH);
				ListView_SetItemText(hLisCtrl,pEntry->nIndex,1,Name);

				ListView_SetImageList(hLisCtrl,hImgListSmall,LVSIL_SMALL);
				ListView_SetImageList(hLisCtrl,hImgListLarge,LVSIL_NORMAL);
				}
			}
		}

		if(!FindNextFile(hFind,&fd))
			hFind = INVALID_HANDLE_VALUE;
	}
}

LRESULT CALLBACK MyWindowProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_CREATE:
		{
		RECT rect;
		LV_COLUMN column;
		GetClientRect(hWnd,&rect);
		hListView = CreateWindow(WC_LISTVIEW,_T(""),LVS_REPORT | LVS_ALIGNLEFT | LVS_AUTOARRANGE | LVS_SINGLESEL | WS_VISIBLE | WS_CHILD | WS_TABSTOP,0,0,rect.right ,rect.bottom,hWnd,NULL,hInst,0);
		CTL_DEBUG((_T("Listview Window %08X\r\n"),hListView));

		memset(&column,0x00,sizeof(column));
		column.mask=LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM|LVCF_TEXT;
		column.fmt=LVCFMT_LEFT;
		column.cx = (rect.right - rect.left) / 3;
		column.iSubItem = 0;
		column.pszText = _T("Name");
		ListView_InsertColumn(hListView,0,&column);
		column.cx = (rect.right - rect.left) - ((rect.right - rect.left) / 3) - 1;
		column.iSubItem = 1;
		column.pszText = _T("Comment");
		ListView_InsertColumn(hListView,1,&column);
		PopulateCPLList(hListView);
		ListView_SetColumnWidth(hListView,2,LVSCW_AUTOSIZE_USEHEADER);
		ListView_Update(hListView,0);
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
		case NM_DBLCLK:
			{
			int nSelect;
			LV_ITEM lvi;
			CPLLISTENTRY *pEntry;
			nSelect=SendMessage(hListView,LVM_GETNEXTITEM,(WPARAM)-1,LVNI_FOCUSED);
			
			if(nSelect==-1) // no items
			{
				MessageBox(hWnd,_T("No Items in ListView"),_T("Error"),MB_OK|MB_ICONINFORMATION);
				break;
			}
			CTL_DEBUG((_T("Select %d\r\n"),nSelect));
			memset(&lvi,0x00,sizeof(lvi));
			lvi.iItem = nSelect;
			lvi.mask = LVIF_PARAM;
			ListView_GetItem(hListView,&lvi);
			pEntry = (CPLLISTENTRY *)lvi.lParam;
			CTL_DEBUG((_T("Listview DblClk Entry %08X\r\n"),pEntry));
			if(pEntry) {
				CTL_DEBUG((_T("Listview DblClk Entry Func %08X\r\n"),pEntry->pFunc));
			}
			if(pEntry && pEntry->pFunc)
				pEntry->pFunc(hListView,CPL_DBLCLK,pEntry->CPLInfo.lData,0);
			}
		}
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
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
			MessageBox(hWnd,_T("Simple Control Panel (not Shell-namespace based)\rCopyright 2004 GkWare e.K.\rhttp://www.gkware.com\rReleased under the GPL"),_T("About the Control Panel"),MB_OK | MB_ICONINFORMATION);
			break;
		}
		break;
	default:
		return DefWindowProc(hWnd,uMsg,wParam,lParam);
	}
	return 0;
}

#ifdef _MSVC
int WINAPI wWinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,WCHAR *lpCmdLine,int nCmdShow)
#else
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,char *lpCmdLine,int nCmdShow)
#endif
{
	MSG msg;
	WNDCLASS wc;
	hInst = hInstance;
	CTL_DEBUG((_T("My Control Panel\r\n")));
	memset(&wc,0x00,sizeof(wc));
	wc.hIcon = LoadIcon(hInst,MAKEINTRESOURCE(IDI_MAINICON));
	wc.lpszClassName = MYWNDCLASS;
	wc.lpszMenuName = _T("MAINMENU");
	wc.lpfnWndProc = MyWindowProc;
	RegisterClass(&wc);
	InitCommonControls();
	hMainWnd = CreateWindowEx(WS_EX_CLIENTEDGE,MYWNDCLASS,_T("Control Panel"),WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,NULL,LoadMenu(hInst,MAKEINTRESOURCE(IDM_MAINMENU)),hInst,0);
	if(!hMainWnd) {
		CTL_DEBUG((_T("Unable to create window\r\n")));
		return -1;
	}
	ShowWindow(hMainWnd,nCmdShow);
	while(GetMessage(&msg,0,0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	return 0;
}
