/*
 * Unit test suite for kernel mode graphics driver
 *
 * Copyright 2019 Zhiyi Zhang
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

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winternl.h"
#include "dwmapi.h"
#include "ddk/d3dkmthk.h"
#include "initguid.h"
#ifdef __REACTOS__
#include <winreg.h>
#endif
#include "setupapi.h"
#include "ntddvdeo.h"
#include "devpkey.h"
#include "cfgmgr32.h"

#include "wine/test.h"

static const WCHAR display1W[] = L"\\\\.\\DISPLAY1";

DEFINE_DEVPROPKEY(DEVPROPKEY_GPU_LUID, 0x60b193cb, 0x5276, 0x4d0f, 0x96, 0xfc, 0xf1, 0x73, 0xab, 0xad, 0x3e, 0xc6, 2);

static NTSTATUS (WINAPI *pD3DKMTCheckOcclusion)(const D3DKMT_CHECKOCCLUSION *);
static NTSTATUS (WINAPI *pD3DKMTCheckVidPnExclusiveOwnership)(const D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP *);
static NTSTATUS (WINAPI *pD3DKMTCloseAdapter)(const D3DKMT_CLOSEADAPTER *);
static NTSTATUS (WINAPI *pD3DKMTCreateDevice)(D3DKMT_CREATEDEVICE *);
static NTSTATUS (WINAPI *pD3DKMTDestroyDevice)(const D3DKMT_DESTROYDEVICE *);
static NTSTATUS (WINAPI *pD3DKMTEnumAdapters2)(D3DKMT_ENUMADAPTERS2 *);
static NTSTATUS (WINAPI *pD3DKMTOpenAdapterFromDeviceName)(D3DKMT_OPENADAPTERFROMDEVICENAME *);
static NTSTATUS (WINAPI *pD3DKMTOpenAdapterFromGdiDisplayName)(D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME *);
static NTSTATUS (WINAPI *pD3DKMTOpenAdapterFromHdc)(D3DKMT_OPENADAPTERFROMHDC *);
static NTSTATUS (WINAPI *pD3DKMTSetVidPnSourceOwner)(const D3DKMT_SETVIDPNSOURCEOWNER *);
static NTSTATUS (WINAPI *pD3DKMTQueryVideoMemoryInfo)(D3DKMT_QUERYVIDEOMEMORYINFO *);
static HRESULT  (WINAPI *pDwmEnableComposition)(UINT);

static BOOL get_primary_adapter_name(WCHAR *name)
{
    DISPLAY_DEVICEW dd;
    DWORD adapter_idx;

    dd.cb = sizeof(dd);
    for (adapter_idx = 0; EnumDisplayDevicesW(NULL, adapter_idx, &dd, 0); ++adapter_idx)
    {
        if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
        {
            lstrcpyW(name, dd.DeviceName);
            return TRUE;
        }
    }

    return FALSE;
}

static void test_D3DKMTOpenAdapterFromGdiDisplayName(void)
{
    D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME open_adapter_gdi_desc;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    DISPLAY_DEVICEW display_device = {sizeof(display_device)};
    NTSTATUS status;
    DWORD i;

    lstrcpyW(open_adapter_gdi_desc.DeviceName, display1W);
    if (!pD3DKMTOpenAdapterFromGdiDisplayName
        || pD3DKMTOpenAdapterFromGdiDisplayName(&open_adapter_gdi_desc) == STATUS_PROCEDURE_NOT_FOUND)
    {
        win_skip("D3DKMTOpenAdapterFromGdiDisplayName() is unavailable.\n");
        return;
    }

    close_adapter_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = pD3DKMTCloseAdapter(&close_adapter_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);

    /* Invalid parameters */
    status = pD3DKMTOpenAdapterFromGdiDisplayName(NULL);
    ok(status == STATUS_UNSUCCESSFUL, "Got unexpected return code %#lx.\n", status);

    memset(&open_adapter_gdi_desc, 0, sizeof(open_adapter_gdi_desc));
    status = pD3DKMTOpenAdapterFromGdiDisplayName(&open_adapter_gdi_desc);
    ok(status == STATUS_UNSUCCESSFUL, "Got unexpected return code %#lx.\n", status);

    /* Open adapter */
    for (i = 0; EnumDisplayDevicesW(NULL, i, &display_device, 0); ++i)
    {
        lstrcpyW(open_adapter_gdi_desc.DeviceName, display_device.DeviceName);
        status = pD3DKMTOpenAdapterFromGdiDisplayName(&open_adapter_gdi_desc);
        if (display_device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
            ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);
        else
        {
            ok(status == STATUS_UNSUCCESSFUL, "Got unexpected return code %#lx.\n", status);
            continue;
        }

        ok(open_adapter_gdi_desc.hAdapter, "Expect not null.\n");
        ok(open_adapter_gdi_desc.AdapterLuid.LowPart || open_adapter_gdi_desc.AdapterLuid.HighPart,
           "Expect LUID not zero.\n");

        close_adapter_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
        status = pD3DKMTCloseAdapter(&close_adapter_desc);
        ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);
    }
}

static void test_D3DKMTOpenAdapterFromHdc(void)
{
    DISPLAY_DEVICEW display_device = {sizeof(display_device)};
    D3DKMT_OPENADAPTERFROMHDC open_adapter_hdc_desc;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    INT adapter_count = 0;
    NTSTATUS status;
    HDC hdc;
    DWORD i;

    if (!pD3DKMTOpenAdapterFromHdc)
    {
        win_skip("D3DKMTOpenAdapterFromHdc() is missing.\n");
        return;
    }

    /* Invalid parameters */
    /* Passing a NULL pointer crashes on Windows 10 >= 2004 */
    if (0) status = pD3DKMTOpenAdapterFromHdc(NULL);

    memset(&open_adapter_hdc_desc, 0, sizeof(open_adapter_hdc_desc));
    status = pD3DKMTOpenAdapterFromHdc(&open_adapter_hdc_desc);
    if (status == STATUS_PROCEDURE_NOT_FOUND)
    {
        win_skip("D3DKMTOpenAdapterFromHdc() is not supported.\n");
        return;
    }
    todo_wine ok(status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status);

    /* Open adapter */
    for (i = 0; EnumDisplayDevicesW(NULL, i, &display_device, 0); ++i)
    {
        if (!(display_device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
            continue;

        adapter_count++;

        hdc = CreateDCW(0, display_device.DeviceName, 0, NULL);
        open_adapter_hdc_desc.hDc = hdc;
        status = pD3DKMTOpenAdapterFromHdc(&open_adapter_hdc_desc);
        todo_wine ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);
        todo_wine ok(open_adapter_hdc_desc.hAdapter, "Expect not null.\n");
        DeleteDC(hdc);

        if (status == STATUS_SUCCESS)
        {
            close_adapter_desc.hAdapter = open_adapter_hdc_desc.hAdapter;
            status = pD3DKMTCloseAdapter(&close_adapter_desc);
            ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);
        }
    }

    /* HDC covering more than two adapters is invalid for D3DKMTOpenAdapterFromHdc */
    hdc = GetDC(0);
    open_adapter_hdc_desc.hDc = hdc;
    status = pD3DKMTOpenAdapterFromHdc(&open_adapter_hdc_desc);
    ReleaseDC(0, hdc);
    todo_wine ok(status == (adapter_count > 1 ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS),
                 "Got unexpected return code %#lx.\n", status);
    if (status == STATUS_SUCCESS)
    {
        close_adapter_desc.hAdapter = open_adapter_hdc_desc.hAdapter;
        status = pD3DKMTCloseAdapter(&close_adapter_desc);
        ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);
    }
}

