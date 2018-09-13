#include "stdafx.h"
#pragma hdrstop

#ifdef POSTSPLIT

//  98/10/02 vtan: Multiple monitor bug fixes.

//  Given an old monitor layout, a new monitor layout and a component
//  this function makes sure that it is in the new layout using a
//  monitor relative scheme. It tries to preserve the position
//  relatively within a given monitor. If the resolution of the monitor
//  changes then this needs to be accounted for also.

//  Preserve the component within a given monitor if the monitor is the
//  same HMONITOR but the GDI co-ordinates change.

//  If the same monitor cannot be located then move the component to the
//  nearest monitor and position it as best on that monitor.

//  If all else fails then use the default component positioning
//  algorithm. This should never happen.

//  Preserve the component within a given monitor if the resolution
//  changes. Preserve the size of the component by MOVING the component
//  in the X and Y axis until the left or the top of the monitor is
//  reached. When either axis reaches 0 then reduce the size of the
//  component until it fits within the given new resolution.

static  const   int     kNameSize = 16;

typedef struct
{
    HMONITOR    miHMONITOR;
    RECT        miDisplayAreaRect,
                miWorkAreaRect;
} tMonitorInfoRec, *tMonitorInfoPtr;

typedef struct
{
    int                 miaCount, miaIndex;
    RECT                miaVirtualScreen;
    tMonitorInfoRec     miaMonitors[1];
} tMonitorInfoArrayRec, *tMonitorInfoArrayPtr;

typedef struct
{
    bool            ciValidData, ciVisible, ciRepositioned;
    TCHAR           ciName[kNameSize];      // this is not excessive but limited
    DWORD           ciItemState;
    int             ciType;
    COMPPOS         ciPosition;
    COMPSTATEINFO   ciStateInfo;
} tComponentInfoRec, *tComponentInfoPtr;

static  tMonitorInfoArrayPtr    gOldMonitorArray = NULL;
static  tMonitorInfoArrayPtr    gNewMonitorArray = NULL;

//  Functions located in Dutil.cpp used for co-ordinate mapping.

void    SetPt (POINT& pt, LONG x, LONG y);
void    OffsetPt (POINT& pt, LONG dh, LONG dv);
void    CalculateVirtualScreen (RECT& virtualScreen);

//  Local function prototypes.

BOOL    CALLBACK    MonitorCountEnumProc (HMONITOR hMonitor, HDC dc, RECT *rc, LPARAM data);
BOOL    CALLBACK    MonitorCalculateEnumProc (HMONITOR hMonitor, HDC dc, RECT *rc, LPARAM data);
HRESULT CalculateCurrentMonitorArray(void);
void    ApplyCurrentMonitorArray (void);
bool    EqualMonitorArray (tMonitorInfoArrayPtr oldMonitorArray, tMonitorInfoArrayPtr newMonitorArray);
int     IndexOfMonitor (tMonitorInfoArrayPtr pMIA, HMONITOR hMonitor);
int     IndexOfMonitor (tMonitorInfoArrayPtr pMIA, POINT& pt);
int     IndexOfMonitor (tMonitorInfoArrayPtr pMIA, RECT& rc);
bool    RepositionDesktopRect (RECT& rcComponent, tMonitorInfoArrayPtr oldMonitorArray, tMonitorInfoArrayPtr newMonitorArray);
bool    RepositionDesktopComponent (COMPPOS& componentPosition, COMPSTATEINFO& componentStateInfo, DWORD dwItemState);
bool    ReadAllComponents (HKEY hKeyDesktop, tComponentInfoPtr& pComponentInfo, DWORD& dwComponentCount);
void    WriteAllComponents (HKEY hKeyDesktop, tComponentInfoPtr pComponentInfo, DWORD dwComponentCount);

BOOL    CALLBACK    MonitorCountEnumProc (HMONITOR hMonitor, HDC dc, RECT *rc, LPARAM data)

//  Count the number of monitors attached to the system.

