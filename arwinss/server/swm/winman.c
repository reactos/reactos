/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         LGPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/swm/winman.c
 * PURPOSE:         Simple Window Manager
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

#include "object.h"
#include "handle.h"
#include "user.h"

#define NDEBUG
#include <debug.h>

/*static*/ inline struct window *get_window( user_handle_t handle );
inline void client_to_screen( struct window *win, int *x, int *y );
void redraw_window( struct window *win, struct region *region, int frame, unsigned int flags );
void req_update_window_zorder( const struct update_window_zorder_request *req, struct update_window_zorder_reply *reply );

PSWM_WINDOW NTAPI SwmGetForegroundWindow(BOOLEAN TopMost);

VOID NTAPI SwmClipAllWindows();

/* GLOBALS *******************************************************************/

LIST_ENTRY SwmWindows;
ERESOURCE SwmLock;
HDC SwmDc; /* Screen DC for copying operations */
SWM_WINDOW SwmRoot;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
SwmAcquire(VOID)
{
    /* Acquire user resource exclusively */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&SwmLock, TRUE);
}

VOID
NTAPI
SwmRelease(VOID)
{
    /* Release user resource */
    ExReleaseResourceLite(&SwmLock);
    KeLeaveCriticalRegion();
}

VOID
NTAPI
SwmCreateScreenDc()
{
    /* Create the display DC */
    SwmDc = (HDC)0;
    RosGdiCreateDC(&SwmDc, NULL, NULL, NULL, NULL, OBJ_DC);

    DPRINT1("Screen hdc is %x\n", SwmDc);

    /* Make it global */
    GDIOBJ_SetOwnership(SwmDc, NULL);
}

HDC SwmGetScreenDC()
{
    /* Lazily create a global screen DC */
    if (!SwmDc) SwmCreateScreenDc();

    return SwmDc;
}

PSWM_WINDOW
NTAPI
SwmGetWindowById(SWM_WINDOW_ID Wid)
{
    /* Right now, Wid is a pointer to SWM_WINDOW structure,
       except for SWM_ROOT_WINDOW_ID which maps to a root window */
    if (Wid == SWM_ROOT_WINDOW_ID) return &SwmRoot;
    return (PSWM_WINDOW)Wid;
}

VOID
NTAPI
SwmInvalidateRegion(PSWM_WINDOW Window, struct region *Region, rectangle_t *Rect)
{
    struct window *Win;
    struct update_window_zorder_request req;
    struct update_window_zorder_reply reply;
    struct region *ClientRegion;
    int client_left = 0, client_top = 0;
    UINT i;
    ULONG flags = RDW_INVALIDATE | RDW_ERASE;

    ClientRegion = create_empty_region();
    copy_region(ClientRegion, Region);

    /* Calculate what areas to paint */
    UserEnterExclusive();

    DPRINT("SwmInvalidateRegion hwnd %x, region:\n", Window->hwnd);
    //SwmDumpRegion(Region);
    Win = get_window((UINT_PTR)Window->hwnd);
    if (!Win)
    {
        UserLeave();
        return;
    }

    //DPRINT1("rect (%d,%d)-(%d,%d)\n", TmpRect.left, TmpRect.top, TmpRect.right, TmpRect.bottom);

    /* Bring every rect in a region to front */
    if (Window != &SwmRoot)
    {
        for (i=0; i<Region->num_rects; i++)
        {
#if 0
            DbgPrint("(%d,%d)-(%d,%d), and redraw coords (%d,%d)-(%d,%d); ", Region->rects[i].left, Region->rects[i].top,
                Region->rects[i].right, Region->rects[i].bottom,
                Region->rects[i].left - Window->Window.left, Region->rects[i].top - Window->Window.top,
                Region->rects[i].right - Window->Window.left, Region->rects[i].bottom - Window->Window.top);
#endif

            req.rect = Region->rects[i];
            req.window = (UINT_PTR)Window->hwnd;
            req_update_window_zorder(&req, &reply);
        }
        //DbgPrint("\n");
    }

    /* Convert region to client coordinates */
    client_to_screen( Win, &client_left, &client_top );
    offset_region(ClientRegion, -client_left, -client_top);

    if (Window != &SwmRoot)
        flags |= RDW_ALLCHILDREN;

    /* Redraw window */
    redraw_window(Win, ClientRegion, 1, flags | RDW_FRAME);

    UserLeave();

    free_region(ClientRegion);
}

