/*
 * Unit tests for monitor APIs
 *
 * Copyright 2005 Huw Davies
 * Copyright 2008 Dmitry Timoshkov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "wine/test.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

static HMODULE hdll;
static LONG (WINAPI *pChangeDisplaySettingsExA)(LPCSTR, LPDEVMODEA, HWND, DWORD, LPVOID);
static LONG (WINAPI *pChangeDisplaySettingsExW)(LPCWSTR, LPDEVMODEW, HWND, DWORD, LPVOID);
static BOOL (WINAPI *pEnumDisplayDevicesA)(LPCSTR,DWORD,LPDISPLAY_DEVICEA,DWORD);
static BOOL (WINAPI *pEnumDisplayMonitors)(HDC,LPRECT,MONITORENUMPROC,LPARAM);
static BOOL (WINAPI *pGetMonitorInfoA)(HMONITOR,LPMONITORINFO);
static HMONITOR (WINAPI *pMonitorFromPoint)(POINT,DWORD);
static HMONITOR (WINAPI *pMonitorFromWindow)(HWND,DWORD);

static void init_function_pointers(void)
{
    hdll = GetModuleHandleA("user32.dll");

#define GET_PROC(func) \
    p ## func = (void*)GetProcAddress(hdll, #func); \
    if(!p ## func) \
      trace("GetProcAddress(%s) failed\n", #func);

    GET_PROC(ChangeDisplaySettingsExA)
    GET_PROC(ChangeDisplaySettingsExW)
    GET_PROC(EnumDisplayDevicesA)
    GET_PROC(EnumDisplayMonitors)
    GET_PROC(GetMonitorInfoA)
    GET_PROC(MonitorFromPoint)
    GET_PROC(MonitorFromWindow)

#undef GET_PROC
}

static BOOL CALLBACK monitor_enum_proc(HMONITOR hmon, HDC hdc, LPRECT lprc,
                                       LPARAM lparam)
{
    MONITORINFOEXA mi;
    char *primary = (char *)lparam;

    mi.cbSize = sizeof(mi);

    ok(pGetMonitorInfoA(hmon, (MONITORINFO*)&mi), "GetMonitorInfo failed\n");
    if (mi.dwFlags & MONITORINFOF_PRIMARY)
        strcpy(primary, mi.szDevice);

    return TRUE;
}

static void test_enumdisplaydevices(void)
{
    DISPLAY_DEVICEA dd;
    char primary_device_name[32];
    char primary_monitor_device_name[32];
    DWORD primary_num = -1, num = 0;
    BOOL ret;

    if (!pEnumDisplayDevicesA)
    {
        win_skip("EnumDisplayDevicesA is not available\n");
        return;
    }

    dd.cb = sizeof(dd);
    while(1)
    {
        BOOL ret;
        HDC dc;
        ret = pEnumDisplayDevicesA(NULL, num, &dd, 0);
        if(!ret) break;
        if(dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
        {
            strcpy(primary_device_name, dd.DeviceName);
            primary_num = num;
        }
        if(dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
        {
            /* test creating DC */
            dc = CreateDCA(dd.DeviceName, NULL, NULL, NULL);
            ok(dc != NULL, "Failed to CreateDC(\"%s\") err=%d\n", dd.DeviceName, GetLastError());
            DeleteDC(dc);
        }
        num++;
    }

    if (primary_num == -1 || !pEnumDisplayMonitors || !pGetMonitorInfoA)
    {
        win_skip("EnumDisplayMonitors or GetMonitorInfoA are not available\n");
        return;
    }

    primary_monitor_device_name[0] = 0;
    ret = pEnumDisplayMonitors(NULL, NULL, monitor_enum_proc, (LPARAM)primary_monitor_device_name);
    ok(ret, "EnumDisplayMonitors failed\n");
    ok(!strcmp(primary_monitor_device_name, primary_device_name),
       "monitor device name %s, device name %s\n", primary_monitor_device_name,
       primary_device_name);
}

struct vid_mode
{
    DWORD w, h, bpp, freq, fields;
    BOOL must_succeed;
};

