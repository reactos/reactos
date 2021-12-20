/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Main Header
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 */

#pragma once

#include "resource.h"

#define STATUS_WINDOW	2001
#define STATUS_SIZE1	85
#define STATUS_SIZE2	157 // he-IL.rc determines minimum width: 72 == 157 - 85
#define STATUS_SIZE3	400

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

	/* 0 - Paused, 1 - High, 2 - Normal, 4 - Low */
	DWORD	UpdateSpeed;

	/* Applications page settings */
	DWORD	ViewMode;

	/* Processes page settings */
	BOOL	ShowProcessesFromAllUsers;
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

/* Forward declarations of functions included in this code module: */
INT_PTR CALLBACK TaskManagerWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL OnCreate(HWND hWnd);
void OnSize(WPARAM nType, int cx, int cy);
void OnMove(WPARAM nType, int cx, int cy);
void FillSolidRect(HDC hDC, LPCRECT lpRect, COLORREF clr);
void LoadSettings(void);
void SaveSettings(void);
void TaskManager_OnRestoreMainWindow(void);
void TaskManager_OnMenuSelect(HWND hWnd, UINT nItemID, UINT nFlags, HMENU hSysMenu);
void TaskManager_OnViewUpdateSpeed(DWORD);
void TaskManager_OnTabWndSelChange(void);
BOOL ConfirmMessageBox(HWND hWnd, LPCWSTR Text, LPCWSTR Title, UINT Type);
VOID ShowWin32Error(DWORD dwError);
LPTSTR GetLastErrorText(LPTSTR lpszBuf, DWORD dwSize);
DWORD EndLocalThread(HANDLE *hThread, DWORD dwThread);