{
    int     *iCounter;

    iCounter = reinterpret_cast<int*>(data);
    ++(*iCounter);
    return(TRUE);
}

BOOL    CALLBACK    MonitorCalculateEnumProc (HMONITOR hMonitor, HDC dc, RECT *rc, LPARAM data)

//  Store each monitor HMONITOR and dimensions in the array.

{
    tMonitorInfoArrayPtr    monitorArray;
    MONITORINFO             monitorInfo;

    monitorArray = reinterpret_cast<tMonitorInfoArrayPtr>(data);
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (GetMonitorInfo(hMonitor, &monitorInfo) != 0)
    {
        tMonitorInfoPtr     pMI;

        pMI = &monitorArray->miaMonitors[monitorArray->miaIndex++];
        pMI->miHMONITOR = hMonitor;
        TBOOL(CopyRect(&pMI->miDisplayAreaRect, &monitorInfo.rcMonitor));
        TBOOL(CopyRect(&pMI->miWorkAreaRect, &monitorInfo.rcWork));
    }
    return(TRUE);
}

HRESULT CalculateCurrentMonitorArray(void)

//  Allocate and fill the monitor rectangle array.

{
    HRESULT hr = E_OUTOFMEMORY;
    int     iCount;

    iCount = 0;
    TBOOL(EnumDisplayMonitors(NULL, NULL, MonitorCountEnumProc, reinterpret_cast<LPARAM>(&iCount)));
    gNewMonitorArray = reinterpret_cast<tMonitorInfoArrayPtr>(LocalAlloc(LMEM_FIXED, sizeof(tMonitorInfoArrayRec) + ((iCount - 1) * sizeof(tMonitorInfoRec))));

    if (gNewMonitorArray)
    {
        gNewMonitorArray->miaCount = iCount;
        gNewMonitorArray->miaIndex = 0;
        CalculateVirtualScreen(gNewMonitorArray->miaVirtualScreen);
        TBOOL(EnumDisplayMonitors(NULL, NULL, MonitorCalculateEnumProc, reinterpret_cast<LPARAM>(gNewMonitorArray)));

        hr = S_OK;
    }

    return hr;
}

void    ApplyCurrentMonitorArray (void)

//  Discard the old and save the new as current monitor
//  rectangle array for the next time the function is called.

{
    if (gOldMonitorArray != NULL)
        (HLOCAL)LocalFree(gOldMonitorArray);
    gOldMonitorArray = gNewMonitorArray;
    gNewMonitorArray = NULL;
}

bool    EqualMonitorArray (tMonitorInfoArrayPtr oldMonitorArray, tMonitorInfoArrayPtr newMonitorArray)

{
    bool    bResult;

    if (oldMonitorArray->miaCount == newMonitorArray->miaCount)
    {
        int     i, iLimit;

        bResult = true;
        for (i = 0, iLimit = oldMonitorArray->miaCount; bResult && (i < iLimit); ++i)
            bResult &= (EqualRect(&oldMonitorArray->miaMonitors[i].miWorkAreaRect, &newMonitorArray->miaMonitors[i].miWorkAreaRect) != 0);
    }
    else
        bResult = false;
    return(bResult);
}

//  These functions determine the index into the monitor
//  rectangle array of a given HMONITOR, POINT or RECT.

int     IndexOfMonitor (tMonitorInfoArrayPtr pMIA, HMONITOR hMonitor)

{
    int                 i, iLimit, iResult;
    tMonitorInfoPtr     pMI;

    iResult = -1;
    for (i = 0, iLimit = pMIA->miaCount, pMI = pMIA->miaMonitors; i < iLimit; ++i, ++pMI)
    {
        if (pMI->miHMONITOR == hMonitor)
        {
            iResult = i;
            break;
        }
    }
    return(iResult);
}

//  Note that the functions that take a POINT or RECT
//  require the co-ordinates to be in TRIDENT co-ordinates.

int     IndexOfMonitor (tMonitorInfoArrayPtr pMIA, POINT& pt)

