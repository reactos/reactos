/*
 *  ReactOS Task Manager
 *
 *  run.c
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
    
#define WIN32_LEAN_AND_MEAN    /* Exclude rarely-used stuff from Windows headers */
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <stdio.h>
    
#include "taskmgr.h"
#include "run.h"

void TaskManager_OnFileNew(void)
{
    HMODULE            hShell32;
    RUNFILEDLG        RunFileDlg;
    OSVERSIONINFO    versionInfo;
    WCHAR            wTitle[40];
    WCHAR            wText[256];
    char            szTitle[40] = "Create New Task";
    char            szText[256] = "Type the name of a program, folder, document, or Internet resource, and Task Manager will open it for you.";

    hShell32 = LoadLibrary(_T("SHELL32.DLL"));
    RunFileDlg = (RUNFILEDLG)(FARPROC)GetProcAddress(hShell32, (char*)((long)0x3D));

    /* Show "Run..." dialog */
    if (RunFileDlg)
    {
        versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&versionInfo);

        if (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szTitle, -1, wTitle, 40);
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szText, -1, wText, 256);
            RunFileDlg(hMainWnd, 0, NULL, (LPCSTR)wTitle, (LPCSTR)wText, RFF_CALCDIRECTORY);
        }
        else
            RunFileDlg(hMainWnd, 0, NULL, szTitle, szText, RFF_CALCDIRECTORY);
    }

    FreeLibrary(hShell32);
}
