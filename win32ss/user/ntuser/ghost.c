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

static UNICODE_STRING GhostClass = RTL_CONSTANT_STRING(GHOSTCLASSNAME);
static UNICODE_STRING GhostProp = RTL_CONSTANT_STRING(GHOST_PROP);

BOOL FASTCALL IntIsGhostWindow(HWND hWnd)
{
    BOOLEAN Ret = FALSE;
    UNICODE_STRING ClassName;
    INT Len;

    // check class name
    RtlInitUnicodeString(&ClassName, NULL);
    Len = NtUserGetClassName(hWnd, TRUE, &ClassName);
    if (Len)
    {
        Ret = RtlEqualUnicodeString(&ClassName, &GhostClass, TRUE);
    }
    RtlFreeUnicodeString(&ClassName);

    return Ret;
}

HWND APIENTRY NtUserGhostWindowFromHungWindow(HWND hwndHung)
{
    RTL_ATOM Atom;
    PWND pHungWnd;
    PPROPERTY Prop;

    pHungWnd = ValidateHwndNoErr(hwndHung);
    if (!pHungWnd)
        return NULL;

    IntGetAtomFromStringOrAtom(&GhostProp, &Atom);

    Prop = UserGetProp(pHungWnd, Atom, TRUE);
    if (Prop && ValidateHwndNoErr((HWND)Prop->Data))
        return (HWND)Prop->Data;

    return NULL;
}

HWND APIENTRY NtUserHungWindowFromGhostWindow(HWND hwndGhost)
{
    GHOST_DATA *UserData;
    PWND pGhostWnd;

    if (!IntIsGhostWindow(hwndGhost))
        return NULL;

    pGhostWnd = ValidateHwndNoErr(hwndGhost);
    if (!pGhostWnd)
        return NULL;

    UserData = (GHOST_DATA *)pGhostWnd->dwUserData;
    if (UserData && ValidateHwndNoErr(UserData->hwndTarget))
        return UserData->hwndTarget;

    return NULL;
}

BOOL FASTCALL IntMakeHungWindowGhosted(HWND hwndHung)
{
    PWND pHungWnd = ValidateHwndNoErr(hwndHung);
    if (!pHungWnd)
        return FALSE;   // not a window

    if (IntIsGhostWindow(hwndHung))
        return FALSE;   // ghost window cannot be being ghosted

    if (!MsqIsHung(pHungWnd->head.pti))
        return FALSE;   // not hung window

    if (!(pHungWnd->style & WS_VISIBLE))
        return FALSE;   // invisible

    if (pHungWnd->style & WS_CHILD)
        return FALSE;   // child

    if (NtUserGhostWindowFromHungWindow(hwndHung))
        return FALSE;   // already ghosting

    // TODO:
    // 1. Create a thread.
    // 2. Create a ghost window in the thread.
    // 3. Do message loop in the thread

    return FALSE;
}
