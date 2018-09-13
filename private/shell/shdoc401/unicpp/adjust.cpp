#include "stdafx.h"
#pragma hdrstop

//#include "deskstat.h"
//#include "dutil.h"
//#include "deskmovr.h"

BOOL ModifyPosAndSize(COMPPOS *pcp, LPCRECT arectNew, int crectNew, LPCRECT arectOldMonitors, LPCRECT arectOld, int crectOld, EnumMonitorsArea* pEMA)
{
    BOOL fModified = FALSE;

    int iXBorder = GET_CXSIZE;
    int iXBorders = 2 * iXBorder;
    int iCaptionSize = GET_CYCAPTION;
    int iYBorders = GET_CYSIZE + iCaptionSize;

    if(crectOld == 0)
    {
        POINT ptTopLeft;
        ptTopLeft.x = pcp->iLeft;
        ptTopLeft.y = pcp->iTop;
        MoveOnScreen(pcp, iXBorders, iYBorders, iXBorder, iCaptionSize, pEMA);
        if(pcp->iLeft != ptTopLeft.x || pcp->iTop != ptTopLeft.y)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    // Find the old virtual WorkArea
    POINT ptOldVirtualMonitorTopLeft;
    ptOldVirtualMonitorTopLeft.x = arectOldMonitors[0].left;
    ptOldVirtualMonitorTopLeft.y = arectOldMonitors[0].top;

    for(int i = 1; i < crectOld; i++)
    {
        if(arectOldMonitors[i].left < ptOldVirtualMonitorTopLeft.x)
        {
            ptOldVirtualMonitorTopLeft.x = arectOldMonitors[i].left;
        }
        if(arectOldMonitors[i].top < ptOldVirtualMonitorTopLeft.y)
        {
            ptOldVirtualMonitorTopLeft.y = arectOldMonitors[i].top;
        }
    }

    //
    // Find the old work area.
    //
    int iIndex = GetWorkAreaIndex(pcp, arectOldMonitors, crectOld, &ptOldVirtualMonitorTopLeft);

    LPCRECT prectOld, prectOldMonitor;
    // If the old work area is non-existent, reset it to the primary work area.
    if ((iIndex < 0) || (iIndex >= crectOld))
    {
        iIndex = 0;
    }
    prectOld = &arectOld[iIndex];
    prectOldMonitor = &arectOldMonitors[iIndex];

    LPCRECT prectNew, prectNewMonitor;
    // If the old work area is beyond the new work areas, reset it to the primary work area.
    if (iIndex >= crectNew)
    {
        iIndex = 0;
    }

    prectNew = &arectNew[iIndex];
    prectNewMonitor = &pEMA->rcMonitor[iIndex];

    if (crectNew != crectOld || !EqualRect(prectNew, prectOld)
            || (pEMA->rcVirtualMonitor.left != ptOldVirtualMonitorTopLeft.x
                || pEMA->rcVirtualMonitor.top != ptOldVirtualMonitorTopLeft.y))
    {
        //
        // Calculate old dimensions.
        //
        int iOldWidth, iOldHeight;
        iOldWidth = (prectOld->right - prectOld->left);
        iOldHeight = (prectOld->bottom - prectOld->top);

        //
        // Calculate new dimensions.
        //
        int iNewWidth, iNewHeight;
        iNewWidth = (prectNew->right - prectNew->left);
        iNewHeight = (prectNew->bottom - prectNew->top);

        ASSERT(iOldWidth > 0 && iOldHeight > 0 && iNewWidth > 0 && iNewHeight > 0);
        if(iOldWidth <= 0 || iOldHeight <= 0 || iNewWidth <= 0 || iNewHeight <= 0)
        {
            return FALSE;   // Make no changes
        }

        fModified = TRUE;

        // Compute values to adjust for the fact that the whole desktop browser window is
        // shifted when a toolbar/tray is snapped to the left or right of the monitor.
        int iLeftOld, iTopOld, iLeftNew, iTopNew, iMonitorRightNew, iMonitorBottomNew;
        if (crectOld == 1)
        {
            iLeftOld = 0;
            iTopOld = 0;
        }
        else
        {
            iLeftOld = prectOld->left - ptOldVirtualMonitorTopLeft.x;
            iTopOld = prectOld->top - ptOldVirtualMonitorTopLeft.y;
        }
        if (crectNew == 1)
        {
            iLeftNew = 0;
            iTopNew = 0;
            iMonitorRightNew = prectNewMonitor->right - prectNew->left;
            iMonitorBottomNew = prectNewMonitor->bottom - prectNew->top;
        }
        else
        {
            iLeftNew = prectNew->left - pEMA->rcVirtualMonitor.left;
            iTopNew = prectNew->top - pEMA->rcVirtualMonitor.top;
            iMonitorRightNew = prectNewMonitor->right - pEMA->rcVirtualMonitor.left;
            iMonitorBottomNew = prectNewMonitor->bottom - pEMA->rcVirtualMonitor.top;
        }

        //
        // Calc the new top left corner of the comp
        // proportionate to the changes.
        //
        // We change the comp position proportional to the change in the old and new monitor sizes
        // only if the resolution has changed and not each time the workarea changes.
        if (EqualRect(prectOldMonitor, prectNewMonitor))
        {
            // Only the workarea has changed
            pcp->iLeft = iLeftNew + (pcp->iLeft - iLeftOld);
            pcp->iTop = iTopNew + (pcp->iTop - iTopOld);
        }
        else
        {
            pcp->iLeft = iLeftNew + ((pcp->iLeft - iLeftOld)
                    * (prectNewMonitor->right - prectNewMonitor->left) / (prectOldMonitor->right - prectOldMonitor->left));
            pcp->iTop = iTopNew + ((pcp->iTop - iTopOld)
                    * (prectNewMonitor->bottom - prectNewMonitor->top) / (prectOldMonitor->bottom - prectOldMonitor->top));
        }

        //
        // Shrink the component to fit on screen if necessary.
        // This is done only if the viewarea has changed, however.
        //
        if(iNewWidth != iOldWidth || iNewHeight != iOldHeight)
        {   // The viewarea has changed
            // Find the right-most and bottom-most ViewAreas.
            int iRightMostViewArea = arectNew[0].right;
            int iBottomMostViewArea = arectNew[0].bottom;
            for(int i = 1; i < crectNew; i++)
            {
                if(arectNew[i].right > iRightMostViewArea)
                {
                    iRightMostViewArea = arectNew[i].right;
                }
                if(arectNew[i].bottom > iBottomMostViewArea)
                {
                    iBottomMostViewArea = arectNew[i].bottom;
                }
            }
            if (crectNew == 1)
            {
                iRightMostViewArea -= arectNew[0].left;
                iBottomMostViewArea -= arectNew[0].top;
            }
            else
            {
                iRightMostViewArea -= pEMA->rcVirtualMonitor.left;
                iBottomMostViewArea -= pEMA->rcVirtualMonitor.top;
            }
            if (pcp->iLeft + (int)pcp->dwWidth + iXBorder > iLeftNew + iNewWidth)
            {
                int iLeftOffset = iNewWidth - (pcp->dwWidth + iXBorders);
                if(iLeftOffset > 0)
                {   // The same width might still fit inside the screen
                    pcp->iLeft = iLeftNew + (iLeftOffset + iXBorder);
                }
                else
                {
                    pcp->iLeft = iLeftNew;  // Adding iXBorder to this causes problems when the iNewWidth value is very low
                    // Only if it's bigger than the virtual view area
                    if(pcp->iLeft + (int)pcp->dwWidth + iXBorder > iRightMostViewArea
                            || pcp->iLeft + (int)pcp->dwWidth + iXBorder <= iMonitorRightNew)
                    {
                        pcp->dwWidth = iNewWidth - iXBorders;
                        // limit shrinkage to keep the handle accessible
                        if ((int)pcp->dwWidth < (4 * iCaptionSize))
                        {
                            pcp->dwWidth = 4 * iCaptionSize;
                        }
                    }
                }
            }
            if (pcp->iTop + (int)pcp->dwHeight + (iYBorders - iCaptionSize) > iTopNew + iNewHeight)
            {
                int iTopOffset = iNewHeight - (pcp->dwHeight + iYBorders);
                if(iTopOffset > 0)
                {   // The same height might still fit inside the screen
                    pcp->iTop = iTopNew + iTopOffset + (iYBorders - iCaptionSize);
                }
                else
                {
                    pcp->iTop = iTopNew;    // Adding iCaptionSize to this causes problems when the iNewHeight value is very low
                    // Only if it's bigger than the virtual view area
                    if(pcp->iTop + (int)pcp->dwHeight + + (iYBorders - iCaptionSize) > iBottomMostViewArea
                            || pcp->iTop + (int)pcp->dwHeight + (iYBorders - iCaptionSize) <= iMonitorBottomNew)
                    {
                        pcp->dwHeight = iNewHeight - iYBorders;
                        // limit shrinkage to keep the handle accessible
                        if ((int)pcp->dwHeight < iCaptionSize)
                        {
                            pcp->dwHeight = iCaptionSize;
                        }
                    }
                }
            }
        }

        //
        // Make sure that the component is visible.
        //
        MoveOnScreen(pcp, iXBorders, iYBorders, iXBorder, iCaptionSize, pEMA);
    }

    return fModified;
}

//
// API to move desktop components to the current monitors.
//
BOOL AdjustDesktopComponents(LPCRECT arectNew, int crectNew,
                             LPCRECT arectOldMonitors, LPCRECT arectOld, int crectOld)
{
    BOOL fRet = FALSE;

    HKEY hkey;
    TCHAR lpszDeskcomp[MAX_PATH];

    // Find the new virtual dimensions.
    EnumMonitorsArea ema;
    GetMonitorSettings(&ema);

    GetRegLocation(lpszDeskcomp, REG_DESKCOMP_COMPONENTS, NULL);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, (LPCTSTR)lpszDeskcomp, 0, KEY_READ,
                     &hkey) == ERROR_SUCCESS)
    {
        TCHAR szSubKeyName[10];
        DWORD dwSubKeyIndex = 0;
        DWORD dwSubKeyLength = ARRAYSIZE(szSubKeyName);

        //
        // Enumerate all the desktop components.
        //
        while (RegEnumKeyEx(hkey, dwSubKeyIndex, szSubKeyName, &dwSubKeyLength,
                            NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
        {
            HKEY    hkeyComp;

            if (RegOpenKeyEx(hkey, szSubKeyName, 0, KEY_READ | KEY_WRITE,
                             &hkeyComp) == ERROR_SUCCESS)
            {
                DWORD dwType, cbData;
                COMPPOS cp;

                //
                // Read the Position value.
                //
                cbData = SIZEOF(cp);
                if (RegQueryValueEx(hkeyComp, REG_VAL_COMP_POSITION, NULL,
                                    &dwType, (LPBYTE)&cp, &cbData)
                                    == ERROR_SUCCESS)
                {
                    //
                    // Move and size component if necessary.
                    //
                    if (ModifyPosAndSize(&cp, arectNew, crectNew,
                                         arectOldMonitors, arectOld, crectOld, &ema))
                    {
                        //
                        // Component's position/size has been modified.
                        // So, we need to update the registry.
                        //
                        if (RegSetValueEx(hkeyComp, REG_VAL_COMP_POSITION, 0,
                                          REG_BINARY, (LPBYTE)&cp, SIZEOF(cp))
                                          == ERROR_SUCCESS)
                        {
                            fRet = TRUE;
                        }
                    }
                }

                RegCloseKey(hkeyComp);
            }

            dwSubKeyLength = ARRAYSIZE(szSubKeyName);
            dwSubKeyIndex++;
        }

        //
        // Set the dirty bit, if anything has been changed!
        //
        if (fRet)
        {
            SetDesktopFlags(COMPONENTS_DIRTY, COMPONENTS_DIRTY);
            REFRESHACTIVEDESKTOP();
        }

        RegCloseKey(hkey);
    }

    return fRet;
}