{
    int                 i, iLimit, iResult;
    tMonitorInfoPtr     pMI;
    POINT               ptLocal;

    ptLocal = pt;
    OffsetPt(ptLocal, +pMIA->miaVirtualScreen.left, +pMIA->miaVirtualScreen.top);
    iResult = -1;
    for (i = 0, iLimit = pMIA->miaCount, pMI = pMIA->miaMonitors; i < iLimit; ++i, ++pMI)
    {
        if (PtInRect(&pMI->miDisplayAreaRect, ptLocal) != 0)
        {
            iResult = i;
            break;
        }
    }
    return(iResult);
}

int     IndexOfMonitor (tMonitorInfoArrayPtr pMIA, RECT& rc)

{
    int     iResult;
    POINT   pt;

    // 99/05/12 #338446 vtan: Try all four corners of the rectangle
    // to find a match.

    pt.x = rc.left;
    pt.y = rc.top;
    iResult = IndexOfMonitor(pMIA, pt);
    if (iResult < 0)
    {
        pt.x = rc.left;
        pt.y = rc.bottom;
        iResult = IndexOfMonitor(pMIA, pt);
        if (iResult < 0)
        {
            pt.x = rc.right;
            pt.y = rc.top;
            iResult = IndexOfMonitor(pMIA, pt);
            if (iResult < 0)
            {
                pt.x = rc.right;
                pt.y = rc.bottom;
                iResult = IndexOfMonitor(pMIA, pt);
            }
        }
    }
    return(iResult);
}

int     IndexOfMonitor (tMonitorInfoArrayPtr pMIA, COMPPOS& componentPosition)

{
    RECT    rcComponent;

    TBOOL(SetRect(&rcComponent, componentPosition.iLeft, componentPosition.iTop, componentPosition.iLeft + componentPosition.dwWidth, componentPosition.iTop + componentPosition.dwHeight));
    return(IndexOfMonitor(pMIA, rcComponent));
}

bool    RepositionDesktopRect (RECT& rcComponent, tMonitorInfoArrayPtr oldMonitorArray, tMonitorInfoArrayPtr newMonitorArray)

//  Reposition the component's RECT based on the logic at the
//  top of the source file. Used for both the component's
//  current position and the restored position.

