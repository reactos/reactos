/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Applications Page
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 *              Copyright 2005 Klemens Friedl <frik85@reactos.at>
 *              Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

extern HWND hApplicationPage;               /* Application List Property Page */
extern HWND hApplicationPageListCtrl;       /* Application ListCtrl Window */
extern HWND hApplicationPageEndTaskButton;  /* Application End Task button */
extern HWND hApplicationPageSwitchToButton; /* Application Switch To button */
extern HWND hApplicationPageNewTaskButton;  /* Application New Task button */

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
