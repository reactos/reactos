#ifndef _MMHELPER_H_
#define _MMHELPER_H_

/* This file contains the declarations of MultiMonitor helper functions used
   inside the shell, they are implemented in shell\lib\mmhelper.cpp */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

HMONITOR GetPrimaryMonitor();
BOOL GetMonitorRects(HMONITOR hMon, LPRECT prc, BOOL bWork);
#define GetMonitorRect(hMon, prc) \
        GetMonitorRects((hMon), (prc), FALSE)
#define GetMonitorWorkArea(hMon, prc) \
        GetMonitorRects((hMon), (prc), TRUE)
#define IsMonitorValid(hMon) \
        GetMonitorRects((hMon), NULL, TRUE)
#define GetNumberOfMonitors() \
        GetSystemMetrics(SM_CMONITORS)
#ifdef __cplusplus
}
#endif	/* __cplusplus */

#endif /* _MMHELPER_H_ */