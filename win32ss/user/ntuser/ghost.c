/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Ghost window
 * FILE:             win32ss/user/ntuser/ghost.c
 * PROGRAMER:        Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
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
