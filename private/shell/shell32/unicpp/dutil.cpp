#include "stdafx.h"
#include "icwcfg.h"
#pragma hdrstop

EXTERN_C const TCHAR c_szControlIni[] = TEXT("control.ini");
EXTERN_C const TCHAR c_szPatterns[] = TEXT("patterns");
EXTERN_C const TCHAR c_szBackgroundPreview2[] = TEXT("BackgroundPreview2");
EXTERN_C const TCHAR c_szComponentPreview[] = TEXT("ComponentPreview");
EXTERN_C const TCHAR c_szRegDeskHtmlProp[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\Display\\shellex\\PropertySheetHandlers\\DeskHtmlExt");
EXTERN_C const TCHAR c_szWallPaperDir[] = TEXT("WallPaperDir");

//  98/10/01 vtan: Added local function prototypes.

//  Some of these functions are commented out. The linker may not be smart
//  enough to strip the dead code so this is done manually. These prototypes
//  will allow the code to compile but it won't link. If you get linker
//  errors, uncomment the desired function and recompile. It should then link.

//  Point arithmetic

void    SetPt (POINT& pt, LONG x, LONG y);
void    OffsetPt (POINT& pt, LONG dh, LONG dv);

//  Virtual screen calculation

BOOL    CALLBACK    GDIToTridentEnumProc (HMONITOR hMonitor, HDC hDC, RECT* rcMonitor, LPARAM lpUserData);
void    CalculateVirtualScreen (RECT& rcVirtualScreen);

//  GDI point to Trident point co-ordinate mapping

void    GDIToTrident (int& leftCoordinate, int& topCoordinate);
void    GDIToTrident (POINT& pt);
void    GDIToTrident (RECT& r);
void    GDIToTrident (HRGN hRgn);
void    TridentToGDI (int& leftCoordinate, int& topCoordinate);
void    TridentToGDI (POINT& pt);
void    TridentToGDI (RECT& r);
void    TridentToGDI (HRGN hRgn);

//  Component position validation

BOOL    CALLBACK    ValidateComponentPositionEnumProc (HMONITOR hMonitor, HDC hdcMonitor, RECT* r, LPARAM lParam);

void    GetNextComponentPosition (COMPPOS *pcp)

{
    int     iScreenWidth, iScreenHeight, iBorderSize;
    DWORD   dwComponentPosition, dwComponentLayer, dwRegDataScratch;
    HKEY    hKey;
    RECT    rcScreen;
    TCHAR   lpszDeskcomp[MAX_PATH];

    TBOOL(SystemParametersInfo(SPI_GETWORKAREA, 0, &rcScreen, false));

    // 99/04/13 vtan: A result of zero-width or zero-height occurred on a machine.
    // Make a defensive stand against this and assert that this happened but also
    // handle this cause so that division by zero doesn't happen.

    iScreenWidth = rcScreen.right - rcScreen.left;
    iScreenHeight = rcScreen.bottom - rcScreen.top;
    iBorderSize = GetSystemMetrics(SM_CYSMCAPTION);

    ASSERT(iScreenWidth > 0);       // get vtan
    ASSERT(iScreenHeight > 0);      // if any of
    ASSERT(iBorderSize > 0);        // these occur

    if ((iScreenWidth <= 0) || (iScreenHeight <= 0) || (iBorderSize <= 0))
    {
        pcp->iLeft = pcp->iTop = 0;
        pcp->dwWidth = MYCURHOME_WIDTH;
        pcp->dwHeight = MYCURHOME_HEIGHT;
    }
    else
    {

        // Get the number of components positioned. If no such registry key exists
        // or an error occurs then use 0.

        GetRegLocation(lpszDeskcomp, SIZECHARS(lpszDeskcomp), REG_DESKCOMP_GENERAL, NULL);
        dwComponentPosition = 0;
        if (RegCreateKeyEx(HKEY_CURRENT_USER, lpszDeskcomp, 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &hKey, &dwRegDataScratch) == ERROR_SUCCESS)
        {
            DWORD   regDataSize;

            regDataSize = sizeof(dwComponentPosition);
            TW32(SHQueryValueEx(hKey, REG_VAL_GENERAL_CCOMPPOS, NULL, &dwRegDataScratch, &dwComponentPosition, &regDataSize));
            TW32(RegCloseKey(hKey));
        }

        // Compute the layer we live on (see below).

        dwComponentLayer = dwComponentPosition / (COMPONENT_PER_ROW * COMPONENT_PER_COL);
        if (((dwComponentLayer * iBorderSize) > (DWORD)(iScreenWidth / (COMPONENT_PER_ROW + 1))) ||
            ((dwComponentLayer * iBorderSize) > (DWORD)(iScreenHeight / (COMPONENT_PER_COL + 1))))
        {
            int     iLayerModulo;

            // 99/04/29 vtan: It's possible for SystemParametersInfo(SPI_GETWORKAREA) to
            // return a work area that's small horizontally. Here's a repro scenario for
            // that.

            // 1. Set screen resolution 1280 x 1024.
            // 2. Move the taskbar to the left of the screen.
            // 3. Grow the taskbar to the right until the center of the screen.
            // 4. Open display control panel.
            // 5. Go to "Settings" tab.
            // 6. Change monitor resolution to 640x480.
            // 7. Click either "OK" or "Apply".
            // 8. BOOM - divide zero.

            iLayerModulo = (iScreenWidth / (COMPONENT_PER_ROW + 1) / iBorderSize);
            if (iLayerModulo != 0)
                dwComponentLayer %= iLayerModulo;
        }

        // Compute the position.  Assuming 3 components per row,
        // and 2 per column, we position components thusly:
        //
        //       +-------+
        //       |x 4 2 0|
        //       |x 5 3 1| <-- screen, divided into 4x3 block coordinates
        //       |x x x x|
        //       +-------+
        //
        // The 6th component sits in a new layer, offset down
        // and to the left of component 0 by the amount iBorder.
        //
        // The first calculation for iLeft and iTop determines the
        // block coordinate value (for instance, component 0 would
        // be at block coordinate value [3,0] and component 1 at [3,1]).
        //
        // The second calculation turns the block coordinate into
        // a screen coordinate.
        //
        // The third calculation adjusts for the border (always down and
        // to the right) and the layers (always down and to the left).

        pcp->iLeft = COMPONENT_PER_ROW - ((dwComponentPosition / COMPONENT_PER_COL) % COMPONENT_PER_ROW); // 3 3 2 2 1 1 3 3 2 2 1 1 ...
        pcp->iLeft *= (iScreenWidth / (COMPONENT_PER_ROW + 1));
        pcp->iLeft += iBorderSize - (dwComponentLayer * iBorderSize);

        pcp->iTop = dwComponentPosition % COMPONENT_PER_COL;  // 0 1 0 1 0 1 ...
        pcp->iTop *= (iScreenHeight / (COMPONENT_PER_COL + 1));
        pcp->iTop += iBorderSize + (dwComponentLayer * iBorderSize);
        pcp->iTop += GET_CYCAPTION;          //vtan: Added this to allow for the title area of the component window

        pcp->dwWidth = (iScreenWidth / (COMPONENT_PER_ROW + 1)) - 2 * iBorderSize;
        pcp->dwHeight = (iScreenHeight / (COMPONENT_PER_COL + 1)) - 2 * iBorderSize;
    }

    if (IS_BIDI_LOCALIZED_SYSTEM())
    {
       pcp->iLeft = iScreenWidth - (pcp->iLeft + pcp->dwWidth);
    }
    
}

void    IncrementComponentsPositioned (void)

{
    DWORD   dwRegDataScratch;
    HKEY    hKey;
    TCHAR   lpszDeskcomp[MAX_PATH];

    // Increment the registry count. If no such count exists create it.

    GetRegLocation(lpszDeskcomp, SIZECHARS(lpszDeskcomp), REG_DESKCOMP_GENERAL, NULL);
    if (RegCreateKeyEx(HKEY_CURRENT_USER, lpszDeskcomp, 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &hKey, &dwRegDataScratch) == ERROR_SUCCESS)
    {
        DWORD   dwComponentPosition, regDataSize;

        regDataSize = sizeof(dwComponentPosition);
        dwComponentPosition = 0;
        TW32(SHQueryValueEx(hKey, REG_VAL_GENERAL_CCOMPPOS, NULL, &dwRegDataScratch, &dwComponentPosition, &regDataSize));
        ++dwComponentPosition;
        TW32(RegSetValueEx(hKey, REG_VAL_GENERAL_CCOMPPOS, 0, REG_DWORD, reinterpret_cast<unsigned char*>(&dwComponentPosition), sizeof(dwComponentPosition)));
        TW32(RegCloseKey(hKey));
    }
}

//  vtan: Point arithmetic functions. Simple. It may be worth
//  converting these to inline C++ functions or macros if they
//  get used a lot.

void    SetPt (POINT& pt, LONG x, LONG y)

{
    pt.x = x;
    pt.y = y;
}

void    OffsetPt (POINT& pt, LONG dh, LONG dv)

{
    pt.x += dh;
    pt.y += dv;
}

BOOL    CALLBACK    GDIToTridentEnumProc (HMONITOR hMonitor, HDC hDC, RECT* rcMonitor, LPARAM lpUserData)

{
    RECT*   prcNew, rcOld;

    prcNew = reinterpret_cast<RECT*>(lpUserData);

    // Documentation for UnionRect does not specify whether the
    // RECT structures passed must be distinct. To be safe they
    // are passed as distinct structures.

    TBOOL(CopyRect(&rcOld, prcNew));
    TBOOL(UnionRect(prcNew, &rcOld, rcMonitor));
    return(TRUE);
}

void    CalculateVirtualScreen (RECT& rcVirtualScreen)

//  vtan: Calculates the virtual screen in GDI co-ordinates for
//  use in converting co-ordinates from trident scheme to GDI
//  scheme.

{
    TBOOL(SetRectEmpty(&rcVirtualScreen));
    TBOOL(EnumDisplayMonitors(NULL, NULL, GDIToTridentEnumProc, reinterpret_cast<LPARAM>(&rcVirtualScreen)));
}

void    GDIToTrident (int& leftCoordinate, int& topCoordinate)

{
    RECT    rcVirtualScreen;

    CalculateVirtualScreen(rcVirtualScreen);
    leftCoordinate -= rcVirtualScreen.left;
    topCoordinate -= rcVirtualScreen.top;
}

/*
void    GDIToTrident (POINT& pt)

{
    RECT    rcVirtualScreen;

    CalculateVirtualScreen(rcVirtualScreen);
    OffsetPt(pt, -rcVirtualScreen.left, -rcVirtualScreen.top);
}
*/

void    GDIToTrident (RECT& rc)

{
    RECT    rcVirtualScreen;

    CalculateVirtualScreen(rcVirtualScreen);
    TBOOL(OffsetRect(&rc, -rcVirtualScreen.left, -rcVirtualScreen.top));
}

void    GDIToTrident (HRGN hRgn)

{
    RECT    rcVirtualScreen;

    CalculateVirtualScreen(rcVirtualScreen);
    TBOOL(OffsetRgn(hRgn, -rcVirtualScreen.left, -rcVirtualScreen.top));
}

/*
void    TridentToGDI (int& leftCoordinate, int& topCoordinate)

{
    RECT    rcVirtualScreen;

    CalculateVirtualScreen(rcVirtualScreen);
    leftCoordinate += rcVirtualScreen.left;
    topCoordinate += rcVirtualScreen.top;
}
*/

/*
void    TridentToGDI (POINT& pt)

{
    RECT    rcVirtualScreen;

    CalculateVirtualScreen(rcVirtualScreen);
    OffsetPt(pt, +rcVirtualScreen.left, +rcVirtualScreen.top);
}
*/

void    TridentToGDI (RECT& rc)

{
    RECT    rcVirtualScreen;

    CalculateVirtualScreen(rcVirtualScreen);
    TBOOL(OffsetRect(&rc, +rcVirtualScreen.left, +rcVirtualScreen.top));
}

/*
void    TridentToGDI (HRGN hRgn)

{
    RECT    rcVirtualScreen;

    CalculateVirtualScreen(rcVirtualScreen);
    (BOOL)OffsetRgn(hRgn, +rcVirtualScreen.left, +rcVirtualScreen.top);
}
*/

//  98/08/14 vtan #196180, #196185: The following code validates
//  a new component's position within the current desktop area. This
//  allows a component to have co-ordinates that seem to be unusual
//  on a single monitor system (such as negative co-ordinates).

class   CRGN
{
    public:
                CRGN (void)                     {   mRgn = CreateRectRgn(0, 0, 0, 0);                               }
                CRGN (const RECT& rc)           {   mRgn = CreateRectRgnIndirect(&rc);                              }
                ~CRGN (void)                    {   TBOOL(DeleteObject(mRgn));                                      }

                operator HRGN (void)    const   {   return(mRgn);                                                   }
        void    SetRegion (const RECT& rc)      {   TBOOL(SetRectRgn(mRgn, rc.left, rc.top, rc.right, rc.bottom));  }
    private:
        HRGN    mRgn;
};

typedef struct
{
    bool    bAllowEntireDesktopRegion;
    int     iMonitorCount;
    CRGN    hRgn;
    int     iWorkAreaCount;
    RECT    *prcWorkAreaRects;
} tDesktopRegion;

void    ListView_GetWorkAreasAsGDI (HWND hWndListView, int iWorkAreaCount, RECT *prcWorkAreas)

{
    int     i;

    ListView_GetWorkAreas(hWndListView, iWorkAreaCount, prcWorkAreas);
    for (i = 0; i < iWorkAreaCount; ++i)
    {
        TridentToGDI(prcWorkAreas[i]);
    }
}

int     CopyMostSuitableListViewWorkAreaRect (const RECT *pcrcMonitor, int iListViewWorkAreaCount, const RECT *pcrcListViewWorkAreaRects, RECT *prcWorkArea)

{
    int         i, iResult;
    const RECT  *pcrcRects;

    // This function given a rectangle for a GDI monitor (typically the monitor's
    // work area) as well as given the desktop's list view work area rectangle
    // array (obtained by ListView_GetWorkArea()) will search the list view
    // work area array to find a match for the GDI monitor and use the list view
    // work area rectangle instead as this has docked toolbar information which
    // GDI does not have access to.

    // This function works on the principle that the list view rectangle is
    // always a complete subset of the GDI monitor rectangle which is true.
    // The list view rectangle may be smaller but it should never be bigger.

    // It's ok to pass a NULL pcrcListViewWorkAreaRects as long as
    // iListViewWorkAreaCount is 0.

    pcrcRects = pcrcListViewWorkAreaRects;
    iResult = -1;
    i = 0;
    while ((iResult == -1) && (i < iListViewWorkAreaCount))
    {
        RECT    rcIntersection;

        TBOOL(IntersectRect(&rcIntersection, pcrcMonitor, pcrcRects));
        if (EqualRect(&rcIntersection, pcrcRects) != 0)
        {
            iResult = i;
        }
        else
        {
            ++pcrcRects;
            ++i;
        }
    }
    if (iResult < 0)
    {
        TraceMsg(TF_WARNING, "CopyMostSuitableListViewWorkAreaRect() unable to find matching list view rectangle for GDI monitor rectangle");
        TBOOL(CopyRect(prcWorkArea, pcrcMonitor));
    }
    else
    {
        TBOOL(CopyRect(prcWorkArea, &pcrcListViewWorkAreaRects[iResult]));
    }
    return(iResult);
}

BOOL    GetMonitorInfoWithCompensation (int iMonitorCount, HMONITOR hMonitor, MONITORINFO *pMonitorInfo)

{
    BOOL    fResult;

    // 99/05/20 #338585 vtan: Transplanted the logic explained in the
    // comment below for #211510 from GetZoomRect to here so that other
    // functions can share the behavior. Remember that this ONLY applies
    // a single monitor system where there is part of the monitor's
    // rectangle excluded by a docked toolbar on the left or top of the
    // monitor. A very specific case.

    // 98/10/30 #211510 vtan: Oops. If the task bar is at the top of the
    // screen and there is only one monitor then the shell returns a work
    // area starting at (0, 0) instead of (0, 28); the same applies when
    // the task bar is at the left of the screen; this does NOT occur in
    // a multiple monitor setting. In the single monitor case GDI returns
    // a work area starting at (0, 28) so this code checks for the case
    // where there is a single monitor and offsets the GDI information to
    // (0, 0) so that it matches the shell work area which is compared
    // against in the while loop.

    fResult = GetMonitorInfo(hMonitor, pMonitorInfo);
    if ((fResult != 0) && (iMonitorCount == 1))
    {
        TBOOL(OffsetRect(&pMonitorInfo->rcWork, -pMonitorInfo->rcWork.left, -pMonitorInfo->rcWork.top));
    }
    return(fResult);
}

//  MonitorCountEnumProc()'s body is located in adjust.cpp

BOOL    CALLBACK    MonitorCountEnumProc (HMONITOR hMonitor, HDC dc, RECT *rc, LPARAM data);

BOOL    CALLBACK    ValidateComponentPositionEnumProc (HMONITOR hMonitor, HDC hdcMonitor, RECT* prc, LPARAM lpUserData)

{
    HRGN            hRgnDesktop;
    HMONITOR        hMonitorTopLeft, hMonitorTopRight;
    POINT           ptAbove;
    RECT            rcMonitor;
    MONITORINFO     monitorInfo;
    tDesktopRegion  *pDesktopRegion;

    pDesktopRegion = reinterpret_cast<tDesktopRegion*>(lpUserData);
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (GetMonitorInfoWithCompensation(pDesktopRegion->iMonitorCount, hMonitor, &monitorInfo) != 0)
    {
        TINT(CopyMostSuitableListViewWorkAreaRect(&monitorInfo.rcWork, pDesktopRegion->iWorkAreaCount, pDesktopRegion->prcWorkAreaRects, &rcMonitor));
    }
    else
    {
        TBOOL(CopyRect(&rcMonitor, prc));
    }

    // If this monitor does not have a monitor above it then
    // make the monitor rectangle one pixel lower from the
    // top.

    CRGN    hRgnMonitor(rcMonitor);

    if (!pDesktopRegion->bAllowEntireDesktopRegion)
    {

        // This bizarre little algorithm calculates the margins of the current
        // monitor that do not have a monitor above them. The rcExclude is the
        // the final rectangle that contains this information and is one pixel
        // high. This calculation is only valid if the entire desktop region
        // has been DISALLOWED (not zooming a component).

        // Note that the algorithm fails if there is a monitor that is above
        // this one but is contained within the confines of it. For example,
        // this monitor is at 1024x768 and the one above is at 640x480 and
        // centered. In this case it should be possible to drop the component
        // on the exact zero pixel point but this case is disallowed due to
        // this fault. No big deal.

        SetPt(ptAbove, rcMonitor.left, rcMonitor.top - 1);
        hMonitorTopLeft = MonitorFromPoint(ptAbove, MONITOR_DEFAULTTONULL);
        SetPt(ptAbove, rcMonitor.right, rcMonitor.top - 1);
        hMonitorTopRight = MonitorFromPoint(ptAbove, MONITOR_DEFAULTTONULL);
        if ((hMonitorTopLeft == NULL) && (hMonitorTopRight == NULL))
        {

            // No monitor above this one

            ++rcMonitor.top;
            hRgnMonitor.SetRegion(rcMonitor);
        }
        else if (hMonitorTopLeft != hMonitorTopRight)
        {
            RECT    rcExclude;

            // Either one or two different monitors above this one
            // == case is the same monitor completely covers this
            // monitor.

            TBOOL(SetRect(&rcExclude, rcMonitor.left, rcMonitor.top, rcMonitor.right, rcMonitor.top + 1));
            if (hMonitorTopLeft != NULL)
            {
                TBOOL(GetMonitorInfoWithCompensation(pDesktopRegion->iMonitorCount, hMonitorTopLeft, &monitorInfo));
                rcExclude.left = monitorInfo.rcWork.right + 1;
            }
            if (hMonitorTopRight != NULL)
            {
                TBOOL(GetMonitorInfoWithCompensation(pDesktopRegion->iMonitorCount, hMonitorTopRight, &monitorInfo));
                rcExclude.right = monitorInfo.rcWork.left;
            }

            CRGN    hRgnExclude(rcExclude);

            TINT(CombineRgn(hRgnMonitor, hRgnMonitor, hRgnExclude, RGN_DIFF));
        }
    }

    hRgnDesktop = pDesktopRegion->hRgn;
    TINT(CombineRgn(hRgnDesktop, hRgnDesktop, hRgnMonitor, RGN_OR));

    return(true);
}

void    ValidateComponentPosition (COMPPOS *pcp, DWORD dwComponentState, int iComponentType, bool *pbChangedPosition, bool *pbChangedSize)

{
    bool            bChangedPosition, bChangedSize;
    HRGN            hRgnDesktop;
    HWND            hWndDesktopListView;
    RECT            rcComponent, rcComponentTop;
    tDesktopRegion  desktopRegion;
    COMPPOS         defaultComponentPosition;

    bChangedPosition = bChangedSize = false;
    GetNextComponentPosition(&defaultComponentPosition);
    GDIToTrident(defaultComponentPosition.iLeft, defaultComponentPosition.iTop);

    // If the component has default left or top then give it the next
    // default component position.

    if ((pcp->iLeft == COMPONENT_DEFAULT_LEFT) && (pcp->iTop == COMPONENT_DEFAULT_TOP))
    {
        pcp->iLeft = defaultComponentPosition.iLeft;
        pcp->iTop = defaultComponentPosition.iTop;
        IncrementComponentsPositioned();
        bChangedPosition = true;
    }

    // If the component has default width or height then give it the
    // next default component size unless it is type COMP_TYPE_PICTURE

    // 98/10/02 #222449 vtan: Only change the size of an unpositioned
    // component if it's not a picture.

    if ((pcp->dwWidth == COMPONENT_DEFAULT_WIDTH) && (pcp->dwHeight == COMPONENT_DEFAULT_HEIGHT) && (iComponentType != COMP_TYPE_PICTURE))
    {
        pcp->dwWidth = defaultComponentPosition.dwWidth;
        pcp->dwHeight = defaultComponentPosition.dwHeight;
        bChangedSize = false;
    }

    // Make sure that the top line of the component is visible or at
    // least one pixel below the top most part of a virtual screen.

    // Check to see if the component has a negative width and height or
    // a width and height that is too small. The only exception to this
    // is if the component is a picture.

    desktopRegion.bAllowEntireDesktopRegion = IsZoomedState(dwComponentState);
    if (iComponentType != COMP_TYPE_PICTURE)
    {
        if (static_cast<int>(pcp->dwWidth) < 10)
        {
            pcp->dwWidth = defaultComponentPosition.dwWidth;
            bChangedSize = false;
        }
        if (static_cast<int>(pcp->dwHeight) < 10)
        {
            pcp->dwHeight= defaultComponentPosition.dwHeight;
            bChangedSize = false;
        }
    }
    TBOOL(SetRect(&rcComponent, pcp->iLeft, pcp->iTop, pcp->iLeft + pcp->dwWidth, pcp->iTop + pcp->dwHeight));
    TBOOL(CopyRect(&rcComponentTop, &rcComponent));
    rcComponentTop.bottom = rcComponentTop.top + 1;

    // Before calculating the desktopRegion as a region by using GDI calls
    // get the List View work area which will have information about docked
    // toolbars in addition to the taskbar which is the only thing that GDI
    // has. This will allow this function to invalidate regions occupied by
    // toolbars also.

    desktopRegion.iWorkAreaCount = 0;
    desktopRegion.prcWorkAreaRects = NULL;

    hWndDesktopListView = FindWindowEx(GetWindow(GetShellWindow(), GW_CHILD), NULL, WC_LISTVIEW, NULL);
    if (hWndDesktopListView != NULL)
    {
        DWORD   dwProcessID;

        GetWindowThreadProcessId(hWndDesktopListView, &dwProcessID);
        if (GetCurrentProcessId() == dwProcessID)
        {
            ListView_GetNumberOfWorkAreas(hWndDesktopListView, &desktopRegion.iWorkAreaCount);
            desktopRegion.prcWorkAreaRects = reinterpret_cast<RECT*>(LocalAlloc(GPTR, desktopRegion.iWorkAreaCount * sizeof(desktopRegion.prcWorkAreaRects[0])));
            ListView_GetWorkAreasAsGDI(hWndDesktopListView, desktopRegion.iWorkAreaCount, desktopRegion.prcWorkAreaRects);
        }
    }

    CRGN    hRgnComponentTop(rcComponentTop), hRgnResult;

    desktopRegion.iMonitorCount = 0;
    TBOOL(EnumDisplayMonitors(NULL, NULL, MonitorCountEnumProc, reinterpret_cast<LPARAM>(&desktopRegion.iMonitorCount)));
    TBOOL(EnumDisplayMonitors(NULL, NULL, ValidateComponentPositionEnumProc, reinterpret_cast<LPARAM>(&desktopRegion)));
    hRgnDesktop = desktopRegion.hRgn;
    GDIToTrident(hRgnDesktop);

    // 99/03/23 #266412 vtan: Make sure that the top pixel of the component is within
    // the visible desktop. This allows the deskmovr to be positioned over the
    // component and therefore allows it to be moved. If the deskmovr cannot be
    // positioned over it then "snap" the component back into the visible region
    // to a maximum best fit algorithm.

    if (CombineRgn(hRgnResult, hRgnDesktop, hRgnComponentTop, RGN_AND) == NULLREGION)
    {
        LONG        lDeltaX, lDeltaY;
        HMONITOR    hMonitorNearest;
        RECT        rcComponentGDI, rcMonitorWork, rcIntersection;
        MONITORINFO monitorInfo;

        TBOOL(CopyRect(&rcComponentGDI, &rcComponent));
        TridentToGDI(rcComponentGDI);
        hMonitorNearest = MonitorFromRect(&rcComponentGDI, MONITOR_DEFAULTTONEAREST);
        ASSERT(hMonitorNearest != NULL);
        monitorInfo.cbSize = sizeof(monitorInfo);
        TBOOL(GetMonitorInfoWithCompensation(desktopRegion.iMonitorCount, hMonitorNearest, &monitorInfo));
        TINT(CopyMostSuitableListViewWorkAreaRect(&monitorInfo.rcWork, desktopRegion.iWorkAreaCount, desktopRegion.prcWorkAreaRects, &rcMonitorWork));
        ++rcMonitorWork.top;
        lDeltaX = lDeltaY = 0;
        if (rcComponentGDI.left < rcMonitorWork.left)
            lDeltaX = rcMonitorWork.left - rcComponentGDI.left;
        if (rcComponentGDI.top < rcMonitorWork.top)
            lDeltaY = rcMonitorWork.top - rcComponentGDI.top;
        if (rcComponentGDI.right > rcMonitorWork.right)
            lDeltaX = rcMonitorWork.right - rcComponentGDI.right;
        if (rcComponentGDI.bottom > rcMonitorWork.bottom)
            lDeltaY = rcMonitorWork.bottom - rcComponentGDI.bottom;
        TBOOL(OffsetRect(&rcComponentGDI, lDeltaX, lDeltaY));
        TBOOL(IntersectRect(&rcIntersection, &rcComponentGDI, &rcMonitorWork));
        GDIToTrident(rcIntersection);
        pcp->iLeft = rcIntersection.left;
        pcp->iTop = rcIntersection.top;
        pcp->dwWidth = rcIntersection.right - rcIntersection.left;
        pcp->dwHeight = rcIntersection.bottom - rcIntersection.top;
        bChangedPosition = bChangedSize = true;
    }

    if (desktopRegion.prcWorkAreaRects != NULL)
        LocalFree(desktopRegion.prcWorkAreaRects);

    if (pbChangedPosition != NULL)
        *pbChangedPosition = bChangedPosition;
    if (pbChangedSize != NULL)
        *pbChangedSize = bChangedSize;
}

//  98/12/11 #250938 vtan: these two functions are lifted from
//  SHBrows2.cpp which is part of browseui.dll.

EXTERN_C    DWORD   WINAPI  IsSmartStart (void);

BOOL    IsICWCompleted (void)

{
    DWORD   dwICWCompleted, dwICWSize;

    dwICWCompleted = 0;
    dwICWSize = sizeof(dwICWCompleted);
    TW32(SHGetValue(HKEY_CURRENT_USER, TEXT(ICW_REGPATHSETTINGS), TEXT(ICW_REGKEYCOMPLETED), NULL, &dwICWCompleted, &dwICWSize));

    // 99/01/15 #272829 vtan: This is a horrible hack!!! If ICW has
    // not been run but settings have been made manually then values
    // in HKCU\Software\Microsoft\Windows\CurrentVersion\Internet Settings\Connections
    // exists with the values given. Look for the presence of a key
    // to resolve that settings are present but that ICW hasn't been
    // launched.

    // The ideal solution is to get ICW to make this determination
    // for us BUT TO NOT LAUNCH ICWCONN1.EXE IN THE PROCESS.
    // Currently it will only launch. There is no way to get the
    // desired result without a launch.

    // 99/02/01 #280138 vtan: Well the solution put in for #272829
    // doesn't work. So peeking at the CheckConnectionWizard()
    // source in inetcfg\export.cpp shows that it uses a
    // wininet.dll function to determine whether manually configured
    // internet settings exist. It also exports this function so
    // look for it and bind to it dynamically. This uses the
    // DELAY_LOAD macros in dllload.c

    if (dwICWCompleted == 0)
    {
        #define SMART_RUNICW    TRUE
        #define SMART_QUITICW   FALSE

        dwICWCompleted = BOOLIFY(IsSmartStart() == SMART_QUITICW);
    }
    return(dwICWCompleted != 0);
}

void    LaunchICW (void)

{
    static  bool    sbCheckedICW = false;

    if (!sbCheckedICW && !IsICWCompleted())
    {
        HINSTANCE   hICWInst;

        // Prevent an error in finding the ICW from causing this to
        // execute again and again and again.

        sbCheckedICW = true;
        hICWInst = LoadLibrary(TEXT("inetcfg.dll"));
        if (hICWInst != NULL)
        {
            PFNCHECKCONNECTIONWIZARD    pfnCheckConnectionWizard;

            pfnCheckConnectionWizard = reinterpret_cast<PFNCHECKCONNECTIONWIZARD>(GetProcAddress(hICWInst, "CheckConnectionWizard"));
            if (pfnCheckConnectionWizard != NULL)
            {
                DWORD   dwICWResult;

                // If the user cancels ICW then it needs to be launched
                // again. Allow this case.

                sbCheckedICW = false;

                pfnCheckConnectionWizard(ICW_LAUNCHFULL | ICW_LAUNCHMANUAL, &dwICWResult);
            }
            TBOOL(FreeLibrary(hICWInst));
        }
    }
}

BOOL    IsLocalPicture (LPCTSTR pszURL)

{
    return(!PathIsURL(pszURL) && IsUrlPicture(pszURL));
}

BOOL    DisableUndisplayableComponents (IActiveDesktop *pIAD)

{
    bool    bHasVisibleNonLocalPicture;
    int     iItemCount;

    // 98/12/16 vtan #250938: If ICW has not been run to completion then only
    // allow the user to show components that are local pictures of some sort.
    // If any components are not local pictures then hide these components,
    // tell the user why it happened and launch ICW.

    bHasVisibleNonLocalPicture = false;
    if (SUCCEEDED(pIAD->GetDesktopItemCount(&iItemCount, 0)))
    {
        int     i;

        for (i = 0; i < iItemCount; ++i)
        {
            COMPONENT   component;

            component.dwSize = sizeof(component);
            if (SUCCEEDED(pIAD->GetDesktopItem(i, &component, 0)) && (component.fChecked != 0))
            {
               bool    bIsVisibleNonLocalPicture;
               TCHAR   szComponentSource[INTERNET_MAX_URL_LENGTH];

               SHUnicodeToTChar(component.wszSource, szComponentSource, ARRAYSIZE(szComponentSource));
               bIsVisibleNonLocalPicture = !IsLocalPicture(szComponentSource);
               bHasVisibleNonLocalPicture = bHasVisibleNonLocalPicture || bIsVisibleNonLocalPicture;
               if (bIsVisibleNonLocalPicture)
               {
                   component.fChecked = FALSE;
                   THR(pIAD->ModifyDesktopItem(&component, COMP_ELEM_CHECKED));
               }
            }
         }
    }
    if (bHasVisibleNonLocalPicture)
    {

        // Apply the changes. This should recurse to CActiveDesktop::_SaveSettings()
        // but this code path is NOT taken because AD_APPLY_REFRESH is not passed in.
        // CActiveDesktop::_SaveSettings() calls this function!

        bHasVisibleNonLocalPicture = FAILED(pIAD->ApplyChanges(AD_APPLY_SAVE | AD_APPLY_HTMLGEN));

        // Notify the user what happened and launch ICW.

        ShellMessageBox(HINST_THISDLL, NULL, MAKEINTRESOURCE(IDS_COMP_ICW_DISABLE), MAKEINTRESOURCE(IDS_COMP_ICW_TITLE), MB_OK);
        LaunchICW();
    }
    return(bHasVisibleNonLocalPicture);
}

int GetIconCountForWorkArea(HWND hwndLV, LPCRECT prect, int crect, int iWorkAreaIndex)
{
    int iCount;

    iCount = ListView_GetItemCount(hwndLV);

    if (crect > 1) 
    {
        int i, iCountWorkArea = 0;

        for (i = 0; i < iCount; i++)
        {
            POINT pt;
            ListView_GetItemPosition(hwndLV, i, &pt);
            if (iWorkAreaIndex == GetWorkAreaIndexFromPoint(pt, prect, crect))
                iCountWorkArea++;
        }

        iCount = iCountWorkArea;
    }

    return iCount;
}

BOOL GetZoomRect(BOOL fFullScreen, BOOL fAdjustListview, int iTridentLeft, int iTridentTop, DWORD dwComponentWidth, DWORD dwComponentHeight, LPRECT prcZoom, LPRECT prcWork)
{
    HWND hwndLV = FindWindowEx(GetWindow(GetShellWindow(), GW_CHILD), NULL, WC_LISTVIEW, NULL);
    int icWorkAreas = 0, iWAC;
    RECT rcWork[LV_MAX_WORKAREAS];

    //
    // First calculate the Work Areas and Work Area index for the component, then perform the
    // particular operation based on lCommand.
    //
    if (hwndLV) {
        DWORD dwpid;
        GetWindowThreadProcessId(hwndLV, &dwpid);
        // This sucks, but the listview doesn't thunk these messages so we can't do
        // this inter-process!
        if (dwpid == GetCurrentProcessId())
        {
            ListView_GetNumberOfWorkAreas(hwndLV, &icWorkAreas);
            if (icWorkAreas <= LV_MAX_WORKAREAS)
                ListView_GetWorkAreas(hwndLV, icWorkAreas, &rcWork);
            else
                hwndLV = NULL;
        } else {
            return FALSE;
        }
    }

    // 98/10/07 vtan: This used to use a variable icWorkAreasAdd.
    // Removed this variable and directly increment icWorkAreas.
    // This doesn't affect the call to ListView_SetWorkAreas()
    // below because in this case hwndLV is NULL.

    if (icWorkAreas == 0)
    {
        RECT rc;

        ++icWorkAreas;
        SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID)&rc, 0);
        rcWork[0] = rc;
        hwndLV = NULL;
    }

    // 98/10/02 #212654 vtan: Changed the calculation code to find a
    // rectangle to zoom the component to based on GDI co-ordinates.
    // The component is passed in trident co-ordinates which are
    // stored in a RECT and converted to GDI co-ordinates. The system
    // then locates the monitor which the component is on and if it
    // cannot find the monitor then defaults to the primary. The
    // dimensions of the monitor are used before converting back to
    // trident co-ordinates.

    int             i, iMonitorCount;
    HMONITOR        hMonitor;
    RECT            rcComponentRect;
    MONITORINFO     monitorInfo;

    iMonitorCount = 0;
    TBOOL(EnumDisplayMonitors(NULL, NULL, MonitorCountEnumProc, reinterpret_cast<LPARAM>(&iMonitorCount)));
    TBOOL(SetRect(&rcComponentRect, iTridentLeft, iTridentTop, iTridentLeft + dwComponentWidth, iTridentTop + dwComponentHeight));
    TridentToGDI(rcComponentRect);
    hMonitor = MonitorFromRect(&rcComponentRect, MONITOR_DEFAULTTOPRIMARY);
    ASSERT(hMonitor != NULL);
    monitorInfo.cbSize = sizeof(monitorInfo);
    TBOOL(GetMonitorInfoWithCompensation(iMonitorCount, hMonitor, &monitorInfo));
    GDIToTrident(monitorInfo.rcWork);

    // 99/05/19 #340772 vtan: Always try to key off work areas returned
    // by ListView_GetWorkAreas because these take into account docked
    // toolbars which GDI does not. In this case the listview work areas
    // will always be the same rectangle when intersected with the GDI
    // work area. Use this rule to determine which listview work area
    // to use as the basis for the zoom rectangle.

    i = CopyMostSuitableListViewWorkAreaRect(&monitorInfo.rcWork, icWorkAreas, rcWork, prcZoom);
    if (i < 0)
    {
        i = 0;
    }
    if (prcWork != NULL)
    {
        TBOOL(CopyRect(prcWork, prcZoom));
    }
    iWAC = i;

    if (!fFullScreen)
    {
        // For the split case we shrink the work area down temporarily to the smallest rectangle
        // that can bound the current number of icons.  This will force the icons into that rectangle,
        // then restore it back to the way it was before.  Finally, we set the size of the split
        // component to fill the rest of the space.
        if (hwndLV) {
            int iCount, iItemsPerColumn, icxWidth, iRightOld;
            DWORD dwSpacing;

            iCount = GetIconCountForWorkArea(hwndLV, rcWork, icWorkAreas, iWAC);
            // Decrement the count so that rounding works right
            if (iCount)     
                iCount--;

            // Calculate the new width for the view rectangle
            dwSpacing = ListView_GetItemSpacing(hwndLV, FALSE);
            iItemsPerColumn = (rcWork[iWAC].bottom - rcWork[iWAC].top) / (HIWORD(dwSpacing));
            if (iItemsPerColumn)
                icxWidth = ((iCount / iItemsPerColumn) + 1) * (LOWORD(dwSpacing));
            else
                icxWidth = LOWORD(dwSpacing);

            // Don't let it get smaller than half the screen
            if (icxWidth > ((rcWork[iWAC].right - rcWork[iWAC].left) / 2))
                icxWidth = (rcWork[iWAC].right - rcWork[iWAC].left) / 2;

            if (fAdjustListview)
            {
                // Now take the old work area rectangle and shrink it to our new width
                iRightOld = rcWork[iWAC].right;
                rcWork[iWAC].right = rcWork[iWAC].left + icxWidth;
                ListView_SetWorkAreas(hwndLV, icWorkAreas, &rcWork);

                // Finally restore the old work area
                rcWork[iWAC].right = iRightOld;
                ListView_SetWorkAreas(hwndLV, icWorkAreas, &rcWork);
            }

            // Adjust the left coordinate of the zoom rect to reflect our calculated split amount
            // the rest of the screen.
            prcZoom->left += icxWidth;
        } else {
            // Fallback case, if there is no listview use 20% of the screen for the icons.
            prcZoom->left += ((prcZoom->right - prcZoom->left) * 2 / 10);
        }
    }

    return TRUE;
}

