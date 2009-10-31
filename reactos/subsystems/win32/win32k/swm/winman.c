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

typedef struct _SWM_WINDOW
{
    HWND hwnd;
    rectangle_t Window;
    struct region *Visible;
    struct region *Invisible;

    LIST_ENTRY Entry;
} SWM_WINDOW, *PSWM_WINDOW;

/*static*/ inline struct window *get_window( user_handle_t handle );
void redraw_window( struct window *win, struct region *region, int frame, unsigned int flags );
void req_update_window_zorder( const struct update_window_zorder_request *req, struct update_window_zorder_reply *reply );

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
    //struct update_window_zorder_reply reply;
    struct region *ClientRegion;

    ClientRegion = create_empty_region();
    copy_region(ClientRegion, Region);

    /* Calculate what areas to paint */
    UserEnterExclusive();

    DPRINT1("SwmInvalidateRegion hwnd %x, region:\n", Window->hwnd);
    SwmDumpRegion(Region);
    Win = get_window((UINT_PTR)Window->hwnd);
    if (!Win)
    {
        UserLeave();
        return;
    }

    /* Convert region to client coordinates */
    offset_region(ClientRegion, -Window->Window.left, -Window->Window.top);

    //DPRINT1("rect (%d,%d)-(%d,%d)\n", TmpRect.left, TmpRect.top, TmpRect.right, TmpRect.bottom);

    /* Update zorder */
    if (Rect)
    {
        req.rect = *Rect;
        req.rect.left -= Window->Window.left;  req.rect.top -= Window->Window.top;
        req.rect.right -= Window->Window.left; req.rect.bottom -= Window->Window.left;
        req.window = (UINT_PTR)Window->hwnd;
        //req_update_window_zorder(&req, &reply);
    }

    /* Redraw window */
    redraw_window(Win, ClientRegion, 1, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN );

    UserLeave();

    free_region(ClientRegion);
}

VOID
NTAPI
SwmMarkInvisible(struct region *Region)
{
    PLIST_ENTRY Current;
    PSWM_WINDOW Window;
    struct region *WindowRegion;
    struct region *InvisibleRegion;

    /* Make a copy of the invisible region */
    InvisibleRegion = create_empty_region();
    copy_region(InvisibleRegion, Region);

    /* Traverse the list to find our window */
    Current = SwmWindows.Flink;
    while(Current != &SwmWindows)
    {
        Window = CONTAINING_RECORD(Current, SWM_WINDOW, Entry);

        /* Get window's region */
        WindowRegion = create_empty_region();
        set_region_rect(WindowRegion, &Window->Window);

        //DPRINT1("Window region ext:\n");
        //SwmDumpRegion(WindowRegion);
        //DPRINT1("Region to mark invisible ext:\n");
        //SwmDumpRegion(Region);

        /* Region to mark invisible for this window = Update region X Window region */
        intersect_region(WindowRegion, WindowRegion, InvisibleRegion);

        /* Check if it's empty */
        if (!is_region_empty(WindowRegion))
        {
            //DPRINT1("Subtracting region\n");
            //SwmDumpRegion(WindowRegion);
            //DPRINT1("From visible region\n");
            //SwmDumpRegion(Window->Visible);

            /* If it's not empty, subtract it from visible region */
            subtract_region(Window->Visible, Window->Visible, WindowRegion);

            /* And subtract that part from our invisible region */
            subtract_region(InvisibleRegion, InvisibleRegion, WindowRegion);
        }

        free_region(WindowRegion);

        /* Break if our whole invisible region is mapped to underlying windows */
        if (is_region_empty(InvisibleRegion)) break;

        /* Advance to the next window */
        Current = Current->Flink;
    }

    free_region(InvisibleRegion);
}

/* NOTE: It alters the passed region! */
VOID
NTAPI
SwmMarkVisible(struct region *Region)
{
    PLIST_ENTRY Current;
    PSWM_WINDOW Window;
    struct region *WindowRegion;

    /* Go through every window from nearest to farthest */
    Current = SwmWindows.Flink;
    while(Current != &SwmWindows)
    {
        Window = CONTAINING_RECORD(Current, SWM_WINDOW, Entry);

        /* Get window's region */
        WindowRegion = create_empty_region();
        set_region_rect(WindowRegion, &Window->Window);

        /* Region to mark invisible for this window = Update region X Window region */
        intersect_region(WindowRegion, WindowRegion, Region);

        /* Check if it's empty */
        if (!is_region_empty(WindowRegion))
        {
            DPRINT1("Invalidating region\n");
            SwmDumpRegion(WindowRegion);
            DPRINT1("of window %x\n", Window->hwnd);

            /* If it's not empty, subtract it from the source region */
            subtract_region(Region, Region, WindowRegion);

            /* And add it to this window's visible region */
            union_region(Window->Visible, Window->Visible, WindowRegion);

            /* Invalidate this region of target window */
            SwmInvalidateRegion(Window, WindowRegion, NULL);

            DPRINT1("Rest of the update region is:\n");
            SwmDumpRegion(Region);
        }

        free_region(WindowRegion);

        /* If region to update became empty, quit */
        if (is_region_empty(Region)) break;

        /* Advance to the next window */
        Current = Current->Flink;
    }
}

