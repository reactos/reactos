/*
 *  ReactOS winfile
 *
 *  worker.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
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
#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
#endif
    
#include <windowsx.h>
#include <process.h>
#include <assert.h>
#define ASSERT assert

#include "main.h"
#include "worker.h"
#include "drivebar.h"


////////////////////////////////////////////////////////////////////////////////
// Global Variables:
//

static HANDLE hMonitorThreadEvent = NULL;	// When this event becomes signaled then we run the monitor thread

void MonitorThreadProc(void *lpParameter);

////////////////////////////////////////////////////////////////////////////////
// Local module support methods
//


LRESULT CALLBACK MoveDialogWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND    hLicenseEditWnd;
    TCHAR   strLicense[0x1000];

    switch (message) {
    case WM_INITDIALOG:
        hLicenseEditWnd = GetDlgItem(hDlg, IDC_LICENSE_EDIT);
        LoadString(hInst, IDS_LICENSE, strLicense, 0x1000);
        SetWindowText(hLicenseEditWnd, strLicense);
        return TRUE;
    case WM_COMMAND:
        if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL)) {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        break;
    }
    return 0;
}

void StartWorkerThread(HWND hWnd)
{
    DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, (DLGPROC)MoveDialogWndProc);
}

////////////////////////////////////////////////////////////////////////////////

void MonitorThreadProc(void *lpParameter)
{
//	ULONG	OldProcessorUsage = 0;
//	ULONG	OldProcessCount = 0;
    HWND hWnd = (HWND)lpParameter;

	// Create the event
	hMonitorThreadEvent = CreateEvent(NULL, TRUE, TRUE, "Winfile Monitor Event");

	// If we couldn't create the event then exit the thread
	if (!hMonitorThreadEvent)
		return;

	while (1) {
		DWORD	dwWaitVal;

		// Wait on the event
		dwWaitVal = WaitForSingleObject(hMonitorThreadEvent, INFINITE);

		// If the wait failed then the event object must have been
		// closed and the task manager is exiting so exit this thread
        if (dwWaitVal == WAIT_FAILED) {
            // CloseHandle(hMonitorThreadEvent); // Should we close the event object handle or not ???
            // hMonitorThreadEvent = NULL; // if we do then check what happens when main thread tries to delete it also....
			return;
        }

		if (dwWaitVal == WAIT_OBJECT_0) {
			// Reset our event
			ResetEvent(hMonitorThreadEvent);


            ConfigureDriveBar(Globals.hDriveBar);

#if 0
			TCHAR	text[260];
			if ((ULONG)SendMessage(hProcessPageListCtrl, LVM_GETITEMCOUNT, 0, 0) != PerfDataGetProcessCount())
				SendMessage(hProcessPageListCtrl, LVM_SETITEMCOUNT, PerfDataGetProcessCount(), /*LVSICF_NOINVALIDATEALL|*/LVSICF_NOSCROLL);
			if (IsWindowVisible(hProcessPage))
				InvalidateRect(hProcessPageListCtrl, NULL, FALSE);
			if (OldProcessorUsage != PerfDataGetProcessorUsage()) {
				OldProcessorUsage = PerfDataGetProcessorUsage();
				wsprintf(text, _T("CPU Usage: %3d%%"), OldProcessorUsage);
				SendMessage(hStatusWnd, SB_SETTEXT, 1, (LPARAM)text);
			}
			if (OldProcessCount != PerfDataGetProcessCount()) {
				OldProcessCount = PerfDataGetProcessCount();
				wsprintf(text, _T("Processes: %d"), OldProcessCount);
				SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)text);
			}
#endif
		}
	}
}

BOOL CreateMonitorThread(HWND hWnd)
{
    _beginthread(MonitorThreadProc, 0, hWnd);
    return TRUE;
}

void SignalMonitorEvent(void)
{
    SetEvent(hMonitorThreadEvent);
}

BOOL DestryMonitorThread(void)
{
	CloseHandle(hMonitorThreadEvent);
    hMonitorThreadEvent = NULL;
    return TRUE;
}

