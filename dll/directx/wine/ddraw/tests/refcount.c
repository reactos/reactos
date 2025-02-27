/*
 * Some unit tests for ddraw reference counting
 *
 * Copyright (C) 2006 Stefan Dösinger
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

#define COBJMACROS

#include "wine/test.h"
#include "ddraw.h"
#include "d3d.h"
#include "unknwn.h"

static HRESULT (WINAPI *pDirectDrawCreateEx)(GUID *driver_guid,
        void **ddraw, REFIID interface_iid, IUnknown *outer);

static void init_function_pointers(void)
{
    HMODULE hmod = GetModuleHandleA("ddraw.dll");
    pDirectDrawCreateEx = (void*)GetProcAddress(hmod, "DirectDrawCreateEx");
}

static ULONG getRefcount(IUnknown *iface)
{
    IUnknown_AddRef(iface);
    return IUnknown_Release(iface);
}

static void test_ddraw_objects(void)
{
    HRESULT hr;
    ULONG ref;
    IDirectDraw7 *DDraw7;
    IDirectDraw4 *DDraw4;
    IDirectDraw2 *DDraw2;
    IDirectDraw  *DDraw1;
    IDirectDrawPalette *palette;
    IDirectDrawSurface7 *surface = NULL;
    IDirectDrawSurface *surface1;
    IDirectDrawSurface4 *surface4;
    PALETTEENTRY Table[256];
    DDSURFACEDESC2 ddsd;

    hr = pDirectDrawCreateEx(NULL, (void **) &DDraw7, &IID_IDirectDraw7, NULL);
    ok(hr == DD_OK || hr == DDERR_NODIRECTDRAWSUPPORT, "Got hr %#lx.\n", hr);
    if(!DDraw7)
    {
        trace("Couldn't create DDraw interface, skipping tests\n");
        return;
    }

    hr = IDirectDraw7_QueryInterface(DDraw7, &IID_IDirectDraw4, (void **) &DDraw4);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    hr = IDirectDraw7_QueryInterface(DDraw7, &IID_IDirectDraw2, (void **) &DDraw2);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    hr = IDirectDraw7_QueryInterface(DDraw7, &IID_IDirectDraw, (void **) &DDraw1);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    ref = getRefcount( (IUnknown *) DDraw7);
    ok(ref == 1, "Got refcount %ld, expected 1\n", ref);

    /* Fails without a cooplevel */
    hr = IDirectDraw7_CreatePalette(DDraw7, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, Table, &palette, NULL);
    ok(hr == DDERR_NOCOOPERATIVELEVELSET, "Got hr %#lx.\n", hr);

    /* This check is before the cooplevel check */
    hr = IDirectDraw7_CreatePalette(DDraw7, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, Table, &palette, (void *) 0xdeadbeef);
    ok(hr == CLASS_E_NOAGGREGATION, "Got hr %#lx.\n", hr);

    hr = IDirectDraw7_SetCooperativeLevel(DDraw7, 0, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsd.dwWidth = 64;
    ddsd.dwHeight = 64;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
    ddsd.ddpfPixelFormat.dwRGBBitCount = 8;

    hr = IDirectDraw7_CreateSurface(DDraw7, &ddsd, &surface, NULL);
    if (!surface)
    {
        win_skip("Could not create surface : %#lx\n", hr);
        IDirectDraw_Release(DDraw1);
        IDirectDraw2_Release(DDraw2);
        IDirectDraw4_Release(DDraw4);
        IDirectDraw7_Release(DDraw7);
        return;
    }
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    /* DDraw refcount increased by 1 */
    ref = getRefcount( (IUnknown *) DDraw7);
    ok(ref == 2, "Got refcount %ld, expected 2\n", ref);

    /* Surface refcount starts with 1 */
    ref = getRefcount( (IUnknown *) surface);
    ok(ref == 1, "Got refcount %ld, expected 1\n", ref);

    hr = IDirectDraw7_CreatePalette(DDraw7, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, Table, &palette, NULL);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    /* DDraw refcount increased by 1 */
    ref = getRefcount( (IUnknown *) DDraw7);
    ok(ref == 3, "Got refcount %ld, expected 3\n", ref);

    /* Palette starts with 1 */
    ref = getRefcount( (IUnknown *) palette);
    ok(ref == 1, "Got refcount %ld, expected 1\n", ref);

    /* Test attaching a palette to a surface */
    hr = IDirectDrawSurface7_SetPalette(surface, palette);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    /* Palette refcount increased, surface stays the same */
    ref = getRefcount( (IUnknown *) palette);
    ok(ref == 2, "Got refcount %ld, expected 2\n", ref);
    ref = getRefcount( (IUnknown *) surface);
    ok(ref == 1, "Got refcount %ld, expected 1\n", ref);

    IDirectDrawSurface7_Release(surface);
    /* Increased before - decrease now */
    ref = getRefcount( (IUnknown *) DDraw7);
    ok(ref == 2, "Got refcount %ld, expected 2\n", ref);

    /* Releasing the surface detaches the palette */
    ref = getRefcount( (IUnknown *) palette);
    ok(ref == 1, "Got refcount %ld, expected 1\n", ref);

    IDirectDrawPalette_Release(palette);

    /* Increased before - decrease now */
    ref = getRefcount( (IUnknown *) DDraw7);
    ok(ref == 1, "Got refcount %ld, expected 1\n", ref);

    /* Not all interfaces are AddRefed when a palette is created */
    hr = IDirectDraw4_CreatePalette(DDraw4, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, Table, &palette, NULL);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    ref = getRefcount( (IUnknown *) DDraw4);
    ok(ref == 2, "Got refcount %ld, expected 2\n", ref);
    IDirectDrawPalette_Release(palette);

    /* No addref here */
    hr = IDirectDraw2_CreatePalette(DDraw2, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, Table, &palette, NULL);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    ref = getRefcount( (IUnknown *) DDraw2);
    ok(ref == 1, "Got refcount %ld, expected 1\n", ref);
    IDirectDrawPalette_Release(palette);

    /* No addref here */
    hr = IDirectDraw_CreatePalette(DDraw1, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, Table, &palette, NULL);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    ref = getRefcount( (IUnknown *) DDraw1);
    ok(ref == 1, "Got refcount %ld, expected 1\n", ref);
    IDirectDrawPalette_Release(palette);

    /* Similar for surfaces */
    hr = IDirectDraw4_CreateSurface(DDraw4, &ddsd, &surface4, NULL);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    ref = getRefcount( (IUnknown *) DDraw4);
    ok(ref == 2, "Got refcount %ld, expected 2\n", ref);
    IDirectDrawSurface4_Release(surface4);

    ddsd.dwSize = sizeof(DDSURFACEDESC);
    hr = IDirectDraw2_CreateSurface(DDraw2, (DDSURFACEDESC *) &ddsd, &surface1, NULL);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    ref = getRefcount( (IUnknown *) DDraw2);
    ok(ref == 1, "Got refcount %ld, expected 1\n", ref);
    IDirectDrawSurface_Release(surface1);

    hr = IDirectDraw_CreateSurface(DDraw1, (DDSURFACEDESC *) &ddsd, &surface1, NULL);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    ref = getRefcount( (IUnknown *) DDraw1);
    ok(ref == 1, "Got refcount %ld, expected 1\n", ref);
    IDirectDrawSurface_Release(surface1);

    IDirectDraw7_Release(DDraw7);
    IDirectDraw4_Release(DDraw4);
    IDirectDraw2_Release(DDraw2);
    IDirectDraw_Release(DDraw1);
}

