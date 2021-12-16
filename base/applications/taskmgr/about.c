/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     About Box.
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 */

#include "precomp.h"

void OnAbout(void)
{
    WCHAR szTaskmgr[128];
    HICON taskmgrIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_TASKMANAGER));

    LoadStringW(hInst, IDS_APP_TITLE, szTaskmgr, sizeof(szTaskmgr)/sizeof(WCHAR));
    ShellAboutW(hMainWnd, szTaskmgr, NULL, taskmgrIcon);
    DeleteObject(taskmgrIcon);
}
