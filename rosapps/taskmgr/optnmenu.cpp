/*
 *  ReactOS Task Manager
 *
 *  optnmenu.cpp
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
	
//
// options.c
//
// Menu item handlers for the options menu.
//

#ifdef _MSC_VER
#include "stdafx.h"
#else
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
#endif
	
#include "taskmgr.h"
#include "optnmenu.h"
#include "ProcessPage.h"

#define OPTIONS_MENU_INDEX	1

void TaskManager_OnOptionsAlwaysOnTop(void)
{
	HMENU	hMenu;
	HMENU	hOptionsMenu;

	hMenu = GetMenu(hMainWnd);
	hOptionsMenu = GetSubMenu(hMenu, OPTIONS_MENU_INDEX);

	//
	// Check or uncheck the always on top menu item
	// and update main window.
	//
	if (GetMenuState(hOptionsMenu, ID_OPTIONS_ALWAYSONTOP, MF_BYCOMMAND) & MF_CHECKED)
	{
		CheckMenuItem(hOptionsMenu, ID_OPTIONS_ALWAYSONTOP, MF_BYCOMMAND|MF_UNCHECKED);
		TaskManagerSettings.AlwaysOnTop = FALSE;
		SetWindowPos(hMainWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
	}
	else
	{
		CheckMenuItem(hOptionsMenu, ID_OPTIONS_ALWAYSONTOP, MF_BYCOMMAND|MF_CHECKED);
		TaskManagerSettings.AlwaysOnTop = TRUE;
		SetWindowPos(hMainWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
	}
}

void TaskManager_OnOptionsMinimizeOnUse(void)
{
	HMENU	hMenu;
	HMENU	hOptionsMenu;

	hMenu = GetMenu(hMainWnd);
	hOptionsMenu = GetSubMenu(hMenu, OPTIONS_MENU_INDEX);

	//
	// Check or uncheck the minimize on use menu item.
	//
	if (GetMenuState(hOptionsMenu, ID_OPTIONS_MINIMIZEONUSE, MF_BYCOMMAND) & MF_CHECKED)
	{
		CheckMenuItem(hOptionsMenu, ID_OPTIONS_MINIMIZEONUSE, MF_BYCOMMAND|MF_UNCHECKED);
		TaskManagerSettings.MinimizeOnUse = FALSE;
	}
	else
	{
		CheckMenuItem(hOptionsMenu, ID_OPTIONS_MINIMIZEONUSE, MF_BYCOMMAND|MF_CHECKED);
		TaskManagerSettings.MinimizeOnUse = TRUE;
	}
}

void TaskManager_OnOptionsHideWhenMinimized(void)
{
	HMENU	hMenu;
	HMENU	hOptionsMenu;

	hMenu = GetMenu(hMainWnd);
	hOptionsMenu = GetSubMenu(hMenu, OPTIONS_MENU_INDEX);

	//
	// Check or uncheck the hide when minimized menu item.
	//
	if (GetMenuState(hOptionsMenu, ID_OPTIONS_HIDEWHENMINIMIZED, MF_BYCOMMAND) & MF_CHECKED)
	{
		CheckMenuItem(hOptionsMenu, ID_OPTIONS_HIDEWHENMINIMIZED, MF_BYCOMMAND|MF_UNCHECKED);
		TaskManagerSettings.HideWhenMinimized = FALSE;
	}
	else
	{
		CheckMenuItem(hOptionsMenu, ID_OPTIONS_HIDEWHENMINIMIZED, MF_BYCOMMAND|MF_CHECKED);
		TaskManagerSettings.HideWhenMinimized = TRUE;
	}
}

void TaskManager_OnOptionsShow16BitTasks(void)
{
	HMENU	hMenu;
	HMENU	hOptionsMenu;

	hMenu = GetMenu(hMainWnd);
	hOptionsMenu = GetSubMenu(hMenu, OPTIONS_MENU_INDEX);

	//
	// FIXME: Currently this is useless because the
	// current implemetation doesn't list the 16-bit
	// processes. I believe that would require querying
	// each ntvdm.exe process for it's children.
	//

	//
	// Check or uncheck the show 16-bit tasks menu item
	//
	if (GetMenuState(hOptionsMenu, ID_OPTIONS_SHOW16BITTASKS, MF_BYCOMMAND) & MF_CHECKED)
	{
		CheckMenuItem(hOptionsMenu, ID_OPTIONS_SHOW16BITTASKS, MF_BYCOMMAND|MF_UNCHECKED);
		TaskManagerSettings.Show16BitTasks = FALSE;
	}
	else
	{
		CheckMenuItem(hOptionsMenu, ID_OPTIONS_SHOW16BITTASKS, MF_BYCOMMAND|MF_CHECKED);
		TaskManagerSettings.Show16BitTasks = TRUE;
	}

	//
	// Refresh the list of processes.
	//
	RefreshProcessPage();
}
