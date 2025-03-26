/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtUserEnumDisplayMonitors
 * PROGRAMMERS:
 */

#include "../win32nt.h"

ULONG gMonitorCount = 0;
HDC ghdcMonitor = 0;
RECT grcMonitor = {0};

BOOL
NTAPI
NtUserEnumDisplayMonitors1(
    HDC hDC,
    LPCRECT lprcClip,
    MONITORENUMPROC lpfnEnum,
    LPARAM dwData)
{
    return (INT)Syscall(L"NtUserEnumDisplayMonitors", 4, &hDC);
}

BOOL CALLBACK
MonitorEnumProc(
    HMONITOR hMonitor,
    HDC hdcMonitor,
    LPRECT lprcMonitor,
    LPARAM dwData)
{
    gMonitorCount++;
    if (gMonitorCount == 1)
    {
        ghdcMonitor = hdcMonitor;
        grcMonitor = *lprcMonitor;
    }
    return TRUE;
}

START_TEST(NtUserEnumDisplayMonitors)
{
    BOOL ret;

    // WILL crash!
//  TEST(NtUserEnumDisplayMonitors1(NULL, NULL, NULL, 0) == 0);

    ret = NtUserEnumDisplayMonitors(0, NULL, MonitorEnumProc, 0);
    TEST(ret == TRUE);
    TEST(gMonitorCount > 0);
    TEST(ghdcMonitor == 0);
    TEST(grcMonitor.left == 0);
    TEST(grcMonitor.right > 0);
    TEST(grcMonitor.top == 0);
    TEST(grcMonitor.bottom > 0);

}
