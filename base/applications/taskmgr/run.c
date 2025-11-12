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
        STARTUPINFOW siInfo = {0};
        siInfo.cb = sizeof(siInfo);
        PROCESS_INFORMATION piInfo = {0};
        ZeroMemory(&piInfo, sizeof(piInfo));
        WCHAR application[256];
        DWORD envarRes = GetEnvironmentVariableW(L"ComSpec", application, 256);
        if(envarRes == 0)
        {
            // couldn't get the environment variable, default to cmd.exe
            // application has a buffer size of 256, it should fit without issues
            wcscpy(application, L"cmd.exe");
        }
        BOOL result = CreateProcessW(NULL, 
                                     application, 
                                     NULL, 
                                     NULL, 
                                     FALSE, 
                                     CREATE_NEW_CONSOLE, 
                                     NULL, 
                                     NULL, &siInfo, 
                                     &piInfo);
        if(result == TRUE)
        {
            CloseHandle(piInfo.hThread);
            CloseHandle(piInfo.hProcess);
        } else {
            // couldn't create cmd.exe from ComSpec value, try again with cmd.exe
            WCHAR appCmd[] = L"cmd.exe";
            result = CreateProcessW(NULL, 
                                     appCmd, 
                                     NULL, 
                                     NULL, 
                                     FALSE, 
                                     CREATE_NEW_CONSOLE, 
                                     NULL, 
                                     NULL, &siInfo, 
                                     &piInfo);
            if(result == TRUE)
            {
                CloseHandle(piInfo.hThread);
                CloseHandle(piInfo.hProcess);
            }
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
