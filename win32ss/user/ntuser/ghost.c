/*
 * PROJECT:     ReactOS user32.dll
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Window ghosting feature
 * COPYRIGHT:   Copyright 2018 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include <win32k.h>
#include "ghostwnd.h"

DBG_DEFAULT_CHANNEL(UserInput);

static UNICODE_STRING GhostClass = RTL_CONSTANT_STRING(GHOSTCLASSNAME);
static UNICODE_STRING TrayWndClass = RTL_CONSTANT_STRING(L"Shell_TrayWnd");
static UNICODE_STRING ProgmanWndClass = RTL_CONSTANT_STRING(L"Progman");

PTHREADINFO gptiGhostThread = NULL;
PKEVENT gpGhostStartupEvent = NULL;

// linked list node
typedef struct GHOST_ENTRY
{
    LIST_ENTRY ListEntry;
    HWND hwndTarget;
    HWND hwndGhost;
} GHOST_ENTRY, *PGHOST_ENTRY;

LIST_ENTRY gGhostListHead = { &gGhostListHead, &gGhostListHead };

PGHOST_ENTRY FASTCALL
IntFindGhostEntryFromTargetWindow(HWND hwndTarget)
{
    PLIST_ENTRY CurrentEntry, ListHead = &gGhostListHead;
    PGHOST_ENTRY pEntry;

    for (CurrentEntry = ListHead->Flink;
         CurrentEntry != ListHead;
         CurrentEntry = CurrentEntry->Flink)
    {
        pEntry = CONTAINING_RECORD(CurrentEntry, GHOST_ENTRY, ListEntry);

        if (pEntry->hwndTarget == hwndTarget)
            return pEntry;
    }

    return NULL;
}

PGHOST_ENTRY FASTCALL
IntFindGhostEntryFromGhostWindow(HWND hwndGhost)
{
    PLIST_ENTRY CurrentEntry, ListHead = &gGhostListHead;
    PGHOST_ENTRY pEntry;

    for (CurrentEntry = ListHead->Flink;
         CurrentEntry != ListHead;
         CurrentEntry = CurrentEntry->Flink)
    {
        pEntry = CONTAINING_RECORD(CurrentEntry, GHOST_ENTRY, ListEntry);

        if (pEntry->hwndGhost == hwndGhost)
            return pEntry;
    }

    return NULL;
}

__inline PGHOST_ENTRY FASTCALL
IntAddGhostEntry(HWND hwndTarget, HWND hwndGhost)
{
    PGHOST_ENTRY pEntry;
    pEntry = ExAllocatePoolWithTag(NonPagedPool, sizeof(*pEntry), USERTAG_GHOST);
    if (!pEntry)
    {
        ERR("ExAllocatePoolWithTag failed\n");
        return NULL;
    }

    pEntry->hwndTarget = hwndTarget;
    pEntry->hwndGhost = hwndGhost;

    InsertTailList(&gGhostListHead, &pEntry->ListEntry);
    return pEntry;
}

__inline BOOL FASTCALL
IntDeleteGhostEntry(HWND hwndTarget)
{
    PGHOST_ENTRY pEntry = IntFindGhostEntryFromTargetWindow(hwndTarget);
    if (!pEntry)
    {
        ERR("Ghost entry not found.\n");
        return FALSE;
    }
    RemoveEntryList(&pEntry->ListEntry);
    ExFreePoolWithTag(pEntry, USERTAG_GHOST);
    return TRUE;
}

BOOL FASTCALL IntCheckWindowClass(PWND Window, PUNICODE_STRING pClass)
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
        Ret = RtlEqualUnicodeString(&ClassName, pClass, TRUE);
    }
    else
    {
        ERR("Unable to get class name\n");
    }
    RtlFreeUnicodeString(&ClassName);

    return Ret;
}

__inline BOOL FASTCALL IntIsShellTrayWnd(PWND Window)
{
    return IntCheckWindowClass(Window, &TrayWndClass);
}

__inline BOOL FASTCALL IntIsProgmanWnd(PWND Window)
{
    return IntCheckWindowClass(Window, &ProgmanWndClass);
}

__inline BOOL FASTCALL IntIsGhostWindow(PWND Window)
{
    return IntCheckWindowClass(Window, &GhostClass);
}

// create a ghost window for the target
PGHOST_ENTRY FASTCALL IntCreateGhostWindowAndAddToList(HWND hwndTarget)
{
    DWORD style, exstyle;
    RECT rc;
    PWND pTargetWnd, pGhostWnd = NULL;
    CREATESTRUCTW Cs;
    LARGE_STRING WindowName;
    PTHREADINFO ptiCurrent;
    PGHOST_ENTRY pEntry = NULL;
    BOOL NoHooks = FALSE;

    ptiCurrent = PsGetCurrentThreadWin32Thread();
    if (!ptiCurrent || !gptiDesktopThread)
    {
        ERR("!ptiCurrent || !gptiDesktopThread\n");
        return NULL;
    }

    if (IntFindGhostEntryFromTargetWindow(hwndTarget) != NULL)
    {
        ERR("Already ghosting: %p.\n", (void *)hwndTarget);
        return NULL;
    }

    // turn off hooks temporarily
    NoHooks = (ptiCurrent->TIF_flags & TIF_DISABLEHOOKS) != 0;
    ptiCurrent->TIF_flags |= TIF_DISABLEHOOKS;
    ptiCurrent->pClientInfo->dwTIFlags = ptiCurrent->TIF_flags;

    // validate window
    pTargetWnd = ValidateHwndNoErr(hwndTarget);
    if (!pTargetWnd)
    {
        ERR("ValidateHwndNoErr: %p\n", (void *)hwndTarget);
        goto Quit;
    }

    // get target info
    style = pTargetWnd->style;
    exstyle = pTargetWnd->ExStyle;
    IntGetWindowRect(pTargetWnd, &rc);
    if (rc.left >= rc.right || rc.top >= rc.bottom)
    {
        ERR("Rect empty: %p\n", (void *)hwndTarget);
        goto Quit;
    }

    // don't use scrollbars.
    style &= ~(WS_HSCROLL | WS_VSCROLL | WS_VISIBLE);

    // initialize CREATESTRUCT
    RtlZeroMemory(&Cs, sizeof(Cs));
    Cs.lpCreateParams = hwndTarget;
    Cs.hInstance = NULL;
    Cs.hMenu = NULL;
    Cs.hwndParent = NULL;
    Cs.cy = rc.bottom - rc.top;
    Cs.cx = rc.right - rc.left;
    Cs.y = rc.top;
    Cs.x = rc.left;
    Cs.style = style;
    Cs.lpszName = NULL;
    Cs.lpszClass = GhostClass.Buffer;
    Cs.dwExStyle = exstyle;

    // WindowName
    RtlZeroMemory(&WindowName, sizeof(WindowName));

    // create the ghost
    pGhostWnd = co_UserCreateWindowEx(&Cs, &GhostClass, &WindowName, NULL);
    if (!pGhostWnd)
    {
        ERR("Failed to create ghost for %p\n", (void *)hwndTarget);
        goto Quit;
    }

    // add to list
    pEntry = IntAddGhostEntry(hwndTarget, pGhostWnd->head.h);

Quit:
    if (!pEntry && pGhostWnd)
    {
        // destroy ghost
        co_UserDestroyWindow(pGhostWnd);
    }

    // resume hooks
    if (!NoHooks)
    {
        ptiCurrent->TIF_flags &= ~TIF_DISABLEHOOKS;
        ptiCurrent->pClientInfo->dwTIFlags = ptiCurrent->TIF_flags;
    }

    return pEntry;
}

BOOL FASTCALL
IntGhostOnDestroy(HWND hwndGhost, HWND hwndTarget)
{
    PWND pGhostWnd;

    pGhostWnd = ValidateHwndNoErr(hwndGhost);
    if (!pGhostWnd)
    {
        ERR("Not a window (%p, %p).\n", (void *)hwndTarget, (void *)hwndGhost);
        return FALSE;
    }

    if (!IntIsGhostWindow(pGhostWnd))
    {
        ERR("Not a Ghost window (%p, %p)\n", (void *)hwndTarget, (void *)hwndGhost);
        return FALSE;
    }

    return IntDeleteGhostEntry(hwndTarget);
}

VOID FASTCALL IntFreeGhostList(VOID)
{
    PLIST_ENTRY CurrentEntry, NextEntry, ListHead = &gGhostListHead;
    PGHOST_ENTRY pEntry;

    for (CurrentEntry = ListHead->Flink;
         CurrentEntry != ListHead;
         CurrentEntry = NextEntry)
    {
        pEntry = CONTAINING_RECORD(CurrentEntry, GHOST_ENTRY, ListEntry);
        NextEntry = CurrentEntry->Flink;
        ExFreePoolWithTag(pEntry, USERTAG_GHOST);
    }

    InitializeListHead(&gGhostListHead);
}

/*
 * GhostThreadMain
 *
 * Creates ghost windows and exits when no non-responsive window remains.
 */
