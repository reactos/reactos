/*
 * Unit tests for ddrawex surfaces
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
/* For IID_IDirectDraw3 - it is not in dxguid.dll */
#define INITGUID

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "ddraw.h"
#include "ddrawex.h"
#include "unknwn.h"

static IDirectDrawFactory *factory;
static HRESULT (WINAPI *pDllGetClassObject)(REFCLSID rclsid, REFIID riid, void **out);

static IDirectDraw *createDDraw(void)
{
    HRESULT hr;
    IDirectDraw *dd;

    hr = IDirectDrawFactory_CreateDirectDraw(factory, NULL, NULL, DDSCL_NORMAL, 0, 0, &dd);
    ok(hr == DD_OK, "Unexpected hr %#lx.\n", hr);
    return SUCCEEDED(hr) ? dd : NULL;
}

static void dctest_surf(IDirectDrawSurface *surf, int ddsdver) {
    HRESULT hr;
    HDC dc, dc2 = (HDC) 0x1234;
    DDSURFACEDESC ddsd;
    DDSURFACEDESC2 ddsd2;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);

    hr = IDirectDrawSurface_GetDC(surf, &dc);
    ok(hr == DD_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectDrawSurface_GetDC(surf, &dc);
    ok(hr == DDERR_DCALREADYCREATED, "Unexpected hr %#lx.\n", hr);
    ok(dc2 == (HDC) 0x1234, "The failed GetDC call changed the dc: %p\n", dc2);

    hr = IDirectDrawSurface_Lock(surf, NULL, ddsdver == 1 ? &ddsd : ((DDSURFACEDESC *) &ddsd2), 0, NULL);
    ok(hr == DDERR_SURFACEBUSY, "Unexpected hr %#lx.\n", hr);

    hr = IDirectDrawSurface_ReleaseDC(surf, dc);
    ok(hr == DD_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDirectDrawSurface_ReleaseDC(surf, dc);
    ok(hr == DDERR_NODC, "Unexpected hr %#lx.\n", hr);
}

static void GetDCTest_main(DDSURFACEDESC *ddsd, DDSURFACEDESC2 *ddsd2, void (*testfunc)(IDirectDrawSurface *surf, int ddsdver))
{
    IDirectDrawSurface *surf;
    IDirectDrawSurface2 *surf2;
    IDirectDrawSurface2 *surf3;
    IDirectDrawSurface4 *surf4;
    HRESULT hr;
    IDirectDraw  *dd1 = createDDraw();
    IDirectDraw2 *dd2;
    IDirectDraw3 *dd3;
    IDirectDraw4 *dd4;

    hr = IDirectDraw_CreateSurface(dd1, ddsd, &surf, NULL);
    if (hr == DDERR_UNSUPPORTEDMODE) {
        win_skip("Unsupported mode\n");
        return;
    }
    ok(hr == DD_OK, "Unexpected hr %#lx.\n", hr);
    testfunc(surf, 1);
    IDirectDrawSurface_Release(surf);

    hr = IDirectDraw_QueryInterface(dd1, &IID_IDirectDraw2, (void **) &dd2);
    ok(hr == DD_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectDraw2_CreateSurface(dd2, ddsd, &surf, NULL);
    ok(hr == DD_OK, "Unexpected hr %#lx.\n", hr);
    testfunc(surf, 1);

    hr = IDirectDrawSurface_QueryInterface(surf, &IID_IDirectDrawSurface2, (void **) &surf2);
    ok(hr == DD_OK, "Unexpected hr %#lx.\n", hr);
    testfunc((IDirectDrawSurface *) surf2, 1);

    IDirectDrawSurface2_Release(surf2);
    IDirectDrawSurface_Release(surf);
    IDirectDraw2_Release(dd2);

    hr = IDirectDraw_QueryInterface(dd1, &IID_IDirectDraw3, (void **) &dd3);
    ok(hr == DD_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectDraw3_CreateSurface(dd3, ddsd, &surf, NULL);
    ok(hr == DD_OK, "Unexpected hr %#lx.\n", hr);
    testfunc(surf, 1);

    hr = IDirectDrawSurface_QueryInterface(surf, &IID_IDirectDrawSurface3, (void **) &surf3);
    ok(hr == DD_OK, "Unexpected hr %#lx.\n", hr);
    testfunc((IDirectDrawSurface *) surf3, 1);

    IDirectDrawSurface3_Release(surf3);
    IDirectDrawSurface_Release(surf);
    IDirectDraw3_Release(dd3);

    hr = IDirectDraw_QueryInterface(dd1, &IID_IDirectDraw4, (void **) &dd4);
    ok(hr == DD_OK, "Unexpected hr %#lx.\n", hr);

    surf = NULL;
    hr = IDirectDraw4_CreateSurface(dd4, ddsd2, &surf4, NULL);
    ok(hr == DD_OK, "Unexpected hr %#lx.\n", hr);
    testfunc((IDirectDrawSurface *) surf4, 2);

    IDirectDrawSurface4_Release(surf4);
    IDirectDraw4_Release(dd4);

    IDirectDraw_Release(dd1);
}

static void GetDCTest(void)
{
    DDSURFACEDESC ddsd;
    DDSURFACEDESC2 ddsd2;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.dwWidth = 64;
    ddsd.dwHeight = 64;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);
    ddsd2.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd2.dwWidth = 64;
    ddsd2.dwHeight = 64;
    ddsd2.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

    GetDCTest_main(&ddsd, &ddsd2, dctest_surf);
}

static void CapsTest(void)
{
    DDSURFACEDESC ddsd;
    IDirectDraw  *dd1 = createDDraw();
    IDirectDrawSurface *surf;
    HRESULT hr;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_VIDEOMEMORY;
    ddsd.dwWidth = 64;
    ddsd.dwHeight = 64;
    hr = IDirectDraw_CreateSurface(dd1, &ddsd, &surf, NULL);
    if (hr == DDERR_UNSUPPORTEDMODE) {
        win_skip("Unsupported mode\n");
        return;
    }
    ok(hr == DD_OK, "Unexpected hr %#lx.\n", hr);
    if(surf) IDirectDrawSurface_Release(surf);

    IDirectDraw_Release(dd1);
}

static void dctest_sysvidmem(IDirectDrawSurface *surf, int ddsdver)
{
    HRESULT hr;
    HDC dc, dc2 = (HDC) 0x1234;
    DDSURFACEDESC ddsd;
    DDSURFACEDESC2 ddsd2;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);

    hr = IDirectDrawSurface_GetDC(surf, &dc);
    ok(hr == DD_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectDrawSurface_GetDC(surf, &dc2);
    ok(hr == DD_OK, "Unexpected hr %#lx.\n", hr);
    ok(dc == dc2, "Got two different DCs\n");

    hr = IDirectDrawSurface_Lock(surf, NULL, ddsdver == 1 ? &ddsd : ((DDSURFACEDESC *) &ddsd2), 0, NULL);
    ok(hr == DD_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectDrawSurface_Lock(surf, NULL, ddsdver == 1 ? &ddsd : ((DDSURFACEDESC *) &ddsd2), 0, NULL);
    ok(hr == DDERR_SURFACEBUSY, "Unexpected hr %#lx.\n", hr);

    hr = IDirectDrawSurface_Unlock(surf, NULL);
    ok(hr == DD_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDirectDrawSurface_Unlock(surf, NULL);
    ok(hr == DDERR_NOTLOCKED, "Unexpected hr %#lx.\n", hr);

    hr = IDirectDrawSurface_ReleaseDC(surf, dc);
    ok(hr == DD_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDirectDrawSurface_ReleaseDC(surf, dc);
    ok(hr == DD_OK, "Unexpected hr %#lx.\n", hr);
    /* That works any number of times... */
    hr = IDirectDrawSurface_ReleaseDC(surf, dc);
    ok(hr == DD_OK, "Unexpected hr %#lx.\n", hr);
}

static void SysVidMemTest(void)
{
    DDSURFACEDESC ddsd;
    DDSURFACEDESC2 ddsd2;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.dwWidth = 64;
    ddsd.dwHeight = 64;
    ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_VIDEOMEMORY;
    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);
    ddsd2.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd2.dwWidth = 64;
    ddsd2.dwHeight = 64;
    ddsd2.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_VIDEOMEMORY;

    GetDCTest_main(&ddsd, &ddsd2, dctest_sysvidmem);
}

static void test_surface_from_dc3(void)
{
    IDirectDrawSurface3 *surf3;
    IDirectDrawSurface *surf1;
    IDirectDrawSurface *tmp;
    DDSURFACEDESC ddsd;
    IDirectDraw3 *dd3;
    IDirectDraw *dd1;
    HRESULT hr;
    HDC dc;

    dd1 = createDDraw();
    hr = IDirectDraw_QueryInterface(dd1, &IID_IDirectDraw3, (void **)&dd3);
    ok(SUCCEEDED(hr), "Unexpected hr %#lx.\n", hr);
    IDirectDraw_Release(dd1);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.dwWidth = 64;
    ddsd.dwHeight = 64;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

    hr = IDirectDraw3_CreateSurface(dd3, &ddsd, &surf1, NULL);
    if (hr == DDERR_UNSUPPORTEDMODE) {
        win_skip("Unsupported mode\n");
        IDirectDraw3_Release(dd3);
        return;
    }
    ok(SUCCEEDED(hr), "Unexpected hr %#lx.\n", hr);

    hr = IDirectDrawSurface3_QueryInterface(surf1, &IID_IDirectDrawSurface, (void **)&surf3);
    ok(SUCCEEDED(hr), "Unexpected hr %#lx.\n", hr);
    IDirectDrawSurface_Release(surf1);

    hr = IDirectDrawSurface3_GetDC(surf3, &dc);
    ok(SUCCEEDED(hr), "Unexpected hr %#lx.\n", hr);

    hr = IDirectDraw3_GetSurfaceFromDC(dd3, dc, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IDirectDraw3_GetSurfaceFromDC(dd3, dc, &tmp);
    ok(SUCCEEDED(hr), "Unexpected hr %#lx.\n", hr);
    ok((IDirectDrawSurface3 *)tmp == surf3, "Expected surface != %p.\n", surf3);

    IUnknown_Release(tmp);

    hr = IDirectDrawSurface3_ReleaseDC(surf3, dc);
    ok(SUCCEEDED(hr), "Unexpected hr %#lx.\n", hr);

    dc = CreateCompatibleDC(NULL);
    ok(!!dc, "CreateCompatibleDC failed.\n");

    tmp = (IDirectDrawSurface *)0xdeadbeef;
    hr = IDirectDraw3_GetSurfaceFromDC(dd3, dc, &tmp);
    ok(hr == DDERR_NOTFOUND, "Unexpected hr %#lx.\n", hr);
    ok(!tmp, "Expected surface NULL, got %p.\n", tmp);

    ok(DeleteDC(dc), "DeleteDC failed.\n");

    IDirectDrawSurface3_Release(surf3);
    IDirectDraw3_Release(dd3);
}

DEFINE_GUID(guid, 0x38594b23, 0x2311, 0x4332, 0x95, 0xde, 0x2b, 0x0c, 0x61, 0xbf, 0x7b, 0x84);

static void test_surface_from_dc4(void)
{
    IDirectDrawSurface4 *surf4;
    IDirectDrawSurface *surf1;
    DDSURFACEDESC2 ddsd2;
    IUnknown *tmp, *tmp2;
    IDirectDraw4 *dd4;
    IDirectDraw *dd1;
    DWORD priv, size;
    HRESULT hr;
    HDC dc;

    dd1 = createDDraw();
    hr = IDirectDraw_QueryInterface(dd1, &IID_IDirectDraw4, (void **)&dd4);
    if (hr == E_NOINTERFACE) {
        win_skip("DirectDraw4 is not supported\n");
        IDirectDraw_Release(dd1);
        return;
    }
    ok(SUCCEEDED(hr), "Unexpected hr %#lx.\n", hr);
    IDirectDraw_Release(dd1);

    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);
    ddsd2.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd2.dwWidth = 64;
    ddsd2.dwHeight = 64;
    ddsd2.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

    hr = IDirectDraw4_CreateSurface(dd4, &ddsd2, &surf4, NULL);
    if (hr == DDERR_UNSUPPORTEDMODE) {
        win_skip("Unsupported mode\n");
        IDirectDraw3_Release(dd4);
        return;
    }
    ok(SUCCEEDED(hr), "Unexpected hr %#lx.\n", hr);

    hr = IDirectDrawSurface4_QueryInterface(surf4, &IID_IDirectDrawSurface, (void **)&surf1);
    ok(SUCCEEDED(hr), "Unexpected hr %#lx.\n", hr);

    priv = 0xdeadbeef;
    size = sizeof(priv);
    hr = IDirectDrawSurface4_SetPrivateData(surf4, &guid, &priv, size, 0);
    ok(SUCCEEDED(hr), "Unexpected hr %#lx.\n", hr);

    priv = 0;
    hr = IDirectDrawSurface4_GetPrivateData(surf4, &guid, &priv, &size);
    ok(SUCCEEDED(hr), "Unexpected hr %#lx.\n", hr);
    ok(priv == 0xdeadbeef, "Unexpected private data %#lx.\n", priv);

    hr = IDirectDrawSurface4_GetDC(surf4, &dc);
    ok(SUCCEEDED(hr), "Unexpected hr %#lx.\n", hr);

    hr = IDirectDraw4_GetSurfaceFromDC(dd4, dc, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IDirectDraw4_GetSurfaceFromDC(dd4, dc, (IDirectDrawSurface4 **)&tmp);
    ok(SUCCEEDED(hr), "Unexpected hr %#lx.\n", hr);
    ok((IDirectDrawSurface4 *)tmp != surf4, "Expected surface != %p.\n", surf4);

    hr = IUnknown_QueryInterface(tmp, &IID_IDirectDrawSurface, (void **)&tmp2);
    ok(SUCCEEDED(hr), "Unexpected hr %#lx.\n", hr);
    ok(tmp2 == tmp, "Expected %p, got %p.\n", tmp, tmp2);
    ok((IDirectDrawSurface *)tmp2 != surf1, "Expected surface != %p.\n", surf1);
    IUnknown_Release(tmp2);

    hr = IUnknown_QueryInterface(tmp, &IID_IDirectDrawSurface4, (void **)&tmp2);
    ok(SUCCEEDED(hr), "Unexpected hr %#lx.\n", hr);
    ok((IDirectDrawSurface4 *)tmp2 != surf4, "Expected surface != %p.\n", surf4);

    priv = 0;
    hr = IDirectDrawSurface4_GetPrivateData((IDirectDrawSurface4 *)tmp2, &guid, &priv, &size);
    ok(SUCCEEDED(hr), "Unexpected hr %#lx.\n", hr);
    ok(priv == 0xdeadbeef, "Unexpected private data %#lx.\n", priv);
    IUnknown_Release(tmp2);

    IUnknown_Release(tmp);

    hr = IDirectDrawSurface4_ReleaseDC(surf4, dc);
    ok(SUCCEEDED(hr), "Unexpected hr %#lx.\n", hr);

    dc = CreateCompatibleDC(NULL);
    ok(!!dc, "CreateCompatibleDC failed.\n");

    tmp = (IUnknown *)0xdeadbeef;
    hr = IDirectDraw4_GetSurfaceFromDC(dd4, dc, (IDirectDrawSurface4 **)&tmp);
    ok(hr == DDERR_NOTFOUND, "Unexpected hr %#lx.\n", hr);
    ok(!tmp, "Expected surface NULL, got %p.\n", tmp);

    ok(DeleteDC(dc), "DeleteDC failed.\n");

    tmp = (IUnknown *)0xdeadbeef;
    hr = IDirectDraw4_GetSurfaceFromDC(dd4, NULL, (IDirectDrawSurface4 **)&tmp);
    ok(hr == DDERR_NOTFOUND, "Unexpected hr %#lx.\n", hr);
    ok(!tmp, "Expected surface NULL, got %p.\n", tmp);

    IDirectDrawSurface_Release(surf1);
    IDirectDrawSurface4_Release(surf4);
    IDirectDraw4_Release(dd4);
}

START_TEST(surface)
{
    IClassFactory *classfactory = NULL;
    ULONG ref;
    HRESULT hr;
    HMODULE hmod = LoadLibraryA("ddrawex.dll");
    if(hmod == NULL) {
        skip("Failed to load ddrawex.dll\n");
        return;
    }
    pDllGetClassObject = (void*)GetProcAddress(hmod, "DllGetClassObject");
    if(pDllGetClassObject == NULL) {
        skip("Failed to get DllGetClassObject\n");
        return;
    }

    hr = pDllGetClassObject(&CLSID_DirectDrawFactory, &IID_IClassFactory, (void **) &classfactory);
    ok(hr == S_OK, "Failed to create a IClassFactory\n");
    if (FAILED(hr)) {
        skip("Failed to get DirectDrawFactory\n");
        return;
    }
    hr = IClassFactory_CreateInstance(classfactory, NULL, &IID_IDirectDrawFactory, (void **) &factory);
    ok(hr == S_OK, "Failed to create a IDirectDrawFactory\n");
    if (FAILED(hr)) {
        IClassFactory_Release(classfactory);
        skip("Failed to get a DirectDrawFactory\n");
        return;
    }

    GetDCTest();
    CapsTest();
    SysVidMemTest();
    test_surface_from_dc3();
    test_surface_from_dc4();

    ref = IDirectDrawFactory_Release(factory);
    ok(ref == 0, "IDirectDrawFactory not cleanly released\n");
    ref = IClassFactory_Release(classfactory);
    todo_wine ok(ref == 1, "Unexpected refcount %lu.\n", ref);
}