VOID
NTAPI
SwmPaintRegion(struct region *Region)
{
    PLIST_ENTRY Current;
    PSWM_WINDOW Window;
    struct region *RegionToPaint;
    struct region *Intersection;

    /* Make a copy of the region */
    RegionToPaint = create_empty_region();
    copy_region(RegionToPaint, Region);

    Intersection = create_empty_region();

    /* Traverse the list of windows and paint if something intersects */
    Current = SwmWindows.Flink;
    while(Current != &SwmWindows)
    {
        Window = CONTAINING_RECORD(Current, SWM_WINDOW, Entry);

        /* Skip hidden windows */
        if (Window->Hidden)
        {
            /* Advance to the next window */
            Current = Current->Flink;
            continue;
        }

        /* Check if this window has something in common with the */
        intersect_region(Intersection, RegionToPaint, Window->Visible);
        if (!is_region_empty(Intersection))
        {
            /* We have a region to paint, subtract it from total update region */
            subtract_region(RegionToPaint, RegionToPaint, Intersection);

            /* Paint it */
            SwmInvalidateRegion(Window, Intersection, NULL);
        }

        /* If we exhausted the painting region - break out of this loop */
        if (is_region_empty(RegionToPaint)) break;

        /* Advance to the next window */
        Current = Current->Flink;
    }

    /* Free allocated regions */
    free_region(RegionToPaint);
    free_region(Intersection);
}

VOID
NTAPI
SwmRecalculateVisibility(PSWM_WINDOW CalcWindow)
{
    PLIST_ENTRY Current;
    PSWM_WINDOW Window;
    struct region *TempRegion;
    struct region *NewRegion, *DiffRegion, *ParentRegion;

    DPRINT("Calculating visibility for %x\n", CalcWindow->hwnd);

    /* Check if this window is already on top */
    if (CalcWindow == SwmGetForegroundWindow(TRUE))
    {
        /* It is, make sure it's fully visible */
        NewRegion = create_empty_region();
        set_region_rect(NewRegion, &CalcWindow->Window);

        /* Compute the difference into the temp region */
        DiffRegion = create_empty_region();
        subtract_region(DiffRegion, NewRegion, CalcWindow->Visible);

        /* Now get rid of the old visible region */
        free_region(CalcWindow->Visible);
        CalcWindow->Visible = NewRegion;

        /* Show the difference, if any */
        //if (!is_region_empty(DiffRegion))
        //    SwmInvalidateRegion(CalcWindow, DiffRegion, NULL);

        /* Free up temporary regions */
        free_region(DiffRegion);

        return;
    }

    ParentRegion = create_empty_region();

    /* Create a whole window region */
    NewRegion = create_empty_region();
    set_region_rect(NewRegion, &CalcWindow->Window);

    /* Compile a total region clipped by parent windows */
    Current = CalcWindow->Entry.Blink;
    while(Current != &SwmWindows)
    {
        Window = CONTAINING_RECORD(Current, SWM_WINDOW, Entry);

        /* Skip hidden windows */
        if (Window->Hidden)
        {
            /* Advance to the next window */
            Current = Current->Blink;
            continue;
        }
        DPRINT("hwnd: %x\n", Window->hwnd);

        /* Calculate window's region */
        TempRegion = create_empty_region();
        set_region_rect(TempRegion, &Window->Window);

        /* Intersect it with the target window's region */
        intersect_region(TempRegion, TempRegion, NewRegion);

        /* Union it with parent if it's not empty and free temp region */
        if (!is_region_empty(TempRegion))
            union_region(ParentRegion, ParentRegion, TempRegion);
        free_region(TempRegion);

        /* Advance to the previous window */
        Current = Current->Blink;
    }

    DPRINT("Parent region:\n");
    //SwmDumpRegion(ParentRegion);

    /* Remove parts clipped by parents from the window region */
    if (!is_region_empty(ParentRegion))
        subtract_region(NewRegion, NewRegion, ParentRegion);

    DPRINT("New visible region:\n");
    //SwmDumpRegion(NewRegion);

    /* Compute the difference between old and new visible
       regions into the temp region */
    DiffRegion = create_empty_region();
    subtract_region(DiffRegion, CalcWindow->Visible, NewRegion);

    DPRINT("Diff between old and new:\n");
    //SwmDumpRegion(DiffRegion);

    /* Now get rid of the old visible region */
    free_region(CalcWindow->Visible);
    CalcWindow->Visible = NewRegion;

    /* Show the difference if any */
    //if (!is_region_empty(DiffRegion))
    //    SwmInvalidateRegion(CalcWindow, DiffRegion, NULL);

    /* Free allocated temporary regions */
    free_region(DiffRegion);
    free_region(ParentRegion);
}

