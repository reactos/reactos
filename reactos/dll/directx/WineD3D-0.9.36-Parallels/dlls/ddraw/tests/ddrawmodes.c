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

static LPDIRECTDRAW lpDD = NULL;
static LPDIRECTDRAWSURFACE lpDDSPrimary = NULL;
static LPDIRECTDRAWSURFACE lpDDSBack = NULL;
static WNDCLASS wc;
static HWND hwnd;
static int modes_cnt;
static int modes_size;
static LPDDSURFACEDESC modes;

static void createwindow(void)
{
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DefWindowProcA;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandleA(0);
    wc.hIcon = LoadIconA(wc.hInstance, IDI_APPLICATION);
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "TestWindowClass";
    if(!RegisterClassA(&wc))
        assert(0);
    
    hwnd = CreateWindowExA(0, "TestWindowClass", "TestWindowClass",
        WS_POPUP, 0, 0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, GetModuleHandleA(0), NULL);
    assert(hwnd != NULL);
    
    ShowWindow(hwnd, SW_HIDE);
    UpdateWindow(hwnd);
    SetFocus(hwnd);
    
}

static BOOL createdirectdraw(void)
{
    HRESULT rc;

    rc = DirectDrawCreate(NULL, &lpDD, NULL);
    ok(rc==DD_OK || rc==DDERR_NODIRECTDRAWSUPPORT, "DirectDrawCreateEx returned: %x\n", rc);
    if (!lpDD) {
        trace("DirectDrawCreateEx() failed with an error %x\n", rc);
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
	}
}

static void adddisplaymode(LPDDSURFACEDESC lpddsd)
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

static HRESULT WINAPI enummodescallback(LPDDSURFACEDESC lpddsd, LPVOID lpContext)
{
    trace("Width = %i, Height = %i, Refresh Rate = %i\r\n",
        lpddsd->dwWidth, lpddsd->dwHeight,
        U2(*lpddsd).dwRefreshRate);
    adddisplaymode(lpddsd);

    return DDENUMRET_OK;
}

static void enumdisplaymodes(void)
{
    DDSURFACEDESC ddsd;
    HRESULT rc;

    ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    rc = IDirectDraw_EnumDisplayModes(lpDD,
        DDEDM_STANDARDVGAMODES, &ddsd, 0, enummodescallback);
    ok(rc==DD_OK || rc==E_INVALIDARG,"EnumDisplayModes returned: %x\n",rc);
}