{
    bool    bRepositionedComponent;
    int     iOldMonitorIndex, iNewMonitorIndex;

    // This is for future expansion. The components are always
    // deemed to be repositioned if this function is called.

    bRepositionedComponent = true;

    // Find out if the monitor which the component was on is still
    // present. To do this find the index of the component in the
    // old monitor array, get the HMONITOR and find that in the new
    // monitor array.

    iOldMonitorIndex = IndexOfMonitor(oldMonitorArray, rcComponent);
    if (iOldMonitorIndex >= 0)
    {
        RECT    *prcOldMonitor, *prcNewMonitor;

        iNewMonitorIndex = IndexOfMonitor(newMonitorArray, oldMonitorArray->miaMonitors[iOldMonitorIndex].miHMONITOR);
        if (iNewMonitorIndex < 0)
        {
            HMONITOR    hMonitor;

            // The component is on a monitor that no longer exists. The only
            // thing to do in this case is to find the nearest monitor based
            // on the GDI co-ordinates and position it on that monitor.

            hMonitor = MonitorFromRect(&rcComponent, MONITOR_DEFAULTTONEAREST);
            iNewMonitorIndex = IndexOfMonitor(newMonitorArray, hMonitor);
            ASSERT(iNewMonitorIndex >= 0);
        }

        // If iNewMonitorIndex was already positive then the monitor which
        // the component was on still exists and simply mapping GDI
        // co-ordinates will work. Otherwise we found the nearest monitor
        // and mapping GDI co-ordinates also works!

        // This maps from the component's OLD co-ordinates in trident
        // co-ordinates to GDI co-ordinates. Then it maps from the GDI
        // co-ordinates to an OLD monitor relative co-ordinate. Then it
        // maps from the OLD monitor relative co-ordinates to the NEW
        // monitor GDI co-ordinates.

        prcOldMonitor = &oldMonitorArray->miaMonitors[iOldMonitorIndex].miDisplayAreaRect;
        prcNewMonitor = &newMonitorArray->miaMonitors[iNewMonitorIndex].miDisplayAreaRect;
        TBOOL(OffsetRect(&rcComponent, +oldMonitorArray->miaVirtualScreen.left, +oldMonitorArray->miaVirtualScreen.top));
        TBOOL(OffsetRect(&rcComponent, -prcOldMonitor->left, -prcOldMonitor->top));
        TBOOL(OffsetRect(&rcComponent, +prcNewMonitor->left, +prcNewMonitor->top));
    }
    else
    {

        // Component exists at an invalid location in the old monitor
        // layout. It may be valid in the new layout. Try this. If that
        // doesn't work then it doesn't exist in the old nor the new
        // layout. It was in no-man's land. Position it using the default
        // positioning system.

        iNewMonitorIndex = IndexOfMonitor(newMonitorArray, rcComponent);
        if (iNewMonitorIndex < 0)
        {
            POINT       ptOrigin;
            COMPPOS     componentPosition;

            GetNextComponentPosition(&componentPosition);
            IncrementComponentsPositioned();
            TBOOL(SetRect(&rcComponent, componentPosition.iLeft, componentPosition.iTop, componentPosition.iLeft + componentPosition.dwWidth, componentPosition.iTop + componentPosition.dwHeight));

            // Get the primary monitor index in our monitor rectangle array.

            SetPt(ptOrigin, 0, 0);
            iNewMonitorIndex = IndexOfMonitor(newMonitorArray, MonitorFromPoint(ptOrigin, MONITOR_DEFAULTTOPRIMARY));
            ASSERT(iNewMonitorIndex >= 0);
        }
    }

    // At this stage the component position is in GDI co-ordinates.
    // Convert from GDI co-ordinates back to trident co-ordinates.

    TBOOL(OffsetRect(&rcComponent, -newMonitorArray->miaVirtualScreen.left, -newMonitorArray->miaVirtualScreen.top));

    return(bRepositionedComponent);
}

bool    RepositionDesktopComponent (COMPPOS& componentPosition, COMPSTATEINFO& componentStateInfo, DWORD dwItemState, int iComponentType)

{
    bool                    bRepositionedComponent;
    tMonitorInfoArrayPtr    oldMonitorArray, newMonitorArray;
    RECT                    rcComponent;

    // Check if the monitor layout has changed. If unchanged then
    // there is no need to move the components.

    oldMonitorArray = gOldMonitorArray;
    newMonitorArray = gNewMonitorArray;
    if (oldMonitorArray == NULL)
    {
        oldMonitorArray = newMonitorArray;
    }

    TBOOL(SetRect(&rcComponent, componentPosition.iLeft, componentPosition.iTop, componentPosition.iLeft + componentPosition.dwWidth, componentPosition.iTop + componentPosition.dwHeight));
    bRepositionedComponent = RepositionDesktopRect(rcComponent, oldMonitorArray, newMonitorArray);
    componentPosition.iLeft = rcComponent.left;
    componentPosition.iTop = rcComponent.top;
    componentPosition.dwWidth = rcComponent.right - rcComponent.left;
    componentPosition.dwHeight = rcComponent.bottom - rcComponent.top;
    ValidateComponentPosition(&componentPosition, dwItemState, iComponentType, NULL, NULL);

    // If the component is zoomed also reposition the restored
    // COMPSTATEINFO.

    if (IsZoomedState(dwItemState))
    {
        COMPPOS     restoredCompPos;

        TBOOL(SetRect(&rcComponent, componentStateInfo.iLeft, componentStateInfo.iTop, componentStateInfo.iLeft + componentStateInfo.dwWidth, componentStateInfo.iTop + componentStateInfo.dwHeight));
        bRepositionedComponent = RepositionDesktopRect(rcComponent, oldMonitorArray, newMonitorArray) || bRepositionedComponent;
        restoredCompPos.iLeft       = componentStateInfo.iLeft      = rcComponent.left;
        restoredCompPos.iTop        = componentStateInfo.iTop       = rcComponent.top;
        restoredCompPos.dwWidth     = componentStateInfo.dwWidth    = rcComponent.right - rcComponent.left;
        restoredCompPos.dwHeight    = componentStateInfo.dwHeight   = rcComponent.bottom - rcComponent.top;
        ZoomComponent(&componentPosition, dwItemState, FALSE);
        restoredCompPos.dwSize = sizeof(restoredCompPos);
        ValidateComponentPosition(&restoredCompPos, IS_NORMAL, iComponentType, NULL, NULL);
    }

    return(bRepositionedComponent);
}