static const struct vid_mode vid_modes_test[] = {
    {640, 480, 0, 0, DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY, 1},
    {640, 480, 0, 0, DM_PELSWIDTH | DM_PELSHEIGHT |                 DM_DISPLAYFREQUENCY, 1},
    {640, 480, 0, 0, DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL                      , 1},
    {640, 480, 0, 0, DM_PELSWIDTH | DM_PELSHEIGHT                                      , 1},
    {640, 480, 0, 0,                                DM_BITSPERPEL                      , 0},
    {640, 480, 0, 0,                                                DM_DISPLAYFREQUENCY, 0},

    {0, 0, 0, 0, DM_PELSWIDTH, 0},
    {0, 0, 0, 0, DM_PELSHEIGHT, 0},

    {640, 480, 0, 0, DM_PELSWIDTH, 0},
    {640, 480, 0, 0, DM_PELSHEIGHT, 0},
    {  0, 480, 0, 0, DM_PELSWIDTH | DM_PELSHEIGHT, 0},
    {640,   0, 0, 0, DM_PELSWIDTH | DM_PELSHEIGHT, 0},

    /* the following test succeeds under XP SP3
    {0, 0, 0, 0, DM_DISPLAYFREQUENCY, 0}
    */
};
#define vid_modes_cnt (sizeof(vid_modes_test) / sizeof(vid_modes_test[0]))

static void test_ChangeDisplaySettingsEx(void)
{
    DEVMODEA dm;
    DEVMODEW dmW;
    DWORD width;
    LONG res;
    int i;

    if (!pChangeDisplaySettingsExA)
    {
        win_skip("ChangeDisplaySettingsExA is not available\n");
        return;
    }

    SetLastError(0xdeadbeef);
    res = EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);
    ok(res, "EnumDisplaySettings error %u\n", GetLastError());

    width = dm.dmPelsWidth;

    dm.dmDriverExtra = 1;
    res = ChangeDisplaySettingsA(&dm, CDS_TEST);
    ok(res == DISP_CHANGE_SUCCESSFUL,
       "ChangeDisplaySettingsA returned %d, expected DISP_CHANGE_SUCCESSFUL\n", res);
    ok(dm.dmDriverExtra == 0 || broken(dm.dmDriverExtra == 1) /* win9x */,
       "ChangeDisplaySettingsA didn't reset dmDriverExtra to 0\n");

    /* crashes under XP SP3 for large dmDriverExtra values */
    dm.dmDriverExtra = 1;
    res = pChangeDisplaySettingsExA(NULL, &dm, NULL, CDS_TEST, NULL);
    ok(res == DISP_CHANGE_SUCCESSFUL,
       "ChangeDisplaySettingsExW returned %d, expected DISP_CHANGE_BADMODE\n", res);
    ok(dm.dmDriverExtra == 1, "ChangeDisplaySettingsExA shouldn't reset dmDriverExtra to 0\n");

    memset(&dmW, 0, sizeof(dmW));
    dmW.dmSize = sizeof(dmW);
    dmW.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
    dmW.dmPelsWidth = dm.dmPelsWidth;
    dmW.dmPelsHeight = dm.dmPelsHeight;
    dmW.dmDriverExtra = 1;
    SetLastError(0xdeadbeef);
    res = ChangeDisplaySettingsW(&dmW, CDS_TEST);
    if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    {
        ok(res == DISP_CHANGE_SUCCESSFUL,
           "ChangeDisplaySettingsW returned %d, expected DISP_CHANGE_SUCCESSFUL\n", res);
        ok(dmW.dmDriverExtra == 0, "ChangeDisplaySettingsW didn't reset dmDriverExtra to 0\n");
    }

    /* Apparently XP treats dmDriverExtra being != 0 as an error */
    dmW.dmDriverExtra = 1;
    res = pChangeDisplaySettingsExW(NULL, &dmW, NULL, CDS_TEST, NULL);
    if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    {
        ok(res == DISP_CHANGE_SUCCESSFUL,
           "ChangeDisplaySettingsExW returned %d, expected DISP_CHANGE_BADMODE\n", res);
        ok(dmW.dmDriverExtra == 1, "ChangeDisplaySettingsExW shouldn't reset dmDriverExtra to 0\n");
    }

    /* the following 2 tests show that dm.dmSize being 0 is invalid, but
     * ChangeDisplaySettingsExA still reports success.
     */
    memset(&dm, 0, sizeof(dm));
    dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
    dm.dmPelsWidth = width;
    res = pChangeDisplaySettingsExA(NULL, &dm, NULL, CDS_TEST, NULL);
    ok(res == DISP_CHANGE_SUCCESSFUL ||
       res == DISP_CHANGE_BADMODE || /* Win98, WinMe */
       res == DISP_CHANGE_FAILED, /* NT4 */
       "ChangeDisplaySettingsExA returned unexpected %d\n", res);

    memset(&dmW, 0, sizeof(dmW));
    dmW.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
    dmW.dmPelsWidth = width;
    SetLastError(0xdeadbeef);
    res = pChangeDisplaySettingsExW(NULL, &dmW, NULL, CDS_TEST, NULL);
    if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
        ok(res == DISP_CHANGE_FAILED ||
           res == DISP_CHANGE_BADPARAM ||  /* NT4 */
           res == DISP_CHANGE_BADMODE /* XP SP3 */,
           "ChangeDisplaySettingsExW returned %d\n", res);

    memset(&dm, 0, sizeof(dm));
    dm.dmSize = sizeof(dm);

    for (i = 0; i < vid_modes_cnt; i++)
    {
        dm.dmPelsWidth        = vid_modes_test[i].w;
        dm.dmPelsHeight       = vid_modes_test[i].h;
        dm.dmBitsPerPel       = vid_modes_test[i].bpp;
        dm.dmDisplayFrequency = vid_modes_test[i].freq;
        dm.dmFields           = vid_modes_test[i].fields;
        res = pChangeDisplaySettingsExA(NULL, &dm, NULL, CDS_TEST, NULL);
        ok(vid_modes_test[i].must_succeed ?
           (res == DISP_CHANGE_SUCCESSFUL || res == DISP_CHANGE_RESTART) :
           (res == DISP_CHANGE_SUCCESSFUL || res == DISP_CHANGE_RESTART ||
            res == DISP_CHANGE_BADMODE || res == DISP_CHANGE_BADPARAM),
           "Unexpected ChangeDisplaySettingsEx() return code for resolution[%d]: %d\n", i, res);

        if (res == DISP_CHANGE_SUCCESSFUL)
        {
            RECT r, r1, virt;

            SetRect(&virt, 0, 0, GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN));
            if (IsRectEmpty(&virt))  /* NT4 doesn't have SM_CX/YVIRTUALSCREEN */
                SetRect(&virt, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
            OffsetRect(&virt, GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN));

            /* Resolution change resets clip rect */
            ok(GetClipCursor(&r), "GetClipCursor() failed\n");
            ok(EqualRect(&r, &virt), "Invalid clip rect: (%d %d) x (%d %d)\n", r.left, r.top, r.right, r.bottom);

            if (!ClipCursor(NULL)) continue;
            ok(GetClipCursor(&r), "GetClipCursor() failed\n");
            ok(EqualRect(&r, &virt), "Invalid clip rect: (%d %d) x (%d %d)\n", r.left, r.top, r.right, r.bottom);

            /* This should always work. Primary monitor is at (0,0) */
            SetRect(&r1, 10, 10, 20, 20);
            ok(ClipCursor(&r1), "ClipCursor() failed\n");
            ok(GetClipCursor(&r), "GetClipCursor() failed\n");
            ok(EqualRect(&r, &r1), "Invalid clip rect: (%d %d) x (%d %d)\n", r.left, r.top, r.right, r.bottom);

            SetRect(&r1, virt.left - 10, virt.top - 10, virt.right + 20, virt.bottom + 20);
            ok(ClipCursor(&r1), "ClipCursor() failed\n");
            ok(GetClipCursor(&r), "GetClipCursor() failed\n");
            ok(EqualRect(&r, &virt) ||
               broken(EqualRect(&r, &r1)) /* win9x */,
               "Invalid clip rect: (%d %d) x (%d %d)\n", r.left, r.top, r.right, r.bottom);
            ClipCursor(&virt);
        }
    }
    res = pChangeDisplaySettingsExA(NULL, NULL, NULL, CDS_RESET, NULL);
    ok(res == DISP_CHANGE_SUCCESSFUL, "Failed to reset default resolution: %d\n", res);
}