void ZoomComponent(COMPPOS * pcp, DWORD dwCurItemState, BOOL fAdjustListview)
{
    RECT rcZoom;

    if (GetZoomRect((dwCurItemState & IS_FULLSCREEN), fAdjustListview, pcp->iLeft, pcp->iTop, pcp->dwWidth, pcp->dwHeight, &rcZoom, NULL))
    {
        // Copy the new Zoom rectangle over and put it on the bottom
        pcp->iLeft = rcZoom.left;
        pcp->iTop = rcZoom.top;
        pcp->dwWidth = rcZoom.right - rcZoom.left;
        pcp->dwHeight = rcZoom.bottom - rcZoom.top;
        pcp->izIndex = 0;
    }
    else
    {
        // Failure implies we couldn't get the zoom rectangle through inter-process calls.  Set the
        // COMPONENTS_ZOOMDIRTY bit here so that when the desktop is refreshed we will recalculate
        // the zoom rectangles in-process inside of EnsureUpdateHTML.
        SetDesktopFlags(COMPONENTS_ZOOMDIRTY, COMPONENTS_ZOOMDIRTY);
    }
}

//
// PositionComponent will assign a screen position and
// make sure it fits on the screen.
//

void PositionComponent(COMPONENTA *pcomp, COMPPOS *pcp, int iCompType, BOOL fCheckItemState)