static void test_D3DKMTEnumAdapters2(void)
{
    D3DKMT_ENUMADAPTERS2 enum_adapters_2_desc = {0};
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    NTSTATUS status;
    UINT i;

    if (!pD3DKMTEnumAdapters2 || pD3DKMTEnumAdapters2(&enum_adapters_2_desc) == STATUS_PROCEDURE_NOT_FOUND)
    {
        win_skip("D3DKMTEnumAdapters2() is unavailable.\n");
        return;
    }

    /* Invalid parameters */
    status = pD3DKMTEnumAdapters2(NULL);
    ok(status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status);

    /* Query the array to allocate */
    memset(&enum_adapters_2_desc, 0, sizeof(enum_adapters_2_desc));
    status = pD3DKMTEnumAdapters2(&enum_adapters_2_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);
    ok(enum_adapters_2_desc.NumAdapters == 32 /* win10 and older */ || enum_adapters_2_desc.NumAdapters == 34 /* win11 */,
       "Got unexpected value %lu.\n", enum_adapters_2_desc.NumAdapters);

    /* Allocate the array */
    enum_adapters_2_desc.pAdapters = calloc(enum_adapters_2_desc.NumAdapters, sizeof(D3DKMT_ADAPTERINFO));
    ok(!!enum_adapters_2_desc.pAdapters, "Expect not null.\n");

    /* Enumerate adapters */
    status = pD3DKMTEnumAdapters2(&enum_adapters_2_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);
    ok(enum_adapters_2_desc.NumAdapters, "Expect not zero.\n");

    for (i = 0; i < enum_adapters_2_desc.NumAdapters; ++i)
    {
        ok(enum_adapters_2_desc.pAdapters[i].hAdapter, "Expect not null.\n");
        ok(enum_adapters_2_desc.pAdapters[i].AdapterLuid.LowPart || enum_adapters_2_desc.pAdapters[i].AdapterLuid.HighPart,
           "Expect LUID not zero.\n");

        close_adapter_desc.hAdapter = enum_adapters_2_desc.pAdapters[i].hAdapter;
        status = pD3DKMTCloseAdapter(&close_adapter_desc);
        ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);
    }

    /* Check for insufficient buffer */
    enum_adapters_2_desc.NumAdapters = 0;
    status = pD3DKMTEnumAdapters2(&enum_adapters_2_desc);
    ok(status == STATUS_BUFFER_TOO_SMALL, "Got unexpected return code %#lx.\n", status);

    free(enum_adapters_2_desc.pAdapters);
}

static void test_D3DKMTCloseAdapter(void)
{
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    NTSTATUS status;

    if (!pD3DKMTCloseAdapter || pD3DKMTCloseAdapter(NULL) == STATUS_PROCEDURE_NOT_FOUND)
    {
        win_skip("D3DKMTCloseAdapter() is unavailable.\n");
        return;
    }

    /* Invalid parameters */
    status = pD3DKMTCloseAdapter(NULL);
    ok(status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status);

    memset(&close_adapter_desc, 0, sizeof(close_adapter_desc));
    status = pD3DKMTCloseAdapter(&close_adapter_desc);
    ok(status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status);
}

static void test_D3DKMTCreateDevice(void)
{
    D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME open_adapter_gdi_desc;
    D3DKMT_CREATEDEVICE create_device_desc;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    D3DKMT_DESTROYDEVICE destroy_device_desc;
    NTSTATUS status;

    if (!pD3DKMTCreateDevice || pD3DKMTCreateDevice(NULL) == STATUS_PROCEDURE_NOT_FOUND)
    {
        win_skip("D3DKMTCreateDevice() is unavailable.\n");
        return;
    }

    /* Invalid parameters */
    status = pD3DKMTCreateDevice(NULL);
    ok(status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status);

    memset(&create_device_desc, 0, sizeof(create_device_desc));
    status = pD3DKMTCreateDevice(&create_device_desc);
    ok(status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status);

    lstrcpyW(open_adapter_gdi_desc.DeviceName, display1W);
    status = pD3DKMTOpenAdapterFromGdiDisplayName(&open_adapter_gdi_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);

    /* Create device */
    create_device_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = pD3DKMTCreateDevice(&create_device_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);
    ok(create_device_desc.hDevice, "Expect not null.\n");
    ok(create_device_desc.pCommandBuffer == NULL, "Expect null.\n");
    ok(create_device_desc.CommandBufferSize == 0, "Got wrong value %#x.\n", create_device_desc.CommandBufferSize);
    ok(create_device_desc.pAllocationList == NULL, "Expect null.\n");
    ok(create_device_desc.AllocationListSize == 0, "Got wrong value %#x.\n", create_device_desc.AllocationListSize);
    ok(create_device_desc.pPatchLocationList == NULL, "Expect null.\n");
    ok(create_device_desc.PatchLocationListSize == 0, "Got wrong value %#x.\n", create_device_desc.PatchLocationListSize);

    destroy_device_desc.hDevice = create_device_desc.hDevice;
    status = pD3DKMTDestroyDevice(&destroy_device_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);

    close_adapter_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = pD3DKMTCloseAdapter(&close_adapter_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);
}

