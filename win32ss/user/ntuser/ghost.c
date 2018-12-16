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

typedef struct GHOST_INFO
{
    HWND hwndTarget;
    KEVENT GhostStartupEvent;
    KEVENT GhostQuitEvent;
    UINT_PTR ThreadId;
} GHOST_INFO;

static GHOST_INFO *gGhostInfo = NULL;

// Private message PM_CREATE_GHOST:
//   wParam: HWND. The target window (hung).
//   lParam: VOID. Ignored.
#define PM_CREATE_GHOST     (WM_APP + 1)

BOOL IntCreateGhostWindow(HWND hwndTarget)
{
    RTL_ATOM Atom;
    DWORD style, exstyle;
    RECT rc;
    PWND pTargetWnd, pGhostWnd;
    CREATESTRUCTW Cs;
    LARGE_STRING WindowName;
    PCLS Class;
    PTHREADINFO pti;

    // get atom for ghost prop
    if (!IntGetAtomFromStringOrAtom(&GhostProp, &Atom))
        return FALSE;

    pTargetWnd = ValidateHwndNoErr(hwndTarget);
    if (!pTargetWnd)
        return FALSE;

    // get target info
    style = pTargetWnd->style;
    exstyle = pTargetWnd->ExStyle;
    IntGetWindowRect(pTargetWnd, &rc);

    // initialize CREATESTRUCT
    RtlZeroMemory(&Cs, sizeof(Cs));
    Cs.lpCreateParams = hwndTarget;
    Cs.hInstance = hModClient;
    Cs.hMenu = NULL;
    Cs.hwndParent = NULL;
    Cs.cy = rc.bottom - rc.top;
    Cs.cx = rc.right - rc.left;
    Cs.y = rc.top;
    Cs.x = rc.left;
    Cs.style = style;
    Cs.lpszName = NULL;
    Cs.lpszClass = GHOSTCLASSNAME;
    Cs.dwExStyle = exstyle;

    // WindowName
    RtlZeroMemory(&WindowName, sizeof(WindowName));

    // get the class and reference it
    Class = IntGetAndReferenceClass(&GhostClass, Cs.hInstance, FALSE);
    if (!Class)
    {
        DPRINT1("Failed to find class %wZ\n", &GhostClass);
        return FALSE;
    }

    // create the ghost
    pGhostWnd = IntCreateWindow(&Cs, &WindowName, Class, NULL, NULL, NULL, NULL);
    if (!pGhostWnd)
    {
        DPRINT1("Failed to create ghost\n");
        return FALSE;
    }

    // dereference the class
    pti = GetW32ThreadInfo();
    IntDereferenceClass(Class, pti->pDeskInfo, pti->ppi);

    // set ghost prop
    return UserSetProp(pTargetWnd, Atom, pGhostWnd->head.h, TRUE);
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

BOOL IntResumeGhostedWindow(HWND hwndTarget, BOOL bDestroyTarget)
{
    HWND hwndGhost = UserGhostWindowFromHungWindow(hwndTarget);

    return co_IntSendMessage(hwndGhost, GWM_UNGHOST, bDestroyTarget, 0);
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

BOOL IntHaveToQuitGhosting(void)
{
    RTL_ATOM ClassAtom;
    HWND hwndDesktop;
    PWND pDesktopWnd;
    UNICODE_STRING WindowName;

    if (!IntGetAtomFromStringOrAtom(&GhostClass, &ClassAtom))
        return FALSE;

    hwndDesktop = IntGetCurrentThreadDesktopWindow();
    pDesktopWnd = ValidateHwndNoErr(hwndDesktop);
    if (!pDesktopWnd)
        return FALSE;

    RtlInitUnicodeString(&WindowName, NULL);

    return IntFindWindow(pDesktopWnd, NULL, ClassAtom, &WindowName) != NULL;
}

VOID NTAPI
GhostThreadProc(_In_ PVOID StartContext)
{
    HWND hwndTarget = StartContext;
    MSG msg;

    if (!IntCreateGhostWindow(hwndTarget))
    {
        DPRINT1("Unable to create ghost\n");
        goto Quit;
    }

    // thread message loop
    for (;;)
    {
        RtlZeroMemory(&msg, sizeof(MSG));

        if (!co_IntGetPeekMessage(&msg, NULL, 0, 0, PM_REMOVE, TRUE))
            break;

        switch (msg.message)
        {
            case PM_CREATE_GHOST:
            {
                hwndTarget = (HWND)msg.wParam;
                if (!IntCreateGhostWindow(hwndTarget))
                {
                    DPRINT1("Unable to create ghost\n");
                }
                break;
            }
        }

        IntTranslateKbdMessage(&msg, 0);
        IntDispatchMessage(&msg);

        if (IntHaveToQuitGhosting())
        {
            DPRINT1("Have to quit ghost thread\n");
            break;
        }
    }

Quit:
    KeSetEvent(&gGhostInfo->GhostQuitEvent, IO_NO_INCREMENT, FALSE);

    ExFreePoolWithTag(gGhostInfo, USERTAG_GHOST);
    gGhostInfo = NULL;
}

BOOL IntAddGhost(HWND hwndTarget)
{
    NTSTATUS Status;
    CLIENT_ID ClientId;
    HANDLE GhostThreadHandle;
    static OBJECT_ATTRIBUTES ObjectAttributes =
        RTL_CONSTANT_OBJECT_ATTRIBUTES(NULL, OBJ_KERNEL_HANDLE);

    if (gGhostInfo)
    {
        return NtUserPostThreadMessage(gGhostInfo->ThreadId, PM_CREATE_GHOST,
                                       (WPARAM)hwndTarget, 0);
    }

    gGhostInfo = ExAllocatePoolWithTag(NonPagedPool, sizeof(GHOST_INFO), USERTAG_GHOST);
    if (!gGhostInfo)
        return FALSE;

    RtlZeroMemory(gGhostInfo, sizeof(*gGhostInfo));
    gGhostInfo->hwndTarget = hwndTarget;

    // create events
    KeInitializeEvent(&gGhostInfo->GhostStartupEvent, SynchronizationEvent, FALSE);
    KeInitializeEvent(&gGhostInfo->GhostQuitEvent, SynchronizationEvent, FALSE);

    // create thread
    Status = PsCreateSystemThread(&GhostThreadHandle,
                                  STANDARD_RIGHTS_ALL,
                                  &ObjectAttributes,
                                  NULL,
                                  &ClientId,
                                  GhostThreadProc,
                                  hwndTarget);
    DPRINT("Thread creation GhostThreadHandle = 0x%p, GhostThreadId = 0x%p, Status = 0x%08lx\n",
           GhostThreadHandle, ClientId.UniqueThread, Status);

    // wait for start up
    NtWaitForSingleObject(&gGhostInfo->GhostStartupEvent, FALSE, NULL);

    // set the thread ID
    gGhostInfo->ThreadId = PtrToUint(ClientId.UniqueThread);

    // close thread handle
    ZwClose(GhostThreadHandle);

    return TRUE;
}

BOOL FASTCALL IntMakeHungWindowGhosted(HWND hwndHung)
{
    PWND pHungWnd = ValidateHwndNoErr(hwndHung);
    if (!pHungWnd)
    {
        DPRINT1("Not a window\n");
        return FALSE;   // not a window
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

    return IntAddGhost(hwndHung);
}
