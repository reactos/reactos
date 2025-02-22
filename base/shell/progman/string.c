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

/*
 * PROJECT:         ReactOS Program Manager
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            base/shell/progman/string.c
 * PURPOSE:         String utility functions
 * PROGRAMMERS:     Ulrich Schmid
 *                  Sylvain Petreolle
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include "progman.h"

WCHAR szTitle[256]; // MAX_STRING_LEN ?

VOID STRING_LoadStrings(VOID)
{
    LoadStringW(Globals.hInstance, IDS_PROGRAM_MANAGER, szTitle, ARRAYSIZE(szTitle));
}

VOID STRING_LoadMenus(VOID)
{
    HMENU hMainMenu;

    /* Create the menu */
    hMainMenu = LoadMenuW(Globals.hInstance, MAKEINTRESOURCEW(MAIN_MENU));
    Globals.hFileMenu     = GetSubMenu(hMainMenu, 0);
    Globals.hOptionMenu   = GetSubMenu(hMainMenu, 1);
    Globals.hWindowsMenu  = GetSubMenu(hMainMenu, 2);
    Globals.hLanguageMenu = GetSubMenu(hMainMenu, 3);

    if (Globals.hMDIWnd)
        SendMessageW(Globals.hMDIWnd, WM_MDISETMENU, (WPARAM)hMainMenu, (LPARAM)Globals.hWindowsMenu);
    else
        SetMenu(Globals.hMainWnd, hMainMenu);

    /* Destroy the old menu */
    if (Globals.hMainMenu)
        DestroyMenu(Globals.hMainMenu);
    Globals.hMainMenu = hMainMenu;
}