static void test_D3DKMTDestroyDevice(void)
{
    D3DKMT_DESTROYDEVICE destroy_device_desc;
    NTSTATUS status;

    if (!pD3DKMTDestroyDevice || pD3DKMTDestroyDevice(NULL) == STATUS_PROCEDURE_NOT_FOUND)
    {
        win_skip("D3DKMTDestroyDevice() is unavailable.\n");
        return;
    }

    /* Invalid parameters */
    status = pD3DKMTDestroyDevice(NULL);
    ok(status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status);

    memset(&destroy_device_desc, 0, sizeof(destroy_device_desc));
    status = pD3DKMTDestroyDevice(&destroy_device_desc);
    ok(status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status);
}

static void test_D3DKMTCheckVidPnExclusiveOwnership(void)
{
    static const DWORD timeout = 1000;
    static const DWORD wait_step = 100;
    D3DKMT_CREATEDEVICE create_device_desc, create_device_desc2;
    D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME open_adapter_gdi_desc;
    D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP check_owner_desc;
    D3DKMT_DESTROYDEVICE destroy_device_desc;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    D3DKMT_VIDPNSOURCEOWNER_TYPE owner_type;
    D3DKMT_SETVIDPNSOURCEOWNER set_owner_desc;
    DWORD total_time;
    NTSTATUS status;
    INT i;

    /* Test cases using single device */
    static const struct test_data1
    {
        D3DKMT_VIDPNSOURCEOWNER_TYPE owner_type;
        NTSTATUS expected_set_status;
        NTSTATUS expected_check_status;
    } tests1[] = {
        /* 0 */
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVEGDI, STATUS_INVALID_PARAMETER, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_SUCCESS},
        /* 10 */
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        /* 20 */
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        /* 30 */
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_INVALID_PARAMETER, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        /* 40 */
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_INVALID_PARAMETER, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        /* 50 */
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_INVALID_PARAMETER, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED + 1, STATUS_INVALID_PARAMETER, STATUS_SUCCESS},
    };

    /* Test cases using two devices consecutively */
    static const struct test_data2
    {
        D3DKMT_VIDPNSOURCEOWNER_TYPE set_owner_type1;
        D3DKMT_VIDPNSOURCEOWNER_TYPE set_owner_type2;
        NTSTATUS expected_set_status1;
        NTSTATUS expected_set_status2;
        NTSTATUS expected_check_status;
    } tests2[] = {
        /* 0 */
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_SUCCESS, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_SUCCESS, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        /* 10 */
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_SUCCESS, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {-1, D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, -1, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, -1, STATUS_SUCCESS, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
    };

    if (!pD3DKMTCheckVidPnExclusiveOwnership || pD3DKMTCheckVidPnExclusiveOwnership(NULL) == STATUS_PROCEDURE_NOT_FOUND)
    {
        /* This is a stub in some drivers (e.g. nulldrv) */
        skip("D3DKMTCheckVidPnExclusiveOwnership() is unavailable.\n");
        return;
    }

    /* Invalid parameters */
    status = pD3DKMTCheckVidPnExclusiveOwnership(NULL);
    ok(status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status);

    memset(&check_owner_desc, 0, sizeof(check_owner_desc));
    status = pD3DKMTCheckVidPnExclusiveOwnership(&check_owner_desc);
    ok(status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status);

    /* Test cases */
    lstrcpyW(open_adapter_gdi_desc.DeviceName, display1W);
    status = pD3DKMTOpenAdapterFromGdiDisplayName(&open_adapter_gdi_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);

    memset(&create_device_desc, 0, sizeof(create_device_desc));
    create_device_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = pD3DKMTCreateDevice(&create_device_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);

    check_owner_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    check_owner_desc.VidPnSourceId = open_adapter_gdi_desc.VidPnSourceId;
    for (i = 0; i < ARRAY_SIZE(tests1); ++i)
    {
        set_owner_desc.hDevice = create_device_desc.hDevice;
        if (tests1[i].owner_type != -1)
        {
            owner_type = tests1[i].owner_type;
            set_owner_desc.pType = &owner_type;
            set_owner_desc.pVidPnSourceId = &open_adapter_gdi_desc.VidPnSourceId;
            set_owner_desc.VidPnSourceCount = 1;
        }
        else
        {
            set_owner_desc.pType = NULL;
            set_owner_desc.pVidPnSourceId = NULL;
            set_owner_desc.VidPnSourceCount = 0;
        }
        status = pD3DKMTSetVidPnSourceOwner(&set_owner_desc);
        ok(status == tests1[i].expected_set_status ||
               /* win8 doesn't support D3DKMT_VIDPNSOURCEOWNER_EMULATED */
               (status == STATUS_INVALID_PARAMETER && tests1[i].owner_type == D3DKMT_VIDPNSOURCEOWNER_EMULATED)
               || (status == STATUS_SUCCESS && tests1[i].owner_type == D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE
                   && tests1[i - 1].owner_type == D3DKMT_VIDPNSOURCEOWNER_EMULATED),
           "Got unexpected return code %#lx at test %d.\n", status, i);

        status = pD3DKMTCheckVidPnExclusiveOwnership(&check_owner_desc);
        /* If don't sleep, D3DKMTCheckVidPnExclusiveOwnership may get STATUS_GRAPHICS_PRESENT_UNOCCLUDED instead
         * of STATUS_SUCCESS */
        if ((tests1[i].expected_check_status == STATUS_SUCCESS && status == STATUS_GRAPHICS_PRESENT_UNOCCLUDED))
        {
            total_time = 0;
            do
            {
                Sleep(wait_step);
                total_time += wait_step;
                status = pD3DKMTCheckVidPnExclusiveOwnership(&check_owner_desc);
            } while (status == STATUS_GRAPHICS_PRESENT_UNOCCLUDED && total_time < timeout);
        }
        ok(status == tests1[i].expected_check_status
               || (status == STATUS_GRAPHICS_PRESENT_OCCLUDED                               /* win8 */
                   && tests1[i].owner_type == D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE
                   && tests1[i - 1].owner_type == D3DKMT_VIDPNSOURCEOWNER_EMULATED),
           "Got unexpected return code %#lx at test %d.\n", status, i);
    }

    /* Set owner and unset owner using different devices */
    memset(&create_device_desc2, 0, sizeof(create_device_desc2));
    create_device_desc2.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = pD3DKMTCreateDevice(&create_device_desc2);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);

    /* Set owner with the first device */
    set_owner_desc.hDevice = create_device_desc.hDevice;
    owner_type = D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE;
    set_owner_desc.pType = &owner_type;
    set_owner_desc.pVidPnSourceId = &open_adapter_gdi_desc.VidPnSourceId;
    set_owner_desc.VidPnSourceCount = 1;
    status = pD3DKMTSetVidPnSourceOwner(&set_owner_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);
    status = pD3DKMTCheckVidPnExclusiveOwnership(&check_owner_desc);
    ok(status == STATUS_GRAPHICS_PRESENT_OCCLUDED, "Got unexpected return code %#lx.\n", status);

    /* Unset owner with the second device */
    set_owner_desc.hDevice = create_device_desc2.hDevice;
    set_owner_desc.pType = NULL;
    set_owner_desc.pVidPnSourceId = NULL;
    set_owner_desc.VidPnSourceCount = 0;
    status = pD3DKMTSetVidPnSourceOwner(&set_owner_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);
    status = pD3DKMTCheckVidPnExclusiveOwnership(&check_owner_desc);
    /* No effect */
    ok(status == STATUS_GRAPHICS_PRESENT_OCCLUDED, "Got unexpected return code %#lx.\n", status);

    /* Unset owner with the first device */
    set_owner_desc.hDevice = create_device_desc.hDevice;
    set_owner_desc.pType = NULL;
    set_owner_desc.pVidPnSourceId = NULL;
    set_owner_desc.VidPnSourceCount = 0;
    status = pD3DKMTSetVidPnSourceOwner(&set_owner_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);
    status = pD3DKMTCheckVidPnExclusiveOwnership(&check_owner_desc);
    /* Proves that the correct device is needed to unset owner */
    ok(status == STATUS_SUCCESS || status == STATUS_GRAPHICS_PRESENT_UNOCCLUDED, "Got unexpected return code %#lx.\n",
       status);

    /* Set owner with the first device, set owner again with the second device */
    for (i = 0; i < ARRAY_SIZE(tests2); ++i)
    {
        if (tests2[i].set_owner_type1 != -1)
        {
            set_owner_desc.hDevice = create_device_desc.hDevice;
            owner_type = tests2[i].set_owner_type1;
            set_owner_desc.pType = &owner_type;
            set_owner_desc.pVidPnSourceId = &open_adapter_gdi_desc.VidPnSourceId;
            set_owner_desc.VidPnSourceCount = 1;
            /* If don't sleep, D3DKMTSetVidPnSourceOwner may return STATUS_OK for D3DKMT_VIDPNSOURCEOWNER_SHARED.
             * Other owner type doesn't seems to be affected. */
            if (tests2[i].set_owner_type1 == D3DKMT_VIDPNSOURCEOWNER_SHARED)
                Sleep(timeout);
            status = pD3DKMTSetVidPnSourceOwner(&set_owner_desc);
            ok(status == tests2[i].expected_set_status1
                   || (status == STATUS_INVALID_PARAMETER                                    /* win8 */
                       && tests2[i].set_owner_type1 == D3DKMT_VIDPNSOURCEOWNER_EMULATED),
               "Got unexpected return code %#lx at test %d.\n", status, i);
        }

        if (tests2[i].set_owner_type2 != -1)
        {
            set_owner_desc.hDevice = create_device_desc2.hDevice;
            owner_type = tests2[i].set_owner_type2;
            set_owner_desc.pType = &owner_type;
            set_owner_desc.pVidPnSourceId = &open_adapter_gdi_desc.VidPnSourceId;
            set_owner_desc.VidPnSourceCount = 1;
            status = pD3DKMTSetVidPnSourceOwner(&set_owner_desc);
            ok(status == tests2[i].expected_set_status2
                   || (status == STATUS_INVALID_PARAMETER                                   /* win8 */
                       && tests2[i].set_owner_type2 == D3DKMT_VIDPNSOURCEOWNER_EMULATED)
                   || (status == STATUS_SUCCESS && tests2[i].set_owner_type1 == D3DKMT_VIDPNSOURCEOWNER_EMULATED
                       && tests2[i].set_owner_type2 == D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE),
               "Got unexpected return code %#lx at test %d.\n", status, i);
        }

        status = pD3DKMTCheckVidPnExclusiveOwnership(&check_owner_desc);
        if ((tests2[i].expected_check_status == STATUS_SUCCESS && status == STATUS_GRAPHICS_PRESENT_UNOCCLUDED))
        {
            total_time = 0;
            do
            {
                Sleep(wait_step);
                total_time += wait_step;
                status = pD3DKMTCheckVidPnExclusiveOwnership(&check_owner_desc);
            } while (status == STATUS_GRAPHICS_PRESENT_UNOCCLUDED && total_time < timeout);
        }
        ok(status == tests2[i].expected_check_status
               || (status == STATUS_GRAPHICS_PRESENT_OCCLUDED                               /* win8 */
                   && tests2[i].set_owner_type2 == D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE
                   && tests2[i].set_owner_type1 == D3DKMT_VIDPNSOURCEOWNER_EMULATED),
           "Got unexpected return code %#lx at test %d.\n", status, i);

        /* Unset owner with first device */
        if (tests2[i].set_owner_type1 != -1)
        {
            set_owner_desc.hDevice = create_device_desc.hDevice;
            set_owner_desc.pType = NULL;
            set_owner_desc.pVidPnSourceId = NULL;
            set_owner_desc.VidPnSourceCount = 0;
            status = pD3DKMTSetVidPnSourceOwner(&set_owner_desc);
            ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx at test %d.\n", status, i);
        }

        /* Unset owner with second device */
        if (tests2[i].set_owner_type2 != -1)
        {
            set_owner_desc.hDevice = create_device_desc2.hDevice;
            set_owner_desc.pType = NULL;
            set_owner_desc.pVidPnSourceId = NULL;
            set_owner_desc.VidPnSourceCount = 0;
            status = pD3DKMTSetVidPnSourceOwner(&set_owner_desc);
            ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx at test %d.\n", status, i);
        }
    }

    /* Destroy devices holding ownership */
    set_owner_desc.hDevice = create_device_desc.hDevice;
    owner_type = D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE;
    set_owner_desc.pType = &owner_type;
    set_owner_desc.pVidPnSourceId = &open_adapter_gdi_desc.VidPnSourceId;
    set_owner_desc.VidPnSourceCount = 1;
    status = pD3DKMTSetVidPnSourceOwner(&set_owner_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);

    destroy_device_desc.hDevice = create_device_desc.hDevice;
    status = pD3DKMTDestroyDevice(&destroy_device_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);

    set_owner_desc.hDevice = create_device_desc2.hDevice;
    owner_type = D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE;
    set_owner_desc.pType = &owner_type;
    set_owner_desc.pVidPnSourceId = &open_adapter_gdi_desc.VidPnSourceId;
    set_owner_desc.VidPnSourceCount = 1;
    status = pD3DKMTSetVidPnSourceOwner(&set_owner_desc);
    /* So ownership is released when device is destroyed. otherwise the return code should be
     * STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE */
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);

    destroy_device_desc.hDevice = create_device_desc2.hDevice;
    status = pD3DKMTDestroyDevice(&destroy_device_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);

    close_adapter_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = pD3DKMTCloseAdapter(&close_adapter_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);
}

