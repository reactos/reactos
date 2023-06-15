/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Tray icon
 * COPYRIGHT:   Copyright (C) 1999-2001 Brian Palmer <brianp@reactos.org>
 */

#pragma once

#define WM_ONTRAYICON   WM_USER + 5

HICON	TrayIcon_GetProcessorUsageIcon(void);
BOOL	TrayIcon_ShellAddTrayIcon(void);
BOOL	TrayIcon_ShellRemoveTrayIcon(void);
BOOL	TrayIcon_ShellUpdateTrayIcon(void);
