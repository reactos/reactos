/*
 * Program Manager
 *
 * Copyright 1996 Ulrich Schmid
 * Copyright 2002 Sylvain Petreolle
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "progman.h"

/* Class names */

WCHAR STRING_MAIN_WIN_CLASS_NAME[]    = {'P','M','M','a','i','n',0};
WCHAR STRING_MDI_WIN_CLASS_NAME[]     = {'M','D','I','C','L','I','E','N','T',0};
WCHAR STRING_GROUP_WIN_CLASS_NAME[]   = {'P','M','G','r','o','u','p',0};
WCHAR STRING_PROGRAM_WIN_CLASS_NAME[] = {'P','M','P','r','o','g','r','a','m',0};

VOID STRING_LoadMenus(VOID)
{
  CHAR   caption[MAX_STRING_LEN];
  HMENU  hMainMenu;

  /* Set frame caption */
  LoadStringA(Globals.hInstance, IDS_PROGRAM_MANAGER, caption, sizeof(caption));
  SetWindowTextA(Globals.hMainWnd, caption);

  /* Create menu */
  hMainMenu = LoadMenuW(Globals.hInstance, MAKEINTRESOURCEW(MAIN_MENU));
  Globals.hFileMenu     = GetSubMenu(hMainMenu, 0);
  Globals.hOptionMenu   = GetSubMenu(hMainMenu, 1);
  Globals.hWindowsMenu  = GetSubMenu(hMainMenu, 2);
  Globals.hLanguageMenu = GetSubMenu(hMainMenu, 3);

  if (Globals.hMDIWnd)
    SendMessageW(Globals.hMDIWnd, WM_MDISETMENU,
		(WPARAM) hMainMenu,
		(LPARAM) Globals.hWindowsMenu);
  else SetMenu(Globals.hMainWnd, hMainMenu);

  /* Destroy old menu */
  if (Globals.hMainMenu) DestroyMenu(Globals.hMainMenu);
  Globals.hMainMenu = hMainMenu;
}
