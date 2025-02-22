/*
 * PROJECT:     ReactOS user32.dll
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Window ghosting feature
 * COPYRIGHT:   Copyright 2018 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include <win32k.h>
#include "ghostwnd.h"

#define NDEBUG
#include <debug.h>

DBG_DEFAULT_CHANNEL(UserInput);

static UNICODE_STRING GhostClass = RTL_CONSTANT_STRING(GHOSTCLASSNAME);
static UNICODE_STRING GhostProp = RTL_CONSTANT_STRING(GHOST_PROP);

PTHREADINFO gptiGhostThread = NULL;

/*
 * GhostThreadMain
 *
 * Creates ghost windows and exits when no non-responsive window remains.
 */
VOID NTAPI
UserGhostThreadEntry(VOID)
{
    TRACE("Ghost thread started\n");

    UserEnterExclusive();

    gptiGhostThread = PsGetCurrentThreadWin32Thread();

    //TODO: Implement. This thread should handle all ghost windows and exit when no ghost window is needed.

    gptiGhostThread = NULL;

    UserLeave();
}

BOOL FASTCALL IntIsGhostWindow(PWND Window)
{
    BOOLEAN Ret = FALSE;
    UNICODE_STRING ClassName;
    INT iCls, Len;
    RTL_ATOM Atom = 0;

    if (!Window)
        return FALSE;

    if (Window->fnid && !(Window->fnid & FNID_DESTROY))
    {
        if (LookupFnIdToiCls(Window->fnid, &iCls))
        {
            Atom = gpsi->atomSysClass[iCls];
        }
    }

    // check class name
    RtlInitUnicodeString(&ClassName, NULL);
    Len = UserGetClassName(Window->pcls, &ClassName, Atom, FALSE);
    if (Len > 0)
    {
        Ret = RtlEqualUnicodeString(&ClassName, &GhostClass, TRUE);
    }
    else
    {
        DPRINT1("Unable to get class name\n");
    }
    RtlFreeUnicodeString(&ClassName);

    return Ret;
}

HWND FASTCALL IntGhostWindowFromHungWindow(PWND pHungWnd)
{
    RTL_ATOM Atom;
    HWND hwndGhost;

    if (!IntGetAtomFromStringOrAtom(&GhostProp, &Atom))
        return NULL;

    hwndGhost = UserGetProp(pHungWnd, Atom, TRUE);
    if (hwndGhost)
    {
        if (ValidateHwndNoErr(hwndGhost))
            return hwndGhost;

        DPRINT("Not a window\n");
    }

    return NULL;
}

HWND FASTCALL UserGhostWindowFromHungWindow(HWND hwndHung)
{
    PWND pHungWnd = ValidateHwndNoErr(hwndHung);
    if (!pHungWnd)
    {
        DPRINT("Not a window\n");
        return NULL;
    }
    return IntGhostWindowFromHungWindow(pHungWnd);
}

HWND FASTCALL IntHungWindowFromGhostWindow(PWND pGhostWnd)
{
    const GHOST_DATA *UserData;
    HWND hwndTarget;

    if (!IntIsGhostWindow(pGhostWnd))
    {
        DPRINT("Not a ghost window\n");
        return NULL;
    }

    UserData = (const GHOST_DATA *)pGhostWnd->dwUserData;
    if (UserData)
    {
        _SEH2_TRY
        {
            ProbeForRead(UserData, sizeof(GHOST_DATA), 1);
            hwndTarget = UserData->hwndTarget;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            DPRINT1("Exception!\n");
            hwndTarget = NULL;
        }
        _SEH2_END;
    }
    else
    {
        DPRINT("No user data\n");
        hwndTarget = NULL;
    }

    if (hwndTarget)
    {
        if (ValidateHwndNoErr(hwndTarget))
            return hwndTarget;

        DPRINT1("Not a window\n");
    }

    return NULL;
}

HWND FASTCALL UserHungWindowFromGhostWindow(HWND hwndGhost)
{
    PWND pGhostWnd = ValidateHwndNoErr(hwndGhost);
    return IntHungWindowFromGhostWindow(pGhostWnd);
}

BOOL FASTCALL IntMakeHungWindowGhosted(HWND hwndHung)
{
    PWND pHungWnd = ValidateHwndNoErr(hwndHung);
    if (!pHungWnd)
    {
        DPRINT1("Not a window\n");
        return FALSE;   // not a window
    }

    if (!MsqIsHung(pHungWnd->head.pti, MSQ_HUNG))
    {
        DPRINT1("Not hung window\n");
        return FALSE;   // not hung window
    }

    if (!(pHungWnd->style & WS_VISIBLE))
        return FALSE;   // invisible

    if (pHungWnd->style & WS_CHILD)
        return FALSE;   // child

    if (IntIsGhostWindow(pHungWnd))
    {
        DPRINT1("IntIsGhostWindow\n");
        return FALSE;   // ghost window cannot be ghosted
    }

    if (IntGhostWindowFromHungWindow(pHungWnd))
    {
        DPRINT("Already ghosting\n");
        return FALSE;   // already ghosting
    }

    // TODO: Find a way to pass the hwnd of pHungWnd to the ghost thread as we can't pass parameters directly

    if (!gptiGhostThread)
        UserCreateSystemThread(ST_GHOST_THREAD);

    return TRUE;
}
