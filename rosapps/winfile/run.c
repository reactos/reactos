/*
 *  ReactOS Task Manager
 *
 *  run.c
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
	
#include <shellapi.h>

#include "main.h"
#include "run.h"


typedef	void (WINAPI *RUNFILEDLG)(
						HWND    hwndOwner, 
						HICON   hIcon, 
						LPCSTR  lpstrDirectory, 
						LPCSTR  lpstrTitle, 
						LPCSTR  lpstrDescription,
						UINT    uFlags); 

//
// Flags for RunFileDlg
//

#define	RFF_NOBROWSE		0x01	// Removes the browse button. 
#define	RFF_NODEFAULT		0x02	// No default item selected. 
#define	RFF_CALCDIRECTORY	0x04	// Calculates the working directory from the file name.
#define	RFF_NOLABEL			0x08	// Removes the edit box label. 
#define	RFF_NOSEPARATEMEM	0x20	// Removes the Separate Memory Space check box (Windows NT only).


// Show "Run..." dialog
void OnFileRun(void)
{
	HMODULE			hShell32;
	RUNFILEDLG		RunFileDlg;
	OSVERSIONINFO	versionInfo;
	WCHAR			wTitle[40];
	WCHAR			wText[256];
	CHAR			szTitle[40] = "Create New Task";
	CHAR			szText[256] = "Type the name of a program, folder, document, or Internet resource, and Task Manager will open it for you.";

	hShell32 = LoadLibrary(_T("SHELL32.DLL"));
	RunFileDlg = (RUNFILEDLG)(FARPROC)GetProcAddress(hShell32, (CHAR*)((long)0x3D));
	if (RunFileDlg)	{
		versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(&versionInfo);
		if (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szTitle, -1, wTitle, 40);
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szText, -1, wText, 256);
			RunFileDlg(Globals.hMainWnd, 0, NULL, (LPCSTR)wTitle, (LPCSTR)wText, RFF_CALCDIRECTORY);
        } else {
			RunFileDlg(Globals.hMainWnd, 0, NULL, szTitle, szText, RFF_CALCDIRECTORY);
        }
	}
	FreeLibrary(hShell32);
}


/*
typedef struct _SHELLEXECUTEINFO{
    DWORD cbSize; 
    ULONG fMask; 
    HWND hwnd; 
    LPCTSTR lpVerb; 
    LPCTSTR lpFile; 
    LPCTSTR lpParameters; 
    LPCTSTR lpDirectory; 
    int nShow; 
    HINSTANCE hInstApp; 
 
    // Optional members 
    LPVOID lpIDList; 
    LPCSTR lpClass; 
    HKEY hkeyClass; 
    DWORD dwHotKey; 
	union {
		HANDLE hIcon;
		HANDLE hMonitor;
	};
    HANDLE hProcess; 
} SHELLEXECUTEINFO, *LPSHELLEXECUTEINFO; 
 */

BOOL OpenTarget(HWND hWnd, TCHAR* target)
{
    BOOL result = FALSE;
    SHELLEXECUTEINFO shExecInfo;

    memset(&shExecInfo, 0, sizeof(shExecInfo));
    shExecInfo.cbSize = sizeof(shExecInfo);
    shExecInfo.fMask = 0;
    shExecInfo.hwnd = hWnd;
    shExecInfo.lpVerb = NULL;
    shExecInfo.lpFile = target;
    shExecInfo.lpParameters = NULL;
    shExecInfo.lpDirectory = NULL;
    shExecInfo.nShow = SW_SHOW;
    shExecInfo.hInstApp = 0;

    result = ShellExecuteEx(&shExecInfo);


    return result;
}
