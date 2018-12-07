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

BOOL FASTCALL LookupFnIdToiCls(int FnId, int *iCls);

INT
UserGetClassName(IN PCLS Class,
                 IN OUT PUNICODE_STRING ClassName,
                 IN RTL_ATOM Atom,
                 IN BOOL Ansi);

static UNICODE_STRING GhostClass = RTL_CONSTANT_STRING(GHOSTCLASSNAME);
static UNICODE_STRING GhostProp = RTL_CONSTANT_STRING(GHOST_PROP);

BOOL FASTCALL IntIsGhostWindow(HWND hWnd)
{
    BOOLEAN Ret = FALSE;
    UNICODE_STRING ClassName;
    INT iCls, Len;
    RTL_ATOM Atom = 0;
    PWND Window = UserGetWindowObject(hWnd);
    if (!Window)
        return FALSE;   // not a window

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
    if (Len)
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

HWND APIENTRY NtUserGhostWindowFromHungWindow(HWND hwndHung)
{
    RTL_ATOM Atom;
    PWND pHungWnd;
    PPROPERTY Prop;
    HWND hwndGhost;

    pHungWnd = ValidateHwndNoErr(hwndHung);
    if (!pHungWnd)
    {
        DPRINT("Not a window\n");
        return NULL;
    }

    if (!IntGetAtomFromStringOrAtom(&GhostProp, &Atom))
        ASSERT(FALSE);

    Prop = UserGetProp(pHungWnd, Atom, TRUE);

    _SEH2_TRY
    {
        hwndGhost = (HWND)(Prop ? Prop->Data : NULL);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT1("Exception!\n");
        hwndGhost = NULL;
    }
    _SEH2_END;

    if (hwndGhost)
    {
        if (ValidateHwndNoErr(hwndGhost))
            return hwndGhost;

        DPRINT("Not a window\n");
    }

    return NULL;
}

HWND APIENTRY NtUserHungWindowFromGhostWindow(HWND hwndGhost)
{
    GHOST_DATA *UserData;
    PWND pGhostWnd;
    HWND hwndTarget;

    if (!IntIsGhostWindow(hwndGhost))
    {
        DPRINT("Not a ghost window\n");
        return NULL;
    }

    pGhostWnd = ValidateHwndNoErr(hwndGhost);
    if (!pGhostWnd)
    {
        DPRINT("Not a window\n");
        return NULL;
    }

    UserData = (GHOST_DATA *)pGhostWnd->dwUserData;
    if (UserData)
    {
        _SEH2_TRY
        {
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

BOOL FASTCALL IntMakeHungWindowGhosted(HWND hwndHung)
{
    PWND pHungWnd = ValidateHwndNoErr(hwndHung);
    if (!pHungWnd)
    {
        DPRINT1("Not a window\n");
        return FALSE;   // not a window
    }

    if (IntIsGhostWindow(hwndHung))
    {
        DPRINT1("IntIsGhostWindow\n");
        return FALSE;   // ghost window cannot be being ghosted
    }

    if (!MsqIsHung(pHungWnd->head.pti))
    {
        DPRINT1("Not hung window\n");
        return FALSE;   // not hung window
    }

    if (!(pHungWnd->style & WS_VISIBLE))
        return FALSE;   // invisible

    if (pHungWnd->style & WS_CHILD)
        return FALSE;   // child

    if (NtUserGhostWindowFromHungWindow(hwndHung))
    {
        DPRINT("Already ghosting\n");
        return FALSE;   // already ghosting
    }

    // TODO:
    // 1. Create a thread.
    // 2. Create a ghost window in the thread.
    // 3. Do message loop in the thread

    return FALSE;
}
