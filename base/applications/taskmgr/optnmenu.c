/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Menu item handlers for the options menu.
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 */

#include "precomp.h"

void TaskManager_OnOptionsAlwaysOnTop(void)
{
    HMENU  hMenu;
    HMENU  hOptionsMenu;

    hMenu = GetMenu(hMainWnd);
    hOptionsMenu = GetSubMenu(hMenu, OPTIONS_MENU_INDEX);

    /*
     * Check or uncheck the always on top menu item
     * and update main window.
     */
    if ((GetWindowLongPtrW(hMainWnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0)
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
    HMENU  hMenu;
    HMENU  hOptionsMenu;

    hMenu = GetMenu(hMainWnd);
    hOptionsMenu = GetSubMenu(hMenu, OPTIONS_MENU_INDEX);

    /*
     * Check or uncheck the minimize on use menu item.
     */
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
    HMENU  hMenu;
    HMENU  hOptionsMenu;

    hMenu = GetMenu(hMainWnd);
    hOptionsMenu = GetSubMenu(hMenu, OPTIONS_MENU_INDEX);

    /*
     * Check or uncheck the hide when minimized menu item.
     */
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
    HMENU  hMenu;
    HMENU  hOptionsMenu;

    hMenu = GetMenu(hMainWnd);
    hOptionsMenu = GetSubMenu(hMenu, OPTIONS_MENU_INDEX);

    /*
     * FIXME: Currently this is useless because the
     * current implementation doesn't list the 16-bit
     * processes. I believe that would require querying
     * each ntvdm.exe process for it's children.
     */

    /*
     * Check or uncheck the show 16-bit tasks menu item
     */
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

    /*
     * Refresh the list of processes.
     */
    RefreshProcessPage();
}