static void test_monitors(void)
{
    HMONITOR monitor, primary;
    POINT pt;

    if (!pMonitorFromPoint || !pMonitorFromWindow)
    {
        win_skip("MonitorFromPoint or MonitorFromWindow are not available\n");
        return;
    }

    pt.x = pt.y = 0;
    primary = pMonitorFromPoint( pt, MONITOR_DEFAULTTOPRIMARY );
    ok( primary != 0, "couldn't get primary monitor\n" );

    monitor = pMonitorFromWindow( 0, MONITOR_DEFAULTTONULL );
    ok( !monitor, "got %p, should not get a monitor for an invalid window\n", monitor );
    monitor = pMonitorFromWindow( 0, MONITOR_DEFAULTTOPRIMARY );
    ok( monitor == primary, "got %p, should get primary %p for MONITOR_DEFAULTTOPRIMARY\n", monitor, primary );
    monitor = pMonitorFromWindow( 0, MONITOR_DEFAULTTONEAREST );
    ok( monitor == primary, "got %p, should get primary %p for MONITOR_DEFAULTTONEAREST\n", monitor, primary );
}

static BOOL CALLBACK find_primary_mon(HMONITOR hmon, HDC hdc, LPRECT rc, LPARAM lp)
{
    MONITORINFO mi;
    BOOL ret;

    mi.cbSize = sizeof(mi);
    ret = pGetMonitorInfoA(hmon, &mi);
    ok(ret, "GetMonitorInfo failed\n");
    if (mi.dwFlags & MONITORINFOF_PRIMARY)
    {
        *(HMONITOR *)lp = hmon;
        return FALSE;
    }
    return TRUE;
}

