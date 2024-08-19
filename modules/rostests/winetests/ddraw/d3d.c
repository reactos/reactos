/*
 * Some unit tests for d3d functions
 *
 * Copyright (C) 2005 Antoine Chavasse
 * Copyright (C) 2006,2011 Stefan Dösinger for CodeWeavers
 * Copyright (C) 2008 Alexander Dorofeyev
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
#include <limits.h>
#include "initguid.h"
#include "ddraw.h"
#include "d3d.h"
#include "unknwn.h"

static IDirectDraw7 *lpDD;
static IDirect3D7 *lpD3D;
static IDirectDrawSurface7 *lpDDS;
static IDirectDrawSurface7 *lpDDSdepth;
static IDirect3DDevice7 *lpD3DDevice;
static IDirect3DVertexBuffer7 *lpVBufSrc;

static IDirectDraw *DirectDraw1 = NULL;
static IDirectDrawSurface *Surface1 = NULL;
static IDirect3D *Direct3D1 = NULL;
static IDirect3DDevice *Direct3DDevice1 = NULL;
static IDirect3DExecuteBuffer *ExecuteBuffer = NULL;
static IDirect3DViewport *Viewport = NULL;
static IDirect3DLight *Light = NULL;

typedef struct {
    int total;
    int rgb;
    int hal;
    int tnlhal;
    int unk;
} D3D7ETest;

typedef struct {
    HRESULT desired_ret;
    int total;
} D3D7ECancelTest;

#define MAX_ENUMERATION_COUNT 10
typedef struct
{
    unsigned int count;
    char *callback_description_ptrs[MAX_ENUMERATION_COUNT];
    char callback_description_strings[MAX_ENUMERATION_COUNT][100];
    char *callback_name_ptrs[MAX_ENUMERATION_COUNT];
    char callback_name_strings[MAX_ENUMERATION_COUNT][100];
} D3D7ELifetimeTest;

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

static HRESULT WINAPI SurfaceCounter(IDirectDrawSurface7 *surface, DDSURFACEDESC2 *desc, void *context)
{
    UINT *num = context;
    (*num)++;
    IDirectDrawSurface_Release(surface);
    return DDENUMRET_OK;
}

static BOOL CreateDirect3D(void)
{
    HRESULT rc;
    DDSURFACEDESC2 ddsd;
    UINT num;

    rc = pDirectDrawCreateEx(NULL, (void**)&lpDD,
        &IID_IDirectDraw7, NULL);
    ok(rc==DD_OK || rc==DDERR_NODIRECTDRAWSUPPORT, "DirectDrawCreateEx returned: %x\n", rc);
    if (!lpDD) {
        trace("DirectDrawCreateEx() failed with an error %x\n", rc);
        return FALSE;
    }

    rc = IDirectDraw_SetCooperativeLevel(lpDD, NULL, DDSCL_NORMAL);
    ok(rc==DD_OK, "SetCooperativeLevel returned: %x\n", rc);

    rc = IDirectDraw7_QueryInterface(lpDD, &IID_IDirect3D7, (void**) &lpD3D);
    if (rc == E_NOINTERFACE) return FALSE;
    ok(rc==DD_OK, "QueryInterface returned: %x\n", rc);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    ddsd.dwWidth = 256;
    ddsd.dwHeight = 256;
    rc = IDirectDraw7_CreateSurface(lpDD, &ddsd, &lpDDS, NULL);
    if (FAILED(rc))
        return FALSE;

    num = 0;
    IDirectDraw7_EnumSurfaces(lpDD, DDENUMSURFACES_ALL | DDENUMSURFACES_DOESEXIST, NULL, &num, SurfaceCounter);
    ok(num == 1, "Has %d surfaces, expected 1\n", num);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
    U1(U4(ddsd).ddpfPixelFormat).dwZBufferBitDepth = 16;
    U3(U4(ddsd).ddpfPixelFormat).dwZBitMask = 0x0000FFFF;
    ddsd.dwWidth = 256;
    ddsd.dwHeight = 256;
    rc = IDirectDraw7_CreateSurface(lpDD, &ddsd, &lpDDSdepth, NULL);
    ok(rc==DD_OK, "CreateSurface returned: %x\n", rc);
    if (FAILED(rc)) {
        lpDDSdepth = NULL;
    } else {
        rc = IDirectDrawSurface_AddAttachedSurface(lpDDS, lpDDSdepth);
        ok(rc == DD_OK, "IDirectDrawSurface_AddAttachedSurface returned %x\n", rc);
        if (FAILED(rc))
            return FALSE;
    }

    rc = IDirect3D7_CreateDevice(lpD3D, &IID_IDirect3DTnLHalDevice, lpDDS,
        &lpD3DDevice);
    ok(rc==D3D_OK || rc==DDERR_NOPALETTEATTACHED || rc==E_OUTOFMEMORY, "CreateDevice returned: %x\n", rc);
    if (!lpD3DDevice) {
        trace("IDirect3D7::CreateDevice() for a TnL Hal device failed with an error %x, trying HAL\n", rc);
        rc = IDirect3D7_CreateDevice(lpD3D, &IID_IDirect3DHALDevice, lpDDS,
            &lpD3DDevice);
        if (!lpD3DDevice) {
            trace("IDirect3D7::CreateDevice() for a HAL device failed with an error %x, trying RGB\n", rc);
            rc = IDirect3D7_CreateDevice(lpD3D, &IID_IDirect3DRGBDevice, lpDDS,
                &lpD3DDevice);
            if (!lpD3DDevice) {
                trace("IDirect3D7::CreateDevice() for a RGB device failed with an error %x, giving up\n", rc);
                return FALSE;
            }
        }
    }

    return TRUE;
}

static void ReleaseDirect3D(void)
{
    if (lpD3DDevice != NULL)
    {
        IDirect3DDevice7_Release(lpD3DDevice);
        lpD3DDevice = NULL;
    }

    if (lpDDSdepth != NULL)
    {
        IDirectDrawSurface_Release(lpDDSdepth);
        lpDDSdepth = NULL;
    }

    if (lpDDS != NULL)
    {
        IDirectDrawSurface_Release(lpDDS);
        lpDDS = NULL;
    }

    if (lpD3D != NULL)
    {
        IDirect3D7_Release(lpD3D);
        lpD3D = NULL;
    }

    if (lpDD != NULL)
    {
        IDirectDraw_Release(lpDD);
        lpDD = NULL;
    }
}

static void LightTest(void)
{
    HRESULT rc;
    D3DLIGHT7 light;
    D3DLIGHT7 defaultlight;
    BOOL bEnabled = FALSE;
    float one = 1.0f;
    float zero= 0.0f;
    D3DMATERIAL7 mat;
    BOOL enabled;
    unsigned int i;
    D3DDEVICEDESC7 caps;

    /* Set a few lights with funky indices. */
    memset(&light, 0, sizeof(light));
    light.dltType = D3DLIGHT_DIRECTIONAL;
    U1(light.dcvDiffuse).r = 0.5f;
    U2(light.dcvDiffuse).g = 0.6f;
    U3(light.dcvDiffuse).b = 0.7f;
    U2(light.dvDirection).y = 1.f;

    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 5, &light);
    ok(rc==D3D_OK, "SetLight returned: %x\n", rc);
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 10, &light);
    ok(rc==D3D_OK, "SetLight returned: %x\n", rc);
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 45, &light);
    ok(rc==D3D_OK, "SetLight returned: %x\n", rc);


    /* Try to retrieve a light beyond the indices of the lights that have
       been set. */
    rc = IDirect3DDevice7_GetLight(lpD3DDevice, 50, &light);
    ok(rc==DDERR_INVALIDPARAMS, "GetLight returned: %x\n", rc);
    rc = IDirect3DDevice7_GetLight(lpD3DDevice, 2, &light);
    ok(rc==DDERR_INVALIDPARAMS, "GetLight returned: %x\n", rc);


    /* Try to retrieve one of the lights that have been set */
    rc = IDirect3DDevice7_GetLight(lpD3DDevice, 10, &light);
    ok(rc==D3D_OK, "GetLight returned: %x\n", rc);


    /* Enable a light that have been previously set. */
    rc = IDirect3DDevice7_LightEnable(lpD3DDevice, 10, TRUE);
    ok(rc==D3D_OK, "LightEnable returned: %x\n", rc);


    /* Enable some lights that have not been previously set, and verify that
       they have been initialized with proper default values. */
    memset(&defaultlight, 0, sizeof(D3DLIGHT7));
    defaultlight.dltType = D3DLIGHT_DIRECTIONAL;
    U1(defaultlight.dcvDiffuse).r = 1.f;
    U2(defaultlight.dcvDiffuse).g = 1.f;
    U3(defaultlight.dcvDiffuse).b = 1.f;
    U3(defaultlight.dvDirection).z = 1.f;

    rc = IDirect3DDevice7_LightEnable(lpD3DDevice, 20, TRUE);
    ok(rc==D3D_OK, "LightEnable returned: %x\n", rc);
    memset(&light, 0, sizeof(D3DLIGHT7));
    rc = IDirect3DDevice7_GetLight(lpD3DDevice, 20, &light);
    ok(rc==D3D_OK, "GetLight returned: %x\n", rc);
    ok(!memcmp(&light, &defaultlight, sizeof(D3DLIGHT7)),
        "light data doesn't match expected default values\n" );

    rc = IDirect3DDevice7_LightEnable(lpD3DDevice, 50, TRUE);
    ok(rc==D3D_OK, "LightEnable returned: %x\n", rc);
    memset(&light, 0, sizeof(D3DLIGHT7));
    rc = IDirect3DDevice7_GetLight(lpD3DDevice, 50, &light);
    ok(rc==D3D_OK, "GetLight returned: %x\n", rc);
    ok(!memcmp(&light, &defaultlight, sizeof(D3DLIGHT7)),
        "light data doesn't match expected default values\n" );


    /* Disable one of the light that have been previously enabled. */
    rc = IDirect3DDevice7_LightEnable(lpD3DDevice, 20, FALSE);
    ok(rc==D3D_OK, "LightEnable returned: %x\n", rc);

    /* Try to retrieve the enable status of some lights */
    /* Light 20 is supposed to be disabled */
    rc = IDirect3DDevice7_GetLightEnable(lpD3DDevice, 20, &bEnabled );
    ok(rc==D3D_OK, "GetLightEnable returned: %x\n", rc);
    ok(!bEnabled, "GetLightEnable says the light is enabled\n");

    /* Light 10 is supposed to be enabled */
    bEnabled = FALSE;
    rc = IDirect3DDevice7_GetLightEnable(lpD3DDevice, 10, &bEnabled );
    ok(rc==D3D_OK, "GetLightEnable returned: %x\n", rc);
    ok(bEnabled, "GetLightEnable says the light is disabled\n");

    /* Light 80 has not been set */
    rc = IDirect3DDevice7_GetLightEnable(lpD3DDevice, 80, &bEnabled );
    ok(rc==DDERR_INVALIDPARAMS, "GetLightEnable returned: %x\n", rc);

    /* Light 23 has not been set */
    rc = IDirect3DDevice7_GetLightEnable(lpD3DDevice, 23, &bEnabled );
    ok(rc==DDERR_INVALIDPARAMS, "GetLightEnable returned: %x\n", rc);

    /* Set some lights with invalid parameters */
    memset(&light, 0, sizeof(D3DLIGHT7));
    light.dltType = 0;
    U1(light.dcvDiffuse).r = 1.f;
    U2(light.dcvDiffuse).g = 1.f;
    U3(light.dcvDiffuse).b = 1.f;
    U3(light.dvDirection).z = 1.f;
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 100, &light);
    ok(rc==DDERR_INVALIDPARAMS, "SetLight returned: %x\n", rc);

    memset(&light, 0, sizeof(D3DLIGHT7));
    light.dltType = 12345;
    U1(light.dcvDiffuse).r = 1.f;
    U2(light.dcvDiffuse).g = 1.f;
    U3(light.dcvDiffuse).b = 1.f;
    U3(light.dvDirection).z = 1.f;
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 101, &light);
    ok(rc==DDERR_INVALIDPARAMS, "SetLight returned: %x\n", rc);

    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 102, NULL);
    ok(rc==DDERR_INVALIDPARAMS, "SetLight returned: %x\n", rc);

    memset(&light, 0, sizeof(D3DLIGHT7));
    light.dltType = D3DLIGHT_SPOT;
    U1(light.dcvDiffuse).r = 1.f;
    U2(light.dcvDiffuse).g = 1.f;
    U3(light.dcvDiffuse).b = 1.f;
    U3(light.dvDirection).z = 1.f;

    light.dvAttenuation0 = -one / zero; /* -INFINITY */
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 103, &light);
    ok(rc==DDERR_INVALIDPARAMS, "SetLight returned: %x\n", rc);

    light.dvAttenuation0 = -1.0;
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 103, &light);
    ok(rc==DDERR_INVALIDPARAMS, "SetLight returned: %x\n", rc);

    light.dvAttenuation0 = 0.0;
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 103, &light);
    ok(rc==D3D_OK, "SetLight returned: %x\n", rc);

    light.dvAttenuation0 = 1.0;
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 103, &light);
    ok(rc==D3D_OK, "SetLight returned: %x\n", rc);

    light.dvAttenuation0 = one / zero; /* +INFINITY */
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 103, &light);
    ok(rc==D3D_OK, "SetLight returned: %x\n", rc);

    light.dvAttenuation0 = zero / zero; /* NaN */
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 103, &light);
    ok(rc==D3D_OK ||
       broken(rc==DDERR_INVALIDPARAMS), "SetLight returned: %x\n", rc);

    /* Directional light ignores attenuation */
    light.dltType = D3DLIGHT_DIRECTIONAL;
    light.dvAttenuation0 = -1.0;
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 103, &light);
    ok(rc==D3D_OK, "SetLight returned: %x\n", rc);

    memset(&mat, 0, sizeof(mat));
    rc = IDirect3DDevice7_SetMaterial(lpD3DDevice, &mat);
    ok(rc == D3D_OK, "IDirect3DDevice7_SetMaterial returned: %x\n", rc);

    U4(mat).power = 129.0;
    rc = IDirect3DDevice7_SetMaterial(lpD3DDevice, &mat);
    ok(rc == D3D_OK, "IDirect3DDevice7_SetMaterial(power = 129.0) returned: %x\n", rc);
    memset(&mat, 0, sizeof(mat));
    rc = IDirect3DDevice7_GetMaterial(lpD3DDevice, &mat);
    ok(rc == D3D_OK, "IDirect3DDevice7_GetMaterial returned: %x\n", rc);
    ok(U4(mat).power == 129, "Returned power is %f\n", U4(mat).power);

    U4(mat).power = -1.0;
    rc = IDirect3DDevice7_SetMaterial(lpD3DDevice, &mat);
    ok(rc == D3D_OK, "IDirect3DDevice7_SetMaterial(power = -1.0) returned: %x\n", rc);
    memset(&mat, 0, sizeof(mat));
    rc = IDirect3DDevice7_GetMaterial(lpD3DDevice, &mat);
    ok(rc == D3D_OK, "IDirect3DDevice7_GetMaterial returned: %x\n", rc);
    ok(U4(mat).power == -1, "Returned power is %f\n", U4(mat).power);

    memset(&caps, 0, sizeof(caps));
    rc = IDirect3DDevice7_GetCaps(lpD3DDevice, &caps);
    ok(rc == D3D_OK, "IDirect3DDevice7_GetCaps failed with %x\n", rc);

    if ( caps.dwMaxActiveLights == (DWORD) -1) {
        /* Some cards without T&L Support return -1 (Examples: Voodoo Banshee, RivaTNT / NV4) */
        skip("T&L not supported\n");
        return;
    }

    for(i = 1; i <= caps.dwMaxActiveLights; i++) {
        rc = IDirect3DDevice7_LightEnable(lpD3DDevice, i, TRUE);
        ok(rc == D3D_OK, "Enabling light %u failed with %x\n", i, rc);
        rc = IDirect3DDevice7_GetLightEnable(lpD3DDevice, i, &enabled);
        ok(rc == D3D_OK, "GetLightEnable on light %u failed with %x\n", i, rc);
        ok(enabled, "Light %d is %s\n", i, enabled ? "enabled" : "disabled");
    }

    /* TODO: Test the rendering results in this situation */
    rc = IDirect3DDevice7_LightEnable(lpD3DDevice, i + 1, TRUE);
    ok(rc == D3D_OK, "Enabling one light more than supported returned %x\n", rc);
    rc = IDirect3DDevice7_GetLightEnable(lpD3DDevice, i + 1, &enabled);
    ok(rc == D3D_OK, "GetLightEnable on light %u failed with %x\n", i + 1,  rc);
    ok(enabled, "Light %d is %s\n", i + 1, enabled ? "enabled" : "disabled");
    rc = IDirect3DDevice7_LightEnable(lpD3DDevice, i + 1, FALSE);
    ok(rc == D3D_OK, "Disabling the additional returned %x\n", rc);

    for(i = 1; i <= caps.dwMaxActiveLights; i++) {
        rc = IDirect3DDevice7_LightEnable(lpD3DDevice, i, FALSE);
        ok(rc == D3D_OK, "Disabling light %u failed with %x\n", i, rc);
    }
}

static void SceneTest(void)
{
    HRESULT                      hr;

    /* Test an EndScene without BeginScene. Should return an error */
    hr = IDirect3DDevice7_EndScene(lpD3DDevice);
    ok(hr == D3DERR_SCENE_NOT_IN_SCENE, "IDirect3DDevice7_EndScene returned %08x\n", hr);

    /* Test a normal BeginScene / EndScene pair, this should work */
    hr = IDirect3DDevice7_BeginScene(lpD3DDevice);
    ok(hr == D3D_OK, "IDirect3DDevice7_BeginScene failed with %08x\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IDirect3DDevice7_EndScene(lpD3DDevice);
        ok(hr == D3D_OK, "IDirect3DDevice7_EndScene failed with %08x\n", hr);
    }

    if (lpDDSdepth)
    {
        DDBLTFX fx;
        memset(&fx, 0, sizeof(fx));
        fx.dwSize = sizeof(fx);

        hr = IDirectDrawSurface7_Blt(lpDDSdepth, NULL, NULL, NULL, DDBLT_DEPTHFILL, &fx);
        ok(hr == D3D_OK, "Depthfill failed outside a BeginScene / EndScene pair, hr 0x%08x\n", hr);

        hr = IDirect3DDevice7_BeginScene(lpD3DDevice);
        ok(hr == D3D_OK, "IDirect3DDevice7_BeginScene failed with %08x\n", hr);
        if (SUCCEEDED(hr))
        {
            hr = IDirectDrawSurface7_Blt(lpDDSdepth, NULL, NULL, NULL, DDBLT_DEPTHFILL, &fx);
            ok(hr == D3D_OK || broken(hr == E_FAIL),
                    "Depthfill failed in a BeginScene / EndScene pair, hr 0x%08x\n", hr);
            hr = IDirect3DDevice7_EndScene(lpD3DDevice);
            ok(hr == D3D_OK, "IDirect3DDevice7_EndScene failed with %08x\n", hr);
        }
    }
    else
    {
        skip("Depth stencil creation failed at startup, skipping depthfill test\n");
    }

    /* Test another EndScene without having begun a new scene. Should return an error */
    hr = IDirect3DDevice7_EndScene(lpD3DDevice);
    ok(hr == D3DERR_SCENE_NOT_IN_SCENE, "IDirect3DDevice7_EndScene returned %08x\n", hr);

    /* Two nested BeginScene and EndScene calls */
    hr = IDirect3DDevice7_BeginScene(lpD3DDevice);
    ok(hr == D3D_OK, "IDirect3DDevice7_BeginScene failed with %08x\n", hr);
    hr = IDirect3DDevice7_BeginScene(lpD3DDevice);
    ok(hr == D3DERR_SCENE_IN_SCENE, "IDirect3DDevice7_BeginScene returned %08x\n", hr);
    hr = IDirect3DDevice7_EndScene(lpD3DDevice);
    ok(hr == D3D_OK, "IDirect3DDevice7_EndScene failed with %08x\n", hr);
    hr = IDirect3DDevice7_EndScene(lpD3DDevice);
    ok(hr == D3DERR_SCENE_NOT_IN_SCENE, "IDirect3DDevice7_EndScene returned %08x\n", hr);

    /* TODO: Verify that blitting works in the same way as in d3d9 */
}

