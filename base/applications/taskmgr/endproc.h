/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Process Termination.
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 *              Copyright 2005 Klemens Friedl <frik85@reactos.at>
 *              Copyright 2014 Ismael Ferreras Morezuelas <swyterzone+ros@gmail.com>
 */

#pragma once

void ProcessPage_OnEndProcess(void);
BOOL IsCriticalProcess(HANDLE hProcess);
void ProcessPage_OnEndProcessTree(void);