{

//  vtan: Vastly simplified routine. The work is now done in
//  ValidateComponentPosition.

    if (ISZOOMED(pcomp))
    {
        if (fCheckItemState)
        {
            SetStateInfo(&pcomp->csiRestored, pcp, IS_NORMAL);
            SetStateInfo(&pcomp->csiOriginal, pcp, pcomp->dwCurItemState);
        }
        ZoomComponent(pcp, pcomp->dwCurItemState, FALSE);
    }
    else
    {
        ValidateComponentPosition(pcp, pcomp->dwCurItemState, iCompType, NULL, NULL);
        if (fCheckItemState)
            SetStateInfo(&pcomp->csiOriginal, pcp, pcomp->dwCurItemState);
    }
}

typedef struct _tagFILETYPEENTRY {
    DWORD dwFlag;
    int iFilterId;
} FILETYPEENTRY;

FILETYPEENTRY afte[] = {
    { GFN_URL, IDS_URL_FILTER, },
    { GFN_CDF, IDS_CDF_FILTER, },
    { GFN_LOCALHTM, IDS_HTMLDOC_FILTER, },
    { GFN_PICTURE,  IDS_IMAGES_FILTER, },
    { GFN_LOCALMHTML, IDS_MHTML_FILTER, },
};

//
// Opens either an HTML page or a picture.
//
BOOL GetFileName(HWND hdlg, LPTSTR pszFileName, int iSize, int iTypeId[], DWORD dwFlags[])
{
    BOOL fRet = FALSE;

    if (dwFlags)
    {
        int i, iIndex, cchRead;
        TCHAR szFilter[MAX_PATH*4];

        //
        // Set the friendly name.
        //
        LPTSTR pchFilter = szFilter;
        int cchFilter = ARRAYSIZE(szFilter) - 2;    // room for term chars

        for(iIndex = 0; dwFlags[iIndex]; iIndex++)
        {
            cchRead = LoadString(HINST_THISDLL, iTypeId[iIndex], pchFilter, cchFilter);
            pchFilter += cchRead + 1;
            cchFilter -= cchRead + 1;

            //
            // Append the file filters.
            //
            BOOL fAddedToString = FALSE;
            for (i=0; (cchFilter>0) && (i<ARRAYSIZE(afte)); i++)
            {
                if (dwFlags[iIndex] & afte[i].dwFlag)
                {
                    if (fAddedToString)
                    {
                        *pchFilter++ = TEXT(';');
                        cchFilter--;
                    }
                    cchRead = LoadString(HINST_THISDLL, afte[i].iFilterId,
                                     pchFilter, cchFilter);
                    pchFilter += cchRead;
                    cchFilter -= cchRead;
                    fAddedToString = TRUE;
                }
            }
            *pchFilter++ = TEXT('\0');
        }

        //
        // Double-NULL terminate the string.
        //
        *pchFilter = TEXT('\0');

        TCHAR szBrowserDir[MAX_PATH];
        lstrcpy(szBrowserDir, pszFileName);
        PathRemoveArgs(szBrowserDir);
        PathRemoveFileSpec(szBrowserDir);

        TCHAR szBuf[MAX_PATH];
        LoadString(HINST_THISDLL, IDS_BROWSE, szBuf, ARRAYSIZE(szBuf));

        *pszFileName = TEXT('\0');

        OPENFILENAME ofn;
        ofn.lStructSize       = SIZEOF(ofn);
        ofn.hwndOwner         = hdlg;
        ofn.hInstance         = NULL;
        ofn.lpstrFilter       = szFilter;
        ofn.lpstrCustomFilter = NULL;
        ofn.nFilterIndex      = 1;
        ofn.nMaxCustFilter    = 0;
        ofn.lpstrFile         = pszFileName;
        ofn.nMaxFile          = iSize;
        ofn.lpstrInitialDir   = szBrowserDir;
        ofn.lpstrTitle        = szBuf;
        ofn.Flags             = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_NODEREFERENCELINKS;
        ofn.lpfnHook          = NULL;
        ofn.lpstrDefExt       = NULL;
        ofn.lpstrFileTitle    = NULL;

        fRet = GetOpenFileName(&ofn);
    }

    return fRet;
}