VOID NTAPI
UserGhostThreadEntry(VOID)
{
    // This thread should handle all ghost windows and exit when no ghost window is needed.
    MSG msg;
    HDESK hDesk;

    TRACE("UserGhostThreadEntry: started.\n");

    ASSERT(!UserIsEntered());
    UserEnterExclusive();

    // attach thread to desktop
    hDesk = UserOpenInputDesktop(0, FALSE, DESKTOP_ALL_ACCESS);
    IntSetThreadDesktop(hDesk, FALSE);

    // get a PTHREADINFO
    FIXME("We're missing a thread lock before this line.\n");
    gptiGhostThread = PsGetCurrentThreadWin32Thread();

    // set event
    KeSetEvent(gpGhostStartupEvent, IO_NO_INCREMENT, FALSE);

    // thread message loop
    while (!IsListEmpty(&gGhostListHead))
    {
        RtlZeroMemory(&msg, sizeof(MSG));

        if (!co_IntGetPeekMessage(&msg, NULL, 0, 0, PM_REMOVE, TRUE))
        {
            IntFreeGhostList();
            break;
        }

        switch (msg.message)
        {
            case GTM_CREATE_GHOST:
            {
                HWND hwndTarget = (HWND)msg.wParam;     // not trusted yet
                HWND hwndGhost = (HWND)msg.lParam;      // not trusted yet

                TRACE("GTM_CREATE_GHOST: %p\n", (void *)hwndTarget, (void *)hwndGhost);
                break;
            }

            case GTM_GHOST_DESTROYED:
            {
                HWND hwndTarget = (HWND)msg.wParam;     // not trusted yet
                HWND hwndGhost = (HWND)msg.lParam;      // not trusted yet

                TRACE("GTM_GHOST_DESTROYED: %p, %p\n",
                      (void *)hwndTarget, (void *)hwndGhost);

                IntGhostOnDestroy(hwndGhost, hwndTarget);
                break;
            }
        }

        IntDispatchMessage(&msg);
    }

    gptiGhostThread = NULL;

    if (gpGhostStartupEvent)
    {
        // delete gpGhostStartupEvent
        ExFreePoolWithTag(gpGhostStartupEvent, USERTAG_GHOST);
        gpGhostStartupEvent = NULL;
    }

    UserLeave();

    TRACE("UserGhostThreadEntry: ended.\n");
}

