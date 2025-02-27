/*
 * Unit tests for ddraw functions
 *
 *
 * Part of this test involves changing the screen resolution. But this is
 * really disrupting if the user is doing something else and is not very nice
 * to CRT screens. Plus, ideally it needs someone watching it to check that
 * each mode displays correctly.
 * So this is only done if the test is being run in interactive mode.
 *
 * Copyright (C) 2003 Sami Aario
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
#include "wine/test.h"
#include "ddraw.h"

static IDirectDrawSurface *lpDDSPrimary;
static IDirectDrawSurface *lpDDSBack;
static IDirectDraw *lpDD;
static WNDCLASSA wc;
static HWND hwnd, hwnd2;
static int modes_cnt;
static int modes_size;
static DDSURFACEDESC *modes;
static RECT rect_before_create;
static RECT rect_after_delete;
static int modes16bpp_cnt;
static int refresh_rate;
static int refresh_rate_cnt;

static HRESULT (WINAPI *pDirectDrawEnumerateA)(LPDDENUMCALLBACKA cb, void *ctx);
static HRESULT (WINAPI *pDirectDrawEnumerateW)(LPDDENUMCALLBACKW cb, void *ctx);
static HRESULT (WINAPI *pDirectDrawEnumerateExA)(LPDDENUMCALLBACKEXA cb, void *ctx, DWORD flags);
static HRESULT (WINAPI *pDirectDrawEnumerateExW)(LPDDENUMCALLBACKEXW cb, void *ctx, DWORD flags);

static void init_function_pointers(void)
{
    HMODULE hmod = GetModuleHandleA("ddraw.dll");
    pDirectDrawEnumerateA = (void*)GetProcAddress(hmod, "DirectDrawEnumerateA");
    pDirectDrawEnumerateW = (void*)GetProcAddress(hmod, "DirectDrawEnumerateW");
    pDirectDrawEnumerateExA = (void*)GetProcAddress(hmod, "DirectDrawEnumerateExA");
    pDirectDrawEnumerateExW = (void*)GetProcAddress(hmod, "DirectDrawEnumerateExW");
}

static HWND createwindow(void)
{
    HWND hwnd;

    hwnd = CreateWindowExA(0, "TestWindowClass", "TestWindowClass",
        WS_POPUP, 0, 0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, GetModuleHandleA(0), NULL);

    ShowWindow(hwnd, SW_HIDE);
    UpdateWindow(hwnd);
    SetFocus(hwnd);

    return hwnd;
}

static BOOL createdirectdraw(void)
{
    HRESULT rc;

    SetRect(&rect_before_create, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));

    rc = DirectDrawCreate(NULL, &lpDD, NULL);
    ok(rc == DD_OK || rc==DDERR_NODIRECTDRAWSUPPORT, "Got hr %#lx.\n", rc);
    if (!lpDD) {
        trace("DirectDrawCreateEx() failed with an error %#lx\n", rc);
        return FALSE;
    }
    return TRUE;
}


static void releasedirectdraw(void)
{
    if( lpDD != NULL )
    {
        IDirectDraw_Release(lpDD);
        lpDD = NULL;
        SetRect(&rect_after_delete, 0, 0,
                GetSystemMetrics(SM_CXSCREEN),
                GetSystemMetrics(SM_CYSCREEN));
        ok(EqualRect(&rect_before_create, &rect_after_delete) != 0,
           "Original display mode was not restored\n");
    }
}

static BOOL WINAPI test_nullcontext_callbackA(GUID *lpGUID,
        char *lpDriverDescription, char *lpDriverName, void *lpContext)
{
    trace("test_nullcontext_callbackA: %p %s %s %p\n",
          lpGUID, lpDriverDescription, lpDriverName, lpContext);

    ok(!lpContext, "Expected NULL lpContext\n");

    return TRUE;
}

static BOOL WINAPI test_context_callbackA(GUID *lpGUID,
        char *lpDriverDescription, char *lpDriverName, void *lpContext)
{
    trace("test_context_callbackA: %p %s %s %p\n",
          lpGUID, lpDriverDescription, lpDriverName, lpContext);

    ok(lpContext == (void *)0xdeadbeef, "Expected non-NULL lpContext\n");

    return TRUE;
}

static void test_DirectDrawEnumerateA(void)
{
    HRESULT ret;

    if (!pDirectDrawEnumerateA)
    {
        win_skip("DirectDrawEnumerateA is not available\n");
        return;
    }

    /* Test with NULL callback parameter. */
    ret = pDirectDrawEnumerateA(NULL, NULL);
    ok(ret == DDERR_INVALIDPARAMS, "Got hr %#lx.\n", ret);

    /* Test with valid callback parameter and NULL context parameter. */
    trace("Calling DirectDrawEnumerateA with test_nullcontext_callbackA callback and NULL context.\n");
    ret = pDirectDrawEnumerateA(test_nullcontext_callbackA, NULL);
    ok(ret == DD_OK, "Got hr %#lx.\n", ret);

    /* Test with valid callback parameter and valid context parameter. */
    trace("Calling DirectDrawEnumerateA with test_context_callbackA callback and non-NULL context.\n");
    ret = pDirectDrawEnumerateA(test_context_callbackA, (void *)0xdeadbeef);
    ok(ret == DD_OK, "Got hr %#lx.\n", ret);
}

