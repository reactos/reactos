/*
 * PROJECT:   ReactOS Task Manager
 * LICENSE:   LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * COPYRIGHT: 1999-2001 Brian Palmer <brianp@reactos.org>
 */

#pragma once

#define WM_ONTRAYICON   (WM_USER + 5)

BOOL TrayIcon_AddIcon(void);
BOOL TrayIcon_RemoveIcon(void);
BOOL TrayIcon_UpdateIcon(void);