static void setdisplaymode(int i)
{
    HRESULT rc;

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_ALLOWMODEX | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(rc==DD_OK,"SetCooperativeLevel returned: %x\n",rc);
    if (modes[i].dwFlags & DDSD_PIXELFORMAT)
    {
        if (modes[i].ddpfPixelFormat.dwFlags & DDPF_RGB)
        {
            rc = IDirectDraw_SetDisplayMode(lpDD,
                modes[i].dwWidth, modes[i].dwHeight,
                U1(modes[i].ddpfPixelFormat).dwRGBBitCount);
            ok(DD_OK==rc || DDERR_UNSUPPORTED==rc,"SetDisplayMode returned: %x\n",rc);
            if (rc == DD_OK)
            {
                RECT r, scrn, virt;

                SetRect(&virt, 0, 0, GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN));
                OffsetRect(&virt, GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN));
                SetRect(&scrn, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
                trace("Mode (%dx%d) [%dx%d] (%d %d)x(%d %d)\n", modes[i].dwWidth, modes[i].dwHeight,
                      scrn.right, scrn.bottom, virt.left, virt.top, virt.right, virt.bottom);

                ok(GetClipCursor(&r), "GetClipCursor() failed\n");
                /* ddraw sets clip rect here to the screen size, even for
                   multiple monitors */
                ok(EqualRect(&r, &scrn), "Invalid clip rect: (%d %d) x (%d %d)\n",
                   r.left, r.top, r.right, r.bottom);

                ok(ClipCursor(NULL), "ClipCursor() failed\n");
                ok(GetClipCursor(&r), "GetClipCursor() failed\n");
                ok(EqualRect(&r, &virt), "Invalid clip rect: (%d %d) x (%d %d)\n",
                   r.left, r.top, r.right, r.bottom);

                rc = IDirectDraw_RestoreDisplayMode(lpDD);
                ok(DD_OK==rc,"RestoreDisplayMode returned: %x\n",rc);
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
    ok(rc==DD_OK,"CreateSurface returned: %x\n",rc);
    ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
    rc = IDirectDrawSurface_GetAttachedSurface(lpDDSPrimary, &ddscaps, &lpDDSBack);
    ok(rc==DD_OK,"GetAttachedSurface returned: %x\n",rc);
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
    ok(rc==DD_OK, "IDirectDrawSurface_GetDC returned: %x\n",rc);
    SetBkColor(hdc, RGB(0, 0, 255));
    SetTextColor(hdc, RGB(255, 255, 0));
    TextOut(hdc, 0, 0, testMsg, lstrlen(testMsg));
    IDirectDrawSurface_ReleaseDC(lpDDSBack, hdc);
    ok(rc==DD_OK, "IDirectDrawSurface_ReleaseDC returned: %x\n",rc);
    
    while (1)
    {
        rc = IDirectDrawSurface_Flip(lpDDSPrimary, NULL, DDFLIP_WAIT);
        ok(rc==DD_OK || rc==DDERR_SURFACELOST, "IDirectDrawSurface_BltFast returned: %x\n",rc);

        if (rc == DD_OK)
        {
            break;
        }
        else if (rc == DDERR_SURFACELOST)
        {
            rc = IDirectDrawSurface_Restore(lpDDSPrimary);
            ok(rc==DD_OK, "IDirectDrawSurface_Restore returned: %x\n",rc);
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
    HRESULT rc;
    DDSURFACEDESC surfacedesc;
    IDirectDrawSurface *surface = (IDirectDrawSurface *) 0xdeadbeef;

    memset(&surfacedesc, 0, sizeof(surfacedesc));
    surfacedesc.dwSize = sizeof(surfacedesc);
    surfacedesc.ddpfPixelFormat.dwSize = sizeof(surfacedesc.ddpfPixelFormat);
    surfacedesc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    surfacedesc.dwBackBufferCount = 1;
    surfacedesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;

    /* Do some tests with DDSCL_NORMAL mode */

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_NORMAL);
    ok(rc==DD_OK,"SetCooperativeLevel(DDSCL_NORMAL) returned: %x\n",rc);

    /* Try creating a double buffered primary in normal mode */
    rc = IDirectDraw_CreateSurface(lpDD, &surfacedesc, &surface, NULL);
    ok(rc == DDERR_NOEXCLUSIVEMODE, "IDirectDraw_CreateSurface returned %08x\n", rc);
    ok(surface == NULL, "Returned surface pointer is %p\n", surface);
    if(surface) IDirectDrawSurface_Release(surface);

    /* Set the focus window */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_SETFOCUSWINDOW);
    ok(rc==DD_OK,"SetCooperativeLevel(DDSCL_SETFOCUSWINDOW) returned: %x\n",rc);

    /* Set the focus window a second time*/
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_SETFOCUSWINDOW);
    ok(rc==DD_OK,"SetCooperativeLevel(DDSCL_SETFOCUSWINDOW) the second time returned: %x\n",rc);

    /* Test DDSCL_SETFOCUSWINDOW with the other flags. They should all fail, except of DDSCL_NOWINDOWCHANGES */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_NORMAL | DDSCL_SETFOCUSWINDOW);
    ok(rc==DDERR_INVALIDPARAMS,"SetCooperativeLevel(DDSCL_NORMAL | DDSCL_SETFOCUSWINDOW) returned: %x\n",rc);

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_SETFOCUSWINDOW);
    ok(rc==DDERR_INVALIDPARAMS,"SetCooperativeLevel(DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_SETFOCUSWINDOW) returned: %x\n",rc);

    /* This one succeeds */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_NOWINDOWCHANGES | DDSCL_SETFOCUSWINDOW);
    ok(rc==DD_OK,"SetCooperativeLevel(DDSCL_NOWINDOWCHANGES | DDSCL_SETFOCUSWINDOW) returned: %x\n",rc);

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_MULTITHREADED | DDSCL_SETFOCUSWINDOW);
    ok(rc==DDERR_INVALIDPARAMS,"SetCooperativeLevel(DDSCL_MULTITHREADED | DDSCL_SETFOCUSWINDOW) returned: %x\n",rc);

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_FPUSETUP | DDSCL_SETFOCUSWINDOW);
    ok(rc==DDERR_INVALIDPARAMS,"SetCooperativeLevel(DDSCL_FPUSETUP | DDSCL_SETFOCUSWINDOW) returned: %x\n",rc);

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_FPUPRESERVE | DDSCL_SETFOCUSWINDOW);
    ok(rc==DDERR_INVALIDPARAMS,"SetCooperativeLevel(DDSCL_FPUPRESERVE | DDSCL_SETFOCUSWINDOW) returned: %x\n",rc);

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_ALLOWREBOOT | DDSCL_SETFOCUSWINDOW);
    ok(rc==DDERR_INVALIDPARAMS,"SetCooperativeLevel(DDSCL_ALLOWREBOOT | DDSCL_SETFOCUSWINDOW) returned: %x\n",rc);

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_ALLOWMODEX | DDSCL_SETFOCUSWINDOW);
    ok(rc==DDERR_INVALIDPARAMS,"SetCooperativeLevel(DDSCL_ALLOWMODEX | DDSCL_SETFOCUSWINDOW) returned: %x\n",rc);

    /* Set the device window without any other flags. Should give an error */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_SETDEVICEWINDOW);
    ok(rc==DDERR_INVALIDPARAMS,"SetCooperativeLevel(DDSCL_SETDEVICEWINDOW) returned: %x\n",rc);

    /* Set device window with DDSCL_NORMAL */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_NORMAL | DDSCL_SETDEVICEWINDOW);
    ok(rc==DD_OK,"SetCooperativeLevel(DDSCL_NORMAL | DDSCL_SETDEVICEWINDOW) returned: %x\n",rc);
 
    /* Also set the focus window. Should give an error */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_ALLOWMODEX | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_SETDEVICEWINDOW | DDSCL_SETFOCUSWINDOW);
    ok(rc==DDERR_INVALIDPARAMS,"SetCooperativeLevel(DDSCL_NORMAL | DDSCL_SETDEVICEWINDOW | DDSCL_SETFOCUSWINDOW) returned: %x\n",rc);

    /* All done */
}

