/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tray Icon.
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 *              Copyright 2005 Klemens Friedl <frik85@reactos.at>
 */

#pragma once

#define WM_ONTRAYICON   (WM_USER + 5)

BOOL TrayIcon_AddIcon(VOID);
BOOL TrayIcon_RemoveIcon(VOID);
BOOL TrayIcon_UpdateIcon(VOID);
