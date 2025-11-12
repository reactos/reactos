/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Run Task
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 *              Copyright 2005 Klemens Friedl <frik85@reactos.at>
 */

#include "precomp.h"

void TaskManager_OnFileNew(void)
{
    HMODULE     hShell32;
    RUNFILEDLG  RunFileDlg;
    WCHAR       szTitle[40];
    WCHAR       szText[256];

    /* If CTRL is held, start the user's command-line shell */
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
    {
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi = {0};
        WCHAR szComSpec[MAX_PATH];
        DWORD envarRes = GetEnvironmentVariableW(L"ComSpec", szComSpec, _countof(szComSpec));
        if (envarRes == 0)
        {
            /* Couldn't get the environment variable, default to cmd.exe */
            wcscpy(szComSpec, L"cmd.exe");
        }
        BOOL result = CreateProcessW(NULL, 
                                     szComSpec, 
                                     NULL, 
                                     NULL, 
                                     FALSE, 
                                     CREATE_NEW_CONSOLE, 
                                     NULL, 
                                     NULL, &si, 
                                     &pi);
        if (!result)
        {
            /* Couldn't create cmd.exe from ComSpec value, try again with cmd.exe */
            WCHAR appCmd[] = L"cmd.exe";
            result = CreateProcessW(NULL, 
                                     appCmd, 
                                     NULL, 
                                     NULL, 
                                     FALSE, 
                                     CREATE_NEW_CONSOLE, 
                                     NULL, 
                                     NULL, &si, 
                                     &pi);
        }

        if (result)
        {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
        return;
    }

    /* Load language strings from resource file */
    LoadStringW(hInst, IDS_CREATENEWTASK, szTitle, _countof(szTitle));
    LoadStringW(hInst, IDS_CREATENEWTASK_DESC, szText, _countof(szText));

    hShell32 = LoadLibraryW(L"SHELL32.DLL");
    RunFileDlg = (RUNFILEDLG)(FARPROC)GetProcAddress(hShell32, (LPCSTR)61);

    /* Show "Run..." dialog */
    if (RunFileDlg)
    {
        HICON hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_TASKMANAGER));

        /* NOTE - don't check whether running on win 9x or NT, let's just
                  assume that a unicode build only runs on NT */
        RunFileDlg(hMainWnd, hIcon, NULL, szTitle, szText, RFF_CALCDIRECTORY);

        DeleteObject(hIcon);
    }

    FreeLibrary(hShell32);
}