static BOOL WINAPI test_callbackW(GUID *lpGUID, WCHAR *lpDriverDescription,
        WCHAR *lpDriverName, void *lpContext)
{
    ok(0, "The callback should not be invoked by DirectDrawEnumerateW\n");
    return TRUE;
}

static void test_DirectDrawEnumerateW(void)
{
    HRESULT ret;

    if (!pDirectDrawEnumerateW)
    {
        win_skip("DirectDrawEnumerateW is not available\n");
        return;
    }

    /* DirectDrawEnumerateW is not implemented on Windows. */

    /* Test with NULL callback parameter. */
    ret = pDirectDrawEnumerateW(NULL, NULL);
    ok(ret == DDERR_INVALIDPARAMS ||
       ret == DDERR_UNSUPPORTED, /* Older ddraw */
       "Expected DDERR_INVALIDPARAMS or DDERR_UNSUPPORTED, got %#lx\n", ret);

    /* Test with invalid callback parameter. */
    ret = pDirectDrawEnumerateW((LPDDENUMCALLBACKW)0xdeadbeef, NULL);
    ok(ret == DDERR_INVALIDPARAMS /* XP */ ||
       ret == DDERR_UNSUPPORTED /* Win7 */,
       "Expected DDERR_INVALIDPARAMS or DDERR_UNSUPPORTED, got %#lx\n", ret);

    /* Test with valid callback parameter and NULL context parameter. */
    ret = pDirectDrawEnumerateW(test_callbackW, NULL);
    ok(ret == DDERR_UNSUPPORTED, "Expected DDERR_UNSUPPORTED, got %#lx\n", ret);
}

static BOOL WINAPI test_nullcontext_callbackExA(GUID *lpGUID, char *lpDriverDescription,
        char *lpDriverName, void *lpContext, HMONITOR hm)
{
    trace("test_nullcontext_callbackExA: %p %s %s %p %p\n", lpGUID,
          lpDriverDescription, lpDriverName, lpContext, hm);

    ok(!lpContext, "Expected NULL lpContext\n");

    return TRUE;
}

static BOOL WINAPI test_context_callbackExA(GUID *lpGUID, char *lpDriverDescription,
        char *lpDriverName, void *lpContext, HMONITOR hm)
{
    trace("test_context_callbackExA: %p %s %s %p %p\n", lpGUID,
          lpDriverDescription, lpDriverName, lpContext, hm);

    ok(lpContext == (void *)0xdeadbeef, "Expected non-NULL lpContext\n");

    return TRUE;
}

static BOOL WINAPI test_count_callbackExA(GUID *lpGUID, char *lpDriverDescription,
        char *lpDriverName, void *lpContext, HMONITOR hm)
{
    DWORD *count = (DWORD *)lpContext;

    trace("test_count_callbackExA: %p %s %s %p %p\n", lpGUID,
          lpDriverDescription, lpDriverName, lpContext, hm);

    (*count)++;

    return TRUE;
}

static void test_DirectDrawEnumerateExA(void)
{
    DWORD callbackCount;
    HRESULT ret;

    if (!pDirectDrawEnumerateExA)
    {
        win_skip("DirectDrawEnumerateExA is not available\n");
        return;
    }

    /* Test with NULL callback parameter. */
    ret = pDirectDrawEnumerateExA(NULL, NULL, 0);
    ok(ret == DDERR_INVALIDPARAMS, "Got hr %#lx.\n", ret);

    /* Test with valid callback parameter and invalid flags */
    ret = pDirectDrawEnumerateExA(test_nullcontext_callbackExA, NULL, ~0);
    ok(ret == DDERR_INVALIDPARAMS, "Got hr %#lx.\n", ret);

    /* Test with valid callback parameter and NULL context parameter. */
    trace("Calling DirectDrawEnumerateExA with empty flags and NULL context.\n");
    ret = pDirectDrawEnumerateExA(test_nullcontext_callbackExA, NULL, 0);
    ok(ret == DD_OK, "Got hr %#lx.\n", ret);

    /* Test with valid callback parameter and non-NULL context parameter. */
    trace("Calling DirectDrawEnumerateExA with empty flags and non-NULL context.\n");
    ret = pDirectDrawEnumerateExA(test_context_callbackExA, (void *)0xdeadbeef, 0);
    ok(ret == DD_OK, "Got hr %#lx.\n", ret);

    /* Test with valid callback parameter and count the number of primary devices */
    callbackCount = 0;
    ret = pDirectDrawEnumerateExA(test_count_callbackExA, &callbackCount, 0);
    ok(ret == DD_OK, "Got hr %#lx.\n", ret);
    ok(callbackCount == 1, "Expected 1 primary device, got %lu\n", callbackCount);

    /* Test with valid callback parameter and count the number of secondary devices */
    callbackCount = 0;
    ret = pDirectDrawEnumerateExA(test_count_callbackExA, &callbackCount,
                                  DDENUM_ATTACHEDSECONDARYDEVICES);
    ok(ret == DD_OK, "Got hr %#lx.\n", ret);
    /* Note: this list includes the primary devices as well and some systems (such as the TestBot)
       do not include any secondary devices */
    ok(callbackCount >= 1, "Expected at least one device, got %lu\n", callbackCount);

    /* Test with valid callback parameter, NULL context parameter, and all flags set. */
    trace("Calling DirectDrawEnumerateExA with all flags set and NULL context.\n");
    ret = pDirectDrawEnumerateExA(test_nullcontext_callbackExA, NULL,
                                  DDENUM_ATTACHEDSECONDARYDEVICES |
                                  DDENUM_DETACHEDSECONDARYDEVICES |
                                  DDENUM_NONDISPLAYDEVICES);
    ok(ret == DD_OK, "Got hr %#lx.\n", ret);
}