//
// Convert a pattern string to a bottom-up array of DWORDs,
// useful for BMP format files.
//
void PatternToDwords(LPTSTR psz, DWORD *pdwBits)
{
    DWORD i, dwVal;

    //
    // Get eight groups of numbers separated by non-numeric characters.
    //
    for (i=0; i<8; i++)
    {
        dwVal = 0;

        if (*psz != TEXT('\0'))
        {
            //
            // Skip over any non-numeric characters.
            //
            while (*psz && (!(*psz >= TEXT('0') && *psz <= TEXT('9'))))
            {
                psz++;
            }

            //
            // Get the next series of digits.
            //
            while (*psz && (*psz >= TEXT('0') && *psz <= TEXT('9')))
            {
                dwVal = dwVal*10 + *psz++ - TEXT('0');
            }
        }

        pdwBits[7-i] = dwVal;
    }
}

//
// Convert a pattern string to a top-down array of WORDs,
// useful for CreateBitmap().
//
void PatternToWords(LPTSTR psz, WORD *pwBits)
{
    WORD i, wVal;

    //
    // Get eight groups of numbers separated by non-numeric characters.
    //
    for (i=0; i<8; i++)
    {
        wVal = 0;

        if (*psz != TEXT('\0'))
        {
            //
            // Skip over any non-numeric characters.
            //
            while (*psz && (!(*psz >= TEXT('0') && *psz <= TEXT('9'))))
            {
                psz++;
            }

            //
            // Get the next series of digits.
            //
            while (*psz && ((*psz >= TEXT('0') && *psz <= TEXT('9'))))
            {
                wVal = wVal*10 + *psz++ - TEXT('0');
            }
        }

        pwBits[i] = wVal;
    }
}