VOID
NTAPI
SwmClipAllWindows()
{
    PLIST_ENTRY Current;
    PSWM_WINDOW Window;

    /* Traverse the list to find our window */
    Current = SwmWindows.Flink;
    while(Current != &SwmWindows)
    {
        Window = CONTAINING_RECORD(Current, SWM_WINDOW, Entry);

        /* Skip hidden windows */
        if (Window->Hidden)
        {
            /* Advance to the next window */
            Current = Current->Flink;
            continue;
        }

        /* Recalculate visibility for this window */
        SwmRecalculateVisibility(Window);

        /* Advance to the next window */
        Current = Current->Flink;
    }
}


SWM_WINDOW_ID
NTAPI
SwmNewWindow(SWM_WINDOW_ID parent, RECT *WindowRect, HWND hWnd, DWORD ex_style)
{
    PSWM_WINDOW Win, FirstNonTop;

    DPRINT("SwmAddWindow %x\n", hWnd);
    DPRINT("rect (%d,%d)-(%d,%d), ex_style %x\n",
        WindowRect->left, WindowRect->top, WindowRect->right, WindowRect->bottom,
        ex_style);

    /* Acquire the lock */
    SwmAcquire();

    /* Allocate entry */
    Win = ExAllocatePool(PagedPool, sizeof(SWM_WINDOW));
    RtlZeroMemory(Win, sizeof(SWM_WINDOW));
    Win->hwnd = hWnd;
    Win->Window.left = WindowRect->left;
    Win->Window.top = WindowRect->top;
    Win->Window.right = WindowRect->right;
    Win->Window.bottom = WindowRect->bottom;

    Win->Visible = create_empty_region();
    Win->Hidden = TRUE;
    //set_region_rect(Win->Visible, &Win->Window);

    /* Set window's flags */
    if (ex_style & WS_EX_TOPMOST) Win->Topmost = TRUE;

    /* Add it to the zorder list */
    if (Win->Topmost)
    {
        /* It's a topmost window, just add it on top */
        InsertHeadList(&SwmWindows, &Win->Entry);
    }
    else
    {
        /* Find the first topmost window and insert before it */
        FirstNonTop = SwmGetForegroundWindow(FALSE);
        InsertHeadList(FirstNonTop->Entry.Blink, &Win->Entry);
    }

    /* Now ensure it is visible on screen */
    //SwmInvalidateRegion(Win, Win->Visible, &Win->Window);

    //SwmClipAllWindows();

    //SwmDumpWindows();

    /* Release the lock */
    SwmRelease();

    return (SWM_WINDOW_ID)Win;
}