VOID
NTAPI
SwmRecalculateVisibility(PSWM_WINDOW CalcWindow)
{
    PLIST_ENTRY Current;
    PSWM_WINDOW Window;
    struct region *Region = create_empty_region();
    struct region *WindowRegion = create_empty_region();

    set_region_rect(WindowRegion, &CalcWindow->Window);

    /* Compile a total region of visible regions of "higher" windows */
    Current = &CalcWindow->Entry;
    while(Current != &SwmWindows)
    {
        Window = CONTAINING_RECORD(Current, SWM_WINDOW, Entry);

        union_region(Region, Region, Window->Visible);

        /* Advance to the previous window */
        Current = Current->Blink;
    }

    /* Crop the region against target window */
    intersect_region(Region, Region, WindowRegion);

    /* Subtract result from target window's visible region */
    subtract_region(CalcWindow->Visible, WindowRegion, Region);

    /* Free allocated temporary regions */
    free_region(Region);
    free_region(WindowRegion);
}

VOID
NTAPI
SwmAddWindow(HWND hWnd, RECT *WindowRect)
{
    PSWM_WINDOW Win;

    DPRINT1("SwmAddWindow %x\n", hWnd);
    DPRINT1("rect (%d,%d)-(%d,%d)\n", WindowRect->left, WindowRect->top, WindowRect->right, WindowRect->bottom);

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
    SwmMarkInvisible(Win->Visible);

    InsertHeadList(&SwmWindows, &Win->Entry);

    /* Now ensure it is visible on screen */
    SwmInvalidateRegion(Win, Win->Visible, &Win->Window);

    //SwmDumpWindows();
    //SwmDebugDrawWindows();

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

    DPRINT1("SwmRemoveWindow %x\n", hWnd);

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
    SwmMarkVisible(Win->Visible);

    /* Free the entry */
    free_region(Win->Visible);
    ExFreePool(Win);

    /* Release the lock */
    SwmRelease();
}

VOID
NTAPI
SwmSetForeground(HWND hWnd)
{
    PSWM_WINDOW SwmWin, Previous;
    struct region *OldVisible;

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

    /* Save previous focus window */
    Previous = CONTAINING_RECORD(SwmWindows.Flink, SWM_WINDOW, Entry);

    /* It's already on top */
    if (Previous->hwnd == hWnd)
    {
        DPRINT1("hwnd %x is already on top\n", hWnd);
        /* Release the lock */
        SwmRelease();
        return;
    }

    DPRINT1("Setting %x as foreground, previous window was %x\n", hWnd, Previous->hwnd);

    /* Remove it from the list */
    RemoveEntryList(&SwmWin->Entry);

    /* Add it to the head of the list */
    InsertHeadList(&SwmWindows, &SwmWin->Entry);

    /* Make it fully visible */
    OldVisible = create_empty_region();
    copy_region(OldVisible, SwmWin->Visible);

    free_region(SwmWin->Visible);
    SwmWin->Visible = create_empty_region();
    set_region_rect(SwmWin->Visible, &SwmWin->Window);

    /* Intersect new visible and old visible to find region for updating */
    intersect_region(OldVisible, OldVisible, SwmWin->Visible);

    /* If it's not empty - draw missing parts */
    if (!is_region_empty(OldVisible))
        SwmInvalidateRegion(SwmWin, OldVisible, NULL);

    free_region(OldVisible);

    /* Update previous window's visible region */
    SwmRecalculateVisibility(Previous);

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
SwmPosChanged(HWND hWnd, const RECT *WindowRect, const RECT *OldRect)
{
    PSWM_WINDOW SwmWin;
    struct region *NewRegion;
    rectangle_t WinRect;

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

    SwmWin->Window.left = WindowRect->left;
    SwmWin->Window.top = WindowRect->top;
    SwmWin->Window.right = WindowRect->right;
    SwmWin->Window.bottom = WindowRect->bottom;

    //SwmDebugDrawWindows();

    /* Assure the moving window is foreground */
    ASSERT(SwmWindows.Flink == &SwmWin->Entry);

    /* Create a region describing new position */
    NewRegion = create_empty_region();
    WinRect.left = WindowRect->left; WinRect.top = WindowRect->top;
    WinRect.right = WindowRect->right; WinRect.bottom = WindowRect->bottom;
    set_region_rect(NewRegion, &WinRect);

    /* Intersect it with the old region */
    intersect_region(NewRegion, NewRegion, SwmWin->Visible);

    /* This window's visibility region will just move, because it
       really equals window's rect. */
    offset_region(SwmWin->Visible,
                  WindowRect->left - OldRect->left,
                  WindowRect->top - OldRect->top);

    /* NewRegion now holds the difference. Mark it visible. */
    SwmMarkVisible(NewRegion);
    free_region(NewRegion);

    /* Redraw window itself too */
    //SwmInvalidateRegion(SwmWin, SwmWin->Visible, NULL);

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

// haaaaaaaaaaaaaaaaaaack
struct region
{
    int size;
    int num_rects;
    rectangle_t *rects;
    rectangle_t extents;
};

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
    Brush = GreCreateSolidBrush(RGB(0xFF, 0, 0));

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
    pDC->pFillBrush = GreCreateSolidBrush(RGB(0,0,0));
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
