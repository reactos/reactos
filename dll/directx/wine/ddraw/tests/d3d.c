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
    ok(rc==DD_OK || rc==DDERR_NODIRECTDRAWSUPPORT, "Got hr %#lx.\n", rc);
    if (!lpDD) {
        trace("DirectDrawCreateEx() failed with an error %#lx\n", rc);
        return FALSE;
    }

    rc = IDirectDraw7_SetCooperativeLevel(lpDD, NULL, DDSCL_NORMAL);
    ok(rc==DD_OK, "Got hr %#lx.\n", rc);

    rc = IDirectDraw7_QueryInterface(lpDD, &IID_IDirect3D7, (void**) &lpD3D);
    if (rc == E_NOINTERFACE)
    {
        IDirectDraw7_Release(lpDD);
        return FALSE;
    }
    ok(rc==DD_OK, "Got hr %#lx.\n", rc);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    ddsd.dwWidth = 256;
    ddsd.dwHeight = 256;
    rc = IDirectDraw7_CreateSurface(lpDD, &ddsd, &lpDDS, NULL);
    if (FAILED(rc))
    {
        IDirect3D7_Release(lpD3D);
        IDirectDraw7_Release(lpDD);
        return FALSE;
    }

    num = 0;
    IDirectDraw7_EnumSurfaces(lpDD, DDENUMSURFACES_ALL | DDENUMSURFACES_DOESEXIST, NULL, &num, SurfaceCounter);
    ok(num == 1, "Has %d surfaces, expected 1\n", num);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
    ddsd.ddpfPixelFormat.dwZBufferBitDepth = 16;
    ddsd.ddpfPixelFormat.dwZBitMask = 0x0000FFFF;
    ddsd.dwWidth = 256;
    ddsd.dwHeight = 256;
    rc = IDirectDraw7_CreateSurface(lpDD, &ddsd, &lpDDSdepth, NULL);
    ok(rc==DD_OK, "Got hr %#lx.\n", rc);
    if (FAILED(rc)) {
        lpDDSdepth = NULL;
    } else {
        rc = IDirectDrawSurface_AddAttachedSurface(lpDDS, lpDDSdepth);
        ok(rc == DD_OK, "Got hr %#lx.\n", rc);
        if (FAILED(rc))
        {
            IDirectDrawSurface7_Release(lpDDSdepth);
            IDirectDrawSurface7_Release(lpDDS);
            IDirect3D7_Release(lpD3D);
            IDirectDraw7_Release(lpDD);
            return FALSE;
        }
    }

    rc = IDirect3D7_CreateDevice(lpD3D, &IID_IDirect3DTnLHalDevice, lpDDS,
        &lpD3DDevice);
    ok(rc==D3D_OK || rc==DDERR_NOPALETTEATTACHED || rc==E_OUTOFMEMORY, "Got hr %#lx.\n", rc);
    if (!lpD3DDevice) {
        trace("IDirect3D7::CreateDevice() for a TnL Hal device failed with an error %#lx, trying HAL\n", rc);
        rc = IDirect3D7_CreateDevice(lpD3D, &IID_IDirect3DHALDevice, lpDDS,
            &lpD3DDevice);
        if (!lpD3DDevice) {
            trace("IDirect3D7::CreateDevice() for a HAL device failed with an error %#lx, trying RGB\n", rc);
            rc = IDirect3D7_CreateDevice(lpD3D, &IID_IDirect3DRGBDevice, lpDDS,
                &lpD3DDevice);
            if (!lpD3DDevice) {
                trace("IDirect3D7::CreateDevice() for a RGB device failed with an error %#lx, giving up\n", rc);
                if (lpDDSdepth)
                    IDirectDrawSurface7_Release(lpDDSdepth);
                IDirectDrawSurface7_Release(lpDDS);
                IDirect3D7_Release(lpD3D);
                IDirectDraw7_Release(lpDD);
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
        IDirectDraw7_Release(lpDD);
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
    light.dcvDiffuse.r = 0.5f;
    light.dcvDiffuse.g = 0.6f;
    light.dcvDiffuse.b = 0.7f;
    light.dvDirection.y = 1.f;

    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 5, &light);
    ok(rc==D3D_OK, "Got hr %#lx.\n", rc);
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 10, &light);
    ok(rc==D3D_OK, "Got hr %#lx.\n", rc);
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 45, &light);
    ok(rc==D3D_OK, "Got hr %#lx.\n", rc);


    /* Try to retrieve a light beyond the indices of the lights that have
       been set. */
    rc = IDirect3DDevice7_GetLight(lpD3DDevice, 50, &light);
    ok(rc==DDERR_INVALIDPARAMS, "Got hr %#lx.\n", rc);
    rc = IDirect3DDevice7_GetLight(lpD3DDevice, 2, &light);
    ok(rc==DDERR_INVALIDPARAMS, "Got hr %#lx.\n", rc);


    /* Try to retrieve one of the lights that have been set */
    rc = IDirect3DDevice7_GetLight(lpD3DDevice, 10, &light);
    ok(rc==D3D_OK, "Got hr %#lx.\n", rc);


    /* Enable a light that have been previously set. */
    rc = IDirect3DDevice7_LightEnable(lpD3DDevice, 10, TRUE);
    ok(rc==D3D_OK, "Got hr %#lx.\n", rc);


    /* Enable some lights that have not been previously set, and verify that
       they have been initialized with proper default values. */
    memset(&defaultlight, 0, sizeof(D3DLIGHT7));
    defaultlight.dltType = D3DLIGHT_DIRECTIONAL;
    defaultlight.dcvDiffuse.r = 1.f;
    defaultlight.dcvDiffuse.g = 1.f;
    defaultlight.dcvDiffuse.b = 1.f;
    defaultlight.dvDirection.z = 1.f;

    rc = IDirect3DDevice7_LightEnable(lpD3DDevice, 20, TRUE);
    ok(rc==D3D_OK, "Got hr %#lx.\n", rc);
    memset(&light, 0, sizeof(D3DLIGHT7));
    rc = IDirect3DDevice7_GetLight(lpD3DDevice, 20, &light);
    ok(rc==D3D_OK, "Got hr %#lx.\n", rc);
    ok(!memcmp(&light, &defaultlight, sizeof(D3DLIGHT7)),
        "light data doesn't match expected default values\n" );

    rc = IDirect3DDevice7_LightEnable(lpD3DDevice, 50, TRUE);
    ok(rc==D3D_OK, "Got hr %#lx.\n", rc);
    memset(&light, 0, sizeof(D3DLIGHT7));
    rc = IDirect3DDevice7_GetLight(lpD3DDevice, 50, &light);
    ok(rc==D3D_OK, "Got hr %#lx.\n", rc);
    ok(!memcmp(&light, &defaultlight, sizeof(D3DLIGHT7)),
        "light data doesn't match expected default values\n" );


    /* Disable one of the light that have been previously enabled. */
    rc = IDirect3DDevice7_LightEnable(lpD3DDevice, 20, FALSE);
    ok(rc==D3D_OK, "Got hr %#lx.\n", rc);

    /* Try to retrieve the enable status of some lights */
    /* Light 20 is supposed to be disabled */
    rc = IDirect3DDevice7_GetLightEnable(lpD3DDevice, 20, &bEnabled );
    ok(rc==D3D_OK, "Got hr %#lx.\n", rc);
    ok(!bEnabled, "GetLightEnable says the light is enabled\n");

    /* Light 10 is supposed to be enabled */
    bEnabled = FALSE;
    rc = IDirect3DDevice7_GetLightEnable(lpD3DDevice, 10, &bEnabled );
    ok(rc==D3D_OK, "Got hr %#lx.\n", rc);
    ok(bEnabled, "GetLightEnable says the light is disabled\n");

    /* Light 80 has not been set */
    rc = IDirect3DDevice7_GetLightEnable(lpD3DDevice, 80, &bEnabled );
    ok(rc==DDERR_INVALIDPARAMS, "Got hr %#lx.\n", rc);

    /* Light 23 has not been set */
    rc = IDirect3DDevice7_GetLightEnable(lpD3DDevice, 23, &bEnabled );
    ok(rc==DDERR_INVALIDPARAMS, "Got hr %#lx.\n", rc);

    /* Set some lights with invalid parameters */
    memset(&light, 0, sizeof(D3DLIGHT7));
    light.dltType = 0;
    light.dcvDiffuse.r = 1.f;
    light.dcvDiffuse.g = 1.f;
    light.dcvDiffuse.b = 1.f;
    light.dvDirection.z = 1.f;
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 100, &light);
    ok(rc==DDERR_INVALIDPARAMS, "Got hr %#lx.\n", rc);

    memset(&light, 0, sizeof(D3DLIGHT7));
    light.dltType = 12345;
    light.dcvDiffuse.r = 1.f;
    light.dcvDiffuse.g = 1.f;
    light.dcvDiffuse.b = 1.f;
    light.dvDirection.z = 1.f;
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 101, &light);
    ok(rc==DDERR_INVALIDPARAMS, "Got hr %#lx.\n", rc);

    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 102, NULL);
    ok(rc==DDERR_INVALIDPARAMS, "Got hr %#lx.\n", rc);

    memset(&light, 0, sizeof(D3DLIGHT7));
    light.dltType = D3DLIGHT_SPOT;
    light.dcvDiffuse.r = 1.f;
    light.dcvDiffuse.g = 1.f;
    light.dcvDiffuse.b = 1.f;
    light.dvDirection.z = 1.f;

    light.dvAttenuation0 = -one / zero; /* -INFINITY */
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 103, &light);
    ok(rc==DDERR_INVALIDPARAMS, "Got hr %#lx.\n", rc);

    light.dvAttenuation0 = -1.0;
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 103, &light);
    ok(rc==DDERR_INVALIDPARAMS, "Got hr %#lx.\n", rc);

    light.dvAttenuation0 = 0.0;
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 103, &light);
    ok(rc==D3D_OK, "Got hr %#lx.\n", rc);

    light.dvAttenuation0 = 1.0;
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 103, &light);
    ok(rc==D3D_OK, "Got hr %#lx.\n", rc);

    light.dvAttenuation0 = one / zero; /* +INFINITY */
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 103, &light);
    ok(rc==D3D_OK, "Got hr %#lx.\n", rc);

    light.dvAttenuation0 = zero / zero; /* NaN */
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 103, &light);
    ok(rc==D3D_OK ||
       broken(rc==DDERR_INVALIDPARAMS), "Got hr %#lx.\n", rc);

    /* Directional light ignores attenuation */
    light.dltType = D3DLIGHT_DIRECTIONAL;
    light.dvAttenuation0 = -1.0;
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 103, &light);
    ok(rc==D3D_OK, "Got hr %#lx.\n", rc);

    memset(&mat, 0, sizeof(mat));
    rc = IDirect3DDevice7_SetMaterial(lpD3DDevice, &mat);
    ok(rc == D3D_OK, "Got hr %#lx.\n", rc);

    mat.power = 129.0;
    rc = IDirect3DDevice7_SetMaterial(lpD3DDevice, &mat);
    ok(rc == D3D_OK, "Got hr %#lx.\n", rc);
    memset(&mat, 0, sizeof(mat));
    rc = IDirect3DDevice7_GetMaterial(lpD3DDevice, &mat);
    ok(rc == D3D_OK, "Got hr %#lx.\n", rc);
    ok(mat.power == 129, "Returned power is %f\n", mat.power);

    mat.power = -1.0;
    rc = IDirect3DDevice7_SetMaterial(lpD3DDevice, &mat);
    ok(rc == D3D_OK, "Got hr %#lx.\n", rc);
    memset(&mat, 0, sizeof(mat));
    rc = IDirect3DDevice7_GetMaterial(lpD3DDevice, &mat);
    ok(rc == D3D_OK, "Got hr %#lx.\n", rc);
    ok(mat.power == -1, "Returned power is %f\n", mat.power);

    memset(&caps, 0, sizeof(caps));
    rc = IDirect3DDevice7_GetCaps(lpD3DDevice, &caps);
    ok(rc == D3D_OK, "Got hr %#lx.\n", rc);

    if ( caps.dwMaxActiveLights == (DWORD) -1) {
        /* Some cards without T&L Support return -1 (Examples: Voodoo Banshee, RivaTNT / NV4) */
        skip("T&L not supported\n");
        return;
    }

    for(i = 1; i <= caps.dwMaxActiveLights; i++) {
        rc = IDirect3DDevice7_LightEnable(lpD3DDevice, i, TRUE);
        ok(rc == D3D_OK, "Enabling light %u failed with %#lx\n", i, rc);
        rc = IDirect3DDevice7_GetLightEnable(lpD3DDevice, i, &enabled);
        ok(rc == D3D_OK, "GetLightEnable on light %u failed with %#lx\n", i, rc);
        ok(enabled, "Light %d is %s\n", i, enabled ? "enabled" : "disabled");
    }

    /* TODO: Test the rendering results in this situation */
    rc = IDirect3DDevice7_LightEnable(lpD3DDevice, i + 1, TRUE);
    ok(rc == D3D_OK, "Got hr %#lx.\n", rc);
    rc = IDirect3DDevice7_GetLightEnable(lpD3DDevice, i + 1, &enabled);
    ok(rc == D3D_OK, "GetLightEnable on light %u failed with %#lx\n", i + 1,  rc);
    ok(enabled, "Light %d is %s\n", i + 1, enabled ? "enabled" : "disabled");
    rc = IDirect3DDevice7_LightEnable(lpD3DDevice, i + 1, FALSE);
    ok(rc == D3D_OK, "Got hr %#lx.\n", rc);

    for(i = 1; i <= caps.dwMaxActiveLights; i++) {
        rc = IDirect3DDevice7_LightEnable(lpD3DDevice, i, FALSE);
        ok(rc == D3D_OK, "Disabling light %u failed with %#lx\n", i, rc);
    }
}