static BOOL WINAPI test_callbackExW(GUID *lpGUID, WCHAR *lpDriverDescription,
        WCHAR *lpDriverName, void *lpContext, HMONITOR hm)
{
    ok(0, "The callback should not be invoked by DirectDrawEnumerateExW.\n");
    return TRUE;
}

static void test_DirectDrawEnumerateExW(void)
{
    HRESULT ret;

    if (!pDirectDrawEnumerateExW)
    {
        win_skip("DirectDrawEnumerateExW is not available\n");
        return;
    }

    /* DirectDrawEnumerateExW is not implemented on Windows. */

    /* Test with NULL callback parameter. */
    ret = pDirectDrawEnumerateExW(NULL, NULL, 0);
    ok(ret == DDERR_UNSUPPORTED, "Got hr %#lx.\n", ret);

    /* Test with invalid callback parameter. */
    ret = pDirectDrawEnumerateExW((LPDDENUMCALLBACKEXW)0xdeadbeef, NULL, 0);
    ok(ret == DDERR_UNSUPPORTED, "Got hr %#lx.\n", ret);

    /* Test with valid callback parameter and invalid flags */
    ret = pDirectDrawEnumerateExW(test_callbackExW, NULL, ~0);
    ok(ret == DDERR_UNSUPPORTED, "Got hr %#lx.\n", ret);

    /* Test with valid callback parameter and NULL context parameter. */
    ret = pDirectDrawEnumerateExW(test_callbackExW, NULL, 0);
    ok(ret == DDERR_UNSUPPORTED, "Got hr %#lx.\n", ret);

    /* Test with valid callback parameter, NULL context parameter, and all flags set. */
    ret = pDirectDrawEnumerateExW(test_callbackExW, NULL,
                                  DDENUM_ATTACHEDSECONDARYDEVICES |
                                  DDENUM_DETACHEDSECONDARYDEVICES |
                                  DDENUM_NONDISPLAYDEVICES);
    ok(ret == DDERR_UNSUPPORTED, "Got hr %#lx.\n", ret);
}

static void adddisplaymode(DDSURFACEDESC *lpddsd)
{
    if (!modes)
        modes = malloc((modes_size = 2) * sizeof(DDSURFACEDESC));
    if (modes_cnt == modes_size)
        modes = realloc(modes, (modes_size *= 2) * sizeof(DDSURFACEDESC));
    assert(modes);
    modes[modes_cnt++] = *lpddsd;
}

static void flushdisplaymodes(void)
{
    free(modes);
    modes = 0;
    modes_cnt = modes_size = 0;
}

static HRESULT WINAPI enummodescallback(DDSURFACEDESC *lpddsd, void *lpContext)
{
    if (winetest_debug > 1)
        trace("Width = %li, Height = %li, bpp = %li, Refresh Rate = %li, Pitch = %li, flags = %#lx\n",
              lpddsd->dwWidth, lpddsd->dwHeight, lpddsd->ddpfPixelFormat.dwRGBBitCount,
              lpddsd->dwRefreshRate, lpddsd->lPitch, lpddsd->dwFlags);

    /* Check that the pitch is valid if applicable */
    if(lpddsd->dwFlags & DDSD_PITCH)
    {
        ok(lpddsd->lPitch != 0, "EnumDisplayModes callback with bad pitch\n");
    }

    /* Check that frequency is valid if applicable
     *
     * This fails on some Windows drivers or Windows versions, so it isn't important
     * apparently
    if(lpddsd->dwFlags & DDSD_REFRESHRATE)
    {
        ok(lpddsd->dwRefreshRate != 0, "EnumDisplayModes callback with bad refresh rate\n");
    }
     */

    adddisplaymode(lpddsd);
    if(lpddsd->ddpfPixelFormat.dwRGBBitCount == 16)
        modes16bpp_cnt++;

    return DDENUMRET_OK;
}

static HRESULT WINAPI enummodescallback_16bit(DDSURFACEDESC *lpddsd, void *lpContext)
{
    if (winetest_debug > 1)
        trace("Width = %li, Height = %li, bpp = %li, Refresh Rate = %li, Pitch = %li, flags = %#lx\n",
              lpddsd->dwWidth, lpddsd->dwHeight, lpddsd->ddpfPixelFormat.dwRGBBitCount,
              lpddsd->dwRefreshRate, lpddsd->lPitch, lpddsd->dwFlags);

    ok(lpddsd->dwFlags == (DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT|DDSD_PITCH|DDSD_REFRESHRATE),
            "Wrong surface description flags %#lx\n", lpddsd->dwFlags);
    ok(lpddsd->ddpfPixelFormat.dwFlags == DDPF_RGB, "Wrong pixel format flag %#lx\n",
            lpddsd->ddpfPixelFormat.dwFlags);
    ok(lpddsd->ddpfPixelFormat.dwRGBBitCount == 16, "Expected 16 bpp got %li\n",
            lpddsd->ddpfPixelFormat.dwRGBBitCount);

    /* Check that the pitch is valid if applicable */
    if(lpddsd->dwFlags & DDSD_PITCH)
    {
        ok(lpddsd->lPitch != 0, "EnumDisplayModes callback with bad pitch\n");
    }

    if(!refresh_rate)
    {
        if(lpddsd->dwRefreshRate )
        {
            refresh_rate = lpddsd->dwRefreshRate;
            refresh_rate_cnt++;
        }
    }
    else
    {
        if(refresh_rate == lpddsd->dwRefreshRate)
            refresh_rate_cnt++;
    }

    modes16bpp_cnt++;

    return DDENUMRET_OK;
}

