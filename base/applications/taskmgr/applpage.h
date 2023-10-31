/*
 * PROJECT:   ReactOS Task Manager
 * LICENSE:   LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * COPYRIGHT: 1999-2001 Brian Palmer <brianp@reactos.org>
 */

#pragma once

extern HWND hApplicationPage;               /* Application List Property Page */
extern HWND hApplicationPageListCtrl;       /* Application ListCtrl Window */
extern HWND hApplicationPageEndTaskButton;
extern HWND hApplicationPageSwitchToButton;
extern HWND hApplicationPageNewTaskButton;

INT_PTR CALLBACK    ApplicationPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void                RefreshApplicationPage(void);
void                UpdateApplicationListControlViewSetting(void);
void                ApplicationPage_OnView(DWORD);
void                ApplicationPage_OnWindowsTile(DWORD);
void                ApplicationPage_OnWindowsMinimize(void);
void                ApplicationPage_OnWindowsMaximize(void);
void                ApplicationPage_OnWindowsCascade(void);
void                ApplicationPage_OnWindowsBringToFront(void);
void                ApplicationPage_OnSwitchTo(void);
void                ApplicationPage_OnEndTask(void);
void                ApplicationPage_OnGotoProcess(void);