HWND FASTCALL IntGhostWindowFromHungWindow(PWND pHungWnd)
{
    PGHOST_ENTRY pEntry = IntFindGhostEntryFromTargetWindow(pHungWnd->head.h);
    if (!pEntry)
    {
        ERR("Not ghost entry found\n");
        return NULL;
    }
    return pEntry->hwndGhost;
}

HWND FASTCALL UserGhostWindowFromHungWindow(HWND hwndHung)
{
    PWND pHungWnd = ValidateHwndNoErr(hwndHung);
    if (!pHungWnd)
    {
        ERR("Not a window: %p\n", (void *)hwndHung);
        return NULL;
    }
    return IntGhostWindowFromHungWindow(pHungWnd);
}

HWND FASTCALL IntHungWindowFromGhostWindow(PWND pGhostWnd)
{
    PGHOST_ENTRY pEntry;

    if (!IntIsGhostWindow(pGhostWnd))
    {
        ERR("Not a ghost window: %p\n", (void *)pGhostWnd->head.h);
        return NULL;
    }

    pEntry = IntFindGhostEntryFromGhostWindow(pGhostWnd->head.h);
    if (!pEntry)
    {
        ERR("Not ghost entry found: %p\n", (void *)pGhostWnd->head.h);
        return NULL;
    }

    return pEntry->hwndTarget;
}

HWND FASTCALL UserHungWindowFromGhostWindow(HWND hwndGhost)
{
    PWND pGhostWnd = ValidateHwndNoErr(hwndGhost);
    return IntHungWindowFromGhostWindow(pGhostWnd);
}

