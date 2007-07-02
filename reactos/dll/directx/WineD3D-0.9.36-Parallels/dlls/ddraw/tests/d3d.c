/*
 * Some unit tests for d3d functions
 *
 * Copyright (C) 2005 Antoine Chavasse
 * Copyright (C) 2006 Stefan Dösinger for CodeWeavers
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

#include "config.h"

#include <assert.h>
#include "wine/test.h"
#include "ddraw.h"
#include "d3d.h"

static LPDIRECTDRAW7           lpDD = NULL;
static LPDIRECT3D7             lpD3D = NULL;
static LPDIRECTDRAWSURFACE7    lpDDS = NULL;
static LPDIRECT3DDEVICE7       lpD3DDevice = NULL;
static LPDIRECT3DVERTEXBUFFER7 lpVBufSrc = NULL;
static LPDIRECT3DVERTEXBUFFER7 lpVBufDest1 = NULL;
static LPDIRECT3DVERTEXBUFFER7 lpVBufDest2 = NULL;

/* To compare bad floating point numbers. Not the ideal way to do it,
 * but it should be enough for here */
#define comparefloat(a, b) ( (((a) - (b)) < 0.0001) && (((a) - (b)) > -0.0001) )

static HRESULT (WINAPI *pDirectDrawCreateEx)(LPGUID,LPVOID*,REFIID,LPUNKNOWN);

typedef struct _VERTEX
{
    float x, y, z;  /* position */
} VERTEX, *LPVERTEX;

typedef struct _TVERTEX
{
    float x, y, z;  /* position */
    float rhw;
} TVERTEX, *LPTVERTEX;


static void init_function_pointers(void)
{
    HMODULE hmod = GetModuleHandleA("ddraw.dll");
    pDirectDrawCreateEx = (void*)GetProcAddress(hmod, "DirectDrawCreateEx");
}


static BOOL CreateDirect3D(void)
{
    HRESULT rc;
    DDSURFACEDESC2 ddsd;

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
    ok(rc==DD_OK, "CreateSurface returned: %x\n", rc);

    rc = IDirect3D7_CreateDevice(lpD3D, &IID_IDirect3DTnLHalDevice, lpDDS,
        &lpD3DDevice);
    ok(rc==D3D_OK || rc==DDERR_NOPALETTEATTACHED || rc==E_OUTOFMEMORY, "CreateDevice returned: %x\n", rc);
    if (!lpD3DDevice) {
        trace("IDirect3D7::CreateDevice() failed with an error %x\n", rc);
        return FALSE;
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

#ifndef WINE_NATIVEWIN32
    light.dvAttenuation0 = -1.0 / 0.0; /* -INFINITY */
#else
    *(long *)&light.dvAttenuation0 = 0xff800000L;
#endif
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

#ifndef WINE_NATIVEWIN32
    light.dvAttenuation0 = 1.0 / 0.0; /* +INFINITY */
#else
    *(long *)&light.dvAttenuation0 = 0x7f800000L;
#endif
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 103, &light);
    ok(rc==D3D_OK, "SetLight returned: %x\n", rc);

#ifndef WINE_NATIVEWIN32
    light.dvAttenuation0 = 0.0 / 0.0; /* NaN */
#else
    *(long *)&light.dvAttenuation0 = 0x7fC00000L;
#endif
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 103, &light);
    ok(rc==D3D_OK, "SetLight returned: %x\n", rc);

    /* Directional light ignores attenuation */
    light.dltType = D3DLIGHT_DIRECTIONAL;
    light.dvAttenuation0 = -1.0;
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 103, &light);
    ok(rc==D3D_OK, "SetLight returned: %x\n", rc);
}

