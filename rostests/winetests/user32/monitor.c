/*
 * Unit tests for monitor APIs
 *
 * Copyright 2005 Huw Davies
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
    if(mi.dwFlags == MONITORINFOF_PRIMARY)
        strcpy(primary, mi.szDevice);

    return TRUE;
}

static void test_enumdisplaydevices(void)
{
    DISPLAY_DEVICEA dd;
    char primary_device_name[32];
    char primary_monitor_device_name[32];
    DWORD primary_num = -1, num = 0;

    dd.cb = sizeof(dd);
    if(pEnumDisplayDevicesA == NULL) return;
    while(1)
    {
        BOOL ret;
        HDC dc;
        ret = pEnumDisplayDevicesA(NULL, num, &dd, 0);
        ok(ret || num != 0, "EnumDisplayDevices fails with num == 0\n");
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
    ok(primary_num != -1, "Didn't get the primary device\n");

    if(pEnumDisplayMonitors && pGetMonitorInfoA) {
        ok(pEnumDisplayMonitors(NULL, NULL, monitor_enum_proc, (LPARAM)primary_monitor_device_name),
           "EnumDisplayMonitors failed\n");

        ok(!strcmp(primary_monitor_device_name, primary_device_name),
           "monitor device name %s, device name %s\n", primary_monitor_device_name,
           primary_device_name);
    }
}

struct vid_mode
{
    DWORD w, h, bpp, freq, fields;
    LONG success;
};

static const struct vid_mode vid_modes_test[] = {
    {640, 480, 0, 0, DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY, 1},
    {640, 480, 0, 0, DM_PELSWIDTH | DM_PELSHEIGHT |                 DM_DISPLAYFREQUENCY, 1},
    {640, 480, 0, 0, DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL                      , 1},
    {640, 480, 0, 0, DM_PELSWIDTH | DM_PELSHEIGHT                                      , 1},
    {640, 480, 0, 0,                                DM_BITSPERPEL                      , 1},
    {640, 480, 0, 0,                                                DM_DISPLAYFREQUENCY, 1},

    {0, 0, 0, 0, DM_PELSWIDTH, 1},
    {0, 0, 0, 0, DM_PELSHEIGHT, 1},

    {640, 480, 0, 0, DM_PELSWIDTH, 0},
    {640, 480, 0, 0, DM_PELSHEIGHT, 0},
    {  0, 480, 0, 0, DM_PELSWIDTH | DM_PELSHEIGHT, 0},
    {640,   0, 0, 0, DM_PELSWIDTH | DM_PELSHEIGHT, 0},

    {0, 0, 0, 0, DM_DISPLAYFREQUENCY, 0},
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
        skip("ChangeDisplaySettingsExA is not available\n");
        return;
    }

    SetLastError(0xdeadbeef);
    res = EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);
    ok(res, "EnumDisplaySettings error %u\n", GetLastError());

    width = dm.dmPelsWidth;

    /* the following 2 tests show that dm.dmSize being 0 is invalid, but
     * ChangeDisplaySettingsExA still reports success.
     */
    memset(&dm, 0, sizeof(dm));
    dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
    dm.dmPelsWidth = width;
    res = pChangeDisplaySettingsExA(NULL, &dm, NULL, CDS_TEST, NULL);
    ok(res == DISP_CHANGE_SUCCESSFUL,
       "ChangeDisplaySettingsExA returned %d, expected DISP_CHANGE_SUCCESSFUL\n", res);

    memset(&dmW, 0, sizeof(dmW));
    dmW.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
    dmW.dmPelsWidth = width;
    SetLastError(0xdeadbeef);
    res = pChangeDisplaySettingsExW(NULL, &dmW, NULL, CDS_TEST, NULL);
    if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
        ok(res == DISP_CHANGE_FAILED,
           "ChangeDisplaySettingsExW returned %d, expected DISP_CHANGE_FAILED\n", res);

    memset(&dm, 0, sizeof(dm));
    dm.dmSize = sizeof(dm);

    for (i = 0; i < vid_modes_cnt; i++)
    {
        dm.dmPelsWidth        = vid_modes_test[i].w;
        dm.dmPelsHeight       = vid_modes_test[i].h;
        dm.dmBitsPerPel       = vid_modes_test[i].bpp;
        dm.dmDisplayFrequency = vid_modes_test[i].freq;
        dm.dmFields           = vid_modes_test[i].fields;
        res = pChangeDisplaySettingsExA(NULL, &dm, NULL, CDS_FULLSCREEN, NULL);
        ok(vid_modes_test[i].success ?
           (res == DISP_CHANGE_SUCCESSFUL) :
           (res == DISP_CHANGE_BADMODE || res == DISP_CHANGE_BADPARAM),
           "Unexpected ChangeDisplaySettingsEx() return code for resolution[%d]: %d\n", i, res);

        if (res == DISP_CHANGE_SUCCESSFUL)
        {
            RECT r, r1, virt;

            SetRect(&virt, 0, 0, GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN));
            OffsetRect(&virt, GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN));

            /* Resolution change resets clip rect */
            ok(GetClipCursor(&r), "GetClipCursor() failed\n");
            ok(EqualRect(&r, &virt), "Invalid clip rect: (%d %d) x (%d %d)\n", r.left, r.top, r.right, r.bottom);

            ok(ClipCursor(NULL), "ClipCursor() failed\n");
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
            ok(EqualRect(&r, &virt), "Invalid clip rect: (%d %d) x (%d %d)\n", r.left, r.top, r.right, r.bottom);
        }
    }
    res = pChangeDisplaySettingsExA(NULL, NULL, NULL, CDS_RESET, NULL);
    ok(res == DISP_CHANGE_SUCCESSFUL, "Failed to reset default resolution: %d\n", res);
}

static void test_monitors(void)
{
    HMONITOR monitor, primary;
    POINT pt;

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


START_TEST(monitor)
{
    init_function_pointers();
    test_enumdisplaydevices();
    if (winetest_interactive)
        test_ChangeDisplaySettingsEx();
    if (pMonitorFromPoint && pMonitorFromWindow)
        test_monitors();
    else
        skip("MonitorFromPoint and/or MonitorFromWindow are not available\n");
}