static void test_D3DKMTSetVidPnSourceOwner(void)
{
    D3DKMT_SETVIDPNSOURCEOWNER set_owner_desc = {0};
    NTSTATUS status;

    if (!pD3DKMTSetVidPnSourceOwner || pD3DKMTSetVidPnSourceOwner(&set_owner_desc) == STATUS_PROCEDURE_NOT_FOUND)
    {
        /* This is a stub in some drivers (e.g. nulldrv) */
        skip("D3DKMTSetVidPnSourceOwner() is unavailable.\n");
        return;
    }

    /* Invalid parameters */
    status = pD3DKMTSetVidPnSourceOwner(&set_owner_desc);
    ok(status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status);
}

static void test_D3DKMTCheckOcclusion(void)
{
    DISPLAY_DEVICEW display_device = {sizeof(display_device)};
    D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME open_adapter_gdi_desc;
    D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP check_owner_desc;
    D3DKMT_SETVIDPNSOURCEOWNER set_owner_desc;
    D3DKMT_DESTROYDEVICE destroy_device_desc;
    D3DKMT_VIDPNSOURCEOWNER_TYPE owner_type;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    D3DKMT_CREATEDEVICE create_device_desc;
    D3DKMT_CHECKOCCLUSION occlusion_desc;
    NTSTATUS expected_occlusion, status;
    INT i, adapter_count = 0;
    HWND hwnd, hwnd2;
    HRESULT hr;

    if (!pD3DKMTCheckOcclusion || pD3DKMTCheckOcclusion(NULL) == STATUS_PROCEDURE_NOT_FOUND)
    {
        todo_wine win_skip("D3DKMTCheckOcclusion() is unavailable.\n");
        return;
    }

    /* NULL parameter check */
    status = pD3DKMTCheckOcclusion(NULL);
    ok(status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status);

    occlusion_desc.hWnd = NULL;
    status = pD3DKMTCheckOcclusion(&occlusion_desc);
    ok(status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status);

    hwnd = CreateWindowA("static", "static1", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 200, 200, 0, 0, 0, 0);
    ok(hwnd != NULL, "Failed to create window.\n");

    occlusion_desc.hWnd = hwnd;
    status = pD3DKMTCheckOcclusion(&occlusion_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);

    /* Minimized state doesn't affect D3DKMTCheckOcclusion */
    ShowWindow(hwnd, SW_MINIMIZE);
    occlusion_desc.hWnd = hwnd;
    status = pD3DKMTCheckOcclusion(&occlusion_desc);
    flaky
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);
    ShowWindow(hwnd, SW_SHOWNORMAL);

    /* Invisible state doesn't affect D3DKMTCheckOcclusion */
    ShowWindow(hwnd, SW_HIDE);
    occlusion_desc.hWnd = hwnd;
    status = pD3DKMTCheckOcclusion(&occlusion_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);
    ShowWindow(hwnd, SW_SHOW);

    /* hwnd2 covers hwnd */
    hwnd2 = CreateWindowA("static", "static2", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 200, 200, 0, 0, 0, 0);
    ok(hwnd2 != NULL, "Failed to create window.\n");

    occlusion_desc.hWnd = hwnd;
    status = pD3DKMTCheckOcclusion(&occlusion_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);

    occlusion_desc.hWnd = hwnd2;
    status = pD3DKMTCheckOcclusion(&occlusion_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);

    /* Composition doesn't affect D3DKMTCheckOcclusion */
    if (pDwmEnableComposition)
    {
        hr = pDwmEnableComposition(DWM_EC_DISABLECOMPOSITION);
        ok(hr == S_OK, "Failed to disable composition.\n");

        occlusion_desc.hWnd = hwnd;
        status = pD3DKMTCheckOcclusion(&occlusion_desc);
        /* This result means that D3DKMTCheckOcclusion doesn't check composition status despite MSDN says it will */
        ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);

        occlusion_desc.hWnd = hwnd2;
        status = pD3DKMTCheckOcclusion(&occlusion_desc);
        ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);

        ShowWindow(hwnd, SW_MINIMIZE);
        occlusion_desc.hWnd = hwnd;
        status = pD3DKMTCheckOcclusion(&occlusion_desc);
        flaky
        ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);
        ShowWindow(hwnd, SW_SHOWNORMAL);

        ShowWindow(hwnd, SW_HIDE);
        occlusion_desc.hWnd = hwnd;
        status = pD3DKMTCheckOcclusion(&occlusion_desc);
        ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);
        ShowWindow(hwnd, SW_SHOW);

        hr = pDwmEnableComposition(DWM_EC_ENABLECOMPOSITION);
        ok(hr == S_OK, "Failed to enable composition.\n");
    }
    else
        skip("Skip testing composition.\n");

    lstrcpyW(open_adapter_gdi_desc.DeviceName, display1W);
    status = pD3DKMTOpenAdapterFromGdiDisplayName(&open_adapter_gdi_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);

    memset(&create_device_desc, 0, sizeof(create_device_desc));
    create_device_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = pD3DKMTCreateDevice(&create_device_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);

    check_owner_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    check_owner_desc.VidPnSourceId = open_adapter_gdi_desc.VidPnSourceId;
    status = pD3DKMTCheckVidPnExclusiveOwnership(&check_owner_desc);
    /* D3DKMTCheckVidPnExclusiveOwnership gets STATUS_GRAPHICS_PRESENT_UNOCCLUDED sometimes and with some delay,
     * it will always return STATUS_SUCCESS. So there are some timing issues here. */
    ok(status == STATUS_SUCCESS || status == STATUS_GRAPHICS_PRESENT_UNOCCLUDED, "Got unexpected return code %#lx.\n", status);

    /* Test D3DKMTCheckOcclusion relationship with video present source owner */
    set_owner_desc.hDevice = create_device_desc.hDevice;
    owner_type = D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE;
    set_owner_desc.pType = &owner_type;
    set_owner_desc.pVidPnSourceId = &open_adapter_gdi_desc.VidPnSourceId;
    set_owner_desc.VidPnSourceCount = 1;
    status = pD3DKMTSetVidPnSourceOwner(&set_owner_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);

    for (i = 0; EnumDisplayDevicesW(NULL, i, &display_device, 0); ++i)
    {
        if ((display_device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
            adapter_count++;
    }
    /* STATUS_GRAPHICS_PRESENT_OCCLUDED on single monitor system. STATUS_SUCCESS on multiple monitor system. */
    expected_occlusion = adapter_count > 1 ? STATUS_SUCCESS : STATUS_GRAPHICS_PRESENT_OCCLUDED;

    occlusion_desc.hWnd = hwnd;
    status = pD3DKMTCheckOcclusion(&occlusion_desc);
    ok(status == expected_occlusion, "Got unexpected return code %#lx.\n", status);

    /* Note hwnd2 is not actually occluded but D3DKMTCheckOcclusion reports STATUS_GRAPHICS_PRESENT_OCCLUDED as well */
    SetWindowPos(hwnd2, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    ShowWindow(hwnd2, SW_SHOW);
    occlusion_desc.hWnd = hwnd2;
    status = pD3DKMTCheckOcclusion(&occlusion_desc);
    ok(status == expected_occlusion, "Got unexpected return code %#lx.\n", status);

    /* Now hwnd is HWND_TOPMOST. Still reports STATUS_GRAPHICS_PRESENT_OCCLUDED */
    ok(SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE), "Failed to SetWindowPos.\n");
    ok(GetWindowLongW(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST, "No WS_EX_TOPMOST style.\n");
    occlusion_desc.hWnd = hwnd;
    status = pD3DKMTCheckOcclusion(&occlusion_desc);
    ok(status == expected_occlusion, "Got unexpected return code %#lx.\n", status);

    DestroyWindow(hwnd2);
    occlusion_desc.hWnd = hwnd;
    status = pD3DKMTCheckOcclusion(&occlusion_desc);
    ok(status == expected_occlusion, "Got unexpected return code %#lx.\n", status);

    check_owner_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    check_owner_desc.VidPnSourceId = open_adapter_gdi_desc.VidPnSourceId;
    status = pD3DKMTCheckVidPnExclusiveOwnership(&check_owner_desc);
    ok(status == STATUS_GRAPHICS_PRESENT_OCCLUDED, "Got unexpected return code %#lx.\n", status);

    /* Unset video present source owner */
    set_owner_desc.hDevice = create_device_desc.hDevice;
    set_owner_desc.pType = NULL;
    set_owner_desc.pVidPnSourceId = NULL;
    set_owner_desc.VidPnSourceCount = 0;
    status = pD3DKMTSetVidPnSourceOwner(&set_owner_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);

    occlusion_desc.hWnd = hwnd;
    status = pD3DKMTCheckOcclusion(&occlusion_desc);
    flaky
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);

    check_owner_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    check_owner_desc.VidPnSourceId = open_adapter_gdi_desc.VidPnSourceId;
    status = pD3DKMTCheckVidPnExclusiveOwnership(&check_owner_desc);
    flaky
    ok(status == STATUS_SUCCESS || status == STATUS_GRAPHICS_PRESENT_UNOCCLUDED, "Got unexpected return code %#lx.\n", status);

    destroy_device_desc.hDevice = create_device_desc.hDevice;
    status = pD3DKMTDestroyDevice(&destroy_device_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);

    close_adapter_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = pD3DKMTCloseAdapter(&close_adapter_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);
    DestroyWindow(hwnd);
}

#ifndef __REACTOS__
static void test_D3DKMTOpenAdapterFromDeviceName_deviface(const GUID *devinterface_guid,
        NTSTATUS expected_status, BOOL todo)
{
    BYTE iface_detail_buffer[sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) + 256 * sizeof(WCHAR)];
    SP_DEVINFO_DATA device_data = {sizeof(device_data)};
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *iface_data;
    D3DKMT_OPENADAPTERFROMDEVICENAME device_name;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    DEVPROPTYPE type;
    NTSTATUS status;
    unsigned int i;
    HDEVINFO set;
    LUID luid;
    BOOL ret;

    set = SetupDiGetClassDevsW(devinterface_guid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    ok(set != INVALID_HANDLE_VALUE, "SetupDiGetClassDevs failed, error %lu.\n", GetLastError());

    iface_data = (SP_DEVICE_INTERFACE_DETAIL_DATA_W *)iface_detail_buffer;
    iface_data->cbSize = sizeof(*iface_data);
    device_name.pDeviceName = iface_data->DevicePath;

    i = 0;
    while (SetupDiEnumDeviceInterfaces(set, NULL, devinterface_guid, i, &iface))
    {
        ret = SetupDiGetDeviceInterfaceDetailW(set, &iface, iface_data,
                sizeof(iface_detail_buffer), NULL, &device_data );
        ok(ret, "Got unexpected ret %d, GetLastError() %lu.\n", ret, GetLastError());

        status = pD3DKMTOpenAdapterFromDeviceName(&device_name);
        todo_wine_if(todo) ok(status == expected_status, "Got status %#lx, expected %#lx.\n", status, expected_status);

        if (!status)
        {
            ret = SetupDiGetDevicePropertyW(set, &device_data, &DEVPROPKEY_GPU_LUID, &type,
                    (BYTE *)&luid, sizeof(luid), NULL, 0);
            ok(ret || GetLastError() == ERROR_NOT_FOUND, "Got unexpected ret %d, GetLastError() %lu.\n",
                    ret, GetLastError());

            if (ret)
            {
                ret = RtlEqualLuid( &luid, &device_name.AdapterLuid);
                todo_wine ok(ret, "Luid does not match.\n");
            }
            else
            {
                skip("Luid not found.\n");
            }

            close_adapter_desc.hAdapter = device_name.hAdapter;
            status = pD3DKMTCloseAdapter(&close_adapter_desc);
            ok(!status, "Got unexpected status %#lx.\n", status);
        }
        ++i;
    }
    if (!i)
        win_skip("No devices found.\n");

    SetupDiDestroyDeviceInfoList( set );
}
#endif

static void test_D3DKMTOpenAdapterFromDeviceName(void)
{
    D3DKMT_OPENADAPTERFROMDEVICENAME device_name;
    NTSTATUS status;

#ifdef __REACTOS__
    if (pD3DKMTOpenAdapterFromDeviceName == NULL)
    {
        win_skip("D3DKMTOpenAdapterFromDeviceName() is not available.\n");
        return;
    }
#endif

    /* Make sure display devices are initialized. */
    SendMessageW(GetDesktopWindow(), WM_NULL, 0, 0);

    status = pD3DKMTOpenAdapterFromDeviceName(NULL);
    if (status == STATUS_PROCEDURE_NOT_FOUND)
    {
        win_skip("D3DKMTOpenAdapterFromDeviceName() is not supported.\n");
        return;
    }
    ok(status == STATUS_INVALID_PARAMETER, "Got unexpected status %#lx.\n", status);

    memset(&device_name, 0, sizeof(device_name));
    status = pD3DKMTOpenAdapterFromDeviceName(&device_name);
    ok(status == STATUS_INVALID_PARAMETER, "Got unexpected status %#lx.\n", status);

#ifndef __REACTOS__
    winetest_push_context("GUID_DEVINTERFACE_DISPLAY_ADAPTER");
    test_D3DKMTOpenAdapterFromDeviceName_deviface(&GUID_DEVINTERFACE_DISPLAY_ADAPTER, STATUS_INVALID_PARAMETER, TRUE);
    winetest_pop_context();

    winetest_push_context("GUID_DISPLAY_DEVICE_ARRIVAL");
    test_D3DKMTOpenAdapterFromDeviceName_deviface(&GUID_DISPLAY_DEVICE_ARRIVAL, STATUS_SUCCESS, FALSE);
    winetest_pop_context();
#endif
}

static void test_D3DKMTQueryVideoMemoryInfo(void)
{
    static const D3DKMT_MEMORY_SEGMENT_GROUP groups[] = {D3DKMT_MEMORY_SEGMENT_GROUP_LOCAL,
                                                         D3DKMT_MEMORY_SEGMENT_GROUP_NON_LOCAL};
    D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME open_adapter_desc;
    D3DKMT_QUERYVIDEOMEMORYINFO query_memory_info;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    NTSTATUS status;
    unsigned int i;
    BOOL ret;

    if (!pD3DKMTQueryVideoMemoryInfo)
    {
        win_skip("D3DKMTQueryVideoMemoryInfo() is unavailable.\n");
        return;
    }

    ret = get_primary_adapter_name(open_adapter_desc.DeviceName);
    ok(ret, "Failed to get primary adapter name.\n");
    status = pD3DKMTOpenAdapterFromGdiDisplayName(&open_adapter_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);

    /* Normal query */
    for (i = 0; i < ARRAY_SIZE(groups); ++i)
    {
        winetest_push_context("group %d", groups[i]);

        query_memory_info.hProcess = NULL;
        query_memory_info.hAdapter = open_adapter_desc.hAdapter;
        query_memory_info.PhysicalAdapterIndex = 0;
        query_memory_info.MemorySegmentGroup = groups[i];
        status = pD3DKMTQueryVideoMemoryInfo(&query_memory_info);
        todo_wine_if (status == STATUS_INVALID_PARAMETER)  /* fails on Wine without a Vulkan adapter */
        ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);
        ok(query_memory_info.Budget >= query_memory_info.AvailableForReservation,
           "Unexpected budget %I64u and reservation %I64u.\n", query_memory_info.Budget,
           query_memory_info.AvailableForReservation);
        ok(query_memory_info.CurrentUsage <= query_memory_info.Budget,
           "Unexpected current usage %I64u.\n", query_memory_info.CurrentUsage);
        ok(query_memory_info.CurrentReservation == 0,
           "Unexpected current reservation %I64u.\n", query_memory_info.CurrentReservation);

        winetest_pop_context();
    }

    /* Query using the current process handle */
    query_memory_info.hProcess = GetCurrentProcess();
    status = pD3DKMTQueryVideoMemoryInfo(&query_memory_info);
    todo_wine_if (status == STATUS_INVALID_PARAMETER)  /* fails on Wine without a Vulkan adapter */
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);

    /* Query using a process handle without PROCESS_QUERY_INFORMATION privilege */
    query_memory_info.hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, GetCurrentProcessId());
    ok(!!query_memory_info.hProcess, "OpenProcess failed, error %ld.\n", GetLastError());
    status = pD3DKMTQueryVideoMemoryInfo(&query_memory_info);
    ok(status == STATUS_ACCESS_DENIED, "Got unexpected return code %#lx.\n", status);
    CloseHandle(query_memory_info.hProcess);
    query_memory_info.hProcess = NULL;

    /* Query using an invalid process handle */
    query_memory_info.hProcess = (HANDLE)0xdeadbeef;
    status = pD3DKMTQueryVideoMemoryInfo(&query_memory_info);
    ok(status == STATUS_INVALID_HANDLE, "Got unexpected return code %#lx.\n", status);
    query_memory_info.hProcess = NULL;

    /* Query using an invalid adapter handle */
    query_memory_info.hAdapter = (D3DKMT_HANDLE)0xdeadbeef;
    status = pD3DKMTQueryVideoMemoryInfo(&query_memory_info);
    ok(status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status);
    query_memory_info.hAdapter = open_adapter_desc.hAdapter;

    /* Query using an invalid adapter index */
    query_memory_info.PhysicalAdapterIndex = 99;
    status = pD3DKMTQueryVideoMemoryInfo(&query_memory_info);
    ok(status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status);
    query_memory_info.PhysicalAdapterIndex = 0;

    /* Query using an invalid memory segment group */
    query_memory_info.MemorySegmentGroup = D3DKMT_MEMORY_SEGMENT_GROUP_NON_LOCAL + 1;
    status = pD3DKMTQueryVideoMemoryInfo(&query_memory_info);
    ok(status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status);

    close_adapter_desc.hAdapter = open_adapter_desc.hAdapter;
    status = pD3DKMTCloseAdapter(&close_adapter_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);
}