static HRESULT WINAPI enummodescallback_count(DDSURFACEDESC *lpddsd, void *lpContext)
{
    ok(lpddsd->dwFlags == (DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT|DDSD_PITCH|DDSD_REFRESHRATE),
            "Wrong surface description flags %#lx\n", lpddsd->dwFlags);

    modes16bpp_cnt++;

    return DDENUMRET_OK;
}

static void enumdisplaymodes(void)
{
    DDSURFACEDESC ddsd;
    HRESULT rc;
    int count, refresh_count;

    ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    /* Flags parameter is reserved in very old ddraw versions (3 and older?) and must be 0 */
    rc = IDirectDraw_EnumDisplayModes(lpDD, 0, &ddsd, 0, enummodescallback);
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);

    count = modes16bpp_cnt;

    modes16bpp_cnt = 0;
    ddsd.dwFlags = DDSD_PIXELFORMAT;
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
    ddsd.ddpfPixelFormat.dwRGBBitCount = 16;
    ddsd.ddpfPixelFormat.dwRBitMask = 0xf800;
    ddsd.ddpfPixelFormat.dwGBitMask = 0x07e0;
    ddsd.ddpfPixelFormat.dwBBitMask = 0x001F;

    rc = IDirectDraw_EnumDisplayModes(lpDD, 0, &ddsd, 0, enummodescallback_16bit);
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);
    ok(modes16bpp_cnt == count, "Expected %d modes got %d\n", count, modes16bpp_cnt);

    modes16bpp_cnt = 0;
    ddsd.ddpfPixelFormat.dwRBitMask = 0x0000;
    ddsd.ddpfPixelFormat.dwGBitMask = 0x0000;
    ddsd.ddpfPixelFormat.dwBBitMask = 0x0000;

    rc = IDirectDraw_EnumDisplayModes(lpDD, 0, &ddsd, 0, enummodescallback_16bit);
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);
    ok(modes16bpp_cnt == count, "Expected %d modes got %d\n", count, modes16bpp_cnt);

    modes16bpp_cnt = 0;
    ddsd.ddpfPixelFormat.dwRBitMask = 0xF0F0;
    ddsd.ddpfPixelFormat.dwGBitMask = 0x0F00;
    ddsd.ddpfPixelFormat.dwBBitMask = 0x000F;

    rc = IDirectDraw_EnumDisplayModes(lpDD, 0, &ddsd, 0, enummodescallback_16bit);
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);
    ok(modes16bpp_cnt == count, "Expected %d modes got %d\n", count, modes16bpp_cnt);


    modes16bpp_cnt = 0;
    ddsd.ddpfPixelFormat.dwFlags = DDPF_YUV;

    rc = IDirectDraw_EnumDisplayModes(lpDD, 0, &ddsd, 0, enummodescallback_16bit);
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);
    ok(modes16bpp_cnt == count, "Expected %d modes got %d\n", count, modes16bpp_cnt);

    modes16bpp_cnt = 0;
    ddsd.ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED8;

    rc = IDirectDraw_EnumDisplayModes(lpDD, 0, &ddsd, 0, enummodescallback_16bit);
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);
    ok(modes16bpp_cnt == count, "Expected %d modes got %d\n", count, modes16bpp_cnt);

    modes16bpp_cnt = 0;
    ddsd.dwFlags = DDSD_PIXELFORMAT;
    ddsd.ddpfPixelFormat.dwFlags = 0;

    rc = IDirectDraw_EnumDisplayModes(lpDD, 0, &ddsd, 0, enummodescallback_16bit);
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);
    ok(modes16bpp_cnt == count, "Expected %d modes got %d\n", count, modes16bpp_cnt);

    modes16bpp_cnt = 0;
    ddsd.dwFlags = 0;

    rc = IDirectDraw_EnumDisplayModes(lpDD, 0, &ddsd, 0, enummodescallback_count);
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);
    ok(modes16bpp_cnt == modes_cnt, "Expected %d modes got %d\n", modes_cnt, modes16bpp_cnt);

    modes16bpp_cnt = 0;
    ddsd.dwFlags = DDSD_PIXELFORMAT | DDSD_PITCH;
    ddsd.lPitch = 123;

    rc = IDirectDraw_EnumDisplayModes(lpDD, 0, &ddsd, 0, enummodescallback_16bit);
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);
    ok(modes16bpp_cnt == count, "Expected %d modes got %d\n", count, modes16bpp_cnt);

    modes16bpp_cnt = 0;
    ddsd.dwFlags = DDSD_PIXELFORMAT | DDSD_REFRESHRATE;
    /* Ask for a refresh rate that could not possibly be used. But note that
     * the Windows 'Standard VGA' driver claims to run the display at 1Hz!
     */
    ddsd.dwRefreshRate = 2;

    rc = IDirectDraw_EnumDisplayModes(lpDD, 0, &ddsd, 0, enummodescallback_16bit);
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);
    ok(modes16bpp_cnt == 0, "Expected 0 modes got %d\n", modes16bpp_cnt);

    modes16bpp_cnt = 0;
    ddsd.dwFlags = DDSD_PIXELFORMAT;

    rc = IDirectDraw_EnumDisplayModes(lpDD, DDEDM_REFRESHRATES, &ddsd, 0, enummodescallback_16bit);
    if(rc == DDERR_INVALIDPARAMS)
    {
        skip("Ddraw version too old. Skipping.\n");
        return;
    }
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);
    refresh_count = refresh_rate_cnt;

    if(refresh_rate)
    {
        modes16bpp_cnt = 0;
        ddsd.dwFlags = DDSD_PIXELFORMAT | DDSD_REFRESHRATE;
        ddsd.dwRefreshRate = refresh_rate;

        rc = IDirectDraw_EnumDisplayModes(lpDD, 0, &ddsd, 0, enummodescallback_16bit);
        ok(rc == DD_OK, "Got hr %#lx.\n", rc);
        ok(modes16bpp_cnt == refresh_count, "Expected %d modes got %d\n", refresh_count, modes16bpp_cnt);
    }

    rc = IDirectDraw_EnumDisplayModes(lpDD, 0, NULL, 0, enummodescallback);
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);
}


