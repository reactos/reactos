/*
 *  ReactOS Task Manager
 *
 *  debug.cpp
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
	
#include "stdafx.h"
#include "taskmgr.h"
#include "debug.h"
#include "ProcessPage.h"
#include "perfdata.h"

void ProcessPage_OnDebug(void)
{
	LVITEM				lvitem;
	ULONG				Index;
	DWORD				dwProcessId;
	TCHAR				strErrorText[260];
	HKEY				hKey;
	TCHAR				strDebugPath[260];
	TCHAR				strDebugger[260];
	DWORD				dwDebuggerSize;
	PROCESS_INFORMATION	pi;
	STARTUPINFO			si;
	HANDLE				hDebugEvent;

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

	if (MessageBox(hMainWnd, "WARNING: Debugging this process may result in loss of data.\nAre you sure you wish to attach the debugger?", "Task Manager Warning", MB_YESNO|MB_ICONWARNING) != IDYES)
	{
		GetLastErrorText(strErrorText, 260);
		MessageBox(hMainWnd, strErrorText, "Unable to Debug Process", MB_OK|MB_ICONSTOP);
		return;
	}

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
	{
		GetLastErrorText(strErrorText, 260);
		MessageBox(hMainWnd, strErrorText, "Unable to Debug Process", MB_OK|MB_ICONSTOP);
		return;
	}

	dwDebuggerSize = 260;
	if (RegQueryValueEx(hKey, "Debugger", NULL, NULL, (LPBYTE)strDebugger, &dwDebuggerSize) != ERROR_SUCCESS)
	{
		GetLastErrorText(strErrorText, 260);
		MessageBox(hMainWnd, strErrorText, "Unable to Debug Process", MB_OK|MB_ICONSTOP);
		RegCloseKey(hKey);
		return;
	}

	RegCloseKey(hKey);

	hDebugEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!hDebugEvent)
	{
		GetLastErrorText(strErrorText, 260);
		MessageBox(hMainWnd, strErrorText, "Unable to Debug Process", MB_OK|MB_ICONSTOP);
		return;
	}

	wsprintf(strDebugPath, strDebugger, dwProcessId, hDebugEvent);

	memset(&pi, 0, sizeof(PROCESS_INFORMATION));
	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	if (!CreateProcess(NULL, strDebugPath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		GetLastErrorText(strErrorText, 260);
		MessageBox(hMainWnd, strErrorText, "Unable to Debug Process", MB_OK|MB_ICONSTOP);
	}

	CloseHandle(hDebugEvent);
}