static void SceneTest(void)
{
    HRESULT                      hr;

    /* Test an EndScene without BeginScene. Should return an error */
    hr = IDirect3DDevice7_EndScene(lpD3DDevice);
    ok(hr == D3DERR_SCENE_NOT_IN_SCENE, "Got hr %#lx.\n", hr);

    /* Test a normal BeginScene / EndScene pair, this should work */
    hr = IDirect3DDevice7_BeginScene(lpD3DDevice);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IDirect3DDevice7_EndScene(lpD3DDevice);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    }

    if (lpDDSdepth)
    {
        DDBLTFX fx;
        memset(&fx, 0, sizeof(fx));
        fx.dwSize = sizeof(fx);

        hr = IDirectDrawSurface7_Blt(lpDDSdepth, NULL, NULL, NULL, DDBLT_DEPTHFILL, &fx);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

        hr = IDirect3DDevice7_BeginScene(lpD3DDevice);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
        {
            hr = IDirectDrawSurface7_Blt(lpDDSdepth, NULL, NULL, NULL, DDBLT_DEPTHFILL, &fx);
            ok(hr == D3D_OK || broken(hr == E_FAIL), "Got hr %#lx.\n", hr);
            hr = IDirect3DDevice7_EndScene(lpD3DDevice);
            ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        }
    }
    else
    {
        skip("Depth stencil creation failed at startup, skipping depthfill test\n");
    }

    /* Test another EndScene without having begun a new scene. Should return an error */
    hr = IDirect3DDevice7_EndScene(lpD3DDevice);
    ok(hr == D3DERR_SCENE_NOT_IN_SCENE, "Got hr %#lx.\n", hr);

    /* Two nested BeginScene and EndScene calls */
    hr = IDirect3DDevice7_BeginScene(lpD3DDevice);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice7_BeginScene(lpD3DDevice);
    ok(hr == D3DERR_SCENE_IN_SCENE, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice7_EndScene(lpD3DDevice);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice7_EndScene(lpD3DDevice);
    ok(hr == D3DERR_SCENE_NOT_IN_SCENE, "Got hr %#lx.\n", hr);

    /* TODO: Verify that blitting works in the same way as in d3d9 */
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
    ok(hr == DDERR_INVALIDPARAMS, "Got hr %#lx.\n", hr);

    memset(&d3d7et, 0, sizeof(d3d7et));
    hr = IDirect3D7_EnumDevices(lpD3D, enumDevicesCallbackTest7, &d3d7et);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    /* A couple of games (Delta Force LW and TFD) rely on this behaviour */
    ok(d3d7et.tnlhal < d3d7et.total, "TnLHal device enumerated as only device.\n");

    /* We make two additional assumptions. */
    ok(d3d7et.rgb, "No RGB Device enumerated.\n");

    if(d3d7et.tnlhal)
        ok(d3d7et.hal, "TnLHal device enumerated, but no Hal device found.\n");

    d3d7_cancel_test.desired_ret = DDENUMRET_CANCEL;
    d3d7_cancel_test.total = 0;
    hr = IDirect3D7_EnumDevices(lpD3D, enumDevicesCancelTest7, &d3d7_cancel_test);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    ok(d3d7_cancel_test.total == 1, "Enumerated a total of %u devices\n",
       d3d7_cancel_test.total);

    /* An enumeration callback can return any value besides DDENUMRET_OK to stop enumeration. */
    d3d7_cancel_test.desired_ret = E_INVALIDARG;
    d3d7_cancel_test.total = 0;
    hr = IDirect3D7_EnumDevices(lpD3D, enumDevicesCancelTest7, &d3d7_cancel_test);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

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
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

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
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

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
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

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
    ok(hr==DD_OK || hr==DDERR_NODIRECTDRAWSUPPORT, "Got hr %#lx.\n", hr);
    if (!DirectDraw1) {
        return FALSE;
    }

    hr = IDirectDraw_SetCooperativeLevel(DirectDraw1, NULL, DDSCL_NORMAL);
    ok(hr==DD_OK, "Got hr %#lx.\n", hr);

    hr = IDirectDraw_QueryInterface(DirectDraw1, &IID_IDirect3D, (void**) &Direct3D1);
    if (hr == E_NOINTERFACE) return FALSE;
    ok(hr==DD_OK, "Got hr %#lx.\n", hr);
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
    ok(hr==D3D_OK || hr==DDERR_NOPALETTEATTACHED || hr==E_OUTOFMEMORY, "Got hr %#lx.\n", hr);
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
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    if(!ExecuteBuffer) {
        return FALSE;
    }

    hr = IDirect3D_CreateViewport(Direct3D1, &Viewport, NULL);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    if(!Viewport) {
        return FALSE;
    }

    hr = IDirect3DViewport_Initialize(Viewport, Direct3D1);
    ok(hr == DDERR_ALREADYINITIALIZED, "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice_AddViewport(Direct3DDevice1, Viewport);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
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
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    hr = IDirect3D_CreateLight(Direct3D1, &Light, NULL);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
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
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    hr = IDirect3DViewport_QueryInterface(Viewport, &IID_IDirect3DViewport2, (void**) &Viewport2);
    ok(hr==D3D_OK, "Got hr %#lx.\n", hr);
    ok(Viewport2 == (IDirect3DViewport2 *)Viewport, "IDirect3DViewport2 iface different from IDirect3DViewport\n");

    hr = IDirect3DViewport_QueryInterface(Viewport, &IID_IDirect3DViewport3, (void**) &Viewport3);
    ok(hr==D3D_OK, "Got hr %#lx.\n", hr);
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
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    memset(&ret_vp1_data, 0xff, sizeof(ret_vp1_data));
    ret_vp1_data.dwSize = sizeof(vp1_data);

    hr = IDirect3DViewport2_GetViewport(Viewport2, &ret_vp1_data);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    ok(ret_vp1_data.dwX == vp1_data.dwX, "dwX is %lu, expected %lu\n", ret_vp1_data.dwX, vp1_data.dwX);
    ok(ret_vp1_data.dwY == vp1_data.dwY, "dwY is %lu, expected %lu\n", ret_vp1_data.dwY, vp1_data.dwY);
    ok(ret_vp1_data.dwWidth == vp1_data.dwWidth, "dwWidth is %lu, expected %lu\n", ret_vp1_data.dwWidth, vp1_data.dwWidth);
    ok(ret_vp1_data.dwHeight == vp1_data.dwHeight, "dwHeight is %lu, expected %lu\n", ret_vp1_data.dwHeight, vp1_data.dwHeight);
    ok(ret_vp1_data.dvMaxX == vp1_data.dvMaxX, "dvMaxX is %f, expected %f\n", ret_vp1_data.dvMaxX, vp1_data.dvMaxX);
    ok(ret_vp1_data.dvMaxY == vp1_data.dvMaxY, "dvMaxY is %f, expected %f\n", ret_vp1_data.dvMaxY, vp1_data.dvMaxY);
    todo_wine ok(ret_vp1_data.dvScaleX == infinity, "dvScaleX is %f, expected %f\n", ret_vp1_data.dvScaleX, infinity);
    todo_wine ok(ret_vp1_data.dvScaleY == infinity, "dvScaleY is %f, expected %f\n", ret_vp1_data.dvScaleY, infinity);
    ok(ret_vp1_data.dvMinZ == 0.0, "dvMinZ is %f, expected 0.0\n", ret_vp1_data.dvMinZ);
    ok(ret_vp1_data.dvMaxZ == 1.0, "dvMaxZ is %f, expected 1.0\n", ret_vp1_data.dvMaxZ);

    hr = IDirect3DViewport2_SetViewport2(Viewport2, &vp2_data);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    memset(&ret_vp2_data, 0xff, sizeof(ret_vp2_data));
    ret_vp2_data.dwSize = sizeof(vp2_data);

    hr = IDirect3DViewport2_GetViewport2(Viewport2, &ret_vp2_data);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    ok(ret_vp2_data.dwX == vp2_data.dwX, "dwX is %lu, expected %lu\n", ret_vp2_data.dwX, vp2_data.dwX);
    ok(ret_vp2_data.dwY == vp2_data.dwY, "dwY is %lu, expected %lu\n", ret_vp2_data.dwY, vp2_data.dwY);
    ok(ret_vp2_data.dwWidth == vp2_data.dwWidth, "dwWidth is %lu, expected %lu\n", ret_vp2_data.dwWidth, vp2_data.dwWidth);
    ok(ret_vp2_data.dwHeight == vp2_data.dwHeight, "dwHeight is %lu, expected %lu\n", ret_vp2_data.dwHeight, vp2_data.dwHeight);
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
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    ok(ret_vp1_data.dwX == vp2_data.dwX, "dwX is %lu, expected %lu.\n", ret_vp1_data.dwX, vp2_data.dwX);
    ok(ret_vp1_data.dwY == vp2_data.dwY, "dwY is %lu, expected %lu.\n", ret_vp1_data.dwY, vp2_data.dwY);
    ok(ret_vp1_data.dwWidth == vp2_data.dwWidth, "dwWidth is %lu, expected %lu.\n", ret_vp1_data.dwWidth, vp2_data.dwWidth);
    ok(ret_vp1_data.dwHeight == vp2_data.dwHeight, "dwHeight is %lu, expected %lu.\n", ret_vp1_data.dwHeight, vp2_data.dwHeight);
    ok(ret_vp1_data.dvMaxX == vp1_data.dvMaxX, "dvMaxX is %f, expected %f.\n", ret_vp1_data.dvMaxX, vp1_data.dvMaxX);
    ok(ret_vp1_data.dvMaxY == vp1_data.dvMaxY, "dvMaxY is %f, expected %f.\n", ret_vp1_data.dvMaxY, vp1_data.dvMaxY);
    ok(ret_vp1_data.dvScaleX == infinity, "dvScaleX is %f, expected %f.\n", ret_vp1_data.dvScaleX, infinity);
    ok(ret_vp1_data.dvScaleY == infinity, "dvScaleY is %f, expected %f.\n", ret_vp1_data.dvScaleY, infinity);
    ok(ret_vp1_data.dvMinZ == 0.0, "dvMinZ is %f, expected 0.0.\n", ret_vp1_data.dvMinZ);
    ok(ret_vp1_data.dvMaxZ == 1.0, "dvMaxZ is %f, expected 1.0.\n", ret_vp1_data.dvMaxZ);

    hr = IDirect3DViewport2_SetViewport2(Viewport2, &vp2_data);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    memset(&ret_vp2_data, 0xff, sizeof(ret_vp2_data));
    ret_vp2_data.dwSize = sizeof(vp2_data);

    hr = IDirect3DViewport2_GetViewport2(Viewport2, &ret_vp2_data);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    ok(ret_vp2_data.dwX == vp2_data.dwX, "dwX is %lu, expected %lu\n", ret_vp2_data.dwX, vp2_data.dwX);
    ok(ret_vp2_data.dwY == vp2_data.dwY, "dwY is %lu, expected %lu\n", ret_vp2_data.dwY, vp2_data.dwY);
    ok(ret_vp2_data.dwWidth == vp2_data.dwWidth, "dwWidth is %lu, expected %lu\n", ret_vp2_data.dwWidth, vp2_data.dwWidth);
    ok(ret_vp2_data.dwHeight == vp2_data.dwHeight, "dwHeight is %lu, expected %lu\n", ret_vp2_data.dwHeight, vp2_data.dwHeight);
    ok(ret_vp2_data.dvClipX == vp2_data.dvClipX, "dvClipX is %f, expected %f\n", ret_vp2_data.dvClipX, vp2_data.dvClipX);
    ok(ret_vp2_data.dvClipY == vp2_data.dvClipY, "dvClipY is %f, expected %f\n", ret_vp2_data.dvClipY, vp2_data.dvClipY);
    ok(ret_vp2_data.dvClipWidth == vp2_data.dvClipWidth, "dvClipWidth is %f, expected %f\n",
        ret_vp2_data.dvClipWidth, vp2_data.dvClipWidth);
    ok(ret_vp2_data.dvClipHeight == vp2_data.dvClipHeight, "dvClipHeight is %f, expected %f\n",
        ret_vp2_data.dvClipHeight, vp2_data.dvClipHeight);
    ok(ret_vp2_data.dvMinZ == vp2_data.dvMinZ, "dvMinZ is %f, expected %f\n", ret_vp2_data.dvMinZ, vp2_data.dvMinZ);
    ok(ret_vp2_data.dvMaxZ == vp2_data.dvMaxZ, "dvMaxZ is %f, expected %f\n", ret_vp2_data.dvMaxZ, vp2_data.dvMaxZ);

    hr = IDirect3DViewport2_SetViewport(Viewport2, &vp1_data);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    memset(&ret_vp1_data, 0xff, sizeof(ret_vp1_data));
    ret_vp1_data.dwSize = sizeof(vp1_data);

    hr = IDirect3DViewport2_GetViewport(Viewport2, &ret_vp1_data);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    ok(ret_vp1_data.dwX == vp1_data.dwX, "dwX is %lu, expected %lu\n", ret_vp1_data.dwX, vp1_data.dwX);
    ok(ret_vp1_data.dwY == vp1_data.dwY, "dwY is %lu, expected %lu\n", ret_vp1_data.dwY, vp1_data.dwY);
    ok(ret_vp1_data.dwWidth == vp1_data.dwWidth, "dwWidth is %lu, expected %lu\n", ret_vp1_data.dwWidth, vp1_data.dwWidth);
    ok(ret_vp1_data.dwHeight == vp1_data.dwHeight, "dwHeight is %lu, expected %lu\n", ret_vp1_data.dwHeight, vp1_data.dwHeight);
    ok(ret_vp1_data.dvMaxX == vp1_data.dvMaxX, "dvMaxX is %f, expected %f\n", ret_vp1_data.dvMaxX, vp1_data.dvMaxX);
    ok(ret_vp1_data.dvMaxY == vp1_data.dvMaxY, "dvMaxY is %f, expected %f\n", ret_vp1_data.dvMaxY, vp1_data.dvMaxY);
    todo_wine ok(ret_vp1_data.dvScaleX == infinity, "dvScaleX is %f, expected %f\n", ret_vp1_data.dvScaleX, infinity);
    todo_wine ok(ret_vp1_data.dvScaleY == infinity, "dvScaleY is %f, expected %f\n", ret_vp1_data.dvScaleY, infinity);
    ok(ret_vp1_data.dvMinZ == 0.0, "dvMinZ is %f, expected 0.0\n", ret_vp1_data.dvMinZ);
    ok(ret_vp1_data.dvMaxZ == 1.0, "dvMaxZ is %f, expected 1.0\n", ret_vp1_data.dvMaxZ);

    memset(&ret_vp2_data, 0xff, sizeof(ret_vp2_data));
    ret_vp2_data.dwSize = sizeof(vp2_data);

    hr = IDirect3DViewport2_GetViewport2(Viewport2, &ret_vp2_data);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    ok(ret_vp2_data.dwX == vp1_data.dwX, "dwX is %lu, expected %lu.\n", ret_vp2_data.dwX, vp1_data.dwX);
    ok(ret_vp2_data.dwY == vp1_data.dwY, "dwY is %lu, expected %lu.\n", ret_vp2_data.dwY, vp1_data.dwY);
    ok(ret_vp2_data.dwWidth == vp1_data.dwWidth, "dwWidth is %lu, expected %lu.\n",
            ret_vp2_data.dwWidth, vp1_data.dwWidth);
    ok(ret_vp2_data.dwHeight == vp1_data.dwHeight, "dwHeight is %lu, expected %lu.\n",
            ret_vp2_data.dwHeight, vp1_data.dwHeight);
    todo_wine ok(ret_vp2_data.dvClipX == vp2_data.dvClipX, "dvClipX is %f, expected %f.\n",
            ret_vp2_data.dvClipX, vp2_data.dvClipX);
    todo_wine ok(ret_vp2_data.dvClipY == vp2_data.dvClipY, "dvClipY is %f, expected %f.\n",
            ret_vp2_data.dvClipY, vp2_data.dvClipY);
    todo_wine ok(ret_vp2_data.dvClipWidth == vp2_data.dvClipWidth, "dvClipWidth is %f, expected %f.\n",
        ret_vp2_data.dvClipWidth, vp2_data.dvClipWidth);
    todo_wine ok(ret_vp2_data.dvClipHeight == vp2_data.dvClipHeight, "dvClipHeight is %f, expected %f.\n",
        ret_vp2_data.dvClipHeight, vp2_data.dvClipHeight);
    ok(ret_vp2_data.dvMinZ == 0.0, "dvMinZ is %f, expected 0.0.\n", ret_vp2_data.dvMinZ);
    ok(ret_vp2_data.dvMaxZ == 1.0, "dvMaxZ is %f, expected 1.0.\n", ret_vp2_data.dvMaxZ);

    IDirect3DViewport2_Release(Viewport2);

    hr = IDirect3DDevice_DeleteViewport(Direct3DDevice1, Viewport);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
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
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    ok(Direct3D_alt == Direct3D1, "Direct3D1 struct pointer mismatch: %p != %p\n", Direct3D_alt, Direct3D1);
    IDirect3D_Release(Direct3D_alt);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    hr = IDirect3DExecuteBuffer_Lock(ExecuteBuffer, &desc);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

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
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice_Execute(Direct3DDevice1, ExecuteBuffer, Viewport, D3DEXECUTE_CLIPPED);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);

    hr = IDirect3DExecuteBuffer_Lock(ExecuteBuffer, &desc);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

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
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice_Execute(Direct3DDevice1, ExecuteBuffer, Viewport, D3DEXECUTE_CLIPPED);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    /* Test rendering 0 triangles */
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);

    hr = IDirect3DExecuteBuffer_Lock(ExecuteBuffer, &desc);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

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
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice_Execute(Direct3DDevice1, ExecuteBuffer, Viewport, D3DEXECUTE_CLIPPED);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice_DeleteViewport(Direct3DDevice1, Viewport);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    hr = IDirect3DViewport_AddLight(Viewport, Light);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    refcount = getRefcount((IUnknown*) Light);
    ok(refcount == 2, "Refcount should be 2, returned is %ld\n", refcount);

    hr = IDirect3DViewport_NextLight(Viewport, NULL, &d3dlight, D3DNEXT_HEAD);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    ok(d3dlight == Light, "Got different light returned %p, expected %p\n", d3dlight, Light);
    refcount = getRefcount((IUnknown*) Light);
    ok(refcount == 3, "Refcount should be 2, returned is %ld\n", refcount);

    hr = IDirect3DViewport_DeleteLight(Viewport, Light);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    refcount = getRefcount((IUnknown*) Light);
    ok(refcount == 2, "Refcount should be 2, returned is %ld\n", refcount);

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
    ddsd.ddpfPixelFormat.dwRGBBitCount = 8;

    hr = IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &TexSurface, NULL);
    ok(hr==D3D_OK, "Got hr %#lx.\n", hr);
    if (FAILED(hr)) {
        skip("IDirectDraw_CreateSurface failed; skipping further tests\n");
        goto cleanup;
    }

    hr = IDirectDrawSurface_QueryInterface(TexSurface, &IID_IDirect3DTexture,
                (void *)&Texture);
    ok(hr==D3D_OK, "Got hr %#lx.\n", hr);
    if (FAILED(hr)) {
        skip("Can't get IDirect3DTexture interface; skipping further tests\n");
        goto cleanup;
    }

    hr = IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &TexSurface2, NULL);
    ok(hr==D3D_OK, "Got hr %#lx.\n", hr);
    if (FAILED(hr)) {
        skip("IDirectDraw_CreateSurface failed; skipping further tests\n");
        goto cleanup;
    }

    hr = IDirectDrawSurface_QueryInterface(TexSurface2, &IID_IDirect3DTexture,
                (void *)&Texture2);
    ok(hr==D3D_OK, "Got hr %#lx.\n", hr);
    if (FAILED(hr)) {
        skip("Can't get IDirect3DTexture interface; skipping further tests\n");
        goto cleanup;
    }

    /* test load of Texture to Texture */
    hr = IDirect3DTexture_Load(Texture, Texture);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    /* test Load when both textures have no palette */
    hr = IDirect3DTexture_Load(Texture2, Texture);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    for (i = 0; i < 256; i++) {
        table1[i].peRed = i;
        table1[i].peGreen = i;
        table1[i].peBlue = i;
        table1[i].peFlags = 0;
    }

    hr = IDirectDraw_CreatePalette(DirectDraw1, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, table1, &palette, NULL);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    if (FAILED(hr)) {
        skip("IDirectDraw_CreatePalette failed; skipping further tests\n");
        goto cleanup;
    }

    /* test Load when source texture has palette and destination has no palette */
    hr = IDirectDrawSurface_SetPalette(TexSurface, palette);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DTexture_Load(Texture2, Texture);
    ok(hr == DDERR_NOPALETTEATTACHED, "Got hr %#lx.\n", hr);

    for (i = 0; i < 256; i++) {
        table2[i].peRed = 255 - i;
        table2[i].peGreen = 255 - i;
        table2[i].peBlue = 255 - i;
        table2[i].peFlags = 0;
    }

    hr = IDirectDraw_CreatePalette(DirectDraw1, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, table2, &palette2, NULL);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    if (FAILED(hr)) {
        skip("IDirectDraw_CreatePalette failed; skipping further tests\n");
        goto cleanup;
    }

    /* test Load when source has no palette and destination has a palette */
    hr = IDirectDrawSurface_SetPalette(TexSurface, NULL);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    hr = IDirectDrawSurface_SetPalette(TexSurface2, palette2);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DTexture_Load(Texture2, Texture);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    hr = IDirectDrawSurface_GetPalette(TexSurface2, &palette_tmp);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    if (!palette_tmp) {
        skip("IDirectDrawSurface_GetPalette failed; skipping color table check\n");
        goto cleanup;
    } else {
        hr = IDirectDrawPalette_GetEntries(palette_tmp, 0, 0, 256, table_tmp);
        ok(hr == DD_OK, "Got hr %#lx.\n", hr);
        ok(colortables_check_equality(table2, table_tmp), "Unexpected palettized texture color table\n");
        IDirectDrawPalette_Release(palette_tmp);
    }

    /* test Load when both textures have palettes */
    hr = IDirectDrawSurface_SetPalette(TexSurface, palette);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DTexture_Load(Texture2, Texture);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DTexture_Load(Texture2, Texture);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    hr = IDirectDrawSurface_GetPalette(TexSurface2, &palette_tmp);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    if (!palette_tmp) {
        skip("IDirectDrawSurface_GetPalette failed; skipping color table check\n");
        goto cleanup;
    } else {
        hr = IDirectDrawPalette_GetEntries(palette_tmp, 0, 0, 256, table_tmp);
        ok(hr == DD_OK, "Got hr %#lx.\n", hr);
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
    ok(rc==D3D_OK || rc==E_OUTOFMEMORY, "Got hr %#lx.\n", rc);
    if (!lpVBufSrc)
    {
        trace("Failed to create vertex buffer, hr %#lx.\n", rc);
        goto out;
    }

    memset(mem.buffer, 0x12, sizeof(mem.buffer));
    mem.desc2.dwSize = sizeof(D3DVERTEXBUFFERDESC)*2;
    rc = IDirect3DVertexBuffer7_GetVertexBufferDesc(lpVBufSrc, &mem.desc2);
    if(rc != D3D_OK)
        skip("GetVertexBuffer Failed!\n");
    ok( mem.desc2.dwSize == sizeof(D3DVERTEXBUFFERDESC)*2, "Size returned from GetVertexBufferDesc does not match the value put in\n" );
    ok( mem.buffer[sizeof(D3DVERTEXBUFFERDESC)] == 0x12, "GetVertexBufferDesc cleared outside of the struct! (dwSize was double the size of the struct)\n");
    ok( mem.desc2.dwCaps == desc.dwCaps, "dwCaps returned differs. Got %#lx, expected %#lx\n", mem.desc2.dwCaps, desc.dwCaps);
    ok( mem.desc2.dwFVF == desc.dwFVF, "dwFVF returned differs. Got %#lx, expected %#lx\n", mem.desc2.dwFVF, desc.dwFVF);
    ok (mem.desc2.dwNumVertices == desc.dwNumVertices, "dwNumVertices returned differs. Got %#lx, expected %#lx\n", mem.desc2.dwNumVertices, desc.dwNumVertices);

    memset(mem.buffer, 0x12, sizeof(mem.buffer));
    mem.desc2.dwSize = 0;
    rc = IDirect3DVertexBuffer7_GetVertexBufferDesc(lpVBufSrc, &mem.desc2);
    if(rc != D3D_OK)
        skip("GetVertexBuffer Failed!\n");
    ok( mem.desc2.dwSize == 0, "Size returned from GetVertexBufferDesc does not match the value put in\n" );
    ok( mem.buffer[sizeof(D3DVERTEXBUFFERDESC)] == 0x12, "GetVertexBufferDesc cleared outside of the struct! (dwSize was 0)\n");
    ok( mem.desc2.dwCaps == desc.dwCaps, "dwCaps returned differs. Got %#lx, expected %#lx\n", mem.desc2.dwCaps, desc.dwCaps);
    ok( mem.desc2.dwFVF == desc.dwFVF, "dwFVF returned differs. Got %#lx, expected %#lx\n", mem.desc2.dwFVF, desc.dwFVF);
    ok (mem.desc2.dwNumVertices == desc.dwNumVertices, "dwNumVertices returned differs. Got %#lx, expected %#lx\n", mem.desc2.dwNumVertices, desc.dwNumVertices);

    memset(mem.buffer, 0x12, sizeof(mem.buffer));
    mem.desc2.dwSize = sizeof(D3DVERTEXBUFFERDESC);
    rc = IDirect3DVertexBuffer7_GetVertexBufferDesc(lpVBufSrc, &mem.desc2);
    if(rc != D3D_OK)
        skip("GetVertexBuffer Failed!\n");
    ok( mem.desc2.dwSize == sizeof(D3DVERTEXBUFFERDESC), "Size returned from GetVertexBufferDesc does not match the value put in\n" );
    ok( mem.buffer[sizeof(D3DVERTEXBUFFERDESC)] == 0x12, "GetVertexBufferDesc cleared outside of the struct! (dwSize was the size of the struct)\n");
    ok( mem.desc2.dwCaps == desc.dwCaps, "dwCaps returned differs. Got %#lx, expected %#lx\n", mem.desc2.dwCaps, desc.dwCaps);
    ok( mem.desc2.dwFVF == desc.dwFVF, "dwFVF returned differs. Got %#lx, expected %#lx\n", mem.desc2.dwFVF, desc.dwFVF);
    ok (mem.desc2.dwNumVertices == desc.dwNumVertices, "dwNumVertices returned differs. Got %#lx, expected %#lx\n", mem.desc2.dwNumVertices, desc.dwNumVertices);

out:
    IDirect3DVertexBuffer7_Release(lpVBufSrc);
}