static void setdisplaymode(int i)
{
    HRESULT rc;
    RECT orig_rect;

    SetRect(&orig_rect, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_ALLOWMODEX | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);
    if (modes[i].dwFlags & DDSD_PIXELFORMAT)
    {
        if (modes[i].ddpfPixelFormat.dwFlags & DDPF_RGB)
        {
            rc = IDirectDraw_SetDisplayMode(lpDD,
                modes[i].dwWidth, modes[i].dwHeight,
                modes[i].ddpfPixelFormat.dwRGBBitCount);
            ok(rc == DD_OK || rc == DDERR_UNSUPPORTED, "Got hr %#lx.\n", rc);
            if (rc == DD_OK)
            {
                RECT scrn, test, virt;

                SetRect(&virt, 0, 0, GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN));
                OffsetRect(&virt, GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN));
                SetRect(&scrn, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
                trace("Mode (%ldx%ld) [%ldx%ld] (%ld %ld)-(%ld %ld)\n", modes[i].dwWidth, modes[i].dwHeight,
                      scrn.right, scrn.bottom, virt.left, virt.top, virt.right, virt.bottom);
                if (!EqualRect(&scrn, &orig_rect))
                {
                    BOOL rect_result, ret;

                    /* Check that the client rect was resized */
                    ret = GetClientRect(hwnd, &test);
                    ok(ret, "GetClientRect returned %d\n", ret);
                    ret = EqualRect(&scrn, &test);
                    todo_wine ok(ret, "Fullscreen window has wrong size\n");

                    /* Check that switching to normal cooperative level
                       does not restore the display mode */
                    rc = IDirectDraw_SetCooperativeLevel(lpDD, hwnd, DDSCL_NORMAL);
                    ok(rc == DD_OK, "Got hr %#lx.\n", rc);
                    SetRect(&test, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
                    rect_result = EqualRect(&scrn, &test);
                    ok(rect_result!=0, "Setting cooperative level to DDSCL_NORMAL changed the display mode\n");

                    /* Go back to fullscreen */
                    rc = IDirectDraw_SetCooperativeLevel(lpDD,
                        hwnd, DDSCL_ALLOWMODEX | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
                    ok(rc == DD_OK, "Got hr %#lx.\n", rc);

                    /* If the display mode was changed, set the correct mode
                       to avoid irrelevant failures */
                    if (rect_result == 0)
                    {
                        rc = IDirectDraw_SetDisplayMode(lpDD,
                            modes[i].dwWidth, modes[i].dwHeight,
                            modes[i].ddpfPixelFormat.dwRGBBitCount);
                        ok(rc == DD_OK, "Got hr %#lx.\n", rc);
                    }
                }
                rc = IDirectDraw_RestoreDisplayMode(lpDD);
                ok(rc == DD_OK, "Got hr %#lx.\n", rc);
            }
        }
    }
}

static void createsurface(void)
{
    DDSURFACEDESC ddsd;
    DDSCAPS ddscaps;
    HRESULT rc;

    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE |
        DDSCAPS_FLIP |
        DDSCAPS_COMPLEX;
    ddsd.dwBackBufferCount = 1;
    rc = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpDDSPrimary, NULL );
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);
    ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
    rc = IDirectDrawSurface_GetAttachedSurface(lpDDSPrimary, &ddscaps, &lpDDSBack);
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);
}

static void destroysurface(void)
{
    if( lpDDSPrimary != NULL )
    {
        IDirectDrawSurface_Release(lpDDSPrimary);
        lpDDSPrimary = NULL;
    }
}

static void testsurface(void)
{
    const char* testMsg = "ddraw device context test";
    HDC hdc;
    HRESULT rc;

    rc = IDirectDrawSurface_GetDC(lpDDSBack, &hdc);
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);
    SetBkColor(hdc, RGB(0, 0, 255));
    SetTextColor(hdc, RGB(255, 255, 0));
    TextOutA(hdc, 0, 0, testMsg, strlen(testMsg));
    rc = IDirectDrawSurface_ReleaseDC(lpDDSBack, hdc);
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);

    while (1)
    {
        rc = IDirectDrawSurface_Flip(lpDDSPrimary, NULL, DDFLIP_WAIT);
        ok(rc == DD_OK || rc==DDERR_SURFACELOST, "Got hr %#lx.\n", rc);

        if (rc == DD_OK)
        {
            break;
        }
        else if (rc == DDERR_SURFACELOST)
        {
            rc = IDirectDrawSurface_Restore(lpDDSPrimary);
            ok(rc == DD_OK, "Got hr %#lx.\n", rc);
        }
    }
}