static void ProcessVerticesTest(void)
{
    D3DVERTEXBUFFERDESC desc;
    HRESULT rc;
    VERTEX *in;
    TVERTEX *out;
    VERTEX *out2;
    D3DVIEWPORT7 vp;
    D3DMATRIX view = {  2.0, 0.0, 0.0, 0.0,
                        0.0, -1.0, 0.0, 0.0,
                        0.0, 0.0, 1.0, 0.0,
                        0.0, 0.0, 0.0, 3.0 };

    D3DMATRIX world = { 0.0, 1.0, 0.0, 0.0,
                        1.0, 0.0, 0.0, 0.0,
                        0.0, 0.0, 0.0, 1.0,
                        0.0, 1.0, 1.0, 1.0 };

    D3DMATRIX proj = {  1.0, 0.0, 0.0, 1.0,
                        0.0, 1.0, 1.0, 0.0,
                        0.0, 1.0, 1.0, 0.0,
                        1.0, 0.0, 0.0, 1.0 };
    /* Create some vertex buffers */

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwCaps = 0;
    desc.dwFVF = D3DFVF_XYZ;
    desc.dwNumVertices = 16;
    rc = IDirect3D7_CreateVertexBuffer(lpD3D, &desc, &lpVBufSrc, 0);
    ok(rc==D3D_OK || rc==E_OUTOFMEMORY, "CreateVertexBuffer returned: %x\n", rc);
    if (!lpVBufSrc)
    {
        trace("IDirect3D7::CreateVertexBuffer() failed with an error %x\n", rc);
        goto out;
    }

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwCaps = 0;
    desc.dwFVF = D3DFVF_XYZRHW;
    desc.dwNumVertices = 16;
    /* Msdn says that the last parameter must be 0 - check that */
    rc = IDirect3D7_CreateVertexBuffer(lpD3D, &desc, &lpVBufDest1, 4);
    ok(rc==D3D_OK || rc==E_OUTOFMEMORY, "CreateVertexBuffer returned: %x\n", rc);
    if (!lpVBufDest1)
    {
        trace("IDirect3D7::CreateVertexBuffer() failed with an error %x\n", rc);
        goto out;
    }

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwCaps = 0;
    desc.dwFVF = D3DFVF_XYZ;
    desc.dwNumVertices = 16;
    /* Msdn says that the last parameter must be 0 - check that */
    rc = IDirect3D7_CreateVertexBuffer(lpD3D, &desc, &lpVBufDest2, 12345678);
    ok(rc==D3D_OK || rc==E_OUTOFMEMORY, "CreateVertexBuffer returned: %x\n", rc);
    if (!lpVBufDest2)
    {
        trace("IDirect3D7::CreateVertexBuffer() failed with an error %x\n", rc);
        goto out;
    }

    rc = IDirect3DVertexBuffer7_Lock(lpVBufSrc, 0, (void **) &in, NULL);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::Lock returned: %x\n", rc);
    if(!in) goto out;

    /* Check basic transformation */

    in[0].x = 0.0;
    in[0].y = 0.0;
    in[0].z = 0.0;

    in[1].x = 1.0;
    in[1].y = 1.0;
    in[1].z = 1.0;

    in[2].x = -1.0;
    in[2].y = -1.0;
    in[2].z = 0.5;

    in[3].x = 0.5;
    in[3].y = -0.5;
    in[3].z = 0.25;
    rc = IDirect3DVertexBuffer7_Unlock(lpVBufSrc);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::Unlock returned: %x\n", rc);

    rc = IDirect3DVertexBuffer7_ProcessVertices(lpVBufDest1, D3DVOP_TRANSFORM, 0, 4, lpVBufSrc, 0, lpD3DDevice, 0);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::ProcessVertices returned: %x\n", rc);

    rc = IDirect3DVertexBuffer7_ProcessVertices(lpVBufDest2, D3DVOP_TRANSFORM, 0, 4, lpVBufSrc, 0, lpD3DDevice, 0);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::ProcessVertices returned: %x\n", rc);

    rc = IDirect3DVertexBuffer7_Lock(lpVBufDest1, 0, (void **) &out, NULL);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::Lock returned: %x\n", rc);
    if(!out) goto out;

    /* Check the results */
    ok( comparefloat(out[0].x, 128.0 ) &&
        comparefloat(out[0].y, 128.0 ) &&
        comparefloat(out[0].z, 0.0 ) &&
        comparefloat(out[0].rhw, 1.0 ),
        "Output 0 vertex is (%f , %f , %f , %f)\n", out[0].x, out[0].y, out[0].z, out[0].rhw);

    ok( comparefloat(out[1].x, 256.0 ) &&
        comparefloat(out[1].y, 0.0 ) &&
        comparefloat(out[1].z, 1.0 ) &&
        comparefloat(out[1].rhw, 1.0 ),
        "Output 1 vertex is (%f , %f , %f , %f)\n", out[1].x, out[1].y, out[1].z, out[1].rhw);

    ok( comparefloat(out[2].x, 0.0 ) &&
        comparefloat(out[2].y, 256.0 ) &&
        comparefloat(out[2].z, 0.5 ) &&
        comparefloat(out[2].rhw, 1.0 ),
        "Output 2 vertex is (%f , %f , %f , %f)\n", out[2].x, out[2].y, out[2].z, out[2].rhw);

    ok( comparefloat(out[3].x, 192.0 ) &&
        comparefloat(out[3].y, 192.0 ) &&
        comparefloat(out[3].z, 0.25 ) &&
        comparefloat(out[3].rhw, 1.0 ),
        "Output 3 vertex is (%f , %f , %f , %f)\n", out[3].x, out[3].y, out[3].z, out[3].rhw);

    rc = IDirect3DVertexBuffer7_Unlock(lpVBufDest1);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::Unlock returned: %x\n", rc);
    out = NULL;

    rc = IDirect3DVertexBuffer7_Lock(lpVBufDest2, 0, (void **) &out2, NULL);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::Lock returned: %x\n", rc);
    if(!out2) goto out;
    /* Small thing without much practial meaning, but I stumbled upon it,
     * so let's check for it: If the output vertex buffer has to RHW value,
     * The RHW value of the last vertex is written into the next vertex
     */
    ok( comparefloat(out2[4].x, 1.0 ) &&
        comparefloat(out2[4].y, 0.0 ) &&
        comparefloat(out2[4].z, 0.0 ),
        "Output 4 vertex is (%f , %f , %f)\n", out2[4].x, out2[4].y, out2[4].z);

    rc = IDirect3DVertexBuffer7_Unlock(lpVBufDest2);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::Unlock returned: %x\n", rc);
    out = NULL;

    /* Try a more complicated viewport, same vertices */
    memset(&vp, 0, sizeof(vp));
    vp.dwX = 10;
    vp.dwY = 5;
    vp.dwWidth = 246;
    vp.dwHeight = 130;
    vp.dvMinZ = -2.0;
    vp.dvMaxZ = 4.0;
    rc = IDirect3DDevice7_SetViewport(lpD3DDevice, &vp);
    ok(rc==D3D_OK, "IDirect3DDevice7_SetViewport failed with rc=%x\n", rc);

    /* Process again */
    rc = IDirect3DVertexBuffer7_ProcessVertices(lpVBufDest1, D3DVOP_TRANSFORM, 0, 4, lpVBufSrc, 0, lpD3DDevice, 0);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::ProcessVertices returned: %x\n", rc);

    rc = IDirect3DVertexBuffer7_Lock(lpVBufDest1, 0, (void **) &out, NULL);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::Lock returned: %x\n", rc);
    if(!out) goto out;

    /* Check the results */
    ok( comparefloat(out[0].x, 133.0 ) &&
        comparefloat(out[0].y, 70.0 ) &&
        comparefloat(out[0].z, -2.0 ) &&
        comparefloat(out[0].rhw, 1.0 ),
        "Output 0 vertex is (%f , %f , %f , %f)\n", out[0].x, out[0].y, out[0].z, out[0].rhw);

    ok( comparefloat(out[1].x, 256.0 ) &&
        comparefloat(out[1].y, 5.0 ) &&
        comparefloat(out[1].z, 4.0 ) &&
        comparefloat(out[1].rhw, 1.0 ),
        "Output 1 vertex is (%f , %f , %f , %f)\n", out[1].x, out[1].y, out[1].z, out[1].rhw);

    ok( comparefloat(out[2].x, 10.0 ) &&
        comparefloat(out[2].y, 135.0 ) &&
        comparefloat(out[2].z, 1.0 ) &&
        comparefloat(out[2].rhw, 1.0 ),
        "Output 2 vertex is (%f , %f , %f , %f)\n", out[1].x, out[1].y, out[1].z, out[1].rhw);

    ok( comparefloat(out[3].x, 194.5 ) &&
        comparefloat(out[3].y, 102.5 ) &&
        comparefloat(out[3].z, -0.5 ) &&
        comparefloat(out[3].rhw, 1.0 ),
        "Output 3 vertex is (%f , %f , %f , %f)\n", out[3].x, out[3].y, out[3].z, out[3].rhw);

    rc = IDirect3DVertexBuffer7_Unlock(lpVBufDest1);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::Unlock returned: %x\n", rc);
    out = NULL;

    /* Play with some matrices. */

    rc = IDirect3DDevice7_SetTransform(lpD3DDevice, D3DTRANSFORMSTATE_VIEW, &view);
    ok(rc==D3D_OK, "IDirect3DDevice7_SetTransform failed\n");

    rc = IDirect3DDevice7_SetTransform(lpD3DDevice, D3DTRANSFORMSTATE_PROJECTION, &proj);
    ok(rc==D3D_OK, "IDirect3DDevice7_SetTransform failed\n");

    rc = IDirect3DDevice7_SetTransform(lpD3DDevice, D3DTRANSFORMSTATE_WORLD, &world);
    ok(rc==D3D_OK, "IDirect3DDevice7_SetTransform failed\n");

    rc = IDirect3DVertexBuffer7_ProcessVertices(lpVBufDest1, D3DVOP_TRANSFORM, 0, 4, lpVBufSrc, 0, lpD3DDevice, 0);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::ProcessVertices returned: %x\n", rc);

    rc = IDirect3DVertexBuffer7_Lock(lpVBufDest1, 0, (void **) &out, NULL);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::Lock returned: %x\n", rc);
    if(!out) goto out;

    /* Keep the viewport simpler, otherwise we get bad numbers to compare */
    vp.dwX = 0;
    vp.dwY = 0;
    vp.dwWidth = 100;
    vp.dwHeight = 100;
    vp.dvMinZ = 1.0;
    vp.dvMaxZ = 0.0;
    rc = IDirect3DDevice7_SetViewport(lpD3DDevice, &vp);
    ok(rc==D3D_OK, "IDirect3DDevice7_SetViewport failed\n");

    /* Check the results */
    ok( comparefloat(out[0].x, 256.0 ) &&    /* X coordinate is cut at the surface edges */
        comparefloat(out[0].y, 70.0 ) &&
        comparefloat(out[0].z, -2.0 ) &&
        comparefloat(out[0].rhw, (1.0 / 3.0)),
        "Output 0 vertex is (%f , %f , %f , %f)\n", out[0].x, out[0].y, out[0].z, out[0].rhw);

    ok( comparefloat(out[1].x, 256.0 ) &&
        comparefloat(out[1].y, 78.125000 ) &&
        comparefloat(out[1].z, -2.750000 ) &&
        comparefloat(out[1].rhw, 0.125000 ),
        "Output 1 vertex is (%f , %f , %f , %f)\n", out[1].x, out[1].y, out[1].z, out[1].rhw);

    ok( comparefloat(out[2].x, 256.0 ) &&
        comparefloat(out[2].y, 44.000000 ) &&
        comparefloat(out[2].z, 0.400000 ) &&
        comparefloat(out[2].rhw, 0.400000 ),
        "Output 2 vertex is (%f , %f , %f , %f)\n", out[2].x, out[2].y, out[2].z, out[2].rhw);

    ok( comparefloat(out[3].x, 256.0 ) &&
        comparefloat(out[3].y, 81.818184 ) &&
        comparefloat(out[3].z, -3.090909 ) &&
        comparefloat(out[3].rhw, 0.363636 ),
        "Output 3 vertex is (%f , %f , %f , %f)\n", out[3].x, out[3].y, out[3].z, out[3].rhw);

    rc = IDirect3DVertexBuffer7_Unlock(lpVBufDest1);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::Unlock returned: %x\n", rc);
    out = NULL;

out:
    IDirect3DVertexBuffer7_Release(lpVBufSrc);
    IDirect3DVertexBuffer7_Release(lpVBufDest1);
    IDirect3DVertexBuffer7_Release(lpVBufDest2);
}