BOOL IsValidPattern(LPCTSTR pszPat)
{
    BOOL fSawANumber = FALSE;

    //
    // We're mainly trying to filter multilingual upgrade cases
    // where the text for "(None)" is unpredictable.
    // Yes, I want to strangle the bastard who decided to
    // write out the none entry.
    //
    while (*pszPat)
    {
        if ((*pszPat < TEXT('0')) || (*pszPat > TEXT('9')))
        {
            //
            // It's not a number, it better be a space.
            //
            if (*pszPat != TEXT(' '))
            {
                return FALSE;
            }
        }
        else
        {
            fSawANumber = TRUE;
        }

        //
        // We avoid the need for AnsiNext by only advancing on US TCHARs.
        //
        pszPat++;
    }

    //
    // TRUE if we saw at least one digit and there were only digits and spaces.
    //
    return fSawANumber;
}

//
// Determines if the wallpaper can be supported in non-active desktop mode.
//
BOOL IsNormalWallpaper(LPCTSTR pszFileName)
{
    BOOL fRet = TRUE;

    if (pszFileName[0] == TEXT('\0'))
    {
        fRet = TRUE;
    }
    else
    {
        LPTSTR pszExt = PathFindExtension(pszFileName);

        //Check for specific files that can be shown only in ActiveDesktop mode!
        if((lstrcmpi(pszExt, TEXT(".GIF")) == 0) ||
           (lstrcmpi(pszExt, TEXT(".JPG")) == 0) ||
           (lstrcmpi(pszExt, TEXT(".JPE")) == 0) ||
           (lstrcmpi(pszExt, TEXT(".JPEG")) == 0) ||
           (lstrcmpi(pszExt, TEXT(".PNG")) == 0) ||
           (lstrcmpi(pszExt, TEXT(".HTM")) == 0) ||
           (lstrcmpi(pszExt, TEXT(".HTML")) == 0) ||
           (lstrcmpi(pszExt, TEXT(".HTT")) == 0))
           return FALSE;

        //Everything else (including *.BMP files) are "normal" wallpapers
    }
    return fRet;
}

//
// Determines if the wallpaper is a picture (vs. HTML).
//
BOOL IsWallpaperPicture(LPCTSTR pszWallpaper)
{
    BOOL fRet = TRUE;

    if (pszWallpaper[0] == TEXT('\0'))
    {
        //
        // Empty wallpapers count as empty pictures.
        //
        fRet = TRUE;
    }
    else
    {
        LPTSTR pszExt = PathFindExtension(pszWallpaper);

        if ((lstrcmpi(pszExt, TEXT(".HTM")) == 0) ||
            (lstrcmpi(pszExt, TEXT(".HTML")) == 0) ||
            (lstrcmpi(pszExt, TEXT(".HTT")) == 0))
        {
            fRet = FALSE;
        }
    }

    return fRet;
}

void OnDesktopSysColorChange(void)
{
    static COLORREF clrBackground = 0xffffffff;
    static COLORREF clrWindowText = 0xffffffff;

    //Get the new colors!
    COLORREF    clrNewBackground = GetSysColor(COLOR_BACKGROUND);
    COLORREF    clrNewWindowText = GetSysColor(COLOR_WINDOWTEXT);

    //Have we initialized these before?
    if(clrBackground != 0xffffffff)  //Have we initialized the statics yet?
    {
        // Our HTML file depends only on these two system colors.
        // Check if either of them has changed!
        // If not, no need to regenerate HTML file. 
        // This avoids infinite loop. And this is a nice optimization.
        if((clrBackground == clrNewBackground) &&
           (clrWindowText == clrNewWindowText))
            return; //No need to do anything. Just return.
    }

    // Remember the new colors in the statics.
    clrBackground = clrNewBackground;
    clrWindowText = clrNewWindowText;

    //
    // The desktop got a WM_SYSCOLORCHANGE.  We need to
    // regenerate the HTML if there are any system colors
    // showing on the desktop.  Patterns and the desktop
    // color are both based on system colors.
    //
    IActiveDesktop *pad;
    if (SUCCEEDED(CActiveDesktop_InternalCreateInstance((LPUNKNOWN *)&pad, IID_IActiveDesktop)))
    {
        BOOL fRegenerateHtml = FALSE;
        WCHAR szWallpaperW[INTERNET_MAX_URL_LENGTH];

        if (SUCCEEDED(pad->GetWallpaper(szWallpaperW, ARRAYSIZE(szWallpaperW), 0)))
        {
            if (!*szWallpaperW)
            {
                //
                // No wallpaper means the desktop color
                // or a pattern is showing - we need to
                // regenerate the desktop HTML.
                //
                fRegenerateHtml = TRUE;
            }
            else
            {
                TCHAR *pszWallpaper;
#ifdef UNICODE
                pszWallpaper = szWallpaperW;
#else
                CHAR szWallpaperA[INTERNET_MAX_URL_LENGTH];
                SHUnicodeToAnsi(szWallpaperW, szWallpaperA, ARRAYSIZE(szWallpaperA));
                pszWallpaper = szWallpaperA;
#endif
                if (IsWallpaperPicture(pszWallpaper))
                {
                    WALLPAPEROPT wpo = { SIZEOF(wpo) };
                    if (SUCCEEDED(pad->GetWallpaperOptions(&wpo, 0)))
                    {
                        if (wpo.dwStyle == WPSTYLE_CENTER)
                        {
                            //
                            // We have a centered picture,
                            // the pattern or desktop color
                            // could be leaking around the edges.
                            // We need to regenerate the desktop
                            // HTML.
                            //
                            fRegenerateHtml = TRUE;
                        }
                    }
                    else
                    {
                        TraceMsg(TF_WARNING, "SYSCLRCHG: Could not get wallpaper options!");
                    }
                }
            }
        }
        else
        {
            TraceMsg(TF_WARNING, "SYSCLRCHG: Could not get selected wallpaper!");
        }

        
        if (fRegenerateHtml)
        {
            DWORD  dwFlags = AD_APPLY_FORCE | AD_APPLY_HTMLGEN | AD_APPLY_REFRESH | AD_APPLY_DYNAMICREFRESH;
            WCHAR   wszPattern[MAX_PATH];
            //If we have a pattern, then we need to force a AD_APPLY_COMPLETEREFRESH
            // because we need to re-generate the pattern.bmp file which can not be 
            // done through dynamic HTML.
            if(SUCCEEDED(pad->GetPattern(wszPattern, sizeof(wszPattern), 0)))
            {
#ifdef UNICODE
                LPTSTR  szPattern = (LPTSTR)wszPattern;
#else
                CHAR   szPattern[MAX_PATH];
                SHUnicodeToAnsi(wszPattern, szPattern, sizeof(szPattern));
#endif //UNICODE
                if(IsValidPattern(szPattern))           //Does this have a pattern?
                    dwFlags &= ~(AD_APPLY_DYNAMICREFRESH);  //Then force a complete refresh!
                    
            }
            pad->ApplyChanges(dwFlags);
        }

        pad->Release();
    }
    else
    {
        TraceMsg(TF_WARNING, "SYSCLRCHG: Could not create CActiveDesktop!");
    }
}

//
// Convert a .URL file into its target.
//
void CheckAndResolveLocalUrlFile(LPTSTR pszFileName, int cchFileName)
{
    //
    // This function only works on *.URL files.
    //
    if (!PathIsURL(pszFileName))
    {
        LPTSTR pszExt;

        //
        // Check if the extension of this file is *.URL
        //
        pszExt = PathFindExtension(pszFileName);
        if (pszExt && *pszExt)
        {
            TCHAR  szUrl[15];
        
            LoadString(HINST_THISDLL, IDS_URL_EXTENSION, szUrl, ARRAYSIZE(szUrl));

            if (lstrcmpi(pszExt, szUrl) == 0)
            {
                HRESULT  hr;

                IUniformResourceLocator *purl;

                hr = CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
                              IID_IUniformResourceLocator,
                              (LPVOID *)&purl);

                if (EVAL(SUCCEEDED(hr)))  // This works for both Ansi and Unicode
                {
                    ASSERT(purl);

                    IPersistFile  *ppf;

                    hr = purl->QueryInterface(IID_IPersistFile, (LPVOID *)&ppf);
                    if (SUCCEEDED(hr))
                    {
                        WCHAR szFileW[MAX_PATH];
                        LPTSTR pszTemp;

                        SHTCharToUnicode(pszFileName, szFileW, ARRAYSIZE(szFileW));
                        ppf->Load(szFileW, STGM_READ);
                        hr = purl->GetURL(&pszTemp);    // Wow, an ANSI/UNICODE COM interface!
                        if (EVAL(SUCCEEDED(hr)))
                        {
                            StrCpyN(pszFileName, pszTemp, cchFileName);
                            CoTaskMemFree(pszTemp);
                        }
                        ppf->Release();
                    }
                    purl->Release();
                }
            }
        }
    }
}