static void testdisplaymodes(void)
{
    int i;

    for (i = 0; i < modes_cnt; ++i)
    {
        setdisplaymode(i);
        createsurface();
        testsurface();
        destroysurface();
    }
}

static void testcooperativelevels_normal(void)
{
    BOOL sfw;
    HRESULT rc;
    DDSURFACEDESC surfacedesc;
    IDirectDrawSurface *surface = (IDirectDrawSurface *) 0xdeadbeef;

    memset(&surfacedesc, 0, sizeof(surfacedesc));
    surfacedesc.dwSize = sizeof(surfacedesc);
    surfacedesc.ddpfPixelFormat.dwSize = sizeof(surfacedesc.ddpfPixelFormat);
    surfacedesc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    surfacedesc.dwBackBufferCount = 1;
    surfacedesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;

    rc = IDirectDraw_SetCooperativeLevel(lpDD, hwnd, DDSCL_SETFOCUSWINDOW | DDSCL_CREATEDEVICEWINDOW);
    ok(rc == DDERR_INVALIDPARAMS, "Got hr %#lx.\n", rc);

    /* Do some tests with DDSCL_NORMAL mode */

    /* Fullscreen mode + normal mode + exclusive mode */

    rc = IDirectDraw_SetCooperativeLevel(lpDD, NULL, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE | DDSCL_NORMAL);
    ok(rc == DDERR_INVALIDPARAMS, "Got hr %#lx.\n", rc);

    sfw=FALSE;
    if(hwnd2)
        sfw=SetForegroundWindow(hwnd2);
    else
        skip("Failed to create the second window\n");

    rc = IDirectDraw_SetCooperativeLevel(lpDD, hwnd, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE | DDSCL_NORMAL);
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);

    if(sfw)
        ok(GetForegroundWindow()==hwnd,"Expected the main windows (%p) for foreground, received the second one (%p)\n",hwnd, hwnd2);

    /* Try creating a double buffered primary in fullscreen + exclusive + normal mode */
    rc = IDirectDraw_CreateSurface(lpDD, &surfacedesc, &surface, NULL);

    if (rc == DDERR_UNSUPPORTEDMODE)
        skip("Unsupported mode\n");
    else
    {
        ok(rc == DD_OK, "Got hr %#lx.\n", rc);
        ok(surface!=NULL, "Returned NULL surface pointer\n");
    }
    if(surface && surface != (IDirectDrawSurface *)0xdeadbeef) IDirectDrawSurface_Release(surface);

    /* Exclusive mode + normal mode */
    rc = IDirectDraw_SetCooperativeLevel(lpDD, hwnd, DDSCL_EXCLUSIVE | DDSCL_NORMAL);
    ok(rc == DDERR_INVALIDPARAMS, "Got hr %#lx.\n", rc);

    /* Fullscreen mode + normal mode */

    sfw=FALSE;
    if(hwnd2) sfw=SetForegroundWindow(hwnd2);

    rc = IDirectDraw_SetCooperativeLevel(lpDD, hwnd, DDSCL_FULLSCREEN | DDSCL_NORMAL);
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);

    if(sfw)
        ok(GetForegroundWindow()==hwnd2,"Expected the second windows (%p) for foreground, received the main one (%p)\n",hwnd2, hwnd);

    rc = IDirectDraw_SetCooperativeLevel(lpDD, NULL, DDSCL_FULLSCREEN | DDSCL_NORMAL);
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);

    /* Try creating a double buffered primary in fullscreen + normal mode */
    rc = IDirectDraw_CreateSurface(lpDD, &surfacedesc, &surface, NULL);
    if (rc == DDERR_UNSUPPORTEDMODE)
        skip("Unsupported mode\n");
    else
    {
        ok(rc == DDERR_NOEXCLUSIVEMODE, "Got hr %#lx.\n", rc);
        ok(surface == NULL, "Returned surface pointer is %p\n", surface);
    }

    if(surface && surface != (IDirectDrawSurface *)0xdeadbeef) IDirectDrawSurface_Release(surface);

    /* switching from Fullscreen mode to Normal mode */

    sfw=FALSE;
    if(hwnd2) sfw=SetForegroundWindow(hwnd2);

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_NORMAL);
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);

    if(sfw)
        ok(GetForegroundWindow()==hwnd2,"Expected the second windows (%p) for foreground, received the main one (%p)\n",hwnd2, hwnd);

    rc = IDirectDraw_SetCooperativeLevel(lpDD, NULL, DDSCL_NORMAL);
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);

    /* Try creating a double buffered primary in normal mode */
    rc = IDirectDraw_CreateSurface(lpDD, &surfacedesc, &surface, NULL);
    if (rc == DDERR_UNSUPPORTEDMODE)
        skip("Unsupported mode\n");
    else
    {
        ok(rc == DDERR_NOEXCLUSIVEMODE, "Got hr %#lx.\n", rc);
        ok(surface == NULL, "Returned surface pointer is %p\n", surface);
    }
    if(surface && surface != (IDirectDrawSurface *)0xdeadbeef) IDirectDrawSurface_Release(surface);

    /* switching from Normal mode to Fullscreen + Normal mode */

    sfw=FALSE;
    if(hwnd2) sfw=SetForegroundWindow(hwnd2);

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_NORMAL | DDSCL_FULLSCREEN);
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);

    if(sfw)
        ok(GetForegroundWindow()==hwnd2,"Expected the second windows (%p) for foreground, received the main one (%p)\n",hwnd2, hwnd);

    rc = IDirectDraw_SetCooperativeLevel(lpDD, NULL, DDSCL_NORMAL | DDSCL_FULLSCREEN);
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);

    /* Set the focus window */

    rc = IDirectDraw_SetCooperativeLevel(lpDD, hwnd, DDSCL_SETFOCUSWINDOW | DDSCL_CREATEDEVICEWINDOW);
    ok(rc == DDERR_INVALIDPARAMS, "Got hr %#lx.\n", rc);

    rc = IDirectDraw_SetCooperativeLevel(lpDD, NULL, DDSCL_SETFOCUSWINDOW);

    if (rc == DDERR_INVALIDPARAMS)
    {
        win_skip("NT4/Win95 do not support cooperative levels DDSCL_SETDEVICEWINDOW and DDSCL_SETFOCUSWINDOW\n");
        return;
    }

    ok(rc == DD_OK, "Got hr %#lx.\n", rc);

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_SETFOCUSWINDOW);

    if (rc == DDERR_INVALIDPARAMS)
    {
        win_skip("NT4/Win95 do not support cooperative levels DDSCL_SETDEVICEWINDOW and DDSCL_SETFOCUSWINDOW\n");
        return;
    }

    ok(rc == DD_OK, "Got hr %#lx.\n", rc);

    /* Set the focus window a second time*/
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_SETFOCUSWINDOW);
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);

    /* Test DDSCL_SETFOCUSWINDOW with the other flags. They should all fail, except of DDSCL_NOWINDOWCHANGES */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_NORMAL | DDSCL_SETFOCUSWINDOW);
    ok(rc == DDERR_INVALIDPARAMS, "Got hr %#lx.\n", rc);

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_SETFOCUSWINDOW);
    ok(rc == DDERR_INVALIDPARAMS, "Got hr %#lx.\n", rc);

    /* This one succeeds */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_NOWINDOWCHANGES | DDSCL_SETFOCUSWINDOW);
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_MULTITHREADED | DDSCL_SETFOCUSWINDOW);
    ok(rc == DDERR_INVALIDPARAMS, "Got hr %#lx.\n", rc);

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_FPUSETUP | DDSCL_SETFOCUSWINDOW);
    ok(rc == DDERR_INVALIDPARAMS, "Got hr %#lx.\n", rc);

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_FPUPRESERVE | DDSCL_SETFOCUSWINDOW);
    ok(rc == DDERR_INVALIDPARAMS, "Got hr %#lx.\n", rc);

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_ALLOWREBOOT | DDSCL_SETFOCUSWINDOW);
    ok(rc == DDERR_INVALIDPARAMS, "Got hr %#lx.\n", rc);

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_ALLOWMODEX | DDSCL_SETFOCUSWINDOW);
    ok(rc == DDERR_INVALIDPARAMS, "Got hr %#lx.\n", rc);

    /* Set the device window without any other flags. Should give an error */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_SETDEVICEWINDOW);
    ok(rc == DDERR_INVALIDPARAMS, "Got hr %#lx.\n", rc);

    /* Set device window with DDSCL_NORMAL */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_NORMAL | DDSCL_SETDEVICEWINDOW);
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);

    /* Also set the focus window. Should give an error */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_ALLOWMODEX | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_SETDEVICEWINDOW | DDSCL_SETFOCUSWINDOW);
    ok(rc == DDERR_INVALIDPARAMS, "Got hr %#lx.\n", rc);

    /* All done */
}

