/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         LGPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/main/cursor.c
 * PURPOSE:         ReactOS cursor support routines
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* Cursor icons list */
LIST_ENTRY CursorIcons;
ERESOURCE CursorIconsLock;

SYSTEM_CURSORINFO CursorInfo;

extern PDEVOBJ PrimarySurface;

BOOL
APIENTRY
RosUserGetCursorPos(LPPOINT pt)
{
    NTSTATUS Status = STATUS_SUCCESS;

    _SEH2_TRY
    {
        ProbeForWrite(pt, sizeof(POINT), 1);
        *pt = CursorInfo.CursorPos;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return NT_SUCCESS(Status);
}

static void NTAPI clip_point_to_rect(LPCRECT rect, LPPOINT pt)
{
    if      (pt->x <  rect->left)   pt->x = rect->left;
    else if (pt->x >= rect->right)  pt->x = rect->right - 1;
    if      (pt->y <  rect->top)    pt->y = rect->top;
    else if (pt->y >= rect->bottom) pt->y = rect->bottom - 1;
}

BOOL
APIENTRY
RosUserSetCursorPos(INT x, INT y)
{
    POINT pos;
    SURFOBJ *pso;

    pos.x = x;
    pos.y = y;

    if(CursorInfo.IsClipped)
    {
        clip_point_to_rect(&CursorInfo.ClipRect, &pos);
    }

    if (CursorInfo.ShowingCursor)
    {
        pso = EngLockSurface(PrimarySurface.pSurface);
        GreMovePointer(pso, pos.x, pos.y, &(GDIDEV(pso)->Pointer.Exclude));
        EngUnlockSurface(pso);
    }

    CursorInfo.CursorPos = pos;

    return TRUE;
}

BOOL
APIENTRY
RosUserClipCursor( LPCRECT UnsafeRect )
{
    NTSTATUS Status = STATUS_SUCCESS;
    RECT Rect;

    if (!UnsafeRect)
    {
        CursorInfo.IsClipped = FALSE;
        return TRUE;
    }

    _SEH2_TRY
    {
        ProbeForRead(UnsafeRect, sizeof(RECT), 1);
        RtlCopyMemory(&Rect, UnsafeRect , sizeof(RECT));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
    {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    CursorInfo.IsClipped = TRUE;
    CursorInfo.ClipRect = Rect;

    RosUserSetCursorPos(CursorInfo.CursorPos.x, CursorInfo.CursorPos.y);

    return TRUE;
}

VOID
APIENTRY
RosUserSetCursor( ICONINFO* IconInfoUnsafe )
{
    ICONINFO IconInfo;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Special case for removing a pointer */
    if (!IconInfoUnsafe)
    {
        GreSetCursor(NULL, &CursorInfo);
        return;
    }

    _SEH2_TRY
    {
        ProbeForRead(IconInfoUnsafe, sizeof(ICONINFO), 1);
        RtlCopyMemory(&IconInfo, IconInfoUnsafe , sizeof(ICONINFO));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (NT_SUCCESS(Status))
    {
        IconInfo.hbmMask = GDI_MapUserHandle(IconInfo.hbmMask);
        IconInfo.hbmColor = GDI_MapUserHandle(IconInfo.hbmColor);
        GreSetCursor(&IconInfo, &CursorInfo);
    }
}

VOID
APIENTRY
RosUserCreateCursorIcon(ICONINFO* IconInfoUnsafe,
                        HCURSOR Handle)
{
    PCURSORICONENTRY pCursorIcon;

    // FIXME: SEH!

    /* Allocate an entry in the cursor icons list */
    pCursorIcon = ExAllocatePool(PagedPool, sizeof(CURSORICONENTRY));
    RtlZeroMemory(pCursorIcon, sizeof(CURSORICONENTRY));

    /* Save the usermode handle and other fields */
    pCursorIcon->hUser = Handle;
    //pCursorIcon->hbmMask = GDI_MapUserHandle(IconInfoUnsafe->hbmMask);
    //pCursorIcon->hbmColor = GDI_MapUserHandle(IconInfoUnsafe->hbmColor);
    pCursorIcon->hbmMask = IconInfoUnsafe->hbmMask;
    pCursorIcon->hbmColor = IconInfoUnsafe->hbmColor;

    /* Acquire lock */
    USER_LockCursorIcons();

    /* Add it to the list */
    InsertTailList(&CursorIcons, &pCursorIcon->Entry);

    /* Release lock */
    USER_UnlockCursorIcons();
}

VOID
APIENTRY
RosUserDestroyCursorIcon(ICONINFO* IconInfoUnsafe,
                         HCURSOR Handle)
{
    PLIST_ENTRY Current;
    PCURSORICONENTRY pCursorIcon;

    /* Acquire lock */
    USER_LockCursorIcons();

    /* Traverse the list to find our mapping */
    Current = CursorIcons.Flink;
    while(Current != &CursorIcons)
    {
        pCursorIcon = CONTAINING_RECORD(Current, CURSORICONENTRY, Entry);

        /* Check if it's our entry */
        if (pCursorIcon->hUser == Handle)
        {
            /* Remove it from the list */
            RemoveEntryList(Current);

            /* Get handles back to the user for proper disposal */
            IconInfoUnsafe->hbmColor = pCursorIcon->hbmColor;
            IconInfoUnsafe->hbmMask = pCursorIcon->hbmMask;

            /* Free memory */
            ExFreePool(pCursorIcon);
            break;
        }

        /* Advance to the next pair */
        Current = Current->Flink;
    }

    /* Release lock */
    USER_UnlockCursorIcons();
}

VOID NTAPI
USER_InitCursorIcons()
{
    NTSTATUS Status;

    /* Initialize cursor icons list and a spinlock */
    InitializeListHead(&CursorIcons);
    Status = ExInitializeResourceLite(&CursorIconsLock);
    if (!NT_SUCCESS(Status))
        DPRINT1("Initializing cursor icons lock failed with Status 0x%08X\n", Status);
}

VOID USER_LockCursorIcons()
{
    /* Acquire lock */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&CursorIconsLock, TRUE);
}

VOID USER_UnlockCursorIcons()
{
    /* Release lock */
    ExReleaseResourceLite(&CursorIconsLock);
    KeLeaveCriticalRegion();
}