static void SetMaterialTest(void)
{
    HRESULT rc;

    rc =IDirect3DDevice7_SetMaterial(lpD3DDevice, NULL);
    ok(rc == DDERR_INVALIDPARAMS, "Got hr %#lx.\n", rc);
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
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
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
    ddsd2.ddpfPixelFormat.dwSize = sizeof(ddsd2.ddpfPixelFormat);
    ddsd2.ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
    ddsd2.ddpfPixelFormat.dwZBufferBitDepth = 16;
    ddsd2.ddpfPixelFormat.dwZBitMask = 0x0000FFFF;

    hr = IDirectDraw7_CreateSurface(lpDD, &ddsd2, &failrt, NULL);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    memset(&vp, 0, sizeof(vp));
    vp.dwX = 10;
    vp.dwY = 10;
    vp.dwWidth = 246;
    vp.dwHeight = 246;
    vp.dvMinZ = 0.25;
    vp.dvMaxZ = 0.75;
    hr = IDirect3DDevice7_SetViewport(lpD3DDevice, &vp);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice7_GetRenderTarget(lpD3DDevice, &oldrt);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    refcount = getRefcount((IUnknown*) oldrt);
    ok(refcount == 3, "Refcount should be 3, returned is %ld\n", refcount);

    refcount = getRefcount((IUnknown*) failrt);
    ok(refcount == 1, "Refcount should be 1, returned is %ld\n", refcount);

    hr = IDirect3DDevice7_SetRenderTarget(lpD3DDevice, failrt, 0);
    ok(hr != D3D_OK, "IDirect3DDevice7_SetRenderTarget succeeded\n");

    refcount = getRefcount((IUnknown*) oldrt);
    ok(refcount == 2, "Refcount should be 2, returned is %ld\n", refcount);

    refcount = getRefcount((IUnknown*) failrt);
    ok(refcount == 2, "Refcount should be 2, returned is %ld\n", refcount);

    hr = IDirect3DDevice7_GetRenderTarget(lpD3DDevice, &temprt);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    ok(failrt == temprt, "Wrong iface returned\n");

    refcount = getRefcount((IUnknown*) failrt);
    ok(refcount == 3, "Refcount should be 3, returned is %ld\n", refcount);

    hr = IDirect3DDevice7_SetRenderTarget(lpD3DDevice, newrt, 0);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    refcount = getRefcount((IUnknown*) failrt);
    ok(refcount == 2, "Refcount should be 2, returned is %ld\n", refcount);

    memset(&vp, 0xff, sizeof(vp));
    hr = IDirect3DDevice7_GetViewport(lpD3DDevice, &vp);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    ok(vp.dwX == 10, "vp.dwX is %lu, expected 10\n", vp.dwX);
    ok(vp.dwY == 10, "vp.dwY is %lu, expected 10\n", vp.dwY);
    ok(vp.dwWidth == 246, "vp.dwWidth is %lu, expected 246\n", vp.dwWidth);
    ok(vp.dwHeight == 246, "vp.dwHeight is %lu, expected 246\n", vp.dwHeight);
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
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice7_BeginStateBlock(lpD3DDevice);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice7_SetRenderTarget(lpD3DDevice, oldrt, 0);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    /* Check this twice, before and after ending the stateblock */
    memset(&vp, 0xff, sizeof(vp));
    hr = IDirect3DDevice7_GetViewport(lpD3DDevice, &vp);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    ok(vp.dwX == 0, "vp.dwX is %lu, expected 0\n", vp.dwX);
    ok(vp.dwY == 0, "vp.dwY is %lu, expected 0\n", vp.dwY);
    ok(vp.dwWidth == 64, "vp.dwWidth is %lu, expected 64\n", vp.dwWidth);
    ok(vp.dwHeight == 64, "vp.dwHeight is %lu, expected 64\n", vp.dwHeight);
    ok(vp.dvMinZ == 0.0, "vp.dvMinZ is %f, expected 0.0\n", vp.dvMinZ);
    ok(vp.dvMaxZ == 1.0, "vp.dvMaxZ is %f, expected 1.0\n", vp.dvMaxZ);

    hr = IDirect3DDevice7_EndStateBlock(lpD3DDevice, &stateblock);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    memset(&vp, 0xff, sizeof(vp));
    hr = IDirect3DDevice7_GetViewport(lpD3DDevice, &vp);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    ok(vp.dwX == 0, "vp.dwX is %lu, expected 0\n", vp.dwX);
    ok(vp.dwY == 0, "vp.dwY is %lu, expected 0\n", vp.dwY);
    ok(vp.dwWidth == 64, "vp.dwWidth is %lu, expected 64\n", vp.dwWidth);
    ok(vp.dwHeight == 64, "vp.dwHeight is %lu, expected 64\n", vp.dwHeight);
    ok(vp.dvMinZ == 0.0, "vp.dvMinZ is %f, expected 0.0\n", vp.dvMinZ);
    ok(vp.dvMaxZ == 1.0, "vp.dvMaxZ is %f, expected 1.0\n", vp.dvMaxZ);

    hr = IDirect3DDevice7_DeleteStateBlock(lpD3DDevice, stateblock);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    memset(&vp, 0, sizeof(vp));
    vp.dwX = 0;
    vp.dwY = 0;
    vp.dwWidth = 256;
    vp.dwHeight = 256;
    vp.dvMinZ = 0.0;
    vp.dvMaxZ = 0.0;
    hr = IDirect3DDevice7_SetViewport(lpD3DDevice, &vp);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

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
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    for(i = 0; i < (sizeof(test_data) / sizeof(*test_data)); i++)
    {
        hr = IDirect3DVertexBuffer7_Lock(buffer, test_data[i].flags, &data, NULL);
        ok(hr == test_data[i].result, "Lock flags %s returned %#lx, expected %#lx\n",
            test_data[i].debug_string, hr, test_data[i].result);
        if(SUCCEEDED(hr))
        {
            ok(data != NULL, "The data pointer returned by Lock is NULL\n");
            hr = IDirect3DVertexBuffer7_Unlock(buffer);
            ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        }
    }

    IDirect3DVertexBuffer7_Release(buffer);
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
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
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
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
    if (surf != NULL)
    {
        hr = IDirectDrawSurface_GetSurfaceDesc(surf, &created_ddsd);
        ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
        ok(created_ddsd.ddsCaps.dwCaps == expected_caps,
                "GetSurfaceDesc returned caps %#lx.\n", created_ddsd.ddsCaps.dwCaps);

        hr = IDirectDrawSurface_QueryInterface(surf, &IID_IDirect3DHALDevice, (void **)&d3dhal);
        ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
        IDirect3DDevice_Release(d3dhal);

        IDirectDrawSurface_Release(surf);
    }

    hr = IDirectDraw_QueryInterface(DirectDraw1, &IID_IDirectDraw2, (void **) &dd2);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);

    hr = IDirectDraw2_CreateSurface(dd2, &ddsd, &surf, NULL);
    ok(hr == DDERR_INVALIDCAPS, "Got hr %#lx.\n", hr);

    IDirectDraw2_Release(dd2);

    hr = IDirectDraw_QueryInterface(DirectDraw1, &IID_IDirectDraw4, (void **) &dd4);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);

    hr = IDirectDraw4_CreateSurface(dd4, &ddsd2, &surf4, NULL);
    ok(hr == DDERR_INVALIDCAPS, "Got hr %#lx.\n", hr);

    IDirectDraw4_Release(dd4);

    hr = IDirectDraw_QueryInterface(DirectDraw1, &IID_IDirectDraw7, (void **) &dd7);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);

    hr = IDirectDraw7_CreateSurface(dd7, &ddsd2, &surf7, NULL);
    ok(hr == DDERR_INVALIDCAPS, "Got hr %#lx.\n", hr);

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
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    /* Perform attachment tests on a back-buffer */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER | DDSCAPS_3DDEVICE;
    ddsd.dwWidth = GetSystemMetrics(SM_CXSCREEN);
    ddsd.dwHeight = GetSystemMetrics(SM_CYSCREEN);
    hr = IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &surface2, NULL);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);

    if (surface2 != NULL)
    {
        /* Try a single primary and a two back buffers */
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        hr = IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &surface1, NULL);
        ok(hr == DD_OK, "Got hr %#lx.\n", hr);

        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER | DDSCAPS_3DDEVICE;
        ddsd.dwWidth = GetSystemMetrics(SM_CXSCREEN);
        ddsd.dwHeight = GetSystemMetrics(SM_CYSCREEN);
        hr = IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &surface3, NULL);
        ok(hr == DD_OK, "Got hr %#lx.\n", hr);

        /* This one has a different size */
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER | DDSCAPS_3DDEVICE;
        ddsd.dwWidth = 128;
        ddsd.dwHeight = 128;
        hr = IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &surface4, NULL);
        ok(hr == DD_OK, "Got hr %#lx.\n", hr);

        hr = IDirectDrawSurface_AddAttachedSurface(surface1, surface2);
        todo_wine ok(hr == DD_OK || broken(hr == DDERR_CANNOTATTACHSURFACE), "Got hr %#lx.\n", hr);
        if(SUCCEEDED(hr))
        {
            /* Try the reverse without detaching first */
            hr = IDirectDrawSurface_AddAttachedSurface(surface2, surface1);
            ok(hr == DDERR_SURFACEALREADYATTACHED, "Got hr %#lx.\n", hr);
            hr = IDirectDrawSurface_DeleteAttachedSurface(surface1, 0, surface2);
            ok(hr == DD_OK, "Got hr %#lx.\n", hr);
        }
        hr = IDirectDrawSurface_AddAttachedSurface(surface2, surface1);
        todo_wine ok(hr == DD_OK || broken(hr == DDERR_CANNOTATTACHSURFACE), "Got hr %#lx.\n", hr);
        if(SUCCEEDED(hr))
        {
            /* Try to detach reversed */
            hr = IDirectDrawSurface_DeleteAttachedSurface(surface1, 0, surface2);
            ok(hr == DDERR_CANNOTDETACHSURFACE, "Got hr %#lx.\n", hr);
            /* Now the proper detach */
            hr = IDirectDrawSurface_DeleteAttachedSurface(surface2, 0, surface1);
            ok(hr == DD_OK, "Got hr %#lx.\n", hr);
        }
        hr = IDirectDrawSurface_AddAttachedSurface(surface2, surface3);
        todo_wine ok(hr == DD_OK || broken(hr == DDERR_CANNOTATTACHSURFACE), "Got hr %#lx.\n", hr);
        if(SUCCEEDED(hr))
        {
            hr = IDirectDrawSurface_DeleteAttachedSurface(surface2, 0, surface3);
            ok(hr == DD_OK, "Got hr %#lx.\n", hr);
        }
        hr = IDirectDrawSurface_AddAttachedSurface(surface1, surface4);
        ok(hr == DDERR_CANNOTATTACHSURFACE, "Got hr %#lx.\n", hr);
        hr = IDirectDrawSurface_AddAttachedSurface(surface4, surface1);
        ok(hr == DDERR_CANNOTATTACHSURFACE, "Got hr %#lx.\n", hr);

        IDirectDrawSurface_Release(surface4);
        IDirectDrawSurface_Release(surface3);
        IDirectDrawSurface_Release(surface2);
        IDirectDrawSurface_Release(surface1);
    }

    hr =IDirectDraw_SetCooperativeLevel(DirectDraw1, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    DestroyWindow(window);
}