static void testcooperativelevels_exclusive(void)
{
    BOOL sfw, success;
    HRESULT rc;
    RECT window_rect;

    /* Do some tests with DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN mode */

    /* First, resize the window so it is not the same size as any screen */
    success = SetWindowPos(hwnd, 0, 0, 0, 281, 92, 0);
    ok(success, "SetWindowPos failed\n");

    /* Try to set exclusive mode only */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_EXCLUSIVE);
    ok(rc == DDERR_INVALIDPARAMS, "Got hr %#lx.\n", rc);

    /* Full screen mode only */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_FULLSCREEN);
    ok(rc == DDERR_INVALIDPARAMS, "Got hr %#lx.\n", rc);

    /* Full screen mode + exclusive mode */

    rc = IDirectDraw_SetCooperativeLevel(lpDD, NULL, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE);
    ok(rc == DDERR_INVALIDPARAMS, "Got hr %#lx.\n", rc);

    sfw=FALSE;
    if(hwnd2)
        sfw=SetForegroundWindow(hwnd2);
    else
        skip("Failed to create the second window\n");

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE);
    ok(rc == DD_OK, "Got hr %#lx.\n", rc);

    if(sfw)
        ok(GetForegroundWindow()==hwnd,"Expected the main windows (%p) for foreground, received the second one (%p)\n",hwnd, hwnd2);

    /* rect_before_create is assumed to hold the screen rect */
    GetClientRect(hwnd, &window_rect);
    success = EqualRect(&rect_before_create, &window_rect);
    ok(success, "Fullscreen window has wrong size.\n");

    /* Set the focus window. Should fail */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_SETFOCUSWINDOW);
    ok(rc == DDERR_HWNDALREADYSET ||
       broken(rc==DDERR_INVALIDPARAMS) /* NT4/Win95 */, "Got hr %#lx.\n", rc);


    /* All done */
}