VOID
NTAPI
SwmAddDesktopWindow(HWND hWnd, UINT Width, UINT Height)
{
#if 0
    PSWM_WINDOW Desktop;

    /* Acquire the lock */
    SwmAcquire();

    /* Check if it's already there */
    Desktop = SwmFindByHwnd(hWnd);

    if (Desktop)
    {
        // TODO: Check if dimensions are the same!

        /* Release the lock */
        SwmRelease();

        return;
    }

    /* Add a desktop window */
    Desktop = ExAllocatePool(PagedPool, sizeof(SWM_WINDOW));
    RtlZeroMemory(Desktop, sizeof(SWM_WINDOW));
    Desktop->hwnd = hWnd;
    Desktop->Window.left = 0;
    Desktop->Window.top = 0;
    Desktop->Window.right = Width;
    Desktop->Window.bottom = Height;

    Desktop->Visible = create_empty_region();
    set_region_rect(Desktop->Visible, &Desktop->Window);

    InsertTailList(&SwmWindows, &Desktop->Entry);

    /* Calculate windows clipping */
    SwmClipAllWindows();

    /* Now ensure it is visible on screen */
    SwmInvalidateRegion(Desktop, Desktop->Visible, &Desktop->Window);

    /* Release the lock */
    SwmRelease();
#endif

    SwmRoot.hwnd = hWnd;
    SwmInvalidateRegion(&SwmRoot, SwmRoot.Visible, &SwmRoot.Window);
}

PSWM_WINDOW
NTAPI
SwmFindByHwnd(HWND hWnd)
{
    PLIST_ENTRY Current;
    PSWM_WINDOW Window;

    /* Traverse the list to find our window */
    Current = SwmWindows.Flink;
    while(Current != &SwmWindows)
    {
        Window = CONTAINING_RECORD(Current, SWM_WINDOW, Entry);

        /* Check if it's our entry */
        if (Window->hwnd == hWnd)
        {
            /* Found it, save it and break out of the loop */
            return Window;
        }

        /* Advance to the next window */
        Current = Current->Flink;
    }

    return NULL;
}

VOID
NTAPI
SwmDestroyWindow(SWM_WINDOW_ID Wid)
{
    PSWM_WINDOW Win;

    /* Check for root window */
    if (Wid == SWM_ROOT_WINDOW_ID) return;

    /* Acquire the lock */
    SwmAcquire();

    /* Get window pointer */
    Win = SwmGetWindowById(Wid);

    DPRINT("SwmRemoveWindow %x\n", Win->hwnd);

    /* Remove it from the zorder list */
    RemoveEntryList(&Win->Entry);

    /* Free the entry */
    free_region(Win->Visible);
    ExFreePool(Win);

    SwmClipAllWindows();

    /* Release the lock */
    SwmRelease();
}

PSWM_WINDOW
NTAPI
SwmGetForegroundWindow(BOOLEAN TopMost)
{
    PLIST_ENTRY Current;
    PSWM_WINDOW Window;

    /* Traverse the list to find top non-hidden window */
    Current = SwmWindows.Flink;
    while(Current != &SwmWindows)
    {
        Window = CONTAINING_RECORD(Current, SWM_WINDOW, Entry);

        if (TopMost)
        {
            /* The first visible window */
            if (!Window->Hidden) return Window;
        }
        else
        {
            /* The first visible non-topmost window */
            if (!Window->Hidden && !Window->Topmost) return Window;
        }

        Current = Current->Flink;
    }

    /* There should always be a desktop window */
    ASSERT(FALSE);
    return NULL;
}


VOID
NTAPI
SwmBringToFront(PSWM_WINDOW SwmWin)
{
    PSWM_WINDOW Previous;

    /* Save previous focus window */
    Previous = SwmGetForegroundWindow(SwmWin->Topmost);

    /* It's already on top */
    if (Previous->hwnd == SwmWin->hwnd)
    {
        DPRINT("hwnd %x is already on top\n", SwmWin->hwnd);
        return;
    }

    DPRINT("Setting %x as foreground, previous window was %x\n", SwmWin->hwnd, Previous->hwnd);

    /* Remove it from the list */
    RemoveEntryList(&SwmWin->Entry);

    if (SwmWin->Topmost)
    {
        /* Add it to the head of the list */
        InsertHeadList(&SwmWindows, &SwmWin->Entry);
    }
    else
    {
        /* Bringing non-topmost window to foreground */
        InsertHeadList(Previous->Entry.Blink, &SwmWin->Entry);
    }

    /* Make it fully visible */
    free_region(SwmWin->Visible);
    SwmWin->Visible = create_empty_region();
    set_region_rect(SwmWin->Visible, &SwmWin->Window);

    // TODO: Redraw only new parts!
    SwmClipAllWindows();
    SwmInvalidateRegion(SwmWin, SwmWin->Visible, NULL);
}

