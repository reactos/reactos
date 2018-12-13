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
    HANDLE GhostStartupEvent;
    HANDLE GhostQuitEvent;
    ULONG_PTR InputThreadId;
} GHOST_INFO;

static GHOST_INFO *gGhostInfo = NULL;

// Private message PM_CREATE_GHOST:
//   wParam: HWND. The target window (hung).
//   lParam: VOID. Ignored.
#define PM_CREATE_GHOST     (WM_APP + 1)

// Private message PM_DESTROY_GHOST:
//   wParam: HWND. The target window.
//   lParam: BOOL. Whether the target is to be destroyed.
#define PM_DESTROY_GHOST    (WM_APP + 2)

BOOL IntCreateGhost(HWND hwndTarget)
{
    RTL_ATOM Atom;
    DWORD style, exstyle;
    RECT rc;
    HWND hwndGhost;
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
    GetWindowRect(hwndTarget, &rc);

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
    Class = IntGetAndReferenceClass(&GhostClass, Cs->hInstance, FALSE);
    if (!Class)
    {
        ERR("Failed to find class %wZ\n", ClassName);
        goto cleanup;
    }

    // create the ghost
    pGhostWnd = IntCreateWindow(&Cs, &WindowName, Class, NULL, NULL, NULL, NULL);
    if (!pGhostWnd)
    {
        ERR("Failed to create ghost\n");
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

    return SendMessageW(hwndGhost, GWM_UNGHOST, hwndTarget, bDestroyTarget);
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
    UNICODE_STRING WindowName;

    RtlInitUnicodeString(&WindowName, NULL);
    if (!IntGetAtomFromStringOrAtom(&GhostClass, &ClassAtom))
        return FALSE;

    return !IntFindWindow(IntGetDesktopWindow(), NULL, ClassAtom, &WindowName);
}

static ULONG NTAPI
GhostThreadProc(PVOID Param)
{
    HWND hwndTarget = (GHOST_INFO *)Param;
    PCSR_THREAD pcsrt;
    HANDLE hThread;
    MSG msg;

    pcsrt = CsrConnectToUser();
    if (!pcsrt)
        goto Quit;

    hThread = pcsrt->ThreadHandle;

    if (!IntCreateGhost(hwndTarget))
    {
        DPRINT1("Unable to create ghost\n");
        goto Quit;
    }

    // thread message loop
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        switch (msg.message)
        {
            case PM_CREATE_GHOST:
            {
                hwndTarget = (HWND)msg.wParam;
                if (!IntCreateGhost(hwndTarget))
                {
                    DPRINT1("Unable to create ghost\n");
                }
                break;
            }
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);

        if (IntHaveToQuitGhosting())
        {
            DPRINT("Have to quit ghost thread\n");
            break;
        }
    }

Quit:
    if (pcsrt)
    {
        if (hThread != pcsrt->ThreadHandle)
            DPRINT1("WARNING!! hThread (0x%p) != pcsrt->ThreadHandle (0x%p), you may expect crashes soon!!\n", hThread, pcsrt->ThreadHandle)
        CsrDereferenceThread(pcsrt);
    }

    RtlFreeHeap(CsrHeap, 0, gGhostInfo);

    NtSetEvent(gGhostInfo->GhostQuitEvent, NULL);

    gGhostInfo = NULL;

    RtlExitUserThread(Status);

    return 0;
}

BOOL IntInitGhostFeature(HWND hwndTarget)
{
    NTSTATUS Status;
    CLIENT_ID ClientId;
    HANDLE GhostThreadHandle;

    if (gGhostInfo)
        return FALSE;

    gGhostInfo = RtlAllocateHeap(CsrHeap, HEAP_ZERO_MEMORY, sizeof(GHOST_INFO));
    if (!gGhostInfo)
        return FALSE;

    gGhostInfo->hwndTarget = hwndTarget;

    // create events
    Status = NtCreateEvent(&gGhostInfo->GhostStartupEvent, EVENT_ALL_ACCESS,
                           NULL, SynchronizationEvent, FALSE);
    Status = NtCreateEvent(&gGhostInfo->GhostQuitEvent, EVENT_ALL_ACCESS,
                           NULL, SynchronizationEvent, FALSE);

    Status = RtlCreateUserThread(NtCurrentProcess(),
                                 NULL,
                                 TRUE, // Start the thread in suspended state
                                 0,
                                 0,
                                 0,
                                 GhostThreadProc,
                                 hwndTarget,
                                 &GhostThreadHandle,
                                 &ClientId);
    if (NT_SUCCESS(Status))
    {
        /* Add it as a static server thread and resume it */
        CsrAddStaticServerThread(hGhostThread, &ClientId, 0);
        Status = NtResumeThread(hGhostThread, NULL);
    }
    DPRINT("Thread creation hGhostThread = 0x%p, GhostThreadId = 0x%p, Status = 0x%08lx\n",
           hGhostThread, ClientId.UniqueThread, Status);

    gGhostInfo->InputThreadId = (ULONG_PTR)ClientId.UniqueThread;

    NtWaitForSingleObject(gGhostInfo->GhostStartupEvent, FALSE, NULL);
    NtClose(gGhostInfo->GhostStartupEvent);
    gGhostInfo->GhostStartupEvent = NULL;

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

    if (gGhostInfo == NULL)
    {
        return IntInitGhostFeature(hwndHung);
    }
    else
    {
        return NtUserPostThreadMessage(gGhostInfo->InputThreadId,
                                       PM_CREATE_GHOST, (WPARAM)hwndTarget, 0);
    }
}