#ifndef __REACTOS__
static void test_gpu_device_properties_guid(const GUID *devinterface_guid)
{
    BYTE iface_detail_buffer[sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) + 256 * sizeof(WCHAR)];
    SP_DEVINFO_DATA device_data = {sizeof(device_data)};
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *iface_data;
    WCHAR device_id[256];
    DEVPROPTYPE type;
    unsigned int i;
    UINT32 value;
    HDEVINFO set;
    BOOL ret;

    /* Make sure display devices are initialized. */
    SendMessageW(GetDesktopWindow(), WM_NULL, 0, 0);

    set = SetupDiGetClassDevsW(devinterface_guid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    ok(set != INVALID_HANDLE_VALUE, "SetupDiGetClassDevs failed, error %lu.\n", GetLastError());

    iface_data = (SP_DEVICE_INTERFACE_DETAIL_DATA_W *)iface_detail_buffer;
    iface_data->cbSize = sizeof(*iface_data);

    i = 0;
    while (SetupDiEnumDeviceInterfaces(set, NULL, devinterface_guid, i, &iface))
    {
        ret = SetupDiGetDeviceInterfaceDetailW(set, &iface, iface_data,
                sizeof(iface_detail_buffer), NULL, &device_data );
        ok(ret, "Got unexpected ret %d, GetLastError() %lu.\n", ret, GetLastError());

        ret = SetupDiGetDevicePropertyW(set, &device_data, &DEVPKEY_Device_MatchingDeviceId, &type,
                (BYTE *)device_id, sizeof(device_id), NULL, 0);
        ok(ret, "Got unexpected ret %d, GetLastError() %lu.\n", ret, GetLastError());
        ok(type == DEVPROP_TYPE_STRING, "Got type %ld.\n", type);

        ret = SetupDiGetDevicePropertyW(set, &device_data, &DEVPKEY_Device_BusNumber, &type,
                (BYTE *)&value, sizeof(value), NULL, 0);
        if (!wcsicmp(device_id, L"root\\basicrender") || !wcsicmp(device_id, L"root\\basicdisplay"))
        {
            ok(!ret, "Found Bus Id.\n");
        }
        else
        {
            ok(ret, "Got unexpected ret %d, GetLastError() %lu, %s.\n", ret, GetLastError(), debugstr_w(device_id));
            ok(type == DEVPROP_TYPE_UINT32, "Got type %ld.\n", type);
        }

        ret = SetupDiGetDevicePropertyW(set, &device_data, &DEVPKEY_Device_RemovalPolicy, &type,
                (BYTE *)&value, sizeof(value), NULL, 0);
        ok(ret, "Got unexpected ret %d, GetLastError() %lu, %s.\n", ret, GetLastError(), debugstr_w(device_id));
        ok(value == CM_REMOVAL_POLICY_EXPECT_NO_REMOVAL || value == CM_REMOVAL_POLICY_EXPECT_ORDERLY_REMOVAL
                || value == CM_REMOVAL_POLICY_EXPECT_SURPRISE_REMOVAL, "Got value %d.\n", value);
        ok(type == DEVPROP_TYPE_UINT32, "Got type %ld.\n", type);
        ++i;
    }
    SetupDiDestroyDeviceInfoList(set);
}