static void StateTest( void )
{
    HRESULT rc;

    /* The msdn says its undocumented, does it return an error too? */
    rc = IDirect3DDevice7_SetRenderState(lpD3DDevice, D3DRENDERSTATE_ZVISIBLE, TRUE);
    ok(rc == D3D_OK, "IDirect3DDevice7_SetRenderState(D3DRENDERSTATE_ZVISIBLE, TRUE) returned %08x\n", rc);
    rc = IDirect3DDevice7_SetRenderState(lpD3DDevice, D3DRENDERSTATE_ZVISIBLE, FALSE);
    ok(rc == D3D_OK, "IDirect3DDevice7_SetRenderState(D3DRENDERSTATE_ZVISIBLE, FALSE) returned %08x\n", rc);
}


static void SceneTest(void)
{
    HRESULT                      hr;

    /* Test an EndScene without beginscene. Should return an error */
    hr = IDirect3DDevice7_EndScene(lpD3DDevice);
    ok(hr == D3DERR_SCENE_NOT_IN_SCENE, "IDirect3DDevice7_EndScene returned %08x\n", hr);

    /* Test a normal BeginScene / EndScene pair, this should work */
    hr = IDirect3DDevice7_BeginScene(lpD3DDevice);
    ok(hr == D3D_OK, "IDirect3DDevice7_BeginScene failed with %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice7_EndScene(lpD3DDevice);
        ok(hr == D3D_OK, "IDirect3DDevice7_EndScene failed with %08x\n", hr);
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

static void LimitTest(void)
{
    IDirectDrawSurface7 *pTexture = NULL;
    HRESULT hr;
    int i;
    DDSURFACEDESC2 ddsd;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    ddsd.dwWidth = 16;
    ddsd.dwHeight = 16;
    hr = IDirectDraw7_CreateSurface(lpDD, &ddsd, &pTexture, NULL);
    ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);
    if(!pTexture) return;

    for(i = 0; i < 8; i++) {
        hr = IDirect3DDevice7_SetTexture(lpD3DDevice, i, pTexture);
        ok(hr == D3D_OK, "IDirect3DDevice8_SetTexture for sampler %d failed with %08x\n", i, hr);
        hr = IDirect3DDevice7_SetTexture(lpD3DDevice, i, NULL);
        ok(hr == D3D_OK, "IDirect3DDevice8_SetTexture for sampler %d failed with %08x\n", i, hr);
        hr = IDirect3DDevice7_SetTextureStageState(lpD3DDevice, i, D3DTSS_COLOROP, D3DTOP_ADD);
        ok(hr == D3D_OK, "IDirect3DDevice8_SetTextureStageState for texture %d failed with %08x\n", i, hr);
    }

    IDirectDrawSurface7_Release(pTexture);
}

START_TEST(d3d)
{
    init_function_pointers();
    if(!pDirectDrawCreateEx) {
        trace("function DirectDrawCreateEx not available, skipping tests\n");
        return;
    }

    if(!CreateDirect3D()) {
        trace("Skipping tests\n");
        return;
    }
    LightTest();
    ProcessVerticesTest();
    StateTest();
    SceneTest();
    LimitTest();
    ReleaseDirect3D();
}
