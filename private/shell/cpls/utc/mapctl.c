/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    mapctl.c

Abstract:

    This module implements the map control for the Date/Time applet.

Revision History:

--*/



//
//  Include Files.
//

#include "timedate.h"
#include <commctrl.h>
#include "worldmap.h"
#include "mapctl.h"
#include "rc.h"




//
//  Constant Declarations.
//

#define MAPCTL_BITMAP_NAME          MAKEINTRESOURCE(IDB_TIMEZONE)

#define MAPCTL_ANIMATION_TIMER      1
#define MAPCTL_ANIMATION_INTERVAL   30
#define MAPCTL_ANIMATION_FACTOR     3

#define MAPCTL_DATA                 0


static const RGBQUAD c_SeaNormalColor       =  { 127,   0,   0,   0 };
static const RGBQUAD c_SeaHiliteColor       =  { 255,   0,   0,   0 };
static const RGBQUAD c_LandNormalColor      =  {   0, 127,   0,   0 };
static const RGBQUAD c_LandHiliteColor      =  {   0, 255,   0,   0 };

static const RGBQUAD c_MonoSeaNormalColor   =  { 255, 255, 255,   0 };
static const RGBQUAD c_MonoSeaHiliteColor   =  {   0,   0,   0,   0 };
static const RGBQUAD c_MonoLandNormalColor  =  {   0,   0,   0,   0 };
static const RGBQUAD c_MonoLandHiliteColor  =  { 255, 255, 255,   0 };




//
//  Typedef Declarations.
//

typedef struct tagMAPCTLDATA
{
    HWND        window;         // window handle for this control
    UINT_PTR    timer;          // timer id for this map control
    int         totaldelta;     // total delta we must cover to center stuff
    BOOL        slowdevice;
    BOOL        captured;
    WORLDMAP    map;            // world map object, at end of struct (>1k)

} MAPCTLDATA, *PMAPCTLDATA;




//
//  Function Prototypes.
//

PMAPCTLDATA
CreateMapControlData(
    HWND window);

void
DestroyMapControlData(
    PMAPCTLDATA ctldata);

LRESULT CALLBACK
MapControlWndProc(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam);

BOOL
MapControlNCCreate(
    PMAPCTLDATA ctldata,
    LPCREATESTRUCT cs);

BOOL
MapControlCreate(
    PMAPCTLDATA ctldata,
    LPCREATESTRUCT cs);

void
MapControlNCDestroy(
    PMAPCTLDATA ctldata);

BOOL
MapControlErase(
    PMAPCTLDATA ctldata,
    HDC dc);

void
MapControlPaint(
    PMAPCTLDATA ctldata);

void
MapControlMouseMove(
    PMAPCTLDATA ctldata,
    int x,
    int y,
    UINT flags);

void
MapControlMouseDown(
    PMAPCTLDATA ctldata,
    BOOL dblclk,
    int x,
    int y,
    UINT flags);

void
MapControlMouseUp(
    PMAPCTLDATA ctldata,
    int x,
    int y,
    UINT flags);

void
MapControlTimerProc(
    PMAPCTLDATA ctldata,
    UINT timer);

void
MapControlRotate(
    PMAPCTLDATA ctldata,
    int delta);

void
MapControlAnimateOneStep(
    PMAPCTLDATA ctldata);





////////////////////////////////////////////////////////////////////////////
//
//  SetMapControlData
//
////////////////////////////////////////////////////////////////////////////

