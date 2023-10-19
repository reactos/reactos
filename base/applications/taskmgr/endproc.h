/*
 * PROJECT:   ReactOS Task Manager
 * LICENSE:   LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * COPYRIGHT: 1999-2001 Brian Palmer <brianp@reactos.org>
 */

#pragma once

void ProcessPage_OnEndProcess(void);
BOOL IsCriticalProcess(HANDLE hProcess);
void ProcessPage_OnEndProcessTree(void);