static void test_work_area(void)
{
    HMONITOR hmon;
    MONITORINFO mi;
    RECT rc_work, rc_normal;
    HWND hwnd;
    WINDOWPLACEMENT wp;
    BOOL ret;

    if (!pEnumDisplayMonitors || !pGetMonitorInfoA)
    {
        win_skip("EnumDisplayMonitors or GetMonitorInfoA are not available\n");
        return;
    }

    hmon = 0;
    ret = pEnumDisplayMonitors(NULL, NULL, find_primary_mon, (LPARAM)&hmon);
    ok(!ret && hmon != 0, "Failed to find primary monitor\n");

    mi.cbSize = sizeof(mi);
    SetLastError(0xdeadbeef);
    ret = pGetMonitorInfoA(hmon, &mi);
    ok(ret, "GetMonitorInfo error %u\n", GetLastError());
    ok(mi.dwFlags & MONITORINFOF_PRIMARY, "not a primary monitor\n");
    trace("primary monitor (%d,%d-%d,%d)\n",
        mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom);

    SetLastError(0xdeadbeef);
    ret = SystemParametersInfo(SPI_GETWORKAREA, 0, &rc_work, 0);
    ok(ret, "SystemParametersInfo error %u\n", GetLastError());
    trace("work area (%d,%d-%d,%d)\n", rc_work.left, rc_work.top, rc_work.right, rc_work.bottom);
    ok(EqualRect(&rc_work, &mi.rcWork), "work area is different\n");

    hwnd = CreateWindowEx(0, "static", NULL, WS_OVERLAPPEDWINDOW|WS_VISIBLE,100,100,10,10,0,0,0,NULL);
    ok(hwnd != 0, "CreateWindowEx failed\n");

    ret = GetWindowRect(hwnd, &rc_normal);
    ok(ret, "GetWindowRect failed\n");
    trace("normal (%d,%d-%d,%d)\n", rc_normal.left, rc_normal.top, rc_normal.right, rc_normal.bottom);

    wp.length = sizeof(wp);
    ret = GetWindowPlacement(hwnd, &wp);
    ok(ret, "GetWindowPlacement failed\n");
    trace("min: %d,%d max %d,%d normal %d,%d-%d,%d\n",
          wp.ptMinPosition.x, wp.ptMinPosition.y,
          wp.ptMaxPosition.x, wp.ptMaxPosition.y,
          wp.rcNormalPosition.left, wp.rcNormalPosition.top,
          wp.rcNormalPosition.right, wp.rcNormalPosition.bottom);
    OffsetRect(&wp.rcNormalPosition, rc_work.left, rc_work.top);
    if (mi.rcMonitor.left != mi.rcWork.left ||
        mi.rcMonitor.top != mi.rcWork.top)  /* FIXME: remove once Wine is fixed */
        todo_wine ok(EqualRect(&rc_normal, &wp.rcNormalPosition), "normal pos is different\n");
    else
        ok(EqualRect(&rc_normal, &wp.rcNormalPosition), "normal pos is different\n");

    SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_TOOLWINDOW);

    wp.length = sizeof(wp);
    ret = GetWindowPlacement(hwnd, &wp);
    ok(ret, "GetWindowPlacement failed\n");
    trace("min: %d,%d max %d,%d normal %d,%d-%d,%d\n",
          wp.ptMinPosition.x, wp.ptMinPosition.y,
          wp.ptMaxPosition.x, wp.ptMaxPosition.y,
          wp.rcNormalPosition.left, wp.rcNormalPosition.top,
          wp.rcNormalPosition.right, wp.rcNormalPosition.bottom);
    ok(EqualRect(&rc_normal, &wp.rcNormalPosition), "normal pos is different\n");

    DestroyWindow(hwnd);
}

START_TEST(monitor)
{
    init_function_pointers();
    test_enumdisplaydevices();
    test_ChangeDisplaySettingsEx();
    test_monitors();
    test_work_area();
}