static void dump_format(const DDPIXELFORMAT *fmt)
{
    trace("dwFlags %08lx, FourCC %08lx, dwZBufferBitDepth %lu, stencil %08lx\n", fmt->dwFlags, fmt->dwFourCC,
          fmt->dwZBufferBitDepth, fmt->dwStencilBitDepth);
    trace("dwZBitMask %08lx, dwStencilBitMask %08lx, dwRGBZBitMask %08lx\n", fmt->dwZBitMask,
          fmt->dwStencilBitMask, fmt->dwRGBZBitMask);
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
    ddsd.ddpfPixelFormat = *fmt;
    ddsd.dwWidth = 1024;
    ddsd.dwHeight = 1024;
    hr = IDirectDraw7_CreateSurface(lpDD, &ddsd, &surface, NULL);
    ok(SUCCEEDED(hr), "IDirectDraw7_CreateSurface failed, hr %#lx.\n", hr);
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    hr = IDirectDrawSurface7_GetSurfaceDesc(surface, &ddsd);
    ok(SUCCEEDED(hr), "IDirectDrawSurface7_GetSurfaceDesc failed, hr %#lx.\n", hr);
    IDirectDrawSurface7_Release(surface);

    ok(ddsd.dwFlags & DDSD_PIXELFORMAT, "DDSD_PIXELFORMAT is not set\n");
    ok(!(ddsd.dwFlags & DDSD_ZBUFFERBITDEPTH), "DDSD_ZBUFFERBITDEPTH is set\n");

    /* 24 bit unpadded depth buffers are actually padded(Geforce 9600, Win7,
     * Radeon 9000M WinXP) */
    if (fmt->dwZBufferBitDepth == 24) expected_pitch = ddsd.dwWidth * 4;
    else expected_pitch = ddsd.dwWidth * fmt->dwZBufferBitDepth / 8;

    /* Some formats(16 bit depth without stencil) return pitch 0
     *
     * The Radeon X1600 Catalyst 10.2 Windows XP driver returns an otherwise sane
     * pitch with an extra 128 bytes, regardless of the format and width */
    if (ddsd.lPitch != 0 && ddsd.lPitch != expected_pitch
            && !broken(ddsd.lPitch == expected_pitch + 128))
    {
        ok(0, "Z buffer pitch is %lu, expected %u\n", ddsd.lPitch, expected_pitch);
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

    ok(SUCCEEDED(hr), "IDirect3D7_EnumZBufferFormats failed, hr %#lx.\n", hr);
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
    ok(hr == DDERR_INVALIDPARAMS, "GetCaps with NULL hel caps returned hr %#lx, expected INVALIDPARAMS.\n", hr);
    ok(hw_caps.dwFlags == 0xdeadbeef, "hw_caps.dwFlags was modified: %#lx.\n", hw_caps.dwFlags);
    hr = IDirect3DDevice_GetCaps(Direct3DDevice1, NULL, &hel_caps);
    ok(hr == DDERR_INVALIDPARAMS, "GetCaps with NULL hw caps returned hr %#lx, expected INVALIDPARAMS.\n", hr);
    ok(hel_caps.dwFlags == 0xdeadc0de, "hel_caps.dwFlags was modified: %#lx.\n", hel_caps.dwFlags);

    /* Successful call: Both are modified */
    hr = IDirect3DDevice_GetCaps(Direct3DDevice1, &hw_caps, &hel_caps);
    ok(hr == D3D_OK, "GetCaps with correct size returned hr %#lx, expected D3D_OK.\n", hr);
    ok(hw_caps.dwFlags != 0xdeadbeef, "hw_caps.dwFlags was not modified: %#lx.\n", hw_caps.dwFlags);
    ok(hel_caps.dwFlags != 0xdeadc0de, "hel_caps.dwFlags was not modified: %#lx.\n", hel_caps.dwFlags);

    memset(&hw_caps, 0, sizeof(hw_caps));
    hw_caps.dwSize = sizeof(hw_caps);
    hw_caps.dwFlags = 0xdeadbeef;
    memset(&hel_caps, 0, sizeof(hel_caps));
    /* Keep dwSize at 0 */
    hel_caps.dwFlags = 0xdeadc0de;

    /* If one is invalid the call fails */
    hr = IDirect3DDevice_GetCaps(Direct3DDevice1, &hw_caps, &hel_caps);
    ok(hr == DDERR_INVALIDPARAMS, "GetCaps with invalid hel_caps size returned hr %#lx, expected INVALIDPARAMS.\n", hr);
    ok(hw_caps.dwFlags == 0xdeadbeef, "hw_caps.dwFlags was modified: %#lx.\n", hw_caps.dwFlags);
    ok(hel_caps.dwFlags == 0xdeadc0de, "hel_caps.dwFlags was modified: %#lx.\n", hel_caps.dwFlags);
    hel_caps.dwSize = sizeof(hel_caps);
    hw_caps.dwSize = sizeof(hw_caps) + 1;
    hr = IDirect3DDevice_GetCaps(Direct3DDevice1, &hw_caps, &hel_caps);
    ok(hr == DDERR_INVALIDPARAMS, "GetCaps with invalid hw_caps size returned hr %#lx, expected INVALIDPARAMS.\n", hr);
    ok(hw_caps.dwFlags == 0xdeadbeef, "hw_caps.dwFlags was modified: %#lx.\n", hw_caps.dwFlags);
    ok(hel_caps.dwFlags == 0xdeadc0de, "hel_caps.dwFlags was modified: %#lx.\n", hel_caps.dwFlags);

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
                ok(hw_caps.dwMinTextureWidth == 0xfefefefe, "hw_caps.dwMinTextureWidth was modified: %#lx.\n",
                        hw_caps.dwMinTextureWidth);
                ok(hel_caps.dwMinTextureWidth == 0xfefefefe, "hel_caps.dwMinTextureWidth was modified: %#lx.\n",
                        hel_caps.dwMinTextureWidth);
                /* drop through */
            case FIELD_OFFSET(D3DDEVICEDESC, dwMaxTextureRepeat): /* 204, DirectX 5, IDirect3DDevice2 */
                ok(hw_caps.dwMaxTextureRepeat == 0xfefefefe, "hw_caps.dwMaxTextureRepeat was modified: %#lx.\n",
                        hw_caps.dwMaxTextureRepeat);
                ok(hel_caps.dwMaxTextureRepeat == 0xfefefefe, "hel_caps.dwMaxTextureRepeat was modified: %#lx.\n",
                        hel_caps.dwMaxTextureRepeat);
                /* drop through */
            case sizeof(D3DDEVICEDESC): /* 252, DirectX 6, IDirect3DDevice3 */
                ok(hr == D3D_OK, "GetCaps with size %u returned hr %#lx, expected D3D_OK.\n", i, hr);
                break;

            default:
                ok(hr == DDERR_INVALIDPARAMS,
                        "GetCaps with size %u returned hr %#lx, expected DDERR_INVALIDPARAMS.\n", i, hr);
                break;
        }
    }

    /* Different valid sizes are OK */
    hw_caps.dwSize = 172;
    hel_caps.dwSize = sizeof(D3DDEVICEDESC);
    hr = IDirect3DDevice_GetCaps(Direct3DDevice1, &hw_caps, &hel_caps);
    ok(hr == D3D_OK, "GetCaps with different sizes returned hr %#lx, expected D3D_OK.\n", hr);
}