void GetMyCurHomePageStartPos(int *piLeft, int *piTop, DWORD *pdwWidth, DWORD *pdwHeight)
{
#define INVALID_POS 0x80000000
    HKEY hkey;

    //
    // Assume nothing.
    //
    *piLeft = INVALID_POS;
    *piTop = INVALID_POS;
    *pdwWidth = INVALID_POS;
    *pdwHeight = INVALID_POS;

    //
    // Read from registry first.
    //
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Internet Explorer\\Main"), 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
    {
        DWORD dwType, cbData;

        cbData = SIZEOF(*piLeft);
        SHQueryValueEx(hkey, TEXT("MyCurHome_Left"), NULL, &dwType, (LPBYTE)piLeft, &cbData);

        cbData = SIZEOF(*piTop);
        SHQueryValueEx(hkey, TEXT("MyCurHome_Top"), NULL, &dwType, (LPBYTE)piTop, &cbData);

        cbData = SIZEOF(*pdwWidth);
        SHQueryValueEx(hkey, TEXT("MyCurHome_Width"), NULL, &dwType, (LPBYTE)pdwWidth, &cbData);

        cbData = SIZEOF(*pdwHeight);
        SHQueryValueEx(hkey, TEXT("MyCurHome_Height"), NULL, &dwType, (LPBYTE)pdwHeight, &cbData);

        RegCloseKey(hkey);
    }

    //
    // Fill in defaults when registry provides no info.
    //
    if (*piLeft == INVALID_POS)
    {
        *piLeft = -MYCURHOME_WIDTH;
    }
    if (*piTop == INVALID_POS)
    {
        *piTop = MYCURHOME_TOP;
    }
    if (*pdwWidth == INVALID_POS)
    {
        *pdwWidth = MYCURHOME_WIDTH;
    }
    if (*pdwHeight == INVALID_POS)
    {
        *pdwHeight = MYCURHOME_HEIGHT;
    }

    //
    // Convert negative values into positive ones.
    //
    RECT rect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, FALSE);
    if (*piLeft < 0)
    {
        *piLeft += (rect.right - rect.left) - *pdwWidth;
    }
    if (*piTop < 0)
    {
        *piTop += (rect.bottom - rect.top) - *pdwHeight;
    }

    // Find the virtual dimensions.
    EnumMonitorsArea ema;
    GetMonitorSettings(&ema);
    // Position it in the primary monitor.
    *piLeft -= ema.rcVirtualMonitor.left;
    *piTop -= ema.rcVirtualMonitor.top;
#undef INVALID_POS

}


//
// Silently adds a specified component to the desktop and use the given
//  apply flags using which you can avoid nested unnecessary HTML generation, 
//  or refreshing which may lead to racing conditions.
// 
//
BOOL AddDesktopComponentNoUI(DWORD dwApplyFlags, LPCTSTR pszUrl, LPCTSTR pszFriendlyName, int iCompType, int iLeft, int iTop, int iWidth, int iHeight, BOOL fChecked, DWORD dwCurItemState, BOOL fNoScroll, BOOL fCanResize)
{
    COMPONENTA Comp;
    BOOL    fRet = FALSE;
    HRESULT hres;

    //
    // Build the pcomp structure.
    //
    Comp.dwSize = sizeof(COMPONENTA);
    Comp.dwID = -1;
    Comp.iComponentType = iCompType;
    Comp.fChecked = fChecked;
    Comp.fDirty = FALSE;
    Comp.fNoScroll = fNoScroll;
    Comp.cpPos.dwSize = SIZEOF(COMPPOS);
    Comp.cpPos.iLeft = iLeft;
    Comp.cpPos.iTop = iTop;
    Comp.cpPos.dwWidth = iWidth;
    Comp.cpPos.dwHeight = iHeight;
    Comp.cpPos.izIndex = (dwCurItemState & IS_NORMAL) ? COMPONENT_TOP : 0;
    Comp.cpPos.fCanResize = fCanResize;
    Comp.cpPos.fCanResizeX = fCanResize;
    Comp.cpPos.fCanResizeY = fCanResize;
    Comp.cpPos.iPreferredLeftPercent = 0;
    Comp.cpPos.iPreferredTopPercent = 0;
    Comp.dwCurItemState = dwCurItemState;
    lstrcpyn(Comp.szSource, pszUrl, ARRAYSIZE(Comp.szSource));
    lstrcpyn(Comp.szSubscribedURL, pszUrl, ARRAYSIZE(Comp.szSource));
    if (pszFriendlyName)
    {
        lstrcpyn(Comp.szFriendlyName, pszFriendlyName, ARRAYSIZE(Comp.szFriendlyName));
    }
    else
    {
        Comp.szFriendlyName[0] = TEXT('\0');
    }

    IActiveDesktop *pActiveDesk;

    //
    // Add it to the system.
    //
    hres = CActiveDesktop_InternalCreateInstance((LPUNKNOWN *)&pActiveDesk, IID_IActiveDesktop);
    if (SUCCEEDED(hres))
    {
        COMPONENT  CompW;

        CompW.dwSize = sizeof(CompW);  //Required for the MultiCompToWideComp to work properly.

        MultiCompToWideComp(&Comp, &CompW);

        pActiveDesk->AddDesktopItem(&CompW, 0);
        pActiveDesk->ApplyChanges(dwApplyFlags);
        pActiveDesk->Release();
        fRet = TRUE;
    }

    return fRet;
}



// Little helper function used to change the safemode state
void SetSafeMode(DWORD dwFlags)
{
    IActiveDesktopP * piadp;

    if (SUCCEEDED(CActiveDesktop_InternalCreateInstance((LPUNKNOWN *)&piadp, IID_IActiveDesktopP)))
    {
        piadp->SetSafeMode(dwFlags);
        piadp->Release();
    }
}

/****************************************************************************
 *
 *  RefreshWebViewDesktop - regenerates desktop HTML from registry and updates
 *                          the screen
 *
 *  ENTRY:
 *      none
 *
 *  RETURNS:
 *      TRUE on success
 *      
 ****************************************************************************/
BOOL PokeWebViewDesktop(DWORD dwFlags)
{
    IActiveDesktop *pad;
    HRESULT     hres;
    BOOL        fRet = FALSE;

    hres = CActiveDesktop_InternalCreateInstance((LPUNKNOWN *)&pad, IID_IActiveDesktop);

    if (SUCCEEDED(hres))
    {
        pad->ApplyChanges(dwFlags);
        pad->Release();

        fRet = TRUE;
    }

    return (fRet);
}

#ifdef WE_WANT_DEFAULT_WALLPAPER

//The following is needed only if we add a default wallpaper.

void SetDefaultWallpaper()
{
    IActiveDesktop *pad;
    if (SUCCEEDED(CActiveDesktop_InternalCreateInstance((LPUNKNOWN *)&pad, IID_IActiveDesktop)))
    {
        DWORD dwWallpaperStyle;
        WALLPAPEROPT wpo;

        // If we have a wallpaper at the old location in the registry, we use it as the default
        TCHAR szWallpaper[INTERNET_MAX_URL_LENGTH];
        // Read the Wallpaper from the Old location.
        if(!GetStringFromReg(HKEY_CURRENT_USER, REGSTR_PATH_DESKTOP, REG_VAL_GENERAL_WALLPAPER, c_szNULL, szWallpaper, ARRAYSIZE(szWallpaper)))
            szWallpaper[0] = TEXT('\0');
        TrimWhiteSpace(szWallpaper);
        
        // Read the wallpaper style
        ReadWallpaperStyleFromReg(REGSTR_PATH_DESKTOP, &dwWallpaperStyle, FALSE);

        wpo.dwSize = SIZEOF(WALLPAPEROPT);
        pad->GetWallpaperOptions(&wpo, 0);
        wpo.dwStyle = dwWallpaperStyle;     //Set the wallpaper style!
        pad->SetWallpaperOptions(&wpo, 0);  //Set the wallpaper style options.
        
        TCHAR szDefaultWallpaper[MAX_PATH];
        GetDefaultWallpaper(szDefaultWallpaper, SIZECHARS(szDefaultWallpaper));
        
        if(szWallpaper[0] == TEXT('\0') || lstrcmpi(szWallpaper, g_szNone) == 0)
        {
            lstrcpy(szWallpaper, szDefaultWallpaper);
        }
        
        WCHAR *pwszWallpaper;
#ifndef UNICODE
        WCHAR   wszWallpaper[INTERNET_MAX_URL_LENGTH];
        SHAnsiToUnicode(szWallpaper, wszWallpaper, ARRAYSIZE(wszWallpaper));
        pwszWallpaper = wszWallpaper;
#else
        pwszWallpaper = szWallpaper;
#endif
        pad->SetWallpaper(pwszWallpaper, 0);
        pad->ApplyChanges(AD_APPLY_SAVE);
        pad->Release();
        // This is a kinda hack, but the best possible solution right now. The scenario is as follows.
        // The Memphis setup guys replace what the user specifies as the wallpaper in the old location
        // and restore it after setup is complete. But, SetDefaultWallpaper() gets called bet. these
        // two times and we are supposed to take a decision on whether to set the default htm wallpaper or not,
        // depending on what the user had set before the installation. The solution is to delay making
        // this decision until after the setup guys have restored the user's wallpaper. We do this in
        // CActiveDesktop::_ReadWallpaper(). We specify that SetDefaultWallpaper() was called by setting
        // the backup wallpaper in the new location to the default wallpaper.
        TCHAR szDeskcomp[MAX_PATH];

        wsprintf(szDeskcomp, REG_DESKCOMP_GENERAL, TEXT("\\"));
        SHSetValue(HKEY_CURRENT_USER, szDeskcomp,
            REG_VAL_GENERAL_BACKUPWALLPAPER, REG_SZ, (LPBYTE)szDefaultWallpaper,
            SIZEOF(TCHAR)*(lstrlen(szDefaultWallpaper)+1));
    }
}

#endif // IF_WE_WANT_DEFAULT_WALLPAPER

#define CCH_NONE 20 //big enough for "(None)" in german
TCHAR g_szNone[CCH_NONE] = {0};

void InitDeskHtmlGlobals(void)
{
    static fGlobalsInited = FALSE;

    if (fGlobalsInited == FALSE)
    {
        LoadString(HINST_THISDLL, IDS_WPNONE, g_szNone, ARRAYSIZE(g_szNone));

        fGlobalsInited = TRUE;
    }
}

//
// Loads the preview bitmap for property sheet pages.
//
HBITMAP LoadMonitorBitmap(void)
{
    HBITMAP hbm,hbmT;
    BITMAP bm;
    HBRUSH hbrT;
    HDC hdc;
    COLORREF c3df = GetSysColor(COLOR_3DFACE);

    hbm = LoadBitmap(HINST_THISDLL, MAKEINTRESOURCE(IDB_MONITOR));
    if (hbm == NULL)
    {
        return NULL;
    }

    //
    // Convert the "base" of the monitor to the right color.
    //
    // The lower left of the bitmap has a transparent color
    // we fixup using FloodFill
    //
    hdc = CreateCompatibleDC(NULL);
    hbmT = (HBITMAP)SelectObject(hdc, hbm);
    hbrT = (HBRUSH)SelectObject(hdc, GetSysColorBrush(COLOR_3DFACE));

    GetObject(hbm, sizeof(bm), &bm);

    ExtFloodFill(hdc, 0, bm.bmHeight-1, GetPixel(hdc, 0, bm.bmHeight-1), FLOODFILLSURFACE);

    //
    // Round off the corners.
    // The bottom two were done by the floodfill above.
    // The top left is important since SS_CENTERIMAGE uses it to fill gaps.
    // The top right should be rounded because the other three are.
    //
    SetPixel( hdc, 0, 0, c3df );
    SetPixel( hdc, bm.bmWidth-1, 0, c3df );

    //
    // Fill in the desktop here.
    //
    HBRUSH hbrOld = (HBRUSH)SelectObject(hdc, GetSysColorBrush(COLOR_DESKTOP));
    PatBlt(hdc, MON_X, MON_Y, MON_DX, MON_DY, PATCOPY);
    SelectObject(hdc, hbrOld);

    //
    // Clean up after ourselves.
    //
    SelectObject(hdc, hbrT);
    SelectObject(hdc, hbmT);
    DeleteDC(hdc);

    return hbm;
}

