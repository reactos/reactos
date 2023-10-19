/*
 * PROJECT:   ReactOS Task Manager
 * LICENSE:   LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * COPYRIGHT: 1999-2001 Brian Palmer <brianp@reactos.org>
 */

#pragma once

extern HWND hProcessPage;
extern HWND hProcessPageListCtrl;
extern HWND hProcessPageHeaderCtrl;

INT_PTR CALLBACK ProcessPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void RefreshProcessPage(void);
DWORD GetSelectedProcessId(void);
