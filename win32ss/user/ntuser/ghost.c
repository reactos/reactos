/*
 * PROJECT:     ReactOS user32.dll
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Ghost window handling
 * COPYRIGHT:   Copyright 2018 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include <win32k.h>

BOOL FASTCALL IntGoGhost(PWND Window, BOOL bGo)
{
    // TODO:
    // 1. Create a thread.
    // 2. Create a ghost window in the thread.
    // 3. Do message loop in the thread
    STUB;
    return FALSE;
}