BOOL FASTCALL IntMakeHungWindowGhosted(HWND hwndHung)
{
    PWND pHungWnd;
    NTSTATUS Status;
    PGHOST_ENTRY pEntry;

    TRACE("IntMakeHungWindowGhosted(hwndHung: %p)\n", (void *)hwndHung);

    if (!gptiDesktopThread || !gpdeskInputDesktop)
    {
        ERR("Desktop not initialized yet (hwndHung: %p).\n", (void *)hwndHung);
        return FALSE;
    }

    if (IntFindGhostEntryFromTargetWindow(hwndHung) != NULL)
    {
        ERR("Already ghosting (hwndHung: %p)\n", (void *)hwndHung);
        return FALSE;   // already ghosting
    }

    pHungWnd = ValidateHwndNoErr(hwndHung);
    if (!pHungWnd)
    {
        ERR("Not a window (hwndHung: %p)\n", (void *)hwndHung);
        return FALSE;   // not a window
    }

    if (!MsqIsHung(pHungWnd->head.pti))
    {
        ERR("Not hung window (hwndHung: %p)\n", (void *)hwndHung);
        return FALSE;   // not hung window
    }

    if (!(pHungWnd->style & WS_VISIBLE))
        return FALSE;   // invisible

    if (pHungWnd->style & WS_CHILD)
        return FALSE;   // child

    if (IntIsGhostWindow(pHungWnd))
    {
        ERR("IntIsGhostWindow (hwndHung: %p)\n", (void *)hwndHung);
        return FALSE;   // ghost window cannot be ghosted
    }

    // "Shell_TrayWnd" is a special window for Taskbar.
    if (IntIsShellTrayWnd(pHungWnd))
    {
        TRACE("IntIsShellTrayWnd (hwndHung: %p)\n", (void *)hwndHung);
        return FALSE;   // Shell_TrayWnd
    }

    // "Progman" is a special window for Desktop.
    if (IntIsProgmanWnd(pHungWnd))
    {
        TRACE("IntIsProgmanWnd (hwndHung: %p)\n", (void *)hwndHung);
        return FALSE;   // Progman
    }

    // create ghost window and add to list
    pEntry = IntCreateGhostWindowAndAddToList(hwndHung);
    if (!pEntry)
    {
        ERR("IntCreateGhostWindowAndAddToList failed: %p\n", (void *)hwndHung);
        return FALSE;
    }

    if (!gptiGhostThread)
    {
        if (gpGhostStartupEvent == NULL)
        {
            // create event
            gpGhostStartupEvent = ExAllocatePoolWithTag(NonPagedPool,
                                                        sizeof(KEVENT),
                                                        USERTAG_GHOST);
            if (!gpGhostStartupEvent)
            {
                ERR("Unable to allocate event\n");
                IntDeleteGhostEntry(hwndHung);
                return FALSE;   // creation failed
            }
            KeInitializeEvent(gpGhostStartupEvent, NotificationEvent, FALSE);
        }

        // create ghost thread
        if (!UserCreateSystemThread(ST_GHOST_THREAD))
        {
            ERR("Unable to create ghost thread for pHungWnd(%p), hwndHung(%p)\n",
                (void *)pHungWnd, (void *)hwndHung);
            IntDeleteGhostEntry(hwndHung);
            return FALSE;   // creation failed
        }

        // wait for event
        UserLeave();
        Status = KeWaitForSingleObject(gpGhostStartupEvent, UserRequest, UserMode, FALSE, NULL);
        UserEnterExclusive();
        if (Status == STATUS_USER_APC)
        {
            ERR("KeWaitForSingleObject failed: Status: 0x%08lx\n", Status);
            IntDeleteGhostEntry(hwndHung);
            return FALSE;
        }
    }

    // Pass the hwnd of pHungWnd to the ghost thread as we can't pass parameters directly
    if (!UserPostThreadMessage(gptiGhostThread, GTM_CREATE_GHOST,
                               (WPARAM)hwndHung, (LPARAM)pEntry->hwndGhost))
    {
        ERR("Unable to post ghost message (%p, %p)\n",
            (void *)hwndHung, (void *)pEntry->hwndGhost);

        IntDeleteGhostEntry(hwndHung);
        return FALSE;
    }

    TRACE("IntMakeHungWindowGhosted(%p): Done.\n", (void *)hwndHung);
    return TRUE;
}