static void test_iface_refcnt(void)
{
    HRESULT hr;
    IDirectDraw  *DDraw1;
    IDirectDraw2 *DDraw2;
    IDirectDraw4 *DDraw4;
    IDirectDraw7 *DDraw7;
    IDirect3D7   *D3D7;
    IDirect3D3   *D3D3;
    IDirect3D2   *D3D2;
    IDirect3D    *D3D1;
    long ref;

    hr = pDirectDrawCreateEx(NULL, (void **) &DDraw7, &IID_IDirectDraw7, NULL);
    ok(hr == DD_OK || hr == DDERR_NODIRECTDRAWSUPPORT, "Got hr %#lx.\n", hr);
    if(!DDraw7)
    {
        trace("Couldn't create DDraw interface, skipping tests\n");
        return;
    }

    ref = getRefcount( (IUnknown *) DDraw7);
    ok(ref == 1, "Initial IDirectDraw7 reference count is %ld\n", ref);

    hr = IDirectDraw7_QueryInterface(DDraw7, &IID_IDirectDraw4, (void **) &DDraw4);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    hr = IDirectDraw7_QueryInterface(DDraw7, &IID_IDirectDraw2, (void **) &DDraw2);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    hr = IDirectDraw7_QueryInterface(DDraw7, &IID_IDirectDraw, (void **) &DDraw1);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    /* All interfaces now have refcount 1! */
    ref = getRefcount( (IUnknown *) DDraw7);
    ok(ref == 1, "IDirectDraw7 reference count is %ld\n", ref);
    ref = getRefcount( (IUnknown *) DDraw7);
    ok(ref == 1, "IDirectDraw7 reference count is %ld\n", ref);
    ref = getRefcount( (IUnknown *) DDraw4);
    ok(ref == 1, "IDirectDraw4 reference count is %ld\n", ref);
    ref = getRefcount( (IUnknown *) DDraw2);
    ok(ref == 1, "IDirectDraw2 reference count is %ld\n", ref);
    ref = getRefcount( (IUnknown *) DDraw1);
    ok(ref == 1, "IDirectDraw reference count is %ld\n", ref);

    hr = IDirectDraw7_QueryInterface(DDraw7, &IID_IDirect3D7, (void **) &D3D7);
    ok(hr == DD_OK || hr == E_NOINTERFACE, /* win64 */
       "IDirectDraw7_QueryInterface returned %#lx\n", hr);
    if (FAILED(hr))
    {
        IDirectDraw7_Release(DDraw7);
        IDirectDraw4_Release(DDraw4);
        IDirectDraw2_Release(DDraw2);
        IDirectDraw_Release(DDraw1);
        skip( "no IDirect3D7 support\n" );
        return;
    }

    /* Apparently IDirectDrawX and IDirect3DX are linked together */
    ref = getRefcount( (IUnknown *) D3D7);
    ok(ref == 2, "IDirect3D7 reference count is %ld\n", ref);
    ref = getRefcount( (IUnknown *) DDraw7);
    ok(ref == 2, "IDirectDraw7 reference count is %ld\n", ref);

    IDirectDraw7_AddRef(DDraw7);
    ref = getRefcount( (IUnknown *) D3D7);
    ok(ref == 3, "IDirect3D7 reference count is %ld\n", ref);
    ref = getRefcount( (IUnknown *) DDraw7);
    ok(ref == 3, "IDirectDraw7 reference count is %ld\n", ref);

    IDirect3D7_Release(D3D7);
    ref = getRefcount( (IUnknown *) D3D7);
    ok(ref == 2, "IDirect3D7 reference count is %ld\n", ref);
    ref = getRefcount( (IUnknown *) DDraw7);
    ok(ref == 2, "IDirectDraw7 reference count is %ld\n", ref);

    /* Can't get older d3d interfaces. WHY????? */
    hr = IDirectDraw7_QueryInterface(DDraw7, &IID_IDirect3D3, (void **) &D3D3);
    todo_wine ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    if(hr == DD_OK && D3D3) IDirect3D3_Release(D3D3);

    hr = IDirectDraw4_QueryInterface(DDraw4, &IID_IDirect3D3, (void **) &D3D3);
    todo_wine ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    if(hr == DD_OK && D3D3) IDirect3D3_Release(D3D3);

    hr = IDirectDraw7_QueryInterface(DDraw7, &IID_IDirect3D2, (void **) &D3D2);
    todo_wine ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    if(hr == DD_OK && D3D2) IDirect3D2_Release(D3D2);

    hr = IDirectDraw2_QueryInterface(DDraw2, &IID_IDirect3D2, (void **) &D3D2);
    todo_wine ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    if(hr == DD_OK && D3D2) IDirect3D2_Release(D3D2);

    hr = IDirectDraw7_QueryInterface(DDraw7, &IID_IDirect3D, (void **) &D3D1);
    todo_wine ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    if(hr == DD_OK && D3D1) IDirect3D_Release(D3D1);

    hr = IDirectDraw_QueryInterface(DDraw1, &IID_IDirect3D, (void **) &D3D1);
    todo_wine ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    if(hr == DD_OK && D3D1) IDirect3D_Release(D3D1);

    hr = IDirect3D7_QueryInterface(D3D7, &IID_IDirect3D, (void **) &D3D1);
    todo_wine ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    if(hr == DD_OK && D3D1) IDirect3D_Release(D3D1);

    /* Try an AddRef, it only affects the AddRefed interface */
    IDirectDraw4_AddRef(DDraw4);
    ref = getRefcount( (IUnknown *) DDraw7);
    ok(ref == 2, "IDirectDraw7 reference count is %ld\n", ref); /* <-- From the d3d query */
    ref = getRefcount( (IUnknown *) DDraw4);
    ok(ref == 2, "IDirectDraw4 reference count is %ld\n", ref); /* <-- The AddRef call */
    ref = getRefcount( (IUnknown *) DDraw2);
    ok(ref == 1, "IDirectDraw2 reference count is %ld\n", ref);
    ref = getRefcount( (IUnknown *) DDraw1);
    ok(ref == 1, "IDirectDraw reference count is %ld\n", ref);
    ref = getRefcount( (IUnknown *) D3D7);
    ok(ref == 2, "IDirect3D7 reference count is %ld\n", ref); /* <-- From the d3d query */
    IDirectDraw4_Release(DDraw4);

    /* Make sure that they are one object, not different ones */
    hr = IDirectDraw4_SetCooperativeLevel(DDraw4, GetDesktopWindow(), DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    /* After an window has been set, DDSCL_SETFOCUSWINDOW should return DDERR_HWNDALREADYSET, see the mode test */
    hr = IDirectDraw7_SetCooperativeLevel(DDraw7, NULL, DDSCL_SETFOCUSWINDOW);
    ok(hr == DDERR_HWNDALREADYSET, "Got hr %#lx.\n", hr);

    /* All done, release all interfaces */
    IDirectDraw7_Release(DDraw7);
    IDirectDraw4_Release(DDraw4);
    IDirectDraw2_Release(DDraw2);
    IDirectDraw_Release(DDraw1);
    IDirect3D7_Release(D3D7);
}

static void test_d3d_ifaces(void)
{
    IDirectDraw *DDraw1;
    IDirectDraw2 *DDraw2;
    IDirectDraw4 *DDraw4;
    IDirect3D *D3D1;
    IDirect3D2 *D3D2;
    IDirect3D3 *D3D3;
    IDirect3D7 *D3D7;
    HRESULT hr;
    long ref;

    hr = DirectDrawCreate(NULL, &DDraw1, NULL);
    ok(hr == DD_OK || hr == DDERR_NODIRECTDRAWSUPPORT, "Got hr %#lx.\n", hr);
    if(!DDraw1)
    {
        trace("DirectDrawCreate failed with %#lx\n", hr);
        return;
    }

    hr = IDirectDraw_QueryInterface(DDraw1, &IID_IDirectDraw2, (void **) &DDraw2);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    hr = IDirectDraw_QueryInterface(DDraw1, &IID_IDirectDraw4, (void **) &DDraw4);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    ref = getRefcount( (IUnknown *) DDraw4);
    ok(ref == 1, "IDirectDraw4 reference count is %ld\n", ref);
    ref = getRefcount( (IUnknown *) DDraw2);
    ok(ref == 1, "IDirectDraw2 reference count is %ld\n", ref);
    ref = getRefcount( (IUnknown *) DDraw1);
    ok(ref == 1, "IDirectDraw reference count is %ld\n", ref);

    /* FIXME: This test suggests that IDirect3D, IDirect3D2 and IDirect3D3 are linked
     * to IDirectDraw. However, they are linked to whatever interface is used to QI the
     * first IDirect3D? interface. If DDraw1 is replaced with DDraw4 here the tests break */
    hr = IDirectDraw_QueryInterface(DDraw1, &IID_IDirect3D, (void **) &D3D1);
    if (hr == E_NOINTERFACE)  /* win64 */
    {
        IDirectDraw4_Release(DDraw4);
        IDirectDraw2_Release(DDraw2);
        IDirectDraw_Release(DDraw1);
        skip( "no IDirect3D support\n" );
        return;
    }
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    ref = getRefcount( (IUnknown *) DDraw4);
    ok(ref == 1, "IDirectDraw4 reference count is %ld\n", ref);
    ref = getRefcount( (IUnknown *) DDraw2);
    ok(ref == 1, "IDirectDraw2 reference count is %ld\n", ref);
    ref = getRefcount( (IUnknown *) DDraw1);
    ok(ref == 2, "IDirectDraw reference count is %ld\n", ref);
    IDirect3D_Release(D3D1);

    hr = IDirectDraw2_QueryInterface(DDraw2, &IID_IDirect3D2, (void **) &D3D2);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    ref = getRefcount( (IUnknown *) DDraw4);
    ok(ref == 1, "IDirectDraw4 reference count is %ld\n", ref);
    ref = getRefcount( (IUnknown *) DDraw2);
    ok(ref == 1, "IDirectDraw2 reference count is %ld\n", ref);
    ref = getRefcount( (IUnknown *) DDraw1);
    ok(ref == 2, "IDirectDraw reference count is %ld\n", ref);
    IDirect3D2_Release(D3D2);

    hr = IDirectDraw4_QueryInterface(DDraw4, &IID_IDirect3D3, (void **) &D3D3);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    ref = getRefcount( (IUnknown *) DDraw4);
    ok(ref == 1, "IDirectDraw4 reference count is %ld\n", ref);
    ref = getRefcount( (IUnknown *) DDraw2);
    ok(ref == 1, "IDirectDraw2 reference count is %ld\n", ref);
    ref = getRefcount( (IUnknown *) DDraw1);
    ok(ref == 2, "IDirectDraw reference count is %ld\n", ref);
    IDirect3D3_Release(D3D3);

    /* Try to AddRef the D3D3 interface that has been released already */
    IDirect3D3_AddRef(D3D3);
    ref = getRefcount( (IUnknown *) DDraw1);
    ok(ref == 2, "IDirectDraw reference count is %ld\n", ref);
    ref = getRefcount( (IUnknown *) D3D3);
    ok(ref == 2, "IDirect3D3 reference count is %ld\n", ref);
    /* The newer interfaces remain untouched */
    ref = getRefcount( (IUnknown *) DDraw4);
    ok(ref == 1, "IDirectDraw4 reference count is %ld\n", ref);
    ref = getRefcount( (IUnknown *) DDraw2);
    ok(ref == 1, "IDirectDraw2 reference count is %ld\n", ref);
    IDirect3D3_Release(D3D3);
    ref = getRefcount( (IUnknown *) DDraw1);
    ok(ref == 1, "IDirectDraw reference count is %ld\n", ref);
    ref = getRefcount( (IUnknown *) DDraw1);
    ok(ref == 1, "IDirectDraw reference count is %ld\n", ref);

    /* It is possible to query any IDirect3D interfaces from any IDirectDraw interface,
     * Except IDirect3D7, it can only be returned by IDirectDraw7(which can't return older ifaces)
     */
    hr = IDirectDraw2_QueryInterface(DDraw2, &IID_IDirect3D, (void **) &D3D1);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    IDirect3D_Release(D3D1);
    hr = IDirectDraw4_QueryInterface(DDraw4, &IID_IDirect3D, (void **) &D3D1);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    IDirect3D_Release(D3D1);

    hr = IDirectDraw_QueryInterface(DDraw1, &IID_IDirect3D2, (void **) &D3D2);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    IDirect3D_Release(D3D2);
    hr = IDirectDraw4_QueryInterface(DDraw4, &IID_IDirect3D2, (void **) &D3D2);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    IDirect3D_Release(D3D2);

    hr = IDirectDraw_QueryInterface(DDraw1, &IID_IDirect3D3, (void **) &D3D3);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    IDirect3D_Release(D3D3);
    hr = IDirectDraw2_QueryInterface(DDraw2, &IID_IDirect3D3, (void **) &D3D3);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    IDirect3D_Release(D3D3);

    /* This does NOT work */
    hr = IDirectDraw_QueryInterface(DDraw1, &IID_IDirect3D7, (void **) &D3D7);
    todo_wine ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    if(D3D7) IDirect3D_Release(D3D7);
    hr = IDirectDraw2_QueryInterface(DDraw2, &IID_IDirect3D7, (void **) &D3D7);
    todo_wine ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    if(D3D7) IDirect3D_Release(D3D7);
    hr = IDirectDraw4_QueryInterface(DDraw4, &IID_IDirect3D7, (void **) &D3D7);
    todo_wine ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    if(D3D7) IDirect3D_Release(D3D7);

    /* Release the interfaces */
    IDirectDraw4_Release(DDraw4);
    IDirectDraw2_Release(DDraw2);
    IDirectDraw_Release(DDraw1);
}

START_TEST(refcount)
{
    init_function_pointers();
    if(!pDirectDrawCreateEx)
    {
        win_skip("function DirectDrawCreateEx not available\n");
        return;
    }
    test_ddraw_objects();
    test_iface_refcnt();
    test_d3d_ifaces();
}
