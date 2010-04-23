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

PSWM_WINDOW NTAPI SwmGetTopWindow();
VOID NTAPI SwmClipAllWindows();

VOID NTAPI SwmDumpRegion(struct region *Region);
VOID NTAPI SwmDumpWindows();
VOID NTAPI SwmDebugDrawWindows();
VOID NTAPI SwmTest();

/* GLOBALS *******************************************************************/

LIST_ENTRY SwmWindows;
ERESOURCE SwmLock;

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
SwmInvalidateRegion(PSWM_WINDOW Window, struct region *Region, rectangle_t *Rect)
{
    struct window *Win;
    struct update_window_zorder_request req;
    struct update_window_zorder_reply reply;
    struct region *ClientRegion;
    int client_left = 0, client_top = 0;
    UINT i;

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

    /* Convert region to client coordinates */
    client_to_screen( Win, &client_left, &client_top );
    offset_region(ClientRegion, -client_left, -client_top);

    /* Redraw window */
    redraw_window(Win, ClientRegion, 1, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN );

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
    if (CalcWindow == SwmGetTopWindow())
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


VOID
NTAPI
SwmAddWindow(HWND hWnd, RECT *WindowRect)
{
    PSWM_WINDOW Win;

    DPRINT("SwmAddWindow %x\n", hWnd);
    DPRINT("rect (%d,%d)-(%d,%d)\n", WindowRect->left, WindowRect->top, WindowRect->right, WindowRect->bottom);

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
    set_region_rect(Win->Visible, &Win->Window);

    /* Now go through the list and remove this rect from all underlying windows visible region */
    //SwmMarkInvisible(Win->Visible);

    InsertHeadList(&SwmWindows, &Win->Entry);

    /* Now ensure it is visible on screen */
    SwmInvalidateRegion(Win, Win->Visible, &Win->Window);

    SwmClipAllWindows();

    //SwmDumpWindows();

    /* Release the lock */
    SwmRelease();
}

VOID
NTAPI
SwmAddDesktopWindow(HWND hWnd, UINT Width, UINT Height)
{
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
SwmRemoveWindow(HWND hWnd)
{
    PSWM_WINDOW Win;

    /* Acquire the lock */
    SwmAcquire();

    DPRINT("SwmRemoveWindow %x\n", hWnd);

    /* Allocate entry */
    Win = SwmFindByHwnd(hWnd);
    //ASSERT(Win != NULL);
    if (!Win)
    {
        /* Release the lock */
        SwmRelease();

        return;
    }

    RemoveEntryList(&Win->Entry);

    /* Mark this region as visible in other window */
    //SwmMarkVisible(Win->Visible);

    /* Free the entry */
    free_region(Win->Visible);
    ExFreePool(Win);

    SwmClipAllWindows();

    /* Release the lock */
    SwmRelease();
}

PSWM_WINDOW
NTAPI
SwmGetTopWindow()
{
    PLIST_ENTRY Current;
    PSWM_WINDOW Window;

    /* Traverse the list to find top non-hidden window */
    Current = SwmWindows.Flink;
    while(Current != &SwmWindows)
    {
        Window = CONTAINING_RECORD(Current, SWM_WINDOW, Entry);

        /* If this window is not hidden - it's the top one */
        if (!Window->Hidden) return Window;

        Current = Current->Flink;
    }

    /* This should never happen */
    ASSERT(FALSE);
    return NULL;
}


VOID
NTAPI
SwmBringToFront(PSWM_WINDOW SwmWin)
{
    PSWM_WINDOW Previous;

    /* Save previous focus window */
    Previous = SwmGetTopWindow();

    /* It's already on top */
    if (Previous->hwnd == SwmWin->hwnd)
    {
        DPRINT("hwnd %x is already on top\n", SwmWin->hwnd);
        return;
    }

    DPRINT("Setting %x as foreground, previous window was %x\n", SwmWin->hwnd, Previous->hwnd);

    /* Remove it from the list */
    RemoveEntryList(&SwmWin->Entry);

    /* Add it to the head of the list */
    InsertHeadList(&SwmWindows, &SwmWin->Entry);

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
SwmSetForeground(HWND hWnd)
{
    PSWM_WINDOW SwmWin;

    /* Acquire the lock */
    SwmAcquire();

    /* Allocate entry */
    SwmWin = SwmFindByHwnd(hWnd);
    //ASSERT(SwmWin != NULL);
    if (!SwmWin)
    {
        /* Release the lock */
        SwmRelease();
        return;
    }

    SwmBringToFront(SwmWin);

    /* Release the lock */
    SwmRelease();
}

VOID
NTAPI
SwmPosChanging(HWND hWnd, const RECT *WindowRect)
{
}

VOID
NTAPI
SwmPosChanged(HWND hWnd, const RECT *WindowRect, const RECT *OldRect, HWND hWndAfter, UINT SwpFlags)
{
    PSWM_WINDOW SwmWin, SwmPrev;

    /* Acquire the lock */
    SwmAcquire();

    /* Allocate entry */
    SwmWin = SwmFindByHwnd(hWnd);
    if (!SwmWin)
    {
        /* Release the lock */
        SwmRelease();
        return;
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

    DPRINT("SwmPosChanged hwnd %x, new rect (%d,%d)-(%d,%d)\n", hWnd,
        WindowRect->left, WindowRect->top, WindowRect->right, WindowRect->bottom);

    SwmWin->Window.left = WindowRect->left;
    SwmWin->Window.top = WindowRect->top;
    SwmWin->Window.right = WindowRect->right;
    SwmWin->Window.bottom = WindowRect->bottom;

    /* Check if we need to change zorder */
    if (hWndAfter && !(SwpFlags & SWP_NOZORDER))
    {
        /* Get the previous window */
        SwmPrev = CONTAINING_RECORD(SwmWin->Entry.Blink, SWM_WINDOW, Entry);

        /* Check if they are different */
        if (SwmPrev->hwnd != hWndAfter)
        {
            DPRINT1("WARNING: Change in zorder is requested but ignored! Previous hwnd %x, but should be %x\n", SwmPrev->hwnd, hWndAfter);
        }
    }

    /* Recalculate all clipping */
    SwmClipAllWindows();

    /* Release the lock */
    SwmRelease();
}

VOID
NTAPI
SwmShowWindow(HWND hWnd, BOOLEAN Show, UINT SwpFlags)
{
    PSWM_WINDOW Win;
    struct region *OldRegion;

    /* Acquire the lock */
    SwmAcquire();

    DPRINT("SwmShowWindow %x, Show %d\n", hWnd, Show);

    /* Allocate entry */
    Win = SwmFindByHwnd(hWnd);
    if (!Win)
    {
        /* Release the lock */
        SwmRelease();
        return;
    }

    if (Show && Win->Hidden)
    {
        /* Change state from hidden to visible */
        DPRINT("Unhiding %x, rect (%d,%d)-(%d,%d)\n", Win->hwnd, Win->Window.left, Win->Window.top, Win->Window.right, Win->Window.bottom);
        Win->Hidden = FALSE;

        /* Make it topmost window if needed */
        if (!(SwpFlags & SWP_NOZORDER))
        {
            /* Remove it from the list */
            RemoveEntryList(&Win->Entry);

            /* Add it to the head of the list */
            InsertHeadList(&SwmWindows, &Win->Entry);
        }

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

    SwmTest();
}

VOID
NTAPI
SwmDumpRegion(struct region *Region)
{
    ULONG i;

    //get_region_extents(Region, &ExtRect);

    for (i=0; i<Region->num_rects; i++)
    {
        DbgPrint("(%d,%d)-(%d,%d) ", Region->rects[i].left, Region->rects[i].top,
            Region->rects[i].right, Region->rects[i].bottom);
    }

    DbgPrint("\n");
}

VOID
NTAPI
SwmDumpWindows()
{
    PLIST_ENTRY Current;
    PSWM_WINDOW Window;

    DPRINT1("Windows in z order: ");

    /* Traverse the list to find our window */
    Current = SwmWindows.Flink;
    while(Current != &SwmWindows)
    {
        Window = CONTAINING_RECORD(Current, SWM_WINDOW, Entry);

        DbgPrint("%x regions: \n", Window->hwnd);
        SwmDumpRegion(Window->Visible);

        /* Advance to the next window */
        Current = Current->Flink;
    }

    DbgPrint("\n");
}

VOID
NTAPI
SwmDebugDrawRect(HDC hDC, rectangle_t *Rect, ULONG Color)
{
    PDC pDC;
    ULONG Scale = 4;

    /* Get a pointer to the DC */
    pDC = DC_Lock(hDC);

    pDC->pLineBrush->BrushObj.iSolidColor = Color;

    GreRectangle(pDC, Rect->left / Scale, Rect->top / Scale, Rect->right / Scale, Rect->bottom / Scale);

    /* Release the object */
    DC_Unlock(pDC);
}

VOID
NTAPI
SwmDebugDrawRegion(HDC hDC, struct region *Region, ULONG Color)
{
    PDC pDC;
    ULONG Scale = 4, i;

    if (is_region_empty(Region)) return;

    /* Get a pointer to the DC */
    pDC = DC_Lock(hDC);

    pDC->pLineBrush->BrushObj.iSolidColor = Color;

    for (i=0; i<Region->num_rects; i++)
        GreRectangle(pDC, Region->rects[i].left / Scale, Region->rects[i].top / Scale,
            Region->rects[i].right / Scale, Region->rects[i].bottom / Scale);

    /* Release the object */
    DC_Unlock(pDC);
}

VOID
NTAPI
SwmDebugDrawWindows()
{
    PLIST_ENTRY Current;
    PSWM_WINDOW Window;
    HDC ScreenDc = 0;
    ROS_DCINFO RosDc = {0};
    PBRUSHGDI Brush, BrushBack;
    PDC pDC;
    RECTL rcSafeBounds;

    /* Create a dc */
    RosGdiCreateDC(&RosDc, &ScreenDc, L"", L"", L"", NULL);

    /* Create a pen and select it */
    Brush = GreCreateSolidBrush(NULL, RGB(0xFF, 0, 0));

    /* Get a pointer to the DC */
    pDC = DC_Lock(ScreenDc);
    GreFreeBrush(pDC->pLineBrush);
    pDC->pLineBrush = Brush;

    /* Set the clipping object */
    IntEngDeleteClipRegion(pDC->CombinedClip);
    RECTL_vSetRect(&rcSafeBounds,
                   0,
                   0,
                   640,
                   480);

    pDC->CombinedClip = IntEngCreateClipRegion(1, &rcSafeBounds, &rcSafeBounds);

    /* Clear the area */
    BrushBack = pDC->pFillBrush;
    pDC->pFillBrush = GreCreateSolidBrush(NULL, RGB(0,0,0));
    GreRectangle(pDC, 0, 0, 800/4, 600/4);
    GreFreeBrush(pDC->pFillBrush);
    pDC->pFillBrush = BrushBack;

    DC_Unlock(pDC);

    /* Traverse the list to find our window */
    Current = SwmWindows.Flink;
    while(Current != &SwmWindows)
    {
        Window = CONTAINING_RECORD(Current, SWM_WINDOW, Entry);

        SwmDebugDrawRect(ScreenDc, &Window->Window, RGB(0,0,255));
        SwmDebugDrawRegion(ScreenDc, Window->Visible, RGB(255,0,0));

        /* Advance to the next window */
        Current = Current->Flink;
    }

    /* Delete screen dc */
    GreFreeBrush(Brush);
    RosGdiDeleteDC(ScreenDc);
}

VOID
NTAPI
SwmTest()
{
#if 0
    RECT rect;
    HWND hwnd;

    /* "desktop" */
    hwnd = (HWND)1;
    rect.left = 0; rect.top = 0;
    rect.right = 100; rect.bottom = 100;
    SwmAddWindow(hwnd, &rect);

    /* win1 */
    hwnd = (HWND)2;
    rect.left = 40; rect.top = 40;
    rect.right = 60; rect.bottom = 60;
    SwmAddWindow(hwnd, &rect);

    /* win2 */
    hwnd = (HWND)3;
    rect.left = 50; rect.top = 50;
    rect.right = 70; rect.bottom = 70;
    SwmAddWindow(hwnd, &rect);

    SwmDumpWindows();
#endif
}

/* EOF */