STDAPI_(DWORD) GetDesktopFlags(void)
{
    DWORD dwFlags = 0, dwType, cbSize = SIZEOF(dwFlags);
    TCHAR lpszDeskcomp[MAX_PATH];

    GetRegLocation(lpszDeskcomp, SIZECHARS(lpszDeskcomp), REG_DESKCOMP_COMPONENTS, NULL);

    SHGetValue(HKEY_CURRENT_USER, lpszDeskcomp, REG_VAL_COMP_GENFLAGS, &dwType, &dwFlags, &cbSize);

    return dwFlags;
}

STDAPI_(BOOL) SetDesktopFlags(DWORD dwMask, DWORD dwNewFlags)
{
    BOOL  fRet = FALSE;
    HKEY  hkey;
    DWORD dwDisposition;
    TCHAR lpszDeskcomp[MAX_PATH];

    GetRegLocation(lpszDeskcomp, SIZECHARS(lpszDeskcomp), REG_DESKCOMP_COMPONENTS, NULL);

    if (RegCreateKeyEx(HKEY_CURRENT_USER, (LPCTSTR)lpszDeskcomp, 
                       0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &hkey,
                       &dwDisposition) == ERROR_SUCCESS)
    {
        DWORD dwFlags;
        DWORD cbSize = SIZEOF(dwFlags);
        DWORD dwType;

        if (SHQueryValueEx(hkey, REG_VAL_COMP_GENFLAGS, NULL, &dwType,
                            (LPBYTE)&dwFlags, &cbSize) != ERROR_SUCCESS)
        {
            dwFlags = 0;
        }

        dwFlags = (dwFlags & ~dwMask) | (dwNewFlags & dwMask);

        if (RegSetValueEx(hkey, REG_VAL_COMP_GENFLAGS, 0, REG_DWORD,
                          (LPBYTE)&dwFlags, sizeof(dwFlags)) == ERROR_SUCCESS)
        {
            fRet = TRUE;
        }
    
        RegCloseKey(hkey);
    }

    return fRet;
}

BOOL UpdateComponentFlags(LPCTSTR pszCompId, DWORD dwMask, DWORD dwNewFlags)
{
    BOOL fRet = FALSE;
    TCHAR szRegPath[MAX_PATH];
    HKEY hkey;

    GetRegLocation(szRegPath, SIZECHARS(szRegPath), REG_DESKCOMP_COMPONENTS, NULL);

    lstrcat(szRegPath, TEXT("\\"));
    lstrcat(szRegPath, pszCompId);

    //Don't use RegCreateKeyEx here. It will result in Null components to be added.
    if (RegOpenKeyEx(HKEY_CURRENT_USER, szRegPath, 0,
                       KEY_READ | KEY_WRITE, &hkey) == ERROR_SUCCESS)
    {
        DWORD dwType, dwFlags, dwDataLength;

        dwDataLength = sizeof(DWORD);
        if(SHQueryValueEx(hkey, REG_VAL_COMP_FLAGS, NULL, &dwType, (LPBYTE)&dwFlags, &dwDataLength) != ERROR_SUCCESS)
        {
            dwFlags = 0;
        }        

        dwNewFlags = (dwFlags & ~dwMask) | (dwNewFlags & dwMask);

        if (RegSetValueEx(hkey, REG_VAL_COMP_FLAGS, 0, REG_DWORD, (LPBYTE)&dwNewFlags,
                          SIZEOF(DWORD)) == ERROR_SUCCESS)
        {
            fRet = TRUE;
        }

        SetDesktopFlags(COMPONENTS_DIRTY, COMPONENTS_DIRTY);

        RegCloseKey(hkey);
    }
    else
    {
        TraceMsg(TF_WARNING, "DS: Unable to UpdateComponentFlags");
    }

    return fRet;
}

DWORD GetCurrentState(LPTSTR pszCompId)
{
    TCHAR szRegPath[MAX_PATH];
    DWORD cbSize, dwType, dwCurState;

    GetRegLocation(szRegPath, SIZECHARS(szRegPath), REG_DESKCOMP_COMPONENTS, NULL);
    lstrcat(szRegPath, TEXT("\\"));
    lstrcat(szRegPath, pszCompId);

    cbSize = sizeof(dwCurState);

    if (SHGetValue(HKEY_CURRENT_USER, szRegPath, REG_VAL_COMP_CURSTATE, &dwType, &dwCurState, &cbSize) != ERROR_SUCCESS)
        dwCurState = IS_NORMAL;

    return dwCurState;
}

BOOL GetSavedStateInfo(LPTSTR pszCompId, LPCOMPSTATEINFO    pCompState, BOOL  fRestoredState)
{
    BOOL fRet = FALSE;
    TCHAR szRegPath[MAX_PATH];
    HKEY hkey;
    LPTSTR  lpValName = (fRestoredState ? REG_VAL_COMP_RESTOREDSTATEINFO : REG_VAL_COMP_ORIGINALSTATEINFO);

    GetRegLocation(szRegPath, SIZECHARS(szRegPath), REG_DESKCOMP_COMPONENTS, NULL);
    lstrcat(szRegPath, TEXT("\\"));
    lstrcat(szRegPath, pszCompId);

    //No need to use RegCreateKeyEx here. Use RegOpenKeyEx instead.
    if (RegOpenKeyEx(HKEY_CURRENT_USER, szRegPath, 0,
                       KEY_READ, &hkey) == ERROR_SUCCESS)
    {
        DWORD   cbSize, dwType;
        
        cbSize = SIZEOF(*pCompState);
        dwType = REG_BINARY;
        
        if (SHQueryValueEx(hkey, lpValName, NULL, &dwType, (LPBYTE)pCompState, &cbSize) != ERROR_SUCCESS)
        {
            //If the item state is missing, read the item current position and
            // and return that as the saved state.
            COMPPOS cpPos;

            cbSize = SIZEOF(cpPos);
            dwType = REG_BINARY;
            if (SHQueryValueEx(hkey, REG_VAL_COMP_POSITION, NULL, &dwType, (LPBYTE)&cpPos, &cbSize) != ERROR_SUCCESS)
            {
                ZeroMemory(&cpPos, SIZEOF(cpPos));
            }            
            SetStateInfo(pCompState, &cpPos, IS_NORMAL);
        }

        RegCloseKey(hkey);
    }
    else
        TraceMsg(TF_WARNING, "DS: Unable to get SavedStateInfo()");

    return fRet;
 }


BOOL UpdateDesktopPosition(LPTSTR pszCompId, int iLeft, int iTop, DWORD dwWidth, DWORD dwHeight, int izIndex,
                            BOOL    fSaveRestorePos, BOOL fSaveOriginal, DWORD dwCurState)
{
    BOOL fRet = FALSE;
    TCHAR szRegPath[MAX_PATH];
    HKEY hkey;

    GetRegLocation(szRegPath, SIZECHARS(szRegPath), REG_DESKCOMP_COMPONENTS, NULL);
    lstrcat(szRegPath, TEXT("\\"));
    lstrcat(szRegPath, pszCompId);

    //Don't use RegCreateKeyEx here; It will result in a NULL component being added.
    if (RegOpenKeyEx(HKEY_CURRENT_USER, szRegPath, 0,
                       KEY_READ | KEY_WRITE, &hkey) == ERROR_SUCCESS)
    {
        COMPPOS         cp;
        DWORD           dwType;
        DWORD           dwDataLength;
        COMPSTATEINFO   csi;

        dwType = REG_BINARY;
        dwDataLength = sizeof(COMPPOS);

        if(SHQueryValueEx(hkey, REG_VAL_COMP_POSITION, NULL, &dwType, (LPBYTE)&cp, &dwDataLength) != ERROR_SUCCESS)
        {
            cp.fCanResize = cp.fCanResizeX = cp.fCanResizeY = TRUE;
            cp.iPreferredLeftPercent = cp.iPreferredTopPercent = 0;
        }

        //Read the current State
        dwType = REG_DWORD;
        dwDataLength = SIZEOF(csi.dwItemState);
        if (SHQueryValueEx(hkey, REG_VAL_COMP_CURSTATE, NULL, &dwType, (LPBYTE)&csi.dwItemState, &dwDataLength) != ERROR_SUCCESS)
        {
            csi.dwItemState = IS_NORMAL;
        }

        if(fSaveRestorePos)
        {
            //We have just read the current position; Let's save it as the restore position.
            SetStateInfo(&csi, &cp, csi.dwItemState);

            //Now that we know the complete current state, save it as the restore state!
            RegSetValueEx(hkey, REG_VAL_COMP_RESTOREDSTATEINFO, 0, REG_BINARY, (LPBYTE)&csi, SIZEOF(csi));
        }

        //Save the current state too!
        if(dwCurState)
            RegSetValueEx(hkey, REG_VAL_COMP_CURSTATE, 0, REG_DWORD, (LPBYTE)&dwCurState, SIZEOF(dwCurState));
            
        cp.dwSize = sizeof(COMPPOS);
        cp.iLeft = iLeft;
        cp.iTop = iTop;
        cp.dwWidth = dwWidth;
        cp.dwHeight = dwHeight;
        cp.izIndex = izIndex;

        if (fSaveOriginal) {
            SetStateInfo(&csi, &cp, csi.dwItemState);
            RegSetValueEx(hkey, REG_VAL_COMP_ORIGINALSTATEINFO, 0, REG_BINARY, (LPBYTE)&csi, SIZEOF(csi));
        }

        if (RegSetValueEx(hkey, REG_VAL_COMP_POSITION, 0, REG_BINARY, (LPBYTE)&cp,
                          SIZEOF(cp)) == ERROR_SUCCESS)
        {
            fRet = TRUE;
        }

        // Don't need to mark as dirty if we're just saving the original pos
        if (!fSaveOriginal)
            SetDesktopFlags(COMPONENTS_DIRTY, COMPONENTS_DIRTY);

        RegCloseKey(hkey);

    }
    else
    {
        TraceMsg(TF_WARNING, "DS: Unable to UpdateDesktopPosition");
    }

    return fRet;
}

void    EnsureFilePathIsPresent (LPCTSTR pszFilePath)

//  Recursively process (in reverse) a path and create the first
//  directory that does not exist and when unwinding the calling
//  chain create each subsequent directory in the path until the
//  whole original path is created.

{
    TCHAR   szDesktopFileDirectory[MAX_PATH];

    lstrcpy(szDesktopFileDirectory, pszFilePath);
    ASSERT(lstrlen(szDesktopFileDirectory) > lstrlen(TEXT("C:\\")));      // something wrong if the root directory is hit
    if ((PathRemoveFileSpec(szDesktopFileDirectory) != 0) && (GetFileAttributes(szDesktopFileDirectory) == 0xFFFFFFFF) && (CreateDirectory(szDesktopFileDirectory, NULL) == 0))
    {
        EnsureFilePathIsPresent(szDesktopFileDirectory);
        TBOOL(CreateDirectory(szDesktopFileDirectory, NULL));
    }
}

void GetPerUserFileName(LPTSTR pszOutputFileName, DWORD dwSize, LPTSTR pszPartialFileName)
{
    LPITEMIDLIST    pidlAppData;

    *pszOutputFileName = TEXT('\0');

    if(dwSize < MAX_PATH)
    {
        ASSERT(FALSE);
        return;
    }

    if(EVAL(SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidlAppData))))
    {
        SHGetPathFromIDList(pidlAppData, pszOutputFileName);
        PathAppend(pszOutputFileName, pszPartialFileName);
        ILFree(pidlAppData);
    }
    EnsureFilePathIsPresent(pszOutputFileName);
}