__inline
void SetMapControlData(
    HWND window,
    PMAPCTLDATA ctldata)
{
    SetWindowLongPtr(window, MAPCTL_DATA, (LONG_PTR)ctldata);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetMapControlData
//
////////////////////////////////////////////////////////////////////////////

__inline
PMAPCTLDATA GetMapControlData(
    HWND window)
{
    return ( (PMAPCTLDATA)GetWindowLongPtr(window, MAPCTL_DATA) );
}


////////////////////////////////////////////////////////////////////////////
//
//  RegisterMapControlStuff
//
////////////////////////////////////////////////////////////////////////////

BOOL RegisterMapControlStuff(
    HINSTANCE instance)
{
    WNDCLASS wc;

    wc.style = 0;
    wc.lpfnWndProc = MapControlWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(PMAPCTLDATA);
    wc.hInstance = instance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = MAPCTL_CLASSNAME;

    return ( RegisterClass(&wc) );
}


////////////////////////////////////////////////////////////////////////////
//
//  MapControlSetSeaRegionHighlight
//
////////////////////////////////////////////////////////////////////////////

void MapControlSetSeaRegionHighlight(
    HWND window,
    int index,
    BOOL highlighted,
    int x,
    int cx)
{
    ChangeWorldMapColor( &GetMapControlData(window)->map,
                         index,
#ifdef ENABLE_MAP
                         highlighted ? &c_SeaHiliteColor : &c_SeaNormalColor,
#else
                         &c_SeaNormalColor,
#endif
                         x,
                         cx );
}


////////////////////////////////////////////////////////////////////////////
//
//  MapControlSetLandRegionHighlight
//
////////////////////////////////////////////////////////////////////////////

void MapControlSetLandRegionHighlight(
    HWND window,
    int index,
    BOOL highlighted,
    int x,
    int cx)
{
    ChangeWorldMapColor( &GetMapControlData(window)->map,
                         index,
#ifdef ENABLE_MAP
                         highlighted ? &c_LandHiliteColor : &c_LandNormalColor,
#else
                         &c_LandNormalColor,
#endif
                         x,
                         cx );
}


////////////////////////////////////////////////////////////////////////////
//
//  _InvalidateProc
//
////////////////////////////////////////////////////////////////////////////

void _InvalidateProc(
    LPARAM data,
    int left,
    int right)
{
#define ctldata ((PMAPCTLDATA)data)
    RECT ri = { left, 0, right, ctldata->map.size.cy };

    InvalidateRect(ctldata->window, &ri, TRUE);
#undef ctldata
}


////////////////////////////////////////////////////////////////////////////
//
//  MapControlInvalidateDirtyRegions
//
////////////////////////////////////////////////////////////////////////////

void MapControlInvalidateDirtyRegions(
    HWND window)
{
    PMAPCTLDATA ctldata = GetMapControlData(window);

    EnumWorldMapDirtySpans( &ctldata->map,
                            _InvalidateProc,
                            (LPARAM)ctldata,
                            TRUE );
}


////////////////////////////////////////////////////////////////////////////
//
//  MapControlRotateTo
//
////////////////////////////////////////////////////////////////////////////

void MapControlRotateTo(
    HWND window,
    int x,
    BOOL animate)
{
    PMAPCTLDATA ctldata = GetMapControlData(window);

    if (ctldata->timer)
    {
        KillTimer(ctldata->window, ctldata->timer);
        ctldata->timer = 0;
    }

    ctldata->totaldelta = (ctldata->map.size.cx / 2) -
                          WorldMapGetDisplayedLocation(&ctldata->map, x);

    if (ctldata->totaldelta)
    {
        UpdateWindow(ctldata->window);

        if (animate)
        {
            ctldata->timer = SetTimer( ctldata->window,
                                       MAPCTL_ANIMATION_TIMER,
                                       MAPCTL_ANIMATION_INTERVAL,
                                       NULL );

            if (ctldata->timer)
            {
                MapControlAnimateOneStep(ctldata);
                return;
            }
        }

        MapControlRotate(ctldata, ctldata->totaldelta);
        ctldata->totaldelta = 0;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CreateMapControlData
//
////////////////////////////////////////////////////////////////////////////

PMAPCTLDATA CreateMapControlData(
    HWND window)
{
    PMAPCTLDATA ctldata = (PMAPCTLDATA)LocalAlloc(LPTR, sizeof(MAPCTLDATA));

    if (ctldata)
    {
        ctldata->window = window;
        SetMapControlData(window, ctldata);
    }

    return (ctldata);
}


////////////////////////////////////////////////////////////////////////////
//
//  DestroyMapControlData
//
////////////////////////////////////////////////////////////////////////////

void DestroyMapControlData(
    PMAPCTLDATA ctldata)
{
    if (ctldata)
    {
        SetMapControlData(ctldata->window, 0);
        LocalFree(ctldata);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  MapControlWndProc
//
////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK MapControlWndProc(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PMAPCTLDATA ctldata = (message == WM_NCCREATE)
                              ? CreateMapControlData(window)
                              : GetMapControlData(window);

    switch (message)
    {
        HANDLE_MSG(ctldata, WM_NCCREATE,    MapControlNCCreate);
        HANDLE_MSG(ctldata, WM_CREATE,      MapControlCreate);
        HANDLE_MSG(ctldata, WM_NCDESTROY,   MapControlNCDestroy);

        HANDLE_MSG(ctldata, WM_ERASEBKGND,  MapControlErase);
        HANDLE_MSG(ctldata, WM_PAINT,       MapControlPaint);

#ifdef ENABLE_MAP
        HANDLE_MSG(ctldata, WM_MOUSEMOVE,   MapControlMouseMove);
        HANDLE_MSG(ctldata, WM_LBUTTONDOWN, MapControlMouseDown);
        HANDLE_MSG(ctldata, WM_LBUTTONUP,   MapControlMouseUp);
#endif
        HANDLE_MSG(ctldata, WM_TIMER,       MapControlTimerProc);
    }

    return ( DefWindowProc(window, message, wParam, lParam) );
}


////////////////////////////////////////////////////////////////////////////
//
//  MapControlNCCreate
//
////////////////////////////////////////////////////////////////////////////

BOOL MapControlNCCreate(
    PMAPCTLDATA ctldata,
    LPCREATESTRUCT cs)
{
    if (!ctldata)
    {
        return (FALSE);
    }

    //
    //  NCDESTROY tries to nuke this if it's nonzero.
    //
    ctldata->timer = 0;

    if (!LoadWorldMap(&ctldata->map, g_hInst, MAPCTL_BITMAP_NAME))
    {
        return (FALSE);
    }

    ctldata->slowdevice = (ctldata->map.prepared.dc != NULL);
    ctldata->totaldelta = 0;
    ctldata->captured = FALSE;
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  MapControlCreate
//
////////////////////////////////////////////////////////////////////////////

BOOL MapControlCreate(
    PMAPCTLDATA ctldata,
    LPCREATESTRUCT cs)
{
    RECT rc = { 0, 0, ctldata->map.size.cx, ctldata->map.size.cy };

    if (AdjustWindowRectEx( &rc,
                            GetWindowLong(ctldata->window, GWL_STYLE),
                            FALSE,
                            GetWindowLong(ctldata->window, GWL_EXSTYLE)))
    {
        SIZE sz = { rc.right - rc.left, rc.bottom - rc.top };

        MoveWindow( ctldata->window,
                    cs->x + ((cs->cx - sz.cx) / 2),
                    cs->y + ((cs->cy - sz.cy) / 2 ),
                    sz.cx,
                    sz.cy,
                    FALSE );
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  MapControlNCDestroy
//
////////////////////////////////////////////////////////////////////////////

void MapControlNCDestroy(
    PMAPCTLDATA ctldata)
{
    if (ctldata)
    {
        if (ctldata->timer)
        {
            KillTimer(ctldata->window, ctldata->timer);
        }

        FreeWorldMap(&ctldata->map);
        DestroyMapControlData(ctldata);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  MapControlErase
//
////////////////////////////////////////////////////////////////////////////

BOOL MapControlErase(
    PMAPCTLDATA ctldata,
    HDC dc)
{
    RECT rce;

    GetClipBox(dc, &rce);

    DrawWorldMap( dc,
                  rce.left,
                  rce.top,
                  rce.right - rce.left,
                  rce.bottom - rce.top,
                  &ctldata->map,
                  rce.left,
                  rce.top,
                  SRCCOPY );

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  MapControlPaint
//
////////////////////////////////////////////////////////////////////////////

void MapControlPaint(
    PMAPCTLDATA ctldata)
{
    PAINTSTRUCT ps;

    BeginPaint(ctldata->window, &ps);
    EndPaint(ctldata->window, &ps);
}


////////////////////////////////////////////////////////////////////////////
//
//  MapControlMouseMove
//
////////////////////////////////////////////////////////////////////////////

void MapControlMouseMove(
    PMAPCTLDATA ctldata,
    int x,
    int y,
    UINT flags)
{
    if (ctldata->captured)
    {
        if (flags & MK_LBUTTON)
        {
            NFYMAPEVENT event;

            event.index = GetWorldMapColorIndex(&ctldata->map, x, y);

            SendNotify( GetParent(ctldata->window),
                        ctldata->window,
                        MAPN_TOUCH,
                        (NMHDR *)&event );
        }
        else
        {
            //
            //  No time to handle WM_CAPTURECHANGED.
            //
            ctldata->captured = FALSE;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  MapControlMouseDown
//
////////////////////////////////////////////////////////////////////////////

void MapControlMouseDown(
    PMAPCTLDATA ctldata,
    BOOL dblclk,
    int x,
    int y,
    UINT flags)
{
    NFYMAPEVENT event;

    SetCapture(ctldata->window);
    ctldata->captured = TRUE;

    if (ctldata->timer)
    {
        KillTimer(ctldata->window, ctldata->timer);
        ctldata->timer = 0;
    }

    event.index = GetWorldMapColorIndex(&ctldata->map, x, y);

    SendNotify( GetParent(ctldata->window),
                ctldata->window,
                MAPN_TOUCH,
                (NMHDR *)&event );
}


////////////////////////////////////////////////////////////////////////////
//
//  MapControlMouseUp
//
////////////////////////////////////////////////////////////////////////////

void MapControlMouseUp(
    PMAPCTLDATA ctldata,
    int x,
    int y,
    UINT flags)
{
    if (ctldata->captured)
    {
        NFYMAPEVENT event;
        int index = GetWorldMapColorIndex(&ctldata->map, x, y);

        ReleaseCapture();

        event.index = index;
        SendNotify( GetParent(ctldata->window),
                    ctldata->window,
                    MAPN_TOUCH,
                    (NMHDR *)&event );

        event.index = index;
        SendNotify( GetParent(ctldata->window),
                    ctldata->window,
                    MAPN_SELECT,
                    (NMHDR *)&event );
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  MapControlTimerProc
//
////////////////////////////////////////////////////////////////////////////

void MapControlTimerProc(
    PMAPCTLDATA ctldata,
    UINT timer)
{
    if (timer == MAPCTL_ANIMATION_TIMER)
    {
        if (ctldata->totaldelta)
        {
            MapControlAnimateOneStep(ctldata);
        }

        if (!ctldata->totaldelta && ctldata->timer)
        {
            KillTimer(ctldata->window, ctldata->timer);
            ctldata->timer = 0;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  MapControlRotate
//
////////////////////////////////////////////////////////////////////////////

void MapControlRotate(
    PMAPCTLDATA ctldata,
    int delta)
{
    RotateWorldMap(&ctldata->map, delta);
    ScrollWindow(ctldata->window, delta, 0, NULL, NULL);
}


////////////////////////////////////////////////////////////////////////////
//
//  MapControlAnimateOneStep
//
////////////////////////////////////////////////////////////////////////////

void MapControlAnimateOneStep(
    PMAPCTLDATA ctldata)
{
    //
    //  Check that we really need to do something.
    //
    if (ctldata->totaldelta)
    {
        //
        //  Try to create a deceleration effect...
        //
        int delta = ctldata->totaldelta / MAPCTL_ANIMATION_FACTOR;
        BOOL negative = (ctldata->totaldelta < 0);

        if (!delta)
        {
            delta = negative ? -1 : 1;
        }

        if (ctldata->slowdevice)
        {
            //
            //  Multiples of 8 will help on planar devices.
            //
            int preferred = ((delta + (negative ? 0 : 7)) & ~7);

            if (preferred)
            {
                delta = preferred;
            }
        }

        //
        //  Don't go too far.
        //
        if ((negative && (delta < ctldata->totaldelta)) ||
            (!negative && (delta > ctldata->totaldelta)))
        {
            delta = ctldata->totaldelta;
        }

        //
        //  Rotate the map and update the total delta left to go.
        //
        ctldata->totaldelta -= delta;
        MapControlRotate(ctldata, delta);
    }
}