static HRESULT WINAPI enumDevicesCallback(GUID *Guid, char *DeviceDescription,
        char *DeviceName, D3DDEVICEDESC *hal, D3DDEVICEDESC *hel, void *ctx)
{
    UINT ver = *((UINT *) ctx);
    if(IsEqualGUID(&IID_IDirect3DRGBDevice, Guid))
    {
        ok((hal->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) == 0,
           "RGB Device %d hal line caps has D3DPTEXTURECAPS_POW2 flag set\n", ver);
        ok((hal->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) == 0,
           "RGB Device %d hal tri caps has D3DPTEXTURECAPS_POW2 flag set\n", ver);
        ok(hel->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2,
           "RGB Device %d hel line caps does not have D3DPTEXTURECAPS_POW2 flag set\n", ver);
        ok(hel->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2,
           "RGB Device %d hel tri caps does not have D3DPTEXTURECAPS_POW2 flag set\n", ver);

        ok((hal->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE) == 0,
           "RGB Device %d hal line caps has D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
        ok((hal->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE) == 0,
           "RGB Device %d hal tri caps has D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
        ok(hel->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE,
           "RGB Device %d hel tri caps does not have D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
        ok(hel->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE,
           "RGB Device %d hel tri caps does not have D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);

        ok(hal->dcmColorModel == 0, "RGB Device %u hal caps has colormodel %u\n", ver, hal->dcmColorModel);
        ok(hel->dcmColorModel == D3DCOLOR_RGB, "RGB Device %u hel caps has colormodel %u\n", ver, hel->dcmColorModel);

        ok(hal->dwFlags == 0, "RGB Device %u hal caps has hardware flags %x\n", ver, hal->dwFlags);
        ok(hel->dwFlags != 0, "RGB Device %u hel caps has hardware flags %x\n", ver, hel->dwFlags);
    }
    else if(IsEqualGUID(&IID_IDirect3DHALDevice, Guid))
    {
        trace("HAL Device %d\n", ver);
        ok(hal->dcmColorModel == D3DCOLOR_RGB, "HAL Device %u hal caps has colormodel %u\n", ver, hel->dcmColorModel);
        ok(hel->dcmColorModel == 0, "HAL Device %u hel caps has colormodel %u\n", ver, hel->dcmColorModel);

        ok(hal->dwFlags != 0, "HAL Device %u hal caps has hardware flags %x\n", ver, hal->dwFlags);
        ok(hel->dwFlags != 0, "HAL Device %u hel caps has hardware flags %x\n", ver, hel->dwFlags);
    }
    else if(IsEqualGUID(&IID_IDirect3DRefDevice, Guid))
    {
        ok((hal->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) == 0,
           "REF Device %d hal line caps has D3DPTEXTURECAPS_POW2 flag set\n", ver);
        ok((hal->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) == 0,
           "REF Device %d hal tri caps has D3DPTEXTURECAPS_POW2 flag set\n", ver);
        ok(hel->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2,
           "REF Device %d hel line caps does not have D3DPTEXTURECAPS_POW2 flag set\n", ver);
        ok(hel->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2,
           "REF Device %d hel tri caps does not have D3DPTEXTURECAPS_POW2 flag set\n", ver);

        ok((hal->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE) == 0,
           "REF Device %d hal line caps has D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
        ok((hal->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE) == 0,
           "REF Device %d hal tri caps has D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
        ok(hel->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE,
           "REF Device %d hel tri caps does not have D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
        ok(hel->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE,
           "REF Device %d hel tri caps does not have D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
    }
    else if(IsEqualGUID(&IID_IDirect3DRampDevice, Guid))
    {
        ok((hal->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) == 0,
           "Ramp Device %d hal line caps has D3DPTEXTURECAPS_POW2 flag set\n", ver);
        ok((hal->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) == 0,
           "Ramp Device %d hal tri caps has D3DPTEXTURECAPS_POW2 flag set\n", ver);
        ok(hel->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2,
           "Ramp Device %d hel line caps does not have D3DPTEXTURECAPS_POW2 flag set\n", ver);
        ok(hel->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2,
           "Ramp Device %d hel tri caps does not have D3DPTEXTURECAPS_POW2 flag set\n", ver);

        ok((hal->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE) == 0,
           "Ramp Device %d hal line caps has D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
        ok((hal->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE) == 0,
           "Ramp Device %d hal tri caps has D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
        ok(hel->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE,
           "Ramp Device %d hel tri caps does not have D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
        ok(hel->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE,
           "Ramp Device %d hel tri caps does not have D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);

        ok(hal->dcmColorModel == 0, "Ramp Device %u hal caps has colormodel %u\n", ver, hal->dcmColorModel);
        ok(hel->dcmColorModel == D3DCOLOR_MONO, "Ramp Device %u hel caps has colormodel %u\n",
                ver, hel->dcmColorModel);

        ok(hal->dwFlags == 0, "Ramp Device %u hal caps has hardware flags %x\n", ver, hal->dwFlags);
        ok(hel->dwFlags != 0, "Ramp Device %u hel caps has hardware flags %x\n", ver, hel->dwFlags);
    }
    else if(IsEqualGUID(&IID_IDirect3DMMXDevice, Guid))
    {
        ok((hal->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) == 0,
           "MMX Device %d hal line caps has D3DPTEXTURECAPS_POW2 flag set\n", ver);
        ok((hal->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) == 0,
           "MMX Device %d hal tri caps has D3DPTEXTURECAPS_POW2 flag set\n", ver);
        ok(hel->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2,
           "MMX Device %d hel line caps does not have D3DPTEXTURECAPS_POW2 flag set\n", ver);
        ok(hel->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2,
           "MMX Device %d hel tri caps does not have D3DPTEXTURECAPS_POW2 flag set\n", ver);

        ok((hal->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE) == 0,
           "MMX Device %d hal line caps has D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
        ok((hal->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE) == 0,
           "MMX Device %d hal tri caps has D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
        ok(hel->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE,
           "MMX Device %d hel tri caps does not have D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
        ok(hel->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE,
           "MMX Device %d hel tri caps does not have D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);

        ok(hal->dcmColorModel == 0, "MMX Device %u hal caps has colormodel %u\n", ver, hal->dcmColorModel);
        ok(hel->dcmColorModel == D3DCOLOR_RGB, "MMX Device %u hel caps has colormodel %u\n", ver, hel->dcmColorModel);

        ok(hal->dwFlags == 0, "MMX Device %u hal caps has hardware flags %x\n", ver, hal->dwFlags);
        ok(hel->dwFlags != 0, "MMX Device %u hel caps has hardware flags %x\n", ver, hel->dwFlags);
    }
    else
    {
        ok(FALSE, "Unexpected device enumerated: \"%s\" \"%s\"\n", DeviceDescription, DeviceName);
        if(hal->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) trace("hal line has pow2 set\n");
        else trace("hal line does NOT have pow2 set\n");
        if(hal->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) trace("hal tri has pow2 set\n");
        else trace("hal tri does NOT have pow2 set\n");
        if(hel->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) trace("hel line has pow2 set\n");
        else trace("hel line does NOT have pow2 set\n");
        if(hel->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) trace("hel tri has pow2 set\n");
        else trace("hel tri does NOT have pow2 set\n");
    }
    return DDENUMRET_OK;
}

static HRESULT WINAPI enumDevicesCallbackTest7(char *DeviceDescription, char *DeviceName,
        D3DDEVICEDESC7 *lpdd7, void *Context)
{
    D3D7ETest *d3d7et = Context;
    if(IsEqualGUID(&lpdd7->deviceGUID, &IID_IDirect3DRGBDevice))
        d3d7et->rgb++;
    else if(IsEqualGUID(&lpdd7->deviceGUID, &IID_IDirect3DHALDevice))
        d3d7et->hal++;
    else if(IsEqualGUID(&lpdd7->deviceGUID, &IID_IDirect3DTnLHalDevice))
        d3d7et->tnlhal++;
    else
        d3d7et->unk++;

    d3d7et->total++;

    return DDENUMRET_OK;
}

static HRESULT WINAPI enumDevicesCancelTest7(char *DeviceDescription, char *DeviceName,
        D3DDEVICEDESC7 *lpdd7, void *Context)
{
    D3D7ECancelTest *d3d7et = Context;

    d3d7et->total++;

    return d3d7et->desired_ret;
}

static HRESULT WINAPI enumDevicesLifetimeTest7(char *DeviceDescription, char *DeviceName,
        D3DDEVICEDESC7 *lpdd7, void *Context)
{
    D3D7ELifetimeTest *ctx = Context;

    if (ctx->count == MAX_ENUMERATION_COUNT)
    {
        ok(0, "Enumerated too many devices for context in callback\n");
        return DDENUMRET_CANCEL;
    }

    ctx->callback_description_ptrs[ctx->count] = DeviceDescription;
    strcpy(ctx->callback_description_strings[ctx->count], DeviceDescription);
    ctx->callback_name_ptrs[ctx->count] = DeviceName;
    strcpy(ctx->callback_name_strings[ctx->count], DeviceName);

    ctx->count++;
    return DDENUMRET_OK;
}

/*  Check the deviceGUID of devices enumerated by
    IDirect3D7_EnumDevices. */
static void D3D7EnumTest(void)
{
    HRESULT hr;
    D3D7ETest d3d7et;
    D3D7ECancelTest d3d7_cancel_test;

    hr = IDirect3D7_EnumDevices(lpD3D, NULL, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirect3D7_EnumDevices returned 0x%08x\n", hr);

    memset(&d3d7et, 0, sizeof(d3d7et));
    hr = IDirect3D7_EnumDevices(lpD3D, enumDevicesCallbackTest7, &d3d7et);
    ok(hr == D3D_OK, "IDirect3D7_EnumDevices returned 0x%08x\n", hr);

    /* A couple of games (Delta Force LW and TFD) rely on this behaviour */
    ok(d3d7et.tnlhal < d3d7et.total, "TnLHal device enumerated as only device.\n");

    /* We make two additional assumptions. */
    ok(d3d7et.rgb, "No RGB Device enumerated.\n");

    if(d3d7et.tnlhal)
        ok(d3d7et.hal, "TnLHal device enumerated, but no Hal device found.\n");

    d3d7_cancel_test.desired_ret = DDENUMRET_CANCEL;
    d3d7_cancel_test.total = 0;
    hr = IDirect3D7_EnumDevices(lpD3D, enumDevicesCancelTest7, &d3d7_cancel_test);
    ok(hr == D3D_OK, "IDirect3D7_EnumDevices returned 0x%08x\n", hr);

    ok(d3d7_cancel_test.total == 1, "Enumerated a total of %u devices\n",
       d3d7_cancel_test.total);

    /* An enumeration callback can return any value besides DDENUMRET_OK to stop enumeration. */
    d3d7_cancel_test.desired_ret = E_INVALIDARG;
    d3d7_cancel_test.total = 0;
    hr = IDirect3D7_EnumDevices(lpD3D, enumDevicesCancelTest7, &d3d7_cancel_test);
    ok(hr == D3D_OK, "IDirect3D7_EnumDevices returned 0x%08x\n", hr);

    ok(d3d7_cancel_test.total == 1, "Enumerated a total of %u devices\n",
       d3d7_cancel_test.total);
}

static void D3D7EnumLifetimeTest(void)
{
    D3D7ELifetimeTest ctx, ctx2;
    HRESULT hr;
    unsigned int i;

    ctx.count = 0;
    hr = IDirect3D7_EnumDevices(lpD3D, enumDevicesLifetimeTest7, &ctx);
    ok(hr == D3D_OK, "IDirect3D7_EnumDevices returned 0x%08x\n", hr);

    /* The enumeration strings remain valid even after IDirect3D7_EnumDevices finishes. */
    for (i = 0; i < ctx.count; i++)
    {
        ok(!strcmp(ctx.callback_description_ptrs[i], ctx.callback_description_strings[i]),
           "Got '%s' and '%s'\n", ctx.callback_description_ptrs[i], ctx.callback_description_strings[i]);
        ok(!strcmp(ctx.callback_name_ptrs[i], ctx.callback_name_strings[i]),
           "Got '%s' and '%s'\n", ctx.callback_name_ptrs[i], ctx.callback_name_strings[i]);
    }

    ctx2.count = 0;
    hr = IDirect3D7_EnumDevices(lpD3D, enumDevicesLifetimeTest7, &ctx2);
    ok(hr == D3D_OK, "IDirect3D7_EnumDevices returned 0x%08x\n", hr);

    /* The enumeration strings and their order are identical across enumerations. */
    ok(ctx.count == ctx2.count, "Enumerated %u and %u devices\n", ctx.count, ctx2.count);
    if (ctx.count == ctx2.count)
    {
        for (i = 0; i < ctx.count; i++)
        {
            ok(ctx.callback_description_ptrs[i] == ctx2.callback_description_ptrs[i],
               "Unequal description pointers %p and %p\n", ctx.callback_description_ptrs[i], ctx2.callback_description_ptrs[i]);
            ok(!strcmp(ctx.callback_description_strings[i], ctx2.callback_description_strings[i]),
               "Got '%s' and '%s'\n", ctx.callback_description_strings[i], ctx2.callback_description_strings[i]);
            ok(ctx.callback_name_ptrs[i] == ctx2.callback_name_ptrs[i],
               "Unequal name pointers %p and %p\n", ctx.callback_name_ptrs[i], ctx2.callback_name_ptrs[i]);
            ok(!strcmp(ctx.callback_name_strings[i], ctx2.callback_name_strings[i]),
               "Got '%s' and '%s'\n", ctx.callback_name_strings[i], ctx2.callback_name_strings[i]);
        }
    }

    /* Try altering the contents of the enumeration strings. */
    for (i = 0; i < ctx2.count; i++)
    {
        strcpy(ctx2.callback_description_ptrs[i], "Fake Description");
        strcpy(ctx2.callback_name_ptrs[i], "Fake Device");
    }

    ctx2.count = 0;
    hr = IDirect3D7_EnumDevices(lpD3D, enumDevicesLifetimeTest7, &ctx2);
    ok(hr == D3D_OK, "IDirect3D7_EnumDevices returned 0x%08x\n", hr);

    /* The original contents of the enumeration strings are not restored. */
    ok(ctx.count == ctx2.count, "Enumerated %u and %u devices\n", ctx.count, ctx2.count);
    if (ctx.count == ctx2.count)
    {
        for (i = 0; i < ctx.count; i++)
        {
            ok(ctx.callback_description_ptrs[i] == ctx2.callback_description_ptrs[i],
               "Unequal description pointers %p and %p\n", ctx.callback_description_ptrs[i], ctx2.callback_description_ptrs[i]);
            ok(strcmp(ctx.callback_description_strings[i], ctx2.callback_description_strings[i]) != 0,
               "Got '%s' and '%s'\n", ctx.callback_description_strings[i], ctx2.callback_description_strings[i]);
            ok(ctx.callback_name_ptrs[i] == ctx2.callback_name_ptrs[i],
               "Unequal name pointers %p and %p\n", ctx.callback_name_ptrs[i], ctx2.callback_name_ptrs[i]);
            ok(strcmp(ctx.callback_name_strings[i], ctx2.callback_name_strings[i]) != 0,
               "Got '%s' and '%s'\n", ctx.callback_name_strings[i], ctx2.callback_name_strings[i]);
        }
    }
}

static void CapsTest(void)
{
    IDirect3D3 *d3d3;
    IDirect3D3 *d3d2;
    IDirectDraw *dd1;
    HRESULT hr;
    UINT ver;

    hr = DirectDrawCreate(NULL, &dd1, NULL);
    ok(hr == DD_OK, "Cannot create a DirectDraw 1 interface, hr = %08x\n", hr);
    hr = IDirectDraw_QueryInterface(dd1, &IID_IDirect3D3, (void **) &d3d3);
    ok(hr == D3D_OK, "IDirectDraw_QueryInterface returned %08x\n", hr);

    hr = IDirect3D3_EnumDevices(d3d3, NULL, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirect3D3_EnumDevices returned 0x%08x\n", hr);

    ver = 3;
    IDirect3D3_EnumDevices(d3d3, enumDevicesCallback, &ver);

    IDirect3D3_Release(d3d3);
    IDirectDraw_Release(dd1);

    hr = DirectDrawCreate(NULL, &dd1, NULL);
    ok(hr == DD_OK, "Cannot create a DirectDraw 1 interface, hr = %08x\n", hr);
    hr = IDirectDraw_QueryInterface(dd1, &IID_IDirect3D2, (void **) &d3d2);
    ok(hr == D3D_OK, "IDirectDraw_QueryInterface returned %08x\n", hr);

    hr = IDirect3D2_EnumDevices(d3d2, NULL, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirect3D2_EnumDevices returned 0x%08x\n", hr);

    ver = 2;
    IDirect3D2_EnumDevices(d3d2, enumDevicesCallback, &ver);

    IDirect3D2_Release(d3d2);
    IDirectDraw_Release(dd1);
}

struct v_in {
    float x, y, z;
};
struct v_out {
    float x, y, z, rhw;
};

static BOOL D3D1_createObjects(void)
{
    HRESULT hr;
    DDSURFACEDESC ddsd;
    D3DEXECUTEBUFFERDESC desc;
    D3DVIEWPORT vp_data;

    /* An IDirect3DDevice cannot be queryInterfaced from an IDirect3DDevice7 on windows */
    hr = DirectDrawCreate(NULL, &DirectDraw1, NULL);
    ok(hr==DD_OK || hr==DDERR_NODIRECTDRAWSUPPORT, "DirectDrawCreate returned: %x\n", hr);
    if (!DirectDraw1) {
        return FALSE;
    }

    hr = IDirectDraw_SetCooperativeLevel(DirectDraw1, NULL, DDSCL_NORMAL);
    ok(hr==DD_OK, "SetCooperativeLevel returned: %x\n", hr);

    hr = IDirectDraw_QueryInterface(DirectDraw1, &IID_IDirect3D, (void**) &Direct3D1);
    if (hr == E_NOINTERFACE) return FALSE;
    ok(hr==DD_OK, "QueryInterface returned: %x\n", hr);
    if (!Direct3D1) {
        return FALSE;
    }

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    ddsd.dwWidth = 256;
    ddsd.dwHeight = 256;
    IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &Surface1, NULL);
    if (!Surface1) {
        skip("DDSCAPS_3DDEVICE surface not available\n");
        return FALSE;
    }

    hr = IDirectDrawSurface_QueryInterface(Surface1, &IID_IDirect3DRGBDevice, (void **) &Direct3DDevice1);
    ok(hr==D3D_OK || hr==DDERR_NOPALETTEATTACHED || hr==E_OUTOFMEMORY, "CreateDevice returned: %x\n", hr);
    if(!Direct3DDevice1) {
        return FALSE;
    }

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = D3DDEB_BUFSIZE | D3DDEB_CAPS;
    desc.dwCaps = D3DDEBCAPS_VIDEOMEMORY;
    desc.dwBufferSize = 128;
    desc.lpData = NULL;
    hr = IDirect3DDevice_CreateExecuteBuffer(Direct3DDevice1, &desc, &ExecuteBuffer, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice_CreateExecuteBuffer failed: %08x\n", hr);
    if(!ExecuteBuffer) {
        return FALSE;
    }

    hr = IDirect3D_CreateViewport(Direct3D1, &Viewport, NULL);
    ok(hr == D3D_OK, "IDirect3D_CreateViewport failed: %08x\n", hr);
    if(!Viewport) {
        return FALSE;
    }

    hr = IDirect3DViewport_Initialize(Viewport, Direct3D1);
    ok(hr == DDERR_ALREADYINITIALIZED, "IDirect3DViewport_Initialize returned %08x\n", hr);

    hr = IDirect3DDevice_AddViewport(Direct3DDevice1, Viewport);
    ok(hr == D3D_OK, "IDirect3DDevice_AddViewport returned %08x\n", hr);
    vp_data.dwSize = sizeof(vp_data);
    vp_data.dwX = 0;
    vp_data.dwY = 0;
    vp_data.dwWidth = 256;
    vp_data.dwHeight = 256;
    vp_data.dvScaleX = 1;
    vp_data.dvScaleY = 1;
    vp_data.dvMaxX = 256;
    vp_data.dvMaxY = 256;
    vp_data.dvMinZ = 0;
    vp_data.dvMaxZ = 1;
    hr = IDirect3DViewport_SetViewport(Viewport, &vp_data);
    ok(hr == D3D_OK, "IDirect3DViewport_SetViewport returned %08x\n", hr);

    hr = IDirect3D_CreateLight(Direct3D1, &Light, NULL);
    ok(hr == D3D_OK, "IDirect3D_CreateLight failed: %08x\n", hr);
    if (!Light)
        return FALSE;

    return TRUE;
}

static void D3D1_releaseObjects(void)
{
    if (Light) IDirect3DLight_Release(Light);
    if (Viewport) IDirect3DViewport_Release(Viewport);
    if (ExecuteBuffer) IDirect3DExecuteBuffer_Release(ExecuteBuffer);
    if (Direct3DDevice1) IDirect3DDevice_Release(Direct3DDevice1);
    if (Surface1) IDirectDrawSurface_Release(Surface1);
    if (Direct3D1) IDirect3D_Release(Direct3D1);
    if (DirectDraw1) IDirectDraw_Release(DirectDraw1);
}

static void ViewportTest(void)
{
    HRESULT hr;
    IDirect3DViewport2 *Viewport2;
    IDirect3DViewport3 *Viewport3;
    D3DVIEWPORT vp1_data, ret_vp1_data;
    D3DVIEWPORT2 vp2_data, ret_vp2_data;
    float infinity;

    *(DWORD*)&infinity = 0x7f800000;

    hr = IDirect3DDevice_AddViewport(Direct3DDevice1, Viewport);
    ok(hr == D3D_OK, "IDirect3DDevice_AddViewport returned %08x\n", hr);

    hr = IDirect3DViewport_QueryInterface(Viewport, &IID_IDirect3DViewport2, (void**) &Viewport2);
    ok(hr==D3D_OK, "QueryInterface returned: %x\n", hr);
    ok(Viewport2 == (IDirect3DViewport2 *)Viewport, "IDirect3DViewport2 iface different from IDirect3DViewport\n");

    hr = IDirect3DViewport_QueryInterface(Viewport, &IID_IDirect3DViewport3, (void**) &Viewport3);
    ok(hr==D3D_OK, "QueryInterface returned: %x\n", hr);
    ok(Viewport3 == (IDirect3DViewport3 *)Viewport, "IDirect3DViewport3 iface different from IDirect3DViewport\n");
    IDirect3DViewport3_Release(Viewport3);

    vp1_data.dwSize = sizeof(vp1_data);
    vp1_data.dwX = 0;
    vp1_data.dwY = 1;
    vp1_data.dwWidth = 256;
    vp1_data.dwHeight = 257;
    vp1_data.dvMaxX = 0;
    vp1_data.dvMaxY = 0;
    vp1_data.dvScaleX = 0;
    vp1_data.dvScaleY = 0;
    vp1_data.dvMinZ = 0.25;
    vp1_data.dvMaxZ = 0.75;

    vp2_data.dwSize = sizeof(vp2_data);
    vp2_data.dwX = 2;
    vp2_data.dwY = 3;
    vp2_data.dwWidth = 258;
    vp2_data.dwHeight = 259;
    vp2_data.dvClipX = 0;
    vp2_data.dvClipY = 0;
    vp2_data.dvClipWidth = 0;
    vp2_data.dvClipHeight = 0;
    vp2_data.dvMinZ = 0.1;
    vp2_data.dvMaxZ = 0.9;

    hr = IDirect3DViewport2_SetViewport(Viewport2, &vp1_data);
    ok(hr == D3D_OK, "IDirect3DViewport2_SetViewport returned %08x\n", hr);

    memset(&ret_vp1_data, 0xff, sizeof(ret_vp1_data));
    ret_vp1_data.dwSize = sizeof(vp1_data);

    hr = IDirect3DViewport2_GetViewport(Viewport2, &ret_vp1_data);
    ok(hr == D3D_OK, "IDirect3DViewport2_GetViewport returned %08x\n", hr);

    ok(ret_vp1_data.dwX == vp1_data.dwX, "dwX is %u, expected %u\n", ret_vp1_data.dwX, vp1_data.dwX);
    ok(ret_vp1_data.dwY == vp1_data.dwY, "dwY is %u, expected %u\n", ret_vp1_data.dwY, vp1_data.dwY);
    ok(ret_vp1_data.dwWidth == vp1_data.dwWidth, "dwWidth is %u, expected %u\n", ret_vp1_data.dwWidth, vp1_data.dwWidth);
    ok(ret_vp1_data.dwHeight == vp1_data.dwHeight, "dwHeight is %u, expected %u\n", ret_vp1_data.dwHeight, vp1_data.dwHeight);
    ok(ret_vp1_data.dvMaxX == vp1_data.dvMaxX, "dvMaxX is %f, expected %f\n", ret_vp1_data.dvMaxX, vp1_data.dvMaxX);
    ok(ret_vp1_data.dvMaxY == vp1_data.dvMaxY, "dvMaxY is %f, expected %f\n", ret_vp1_data.dvMaxY, vp1_data.dvMaxY);
    todo_wine ok(ret_vp1_data.dvScaleX == infinity, "dvScaleX is %f, expected %f\n", ret_vp1_data.dvScaleX, infinity);
    todo_wine ok(ret_vp1_data.dvScaleY == infinity, "dvScaleY is %f, expected %f\n", ret_vp1_data.dvScaleY, infinity);
    ok(ret_vp1_data.dvMinZ == 0.0, "dvMinZ is %f, expected 0.0\n", ret_vp1_data.dvMinZ);
    ok(ret_vp1_data.dvMaxZ == 1.0, "dvMaxZ is %f, expected 1.0\n", ret_vp1_data.dvMaxZ);

    hr = IDirect3DViewport2_SetViewport2(Viewport2, &vp2_data);
    ok(hr == D3D_OK, "IDirect3DViewport2_SetViewport2 returned %08x\n", hr);

    memset(&ret_vp2_data, 0xff, sizeof(ret_vp2_data));
    ret_vp2_data.dwSize = sizeof(vp2_data);

    hr = IDirect3DViewport2_GetViewport2(Viewport2, &ret_vp2_data);
    ok(hr == D3D_OK, "IDirect3DViewport2_GetViewport2 returned %08x\n", hr);

    ok(ret_vp2_data.dwX == vp2_data.dwX, "dwX is %u, expected %u\n", ret_vp2_data.dwX, vp2_data.dwX);
    ok(ret_vp2_data.dwY == vp2_data.dwY, "dwY is %u, expected %u\n", ret_vp2_data.dwY, vp2_data.dwY);
    ok(ret_vp2_data.dwWidth == vp2_data.dwWidth, "dwWidth is %u, expected %u\n", ret_vp2_data.dwWidth, vp2_data.dwWidth);
    ok(ret_vp2_data.dwHeight == vp2_data.dwHeight, "dwHeight is %u, expected %u\n", ret_vp2_data.dwHeight, vp2_data.dwHeight);
    ok(ret_vp2_data.dvClipX == vp2_data.dvClipX, "dvClipX is %f, expected %f\n", ret_vp2_data.dvClipX, vp2_data.dvClipX);
    ok(ret_vp2_data.dvClipY == vp2_data.dvClipY, "dvClipY is %f, expected %f\n", ret_vp2_data.dvClipY, vp2_data.dvClipY);
    ok(ret_vp2_data.dvClipWidth == vp2_data.dvClipWidth, "dvClipWidth is %f, expected %f\n",
        ret_vp2_data.dvClipWidth, vp2_data.dvClipWidth);
    ok(ret_vp2_data.dvClipHeight == vp2_data.dvClipHeight, "dvClipHeight is %f, expected %f\n",
        ret_vp2_data.dvClipHeight, vp2_data.dvClipHeight);
    ok(ret_vp2_data.dvMinZ == vp2_data.dvMinZ, "dvMinZ is %f, expected %f\n", ret_vp2_data.dvMinZ, vp2_data.dvMinZ);
    ok(ret_vp2_data.dvMaxZ == vp2_data.dvMaxZ, "dvMaxZ is %f, expected %f\n", ret_vp2_data.dvMaxZ, vp2_data.dvMaxZ);

    memset(&ret_vp1_data, 0xff, sizeof(ret_vp1_data));
    ret_vp1_data.dwSize = sizeof(vp1_data);

    hr = IDirect3DViewport2_GetViewport(Viewport2, &ret_vp1_data);
    ok(hr == D3D_OK, "IDirect3DViewport2_GetViewport returned %08x\n", hr);

    ok(ret_vp1_data.dwX == vp2_data.dwX, "dwX is %u, expected %u\n", ret_vp1_data.dwX, vp2_data.dwX);
    ok(ret_vp1_data.dwY == vp2_data.dwY, "dwY is %u, expected %u\n", ret_vp1_data.dwY, vp2_data.dwY);
    ok(ret_vp1_data.dwWidth == vp2_data.dwWidth, "dwWidth is %u, expected %u\n", ret_vp1_data.dwWidth, vp2_data.dwWidth);
    ok(ret_vp1_data.dwHeight == vp2_data.dwHeight, "dwHeight is %u, expected %u\n", ret_vp1_data.dwHeight, vp2_data.dwHeight);
    ok(ret_vp1_data.dvMaxX == vp1_data.dvMaxX, "dvMaxX is %f, expected %f\n", ret_vp1_data.dvMaxX, vp1_data.dvMaxX);
    ok(ret_vp1_data.dvMaxY == vp1_data.dvMaxY, "dvMaxY is %f, expected %f\n", ret_vp1_data.dvMaxY, vp1_data.dvMaxY);
    todo_wine ok(ret_vp1_data.dvScaleX == infinity, "dvScaleX is %f, expected %f\n", ret_vp1_data.dvScaleX, infinity);
    todo_wine ok(ret_vp1_data.dvScaleY == infinity, "dvScaleY is %f, expected %f\n", ret_vp1_data.dvScaleY, infinity);
    todo_wine ok(ret_vp1_data.dvMinZ == 0.0, "dvMinZ is %f, expected 0.0\n", ret_vp1_data.dvMinZ);
    todo_wine ok(ret_vp1_data.dvMaxZ == 1.0, "dvMaxZ is %f, expected 1.0\n", ret_vp1_data.dvMaxZ);

    hr = IDirect3DViewport2_SetViewport2(Viewport2, &vp2_data);
    ok(hr == D3D_OK, "IDirect3DViewport2_SetViewport2 returned %08x\n", hr);

    memset(&ret_vp2_data, 0xff, sizeof(ret_vp2_data));
    ret_vp2_data.dwSize = sizeof(vp2_data);

    hr = IDirect3DViewport2_GetViewport2(Viewport2, &ret_vp2_data);
    ok(hr == D3D_OK, "IDirect3DViewport2_GetViewport2 returned %08x\n", hr);

    ok(ret_vp2_data.dwX == vp2_data.dwX, "dwX is %u, expected %u\n", ret_vp2_data.dwX, vp2_data.dwX);
    ok(ret_vp2_data.dwY == vp2_data.dwY, "dwY is %u, expected %u\n", ret_vp2_data.dwY, vp2_data.dwY);
    ok(ret_vp2_data.dwWidth == vp2_data.dwWidth, "dwWidth is %u, expected %u\n", ret_vp2_data.dwWidth, vp2_data.dwWidth);
    ok(ret_vp2_data.dwHeight == vp2_data.dwHeight, "dwHeight is %u, expected %u\n", ret_vp2_data.dwHeight, vp2_data.dwHeight);
    ok(ret_vp2_data.dvClipX == vp2_data.dvClipX, "dvClipX is %f, expected %f\n", ret_vp2_data.dvClipX, vp2_data.dvClipX);
    ok(ret_vp2_data.dvClipY == vp2_data.dvClipY, "dvClipY is %f, expected %f\n", ret_vp2_data.dvClipY, vp2_data.dvClipY);
    ok(ret_vp2_data.dvClipWidth == vp2_data.dvClipWidth, "dvClipWidth is %f, expected %f\n",
        ret_vp2_data.dvClipWidth, vp2_data.dvClipWidth);
    ok(ret_vp2_data.dvClipHeight == vp2_data.dvClipHeight, "dvClipHeight is %f, expected %f\n",
        ret_vp2_data.dvClipHeight, vp2_data.dvClipHeight);
    ok(ret_vp2_data.dvMinZ == vp2_data.dvMinZ, "dvMinZ is %f, expected %f\n", ret_vp2_data.dvMinZ, vp2_data.dvMinZ);
    ok(ret_vp2_data.dvMaxZ == vp2_data.dvMaxZ, "dvMaxZ is %f, expected %f\n", ret_vp2_data.dvMaxZ, vp2_data.dvMaxZ);

    hr = IDirect3DViewport2_SetViewport(Viewport2, &vp1_data);
    ok(hr == D3D_OK, "IDirect3DViewport2_SetViewport returned %08x\n", hr);

    memset(&ret_vp1_data, 0xff, sizeof(ret_vp1_data));
    ret_vp1_data.dwSize = sizeof(vp1_data);

    hr = IDirect3DViewport2_GetViewport(Viewport2, &ret_vp1_data);
    ok(hr == D3D_OK, "IDirect3DViewport2_GetViewport returned %08x\n", hr);

    ok(ret_vp1_data.dwX == vp1_data.dwX, "dwX is %u, expected %u\n", ret_vp1_data.dwX, vp1_data.dwX);
    ok(ret_vp1_data.dwY == vp1_data.dwY, "dwY is %u, expected %u\n", ret_vp1_data.dwY, vp1_data.dwY);
    ok(ret_vp1_data.dwWidth == vp1_data.dwWidth, "dwWidth is %u, expected %u\n", ret_vp1_data.dwWidth, vp1_data.dwWidth);
    ok(ret_vp1_data.dwHeight == vp1_data.dwHeight, "dwHeight is %u, expected %u\n", ret_vp1_data.dwHeight, vp1_data.dwHeight);
    ok(ret_vp1_data.dvMaxX == vp1_data.dvMaxX, "dvMaxX is %f, expected %f\n", ret_vp1_data.dvMaxX, vp1_data.dvMaxX);
    ok(ret_vp1_data.dvMaxY == vp1_data.dvMaxY, "dvMaxY is %f, expected %f\n", ret_vp1_data.dvMaxY, vp1_data.dvMaxY);
    todo_wine ok(ret_vp1_data.dvScaleX == infinity, "dvScaleX is %f, expected %f\n", ret_vp1_data.dvScaleX, infinity);
    todo_wine ok(ret_vp1_data.dvScaleY == infinity, "dvScaleY is %f, expected %f\n", ret_vp1_data.dvScaleY, infinity);
    ok(ret_vp1_data.dvMinZ == 0.0, "dvMinZ is %f, expected 0.0\n", ret_vp1_data.dvMinZ);
    ok(ret_vp1_data.dvMaxZ == 1.0, "dvMaxZ is %f, expected 1.0\n", ret_vp1_data.dvMaxZ);

    memset(&ret_vp2_data, 0xff, sizeof(ret_vp2_data));
    ret_vp2_data.dwSize = sizeof(vp2_data);

    hr = IDirect3DViewport2_GetViewport2(Viewport2, &ret_vp2_data);
    ok(hr == D3D_OK, "IDirect3DViewport2_GetViewport2 returned %08x\n", hr);

    ok(ret_vp2_data.dwX == vp1_data.dwX, "dwX is %u, expected %u\n", ret_vp2_data.dwX, vp1_data.dwX);
    ok(ret_vp2_data.dwY == vp1_data.dwY, "dwY is %u, expected %u\n", ret_vp2_data.dwY, vp1_data.dwY);
    ok(ret_vp2_data.dwWidth == vp1_data.dwWidth, "dwWidth is %u, expected %u\n", ret_vp2_data.dwWidth, vp1_data.dwWidth);
    ok(ret_vp2_data.dwHeight == vp1_data.dwHeight, "dwHeight is %u, expected %u\n", ret_vp2_data.dwHeight, vp1_data.dwHeight);
    ok(ret_vp2_data.dvClipX == vp2_data.dvClipX, "dvClipX is %f, expected %f\n", ret_vp2_data.dvClipX, vp2_data.dvClipX);
    ok(ret_vp2_data.dvClipY == vp2_data.dvClipY, "dvClipY is %f, expected %f\n", ret_vp2_data.dvClipY, vp2_data.dvClipY);
    ok(ret_vp2_data.dvClipWidth == vp2_data.dvClipWidth, "dvClipWidth is %f, expected %f\n",
        ret_vp2_data.dvClipWidth, vp2_data.dvClipWidth);
    ok(ret_vp2_data.dvClipHeight == vp2_data.dvClipHeight, "dvClipHeight is %f, expected %f\n",
        ret_vp2_data.dvClipHeight, vp2_data.dvClipHeight);
    ok(ret_vp2_data.dvMinZ == 0.0, "dvMinZ is %f, expected 0.0\n", ret_vp2_data.dvMinZ);
    ok(ret_vp2_data.dvMaxZ == 1.0, "dvMaxZ is %f, expected 1.0\n", ret_vp2_data.dvMaxZ);

    IDirect3DViewport2_Release(Viewport2);

    hr = IDirect3DDevice_DeleteViewport(Direct3DDevice1, Viewport);
    ok(hr == D3D_OK, "IDirect3DDevice_DeleteViewport returned %08x\n", hr);
}

static void Direct3D1Test(void)
{
    HRESULT hr;
    D3DEXECUTEBUFFERDESC desc;
    D3DINSTRUCTION *instr;
    D3DBRANCH *branch;
    IDirect3D *Direct3D_alt;
    IDirect3DLight *d3dlight;
    ULONG refcount;
    unsigned int idx = 0;

    /* Interface consistency check. */
    hr = IDirect3DDevice_GetDirect3D(Direct3DDevice1, &Direct3D_alt);
    ok(hr == D3D_OK, "IDirect3DDevice_GetDirect3D failed: %08x\n", hr);
    ok(Direct3D_alt == Direct3D1, "Direct3D1 struct pointer mismatch: %p != %p\n", Direct3D_alt, Direct3D1);
    IDirect3D_Release(Direct3D_alt);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    hr = IDirect3DExecuteBuffer_Lock(ExecuteBuffer, &desc);
    ok(hr == D3D_OK, "IDirect3DExecuteBuffer_Lock failed: %08x\n", hr);

    memset(desc.lpData, 0, 128);
    instr = desc.lpData;
    instr[idx].bOpcode = D3DOP_BRANCHFORWARD;
    instr[idx].bSize = sizeof(*branch);
    instr[idx].wCount = 1;
    idx++;
    branch = (D3DBRANCH *) &instr[idx];
    branch->dwMask = 0x0;
    branch->dwValue = 1;
    branch->bNegate = TRUE;
    branch->dwOffset = 0;
    idx += (sizeof(*branch) / sizeof(*instr));
    instr[idx].bOpcode = D3DOP_EXIT;
    instr[idx].bSize = 0;
    instr[idx].wCount = 0;
    hr = IDirect3DExecuteBuffer_Unlock(ExecuteBuffer);
    ok(hr == D3D_OK, "IDirect3DExecuteBuffer_Unlock failed: %08x\n", hr);

    hr = IDirect3DDevice_Execute(Direct3DDevice1, ExecuteBuffer, Viewport, D3DEXECUTE_CLIPPED);
    ok(hr == D3D_OK, "IDirect3DDevice_Execute returned %08x\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);

    hr = IDirect3DExecuteBuffer_Lock(ExecuteBuffer, &desc);
    ok(hr == D3D_OK, "IDirect3DExecuteBuffer_Lock failed: %08x\n", hr);

    memset(desc.lpData, 0, 128);
    instr = desc.lpData;
    idx = 0;
    instr[idx].bOpcode = D3DOP_BRANCHFORWARD;
    instr[idx].bSize = sizeof(*branch);
    instr[idx].wCount = 1;
    idx++;
    branch = (D3DBRANCH *) &instr[idx];
    branch->dwMask = 0x0;
    branch->dwValue = 1;
    branch->bNegate = TRUE;
    branch->dwOffset = 64;
    instr = (D3DINSTRUCTION*)((char*)desc.lpData + 64);
    instr[0].bOpcode = D3DOP_EXIT;
    instr[0].bSize = 0;
    instr[0].wCount = 0;
    hr = IDirect3DExecuteBuffer_Unlock(ExecuteBuffer);
    ok(hr == D3D_OK, "IDirect3DExecuteBuffer_Unlock failed: %08x\n", hr);

    hr = IDirect3DDevice_Execute(Direct3DDevice1, ExecuteBuffer, Viewport, D3DEXECUTE_CLIPPED);
    ok(hr == D3D_OK, "IDirect3DDevice_Execute returned %08x\n", hr);

    /* Test rendering 0 triangles */
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);

    hr = IDirect3DExecuteBuffer_Lock(ExecuteBuffer, &desc);
    ok(hr == D3D_OK, "IDirect3DExecuteBuffer_Lock failed: %08x\n", hr);

    memset(desc.lpData, 0, 128);
    instr = desc.lpData;

    instr->bOpcode = D3DOP_TRIANGLE;
    instr->bSize = sizeof(D3DOP_TRIANGLE);
    instr->wCount = 0;
    instr++;
    instr->bOpcode = D3DOP_EXIT;
    instr->bSize = 0;
    instr->wCount = 0;
    hr = IDirect3DExecuteBuffer_Unlock(ExecuteBuffer);
    ok(hr == D3D_OK, "IDirect3DExecuteBuffer_Unlock failed: %08x\n", hr);

    hr = IDirect3DDevice_Execute(Direct3DDevice1, ExecuteBuffer, Viewport, D3DEXECUTE_CLIPPED);
    ok(hr == D3D_OK, "IDirect3DDevice_Execute returned %08x\n", hr);

    hr = IDirect3DDevice_DeleteViewport(Direct3DDevice1, Viewport);
    ok(hr == D3D_OK, "IDirect3DDevice_DeleteViewport returned %08x\n", hr);

    hr = IDirect3DViewport_AddLight(Viewport, Light);
    ok(hr == D3D_OK, "IDirect3DViewport_AddLight returned %08x\n", hr);
    refcount = getRefcount((IUnknown*) Light);
    ok(refcount == 2, "Refcount should be 2, returned is %d\n", refcount);

    hr = IDirect3DViewport_NextLight(Viewport, NULL, &d3dlight, D3DNEXT_HEAD);
    ok(hr == D3D_OK, "IDirect3DViewport_AddLight returned %08x\n", hr);
    ok(d3dlight == Light, "Got different light returned %p, expected %p\n", d3dlight, Light);
    refcount = getRefcount((IUnknown*) Light);
    ok(refcount == 3, "Refcount should be 2, returned is %d\n", refcount);

    hr = IDirect3DViewport_DeleteLight(Viewport, Light);
    ok(hr == D3D_OK, "IDirect3DViewport_DeleteLight returned %08x\n", hr);
    refcount = getRefcount((IUnknown*) Light);
    ok(refcount == 2, "Refcount should be 2, returned is %d\n", refcount);

    IDirect3DLight_Release(Light);
}

static BOOL colortables_check_equality(PALETTEENTRY table1[256], PALETTEENTRY table2[256])
{
    int i;

    for (i = 0; i < 256; i++) {
       if (table1[i].peRed != table2[i].peRed || table1[i].peGreen != table2[i].peGreen ||
           table1[i].peBlue != table2[i].peBlue) return FALSE;
    }

    return TRUE;
}

/* test palette handling in IDirect3DTexture_Load */
static void TextureLoadTest(void)
{
    IDirectDrawSurface *TexSurface = NULL;
    IDirect3DTexture *Texture = NULL;
    IDirectDrawSurface *TexSurface2 = NULL;
    IDirect3DTexture *Texture2 = NULL;
    IDirectDrawPalette *palette = NULL;
    IDirectDrawPalette *palette2 = NULL;
    IDirectDrawPalette *palette_tmp = NULL;
    PALETTEENTRY table1[256], table2[256], table_tmp[256];
    HRESULT hr;
    DDSURFACEDESC ddsd;
    int i;

    memset (&ddsd, 0, sizeof (ddsd));
    ddsd.dwSize = sizeof (ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.dwHeight = 128;
    ddsd.dwWidth = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
    U1(ddsd.ddpfPixelFormat).dwRGBBitCount = 8;

    hr = IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &TexSurface, NULL);
    ok(hr==D3D_OK, "CreateSurface returned: %x\n", hr);
    if (FAILED(hr)) {
        skip("IDirectDraw_CreateSurface failed; skipping further tests\n");
        goto cleanup;
    }

    hr = IDirectDrawSurface_QueryInterface(TexSurface, &IID_IDirect3DTexture,
                (void *)&Texture);
    ok(hr==D3D_OK, "IDirectDrawSurface_QueryInterface returned: %x\n", hr);
    if (FAILED(hr)) {
        skip("Can't get IDirect3DTexture interface; skipping further tests\n");
        goto cleanup;
    }

    hr = IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &TexSurface2, NULL);
    ok(hr==D3D_OK, "CreateSurface returned: %x\n", hr);
    if (FAILED(hr)) {
        skip("IDirectDraw_CreateSurface failed; skipping further tests\n");
        goto cleanup;
    }

    hr = IDirectDrawSurface_QueryInterface(TexSurface2, &IID_IDirect3DTexture,
                (void *)&Texture2);
    ok(hr==D3D_OK, "IDirectDrawSurface_QueryInterface returned: %x\n", hr);
    if (FAILED(hr)) {
        skip("Can't get IDirect3DTexture interface; skipping further tests\n");
        goto cleanup;
    }

    /* test load of Texture to Texture */
    hr = IDirect3DTexture_Load(Texture, Texture);
    ok(hr == DD_OK, "IDirect3DTexture_Load returned %08x\n", hr);

    /* test Load when both textures have no palette */
    hr = IDirect3DTexture_Load(Texture2, Texture);
    ok(hr == DD_OK, "IDirect3DTexture_Load returned %08x\n", hr);

    for (i = 0; i < 256; i++) {
        table1[i].peRed = i;
        table1[i].peGreen = i;
        table1[i].peBlue = i;
        table1[i].peFlags = 0;
    }

    hr = IDirectDraw_CreatePalette(DirectDraw1, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, table1, &palette, NULL);
    ok(hr == DD_OK, "CreatePalette returned %08x\n", hr);
    if (FAILED(hr)) {
        skip("IDirectDraw_CreatePalette failed; skipping further tests\n");
        goto cleanup;
    }

    /* test Load when source texture has palette and destination has no palette */
    hr = IDirectDrawSurface_SetPalette(TexSurface, palette);
    ok(hr == DD_OK, "IDirectDrawSurface_SetPalette returned %08x\n", hr);
    hr = IDirect3DTexture_Load(Texture2, Texture);
    ok(hr == DDERR_NOPALETTEATTACHED, "IDirect3DTexture_Load returned %08x\n", hr);

    for (i = 0; i < 256; i++) {
        table2[i].peRed = 255 - i;
        table2[i].peGreen = 255 - i;
        table2[i].peBlue = 255 - i;
        table2[i].peFlags = 0;
    }

    hr = IDirectDraw_CreatePalette(DirectDraw1, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, table2, &palette2, NULL);
    ok(hr == DD_OK, "CreatePalette returned %08x\n", hr);
    if (FAILED(hr)) {
        skip("IDirectDraw_CreatePalette failed; skipping further tests\n");
        goto cleanup;
    }

    /* test Load when source has no palette and destination has a palette */
    hr = IDirectDrawSurface_SetPalette(TexSurface, NULL);
    ok(hr == DD_OK, "IDirectDrawSurface_SetPalette returned %08x\n", hr);
    hr = IDirectDrawSurface_SetPalette(TexSurface2, palette2);
    ok(hr == DD_OK, "IDirectDrawSurface_SetPalette returned %08x\n", hr);
    hr = IDirect3DTexture_Load(Texture2, Texture);
    ok(hr == DD_OK, "IDirect3DTexture_Load returned %08x\n", hr);
    hr = IDirectDrawSurface_GetPalette(TexSurface2, &palette_tmp);
    ok(hr == DD_OK, "IDirectDrawSurface_GetPalette returned %08x\n", hr);
    if (!palette_tmp) {
        skip("IDirectDrawSurface_GetPalette failed; skipping color table check\n");
        goto cleanup;
    } else {
        hr = IDirectDrawPalette_GetEntries(palette_tmp, 0, 0, 256, table_tmp);
        ok(hr == DD_OK, "IDirectDrawPalette_GetEntries returned %08x\n", hr);
        ok(colortables_check_equality(table2, table_tmp), "Unexpected palettized texture color table\n");
        IDirectDrawPalette_Release(palette_tmp);
    }

    /* test Load when both textures have palettes */
    hr = IDirectDrawSurface_SetPalette(TexSurface, palette);
    ok(hr == DD_OK, "IDirectDrawSurface_SetPalette returned %08x\n", hr);
    hr = IDirect3DTexture_Load(Texture2, Texture);
    ok(hr == DD_OK, "IDirect3DTexture_Load returned %08x\n", hr);
    hr = IDirect3DTexture_Load(Texture2, Texture);
    ok(hr == DD_OK, "IDirect3DTexture_Load returned %08x\n", hr);
    hr = IDirectDrawSurface_GetPalette(TexSurface2, &palette_tmp);
    ok(hr == DD_OK, "IDirectDrawSurface_GetPalette returned %08x\n", hr);
    if (!palette_tmp) {
        skip("IDirectDrawSurface_GetPalette failed; skipping color table check\n");
        goto cleanup;
    } else {
        hr = IDirectDrawPalette_GetEntries(palette_tmp, 0, 0, 256, table_tmp);
        ok(hr == DD_OK, "IDirectDrawPalette_GetEntries returned %08x\n", hr);
        ok(colortables_check_equality(table1, table_tmp), "Unexpected palettized texture color table\n");
        IDirectDrawPalette_Release(palette_tmp);
    }

    cleanup:

    if (palette) IDirectDrawPalette_Release(palette);
    if (palette2) IDirectDrawPalette_Release(palette2);
    if (TexSurface) IDirectDrawSurface_Release(TexSurface);
    if (Texture) IDirect3DTexture_Release(Texture);
    if (TexSurface2) IDirectDrawSurface_Release(TexSurface2);
    if (Texture2) IDirect3DTexture_Release(Texture2);
}

static void VertexBufferDescTest(void)
{
    HRESULT rc;
    D3DVERTEXBUFFERDESC desc;
    union mem_t
    {
        D3DVERTEXBUFFERDESC desc2;
        unsigned char buffer[512];
    } mem;

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwCaps = 0;
    desc.dwFVF = D3DFVF_XYZ;
    desc.dwNumVertices = 1;
    rc = IDirect3D7_CreateVertexBuffer(lpD3D, &desc, &lpVBufSrc, 0);
    ok(rc==D3D_OK || rc==E_OUTOFMEMORY, "CreateVertexBuffer returned: %x\n", rc);
    if (!lpVBufSrc)
    {
        trace("IDirect3D7::CreateVertexBuffer() failed with an error %x\n", rc);
        goto out;
    }

    memset(mem.buffer, 0x12, sizeof(mem.buffer));
    mem.desc2.dwSize = sizeof(D3DVERTEXBUFFERDESC)*2;
    rc = IDirect3DVertexBuffer7_GetVertexBufferDesc(lpVBufSrc, &mem.desc2);
    if(rc != D3D_OK)
        skip("GetVertexBuffer Failed!\n");
    ok( mem.desc2.dwSize == sizeof(D3DVERTEXBUFFERDESC)*2, "Size returned from GetVertexBufferDesc does not match the value put in\n" );
    ok( mem.buffer[sizeof(D3DVERTEXBUFFERDESC)] == 0x12, "GetVertexBufferDesc cleared outside of the struct! (dwSize was double the size of the struct)\n");
    ok( mem.desc2.dwCaps == desc.dwCaps, "dwCaps returned differs. Got %x, expected %x\n", mem.desc2.dwCaps, desc.dwCaps);
    ok( mem.desc2.dwFVF == desc.dwFVF, "dwFVF returned differs. Got %x, expected %x\n", mem.desc2.dwFVF, desc.dwFVF);
    ok (mem.desc2.dwNumVertices == desc.dwNumVertices, "dwNumVertices returned differs. Got %x, expected %x\n", mem.desc2.dwNumVertices, desc.dwNumVertices);

    memset(mem.buffer, 0x12, sizeof(mem.buffer));
    mem.desc2.dwSize = 0;
    rc = IDirect3DVertexBuffer7_GetVertexBufferDesc(lpVBufSrc, &mem.desc2);
    if(rc != D3D_OK)
        skip("GetVertexBuffer Failed!\n");
    ok( mem.desc2.dwSize == 0, "Size returned from GetVertexBufferDesc does not match the value put in\n" );
    ok( mem.buffer[sizeof(D3DVERTEXBUFFERDESC)] == 0x12, "GetVertexBufferDesc cleared outside of the struct! (dwSize was 0)\n");
    ok( mem.desc2.dwCaps == desc.dwCaps, "dwCaps returned differs. Got %x, expected %x\n", mem.desc2.dwCaps, desc.dwCaps);
    ok( mem.desc2.dwFVF == desc.dwFVF, "dwFVF returned differs. Got %x, expected %x\n", mem.desc2.dwFVF, desc.dwFVF);
    ok (mem.desc2.dwNumVertices == desc.dwNumVertices, "dwNumVertices returned differs. Got %x, expected %x\n", mem.desc2.dwNumVertices, desc.dwNumVertices);

    memset(mem.buffer, 0x12, sizeof(mem.buffer));
    mem.desc2.dwSize = sizeof(D3DVERTEXBUFFERDESC);
    rc = IDirect3DVertexBuffer7_GetVertexBufferDesc(lpVBufSrc, &mem.desc2);
    if(rc != D3D_OK)
        skip("GetVertexBuffer Failed!\n");
    ok( mem.desc2.dwSize == sizeof(D3DVERTEXBUFFERDESC), "Size returned from GetVertexBufferDesc does not match the value put in\n" );
    ok( mem.buffer[sizeof(D3DVERTEXBUFFERDESC)] == 0x12, "GetVertexBufferDesc cleared outside of the struct! (dwSize was the size of the struct)\n");
    ok( mem.desc2.dwCaps == desc.dwCaps, "dwCaps returned differs. Got %x, expected %x\n", mem.desc2.dwCaps, desc.dwCaps);
    ok( mem.desc2.dwFVF == desc.dwFVF, "dwFVF returned differs. Got %x, expected %x\n", mem.desc2.dwFVF, desc.dwFVF);
    ok (mem.desc2.dwNumVertices == desc.dwNumVertices, "dwNumVertices returned differs. Got %x, expected %x\n", mem.desc2.dwNumVertices, desc.dwNumVertices);

out:
    IDirect3DVertexBuffer7_Release(lpVBufSrc);
}

#define IS_VALUE_NEAR(a, b)    ( ((a) == (b)) || ((a) == (b) - 1) || ((a) == (b) + 1) )
#define MIN(a, b)    ((a) < (b) ? (a) : (b))

static void DeviceLoadTest(void)
{
    DDSURFACEDESC2 ddsd;
    IDirectDrawSurface7 *texture_levels[2][8];
    IDirectDrawSurface7 *cube_face_levels[2][6][8];
    DWORD flags;
    HRESULT hr;
    DDBLTFX ddbltfx;
    RECT loadrect;
    POINT loadpoint;
    int i, i1, i2;
    unsigned diff_count = 0, diff_count2 = 0;
    unsigned x, y;
    BOOL load_mip_subset_broken = FALSE;
    IDirectDrawPalette *palettes[5];
    PALETTEENTRY table1[256];
    DDCOLORKEY ddckey;
    D3DDEVICEDESC7 d3dcaps;

    /* Test loading of texture subrectangle with a mipmap surface. */
    memset(texture_levels, 0, sizeof(texture_levels));
    memset(cube_face_levels, 0, sizeof(cube_face_levels));
    memset(palettes, 0, sizeof(palettes));

    for (i = 0; i < 2; i++)
    {
        memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
        ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
        ddsd.dwWidth = 128;
        ddsd.dwHeight = 128;
        U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
        U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB;
        U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 32;
        U2(U4(ddsd).ddpfPixelFormat).dwRBitMask = 0x00FF0000;
        U3(U4(ddsd).ddpfPixelFormat).dwGBitMask = 0x0000FF00;
        U4(U4(ddsd).ddpfPixelFormat).dwBBitMask = 0x000000FF;
        hr = IDirectDraw7_CreateSurface(lpDD, &ddsd, &texture_levels[i][0], NULL);
        ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);
        if (FAILED(hr)) goto out;

        /* Check the number of created mipmaps */
        memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
        ddsd.dwSize = sizeof(ddsd);
        hr = IDirectDrawSurface7_GetSurfaceDesc(texture_levels[i][0], &ddsd);
        ok(hr==DD_OK,"IDirectDrawSurface7_GetSurfaceDesc returned: %x\n",hr);
        ok(U2(ddsd).dwMipMapCount == 8, "unexpected mip count %u\n", U2(ddsd).dwMipMapCount);
        if (U2(ddsd).dwMipMapCount != 8) goto out;

        for (i1 = 1; i1 < 8; i1++)
        {
            hr = IDirectDrawSurface7_GetAttachedSurface(texture_levels[i][i1 - 1], &ddsd.ddsCaps, &texture_levels[i][i1]);
            ok(hr == DD_OK, "GetAttachedSurface returned %08x\n", hr);
            if (FAILED(hr)) goto out;
        }
    }

    for (i1 = 0; i1 < 8; i1++)
    {
        memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
        ddsd.dwSize = sizeof(ddsd);
        hr = IDirectDrawSurface7_Lock(texture_levels[0][i1], NULL, &ddsd, DDLOCK_WAIT, NULL);
        ok(hr==DD_OK, "IDirectDrawSurface7_Lock returned: %x\n",hr);
        if (FAILED(hr)) goto out;

        for (y = 0 ; y < ddsd.dwHeight; y++)
        {
            DWORD *textureRow = (DWORD*)((char*)ddsd.lpSurface + y * U1(ddsd).lPitch);

            for (x = 0; x < ddsd.dwWidth;  x++)
            {
                /* x stored in green component, y in blue. */
                DWORD color = 0xff0000 | (x << 8)  | y;
                *textureRow++ = color;
            }
        }

        hr = IDirectDrawSurface7_Unlock(texture_levels[0][i1], NULL);
        ok(hr==DD_OK, "IDirectDrawSurface7_Unlock returned: %x\n",hr);
    }

    for (i1 = 0; i1 < 8; i1++)
    {
        memset(&ddbltfx, 0, sizeof(ddbltfx));
        ddbltfx.dwSize = sizeof(ddbltfx);
        U5(ddbltfx).dwFillColor = 0;
        hr = IDirectDrawSurface7_Blt(texture_levels[1][i1], NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
        ok(hr == DD_OK, "IDirectDrawSurface7_Blt failed with %08x\n", hr);
    }

    /* First test some broken coordinates. */
    loadpoint.x = loadpoint.y = 0;
    SetRectEmpty(&loadrect);
    hr = IDirect3DDevice7_Load(lpD3DDevice, texture_levels[1][0], &loadpoint, texture_levels[0][0], &loadrect, 0);
    ok(hr==DDERR_INVALIDPARAMS, "IDirect3DDevice7_Load returned: %x\n",hr);

    loadpoint.x = loadpoint.y = 50;
    loadrect.left = 0;
    loadrect.top = 0;
    loadrect.right = 100;
    loadrect.bottom = 100;
    hr = IDirect3DDevice7_Load(lpD3DDevice, texture_levels[1][0], &loadpoint, texture_levels[0][0], &loadrect, 0);
    ok(hr==DDERR_INVALIDPARAMS, "IDirect3DDevice7_Load returned: %x\n",hr);

    /* Test actual loading. */
    loadpoint.x = loadpoint.y = 31;
    loadrect.left = 30;
    loadrect.top = 20;
    loadrect.right = 93;
    loadrect.bottom = 52;

    hr = IDirect3DDevice7_Load(lpD3DDevice, texture_levels[1][0], &loadpoint, texture_levels[0][0], &loadrect, 0);
    ok(hr==D3D_OK, "IDirect3DDevice7_Load returned: %x\n",hr);

    for (i1 = 0; i1 < 8; i1++)
    {
        diff_count = 0;
        diff_count2 = 0;

        memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
        ddsd.dwSize = sizeof(ddsd);
        hr = IDirectDrawSurface7_Lock(texture_levels[1][i1], NULL, &ddsd, DDLOCK_WAIT, NULL);
        ok(hr==DD_OK, "IDirectDrawSurface7_Lock returned: %x\n",hr);
        if (FAILED(hr)) goto out;

        for (y = 0 ; y < ddsd.dwHeight; y++)
        {
            DWORD *textureRow = (DWORD*)((char*)ddsd.lpSurface + y * U1(ddsd).lPitch);

            for (x = 0; x < ddsd.dwWidth;  x++)
            {
                DWORD color = *textureRow++;

                if (x < loadpoint.x || x >= loadpoint.x + loadrect.right - loadrect.left ||
                    y < loadpoint.y || y >= loadpoint.y + loadrect.bottom - loadrect.top)
                {
                    if (color & 0xffffff) diff_count++;
                }
                else
                {
                    DWORD r = (color & 0xff0000) >> 16;
                    DWORD g = (color & 0xff00) >> 8;
                    DWORD b = (color & 0xff);

                    if (r != 0xff || g != x + loadrect.left - loadpoint.x || b != y + loadrect.top - loadpoint.y) diff_count++;
                }

                /* This codepath is for software RGB device. It has what looks like some weird off by one errors, but may
                   technically be correct as it's not precisely defined by docs. */
                if (x < loadpoint.x || x >= loadpoint.x + loadrect.right - loadrect.left ||
                    y < loadpoint.y || y >= loadpoint.y + loadrect.bottom - loadrect.top + 1)
                {
                    if (color & 0xffffff) diff_count2++;
                }
                else
                {
                    DWORD r = (color & 0xff0000) >> 16;
                    DWORD g = (color & 0xff00) >> 8;
                    DWORD b = (color & 0xff);

                    if (r != 0xff || !IS_VALUE_NEAR(g, x + loadrect.left - loadpoint.x) ||
                        !IS_VALUE_NEAR(b, y + loadrect.top - loadpoint.y)) diff_count2++;
                }
            }
        }

        hr = IDirectDrawSurface7_Unlock(texture_levels[1][i1], NULL);
        ok(hr==DD_OK, "IDirectDrawSurface7_Unlock returned: %x\n",hr);

        ok(diff_count == 0 || diff_count2 == 0, "Unexpected destination texture level pixels; %u differences at %d level\n",
                MIN(diff_count, diff_count2), i1);

        loadpoint.x /= 2;
        loadpoint.y /= 2;
        loadrect.top /= 2;
        loadrect.left /= 2;
        loadrect.right = (loadrect.right + 1) / 2;
        loadrect.bottom = (loadrect.bottom + 1) / 2;
    }

    /* This crashes on native (tested on real windows XP / directx9 / nvidia and
     * qemu Win98 / directx7 / RGB software rasterizer):
     * passing non toplevel surfaces (sublevels) to Load (DX7 docs tell not to do this)
    hr = IDirect3DDevice7_Load(lpD3DDevice, texture_levels[1][1], NULL, texture_levels[0][1], NULL, 0);
    */

    /* Freed in reverse order as native seems to dislike and crash on freeing top level surface first. */
    for (i = 0; i < 2; i++)
    {
        for (i1 = 7; i1 >= 0; i1--)
        {
            if (texture_levels[i][i1]) IDirectDrawSurface7_Release(texture_levels[i][i1]);
        }
    }
    memset(texture_levels, 0, sizeof(texture_levels));

    /* Test texture size mismatch. */
    for (i = 0; i < 2; i++)
    {
        memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
        ddsd.dwWidth = i ? 256 : 128;
        ddsd.dwHeight = 128;
        hr = IDirectDraw7_CreateSurface(lpDD, &ddsd, &texture_levels[i][0], NULL);
        ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);
        if (FAILED(hr)) goto out;
    }

    hr = IDirect3DDevice7_Load(lpD3DDevice, texture_levels[1][0], NULL, texture_levels[0][0], NULL, 0);
    ok(hr==DDERR_INVALIDPARAMS, "IDirect3DDevice7_Load returned: %x\n",hr);

    hr = IDirect3DDevice7_Load(lpD3DDevice, texture_levels[0][0], NULL, texture_levels[1][0], NULL, 0);
    ok(hr==DDERR_INVALIDPARAMS, "IDirect3DDevice7_Load returned: %x\n",hr);

    IDirectDrawSurface7_Release(texture_levels[0][0]);
    IDirectDrawSurface7_Release(texture_levels[1][0]);
    memset(texture_levels, 0, sizeof(texture_levels));

    memset(&d3dcaps, 0, sizeof(d3dcaps));
    hr = IDirect3DDevice7_GetCaps(lpD3DDevice, &d3dcaps);
    ok(hr == D3D_OK, "IDirect3DDevice7_GetCaps returned %08x\n", hr);

    if (!(d3dcaps.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_CUBEMAP))
    {
        skip("No cubemap support\n");
    }
    else
    {
        /* Test loading mipmapped cubemap texture subrectangle from another similar texture. */
        for (i = 0; i < 2; i++)
        {
            memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
            ddsd.dwSize = sizeof(ddsd);
            ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
            ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
            ddsd.ddsCaps.dwCaps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALLFACES;
            ddsd.dwWidth = 128;
            ddsd.dwHeight = 128;
            U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
            U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB;
            U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 32;
            U2(U4(ddsd).ddpfPixelFormat).dwRBitMask = 0x00FF0000;
            U3(U4(ddsd).ddpfPixelFormat).dwGBitMask = 0x0000FF00;
            U4(U4(ddsd).ddpfPixelFormat).dwBBitMask = 0x000000FF;
            hr = IDirectDraw7_CreateSurface(lpDD, &ddsd, &cube_face_levels[i][0][0], NULL);
            ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);
            if (FAILED(hr)) goto out;

            flags = DDSCAPS2_CUBEMAP_NEGATIVEX;
            for (i1 = 1; i1 < 6; i1++, flags <<= 1)
            {
                ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
                ddsd.ddsCaps.dwCaps2 = DDSCAPS2_CUBEMAP | flags;
                hr = IDirectDrawSurface7_GetAttachedSurface(cube_face_levels[i][0][0], &ddsd.ddsCaps, &cube_face_levels[i][i1][0]);
                ok(hr == DD_OK, "GetAttachedSurface returned %08x\n", hr);
                if (FAILED(hr)) goto out;
            }

            for (i1 = 0; i1 < 6; i1++)
            {
                /* Check the number of created mipmaps */
                memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
                ddsd.dwSize = sizeof(ddsd);
                hr = IDirectDrawSurface7_GetSurfaceDesc(cube_face_levels[i][i1][0], &ddsd);
                ok(hr==DD_OK,"IDirectDrawSurface7_GetSurfaceDesc returned: %x\n",hr);
                ok(U2(ddsd).dwMipMapCount == 8, "unexpected mip count %u\n", U2(ddsd).dwMipMapCount);
                if (U2(ddsd).dwMipMapCount != 8) goto out;

                for (i2 = 1; i2 < 8; i2++)
                {
                    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP;
                    ddsd.ddsCaps.dwCaps2 = DDSCAPS2_MIPMAPSUBLEVEL;
                    hr = IDirectDrawSurface7_GetAttachedSurface(cube_face_levels[i][i1][i2 - 1], &ddsd.ddsCaps, &cube_face_levels[i][i1][i2]);
                    ok(hr == DD_OK, "GetAttachedSurface returned %08x\n", hr);
                    if (FAILED(hr)) goto out;
                }
            }
        }

        for (i = 0; i < 6; i++)
            for (i1 = 0; i1 < 8; i1++)
            {
                memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
                ddsd.dwSize = sizeof(ddsd);
                hr = IDirectDrawSurface7_Lock(cube_face_levels[0][i][i1], NULL, &ddsd, DDLOCK_WAIT, NULL);
                ok(hr==DD_OK, "IDirectDrawSurface7_Lock returned: %x\n",hr);
                if (FAILED(hr)) goto out;

                for (y = 0 ; y < ddsd.dwHeight; y++)
                {
                    DWORD *textureRow = (DWORD*)((char*)ddsd.lpSurface + y * U1(ddsd).lPitch);

                    for (x = 0; x < ddsd.dwWidth;  x++)
                    {
                        /* face number in low 4 bits of red, x stored in green component, y in blue. */
                        DWORD color = 0xf00000 | (i << 16) | (x << 8)  | y;
                        *textureRow++ = color;
                    }
                }

                hr = IDirectDrawSurface7_Unlock(cube_face_levels[0][i][i1], NULL);
                ok(hr==DD_OK, "IDirectDrawSurface7_Unlock returned: %x\n",hr);
            }

        for (i = 0; i < 6; i++)
            for (i1 = 0; i1 < 8; i1++)
            {
                memset(&ddbltfx, 0, sizeof(ddbltfx));
                ddbltfx.dwSize = sizeof(ddbltfx);
                U5(ddbltfx).dwFillColor = 0;
                hr = IDirectDrawSurface7_Blt(cube_face_levels[1][i][i1], NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
                ok(hr == DD_OK, "IDirectDrawSurface7_Blt failed with %08x\n", hr);
            }

        loadpoint.x = loadpoint.y = 10;
        loadrect.left = 30;
        loadrect.top = 20;
        loadrect.right = 93;
        loadrect.bottom = 52;

        hr = IDirect3DDevice7_Load(lpD3DDevice, cube_face_levels[1][0][0], &loadpoint, cube_face_levels[0][0][0], &loadrect,
                                        DDSCAPS2_CUBEMAP_ALLFACES);
        ok(hr==D3D_OK, "IDirect3DDevice7_Load returned: %x\n",hr);

        for (i = 0; i < 6; i++)
        {
            loadpoint.x = loadpoint.y = 10;
            loadrect.left = 30;
            loadrect.top = 20;
            loadrect.right = 93;
            loadrect.bottom = 52;

            for (i1 = 0; i1 < 8; i1++)
            {
                diff_count = 0;
                diff_count2 = 0;

                memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
                ddsd.dwSize = sizeof(ddsd);
                hr = IDirectDrawSurface7_Lock(cube_face_levels[1][i][i1], NULL, &ddsd, DDLOCK_WAIT, NULL);
                ok(hr==DD_OK, "IDirectDrawSurface7_Lock returned: %x\n",hr);
                if (FAILED(hr)) goto out;

                for (y = 0 ; y < ddsd.dwHeight; y++)
                {
                    DWORD *textureRow = (DWORD*)((char*)ddsd.lpSurface + y * U1(ddsd).lPitch);

                    for (x = 0; x < ddsd.dwWidth;  x++)
                    {
                        DWORD color = *textureRow++;

                        if (x < loadpoint.x || x >= loadpoint.x + loadrect.right - loadrect.left ||
                            y < loadpoint.y || y >= loadpoint.y + loadrect.bottom - loadrect.top)
                        {
                            if (color & 0xffffff) diff_count++;
                        }
                        else
                        {
                            DWORD r = (color & 0xff0000) >> 16;
                            DWORD g = (color & 0xff00) >> 8;
                            DWORD b = (color & 0xff);

                            if (r != (0xf0 | i) || g != x + loadrect.left - loadpoint.x ||
                                b != y + loadrect.top - loadpoint.y) diff_count++;
                        }

                        /* This codepath is for software RGB device. It has what looks like some weird off by one errors, but may
                        technically be correct as it's not precisely defined by docs. */
                        if (x < loadpoint.x || x >= loadpoint.x + loadrect.right - loadrect.left ||
                            y < loadpoint.y || y >= loadpoint.y + loadrect.bottom - loadrect.top + 1)
                        {
                            if (color & 0xffffff) diff_count2++;
                        }
                        else
                        {
                            DWORD r = (color & 0xff0000) >> 16;
                            DWORD g = (color & 0xff00) >> 8;
                            DWORD b = (color & 0xff);

                            if (r != (0xf0 | i) || !IS_VALUE_NEAR(g, x + loadrect.left - loadpoint.x) ||
                                !IS_VALUE_NEAR(b, y + loadrect.top - loadpoint.y)) diff_count2++;
                        }
                    }
                }

                hr = IDirectDrawSurface7_Unlock(cube_face_levels[1][i][i1], NULL);
                ok(hr==DD_OK, "IDirectDrawSurface7_Unlock returned: %x\n",hr);

                ok(diff_count == 0 || diff_count2 == 0,
                    "Unexpected destination texture level pixels; %u differences at face %x level %d\n",
                    MIN(diff_count, diff_count2), i, i1);

                loadpoint.x /= 2;
                loadpoint.y /= 2;
                loadrect.top /= 2;
                loadrect.left /= 2;
                loadrect.right = (loadrect.right + 1) / 2;
                loadrect.bottom = (loadrect.bottom + 1) / 2;
            }
        }

        for (i = 0; i < 2; i++)
            for (i1 = 5; i1 >= 0; i1--)
                for (i2 = 7; i2 >= 0; i2--)
                {
                    if (cube_face_levels[i][i1][i2]) IDirectDrawSurface7_Release(cube_face_levels[i][i1][i2]);
                }
        memset(cube_face_levels, 0, sizeof(cube_face_levels));

        /* Test cubemap loading from regular texture. */
        memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX;
        ddsd.ddsCaps.dwCaps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALLFACES;
        ddsd.dwWidth = 128;
        ddsd.dwHeight = 128;
        hr = IDirectDraw7_CreateSurface(lpDD, &ddsd, &cube_face_levels[0][0][0], NULL);
        ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);
        if (FAILED(hr)) goto out;

        memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
        ddsd.dwWidth = 128;
        ddsd.dwHeight = 128;
        hr = IDirectDraw7_CreateSurface(lpDD, &ddsd, &texture_levels[0][0], NULL);
        ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);
        if (FAILED(hr)) goto out;

        hr = IDirect3DDevice7_Load(lpD3DDevice, cube_face_levels[0][0][0], NULL, texture_levels[0][0], NULL,
                                        DDSCAPS2_CUBEMAP_ALLFACES);
        ok(hr==DDERR_INVALIDPARAMS, "IDirect3DDevice7_Load returned: %x\n",hr);

        IDirectDrawSurface7_Release(cube_face_levels[0][0][0]);
        memset(cube_face_levels, 0, sizeof(cube_face_levels));
        IDirectDrawSurface7_Release(texture_levels[0][0]);
        memset(texture_levels, 0, sizeof(texture_levels));

        /* Partial cube maps(e.g. created with an explicitly set DDSCAPS2_CUBEMAP_POSITIVEX flag)
         * BSOD some Windows machines when an app tries to create them(Radeon X1600, Windows XP,
         * Catalyst 10.2 driver, 6.14.10.6925)
         */
    }

    /* Test texture loading with different mip level count (larger levels match, smaller levels missing in destination. */
    for (i = 0; i < 2; i++)
    {
        memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_MIPMAPCOUNT;
        ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
        ddsd.dwWidth = 128;
        ddsd.dwHeight = 128;
        U2(ddsd).dwMipMapCount = i ? 4 : 8;
        U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
        U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB;
        U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 32;
        U2(U4(ddsd).ddpfPixelFormat).dwRBitMask = 0x00FF0000;
        U3(U4(ddsd).ddpfPixelFormat).dwGBitMask = 0x0000FF00;
        U4(U4(ddsd).ddpfPixelFormat).dwBBitMask = 0x000000FF;
        hr = IDirectDraw7_CreateSurface(lpDD, &ddsd, &texture_levels[i][0], NULL);
        ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);
        if (FAILED(hr)) goto out;

        /* Check the number of created mipmaps */
        memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
        ddsd.dwSize = sizeof(ddsd);
        hr = IDirectDrawSurface7_GetSurfaceDesc(texture_levels[i][0], &ddsd);
        ok(hr==DD_OK,"IDirectDrawSurface7_GetSurfaceDesc returned: %x\n",hr);
        ok(U2(ddsd).dwMipMapCount == (i ? 4 : 8), "unexpected mip count %u\n", U2(ddsd).dwMipMapCount);
        if (U2(ddsd).dwMipMapCount != (i ? 4 : 8)) goto out;

        for (i1 = 1; i1 < (i ? 4 : 8); i1++)
        {
            hr = IDirectDrawSurface7_GetAttachedSurface(texture_levels[i][i1 - 1], &ddsd.ddsCaps, &texture_levels[i][i1]);
            ok(hr == DD_OK, "GetAttachedSurface returned %08x\n", hr);
            if (FAILED(hr)) goto out;
        }
    }

    for (i1 = 0; i1 < 8; i1++)
    {
        memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
        ddsd.dwSize = sizeof(ddsd);
        hr = IDirectDrawSurface7_Lock(texture_levels[0][i1], NULL, &ddsd, DDLOCK_WAIT, NULL);
        ok(hr==DD_OK, "IDirectDrawSurface7_Lock returned: %x\n",hr);
        if (FAILED(hr)) goto out;

        for (y = 0 ; y < ddsd.dwHeight; y++)
        {
            DWORD *textureRow = (DWORD*)((char*)ddsd.lpSurface + y * U1(ddsd).lPitch);

            for (x = 0; x < ddsd.dwWidth;  x++)
            {
                /* x stored in green component, y in blue. */
                DWORD color = 0xf00000 | (i1 << 16) | (x << 8)  | y;
                *textureRow++ = color;
            }
        }

        hr = IDirectDrawSurface7_Unlock(texture_levels[0][i1], NULL);
        ok(hr==DD_OK, "IDirectDrawSurface7_Unlock returned: %x\n",hr);
    }

    for (i1 = 0; i1 < 4; i1++)
    {
        memset(&ddbltfx, 0, sizeof(ddbltfx));
        ddbltfx.dwSize = sizeof(ddbltfx);
        U5(ddbltfx).dwFillColor = 0;
        hr = IDirectDrawSurface7_Blt(texture_levels[1][i1], NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
        ok(hr == DD_OK, "IDirectDrawSurface7_Blt failed with %08x\n", hr);
    }

    loadpoint.x = loadpoint.y = 31;
    loadrect.left = 30;
    loadrect.top = 20;
    loadrect.right = 93;
    loadrect.bottom = 52;

    /* Destination mip levels are a subset of source mip levels. */
    hr = IDirect3DDevice7_Load(lpD3DDevice, texture_levels[1][0], &loadpoint, texture_levels[0][0], &loadrect, 0);
    ok(hr==D3D_OK, "IDirect3DDevice7_Load returned: %x\n",hr);

    for (i1 = 0; i1 < 4; i1++)
    {
        diff_count = 0;
        diff_count2 = 0;

        memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
        ddsd.dwSize = sizeof(ddsd);
        hr = IDirectDrawSurface7_Lock(texture_levels[1][i1], NULL, &ddsd, DDLOCK_WAIT, NULL);
        ok(hr==DD_OK, "IDirectDrawSurface7_Lock returned: %x\n",hr);
        if (FAILED(hr)) goto out;

        for (y = 0 ; y < ddsd.dwHeight; y++)
        {
            DWORD *textureRow = (DWORD*)((char*)ddsd.lpSurface + y * U1(ddsd).lPitch);

            for (x = 0; x < ddsd.dwWidth;  x++)
            {
                DWORD color = *textureRow++;

                if (x < loadpoint.x || x >= loadpoint.x + loadrect.right - loadrect.left ||
                    y < loadpoint.y || y >= loadpoint.y + loadrect.bottom - loadrect.top)
                {
                    if (color & 0xffffff) diff_count++;
                }
                else
                {
                    DWORD r = (color & 0xff0000) >> 16;
                    DWORD g = (color & 0xff00) >> 8;
                    DWORD b = (color & 0xff);

                    if (r != (0xf0 | i1) || g != x + loadrect.left - loadpoint.x ||
                        b != y + loadrect.top - loadpoint.y) diff_count++;
                }

                /* This codepath is for software RGB device. It has what looks like some weird off by one errors, but may
                technically be correct as it's not precisely defined by docs. */
                if (x < loadpoint.x || x >= loadpoint.x + loadrect.right - loadrect.left ||
                    y < loadpoint.y || y >= loadpoint.y + loadrect.bottom - loadrect.top + 1)
                {
                    if (color & 0xffffff) diff_count2++;
                }
                else
                {
                    DWORD r = (color & 0xff0000) >> 16;
                    DWORD g = (color & 0xff00) >> 8;
                    DWORD b = (color & 0xff);

                    if (r != (0xf0 | i1) || !IS_VALUE_NEAR(g, x + loadrect.left - loadpoint.x) ||
                        !IS_VALUE_NEAR(b, y + loadrect.top - loadpoint.y)) diff_count2++;
                }
            }
        }

        hr = IDirectDrawSurface7_Unlock(texture_levels[1][i1], NULL);
        ok(hr==DD_OK, "IDirectDrawSurface7_Unlock returned: %x\n",hr);

        ok(diff_count == 0 || diff_count2 == 0, "Unexpected destination texture level pixels; %u differences at %d level\n",
             MIN(diff_count, diff_count2), i1);

        loadpoint.x /= 2;
        loadpoint.y /= 2;
        loadrect.top /= 2;
        loadrect.left /= 2;
        loadrect.right = (loadrect.right + 1) / 2;
        loadrect.bottom = (loadrect.bottom + 1) / 2;
    }

    /* Destination mip levels are a superset of source mip levels (should fail). */
    hr = IDirect3DDevice7_Load(lpD3DDevice, texture_levels[0][0], &loadpoint, texture_levels[1][0], &loadrect, 0);
    ok(hr==DDERR_INVALIDPARAMS, "IDirect3DDevice7_Load returned: %x\n",hr);

    for (i = 0; i < 2; i++)
    {
        for (i1 = 7; i1 >= 0; i1--)
        {
            if (texture_levels[i][i1]) IDirectDrawSurface7_Release(texture_levels[i][i1]);
        }
    }
    memset(texture_levels, 0, sizeof(texture_levels));

    /* Test loading from mipmap texture to a regular texture that matches one sublevel in size. */
    memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 128;
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(ddsd).ddpfPixelFormat).dwRBitMask = 0x00FF0000;
    U3(U4(ddsd).ddpfPixelFormat).dwGBitMask = 0x0000FF00;
    U4(U4(ddsd).ddpfPixelFormat).dwBBitMask = 0x000000FF;
    hr = IDirectDraw7_CreateSurface(lpDD, &ddsd, &texture_levels[0][0], NULL);
    ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);
    if (FAILED(hr)) goto out;

    memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    ddsd.dwWidth = 32;
    ddsd.dwHeight = 32;
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(ddsd).ddpfPixelFormat).dwRBitMask = 0x00FF0000;
    U3(U4(ddsd).ddpfPixelFormat).dwGBitMask = 0x0000FF00;
    U4(U4(ddsd).ddpfPixelFormat).dwBBitMask = 0x000000FF;
    hr = IDirectDraw7_CreateSurface(lpDD, &ddsd, &texture_levels[1][0], NULL);
    ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);
    if (FAILED(hr)) goto out;

    for (i1 = 1; i1 < 8; i1++)
    {
        hr = IDirectDrawSurface7_GetAttachedSurface(texture_levels[0][i1 - 1], &ddsd.ddsCaps, &texture_levels[0][i1]);
        ok(hr == DD_OK, "GetAttachedSurface returned %08x\n", hr);
        if (FAILED(hr)) goto out;
    }

    for (i1 = 0; i1 < 8; i1++)
    {
        memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
        ddsd.dwSize = sizeof(ddsd);
        hr = IDirectDrawSurface7_Lock(texture_levels[0][i1], NULL, &ddsd, DDLOCK_WAIT, NULL);
        ok(hr==DD_OK, "IDirectDrawSurface7_Lock returned: %x\n",hr);
        if (FAILED(hr)) goto out;

        for (y = 0 ; y < ddsd.dwHeight; y++)
        {
            DWORD *textureRow = (DWORD*)((char*)ddsd.lpSurface + y * U1(ddsd).lPitch);

            for (x = 0; x < ddsd.dwWidth;  x++)
            {
                /* x stored in green component, y in blue. */
                DWORD color = 0xf00000 | (i1 << 16) | (x << 8)  | y;
                *textureRow++ = color;
            }
        }

        hr = IDirectDrawSurface7_Unlock(texture_levels[0][i1], NULL);
        ok(hr==DD_OK, "IDirectDrawSurface7_Unlock returned: %x\n",hr);
    }

    memset(&ddbltfx, 0, sizeof(ddbltfx));
    ddbltfx.dwSize = sizeof(ddbltfx);
    U5(ddbltfx).dwFillColor = 0;
    hr = IDirectDrawSurface7_Blt(texture_levels[1][0], NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
    ok(hr == DD_OK, "IDirectDrawSurface7_Blt failed with %08x\n", hr);

    loadpoint.x = loadpoint.y = 32;
    loadrect.left = 32;
    loadrect.top = 32;
    loadrect.right = 96;
    loadrect.bottom = 96;

    hr = IDirect3DDevice7_Load(lpD3DDevice, texture_levels[1][0], &loadpoint, texture_levels[0][0], &loadrect, 0);
    ok(hr==D3D_OK, "IDirect3DDevice7_Load returned: %x\n",hr);

    loadpoint.x /= 4;
    loadpoint.y /= 4;
    loadrect.top /= 4;
    loadrect.left /= 4;
    loadrect.right = (loadrect.right + 3) / 4;
    loadrect.bottom = (loadrect.bottom + 3) / 4;

    /* NOTE: something in either nvidia driver or directx9 on WinXP appears to be broken:
     * this kind of Load calls (to subset with smaller surface(s)) produces wrong results with
     * copied subrectangles divided more than needed, without apparent logic. But it works
     * as expected on qemu / Win98 / directx7 / RGB device. Some things are broken on XP, e.g.
     * some games don't work that worked in Win98, so it is assumed here XP results are wrong.
     * The following code attempts to detect broken results, actual tests will then be skipped
     */
    load_mip_subset_broken = TRUE;
    diff_count = 0;

    memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
    ddsd.dwSize = sizeof(ddsd);
    hr = IDirectDrawSurface7_Lock(texture_levels[1][0], NULL, &ddsd, DDLOCK_WAIT, NULL);
    ok(hr==DD_OK, "IDirectDrawSurface7_Lock returned: %x\n",hr);
    if (FAILED(hr)) goto out;

    for (y = 0 ; y < ddsd.dwHeight; y++)
    {
        DWORD *textureRow = (DWORD*)((char*)ddsd.lpSurface + y * U1(ddsd).lPitch);

        for (x = 0; x < ddsd.dwWidth;  x++)
        {
            DWORD color = *textureRow++;

            if (x < 2 || x >= 2 + 4 ||
                y < 2 || y >= 2 + 4)
            {
                if (color & 0xffffff) diff_count++;
            }
            else
            {
                DWORD r = (color & 0xff0000) >> 16;

                if ((r & (0xf0)) != 0xf0) diff_count++;
            }
        }
    }

    if (diff_count) load_mip_subset_broken = FALSE;

    if (load_mip_subset_broken) {
        skip("IDirect3DDevice7_Load is broken (happens on some modern Windows installations like XP). Skipping affected tests.\n");
    } else {
        diff_count = 0;

        for (y = 0 ; y < ddsd.dwHeight; y++)
        {
            DWORD *textureRow = (DWORD*)((char*)ddsd.lpSurface + y * U1(ddsd).lPitch);

            for (x = 0; x < ddsd.dwWidth;  x++)
            {
                DWORD color = *textureRow++;

                if (x < loadpoint.x || x >= loadpoint.x + loadrect.right - loadrect.left ||
                    y < loadpoint.y || y >= loadpoint.y + loadrect.bottom - loadrect.top)
                {
                    if (color & 0xffffff) diff_count++;
                }
                else
                {
                    DWORD r = (color & 0xff0000) >> 16;
                    DWORD g = (color & 0xff00) >> 8;
                    DWORD b = (color & 0xff);

                    if (r != (0xf0 | 2) || g != x + loadrect.left - loadpoint.x ||
                        b != y + loadrect.top - loadpoint.y) diff_count++;
                }
            }
        }
    }

    hr = IDirectDrawSurface7_Unlock(texture_levels[1][0], NULL);
    ok(hr==DD_OK, "IDirectDrawSurface7_Unlock returned: %x\n",hr);

    ok(diff_count == 0, "Unexpected destination texture level pixels; %u differences\n", diff_count);

    for (i = 0; i < 2; i++)
    {
        for (i1 = 7; i1 >= 0; i1--)
        {
            if (texture_levels[i][i1]) IDirectDrawSurface7_Release(texture_levels[i][i1]);
        }
    }
    memset(texture_levels, 0, sizeof(texture_levels));

    if (!load_mip_subset_broken)
    {
        /* Test loading when destination mip levels are a subset of source mip levels and start from smaller
        * surface (than first source mip level)
        */
        for (i = 0; i < 2; i++)
        {
            memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
            ddsd.dwSize = sizeof(ddsd);
            ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
            if (i) ddsd.dwFlags |= DDSD_MIPMAPCOUNT;
            ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
            ddsd.dwWidth = i ? 32 : 128;
            ddsd.dwHeight = i ? 32 : 128;
            if (i) U2(ddsd).dwMipMapCount = 4;
            U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
            U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB;
            U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 32;
            U2(U4(ddsd).ddpfPixelFormat).dwRBitMask = 0x00FF0000;
            U3(U4(ddsd).ddpfPixelFormat).dwGBitMask = 0x0000FF00;
            U4(U4(ddsd).ddpfPixelFormat).dwBBitMask = 0x000000FF;
            hr = IDirectDraw7_CreateSurface(lpDD, &ddsd, &texture_levels[i][0], NULL);
            ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);
            if (FAILED(hr)) goto out;

            /* Check the number of created mipmaps */
            memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
            ddsd.dwSize = sizeof(ddsd);
            hr = IDirectDrawSurface7_GetSurfaceDesc(texture_levels[i][0], &ddsd);
            ok(hr==DD_OK,"IDirectDrawSurface7_GetSurfaceDesc returned: %x\n",hr);
            ok(U2(ddsd).dwMipMapCount == (i ? 4 : 8), "unexpected mip count %u\n", U2(ddsd).dwMipMapCount);
            if (U2(ddsd).dwMipMapCount != (i ? 4 : 8)) goto out;

            for (i1 = 1; i1 < (i ? 4 : 8); i1++)
            {
                hr = IDirectDrawSurface7_GetAttachedSurface(texture_levels[i][i1 - 1], &ddsd.ddsCaps, &texture_levels[i][i1]);
                ok(hr == DD_OK, "GetAttachedSurface returned %08x\n", hr);
                if (FAILED(hr)) goto out;
            }
        }

        for (i1 = 0; i1 < 8; i1++)
        {
            memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
            ddsd.dwSize = sizeof(ddsd);
            hr = IDirectDrawSurface7_Lock(texture_levels[0][i1], NULL, &ddsd, DDLOCK_WAIT, NULL);
            ok(hr==DD_OK, "IDirectDrawSurface7_Lock returned: %x\n",hr);
            if (FAILED(hr)) goto out;

            for (y = 0 ; y < ddsd.dwHeight; y++)
            {
                DWORD *textureRow = (DWORD*)((char*)ddsd.lpSurface + y * U1(ddsd).lPitch);

                for (x = 0; x < ddsd.dwWidth;  x++)
                {
                    /* x stored in green component, y in blue. */
                    DWORD color = 0xf00000 | (i1 << 16) | (x << 8)  | y;
                    *textureRow++ = color;
                }
            }

            hr = IDirectDrawSurface7_Unlock(texture_levels[0][i1], NULL);
            ok(hr==DD_OK, "IDirectDrawSurface7_Unlock returned: %x\n",hr);
        }

        for (i1 = 0; i1 < 4; i1++)
        {
            memset(&ddbltfx, 0, sizeof(ddbltfx));
            ddbltfx.dwSize = sizeof(ddbltfx);
            U5(ddbltfx).dwFillColor = 0;
            hr = IDirectDrawSurface7_Blt(texture_levels[1][i1], NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
            ok(hr == DD_OK, "IDirectDrawSurface7_Blt failed with %08x\n", hr);
        }

        loadpoint.x = loadpoint.y = 0;
        loadrect.left = 0;
        loadrect.top = 0;
        loadrect.right = 64;
        loadrect.bottom = 64;

        hr = IDirect3DDevice7_Load(lpD3DDevice, texture_levels[1][0], &loadpoint, texture_levels[0][0], &loadrect, 0);
        ok(hr==D3D_OK, "IDirect3DDevice7_Load returned: %x\n",hr);

        i = 0;
        for (i1 = 0; i1 < 8 && i < 4; i1++)
        {
            DDSURFACEDESC2 ddsd2;

            memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
            ddsd.dwSize = sizeof(ddsd);
            hr = IDirectDrawSurface7_GetSurfaceDesc(texture_levels[0][i1], &ddsd);
            ok(SUCCEEDED(hr), "IDirectDrawSurface7_GetSurfaceDesc returned %#x.\n", hr);

            memset(&ddsd2, 0, sizeof(DDSURFACEDESC2));
            ddsd2.dwSize = sizeof(ddsd2);
            hr = IDirectDrawSurface7_GetSurfaceDesc(texture_levels[1][i], &ddsd2);
            ok(SUCCEEDED(hr), "IDirectDrawSurface7_GetSurfaceDesc returned %#x.\n", hr);

            if (ddsd.dwWidth == ddsd2.dwWidth && ddsd.dwHeight == ddsd2.dwHeight)
            {
                diff_count = 0;

                memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
                ddsd.dwSize = sizeof(ddsd);
                hr = IDirectDrawSurface7_Lock(texture_levels[1][i], NULL, &ddsd, DDLOCK_WAIT, NULL);
                ok(hr==DD_OK, "IDirectDrawSurface7_Lock returned: %x\n",hr);
                if (FAILED(hr)) goto out;

                for (y = 0 ; y < ddsd.dwHeight; y++)
                {
                    DWORD *textureRow = (DWORD*)((char*)ddsd.lpSurface + y * U1(ddsd).lPitch);

                    for (x = 0; x < ddsd.dwWidth;  x++)
                    {
                        DWORD color = *textureRow++;

                        if (x < loadpoint.x || x >= loadpoint.x + loadrect.right - loadrect.left ||
                            y < loadpoint.y || y >= loadpoint.y + loadrect.bottom - loadrect.top)
                        {
                            if (color & 0xffffff) diff_count++;
                        }
                        else
                        {
                            DWORD r = (color & 0xff0000) >> 16;
                            DWORD g = (color & 0xff00) >> 8;
                            DWORD b = (color & 0xff);

                            if (r != (0xf0 | i1) || g != x + loadrect.left - loadpoint.x ||
                                b != y + loadrect.top - loadpoint.y) diff_count++;
                        }
                    }
                }

                hr = IDirectDrawSurface7_Unlock(texture_levels[1][i], NULL);
                ok(hr==DD_OK, "IDirectDrawSurface7_Unlock returned: %x\n",hr);

                ok(diff_count == 0, "Unexpected destination texture level pixels; %u differences at %d level\n", diff_count, i1);

                i++;
            }

            loadpoint.x /= 2;
            loadpoint.y /= 2;
            loadrect.top /= 2;
            loadrect.left /= 2;
            loadrect.right = (loadrect.right + 1) / 2;
            loadrect.bottom = (loadrect.bottom + 1) / 2;
        }

        for (i = 0; i < 2; i++)
        {
            for (i1 = 7; i1 >= 0; i1--)
            {
                if (texture_levels[i][i1]) IDirectDrawSurface7_Release(texture_levels[i][i1]);
            }
        }
        memset(texture_levels, 0, sizeof(texture_levels));
    }

    /* Test palette copying. */
    for (i = 0; i < 2; i++)
    {
        memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
        ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
        ddsd.dwWidth = 128;
        ddsd.dwHeight = 128;
        U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
        U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
        U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 8;
        hr = IDirectDraw7_CreateSurface(lpDD, &ddsd, &texture_levels[i][0], NULL);
        ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);
        if (FAILED(hr)) goto out;

        /* Check the number of created mipmaps */
        memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
        ddsd.dwSize = sizeof(ddsd);
        hr = IDirectDrawSurface7_GetSurfaceDesc(texture_levels[i][0], &ddsd);
        ok(hr==DD_OK,"IDirectDrawSurface7_GetSurfaceDesc returned: %x\n",hr);
        ok(U2(ddsd).dwMipMapCount == 8, "unexpected mip count %u\n", U2(ddsd).dwMipMapCount);
        if (U2(ddsd).dwMipMapCount != 8) goto out;

        for (i1 = 1; i1 < 8; i1++)
        {
            hr = IDirectDrawSurface7_GetAttachedSurface(texture_levels[i][i1 - 1], &ddsd.ddsCaps, &texture_levels[i][i1]);
            ok(hr == DD_OK, "GetAttachedSurface returned %08x\n", hr);
            if (FAILED(hr)) goto out;
        }
    }

    memset(table1, 0, sizeof(table1));
    for (i = 0; i < 3; i++)
    {
        table1[0].peBlue = i + 1;
        hr = IDirectDraw7_CreatePalette(lpDD, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, table1, &palettes[i], NULL);
        ok(hr == DD_OK, "CreatePalette returned %08x\n", hr);
        if (FAILED(hr))
        {
            skip("IDirectDraw7_CreatePalette failed; skipping further tests\n");
            goto out;
        }
    }

    hr = IDirectDrawSurface7_SetPalette(texture_levels[0][0], palettes[0]);
    ok(hr==DD_OK, "IDirectDrawSurface7_SetPalette returned: %x\n", hr);

    hr = IDirect3DDevice7_Load(lpD3DDevice, texture_levels[1][0], NULL, texture_levels[0][0], NULL, 0);
    ok(hr==D3D_OK, "IDirect3DDevice7_Load returned: %x\n",hr);

    hr = IDirectDrawSurface7_GetPalette(texture_levels[0][1], &palettes[4]);
    ok(hr==DDERR_NOPALETTEATTACHED, "IDirectDrawSurface7_GetPalette returned: %x\n", hr);

    hr = IDirectDrawSurface7_GetPalette(texture_levels[1][0], &palettes[4]);
    ok(hr==DDERR_NOPALETTEATTACHED, "IDirectDrawSurface7_GetPalette returned: %x\n", hr);

    hr = IDirectDrawSurface7_SetPalette(texture_levels[0][1], palettes[1]);
    ok(hr==DDERR_NOTONMIPMAPSUBLEVEL, "IDirectDrawSurface7_SetPalette returned: %x\n", hr);
    hr = IDirectDrawSurface7_SetPalette(texture_levels[1][0], palettes[2]);
    ok(hr==DD_OK, "IDirectDrawSurface7_SetPalette returned: %x\n", hr);

    hr = IDirect3DDevice7_Load(lpD3DDevice, texture_levels[1][0], NULL, texture_levels[0][0], NULL, 0);
    ok(hr==D3D_OK, "IDirect3DDevice7_Load returned: %x\n",hr);

    memset(table1, 0, sizeof(table1));
    hr = IDirectDrawSurface7_GetPalette(texture_levels[1][0], &palettes[4]);
    ok(hr==DD_OK, "IDirectDrawSurface7_GetPalette returned: %x\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IDirectDrawPalette_GetEntries(palettes[4], 0, 0, 256, table1);
        ok(hr == DD_OK, "IDirectDrawPalette_GetEntries returned %08x\n", hr);
        ok(table1[0].peBlue == 1, "Unexpected palette color after load: %u\n", (unsigned)table1[0].peBlue);
    }

    /* Test colorkey copying. */
    ddckey.dwColorSpaceLowValue = ddckey.dwColorSpaceHighValue = 64;
    hr = IDirectDrawSurface7_SetColorKey(texture_levels[0][0], DDCKEY_SRCBLT, &ddckey);
    ok(hr==DD_OK, "IDirectDrawSurface7_SetColorKey returned: %x\n", hr);
    hr = IDirectDrawSurface7_SetColorKey(texture_levels[0][1], DDCKEY_SRCBLT, &ddckey);
    ok(hr == DDERR_NOTONMIPMAPSUBLEVEL, "Got unexpected hr %#x.\n", hr);

    hr = IDirectDrawSurface7_GetColorKey(texture_levels[1][0], DDCKEY_SRCBLT, &ddckey);
    ok(hr==DDERR_NOCOLORKEY, "IDirectDrawSurface7_GetColorKey returned: %x\n", hr);

    hr = IDirect3DDevice7_Load(lpD3DDevice, texture_levels[1][0], NULL, texture_levels[0][0], NULL, 0);
    ok(hr==D3D_OK, "IDirect3DDevice7_Load returned: %x\n",hr);

    hr = IDirectDrawSurface7_GetColorKey(texture_levels[1][0], DDCKEY_SRCBLT, &ddckey);
    ok(hr==DD_OK, "IDirectDrawSurface7_GetColorKey returned: %x\n", hr);
    ok(ddckey.dwColorSpaceLowValue == ddckey.dwColorSpaceHighValue && ddckey.dwColorSpaceLowValue == 64,
        "Unexpected color key values: %u - %u\n", ddckey.dwColorSpaceLowValue, ddckey.dwColorSpaceHighValue);

    out:

    for (i = 0; i < 5; i++)
    {
        if (palettes[i]) IDirectDrawPalette_Release(palettes[i]);
    }

    for (i = 0; i < 2; i++)
    {
        for (i1 = 7; i1 >= 0; i1--)
        {
            if (texture_levels[i][i1]) IDirectDrawSurface7_Release(texture_levels[i][i1]);
        }
    }

    for (i = 0; i < 2; i++)
        for (i1 = 5; i1 >= 0; i1--)
            for (i2 = 7; i2 >= 0; i2--)
            {
                if (cube_face_levels[i][i1][i2]) IDirectDrawSurface7_Release(cube_face_levels[i][i1][i2]);
            }
}

static void SetMaterialTest(void)
{
    HRESULT rc;

    rc =IDirect3DDevice7_SetMaterial(lpD3DDevice, NULL);
    ok(rc == DDERR_INVALIDPARAMS, "Expected DDERR_INVALIDPARAMS, got %x\n", rc);
}

static void SetRenderTargetTest(void)
{
    HRESULT hr;
    IDirectDrawSurface7 *newrt, *failrt, *oldrt, *temprt;
    D3DVIEWPORT7 vp;
    DDSURFACEDESC2 ddsd, ddsd2;
    DWORD stateblock;
    ULONG refcount;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_3DDEVICE;
    ddsd.dwWidth = 64;
    ddsd.dwHeight = 64;

    hr = IDirectDraw7_CreateSurface(lpDD, &ddsd, &newrt, NULL);
    ok(hr == DD_OK, "IDirectDraw7_CreateSurface failed, hr=0x%08x\n", hr);
    if(FAILED(hr))
    {
        skip("Skipping SetRenderTarget test\n");
        return;
    }

    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);
    ddsd2.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsd2.ddsCaps.dwCaps = DDSCAPS_3DDEVICE | DDSCAPS_ZBUFFER;
    ddsd2.dwWidth = 64;
    ddsd2.dwHeight = 64;
    U4(ddsd2).ddpfPixelFormat.dwSize = sizeof(U4(ddsd2).ddpfPixelFormat);
    U4(ddsd2).ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
    U1(U4(ddsd2).ddpfPixelFormat).dwZBufferBitDepth = 16;
    U3(U4(ddsd2).ddpfPixelFormat).dwZBitMask = 0x0000FFFF;

    hr = IDirectDraw7_CreateSurface(lpDD, &ddsd2, &failrt, NULL);
    ok(hr == DD_OK, "IDirectDraw7_CreateSurface failed, hr=0x%08x\n", hr);

    memset(&vp, 0, sizeof(vp));
    vp.dwX = 10;
    vp.dwY = 10;
    vp.dwWidth = 246;
    vp.dwHeight = 246;
    vp.dvMinZ = 0.25;
    vp.dvMaxZ = 0.75;
    hr = IDirect3DDevice7_SetViewport(lpD3DDevice, &vp);
    ok(hr == D3D_OK, "IDirect3DDevice7_SetViewport failed, hr=0x%08x\n", hr);

    hr = IDirect3DDevice7_GetRenderTarget(lpD3DDevice, &oldrt);
    ok(hr == DD_OK, "IDirect3DDevice7_GetRenderTarget failed, hr=0x%08x\n", hr);

    refcount = getRefcount((IUnknown*) oldrt);
    ok(refcount == 3, "Refcount should be 3, returned is %d\n", refcount);

    refcount = getRefcount((IUnknown*) failrt);
    ok(refcount == 1, "Refcount should be 1, returned is %d\n", refcount);

    hr = IDirect3DDevice7_SetRenderTarget(lpD3DDevice, failrt, 0);
    ok(hr != D3D_OK, "IDirect3DDevice7_SetRenderTarget succeeded\n");

    refcount = getRefcount((IUnknown*) oldrt);
    ok(refcount == 2, "Refcount should be 2, returned is %d\n", refcount);

    refcount = getRefcount((IUnknown*) failrt);
    ok(refcount == 2, "Refcount should be 2, returned is %d\n", refcount);

    hr = IDirect3DDevice7_GetRenderTarget(lpD3DDevice, &temprt);
    ok(hr == DD_OK, "IDirect3DDevice7_GetRenderTarget failed, hr=0x%08x\n", hr);
    ok(failrt == temprt, "Wrong iface returned\n");

    refcount = getRefcount((IUnknown*) failrt);
    ok(refcount == 3, "Refcount should be 3, returned is %d\n", refcount);

    hr = IDirect3DDevice7_SetRenderTarget(lpD3DDevice, newrt, 0);
    ok(hr == D3D_OK, "IDirect3DDevice7_SetRenderTarget failed, hr=0x%08x\n", hr);

    refcount = getRefcount((IUnknown*) failrt);
    ok(refcount == 2, "Refcount should be 2, returned is %d\n", refcount);

    memset(&vp, 0xff, sizeof(vp));
    hr = IDirect3DDevice7_GetViewport(lpD3DDevice, &vp);
    ok(hr == D3D_OK, "IDirect3DDevice7_GetViewport failed, hr=0x%08x\n", hr);
    ok(vp.dwX == 10, "vp.dwX is %u, expected 10\n", vp.dwX);
    ok(vp.dwY == 10, "vp.dwY is %u, expected 10\n", vp.dwY);
    ok(vp.dwWidth == 246, "vp.dwWidth is %u, expected 246\n", vp.dwWidth);
    ok(vp.dwHeight == 246, "vp.dwHeight is %u, expected 246\n", vp.dwHeight);
    ok(vp.dvMinZ == 0.25, "vp.dvMinZ is %f, expected 0.25\n", vp.dvMinZ);
    ok(vp.dvMaxZ == 0.75, "vp.dvMaxZ is %f, expected 0.75\n", vp.dvMaxZ);

    memset(&vp, 0, sizeof(vp));
    vp.dwX = 0;
    vp.dwY = 0;
    vp.dwWidth = 64;
    vp.dwHeight = 64;
    vp.dvMinZ = 0.0;
    vp.dvMaxZ = 1.0;
    hr = IDirect3DDevice7_SetViewport(lpD3DDevice, &vp);
    ok(hr == D3D_OK, "IDirect3DDevice7_SetViewport failed, hr=0x%08x\n", hr);

    hr = IDirect3DDevice7_BeginStateBlock(lpD3DDevice);
    ok(hr == D3D_OK, "IDirect3DDevice7_BeginStateblock failed, hr=0x%08x\n", hr);
    hr = IDirect3DDevice7_SetRenderTarget(lpD3DDevice, oldrt, 0);
    ok(hr == D3D_OK, "IDirect3DDevice7_SetRenderTarget failed, hr=0x%08x\n", hr);

    /* Check this twice, before and after ending the stateblock */
    memset(&vp, 0xff, sizeof(vp));
    hr = IDirect3DDevice7_GetViewport(lpD3DDevice, &vp);
    ok(hr == D3D_OK, "IDirect3DDevice7_GetViewport failed, hr=0x%08x\n", hr);
    ok(vp.dwX == 0, "vp.dwX is %u, expected 0\n", vp.dwX);
    ok(vp.dwY == 0, "vp.dwY is %u, expected 0\n", vp.dwY);
    ok(vp.dwWidth == 64, "vp.dwWidth is %u, expected 64\n", vp.dwWidth);
    ok(vp.dwHeight == 64, "vp.dwHeight is %u, expected 64\n", vp.dwHeight);
    ok(vp.dvMinZ == 0.0, "vp.dvMinZ is %f, expected 0.0\n", vp.dvMinZ);
    ok(vp.dvMaxZ == 1.0, "vp.dvMaxZ is %f, expected 1.0\n", vp.dvMaxZ);

    hr = IDirect3DDevice7_EndStateBlock(lpD3DDevice, &stateblock);
    ok(hr == D3D_OK, "IDirect3DDevice7_EndStateblock failed, hr=0x%08x\n", hr);

    memset(&vp, 0xff, sizeof(vp));
    hr = IDirect3DDevice7_GetViewport(lpD3DDevice, &vp);
    ok(hr == D3D_OK, "IDirect3DDevice7_GetViewport failed, hr=0x%08x\n", hr);
    ok(vp.dwX == 0, "vp.dwX is %u, expected 0\n", vp.dwX);
    ok(vp.dwY == 0, "vp.dwY is %u, expected 0\n", vp.dwY);
    ok(vp.dwWidth == 64, "vp.dwWidth is %u, expected 64\n", vp.dwWidth);
    ok(vp.dwHeight == 64, "vp.dwHeight is %u, expected 64\n", vp.dwHeight);
    ok(vp.dvMinZ == 0.0, "vp.dvMinZ is %f, expected 0.0\n", vp.dvMinZ);
    ok(vp.dvMaxZ == 1.0, "vp.dvMaxZ is %f, expected 1.0\n", vp.dvMaxZ);

    hr = IDirect3DDevice7_DeleteStateBlock(lpD3DDevice, stateblock);
    ok(hr == D3D_OK, "IDirect3DDevice7_DeleteStateblock failed, hr=0x%08x\n", hr);

    memset(&vp, 0, sizeof(vp));
    vp.dwX = 0;
    vp.dwY = 0;
    vp.dwWidth = 256;
    vp.dwHeight = 256;
    vp.dvMinZ = 0.0;
    vp.dvMaxZ = 0.0;
    hr = IDirect3DDevice7_SetViewport(lpD3DDevice, &vp);
    ok(hr == D3D_OK, "IDirect3DDevice7_SetViewport failed, hr=0x%08x\n", hr);

    IDirectDrawSurface7_Release(oldrt);
    IDirectDrawSurface7_Release(newrt);
    IDirectDrawSurface7_Release(failrt);
    IDirectDrawSurface7_Release(failrt);
}

static void VertexBufferLockRest(void)
{
    D3DVERTEXBUFFERDESC desc;
    IDirect3DVertexBuffer7 *buffer;
    HRESULT hr;
    unsigned int i;
    void *data;
    const struct
    {
        DWORD flags;
        const char *debug_string;
        HRESULT result;
    }
    test_data[] =
    {
        {0,                                         "(none)",                                       D3D_OK },
        {DDLOCK_WAIT,                               "DDLOCK_WAIT",                                  D3D_OK },
        {DDLOCK_EVENT,                              "DDLOCK_EVENT",                                 D3D_OK },
        {DDLOCK_READONLY,                           "DDLOCK_READONLY",                              D3D_OK },
        {DDLOCK_WRITEONLY,                          "DDLOCK_WRITEONLY",                             D3D_OK },
        {DDLOCK_NOSYSLOCK,                          "DDLOCK_NOSYSLOCK",                             D3D_OK },
        {DDLOCK_NOOVERWRITE,                        "DDLOCK_NOOVERWRITE",                           D3D_OK },
        {DDLOCK_DISCARDCONTENTS,                    "DDLOCK_DISCARDCONTENTS",                       D3D_OK },

        {DDLOCK_READONLY | DDLOCK_WRITEONLY,        "DDLOCK_READONLY | DDLOCK_WRITEONLY",           D3D_OK },
        {DDLOCK_READONLY | DDLOCK_DISCARDCONTENTS,  "DDLOCK_READONLY | DDLOCK_DISCARDCONTENTS",     D3D_OK },
        {0xdeadbeef,                                "0xdeadbeef",                                   D3D_OK },
    };

    memset(&desc, 0 , sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwCaps = 0;
    desc.dwFVF = D3DFVF_XYZ;
    desc.dwNumVertices = 64;
    hr = IDirect3D7_CreateVertexBuffer(lpD3D, &desc, &buffer, 0);
    ok(hr == D3D_OK, "IDirect3D7_CreateVertexBuffer failed, 0x%08x\n", hr);

    for(i = 0; i < (sizeof(test_data) / sizeof(*test_data)); i++)
    {
        hr = IDirect3DVertexBuffer7_Lock(buffer, test_data[i].flags, &data, NULL);
        ok(hr == test_data[i].result, "Lock flags %s returned 0x%08x, expected 0x%08x\n",
            test_data[i].debug_string, hr, test_data[i].result);
        if(SUCCEEDED(hr))
        {
            ok(data != NULL, "The data pointer returned by Lock is NULL\n");
            hr = IDirect3DVertexBuffer7_Unlock(buffer);
            ok(hr == D3D_OK, "IDirect3DVertexBuffer7_Unlock failed, 0x%08x\n", hr);
        }
    }

    IDirect3DVertexBuffer7_Release(buffer);
}

static void FindDevice(void)
{
    static const struct
    {
        const GUID *guid;
        BOOL todo;
    } deviceGUIDs[] =
    {
        {&IID_IDirect3DRampDevice, TRUE},
        {&IID_IDirect3DRGBDevice,  FALSE},
    };

    static const GUID *nonexistent_deviceGUIDs[] = {&IID_IDirect3DMMXDevice,
                                                    &IID_IDirect3DRefDevice,
                                                    &IID_IDirect3DTnLHalDevice,
                                                    &IID_IDirect3DNullDevice};

    D3DFINDDEVICESEARCH search = {0};
    D3DFINDDEVICERESULT result = {0};
    IDirect3DDevice *d3dhal;
    HRESULT hr;
    int i;

    /* Test invalid parameters. */
    hr = IDirect3D_FindDevice(Direct3D1, NULL, NULL);
    ok(hr == DDERR_INVALIDPARAMS,
       "Expected IDirect3D1::FindDevice to return DDERR_INVALIDPARAMS, got 0x%08x\n", hr);

    hr = IDirect3D_FindDevice(Direct3D1, NULL, &result);
    ok(hr == DDERR_INVALIDPARAMS,
       "Expected IDirect3D1::FindDevice to return DDERR_INVALIDPARAMS, got 0x%08x\n", hr);

    hr = IDirect3D_FindDevice(Direct3D1, &search, NULL);
    ok(hr == DDERR_INVALIDPARAMS,
       "Expected IDirect3D1::FindDevice to return DDERR_INVALIDPARAMS, got 0x%08x\n", hr);

    search.dwSize = 0;
    result.dwSize = 0;

    hr = IDirect3D_FindDevice(Direct3D1, &search, &result);
    ok(hr == DDERR_INVALIDPARAMS,
       "Expected IDirect3D1::FindDevice to return DDERR_INVALIDPARAMS, got 0x%08x\n", hr);

    search.dwSize = sizeof(search) + 1;
    result.dwSize = sizeof(result) + 1;

    hr = IDirect3D_FindDevice(Direct3D1, &search, &result);
    ok(hr == DDERR_INVALIDPARAMS,
       "Expected IDirect3D1::FindDevice to return DDERR_INVALIDPARAMS, got 0x%08x\n", hr);

    /* Specifying no flags is permitted. */
    search.dwSize = sizeof(search);
    search.dwFlags = 0;
    result.dwSize = sizeof(result);

    hr = IDirect3D_FindDevice(Direct3D1, &search, &result);
    ok(hr == D3D_OK,
       "Expected IDirect3D1::FindDevice to return D3D_OK, got 0x%08x\n", hr);

    /* Try an arbitrary non-device GUID. */
    search.dwSize = sizeof(search);
    search.dwFlags = D3DFDS_GUID;
    search.guid = IID_IDirect3D;
    result.dwSize = sizeof(result);

    hr = IDirect3D_FindDevice(Direct3D1, &search, &result);
    ok(hr == DDERR_NOTFOUND,
       "Expected IDirect3D1::FindDevice to return DDERR_NOTFOUND, got 0x%08x\n", hr);

    /* These GUIDs appear to be never present. */
    for (i = 0; i < sizeof(nonexistent_deviceGUIDs)/sizeof(nonexistent_deviceGUIDs[0]); i++)
    {
        search.dwSize = sizeof(search);
        search.dwFlags = D3DFDS_GUID;
        search.guid = *nonexistent_deviceGUIDs[i];
        result.dwSize = sizeof(result);

        hr = IDirect3D_FindDevice(Direct3D1, &search, &result);
        ok(hr == DDERR_NOTFOUND,
           "[%d] Expected IDirect3D1::FindDevice to return DDERR_NOTFOUND, got 0x%08x\n", i, hr);
    }

    /* The HAL device can only be enumerated if hardware acceleration is present. */
    search.dwSize = sizeof(search);
    search.dwFlags = D3DFDS_GUID;
    search.guid = IID_IDirect3DHALDevice;
    result.dwSize = sizeof(result);

    hr = IDirect3D_FindDevice(Direct3D1, &search, &result);
    trace("IDirect3D::FindDevice returned 0x%08x for the HAL device GUID\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IDirectDrawSurface_QueryInterface(Surface1, &IID_IDirect3DHALDevice, (void **)&d3dhal);
        /* Currently Wine only supports the creation of one Direct3D device
         * for a given DirectDraw instance. */
        ok(SUCCEEDED(hr) || broken(hr == DDERR_INVALIDPIXELFORMAT) /* XP/Win2003 Wow64 on VMware */,
           "Expected IDirectDrawSurface::QueryInterface to succeed, got 0x%08x\n", hr);

        if (SUCCEEDED(hr))
            IDirect3DDevice_Release(d3dhal);
    }
    else
    {
        hr = IDirectDrawSurface_QueryInterface(Surface1, &IID_IDirect3DHALDevice, (void **)&d3dhal);
        ok(FAILED(hr), "Expected IDirectDrawSurface::QueryInterface to fail, got 0x%08x\n", hr);

        if (SUCCEEDED(hr))
            IDirect3DDevice_Release(d3dhal);
    }

    /* These GUIDs appear to be always present. */
    for (i = 0; i < sizeof(deviceGUIDs)/sizeof(deviceGUIDs[0]); i++)
    {
        search.dwSize = sizeof(search);
        search.dwFlags = D3DFDS_GUID;
        search.guid = *deviceGUIDs[i].guid;
        result.dwSize = sizeof(result);

        hr = IDirect3D_FindDevice(Direct3D1, &search, &result);

        todo_wine_if (deviceGUIDs[i].todo)
            ok(hr == D3D_OK,
               "[%d] Expected IDirect3D1::FindDevice to return D3D_OK, got 0x%08x\n", i, hr);
    }

    /* Curiously the color model criteria seem to be ignored. */
    search.dwSize = sizeof(search);
    search.dwFlags = D3DFDS_COLORMODEL;
    search.dcmColorModel = 0xdeadbeef;
    result.dwSize = sizeof(result);

    hr = IDirect3D_FindDevice(Direct3D1, &search, &result);
    todo_wine
    ok(hr == D3D_OK,
       "Expected IDirect3D1::FindDevice to return D3D_OK, got 0x%08x\n", hr);
}

static void BackBuffer3DCreateSurfaceTest(void)
{
    DDSURFACEDESC ddsd;
    DDSURFACEDESC created_ddsd;
    DDSURFACEDESC2 ddsd2;
    IDirectDrawSurface *surf;
    IDirectDrawSurface4 *surf4;
    IDirectDrawSurface7 *surf7;
    HRESULT hr;
    IDirectDraw2 *dd2;
    IDirectDraw4 *dd4;
    IDirectDraw7 *dd7;
    DDCAPS ddcaps;
    IDirect3DDevice *d3dhal;

    const DWORD caps = DDSCAPS_BACKBUFFER | DDSCAPS_3DDEVICE;
    const DWORD expected_caps = DDSCAPS_BACKBUFFER | DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;

    memset(&ddcaps, 0, sizeof(ddcaps));
    ddcaps.dwSize = sizeof(DDCAPS);
    hr = IDirectDraw_GetCaps(DirectDraw1, &ddcaps, NULL);
    ok(SUCCEEDED(hr), "DirectDraw_GetCaps failed: 0x%08x\n", hr);
    if (!(ddcaps.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
    {
        skip("DDraw reported no VIDEOMEMORY cap. Broken video driver? Skipping surface caps tests.\n");
        return ;
    }

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.dwWidth = 64;
    ddsd.dwHeight = 64;
    ddsd.ddsCaps.dwCaps = caps;
    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);
    ddsd2.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd2.dwWidth = 64;
    ddsd2.dwHeight = 64;
    ddsd2.ddsCaps.dwCaps = caps;
    memset(&created_ddsd, 0, sizeof(created_ddsd));
    created_ddsd.dwSize = sizeof(DDSURFACEDESC);

    hr = IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &surf, NULL);
    ok(SUCCEEDED(hr), "IDirectDraw_CreateSurface failed: 0x%08x\n", hr);
    if (surf != NULL)
    {
        hr = IDirectDrawSurface_GetSurfaceDesc(surf, &created_ddsd);
        ok(SUCCEEDED(hr), "IDirectDraw_GetSurfaceDesc failed: 0x%08x\n", hr);
        ok(created_ddsd.ddsCaps.dwCaps == expected_caps,
           "GetSurfaceDesc returned caps %x, expected %x\n", created_ddsd.ddsCaps.dwCaps,
           expected_caps);

        hr = IDirectDrawSurface_QueryInterface(surf, &IID_IDirect3DHALDevice, (void **)&d3dhal);
        /* Currently Wine only supports the creation of one Direct3D device
           for a given DirectDraw instance. It has been created already
           in D3D1_createObjects() - IID_IDirect3DRGBDevice */
        todo_wine ok(SUCCEEDED(hr), "Expected IDirectDrawSurface::QueryInterface to succeed, got 0x%08x\n", hr);

        if (SUCCEEDED(hr))
            IDirect3DDevice_Release(d3dhal);

        IDirectDrawSurface_Release(surf);
    }

    hr = IDirectDraw_QueryInterface(DirectDraw1, &IID_IDirectDraw2, (void **) &dd2);
    ok(SUCCEEDED(hr), "IDirectDraw_QueryInterface failed: 0x%08x\n", hr);

    hr = IDirectDraw2_CreateSurface(dd2, &ddsd, &surf, NULL);
    ok(hr == DDERR_INVALIDCAPS, "IDirectDraw2_CreateSurface didn't return %x08x, but %x08x\n",
       DDERR_INVALIDCAPS, hr);

    IDirectDraw2_Release(dd2);

    hr = IDirectDraw_QueryInterface(DirectDraw1, &IID_IDirectDraw4, (void **) &dd4);
    ok(SUCCEEDED(hr), "IDirectDraw_QueryInterface failed: 0x%08x\n", hr);

    hr = IDirectDraw4_CreateSurface(dd4, &ddsd2, &surf4, NULL);
    ok(hr == DDERR_INVALIDCAPS, "IDirectDraw4_CreateSurface didn't return %x08x, but %x08x\n",
       DDERR_INVALIDCAPS, hr);

    IDirectDraw4_Release(dd4);

    hr = IDirectDraw_QueryInterface(DirectDraw1, &IID_IDirectDraw7, (void **) &dd7);
    ok(SUCCEEDED(hr), "IDirectDraw_QueryInterface failed: 0x%08x\n", hr);

    hr = IDirectDraw7_CreateSurface(dd7, &ddsd2, &surf7, NULL);
    ok(hr == DDERR_INVALIDCAPS, "IDirectDraw7_CreateSurface didn't return %x08x, but %x08x\n",
       DDERR_INVALIDCAPS, hr);

    IDirectDraw7_Release(dd7);
}

static void BackBuffer3DAttachmentTest(void)
{
    HRESULT hr;
    IDirectDrawSurface *surface1, *surface2, *surface3, *surface4;
    DDSURFACEDESC ddsd;
    HWND window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            100, 100, 160, 160, NULL, NULL, NULL, NULL);

    hr = IDirectDraw_SetCooperativeLevel(DirectDraw1, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "SetCooperativeLevel returned %08x\n", hr);

    /* Perform attachment tests on a back-buffer */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER | DDSCAPS_3DDEVICE;
    ddsd.dwWidth = GetSystemMetrics(SM_CXSCREEN);
    ddsd.dwHeight = GetSystemMetrics(SM_CYSCREEN);
    hr = IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &surface2, NULL);
    ok(SUCCEEDED(hr), "CreateSurface returned: %x\n",hr);

    if (surface2 != NULL)
    {
        /* Try a single primary and a two back buffers */
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        hr = IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &surface1, NULL);
        ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);

        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER | DDSCAPS_3DDEVICE;
        ddsd.dwWidth = GetSystemMetrics(SM_CXSCREEN);
        ddsd.dwHeight = GetSystemMetrics(SM_CYSCREEN);
        hr = IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &surface3, NULL);
        ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);

        /* This one has a different size */
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER | DDSCAPS_3DDEVICE;
        ddsd.dwWidth = 128;
        ddsd.dwHeight = 128;
        hr = IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &surface4, NULL);
        ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);

        hr = IDirectDrawSurface_AddAttachedSurface(surface1, surface2);
        todo_wine ok(hr == DD_OK || broken(hr == DDERR_CANNOTATTACHSURFACE),
           "Attaching a back buffer to a front buffer returned %08x\n", hr);
        if(SUCCEEDED(hr))
        {
            /* Try the reverse without detaching first */
            hr = IDirectDrawSurface_AddAttachedSurface(surface2, surface1);
            ok(hr == DDERR_SURFACEALREADYATTACHED, "Attaching an attached surface to its attachee returned %08x\n", hr);
            hr = IDirectDrawSurface_DeleteAttachedSurface(surface1, 0, surface2);
            ok(hr == DD_OK, "DeleteAttachedSurface failed with %08x\n", hr);
        }
        hr = IDirectDrawSurface_AddAttachedSurface(surface2, surface1);
        todo_wine ok(hr == DD_OK || broken(hr == DDERR_CANNOTATTACHSURFACE),
           "Attaching a front buffer to a back buffer returned %08x\n", hr);
        if(SUCCEEDED(hr))
        {
            /* Try to detach reversed */
            hr = IDirectDrawSurface_DeleteAttachedSurface(surface1, 0, surface2);
            ok(hr == DDERR_CANNOTDETACHSURFACE, "DeleteAttachedSurface returned %08x\n", hr);
            /* Now the proper detach */
            hr = IDirectDrawSurface_DeleteAttachedSurface(surface2, 0, surface1);
            ok(hr == DD_OK, "DeleteAttachedSurface failed with %08x\n", hr);
        }
        hr = IDirectDrawSurface_AddAttachedSurface(surface2, surface3);
        todo_wine ok(hr == DD_OK || broken(hr == DDERR_CANNOTATTACHSURFACE),
           "Attaching a back buffer to another back buffer returned %08x\n", hr);
        if(SUCCEEDED(hr))
        {
            hr = IDirectDrawSurface_DeleteAttachedSurface(surface2, 0, surface3);
            ok(hr == DD_OK, "DeleteAttachedSurface failed with %08x\n", hr);
        }
        hr = IDirectDrawSurface_AddAttachedSurface(surface1, surface4);
        ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching a back buffer to a front buffer of different size returned %08x\n", hr);
        hr = IDirectDrawSurface_AddAttachedSurface(surface4, surface1);
        ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching a front buffer to a back buffer of different size returned %08x\n", hr);

        IDirectDrawSurface_Release(surface4);
        IDirectDrawSurface_Release(surface3);
        IDirectDrawSurface_Release(surface2);
        IDirectDrawSurface_Release(surface1);
    }

    hr =IDirectDraw_SetCooperativeLevel(DirectDraw1, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "SetCooperativeLevel returned %08x\n", hr);

    DestroyWindow(window);
}

static void dump_format(const DDPIXELFORMAT *fmt)
{
    trace("dwFlags %08x, FourCC %08x, dwZBufferBitDepth %u, stencil %08x\n", fmt->dwFlags, fmt->dwFourCC,
          U1(*fmt).dwZBufferBitDepth, U2(*fmt).dwStencilBitDepth);
    trace("dwZBitMask %08x, dwStencilBitMask %08x, dwRGBZBitMask %08x\n", U3(*fmt).dwZBitMask,
          U4(*fmt).dwStencilBitMask, U5(*fmt).dwRGBZBitMask);
}

static HRESULT WINAPI enum_z_fmt_cb(DDPIXELFORMAT *fmt, void *ctx)
{
    static const DDPIXELFORMAT formats[] =
    {
        {
            sizeof(DDPIXELFORMAT), DDPF_ZBUFFER, 0,
            {16}, {0}, {0x0000ffff}, {0x00000000}, {0x00000000}
        },
        {
            sizeof(DDPIXELFORMAT), DDPF_ZBUFFER, 0,
            {32}, {0}, {0xffffff00}, {0x00000000}, {0x00000000}
        },
        {
            sizeof(DDPIXELFORMAT), DDPF_ZBUFFER | DDPF_STENCILBUFFER, 0,
            {32}, {8}, {0xffffff00}, {0x000000ff}, {0x00000000}
        },
        {
            sizeof(DDPIXELFORMAT), DDPF_ZBUFFER, 0,
            {32}, {0}, {0x00ffffff}, {0x00000000}, {0x00000000}
        },
        {
            sizeof(DDPIXELFORMAT), DDPF_ZBUFFER | DDPF_STENCILBUFFER, 0,
            {32}, {8}, {0x00ffffff}, {0xff000000}, {0x00000000}
        },
        {
            sizeof(DDPIXELFORMAT), DDPF_ZBUFFER, 0,
            {24}, {0}, {0x00ffffff}, {0x00000000}, {0x00000000}
        },
        {
            sizeof(DDPIXELFORMAT), DDPF_ZBUFFER, 0,
            {32}, {0}, {0xffffffff}, {0x00000000}, {0x00000000}
        },
    };
    unsigned int *count = ctx, i, expected_pitch;
    DDSURFACEDESC2 ddsd;
    IDirectDrawSurface7 *surface;
    HRESULT hr;
    (*count)++;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    U4(ddsd).ddpfPixelFormat = *fmt;
    ddsd.dwWidth = 1024;
    ddsd.dwHeight = 1024;
    hr = IDirectDraw7_CreateSurface(lpDD, &ddsd, &surface, NULL);
    ok(SUCCEEDED(hr), "IDirectDraw7_CreateSurface failed, hr %#x.\n", hr);
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    hr = IDirectDrawSurface7_GetSurfaceDesc(surface, &ddsd);
    ok(SUCCEEDED(hr), "IDirectDrawSurface7_GetSurfaceDesc failed, hr %#x.\n", hr);
    IDirectDrawSurface7_Release(surface);

    ok(ddsd.dwFlags & DDSD_PIXELFORMAT, "DDSD_PIXELFORMAT is not set\n");
    ok(!(ddsd.dwFlags & DDSD_ZBUFFERBITDEPTH), "DDSD_ZBUFFERBITDEPTH is set\n");

    /* 24 bit unpadded depth buffers are actually padded(Geforce 9600, Win7,
     * Radeon 9000M WinXP) */
    if (U1(*fmt).dwZBufferBitDepth == 24) expected_pitch = ddsd.dwWidth * 4;
    else expected_pitch = ddsd.dwWidth * U1(*fmt).dwZBufferBitDepth / 8;

    /* Some formats(16 bit depth without stencil) return pitch 0
     *
     * The Radeon X1600 Catalyst 10.2 Windows XP driver returns an otherwise sane
     * pitch with an extra 128 bytes, regardless of the format and width */
    if (U1(ddsd).lPitch != 0 && U1(ddsd).lPitch != expected_pitch
            && !broken(U1(ddsd).lPitch == expected_pitch + 128))
    {
        ok(0, "Z buffer pitch is %u, expected %u\n", U1(ddsd).lPitch, expected_pitch);
        dump_format(fmt);
    }

    for (i = 0; i < (sizeof(formats)/sizeof(*formats)); i++)
    {
        if (memcmp(&formats[i], fmt, fmt->dwSize) == 0) return DDENUMRET_OK;
    }

    ok(0, "Unexpected Z format enumerated\n");
    dump_format(fmt);

    return DDENUMRET_OK;
}

static void z_format_test(void)
{
    unsigned int count = 0;
    HRESULT hr;

    hr = IDirect3D7_EnumZBufferFormats(lpD3D, &IID_IDirect3DHALDevice, enum_z_fmt_cb, &count);
    if (hr == DDERR_NOZBUFFERHW)
    {
        skip("Z buffers not supported, skipping Z buffer format test\n");
        return;
    }

    ok(SUCCEEDED(hr), "IDirect3D7_EnumZBufferFormats failed, hr %#x.\n", hr);
    ok(count, "Expected at least one supported Z Buffer format\n");
}

static void test_get_caps1(void)
{
    D3DDEVICEDESC hw_caps, hel_caps;
    HRESULT hr;
    unsigned int i;

    memset(&hw_caps, 0, sizeof(hw_caps));
    hw_caps.dwSize = sizeof(hw_caps);
    hw_caps.dwFlags = 0xdeadbeef;
    memset(&hel_caps, 0, sizeof(hel_caps));
    hel_caps.dwSize = sizeof(hel_caps);
    hel_caps.dwFlags = 0xdeadc0de;

    /* NULL pointers */
    hr = IDirect3DDevice_GetCaps(Direct3DDevice1, &hw_caps, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "GetCaps with NULL hel caps returned hr %#x, expected INVALIDPARAMS.\n", hr);
    ok(hw_caps.dwFlags == 0xdeadbeef, "hw_caps.dwFlags was modified: %#x.\n", hw_caps.dwFlags);
    hr = IDirect3DDevice_GetCaps(Direct3DDevice1, NULL, &hel_caps);
    ok(hr == DDERR_INVALIDPARAMS, "GetCaps with NULL hw caps returned hr %#x, expected INVALIDPARAMS.\n", hr);
    ok(hel_caps.dwFlags == 0xdeadc0de, "hel_caps.dwFlags was modified: %#x.\n", hel_caps.dwFlags);

    /* Successful call: Both are modified */
    hr = IDirect3DDevice_GetCaps(Direct3DDevice1, &hw_caps, &hel_caps);
    ok(hr == D3D_OK, "GetCaps with correct size returned hr %#x, expected D3D_OK.\n", hr);
    ok(hw_caps.dwFlags != 0xdeadbeef, "hw_caps.dwFlags was not modified: %#x.\n", hw_caps.dwFlags);
    ok(hel_caps.dwFlags != 0xdeadc0de, "hel_caps.dwFlags was not modified: %#x.\n", hel_caps.dwFlags);

    memset(&hw_caps, 0, sizeof(hw_caps));
    hw_caps.dwSize = sizeof(hw_caps);
    hw_caps.dwFlags = 0xdeadbeef;
    memset(&hel_caps, 0, sizeof(hel_caps));
    /* Keep dwSize at 0 */
    hel_caps.dwFlags = 0xdeadc0de;

    /* If one is invalid the call fails */
    hr = IDirect3DDevice_GetCaps(Direct3DDevice1, &hw_caps, &hel_caps);
    ok(hr == DDERR_INVALIDPARAMS, "GetCaps with invalid hel_caps size returned hr %#x, expected INVALIDPARAMS.\n", hr);
    ok(hw_caps.dwFlags == 0xdeadbeef, "hw_caps.dwFlags was modified: %#x.\n", hw_caps.dwFlags);
    ok(hel_caps.dwFlags == 0xdeadc0de, "hel_caps.dwFlags was modified: %#x.\n", hel_caps.dwFlags);
    hel_caps.dwSize = sizeof(hel_caps);
    hw_caps.dwSize = sizeof(hw_caps) + 1;
    hr = IDirect3DDevice_GetCaps(Direct3DDevice1, &hw_caps, &hel_caps);
    ok(hr == DDERR_INVALIDPARAMS, "GetCaps with invalid hw_caps size returned hr %#x, expected INVALIDPARAMS.\n", hr);
    ok(hw_caps.dwFlags == 0xdeadbeef, "hw_caps.dwFlags was modified: %#x.\n", hw_caps.dwFlags);
    ok(hel_caps.dwFlags == 0xdeadc0de, "hel_caps.dwFlags was modified: %#x.\n", hel_caps.dwFlags);

    for (i = 0; i < 1024; i++)
    {
        memset(&hw_caps, 0xfe, sizeof(hw_caps));
        memset(&hel_caps, 0xfe, sizeof(hel_caps));
        hw_caps.dwSize = hel_caps.dwSize = i;
        hr = IDirect3DDevice_GetCaps(Direct3DDevice1, &hw_caps, &hel_caps);
        switch (i)
        {
            /* D3DDEVICEDESCSIZE in old sdk versions */
            case FIELD_OFFSET(D3DDEVICEDESC, dwMinTextureWidth): /* 172, DirectX 3, IDirect3DDevice1 */
                ok(hw_caps.dwMinTextureWidth == 0xfefefefe, "hw_caps.dwMinTextureWidth was modified: %#x.\n",
                        hw_caps.dwMinTextureWidth);
                ok(hel_caps.dwMinTextureWidth == 0xfefefefe, "hel_caps.dwMinTextureWidth was modified: %#x.\n",
                        hel_caps.dwMinTextureWidth);
                /* drop through */
            case FIELD_OFFSET(D3DDEVICEDESC, dwMaxTextureRepeat): /* 204, DirectX 5, IDirect3DDevice2 */
                ok(hw_caps.dwMaxTextureRepeat == 0xfefefefe, "hw_caps.dwMaxTextureRepeat was modified: %#x.\n",
                        hw_caps.dwMaxTextureRepeat);
                ok(hel_caps.dwMaxTextureRepeat == 0xfefefefe, "hel_caps.dwMaxTextureRepeat was modified: %#x.\n",
                        hel_caps.dwMaxTextureRepeat);
                /* drop through */
            case sizeof(D3DDEVICEDESC): /* 252, DirectX 6, IDirect3DDevice3 */
                ok(hr == D3D_OK, "GetCaps with size %u returned hr %#x, expected D3D_OK.\n", i, hr);
                break;

            default:
                ok(hr == DDERR_INVALIDPARAMS,
                        "GetCaps with size %u returned hr %#x, expected DDERR_INVALIDPARAMS.\n", i, hr);
                break;
        }
    }

    /* Different valid sizes are OK */
    hw_caps.dwSize = 172;
    hel_caps.dwSize = sizeof(D3DDEVICEDESC);
    hr = IDirect3DDevice_GetCaps(Direct3DDevice1, &hw_caps, &hel_caps);
    ok(hr == D3D_OK, "GetCaps with different sizes returned hr %#x, expected D3D_OK.\n", hr);
}

static void test_get_caps7(void)
{
    HRESULT hr;
    D3DDEVICEDESC7 desc;

    hr = IDirect3DDevice7_GetCaps(lpD3DDevice, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirect3DDevice7::GetCaps(NULL) returned hr %#x, expected INVALIDPARAMS.\n", hr);

    memset(&desc, 0, sizeof(desc));
    hr = IDirect3DDevice7_GetCaps(lpD3DDevice, &desc);
    ok(hr == D3D_OK, "IDirect3DDevice7::GetCaps(non-NULL) returned hr %#x, expected D3D_OK.\n", hr);

    /* There's no dwSize in D3DDEVICEDESC7 */
}

struct d3d2_test_context
{
    IDirectDraw *ddraw;
    IDirect3D2 *d3d;
    IDirectDrawSurface *surface;
    IDirect3DDevice2 *device;
    IDirect3DViewport2 *viewport;
};

static void d3d2_release_objects(struct d3d2_test_context *context)
{
    LONG ref;
    HRESULT hr;

    if (context->viewport)
    {
        hr = IDirect3DDevice2_DeleteViewport(context->device, context->viewport);
        ok(hr == D3D_OK, "DeleteViewport returned %08x.\n", hr);
        ref = IDirect3DViewport2_Release(context->viewport);
        ok(ref == 0, "Viewport has reference count %d, expected 0.\n", ref);
    }
    if (context->device)
    {
        ref = IDirect3DDevice2_Release(context->device);
        ok(ref == 0, "Device has reference count %d, expected 0.\n", ref);
    }
    if (context->surface)
    {
        ref = IDirectDrawSurface_Release(context->surface);
        ok(ref == 0, "Surface has reference count %d, expected 0.\n", ref);
    }
    if (context->d3d)
    {
        ref = IDirect3D2_Release(context->d3d);
        ok(ref == 1, "IDirect3D2 has reference count %d, expected 1.\n", ref);
    }
    if (context->ddraw)
    {
        ref = IDirectDraw_Release(context->ddraw);
        ok(ref == 0, "DDraw has reference count %d, expected 0.\n", ref);
    }
}

static BOOL d3d2_create_objects(struct d3d2_test_context *context)
{
    HRESULT hr;
    DDSURFACEDESC ddsd;
    D3DVIEWPORT vp_data;

    memset(context, 0, sizeof(*context));

    hr = DirectDrawCreate(NULL, &context->ddraw, NULL);
    ok(hr == DD_OK || hr == DDERR_NODIRECTDRAWSUPPORT, "DirectDrawCreate failed: %08x.\n", hr);
    if (!context->ddraw) goto error;

    hr = IDirectDraw_SetCooperativeLevel(context->ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "SetCooperativeLevel failed: %08x.\n", hr);
    if (FAILED(hr)) goto error;

    hr = IDirectDraw_QueryInterface(context->ddraw, &IID_IDirect3D2, (void**) &context->d3d);
    ok(hr == DD_OK || hr == E_NOINTERFACE, "QueryInterface failed: %08x.\n", hr);
    if (!context->d3d) goto error;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    ddsd.dwWidth = 256;
    ddsd.dwHeight = 256;
    IDirectDraw_CreateSurface(context->ddraw, &ddsd, &context->surface, NULL);
    if (!context->surface)
    {
        skip("DDSCAPS_3DDEVICE surface not available.\n");
        goto error;
    }

    hr = IDirect3D2_CreateDevice(context->d3d, &IID_IDirect3DHALDevice, context->surface, &context->device);
    ok(hr == D3D_OK  || hr == E_OUTOFMEMORY || hr == E_NOINTERFACE, "CreateDevice failed: %08x.\n", hr);
    if (!context->device) goto error;

    hr = IDirect3D2_CreateViewport(context->d3d, &context->viewport, NULL);
    ok(hr == D3D_OK, "CreateViewport failed: %08x.\n", hr);
    if (!context->viewport) goto error;

    hr = IDirect3DDevice2_AddViewport(context->device, context->viewport);
    ok(hr == D3D_OK, "AddViewport returned %08x.\n", hr);
    vp_data.dwSize = sizeof(vp_data);
    vp_data.dwX = 0;
    vp_data.dwY = 0;
    vp_data.dwWidth = 256;
    vp_data.dwHeight = 256;
    vp_data.dvScaleX = 1;
    vp_data.dvScaleY = 1;
    vp_data.dvMaxX = 256;
    vp_data.dvMaxY = 256;
    vp_data.dvMinZ = 0;
    vp_data.dvMaxZ = 1;
    hr = IDirect3DViewport2_SetViewport(context->viewport, &vp_data);
    ok(hr == D3D_OK, "SetViewport returned %08x.\n", hr);

    return TRUE;

error:
    d3d2_release_objects(context);
    return FALSE;
}

static void test_get_caps2(const struct d3d2_test_context *context)
{
    D3DDEVICEDESC hw_caps, hel_caps;
    HRESULT hr;
    unsigned int i;

    memset(&hw_caps, 0, sizeof(hw_caps));
    hw_caps.dwSize = sizeof(hw_caps);
    hw_caps.dwFlags = 0xdeadbeef;
    memset(&hel_caps, 0, sizeof(hel_caps));
    hel_caps.dwSize = sizeof(hel_caps);
    hel_caps.dwFlags = 0xdeadc0de;

    /* NULL pointers */
    hr = IDirect3DDevice2_GetCaps(context->device, &hw_caps, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "GetCaps with NULL hel caps returned hr %#x, expected INVALIDPARAMS.\n", hr);
    ok(hw_caps.dwFlags == 0xdeadbeef, "hw_caps.dwFlags was modified: %#x.\n", hw_caps.dwFlags);
    hr = IDirect3DDevice2_GetCaps(context->device, NULL, &hel_caps);
    ok(hr == DDERR_INVALIDPARAMS, "GetCaps with NULL hw caps returned hr %#x, expected INVALIDPARAMS.\n", hr);
    ok(hel_caps.dwFlags == 0xdeadc0de, "hel_caps.dwFlags was modified: %#x.\n", hel_caps.dwFlags);

    /* Successful call: Both are modified */
    hr = IDirect3DDevice2_GetCaps(context->device, &hw_caps, &hel_caps);
    ok(hr == D3D_OK, "GetCaps with correct size returned hr %#x, expected D3D_OK.\n", hr);
    ok(hw_caps.dwFlags != 0xdeadbeef, "hw_caps.dwFlags was not modified: %#x.\n", hw_caps.dwFlags);
    ok(hel_caps.dwFlags != 0xdeadc0de, "hel_caps.dwFlags was not modified: %#x.\n", hel_caps.dwFlags);

    memset(&hw_caps, 0, sizeof(hw_caps));
    hw_caps.dwSize = sizeof(hw_caps);
    hw_caps.dwFlags = 0xdeadbeef;
    memset(&hel_caps, 0, sizeof(hel_caps));
    /* Keep dwSize at 0 */
    hel_caps.dwFlags = 0xdeadc0de;

    /* If one is invalid the call fails */
    hr = IDirect3DDevice2_GetCaps(context->device, &hw_caps, &hel_caps);
    ok(hr == DDERR_INVALIDPARAMS, "GetCaps with invalid hel_caps size returned hr %#x, expected INVALIDPARAMS.\n", hr);
    ok(hw_caps.dwFlags == 0xdeadbeef, "hw_caps.dwFlags was modified: %#x.\n", hw_caps.dwFlags);
    ok(hel_caps.dwFlags == 0xdeadc0de, "hel_caps.dwFlags was modified: %#x.\n", hel_caps.dwFlags);
    hel_caps.dwSize = sizeof(hel_caps);
    hw_caps.dwSize = sizeof(hw_caps) + 1;
    hr = IDirect3DDevice2_GetCaps(context->device, &hw_caps, &hel_caps);
    ok(hr == DDERR_INVALIDPARAMS, "GetCaps with invalid hw_caps size returned hr %#x, expected INVALIDPARAMS.\n", hr);
    ok(hw_caps.dwFlags == 0xdeadbeef, "hw_caps.dwFlags was modified: %#x.\n", hw_caps.dwFlags);
    ok(hel_caps.dwFlags == 0xdeadc0de, "hel_caps.dwFlags was modified: %#x.\n", hel_caps.dwFlags);

    for (i = 0; i < 1024; i++)
    {
        memset(&hw_caps, 0xfe, sizeof(hw_caps));
        memset(&hel_caps, 0xfe, sizeof(hel_caps));
        hw_caps.dwSize = hel_caps.dwSize = i;
        hr = IDirect3DDevice2_GetCaps(context->device, &hw_caps, &hel_caps);
        switch (i)
        {
            /* D3DDEVICEDESCSIZE in old sdk versions */
            case FIELD_OFFSET(D3DDEVICEDESC, dwMinTextureWidth): /* 172, DirectX 3, IDirect3DDevice1 */
                ok(hw_caps.dwMinTextureWidth == 0xfefefefe, "dwMinTextureWidth was modified: %#x.\n",
                        hw_caps.dwMinTextureWidth);
                ok(hel_caps.dwMinTextureWidth == 0xfefefefe, "dwMinTextureWidth was modified: %#x.\n",
                        hel_caps.dwMinTextureWidth);
                /* drop through */
            case FIELD_OFFSET(D3DDEVICEDESC, dwMaxTextureRepeat): /* 204, DirectX 5, IDirect3DDevice2 */
                ok(hw_caps.dwMaxTextureRepeat == 0xfefefefe, "dwMaxTextureRepeat was modified: %#x.\n",
                        hw_caps.dwMaxTextureRepeat);
                ok(hel_caps.dwMaxTextureRepeat == 0xfefefefe, "dwMaxTextureRepeat was modified: %#x.\n",
                        hel_caps.dwMaxTextureRepeat);
                /* drop through */
            case sizeof(D3DDEVICEDESC): /* 252, DirectX 6, IDirect3DDevice3 */
                ok(hr == D3D_OK, "GetCaps with size %u returned hr %#x, expected D3D_OK.\n", i, hr);
                break;

            default:
                ok(hr == DDERR_INVALIDPARAMS,
                        "GetCaps with size %u returned hr %#x, expected DDERR_INVALIDPARAMS.\n", i, hr);
                break;
        }
    }

    /* Different valid sizes are OK */
    hw_caps.dwSize = 172;
    hel_caps.dwSize = sizeof(D3DDEVICEDESC);
    hr = IDirect3DDevice2_GetCaps(context->device, &hw_caps, &hel_caps);
    ok(hr == D3D_OK, "GetCaps with different sizes returned hr %#x, expected D3D_OK.\n", hr);
}

START_TEST(d3d)
{
    struct d3d2_test_context d3d2_context;
    void (* const d3d2_tests[])(const struct d3d2_test_context *) =
    {
        test_get_caps2
    };
    unsigned int i;

    init_function_pointers();
    if(!pDirectDrawCreateEx) {
        win_skip("function DirectDrawCreateEx not available\n");
        return;
    }

    if(!CreateDirect3D()) {
        skip("Skipping d3d7 tests\n");
    } else {
        LightTest();
        SceneTest();
        D3D7EnumTest();
        D3D7EnumLifetimeTest();
        SetMaterialTest();
        CapsTest();
        VertexBufferDescTest();
        DeviceLoadTest();
        SetRenderTargetTest();
        VertexBufferLockRest();
        z_format_test();
        test_get_caps7();
        ReleaseDirect3D();
    }

    for (i = 0; i < (sizeof(d3d2_tests) / sizeof(*d3d2_tests)); i++)
    {
        if (!d3d2_create_objects(&d3d2_context))
        {
            ok(!i, "Unexpected d3d2 initialization failure.\n");
            skip("Skipping d3d2 tests.\n");
            break;
        }
        d3d2_tests[i](&d3d2_context);
        d3d2_release_objects(&d3d2_context);
    }

    if (!D3D1_createObjects()) {
        skip("Skipping d3d1 tests\n");
    } else {
        Direct3D1Test();
        TextureLoadTest();
        ViewportTest();
        FindDevice();
        BackBuffer3DCreateSurfaceTest();
        BackBuffer3DAttachmentTest();
        test_get_caps1();
        D3D1_releaseObjects();
    }
}
