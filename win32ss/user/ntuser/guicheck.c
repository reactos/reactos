/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          GUI state check
 * FILE:             win32ss/user/ntuser/guicheck.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * NOTES:            The GuiCheck() function performs a few delayed operations:
 *                   1) A GUI process is assigned a window station
 *                   2) A message queue is created for a GUI thread before use
 *                   3) The system window classes are registered for a process
 */

#include <win32k.h>

/* GLOBALS *******************************************************************/

static LONG NrGuiAppsRunning = 0;

/* FUNCTIONS *****************************************************************/

static BOOL FASTCALL
co_AddGuiApp(PPROCESSINFO W32Data)
{
    W32Data->W32PF_flags |= W32PF_CREATEDWINORDC;
    if (InterlockedIncrement(&NrGuiAppsRunning) == 1)
    {
        BOOL Initialized;

        Initialized = co_IntInitializeDesktopGraphics();

        if (!Initialized)
        {
            W32Data->W32PF_flags &= ~W32PF_CREATEDWINORDC;
            InterlockedDecrement(&NrGuiAppsRunning);
            return FALSE;
        }
    }
    return TRUE;
}

static void FASTCALL
RemoveGuiApp(PPROCESSINFO W32Data)
{
    W32Data->W32PF_flags &= ~W32PF_CREATEDWINORDC;
    if (InterlockedDecrement(&NrGuiAppsRunning) == 0)
    {
        IntEndDesktopGraphics();
    }
}

BOOL FASTCALL
co_IntGraphicsCheck(BOOL Create)
{
    PPROCESSINFO W32Data;

    W32Data = PsGetCurrentProcessWin32Process();
    if (Create)
    {
        if (!(W32Data->W32PF_flags & W32PF_CREATEDWINORDC) && !(W32Data->W32PF_flags & W32PF_MANUALGUICHECK))
        {
            return co_AddGuiApp(W32Data);
        }
    }
    else
    {
        if ((W32Data->W32PF_flags & W32PF_CREATEDWINORDC) && !(W32Data->W32PF_flags & W32PF_MANUALGUICHECK))
        {
            RemoveGuiApp(W32Data);
        }
    }

    return TRUE;
}

VOID
FASTCALL
co_IntUserManualGuiCheck(BOOL Create)
{
    PPROCESSINFO W32Data = (PPROCESSINFO)PsGetCurrentProcessWin32Process();
    W32Data->W32PF_flags |= W32PF_MANUALGUICHECK;

    if (Create)
    {
        co_AddGuiApp(W32Data);
    }
    else
    {
        RemoveGuiApp(W32Data);
    }
}

/* EOF */
