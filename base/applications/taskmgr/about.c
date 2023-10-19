/*
 * PROJECT:   ReactOS Task Manager
 * LICENSE:   LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * COPYRIGHT: 1999-2001 Brian Palmer <brianp@reactos.org>
 */

#include "precomp.h"

void OnAbout(void)
{
    WCHAR szTaskmgr[128];

    LoadStringW(hInst, IDS_APP_TITLE, szTaskmgr, _countof(szTaskmgr));
    ShellAboutW(hMainWnd, szTaskmgr, 0,
                LoadIconW(hInst, MAKEINTRESOURCEW(IDI_TASKMANAGER)));
}
