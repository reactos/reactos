/*
 *  ReactOS Task Manager
 *
 *  performancepage.h
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
	
#ifndef __PERFORMANCEPAGE_H
#define __PERFORMANCEPAGE_H

#ifdef __cplusplus
extern "C" {
#endif

extern	HWND		hPerformancePage;		/* Performance Property Page */
LRESULT CALLBACK	PerformancePageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void	RefreshPerformancePage(void);
void	PerformancePage_OnViewShowKernelTimes(void);
void	PerformancePage_OnViewCPUHistoryOneGraphAll(void);
void	PerformancePage_OnViewCPUHistoryOneGraphPerCPU(void);

#ifdef __cplusplus
};
#endif

#endif /* __PERFORMANCEPAGE_H */
