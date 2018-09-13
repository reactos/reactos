#include "proj.h"
#include <multimon.h>
#include <mmhelper.h>

/* This file contains the Multi-Monitor helper functions used in side the 
   shell. -- dli */
 
HMONITOR GetPrimaryMonitor()
{
    POINT pt = {0,0};
    return MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY); 
}

// Gets the Monitor's bounding or work rectangle, if the hMon is bad, return
// the primary monitor's bounding rectangle. 
BOOL GetMonitorRects(HMONITOR hMon, LPRECT prc, BOOL bWork)
{
    MONITORINFO mi; 
    mi.cbSize = sizeof(mi);
    if (hMon && GetMonitorInfo(hMon, &mi))
    {
        if (!prc)
            return TRUE;
        
        else if (bWork)
            CopyRect(prc, &mi.rcWork);
        else 
            CopyRect(prc, &mi.rcMonitor);
        
        return TRUE;
    }
    
    if (prc)
        SetRect(prc, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    return FALSE;
}
