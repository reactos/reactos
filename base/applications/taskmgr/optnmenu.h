/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Menu item handlers for the options menu.
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 */

#pragma once

#define OPTIONS_MENU_INDEX  1

void TaskManager_OnOptionsAlwaysOnTop(void);
void TaskManager_OnOptionsMinimizeOnUse(void);
void TaskManager_OnOptionsHideWhenMinimized(void);
void TaskManager_OnOptionsShow16BitTasks(void);
