/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Definitions necessary to use Microsoft's "Run" dialog
 * COPYRIGHT:   Copyright (C) 1999-2001 Brian Palmer <brianp@reactos.org>
 *              Copyright (C) 2005 Klemens Friedl <frik85@reactos.at>
 */

#pragma once

void TaskManager_OnFileNew(void);

typedef void (WINAPI *RUNFILEDLG)(
HWND    hwndOwner,
HICON   hIcon,
LPCWSTR lpstrDirectory,
LPCWSTR lpstrTitle,
LPCWSTR lpstrDescription,
UINT    uFlags);

/*
 * Flags for RunFileDlg
 */
#define RFF_NOBROWSE        0x01    /* Removes the browse button. */
#define RFF_NODEFAULT       0x02    /* No default item selected. */
#define RFF_CALCDIRECTORY   0x04    /* Calculates the working directory from the file name. */
#define RFF_NOLABEL         0x08    /* Removes the edit box label. */
#define RFF_NOSEPARATEMEM   0x20    /* Removes the Separate Memory Space check box (Windows NT only). */
