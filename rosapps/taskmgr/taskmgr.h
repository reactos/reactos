/*
 *  ReactOS Task Manager
 *
 *  taskmgr.h
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
	

#if !defined(AFX_TASKMGR_H__392F6393_0279_11D3_9C02_004005E27102__INCLUDED_)
#define AFX_TASKMGR_H__392F6393_0279_11D3_9C02_004005E27102__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"


#define STATUS_WINDOW	2001

typedef struct
{
	// Window size & position settings
	BOOL	Maximized;
	int		Left;
	int		Top;
	int		Right;
	int		Bottom;

	// Tab settings
	int		ActiveTabPage;

	// Options menu settings
	BOOL	AlwaysOnTop;
	BOOL	MinimizeOnUse;
	BOOL	HideWhenMinimized;
	BOOL	Show16BitTasks;

	// Update speed settings
	int		UpdateSpeed; // How many half-seconds in between updates (i.e. 0 - Paused, 1 - High, 2 - Normal, 4 - Low)

	// Applications page settings
	BOOL	View_LargeIcons;
	BOOL	View_SmallIcons;
	BOOL	View_Details;

	// Processes page settings
	BOOL	ShowProcessesFromAllUsers; // Server-only?
	BOOL	Column_ImageName;
	BOOL	Column_PID;
	BOOL	Column_CPUUsage;
	BOOL	Column_CPUTime;
	BOOL	Column_MemoryUsage;
	BOOL	Column_MemoryUsageDelta;
	BOOL	Column_PeakMemoryUsage;
	BOOL	Column_PageFaults;
	BOOL	Column_USERObjects;
	BOOL	Column_IOReads;
	BOOL	Column_IOReadBytes;
	BOOL	Column_SessionID; // Server-only?
	BOOL	Column_UserName; // Server-only?
	BOOL	Column_PageFaultsDelta;
	BOOL	Column_VirtualMemorySize;
	BOOL	Column_PagedPool;
	BOOL	Column_NonPagedPool;
	BOOL	Column_BasePriority;
	BOOL	Column_HandleCount;
	BOOL	Column_ThreadCount;
	BOOL	Column_GDIObjects;
	BOOL	Column_IOWrites;
	BOOL	Column_IOWriteBytes;
	BOOL	Column_IOOther;
	BOOL	Column_IOOtherBytes;
	int		ColumnOrderArray[25];
	int		ColumnSizeArray[25];
	int		SortColumn;
	BOOL	SortAscending;

	// Performance page settings
	BOOL	CPUHistory_OneGraphPerCPU;
	BOOL	ShowKernelTimes;

} TASKMANAGER_SETTINGS, *LPTASKMANAGER_SETTINGS;

// Global Variables:
extern	HINSTANCE	hInst;						// current instance
extern	HWND		hMainWnd;					// Main Window
extern	HWND		hStatusWnd;					// Status Bar Window
extern	HWND		hTabWnd;					// Tab Control Window

extern	int			nMinimumWidth;				// Minimum width of the dialog (OnSize()'s cx)
extern	int			nMinimumHeight;				// Minimum height of the dialog (OnSize()'s cy)
extern	int			nOldWidth;					// Holds the previous client area width
extern	int			nOldHeight;					// Holds the previous client area height

extern	TASKMANAGER_SETTINGS	TaskManagerSettings;

// Foward declarations of functions included in this code module:
LRESULT CALLBACK TaskManagerWndProc(HWND, UINT, WPARAM, LPARAM);

BOOL OnCreate(HWND hWnd);
void OnSize( UINT nType, int cx, int cy );

void FillSolidRect(HDC hDC, LPCRECT lpRect, COLORREF clr);
void FillSolidRect(HDC hDC, int x, int y, int cx, int cy, COLORREF clr);
void Draw3dRect(HDC hDC, int x, int y, int cx, int cy, COLORREF clrTopLeft, COLORREF clrBottomRight);
void Draw3dRect(HDC hDC, LPRECT lpRect, COLORREF clrTopLeft, COLORREF clrBottomRight);

void LoadSettings(void);
void SaveSettings(void);

void TaskManager_OnEnterMenuLoop(HWND hWnd);
void TaskManager_OnExitMenuLoop(HWND hWnd);
void TaskManager_OnMenuSelect(HWND hWnd, UINT nItemID, UINT nFlags, HMENU hSysMenu);

void TaskManager_OnViewUpdateSpeedHigh(void);
void TaskManager_OnViewUpdateSpeedNormal(void);
void TaskManager_OnViewUpdateSpeedLow(void);
void TaskManager_OnViewUpdateSpeedPaused(void);

void TaskManager_OnTabWndSelChange(void);

LPTSTR GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize );

#endif // !defined(AFX_TASKMGR_H__392F6393_0279_11D3_9C02_004005E27102__INCLUDED_)
