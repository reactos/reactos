/*
 *  ReactOS Task Manager
 *
 *  processpage.h
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
	
#ifndef __PROCESSPAGE_H
#define __PROCESSPAGE_H

extern	HWND		hProcessPage;						// Process List Property Page
extern	HWND		hProcessPageListCtrl;				// Process ListCtrl Window
extern	HWND		hProcessPageHeaderCtrl;				// Process Header Control
extern	HWND		hProcessPageEndProcessButton;		// Process End Process button
extern	HWND		hProcessPageShowAllProcessesButton;	// Process Show All Processes checkbox

LRESULT CALLBACK	ProcessPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void				RefreshProcessPage(void);

#endif // defined __PROCESSPAGE_H
