/*
 *  ReactOS Task Manager
 *
 *  endproc.cpp
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
#include "endproc.h"
#include "ProcessPage.h"
#include "perfdata.h"

void ProcessPage_OnEndProcess(void)
{
	LVITEM			lvitem;
	ULONG			Index;
	DWORD			dwProcessId;
	HANDLE			hProcess;
	TCHAR			strErrorText[260];

	for (Index=0; Index<(ULONG)ListView_GetItemCount(hProcessPageListCtrl); Index++)
	{
		memset(&lvitem, 0, sizeof(LVITEM));

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

	if (MessageBox(hMainWnd, "WARNING: Terminating a process can cause undesired\nresults including loss of data and system instability. The\nprocess will not be given the chance to save its state or\ndata before it is terminated. Are you sure you want to\nterminate the process?", "Task Manager Warning", MB_YESNO|MB_ICONWARNING) != IDYES)
		return;

	hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, dwProcessId);

	if (!hProcess)
	{
		GetLastErrorText(strErrorText, 260);
		MessageBox(hMainWnd, strErrorText, "Unable to Terminate Process", MB_OK|MB_ICONSTOP);
		return;
	}

	if (!TerminateProcess(hProcess, 0))
	{
		GetLastErrorText(strErrorText, 260);
		MessageBox(hMainWnd, strErrorText, "Unable to Terminate Process", MB_OK|MB_ICONSTOP);
	}

	CloseHandle(hProcess);
}

void ProcessPage_OnEndProcessTree(void)
{
	LVITEM			lvitem;
	ULONG			Index;
	DWORD			dwProcessId;
	HANDLE			hProcess;
	TCHAR			strErrorText[260];

	for (Index=0; Index<(ULONG)ListView_GetItemCount(hProcessPageListCtrl); Index++)
	{
		memset(&lvitem, 0, sizeof(LVITEM));

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

	if (MessageBox(hMainWnd, "WARNING: Terminating a process can cause undesired\nresults including loss of data and system instability. The\nprocess will not be given the chance to save its state or\ndata before it is terminated. Are you sure you want to\nterminate the process?", "Task Manager Warning", MB_YESNO|MB_ICONWARNING) != IDYES)
		return;

	hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, dwProcessId);

	if (!hProcess)
	{
		GetLastErrorText(strErrorText, 260);
		MessageBox(hMainWnd, strErrorText, "Unable to Terminate Process", MB_OK|MB_ICONSTOP);
		return;
	}

	if (!TerminateProcess(hProcess, 0))
	{
		GetLastErrorText(strErrorText, 260);
		MessageBox(hMainWnd, strErrorText, "Unable to Terminate Process", MB_OK|MB_ICONSTOP);
	}

	CloseHandle(hProcess);
}