VOID
NTAPI
SwmSetForeground(SWM_WINDOW_ID Wid)
{
    PSWM_WINDOW SwmWin;
    extern struct window *shell_window;

    /* Acquire the lock */
    SwmAcquire();

    /* Get the window pointer */
    SwmWin = SwmGetWindowById(Wid);

    /* Check for a shell window */
    UserEnterExclusive();

    /* Don't allow the shell window to become foreground */
    if(shell_window &&
       (get_window((UINT_PTR)SwmWin->hwnd) == shell_window))
    {
        SwmRelease();
        UserLeave();
        return;
    }

    UserLeave();

    SwmBringToFront(SwmWin);

    /* Release the lock */
    SwmRelease();
}

VOID
NTAPI
SwmCopyBits(const PSWM_WINDOW SwmWin, const RECT *OldRect)
{
    //RECTL rcBounds;
    PDC pDC;
    PLIST_ENTRY Current;
    struct region *TempRegion, *ParentRegion, *WinRegion = NULL;
    rectangle_t rcScreen, rcOldWindow;
    PSWM_WINDOW Window;

    /* Lazily create a global screen DC */
    if (!SwmDc) SwmCreateScreenDc();

    /* Set clipping to prevent touching higher-level windows */
    pDC = DC_LockDc(SwmDc);

    /* Check if we have higher-level windows */
    if (SwmWin->Entry.Blink != &SwmWindows)
    {
        /* Create a whole screen region */
        rcScreen.left = SwmRoot.Window.left;
        rcScreen.top = SwmRoot.Window.top;
        rcScreen.right = SwmRoot.Window.right;
        rcScreen.bottom = SwmRoot.Window.bottom;

        /* Free user clipping, if any */
        if (pDC->Clipping) free_region(pDC->Clipping);
        pDC->Clipping = NULL;

        ParentRegion = create_empty_region();

        /* Compile a total region clipped by parent windows */
        Current = SwmWin->Entry.Blink;
        while(Current != &SwmWindows)
        {
            Window = CONTAINING_RECORD(Current, SWM_WINDOW, Entry);

            /* Skip hidden windows */
            if (Window->Hidden)
            {
                /* Advance to the next window */
                Current = Current->Blink;
                continue;
            }
            DPRINT("hwnd: %x\n", Window->hwnd);

            /* Calculate window's region */
            TempRegion = create_empty_region();
            set_region_rect(TempRegion, &Window->Window);

            /* Union it with parent if it's not empty and free temp region */
            if (!is_region_empty(TempRegion))
                union_region(ParentRegion, ParentRegion, TempRegion);
            free_region(TempRegion);

            /* Advance to the previous window */
            Current = Current->Blink;
        }

        /* Remove parts clipped by parents from the window region, and set result as a user clipping region */
        if (!is_region_empty(ParentRegion))
        {
            TempRegion = create_empty_region();
            set_region_rect(TempRegion, &rcScreen);
            subtract_region(TempRegion, TempRegion, ParentRegion);
            pDC->Clipping = TempRegion;
        }

        /* Set DC clipping */
        RosGdiUpdateClipping(pDC, TRUE);

        /* Get the part which was previously hidden by parent area */
        WinRegion = create_empty_region();
        rcOldWindow.bottom = OldRect->bottom;
        rcOldWindow.left = OldRect->left;
        rcOldWindow.top = OldRect->top;
        rcOldWindow.right = OldRect->right;
        set_region_rect(WinRegion, &rcOldWindow);

        intersect_region(WinRegion, WinRegion, ParentRegion);
        if (!is_region_empty(WinRegion))
        {
            /* Offset it to the new position */
            offset_region(WinRegion,
                SwmWin->Window.left - OldRect->left,
                SwmWin->Window.top - OldRect->top);

            /* Paint it */
            SwmPaintRegion(WinRegion);
        }

        free_region(WinRegion);
        free_region(ParentRegion);
    }
    else
    {
        /* Simple case, no clipping */

        if (pDC->Clipping) free_region(pDC->Clipping);
        pDC->Clipping = NULL;

        /* Set DC clipping */
        RosGdiUpdateClipping(pDC, TRUE);
    }

    DC_UnlockDc(pDC);

    /* Copy bits */
    RosGdiBitBlt(SwmDc, SwmWin->Window.left, SwmWin->Window.top, SwmWin->Window.right - SwmWin->Window.left,
        SwmWin->Window.bottom - SwmWin->Window.top, SwmDc, OldRect->left, OldRect->top, SRCCOPY);
}

