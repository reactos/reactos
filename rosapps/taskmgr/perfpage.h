/*
 *  ReactOS Task Manager
 *
 *  performancepage.h
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
	
#ifndef __PERFORMANCEPAGE_H
#define __PERFORMANCEPAGE_H

#ifdef __cplusplus
extern "C" {
#endif


extern	HWND		hPerformancePage;				// Performance Property Page

LRESULT CALLBACK	PerformancePageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void				RefreshPerformancePage(void);

void				PerformancePage_OnViewShowKernelTimes(void);
void				PerformancePage_OnViewCPUHistoryOneGraphAll(void);
void				PerformancePage_OnViewCPUHistoryOneGraphPerCPU(void);


#ifdef __cplusplus
};
#endif

#endif // __PERFORMANCEPAGE_H
