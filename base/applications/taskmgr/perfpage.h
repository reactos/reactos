/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Performance Page
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 */

#pragma once

extern HWND hPerformancePage;   /* Performance Property Page */
INT_PTR CALLBACK PerformancePageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

void RefreshPerformancePage(void);
void PerformancePage_OnViewShowKernelTimes(void);

VOID
PerformancePage_OnViewCPUHistoryGraph(
    _In_ BOOL bShowAll);

#ifdef __cplusplus
};
#endif