VOID
NTAPI
SwmPosChanged(SWM_WINDOW_ID Wid, const RECT *WindowRect, const RECT *OldRect, HWND hWndAfter, UINT SwpFlags)
{
    PSWM_WINDOW SwmWin, SwmPrev;
    LONG Width, Height, OldWidth, OldHeight;
    BOOLEAN IsMove = FALSE;
    struct region *OldRegion, *DiffRegion;
    rectangle_t OldRectSafe;

    /* Check for root window */
    if (Wid == SWM_ROOT_WINDOW_ID) return;

    /* Acquire the lock */
    SwmAcquire();

    SwmWin = SwmGetWindowById(Wid);

    /* Save parameters */
    OldRectSafe.left = OldRect->left; OldRectSafe.top = OldRect->top;
    OldRectSafe.right = OldRect->right; OldRectSafe.bottom = OldRect->bottom;

    Width = WindowRect->right - WindowRect->left;
    Height = WindowRect->bottom - WindowRect->top;

    OldWidth = OldRect->right - OldRect->left;
    OldHeight = OldRect->bottom - OldRect->top;

    /* Detect if it's a move without resizing */
    if ((WindowRect->top != OldRect->top ||
        WindowRect->left != OldRect->left) &&
        Width == OldWidth &&
        Height == OldHeight)
    {
        IsMove = TRUE;
    }

    /* Check if window really moved anywhere (origin, size or z order) */
    if (hWndAfter == 0 &&
        WindowRect->left - OldRect->left == 0 &&
        WindowRect->top - OldRect->top == 0 &&
        WindowRect->right - OldRect->right == 0 &&
        WindowRect->bottom - OldRect->bottom == 0)
    {
        /* Release the lock */
        SwmRelease();
        return;
    }

    DPRINT("SwmPosChanged hwnd %x, new rect (%d,%d)-(%d,%d)\n", SwmWin->hwnd,
        WindowRect->left, WindowRect->top, WindowRect->right, WindowRect->bottom);

    SwmWin->Window.left = WindowRect->left;
    SwmWin->Window.top = WindowRect->top;
    SwmWin->Window.right = WindowRect->right;
    SwmWin->Window.bottom = WindowRect->bottom;

    /* Check if we need to change zorder */
    if (hWndAfter && !(SwpFlags & SWP_NOZORDER))
    {
        /* Get the previous window */
        if (SwmWin->Entry.Blink != &SwmWindows)
            SwmPrev = CONTAINING_RECORD(SwmWin->Entry.Blink, SWM_WINDOW, Entry);
        else
            SwmPrev = SwmWin;

        /* Check if they are different */
        if (SwmPrev->hwnd != hWndAfter)
        {
            DPRINT1("WARNING: Change in zorder is requested but ignored! Previous hwnd %x, but should be %x\n", SwmPrev->hwnd, hWndAfter);
        }
    }

    /* Recalculate all clipping */
    SwmClipAllWindows();

    /* Copy bitmap bits if it's a move */
    if (IsMove) SwmCopyBits(SwmWin, OldRect);

    /* Paint area changed after moving or resizing */
    if (Width < OldWidth ||
        Height < OldHeight ||
        IsMove)
    {
        /* Subtract new visible rect from the old one,
           and invalidate the result */
        OldRegion = create_empty_region();
        set_region_rect(OldRegion, &OldRectSafe);

        /* Compute the difference into the temp region */
        DiffRegion = create_empty_region();
        subtract_region(DiffRegion, OldRegion, SwmWin->Visible);

        /* Paint it */
        SwmPaintRegion(DiffRegion);

        /* Free temporary regions */
        free_region(OldRegion);
        free_region(DiffRegion);
    }

    /* Release the lock */
    SwmRelease();
}

