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

static SYSTEM_CURSORINFO CursorInfo;

VOID 
UserGetCursorPos(LPPOINT pt)
{
    *pt = CursorInfo.CursorPos;
}

BOOL
APIENTRY
RosUserGetCursorPos(LPPOINT pt)
{
    _SEH2_TRY
    {
        ProbeForWrite(pt, sizeof(POINT), 1);
        *pt = CursorInfo.CursorPos;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return FALSE);
    }
    _SEH2_END;

    return TRUE;
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

    pos.x = x;
    pos.y = y;

    if(CursorInfo.IsClipped)
    {
        clip_point_to_rect(&CursorInfo.ClipRect, &pos);
    }

    if (CursorInfo.ShowingCursor)
    {
        HDC hDCscreen = SwmGetScreenDC();
        if (!hDCscreen) return FALSE;

        GreMovePointer(hDCscreen, pos.x, pos.y);
    }

    CursorInfo.CursorPos = pos;

    return TRUE;
}

BOOL
APIENTRY
RosUserClipCursor( LPCRECT UnsafeRect )
{
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
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        _SEH2_YIELD(return FALSE);
    }
    _SEH2_END;

    CursorInfo.IsClipped = TRUE;
    CursorInfo.ClipRect = Rect;

    RosUserSetCursorPos(CursorInfo.CursorPos.x, CursorInfo.CursorPos.y);

    return TRUE;
}

VOID UserSetCursor(  ICONINFO* IconInfo )
{
    HDC hDCscreen;

    hDCscreen = SwmGetScreenDC();
    if (!hDCscreen) return;

    if (!IconInfo)
    {
        if (CursorInfo.ShowingCursor == FALSE)
            return;

        DPRINT("Removing pointer!\n");
        /* Remove the cursor if it was displayed */
        GreMovePointer(hDCscreen, -1, -1);

        CursorInfo.ShowingCursor = FALSE;
        return;
    }

    GreSetPointerShape( hDCscreen, 
                        IconInfo->hbmMask, 
                        IconInfo->hbmColor, 
                        IconInfo->xHotspot,
                        IconInfo->yHotspot,
                        CursorInfo.CursorPos.x, 
                        CursorInfo.CursorPos.y);

    CursorInfo.ShowingCursor = TRUE;
}

VOID
APIENTRY
RosUserSetCursor( ICONINFO* IconInfoUnsafe )
{
    ICONINFO IconInfo;

    if(IconInfoUnsafe == NULL)
    {
        UserSetCursor(NULL);
    }
    else
    {
        _SEH2_TRY
        {
            ProbeForRead(IconInfoUnsafe, sizeof(ICONINFO), 1);
            RtlCopyMemory(&IconInfo, IconInfoUnsafe , sizeof(ICONINFO));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return;)
        }
        _SEH2_END;

        UserSetCursor(&IconInfo);
    }
}