static void test_gpu_device_properties(void)
{
    winetest_push_context("GUID_DEVINTERFACE_DISPLAY_ADAPTER");
    test_gpu_device_properties_guid(&GUID_DEVINTERFACE_DISPLAY_ADAPTER);
    winetest_pop_context();
    winetest_push_context("GUID_DISPLAY_DEVICE_ARRIVAL");
    test_gpu_device_properties_guid(&GUID_DISPLAY_DEVICE_ARRIVAL);
    winetest_pop_context();
}
#endif

START_TEST(driver)
{
    HMODULE gdi32 = GetModuleHandleA("gdi32.dll");
    HMODULE dwmapi = LoadLibraryA("dwmapi.dll");

    pD3DKMTCheckOcclusion = (void *)GetProcAddress(gdi32, "D3DKMTCheckOcclusion");
    pD3DKMTCheckVidPnExclusiveOwnership = (void *)GetProcAddress(gdi32, "D3DKMTCheckVidPnExclusiveOwnership");
    pD3DKMTCloseAdapter = (void *)GetProcAddress(gdi32, "D3DKMTCloseAdapter");
    pD3DKMTCreateDevice = (void *)GetProcAddress(gdi32, "D3DKMTCreateDevice");
    pD3DKMTDestroyDevice = (void *)GetProcAddress(gdi32, "D3DKMTDestroyDevice");
    pD3DKMTEnumAdapters2 = (void *)GetProcAddress(gdi32, "D3DKMTEnumAdapters2");
    pD3DKMTOpenAdapterFromDeviceName = (void *)GetProcAddress(gdi32, "D3DKMTOpenAdapterFromDeviceName");
    pD3DKMTOpenAdapterFromGdiDisplayName = (void *)GetProcAddress(gdi32, "D3DKMTOpenAdapterFromGdiDisplayName");
    pD3DKMTOpenAdapterFromHdc = (void *)GetProcAddress(gdi32, "D3DKMTOpenAdapterFromHdc");
    pD3DKMTSetVidPnSourceOwner = (void *)GetProcAddress(gdi32, "D3DKMTSetVidPnSourceOwner");
    pD3DKMTQueryVideoMemoryInfo = (void *)GetProcAddress(gdi32, "D3DKMTQueryVideoMemoryInfo");

    if (dwmapi)
        pDwmEnableComposition = (void *)GetProcAddress(dwmapi, "DwmEnableComposition");

    test_D3DKMTOpenAdapterFromGdiDisplayName();
    test_D3DKMTOpenAdapterFromHdc();
    test_D3DKMTEnumAdapters2();
    test_D3DKMTCloseAdapter();
    test_D3DKMTCreateDevice();
    test_D3DKMTDestroyDevice();
    test_D3DKMTCheckVidPnExclusiveOwnership();
    test_D3DKMTSetVidPnSourceOwner();
    test_D3DKMTCheckOcclusion();
    test_D3DKMTOpenAdapterFromDeviceName();
    test_D3DKMTQueryVideoMemoryInfo();
#ifndef __REACTOS__
    test_gpu_device_properties();
#endif

    FreeLibrary(dwmapi);
}