VOID
NTAPI
SwmShowWindow(SWM_WINDOW_ID Wid, BOOLEAN Show, UINT SwpFlags)
{
    PSWM_WINDOW Win;
    struct region *OldRegion;

    /* Check for root window */
    if (Wid == SWM_ROOT_WINDOW_ID) return;

    /* Acquire the lock */
    SwmAcquire();

    Win = SwmGetWindowById(Wid);

    DPRINT("SwmShowWindow %x, Show %d\n", Win->hwnd, Show);

    if (Show && Win->Hidden)
    {
        /* Change state from hidden to visible */
        DPRINT("Unhiding %x, rect (%d,%d)-(%d,%d)\n", Win->hwnd, Win->Window.left, Win->Window.top, Win->Window.right, Win->Window.bottom);
        Win->Hidden = FALSE;

#if 0
        /* Make it topmost window if needed */
        if (!(SwpFlags & SWP_NOZORDER))
        {
            /* Remove it from the list */
            RemoveEntryList(&Win->Entry);

            /* Add it to the head of the list */
            InsertHeadList(&SwmWindows, &Win->Entry);
        }
#endif

        /* Calculate visible regions for all windows */
        SwmClipAllWindows();

        /* Draw the newly appeared window */
        SwmInvalidateRegion(Win, Win->Visible, NULL);
    }
    else if (!Show && !Win->Hidden)
    {
        DPRINT("Hiding %x\n", Win->hwnd);
        /* Change state from visible to hidden */
        Win->Hidden = TRUE;

        /* Save its visible region */
        OldRegion = Win->Visible;

        /* Put an empty visible region */
        Win->Visible = create_empty_region();

        /* Recalculate clipping */
        SwmClipAllWindows();

        /* Show region which was taken by this window and free it */
        SwmPaintRegion(OldRegion);
        free_region(OldRegion);
    }

    /* Release the lock */
    SwmRelease();
}

HWND
NTAPI
SwmGetWindowFromPoint(LONG x, LONG y)
{
    PLIST_ENTRY Current;
    PSWM_WINDOW Window;

    /* Acquire the lock */
    SwmAcquire();

    /* Traverse the list to find our window */
    Current = SwmWindows.Flink;
    while(Current != &SwmWindows)
    {
        Window = CONTAINING_RECORD(Current, SWM_WINDOW, Entry);

        /* Skip hidden windows */
        if (Window->Hidden)
        {
            /* Advance to the next window */
            Current = Current->Flink;
            continue;
        }

        if (point_in_region(Window->Visible, x, y))
        {
            /* Release the lock */
            SwmRelease();

            return Window->hwnd;
        }
        /* Advance to the next window */
        Current = Current->Flink;
    }

    /* Release the lock */
    SwmRelease();

    return 0;
}

VOID
NTAPI
SwmUpdateRootWindow(SURFOBJ *SurfObj)
{
    /* Initialize a root window */
    SwmRoot.Window.left = 0;
    SwmRoot.Window.top = 0;
    SwmRoot.Window.right = SurfObj->sizlBitmap.cx;
    SwmRoot.Window.bottom = SurfObj->sizlBitmap.cy;
    SwmRoot.Hidden = FALSE;
    SwmRoot.Topmost = FALSE;

    set_region_rect(SwmRoot.Visible, &SwmRoot.Window);
}

VOID
NTAPI
SwmInitialize()
{
    NTSTATUS Status;

    /* Initialize handles list and a spinlock */
    InitializeListHead(&SwmWindows);

    /* Initialize SWM access resource */
    Status = ExInitializeResourceLite(&SwmLock);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failure initializing SWM resource!\n");
    }

    /* Initialize a root window */
    SwmRoot.Window.left = 0;
    SwmRoot.Window.top = 0;
    SwmRoot.Window.right = 0;
    SwmRoot.Window.bottom = 0;
    SwmRoot.Hidden = FALSE;
    SwmRoot.Topmost = FALSE;
    SwmRoot.Visible = create_empty_region();

    InsertHeadList(&SwmWindows, &SwmRoot.Entry);
}

/* EOF */