static void test_get_caps7(void)
{
    HRESULT hr;
    D3DDEVICEDESC7 desc;

    hr = IDirect3DDevice7_GetCaps(lpD3DDevice, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirect3DDevice7::GetCaps(NULL) returned hr %#lx, expected INVALIDPARAMS.\n", hr);

    memset(&desc, 0, sizeof(desc));
    hr = IDirect3DDevice7_GetCaps(lpD3DDevice, &desc);
    ok(hr == D3D_OK, "IDirect3DDevice7::GetCaps(non-NULL) returned hr %#lx, expected D3D_OK.\n", hr);

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
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        ref = IDirect3DViewport2_Release(context->viewport);
        ok(ref == 0, "Unexpected refcount %ld.\n", ref);
    }
    if (context->device)
    {
        ref = IDirect3DDevice2_Release(context->device);
        ok(ref == 0, "Unexpected refcount %ld.\n", ref);
    }
    if (context->surface)
    {
        ref = IDirectDrawSurface_Release(context->surface);
        ok(ref == 0, "Unexpected refcount %ld.\n", ref);
    }
    if (context->d3d)
    {
        ref = IDirect3D2_Release(context->d3d);
        ok(ref == 1, "Unexpected refcount %ld.\n", ref);
    }
    if (context->ddraw)
    {
        ref = IDirectDraw_Release(context->ddraw);
        ok(ref == 0, "Unexpected refcount %ld.\n", ref);
    }
}

