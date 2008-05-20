/*
 *  ReactOS Task Manager
 *
 *  taskmgr.h
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

#ifndef __TASKMGR_H__
#define __TASKMGR_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
/*MF
typedef struct _IO_COUNTERS {
	ULONGLONG  ReadOperationCount;
	ULONGLONG  WriteOperationCount;
	ULONGLONG  OtherOperationCount;
	ULONGLONG ReadTransferCount;
	ULONGLONG WriteTransferCount;
	ULONGLONG OtherTransferCount;
} IO_COUNTERS, *PIO_COUNTERS;
*/
#endif /* _MSC_VER */

#include "resource.h"

#define RUN_APPS_PAGE
#define RUN_PROC_PAGE
#define RUN_PERF_PAGE

#define STATUS_WINDOW	2001

typedef struct
{
	/* Window size & position settings */
	BOOL	Maximized;
	int	Left;
	int	Top;
	int	Right;
	int	Bottom;

	/* Tab settings */
	int	ActiveTabPage;

	/* Options menu settings */
	BOOL	AlwaysOnTop;
	BOOL	MinimizeOnUse;
	BOOL	HideWhenMinimized;
	BOOL	Show16BitTasks;

	/* Update speed settings */
	/* How many half-seconds in between updates (i.e. 0 - Paused, 1 - High, 2 - Normal, 4 - Low) */
	DWORD	UpdateSpeed;

	/* Applications page settings */
	DWORD	ViewMode;

	/* Processes page settings */
	BOOL	ShowProcessesFromAllUsers; /* Server-only? */
	BOOL	Columns[COLUMN_NMAX];
	int		ColumnOrderArray[COLUMN_NMAX];
	int		ColumnSizeArray[COLUMN_NMAX];
	int		SortColumn;
	BOOL	SortAscending;

	/* Performance page settings */
	BOOL	CPUHistory_OneGraphPerCPU;
	BOOL	ShowKernelTimes;

} TASKMANAGER_SETTINGS, *LPTASKMANAGER_SETTINGS;

/* Global Variables: */
extern	HINSTANCE	hInst;						/* current instance */
extern	HWND		hMainWnd;					/* Main Window */
extern	HWND		hStatusWnd;					/* Status Bar Window */
extern	HWND		hTabWnd;					/* Tab Control Window */
extern	int			nMinimumWidth;				/* Minimum width of the dialog (OnSize()'s cx) */
extern	int			nMinimumHeight;				/* Minimum height of the dialog (OnSize()'s cy) */
extern	int			nOldWidth;					/* Holds the previous client area width */
extern	int			nOldHeight;					/* Holds the previous client area height */
extern	TASKMANAGER_SETTINGS	TaskManagerSettings;

/* Foward declarations of functions included in this code module: */
INT_PTR CALLBACK TaskManagerWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL OnCreate(HWND hWnd);
void OnSize(WPARAM nType, int cx, int cy);
void OnMove(WPARAM nType, int cx, int cy);
void FillSolidRect(HDC hDC, LPCRECT lpRect, COLORREF clr);
void FillSolidRect2(HDC hDC, int x, int y, int cx, int cy, COLORREF clr);
void Draw3dRect(HDC hDC, int x, int y, int cx, int cy, COLORREF clrTopLeft, COLORREF clrBottomRight);
void Draw3dRect2(HDC hDC, LPRECT lpRect, COLORREF clrTopLeft, COLORREF clrBottomRight);
void LoadSettings(void);
void SaveSettings(void);
void TaskManager_OnRestoreMainWindow(void);
void TaskManager_OnEnterMenuLoop(HWND hWnd);
void TaskManager_OnExitMenuLoop(HWND hWnd);
void TaskManager_OnMenuSelect(HWND hWnd, UINT nItemID, UINT nFlags, HMENU hSysMenu);
void TaskManager_OnViewUpdateSpeed(DWORD);
void TaskManager_OnViewRefresh(void);
void TaskManager_OnTabWndSelChange(void);
LPTSTR GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize );

#ifdef __cplusplus
}
#endif

#endif /* __TASKMGR_H__ */