void GetRegLocation(LPTSTR lpszResult, DWORD cchResult, LPCTSTR lpszKey, LPCTSTR lpszScheme)
{
    TCHAR szSubkey[MAX_PATH];
    DWORD dwDataLength = sizeof(szSubkey) - 2 * sizeof(TCHAR);
    DWORD dwType;

    lstrcpy(szSubkey, TEXT("\\"));

    //  use what was given or get it from the registry
    if (lpszScheme)
        StrCatBuff(szSubkey, lpszScheme, SIZECHARS(szSubkey));
    else
        SHGetValue(HKEY_CURRENT_USER, REG_DESKCOMP_SCHEME, REG_VAL_SCHEME_DISPLAY, &dwType,
            (LPBYTE)(szSubkey) + sizeof(TCHAR), &dwDataLength);

    if (szSubkey[1])
        StrCatBuff(szSubkey, TEXT("\\"), SIZECHARS(szSubkey));

    wnsprintf(lpszResult, cchResult, lpszKey, szSubkey);
}

BOOL ValidateFileName(HWND hwnd, LPCTSTR pszFilename, int iTypeString)
{
    BOOL fRet = TRUE;

    DWORD dwAttributes = GetFileAttributes(pszFilename);
    if ((dwAttributes != 0xFFFFFFFF) &&
        (dwAttributes & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN)))
    {
        TCHAR szType1[64];
        TCHAR szType2[64];

        LoadString(HINST_THISDLL, iTypeString, szType1, ARRAYSIZE(szType1));
        LoadString(HINST_THISDLL, iTypeString+1, szType2, ARRAYSIZE(szType2));
        if (ShellMessageBox(HINST_THISDLL, hwnd,
                            MAKEINTRESOURCE(IDS_VALIDFN_FMT),
                            MAKEINTRESOURCE(IDS_VALIDFN_TITLE),
                            MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2,
                            szType1, szType2) == IDNO)
        {
            fRet = FALSE;
        }
    }

    return fRet;
}

void GetWallpaperDirName(LPTSTR lpszWallPaperDir, int iBuffSize)
{
    //Compute the default wallpaper name.
    GetWindowsDirectory(lpszWallPaperDir, iBuffSize);
    lstrcat(lpszWallPaperDir, DESKTOPHTML_WEB_DIR);

    //Read it from the registry key, if it is set!
    DWORD dwType;
    DWORD cbData = (DWORD)iBuffSize;
    SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_SETUP, c_szWallPaperDir, &dwType, (LPVOID)lpszWallPaperDir, &cbData);
    // BUGBUG: Currently the type is wrongly being set to REG_SZ. When this is changed by setup, uncomment the if() below.
    // if (dwType == REG_EXPAND_SZ)
    {
        TCHAR szExp[MAX_PATH];

        SHExpandEnvironmentStrings(lpszWallPaperDir, szExp, ARRAYSIZE(szExp));
        lstrcpyn(lpszWallPaperDir, szExp, iBuffSize);
    }
}

BOOL CALLBACK MultiMonEnumAreaCallBack(HMONITOR hMonitor, HDC hdc, LPRECT lprc, LPARAM lData)
{
    EnumMonitorsArea* pEMA = (EnumMonitorsArea*)lData;
    
    if (pEMA->iMonitors > LV_MAX_WORKAREAS - 1)
    {
        //ignore the other monitors because we can only handle up to LV_MAX_WORKAREAS
        //BUGBUG: should we dynamically allocate this?
        return FALSE;
    }
    GetMonitorRect(hMonitor, &pEMA->rcMonitor[pEMA->iMonitors]);
    GetMonitorWorkArea(hMonitor, &pEMA->rcWorkArea[pEMA->iMonitors]);
    if(pEMA->iMonitors == 0)
    {
        pEMA->rcVirtualMonitor.left = pEMA->rcMonitor[0].left;
        pEMA->rcVirtualMonitor.top = pEMA->rcMonitor[0].top;
        pEMA->rcVirtualMonitor.right = pEMA->rcMonitor[0].right;
        pEMA->rcVirtualMonitor.bottom = pEMA->rcMonitor[0].bottom;

        pEMA->rcVirtualWorkArea.left = pEMA->rcWorkArea[0].left;
        pEMA->rcVirtualWorkArea.top = pEMA->rcWorkArea[0].top;
        pEMA->rcVirtualWorkArea.right = pEMA->rcWorkArea[0].right;
        pEMA->rcVirtualWorkArea.bottom = pEMA->rcWorkArea[0].bottom;
    }
    else
    {
        if(pEMA->rcMonitor[pEMA->iMonitors].left < pEMA->rcVirtualMonitor.left)
        {
            pEMA->rcVirtualMonitor.left = pEMA->rcMonitor[pEMA->iMonitors].left;
        }
        if(pEMA->rcMonitor[pEMA->iMonitors].top < pEMA->rcVirtualMonitor.top)
        {
            pEMA->rcVirtualMonitor.top = pEMA->rcMonitor[pEMA->iMonitors].top;
        }
        if(pEMA->rcMonitor[pEMA->iMonitors].right > pEMA->rcVirtualMonitor.right)
        {
            pEMA->rcVirtualMonitor.right = pEMA->rcMonitor[pEMA->iMonitors].right;
        }
        if(pEMA->rcMonitor[pEMA->iMonitors].bottom > pEMA->rcVirtualMonitor.bottom)
        {
            pEMA->rcVirtualMonitor.bottom = pEMA->rcMonitor[pEMA->iMonitors].bottom;
        }

        if(pEMA->rcWorkArea[pEMA->iMonitors].left < pEMA->rcVirtualWorkArea.left)
        {
            pEMA->rcVirtualWorkArea.left = pEMA->rcWorkArea[pEMA->iMonitors].left;
        }
        if(pEMA->rcWorkArea[pEMA->iMonitors].top < pEMA->rcVirtualWorkArea.top)
        {
            pEMA->rcVirtualWorkArea.top = pEMA->rcWorkArea[pEMA->iMonitors].top;
        }
        if(pEMA->rcWorkArea[pEMA->iMonitors].right > pEMA->rcVirtualWorkArea.right)
        {
            pEMA->rcVirtualWorkArea.right = pEMA->rcWorkArea[pEMA->iMonitors].right;
        }
        if(pEMA->rcWorkArea[pEMA->iMonitors].bottom > pEMA->rcVirtualWorkArea.bottom)
        {
            pEMA->rcVirtualWorkArea.bottom = pEMA->rcWorkArea[pEMA->iMonitors].bottom;
        }
    }
    pEMA->iMonitors++;
    return TRUE;
}

void GetMonitorSettings(EnumMonitorsArea* ema)
{
    ema->iMonitors = 0;

    ema->rcVirtualMonitor.left = 0;
    ema->rcVirtualMonitor.top = 0;
    ema->rcVirtualMonitor.right = 0;
    ema->rcVirtualMonitor.bottom = 0;

    ema->rcVirtualWorkArea.left = 0;
    ema->rcVirtualWorkArea.top = 0;
    ema->rcVirtualWorkArea.right = 0;
    ema->rcVirtualWorkArea.bottom = 0;

    EnumDisplayMonitors(NULL, NULL, MultiMonEnumAreaCallBack, (LPARAM)ema);
}

int _GetWorkAreaIndexWorker(POINT pt, LPCRECT prect, int crect)
{
    int iIndex;

    for (iIndex = 0; iIndex < crect; iIndex++)
    {
        if (PtInRect(&prect[iIndex], pt))
        {
            return iIndex;
        }
    }

    return -1;
}

int GetWorkAreaIndexFromPoint(POINT pt, LPCRECT prect, int crect)
{
    ASSERT(crect);

    // Map to correct coords...
    pt.x += prect[0].left;
    pt.y += prect[0].top;

    return _GetWorkAreaIndexWorker(pt, prect, crect);
}

int GetWorkAreaIndex(COMPPOS *pcp, LPCRECT prect, int crect, LPPOINT lpptVirtualTopLeft)
{
    POINT ptComp;
    ptComp.x = pcp->iLeft + lpptVirtualTopLeft->x;
    ptComp.y = pcp->iTop + lpptVirtualTopLeft->y;

    return _GetWorkAreaIndexWorker(ptComp, prect, crect);
}

// Prepends the Web wallpaper directory or the system directory to szWallpaper, if necessary
// (i.e., if the path is not specified). The return value is in szWallpaperWithPath, which is iBufSize
// bytes long
void GetWallpaperWithPath(LPCTSTR szWallpaper, LPTSTR szWallpaperWithPath, int iBufSize)
{
    if(szWallpaper[0] && lstrcmpi(szWallpaper, g_szNone) != 0 && !StrChr(szWallpaper, TEXT('\\'))
            && !StrChr(szWallpaper, TEXT(':'))) // The file could be d:foo.bmp
    {
        // If the file is a normal wallpaper, we prepend the windows directory to the filename
        if(IsNormalWallpaper(szWallpaper))
        {
            GetWindowsDirectory(szWallpaperWithPath, iBufSize);
        }
        // else we prepend the wallpaper directory to the filename
        else
        {
            GetWallpaperDirName(szWallpaperWithPath, iBufSize);
        }
        lstrcat(szWallpaperWithPath, TEXT("\\"));
        lstrcat(szWallpaperWithPath, szWallpaper);
    }
    else
    {
        lstrcpyn(szWallpaperWithPath, szWallpaper, iBufSize);
    }
}

#ifdef WE_WANT_DEFAULT_WALLPAPER

// The default wallpaper's name might be different depending on the platform. Hence this function.
void GetDefaultWallpaper(LPTSTR psz, DWORD cch)
{
    GetWallpaperDirName(psz, cch);
    StrCatBuff(psz, TEXT("\\"), cch);

    if (g_bRunOnMemphis)
    {
        StrCatBuff(psz, DESKTOPHTML_DEFAULT_MEMPHIS_WALLPAPER, cch);
    }
    else if (g_bRunOnNT5)
    {
        StrCatBuff(psz, DESKTOPHTML_DEFAULT_NT5_WALLPAPER, cch);
    }
    else
    {
        StrCatBuff(psz, DESKTOPHTML_DEFAULT_WALLPAPER, cch);
    }
}

#endif //WE_WANT_DEFAULT_WALLPAPER

BOOL GetViewAreas(LPRECT lprcViewAreas, int* pnViewAreas)
{
    BOOL bRet = FALSE;
    HWND hwndDesktop = GetShellWindow();    // This is the "normal" desktop
    
    if (hwndDesktop && IsWindow(hwndDesktop))
    {
        DWORD dwProcID, dwCurrentProcID;
        
        GetWindowThreadProcessId(hwndDesktop, &dwProcID);
        dwCurrentProcID = GetCurrentProcessId();
        if (dwCurrentProcID == dwProcID) {
            SendMessage(hwndDesktop, DTM_GETVIEWAREAS, (WPARAM)pnViewAreas, (LPARAM)lprcViewAreas);
            if (*pnViewAreas <= 0)
            {
                bRet = FALSE;
            }
            else
            {
                bRet = TRUE;
            }
        }
        else
        {
            bRet = FALSE;
        }
    }
    return bRet;
}

// We need to enforce a minimum size for the deskmovr caption since it doesn't look
// right drawn any smaller
int GetcyCaption()
{
    int cyCaption = GetSystemMetrics(SM_CYSMCAPTION);

    if (cyCaption < 15)
        cyCaption = 15;

    cyCaption -= GetSystemMetrics(SM_CYBORDER);

    return cyCaption;
}

