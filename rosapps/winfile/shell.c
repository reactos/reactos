/*
 *  ReactOS winfile
 *
 *  shell.c
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
    
#include "main.h"
#include "shell.h"
#include "format.h"


////////////////////////////////////////////////////////////////////////////////
// Global Variables:
//

//DWORD WINAPI SHFormatDrive(HWND hWnd, UINT drive, UINT fmtID, UINT options);

typedef DWORD (WINAPI *SHFormatDrive_Ptr)(HWND, UINT, UINT, UINT);


BOOL CheckShellAvailable(void)
{
	HMODULE	hShell32;

	hShell32 = LoadLibrary(_T("SHELL32.DLL"));
    if (hShell32) {
        FreeLibrary(hShell32);
	}
    return (hShell32 != NULL);
}


void FormatDisk(HWND hWnd)
{
//	SHFormatDrive(hWnd, 0 /* A: */, SHFMT_ID_DEFAULT, 0);
	HMODULE	hShell32;
    SHFormatDrive_Ptr pSHFormatDrive;

	hShell32 = LoadLibrary(_T("SHELL32.DLL"));
    if (hShell32) {
        pSHFormatDrive = (SHFormatDrive_Ptr)(FARPROC)GetProcAddress(hShell32, "SHFormatDrive");
        if (pSHFormatDrive)	{
		    UINT OldMode = SetErrorMode(0); // Get the current Error Mode settings.
		    SetErrorMode(OldMode & ~SEM_FAILCRITICALERRORS); // Force O/S to handle
            pSHFormatDrive(hWnd, 0 /* A: */, SHFMT_ID_DEFAULT, 0);
		    SetErrorMode(OldMode); // Put it back the way it was. 			
        }
        FreeLibrary(hShell32);
	}
}

void CopyDisk(HWND hWnd)
{
}

void LabelDisk(HWND hWnd)
{
}

void ModifySharing(HWND hWnd, BOOL create)
{
}
