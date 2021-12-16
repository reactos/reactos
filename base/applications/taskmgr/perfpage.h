/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Performance Page.
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

extern	HWND		hPerformancePage;		/* Performance Property Page */
INT_PTR CALLBACK	PerformancePageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void	RefreshPerformancePage(void);
void	PerformancePage_OnViewShowKernelTimes(void);
void	PerformancePage_OnViewCPUHistoryOneGraphAll(void);
void	PerformancePage_OnViewCPUHistoryOneGraphPerCPU(void);

#ifdef __cplusplus
};
#endif