static void testcooperativelevels_exclusive(void)
{
    HRESULT rc;

    /* Do some tests with DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN mode */

    /* Try to set exclusive mode only */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_EXCLUSIVE);
    ok(rc==DDERR_INVALIDPARAMS,"SetCooperativeLevel(DDSCL_EXCLUSIVE) returned: %x\n",rc);

    /* Full screen mode only */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_FULLSCREEN);
    ok(rc==DDERR_INVALIDPARAMS,"SetCooperativeLevel(DDSCL_FULLSCREEN) returned: %x\n",rc);

    /* Full screen mode + exclusive mode */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE);
    ok(rc==DD_OK,"SetCooperativeLevel(DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN) returned: %x\n",rc);

    /* Set the focus window. Should fail */
    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_SETFOCUSWINDOW);
    ok(rc==DDERR_HWNDALREADYSET,"SetCooperativeLevel(DDSCL_SETFOCUSWINDOW) returned: %x\n",rc);


    /* All done */
}

START_TEST(ddrawmodes)
{
    createwindow();
    if (!createdirectdraw())
        return;
    enumdisplaymodes();
    if (winetest_interactive)
        testdisplaymodes();
    flushdisplaymodes();
    releasedirectdraw();

    createdirectdraw();
    testcooperativelevels_normal();
    releasedirectdraw();

    createdirectdraw();
    testcooperativelevels_exclusive();
    releasedirectdraw();
}
