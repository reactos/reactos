/*
 *  ReactOS Task Manager
 *
 *  affinity.cpp
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
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
	
#ifdef _MSC_VER
#include "stdafx.h"
#else
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
#endif
	
#include "taskmgr.h"
#include "ProcessPage.h"
#include "affinity.h"
#include "perfdata.h"

HANDLE		hProcessAffinityHandle;

LRESULT CALLBACK AffinityDialogWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

void ProcessPage_OnSetAffinity(void)
{
	LV_ITEM			lvitem;
	ULONG			Index;
	DWORD			dwProcessId;
	TCHAR			strErrorText[260];

	for (Index=0; Index<(ULONG)ListView_GetItemCount(hProcessPageListCtrl); Index++)
	{
		memset(&lvitem, 0, sizeof(LV_ITEM));

		lvitem.mask = LVIF_STATE;
		lvitem.stateMask = LVIS_SELECTED;
		lvitem.iItem = Index;

		ListView_GetItem(hProcessPageListCtrl, &lvitem);

		if (lvitem.state & LVIS_SELECTED)
			break;
	}

	dwProcessId = PerfDataGetProcessId(Index);

	if ((ListView_GetSelectedCount(hProcessPageListCtrl) != 1) || (dwProcessId == 0))
		return;

	hProcessAffinityHandle = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_SET_INFORMATION, FALSE, dwProcessId);

	if (!hProcessAffinityHandle)
	{
		GetLastErrorText(strErrorText, 260);
		MessageBox(hMainWnd, strErrorText, "Unable to Access or Set Process Affinity", MB_OK|MB_ICONSTOP);
		return;
	}

	DialogBox(hInst, MAKEINTRESOURCE(IDD_AFFINITY_DIALOG), hMainWnd, (DLGPROC)AffinityDialogWndProc);

	if (hProcessAffinityHandle)
	{
		CloseHandle(hProcessAffinityHandle);
		hProcessAffinityHandle = NULL;
	}
}

LRESULT CALLBACK AffinityDialogWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DWORD	dwProcessAffinityMask = 0;
	DWORD	dwSystemAffinityMask = 0;
	TCHAR	strErrorText[260];

	switch (message)
	{
	case WM_INITDIALOG:

		//
		// Get the current affinity mask for the process and
		// the number of CPUs present in the system
		//
		if (!GetProcessAffinityMask(hProcessAffinityHandle, &dwProcessAffinityMask, &dwSystemAffinityMask))
		{
			GetLastErrorText(strErrorText, 260);
			EndDialog(hDlg, 0);
			MessageBox(hMainWnd, strErrorText, "Unable to Access or Set Process Affinity", MB_OK|MB_ICONSTOP);
		}

		//
		// Enable a checkbox for each processor present in the system
		//
		if (dwSystemAffinityMask & 0x00000001)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU0), TRUE);
		if (dwSystemAffinityMask & 0x00000002)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU1), TRUE);
		if (dwSystemAffinityMask & 0x00000004)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU2), TRUE);
		if (dwSystemAffinityMask & 0x00000008)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU3), TRUE);
		if (dwSystemAffinityMask & 0x00000010)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU4), TRUE);
		if (dwSystemAffinityMask & 0x00000020)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU5), TRUE);
		if (dwSystemAffinityMask & 0x00000040)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU6), TRUE);
		if (dwSystemAffinityMask & 0x00000080)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU7), TRUE);
		if (dwSystemAffinityMask & 0x00000100)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU8), TRUE);
		if (dwSystemAffinityMask & 0x00000200)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU9), TRUE);
		if (dwSystemAffinityMask & 0x00000400)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU10), TRUE);
		if (dwSystemAffinityMask & 0x00000800)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU11), TRUE);
		if (dwSystemAffinityMask & 0x00001000)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU12), TRUE);
		if (dwSystemAffinityMask & 0x00002000)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU13), TRUE);
		if (dwSystemAffinityMask & 0x00004000)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU14), TRUE);
		if (dwSystemAffinityMask & 0x00008000)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU15), TRUE);
		if (dwSystemAffinityMask & 0x00010000)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU16), TRUE);
		if (dwSystemAffinityMask & 0x00020000)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU17), TRUE);
		if (dwSystemAffinityMask & 0x00040000)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU18), TRUE);
		if (dwSystemAffinityMask & 0x00080000)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU19), TRUE);
		if (dwSystemAffinityMask & 0x00100000)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU20), TRUE);
		if (dwSystemAffinityMask & 0x00200000)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU21), TRUE);
		if (dwSystemAffinityMask & 0x00400000)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU22), TRUE);
		if (dwSystemAffinityMask & 0x00800000)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU23), TRUE);
		if (dwSystemAffinityMask & 0x01000000)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU24), TRUE);
		if (dwSystemAffinityMask & 0x02000000)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU25), TRUE);
		if (dwSystemAffinityMask & 0x04000000)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU26), TRUE);
		if (dwSystemAffinityMask & 0x08000000)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU27), TRUE);
		if (dwSystemAffinityMask & 0x10000000)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU28), TRUE);
		if (dwSystemAffinityMask & 0x20000000)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU29), TRUE);
		if (dwSystemAffinityMask & 0x40000000)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU30), TRUE);
		if (dwSystemAffinityMask & 0x80000000)
			EnableWindow(GetDlgItem(hDlg, IDC_CPU31), TRUE);


		//
		// Check each checkbox that the current process
		// has affinity with
		//
		if (dwProcessAffinityMask & 0x00000001)
			SendMessage(GetDlgItem(hDlg, IDC_CPU0), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x00000002)
			SendMessage(GetDlgItem(hDlg, IDC_CPU1), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x00000004)
			SendMessage(GetDlgItem(hDlg, IDC_CPU2), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x00000008)
			SendMessage(GetDlgItem(hDlg, IDC_CPU3), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x00000010)
			SendMessage(GetDlgItem(hDlg, IDC_CPU4), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x00000020)
			SendMessage(GetDlgItem(hDlg, IDC_CPU5), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x00000040)
			SendMessage(GetDlgItem(hDlg, IDC_CPU6), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x00000080)
			SendMessage(GetDlgItem(hDlg, IDC_CPU7), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x00000100)
			SendMessage(GetDlgItem(hDlg, IDC_CPU8), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x00000200)
			SendMessage(GetDlgItem(hDlg, IDC_CPU9), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x00000400)
			SendMessage(GetDlgItem(hDlg, IDC_CPU10), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x00000800)
			SendMessage(GetDlgItem(hDlg, IDC_CPU11), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x00001000)
			SendMessage(GetDlgItem(hDlg, IDC_CPU12), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x00002000)
			SendMessage(GetDlgItem(hDlg, IDC_CPU13), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x00004000)
			SendMessage(GetDlgItem(hDlg, IDC_CPU14), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x00008000)
			SendMessage(GetDlgItem(hDlg, IDC_CPU15), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x00010000)
			SendMessage(GetDlgItem(hDlg, IDC_CPU16), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x00020000)
			SendMessage(GetDlgItem(hDlg, IDC_CPU17), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x00040000)
			SendMessage(GetDlgItem(hDlg, IDC_CPU18), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x00080000)
			SendMessage(GetDlgItem(hDlg, IDC_CPU19), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x00100000)
			SendMessage(GetDlgItem(hDlg, IDC_CPU20), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x00200000)
			SendMessage(GetDlgItem(hDlg, IDC_CPU21), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x00400000)
			SendMessage(GetDlgItem(hDlg, IDC_CPU22), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x00800000)
			SendMessage(GetDlgItem(hDlg, IDC_CPU23), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x01000000)
			SendMessage(GetDlgItem(hDlg, IDC_CPU24), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x02000000)
			SendMessage(GetDlgItem(hDlg, IDC_CPU25), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x04000000)
			SendMessage(GetDlgItem(hDlg, IDC_CPU26), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x08000000)
			SendMessage(GetDlgItem(hDlg, IDC_CPU27), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x10000000)
			SendMessage(GetDlgItem(hDlg, IDC_CPU28), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x20000000)
			SendMessage(GetDlgItem(hDlg, IDC_CPU29), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x40000000)
			SendMessage(GetDlgItem(hDlg, IDC_CPU30), BM_SETCHECK, BST_CHECKED, 0);
		if (dwProcessAffinityMask & 0x80000000)
			SendMessage(GetDlgItem(hDlg, IDC_CPU31), BM_SETCHECK, BST_CHECKED, 0);

		return TRUE;

	case WM_COMMAND:

		//
		// If the user has cancelled the dialog box
		// then just close it
		//
		if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}

		//
		// The user has clicked OK -- so now we have
		// to adjust the process affinity mask
		//
		if (LOWORD(wParam) == IDOK)
		{
			//
			// First we have to create a mask out of each
			// checkbox that the user checked.
			//
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU0), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x00000001;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU1), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x00000002;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU2), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x00000004;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU3), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x00000008;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU4), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x00000010;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU5), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x00000020;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU6), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x00000040;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU7), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x00000080;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU8), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x00000100;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU9), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x00000200;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU10), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x00000400;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU11), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x00000800;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU12), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x00001000;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU13), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x00002000;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU14), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x00004000;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU15), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x00008000;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU16), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x00010000;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU17), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x00020000;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU18), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x00040000;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU19), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x00080000;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU20), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x00100000;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU21), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x00200000;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU22), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x00400000;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU23), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x00800000;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU24), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x01000000;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU25), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x02000000;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU26), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x04000000;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU27), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x08000000;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU28), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x10000000;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU29), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x20000000;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU30), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x40000000;
			if (SendMessage(GetDlgItem(hDlg, IDC_CPU31), BM_GETCHECK, 0, 0))
				dwProcessAffinityMask |= 0x80000000;

			//
			// Make sure they are giving the process affinity
			// with at least one processor. I'd hate to see a
			// process that is not in a wait state get deprived
			// of it's cpu time.
			//
			if (!dwProcessAffinityMask)
			{
				MessageBox(hDlg, "The process must have affinity with at least one processor.", "Invalid Option", MB_OK|MB_ICONSTOP);
				return TRUE;
			}

			//
			// Try to set the process affinity
			//
			if (!SetProcessAffinityMask(hProcessAffinityHandle, dwProcessAffinityMask))
			{
				GetLastErrorText(strErrorText, 260);
				EndDialog(hDlg, LOWORD(wParam));
				MessageBox(hMainWnd, strErrorText, "Unable to Access or Set Process Affinity", MB_OK|MB_ICONSTOP);
			}

			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}

		break;
	}

    return 0;
}