bool    ReadAllComponents (HKEY hKeyDesktop, tComponentInfoPtr& pComponentInfo, DWORD& dwComponentCount)

{
    tComponentInfoPtr   pCI;

    if (RegQueryInfoKey(hKeyDesktop, NULL, NULL, NULL, &dwComponentCount, NULL, NULL, NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
        dwComponentCount = 0;
    if (dwComponentCount > 0)
    {

        // 99/08/09 #383184: STRESS fix. Allocate the whole block of
        // memory required for the components but allocate one extra
        // entry. advapi32!RegEnumKeyEx will try to access the memory
        // before determining that there is a failure condition. With
        // pageheap on (and the block allocated at the end of the page)
        // this causes an access violation. The simplest fix is to add
        // an extra entry.

        pComponentInfo = pCI = reinterpret_cast<tComponentInfoPtr>(LocalAlloc(LPTR, (dwComponentCount + 1) * sizeof(*pCI)));      // LMEM_FIXED | LMEM_ZEROINIT
        if (pCI != NULL)
        {
            DWORD   dwIndex, dwSize;

            // Enumerate all the desktop components.

            dwIndex = 0;
            dwSize = sizeof(pCI->ciName);
            while (RegEnumKeyEx(hKeyDesktop, dwIndex, pCI->ciName, &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
            {
                CRegKey     regKeyComponent;

                if (regKeyComponent.Open(hKeyDesktop, pCI->ciName, KEY_READ | KEY_WRITE) == ERROR_SUCCESS)
                {
                    DWORD   dwType, cbData;

                    // Read the Position value.

                    cbData = sizeof(pCI->ciPosition);
                    if (SHQueryValueEx(regKeyComponent, REG_VAL_COMP_POSITION, NULL, &dwType, &pCI->ciPosition, &cbData) == ERROR_SUCCESS)
                    {
                        DWORD   dwFlags;

                        pCI->ciValidData = true;
                        if (SHQueryValueEx(regKeyComponent, REG_VAL_COMP_FLAGS, NULL, &dwType, &dwFlags, &cbData) == ERROR_SUCCESS)
                        {
                            pCI->ciVisible = ((dwFlags & COMP_SELECTED) != 0);
                            pCI->ciType = (dwFlags & COMP_TYPE_MASK);
                        }
                        else
                        {
                            pCI->ciVisible = false;
                            pCI->ciType = COMP_TYPE_WEBSITE;
                        }
                        pCI->ciItemState = IS_NORMAL;        // if missing (IE4 machine) or error the assume normal
                        cbData = sizeof(pCI->ciItemState);
                        if ((SHQueryValueEx(regKeyComponent, REG_VAL_COMP_CURSTATE, NULL, &dwType, &pCI->ciItemState, &cbData) == ERROR_SUCCESS))
                        {

                            // If the component is zoomed also read in the COMPSTATEINFO.

                            if (IsZoomedState(pCI->ciItemState))
                            {
                                cbData = sizeof(pCI->ciStateInfo);
                                TW32(SHQueryValueEx(regKeyComponent, REG_VAL_COMP_RESTOREDSTATEINFO, NULL, &dwType, &pCI->ciStateInfo, &cbData));
                            }
                        }
                    }
                }
                ++pCI;
                ++dwIndex;
                dwSize = sizeof(pCI->ciName);
            }
        }
    }
    return((dwComponentCount != 0) && (pComponentInfo != NULL));
}

int     IndexOfComponent (tComponentInfoPtr pComponentInfo, DWORD dwComponentCount, LPCTSTR pcszName)

{
    int     iResult, i;

    for (iResult = -1, i = 0; (iResult < 0) && (i < static_cast<int>(dwComponentCount)); ++i)
    {
        if (lstrcmp(pComponentInfo[i].ciName, pcszName) == 0)
            iResult = i;
    }
    return(iResult);
}

void    WriteAllComponents (HKEY hKeyDesktop, tComponentInfoPtr pComponentInfo, DWORD dwComponentCount)

{
    TCHAR   szSubKeyName[kNameSize];
    DWORD   dwSubKeyIndex, dwSubKeySize;

    // Enumerate all the desktop components.

    dwSubKeyIndex = 0;
    dwSubKeySize = sizeof(szSubKeyName);
    while (RegEnumKeyEx(hKeyDesktop, dwSubKeyIndex, szSubKeyName, &dwSubKeySize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        CRegKey     regKeyComponent;

        if (regKeyComponent.Open(hKeyDesktop, szSubKeyName, KEY_READ | KEY_WRITE) == ERROR_SUCCESS)
        {
            int     i;

            i = IndexOfComponent(pComponentInfo, dwComponentCount, szSubKeyName);
            if ((i >= 0) && pComponentInfo[i].ciRepositioned)
            {
                TW32(RegSetValueEx(regKeyComponent, REG_VAL_COMP_POSITION, 0, REG_BINARY, reinterpret_cast<const unsigned char*>(&pComponentInfo[i].ciPosition), sizeof(pComponentInfo[i].ciPosition)));
                TW32(RegSetValueEx(regKeyComponent, REG_VAL_COMP_CURSTATE, NULL, REG_BINARY, reinterpret_cast<const unsigned char*>(&pComponentInfo[i].ciItemState), sizeof(pComponentInfo[i].ciItemState)));

                // If the component is zoomed also write out the COMPSTATEINFO.

                if (IsZoomedState(pComponentInfo[i].ciItemState))
                {
                    TW32(RegSetValueEx(regKeyComponent, REG_VAL_COMP_RESTOREDSTATEINFO, 0, REG_BINARY, reinterpret_cast<const unsigned char*>(&pComponentInfo[i].ciStateInfo), sizeof(pComponentInfo[i].ciStateInfo)));
                }
            }
        }
        ++dwSubKeyIndex;
        dwSubKeySize = sizeof(szSubKeyName);
    }
}

BOOL    AdjustDesktopComponents (LPCRECT arectNew,
                                 int crectNew,
                                 LPCRECT arectOldMonitors,
                                 LPCRECT arectOld,
                                 int crectOld)

{
    static  const   int kMaximumMonitorCount = 16;
    HRESULT                 hr;
    bool                    bRepositionedComponent;
    int                     zoomedComponentIndices[kMaximumMonitorCount];        // 16 monitors limitation here - make dynamic if required
    int                     i;
    tMonitorInfoArrayPtr    oldMonitorArray;
    CRegKey                 regKeyDesktop;
    TCHAR                   lpszDeskcomp[MAX_PATH];

    for (i = 0; i < kMaximumMonitorCount; ++i)
        zoomedComponentIndices[i] = -1;

    bRepositionedComponent = false;
    hr = CalculateCurrentMonitorArray();
    if (SUCCEEDED(hr))
    {
        oldMonitorArray = gOldMonitorArray;
        if (oldMonitorArray == NULL)
            oldMonitorArray = gNewMonitorArray;
        GetRegLocation(lpszDeskcomp, SIZECHARS(lpszDeskcomp), REG_DESKCOMP_COMPONENTS, NULL);
        if (regKeyDesktop.Open(HKEY_CURRENT_USER, lpszDeskcomp, KEY_READ) == ERROR_SUCCESS)
        {
            DWORD               dwComponentCount;
            tComponentInfoPtr   pComponentInfo;

            // Enumerate all the desktop components.

            if (ReadAllComponents(regKeyDesktop, pComponentInfo, dwComponentCount))
            {
                tComponentInfoPtr   pCI;

                for (pCI = pComponentInfo, i = 0; i < static_cast<int>(dwComponentCount); ++pCI, ++i)
                {
                    int     iPreviousMonitorIndexOfComponent;

                    // Calculate the previous monitor position BEFORE the component
                    // gets repositioned.

                    iPreviousMonitorIndexOfComponent = IndexOfMonitor(oldMonitorArray, pCI->ciPosition);
                    if (RepositionDesktopComponent(pCI->ciPosition, pCI->ciStateInfo, pCI->ciItemState, pCI->ciType))
                    {
                        int     iCurrentMonitorIndexOfComponent;

                        pCI->ciRepositioned = bRepositionedComponent = true;
                        iCurrentMonitorIndexOfComponent = IndexOfMonitor(gNewMonitorArray, pCI->ciPosition);
                        if (iCurrentMonitorIndexOfComponent >= 0)
                        {

                            // 99/05/12 #338446 vtan: Only use a zero or positive index into the
                            // monitor array. -1 is invalid and will cause an AV. This should NEVER
                            // happen but rather than assert this condition is handled.

                            if (IsZoomedState(pCI->ciItemState) && (zoomedComponentIndices[iCurrentMonitorIndexOfComponent] >= 0))
                            {
                                tComponentInfoPtr   pCIToRestore;

                                // This component is zoomed on a monitor that already has a zoomed
                                // component. Compare this component and the component already on the
                                // monitor. The one that was there before is the one that stays. The one
                                // that shouldn't be there is the one that gets restored.

                                if ((iPreviousMonitorIndexOfComponent == iCurrentMonitorIndexOfComponent) && pCI->ciVisible)
                                    pCIToRestore = pComponentInfo + zoomedComponentIndices[iCurrentMonitorIndexOfComponent];
                                else
                                    pCIToRestore = pCI;
                                pCIToRestore->ciPosition.iLeft     = pCIToRestore->ciStateInfo.iLeft;
                                pCIToRestore->ciPosition.iTop      = pCIToRestore->ciStateInfo.iTop;
                                pCIToRestore->ciPosition.dwWidth   = pCIToRestore->ciStateInfo.dwWidth;
                                pCIToRestore->ciPosition.dwHeight  = pCIToRestore->ciStateInfo.dwHeight;
                                pCIToRestore->ciPosition.izIndex   = COMPONENT_TOP;
                                pCIToRestore->ciItemState = IS_NORMAL;
                            }

                            // If the component is zoomed also write out the COMPSTATEINFO.

                            if (IsZoomedState(pCI->ciItemState))
                            {
                                zoomedComponentIndices[iCurrentMonitorIndexOfComponent] = i;
                            }
                        }
                    }
                }
                WriteAllComponents(regKeyDesktop, pComponentInfo, dwComponentCount);
                LocalFree(pComponentInfo);
            }

            if (bRepositionedComponent)
            {
                SHELLSTATE  ss;

                SetDesktopFlags(COMPONENTS_DIRTY, COMPONENTS_DIRTY);
                SHGetSetSettings(&ss, SSF_DESKTOPHTML, FALSE);

                // Refresh only if AD is turned on.

                if(ss.fDesktopHTML)
                {

                    // 98/09/22 #182982 vtan: Use dynamic HTML by default to refresh.
                    // Only disallow usage when specifically told to by a flag.

                    PokeWebViewDesktop(AD_APPLY_FORCE | AD_APPLY_HTMLGEN | AD_APPLY_REFRESH);
                }
            }
        }
        ApplyCurrentMonitorArray();
    }
    
    return(bRepositionedComponent);
}

#endif