static BOOL d3d2_create_objects(struct d3d2_test_context *context)
{
    HRESULT hr;
    DDSURFACEDESC ddsd;
    D3DVIEWPORT vp_data;

    memset(context, 0, sizeof(*context));

    hr = DirectDrawCreate(NULL, &context->ddraw, NULL);
    ok(hr == DD_OK || hr == DDERR_NODIRECTDRAWSUPPORT, "Got hr %#lx.\n", hr);
    if (!context->ddraw) goto error;

    hr = IDirectDraw_SetCooperativeLevel(context->ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    if (FAILED(hr)) goto error;

    hr = IDirectDraw_QueryInterface(context->ddraw, &IID_IDirect3D2, (void**) &context->d3d);
    ok(hr == DD_OK || hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
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
    ok(hr == D3D_OK  || hr == E_OUTOFMEMORY || hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    if (!context->device) goto error;

    hr = IDirect3D2_CreateViewport(context->d3d, &context->viewport, NULL);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    if (!context->viewport) goto error;

    hr = IDirect3DDevice2_AddViewport(context->device, context->viewport);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
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
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

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
    ok(hr == DDERR_INVALIDPARAMS, "GetCaps with NULL hel caps returned hr %#lx, expected INVALIDPARAMS.\n", hr);
    ok(hw_caps.dwFlags == 0xdeadbeef, "hw_caps.dwFlags was modified: %#lx.\n", hw_caps.dwFlags);
    hr = IDirect3DDevice2_GetCaps(context->device, NULL, &hel_caps);
    ok(hr == DDERR_INVALIDPARAMS, "GetCaps with NULL hw caps returned hr %#lx, expected INVALIDPARAMS.\n", hr);
    ok(hel_caps.dwFlags == 0xdeadc0de, "hel_caps.dwFlags was modified: %#lx.\n", hel_caps.dwFlags);

    /* Successful call: Both are modified */
    hr = IDirect3DDevice2_GetCaps(context->device, &hw_caps, &hel_caps);
    ok(hr == D3D_OK, "GetCaps with correct size returned hr %#lx, expected D3D_OK.\n", hr);
    ok(hw_caps.dwFlags != 0xdeadbeef, "hw_caps.dwFlags was not modified: %#lx.\n", hw_caps.dwFlags);
    ok(hel_caps.dwFlags != 0xdeadc0de, "hel_caps.dwFlags was not modified: %#lx.\n", hel_caps.dwFlags);

    memset(&hw_caps, 0, sizeof(hw_caps));
    hw_caps.dwSize = sizeof(hw_caps);
    hw_caps.dwFlags = 0xdeadbeef;
    memset(&hel_caps, 0, sizeof(hel_caps));
    /* Keep dwSize at 0 */
    hel_caps.dwFlags = 0xdeadc0de;

    /* If one is invalid the call fails */
    hr = IDirect3DDevice2_GetCaps(context->device, &hw_caps, &hel_caps);
    ok(hr == DDERR_INVALIDPARAMS, "GetCaps with invalid hel_caps size returned hr %#lx, expected INVALIDPARAMS.\n", hr);
    ok(hw_caps.dwFlags == 0xdeadbeef, "hw_caps.dwFlags was modified: %#lx.\n", hw_caps.dwFlags);
    ok(hel_caps.dwFlags == 0xdeadc0de, "hel_caps.dwFlags was modified: %#lx.\n", hel_caps.dwFlags);
    hel_caps.dwSize = sizeof(hel_caps);
    hw_caps.dwSize = sizeof(hw_caps) + 1;
    hr = IDirect3DDevice2_GetCaps(context->device, &hw_caps, &hel_caps);
    ok(hr == DDERR_INVALIDPARAMS, "GetCaps with invalid hw_caps size returned hr %#lx, expected INVALIDPARAMS.\n", hr);
    ok(hw_caps.dwFlags == 0xdeadbeef, "hw_caps.dwFlags was modified: %#lx.\n", hw_caps.dwFlags);
    ok(hel_caps.dwFlags == 0xdeadc0de, "hel_caps.dwFlags was modified: %#lx.\n", hel_caps.dwFlags);

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
                ok(hw_caps.dwMinTextureWidth == 0xfefefefe, "dwMinTextureWidth was modified: %#lx.\n",
                        hw_caps.dwMinTextureWidth);
                ok(hel_caps.dwMinTextureWidth == 0xfefefefe, "dwMinTextureWidth was modified: %#lx.\n",
                        hel_caps.dwMinTextureWidth);
                /* drop through */
            case FIELD_OFFSET(D3DDEVICEDESC, dwMaxTextureRepeat): /* 204, DirectX 5, IDirect3DDevice2 */
                ok(hw_caps.dwMaxTextureRepeat == 0xfefefefe, "dwMaxTextureRepeat was modified: %#lx.\n",
                        hw_caps.dwMaxTextureRepeat);
                ok(hel_caps.dwMaxTextureRepeat == 0xfefefefe, "dwMaxTextureRepeat was modified: %#lx.\n",
                        hel_caps.dwMaxTextureRepeat);
                /* drop through */
            case sizeof(D3DDEVICEDESC): /* 252, DirectX 6, IDirect3DDevice3 */
                ok(hr == D3D_OK, "GetCaps with size %u returned hr %#lx, expected D3D_OK.\n", i, hr);
                break;

            default:
                ok(hr == DDERR_INVALIDPARAMS,
                        "GetCaps with size %u returned hr %#lx, expected DDERR_INVALIDPARAMS.\n", i, hr);
                break;
        }
    }

    /* Different valid sizes are OK */
    hw_caps.dwSize = 172;
    hel_caps.dwSize = sizeof(D3DDEVICEDESC);
    hr = IDirect3DDevice2_GetCaps(context->device, &hw_caps, &hel_caps);
    ok(hr == D3D_OK, "GetCaps with different sizes returned hr %#lx, expected D3D_OK.\n", hr);
}

START_TEST(d3d)
{
    struct d3d2_test_context d3d2_context;
    void (* const d3d2_tests[])(const struct d3d2_test_context *) =
    {
        test_get_caps2
    };
    unsigned int i;

    /* These tests are deprecated. New tests should be added to tests/ddraw{1,2,4,7}.c */

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
        VertexBufferDescTest();
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
        BackBuffer3DCreateSurfaceTest();
        BackBuffer3DAttachmentTest();
        test_get_caps1();
        D3D1_releaseObjects();
    }
}
