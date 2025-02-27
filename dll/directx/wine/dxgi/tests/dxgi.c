/*
 * Copyright 2008 Henri Verbeet for CodeWeavers
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

#include <assert.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#define COBJMACROS
#include "initguid.h"
#include "dxgi1_6.h"
#include "d3d11.h"
#include "d3d12.h"
#include "d3d12sdklayers.h"
#include "winternl.h"
#include "ddk/d3dkmthk.h"
#include "wine/test.h"

enum frame_latency
{
    DEFAULT_FRAME_LATENCY =  3,
    MAX_FRAME_LATENCY     = 16,
};

static DEVMODEW registry_mode;

static HRESULT (WINAPI *pCreateDXGIFactory1)(REFIID iid, void **factory);
static HRESULT (WINAPI *pCreateDXGIFactory2)(UINT flags, REFIID iid, void **factory);

static NTSTATUS (WINAPI *pD3DKMTCheckVidPnExclusiveOwnership)(const D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP *desc);
static NTSTATUS (WINAPI *pD3DKMTCloseAdapter)(const D3DKMT_CLOSEADAPTER *desc);
static NTSTATUS (WINAPI *pD3DKMTOpenAdapterFromGdiDisplayName)(D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME *desc);

static HRESULT (WINAPI *pD3D11CreateDevice)(IDXGIAdapter *adapter, D3D_DRIVER_TYPE driver_type, HMODULE swrast, UINT flags,
        const D3D_FEATURE_LEVEL *feature_levels, UINT levels, UINT sdk_version, ID3D11Device **device_out,
        D3D_FEATURE_LEVEL *obtained_feature_level, ID3D11DeviceContext **immediate_context);

static PFN_D3D12_CREATE_DEVICE pD3D12CreateDevice;
static PFN_D3D12_GET_DEBUG_INTERFACE pD3D12GetDebugInterface;

static unsigned int use_adapter_idx;
static BOOL use_warp_adapter;
static BOOL use_mt = TRUE;

static struct test_entry
{
    void (*test)(void);
} *mt_tests;
size_t mt_tests_size, mt_test_count;

static void queue_test(void (*test)(void))
{
    if (mt_test_count >= mt_tests_size)
    {
        mt_tests_size = max(16, mt_tests_size * 2);
        mt_tests = realloc(mt_tests, mt_tests_size * sizeof(*mt_tests));
    }
    mt_tests[mt_test_count++].test = test;
}

static DWORD WINAPI thread_func(void *ctx)
{
    LONG *i = ctx, j;

    while (*i < mt_test_count)
    {
        j = *i;
        if (InterlockedCompareExchange(i, j + 1, j) == j)
            mt_tests[j].test();
    }

    return 0;
}

static void run_queued_tests(void)
{
    unsigned int thread_count, i;
    HANDLE *threads;
    SYSTEM_INFO si;
    LONG test_idx;

    if (!use_mt)
    {
        for (i = 0; i < mt_test_count; ++i)
        {
            mt_tests[i].test();
        }

        return;
    }

    GetSystemInfo(&si);
    thread_count = si.dwNumberOfProcessors;
    threads = calloc(thread_count, sizeof(*threads));
    for (i = 0, test_idx = 0; i < thread_count; ++i)
    {
        threads[i] = CreateThread(NULL, 0, thread_func, &test_idx, 0, NULL);
        ok(!!threads[i], "Failed to create thread %u.\n", i);
    }
    WaitForMultipleObjects(thread_count, threads, TRUE, INFINITE);
    for (i = 0; i < thread_count; ++i)
    {
        CloseHandle(threads[i]);
    }
    free(threads);
}

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

static void get_virtual_rect(RECT *rect)
{
    rect->left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    rect->top = GetSystemMetrics(SM_YVIRTUALSCREEN);
    rect->right = rect->left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
    rect->bottom = rect->top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
}

static BOOL equal_mode_rect(const DEVMODEW *mode1, const DEVMODEW *mode2)
{
    return mode1->dmPosition.x == mode2->dmPosition.x
            && mode1->dmPosition.y == mode2->dmPosition.y
            && mode1->dmPelsWidth == mode2->dmPelsWidth
            && mode1->dmPelsHeight == mode2->dmPelsHeight;
}

/* Free original_modes after finished using it */
static BOOL save_display_modes(DEVMODEW **original_modes, unsigned int *display_count)
{
    unsigned int number, size = 2, count = 0, index = 0;
    DISPLAY_DEVICEW display_device;
    DEVMODEW *modes, *tmp;

    if (!(modes = malloc(size * sizeof(*modes))))
        return FALSE;

    display_device.cb = sizeof(display_device);
    while (EnumDisplayDevicesW(NULL, index++, &display_device, 0))
    {
        /* Skip software devices */
        if (swscanf(display_device.DeviceName, L"\\\\.\\DISPLAY%u", &number) != 1)
            continue;

        if (!(display_device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
            continue;

        if (count >= size)
        {
            size *= 2;
            if (!(tmp = realloc(modes, size * sizeof(*modes))))
            {
                free(modes);
                return FALSE;
            }
            modes = tmp;
        }

        memset(&modes[count], 0, sizeof(modes[count]));
        modes[count].dmSize = sizeof(modes[count]);
        if (!EnumDisplaySettingsW(display_device.DeviceName, ENUM_CURRENT_SETTINGS, &modes[count]))
        {
            free(modes);
            return FALSE;
        }

        lstrcpyW(modes[count++].dmDeviceName, display_device.DeviceName);
    }

    *original_modes = modes;
    *display_count = count;
    return TRUE;
}

static BOOL restore_display_modes(DEVMODEW *modes, unsigned int count)
{
    unsigned int index;
    LONG ret;

    for (index = 0; index < count; ++index)
    {
        ret = ChangeDisplaySettingsExW(modes[index].dmDeviceName, &modes[index], NULL,
                CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
        if (ret != DISP_CHANGE_SUCCESSFUL)
            return FALSE;
    }
    ret = ChangeDisplaySettingsExW(NULL, NULL, NULL, 0, NULL);
    return ret == DISP_CHANGE_SUCCESSFUL;
}

/* try to make sure pending X events have been processed before continuing */
static void flush_events(void)
{
    int diff = 200;
    DWORD time;
    MSG msg;

    time = GetTickCount() + diff;
    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects(0, NULL, FALSE, 100, QS_ALLINPUT) == WAIT_TIMEOUT)
            break;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
            DispatchMessageA(&msg);
        diff = time - GetTickCount();
    }
}

#define check_interface(a, b, c, d) check_interface_(__LINE__, a, b, c, d)
static HRESULT check_interface_(unsigned int line, void *iface, REFIID iid,
        BOOL supported, BOOL is_broken)
{
    HRESULT hr, expected_hr, broken_hr;
    IUnknown *unknown = iface, *out;

    if (supported)
    {
        expected_hr = S_OK;
        broken_hr = E_NOINTERFACE;
    }
    else
    {
        expected_hr = E_NOINTERFACE;
        broken_hr = S_OK;
    }

    out = (IUnknown *)0xdeadbeef;
    hr = IUnknown_QueryInterface(unknown, iid, (void **)&out);
    ok_(__FILE__, line)(hr == expected_hr || broken(is_broken && hr == broken_hr),
            "Got unexpected hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(out);
    else
        ok_(__FILE__, line)(!out, "Got unexpected pointer %p.\n", out);
    return hr;
}

static BOOL is_flip_model(DXGI_SWAP_EFFECT swap_effect)
{
    return swap_effect == DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
            || swap_effect == DXGI_SWAP_EFFECT_FLIP_DISCARD;
}

static unsigned int check_multisample_quality_levels(IDXGIDevice *dxgi_device,
        DXGI_FORMAT format, unsigned int sample_count)
{
    ID3D10Device *device;
    unsigned int levels;
    HRESULT hr;

    hr = IDXGIDevice_QueryInterface(dxgi_device, &IID_ID3D10Device, (void **)&device);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3D10Device_CheckMultisampleQualityLevels(device, format, sample_count, &levels);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D10Device_Release(device);

    return levels;
}

#define MODE_DESC_IGNORE_RESOLUTION        0x00000001u
#define MODE_DESC_IGNORE_REFRESH_RATE      0x00000002u
#define MODE_DESC_IGNORE_FORMAT            0x00000004u
#define MODE_DESC_IGNORE_SCANLINE_ORDERING 0x00000008u
#define MODE_DESC_IGNORE_SCALING           0x00000010u
#define MODE_DESC_IGNORE_EXACT_RESOLUTION  0x00000020u

#define MODE_DESC_CHECK_RESOLUTION         (~MODE_DESC_IGNORE_RESOLUTION & ~MODE_DESC_IGNORE_EXACT_RESOLUTION)
#define MODE_DESC_CHECK_FORMAT             (~MODE_DESC_IGNORE_FORMAT)

#define check_mode_desc(a, b, c) check_mode_desc_(__LINE__, a, b, c)
static void check_mode_desc_(unsigned int line, const DXGI_MODE_DESC *desc,
        const DXGI_MODE_DESC *expected_desc, unsigned int ignore_flags)
{
    if (!(ignore_flags & MODE_DESC_IGNORE_RESOLUTION))
    {
        if (ignore_flags & MODE_DESC_IGNORE_EXACT_RESOLUTION)
            ok_(__FILE__, line)(desc->Width * desc->Height ==
                    expected_desc->Width * expected_desc->Height,
                    "Got resolution %ux%u, expected %ux%u.\n",
                    desc->Width, desc->Height, expected_desc->Width, expected_desc->Height);
        else
            ok_(__FILE__, line)(desc->Width == expected_desc->Width &&
                    desc->Height == expected_desc->Height,
                    "Got resolution %ux%u, expected %ux%u.\n",
                    desc->Width, desc->Height, expected_desc->Width, expected_desc->Height);
    }
    if (!(ignore_flags & MODE_DESC_IGNORE_REFRESH_RATE))
    {
        ok_(__FILE__, line)(desc->RefreshRate.Numerator == expected_desc->RefreshRate.Numerator
                && desc->RefreshRate.Denominator == expected_desc->RefreshRate.Denominator,
                "Got refresh rate %u / %u, expected %u / %u.\n",
                desc->RefreshRate.Numerator, desc->RefreshRate.Denominator,
                expected_desc->RefreshRate.Denominator, expected_desc->RefreshRate.Denominator);
    }
    if (!(ignore_flags & MODE_DESC_IGNORE_FORMAT))
    {
        ok_(__FILE__, line)(desc->Format == expected_desc->Format,
                "Got format %#x, expected %#x.\n", desc->Format, expected_desc->Format);
    }
    if (!(ignore_flags & MODE_DESC_IGNORE_SCANLINE_ORDERING))
    {
        ok_(__FILE__, line)(desc->ScanlineOrdering == expected_desc->ScanlineOrdering,
                "Got scanline ordering %#x, expected %#x.\n",
                desc->ScanlineOrdering, expected_desc->ScanlineOrdering);
    }
    if (!(ignore_flags & MODE_DESC_IGNORE_SCALING))
    {
        ok_(__FILE__, line)(desc->Scaling == expected_desc->Scaling,
                "Got scaling %#x, expected %#x.\n",
                desc->Scaling, expected_desc->Scaling);
    }
}

static BOOL equal_luid(LUID a, LUID b)
{
    return a.LowPart == b.LowPart && a.HighPart == b.HighPart;
}

#define check_adapter_desc(a, b) check_adapter_desc_(__LINE__, a, b)
static void check_adapter_desc_(unsigned int line, const DXGI_ADAPTER_DESC *desc,
        const struct DXGI_ADAPTER_DESC *expected_desc)
{
    ok_(__FILE__, line)(!lstrcmpW(desc->Description, expected_desc->Description),
            "Got description %s, expected %s.\n",
            wine_dbgstr_w(desc->Description), wine_dbgstr_w(expected_desc->Description));
    ok_(__FILE__, line)(desc->VendorId == expected_desc->VendorId,
            "Got vendor id %04x, expected %04x.\n",
            desc->VendorId, expected_desc->VendorId);
    ok_(__FILE__, line)(desc->DeviceId == expected_desc->DeviceId,
            "Got device id %04x, expected %04x.\n",
            desc->DeviceId, expected_desc->DeviceId);
    ok_(__FILE__, line)(desc->SubSysId == expected_desc->SubSysId,
            "Got subsys id %04x, expected %04x.\n",
            desc->SubSysId, expected_desc->SubSysId);
    ok_(__FILE__, line)(desc->Revision == expected_desc->Revision,
            "Got revision %02x, expected %02x.\n",
            desc->Revision, expected_desc->Revision);
    ok_(__FILE__, line)(desc->DedicatedVideoMemory == expected_desc->DedicatedVideoMemory,
            "Got dedicated video memory %Iu, expected %Iu.\n",
            desc->DedicatedVideoMemory, expected_desc->DedicatedVideoMemory);
    ok_(__FILE__, line)(desc->DedicatedSystemMemory == expected_desc->DedicatedSystemMemory,
            "Got dedicated system memory %Iu, expected %Iu.\n",
            desc->DedicatedSystemMemory, expected_desc->DedicatedSystemMemory);
    ok_(__FILE__, line)(desc->SharedSystemMemory == expected_desc->SharedSystemMemory,
            "Got shared system memory %Iu, expected %Iu.\n",
            desc->SharedSystemMemory, expected_desc->SharedSystemMemory);
    ok_(__FILE__, line)(equal_luid(desc->AdapterLuid, expected_desc->AdapterLuid),
            "Got LUID %08lx:%08lx, expected %08lx:%08lx.\n",
            desc->AdapterLuid.HighPart, desc->AdapterLuid.LowPart,
            expected_desc->AdapterLuid.HighPart, expected_desc->AdapterLuid.LowPart);
}

#define check_output_desc(a, b) check_output_desc_(__LINE__, a, b)
static void check_output_desc_(unsigned int line, const DXGI_OUTPUT_DESC *desc,
        const struct DXGI_OUTPUT_DESC *expected_desc)
{
    ok_(__FILE__, line)(!lstrcmpW(desc->DeviceName, expected_desc->DeviceName),
            "Got unexpected device name %s, expected %s.\n",
            wine_dbgstr_w(desc->DeviceName), wine_dbgstr_w(expected_desc->DeviceName));
    ok_(__FILE__, line)(EqualRect(&desc->DesktopCoordinates, &expected_desc->DesktopCoordinates),
            "Got unexpected desktop coordinates %s, expected %s.\n",
            wine_dbgstr_rect(&desc->DesktopCoordinates),
            wine_dbgstr_rect(&expected_desc->DesktopCoordinates));
}

#define check_output_equal(a, b) check_output_equal_(__LINE__, a, b)
static void check_output_equal_(unsigned int line, IDXGIOutput *output1, IDXGIOutput *output2)
{
    DXGI_OUTPUT_DESC desc1, desc2;
    HRESULT hr;

    hr = IDXGIOutput_GetDesc(output1, &desc1);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGIOutput_GetDesc(output2, &desc2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_output_desc_(line, &desc1, &desc2);
}

static BOOL output_belongs_to_adapter(IDXGIOutput *output, IDXGIAdapter *adapter)
{
    DXGI_OUTPUT_DESC output_desc, desc;
    unsigned int output_idx;
    IDXGIOutput *o;
    HRESULT hr;

    hr = IDXGIOutput_GetDesc(output, &output_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    for (output_idx = 0; IDXGIAdapter_EnumOutputs(adapter, output_idx, &o) != DXGI_ERROR_NOT_FOUND; ++output_idx)
    {
        hr = IDXGIOutput_GetDesc(o, &desc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        IDXGIOutput_Release(o);

        if (!lstrcmpW(desc.DeviceName, output_desc.DeviceName)
                && EqualRect(&desc.DesktopCoordinates, &output_desc.DesktopCoordinates))
            return TRUE;
    }

    return FALSE;
}

struct fullscreen_state
{
    DWORD style;
    DWORD exstyle;
    RECT window_rect;
    RECT client_rect;
    HMONITOR monitor;
    RECT monitor_rect;
};

struct swapchain_fullscreen_state
{
    struct fullscreen_state fullscreen_state;
    BOOL fullscreen;
    IDXGIOutput *target;
};

#define capture_fullscreen_state(a, b) capture_fullscreen_state_(__LINE__, a, b)
static void capture_fullscreen_state_(unsigned int line, struct fullscreen_state *state, HWND window)
{
    MONITORINFOEXW monitor_info;
    BOOL ret;

    state->style = GetWindowLongA(window, GWL_STYLE);
    state->exstyle = GetWindowLongA(window, GWL_EXSTYLE);

    ret = GetWindowRect(window, &state->window_rect);
    ok_(__FILE__, line)(ret, "GetWindowRect failed.\n");
    ret = GetClientRect(window, &state->client_rect);
    ok_(__FILE__, line)(ret, "GetClientRect failed.\n");

    state->monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONULL);
    ok_(__FILE__, line)(!!state->monitor, "Failed to get monitor from window.\n");

    monitor_info.cbSize = sizeof(monitor_info);
    ret = GetMonitorInfoW(state->monitor, (MONITORINFO *)&monitor_info);
    ok_(__FILE__, line)(ret, "Failed to get monitor info.\n");
    state->monitor_rect = monitor_info.rcMonitor;
}

static void check_fullscreen_state_(unsigned int line, const struct fullscreen_state *state,
        const struct fullscreen_state *expected_state, BOOL windowed)
{
    todo_wine_if(!windowed)
    ok_(__FILE__, line)((state->style & ~WS_VISIBLE) == (expected_state->style & ~WS_VISIBLE),
            "Got style %#lx, expected %#lx.\n",
            state->style & ~(DWORD)WS_VISIBLE, expected_state->style & ~(DWORD)WS_VISIBLE);
    ok_(__FILE__, line)((state->exstyle & ~WS_EX_TOPMOST) == (expected_state->exstyle & ~WS_EX_TOPMOST),
            "Got exstyle %#lx, expected %#lx.\n",
            state->exstyle & ~(DWORD)WS_EX_TOPMOST, expected_state->exstyle & ~(DWORD)WS_EX_TOPMOST);
    ok_(__FILE__, line)(EqualRect(&state->window_rect, &expected_state->window_rect),
            "Got window rect %s, expected %s.\n",
            wine_dbgstr_rect(&state->window_rect), wine_dbgstr_rect(&expected_state->window_rect));
    ok_(__FILE__, line)(EqualRect(&state->client_rect, &expected_state->client_rect),
            "Got client rect %s, expected %s.\n",
            wine_dbgstr_rect(&state->client_rect), wine_dbgstr_rect(&expected_state->client_rect));
    ok_(__FILE__, line)(state->monitor == expected_state->monitor,
            "Got monitor %p, expected %p.\n",
            state->monitor, expected_state->monitor);
    ok_(__FILE__, line)(EqualRect(&state->monitor_rect, &expected_state->monitor_rect),
            "Got monitor rect %s, expected %s.\n",
            wine_dbgstr_rect(&state->monitor_rect), wine_dbgstr_rect(&expected_state->monitor_rect));
}

#define check_window_fullscreen_state(a, b) check_window_fullscreen_state_(__LINE__, a, b, TRUE)
static void check_window_fullscreen_state_(unsigned int line, HWND window,
        const struct fullscreen_state *expected_state, BOOL windowed)
{
    struct fullscreen_state current_state;
    capture_fullscreen_state_(line, &current_state, window);
    check_fullscreen_state_(line, &current_state, expected_state, windowed);
}

#define check_swapchain_fullscreen_state(a, b) check_swapchain_fullscreen_state_(__LINE__, a, b)
static void check_swapchain_fullscreen_state_(unsigned int line, IDXGISwapChain *swapchain,
        const struct swapchain_fullscreen_state *expected_state)
{
    IDXGIOutput *containing_output, *target;
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    BOOL fullscreen;
    HRESULT hr;

    hr = IDXGISwapChain_GetDesc(swapchain, &swapchain_desc);
    ok_(__FILE__, line)(hr == S_OK, "Failed to get swapchain desc, hr %#lx.\n", hr);
    check_window_fullscreen_state_(line, swapchain_desc.OutputWindow,
            &expected_state->fullscreen_state, swapchain_desc.Windowed);

    ok_(__FILE__, line)(swapchain_desc.Windowed == !expected_state->fullscreen,
            "Got windowed %#x, expected %#x.\n",
            swapchain_desc.Windowed, !expected_state->fullscreen);

    hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, &target);
    ok_(__FILE__, line)(hr == S_OK, "Failed to get fullscreen state, hr %#lx.\n", hr);
    ok_(__FILE__, line)(fullscreen == expected_state->fullscreen, "Got fullscreen %#x, expected %#x.\n",
            fullscreen, expected_state->fullscreen);

    if (!swapchain_desc.Windowed && expected_state->fullscreen)
    {
        IDXGIAdapter *adapter;

        hr = IDXGISwapChain_GetContainingOutput(swapchain, &containing_output);
        ok_(__FILE__, line)(hr == S_OK, "Failed to get containing output, hr %#lx.\n", hr);

        hr = IDXGIOutput_GetParent(containing_output, &IID_IDXGIAdapter, (void **)&adapter);
        ok_(__FILE__, line)(hr == S_OK, "Failed to get parent, hr %#lx.\n", hr);

        check_output_equal_(line, target, expected_state->target);
        ok_(__FILE__, line)(target == containing_output, "Got target %p, expected %p.\n",
                target, containing_output);
        ok_(__FILE__, line)(output_belongs_to_adapter(target, adapter),
                "Output %p doesn't belong to adapter %p.\n",
                target, adapter);

        IDXGIOutput_Release(target);
        IDXGIOutput_Release(containing_output);
        IDXGIAdapter_Release(adapter);
    }
    else
    {
        ok_(__FILE__, line)(!target, "Got unexpected target %p.\n", target);
    }
}

#define compute_expected_swapchain_fullscreen_state_after_fullscreen_change(a, b, c, d, e, f) \
        compute_expected_swapchain_fullscreen_state_after_fullscreen_change_(__LINE__, a, b, c, d, e, f)
static void compute_expected_swapchain_fullscreen_state_after_fullscreen_change_(unsigned int line,
        struct swapchain_fullscreen_state *state, const DXGI_SWAP_CHAIN_DESC *swapchain_desc,
        const RECT *old_monitor_rect, unsigned int new_width, unsigned int new_height, IDXGIOutput *target)
{
    if (!new_width && !new_height)
    {
        RECT client_rect;
        GetClientRect(swapchain_desc->OutputWindow, &client_rect);
        new_width = client_rect.right - client_rect.left;
        new_height = client_rect.bottom - client_rect.top;
    }

    if (target)
    {
        DXGI_MODE_DESC mode_desc = swapchain_desc->BufferDesc;
        HRESULT hr;

        mode_desc.Width = new_width;
        mode_desc.Height = new_height;
        hr = IDXGIOutput_FindClosestMatchingMode(target, &mode_desc, &mode_desc, NULL);
        ok_(__FILE__, line)(SUCCEEDED(hr), "FindClosestMatchingMode failed, hr %#lx.\n", hr);
        new_width = mode_desc.Width;
        new_height = mode_desc.Height;
    }

    state->fullscreen_state.style &= WS_VISIBLE | WS_CLIPSIBLINGS;
    state->fullscreen_state.exstyle &= WS_EX_TOPMOST;

    state->fullscreen = TRUE;
    if (swapchain_desc->Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH)
    {
        unsigned int new_x = (old_monitor_rect->left >= 0)
                ? old_monitor_rect->left : old_monitor_rect->right - new_width;
        unsigned new_y = (old_monitor_rect->top >= 0)
                ? old_monitor_rect->top : old_monitor_rect->bottom - new_height;
        RECT new_monitor_rect = {0, 0, new_width, new_height};
        OffsetRect(&new_monitor_rect, new_x, new_y);

        SetRect(&state->fullscreen_state.client_rect, 0, 0, new_width, new_height);
        state->fullscreen_state.monitor_rect = new_monitor_rect;
        state->fullscreen_state.window_rect = new_monitor_rect;

        if (target)
            state->target = target;
    }
    else
    {
        state->fullscreen_state.window_rect = *old_monitor_rect;
        SetRect(&state->fullscreen_state.client_rect, 0, 0,
                old_monitor_rect->right - old_monitor_rect->left,
                old_monitor_rect->bottom - old_monitor_rect->top);
    }
}

#define wait_fullscreen_state(a, b, c) wait_fullscreen_state_(__LINE__, a, b, c)
static void wait_fullscreen_state_(unsigned int line, IDXGISwapChain *swapchain, BOOL expected, BOOL todo)
{
    static const unsigned int wait_timeout = 2000;
    static const unsigned int wait_step = 100;
    unsigned int total_time = 0;
    HRESULT hr;
    BOOL state;

    while (total_time < wait_timeout)
    {
        state = !expected;
        if (FAILED(hr = IDXGISwapChain_GetFullscreenState(swapchain, &state, NULL)))
            break;
        if (state == expected)
            break;
        Sleep(wait_step);
        total_time += wait_step;
    }
    ok_(__FILE__, line)(hr == S_OK, "Failed to get fullscreen state, hr %#lx.\n", hr);
    todo_wine_if(todo) ok_(__FILE__, line)(state == expected,
            "Got unexpected state %#x, expected %#x.\n", state, expected);
}

/* VidPN exclusive ownership doesn't change immediately.
 * This helper is used to wait for the expected status */
#define get_expected_vidpn_exclusive_ownership(a, b) \
        get_expected_vidpn_exclusive_ownership_(__LINE__, a, b)
static NTSTATUS get_expected_vidpn_exclusive_ownership_(unsigned int line,
        const D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP *desc, NTSTATUS expected)
{
    static const unsigned int wait_timeout = 2000;
    static const unsigned int wait_step = 100;
    unsigned int total_time = 0;
    NTSTATUS status;

    while (total_time < wait_timeout)
    {
        status = pD3DKMTCheckVidPnExclusiveOwnership(desc);
        if (status == expected)
            break;
        Sleep(wait_step);
        total_time += wait_step;
    }
    return status;
}

static HWND create_window(void)
{
    RECT r = {0, 0, 640, 480};

    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW | WS_VISIBLE, FALSE);

    return CreateWindowA("static", "dxgi_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, r.right - r.left, r.bottom - r.top, NULL, NULL, NULL, NULL);
}

static IDXGIAdapter *create_adapter(void)
{
    IDXGIFactory4 *factory4;
    IDXGIFactory *factory;
    IDXGIAdapter *adapter;
    HRESULT hr;

    if (!use_warp_adapter && !use_adapter_idx)
        return NULL;

    if (FAILED(hr = CreateDXGIFactory(&IID_IDXGIFactory, (void **)&factory)))
    {
        trace("Failed to create IDXGIFactory, hr %#lx.\n", hr);
        return NULL;
    }

    adapter = NULL;
    if (use_warp_adapter)
    {
        if (SUCCEEDED(hr = IDXGIFactory_QueryInterface(factory, &IID_IDXGIFactory4, (void **)&factory4)))
        {
            hr = IDXGIFactory4_EnumWarpAdapter(factory4, &IID_IDXGIAdapter, (void **)&adapter);
            IDXGIFactory4_Release(factory4);
        }
        else
        {
            trace("Failed to get IDXGIFactory4, hr %#lx.\n", hr);
        }
    }
    else
    {
        hr = IDXGIFactory_EnumAdapters(factory, use_adapter_idx, &adapter);
    }
    IDXGIFactory_Release(factory);
    if (FAILED(hr))
        trace("Failed to get adapter, hr %#lx.\n", hr);
    return adapter;
}

static IDXGIDevice *create_device(unsigned int flags)
{
    IDXGIDevice *dxgi_device;
    ID3D10Device1 *device;
    IDXGIAdapter *adapter;
    HRESULT hr;

    adapter = create_adapter();
    hr = D3D10CreateDevice1(adapter, D3D10_DRIVER_TYPE_HARDWARE, NULL,
            flags, D3D10_FEATURE_LEVEL_10_0, D3D10_1_SDK_VERSION, &device);
    if (adapter)
        IDXGIAdapter_Release(adapter);
    if (SUCCEEDED(hr))
        goto success;

    if (SUCCEEDED(D3D10CreateDevice1(NULL, D3D10_DRIVER_TYPE_WARP, NULL,
            flags, D3D10_FEATURE_LEVEL_10_0, D3D10_1_SDK_VERSION, &device)))
        goto success;
    if (SUCCEEDED(D3D10CreateDevice1(NULL, D3D10_DRIVER_TYPE_REFERENCE, NULL,
            flags, D3D10_FEATURE_LEVEL_10_0, D3D10_1_SDK_VERSION, &device)))
        goto success;

    return NULL;

success:
    hr = ID3D10Device1_QueryInterface(device, &IID_IDXGIDevice, (void **)&dxgi_device);
    ok(SUCCEEDED(hr), "Created device does not implement IDXGIDevice\n");
    ID3D10Device1_Release(device);

    return dxgi_device;
}

static IDXGIDevice *create_d3d11_device(void)
{
    static const D3D_FEATURE_LEVEL feature_level[] =
    {
        D3D_FEATURE_LEVEL_11_0,
    };
    unsigned int feature_level_count = ARRAY_SIZE(feature_level);
    IDXGIDevice *device = NULL;
    ID3D11Device *d3d_device;
    HRESULT hr;

    if (!pD3D11CreateDevice)
        return NULL;

    hr = pD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, feature_level, feature_level_count,
            D3D11_SDK_VERSION, &d3d_device, NULL, NULL);
    if (FAILED(hr))
        hr = pD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_WARP, NULL, 0, feature_level, feature_level_count,
                D3D11_SDK_VERSION, &d3d_device, NULL, NULL);
    if (FAILED(hr))
        hr = pD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_REFERENCE, NULL, 0, feature_level, feature_level_count,
                D3D11_SDK_VERSION, &d3d_device, NULL, NULL);

    if (SUCCEEDED(hr))
    {
        hr = ID3D11Device_QueryInterface(d3d_device, &IID_IDXGIDevice, (void **)&device);
        ok(SUCCEEDED(hr), "Created device does not implement IDXGIDevice.\n");
        ID3D11Device_Release(d3d_device);
    }

    return device;
}

static ID3D12Device *create_d3d12_device(void)
{
    IDXGIAdapter *adapter;
    ID3D12Device *device;
    HRESULT hr;

    if (!pD3D12CreateDevice)
        return NULL;

    adapter = create_adapter();
    hr = pD3D12CreateDevice((IUnknown *)adapter, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device, (void **)&device);
    if (adapter)
        IDXGIAdapter_Release(adapter);
    if (FAILED(hr))
        return NULL;

    return device;
}

static ID3D12CommandQueue *create_d3d12_direct_queue(ID3D12Device *device)
{
    D3D12_COMMAND_QUEUE_DESC command_queue_desc;
    ID3D12CommandQueue *queue;
    HRESULT hr;

    command_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    command_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    command_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    command_queue_desc.NodeMask = 0;
    hr = ID3D12Device_CreateCommandQueue(device, &command_queue_desc,
            &IID_ID3D12CommandQueue, (void **)&queue);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    return queue;
}

static HRESULT wait_for_fence(ID3D12Fence *fence, UINT64 value)
{
    HANDLE event;
    HRESULT hr;
    DWORD ret;

    if (ID3D12Fence_GetCompletedValue(fence) >= value)
        return S_OK;

    if (!(event = CreateEventA(NULL, FALSE, FALSE, NULL)))
        return E_FAIL;

    if (FAILED(hr = ID3D12Fence_SetEventOnCompletion(fence, value, event)))
    {
        CloseHandle(event);
        return hr;
    }

    ret = WaitForSingleObject(event, INFINITE);
    CloseHandle(event);
    return ret == WAIT_OBJECT_0 ? S_OK : E_FAIL;
}

#define wait_queue_idle(a, b) wait_queue_idle_(__LINE__, a, b)
static void wait_queue_idle_(unsigned int line, ID3D12Device *device, ID3D12CommandQueue *queue)
{
    ID3D12Fence *fence;
    HRESULT hr;

    hr = ID3D12Device_CreateFence(device, 0, D3D12_FENCE_FLAG_NONE,
            &IID_ID3D12Fence, (void **)&fence);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create fence, hr %#lx.\n", hr);

    hr = ID3D12CommandQueue_Signal(queue, fence, 1);
    ok_(__FILE__, line)(hr == S_OK, "Failed to signal fence, hr %#lx.\n", hr);
    hr = wait_for_fence(fence, 1);
    ok_(__FILE__, line)(hr == S_OK, "Failed to wait for fence, hr %#lx.\n", hr);

    ID3D12Fence_Release(fence);
}

#define wait_device_idle(a) wait_device_idle_(__LINE__, a)
static void wait_device_idle_(unsigned int line, IUnknown *device)
{
    ID3D12Device *d3d12_device;
    ID3D12CommandQueue *queue;
    HRESULT hr;

    hr = IUnknown_QueryInterface(device, &IID_ID3D12CommandQueue, (void **)&queue);
    if (hr != S_OK)
        return;

    hr = ID3D12CommandQueue_GetDevice(queue, &IID_ID3D12Device, (void **)&d3d12_device);
    ok_(__FILE__, line)(hr == S_OK, "Failed to get d3d12 device, hr %#lx.\n", hr);

    wait_queue_idle_(line, d3d12_device, queue);

    ID3D12CommandQueue_Release(queue);
    ID3D12Device_Release(d3d12_device);
}

#define get_factory(a, b, c) get_factory_(__LINE__, a, b, c)
static void get_factory_(unsigned int line, IUnknown *device, BOOL is_d3d12, IDXGIFactory **factory)
{
    IDXGIDevice *dxgi_device;
    IDXGIAdapter *adapter;
    HRESULT hr;

    if (is_d3d12)
    {
        hr = CreateDXGIFactory(&IID_IDXGIFactory, (void **)factory);
        ok_(__FILE__, line)(hr == S_OK, "Failed to create factory, hr %#lx.\n", hr);
    }
    else
    {
        dxgi_device = (IDXGIDevice *)device;
        hr = IDXGIDevice_GetAdapter(dxgi_device, &adapter);
        ok_(__FILE__, line)(hr == S_OK, "Failed to get adapter, hr %#lx.\n", hr);
        hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)factory);
        ok_(__FILE__, line)(hr == S_OK, "Failed to get parent, hr %#lx.\n", hr);
        IDXGIAdapter_Release(adapter);
    }
}

#define get_adapter(a, b) get_adapter_(__LINE__, a, b)
static IDXGIAdapter *get_adapter_(unsigned int line, IUnknown *device, BOOL is_d3d12)
{
    IDXGIAdapter *adapter = NULL;
    ID3D12Device *d3d12_device;
    IDXGIFactory4 *factory4;
    IDXGIFactory *factory;
    HRESULT hr;
    LUID luid;

    if (is_d3d12)
    {
        get_factory_(line, device, is_d3d12, &factory);
        hr = ID3D12CommandQueue_GetDevice((ID3D12CommandQueue *)device, &IID_ID3D12Device, (void **)&d3d12_device);
        ok_(__FILE__, line)(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        luid = ID3D12Device_GetAdapterLuid(d3d12_device);
        ID3D12Device_Release(d3d12_device);
        hr = IDXGIFactory_QueryInterface(factory, &IID_IDXGIFactory4, (void **)&factory4);
        ok_(__FILE__, line)(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = IDXGIFactory4_EnumAdapterByLuid(factory4, luid, &IID_IDXGIAdapter, (void **)&adapter);
        IDXGIFactory4_Release(factory4);
        IDXGIFactory_Release(factory);
    }
    else
    {
        hr = IDXGIDevice_GetAdapter((IDXGIDevice *)device, &adapter);
        ok_(__FILE__, line)(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    }

    return adapter;
}

#define create_swapchain(a, b, c, d) create_swapchain_(__LINE__, a, b, c, d)
static IDXGISwapChain *create_swapchain_(unsigned int line, IUnknown *device, BOOL is_d3d12, HWND window, UINT flags)
{
    DXGI_SWAP_CHAIN_DESC desc;
    IDXGISwapChain *swapchain;
    IDXGIFactory *factory;
    HRESULT hr;

    desc.BufferDesc.Width = 640;
    desc.BufferDesc.Height = 480;
    desc.BufferDesc.RefreshRate.Numerator = 60;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = is_d3d12 ? 2 : 1;
    desc.OutputWindow = window;
    desc.Windowed = TRUE;
    desc.SwapEffect = is_d3d12 ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD;
    desc.Flags = flags;

    get_factory(device, is_d3d12, &factory);
    hr = IDXGIFactory_CreateSwapChain(factory, device, &desc, &swapchain);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create swapchain, hr %#lx.\n", hr);
    IDXGIFactory_Release(factory);

    return swapchain;
}

static void test_adapter_desc(void)
{
    DXGI_ADAPTER_DESC1 desc1;
    IDXGIAdapter1 *adapter1;
    DXGI_ADAPTER_DESC desc;
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIAdapter_GetDesc(adapter, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIAdapter_GetDesc(adapter, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    trace("%s.\n", wine_dbgstr_w(desc.Description));
    trace("%04x: %04x:%04x (rev %02x).\n",
            desc.SubSysId, desc.VendorId, desc.DeviceId, desc.Revision);
    trace("Dedicated video memory: %Iu (%Iu MB).\n",
            desc.DedicatedVideoMemory, desc.DedicatedVideoMemory / (1024 * 1024));
    trace("Dedicated system memory: %Iu (%Iu MB).\n",
            desc.DedicatedSystemMemory, desc.DedicatedSystemMemory / (1024 * 1024));
    trace("Shared system memory: %Iu (%Iu MB).\n",
            desc.SharedSystemMemory, desc.SharedSystemMemory / (1024 * 1024));
    trace("LUID: %08lx:%08lx.\n", desc.AdapterLuid.HighPart, desc.AdapterLuid.LowPart);

    hr = IDXGIAdapter_QueryInterface(adapter, &IID_IDXGIAdapter1, (void **)&adapter1);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE), "Got unexpected hr %#lx.\n", hr);
    if (hr == E_NOINTERFACE)
        goto done;

    hr = IDXGIAdapter1_GetDesc1(adapter1, &desc1);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(!lstrcmpW(desc.Description, desc1.Description),
            "Got unexpected description %s.\n", wine_dbgstr_w(desc1.Description));
    ok(desc1.VendorId == desc.VendorId, "Got unexpected vendor ID %04x.\n", desc1.VendorId);
    ok(desc1.DeviceId == desc.DeviceId, "Got unexpected device ID %04x.\n", desc1.DeviceId);
    ok(desc1.SubSysId == desc.SubSysId, "Got unexpected sub system ID %04x.\n", desc1.SubSysId);
    ok(desc1.Revision == desc.Revision, "Got unexpected revision %02x.\n", desc1.Revision);
    ok(desc1.DedicatedVideoMemory == desc.DedicatedVideoMemory,
            "Got unexpected dedicated video memory %Iu.\n", desc1.DedicatedVideoMemory);
    ok(desc1.DedicatedSystemMemory == desc.DedicatedSystemMemory,
            "Got unexpected dedicated system memory %Iu.\n", desc1.DedicatedSystemMemory);
    ok(desc1.SharedSystemMemory == desc.SharedSystemMemory,
            "Got unexpected shared system memory %Iu.\n", desc1.SharedSystemMemory);
    ok(equal_luid(desc1.AdapterLuid, desc.AdapterLuid),
            "Got unexpected adapter LUID %08lx:%08lx.\n", desc1.AdapterLuid.HighPart, desc1.AdapterLuid.LowPart);
    trace("Flags: %08x.\n", desc1.Flags);

    IDXGIAdapter1_Release(adapter1);

done:
    IDXGIAdapter_Release(adapter);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

static void test_adapter_luid(void)
{
    DXGI_ADAPTER_DESC device_adapter_desc, desc, desc2;
    static const LUID luid = {0xdeadbeef, 0xdeadbeef};
    IDXGIAdapter *adapter, *adapter2;
    unsigned int found_adapter_count;
    unsigned int adapter_index;
    BOOL is_null_luid_adapter;
    IDXGIFactory4 *factory4;
    IDXGIFactory *factory;
    BOOL have_unique_luid;
    IDXGIDevice *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGIAdapter_GetDesc(adapter, &device_adapter_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IDXGIAdapter_Release(adapter);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);

    is_null_luid_adapter = !device_adapter_desc.AdapterLuid.LowPart
            && !device_adapter_desc.SubSysId && !device_adapter_desc.Revision
            && !device_adapter_desc.VendorId && !device_adapter_desc.DeviceId;

    hr = CreateDXGIFactory(&IID_IDXGIFactory, (void **)&factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIFactory_QueryInterface(factory, &IID_IDXGIFactory4, (void **)&factory4);
    ok(hr == S_OK || hr == E_NOINTERFACE, "Got unexpected hr %#lx.\n", hr);

    have_unique_luid = TRUE;
    found_adapter_count = 0;
    adapter_index = 0;
    while ((hr = IDXGIFactory_EnumAdapters(factory, adapter_index, &adapter)) == S_OK)
    {
        hr = IDXGIAdapter_GetDesc(adapter, &desc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        if (equal_luid(desc.AdapterLuid, device_adapter_desc.AdapterLuid))
        {
            check_adapter_desc(&desc, &device_adapter_desc);
            ++found_adapter_count;
        }

        if (equal_luid(desc.AdapterLuid, luid))
            have_unique_luid = FALSE;

        if (factory4)
        {
            hr = IDXGIFactory4_EnumAdapterByLuid(factory4, desc.AdapterLuid,
                    &IID_IDXGIAdapter, (void **)&adapter2);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
            hr = IDXGIAdapter_GetDesc(adapter2, &desc2);
            ok(hr == S_OK, "Failed to get adapter desc, hr %#lx.\n", hr);
            check_adapter_desc(&desc2, &desc);
            ok(adapter2 != adapter, "Expected to get new instance of IDXGIAdapter.\n");
            refcount = IDXGIAdapter_Release(adapter2);
            ok(!refcount, "Adapter has %lu references left.\n", refcount);
        }

        refcount = IDXGIAdapter_Release(adapter);
        ok(!refcount, "Adapter has %lu references left.\n", refcount);

        ++adapter_index;
    }
    ok(hr == DXGI_ERROR_NOT_FOUND, "Got unexpected hr %#lx.\n", hr);

    /* Older versions of WARP aren't enumerated by IDXGIFactory_EnumAdapters(). */
    ok(found_adapter_count == 1 || broken(is_null_luid_adapter),
            "Found %u adapters for LUID %08lx:%08lx.\n",
            found_adapter_count, device_adapter_desc.AdapterLuid.HighPart,
            device_adapter_desc.AdapterLuid.LowPart);

    if (factory4)
        IDXGIFactory4_Release(factory4);
    refcount = IDXGIFactory_Release(factory);
    ok(!refcount, "Factory has %lu references left.\n", refcount);

    if (!pCreateDXGIFactory2
            || FAILED(hr = pCreateDXGIFactory2(0, &IID_IDXGIFactory4, (void **)&factory4)))
    {
        skip("DXGI 1.4 is not available.\n");
        return;
    }

    hr = IDXGIFactory4_EnumAdapterByLuid(factory4, device_adapter_desc.AdapterLuid,
            &IID_IDXGIAdapter, NULL);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIFactory4_EnumAdapterByLuid(factory4, device_adapter_desc.AdapterLuid,
            &IID_IDXGIAdapter, (void **)&adapter);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IDXGIAdapter_GetDesc(adapter, &desc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        check_adapter_desc(&desc, &device_adapter_desc);
        refcount = IDXGIAdapter_Release(adapter);
        ok(!refcount, "Adapter has %lu references left.\n", refcount);
    }

    if (have_unique_luid)
    {
        hr = IDXGIFactory4_EnumAdapterByLuid(factory4, luid, &IID_IDXGIAdapter, (void **)&adapter);
        ok(hr == DXGI_ERROR_NOT_FOUND, "Got unexpected hr %#lx.\n", hr);
    }
    else
    {
        skip("Our LUID is not unique.\n");
    }

    refcount = IDXGIFactory4_Release(factory4);
    ok(!refcount, "Factory has %lu references left.\n", refcount);
}

static void test_query_video_memory_info(void)
{
    DXGI_QUERY_VIDEO_MEMORY_INFO memory_info;
    IDXGIAdapter3 *adapter3;
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGIAdapter_QueryInterface(adapter, &IID_IDXGIAdapter3, (void **)&adapter3);
    ok(hr == S_OK || hr == E_NOINTERFACE, "Got unexpected hr %#lx.\n", hr);
    if (hr == E_NOINTERFACE)
        goto done;

    hr = IDXGIAdapter3_QueryVideoMemoryInfo(adapter3, 0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memory_info);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(memory_info.Budget >= memory_info.AvailableForReservation,
            "Available for reservation 0x%s is greater than budget 0x%s.\n",
            wine_dbgstr_longlong(memory_info.AvailableForReservation),
            wine_dbgstr_longlong(memory_info.Budget));
    ok(!memory_info.CurrentReservation, "Got unexpected current reservation 0x%s.\n",
            wine_dbgstr_longlong(memory_info.CurrentReservation));

    hr = IDXGIAdapter3_QueryVideoMemoryInfo(adapter3, 0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &memory_info);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(memory_info.Budget >= memory_info.AvailableForReservation,
            "Available for reservation 0x%s is greater than budget 0x%s.\n",
            wine_dbgstr_longlong(memory_info.AvailableForReservation),
            wine_dbgstr_longlong(memory_info.Budget));
    ok(!memory_info.CurrentReservation, "Got unexpected current reservation 0x%s.\n",
            wine_dbgstr_longlong(memory_info.CurrentReservation));

    hr = IDXGIAdapter3_QueryVideoMemoryInfo(adapter3, 0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL + 1, &memory_info);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    IDXGIAdapter3_Release(adapter3);

done:
    IDXGIAdapter_Release(adapter);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

static void test_check_interface_support(void)
{
    LARGE_INTEGER driver_version;
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    IUnknown *iface;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIAdapter_CheckInterfaceSupport(adapter, &IID_IDXGIDevice, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGIAdapter_CheckInterfaceSupport(adapter, &IID_IDXGIDevice, &driver_version);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGIAdapter_CheckInterfaceSupport(adapter, &IID_ID3D10Device, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGIAdapter_CheckInterfaceSupport(adapter, &IID_ID3D10Device, &driver_version);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    trace("UMD version: %u.%u.%u.%u.\n",
            HIWORD(driver_version.HighPart), LOWORD(driver_version.HighPart),
            HIWORD(driver_version.LowPart), LOWORD(driver_version.LowPart));

    hr = IDXGIDevice_QueryInterface(device, &IID_ID3D10Device1, (void **)&iface);
    if (SUCCEEDED(hr))
    {
        IUnknown_Release(iface);
        hr = IDXGIAdapter_CheckInterfaceSupport(adapter, &IID_ID3D10Device1, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = IDXGIAdapter_CheckInterfaceSupport(adapter, &IID_ID3D10Device1, &driver_version);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    }
    else
    {
        win_skip("D3D10.1 is not supported.\n");
    }

    hr = IDXGIAdapter_CheckInterfaceSupport(adapter, &IID_ID3D11Device, NULL);
    ok(hr == DXGI_ERROR_UNSUPPORTED, "Got unexpected hr %#lx.\n", hr);
    driver_version.LowPart = driver_version.HighPart = 0xdeadbeef;
    hr = IDXGIAdapter_CheckInterfaceSupport(adapter, &IID_ID3D11Device, &driver_version);
    ok(hr == DXGI_ERROR_UNSUPPORTED, "Got unexpected hr %#lx.\n", hr);
    ok(driver_version.HighPart == 0xdeadbeef, "Got unexpected driver version %#lx.\n", driver_version.HighPart);
    ok(driver_version.LowPart == 0xdeadbeef, "Got unexpected driver version %#lx.\n", driver_version.LowPart);

    IDXGIAdapter_Release(adapter);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

static void test_create_surface(void)
{
    ID3D11Texture2D *texture2d;
    DXGI_SURFACE_DESC desc;
    IDXGISurface *surface;
    IDXGIDevice *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    desc.Width = 512;
    desc.Height = 512;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    hr = IDXGIDevice_CreateSurface(device, &desc, 1, DXGI_USAGE_RENDER_TARGET_OUTPUT, NULL, &surface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    check_interface(surface, &IID_IDXGIResource, TRUE, FALSE);
    check_interface(surface, &IID_ID3D10Texture2D, TRUE, FALSE);
    /* Not available on all Windows versions. */
    check_interface(surface, &IID_ID3D11Texture2D, TRUE, TRUE);
    /* Not available on all Windows versions. */
    check_interface(surface, &IID_IDXGISurface1, TRUE, TRUE);

    IDXGISurface_Release(surface);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);

    /* DXGI_USAGE_UNORDERED_ACCESS */
    if (!(device = create_d3d11_device()))
    {
        skip("Failed to create D3D11 device.\n");
        return;
    }

    surface = NULL;
    hr = IDXGIDevice_CreateSurface(device, &desc, 1, DXGI_USAGE_UNORDERED_ACCESS, NULL, &surface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    if (surface)
    {
        ID3D11UnorderedAccessView *uav;
        ID3D11Device *d3d_device;

        hr = IDXGISurface_QueryInterface(surface, &IID_ID3D11Texture2D, (void **)&texture2d);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        ID3D11Texture2D_GetDevice(texture2d, &d3d_device);

        hr = ID3D11Device_CreateUnorderedAccessView(d3d_device, (ID3D11Resource *)texture2d, NULL, &uav);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ID3D11UnorderedAccessView_Release(uav);

        ID3D11Device_Release(d3d_device);
        ID3D11Texture2D_Release(texture2d);

        IDXGISurface_Release(surface);
    }

    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

static void test_parents(void)
{
    DXGI_SURFACE_DESC surface_desc;
    IDXGISurface *surface;
    IDXGIFactory *factory;
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    IDXGIOutput *output;
    IUnknown *parent;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    surface_desc.Width = 512;
    surface_desc.Height = 512;
    surface_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    surface_desc.SampleDesc.Count = 1;
    surface_desc.SampleDesc.Quality = 0;

    hr = IDXGIDevice_CreateSurface(device, &surface_desc, 1, DXGI_USAGE_RENDER_TARGET_OUTPUT, NULL, &surface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGISurface_GetParent(surface, &IID_IDXGIDevice, (void **)&parent);
    IDXGISurface_Release(surface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(parent == (IUnknown *)device, "Got parent %p, expected %p.\n", parent, device);
    IUnknown_Release(parent);

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIAdapter_EnumOutputs(adapter, 0, &output);
    if (hr == DXGI_ERROR_NOT_FOUND)
    {
        skip("Adapter has not outputs.\n");
    }
    else
    {
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = IDXGIOutput_GetParent(output, &IID_IDXGIAdapter, (void **)&parent);
        IDXGIOutput_Release(output);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(parent == (IUnknown *)adapter, "Got parent %p, expected %p.\n", parent, adapter);
        IUnknown_Release(parent);
    }

    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIFactory_GetParent(factory, &IID_IUnknown, (void **)&parent);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#lx.\n", hr);
    ok(parent == NULL, "Got parent %p, expected %p.\n", parent, NULL);
    IDXGIFactory_Release(factory);

    hr = IDXGIDevice_GetParent(device, &IID_IDXGIAdapter, (void **)&parent);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(parent == (IUnknown *)adapter, "Got parent %p, expected %p.\n", parent, adapter);
    IUnknown_Release(parent);

    IDXGIAdapter_Release(adapter);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

static void test_output(void)
{
    unsigned int mode_count, mode_count_comp, i, last_height, last_width;
    double last_refresh_rate;
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    HRESULT hr;
    IDXGIOutput *output;
    ULONG refcount;
    DXGI_MODE_DESC *modes;

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIAdapter_EnumOutputs(adapter, 0, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIAdapter_EnumOutputs(adapter, 0, &output);
    if (hr == DXGI_ERROR_NOT_FOUND)
    {
        skip("Adapter doesn't have any outputs.\n");
        IDXGIAdapter_Release(adapter);
        IDXGIDevice_Release(device);
        return;
    }
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM, 0, NULL, NULL);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM, 0, &mode_count, NULL);
    ok(hr == S_OK|| broken(hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE), /* Remote Desktop Services / Win 7 testbot */
            "Got unexpected hr %#lx.\n", hr);
    if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
    {
        win_skip("GetDisplayModeList() not supported.\n");
        IDXGIOutput_Release(output);
        IDXGIAdapter_Release(adapter);
        IDXGIDevice_Release(device);
        return;
    }
    mode_count_comp = mode_count;

    hr = IDXGIOutput_GetDisplayModeList(output, 0, 0, &mode_count, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!mode_count, "Got unexpected mode_count %u.\n", mode_count);

    hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_ENUM_MODES_SCALING, &mode_count, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(mode_count >= mode_count_comp, "Got unexpected mode_count %u, expected >= %u.\n", mode_count, mode_count_comp);
    mode_count_comp = mode_count;

    modes = calloc(mode_count + 10, sizeof(*modes));
    ok(!!modes, "Failed to allocate memory.\n");

    hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_ENUM_MODES_SCALING, NULL, modes);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
    ok(!modes[0].Height, "No output was expected.\n");

    mode_count = 0;
    hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_ENUM_MODES_SCALING, &mode_count, modes);
    ok(hr == DXGI_ERROR_MORE_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(!modes[0].Height, "No output was expected.\n");

    mode_count = mode_count_comp;
    hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_ENUM_MODES_SCALING, &mode_count, modes);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(mode_count == mode_count_comp, "Got unexpected mode_count %u, expected %u.\n", mode_count, mode_count_comp);

    last_width = last_height = 0;
    last_refresh_rate = 0.;
    for (i = 0; i < mode_count; i++)
    {
        double refresh_rate = modes[i].RefreshRate.Numerator / (double)modes[i].RefreshRate.Denominator;

        ok(modes[i].Width && modes[i].Height, "Mode %u: Invalid dimensions %ux%u.\n",
                i, modes[i].Width, modes[i].Height);

        ok(modes[i].Width >= last_width,
                "Mode %u: Modes should have been sorted, width %u < %u.\n", i, modes[i].Width, last_width);
        if (modes[i].Width != last_width)
        {
            last_width = modes[i].Width;
            last_height = 0;
            last_refresh_rate = 0.;
            continue;
        }

        ok(modes[i].Height >= last_height,
                "Mode %u: Modes should have been sorted, height %u < %u.\n", i, modes[i].Height, last_height);
        if (modes[i].Height != last_height)
        {
            last_height = modes[i].Height;
            last_refresh_rate = 0.;
            continue;
        }

        ok(refresh_rate >= last_refresh_rate,
                "Mode %u: Modes should have been sorted, refresh rate %f < %f.\n", i, refresh_rate, last_refresh_rate);
        if (refresh_rate != last_refresh_rate)
            last_refresh_rate = refresh_rate;
    }

    mode_count += 5;
    hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_ENUM_MODES_SCALING, &mode_count, modes);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(mode_count == mode_count_comp, "Got unexpected mode_count %u, expected %u.\n", mode_count, mode_count_comp);

    if (mode_count_comp)
    {
        mode_count = mode_count_comp - 1;
        hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM,
                DXGI_ENUM_MODES_SCALING, &mode_count, modes);
        ok(hr == DXGI_ERROR_MORE_DATA, "Got unexpected hr %#lx.\n", hr);
        ok(mode_count == mode_count_comp - 1, "Got unexpected mode_count %u, expected %u.\n",
                mode_count, mode_count_comp - 1);
    }
    else
    {
        skip("Not enough modes for test.\n");
    }

    free(modes);
    IDXGIOutput_Release(output);
    IDXGIAdapter_Release(adapter);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

static void test_find_closest_matching_mode(void)
{
    static const DXGI_MODE_SCALING scaling_tests[] =
    {
        DXGI_MODE_SCALING_CENTERED,
        DXGI_MODE_SCALING_STRETCHED
    };
    DXGI_MODE_DESC *modes, mode, matching_mode;
    unsigned int i, j, mode_count;
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    IDXGIOutput *output;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIAdapter_EnumOutputs(adapter, 0, &output);
    if (hr == DXGI_ERROR_NOT_FOUND)
    {
        win_skip("Adapter doesn't have any outputs.\n");
        IDXGIAdapter_Release(adapter);
        IDXGIDevice_Release(device);
        return;
    }
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    memset(&mode, 0, sizeof(mode));
    hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
    ok(hr == DXGI_ERROR_INVALID_CALL || broken(hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE), /* Win 7 testbot */
            "Got unexpected hr %#lx.\n", hr);
    if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
    {
        win_skip("FindClosestMatchingMode() not supported.\n");
        goto done;
    }

    memset(&mode, 0, sizeof(mode));
    hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, (IUnknown *)device);
    todo_wine ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM, 0, &mode_count, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    modes = calloc(mode_count, sizeof(*modes));
    ok(!!modes, "Failed to allocate memory.\n");

    hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM, 0, &mode_count, modes);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    for (i = 0; i < mode_count; ++i)
    {
        mode = modes[i];
        hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        check_mode_desc(&matching_mode, &modes[i], MODE_DESC_IGNORE_SCALING);

        mode.Format = DXGI_FORMAT_UNKNOWN;
        hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
        ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);

        mode = modes[i];
        mode.Width = 0;
        hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
        ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);

        mode = modes[i];
        mode.Height = 0;
        hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
        ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);

        mode = modes[i];
        mode.Width = mode.Height = 0;
        hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        check_mode_desc(&matching_mode, &modes[i], MODE_DESC_IGNORE_SCALING | MODE_DESC_IGNORE_RESOLUTION);
        ok(matching_mode.Width > 0 && matching_mode.Height > 0, "Got unexpected resolution %ux%u.\n",
                matching_mode.Width, matching_mode.Height);

        mode = modes[i];
        mode.RefreshRate.Numerator = mode.RefreshRate.Denominator = 0;
        hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        check_mode_desc(&matching_mode, &modes[i], MODE_DESC_IGNORE_SCALING | MODE_DESC_IGNORE_REFRESH_RATE);
        ok(matching_mode.RefreshRate.Numerator > 0 && matching_mode.RefreshRate.Denominator > 0,
                "Got unexpected refresh rate %u / %u.\n",
                matching_mode.RefreshRate.Numerator, matching_mode.RefreshRate.Denominator);

        mode = modes[i];
        mode.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        check_mode_desc(&matching_mode, &modes[i], MODE_DESC_IGNORE_SCALING | MODE_DESC_IGNORE_SCANLINE_ORDERING);
        ok(matching_mode.ScanlineOrdering, "Got unexpected scanline ordering %#x.\n",
                matching_mode.ScanlineOrdering);

        memset(&mode, 0, sizeof(mode));
        mode.Width = modes[i].Width;
        mode.Height = modes[i].Height;
        mode.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        check_mode_desc(&matching_mode, &modes[i], MODE_DESC_CHECK_RESOLUTION & MODE_DESC_CHECK_FORMAT);

        memset(&mode, 0, sizeof(mode));
        mode.Width = modes[i].Width - 1;
        mode.Height = modes[i].Height - 1;
        mode.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        check_mode_desc(&matching_mode, &modes[i],
                (MODE_DESC_CHECK_RESOLUTION & MODE_DESC_CHECK_FORMAT) | MODE_DESC_IGNORE_EXACT_RESOLUTION);

        memset(&mode, 0, sizeof(mode));
        mode.Width = modes[i].Width + 1;
        mode.Height = modes[i].Height + 1;
        mode.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        check_mode_desc(&matching_mode, &modes[i],
                (MODE_DESC_CHECK_RESOLUTION & MODE_DESC_CHECK_FORMAT) | MODE_DESC_IGNORE_EXACT_RESOLUTION);
    }

    memset(&mode, 0, sizeof(mode));
    mode.Width = mode.Height = 10;
    mode.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    /* Find mode for the lowest resolution. */
    mode = modes[0];
    for (i = 0; i < mode_count; ++i)
    {
        if (mode.Width >= modes[i].Width && mode.Height >= modes[i].Height)
            mode = modes[i];
    }
    check_mode_desc(&matching_mode, &mode, MODE_DESC_CHECK_RESOLUTION & MODE_DESC_CHECK_FORMAT);

    memset(&mode, 0, sizeof(mode));
    mode.Width = modes[0].Width;
    mode.Height = modes[0].Height;
    mode.Format = modes[0].Format;
    mode.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST;
    hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_mode_desc(&matching_mode, &modes[0], MODE_DESC_CHECK_RESOLUTION & MODE_DESC_CHECK_FORMAT);

    memset(&mode, 0, sizeof(mode));
    mode.Width = modes[0].Width;
    mode.Height = modes[0].Height;
    mode.Format = modes[0].Format;
    mode.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST;
    hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_mode_desc(&matching_mode, &modes[0], MODE_DESC_CHECK_RESOLUTION & MODE_DESC_CHECK_FORMAT);

    for (i = 0; i < ARRAY_SIZE(scaling_tests); ++i)
    {
        for (j = 0; j < mode_count; ++j)
        {
            if (modes[j].Scaling != scaling_tests[i])
                continue;

            memset(&mode, 0, sizeof(mode));
            mode.Width = modes[j].Width;
            mode.Height = modes[j].Height;
            mode.Format = modes[j].Format;
            mode.Scaling = modes[j].Scaling;
            hr = IDXGIOutput_FindClosestMatchingMode(output, &mode, &matching_mode, NULL);
            ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
            check_mode_desc(&matching_mode, &modes[j],
                    MODE_DESC_IGNORE_REFRESH_RATE | MODE_DESC_IGNORE_SCANLINE_ORDERING);
            break;
        }
    }

    free(modes);

done:
    IDXGIOutput_Release(output);
    IDXGIAdapter_Release(adapter);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

struct refresh_rates
{
    UINT numerator;
    UINT denominator;
    BOOL numerator_should_pass;
    BOOL denominator_should_pass;
};

static void test_create_swapchain(void)
{
    struct swapchain_fullscreen_state initial_state, expected_state;
    unsigned int  i, expected_width, expected_height;
    DXGI_SWAP_CHAIN_DESC creation_desc, result_desc;
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreen_desc;
    DXGI_SWAP_CHAIN_DESC1 swapchain_desc;
    IDXGIDevice *device, *bgra_device;
    ULONG refcount, expected_refcount;
    IUnknown *obj, *obj2, *parent;
    IDXGISwapChain1 *swapchain1;
    RECT *expected_client_rect;
    IDXGISwapChain *swapchain;
    IDXGISurface1 *surface;
    IDXGIAdapter *adapter;
    IDXGIFactory *factory;
    IDXGIOutput *target;
    BOOL fullscreen;
    HWND window;
    HRESULT hr;

    const struct refresh_rates refresh_list[] =
    {
        {60, 60, FALSE, FALSE},
        {60,  0,  TRUE, FALSE},
        {60,  1,  TRUE,  TRUE},
        { 0, 60,  TRUE, FALSE},
        { 0,  0,  TRUE, FALSE},
    };

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    creation_desc.BufferDesc.Width = 800;
    creation_desc.BufferDesc.Height = 600;
    creation_desc.BufferDesc.RefreshRate.Numerator = 60;
    creation_desc.BufferDesc.RefreshRate.Denominator = 60;
    creation_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    creation_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    creation_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    creation_desc.SampleDesc.Count = 1;
    creation_desc.SampleDesc.Quality = 0;
    creation_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    creation_desc.BufferCount = 1;
    creation_desc.OutputWindow = NULL;
    creation_desc.Windowed = TRUE;
    creation_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    creation_desc.Flags = 0;

    hr = IDXGIDevice_QueryInterface(device, &IID_IUnknown, (void **)&obj);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    expected_refcount = get_refcount(adapter);
    refcount = get_refcount(factory);
    ok(refcount == 2, "Got unexpected refcount %lu.\n", refcount);
    refcount = get_refcount(device);
    ok(refcount == 2, "Got unexpected refcount %lu.\n", refcount);

    creation_desc.OutputWindow = NULL;
    hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);

    creation_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 0, 0, 0, 0, 0, 0);
    memset(&initial_state, 0, sizeof(initial_state));
    capture_fullscreen_state(&initial_state.fullscreen_state, creation_desc.OutputWindow);

    hr = IDXGIFactory_CreateSwapChain(factory, NULL, &creation_desc, &swapchain);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGIFactory_CreateSwapChain(factory, obj, NULL, &swapchain);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, NULL);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    refcount = get_refcount(adapter);
    ok(refcount >= expected_refcount, "Got refcount %lu, expected >= %lu.\n", refcount, expected_refcount);
    refcount = get_refcount(factory);
    todo_wine ok(refcount == 4, "Got unexpected refcount %lu.\n", refcount);
    refcount = get_refcount(device);
    ok(refcount == 3, "Got unexpected refcount %lu.\n", refcount);

    hr = IDXGISwapChain_GetDesc(swapchain, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGISwapChain_GetParent(swapchain, &IID_IUnknown, (void **)&parent);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(parent == (IUnknown *)factory, "Got unexpected parent interface pointer %p.\n", parent);
    refcount = IUnknown_Release(parent);
    todo_wine ok(refcount == 4, "Got unexpected refcount %lu.\n", refcount);

    hr = IDXGISwapChain_GetParent(swapchain, &IID_IDXGIFactory, (void **)&parent);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(parent == (IUnknown *)factory, "Got unexpected parent interface pointer %p.\n", parent);
    refcount = IUnknown_Release(parent);
    todo_wine ok(refcount == 4, "Got unexpected refcount %lu.\n", refcount);

    hr = IDXGISwapChain_QueryInterface(swapchain, &IID_IDXGISwapChain1, (void **)&swapchain1);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Got unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IDXGISwapChain1_GetDesc1(swapchain1, NULL);
        ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
        hr = IDXGISwapChain1_GetDesc1(swapchain1, &swapchain_desc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(!swapchain_desc.Stereo, "Got unexpected stereo %#x.\n", swapchain_desc.Stereo);
        ok(swapchain_desc.Scaling == DXGI_SCALING_STRETCH,
                "Got unexpected scaling %#x.\n", swapchain_desc.Scaling);
        ok(swapchain_desc.AlphaMode == DXGI_ALPHA_MODE_IGNORE,
                "Got unexpected alpha mode %#x.\n", swapchain_desc.AlphaMode);
        hr = IDXGISwapChain1_GetFullscreenDesc(swapchain1, NULL);
        ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
        hr = IDXGISwapChain1_GetFullscreenDesc(swapchain1, &fullscreen_desc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(fullscreen_desc.Windowed == creation_desc.Windowed,
                "Got unexpected windowed %#x.\n", fullscreen_desc.Windowed);
        hr = IDXGISwapChain1_GetHwnd(swapchain1, &window);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(window == creation_desc.OutputWindow, "Got unexpected window %p.\n", window);
        IDXGISwapChain1_Release(swapchain1);
    }

    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "Swapchain has %lu references left.\n", refcount);

    refcount = get_refcount(factory);
    ok(refcount == 2, "Got unexpected refcount %lu.\n", refcount);

    for (i = 0; i < ARRAY_SIZE(refresh_list); ++i)
    {
        creation_desc.BufferDesc.RefreshRate.Numerator = refresh_list[i].numerator;
        creation_desc.BufferDesc.RefreshRate.Denominator = refresh_list[i].denominator;

        hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);

        hr = IDXGISwapChain_GetDesc(swapchain, &result_desc);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);

        ok(result_desc.Windowed == creation_desc.Windowed, "Test %u: Got unexpected windowed %#x.\n",
                i, result_desc.Windowed);

        todo_wine_if (!refresh_list[i].numerator_should_pass)
            ok(result_desc.BufferDesc.RefreshRate.Numerator == refresh_list[i].numerator,
                    "Numerator %u is %u.\n", i, result_desc.BufferDesc.RefreshRate.Numerator);

        todo_wine_if (!refresh_list[i].denominator_should_pass)
            ok(result_desc.BufferDesc.RefreshRate.Denominator == refresh_list[i].denominator,
                    "Denominator %u is %u.\n", i, result_desc.BufferDesc.RefreshRate.Denominator);

        fullscreen = 0xdeadbeef;
        target = (void *)0xdeadbeef;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, &target);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        ok(!fullscreen, "Test %u: Got unexpected fullscreen %#x.\n", i, fullscreen);
        ok(!target, "Test %u: Got unexpected target %p.\n", i, target);

        hr = IDXGISwapChain_GetFullscreenState(swapchain, NULL, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        fullscreen = 0xdeadbeef;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        ok(!fullscreen, "Test %u: Got unexpected fullscreen %#x.\n", i, fullscreen);
        target = (void *)0xdeadbeef;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, NULL, &target);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        ok(!target, "Test %u: Got unexpected target %p.\n", i, target);

        check_swapchain_fullscreen_state(swapchain, &initial_state);
        IDXGISwapChain_Release(swapchain);
    }

    check_window_fullscreen_state(creation_desc.OutputWindow, &initial_state.fullscreen_state);

    /* Test GDI-compatible swapchain */
    bgra_device = create_device(D3D10_CREATE_DEVICE_BGRA_SUPPORT);
    ok(!!bgra_device, "Failed to create BGRA capable device.\n");

    hr = IDXGIDevice_QueryInterface(bgra_device, &IID_IUnknown, (void **)&obj2);
    ok(hr == S_OK, "IDXGIDevice does not implement IUnknown.\n");

    hr = IDXGIFactory_CreateSwapChain(factory, obj2, &creation_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_IDXGISurface1, (void **)&surface);
    if (SUCCEEDED(hr))
    {
        HDC hdc;

        hr = IDXGISurface1_GetDC(surface, FALSE, &hdc);
        ok(FAILED(hr), "Got unexpected hr %#lx.\n", hr);

        IDXGISurface1_Release(surface);
        IDXGISwapChain_Release(swapchain);

        creation_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        creation_desc.Flags = DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE;

        hr = IDXGIFactory_CreateSwapChain(factory, obj2, &creation_desc, &swapchain);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        creation_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        creation_desc.Flags = 0;

        hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_IDXGISurface1, (void **)&surface);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = IDXGISurface1_GetDC(surface, FALSE, &hdc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        IDXGISurface1_ReleaseDC(surface, NULL);

        IDXGISurface1_Release(surface);
        IDXGISwapChain_Release(swapchain);
    }
    else
    {
        win_skip("IDXGISurface1 is not supported, skipping GetDC() tests.\n");
        IDXGISwapChain_Release(swapchain);
    }
    IUnknown_Release(obj2);
    IDXGIDevice_Release(bgra_device);

    creation_desc.Windowed = FALSE;

    for (i = 0; i < ARRAY_SIZE(refresh_list); ++i)
    {
        creation_desc.BufferDesc.RefreshRate.Numerator = refresh_list[i].numerator;
        creation_desc.BufferDesc.RefreshRate.Denominator = refresh_list[i].denominator;

        hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
        ok(hr == S_OK || hr == DXGI_STATUS_OCCLUDED, "Test %u: Got unexpected hr %#lx.\n", i, hr);

        hr = IDXGISwapChain_GetDesc(swapchain, &result_desc);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);

        /* When numerator is non-zero and denominator is zero, the windowed mode is used.
         * Additionally, some versions of WARP seem to always fail to change fullscreen state. */
        if (result_desc.Windowed != creation_desc.Windowed)
            trace("Test %u: Failed to change fullscreen state.\n", i);

        todo_wine_if (!refresh_list[i].numerator_should_pass)
            ok(result_desc.BufferDesc.RefreshRate.Numerator == refresh_list[i].numerator,
                    "Numerator %u is %u.\n", i, result_desc.BufferDesc.RefreshRate.Numerator);

        todo_wine_if (!refresh_list[i].denominator_should_pass)
            ok(result_desc.BufferDesc.RefreshRate.Denominator == refresh_list[i].denominator,
                    "Denominator %u is %u.\n", i, result_desc.BufferDesc.RefreshRate.Denominator);

        fullscreen = FALSE;
        target = NULL;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, &target);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        ok(fullscreen == !result_desc.Windowed, "Test %u: Got fullscreen %#x, expected %#x.\n",
                i, fullscreen, result_desc.Windowed);
        ok(result_desc.Windowed ? !target : !!target, "Test %u: Got unexpected target %p.\n", i, target);
        if (!result_desc.Windowed)
        {
            IDXGIOutput *containing_output;
            hr = IDXGISwapChain_GetContainingOutput(swapchain, &containing_output);
            ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
            ok(containing_output == target, "Test %u: Got unexpected containing output pointer %p.\n",
                    i, containing_output);
            IDXGIOutput_Release(containing_output);

            ok(output_belongs_to_adapter(target, adapter),
                    "Test %u: Output %p doesn't belong to adapter %p.\n",
                    i, target, adapter);
            IDXGIOutput_Release(target);

            hr = IDXGISwapChain_GetFullscreenState(swapchain, NULL, NULL);
            ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
            fullscreen = FALSE;
            hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
            ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
            ok(fullscreen, "Test %u: Got unexpected fullscreen %#x.\n", i, fullscreen);
            target = NULL;
            hr = IDXGISwapChain_GetFullscreenState(swapchain, NULL, &target);
            ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
            ok(!!target, "Test %u: Got unexpected target %p.\n", i, target);
            IDXGIOutput_Release(target);
        }

        hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);

        fullscreen = 0xdeadbeef;
        target = (void *)0xdeadbeef;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, &target);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        ok(!fullscreen, "Test %u: Got unexpected fullscreen %#x.\n", i, fullscreen);
        ok(!target, "Test %u: Got unexpected target %p.\n", i, target);

        check_swapchain_fullscreen_state(swapchain, &initial_state);
        IDXGISwapChain_Release(swapchain);
    }

    check_window_fullscreen_state(creation_desc.OutputWindow, &initial_state.fullscreen_state);

    /* Test swapchain creation with DXGI_FORMAT_UNKNOWN. */
    creation_desc.BufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    creation_desc.Windowed = TRUE;
    creation_desc.Flags = 0;
    hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    creation_desc.Windowed = FALSE;
    hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    creation_desc.BufferCount = 2;
    creation_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
    ok(hr == E_INVALIDARG || hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
    creation_desc.BufferCount = 1;
    creation_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    check_window_fullscreen_state(creation_desc.OutputWindow, &initial_state.fullscreen_state);

    /* Test swapchain creation with backbuffer width and height equal to 0. */
    expected_state = initial_state;
    expected_client_rect = &expected_state.fullscreen_state.client_rect;

    /* Windowed */
    expected_width = expected_client_rect->right;
    expected_height = expected_client_rect->bottom;

    creation_desc.BufferDesc.Width = 0;
    creation_desc.BufferDesc.Height = 0;
    creation_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    creation_desc.Windowed = TRUE;
    creation_desc.Flags = 0;
    hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_GetDesc(swapchain, &result_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(result_desc.BufferDesc.Width == expected_width, "Got width %u, expected %u.\n",
            result_desc.BufferDesc.Width, expected_width);
    ok(result_desc.BufferDesc.Height == expected_height, "Got height %u, expected %u.\n",
            result_desc.BufferDesc.Height, expected_height);
    check_swapchain_fullscreen_state(swapchain, &expected_state);
    IDXGISwapChain_Release(swapchain);

    DestroyWindow(creation_desc.OutputWindow);
    creation_desc.OutputWindow = CreateWindowA("static", "dxgi_test",
            WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
            0, 0, 222, 222, 0, 0, 0, 0);
    expected_state.fullscreen_state.style = WS_CLIPSIBLINGS | WS_CAPTION
            | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
    SetRect(&expected_state.fullscreen_state.window_rect, 0, 0, 222, 222);
    GetClientRect(creation_desc.OutputWindow, expected_client_rect);
    expected_width = expected_client_rect->right;
    expected_height = expected_client_rect->bottom;

    creation_desc.BufferDesc.Width = 0;
    creation_desc.BufferDesc.Height = 0;
    hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_GetDesc(swapchain, &result_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(result_desc.BufferDesc.Width == expected_width, "Got width %u, expected %u.\n",
            result_desc.BufferDesc.Width, expected_width);
    ok(result_desc.BufferDesc.Height == expected_height, "Got height %u, expected %u.\n",
            result_desc.BufferDesc.Height, expected_height);
    check_swapchain_fullscreen_state(swapchain, &expected_state);
    IDXGISwapChain_Release(swapchain);

    DestroyWindow(creation_desc.OutputWindow);
    creation_desc.OutputWindow = CreateWindowA("static", "dxgi_test",
            WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
            1, 1, 0, 0, 0, 0, 0, 0);
    expected_state.fullscreen_state.style = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    expected_state.fullscreen_state.exstyle = 0;
    SetRect(&expected_state.fullscreen_state.window_rect, 1, 1, 1, 1);
    SetRectEmpty(expected_client_rect);
    expected_width = expected_height = 8;

    creation_desc.BufferDesc.Width = 0;
    creation_desc.BufferDesc.Height = 0;
    hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_GetDesc(swapchain, &result_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(result_desc.BufferDesc.Width == expected_width, "Got width %u, expected %u.\n",
            result_desc.BufferDesc.Width, expected_width);
    ok(result_desc.BufferDesc.Height == expected_height, "Got height %u, expected %u.\n",
            result_desc.BufferDesc.Height, expected_height);
    check_swapchain_fullscreen_state(swapchain, &expected_state);
    IDXGISwapChain_Release(swapchain);

    DestroyWindow(creation_desc.OutputWindow);
    creation_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 0, 0, 0, 0, 0, 0);
    check_window_fullscreen_state(creation_desc.OutputWindow, &initial_state.fullscreen_state);

    /* Fullscreen */
    creation_desc.Windowed = FALSE;
    hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
    ok(hr == S_OK || hr == DXGI_STATUS_OCCLUDED, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_GetDesc(swapchain, &result_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_GetContainingOutput(swapchain, &expected_state.target);
    ok(hr == S_OK || broken(hr == DXGI_ERROR_UNSUPPORTED) /* Win 7 testbot */,
            "Got unexpected hr %#lx.\n", hr);
    check_swapchain_fullscreen_state(swapchain, &initial_state);
    IDXGISwapChain_Release(swapchain);
    if (hr == DXGI_ERROR_UNSUPPORTED)
    {
        win_skip("GetContainingOutput() not supported.\n");
        goto done;
    }
    if (result_desc.Windowed)
    {
        win_skip("Fullscreen not supported.\n");
        IDXGIOutput_Release(expected_state.target);
        goto done;
    }

    creation_desc.BufferDesc.Width = 0;
    creation_desc.BufferDesc.Height = 0;
    creation_desc.Windowed = FALSE;
    creation_desc.Flags = 0;
    compute_expected_swapchain_fullscreen_state_after_fullscreen_change(&expected_state,
            &creation_desc, &initial_state.fullscreen_state.monitor_rect, 0, 0, expected_state.target);
    expected_width = expected_client_rect->right - expected_client_rect->left;
    expected_height = expected_client_rect->bottom - expected_client_rect->top;

    hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_GetDesc(swapchain, &result_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    todo_wine ok(result_desc.BufferDesc.Width == expected_width, "Got width %u, expected %u.\n",
            result_desc.BufferDesc.Width, expected_width);
    todo_wine ok(result_desc.BufferDesc.Height == expected_height, "Got height %u, expected %u.\n",
            result_desc.BufferDesc.Height, expected_height);
    check_swapchain_fullscreen_state(swapchain, &expected_state);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_swapchain_fullscreen_state(swapchain, &initial_state);
    IDXGISwapChain_Release(swapchain);

    /* Fullscreen and DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH */
    creation_desc.BufferDesc.Width = 0;
    creation_desc.BufferDesc.Height = 0;
    creation_desc.Windowed = FALSE;
    creation_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    compute_expected_swapchain_fullscreen_state_after_fullscreen_change(&expected_state,
            &creation_desc, &initial_state.fullscreen_state.monitor_rect, 0, 0, expected_state.target);
    expected_width = expected_client_rect->right - expected_client_rect->left;
    expected_height = expected_client_rect->bottom - expected_client_rect->top;

    hr = IDXGIFactory_CreateSwapChain(factory, obj, &creation_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_GetDesc(swapchain, &result_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    todo_wine ok(result_desc.BufferDesc.Width == expected_width, "Got width %u, expected %u.\n",
            result_desc.BufferDesc.Width, expected_width);
    todo_wine ok(result_desc.BufferDesc.Height == expected_height, "Got height %u, expected %u.\n",
            result_desc.BufferDesc.Height, expected_height);
    check_swapchain_fullscreen_state(swapchain, &expected_state);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_swapchain_fullscreen_state(swapchain, &initial_state);
    IDXGISwapChain_Release(swapchain);

    IDXGIOutput_Release(expected_state.target);

done:
    IUnknown_Release(obj);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
    refcount = IDXGIAdapter_Release(adapter);
    ok(!refcount, "Adapter has %lu references left.\n", refcount);
    refcount = IDXGIFactory_Release(factory);
    ok(!refcount, "Factory has %lu references left.\n", refcount);
    check_window_fullscreen_state(creation_desc.OutputWindow, &initial_state.fullscreen_state);
    DestroyWindow(creation_desc.OutputWindow);
}

static void test_get_containing_output(IUnknown *device, BOOL is_d3d12)
{
    unsigned int adapter_idx, output_idx, output_count;
    DXGI_OUTPUT_DESC output_desc, output_desc2;
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    IDXGIOutput *output, *output2;
    MONITORINFOEXW monitor_info;
    IDXGISwapChain *swapchain;
    IDXGIFactory *factory;
    IDXGIAdapter *adapter;
    POINT points[4 * 16];
    unsigned int i, j;
    HMONITOR monitor;
    BOOL fullscreen;
    ULONG refcount;
    HRESULT hr;
    BOOL ret;

    adapter = get_adapter(device, is_d3d12);
    if (!adapter)
    {
        skip("Failed to get adapter on Direct3D %d.\n", is_d3d12 ? 12 : 10);
        return;
    }

    output_count = 0;
    while ((hr = IDXGIAdapter_EnumOutputs(adapter, output_count, &output)) != DXGI_ERROR_NOT_FOUND)
    {
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        IDXGIOutput_Release(output);
        ++output_count;
    }
    IDXGIAdapter_Release(adapter);

    swapchain_desc.BufferDesc.Width = 100;
    swapchain_desc.BufferDesc.Height = 100;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 60;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = is_d3d12 ? 2 : 1;
    swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 100, 100, 0, 0, 0, 0);
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = is_d3d12 ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;

    get_factory(device, is_d3d12, &factory);
    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    monitor = MonitorFromWindow(swapchain_desc.OutputWindow, 0);
    ok(!!monitor, "MonitorFromWindow failed.\n");

    monitor_info.cbSize = sizeof(monitor_info);
    ret = GetMonitorInfoW(monitor, (MONITORINFO *)&monitor_info);
    ok(ret, "Failed to get monitor info.\n");

    hr = IDXGISwapChain_GetContainingOutput(swapchain, &output);
    ok(SUCCEEDED(hr) || broken(hr == DXGI_ERROR_UNSUPPORTED) /* Win 7 testbot */,
            "Got unexpected hr %#lx.\n", hr);
    if (hr == DXGI_ERROR_UNSUPPORTED)
    {
        win_skip("GetContainingOutput() not supported.\n");
        goto done;
    }

    hr = IDXGIOutput_GetDesc(output, &output_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGISwapChain_GetContainingOutput(swapchain, &output2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(output != output2, "Got unexpected output pointers %p, %p.\n", output, output2);
    check_output_equal(output, output2);

    refcount = IDXGIOutput_Release(output);
    ok(!refcount, "IDXGIOutput has %lu references left.\n", refcount);
    refcount = IDXGIOutput_Release(output2);
    ok(!refcount, "IDXGIOutput has %lu references left.\n", refcount);

    ok(!lstrcmpW(output_desc.DeviceName, monitor_info.szDevice),
            "Got unexpected device name %s, expected %s.\n",
            wine_dbgstr_w(output_desc.DeviceName), wine_dbgstr_w(monitor_info.szDevice));
    ok(EqualRect(&output_desc.DesktopCoordinates, &monitor_info.rcMonitor),
            "Got unexpected desktop coordinates %s, expected %s.\n",
            wine_dbgstr_rect(&output_desc.DesktopCoordinates),
            wine_dbgstr_rect(&monitor_info.rcMonitor));

    for (adapter_idx = 0; SUCCEEDED(IDXGIFactory_EnumAdapters(factory, adapter_idx, &adapter));
            ++adapter_idx)
    {
        for (output_idx = 0; SUCCEEDED(IDXGIAdapter_EnumOutputs(adapter, output_idx, &output));
                ++output_idx)
        {
            hr = IDXGIOutput_GetDesc(output, &output_desc);
            ok(hr == S_OK, "Adapter %u output %u: Got unexpected hr %#lx.\n", adapter_idx,
                    output_idx, hr);

            /* Move the OutputWindow to the current output. */
            ret = SetWindowPos(swapchain_desc.OutputWindow, 0, output_desc.DesktopCoordinates.left,
                    output_desc.DesktopCoordinates.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            ok(ret, "Adapter %u output %u: SetWindowPos failed.\n", adapter_idx, output_idx);

            hr = IDXGISwapChain_GetContainingOutput(swapchain, &output2);
            if (FAILED(hr))
            {
                win_skip("Adapter %u output %u: GetContainingOutput failed, hr %#lx.\n",
                        adapter_idx, output_idx, hr);
                IDXGIOutput_Release(output);
                continue;
            }

            check_output_equal(output, output2);

            refcount = IDXGIOutput_Release(output2);
            ok(!refcount, "Adapter %u output %u: IDXGIOutput has %lu references left.\n",
                    adapter_idx, output_idx, refcount);

            /* Move the OutputWindow around the corners of the current output desktop coordinates. */
            for (i = 0; i < 4; ++i)
            {
                static const POINT offsets[] =
                {
                    {  0,   0},
                    {-49,   0}, {-50,   0}, {-51,   0},
                    {  0, -49}, {  0, -50}, {  0, -51},
                    {-49, -49}, {-50, -49}, {-51, -49},
                    {-49, -50}, {-50, -50}, {-51, -50},
                    {-49, -51}, {-50, -51}, {-51, -51},
                };
                unsigned int x = 0, y = 0;

                switch (i)
                {
                    case 0:
                        x = output_desc.DesktopCoordinates.left;
                        y = output_desc.DesktopCoordinates.top;
                        break;
                    case 1:
                        x = output_desc.DesktopCoordinates.right;
                        y = output_desc.DesktopCoordinates.top;
                        break;
                    case 2:
                        x = output_desc.DesktopCoordinates.right;
                        y = output_desc.DesktopCoordinates.bottom;
                        break;
                    case 3:
                        x = output_desc.DesktopCoordinates.left;
                        y = output_desc.DesktopCoordinates.bottom;
                        break;
                }

                for (j = 0; j < ARRAY_SIZE(offsets); ++j)
                {
                    unsigned int idx = ARRAY_SIZE(offsets) * i + j;
                    assert(idx < ARRAY_SIZE(points));
                    points[idx].x = x + offsets[j].x;
                    points[idx].y = y + offsets[j].y;
                }
            }

            for (i = 0; i < ARRAY_SIZE(points); ++i)
            {
                ret = SetWindowPos(swapchain_desc.OutputWindow, 0, points[i].x, points[i].y,
                        0, 0, SWP_NOSIZE | SWP_NOZORDER);
                ok(ret, "Adapter %u output %u point %u: Failed to set window position.\n",
                        adapter_idx, output_idx, i);

                monitor = MonitorFromWindow(swapchain_desc.OutputWindow, MONITOR_DEFAULTTONEAREST);
                ok(!!monitor, "Adapter %u output %u point %u: Failed to get monitor from window.\n",
                        adapter_idx, output_idx, i);

                monitor_info.cbSize = sizeof(monitor_info);
                ret = GetMonitorInfoW(monitor, (MONITORINFO *)&monitor_info);
                ok(ret, "Adapter %u output %u point %u: Failed to get monitor info.\n", adapter_idx,
                        output_idx, i);

                hr = IDXGISwapChain_GetContainingOutput(swapchain, &output2);
                ok(hr == S_OK || broken(hr == DXGI_ERROR_UNSUPPORTED),
                        "Adapter %u output %u point %u: Got unexpected hr %#lx.\n", adapter_idx, output_idx, i, hr);
                if (hr != S_OK)
                    continue;
                ok(!!output2, "Adapter %u output %u point %u: Got unexpected containing output %p.\n",
                        adapter_idx, output_idx, i, output2);
                hr = IDXGIOutput_GetDesc(output2, &output_desc);
                ok(hr == S_OK, "Adapter %u output %u point %u: Got unexpected hr %#lx.\n",
                        adapter_idx, output_idx, i, hr);
                refcount = IDXGIOutput_Release(output2);
                ok(!refcount, "Adapter %u output %u point %u: IDXGIOutput has %lu references left.\n",
                        adapter_idx, output_idx, i, refcount);

                ok(!lstrcmpW(output_desc.DeviceName, monitor_info.szDevice),
                        "Adapter %u output %u point %u: Got unexpected device name %s, expected %s.\n",
                        adapter_idx, output_idx, i, wine_dbgstr_w(output_desc.DeviceName),
                        wine_dbgstr_w(monitor_info.szDevice));
                ok(EqualRect(&output_desc.DesktopCoordinates, &monitor_info.rcMonitor),
                        "Adapter %u output %u point %u: Expect desktop coordinates %s, got %s.\n",
                        adapter_idx, output_idx, i,
                        wine_dbgstr_rect(&output_desc.DesktopCoordinates),
                        wine_dbgstr_rect(&monitor_info.rcMonitor));
            }
            IDXGIOutput_Release(output);
        }
        IDXGIAdapter_Release(adapter);
    }

    /* Test GetContainingOutput with a full screen swapchain. The containing output should stay
     * the same even if the device window is moved */
    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
    if (FAILED(hr))
    {
        skip("SetFullscreenState failed, hr %#lx.\n", hr);
        goto done;
    }

    hr = IDXGISwapChain_GetContainingOutput(swapchain, &output2);
    if (FAILED(hr))
    {
        win_skip("GetContainingOutput failed, hr %#lx.\n", hr);
        IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
        goto done;
    }

    for (adapter_idx = 0; SUCCEEDED(IDXGIFactory_EnumAdapters(factory, adapter_idx, &adapter));
            ++adapter_idx)
    {
        for (output_idx = 0; SUCCEEDED(IDXGIAdapter_EnumOutputs(adapter, output_idx, &output));
                ++output_idx)
        {
            hr = IDXGIOutput_GetDesc(output, &output_desc);
            ok(hr == S_OK, "Adapter %u output %u: Got unexpected hr %#lx.\n", adapter_idx, output_idx, hr);
            IDXGIOutput_Release(output);

            /* Move the OutputWindow to the current output. */
            ret = SetWindowPos(swapchain_desc.OutputWindow, 0, output_desc.DesktopCoordinates.left,
                    output_desc.DesktopCoordinates.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            ok(ret, "Adapter %u output %u: SetWindowPos failed.\n", adapter_idx, output_idx);

            hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, &output);
            ok(hr == S_OK, "Adapter %u output %u: Got unexpected hr %#lx.\n",
                    adapter_idx, output_idx, hr);
            ok(fullscreen, "Adapter %u output %u: Expect swapchain full screen.\n", adapter_idx,
                    output_idx);
            ok(output == output2, "Adapter %u output %u: Expect output %p, got %p.\n",
                    adapter_idx, output_idx, output2, output);
            IDXGIOutput_Release(output);

            hr = IDXGISwapChain_GetContainingOutput(swapchain, &output);
            ok(hr == S_OK, "Adapter %u output %u: Got unexpected hr %#lx.\n", adapter_idx, output_idx, hr);
            ok(output == output2, "Adapter %u output %u: Expect output %p, got %p.\n",
                    adapter_idx, output_idx, output2, output);
            IDXGIOutput_Release(output);
        }
        IDXGIAdapter_Release(adapter);
    }

    IDXGIOutput_Release(output2);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Test GetContainingOutput after a full screen swapchain is made windowed by pressing
     * Alt+Enter, then move it to another output and use Alt+Enter to enter full screen */
    output = NULL;
    output2 = NULL;
    for (adapter_idx = 0; SUCCEEDED(IDXGIFactory_EnumAdapters(factory, adapter_idx, &adapter));
            ++adapter_idx)
    {
        for (output_idx = 0; SUCCEEDED(IDXGIAdapter_EnumOutputs(adapter, output_idx,
                output ? &output2 : &output)); ++output_idx)
        {
            if (output2)
                break;
        }

        IDXGIAdapter_Release(adapter);
        if (output2)
            break;
    }

    if (output && output2)
    {
        hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, output);
        IDXGIOutput_Release(output);
        if (FAILED(hr))
        {
            skip("SetFullscreenState failed, hr %#lx.\n", hr);
            IDXGIOutput_Release(output2);
            goto done;
        }

        /* Post an Alt + VK_RETURN WM_SYSKEYDOWN to leave full screen on the first output */
        PostMessageA(swapchain_desc.OutputWindow, WM_SYSKEYDOWN, VK_RETURN,
                (MapVirtualKeyA(VK_RETURN, MAPVK_VK_TO_VSC) << 16) | 0x20000001);
        flush_events();
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(!fullscreen, "Expect swapchain not full screen.\n");

        /* Move the swapchain output window to the second output */
        hr = IDXGIOutput_GetDesc(output2, &output_desc2);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ret = SetWindowPos(swapchain_desc.OutputWindow, 0, output_desc2.DesktopCoordinates.left,
                output_desc2.DesktopCoordinates.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        ok(ret, "SetWindowPos failed.\n");

        /* Post an Alt + VK_RETURN WM_SYSKEYDOWN to enter full screen on the second output */
        PostMessageA(swapchain_desc.OutputWindow, WM_SYSKEYDOWN, VK_RETURN,
                (MapVirtualKeyA(VK_RETURN, MAPVK_VK_TO_VSC) << 16) | 0x20000001);
        flush_events();
        output = NULL;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, &output);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(fullscreen, "Expect swapchain full screen.\n");
        ok(!!output, "Expect output not NULL.\n");
        hr = IDXGIOutput_GetDesc(output, &output_desc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = IDXGIOutput_GetDesc(output2, &output_desc2);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(output_desc.DeviceName, output_desc2.DeviceName),
                "Expect device name %s, got %s.\n", wine_dbgstr_w(output_desc2.DeviceName),
                wine_dbgstr_w(output_desc.DeviceName));
        IDXGIOutput_Release(output);

        output = NULL;
        hr = IDXGISwapChain_GetContainingOutput(swapchain, &output);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = IDXGIOutput_GetDesc(output, &output_desc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = IDXGIOutput_GetDesc(output2, &output_desc2);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(output_desc.DeviceName, output_desc2.DeviceName),
                "Expect device name %s, got %s.\n", wine_dbgstr_w(output_desc2.DeviceName),
                wine_dbgstr_w(output_desc.DeviceName));

        hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    }
    else
    {
        skip("This test requires two outputs.\n");
    }

    if (output)
        IDXGIOutput_Release(output);
    if (output2)
        IDXGIOutput_Release(output2);

done:
    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);
    refcount = IDXGIFactory_Release(factory);
    ok(refcount == !is_d3d12, "IDXGIFactory has %lu references left.\n", refcount);
    DestroyWindow(swapchain_desc.OutputWindow);
}

static void test_swapchain_fullscreen_state(IDXGISwapChain *swapchain,
        IDXGIAdapter *adapter, const struct swapchain_fullscreen_state *initial_state)
{
    MONITORINFOEXW monitor_info, *output_monitor_info;
    struct swapchain_fullscreen_state expected_state;
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    DXGI_OUTPUT_DESC output_desc;
    unsigned int i, output_count;
    IDXGIOutput *output;
    HRESULT hr;
    BOOL ret;

    hr = IDXGISwapChain_GetDesc(swapchain, &swapchain_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    check_swapchain_fullscreen_state(swapchain, initial_state);

    expected_state = *initial_state;
    compute_expected_swapchain_fullscreen_state_after_fullscreen_change(&expected_state,
            &swapchain_desc, &initial_state->fullscreen_state.monitor_rect, 800, 600, NULL);
    hr = IDXGISwapChain_GetContainingOutput(swapchain, &expected_state.target);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
    ok(hr == S_OK || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE, "Got unexpected hr %#lx.\n", hr);
    if (FAILED(hr))
    {
        skip("Could not change fullscreen state.\n");
        IDXGIOutput_Release(expected_state.target);
        return;
    }
    check_swapchain_fullscreen_state(swapchain, &expected_state);

    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_swapchain_fullscreen_state(swapchain, &expected_state);

    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_swapchain_fullscreen_state(swapchain, initial_state);

    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_swapchain_fullscreen_state(swapchain, initial_state);

    IDXGIOutput_Release(expected_state.target);
    expected_state.target = NULL;

    output_count = 0;
    while (IDXGIAdapter_EnumOutputs(adapter, output_count, &output) != DXGI_ERROR_NOT_FOUND)
    {
        IDXGIOutput_Release(output);
        ++output_count;
    }

    output_monitor_info = calloc(output_count, sizeof(*output_monitor_info));
    ok(!!output_monitor_info, "Failed to allocate memory.\n");
    for (i = 0; i < output_count; ++i)
    {
        hr = IDXGIAdapter_EnumOutputs(adapter, i, &output);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = IDXGIOutput_GetDesc(output, &output_desc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        output_monitor_info[i].cbSize = sizeof(*output_monitor_info);
        ret = GetMonitorInfoW(output_desc.Monitor, (MONITORINFO *)&output_monitor_info[i]);
        ok(ret, "Failed to get monitor info.\n");

        IDXGIOutput_Release(output);
    }

    for (i = 0; i < output_count; ++i)
    {
        RECT orig_monitor_rect = output_monitor_info[i].rcMonitor;
        IDXGIOutput *target;
        BOOL fullscreen;

        hr = IDXGIAdapter_EnumOutputs(adapter, i, &output);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = IDXGIOutput_GetDesc(output, &output_desc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        expected_state = *initial_state;
        expected_state.target = output;
        expected_state.fullscreen_state.monitor = output_desc.Monitor;
        expected_state.fullscreen_state.monitor_rect = orig_monitor_rect;
        compute_expected_swapchain_fullscreen_state_after_fullscreen_change(&expected_state,
                &swapchain_desc, &orig_monitor_rect, 800, 600, NULL);

        hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, output);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        check_swapchain_fullscreen_state(swapchain, &expected_state);

        target = NULL;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, NULL, &target);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(target == output, "Got target pointer %p, expected %p.\n", target, output);
        IDXGIOutput_Release(target);
        fullscreen = FALSE;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(fullscreen, "Got unexpected fullscreen %#x.\n", fullscreen);

        hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, output);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        check_swapchain_fullscreen_state(swapchain, &expected_state);
        hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, output);
        ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
        check_swapchain_fullscreen_state(swapchain, &expected_state);
        hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        check_swapchain_fullscreen_state(swapchain, initial_state);

        fullscreen = TRUE;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(!fullscreen, "Got unexpected fullscreen %#x.\n", fullscreen);

        check_swapchain_fullscreen_state(swapchain, initial_state);
        monitor_info.cbSize = sizeof(monitor_info);
        ret = GetMonitorInfoW(output_desc.Monitor, (MONITORINFO *)&monitor_info);
        ok(ret, "Failed to get monitor info.\n");
        ok(EqualRect(&monitor_info.rcMonitor, &orig_monitor_rect), "Got monitor rect %s, expected %s.\n",
                wine_dbgstr_rect(&monitor_info.rcMonitor), wine_dbgstr_rect(&orig_monitor_rect));

        hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, output);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        IDXGIOutput_Release(output);
    }

    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_swapchain_fullscreen_state(swapchain, initial_state);

    for (i = 0; i < output_count; ++i)
    {
        hr = IDXGIAdapter_EnumOutputs(adapter, i, &output);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = IDXGIOutput_GetDesc(output, &output_desc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        monitor_info.cbSize = sizeof(monitor_info);
        ret = GetMonitorInfoW(output_desc.Monitor, (MONITORINFO *)&monitor_info);
        ok(ret, "Failed to get monitor info.\n");

        ok(EqualRect(&monitor_info.rcMonitor, &output_monitor_info[i].rcMonitor),
                "Got monitor rect %s, expected %s.\n",
                wine_dbgstr_rect(&monitor_info.rcMonitor),
                wine_dbgstr_rect(&output_monitor_info[i].rcMonitor));

        IDXGIOutput_Release(output);
    }

    free(output_monitor_info);
}

static void test_set_fullscreen(IUnknown *device, BOOL is_d3d12)
{
    struct swapchain_fullscreen_state initial_state;
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    IDXGIAdapter *adapter = NULL;
    IDXGISwapChain *swapchain;
    IDXGIFactory *factory;
    IDXGIOutput *output;
    BOOL fullscreen;
    ULONG refcount;
    HRESULT hr;

    get_factory(device, is_d3d12, &factory);

    swapchain_desc.BufferDesc.Width = 800;
    swapchain_desc.BufferDesc.Height = 600;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 60;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = is_d3d12 ? 2 : 1;
    swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = is_d3d12 ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;

    memset(&initial_state, 0, sizeof(initial_state));
    capture_fullscreen_state(&initial_state.fullscreen_state, swapchain_desc.OutputWindow);
    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_GetContainingOutput(swapchain, &output);
    ok(hr == S_OK || broken(hr == DXGI_ERROR_UNSUPPORTED), /* Win 7 testbot */
            "Got unexpected hr %#lx.\n", hr);
    if (FAILED(hr))
    {
        skip("Could not get output.\n");
        goto done;
    }
    hr = IDXGIOutput_GetParent(output, &IID_IDXGIAdapter, (void **)&adapter);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IDXGIOutput_Release(output);

    check_swapchain_fullscreen_state(swapchain, &initial_state);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
    ok(hr == S_OK || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE
            || broken(hr == DXGI_ERROR_UNSUPPORTED), /* Win 7 testbot */
            "Got unexpected hr %#lx.\n", hr);
    if (FAILED(hr))
    {
        skip("Could not change fullscreen state.\n");
        goto done;
    }
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);

    DestroyWindow(swapchain_desc.OutputWindow);
    swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    check_window_fullscreen_state(swapchain_desc.OutputWindow, &initial_state.fullscreen_state);
    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_swapchain_fullscreen_state(swapchain, &initial_state);
    test_swapchain_fullscreen_state(swapchain, adapter, &initial_state);
    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);

    DestroyWindow(swapchain_desc.OutputWindow);
    swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    check_window_fullscreen_state(swapchain_desc.OutputWindow, &initial_state.fullscreen_state);
    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!fullscreen, "Got unexpected fullscreen %#x.\n", fullscreen);
    DestroyWindow(swapchain_desc.OutputWindow);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!fullscreen, "Got unexpected fullscreen %#x.\n", fullscreen);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
    ok(hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!fullscreen, "Got unexpected fullscreen %#x.\n", fullscreen);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);

    swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    check_window_fullscreen_state(swapchain_desc.OutputWindow, &initial_state.fullscreen_state);
    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!!fullscreen, "Got unexpected fullscreen %#x.\n", fullscreen);
    DestroyWindow(swapchain_desc.OutputWindow);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!!fullscreen, "Got unexpected fullscreen %#x.\n", fullscreen);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!fullscreen, "Got unexpected fullscreen %#x.\n", fullscreen);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
    ok(hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!fullscreen, "Got unexpected fullscreen %#x.\n", fullscreen);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);

    swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    check_window_fullscreen_state(swapchain_desc.OutputWindow, &initial_state.fullscreen_state);
    swapchain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_swapchain_fullscreen_state(swapchain, &initial_state);
    test_swapchain_fullscreen_state(swapchain, adapter, &initial_state);

done:
    if (adapter)
        IDXGIAdapter_Release(adapter);
    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);
    check_window_fullscreen_state(swapchain_desc.OutputWindow, &initial_state.fullscreen_state);
    DestroyWindow(swapchain_desc.OutputWindow);

    refcount = IDXGIFactory_Release(factory);
    ok(refcount == !is_d3d12, "Got unexpected refcount %lu.\n", refcount);
}

static void test_default_fullscreen_target_output(IUnknown *device, BOOL is_d3d12)
{
    IDXGIOutput *output, *containing_output, *target;
    unsigned int adapter_idx, output_idx;
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    DXGI_OUTPUT_DESC output_desc;
    unsigned int width, height;
    IDXGISwapChain *swapchain;
    IDXGIFactory *factory;
    IDXGIAdapter *adapter;
    BOOL fullscreen, ret;
    RECT window_rect;
    ULONG refcount;
    HRESULT hr;

    get_factory(device, is_d3d12, &factory);

    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 60;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = is_d3d12 ? 2 : 1;
    swapchain_desc.SwapEffect = is_d3d12 ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;

    for (adapter_idx = 0; SUCCEEDED(IDXGIFactory_EnumAdapters(factory, adapter_idx, &adapter));
            ++adapter_idx)
    {
        for (output_idx = 0; SUCCEEDED(IDXGIAdapter_EnumOutputs(adapter, output_idx, &output));
                ++output_idx)
        {
            /* Windowed swapchain */
            swapchain_desc.BufferDesc.Width = 640;
            swapchain_desc.BufferDesc.Height = 480;
            swapchain_desc.OutputWindow = create_window();
            swapchain_desc.Windowed = TRUE;
            hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
            ok(hr == S_OK, "Adapter %u output %u: Got unexpected hr %#lx.\n", adapter_idx, output_idx, hr);

            hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, &containing_output);
            ok(hr == S_OK, "Adapter %u output %u: Got unexpected hr %#lx.\n", adapter_idx, output_idx, hr);
            ok(!fullscreen, "Adapter %u output %u: Expected not fullscreen.\n", adapter_idx, output_idx);
            ok(!containing_output, "Adapter %u output %u: Expected a null output.\n", adapter_idx, output_idx);

            /* Move the OutputWindow to the current output. */
            hr = IDXGIOutput_GetDesc(output, &output_desc);
            ok(hr == S_OK, "Adapter %u output %u: Got unexpected hr %#lx.\n", adapter_idx, output_idx, hr);
            ret = SetWindowPos(swapchain_desc.OutputWindow, 0,
                    output_desc.DesktopCoordinates.left, output_desc.DesktopCoordinates.top,
                    0, 0, SWP_NOSIZE | SWP_NOZORDER);
            ok(ret, "Adapter %u output %u: SetWindowPos failed, error %#lx.\n", adapter_idx,
                    output_idx, GetLastError());

            hr = IDXGISwapChain_GetContainingOutput(swapchain, &containing_output);
            ok(hr == S_OK || broken(hr == DXGI_ERROR_UNSUPPORTED) /* Win 7 testbot */,
                    "Adapter %u output %u: Got unexpected hr %#lx.\n", adapter_idx, output_idx, hr);
            if (hr == DXGI_ERROR_UNSUPPORTED)
            {
                win_skip("Adapter %u output %u: GetContainingOutput() not supported.\n",
                        adapter_idx, output_idx);
                IDXGISwapChain_Release(swapchain);
                IDXGIOutput_Release(output);
                DestroyWindow(swapchain_desc.OutputWindow);
                continue;
            }

            hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
            ok(hr == S_OK || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE,
                    "Adapter %u output %u: Got unexpected hr %#lx.\n", adapter_idx, output_idx, hr);
            if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
            {
                skip("Adapter %u output %u: Could not change fullscreen state.\n", adapter_idx,
                        output_idx);
                IDXGIOutput_Release(containing_output);
                IDXGISwapChain_Release(swapchain);
                IDXGIOutput_Release(output);
                DestroyWindow(swapchain_desc.OutputWindow);
                continue;
            }
            GetWindowRect(swapchain_desc.OutputWindow, &window_rect);
            ok(EqualRect(&window_rect, &output_desc.DesktopCoordinates),
                    "Adapter %u output %u: Expect window rect %s, got %s.\n", adapter_idx,
                    output_idx, wine_dbgstr_rect(&output_desc.DesktopCoordinates),
                    wine_dbgstr_rect(&window_rect));

            target = NULL;
            hr = IDXGISwapChain_GetFullscreenState(swapchain, NULL, &target);
            ok(hr == S_OK, "Adapter %u output %u: Got unexpected hr %#lx.\n", adapter_idx, output_idx, hr);
            ok(target != containing_output,
                    "Adapter %u output %u: Got unexpected output %p, expected %p.\n", adapter_idx,
                    output_idx, target, containing_output);
            check_output_equal(target, containing_output);

            refcount = IDXGIOutput_Release(containing_output);
            ok(!refcount, "Adapter %u output %u: IDXGIOutput has %lu references left.\n",
                    adapter_idx, output_idx, refcount);

            hr = IDXGISwapChain_GetContainingOutput(swapchain, &containing_output);
            ok(hr == S_OK, "Adapter %u output %u: Got unexpected hr %#lx.\n", adapter_idx, output_idx, hr);
            ok(containing_output == target,
                    "Adapter %u output %u: Got unexpected containing output %p, expected %p.\n",
                    adapter_idx, output_idx, containing_output, target);
            refcount = IDXGIOutput_Release(containing_output);
            ok(refcount >= 2, "Adapter %u output %u: Got unexpected refcount %lu.\n", adapter_idx,
                    output_idx, refcount);
            refcount = IDXGIOutput_Release(target);
            ok(refcount >= 1, "Adapter %u output %u: Got unexpected refcount %lu.\n", adapter_idx,
                    output_idx, refcount);

            hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
            ok(hr == S_OK, "Adapter %u output %u: Got unexpected hr %#lx.\n", adapter_idx, output_idx, hr);
            refcount = IDXGISwapChain_Release(swapchain);
            ok(!refcount, "Adapter %u output %u: IDXGISwapChain has %lu references left.\n",
                    adapter_idx, output_idx, refcount);
            DestroyWindow(swapchain_desc.OutputWindow);

            /* Full screen swapchain */
            width = output_desc.DesktopCoordinates.right - output_desc.DesktopCoordinates.left;
            height = output_desc.DesktopCoordinates.bottom - output_desc.DesktopCoordinates.top;
            swapchain_desc.BufferDesc.Width = width;
            swapchain_desc.BufferDesc.Height = height;
            swapchain_desc.OutputWindow = create_window();
            swapchain_desc.Windowed = FALSE;
            ret = SetWindowPos(swapchain_desc.OutputWindow, 0, output_desc.DesktopCoordinates.left,
                    output_desc.DesktopCoordinates.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            ok(ret, "Adapter %u output %u: SetWindowPos failed, error %#lx.\n", adapter_idx,
                    output_idx, GetLastError());
            hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
            if (FAILED(hr))
            {
                skip("Adapter %u output %u: CreateSwapChain failed, hr %#lx.\n", adapter_idx,
                        output_idx, hr);
                IDXGIOutput_Release(output);
                DestroyWindow(swapchain_desc.OutputWindow);
                continue;
            }

            hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, &containing_output);
            ok(hr == S_OK, "Adapter %u output %u: Got unexpected hr %#lx.\n", adapter_idx, output_idx, hr);
            ok(fullscreen, "Adapter %u output %u: Expected fullscreen.\n", adapter_idx, output_idx);
            ok(!!containing_output, "Adapter %u output %u: Expected a valid output.\n", adapter_idx, output_idx);
            if (containing_output)
                IDXGIOutput_Release(containing_output);

            ret = GetWindowRect(swapchain_desc.OutputWindow, &window_rect);
            ok(ret, "Adapter %u output %u: GetWindowRect failed, error %#lx.\n", adapter_idx,
                    output_idx, GetLastError());
            ok(EqualRect(&window_rect, &output_desc.DesktopCoordinates),
                    "Adapter %u output %u: Expect window rect %s, got %s.\n", adapter_idx,
                    output_idx, wine_dbgstr_rect(&output_desc.DesktopCoordinates),
                    wine_dbgstr_rect(&window_rect));

            hr = IDXGISwapChain_GetContainingOutput(swapchain, &containing_output);
            ok(hr == S_OK, "Adapter %u output %u: Got unexpected hr %#lx.\n", adapter_idx, output_idx, hr);
            ok(containing_output != output,
                    "Adapter %u output %u: Got unexpected output %p, expected %p.\n", adapter_idx,
                    output_idx, output, containing_output);
            check_output_equal(output, containing_output);
            IDXGIOutput_Release(containing_output);

            hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
            ok(hr == S_OK, "Adapter %u output %u: Got unexpected hr %#lx.\n", adapter_idx, output_idx, hr);
            refcount = IDXGISwapChain_Release(swapchain);
            ok(!refcount, "Adapter %u output %u: IDXGISwapChain has %lu references left.\n",
                    adapter_idx, output_idx, refcount);
            refcount = IDXGIOutput_Release(output);
            ok(!refcount, "Adapter %u output %u: IDXGIOutput has %lu references left.\n",
                    adapter_idx, output_idx, refcount);
            DestroyWindow(swapchain_desc.OutputWindow);
        }
        IDXGIAdapter_Release(adapter);
    }

    refcount = IDXGIFactory_Release(factory);
    ok(refcount == !is_d3d12, "IDXGIFactory has %lu references left.\n", refcount);
}

static void test_windowed_resize_target(IDXGISwapChain *swapchain, HWND window,
        struct swapchain_fullscreen_state *state)
{
    struct swapchain_fullscreen_state expected_state;
    struct fullscreen_state *e;
    DXGI_MODE_DESC mode;
    RECT window_rect;
    unsigned int i;
    HRESULT hr;
    BOOL ret;

    static const struct
    {
        unsigned int width, height;
    }
    sizes[] =
    {
        {200, 200},
        {400, 200},
        {400, 400},
        {600, 800},
        {1000, 600},
        {1600, 100},
        {2000, 1000},
    };

    check_swapchain_fullscreen_state(swapchain, state);
    expected_state = *state;
    e = &expected_state.fullscreen_state;

    for (i = 0; i < ARRAY_SIZE(sizes); ++i)
    {
        SetRect(&e->client_rect, 0, 0, sizes[i].width, sizes[i].height);
        e->window_rect = e->client_rect;
        ret = AdjustWindowRectEx(&e->window_rect, GetWindowLongW(window, GWL_STYLE),
                FALSE, GetWindowLongW(window, GWL_EXSTYLE));
        ok(ret, "AdjustWindowRectEx failed.\n");
        if (GetMenu(window))
            e->client_rect.bottom -= GetSystemMetrics(SM_CYMENU);
        SetRect(&e->window_rect, 0, 0,
                e->window_rect.right - e->window_rect.left,
                e->window_rect.bottom - e->window_rect.top);
        GetWindowRect(window, &window_rect);
        OffsetRect(&e->window_rect, window_rect.left, window_rect.top);
        if (e->window_rect.right >= e->monitor_rect.right
                || e->window_rect.bottom >= e->monitor_rect.bottom)
        {
            skip("Test %u: Window %s does not fit on screen %s.\n",
                    i, wine_dbgstr_rect(&e->window_rect), wine_dbgstr_rect(&e->monitor_rect));
            continue;
        }

        memset(&mode, 0, sizeof(mode));
        mode.Width = sizes[i].width;
        mode.Height = sizes[i].height;
        hr = IDXGISwapChain_ResizeTarget(swapchain, &mode);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        check_swapchain_fullscreen_state(swapchain, &expected_state);
    }

    ret = SetWindowPos(window, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOZORDER);
    ok(ret, "SetWindowPos failed, error %#lx.\n", GetLastError());
    GetWindowRect(window, &e->window_rect);
    GetClientRect(window, &e->client_rect);
    ret = SetWindowPos(window, 0, 0, 0, 200, 200, SWP_NOMOVE | SWP_NOZORDER);
    ok(ret, "SetWindowPos failed, error %#lx.\n", GetLastError());

    memset(&mode, 0, sizeof(mode));
    hr = IDXGISwapChain_ResizeTarget(swapchain, &mode);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_swapchain_fullscreen_state(swapchain, &expected_state);

    GetWindowRect(window, &e->window_rect);
    GetClientRect(window, &e->client_rect);
    *state = expected_state;
}

static void test_fullscreen_resize_target(IDXGISwapChain *swapchain,
        const struct swapchain_fullscreen_state *initial_state)
{
    struct swapchain_fullscreen_state expected_state;
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    DXGI_OUTPUT_DESC output_desc;
    unsigned int i, mode_count;
    DXGI_MODE_DESC *modes;
    IDXGIOutput *target;
    HRESULT hr;

    hr = IDXGISwapChain_GetDesc(swapchain, &swapchain_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGISwapChain_GetFullscreenState(swapchain, NULL, &target);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIOutput_GetDisplayModeList(target, DXGI_FORMAT_R8G8B8A8_UNORM, 0, &mode_count, NULL);
    ok(hr == S_OK || broken(hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE), /* Win 7 testbot */
            "Got unexpected hr %#lx.\n", hr);
    if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
    {
        win_skip("GetDisplayModeList() not supported.\n");
        IDXGIOutput_Release(target);
        return;
    }

    modes = calloc(mode_count, sizeof(*modes));
    ok(!!modes, "Failed to allocate memory.\n");

    hr = IDXGIOutput_GetDisplayModeList(target, DXGI_FORMAT_R8G8B8A8_UNORM, 0, &mode_count, modes);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    expected_state = *initial_state;
    for (i = 0; i < min(mode_count, 20); ++i)
    {
        /* FIXME: Modes with scaling aren't fully tested. */
        if (!(swapchain_desc.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH)
                && modes[i].Scaling != DXGI_MODE_SCALING_UNSPECIFIED)
            continue;

        hr = IDXGIOutput_GetDesc(target, &output_desc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        compute_expected_swapchain_fullscreen_state_after_fullscreen_change(&expected_state,
                &swapchain_desc, &output_desc.DesktopCoordinates, modes[i].Width, modes[i].Height, NULL);

        hr = IDXGISwapChain_ResizeTarget(swapchain, &modes[i]);
        ok(hr == S_OK || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE, "Got unexpected hr %#lx.\n", hr);
        if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
        {
            skip("Failed to change to video mode %u.\n", i);
            break;
        }
        check_swapchain_fullscreen_state(swapchain, &expected_state);

        hr = IDXGIOutput_GetDesc(target, &output_desc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(EqualRect(&output_desc.DesktopCoordinates, &expected_state.fullscreen_state.monitor_rect),
                "Got desktop coordinates %s, expected %s.\n",
                wine_dbgstr_rect(&output_desc.DesktopCoordinates),
                wine_dbgstr_rect(&expected_state.fullscreen_state.monitor_rect));
    }

    free(modes);
    IDXGIOutput_Release(target);
}

static void test_resize_target(IUnknown *device, BOOL is_d3d12)
{
    struct swapchain_fullscreen_state initial_state, expected_state;
    unsigned int adapter_idx, output_idx, test_idx;
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    DXGI_OUTPUT_DESC output_desc;
    IDXGISwapChain *swapchain;
    IDXGIFactory *factory;
    IDXGIAdapter *adapter;
    IDXGIOutput *output;
    ULONG refcount;
    HRESULT hr;

    static const struct
    {
        POINT origin;
        BOOL fullscreen;
        BOOL menu;
        unsigned int flags;
    }
    tests[] =
    {
        {{ 0,  0}, TRUE,  FALSE, 0},
        {{10, 10}, TRUE,  FALSE, 0},
        {{ 0,  0}, TRUE,  FALSE, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH},
        {{10, 10}, TRUE,  FALSE, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH},
        {{ 0,  0}, FALSE, FALSE, 0},
        {{ 0,  0}, FALSE, FALSE, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH},
        {{10, 10}, FALSE, FALSE, 0},
        {{10, 10}, FALSE, FALSE, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH},
        {{ 0,  0}, FALSE, TRUE,  0},
        {{ 0,  0}, FALSE, TRUE,  DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH},
        {{10, 10}, FALSE, TRUE,  0},
        {{10, 10}, FALSE, TRUE,  DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH},
    };

    get_factory(device, is_d3d12, &factory);

    swapchain_desc.BufferDesc.Width = 800;
    swapchain_desc.BufferDesc.Height = 600;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 1;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = is_d3d12 ? 2 : 1;
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = is_d3d12 ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;

    for (adapter_idx = 0; SUCCEEDED(IDXGIFactory_EnumAdapters(factory, adapter_idx, &adapter));
            ++adapter_idx)
    {
        for (output_idx = 0; SUCCEEDED(IDXGIAdapter_EnumOutputs(adapter, output_idx, &output));
                ++output_idx)
        {
            hr = IDXGIOutput_GetDesc(output, &output_desc);
            ok(hr == S_OK, "Adapter %u output %u: Got unexpected hr %#lx.\n", adapter_idx, output_idx, hr);

            for (test_idx = 0; test_idx < ARRAY_SIZE(tests); ++test_idx)
            {
                swapchain_desc.Flags = tests[test_idx].flags;
                swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0,
                        output_desc.DesktopCoordinates.left + tests[test_idx].origin.x,
                        output_desc.DesktopCoordinates.top + tests[test_idx].origin.y,
                        400, 200, 0, 0, 0, 0);
                if (tests[test_idx].menu)
                {
                    HMENU menu_bar = CreateMenu();
                    HMENU menu = CreateMenu();
                    AppendMenuA(menu_bar, MF_POPUP, (UINT_PTR)menu, "Menu");
                    SetMenu(swapchain_desc.OutputWindow, menu_bar);
                }

                memset(&initial_state, 0, sizeof(initial_state));
                capture_fullscreen_state(&initial_state.fullscreen_state, swapchain_desc.OutputWindow);

                hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
                ok(hr == S_OK, "Adapter %u output %u test %u: Got unexpected hr %#lx.\n",
                        adapter_idx, output_idx, test_idx, hr);
                check_swapchain_fullscreen_state(swapchain, &initial_state);

                expected_state = initial_state;
                if (tests[test_idx].fullscreen)
                {
                    expected_state.fullscreen = TRUE;
                    compute_expected_swapchain_fullscreen_state_after_fullscreen_change(&expected_state,
                            &swapchain_desc, &initial_state.fullscreen_state.monitor_rect, 800, 600, NULL);
                    hr = IDXGISwapChain_GetContainingOutput(swapchain, &expected_state.target);
                    ok(hr == S_OK || broken(hr == DXGI_ERROR_UNSUPPORTED) /* Win 7 testbot */,
                            "Adapter %u output %u test %u: Got unexpected hr %#lx.\n",
                            adapter_idx, output_idx, test_idx, hr);
                    if (hr == DXGI_ERROR_UNSUPPORTED)
                    {
                        win_skip("Adapter %u output %u test %u: GetContainingOutput() not supported.\n",
                                adapter_idx, output_idx, test_idx);
                        IDXGISwapChain_Release(swapchain);
                        DestroyWindow(swapchain_desc.OutputWindow);
                        continue;
                    }

                    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
                    ok(hr == S_OK || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE,
                            "Adapter %u output %u test %u: Got unexpected hr %#lx.\n",
                            adapter_idx, output_idx, test_idx, hr);
                    if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
                    {
                        skip("Adapter %u output %u test %u: Could not change fullscreen state.\n",
                                adapter_idx, output_idx, test_idx);
                        IDXGIOutput_Release(expected_state.target);
                        IDXGISwapChain_Release(swapchain);
                        DestroyWindow(swapchain_desc.OutputWindow);
                        continue;
                    }
                }
                check_swapchain_fullscreen_state(swapchain, &expected_state);

                hr = IDXGISwapChain_ResizeTarget(swapchain, NULL);
                ok(hr == DXGI_ERROR_INVALID_CALL, "Adapter %u output %u test %u: Got unexpected hr %#lx.\n",
                        adapter_idx, output_idx, test_idx, hr);
                check_swapchain_fullscreen_state(swapchain, &expected_state);

                if (tests[test_idx].fullscreen)
                {
                    test_fullscreen_resize_target(swapchain, &expected_state);

                    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
                    ok(hr == S_OK, "Adapter %u output %u test %u: Got unexpected hr %#lx.\n",
                            adapter_idx, output_idx, test_idx, hr);
                    check_swapchain_fullscreen_state(swapchain, &initial_state);
                    IDXGIOutput_Release(expected_state.target);
                    check_swapchain_fullscreen_state(swapchain, &initial_state);
                    expected_state = initial_state;
                }
                else
                {
                    test_windowed_resize_target(swapchain, swapchain_desc.OutputWindow, &expected_state);

                    check_swapchain_fullscreen_state(swapchain, &expected_state);
                }

                refcount = IDXGISwapChain_Release(swapchain);
                ok(!refcount, "Adapter %u output %u test %u: IDXGISwapChain has %lu references left.\n",
                        adapter_idx, output_idx, test_idx, refcount);
                check_window_fullscreen_state(swapchain_desc.OutputWindow, &expected_state.fullscreen_state);
                DestroyWindow(swapchain_desc.OutputWindow);
            }
            IDXGIOutput_Release(output);
        }
        IDXGIAdapter_Release(adapter);
    }
    refcount = IDXGIFactory_Release(factory);
    ok(refcount == !is_d3d12, "Got unexpected refcount %lu.\n", refcount);
}

struct resize_target_data
{
    IDXGISwapChain *swapchain;
    BOOL test_nested_sfs;
};

static LRESULT CALLBACK resize_target_wndproc(HWND hwnd, unsigned int message, WPARAM wparam, LPARAM lparam)
{
    struct resize_target_data *data = (struct resize_target_data *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    DXGI_SWAP_CHAIN_DESC desc;
    HRESULT hr;
    BOOL fs;

    switch (message)
    {
        case WM_SIZE:
            ok(!!data, "GWLP_USERDATA is NULL.\n");
            hr = IDXGISwapChain_GetDesc(data->swapchain, &desc);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
            ok(desc.BufferDesc.Width == 800, "Got unexpected buffer width %u.\n", desc.BufferDesc.Width);
            ok(desc.BufferDesc.Height == 600, "Got unexpected buffer height %u.\n", desc.BufferDesc.Height);
            return 0;

        case WM_WINDOWPOSCHANGED:
            if (!data->test_nested_sfs)
                break;

            /* We are not supposed to deadlock if the window is owned by a different thread.
             * The current fullscreen state and consequently the return value of the nested
             * SetFullscreenState call are racy on Windows, do not test them. */
            hr = IDXGISwapChain_GetFullscreenState(data->swapchain, &fs, NULL);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
            IDXGISwapChain_SetFullscreenState(data->swapchain, FALSE, NULL);
            break;
    }
    return DefWindowProcA(hwnd, message, wparam, lparam);
}

struct window_thread_data
{
    HWND window;
    HANDLE window_created;
    HANDLE finished;
};

static DWORD WINAPI window_thread(void *data)
{
    struct window_thread_data *thread_data = data;
    unsigned int ret;
    WNDCLASSA wc;
    MSG msg;

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = resize_target_wndproc;
    wc.lpszClassName = "dxgi_resize_target_wndproc_wc";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");

    thread_data->window = CreateWindowA("dxgi_resize_target_wndproc_wc", "dxgi_test", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    ok(!!thread_data->window, "Failed to create window.\n");

    ret = SetEvent(thread_data->window_created);
    ok(ret, "Failed to set event, last error %#lx.\n", GetLastError());

    for (;;)
    {
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
            DispatchMessageA(&msg);

        ret = WaitForSingleObject(thread_data->finished, 0);
        if (ret != WAIT_TIMEOUT)
            break;
    }
    ok(ret == WAIT_OBJECT_0, "Failed to wait for event, ret %#x, last error %#lx.\n", ret, GetLastError());

    DestroyWindow(thread_data->window);
    thread_data->window = NULL;

    UnregisterClassA("dxgi_resize_target_wndproc_wc", GetModuleHandleA(NULL));

    return 0;
}

static void test_resize_target_wndproc(IUnknown *device, BOOL is_d3d12)
{
    struct resize_target_data window_data = {0};
    struct window_thread_data thread_data;
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    IDXGISwapChain *swapchain;
    IDXGIFactory *factory;
    DXGI_MODE_DESC mode;
    unsigned int ret;
    ULONG refcount;
    LONG_PTR data;
    HANDLE thread;
    HRESULT hr;
    RECT rect;
    BOOL fs;

    get_factory(device, is_d3d12, &factory);

    memset(&thread_data, 0, sizeof(thread_data));
    thread_data.window_created = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(!!thread_data.window_created, "Failed to create event, last error %#lx.\n", GetLastError());
    thread_data.finished = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(!!thread_data.finished, "Failed to create event, last error %#lx.\n", GetLastError());

    thread = CreateThread(NULL, 0, window_thread, &thread_data, 0, NULL);
    ok(!!thread, "Failed to create thread, last error %#lx.\n", GetLastError());
    ret = WaitForSingleObject(thread_data.window_created, INFINITE);
    ok(ret == WAIT_OBJECT_0, "Failed to wait for thread, ret %#x, last error %#lx.\n", ret, GetLastError());

    swapchain_desc.BufferDesc.Width = 800;
    swapchain_desc.BufferDesc.Height = 600;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 1;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = is_d3d12 ? 2 : 1;
    swapchain_desc.OutputWindow = thread_data.window;
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = is_d3d12 ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;
    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    window_data.swapchain = swapchain;
    data = SetWindowLongPtrA(thread_data.window, GWLP_USERDATA, (LONG_PTR)&window_data);
    ok(!data, "Got unexpected GWLP_USERDATA %p.\n", (void *)data);

    memset(&mode, 0, sizeof(mode));
    mode.Width = 600;
    mode.Height = 400;
    hr = IDXGISwapChain_ResizeTarget(swapchain, &mode);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGISwapChain_GetDesc(swapchain, &swapchain_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(swapchain_desc.BufferDesc.Width == 800,
            "Got unexpected buffer width %u.\n", swapchain_desc.BufferDesc.Width);
    ok(swapchain_desc.BufferDesc.Height == 600,
            "Got unexpected buffer height %u.\n", swapchain_desc.BufferDesc.Height);

    ret = GetClientRect(swapchain_desc.OutputWindow, &rect);
    ok(ret, "Failed to get client rect.\n");
    ok(rect.right == mode.Width && rect.bottom == mode.Height,
            "Got unexpected client rect %s.\n", wine_dbgstr_rect(&rect));

    /* Win7 testbot reports no output for the swapchain and can't switch to fullscreen. */
    window_data.test_nested_sfs = TRUE;
    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
    ok(hr == S_OK || broken(hr == DXGI_ERROR_UNSUPPORTED), "Got unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fs, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(fs, "Got unexpected fullscreen state %x.\n", fs);
    }
    window_data.test_nested_sfs = FALSE;

    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ret = SetEvent(thread_data.finished);
    ok(ret, "Failed to set event, last error %#lx.\n", GetLastError());
    ret = WaitForSingleObject(thread, INFINITE);
    ok(ret == WAIT_OBJECT_0, "Failed to wait for thread, ret %#x, last error %#lx.\n", ret, GetLastError());
    CloseHandle(thread);
    CloseHandle(thread_data.window_created);
    CloseHandle(thread_data.finished);

    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);

    refcount = IDXGIFactory_Release(factory);
    ok(refcount == !is_d3d12, "Got unexpected refcount %lu.\n", refcount);
}

static void test_inexact_modes(void)
{
    struct swapchain_fullscreen_state initial_state, expected_state;
    DXGI_SWAP_CHAIN_DESC swapchain_desc, result_desc;
    IDXGIOutput *output = NULL;
    IDXGISwapChain *swapchain;
    IDXGIFactory *factory;
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    unsigned int i;
    ULONG refcount;
    HRESULT hr;

    static const struct
    {
        unsigned int width, height;
    }
    sizes[] =
    {
        {101, 101},
        {203, 204},
        {799, 601},
    };

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    swapchain_desc.BufferDesc.Width = 800;
    swapchain_desc.BufferDesc.Height = 600;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 1;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 1;
    swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    swapchain_desc.Windowed = FALSE;
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    memset(&initial_state, 0, sizeof(initial_state));
    capture_fullscreen_state(&initial_state.fullscreen_state, swapchain_desc.OutputWindow);

    hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &swapchain_desc, &swapchain);
    ok(hr == S_OK || hr == DXGI_STATUS_OCCLUDED, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_GetDesc(swapchain, &result_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_GetContainingOutput(swapchain, &output);
    ok(hr == S_OK || broken(hr == DXGI_ERROR_UNSUPPORTED) /* Win 7 testbot */,
            "Got unexpected hr %#lx.\n", hr);
    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);
    if (hr == DXGI_ERROR_UNSUPPORTED)
    {
        win_skip("GetContainingOutput() not supported.\n");
        goto done;
    }
    if (result_desc.Windowed)
    {
        win_skip("Fullscreen not supported.\n");
        goto done;
    }

    check_window_fullscreen_state(swapchain_desc.OutputWindow, &initial_state.fullscreen_state);

    for (i = 0; i < ARRAY_SIZE(sizes); ++i)
    {
        /* Test CreateSwapChain(). */
        swapchain_desc.BufferDesc.Width = sizes[i].width;
        swapchain_desc.BufferDesc.Height = sizes[i].height;
        swapchain_desc.Windowed = FALSE;

        expected_state = initial_state;
        compute_expected_swapchain_fullscreen_state_after_fullscreen_change(&expected_state,
                &swapchain_desc, &initial_state.fullscreen_state.monitor_rect,
                sizes[i].width, sizes[i].height, output);

        hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &swapchain_desc, &swapchain);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        check_swapchain_fullscreen_state(swapchain, &expected_state);
        hr = IDXGISwapChain_GetDesc(swapchain, &result_desc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(result_desc.BufferDesc.Width == sizes[i].width, "Got width %u, expected %u.\n",
                result_desc.BufferDesc.Width, sizes[i].width);
        ok(result_desc.BufferDesc.Height == sizes[i].height, "Got height %u, expected %u.\n",
                result_desc.BufferDesc.Height, sizes[i].height);

        hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        check_swapchain_fullscreen_state(swapchain, &initial_state);

        refcount = IDXGISwapChain_Release(swapchain);
        ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);

        /* Test SetFullscreenState(). */
        swapchain_desc.BufferDesc.Width = sizes[i].width;
        swapchain_desc.BufferDesc.Height = sizes[i].height;
        swapchain_desc.Windowed = TRUE;

        hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &swapchain_desc, &swapchain);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, output);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        check_swapchain_fullscreen_state(swapchain, &expected_state);
        hr = IDXGISwapChain_GetDesc(swapchain, &result_desc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(result_desc.BufferDesc.Width == sizes[i].width, "Got width %u, expected %u.\n",
                result_desc.BufferDesc.Width, sizes[i].width);
        ok(result_desc.BufferDesc.Height == sizes[i].height, "Got height %u, expected %u.\n",
                result_desc.BufferDesc.Height, sizes[i].height);

        hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        check_swapchain_fullscreen_state(swapchain, &initial_state);

        refcount = IDXGISwapChain_Release(swapchain);
        ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);

        /* Test ResizeTarget(). */
        swapchain_desc.BufferDesc.Width = 800;
        swapchain_desc.BufferDesc.Height = 600;
        swapchain_desc.Windowed = TRUE;

        hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &swapchain_desc, &swapchain);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, output);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        swapchain_desc.BufferDesc.Width = sizes[i].width;
        swapchain_desc.BufferDesc.Height = sizes[i].height;
        hr = IDXGISwapChain_ResizeTarget(swapchain, &swapchain_desc.BufferDesc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        check_swapchain_fullscreen_state(swapchain, &expected_state);
        hr = IDXGISwapChain_GetDesc(swapchain, &result_desc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(result_desc.BufferDesc.Width == 800, "Got width %u.\n", result_desc.BufferDesc.Width);
        ok(result_desc.BufferDesc.Height == 600, "Got height %u.\n", result_desc.BufferDesc.Height);

        hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        check_swapchain_fullscreen_state(swapchain, &initial_state);

        refcount = IDXGISwapChain_Release(swapchain);
        ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);
    }

done:
    if (output)
        IDXGIOutput_Release(output);
    IDXGIAdapter_Release(adapter);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
    refcount = IDXGIFactory_Release(factory);
    ok(!refcount, "Factory has %lu references left.\n", refcount);
    DestroyWindow(swapchain_desc.OutputWindow);
}

static void test_create_factory(void)
{
    IUnknown *iface;
    ULONG refcount;
    HRESULT hr;

    iface = (void *)0xdeadbeef;
    hr = CreateDXGIFactory(&IID_IDXGIDevice, (void **)&iface);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#lx.\n", hr);
    ok(!iface, "Got unexpected iface %p.\n", iface);

    hr = CreateDXGIFactory(&IID_IUnknown, (void **)&iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IUnknown_Release(iface);

    hr = CreateDXGIFactory(&IID_IDXGIObject, (void **)&iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IUnknown_Release(iface);

    hr = CreateDXGIFactory(&IID_IDXGIFactory, (void **)&iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_interface(iface, &IID_IDXGIFactory1, FALSE, FALSE);
    IUnknown_Release(iface);

    iface = (void *)0xdeadbeef;
    hr = CreateDXGIFactory(&IID_IDXGIFactory1, (void **)&iface);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#lx.\n", hr);
    ok(!iface, "Got unexpected iface %p.\n", iface);

    iface = NULL;
    hr = CreateDXGIFactory(&IID_IDXGIFactory2, (void **)&iface);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Got unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
    {
        refcount = IUnknown_Release(iface);
        ok(!refcount, "Factory has %lu references left.\n", refcount);
    }

    if (!pCreateDXGIFactory1)
    {
        win_skip("CreateDXGIFactory1 not available.\n");
        return;
    }

    iface = (void *)0xdeadbeef;
    hr = pCreateDXGIFactory1(&IID_IDXGIDevice, (void **)&iface);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#lx.\n", hr);
    ok(!iface, "Got unexpected iface %p.\n", iface);

    hr = pCreateDXGIFactory1(&IID_IUnknown, (void **)&iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IUnknown_Release(iface);

    hr = pCreateDXGIFactory1(&IID_IDXGIObject, (void **)&iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IUnknown_Release(iface);

    hr = pCreateDXGIFactory1(&IID_IDXGIFactory, (void **)&iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_interface(iface, &IID_IDXGIFactory1, TRUE, FALSE);
    refcount = IUnknown_Release(iface);
    ok(!refcount, "Factory has %lu references left.\n", refcount);

    hr = pCreateDXGIFactory1(&IID_IDXGIFactory1, (void **)&iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IUnknown_Release(iface);

    iface = NULL;
    hr = pCreateDXGIFactory1(&IID_IDXGIFactory2, (void **)&iface);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Got unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
    {
        refcount = IUnknown_Release(iface);
        ok(!refcount, "Factory has %lu references left.\n", refcount);
    }

    if (!pCreateDXGIFactory2)
    {
        win_skip("CreateDXGIFactory2 not available.\n");
        return;
    }

    hr = pCreateDXGIFactory2(0, &IID_IDXGIFactory3, (void **)&iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_interface(iface, &IID_IDXGIFactory, TRUE, FALSE);
    check_interface(iface, &IID_IDXGIFactory1, TRUE, FALSE);
    check_interface(iface, &IID_IDXGIFactory2, TRUE, FALSE);
    check_interface(iface, &IID_IDXGIFactory3, TRUE, FALSE);
    /* Not available on all Windows versions. */
    check_interface(iface, &IID_IDXGIFactory4, TRUE, TRUE);
    check_interface(iface, &IID_IDXGIFactory5, TRUE, TRUE);
    refcount = IUnknown_Release(iface);
    ok(!refcount, "Factory has %lu references left.\n", refcount);

    hr = pCreateDXGIFactory2(0, &IID_IDXGIFactory, (void **)&iface);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_interface(iface, &IID_IDXGIFactory, TRUE, FALSE);
    check_interface(iface, &IID_IDXGIFactory1, TRUE, FALSE);
    check_interface(iface, &IID_IDXGIFactory2, TRUE, FALSE);
    check_interface(iface, &IID_IDXGIFactory3, TRUE, FALSE);
    refcount = IUnknown_Release(iface);
    ok(!refcount, "Factory has %lu references left.\n", refcount);
}

static void test_private_data(void)
{
    ULONG refcount, expected_refcount;
    IDXGIDevice *device;
    HRESULT hr;
    IDXGIDevice *test_object;
    IUnknown *ptr;
    static const DWORD data[] = {1, 2, 3, 4};
    UINT size;
    static const GUID dxgi_private_data_test_guid =
    {
        0xfdb37466,
        0x428f,
        0x4edf,
        {0xa3, 0x7f, 0x9b, 0x1d, 0xf4, 0x88, 0xc5, 0xfc}
    };
    static const GUID dxgi_private_data_test_guid2 =
    {
        0x2e5afac2,
        0x87b5,
        0x4c10,
        {0x9b, 0x4b, 0x89, 0xd7, 0xd1, 0x12, 0xe7, 0x2b}
    };

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    test_object = create_device(0);

    /* SetPrivateData with a pointer of NULL has the purpose of FreePrivateData in previous
     * d3d versions. A successful clear returns S_OK. A redundant clear S_FALSE. Setting a
     * NULL interface is not considered a clear but as setting an interface pointer that
     * happens to be NULL. */
    hr = IDXGIDevice_SetPrivateData(device, &dxgi_private_data_test_guid, 0, NULL);
    ok(hr == S_FALSE, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGIDevice_SetPrivateData(device, &dxgi_private_data_test_guid, ~0U, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGIDevice_SetPrivateData(device, &dxgi_private_data_test_guid, ~0U, NULL);
    ok(hr == S_FALSE, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    size = sizeof(ptr) * 2;
    ptr = (IUnknown *)0xdeadbeef;
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid, &size, &ptr);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!ptr, "Got unexpected pointer %p.\n", ptr);
    ok(size == sizeof(IUnknown *), "Got unexpected size %u.\n", size);

    refcount = get_refcount(test_object);
    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid,
            (IUnknown *)test_object);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    expected_refcount = refcount + 1;
    refcount = get_refcount(test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %lu, expected %lu.\n", refcount, expected_refcount);
    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid,
            (IUnknown *)test_object);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    refcount = get_refcount(test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %lu, expected %lu.\n", refcount, expected_refcount);

    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    expected_refcount--;
    refcount = get_refcount(test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %lu, expected %lu.\n", refcount, expected_refcount);

    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid,
            (IUnknown *)test_object);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    size = sizeof(data);
    hr = IDXGIDevice_SetPrivateData(device, &dxgi_private_data_test_guid, size, data);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    refcount = get_refcount(test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %lu, expected %lu.\n", refcount, expected_refcount);
    hr = IDXGIDevice_SetPrivateData(device, &dxgi_private_data_test_guid, 42, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGIDevice_SetPrivateData(device, &dxgi_private_data_test_guid, 42, NULL);
    ok(hr == S_FALSE, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIDevice_SetPrivateDataInterface(device, &dxgi_private_data_test_guid,
            (IUnknown *)test_object);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    expected_refcount++;
    size = 2 * sizeof(ptr);
    ptr = NULL;
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid, &size, &ptr);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(size == sizeof(test_object), "Got unexpected size %u.\n", size);
    expected_refcount++;
    refcount = get_refcount(test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %lu, expected %lu.\n", refcount, expected_refcount);
    if (ptr)
        IUnknown_Release(ptr);
    expected_refcount--;

    ptr = (IUnknown *)0xdeadbeef;
    size = 1;
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid, &size, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(size == sizeof(device), "Got unexpected size %u.\n", size);
    size = 2 * sizeof(ptr);
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid, &size, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(size == sizeof(device), "Got unexpected size %u.\n", size);
    refcount = get_refcount(test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %lu, expected %lu.\n", refcount, expected_refcount);

    size = 1;
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid, &size, &ptr);
    ok(hr == DXGI_ERROR_MORE_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(size == sizeof(device), "Got unexpected size %u.\n", size);
    ok(ptr == (IUnknown *)0xdeadbeef, "Got unexpected pointer %p.\n", ptr);
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid2, NULL, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    size = 0xdeadbabe;
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid2, &size, &ptr);
    ok(hr == DXGI_ERROR_NOT_FOUND, "Got unexpected hr %#lx.\n", hr);
    ok(size == 0, "Got unexpected size %u.\n", size);
    ok(ptr == (IUnknown *)0xdeadbeef, "Got unexpected pointer %p.\n", ptr);
    hr = IDXGIDevice_GetPrivateData(device, &dxgi_private_data_test_guid, NULL, &ptr);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    ok(ptr == (IUnknown *)0xdeadbeef, "Got unexpected pointer %p.\n", ptr);

    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
    refcount = IDXGIDevice_Release(test_object);
    ok(!refcount, "Test object has %lu references left.\n", refcount);
}

#define check_surface_desc(a, b) check_surface_desc_(__LINE__, a, b)
static void check_surface_desc_(unsigned int line, IDXGISurface *surface,
        const DXGI_SWAP_CHAIN_DESC *swapchain_desc)
{
    DXGI_SURFACE_DESC surface_desc;
    HRESULT hr;

    hr = IDXGISurface_GetDesc(surface, &surface_desc);
    ok_(__FILE__, line)(hr == S_OK, "Failed to get surface desc, hr %#lx.\n", hr);
    ok_(__FILE__, line)(surface_desc.Width == swapchain_desc->BufferDesc.Width,
            "Got Width %u, expected %u.\n", surface_desc.Width, swapchain_desc->BufferDesc.Width);
    ok_(__FILE__, line)(surface_desc.Height == swapchain_desc->BufferDesc.Height,
            "Got Height %u, expected %u.\n", surface_desc.Height, swapchain_desc->BufferDesc.Height);
    ok_(__FILE__, line)(surface_desc.Format == swapchain_desc->BufferDesc.Format,
            "Got Format %#x, expected %#x.\n", surface_desc.Format, swapchain_desc->BufferDesc.Format);
    ok_(__FILE__, line)(surface_desc.SampleDesc.Count == 1,
            "Got unexpected SampleDesc.Count %u.\n", surface_desc.SampleDesc.Count);
    ok_(__FILE__, line)(!surface_desc.SampleDesc.Quality,
            "Got unexpected SampleDesc.Quality %u.\n", surface_desc.SampleDesc.Quality);
}

#define check_texture_desc(a, b) check_texture_desc_(__LINE__, a, b)
static void check_texture_desc_(unsigned int line, ID3D10Texture2D *texture,
        const DXGI_SWAP_CHAIN_DESC *swapchain_desc)
{
    D3D10_TEXTURE2D_DESC texture_desc;

    ID3D10Texture2D_GetDesc(texture, &texture_desc);
    ok_(__FILE__, line)(texture_desc.Width == swapchain_desc->BufferDesc.Width,
            "Got Width %u, expected %u.\n", texture_desc.Width, swapchain_desc->BufferDesc.Width);
    ok_(__FILE__, line)(texture_desc.Height == swapchain_desc->BufferDesc.Height,
            "Got Height %u, expected %u.\n", texture_desc.Height, swapchain_desc->BufferDesc.Height);
    ok_(__FILE__, line)(texture_desc.MipLevels == 1, "Got unexpected MipLevels %u.\n", texture_desc.MipLevels);
    ok_(__FILE__, line)(texture_desc.ArraySize == 1, "Got unexpected ArraySize %u.\n", texture_desc.ArraySize);
    ok_(__FILE__, line)(texture_desc.Format == swapchain_desc->BufferDesc.Format,
            "Got Format %#x, expected %#x.\n", texture_desc.Format, swapchain_desc->BufferDesc.Format);
    ok_(__FILE__, line)(texture_desc.SampleDesc.Count == 1,
            "Got unexpected SampleDesc.Count %u.\n", texture_desc.SampleDesc.Count);
    ok_(__FILE__, line)(!texture_desc.SampleDesc.Quality,
            "Got unexpected SampleDesc.Quality %u.\n", texture_desc.SampleDesc.Quality);
    ok_(__FILE__, line)(texture_desc.Usage == D3D10_USAGE_DEFAULT,
            "Got unexpected Usage %#x.\n", texture_desc.Usage);
    ok_(__FILE__, line)(texture_desc.BindFlags == D3D10_BIND_RENDER_TARGET,
            "Got unexpected BindFlags %#x.\n", texture_desc.BindFlags);
    ok_(__FILE__, line)(!texture_desc.CPUAccessFlags,
            "Got unexpected CPUAccessFlags %#x.\n", texture_desc.CPUAccessFlags);
    ok_(__FILE__, line)(!texture_desc.MiscFlags, "Got unexpected MiscFlags %#x.\n", texture_desc.MiscFlags);
}

#define check_resource_desc(a, b) check_resource_desc_(__LINE__, a, b)
static void check_resource_desc_(unsigned int line, ID3D12Resource *resource,
        const DXGI_SWAP_CHAIN_DESC *swapchain_desc)
{
    D3D12_RESOURCE_DESC resource_desc;

    resource_desc = ID3D12Resource_GetDesc(resource);
    ok_(__FILE__, line)(resource_desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            "Got unexpected Dimension %#x.\n", resource_desc.Dimension);
    ok_(__FILE__, line)(resource_desc.Width == swapchain_desc->BufferDesc.Width, "Got Width %s, expected %u.\n",
            wine_dbgstr_longlong(resource_desc.Width), swapchain_desc->BufferDesc.Width);
    ok_(__FILE__, line)(resource_desc.Height == swapchain_desc->BufferDesc.Height,
            "Got Height %u, expected %u.\n", resource_desc.Height, swapchain_desc->BufferDesc.Height);
    ok_(__FILE__, line)(resource_desc.DepthOrArraySize == 1,
            "Got unexpected DepthOrArraySize %u.\n", resource_desc.DepthOrArraySize);
    ok_(__FILE__, line)(resource_desc.MipLevels == 1,
            "Got unexpected MipLevels %u.\n", resource_desc.MipLevels);
    ok_(__FILE__, line)(resource_desc.Format == swapchain_desc->BufferDesc.Format,
            "Got Format %#x, expected %#x.\n", resource_desc.Format, swapchain_desc->BufferDesc.Format);
    ok_(__FILE__, line)(resource_desc.SampleDesc.Count == 1,
            "Got unexpected SampleDesc.Count %u.\n", resource_desc.SampleDesc.Count);
    ok_(__FILE__, line)(!resource_desc.SampleDesc.Quality,
            "Got unexpected SampleDesc.Quality %u.\n", resource_desc.SampleDesc.Quality);
    ok_(__FILE__, line)(resource_desc.Layout == D3D12_TEXTURE_LAYOUT_UNKNOWN,
            "Got unexpected Layout %#x.\n", resource_desc.Layout);
}

static void test_swapchain_resize(IUnknown *device, BOOL is_d3d12)
{
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    DXGI_SWAP_EFFECT swap_effect;
    IDXGIResource *dxgi_resource;
    IDXGISwapChain3 *swapchain3;
    IUnknown *present_queue[2];
    IDXGISwapChain *swapchain;
    ID3D12Resource *resource;
    ID3D10Texture2D *texture;
    HRESULT hr, expected_hr;
    IDXGISurface *surface;
    IDXGIFactory *factory;
    RECT client_rect, r;
    UINT node_mask[2];
    ULONG refcount;
    HWND window;
    BOOL ret;

    get_factory(device, is_d3d12, &factory);

    window = create_window();
    ret = GetClientRect(window, &client_rect);
    ok(ret, "Failed to get client rect.\n");

    swap_effect = is_d3d12 ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD;

    swapchain_desc.BufferDesc.Width = 640;
    swapchain_desc.BufferDesc.Height = 480;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 1;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 2;
    swapchain_desc.OutputWindow = window;
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = swap_effect;
    swapchain_desc.Flags = 0;

    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    expected_hr = is_d3d12 ? E_NOINTERFACE : S_OK;
    hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_IDXGIResource, (void **)&dxgi_resource);
    ok(hr == expected_hr, "Got unexpected hr %#lx, expected %#lx.\n", hr, expected_hr);
    ok(!dxgi_resource || hr == S_OK, "Got unexpected pointer %p.\n", dxgi_resource);
    hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_IDXGISurface, (void **)&surface);
    ok(hr == expected_hr, "Got unexpected hr %#lx, expected %#lx.\n", hr, expected_hr);
    ok(!surface || hr == S_OK, "Got unexpected pointer %p.\n", surface);
    hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_ID3D10Texture2D, (void **)&texture);
    ok(hr == expected_hr, "Got unexpected hr %#lx, expected %#lx.\n", hr, expected_hr);
    ok(!texture || hr == S_OK, "Got unexpected pointer %p.\n", texture);

    expected_hr = is_d3d12 ? S_OK : E_NOINTERFACE;
    hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_ID3D12Resource, (void **)&resource);
    ok(hr == expected_hr, "Got unexpected hr %#lx, expected %#lx.\n", hr, expected_hr);
    ok(!resource || hr == S_OK, "Got unexpected pointer %p.\n", resource);

    ret = GetClientRect(window, &r);
    ok(ret, "Failed to get client rect.\n");
    ok(EqualRect(&r, &client_rect), "Got unexpected rect %s, expected %s.\n",
            wine_dbgstr_rect(&r), wine_dbgstr_rect(&client_rect));

    memset(&swapchain_desc, 0, sizeof(swapchain_desc));
    hr = IDXGISwapChain_GetDesc(swapchain, &swapchain_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(swapchain_desc.BufferDesc.Width == 640,
            "Got unexpected BufferDesc.Width %u.\n", swapchain_desc.BufferDesc.Width);
    ok(swapchain_desc.BufferDesc.Height == 480,
            "Got unexpected bufferDesc.Height %u.\n", swapchain_desc.BufferDesc.Height);
    ok(swapchain_desc.BufferDesc.RefreshRate.Numerator == 60,
            "Got unexpected BufferDesc.RefreshRate.Numerator %u.\n",
            swapchain_desc.BufferDesc.RefreshRate.Numerator);
    ok(swapchain_desc.BufferDesc.RefreshRate.Denominator == 1,
            "Got unexpected BufferDesc.RefreshRate.Denominator %u.\n",
            swapchain_desc.BufferDesc.RefreshRate.Denominator);
    ok(swapchain_desc.BufferDesc.Format == DXGI_FORMAT_R8G8B8A8_UNORM,
            "Got unexpected BufferDesc.Format %#x.\n", swapchain_desc.BufferDesc.Format);
    ok(swapchain_desc.BufferDesc.ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
            "Got unexpected BufferDesc.ScanlineOrdering %#x.\n", swapchain_desc.BufferDesc.ScanlineOrdering);
    ok(swapchain_desc.BufferDesc.Scaling == DXGI_MODE_SCALING_UNSPECIFIED,
            "Got unexpected BufferDesc.Scaling %#x.\n", swapchain_desc.BufferDesc.Scaling);
    ok(swapchain_desc.SampleDesc.Count == 1,
            "Got unexpected SampleDesc.Count %u.\n", swapchain_desc.SampleDesc.Count);
    ok(!swapchain_desc.SampleDesc.Quality,
            "Got unexpected SampleDesc.Quality %u.\n", swapchain_desc.SampleDesc.Quality);
    ok(swapchain_desc.BufferUsage == DXGI_USAGE_RENDER_TARGET_OUTPUT,
            "Got unexpected BufferUsage %#x.\n", swapchain_desc.BufferUsage);
    ok(swapchain_desc.BufferCount == 2,
            "Got unexpected BufferCount %u.\n", swapchain_desc.BufferCount);
    ok(swapchain_desc.OutputWindow == window,
            "Got unexpected OutputWindow %p, expected %p.\n", swapchain_desc.OutputWindow, window);
    ok(swapchain_desc.Windowed,
            "Got unexpected Windowed %#x.\n", swapchain_desc.Windowed);
    ok(swapchain_desc.SwapEffect == swap_effect,
            "Got unexpected SwapEffect %#x.\n", swapchain_desc.SwapEffect);
    ok(!swapchain_desc.Flags,
            "Got unexpected Flags %#x.\n", swapchain_desc.Flags);

    if (surface)
        check_surface_desc(surface, &swapchain_desc);
    if (texture)
        check_texture_desc(texture, &swapchain_desc);
    if (resource)
        check_resource_desc(resource, &swapchain_desc);

    hr = IDXGISwapChain_ResizeBuffers(swapchain, 2, 320, 240, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);

    ret = GetClientRect(window, &r);
    ok(ret, "Failed to get client rect.\n");
    ok(EqualRect(&r, &client_rect), "Got unexpected rect %s, expected %s.\n",
            wine_dbgstr_rect(&r), wine_dbgstr_rect(&client_rect));

    memset(&swapchain_desc, 0, sizeof(swapchain_desc));
    hr = IDXGISwapChain_GetDesc(swapchain, &swapchain_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(swapchain_desc.BufferDesc.Width == 640,
            "Got unexpected BufferDesc.Width %u.\n", swapchain_desc.BufferDesc.Width);
    ok(swapchain_desc.BufferDesc.Height == 480,
            "Got unexpected bufferDesc.Height %u.\n", swapchain_desc.BufferDesc.Height);
    ok(swapchain_desc.BufferDesc.RefreshRate.Numerator == 60,
            "Got unexpected BufferDesc.RefreshRate.Numerator %u.\n",
            swapchain_desc.BufferDesc.RefreshRate.Numerator);
    ok(swapchain_desc.BufferDesc.RefreshRate.Denominator == 1,
            "Got unexpected BufferDesc.RefreshRate.Denominator %u.\n",
            swapchain_desc.BufferDesc.RefreshRate.Denominator);
    ok(swapchain_desc.BufferDesc.Format == DXGI_FORMAT_R8G8B8A8_UNORM,
            "Got unexpected BufferDesc.Format %#x.\n", swapchain_desc.BufferDesc.Format);
    ok(swapchain_desc.BufferDesc.ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
            "Got unexpected BufferDesc.ScanlineOrdering %#x.\n", swapchain_desc.BufferDesc.ScanlineOrdering);
    ok(swapchain_desc.BufferDesc.Scaling == DXGI_MODE_SCALING_UNSPECIFIED,
            "Got unexpected BufferDesc.Scaling %#x.\n", swapchain_desc.BufferDesc.Scaling);
    ok(swapchain_desc.SampleDesc.Count == 1,
            "Got unexpected SampleDesc.Count %u.\n", swapchain_desc.SampleDesc.Count);
    ok(!swapchain_desc.SampleDesc.Quality,
            "Got unexpected SampleDesc.Quality %u.\n", swapchain_desc.SampleDesc.Quality);
    ok(swapchain_desc.BufferUsage == DXGI_USAGE_RENDER_TARGET_OUTPUT,
            "Got unexpected BufferUsage %#x.\n", swapchain_desc.BufferUsage);
    ok(swapchain_desc.BufferCount == 2,
            "Got unexpected BufferCount %u.\n", swapchain_desc.BufferCount);
    ok(swapchain_desc.OutputWindow == window,
            "Got unexpected OutputWindow %p, expected %p.\n", swapchain_desc.OutputWindow, window);
    ok(swapchain_desc.Windowed,
            "Got unexpected Windowed %#x.\n", swapchain_desc.Windowed);
    ok(swapchain_desc.SwapEffect == swap_effect,
            "Got unexpected SwapEffect %#x.\n", swapchain_desc.SwapEffect);
    ok(!swapchain_desc.Flags,
            "Got unexpected Flags %#x.\n", swapchain_desc.Flags);

    if (surface)
    {
        check_surface_desc(surface, &swapchain_desc);
        IDXGISurface_Release(surface);
    }
    if (texture)
    {
        check_texture_desc(texture, &swapchain_desc);
        ID3D10Texture2D_Release(texture);
    }
    if (resource)
    {
        check_resource_desc(resource, &swapchain_desc);
        ID3D12Resource_Release(resource);
    }
    if (dxgi_resource)
        IDXGIResource_Release(dxgi_resource);

    hr = IDXGISwapChain_ResizeBuffers(swapchain, 2, 320, 240, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    expected_hr = is_d3d12 ? E_NOINTERFACE : S_OK;
    hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_IDXGIResource, (void **)&dxgi_resource);
    ok(hr == expected_hr, "Got unexpected hr %#lx, expected %#lx.\n", hr, expected_hr);
    ok(!surface || hr == S_OK, "Got unexpected pointer %p.\n", surface);
    hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_IDXGISurface, (void **)&surface);
    ok(hr == expected_hr, "Got unexpected hr %#lx, expected %#lx.\n", hr, expected_hr);
    ok(!surface || hr == S_OK, "Got unexpected pointer %p.\n", surface);
    hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_ID3D10Texture2D, (void **)&texture);
    ok(hr == expected_hr, "Got unexpected hr %#lx, expected %#lx.\n", hr, expected_hr);
    ok(!texture || hr == S_OK, "Got unexpected pointer %p.\n", texture);

    expected_hr = is_d3d12 ? S_OK : E_NOINTERFACE;
    hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_ID3D12Resource, (void **)&resource);
    ok(hr == expected_hr, "Got unexpected hr %#lx, expected %#lx.\n", hr, expected_hr);
    ok(!resource || hr == S_OK, "Got unexpected pointer %p.\n", resource);

    ret = GetClientRect(window, &r);
    ok(ret, "Failed to get client rect.\n");
    ok(EqualRect(&r, &client_rect), "Got unexpected rect %s, expected %s.\n",
            wine_dbgstr_rect(&r), wine_dbgstr_rect(&client_rect));

    memset(&swapchain_desc, 0, sizeof(swapchain_desc));
    hr = IDXGISwapChain_GetDesc(swapchain, &swapchain_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(swapchain_desc.BufferDesc.Width == 320,
            "Got unexpected BufferDesc.Width %u.\n", swapchain_desc.BufferDesc.Width);
    ok(swapchain_desc.BufferDesc.Height == 240,
            "Got unexpected bufferDesc.Height %u.\n", swapchain_desc.BufferDesc.Height);
    ok(swapchain_desc.BufferDesc.RefreshRate.Numerator == 60,
            "Got unexpected BufferDesc.RefreshRate.Numerator %u.\n",
            swapchain_desc.BufferDesc.RefreshRate.Numerator);
    ok(swapchain_desc.BufferDesc.RefreshRate.Denominator == 1,
            "Got unexpected BufferDesc.RefreshRate.Denominator %u.\n",
            swapchain_desc.BufferDesc.RefreshRate.Denominator);
    ok(swapchain_desc.BufferDesc.Format == DXGI_FORMAT_B8G8R8A8_UNORM,
            "Got unexpected BufferDesc.Format %#x.\n", swapchain_desc.BufferDesc.Format);
    ok(swapchain_desc.BufferDesc.ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
            "Got unexpected BufferDesc.ScanlineOrdering %#x.\n", swapchain_desc.BufferDesc.ScanlineOrdering);
    ok(swapchain_desc.BufferDesc.Scaling == DXGI_MODE_SCALING_UNSPECIFIED,
            "Got unexpected BufferDesc.Scaling %#x.\n", swapchain_desc.BufferDesc.Scaling);
    ok(swapchain_desc.SampleDesc.Count == 1,
            "Got unexpected SampleDesc.Count %u.\n", swapchain_desc.SampleDesc.Count);
    ok(!swapchain_desc.SampleDesc.Quality,
            "Got unexpected SampleDesc.Quality %u.\n", swapchain_desc.SampleDesc.Quality);
    ok(swapchain_desc.BufferUsage == DXGI_USAGE_RENDER_TARGET_OUTPUT,
            "Got unexpected BufferUsage %#x.\n", swapchain_desc.BufferUsage);
    ok(swapchain_desc.BufferCount == 2,
            "Got unexpected BufferCount %u.\n", swapchain_desc.BufferCount);
    ok(swapchain_desc.OutputWindow == window,
            "Got unexpected OutputWindow %p, expected %p.\n", swapchain_desc.OutputWindow, window);
    ok(swapchain_desc.Windowed,
            "Got unexpected Windowed %#x.\n", swapchain_desc.Windowed);
    ok(swapchain_desc.SwapEffect == swap_effect,
            "Got unexpected SwapEffect %#x.\n", swapchain_desc.SwapEffect);
    ok(!swapchain_desc.Flags,
            "Got unexpected Flags %#x.\n", swapchain_desc.Flags);

    if (surface)
    {
        check_surface_desc(surface, &swapchain_desc);
        IDXGISurface_Release(surface);
    }
    if (texture)
    {
        check_texture_desc(texture, &swapchain_desc);
        ID3D10Texture2D_Release(texture);
    }
    if (resource)
    {
        check_resource_desc(resource, &swapchain_desc);
        ID3D12Resource_Release(resource);
    }
    if (dxgi_resource)
        IDXGIResource_Release(dxgi_resource);

    hr = IDXGISwapChain_ResizeBuffers(swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    memset(&swapchain_desc, 0, sizeof(swapchain_desc));
    hr = IDXGISwapChain_GetDesc(swapchain, &swapchain_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(swapchain_desc.BufferDesc.Width == client_rect.right - client_rect.left,
            "Got unexpected BufferDesc.Width %u, expected %lu.\n",
            swapchain_desc.BufferDesc.Width, client_rect.right - client_rect.left);
    ok(swapchain_desc.BufferDesc.Height == client_rect.bottom - client_rect.top,
            "Got unexpected bufferDesc.Height %u, expected %lu.\n",
            swapchain_desc.BufferDesc.Height, client_rect.bottom - client_rect.top);
    ok(swapchain_desc.BufferDesc.RefreshRate.Numerator == 60,
            "Got unexpected BufferDesc.RefreshRate.Numerator %u.\n",
            swapchain_desc.BufferDesc.RefreshRate.Numerator);
    ok(swapchain_desc.BufferDesc.RefreshRate.Denominator == 1,
            "Got unexpected BufferDesc.RefreshRate.Denominator %u.\n",
            swapchain_desc.BufferDesc.RefreshRate.Denominator);
    ok(swapchain_desc.BufferDesc.Format == DXGI_FORMAT_B8G8R8A8_UNORM,
            "Got unexpected BufferDesc.Format %#x.\n", swapchain_desc.BufferDesc.Format);
    ok(swapchain_desc.BufferDesc.ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
            "Got unexpected BufferDesc.ScanlineOrdering %#x.\n", swapchain_desc.BufferDesc.ScanlineOrdering);
    ok(swapchain_desc.BufferDesc.Scaling == DXGI_MODE_SCALING_UNSPECIFIED,
            "Got unexpected BufferDesc.Scaling %#x.\n", swapchain_desc.BufferDesc.Scaling);
    ok(swapchain_desc.SampleDesc.Count == 1,
            "Got unexpected SampleDesc.Count %u.\n", swapchain_desc.SampleDesc.Count);
    ok(!swapchain_desc.SampleDesc.Quality,
            "Got unexpected SampleDesc.Quality %u.\n", swapchain_desc.SampleDesc.Quality);
    ok(swapchain_desc.BufferUsage == DXGI_USAGE_RENDER_TARGET_OUTPUT,
            "Got unexpected BufferUsage %#x.\n", swapchain_desc.BufferUsage);
    ok(swapchain_desc.BufferCount == 2,
            "Got unexpected BufferCount %u.\n", swapchain_desc.BufferCount);
    ok(swapchain_desc.OutputWindow == window,
            "Got unexpected OutputWindow %p, expected %p.\n", swapchain_desc.OutputWindow, window);
    ok(swapchain_desc.Windowed,
            "Got unexpected Windowed %#x.\n", swapchain_desc.Windowed);
    ok(swapchain_desc.SwapEffect == swap_effect,
            "Got unexpected SwapEffect %#x.\n", swapchain_desc.SwapEffect);
    ok(!swapchain_desc.Flags,
            "Got unexpected Flags %#x.\n", swapchain_desc.Flags);

    node_mask[0] = 1;
    node_mask[1] = 1;
    present_queue[0] = device;
    present_queue[1] = device;
    if (IDXGISwapChain_QueryInterface(swapchain, &IID_IDXGISwapChain3, (void **)&swapchain3) == E_NOINTERFACE)
    {
        win_skip("IDXGISwapChain3 is not supported.\n");
    }
    else if (!is_d3d12)
    {
        hr = IDXGISwapChain3_ResizeBuffers1(swapchain3, 2, 320, 240,
                DXGI_FORMAT_B8G8R8A8_UNORM, 0, node_mask, present_queue);
        todo_wine ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
        IDXGISwapChain3_Release(swapchain3);
    }
    else
    {
        hr = IDXGISwapChain3_ResizeBuffers1(swapchain3, 2, 320, 240,
                DXGI_FORMAT_B8G8R8A8_UNORM, 0, node_mask, present_queue);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = IDXGISwapChain3_ResizeBuffers1(swapchain3, 2, 320, 240,
                DXGI_FORMAT_B8G8R8A8_UNORM, 0, NULL, present_queue);
        ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
        hr = IDXGISwapChain3_ResizeBuffers1(swapchain3, 2, 320, 240, DXGI_FORMAT_B8G8R8A8_UNORM, 0, NULL, NULL);
        ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
        hr = IDXGISwapChain3_ResizeBuffers1(swapchain3, 0, 320, 240, DXGI_FORMAT_B8G8R8A8_UNORM, 0, NULL, NULL);
        ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
        node_mask[0] = 2;
        node_mask[1] = 2;
        hr = IDXGISwapChain3_ResizeBuffers1(swapchain3, 2, 320, 240,
                DXGI_FORMAT_B8G8R8A8_UNORM, 0, node_mask, present_queue);
        ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
        /* Windows validates node masks even when the buffer count is zero.
         * It defaults to the current buffer count. NULL queues cause some
         * Windows versions to crash. */
        hr = IDXGISwapChain3_ResizeBuffers1(swapchain3, 0, 320, 240,
                DXGI_FORMAT_B8G8R8A8_UNORM, 0, node_mask, present_queue);
        ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
        IDXGISwapChain3_Release(swapchain3);
    }

    IDXGISwapChain_Release(swapchain);
    DestroyWindow(window);
    refcount = IDXGIFactory_Release(factory);
    ok(refcount == !is_d3d12, "Got unexpected refcount %lu.\n", refcount);
}

static void test_swapchain_parameters(void)
{
    DXGI_USAGE usage, expected_usage, broken_usage;
    D3D10_TEXTURE2D_DESC d3d10_texture_desc;
    D3D11_TEXTURE2D_DESC d3d11_texture_desc;
    unsigned int expected_bind_flags;
    ID3D10Texture2D *d3d10_texture;
    ID3D11Texture2D *d3d11_texture;
    DXGI_SWAP_CHAIN_DESC desc;
    IDXGISwapChain *swapchain;
    IDXGIResource *resource;
    IDXGIAdapter *adapter;
    IDXGIFactory *factory;
    IDXGIDevice *device;
    unsigned int i, j;
    ULONG refcount;
    IUnknown *obj;
    HWND window;
    HRESULT hr;

    static const struct
    {
        BOOL windowed;
        UINT buffer_count;
        DXGI_SWAP_EFFECT swap_effect;
        HRESULT hr, vista_hr;
        UINT highest_accessible_buffer;
    }
    tests[] =
    {
        /* 0 */
        {TRUE,   1, DXGI_SWAP_EFFECT_DISCARD,         S_OK,                    S_OK,                     0},
        {TRUE,   2, DXGI_SWAP_EFFECT_DISCARD,         S_OK,                    S_OK,                     0},
        {TRUE,   1, DXGI_SWAP_EFFECT_SEQUENTIAL,      S_OK,                    S_OK,                     0},
        {TRUE,   2, DXGI_SWAP_EFFECT_SEQUENTIAL,      S_OK,                    S_OK,                     1},
        {TRUE,   3, DXGI_SWAP_EFFECT_SEQUENTIAL,      S_OK,                    S_OK,                     2},
        /* 5 */
        {TRUE,   0, 2 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   1, 2 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   2, 2 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   0, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   1, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        /* 10 */
        {TRUE,   2, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, S_OK,                    DXGI_ERROR_INVALID_CALL,  1},
        {TRUE,   3, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, S_OK,                    DXGI_ERROR_INVALID_CALL,  2},
        {TRUE,   0, DXGI_SWAP_EFFECT_FLIP_DISCARD,    DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   1, DXGI_SWAP_EFFECT_FLIP_DISCARD,    DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   2, DXGI_SWAP_EFFECT_FLIP_DISCARD,    S_OK,                    DXGI_ERROR_INVALID_CALL,  0},
        /* 15 */
        {TRUE,   0, 5 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   1, 5 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   2, 5 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,  16, DXGI_SWAP_EFFECT_DISCARD,         S_OK,                    S_OK,                     0},
        {TRUE,  16, DXGI_SWAP_EFFECT_SEQUENTIAL,      S_OK,                    S_OK,                    15},
        /* 20 */
        {TRUE,  16, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, S_OK,                    DXGI_ERROR_INVALID_CALL, 15},
        {TRUE,  16, DXGI_SWAP_EFFECT_FLIP_DISCARD,    S_OK,                    DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,  17, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  1, DXGI_SWAP_EFFECT_DISCARD,         S_OK,                    S_OK,                     0},
        {FALSE,  2, DXGI_SWAP_EFFECT_DISCARD,         S_OK,                    S_OK,                     0},
        /* 25 */
        {FALSE,  1, DXGI_SWAP_EFFECT_SEQUENTIAL,      S_OK,                    S_OK,                     0},
        {FALSE,  2, DXGI_SWAP_EFFECT_SEQUENTIAL,      S_OK,                    S_OK,                     1},
        {FALSE,  3, DXGI_SWAP_EFFECT_SEQUENTIAL,      S_OK,                    S_OK,                     2},
        {FALSE,  0, 2 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  1, 2 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        /* 30 */
        {FALSE,  2, 2 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  0, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  1, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  2, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, S_OK,                    DXGI_ERROR_INVALID_CALL,  1},
        {FALSE,  3, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, S_OK,                    DXGI_ERROR_INVALID_CALL,  2},
        /* 35 */
        {FALSE,  0, DXGI_SWAP_EFFECT_FLIP_DISCARD,    DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  1, DXGI_SWAP_EFFECT_FLIP_DISCARD,    DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  2, DXGI_SWAP_EFFECT_FLIP_DISCARD,    S_OK,                    DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  0, 5 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  1, 5 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        /* 40 */
        {FALSE,  2, 5 /* undefined */,                DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE, 16, DXGI_SWAP_EFFECT_DISCARD,         S_OK,                    S_OK,                     0},
        {FALSE, 16, DXGI_SWAP_EFFECT_SEQUENTIAL,      S_OK,                    S_OK,                    15},
        {FALSE, 16, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, S_OK,                    DXGI_ERROR_INVALID_CALL, 15},
        {FALSE, 17, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        /* 45 */
        {FALSE, 17, DXGI_SWAP_EFFECT_FLIP_DISCARD,    DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},

        /* The following test fails on Nvidia with E_OUTOFMEMORY and leaks device references in the
         * process. Disable it for now.
        {FALSE, 16, DXGI_SWAP_EFFECT_FLIP_DISCARD,    S_OK,                    DXGI_ERROR_INVALID_CALL,  0},
         */

        /* The following tests crash on Win10 1909
        {TRUE,   0, DXGI_SWAP_EFFECT_DISCARD,         DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,   0, DXGI_SWAP_EFFECT_SEQUENTIAL,      DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,  17, DXGI_SWAP_EFFECT_DISCARD,         DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,  17, DXGI_SWAP_EFFECT_SEQUENTIAL,      DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {TRUE,  17, DXGI_SWAP_EFFECT_DISCARD,         DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  0, DXGI_SWAP_EFFECT_DISCARD,         DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE,  0, DXGI_SWAP_EFFECT_SEQUENTIAL,      DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE, 17, DXGI_SWAP_EFFECT_DISCARD,         DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        {FALSE, 17, DXGI_SWAP_EFFECT_SEQUENTIAL,      DXGI_ERROR_INVALID_CALL, DXGI_ERROR_INVALID_CALL,  0},
        */
    };
    static const DXGI_USAGE usage_tests[] =
    {
        0,
        DXGI_USAGE_BACK_BUFFER,
        DXGI_USAGE_SHADER_INPUT,
        DXGI_USAGE_RENDER_TARGET_OUTPUT,
        DXGI_USAGE_DISCARD_ON_PRESENT,
        DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER,
        DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_DISCARD_ON_PRESENT,
        DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_DISCARD_ON_PRESENT,
        DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT,
        DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_DISCARD_ON_PRESENT,
    };

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }
    window = create_window();

    hr = IDXGIDevice_QueryInterface(device, &IID_IUnknown, (void **)&obj);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IDXGIAdapter_Release(adapter);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        memset(&desc, 0, sizeof(desc));
        desc.BufferDesc.Width = registry_mode.dmPelsWidth;
        desc.BufferDesc.Height = registry_mode.dmPelsHeight;
        desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.OutputWindow = window;

        desc.Windowed = tests[i].windowed;
        desc.BufferCount = tests[i].buffer_count;
        desc.SwapEffect = tests[i].swap_effect;

        hr = IDXGIFactory_CreateSwapChain(factory, obj, &desc, &swapchain);
        ok(hr == tests[i].hr || broken(hr == tests[i].vista_hr)
                || (SUCCEEDED(tests[i].hr) && hr == DXGI_STATUS_OCCLUDED),
                "Got unexpected hr %#lx, test %u.\n", hr, i);
        if (FAILED(hr))
            continue;

        hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_IDXGIResource, (void **)&resource);
        ok(hr == S_OK, "Got unexpected hr %#lx, test %u.\n", hr, i);

        expected_usage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER;
        hr = IDXGIResource_GetUsage(resource, &usage);
        ok(hr == S_OK, "Got unexpected hr %#lx, test %u.\n", hr, i);
        ok((usage & expected_usage) == expected_usage, "Got usage %x, expected %x, test %u.\n",
                usage, expected_usage, i);

        IDXGIResource_Release(resource);

        hr = IDXGISwapChain_GetDesc(swapchain, &desc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        for (j = 1; j <= tests[i].highest_accessible_buffer; j++)
        {
            hr = IDXGISwapChain_GetBuffer(swapchain, j, &IID_IDXGIResource, (void **)&resource);
            ok(hr == S_OK, "Got unexpected hr %#lx, test %u, buffer %u.\n", hr, i, j);

            /* Buffers > 0 are supposed to be read only. This is the case except that in
             * fullscreen mode on Windows <= 8 the last backbuffer (BufferCount - 1) is
             * writable. This is not the case if an unsupported refresh rate is passed
             * for some reason, probably because the invalid refresh rate triggers a
             * kinda-sorta windowed mode.
             *
             * On Windows 10 all buffers > 0 are read-only. Mark the earlier behavior
             * broken.
             *
             * This last buffer acts as a shadow frontbuffer. Writing to it doesn't show
             * the draw on the screen right away (Aero on or off doesn't matter), but
             * Present with DXGI_PRESENT_DO_NOT_SEQUENCE will show the modifications.
             *
             * Note that if the application doesn't have focus creating a fullscreen
             * swapchain returns DXGI_STATUS_OCCLUDED and we get a windowed swapchain,
             * so use the Windowed property of the swapchain that was actually created. */
            expected_usage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_READ_ONLY;
            broken_usage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER;

            if (desc.Windowed || j < tests[i].highest_accessible_buffer)
                broken_usage |= DXGI_USAGE_READ_ONLY;

            hr = IDXGIResource_GetUsage(resource, &usage);
            ok(hr == S_OK, "Got unexpected hr %#lx, test %u, buffer %u.\n", hr, i, j);
            ok(usage == expected_usage || broken(usage == broken_usage),
                    "Got usage %x, expected %x, test %u, buffer %u.\n",
                    usage, expected_usage, i, j);

            IDXGIResource_Release(resource);
        }

        if (strcmp(winetest_platform, "wine"))
        {
            hr = IDXGISwapChain_GetBuffer(swapchain, j, &IID_IDXGIResource, (void **)&resource);
            ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx, test %u.\n", hr, i);
        }

        hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx, test %u.\n", hr, i);

        IDXGISwapChain_Release(swapchain);
    }

    for (i = 0; i < ARRAY_SIZE(usage_tests); ++i)
    {
        usage = usage_tests[i];

        memset(&desc, 0, sizeof(desc));
        desc.BufferDesc.Width = registry_mode.dmPelsWidth;
        desc.BufferDesc.Height = registry_mode.dmPelsHeight;
        desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.BufferUsage = usage;
        desc.BufferCount = 1;
        desc.OutputWindow = window;
        desc.Windowed = TRUE;
        desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        hr = IDXGIFactory_CreateSwapChain(factory, obj, &desc, &swapchain);
        ok(hr == S_OK, "Got unexpected hr %#lx, test %u.\n", hr, i);

        hr = IDXGISwapChain_GetDesc(swapchain, &desc);
        ok(hr == S_OK, "Got unexpected hr %#lx, test %u.\n", hr, i);
        todo_wine_if(usage & ~(DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT))
        ok(desc.BufferUsage == usage, "Got usage %#x, expected %#x, test %u.\n", desc.BufferUsage, usage, i);

        expected_bind_flags = 0;
        if (usage & DXGI_USAGE_RENDER_TARGET_OUTPUT)
            expected_bind_flags |= D3D11_BIND_RENDER_TARGET;
        if (usage & DXGI_USAGE_SHADER_INPUT)
            expected_bind_flags |= D3D11_BIND_SHADER_RESOURCE;

        hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_ID3D10Texture2D, (void **)&d3d10_texture);
        ok(hr == S_OK, "Got unexpected hr %#lx, test %u.\n", hr, i);
        ID3D10Texture2D_GetDesc(d3d10_texture, &d3d10_texture_desc);
        ok(d3d10_texture_desc.BindFlags == expected_bind_flags,
                "Got d3d10 bind flags %#x, expected %#x, test %u.\n",
                d3d10_texture_desc.BindFlags, expected_bind_flags, i);
        ID3D10Texture2D_Release(d3d10_texture);

        hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_ID3D11Texture2D, (void **)&d3d11_texture);
        ok(hr == S_OK || broken(hr == E_NOINTERFACE), "Got unexpected hr %#lx, test %u.\n", hr, i);
        if (SUCCEEDED(hr))
        {
            ID3D11Texture2D_GetDesc(d3d11_texture, &d3d11_texture_desc);
            ok(d3d11_texture_desc.BindFlags == expected_bind_flags,
                    "Got d3d11 bind flags %#x, expected %#x, test %u.\n",
                    d3d11_texture_desc.BindFlags, expected_bind_flags, i);
            ID3D11Texture2D_Release(d3d11_texture);
        }

        hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_IDXGIResource, (void **)&resource);
        ok(hr == S_OK, "Got unexpected hr %#lx, test %u.\n", hr, i);
        expected_usage = usage | DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_DISCARD_ON_PRESENT;
        hr = IDXGIResource_GetUsage(resource, &usage);
        ok(hr == S_OK, "Got unexpected hr %#lx, test %u.\n", hr, i);
        ok(usage == expected_usage, "Got usage %x, expected %x, test %u.\n", usage, expected_usage, i);
        IDXGIResource_Release(resource);

        IDXGISwapChain_Release(swapchain);
    }

    /* multisampling */
    memset(&desc, 0, sizeof(desc));
    desc.BufferDesc.Width = registry_mode.dmPelsWidth;
    desc.BufferDesc.Height = registry_mode.dmPelsHeight;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 4;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 4;
    desc.OutputWindow = window;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    hr = IDXGIFactory_CreateSwapChain(factory, obj, &desc, &swapchain);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    hr = IDXGIFactory_CreateSwapChain(factory, obj, &desc, &swapchain);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
    if (check_multisample_quality_levels(device, desc.BufferDesc.Format, desc.SampleDesc.Count))
    {
        desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        hr = IDXGIFactory_CreateSwapChain(factory, obj, &desc, &swapchain);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        IDXGISwapChain_Release(swapchain);
        desc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
        hr = IDXGIFactory_CreateSwapChain(factory, obj, &desc, &swapchain);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        IDXGISwapChain_Release(swapchain);
    }
    else
    {
        skip("Multisampling not supported for DXGI_FORMAT_R8G8B8A8_UNORM.\n");
    }

    IDXGIFactory_Release(factory);
    IUnknown_Release(obj);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
    DestroyWindow(window);
}

static void test_swapchain_present(IUnknown *device, BOOL is_d3d12)
{
    static const DWORD flags[] = {0, DXGI_PRESENT_TEST};
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    IDXGISwapChain *swapchain;
    IDXGIFactory *factory;
    IDXGIOutput *output;
    BOOL fullscreen;
    unsigned int i;
    ULONG refcount;
    HRESULT hr;

    get_factory(device, is_d3d12, &factory);

    swapchain_desc.BufferDesc.Width = 800;
    swapchain_desc.BufferDesc.Height = 600;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 60;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = is_d3d12 ? 2 : 1;
    swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = is_d3d12 ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;

    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    for (i = 0; i < 10; ++i)
    {
        hr = IDXGISwapChain_Present(swapchain, i, 0);
        ok(hr == (i <= 4 ? S_OK : DXGI_ERROR_INVALID_CALL),
                "Got unexpected hr %#lx for sync interval %u.\n", hr, i);
    }
    hr = IDXGISwapChain_Present(swapchain, 0, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(flags); ++i)
    {
        HWND occluding_window = CreateWindowA("static", "occluding_window",
                WS_POPUP | WS_VISIBLE, 0, 0, 400, 200, NULL, NULL, NULL, NULL);

        /* Another window covers the swapchain window. Not reported as occluded. */
        hr = IDXGISwapChain_Present(swapchain, 0, flags[i]);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);

        /* Minimised window. */
        ShowWindow(swapchain_desc.OutputWindow, SW_MINIMIZE);
        hr = IDXGISwapChain_Present(swapchain, 0, flags[i]);
        ok(hr == (is_d3d12 ? S_OK : DXGI_STATUS_OCCLUDED), "Test %u: Got unexpected hr %#lx.\n", i, hr);
        ShowWindow(swapchain_desc.OutputWindow, SW_NORMAL);

        /* Hidden window. */
        ShowWindow(swapchain_desc.OutputWindow, SW_HIDE);
        hr = IDXGISwapChain_Present(swapchain, 0, flags[i]);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        ShowWindow(swapchain_desc.OutputWindow, SW_SHOW);
        DestroyWindow(occluding_window);

        /* Test that IDXGIOutput_ReleaseOwnership() makes the swapchain exit
         * fullscreen. */
        hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
        /* DXGI_ERROR_NOT_CURRENTLY_AVAILABLE on some machines.
         * DXGI_ERROR_UNSUPPORTED on the Windows 7 testbot. */
        if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE || broken(hr == DXGI_ERROR_UNSUPPORTED))
        {
            skip("Test %u: Could not change fullscreen state.\n", i);
            continue;
        }
        flush_events();
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        hr = IDXGISwapChain_ResizeBuffers(swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        hr = IDXGISwapChain_Present(swapchain, 0, flags[i]);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        output = NULL;
        fullscreen = FALSE;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, &output);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        ok(fullscreen, "Test %u: Got unexpected fullscreen status.\n", i);
        ok(!!output, "Test %u: Got unexpected output.\n", i);

        if (output)
            IDXGIOutput_ReleaseOwnership(output);
        /* Still fullscreen. */
        fullscreen = FALSE;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        ok(fullscreen, "Test %u: Got unexpected fullscreen status.\n", i);
        /* Calling IDXGISwapChain_Present() will exit fullscreen. */
        hr = IDXGISwapChain_Present(swapchain, 0, flags[i]);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        fullscreen = TRUE;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        /* Now fullscreen mode is exited. */
        if (!flags[i] && !is_d3d12)
            /* Still fullscreen on vista and 2008. */
            todo_wine ok(!fullscreen || broken(fullscreen), "Test %u: Got unexpected fullscreen status.\n", i);
        else
            ok(fullscreen, "Test %u: Got unexpected fullscreen status.\n", i);
        if (output)
            IDXGIOutput_Release(output);

        /* Test creating a window when swapchain is in fullscreen.
         *
         * The window should break the swapchain out of fullscreen mode on
         * d3d10/11. D3d12 is different, a new occluding window doesn't break
         * the swapchain out of fullscreen because d3d12 fullscreen swapchains
         * don't take exclusive ownership over the output, nor do they disable
         * compositing. D3d12 fullscreen mode acts just like borderless
         * fullscreen window mode. */
        hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        fullscreen = FALSE;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        ok(fullscreen, "Test %u: Got unexpected fullscreen status.\n", i);
        hr = IDXGISwapChain_ResizeBuffers(swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        hr = IDXGISwapChain_Present(swapchain, 0, flags[i]);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);

        occluding_window = CreateWindowA("static", "occluding_window", WS_POPUP, 0, 0, 400, 200, 0, 0, 0, 0);
        /* An invisible window doesn't cause the swapchain to exit fullscreen
         * mode. */
        hr = IDXGISwapChain_ResizeBuffers(swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        hr = IDXGISwapChain_Present(swapchain, 0, flags[i]);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        fullscreen = FALSE;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        ok(fullscreen, "Test %u: Got unexpected fullscreen status.\n", i);
        /* A visible, but with bottom z-order window still causes the
         * swapchain to exit fullscreen mode. */
        SetWindowPos(occluding_window, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        ShowWindow(occluding_window, SW_SHOW);
        /* Fullscreen mode takes a while to exit. */
        if (!is_d3d12)
            wait_fullscreen_state(swapchain, FALSE, TRUE);

        /* No longer fullscreen before calling IDXGISwapChain_Present() except
         * for d3d12. */
        fullscreen = TRUE;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        todo_wine_if(!is_d3d12) ok(is_d3d12 ? fullscreen : !fullscreen,
                "Test %u: Got unexpected fullscreen status.\n", i);

        hr = IDXGISwapChain_ResizeBuffers(swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        hr = IDXGISwapChain_Present(swapchain, 0, flags[i]);
        todo_wine_if(is_d3d12) ok(hr == (is_d3d12 ? DXGI_STATUS_OCCLUDED : S_OK),
                "Test %u: Got unexpected hr %#lx.\n", i, hr);

        fullscreen = TRUE;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        if (flags[i] == DXGI_PRESENT_TEST)
            todo_wine_if(!is_d3d12) ok(is_d3d12 ? fullscreen : !fullscreen,
                    "Test %u: Got unexpected fullscreen status.\n", i);
        else
            todo_wine ok(!fullscreen, "Test %u: Got unexpected fullscreen status.\n", i);

        /* Even though d3d12 doesn't exit fullscreen, a
         * IDXGISwapChain_ResizeBuffers() is still needed for subsequent
         * IDXGISwapChain_Present() calls to work, otherwise they will return
         * DXGI_ERROR_INVALID_CALL */
        hr = IDXGISwapChain_ResizeBuffers(swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        hr = IDXGISwapChain_Present(swapchain, 0, flags[i]);
        if (flags[i] == DXGI_PRESENT_TEST)
            todo_wine_if(is_d3d12) ok(hr == (is_d3d12 ? DXGI_STATUS_OCCLUDED : S_OK),
                    "Test %u: Got unexpected hr %#lx.\n", i, hr);
        else
            ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);

        /* Trying to break out of fullscreen mode again. This time, don't call
         * IDXGISwapChain_GetFullscreenState() before IDXGISwapChain_Present(). */
        ShowWindow(occluding_window, SW_HIDE);
        hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        ShowWindow(occluding_window, SW_SHOW);

        hr = IDXGISwapChain_ResizeBuffers(swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        hr = IDXGISwapChain_Present(swapchain, 0, flags[i]);
        /* hr == S_OK on vista and 2008 */
        todo_wine ok(hr == DXGI_STATUS_OCCLUDED || broken(hr == S_OK),
                "Test %u: Got unexpected hr %#lx.\n", i, hr);

        hr = IDXGISwapChain_ResizeBuffers(swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        hr = IDXGISwapChain_Present(swapchain, 0, flags[i]);
        if (flags[i] == DXGI_PRESENT_TEST)
        {
            todo_wine ok(hr == DXGI_STATUS_OCCLUDED || broken(hr == S_OK),
                    "Test %u: Got unexpected hr %#lx.\n", i, hr);
            /* IDXGISwapChain_Present() without flags refreshes the occlusion
             * state. */
            hr = IDXGISwapChain_ResizeBuffers(swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
            ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
            hr = IDXGISwapChain_Present(swapchain, 0, 0);
            todo_wine ok(hr == DXGI_STATUS_OCCLUDED || broken(hr == S_OK),
                    "Test %u: Got unexpected hr %#lx.\n", i, hr);
            hr = IDXGISwapChain_ResizeBuffers(swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
            ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
            hr = IDXGISwapChain_Present(swapchain, 0, DXGI_PRESENT_TEST);
            ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        }
        else
        {
            ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        }
        fullscreen = TRUE;
        hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        todo_wine ok(!fullscreen, "Test %u: Got unexpected fullscreen status.\n", i);

        DestroyWindow(occluding_window);
        flush_events();
        hr = IDXGISwapChain_ResizeBuffers(swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        hr = IDXGISwapChain_Present(swapchain, 0, flags[i]);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);

        hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        hr = IDXGISwapChain_ResizeBuffers(swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
    }

    wait_device_idle(device);

    IDXGISwapChain_Release(swapchain);
    DestroyWindow(swapchain_desc.OutputWindow);
    refcount = IDXGIFactory_Release(factory);
    ok(refcount == !is_d3d12, "Got unexpected refcount %lu.\n", refcount);
}

static void test_swapchain_backbuffer_index(IUnknown *device, BOOL is_d3d12)
{
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    unsigned int index, expected_index;
    IDXGISwapChain3 *swapchain3;
    ID3D12Device *d3d12_device;
    ID3D12CommandQueue *queue;
    IDXGISwapChain *swapchain;
    HRESULT hr, expected_hr;
    IDXGIFactory *factory;
    ID3D12Fence *fence;
    unsigned int i, j;
    ULONG refcount;
    RECT rect;
    BOOL ret;

    static const DXGI_SWAP_EFFECT swap_effects[] =
    {
        DXGI_SWAP_EFFECT_DISCARD,
        DXGI_SWAP_EFFECT_SEQUENTIAL,
        DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,
        DXGI_SWAP_EFFECT_FLIP_DISCARD,
    };

    get_factory(device, is_d3d12, &factory);

    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 60;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.OutputWindow = create_window();
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;

    ret = GetClientRect(swapchain_desc.OutputWindow, &rect);
    ok(ret, "Failed to get client rect.\n");
    swapchain_desc.BufferDesc.Width = rect.right;
    swapchain_desc.BufferDesc.Height = rect.bottom;

    for (i = 0; i < ARRAY_SIZE(swap_effects); ++i)
    {
        swapchain_desc.BufferCount = 4;
        swapchain_desc.SwapEffect = swap_effects[i];
        expected_hr = is_d3d12 && !is_flip_model(swap_effects[i]) ? DXGI_ERROR_INVALID_CALL : S_OK;
        hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
        ok(hr == expected_hr, "Got unexpected hr %#lx, expected %#lx.\n", hr, expected_hr);
        if (FAILED(hr))
            continue;

        hr = IDXGISwapChain_QueryInterface(swapchain, &IID_IDXGISwapChain3, (void **)&swapchain3);
        if (hr == E_NOINTERFACE)
        {
            skip("IDXGISwapChain3 is not supported.\n");
            IDXGISwapChain_Release(swapchain);
            goto done;
        }

        for (j = 0; j < 2 * swapchain_desc.BufferCount + 2; ++j)
        {
            index = IDXGISwapChain3_GetCurrentBackBufferIndex(swapchain3);
            expected_index = is_d3d12 ? j % swapchain_desc.BufferCount : 0;
            ok(index == expected_index, "Got back buffer index %u, expected %u.\n", index, expected_index);
            hr = IDXGISwapChain3_Present(swapchain3, 0, 0);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        }

        swapchain_desc.BufferCount = 3;
        hr = IDXGISwapChain_ResizeBuffers(swapchain, swapchain_desc.BufferCount,
                rect.right, rect.bottom, DXGI_FORMAT_UNKNOWN, swapchain_desc.Flags);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        /* The back buffer index restarts from 0. */
        for (j = 0; j < swapchain_desc.BufferCount + 2; ++j)
        {
            index = IDXGISwapChain3_GetCurrentBackBufferIndex(swapchain3);
            expected_index = is_d3d12 ? j % swapchain_desc.BufferCount : 0;
            ok(index == expected_index, "Got back buffer index %u, expected %u.\n", index, expected_index);
            hr = IDXGISwapChain3_Present(swapchain3, 0, 0);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        }

        hr = IDXGISwapChain_ResizeBuffers(swapchain, swapchain_desc.BufferCount,
                rect.right, rect.bottom, DXGI_FORMAT_UNKNOWN, swapchain_desc.Flags);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        /* Even when not really changing the buffer count. */
        for (j = 0; j < swapchain_desc.BufferCount + 1; ++j)
        {
            index = IDXGISwapChain3_GetCurrentBackBufferIndex(swapchain3);
            expected_index = is_d3d12 ? j % swapchain_desc.BufferCount : 0;
            ok(index == expected_index, "Got back buffer index %u, expected %u.\n", index, expected_index);
            hr = IDXGISwapChain3_Present(swapchain3, 0, 0);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        }

        hr = IDXGISwapChain_ResizeBuffers(swapchain, 0,
                rect.right, rect.bottom, DXGI_FORMAT_UNKNOWN, swapchain_desc.Flags);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        for (j = 0; j < swapchain_desc.BufferCount + 2; ++j)
        {
            index = IDXGISwapChain3_GetCurrentBackBufferIndex(swapchain3);
            expected_index = is_d3d12 ? j % swapchain_desc.BufferCount : 0;
            ok(index == expected_index, "Got back buffer index %u, expected %u.\n", index, expected_index);
            hr = IDXGISwapChain3_Present(swapchain3, 0, 0);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        }

        if (is_d3d12)
        {
            hr = IUnknown_QueryInterface(device, &IID_ID3D12CommandQueue, (void **)&queue);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            hr = ID3D12CommandQueue_GetDevice(queue, &IID_ID3D12Device, (void **)&d3d12_device);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            hr = ID3D12Device_CreateFence(d3d12_device, 0, 0, &IID_ID3D12Fence, (void **)&fence);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            hr = ID3D12CommandQueue_Wait(queue, fence, 1);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            hr = IDXGISwapChain_ResizeBuffers(swapchain, 0,
                    rect.right, rect.bottom, DXGI_FORMAT_UNKNOWN, swapchain_desc.Flags);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            /* The back buffer index is updated when Present() is
             * called, not when frames are presented. */
            for (j = 0; j < swapchain_desc.BufferCount + 2; ++j)
            {
                index = IDXGISwapChain3_GetCurrentBackBufferIndex(swapchain3);
                expected_index = j % swapchain_desc.BufferCount;
                ok(index == expected_index, "Got back buffer index %u, expected %u.\n", index, expected_index);
                hr = IDXGISwapChain3_Present(swapchain3, 0, 0);
                ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
            }

            index = IDXGISwapChain3_GetCurrentBackBufferIndex(swapchain3);
            expected_index = j % swapchain_desc.BufferCount;
            ok(index == expected_index, "Got back buffer index %u, expected %u.\n", index, expected_index);

            hr = ID3D12Fence_Signal(fence, 1);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            index = IDXGISwapChain3_GetCurrentBackBufferIndex(swapchain3);
            expected_index = j % swapchain_desc.BufferCount;
            ok(index == expected_index, "Got back buffer index %u, expected %u.\n", index, expected_index);

            refcount = ID3D12Fence_Release(fence);
            ok(!refcount, "Fence has %lu references left.\n", refcount);
            ID3D12Device_Release(d3d12_device);
            ID3D12CommandQueue_Release(queue);
        }

        wait_device_idle(device);

        IDXGISwapChain3_Release(swapchain3);
        refcount = IDXGISwapChain_Release(swapchain);
        ok(!refcount, "Swapchain has %lu references left.\n", refcount);
    }

done:
    DestroyWindow(swapchain_desc.OutputWindow);
    refcount = IDXGIFactory_Release(factory);
    ok(refcount == !is_d3d12, "Got unexpected refcount %lu.\n", refcount);
}

static void test_swapchain_formats(IUnknown *device, BOOL is_d3d12)
{
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    IDXGISwapChain *swapchain;
    HRESULT hr, expected_hr;
    IDXGIFactory *factory;
    unsigned int i;
    ULONG refcount;
    RECT rect;
    BOOL ret;

    static const struct
    {
        DXGI_FORMAT format;
        DXGI_SWAP_EFFECT swap_effect;
        BOOL supported;
    }
    tests[] =
    {
        {DXGI_FORMAT_UNKNOWN,                    DXGI_SWAP_EFFECT_DISCARD,         FALSE},
        {DXGI_FORMAT_UNKNOWN,                    DXGI_SWAP_EFFECT_SEQUENTIAL,      FALSE},
        {DXGI_FORMAT_UNKNOWN,                    DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, FALSE},
        {DXGI_FORMAT_UNKNOWN,                    DXGI_SWAP_EFFECT_FLIP_DISCARD,    FALSE},
        {DXGI_FORMAT_R8G8B8A8_UNORM,             DXGI_SWAP_EFFECT_DISCARD,         TRUE},
        {DXGI_FORMAT_R8G8B8A8_UNORM,             DXGI_SWAP_EFFECT_SEQUENTIAL,      TRUE},
        {DXGI_FORMAT_R8G8B8A8_UNORM,             DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, TRUE},
        {DXGI_FORMAT_R8G8B8A8_UNORM,             DXGI_SWAP_EFFECT_FLIP_DISCARD,    TRUE},
        {DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,        DXGI_SWAP_EFFECT_DISCARD,         TRUE},
        {DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,        DXGI_SWAP_EFFECT_SEQUENTIAL,      TRUE},
        {DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,        DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, FALSE},
        {DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,        DXGI_SWAP_EFFECT_FLIP_DISCARD,    FALSE},
        {DXGI_FORMAT_B8G8R8A8_UNORM,             DXGI_SWAP_EFFECT_DISCARD,         TRUE},
        {DXGI_FORMAT_B8G8R8A8_UNORM,             DXGI_SWAP_EFFECT_SEQUENTIAL,      TRUE},
        {DXGI_FORMAT_B8G8R8A8_UNORM,             DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, TRUE},
        {DXGI_FORMAT_B8G8R8A8_UNORM,             DXGI_SWAP_EFFECT_FLIP_DISCARD,    TRUE},
        {DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,        DXGI_SWAP_EFFECT_DISCARD,         TRUE},
        {DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,        DXGI_SWAP_EFFECT_SEQUENTIAL,      TRUE},
        {DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,        DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, FALSE},
        {DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,        DXGI_SWAP_EFFECT_FLIP_DISCARD,    FALSE},
        {DXGI_FORMAT_R10G10B10A2_UNORM,          DXGI_SWAP_EFFECT_DISCARD,         TRUE},
        {DXGI_FORMAT_R10G10B10A2_UNORM,          DXGI_SWAP_EFFECT_SEQUENTIAL,      TRUE},
        {DXGI_FORMAT_R10G10B10A2_UNORM,          DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, TRUE},
        {DXGI_FORMAT_R10G10B10A2_UNORM,          DXGI_SWAP_EFFECT_FLIP_DISCARD,    TRUE},
        {DXGI_FORMAT_R16G16B16A16_FLOAT,         DXGI_SWAP_EFFECT_DISCARD,         TRUE},
        {DXGI_FORMAT_R16G16B16A16_FLOAT,         DXGI_SWAP_EFFECT_SEQUENTIAL,      TRUE},
        {DXGI_FORMAT_R16G16B16A16_FLOAT,         DXGI_SWAP_EFFECT_FLIP_DISCARD,    TRUE},
        {DXGI_FORMAT_R16G16B16A16_FLOAT,         DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, TRUE},
        {DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, DXGI_SWAP_EFFECT_FLIP_DISCARD,    FALSE},
        {DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, FALSE},
    };

    get_factory(device, is_d3d12, &factory);

    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 60;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 4;
    swapchain_desc.OutputWindow = create_window();
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.Flags = 0;

    ret = GetClientRect(swapchain_desc.OutputWindow, &rect);
    ok(ret, "Failed to get client rect.\n");
    swapchain_desc.BufferDesc.Width = rect.right;
    swapchain_desc.BufferDesc.Height = rect.bottom;

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        if (is_d3d12 && !is_flip_model(tests[i].swap_effect))
            continue;

        swapchain_desc.BufferDesc.Format = tests[i].format;
        swapchain_desc.SwapEffect = tests[i].swap_effect;
        hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
        expected_hr = tests[i].supported ? S_OK : DXGI_ERROR_INVALID_CALL;
        if (tests[i].format == DXGI_FORMAT_UNKNOWN && !is_d3d12)
            expected_hr = E_INVALIDARG;
        ok(hr == expected_hr
                /* Flip presentation model not supported. */
                || broken(hr == DXGI_ERROR_INVALID_CALL && is_flip_model(tests[i].swap_effect) && !is_d3d12),
                "Test %u, d3d12 %#x: Got unexpected hr %#lx, expected %#lx.\n", i, is_d3d12, hr, expected_hr);

        if (SUCCEEDED(hr))
        {
            refcount = IDXGISwapChain_Release(swapchain);
            ok(!refcount, "Swapchain has %lu references left.\n", refcount);
        }
    }

    DestroyWindow(swapchain_desc.OutputWindow);
    refcount = IDXGIFactory_Release(factory);
    ok(refcount == !is_d3d12, "Got unexpected refcount %lu.\n", refcount);
}

static void test_maximum_frame_latency(void)
{
    IDXGIDevice1 *device1;
    IDXGIDevice *device;
    UINT max_latency;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    if (SUCCEEDED(IDXGIDevice_QueryInterface(device, &IID_IDXGIDevice1, (void **)&device1)))
    {
        hr = IDXGIDevice1_GetMaximumFrameLatency(device1, NULL);
        ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);

        hr = IDXGIDevice1_GetMaximumFrameLatency(device1, &max_latency);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(max_latency == DEFAULT_FRAME_LATENCY, "Got unexpected maximum frame latency %u.\n", max_latency);

        hr = IDXGIDevice1_SetMaximumFrameLatency(device1, MAX_FRAME_LATENCY);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = IDXGIDevice1_GetMaximumFrameLatency(device1, &max_latency);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(max_latency == MAX_FRAME_LATENCY, "Got unexpected maximum frame latency %u.\n", max_latency);

        hr = IDXGIDevice1_SetMaximumFrameLatency(device1, MAX_FRAME_LATENCY + 1);
        ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
        hr = IDXGIDevice1_GetMaximumFrameLatency(device1, &max_latency);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(max_latency == MAX_FRAME_LATENCY, "Got unexpected maximum frame latency %u.\n", max_latency);

        hr = IDXGIDevice1_SetMaximumFrameLatency(device1, 0);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = IDXGIDevice1_GetMaximumFrameLatency(device1, &max_latency);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        /* 0 does not reset to the default frame latency on all Windows versions. */
        ok(max_latency == DEFAULT_FRAME_LATENCY || broken(!max_latency),
                "Got unexpected maximum frame latency %u.\n", max_latency);

        IDXGIDevice1_Release(device1);
    }
    else
    {
        win_skip("IDXGIDevice1 is not implemented.\n");
    }

    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

static void test_output_desc(void)
{
    IDXGIAdapter *adapter, *adapter2;
    IDXGIOutput *output, *output2;
    DXGI_OUTPUT_DESC desc;
    IDXGIFactory *factory;
    unsigned int i, j;
    ULONG refcount;
    HRESULT hr;

    hr = CreateDXGIFactory(&IID_IDXGIFactory, (void **)&factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    for (i = 0; ; ++i)
    {
        hr = IDXGIFactory_EnumAdapters(factory, i, &adapter);
        if (hr == DXGI_ERROR_NOT_FOUND)
            break;
        winetest_push_context("Adapter %u", i);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = IDXGIFactory_EnumAdapters(factory, i, &adapter2);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(adapter != adapter2, "Expected to get new instance of IDXGIAdapter, %p == %p.\n", adapter, adapter2);
        refcount = get_refcount(adapter);
        ok(refcount == 1, "Get unexpected refcount %lu.\n", refcount);
        IDXGIAdapter_Release(adapter2);

        refcount = get_refcount(factory);
        ok(refcount == 2, "Get unexpected refcount %lu.\n", refcount);
        refcount = get_refcount(adapter);
        ok(refcount == 1, "Get unexpected refcount %lu.\n", refcount);

        for (j = 0; ; ++j)
        {
            MONITORINFOEXW monitor_info;
            BOOL ret;

            hr = IDXGIAdapter_EnumOutputs(adapter, j, &output);
            if (hr == DXGI_ERROR_NOT_FOUND)
                break;
            winetest_push_context("Output %u", j);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            hr = IDXGIAdapter_EnumOutputs(adapter, j, &output2);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
            ok(output != output2, "Expected to get new instance of IDXGIOutput, %p == %p.\n", output, output2);
            refcount = get_refcount(output);
            ok(refcount == 1, "Get unexpected refcount %lu.\n", refcount);
            IDXGIOutput_Release(output2);

            refcount = get_refcount(factory);
            ok(refcount == 2, "Get unexpected refcount %lu.\n", refcount);
            refcount = get_refcount(adapter);
            ok(refcount == 2, "Get unexpected refcount %lu.\n", refcount);
            refcount = get_refcount(output);
            ok(refcount == 1, "Get unexpected refcount %lu.\n", refcount);

            hr = IDXGIOutput_GetDesc(output, &desc);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            monitor_info.cbSize = sizeof(monitor_info);
            ret = GetMonitorInfoW(desc.Monitor, (MONITORINFO *)&monitor_info);
            ok(ret, "Failed to get monitor info.\n");
            ok(!lstrcmpW(desc.DeviceName, monitor_info.szDevice), "Got unexpected device name %s, expected %s.\n",
                    wine_dbgstr_w(desc.DeviceName), wine_dbgstr_w(monitor_info.szDevice));
            ok(EqualRect(&desc.DesktopCoordinates, &monitor_info.rcMonitor),
                    "Got unexpected desktop coordinates %s, expected %s.\n",
                    wine_dbgstr_rect(&desc.DesktopCoordinates),
                    wine_dbgstr_rect(&monitor_info.rcMonitor));

            IDXGIOutput_Release(output);
            refcount = get_refcount(adapter);
            ok(refcount == 1, "Get unexpected refcount %lu.\n", refcount);

            winetest_pop_context();
        }

        IDXGIAdapter_Release(adapter);
        refcount = get_refcount(factory);
        ok(refcount == 1, "Get unexpected refcount %lu.\n", refcount);

        winetest_pop_context();
    }

    refcount = IDXGIFactory_Release(factory);
    ok(!refcount, "IDXGIFactory has %lu references left.\n", refcount);
}

struct dxgi_factory
{
    IDXGIFactory IDXGIFactory_iface;
    IDXGIFactory *wrapped_iface;
    unsigned int wrapped_adapter_count;
};

static inline struct dxgi_factory *impl_from_IDXGIFactory(IDXGIFactory *iface)
{
    return CONTAINING_RECORD(iface, struct dxgi_factory, IDXGIFactory_iface);
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_QueryInterface(IDXGIFactory *iface, REFIID iid, void **out)
{
    struct dxgi_factory *factory = impl_from_IDXGIFactory(iface);

    if (IsEqualGUID(iid, &IID_IDXGIFactory)
            || IsEqualGUID(iid, &IID_IDXGIObject)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        IDXGIFactory_AddRef(iface);
        *out = iface;
        return S_OK;
    }
    return IDXGIFactory_QueryInterface(factory->wrapped_iface, iid, out);
}

static ULONG STDMETHODCALLTYPE dxgi_factory_AddRef(IDXGIFactory *iface)
{
    struct dxgi_factory *factory = impl_from_IDXGIFactory(iface);
    return IDXGIFactory_AddRef(factory->wrapped_iface);
}

static ULONG STDMETHODCALLTYPE dxgi_factory_Release(IDXGIFactory *iface)
{
    struct dxgi_factory *factory = impl_from_IDXGIFactory(iface);
    return IDXGIFactory_Release(factory->wrapped_iface);
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_SetPrivateData(IDXGIFactory *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct dxgi_factory *factory = impl_from_IDXGIFactory(iface);
    return IDXGIFactory_SetPrivateData(factory->wrapped_iface, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_SetPrivateDataInterface(IDXGIFactory *iface,
        REFGUID guid, const IUnknown *object)
{
    struct dxgi_factory *factory = impl_from_IDXGIFactory(iface);
    return IDXGIFactory_SetPrivateDataInterface(factory->wrapped_iface, guid, object);
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_GetPrivateData(IDXGIFactory *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct dxgi_factory *factory = impl_from_IDXGIFactory(iface);
    return IDXGIFactory_GetPrivateData(factory->wrapped_iface, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_GetParent(IDXGIFactory *iface, REFIID iid, void **parent)
{
    struct dxgi_factory *factory = impl_from_IDXGIFactory(iface);
    return IDXGIFactory_GetParent(factory->wrapped_iface, iid, parent);
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_EnumAdapters(IDXGIFactory *iface,
        UINT adapter_idx, IDXGIAdapter **adapter)
{
    struct dxgi_factory *factory = impl_from_IDXGIFactory(iface);
    HRESULT hr;

    if (SUCCEEDED(hr = IDXGIFactory_EnumAdapters(factory->wrapped_iface, adapter_idx, adapter)))
        ++factory->wrapped_adapter_count;
    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_MakeWindowAssociation(IDXGIFactory *iface,
        HWND window, UINT flags)
{
    struct dxgi_factory *factory = impl_from_IDXGIFactory(iface);
    return IDXGIFactory_MakeWindowAssociation(factory->wrapped_iface, window, flags);
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_GetWindowAssociation(IDXGIFactory *iface, HWND *window)
{
    struct dxgi_factory *factory = impl_from_IDXGIFactory(iface);
    return IDXGIFactory_GetWindowAssociation(factory->wrapped_iface, window);
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_CreateSwapChain(IDXGIFactory *iface,
        IUnknown *device, DXGI_SWAP_CHAIN_DESC *desc, IDXGISwapChain **swapchain)
{
    struct dxgi_factory *factory = impl_from_IDXGIFactory(iface);
    return IDXGIFactory_CreateSwapChain(factory->wrapped_iface, device, desc, swapchain);
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_CreateSoftwareAdapter(IDXGIFactory *iface,
        HMODULE swrast, IDXGIAdapter **adapter)
{
    struct dxgi_factory *factory = impl_from_IDXGIFactory(iface);
    return IDXGIFactory_CreateSoftwareAdapter(factory->wrapped_iface, swrast, adapter);
}

static const struct IDXGIFactoryVtbl dxgi_factory_vtbl =
{
    dxgi_factory_QueryInterface,
    dxgi_factory_AddRef,
    dxgi_factory_Release,
    dxgi_factory_SetPrivateData,
    dxgi_factory_SetPrivateDataInterface,
    dxgi_factory_GetPrivateData,
    dxgi_factory_GetParent,
    dxgi_factory_EnumAdapters,
    dxgi_factory_MakeWindowAssociation,
    dxgi_factory_GetWindowAssociation,
    dxgi_factory_CreateSwapChain,
    dxgi_factory_CreateSoftwareAdapter,
};

struct dxgi_adapter
{
    IDXGIAdapter IDXGIAdapter_iface;
    IDXGIAdapter *wrapped_iface;
    struct dxgi_factory factory;
    unsigned int wrapped_output_count;
};

static inline struct dxgi_adapter *impl_from_IDXGIAdapter(IDXGIAdapter *iface)
{
    return CONTAINING_RECORD(iface, struct dxgi_adapter, IDXGIAdapter_iface);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_QueryInterface(IDXGIAdapter *iface, REFIID iid, void **out)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter(iface);

    if (IsEqualGUID(iid, &IID_IDXGIAdapter)
            || IsEqualGUID(iid, &IID_IDXGIObject)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        IDXGIAdapter_AddRef(adapter->wrapped_iface);
        *out = iface;
        return S_OK;
    }
    return IDXGIAdapter_QueryInterface(adapter->wrapped_iface, iid, out);
}

static ULONG STDMETHODCALLTYPE dxgi_adapter_AddRef(IDXGIAdapter *iface)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter(iface);
    return IDXGIAdapter_AddRef(adapter->wrapped_iface);
}

static ULONG STDMETHODCALLTYPE dxgi_adapter_Release(IDXGIAdapter *iface)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter(iface);
    return IDXGIAdapter_Release(adapter->wrapped_iface);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_SetPrivateData(IDXGIAdapter *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter(iface);
    return IDXGIAdapter_SetPrivateData(adapter->wrapped_iface, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_SetPrivateDataInterface(IDXGIAdapter *iface,
        REFGUID guid, const IUnknown *object)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter(iface);
    return IDXGIAdapter_SetPrivateDataInterface(adapter->wrapped_iface, guid, object);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetPrivateData(IDXGIAdapter *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter(iface);
    return IDXGIAdapter_GetPrivateData(adapter->wrapped_iface, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetParent(IDXGIAdapter *iface, REFIID iid, void **parent)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter(iface);
    return IDXGIFactory_QueryInterface(&adapter->factory.IDXGIFactory_iface, iid, parent);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_EnumOutputs(IDXGIAdapter *iface,
        UINT output_idx, IDXGIOutput **output)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter(iface);
    HRESULT hr;

    if (SUCCEEDED(hr = IDXGIAdapter_EnumOutputs(adapter->wrapped_iface, output_idx, output)))
        ++adapter->wrapped_output_count;
    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_GetDesc(IDXGIAdapter *iface, DXGI_ADAPTER_DESC *desc)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter(iface);
    return IDXGIAdapter_GetDesc(adapter->wrapped_iface, desc);
}

static HRESULT STDMETHODCALLTYPE dxgi_adapter_CheckInterfaceSupport(IDXGIAdapter *iface,
        REFGUID guid, LARGE_INTEGER *umd_version)
{
    struct dxgi_adapter *adapter = impl_from_IDXGIAdapter(iface);
    return IDXGIAdapter_CheckInterfaceSupport(adapter->wrapped_iface, guid, umd_version);
}

static const struct IDXGIAdapterVtbl dxgi_adapter_vtbl =
{
    dxgi_adapter_QueryInterface,
    dxgi_adapter_AddRef,
    dxgi_adapter_Release,
    dxgi_adapter_SetPrivateData,
    dxgi_adapter_SetPrivateDataInterface,
    dxgi_adapter_GetPrivateData,
    dxgi_adapter_GetParent,
    dxgi_adapter_EnumOutputs,
    dxgi_adapter_GetDesc,
    dxgi_adapter_CheckInterfaceSupport,
};

static void test_object_wrapping(void)
{
    struct dxgi_adapter wrapper;
    DXGI_ADAPTER_DESC desc;
    IDXGIAdapter *adapter;
    IDXGIFactory *factory;
    ID3D10Device1 *device;
    ULONG refcount;
    HRESULT hr;

    hr = CreateDXGIFactory(&IID_IDXGIFactory, (void **)&factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIFactory_EnumAdapters(factory, 0, &adapter);
    if (hr == DXGI_ERROR_NOT_FOUND)
    {
        skip("Could not enumerate adapters.\n");
        IDXGIFactory_Release(factory);
        return;
    }
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    wrapper.IDXGIAdapter_iface.lpVtbl = &dxgi_adapter_vtbl;
    wrapper.wrapped_iface = adapter;
    wrapper.factory.IDXGIFactory_iface.lpVtbl = &dxgi_factory_vtbl;
    wrapper.factory.wrapped_iface = factory;
    wrapper.factory.wrapped_adapter_count = 0;
    wrapper.wrapped_output_count = 0;

    hr = D3D10CreateDevice1(&wrapper.IDXGIAdapter_iface, D3D10_DRIVER_TYPE_HARDWARE, NULL,
            0, D3D10_FEATURE_LEVEL_10_0, D3D10_1_SDK_VERSION, &device);
    if (SUCCEEDED(hr))
    {
        refcount = ID3D10Device1_Release(device);
        ok(!refcount, "Device has %lu references left.\n", refcount);
    }

    hr = IDXGIAdapter_GetDesc(&wrapper.IDXGIAdapter_iface, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!wrapper.factory.wrapped_adapter_count, "Got unexpected wrapped adapter count %u.\n",
            wrapper.factory.wrapped_adapter_count);
    ok(!wrapper.wrapped_output_count, "Got unexpected wrapped output count %u.\n", wrapper.wrapped_output_count);

    refcount = IDXGIAdapter_Release(&wrapper.IDXGIAdapter_iface);
    ok(!refcount, "Adapter has %lu references left.\n", refcount);
    refcount = IDXGIFactory_Release(factory);
    ok(!refcount, "Factory has %lu references left.\n", refcount);
}

struct adapter_info
{
    const WCHAR *name;
    HMONITOR monitor;
};

static BOOL CALLBACK enum_monitor_proc(HMONITOR monitor, HDC dc, RECT *rect, LPARAM lparam)
{
    struct adapter_info *adapter_info = (struct adapter_info *)lparam;
    MONITORINFOEXW monitor_info;

    monitor_info.cbSize = sizeof(monitor_info);
    if (GetMonitorInfoW(monitor, (MONITORINFO *)&monitor_info)
            && !lstrcmpiW(adapter_info->name, monitor_info.szDevice))
    {
        adapter_info->monitor = monitor;
        return FALSE;
    }

    return TRUE;
}

static HMONITOR get_monitor(const WCHAR *adapter_name)
{
    struct adapter_info info = {adapter_name, NULL};

    EnumDisplayMonitors(NULL, NULL, enum_monitor_proc, (LPARAM)&info);
    return info.monitor;
}

static void test_multi_adapter(void)
{
    unsigned int output_count = 0, expected_output_count = 0;
    unsigned int adapter_index, output_index, device_index;
    DXGI_OUTPUT_DESC old_output_desc, output_desc;
    DXGI_ADAPTER_DESC1 adapter_desc1;
    DXGI_ADAPTER_DESC adapter_desc;
    DISPLAY_DEVICEW display_device;
    MONITORINFO monitor_info;
    DEVMODEW old_mode, mode;
    IDXGIAdapter1 *adapter1;
    IDXGIFactory *factory;
    IDXGIAdapter *adapter;
    IDXGIOutput *output;
    HMONITOR monitor;
    BOOL found;
    HRESULT hr;
    LONG ret;

    if (FAILED(hr = CreateDXGIFactory(&IID_IDXGIFactory, (void **)&factory)))
    {
        skip("Failed to create IDXGIFactory, hr %#lx.\n", hr);
        return;
    }

    hr = IDXGIFactory_EnumAdapters(factory, 0, NULL);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIFactory_EnumAdapters(factory, 0, &adapter);
    if (hr == DXGI_ERROR_NOT_FOUND)
    {
        skip("Could not enumerate adapters.\n");
        IDXGIFactory_Release(factory);
        return;
    }
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    for (adapter_index = 0; SUCCEEDED(IDXGIFactory_EnumAdapters(factory, adapter_index, &adapter)); ++adapter_index)
    {
        for (output_index = 0; SUCCEEDED(IDXGIAdapter_EnumOutputs(adapter, output_index, &output)); ++output_index)
        {
            hr = IDXGIOutput_GetDesc(output, &output_desc);
            ok(hr == S_OK, "Adapter %u output %u: Got unexpected hr %#lx.\n", adapter_index,
                    output_index, hr);

            found = FALSE;
            display_device.cb = sizeof(display_device);
            for (device_index = 0; EnumDisplayDevicesW(NULL, device_index, &display_device, 0); ++device_index)
            {
                if (!lstrcmpiW(display_device.DeviceName, output_desc.DeviceName))
                {
                    found = TRUE;
                    break;
                }
            }
            ok(found, "Adapter %u output %u: Failed to find device %s.\n",
                    adapter_index, output_index, wine_dbgstr_w(output_desc.DeviceName));

            ok(display_device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP,
                    "Adapter %u output %u: Got unexpected state flags %#lx.\n", adapter_index,
                    output_index, display_device.StateFlags);
            if (!adapter_index && !output_index)
                ok(display_device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE,
                        "Adapter %u output %u: Got unexpected state flags %#lx.\n", adapter_index,
                        output_index, display_device.StateFlags);
            else
                ok(!(display_device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE),
                        "Adapter %u output %u: Got unexpected state flags %#lx.\n", adapter_index,
                        output_index, display_device.StateFlags);

            /* Should have the same monitor handle. */
            monitor = get_monitor(display_device.DeviceName);
            ok(!!monitor, "Adapter %u output %u: Failed to find monitor %s.\n", adapter_index,
                    output_index, wine_dbgstr_w(display_device.DeviceName));
            ok(monitor == output_desc.Monitor,
                    "Adapter %u output %u: Got unexpected monitor %p, expected %p.\n",
                    adapter_index, output_index, monitor, output_desc.Monitor);

            /* Should have the same monitor rectangle. */
            monitor_info.cbSize = sizeof(monitor_info);
            ret = GetMonitorInfoA(monitor, &monitor_info);
            ok(ret, "Adapter %u output %u: Failed to get monitor info, error %#lx.\n", adapter_index,
                    output_index, GetLastError());
            ok(EqualRect(&monitor_info.rcMonitor, &output_desc.DesktopCoordinates),
                    "Adapter %u output %u: Got unexpected output rect %s, expected %s.\n",
                    adapter_index, output_index, wine_dbgstr_rect(&monitor_info.rcMonitor),
                    wine_dbgstr_rect(&output_desc.DesktopCoordinates));

            ++output_count;

            /* Test output description after it got detached */
            if (display_device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
            {
                IDXGIOutput_Release(output);
                continue;
            }

            old_output_desc = output_desc;

            /* Save current display settings */
            memset(&old_mode, 0, sizeof(old_mode));
            old_mode.dmSize = sizeof(old_mode);
            ret = EnumDisplaySettingsW(display_device.DeviceName, ENUM_CURRENT_SETTINGS, &old_mode);
            /* Win10 TestBots may return FALSE but it's actually successful */
            ok(ret || broken(!ret),
                    "Adapter %u output %u: EnumDisplaySettingsW failed for %s, error %#lx.\n",
                    adapter_index, output_index, wine_dbgstr_w(display_device.DeviceName),
                    GetLastError());

            /* Detach */
            memset(&mode, 0, sizeof(mode));
            mode.dmSize = sizeof(mode);
            mode.dmFields = DM_POSITION | DM_PELSWIDTH | DM_PELSHEIGHT;
            mode.dmPosition = old_mode.dmPosition;
            ret = ChangeDisplaySettingsExW(display_device.DeviceName, &mode, NULL,
                    CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
            ok(ret == DISP_CHANGE_SUCCESSFUL,
                    "Adapter %u output %u: ChangeDisplaySettingsExW %s returned unexpected %ld.\n",
                    adapter_index, output_index, wine_dbgstr_w(display_device.DeviceName), ret);
            ret = ChangeDisplaySettingsExW(display_device.DeviceName, NULL, NULL, 0, NULL);
            ok(ret == DISP_CHANGE_SUCCESSFUL,
                    "Adapter %u output %u: ChangeDisplaySettingsExW %s returned unexpected %ld.\n",
                    adapter_index, output_index, wine_dbgstr_w(display_device.DeviceName), ret);

            /* Check if it is really detached */
            memset(&mode, 0, sizeof(mode));
            mode.dmSize = sizeof(mode);
            ret = EnumDisplaySettingsW(display_device.DeviceName, ENUM_CURRENT_SETTINGS, &mode);
            /* Win10 TestBots may return FALSE but it's actually successful */
            ok(ret || broken(!ret) ,
                    "Adapter %u output %u: EnumDisplaySettingsW failed for %s, error %#lx.\n",
                    adapter_index, output_index, wine_dbgstr_w(display_device.DeviceName),
                    GetLastError());
            if (mode.dmPelsWidth && mode.dmPelsHeight)
            {
                skip("Adapter %u output %u: Failed to detach device %s.\n", adapter_index,
                        output_index, wine_dbgstr_w(display_device.DeviceName));
                IDXGIOutput_Release(output);
                continue;
            }

            /* Only the AttachedToDesktop field is updated after an output is detached.
             * IDXGIAdapter_EnumOutputs() has to be called again to get other fields updated.
             * But resolution changes are reflected right away. This weird behaviour is currently
             * unimplemented in Wine */
            memset(&output_desc, 0, sizeof(output_desc));
            hr = IDXGIOutput_GetDesc(output, &output_desc);
            ok(hr == S_OK, "Adapter %u output %u: Got unexpected hr %#lx.\n", adapter_index,
                    output_index, hr);
            ok(!lstrcmpiW(output_desc.DeviceName, old_output_desc.DeviceName),
                    "Adapter %u output %u: Expect device name %s, got %s.\n", adapter_index,
                    output_index, wine_dbgstr_w(old_output_desc.DeviceName),
                    wine_dbgstr_w(output_desc.DeviceName));
            todo_wine
            ok(EqualRect(&output_desc.DesktopCoordinates, &old_output_desc.DesktopCoordinates),
                    "Adapter %u output %u: Expect desktop coordinates %s, got %s.\n",
                    adapter_index, output_index,
                    wine_dbgstr_rect(&old_output_desc.DesktopCoordinates),
                    wine_dbgstr_rect(&output_desc.DesktopCoordinates));
            ok(!output_desc.AttachedToDesktop,
                    "Adapter %u output %u: Expect output not attached to desktop.\n", adapter_index,
                    output_index);
            ok(output_desc.Rotation == old_output_desc.Rotation,
                    "Adapter %u output %u: Expect rotation %#x, got %#x.\n", adapter_index,
                    output_index, old_output_desc.Rotation, output_desc.Rotation);
            todo_wine
            ok(output_desc.Monitor == old_output_desc.Monitor,
                    "Adapter %u output %u: Expect monitor %p, got %p.\n", adapter_index,
                    output_index, old_output_desc.Monitor, output_desc.Monitor);
            IDXGIOutput_Release(output);

            /* Call IDXGIAdapter_EnumOutputs() again to get up-to-date output description */
            hr = IDXGIAdapter_EnumOutputs(adapter, output_index, &output);
            ok(hr == S_OK, "Adapter %u output %u: Got unexpected hr %#lx.\n", adapter_index,
                    output_index, hr);
            memset(&output_desc, 0, sizeof(output_desc));
            hr = IDXGIOutput_GetDesc(output, &output_desc);
            ok(hr == S_OK, "Adapter %u output %u: Got unexpected hr %#lx.\n", adapter_index,
                    output_index, hr);
            ok(!lstrcmpiW(output_desc.DeviceName, display_device.DeviceName),
                    "Adapter %u output %u: Expect device name %s, got %s.\n", adapter_index,
                    output_index, wine_dbgstr_w(display_device.DeviceName),
                    wine_dbgstr_w(output_desc.DeviceName));
            ok(IsRectEmpty(&output_desc.DesktopCoordinates),
                    "Adapter %u output %u: Expect desktop rect empty, got %s.\n", adapter_index,
                    output_index, wine_dbgstr_rect(&output_desc.DesktopCoordinates));
            ok(!output_desc.AttachedToDesktop,
                    "Adapter %u output %u: Expect output not attached to desktop.\n", adapter_index,
                    output_index);
            ok(output_desc.Rotation == DXGI_MODE_ROTATION_IDENTITY,
                    "Adapter %u output %u: Expect rotation %#x, got %#x.\n", adapter_index,
                    output_index, DXGI_MODE_ROTATION_IDENTITY, output_desc.Rotation);
            ok(!output_desc.Monitor, "Adapter %u output %u: Expect monitor NULL.\n", adapter_index,
                    output_index);

            /* Restore settings */
            ret = ChangeDisplaySettingsExW(display_device.DeviceName, &old_mode, NULL,
                    CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
            ok(ret == DISP_CHANGE_SUCCESSFUL,
                    "Adapter %u output %u: ChangeDisplaySettingsExW %s returned unexpected %ld.\n",
                    adapter_index, output_index, wine_dbgstr_w(display_device.DeviceName), ret);
            ret = ChangeDisplaySettingsExW(display_device.DeviceName, NULL, NULL, 0, NULL);
            ok(ret == DISP_CHANGE_SUCCESSFUL,
                    "Adapter %u output %u: ChangeDisplaySettingsExW %s returned unexpected %ld.\n",
                    adapter_index, output_index, wine_dbgstr_w(display_device.DeviceName), ret);

            IDXGIOutput_Release(output);
        }

        IDXGIAdapter_Release(adapter);
    }

    /* Windows 8+ always have a WARP adapter present at the end. */
    todo_wine ok(adapter_index >= 2 || broken(adapter_index < 2) /* Windows 7 and before */,
            "Got unexpected adapter count %u.\n", adapter_index);
    if (adapter_index < 2)
    {
        todo_wine win_skip("WARP adapter missing, skipping tests.\n");
        goto done;
    }

    hr = IDXGIFactory_EnumAdapters(factory, adapter_index - 1, &adapter);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGIAdapter_GetDesc(adapter, &adapter_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    todo_wine ok(!lstrcmpW(adapter_desc.Description, L"Microsoft Basic Render Driver"),
            "Got unexpected description %s.\n", wine_dbgstr_w(adapter_desc.Description));
    todo_wine ok(adapter_desc.VendorId == 0x1414,
            "Got unexpected vendor ID %#x.\n", adapter_desc.VendorId);
    todo_wine ok(adapter_desc.DeviceId == 0x008c,
            "Got unexpected device ID %#x.\n", adapter_desc.DeviceId);
    ok(adapter_desc.SubSysId == 0x0000,
            "Got unexpected sub-system ID %#x.\n", adapter_desc.SubSysId);
    ok(adapter_desc.Revision == 0x0000,
            "Got unexpected revision %#x.\n", adapter_desc.Revision);
    todo_wine ok(!adapter_desc.DedicatedVideoMemory,
            "Got unexpected DedicatedVideoMemory %#Ix.\n", adapter_desc.DedicatedVideoMemory);
    ok(!adapter_desc.DedicatedSystemMemory,
            "Got unexpected DedicatedSystemMemory %#Ix.\n", adapter_desc.DedicatedSystemMemory);

    hr = IDXGIAdapter_QueryInterface(adapter, &IID_IDXGIAdapter1, (void **)&adapter1);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE), "Got unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IDXGIAdapter1_GetDesc1(adapter1, &adapter_desc1);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        todo_wine ok(adapter_desc1.Flags == DXGI_ADAPTER_FLAG_SOFTWARE,
                "Got unexpected flags %#x.\n", adapter_desc1.Flags);
        IDXGIAdapter1_Release(adapter1);
    }

    IDXGIAdapter_Release(adapter);

done:
    IDXGIFactory_Release(factory);

    expected_output_count = GetSystemMetrics(SM_CMONITORS);
    ok(output_count == expected_output_count, "Expect output count %d, got %d\n",
            expected_output_count, output_count);
}

struct message
{
    unsigned int message;
    BOOL check_wparam;
    WPARAM expect_wparam;
};

static BOOL expect_no_messages;
static const struct message *expect_messages;
static const struct message *expect_messages_broken;

static BOOL check_message(const struct message *expected,
        HWND hwnd, unsigned int message, WPARAM wparam, LPARAM lparam)
{
    if (expected->message != message)
        return FALSE;

    if (expected->check_wparam)
    {
        ok(wparam == expected->expect_wparam,
                "Got unexpected wparam %Ix for message %x, expected %Ix.\n",
                wparam, message, expected->expect_wparam);
    }

    return TRUE;
}

static LRESULT CALLBACK test_wndproc(HWND hwnd, unsigned int message, WPARAM wparam, LPARAM lparam)
{
    IDXGISwapChain *swapchain = (IDXGISwapChain *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    static BOOL reentry;
    IDXGIOutput *target;
    HRESULT hr, hr2;
    BOOL fs;

    flaky
    ok(!expect_no_messages, "Got unexpected message %#x, hwnd %p, wparam %#Ix, lparam %#Ix.\n",
            message, hwnd, wparam, lparam);

    ok(!reentry, "Re-entered wndproc in nested SetFullscreenState call\n");

    if (expect_messages)
    {
        if (check_message(expect_messages, hwnd, message, wparam, lparam))
        {
            ++expect_messages;

            if (swapchain)
            {
                reentry = TRUE;

                hr = IDXGISwapChain_GetFullscreenState(swapchain, &fs, NULL);
                ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

                /* Priority of error values. */
                hr = IDXGISwapChain_GetContainingOutput(swapchain, &target);
                ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
                hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, target);
                ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", DXGI_ERROR_INVALID_CALL);
                IDXGIOutput_Release(target);

                hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
                hr2 = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);

                if (fs)
                {
                    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
                    ok(hr2 == DXGI_STATUS_MODE_CHANGE_IN_PROGRESS, "Got unexpected hr %#lx.\n", hr);
                }
                else
                {
                    ok(hr == DXGI_STATUS_MODE_CHANGE_IN_PROGRESS, "Got unexpected hr %#lx.\n", hr);
                    ok(hr2 == S_OK, "Got unexpected hr %#lx.\n", hr);
                }
                reentry = FALSE;
            }
        }
    }

    if (expect_messages_broken)
    {
        if (check_message(expect_messages_broken, hwnd, message, wparam, lparam))
            ++expect_messages_broken;
    }

    return DefWindowProcA(hwnd, message, wparam, lparam);
}

static void test_swapchain_window_messages(IUnknown *device, BOOL is_d3d12)
{
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    IDXGISwapChain *swapchain;
    DXGI_MODE_DESC mode_desc;
    IDXGIFactory *factory;
    ULONG refcount;
    WNDCLASSA wc;
    HWND window;
    HRESULT hr;

    static const struct message enter_fullscreen_messages[] =
    {
        {WM_STYLECHANGING,     TRUE,  GWL_STYLE},
        {WM_STYLECHANGED,      TRUE,  GWL_STYLE},
        {WM_STYLECHANGING,     TRUE,  GWL_EXSTYLE},
        {WM_STYLECHANGED,      TRUE,  GWL_EXSTYLE},
        {WM_WINDOWPOSCHANGING, FALSE, 0},
        {WM_GETMINMAXINFO,     FALSE, 0},
        {WM_NCCALCSIZE,        FALSE, 0},
        {WM_WINDOWPOSCHANGED,  FALSE, 0},
        {WM_MOVE,              FALSE, 0},
        {WM_SIZE,              FALSE, 0},
        {0,                    FALSE, 0},
    };
    static const struct message enter_fullscreen_messages_vista[] =
    {
        {WM_STYLECHANGING,     TRUE,  GWL_STYLE},
        {WM_STYLECHANGED,      TRUE,  GWL_STYLE},
        {WM_WINDOWPOSCHANGING, FALSE, 0},
        {WM_NCCALCSIZE,        FALSE, 0},
        {WM_WINDOWPOSCHANGED,  FALSE, 0},
        {WM_MOVE,              FALSE, 0},
        {WM_SIZE,              FALSE, 0},
        {WM_STYLECHANGING,     TRUE,  GWL_EXSTYLE},
        {WM_STYLECHANGED,      TRUE,  GWL_EXSTYLE},
        {WM_WINDOWPOSCHANGING, FALSE, 0},
        {WM_GETMINMAXINFO,     FALSE, 0},
        {WM_NCCALCSIZE,        FALSE, 0},
        {WM_WINDOWPOSCHANGED,  FALSE, 0},
        {WM_SIZE,              FALSE, 0},
        {0,                    FALSE, 0},
    };
    static const struct message leave_fullscreen_messages[] =
    {
        {WM_STYLECHANGING,     TRUE,  GWL_STYLE},
        {WM_STYLECHANGED,      TRUE,  GWL_STYLE},
        {WM_STYLECHANGING,     TRUE,  GWL_EXSTYLE},
        {WM_STYLECHANGED,      TRUE,  GWL_EXSTYLE},
        {WM_WINDOWPOSCHANGING, FALSE, 0},
        {WM_GETMINMAXINFO,     FALSE, 0},
        {WM_NCCALCSIZE,        FALSE, 0},
        {WM_WINDOWPOSCHANGED,  FALSE, 0},
        {WM_MOVE,              FALSE, 0},
        {WM_SIZE,              FALSE, 0},
        {0,                    FALSE, 0},
    };
    static const struct message resize_target_messages[] =
    {
        {WM_WINDOWPOSCHANGING, FALSE, 0},
        {WM_GETMINMAXINFO,     FALSE, 0},
        {WM_NCCALCSIZE,        FALSE, 0},
        {WM_WINDOWPOSCHANGED,  FALSE, 0},
        {WM_SIZE,              FALSE, 0},
        {0,                    FALSE, 0},
    };

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = test_wndproc;
    wc.lpszClassName = "dxgi_test_wndproc_wc";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");
    window = CreateWindowA("dxgi_test_wndproc_wc", "dxgi_test", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    ok(!!window, "Failed to create window.\n");

    get_factory(device, is_d3d12, &factory);

    swapchain_desc.BufferDesc.Width = 800;
    swapchain_desc.BufferDesc.Height = 600;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 60;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = is_d3d12 ? 2 : 1;
    swapchain_desc.OutputWindow = window;
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = is_d3d12 ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;

    /* create swapchain */
    flush_events();
    expect_no_messages = TRUE;
    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    flush_events();
    expect_no_messages = FALSE;

    /* resize target */
    expect_messages = resize_target_messages;
    memset(&mode_desc, 0, sizeof(mode_desc));
    mode_desc.Width = 800;
    mode_desc.Height = 600;
    hr = IDXGISwapChain_ResizeTarget(swapchain, &mode_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    flush_events();
    ok(!expect_messages->message, "Expected message %#x.\n", expect_messages->message);

    expect_messages = resize_target_messages;
    memset(&mode_desc, 0, sizeof(mode_desc));
    mode_desc.Width = 400;
    mode_desc.Height = 200;
    hr = IDXGISwapChain_ResizeTarget(swapchain, &mode_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    flush_events();
    ok(!expect_messages->message, "Expected message %#x.\n", expect_messages->message);

    /* enter fullscreen */
    SetWindowLongPtrA(window, GWLP_USERDATA, (LONG_PTR)swapchain);
    expect_messages = enter_fullscreen_messages;
    expect_messages_broken = enter_fullscreen_messages_vista;
    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
    ok(hr == S_OK || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE
             || broken(hr == DXGI_ERROR_UNSUPPORTED), /* Win 7 testbot */
            "Got unexpected hr %#lx.\n", hr);
    SetWindowLongPtrA(window, GWLP_USERDATA, (LONG_PTR)NULL);
    if (FAILED(hr))
    {
        skip("Could not change fullscreen state.\n");
        goto done;
    }
    flush_events();
    todo_wine
    ok(!expect_messages->message || broken(!expect_messages_broken->message),
            "Expected message %#x or %#x.\n",
            expect_messages->message, expect_messages_broken->message);
    expect_messages_broken = NULL;

    /* leave fullscreen */
    expect_messages = leave_fullscreen_messages;
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    flush_events();
    ok(!expect_messages->message, "Expected message %#x.\n", expect_messages->message);
    expect_messages = NULL;

    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);

    /* create fullscreen swapchain */
    DestroyWindow(window);
    window = CreateWindowA("dxgi_test_wndproc_wc", "dxgi_test", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    ok(!!window, "Failed to create window.\n");
    swapchain_desc.OutputWindow = window;
    swapchain_desc.Windowed = FALSE;
    swapchain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    flush_events();

    expect_messages = enter_fullscreen_messages;
    expect_messages_broken = enter_fullscreen_messages_vista;
    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    flush_events();
    todo_wine
    ok(!expect_messages->message || broken(!expect_messages_broken->message),
            "Expected message %#x or %#x.\n",
            expect_messages->message, expect_messages_broken->message);
    expect_messages_broken = NULL;

    /* leave fullscreen */
    SetWindowLongPtrA(window, GWLP_USERDATA, (LONG_PTR)swapchain);
    expect_messages = leave_fullscreen_messages;
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    SetWindowLongPtrA(window, GWLP_USERDATA, (LONG_PTR)NULL);
    flush_events();
    ok(!expect_messages->message, "Expected message %#x.\n", expect_messages->message);
    expect_messages = NULL;

done:
    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);
    DestroyWindow(window);

    refcount = IDXGIFactory_Release(factory);
    ok(refcount == !is_d3d12, "Got unexpected refcount %lu.\n", refcount);

    UnregisterClassA("dxgi_test_wndproc_wc", GetModuleHandleA(NULL));
}

static void test_swapchain_window_styles(void)
{
    LONG style, exstyle, fullscreen_style, fullscreen_exstyle;
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    IDXGISwapChain *swapchain;
    IDXGIFactory *factory;
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    ULONG refcount;
    unsigned int i;
    HRESULT hr;

    static const struct
    {
        LONG style, exstyle;
        LONG expected_style, expected_exstyle;
    }
    tests[] =
    {
        {WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, 0,
         WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPSIBLINGS,
         WS_EX_WINDOWEDGE},
        {WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE, 0,
         WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPSIBLINGS | WS_VISIBLE,
         WS_EX_WINDOWEDGE},
        {WS_OVERLAPPED | WS_VISIBLE, 0,
         WS_OVERLAPPED | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CAPTION, WS_EX_WINDOWEDGE},
        {WS_OVERLAPPED | WS_MAXIMIZE, 0,
         WS_OVERLAPPED | WS_MAXIMIZE | WS_CLIPSIBLINGS | WS_CAPTION, WS_EX_WINDOWEDGE},
        {WS_OVERLAPPED | WS_MINIMIZE, 0,
         WS_OVERLAPPED | WS_MINIMIZE | WS_CLIPSIBLINGS | WS_CAPTION, WS_EX_WINDOWEDGE},
        {WS_CAPTION | WS_DISABLED, WS_EX_TOPMOST,
         WS_CAPTION | WS_DISABLED | WS_CLIPSIBLINGS, WS_EX_TOPMOST | WS_EX_WINDOWEDGE},
        {WS_CAPTION | WS_DISABLED | WS_VISIBLE, WS_EX_TOPMOST,
         WS_CAPTION | WS_DISABLED | WS_VISIBLE | WS_CLIPSIBLINGS, WS_EX_TOPMOST | WS_EX_WINDOWEDGE},
        {WS_CAPTION | WS_SYSMENU | WS_VISIBLE, WS_EX_APPWINDOW,
         WS_CAPTION | WS_SYSMENU | WS_VISIBLE | WS_CLIPSIBLINGS, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE},
        {WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_BORDER | WS_DLGFRAME
         | WS_VSCROLL | WS_HSCROLL | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
         0,
         WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_BORDER | WS_DLGFRAME
         | WS_VSCROLL | WS_HSCROLL | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
         WS_EX_WINDOWEDGE},
    };

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IDXGIAdapter_Release(adapter);

    swapchain_desc.BufferDesc.Width = 800;
    swapchain_desc.BufferDesc.Height = 600;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 60;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 1;
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        winetest_push_context("Test %u", i);

        swapchain_desc.OutputWindow = CreateWindowExA(tests[i].exstyle, "static", "dxgi_test",
                tests[i].style, 0, 0, 400, 200, 0, 0, 0, 0);

        style = GetWindowLongA(swapchain_desc.OutputWindow, GWL_STYLE);
        exstyle = GetWindowLongA(swapchain_desc.OutputWindow, GWL_EXSTYLE);
        ok(style == tests[i].expected_style, "Got unexpected style %#lx, expected %#lx.\n",
                style, tests[i].expected_style);
        flaky_if(i == 4)
        ok(exstyle == tests[i].expected_exstyle, "Got unexpected exstyle %#lx, expected %#lx.\n",
                exstyle, tests[i].expected_exstyle);

        fullscreen_style = tests[i].expected_style & ~(WS_POPUP | WS_MAXIMIZEBOX
                | WS_MINIMIZEBOX | WS_THICKFRAME | WS_SYSMENU | WS_DLGFRAME | WS_BORDER);
        fullscreen_exstyle = tests[i].expected_exstyle & ~(WS_EX_DLGMODALFRAME
                | WS_EX_TOOLWINDOW | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_CONTEXTHELP);
        fullscreen_exstyle |= WS_EX_TOPMOST;

        hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &swapchain_desc, &swapchain);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        style = GetWindowLongA(swapchain_desc.OutputWindow, GWL_STYLE);
        exstyle = GetWindowLongA(swapchain_desc.OutputWindow, GWL_EXSTYLE);
        ok(style == tests[i].expected_style, "Got unexpected style %#lx, expected %#lx.\n",
                style, tests[i].expected_style);
        flaky_if(i == 4)
        ok(exstyle == tests[i].expected_exstyle, "Got unexpected exstyle %#lx, expected %#lx.\n",
                exstyle, tests[i].expected_exstyle);

        hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
        ok(hr == S_OK || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE
                || broken(hr == DXGI_ERROR_UNSUPPORTED), /* Win 7 testbot */
                "Got unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
        {
            style = GetWindowLongA(swapchain_desc.OutputWindow, GWL_STYLE);
            exstyle = GetWindowLongA(swapchain_desc.OutputWindow, GWL_EXSTYLE);
            todo_wine ok(style == fullscreen_style, "Got unexpected style %#lx, expected %#lx.\n",
                    style, fullscreen_style);
            ok(exstyle == fullscreen_exstyle, "Got unexpected exstyle %#lx, expected %#lx.\n",
                    exstyle, fullscreen_exstyle);

            hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        }
        else
        {
            skip("Could not change fullscreen state.\n");
        }

        style = GetWindowLongA(swapchain_desc.OutputWindow, GWL_STYLE);
        exstyle = GetWindowLongA(swapchain_desc.OutputWindow, GWL_EXSTYLE);
        ok(style == tests[i].expected_style, "Got unexpected style %#lx, expected %#lx.\n",
                style, tests[i].expected_style);
        flaky_if(i == 4)
        ok(exstyle == tests[i].expected_exstyle, "Got unexpected exstyle %#lx, expected %#lx.\n",
                exstyle, tests[i].expected_exstyle);

        hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
        ok(hr == S_OK || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE
                || broken(hr == DXGI_ERROR_UNSUPPORTED), /* Win 7 testbot */
                "Got unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
        {
            style = GetWindowLongA(swapchain_desc.OutputWindow, GWL_STYLE);
            exstyle = GetWindowLongA(swapchain_desc.OutputWindow, GWL_EXSTYLE);
            todo_wine ok(style == fullscreen_style, "Got unexpected style %#lx, expected %#lx.\n",
                    style, fullscreen_style);
            ok(exstyle == fullscreen_exstyle, "Got unexpected exstyle %#lx, expected %#lx.\n",
                    exstyle, fullscreen_exstyle);

            SetWindowLongW(swapchain_desc.OutputWindow, GWL_STYLE, fullscreen_style);
            SetWindowLongW(swapchain_desc.OutputWindow, GWL_EXSTYLE, fullscreen_exstyle);

            hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            style = GetWindowLongA(swapchain_desc.OutputWindow, GWL_STYLE);
            exstyle = GetWindowLongA(swapchain_desc.OutputWindow, GWL_EXSTYLE);
            todo_wine ok(style == tests[i].expected_style, "Got unexpected style %#lx, expected %#lx.\n",
                    style, tests[i].expected_style);
            flaky_if(i == 4) todo_wine
            ok(exstyle == tests[i].expected_exstyle, "Got unexpected exstyle %#lx, expected %#lx.\n",
                    exstyle, tests[i].expected_exstyle);
        }
        else
        {
            skip("Could not change fullscreen state.\n");
        }

        refcount = IDXGISwapChain_Release(swapchain);
        ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);

        style = GetWindowLongA(swapchain_desc.OutputWindow, GWL_STYLE);
        exstyle = GetWindowLongA(swapchain_desc.OutputWindow, GWL_EXSTYLE);
        todo_wine ok(style == tests[i].expected_style, "Got unexpected style %#lx, expected %#lx.\n",
                style, tests[i].expected_style);
        flaky_if(i == 4) todo_wine
        ok(exstyle == tests[i].expected_exstyle, "Got unexpected exstyle %#lx, expected %#lx.\n",
                exstyle, tests[i].expected_exstyle);

        DestroyWindow(swapchain_desc.OutputWindow);

        winetest_pop_context();
    }

    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
    refcount = IDXGIFactory_Release(factory);
    ok(!refcount, "Factory has %lu references left.\n", refcount);
}

static void test_gamma_control(void)
{
    DXGI_GAMMA_CONTROL_CAPABILITIES caps;
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    IDXGISwapChain *swapchain;
    DXGI_GAMMA_CONTROL gamma;
    IDXGIFactory *factory;
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    IDXGIOutput *output;
    unsigned int i;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIAdapter_EnumOutputs(adapter, 0, &output);
    if (hr == DXGI_ERROR_NOT_FOUND)
    {
        skip("Adapter doesn't have any outputs.\n");
        IDXGIAdapter_Release(adapter);
        IDXGIDevice_Release(device);
        return;
    }
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIOutput_GetGammaControlCapabilities(output, &caps);
    todo_wine
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
    IDXGIOutput_Release(output);

    swapchain_desc.BufferDesc.Width = 640;
    swapchain_desc.BufferDesc.Height = 480;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 60;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 1;
    swapchain_desc.OutputWindow = create_window();
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;

    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
    ok(hr == S_OK || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE
            || broken(hr == DXGI_ERROR_UNSUPPORTED), /* Win 7 testbot */
            "Got unexpected hr %#lx.\n", hr);
    if (FAILED(hr))
    {
        skip("Could not change fullscreen state.\n");
        goto done;
    }

    hr = IDXGISwapChain_GetContainingOutput(swapchain, &output);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    memset(&caps, 0, sizeof(caps));
    hr = IDXGIOutput_GetGammaControlCapabilities(output, &caps);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ok(caps.MaxConvertedValue > caps.MinConvertedValue
            || broken(caps.MaxConvertedValue == 0.0f && caps.MinConvertedValue == 1.0f) /* WARP */,
            "Expected max gamma value (%.8e) to be bigger than min value (%.8e).\n",
            caps.MaxConvertedValue, caps.MinConvertedValue);

    for (i = 1; i < caps.NumGammaControlPoints; ++i)
    {
        ok(caps.ControlPointPositions[i] > caps.ControlPointPositions[i - 1],
                "Expected control point positions to be strictly monotonically increasing (%.8e > %.8e).\n",
                caps.ControlPointPositions[i], caps.ControlPointPositions[i - 1]);
    }

    memset(&gamma, 0, sizeof(gamma));
    hr = IDXGIOutput_GetGammaControl(output, &gamma);
    todo_wine
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGIOutput_SetGammaControl(output, &gamma);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    IDXGIOutput_Release(output);

    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

done:
    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);
    DestroyWindow(swapchain_desc.OutputWindow);

    IDXGIAdapter_Release(adapter);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
    refcount = IDXGIFactory_Release(factory);
    ok(!refcount, "Factory has %lu references left.\n", refcount);
}

static void test_window_association(IUnknown *device, BOOL is_d3d12)
{
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    LONG_PTR original_wndproc, wndproc;
    IDXGIFactory *factory, *factory2;
    IDXGISwapChain *swapchain;
    IDXGIOutput *output;
    HWND hwnd, hwnd2;
    BOOL fullscreen;
    unsigned int i;
    ULONG refcount;
    HRESULT hr;

    static const struct
    {
        UINT flag;
        BOOL expect_fullscreen;
        BOOL broken_d3d10;
    }
    tests[] =
    {
        /* There are two reasons why VK_TAB and VK_ESC are not tested here:
         *
         * - Posting them to the window doesn't exit fullscreen like
         *   Alt+Enter does. Alt+Tab and Alt+Esc are handled somewhere else.
         *   E.g., not calling IDXGISwapChain::Present() will break Alt+Tab
         *   and Alt+Esc while Alt+Enter will still function.
         *
         * - Posting them hangs the posting thread. Another thread that keeps
         *   sending input is needed to avoid the hang. The hang is not
         *   because of flush_events(). */
        {0, TRUE},
        {0, FALSE},
        {DXGI_MWA_NO_WINDOW_CHANGES, FALSE},
        {DXGI_MWA_NO_WINDOW_CHANGES, FALSE},
        {DXGI_MWA_NO_ALT_ENTER, FALSE, TRUE},
        {DXGI_MWA_NO_ALT_ENTER, FALSE},
        {DXGI_MWA_NO_PRINT_SCREEN, TRUE},
        {DXGI_MWA_NO_PRINT_SCREEN, FALSE},
        {0, TRUE},
        {0, FALSE}
    };

    swapchain_desc.BufferDesc.Width = 640;
    swapchain_desc.BufferDesc.Height = 480;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 1;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = is_d3d12 ? 2 : 1;
    swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = is_d3d12 ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;

    original_wndproc = GetWindowLongPtrW(swapchain_desc.OutputWindow, GWLP_WNDPROC);

    hwnd2 = CreateWindowA("static", "dxgi_test2", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    hr = CreateDXGIFactory(&IID_IDXGIFactory, (void **)&factory2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    get_factory(device, is_d3d12, &factory);

    hr = IDXGIFactory_GetWindowAssociation(factory, NULL);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);

    for (i = 0; i <= DXGI_MWA_VALID; ++i)
    {
        hr = IDXGIFactory_MakeWindowAssociation(factory, NULL, i);
        ok(hr == S_OK, "Got unexpected hr %#lx for flags %#x.\n", hr, i);

        hr = IDXGIFactory_MakeWindowAssociation(factory, swapchain_desc.OutputWindow, i);
        ok(hr == S_OK, "Got unexpected hr %#lx for flags %#x.\n", hr, i);

        wndproc = GetWindowLongPtrW(swapchain_desc.OutputWindow, GWLP_WNDPROC);
        ok(wndproc == original_wndproc, "Got unexpected wndproc %#Ix, expected %#Ix for flags %#x.\n",
                wndproc, original_wndproc, i);

        hwnd = (HWND)0xdeadbeef;
        hr = IDXGIFactory_GetWindowAssociation(factory, &hwnd);
        ok(hr == S_OK, "Got unexpected hr %#lx for flags %#x.\n", hr, i);
        /* Apparently GetWindowAssociation() always returns NULL, even when
         * MakeWindowAssociation() and GetWindowAssociation() are both
         * successfully called. */
        ok(!hwnd, "Expect null associated window.\n");
    }

    hr = IDXGIFactory_MakeWindowAssociation(factory, swapchain_desc.OutputWindow, DXGI_MWA_VALID + 1);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);

    /* Alt+Enter tests. */
    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    wndproc = GetWindowLongPtrW(swapchain_desc.OutputWindow, GWLP_WNDPROC);
    ok(wndproc == original_wndproc, "Got unexpected wndproc %#Ix, expected %#Ix.\n", wndproc, original_wndproc);

    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
    ok(hr == S_OK || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE
            || broken(hr == DXGI_ERROR_UNSUPPORTED) /* Windows 7 testbot */,
            "Got unexpected hr %#lx.\n", hr);
    if (FAILED(hr))
    {
        skip("Could not change fullscreen state.\n");
    }
    else
    {
        hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        for (i = 0; i < ARRAY_SIZE(tests); ++i)
        {
            winetest_push_context("Test %u", i);

            /* First associate a window with the opposite flags. */
            hr = IDXGIFactory_MakeWindowAssociation(factory, hwnd2, ~tests[i].flag & DXGI_MWA_VALID);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            /* Associate the current test window. */
            hwnd = tests[i].flag ? swapchain_desc.OutputWindow : NULL;
            hr = IDXGIFactory_MakeWindowAssociation(factory, hwnd, tests[i].flag);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            /* Associating a new test window doesn't override the old window. */
            hr = IDXGIFactory_MakeWindowAssociation(factory, hwnd2, ~tests[i].flag & DXGI_MWA_VALID);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            /* Associations with a different factory don't affect the existing
             * association. */
            hr = IDXGIFactory_MakeWindowAssociation(factory2, hwnd, ~tests[i].flag & DXGI_MWA_VALID);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            /* Post synthesized Alt + VK_RETURN WM_SYSKEYDOWN. */
            PostMessageA(swapchain_desc.OutputWindow, WM_SYSKEYDOWN, VK_RETURN,
                    (MapVirtualKeyA(VK_RETURN, MAPVK_VK_TO_VSC) << 16) | 0x20000001);
            flush_events();
            output = NULL;
            hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, &output);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
            ok(fullscreen == tests[i].expect_fullscreen
                    || broken(tests[i].broken_d3d10 && fullscreen),
                    "Got unexpected fullscreen %#x.\n", fullscreen);
            ok(fullscreen ? !!output : !output, "Got unexpected output.\n");
            if (output)
                IDXGIOutput_Release(output);

            wndproc = GetWindowLongPtrW(swapchain_desc.OutputWindow, GWLP_WNDPROC);
            ok(wndproc == original_wndproc, "Got unexpected wndproc %#Ix, expected %#Ix.\n",
                    wndproc, original_wndproc);

            winetest_pop_context();
        }
    }

    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    refcount = IDXGIFactory_Release(factory2);
    ok(!refcount, "Factory has %lu references left.\n", refcount);
    DestroyWindow(hwnd2);

    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);
    DestroyWindow(swapchain_desc.OutputWindow);

    refcount = IDXGIFactory_Release(factory);
    ok(refcount == !is_d3d12, "IDXGIFactory has %lu references left.\n", refcount);
}

static void test_output_ownership(IUnknown *device, BOOL is_d3d12)
{
    D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME open_adapter_gdi_desc;
    D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP check_ownership_desc;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    DXGI_OUTPUT_DESC output_desc;
    IDXGISwapChain *swapchain;
    IDXGIFactory *factory;
    IDXGIAdapter *adapter;
    IDXGIOutput *output;
    BOOL fullscreen;
    NTSTATUS status;
    ULONG refcount;
    HRESULT hr;

    if (!pD3DKMTCheckVidPnExclusiveOwnership
            || pD3DKMTCheckVidPnExclusiveOwnership(NULL) == STATUS_PROCEDURE_NOT_FOUND)
    {
        win_skip("D3DKMTCheckVidPnExclusiveOwnership() is unavailable.\n");
        return;
    }

    get_factory(device, is_d3d12, &factory);
    adapter = get_adapter(device, is_d3d12);
    if (!adapter)
    {
        skip("Failed to get adapter on Direct3D %d.\n", is_d3d12 ? 12 : 10);
        IDXGIFactory_Release(factory);
        return;
    }

    hr = IDXGIAdapter_EnumOutputs(adapter, 0, &output);
    IDXGIAdapter_Release(adapter);
    if (hr == DXGI_ERROR_NOT_FOUND)
    {
        skip("Adapter doesn't have any outputs.\n");
        IDXGIFactory_Release(factory);
        return;
    }
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIOutput_GetDesc(output, &output_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    lstrcpyW(open_adapter_gdi_desc.DeviceName, output_desc.DeviceName);
    status = pD3DKMTOpenAdapterFromGdiDisplayName(&open_adapter_gdi_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);

    check_ownership_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    check_ownership_desc.VidPnSourceId = open_adapter_gdi_desc.VidPnSourceId;
    status = get_expected_vidpn_exclusive_ownership(&check_ownership_desc, STATUS_SUCCESS);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx, expected %#lx.\n", status,
            STATUS_SUCCESS);

    swapchain_desc.BufferDesc.Width = 800;
    swapchain_desc.BufferDesc.Height = 600;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 1;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = is_d3d12 ? 2 : 1;
    swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 400, 200, NULL, NULL, NULL, NULL);
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = is_d3d12 ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;
    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Swapchain in fullscreen mode. */
    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, output);
    /* DXGI_ERROR_NOT_CURRENTLY_AVAILABLE on some machines.
     * DXGI_ERROR_UNSUPPORTED on the Windows 7 testbot. */
    if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE || broken(hr == DXGI_ERROR_UNSUPPORTED))
    {
        skip("Failed to change fullscreen state.\n");
        goto done;
    }
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    fullscreen = FALSE;
    hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(fullscreen, "Got unexpected fullscreen state.\n");
    /* Win10 1909 doesn't seem to grab output exclusive ownership.
     * And all output ownership calls return S_OK on D3D10 and D3D12 with 1909. */
    if (is_d3d12)
    {
        status = get_expected_vidpn_exclusive_ownership(&check_ownership_desc, STATUS_SUCCESS);
        ok(status == STATUS_SUCCESS, "Got unexpected status %#lx, expected %#lx.\n", status,
                STATUS_SUCCESS);
    }
    else
    {
        status = get_expected_vidpn_exclusive_ownership(&check_ownership_desc,
                STATUS_GRAPHICS_PRESENT_OCCLUDED);
        todo_wine ok(status == STATUS_GRAPHICS_PRESENT_OCCLUDED ||
                broken(status == STATUS_SUCCESS), /* Win10 1909 */
                "Got unexpected status %#lx, expected %#lx.\n", status,
                STATUS_GRAPHICS_PRESENT_OCCLUDED);
    }
    hr = IDXGIOutput_TakeOwnership(output, NULL, FALSE);
    ok(hr == DXGI_ERROR_INVALID_CALL || broken(hr == S_OK), /* Win10 1909 */
            "Got unexpected hr %#lx.\n", hr);
    hr = IDXGIOutput_TakeOwnership(output, NULL, TRUE);
    ok(hr == DXGI_ERROR_INVALID_CALL || broken(hr == S_OK), /* Win10 1909 */
            "Got unexpected hr %#lx.\n", hr);
    hr = IDXGIOutput_TakeOwnership(output, device, FALSE);
    if (is_d3d12)
        todo_wine ok(hr == E_NOINTERFACE || hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    else
        todo_wine ok(hr == E_INVALIDARG || broken(hr == S_OK), /* Win10 1909 */
                "Got unexpected hr %#lx.\n", hr);
    hr = IDXGIOutput_TakeOwnership(output, device, TRUE);
    ok(hr == E_NOINTERFACE || hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IDXGIOutput_ReleaseOwnership(output);
    status = get_expected_vidpn_exclusive_ownership(&check_ownership_desc, STATUS_SUCCESS);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx, expected %#lx.\n", status,
            STATUS_SUCCESS);

    /* IDXGIOutput_TakeOwnership always returns E_NOINTERFACE for d3d12. Tests
     * finished. */
    if (is_d3d12)
        goto done;

    hr = IDXGIOutput_TakeOwnership(output, device, FALSE);
    ok(hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE || broken(hr == S_OK), /* Win10 1909 */
            "Got unexpected hr %#lx.\n", hr);
    IDXGIOutput_ReleaseOwnership(output);

    hr = IDXGIOutput_TakeOwnership(output, device, TRUE);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    /* Note that the "exclusive" parameter to IDXGIOutput_TakeOwnership()
     * seems to behave opposite to what's described by MSDN. */
    status = get_expected_vidpn_exclusive_ownership(&check_ownership_desc,
            STATUS_GRAPHICS_PRESENT_OCCLUDED);
    ok(status == STATUS_GRAPHICS_PRESENT_OCCLUDED ||
            broken(status == STATUS_SUCCESS), /* Win10 1909 */
            "Got unexpected status %#lx, expected %#lx.\n", status, STATUS_GRAPHICS_PRESENT_OCCLUDED);
    hr = IDXGIOutput_TakeOwnership(output, device, FALSE);
    ok(hr == E_INVALIDARG || broken(hr == S_OK) /* Win10 1909 */, "Got unexpected hr %#lx.\n", hr);
    IDXGIOutput_ReleaseOwnership(output);

    /* Swapchain in windowed mode. */
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    fullscreen = TRUE;
    hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!fullscreen, "Unexpected fullscreen state.\n");
    status = get_expected_vidpn_exclusive_ownership(&check_ownership_desc, STATUS_SUCCESS);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx, expected %#lx.\n", status,
            STATUS_SUCCESS);

    hr = IDXGIOutput_TakeOwnership(output, device, FALSE);
    ok(hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE || broken(hr == S_OK), /* Win10 1909 */
            "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIOutput_TakeOwnership(output, device, TRUE);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    status = get_expected_vidpn_exclusive_ownership(&check_ownership_desc,
            STATUS_GRAPHICS_PRESENT_OCCLUDED);
    ok(status == STATUS_GRAPHICS_PRESENT_OCCLUDED || broken(hr == S_OK), /* Win10 1909 */
            "Got unexpected status %#lx, expected %#lx.\n", status, STATUS_GRAPHICS_PRESENT_OCCLUDED);
    IDXGIOutput_ReleaseOwnership(output);
    status = get_expected_vidpn_exclusive_ownership(&check_ownership_desc, STATUS_SUCCESS);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx, expected %#lx.\n", status,
            STATUS_SUCCESS);

done:
    IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    wait_device_idle(device);

    IDXGIOutput_Release(output);
    IDXGISwapChain_Release(swapchain);
    DestroyWindow(swapchain_desc.OutputWindow);
    refcount = IDXGIFactory_Release(factory);
    ok(refcount == !is_d3d12, "Got unexpected refcount %lu.\n", refcount);

    close_adapter_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = pD3DKMTCloseAdapter(&close_adapter_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
}

static void test_cursor_clipping(IUnknown *device, BOOL is_d3d12)
{
    unsigned int adapter_idx, output_idx, mode_idx, mode_count;
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    DXGI_OUTPUT_DESC output_desc;
    IDXGIAdapter *adapter = NULL;
    RECT virtual_rect, clip_rect;
    unsigned int width, height;
    IDXGISwapChain *swapchain;
    DXGI_MODE_DESC *modes;
    IDXGIFactory *factory;
    IDXGIOutput *output;
    ULONG refcount;
    HRESULT hr;
    BOOL ret;

    get_factory(device, is_d3d12, &factory);

    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = is_d3d12 ? 2 : 1;
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = is_d3d12 ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    for (adapter_idx = 0; SUCCEEDED(IDXGIFactory_EnumAdapters(factory, adapter_idx, &adapter));
            ++adapter_idx)
    {
        winetest_push_context("Adapter %u", adapter_idx);

        for (output_idx = 0; SUCCEEDED(IDXGIAdapter_EnumOutputs(adapter, output_idx, &output));
                ++output_idx)
        {
            winetest_push_context("Output %u", output_idx);

            hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM, 0, &mode_count, NULL);
            ok(hr == S_OK || broken(hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE), /* Win 7 TestBots */
                    "Got unexpected hr %#lx.\n", hr);
            if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
            {
                win_skip("GetDisplayModeList() not supported.\n");
                IDXGIOutput_Release(output);
                winetest_pop_context();
                continue;
            }

            modes = calloc(mode_count, sizeof(*modes));
            hr = IDXGIOutput_GetDisplayModeList(output, DXGI_FORMAT_R8G8B8A8_UNORM, 0, &mode_count, modes);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            hr = IDXGIOutput_GetDesc(output, &output_desc);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
            width = output_desc.DesktopCoordinates.right - output_desc.DesktopCoordinates.left;
            height = output_desc.DesktopCoordinates.bottom - output_desc.DesktopCoordinates.top;
            for (mode_idx = 0; mode_idx < mode_count; ++mode_idx)
            {
                if (modes[mode_idx].Width != width && modes[mode_idx].Height != height)
                    break;
            }
            ok(modes[mode_idx].Width != width && modes[mode_idx].Height != height,
                    "Failed to find a different mode than %ux%u.\n", width, height);

            ret = ClipCursor(NULL);
            ok(ret, "ClipCursor failed, error %#lx.\n", GetLastError());
            get_virtual_rect(&virtual_rect);
            ret = GetClipCursor(&clip_rect);
            ok(ret, "GetClipCursor failed, error %#lx.\n", GetLastError());
            ok(EqualRect(&clip_rect, &virtual_rect), "Expect clip rect %s, got %s.\n",
                    wine_dbgstr_rect(&virtual_rect), wine_dbgstr_rect(&clip_rect));

            swapchain_desc.BufferDesc.Width = modes[mode_idx].Width;
            swapchain_desc.BufferDesc.Height = modes[mode_idx].Height;
            swapchain_desc.BufferDesc.RefreshRate = modes[mode_idx].RefreshRate;
            swapchain_desc.BufferDesc.Format = modes[mode_idx].Format;
            swapchain_desc.BufferDesc.ScanlineOrdering = modes[mode_idx].ScanlineOrdering;
            swapchain_desc.BufferDesc.Scaling = modes[mode_idx].Scaling;
            swapchain_desc.OutputWindow = create_window();
            free(modes);
            hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            flush_events();
            get_virtual_rect(&virtual_rect);
            ret = GetClipCursor(&clip_rect);
            ok(ret, "GetClipCursor failed, error %#lx.\n", GetLastError());
            ok(EqualRect(&clip_rect, &virtual_rect), "Expect clip rect %s, got %s.\n",
                    wine_dbgstr_rect(&virtual_rect), wine_dbgstr_rect(&clip_rect));

            hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
            ok(hr == S_OK || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE
                    || broken(hr == DXGI_ERROR_UNSUPPORTED), /* Win 7 testbot */
                    "Got unexpected hr %#lx.\n", hr);
            if (FAILED(hr))
            {
                skip("Could not change fullscreen state, hr %#lx.\n", hr);
                IDXGISwapChain_Release(swapchain);
                IDXGIOutput_Release(output);
                DestroyWindow(swapchain_desc.OutputWindow);
                winetest_pop_context();
                continue;
            }

            flush_events();
            get_virtual_rect(&virtual_rect);
            ret = GetClipCursor(&clip_rect);
            ok(ret, "GetClipCursor failed, error %#lx.\n", GetLastError());
            ok(EqualRect(&clip_rect, &virtual_rect), "Expect clip rect %s, got %s.\n",
                    wine_dbgstr_rect(&virtual_rect), wine_dbgstr_rect(&clip_rect));

            hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
            refcount = IDXGISwapChain_Release(swapchain);
            ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);
            refcount = IDXGIOutput_Release(output);
            ok(!refcount, "IDXGIOutput has %lu references left.\n", refcount);
            DestroyWindow(swapchain_desc.OutputWindow);

            flush_events();
            get_virtual_rect(&virtual_rect);
            ret = GetClipCursor(&clip_rect);
            ok(ret, "GetClipCursor failed, error %#lx.\n", GetLastError());
            ok(EqualRect(&clip_rect, &virtual_rect), "Expect clip rect %s, got %s.\n",
                    wine_dbgstr_rect(&virtual_rect), wine_dbgstr_rect(&clip_rect));

            winetest_pop_context();
        }

        IDXGIAdapter_Release(adapter);

        winetest_pop_context();
    }

    refcount = IDXGIFactory_Release(factory);
    ok(refcount == !is_d3d12, "Got unexpected refcount %lu.\n", refcount);
}

static void test_factory_check_feature_support(void)
{
    IDXGIFactory5 *factory;
    ULONG ref_count;
    HRESULT hr;
    BOOL data;

    if (FAILED(hr = CreateDXGIFactory(&IID_IDXGIFactory5, (void**)&factory)))
    {
        win_skip("IDXGIFactory5 is not available.\n");
        return;
    }

    hr = IDXGIFactory5_CheckFeatureSupport(factory, 0x12345678, (void *)&data, sizeof(data));
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);

    /* Crashes on Windows. */
    if (0)
    {
        hr = IDXGIFactory5_CheckFeatureSupport(factory, DXGI_FEATURE_PRESENT_ALLOW_TEARING, NULL, sizeof(data));
        ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
    }

    hr = IDXGIFactory5_CheckFeatureSupport(factory, DXGI_FEATURE_PRESENT_ALLOW_TEARING, &data, sizeof(data) - 1);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIFactory5_CheckFeatureSupport(factory, DXGI_FEATURE_PRESENT_ALLOW_TEARING, &data, sizeof(data) + 1);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);

    data = (BOOL)0xdeadbeef;
    hr = IDXGIFactory5_CheckFeatureSupport(factory, DXGI_FEATURE_PRESENT_ALLOW_TEARING, &data, sizeof(data));
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(data == TRUE || data == FALSE, "Got unexpected data %#x.\n", data);

    ref_count = IDXGIFactory5_Release(factory);
    ok(!ref_count, "Factory has %lu references left.\n", ref_count);
}

static void test_frame_latency_event(IUnknown *device, BOOL is_d3d12)
{
    DXGI_SWAP_CHAIN_DESC1 swapchain_desc;
    HANDLE semaphore, semaphore2;
    IDXGISwapChain2 *swapchain2;
    IDXGISwapChain1 *swapchain1;
    ID3D12Device *d3d12_device;
    ID3D12CommandQueue *queue;
    IDXGIFactory2 *factory2;
    IDXGIFactory *factory;
    ID3D12Fence *fence;
    UINT frame_latency;
    DWORD wait_result;
    ULONG ref_count;
    unsigned int i;
    HWND window;
    HRESULT hr;
    BOOL ret;

    get_factory(device, is_d3d12, &factory);

    hr = IDXGIFactory_QueryInterface(factory, &IID_IDXGIFactory2, (void**)&factory2);
    IDXGIFactory_Release(factory);
    if (FAILED(hr))
    {
        win_skip("IDXGIFactory2 not available.\n");
        return;
    }

    window = create_window();

    swapchain_desc.Width = 640;
    swapchain_desc.Height = 480;
    swapchain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.Stereo = FALSE;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 2;
    swapchain_desc.Scaling = DXGI_SCALING_STRETCH;
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapchain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapchain_desc.Flags = 0;

    hr = IDXGIFactory2_CreateSwapChainForHwnd(factory2, device,
            window, &swapchain_desc, NULL, NULL, &swapchain1);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGISwapChain1_QueryInterface(swapchain1, &IID_IDXGISwapChain2, (void**)&swapchain2);
    IDXGISwapChain1_Release(swapchain1);
    if (FAILED(hr))
    {
        win_skip("IDXGISwapChain2 not available.\n");
        IDXGIFactory2_Release(factory2);
        DestroyWindow(window);
        return;
    }

    /* test swap chain without waitable object */
    frame_latency = 0xdeadbeef;
    hr = IDXGISwapChain2_GetMaximumFrameLatency(swapchain2, &frame_latency);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
    ok(frame_latency == 0xdeadbeef, "Got unexpected frame latency %#x.\n", frame_latency);
    hr = IDXGISwapChain2_SetMaximumFrameLatency(swapchain2, 1);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
    semaphore = IDXGISwapChain2_GetFrameLatencyWaitableObject(swapchain2);
    ok(!semaphore, "Got unexpected semaphore %p.\n", semaphore);

    ref_count = IDXGISwapChain2_Release(swapchain2);
    ok(!ref_count, "Swap chain has %lu references left.\n", ref_count);

    /* test swap chain with waitable object */
    swapchain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

    hr = IDXGIFactory2_CreateSwapChainForHwnd(factory2, device,
            window, &swapchain_desc, NULL, NULL, &swapchain1);
    ok(hr == S_OK, "Failed to create swap chain, hr %#lx.\n", hr);
    hr = IDXGISwapChain1_QueryInterface(swapchain1, &IID_IDXGISwapChain2, (void**)&swapchain2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IDXGISwapChain1_Release(swapchain1);

    semaphore = IDXGISwapChain2_GetFrameLatencyWaitableObject(swapchain2);
    ok(!!semaphore, "Got unexpected NULL semaphore.\n");

    /* a new duplicate handle is returned each time */
    semaphore2 = IDXGISwapChain2_GetFrameLatencyWaitableObject(swapchain2);
    ok(!!semaphore2, "Got unexpected NULL semaphore.\n");
    ok(semaphore != semaphore2, "Got the same semaphore twice %p.\n", semaphore);

    ret = CloseHandle(semaphore);
    ok(!!ret, "Failed to close handle, last error %lu.\n", GetLastError());
    ret = CloseHandle(semaphore2);
    ok(!!ret, "Failed to close handle, last error %lu.\n", GetLastError());

    semaphore = IDXGISwapChain2_GetFrameLatencyWaitableObject(swapchain2);
    ok(!!semaphore, "Got unexpected NULL semaphore.\n");

    wait_result = WaitForSingleObject(semaphore, 0);
    ok(!wait_result, "Got unexpected wait result %#lx.\n", wait_result);
    wait_result = WaitForSingleObject(semaphore, 0);
    ok(wait_result == WAIT_TIMEOUT, "Got unexpected wait result %#lx.\n", wait_result);

    hr = IDXGISwapChain2_GetMaximumFrameLatency(swapchain2, &frame_latency);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(frame_latency == 1, "Got unexpected frame latency %#x.\n", frame_latency);

    hr = IDXGISwapChain2_SetMaximumFrameLatency(swapchain2, 0);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain2_GetMaximumFrameLatency(swapchain2, &frame_latency);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(frame_latency == 1, "Got unexpected frame latency %#x.\n", frame_latency);

    /* raising the maximum frame latency releases the semaphore the
     * corresponding number of times */
    hr = IDXGISwapChain2_SetMaximumFrameLatency(swapchain2, 3);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain2_GetMaximumFrameLatency(swapchain2, &frame_latency);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(frame_latency == 3, "Got unexpected frame latency %#x.\n", frame_latency);

    wait_result = WaitForSingleObject(semaphore, 0);
    todo_wine ok(!wait_result, "Got unexpected wait result %#lx.\n", wait_result);
    wait_result = WaitForSingleObject(semaphore, 0);
    todo_wine ok(!wait_result, "Got unexpected wait result %#lx.\n", wait_result);
    wait_result = WaitForSingleObject(semaphore, 100);
    ok(wait_result == WAIT_TIMEOUT, "Got unexpected wait result %#lx.\n", wait_result);

    /* lowering the maximum frame latency doesn't seem to impact the
     * semaphore */
    hr = IDXGISwapChain2_SetMaximumFrameLatency(swapchain2, 1);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain2_GetMaximumFrameLatency(swapchain2, &frame_latency);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(frame_latency == 1, "Got unexpected frame latency %#x.\n", frame_latency);

    wait_result = WaitForSingleObject(semaphore, 100);
    ok(wait_result == WAIT_TIMEOUT, "Got unexpected wait result %#lx.\n", wait_result);

    for (i = 0; i < 5; i++)
    {
        hr = IDXGISwapChain2_Present(swapchain2, 0, 0);
        ok(hr == S_OK, "Present %u failed with hr %#lx.\n", i, hr);

        wait_result = WaitForSingleObject(semaphore, 100);
        ok(!wait_result, "Got unexpected wait result %#lx.\n", wait_result);
    }

    wait_result = WaitForSingleObject(semaphore, 100);
    ok(wait_result == WAIT_TIMEOUT, "Got unexpected wait result %#lx.\n", wait_result);

    /* each frame presentation releases the semaphore */
    for (i = 0; i < 5; i++)
    {
        hr = IDXGISwapChain2_Present(swapchain2, 0, 0);
        ok(hr == S_OK, "Present %u failed with hr %#lx.\n", i, hr);
    }

    Sleep(100);

    for (i = 0; i < 5; i++)
    {
        wait_result = WaitForSingleObject(semaphore, 100);
        todo_wine_if(i != 0)
        ok(!wait_result, "Got unexpected wait result %#lx.\n", wait_result);
    }

    wait_result = WaitForSingleObject(semaphore, 100);
    ok(wait_result == WAIT_TIMEOUT, "Got unexpected wait result %#lx.\n", wait_result);

    if (is_d3d12)
    {
        hr = IUnknown_QueryInterface(device, &IID_ID3D12CommandQueue, (void **)&queue);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = ID3D12CommandQueue_GetDevice(queue, &IID_ID3D12Device, (void **)&d3d12_device);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = ID3D12Device_CreateFence(d3d12_device, 0, 0, &IID_ID3D12Fence, (void **)&fence);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        /* the semaphore is released when the frame is really
         * presented, not when Present() is called */
        for (i = 0; i < 3; i++)
        {
            hr = IDXGISwapChain2_Present(swapchain2, 0, 0);
            ok(hr == S_OK, "Present %u failed with hr %#lx.\n", i, hr);
        }

        hr = ID3D12CommandQueue_Wait(queue, fence, 1);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        for (i = 0; i < 4; i++)
        {
            hr = IDXGISwapChain2_Present(swapchain2, 0, 0);
            ok(hr == S_OK, "Present %u failed with hr %#lx.\n", i, hr);
        }

        for (i = 0; i < 3; i++)
        {
            wait_result = WaitForSingleObject(semaphore, 100);
            todo_wine_if(i != 0)
            ok(!wait_result, "Got unexpected wait result %#lx.\n", wait_result);
        }

        wait_result = WaitForSingleObject(semaphore, 100);
        ok(wait_result == WAIT_TIMEOUT, "Got unexpected wait result %#lx.\n", wait_result);

        hr = ID3D12Fence_Signal(fence, 1);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        Sleep(100);

        for (i = 0; i < 4; i++)
        {
            wait_result = WaitForSingleObject(semaphore, 100);
            todo_wine_if(i != 0)
            ok(!wait_result, "Got unexpected wait result %#lx.\n", wait_result);
        }

        wait_result = WaitForSingleObject(semaphore, 100);
        ok(wait_result == WAIT_TIMEOUT, "Got unexpected wait result %#lx.\n", wait_result);

        ref_count = ID3D12Fence_Release(fence);
        ok(!ref_count, "Fence has %lu references left.\n", ref_count);
        ID3D12Device_Release(d3d12_device);
        ID3D12CommandQueue_Release(queue);
    }

    ref_count = IDXGISwapChain2_Release(swapchain2);
    ok(!ref_count, "Swap chain has %lu references left.\n", ref_count);
    DestroyWindow(window);
    ref_count = IDXGIFactory2_Release(factory2);
    ok(ref_count == !is_d3d12, "Factory has %lu references left.\n", ref_count);
}

static void test_colour_space_support(IUnknown *device, BOOL is_d3d12)
{
    DXGI_SWAP_CHAIN_DESC1 swapchain_desc;
    IDXGISwapChain3 *swapchain3;
    IDXGISwapChain1 *swapchain1;
    IDXGIFactory2 *factory2;
    IDXGIFactory *factory;
    ULONG ref_count;
    unsigned int i;
    UINT support;
    HWND window;
    HRESULT hr;

    static const DXGI_COLOR_SPACE_TYPE colour_spaces[] =
    {
        DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709,
        DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709,
        DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709,
        DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P2020,
        DXGI_COLOR_SPACE_RESERVED,
        DXGI_COLOR_SPACE_YCBCR_FULL_G22_NONE_P709_X601,
        DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P601,
        DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P601,
        DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709,
        DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P709,
        DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P2020,
        DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P2020,
        DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020,
        DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_LEFT_P2020,
        DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020,
        DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_TOPLEFT_P2020,
        DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_TOPLEFT_P2020,
        DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020,
        DXGI_COLOR_SPACE_YCBCR_STUDIO_GHLG_TOPLEFT_P2020,
        DXGI_COLOR_SPACE_YCBCR_FULL_GHLG_TOPLEFT_P2020,
    };

    get_factory(device, is_d3d12, &factory);

    hr = IDXGIFactory_QueryInterface(factory, &IID_IDXGIFactory2, (void**)&factory2);
    IDXGIFactory_Release(factory);
    if (FAILED(hr))
    {
        win_skip("IDXGIFactory2 not available.\n");
        return;
    }

    window = create_window();

    swapchain_desc.Width = 640;
    swapchain_desc.Height = 480;
    swapchain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.Stereo = FALSE;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 2;
    swapchain_desc.Scaling = DXGI_SCALING_STRETCH;
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapchain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapchain_desc.Flags = 0;

    hr = IDXGIFactory2_CreateSwapChainForHwnd(factory2, device,
            window, &swapchain_desc, NULL, NULL, &swapchain1);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGISwapChain1_QueryInterface(swapchain1, &IID_IDXGISwapChain3, (void**)&swapchain3);
    IDXGISwapChain1_Release(swapchain1);
    if (FAILED(hr))
    {
        win_skip("IDXGISwapChain3 not available.\n");
        IDXGIFactory2_Release(factory2);
        DestroyWindow(window);
        return;
    }

    for (i = 0; i < ARRAY_SIZE(colour_spaces); ++i)
    {
        support = 0xdeadbeef;
        hr = IDXGISwapChain3_CheckColorSpaceSupport(swapchain3, colour_spaces[i], &support);
        ok(hr == S_OK, "Got unexpected hr %#lx for test %u.\n", hr, i);
        ok(!(support & ~DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT),
                "Got unexpected support flags %#x for test %u.\n", support, i);

        if (colour_spaces[i] == DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709)
        {
            ok(support & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT,
                    "Required colour space not supported for test %u.\n", i);
        }
        else if (colour_spaces[i] == DXGI_COLOR_SPACE_RESERVED)
        {
            ok(!support, "Invalid colour space supported for test %u.\n", i);
        }

        hr = IDXGISwapChain3_SetColorSpace1(swapchain3, colour_spaces[i]);
        ok(hr == (support & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) ? S_OK : E_INVALIDARG,
                "Got unexpected hr %#lx for text %u.\n", hr, i);
    }

    ref_count = IDXGISwapChain3_Release(swapchain3);
    ok(!ref_count, "Swap chain has %lu references left.\n", ref_count);
    DestroyWindow(window);
    ref_count = IDXGIFactory2_Release(factory2);
    ok(ref_count == !is_d3d12, "Factory has %lu references left.\n", ref_count);
}

static void test_mode_change(IUnknown *device, BOOL is_d3d12)
{
    unsigned int user32_width = 0, user32_height = 0, d3d_width = 0, d3d_height = 0;
    unsigned int display_count = 0, mode_idx = 0, adapter_idx, output_idx;
    DEVMODEW *original_modes = NULL, old_devmode, devmode, devmode2;
    DXGI_SWAP_CHAIN_DESC swapchain_desc, swapchain_desc2;
    IDXGIOutput *output, *second_output = NULL;
    WCHAR second_monitor_name[CCHDEVICENAME];
    IDXGISwapChain *swapchain, *swapchain2;
    DXGI_OUTPUT_DESC output_desc;
    IDXGIAdapter *adapter;
    IDXGIFactory *factory;
    BOOL fullscreen, ret;
    LONG change_ret;
    ULONG refcount;
    HRESULT hr;

    memset(&devmode, 0, sizeof(devmode));
    devmode.dmSize = sizeof(devmode);
    ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode);
    ok(ret, "EnumDisplaySettingsW failed, error %#lx.\n", GetLastError());
    ok(equal_mode_rect(&devmode, &registry_mode), "Got a different mode.\n");
    ret = EnumDisplaySettingsW(NULL, ENUM_REGISTRY_SETTINGS, &devmode);
    ok(ret, "EnumDisplaySettingsW failed, error %#lx.\n", GetLastError());
    ok(equal_mode_rect(&devmode, &registry_mode), "Got a different mode.\n");

    while (EnumDisplaySettingsW(NULL, mode_idx++, &devmode))
    {
        if (devmode.dmPelsWidth == registry_mode.dmPelsWidth
                && devmode.dmPelsHeight == registry_mode.dmPelsHeight)
            continue;

        if (!d3d_width && !d3d_height)
        {
            d3d_width = devmode.dmPelsWidth;
            d3d_height = devmode.dmPelsHeight;
            continue;
        }

        if (devmode.dmPelsWidth == d3d_width && devmode.dmPelsHeight == d3d_height)
            continue;

        user32_width = devmode.dmPelsWidth;
        user32_height = devmode.dmPelsHeight;
        break;
    }
    if (!user32_width || !user32_height)
    {
        skip("Failed to find three different display modes for the primary output.\n");
        return;
    }

    ret = save_display_modes(&original_modes, &display_count);
    ok(ret, "Failed to save original display modes.\n");

    get_factory(device, is_d3d12, &factory);

    /* Test that no mode restorations if no mode changes actually happened */
    change_ret = ChangeDisplaySettingsW(&devmode, CDS_UPDATEREGISTRY | CDS_NORESET);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsW failed with %ld.\n", change_ret);

    swapchain_desc.BufferDesc.Width = registry_mode.dmPelsWidth;
    swapchain_desc.BufferDesc.Height = registry_mode.dmPelsHeight;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 1;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = is_d3d12 ? 2 : 1;
    swapchain_desc.OutputWindow = CreateWindowA("static", "dxgi_test", 0, 0, 0, 400, 200, 0, 0, 0, 0);
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = is_d3d12 ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);

    memset(&devmode2, 0, sizeof(devmode2));
    devmode2.dmSize = sizeof(devmode2);
    ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode2);
    ok(ret, "EnumDisplaySettingsW failed, error %#lx.\n", GetLastError());
    ok(equal_mode_rect(&devmode2, &registry_mode), "Got a different mode.\n");
    ret = restore_display_modes(original_modes, display_count);
    ok(ret, "Failed to restore display modes.\n");

    /* If current display settings are different than the display settings in registry before
     * calling SetFullscreenState() */
    change_ret = ChangeDisplaySettingsW(&devmode, CDS_UPDATEREGISTRY | CDS_NORESET);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsW failed with %ld.\n", change_ret);

    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
    ok(hr == DXGI_ERROR_UNSUPPORTED /* Win7 */
            || hr == S_OK /* Win8~Win10 1909 */
            || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE, /* Win10 2004 */
            "Got unexpected hr %#lx.\n", hr);

    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);
    ret = restore_display_modes(original_modes, display_count);
    ok(ret, "Failed to restore display modes.\n");

    /* Test that mode restorations use display settings in the registry with a fullscreen device */
    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
    if (FAILED(hr))
    {
        skip("SetFullscreenState failed, hr %#lx.\n", hr);
        refcount = IDXGISwapChain_Release(swapchain);
        ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);
        goto done;
    }

    change_ret = ChangeDisplaySettingsW(&devmode, CDS_UPDATEREGISTRY | CDS_NORESET);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsW failed with %ld.\n", change_ret);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode2);
    ok(ret, "EnumDisplaySettingsW failed, error %#lx.\n", GetLastError());
    ok(equal_mode_rect(&devmode2, &devmode), "Got a different mode.\n");
    ret = EnumDisplaySettingsW(NULL, ENUM_REGISTRY_SETTINGS, &devmode2);
    ok(ret, "EnumDisplaySettingsW failed, error %#lx.\n", GetLastError());
    ok(equal_mode_rect(&devmode2, &devmode), "Got a different mode.\n");
    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);
    ret = restore_display_modes(original_modes, display_count);
    ok(ret, "Failed to restore display modes.\n");

    for (adapter_idx = 0; SUCCEEDED(IDXGIFactory_EnumAdapters(factory, adapter_idx, &adapter)); ++adapter_idx)
    {
        for (output_idx = 0; SUCCEEDED(IDXGIAdapter_EnumOutputs(adapter, output_idx, &output)); ++output_idx)
        {
            hr = IDXGIOutput_GetDesc(output, &output_desc);
            ok(hr == S_OK, "Adapter %u output %u: Got unexpected hr %#lx.\n", adapter_idx, output_idx, hr);

            if ((adapter_idx || output_idx) && output_desc.AttachedToDesktop)
            {
                second_output = output;
                break;
            }

            IDXGIOutput_Release(output);
        }

        IDXGIAdapter_Release(adapter);
        if (second_output)
            break;
    }

    if (!second_output)
    {
        skip("Following tests require two monitors.\n");
        goto done;
    }
    lstrcpyW(second_monitor_name, output_desc.DeviceName);

    memset(&old_devmode, 0, sizeof(old_devmode));
    old_devmode.dmSize = sizeof(old_devmode);
    ret = EnumDisplaySettingsW(second_monitor_name, ENUM_CURRENT_SETTINGS, &old_devmode);
    ok(ret, "EnumDisplaySettingsW failed, error %#lx.\n", GetLastError());

    mode_idx = 0;
    d3d_width = 0;
    d3d_height = 0;
    user32_width = 0;
    user32_height = 0;
    while (EnumDisplaySettingsW(second_monitor_name, mode_idx++, &devmode))
    {
        if (devmode.dmPelsWidth == old_devmode.dmPelsWidth
                && devmode.dmPelsHeight == old_devmode.dmPelsHeight)
            continue;

        if (!d3d_width && !d3d_height)
        {
            d3d_width = devmode.dmPelsWidth;
            d3d_height = devmode.dmPelsHeight;
            continue;
        }

        if (devmode.dmPelsWidth == d3d_width && devmode.dmPelsHeight == d3d_height)
            continue;

        user32_width = devmode.dmPelsWidth;
        user32_height = devmode.dmPelsHeight;
        break;
    }
    if (!user32_width || !user32_height)
    {
        skip("Failed to find three different display modes for the second output.\n");
        goto done;
    }

    /* Test that mode restorations for non-primary outputs upon fullscreen state changes */
    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    change_ret = ChangeDisplaySettingsExW(second_monitor_name, &devmode, NULL, CDS_RESET, NULL);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExW failed with %ld.\n", change_ret);
    ret = EnumDisplaySettingsW(second_monitor_name, ENUM_CURRENT_SETTINGS, &devmode2);
    ok(ret, "EnumDisplaySettingsW failed, error %#lx.\n", GetLastError());
    if (devmode2.dmPelsWidth == old_devmode.dmPelsWidth
            && devmode2.dmPelsHeight == old_devmode.dmPelsHeight)
    {
        skip("Failed to change display settings of the second monitor.\n");
        hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        refcount = IDXGISwapChain_Release(swapchain);
        ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);
        goto done;
    }

    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ret = EnumDisplaySettingsW(second_monitor_name, ENUM_CURRENT_SETTINGS, &devmode2);
    ok(ret, "EnumDisplaySettingsW failed, error %#lx.\n", GetLastError());
    ok(equal_mode_rect(&devmode2, &old_devmode), "Got a different mode.\n");
    ret = EnumDisplaySettingsW(second_monitor_name, ENUM_REGISTRY_SETTINGS, &devmode2);
    ok(ret, "EnumDisplaySettingsW failed, error %#lx.\n", GetLastError());
    ok(equal_mode_rect(&devmode2, &old_devmode), "Got a different mode.\n");
    hr = IDXGIOutput_GetDesc(second_output, &output_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(output_desc.DesktopCoordinates.right - output_desc.DesktopCoordinates.left ==
            old_devmode.dmPelsWidth, "Expected width %lu, got %lu.\n", old_devmode.dmPelsWidth,
            output_desc.DesktopCoordinates.right - output_desc.DesktopCoordinates.left);
    ok(output_desc.DesktopCoordinates.bottom - output_desc.DesktopCoordinates.top ==
            old_devmode.dmPelsHeight, "Expected height %lu, got %lu.\n", old_devmode.dmPelsHeight,
            output_desc.DesktopCoordinates.bottom - output_desc.DesktopCoordinates.top);

    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);
    ret = restore_display_modes(original_modes, display_count);
    ok(ret, "Failed to restore display modes.\n");

    /* Test that mode restorations for non-primary outputs use display settings in the registry */
    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    change_ret = ChangeDisplaySettingsExW(second_monitor_name, &devmode, NULL,
            CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExW failed with %ld.\n", change_ret);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ret = EnumDisplaySettingsW(second_monitor_name, ENUM_CURRENT_SETTINGS, &devmode2);
    ok(ret, "EnumDisplaySettingsW failed, error %#lx.\n", GetLastError());
    ok(devmode2.dmPelsWidth == devmode.dmPelsWidth && devmode2.dmPelsHeight == devmode.dmPelsHeight,
            "Expected resolution %lux%lu, got %lux%lu.\n", devmode.dmPelsWidth, devmode.dmPelsHeight,
            devmode2.dmPelsWidth, devmode2.dmPelsHeight);
    ret = EnumDisplaySettingsW(second_monitor_name, ENUM_REGISTRY_SETTINGS, &devmode2);
    ok(ret, "EnumDisplaySettingsW failed, error %#lx.\n", GetLastError());
    ok(devmode2.dmPelsWidth == devmode.dmPelsWidth && devmode2.dmPelsHeight == devmode.dmPelsHeight,
            "Expected resolution %lux%lu, got %lux%lu.\n", devmode.dmPelsWidth, devmode.dmPelsHeight,
            devmode2.dmPelsWidth, devmode2.dmPelsHeight);
    hr = IDXGIOutput_GetDesc(second_output, &output_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(output_desc.DesktopCoordinates.right - output_desc.DesktopCoordinates.left ==
            devmode.dmPelsWidth, "Expected width %lu, got %lu.\n", devmode.dmPelsWidth,
            output_desc.DesktopCoordinates.right - output_desc.DesktopCoordinates.left);
    ok(output_desc.DesktopCoordinates.bottom - output_desc.DesktopCoordinates.top ==
            devmode.dmPelsHeight, "Expected height %lu, got %lu.\n", devmode.dmPelsHeight,
            output_desc.DesktopCoordinates.bottom - output_desc.DesktopCoordinates.top);

    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);
    ret = restore_display_modes(original_modes, display_count);
    ok(ret, "Failed to restore display modes.\n");

    /* Test that mode restorations for non-primary outputs on fullscreen state changes when there
     * are two fullscreen swapchains on different outputs */
    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    swapchain_desc2 = swapchain_desc;
    swapchain_desc.BufferDesc.Width = d3d_width;
    swapchain_desc.BufferDesc.Height = d3d_height;
    swapchain_desc2.OutputWindow = CreateWindowA("static", "dxgi_test2", 0,
            old_devmode.dmPosition.x, old_devmode.dmPosition.y, 400, 200, 0, 0, 0, 0);
    hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc2, &swapchain2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_SetFullscreenState(swapchain, TRUE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_SetFullscreenState(swapchain2, TRUE, NULL);
    if (FAILED(hr))
    {
        skip("SetFullscreenState failed, hr %#lx.\n", hr);
        refcount = IDXGISwapChain_Release(swapchain2);
        ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);
        hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        refcount = IDXGISwapChain_Release(swapchain);
        ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);
        goto done;
    }

    hr = IDXGISwapChain_SetFullscreenState(swapchain, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGISwapChain_GetFullscreenState(swapchain, &fullscreen, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!fullscreen, "Expected swapchain not fullscreen.\n");
    hr = IDXGISwapChain_GetFullscreenState(swapchain2, &fullscreen, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(fullscreen, "Expected swapchain fullscreen.\n");

    ret = EnumDisplaySettingsW(second_monitor_name, ENUM_CURRENT_SETTINGS, &devmode2);
    ok(ret, "EnumDisplaySettingsW failed, error %#lx.\n", GetLastError());
    ok(equal_mode_rect(&devmode2, &old_devmode), "Got a different mode.\n");
    ret = EnumDisplaySettingsW(second_monitor_name, ENUM_REGISTRY_SETTINGS, &devmode2);
    ok(ret, "EnumDisplaySettingsW failed, error %#lx.\n", GetLastError());
    ok(equal_mode_rect(&devmode2, &old_devmode), "Got a different mode.\n");
    hr = IDXGIOutput_GetDesc(second_output, &output_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(output_desc.DesktopCoordinates.right - output_desc.DesktopCoordinates.left ==
            old_devmode.dmPelsWidth, "Expected width %lu, got %lu.\n", old_devmode.dmPelsWidth,
            output_desc.DesktopCoordinates.right - output_desc.DesktopCoordinates.left);
    ok(output_desc.DesktopCoordinates.bottom - output_desc.DesktopCoordinates.top ==
            old_devmode.dmPelsHeight, "Expected height %lu, got %lu.\n", old_devmode.dmPelsHeight,
            output_desc.DesktopCoordinates.bottom - output_desc.DesktopCoordinates.top);

    hr = IDXGISwapChain_SetFullscreenState(swapchain2, FALSE, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    refcount = IDXGISwapChain_Release(swapchain2);
    ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);
    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "IDXGISwapChain has %lu references left.\n", refcount);
    DestroyWindow(swapchain_desc2.OutputWindow);
    ret = restore_display_modes(original_modes, display_count);
    ok(ret, "Failed to restore display modes.\n");

done:
    if (second_output)
        IDXGIOutput_Release(second_output);
    DestroyWindow(swapchain_desc.OutputWindow);
    refcount = IDXGIFactory_Release(factory);
    ok(refcount == !is_d3d12, "Got unexpected refcount %lu.\n", refcount);
    ret = restore_display_modes(original_modes, display_count);
    ok(ret, "Failed to restore display modes.\n");
    free(original_modes);
}

static void test_swapchain_present_count(IUnknown *device, BOOL is_d3d12)
{
    static const UINT test_flags[] =
    {
        0,
        DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT,
    };

    UINT present_count, expected;
    ID3D12Device *d3d12_device;
    ID3D12CommandQueue *queue;
    IDXGISwapChain *swapchain;
    ID3D12Fence *fence;
    unsigned int i;
    HWND window;
    HRESULT hr;

    window = create_window();

    for (i = 0; i < ARRAY_SIZE(test_flags); ++i)
    {
        UINT flags = test_flags[i];

        if (!is_d3d12 && (flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT))
            continue;

        swapchain = create_swapchain(device, is_d3d12, window, flags);

        present_count = ~0u;
        hr = IDXGISwapChain_GetLastPresentCount(swapchain, &present_count);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(!present_count, "Got unexpected present count %u.\n", present_count);

        hr = IDXGISwapChain_Present(swapchain, 0, 0);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        expected = present_count + 1;
        hr = IDXGISwapChain_GetLastPresentCount(swapchain, &present_count);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(present_count == expected, "Got unexpected present count %u, expected %u.\n", present_count, expected);

        hr = IDXGISwapChain_Present(swapchain, 10, 0);
        ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
        expected = present_count;
        hr = IDXGISwapChain_GetLastPresentCount(swapchain, &present_count);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(present_count == expected, "Got unexpected present count %u, expected %u.\n", present_count, expected);

        hr = IDXGISwapChain_Present(swapchain, 0, DXGI_PRESENT_TEST);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        expected = present_count;
        hr = IDXGISwapChain_GetLastPresentCount(swapchain, &present_count);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(present_count == expected, "Got unexpected present count %u, expected %u.\n", present_count, expected);

        ShowWindow(window, SW_MINIMIZE);
        hr = IDXGISwapChain_Present(swapchain, 0, 0);
        ok(hr == (is_d3d12 ? S_OK : DXGI_STATUS_OCCLUDED), "Got unexpected hr %#lx.\n", hr);
        expected = present_count + !!is_d3d12;
        hr = IDXGISwapChain_GetLastPresentCount(swapchain, &present_count);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(present_count == expected, "Got unexpected present count %u, expected %u.\n", present_count, expected);

        ShowWindow(window, SW_NORMAL);
        hr = IDXGISwapChain_Present(swapchain, 0, 0);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        expected = present_count + 1;
        hr = IDXGISwapChain_GetLastPresentCount(swapchain, &present_count);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(present_count == expected, "Got unexpected present count %u, expected %u.\n", present_count, expected);

        if (is_d3d12)
        {
            hr = IUnknown_QueryInterface(device, &IID_ID3D12CommandQueue, (void **)&queue);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            hr = ID3D12CommandQueue_GetDevice(queue, &IID_ID3D12Device, (void **)&d3d12_device);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            hr = ID3D12Device_CreateFence(d3d12_device, 0, 0, &IID_ID3D12Fence, (void **)&fence);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            hr = ID3D12CommandQueue_Wait(queue, fence, 1);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            /* The present count is updated when Present() is called,
             * not when frames are presented. */
            hr = IDXGISwapChain_Present(swapchain, 0, 0);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
            expected = present_count + 1;
            hr = IDXGISwapChain_GetLastPresentCount(swapchain, &present_count);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
            ok(present_count == expected, "Got unexpected present count %u, expected %u.\n", present_count, expected);

            hr = ID3D12Fence_Signal(fence, 1);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            expected = present_count;
            hr = IDXGISwapChain_GetLastPresentCount(swapchain, &present_count);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
            ok(present_count == expected, "Got unexpected present count %u, expected %u.\n", present_count, expected);

            ID3D12Fence_Release(fence);
            ID3D12Device_Release(d3d12_device);
            ID3D12CommandQueue_Release(queue);
        }

        IDXGISwapChain_Release(swapchain);
    }

    DestroyWindow(window);
}

static void test_video_memory_budget_notification(void)
{
    DXGI_QUERY_VIDEO_MEMORY_INFO memory_info;
    IDXGIAdapter3 *adapter3;
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    DWORD cookie, ret;
    ULONG refcount;
    HANDLE event;
    HRESULT hr;

    if (!(device = create_device(0)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGIAdapter_QueryInterface(adapter, &IID_IDXGIAdapter3, (void **)&adapter3);
    ok(hr == S_OK || hr == E_NOINTERFACE, "Got unexpected hr %#lx.\n", hr);
    if (hr == E_NOINTERFACE)
        goto done;

    hr = IDXGIAdapter3_RegisterVideoMemoryBudgetChangeNotificationEvent(adapter3, NULL, &cookie);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);

    event = CreateEventW(NULL, FALSE, FALSE, NULL);
    hr = IDXGIAdapter3_RegisterVideoMemoryBudgetChangeNotificationEvent(adapter3, event, NULL);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGIAdapter3_RegisterVideoMemoryBudgetChangeNotificationEvent(adapter3, event, &cookie);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGIAdapter3_QueryVideoMemoryInfo(adapter3, 0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memory_info);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    if (!memory_info.Budget)
    {
        hr = IDXGIAdapter3_QueryVideoMemoryInfo(adapter3, 0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &memory_info);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    }
    if (memory_info.Budget)
    {
        ret = WaitForSingleObject(event, 1000);
        ok(ret == WAIT_OBJECT_0, "Expected event fired.\n");
    }

    IDXGIAdapter3_UnregisterVideoMemoryBudgetChangeNotification(adapter3, cookie);
    IDXGIAdapter3_Release(adapter3);
    CloseHandle(event);

done:
    IDXGIAdapter_Release(adapter);
    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

static void test_zero_size(IUnknown *device, BOOL is_d3d12)
{
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    IDXGISwapChain *swapchain;
    unsigned int i, expected;
    IDXGIFactory *factory;
    HWND window;
    HRESULT hr;
    RECT r;

    static const struct
    {
        INT w, h;
    }
    tests[] =
    {
        {0, 0},
        {200, 0},
        {0, 200},
        {200, 200},
        /* FIXME: How to create a window with a client rect width or height above 0 but less than 8?
         * If I try to AdjustWindowRect a (100,100)-(104,104) rectangle I get a larger window. I
         * suspect the decoration style and DPI will play into it too. The size below creates a
         * 176x4 client rect on my system. */
        {100, 60}
    };

    get_factory(device, is_d3d12, &factory);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        window = CreateWindowA("static", "dxgi_test", WS_VISIBLE, 0, 0, tests[i].w, tests[i].h, 0, 0, 0, 0);
        swapchain_desc.BufferDesc.Width = 0;
        swapchain_desc.BufferDesc.Height = 0;
        swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
        swapchain_desc.BufferDesc.RefreshRate.Denominator = 60;
        swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        swapchain_desc.SampleDesc.Count = 1;
        swapchain_desc.SampleDesc.Quality = 0;
        swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchain_desc.BufferCount = is_d3d12 ? 2 : 1;
        swapchain_desc.OutputWindow = window;
        swapchain_desc.Windowed = TRUE;
        swapchain_desc.SwapEffect = is_d3d12 ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD;
        swapchain_desc.Flags = 0;

        hr = IDXGIFactory_CreateSwapChain(factory, device, &swapchain_desc, &swapchain);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(!swapchain_desc.BufferDesc.Width && !swapchain_desc.BufferDesc.Height, "Input desc was modified.\n");

        hr = IDXGISwapChain_GetDesc(swapchain, &swapchain_desc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        GetClientRect(swapchain_desc.OutputWindow, &r);
        expected = r.right > 0 ? r.right : 8;
        ok(swapchain_desc.BufferDesc.Width == expected, "Got width %u, expected %u, test %u.\n",
                swapchain_desc.BufferDesc.Width, expected, i);
        expected = r.bottom > 0 ? r.bottom : 8;
        ok(swapchain_desc.BufferDesc.Height == expected, "Got height %u, expected %u, test %u.\n",
                swapchain_desc.BufferDesc.Height, expected, i);

        IDXGISwapChain_Release(swapchain);
        DestroyWindow(window);
    }

    IDXGIFactory_Release(factory);
}

static void run_on_d3d10(void (*test_func)(IUnknown *device, BOOL is_d3d12))
{
    IDXGIDevice *device;
    ULONG refcount;

    if (!(device = create_device(0)))
    {
        skip("Failed to create Direct3D 10 device.\n");
        return;
    }

    test_func((IUnknown *)device, FALSE);

    refcount = IDXGIDevice_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

static void run_on_d3d12(void (*test_func)(IUnknown *device, BOOL is_d3d12))
{
    ID3D12CommandQueue *queue;
    ID3D12Device *device;
    ULONG refcount;

    if (!(device = create_d3d12_device()))
    {
        skip("Failed to create Direct3D 12 device.\n");
        return;
    }

    queue = create_d3d12_direct_queue(device);

    test_func((IUnknown *)queue, TRUE);

    wait_queue_idle(device, queue);

    refcount = ID3D12CommandQueue_Release(queue);
    ok(!refcount, "Command queue has %lu references left.\n", refcount);
    refcount = ID3D12Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

START_TEST(dxgi)
{
    HMODULE dxgi_module, d3d11_module, d3d12_module, gdi32_module;
    BOOL enable_debug_layer = FALSE;
    unsigned int argc, i;
    ID3D12Debug *debug;
    char **argv;

    dxgi_module = GetModuleHandleA("dxgi.dll");
    pCreateDXGIFactory1 = (void *)GetProcAddress(dxgi_module, "CreateDXGIFactory1");
    pCreateDXGIFactory2 = (void *)GetProcAddress(dxgi_module, "CreateDXGIFactory2");

    gdi32_module = GetModuleHandleA("gdi32.dll");
    pD3DKMTCheckVidPnExclusiveOwnership = (void *)GetProcAddress(gdi32_module, "D3DKMTCheckVidPnExclusiveOwnership");
    pD3DKMTCloseAdapter = (void *)GetProcAddress(gdi32_module, "D3DKMTCloseAdapter");
    pD3DKMTOpenAdapterFromGdiDisplayName = (void *)GetProcAddress(gdi32_module, "D3DKMTOpenAdapterFromGdiDisplayName");

    d3d11_module = LoadLibraryA("d3d11.dll");
    pD3D11CreateDevice = (void *)GetProcAddress(d3d11_module, "D3D11CreateDevice");

    registry_mode.dmSize = sizeof(registry_mode);
    ok(EnumDisplaySettingsW(NULL, ENUM_REGISTRY_SETTINGS, &registry_mode), "Failed to get display mode.\n");

    use_mt = !getenv("WINETEST_NO_MT_D3D");
    /* Some host drivers (MacOS, Mesa radeonsi) never unmap memory even when
     * requested. When using the chunk allocator, running the tests with more
     * than one thread can exceed the 32-bit virtual address space. */
    if (sizeof(void *) == 4 && !strcmp(winetest_platform, "wine"))
        use_mt = FALSE;

    argc = winetest_get_mainargs(&argv);
    for (i = 2; i < argc; ++i)
    {
        if (!strcmp(argv[i], "--validate"))
            enable_debug_layer = TRUE;
        else if (!strcmp(argv[i], "--warp"))
            use_warp_adapter = TRUE;
        else if (!strcmp(argv[i], "--adapter") && i + 1 < argc)
            use_adapter_idx = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--single"))
            use_mt = FALSE;
    }

    queue_test(test_adapter_desc);
    queue_test(test_adapter_luid);
    queue_test(test_query_video_memory_info);
    queue_test(test_check_interface_support);
    queue_test(test_create_surface);
    queue_test(test_parents);
    queue_test(test_output);
    queue_test(test_find_closest_matching_mode);
    queue_test(test_create_factory);
    queue_test(test_private_data);
    queue_test(test_maximum_frame_latency);
    queue_test(test_output_desc);
    queue_test(test_object_wrapping);
    queue_test(test_factory_check_feature_support);
    queue_test(test_video_memory_budget_notification);

    run_queued_tests();

    /* These tests use full-screen swapchains, so shouldn't run in parallel. */
    test_create_swapchain();
    test_inexact_modes();
    test_gamma_control();
    test_multi_adapter();
    test_swapchain_parameters();
    test_swapchain_window_styles();
    run_on_d3d10(test_set_fullscreen);
    run_on_d3d10(test_resize_target);
    run_on_d3d10(test_swapchain_resize);
    run_on_d3d10(test_swapchain_present);
    run_on_d3d10(test_swapchain_backbuffer_index);
    run_on_d3d10(test_swapchain_formats);
    run_on_d3d10(test_output_ownership);
    run_on_d3d10(test_cursor_clipping);
    run_on_d3d10(test_get_containing_output);
    run_on_d3d10(test_window_association);
    run_on_d3d10(test_default_fullscreen_target_output);
    run_on_d3d10(test_mode_change);
    run_on_d3d10(test_swapchain_present_count);
    run_on_d3d10(test_resize_target_wndproc);
    run_on_d3d10(test_swapchain_window_messages);
    run_on_d3d10(test_zero_size);

    if (!(d3d12_module = LoadLibraryA("d3d12.dll")))
    {
        skip("Direct3D 12 is not available.\n");
        return;
    }

    pD3D12CreateDevice = (void *)GetProcAddress(d3d12_module, "D3D12CreateDevice");
    pD3D12GetDebugInterface = (void *)GetProcAddress(d3d12_module, "D3D12GetDebugInterface");

    if (enable_debug_layer && SUCCEEDED(pD3D12GetDebugInterface(&IID_ID3D12Debug, (void **)&debug)))
    {
        ID3D12Debug_EnableDebugLayer(debug);
        ID3D12Debug_Release(debug);
    }

    run_on_d3d12(test_set_fullscreen);
    run_on_d3d12(test_resize_target);
    run_on_d3d12(test_swapchain_resize);
    run_on_d3d12(test_swapchain_present);
    run_on_d3d12(test_swapchain_backbuffer_index);
    run_on_d3d12(test_swapchain_formats);
    run_on_d3d12(test_output_ownership);
    run_on_d3d12(test_cursor_clipping);
    run_on_d3d12(test_frame_latency_event);
    run_on_d3d12(test_colour_space_support);
    run_on_d3d12(test_get_containing_output);
    run_on_d3d12(test_window_association);
    run_on_d3d12(test_default_fullscreen_target_output);
    run_on_d3d12(test_mode_change);
    run_on_d3d12(test_swapchain_present_count);
    run_on_d3d12(test_resize_target_wndproc);
    run_on_d3d12(test_swapchain_window_messages);
    run_on_d3d12(test_zero_size);

    FreeLibrary(d3d12_module);
}
