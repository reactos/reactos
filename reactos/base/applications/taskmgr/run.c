/*
 *  ReactOS Task Manager
 *
 *  run.c
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
 *                2005         Klemens Friedl <frik85@reactos.at>
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

#include <precomp.h>

void TaskManager_OnFileNew(void)
{
    HMODULE     hShell32;
    RUNFILEDLG  RunFileDlg;
    WCHAR       szTitle[40];
    WCHAR       szText[256];

    /* Load language strings from resource file */
    LoadString(hInst, IDS_CREATENEWTASK, szTitle, sizeof(szTitle) / sizeof(szTitle[0]));
    LoadString(hInst, IDS_CREATENEWTASK_DESC, szText, sizeof(szText) / sizeof(szText[0]));


    hShell32 = LoadLibrary(L"SHELL32.DLL");
    RunFileDlg = (RUNFILEDLG)(FARPROC)GetProcAddress(hShell32, (LPCSTR)0x3D);

    /* Show "Run..." dialog */
    if (RunFileDlg)
    {
        /* NOTE - don't check whether running on win 9x or NT, let's just
                  assume that a unicode build only runs on NT */
        RunFileDlg(hMainWnd, 0, NULL, szTitle, szText, RFF_CALCDIRECTORY);
    }

    FreeLibrary(hShell32);
}