static void testddraw3(void)
{
    const GUID My_IID_IDirectDraw3 = {
        0x618f8ad4,
        0x8b7a,
        0x11d0,
        { 0x8f,0xcc,0x0,0xc0,0x4f,0xd9,0x18,0x9d }
    };
    IDirectDraw3 *dd3;
    HRESULT hr;
    hr = IDirectDraw_QueryInterface(lpDD, &My_IID_IDirectDraw3, (void **) &dd3);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    if(SUCCEEDED(hr) && dd3) IDirectDraw3_Release(dd3);
}

static void testddraw7(void)
{
    IDirectDraw7 *dd7;
    HRESULT hr;
    DDDEVICEIDENTIFIER2 *pdddi2;
    DWORD dddi2Bytes;
    DWORD *pend;

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw7, (void **) &dd7);
    if (hr==E_NOINTERFACE)
    {
        win_skip("DirectDraw7 is not supported\n");
        return;
    }
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    if (hr==DD_OK)
    {
         dddi2Bytes = FIELD_OFFSET(DDDEVICEIDENTIFIER2, dwWHQLLevel) + sizeof(DWORD);

         pdddi2 = malloc(dddi2Bytes + 2 * sizeof(DWORD));
         pend = (DWORD *)((char *)pdddi2 + dddi2Bytes);
         pend[0] = 0xdeadbeef;
         pend[1] = 0xdeadbeef;

         hr = IDirectDraw7_GetDeviceIdentifier(dd7, pdddi2, 0);
         ok(hr == DD_OK, "Got hr %#lx.\n", hr);

         if (hr==DD_OK)
         {
             /* szDriver contains the name of the driver DLL */
             ok(strstr(pdddi2->szDriver, ".dll")!=NULL, "szDriver does not contain DLL name\n");
             /* check how strings are copied into the structure */
             ok(pdddi2->szDriver[MAX_DDDEVICEID_STRING - 1]==0, "szDriver not cleared\n");
             ok(pdddi2->szDescription[MAX_DDDEVICEID_STRING - 1]==0, "szDescription not cleared\n");
             /* verify that 8 byte structure size alignment will not overwrite memory */
             ok(pend[0]==0xdeadbeef || broken(pend[0] != 0xdeadbeef), /* win2k */
                "memory beyond DDDEVICEIDENTIFIER2 overwritten\n");
             ok(pend[1]==0xdeadbeef, "memory beyond DDDEVICEIDENTIFIER2 overwritten\n");
         }

         /* recheck with the DDGDI_GETHOSTIDENTIFIER flag */
         pend[0] = 0xdeadbeef;
         pend[1] = 0xdeadbeef;
         hr = IDirectDraw7_GetDeviceIdentifier(dd7, pdddi2, DDGDI_GETHOSTIDENTIFIER);
         ok(hr == DD_OK, "Got hr %#lx.\n", hr);
         if (hr==DD_OK)
         {
             /* szDriver contains the name of the driver DLL */
             ok(strstr(pdddi2->szDriver, ".dll")!=NULL, "szDriver does not contain DLL name\n");
             /* check how strings are copied into the structure */
             ok(pdddi2->szDriver[MAX_DDDEVICEID_STRING - 1]==0, "szDriver not cleared\n");
             ok(pdddi2->szDescription[MAX_DDDEVICEID_STRING - 1]==0, "szDescription not cleared\n");
             /* verify that 8 byte structure size alignment will not overwrite memory */
             ok(pend[0]==0xdeadbeef || broken(pend[0] != 0xdeadbeef), /* win2k */
                "memory beyond DDDEVICEIDENTIFIER2 overwritten\n");
             ok(pend[1]==0xdeadbeef, "memory beyond DDDEVICEIDENTIFIER2 overwritten\n");
         }

         IDirectDraw_Release(dd7);
         free(pdddi2);
    }
}

START_TEST(ddrawmodes)
{
    init_function_pointers();

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = GetModuleHandleA(0);
    wc.hbrBackground = GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "TestWindowClass";
    if (!RegisterClassA(&wc))
    {
        skip("RegisterClassA failed\n");
        return;
    }

    hwnd2=createwindow();
    hwnd=createwindow();

    if (!hwnd)
    {
        skip("Failed to create the main window\n");
        return;
    }

    if (!createdirectdraw())
    {
        skip("Failed to create the direct draw object\n");
        return;
    }

    test_DirectDrawEnumerateA();
    test_DirectDrawEnumerateW();
    test_DirectDrawEnumerateExA();
    test_DirectDrawEnumerateExW();

    enumdisplaymodes();
    if (winetest_interactive)
        testdisplaymodes();
    flushdisplaymodes();
    testddraw3();
    testddraw7();
    releasedirectdraw();

    createdirectdraw();
    testcooperativelevels_normal();
    releasedirectdraw();

    createdirectdraw();
    testcooperativelevels_exclusive();
    releasedirectdraw();
}
