/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Tray Icon.
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 *              Copyright 2005 Klemens Friedl <frik85@reactos.at>
 */

#pragma once

#define WM_ONTRAYICON   (WM_USER + 5)

BOOL TrayIcon_AddIcon(VOID);
BOOL TrayIcon_RemoveIcon(VOID);
BOOL TrayIcon_UpdateIcon(VOID);
