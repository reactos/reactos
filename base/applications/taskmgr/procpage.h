/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Processes Page.
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 *              Copyright 2009 Maxime Vernier <maxime.vernier@gmail.com>
 *              Copyright 2022 Thamatip Chitpong <tangaming123456@outlook.com>
 */

#pragma once

extern HWND hProcessPage;           /* Process List Property Page */
extern HWND hProcessPageListCtrl;   /* Process ListCtrl Window */
extern HWND hProcessPageHeaderCtrl; /* Process Header Control */

INT_PTR CALLBACK ProcessPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void  RefreshProcessPage(void);
DWORD GetSelectedProcessId(void);
void  ProcessPage_OnProperties(void);
void  ProcessPage_OnOpenFileLocation(void);
