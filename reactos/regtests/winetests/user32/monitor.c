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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#define _WIN32_WINNT 0x0500

#include "wine/test.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

static HMODULE hdll;
static BOOL (WINAPI *pEnumDisplayDevicesA)(LPCSTR,DWORD,LPDISPLAY_DEVICEA,DWORD);
static BOOL (WINAPI *pEnumDisplayMonitors)(HDC,LPRECT,MONITORENUMPROC,LPARAM);
static BOOL (WINAPI *pGetMonitorInfoA)(HMONITOR,LPMONITORINFO);

static void init_function_pointers(void)
{
    hdll = GetModuleHandleA("user32.dll");

    if(hdll)
    {
       pEnumDisplayDevicesA = (void*)GetProcAddress(hdll, "EnumDisplayDevicesA");
       pEnumDisplayMonitors = (void*)GetProcAddress(hdll, "EnumDisplayMonitors");
       pGetMonitorInfoA = (void*)GetProcAddress(hdll, "GetMonitorInfoA");
    }
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
            ok(dc != NULL, "Failed to CreateDC(\"%s\") err=%ld\n", dd.DeviceName, GetLastError());
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


START_TEST(monitor)
{
    init_function_pointers();
    test_enumdisplaydevices();
}
