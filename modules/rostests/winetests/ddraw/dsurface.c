/*
 * Unit tests for (a few) ddraw surface functions
 *
 * Copyright (C) 2005 Antoine Chavasse (a.chavasse@gmail.com)
 * Copyright (C) 2005 Christian Costa
 * Copyright 2005 Ivan Leo Puoti
 * Copyright (C) 2007-2009, 2011 Stefan Dösinger for CodeWeavers
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
#include "wine/exception.h"
#include "ddraw.h"
#include "d3d.h"
#include "unknwn.h"

static HRESULT (WINAPI *pDirectDrawCreateEx)(GUID *, void **, REFIID, IUnknown *);

static IDirectDraw *lpDD;
static DDCAPS ddcaps;

static BOOL CreateDirectDraw(void)
{
    HRESULT rc;

    rc = DirectDrawCreate(NULL, &lpDD, NULL);
    ok(rc==DD_OK || rc==DDERR_NODIRECTDRAWSUPPORT, "DirectDrawCreateEx returned: %x\n", rc);
    if (!lpDD) {
        trace("DirectDrawCreateEx() failed with an error %x\n", rc);
        return FALSE;
    }

    rc = IDirectDraw_SetCooperativeLevel(lpDD, NULL, DDSCL_NORMAL);
    ok(rc==DD_OK,"SetCooperativeLevel returned: %x\n",rc);

    return TRUE;
}


static void ReleaseDirectDraw(void)
{
    if( lpDD != NULL )
    {
        IDirectDraw_Release(lpDD);
        lpDD = NULL;
    }
}

/* The following tests test which interface is returned by IDirectDrawSurfaceX::GetDDInterface.
 * It uses refcounts to test that and compares the interface addresses. Partially fits here, and
 * partially in the refcount test
 */

static ULONG getref(IUnknown *iface)
{
    IUnknown_AddRef(iface);
    return IUnknown_Release(iface);
}

static void GetDDInterface_1(void)
{
    IDirectDrawSurface2 *dsurface2;
    IDirectDrawSurface *dsurface;
    DDSURFACEDESC surface;
    HRESULT ret;
    IDirectDraw2 *dd2;
    IDirectDraw4 *dd4;
    IDirectDraw7 *dd7;
    ULONG ref1, ref2, ref4, ref7;
    void *dd;

    /* Create a surface */
    ZeroMemory(&surface, sizeof(surface));
    surface.dwSize = sizeof(surface);
    surface.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
    surface.dwHeight = 10;
    surface.dwWidth = 10;
    ret = IDirectDraw_CreateSurface(lpDD, &surface, &dsurface, NULL);
    if(ret != DD_OK)
    {
        ok(FALSE, "IDirectDraw::CreateSurface failed with error %x\n", ret);
        return;
    }
    ret = IDirectDrawSurface_QueryInterface(dsurface, &IID_IDirectDrawSurface2, (void **) &dsurface2);
    ok(ret == DD_OK, "IDirectDrawSurface_QueryInterface returned %08x\n", ret);
    ret = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw2, (void **) &dd2);
    ok(ret == DD_OK, "IDirectDraw7_QueryInterface returned %08x\n", ret);
    ret = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw4, (void **) &dd4);
    ok(ret == DD_OK, "IDirectDraw7_QueryInterface returned %08x\n", ret);
    ret = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw7, (void **) &dd7);
    ok(ret == DD_OK, "IDirectDraw7_QueryInterface returned %08x\n", ret);

    ref1 = getref((IUnknown *) lpDD);
    ok(ref1 == 1, "IDirectDraw refcount is %d\n", ref1);
    ref2 = getref((IUnknown *) dd2);
    ok(ref2 == 1, "IDirectDraw2 refcount is %d\n", ref2);
    ref4 = getref((IUnknown *) dd4);
    ok(ref4 == 1, "IDirectDraw4 refcount is %d\n", ref4);
    ref7 = getref((IUnknown *) dd7);
    ok(ref7 == 1, "IDirectDraw7 refcount is %d\n", ref7);


    ret = IDirectDrawSurface2_GetDDInterface(dsurface2, &dd);
    ok(ret == DD_OK, "IDirectDrawSurface7_GetDDInterface returned %08x\n", ret);
    ok(getref((IUnknown *) lpDD) == ref1 + 1, "IDirectDraw refcount was increased by %d\n", getref((IUnknown *) lpDD) - ref1);
    ok(getref((IUnknown *) dd2) == ref2 + 0, "IDirectDraw2 refcount was increased by %d\n", getref((IUnknown *) dd2) - ref2);
    ok(getref((IUnknown *) dd4) == ref4 + 0, "IDirectDraw4 refcount was increased by %d\n", getref((IUnknown *) dd4) - ref4);
    ok(getref((IUnknown *) dd7) == ref7 + 0, "IDirectDraw7 refcount was increased by %d\n", getref((IUnknown *) dd7) - ref7);

    ok(dd == lpDD, "Returned interface pointer is not equal to the creation interface\n");
    IUnknown_Release((IUnknown *) dd);

    /* try a NULL pointer */
    ret = IDirectDrawSurface2_GetDDInterface(dsurface2, NULL);
    ok(ret == DDERR_INVALIDPARAMS, "IDirectDrawSurface7_GetDDInterface returned %08x\n", ret);

    IDirectDraw_Release(dd2);
    IDirectDraw_Release(dd4);
    IDirectDraw_Release(dd7);
    IDirectDrawSurface2_Release(dsurface2);
    IDirectDrawSurface_Release(dsurface);
}

static void GetDDInterface_2(void)
{
    IDirectDrawSurface2 *dsurface2;
    IDirectDrawSurface *dsurface;
    DDSURFACEDESC surface;
    HRESULT ret;
    IDirectDraw2 *dd2;
    IDirectDraw4 *dd4;
    IDirectDraw7 *dd7;
    ULONG ref1, ref2, ref4, ref7;
    void *dd;

    ret = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw2, (void **) &dd2);
    ok(ret == DD_OK, "IDirectDraw7_QueryInterface returned %08x\n", ret);
    ret = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw4, (void **) &dd4);
    ok(ret == DD_OK, "IDirectDraw7_QueryInterface returned %08x\n", ret);
    ret = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw7, (void **) &dd7);
    ok(ret == DD_OK, "IDirectDraw7_QueryInterface returned %08x\n", ret);

    /* Create a surface */
    ZeroMemory(&surface, sizeof(surface));
    surface.dwSize = sizeof(surface);
    surface.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
    surface.dwHeight = 10;
    surface.dwWidth = 10;
    ret = IDirectDraw2_CreateSurface(dd2, &surface, &dsurface, NULL);
    if(ret != DD_OK)
    {
        ok(FALSE, "IDirectDraw::CreateSurface failed with error %x\n", ret);
        return;
    }
    ret = IDirectDrawSurface_QueryInterface(dsurface, &IID_IDirectDrawSurface2, (void **) &dsurface2);
    ok(ret == DD_OK, "IDirectDrawSurface_QueryInterface returned %08x\n", ret);

    ref1 = getref((IUnknown *) lpDD);
    ok(ref1 == 1, "IDirectDraw refcount is %d\n", ref1);
    ref2 = getref((IUnknown *) dd2);
    ok(ref2 == 1, "IDirectDraw2 refcount is %d\n", ref2);
    ref4 = getref((IUnknown *) dd4);
    ok(ref4 == 1, "IDirectDraw4 refcount is %d\n", ref4);
    ref7 = getref((IUnknown *) dd7);
    ok(ref7 == 1, "IDirectDraw7 refcount is %d\n", ref7);


    ret = IDirectDrawSurface2_GetDDInterface(dsurface2, &dd);
    ok(ret == DD_OK, "IDirectDrawSurface7_GetDDInterface returned %08x\n", ret);
    ok(getref((IUnknown *) lpDD) == ref1 + 0, "IDirectDraw refcount was increased by %d\n", getref((IUnknown *) lpDD) - ref1);
    ok(getref((IUnknown *) dd2) == ref2 + 1, "IDirectDraw2 refcount was increased by %d\n", getref((IUnknown *) dd2) - ref2);
    ok(getref((IUnknown *) dd4) == ref4 + 0, "IDirectDraw4 refcount was increased by %d\n", getref((IUnknown *) dd4) - ref4);
    ok(getref((IUnknown *) dd7) == ref7 + 0, "IDirectDraw7 refcount was increased by %d\n", getref((IUnknown *) dd7) - ref7);

    ok(dd == dd2, "Returned interface pointer is not equal to the creation interface\n");
    IUnknown_Release((IUnknown *) dd);

    IDirectDraw_Release(dd2);
    IDirectDraw_Release(dd4);
    IDirectDraw_Release(dd7);
    IDirectDrawSurface2_Release(dsurface2);
    IDirectDrawSurface_Release(dsurface);
}

static void GetDDInterface_4(void)
{
    IDirectDrawSurface4 *dsurface4;
    IDirectDrawSurface2 *dsurface2;
    DDSURFACEDESC2 surface;
    HRESULT ret;
    IDirectDraw2 *dd2;
    IDirectDraw4 *dd4;
    IDirectDraw7 *dd7;
    ULONG ref1, ref2, ref4, ref7;
    void *dd;

    ret = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw2, (void **) &dd2);
    ok(ret == DD_OK, "IDirectDraw7_QueryInterface returned %08x\n", ret);
    ret = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw4, (void **) &dd4);
    ok(ret == DD_OK, "IDirectDraw7_QueryInterface returned %08x\n", ret);
    ret = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw7, (void **) &dd7);
    ok(ret == DD_OK, "IDirectDraw7_QueryInterface returned %08x\n", ret);

    /* Create a surface */
    ZeroMemory(&surface, sizeof(surface));
    surface.dwSize = sizeof(surface);
    surface.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
    surface.dwHeight = 10;
    surface.dwWidth = 10;
    ret = IDirectDraw4_CreateSurface(dd4, &surface, &dsurface4, NULL);
    if(ret != DD_OK)
    {
        ok(FALSE, "IDirectDraw::CreateSurface failed with error %x\n", ret);
        return;
    }
    ret = IDirectDrawSurface4_QueryInterface(dsurface4, &IID_IDirectDrawSurface2, (void **) &dsurface2);
    ok(ret == DD_OK, "IDirectDrawSurface_QueryInterface returned %08x\n", ret);

    ref1 = getref((IUnknown *) lpDD);
    ok(ref1 == 1, "IDirectDraw refcount is %d\n", ref1);
    ref2 = getref((IUnknown *) dd2);
    ok(ref2 == 1, "IDirectDraw2 refcount is %d\n", ref2);
    ref4 = getref((IUnknown *) dd4);
    ok(ref4 == 2, "IDirectDraw4 refcount is %d\n", ref4);
    ref7 = getref((IUnknown *) dd7);
    ok(ref7 == 1, "IDirectDraw7 refcount is %d\n", ref7);

    ret = IDirectDrawSurface4_GetDDInterface(dsurface4, &dd);
    ok(ret == DD_OK, "IDirectDrawSurface7_GetDDInterface returned %08x\n", ret);
    ok(getref((IUnknown *) lpDD) == ref1 + 0, "IDirectDraw refcount was increased by %d\n", getref((IUnknown *) lpDD) - ref1);
    ok(getref((IUnknown *) dd2) == ref2 + 0, "IDirectDraw2 refcount was increased by %d\n", getref((IUnknown *) dd2) - ref2);
    ok(getref((IUnknown *) dd4) == ref4 + 1, "IDirectDraw4 refcount was increased by %d\n", getref((IUnknown *) dd4) - ref4);
    ok(getref((IUnknown *) dd7) == ref7 + 0, "IDirectDraw7 refcount was increased by %d\n", getref((IUnknown *) dd7) - ref7);

    ok(dd == dd4, "Returned interface pointer is not equal to the creation interface\n");
    IUnknown_Release((IUnknown *) dd);

    /* Now test what happens if we QI the surface for some other version - It should still return the creation interface */
    ret = IDirectDrawSurface2_GetDDInterface(dsurface2, &dd);
    ok(ret == DD_OK, "IDirectDrawSurface7_GetDDInterface returned %08x\n", ret);
    ok(getref((IUnknown *) lpDD) == ref1 + 0, "IDirectDraw refcount was increased by %d\n", getref((IUnknown *) lpDD) - ref1);
    ok(getref((IUnknown *) dd2) == ref2 + 0, "IDirectDraw2 refcount was increased by %d\n", getref((IUnknown *) dd2) - ref2);
    ok(getref((IUnknown *) dd4) == ref4 + 1, "IDirectDraw4 refcount was increased by %d\n", getref((IUnknown *) dd4) - ref4);
    ok(getref((IUnknown *) dd7) == ref7 + 0, "IDirectDraw7 refcount was increased by %d\n", getref((IUnknown *) dd7) - ref7);

    ok(dd == dd4, "Returned interface pointer is not equal to the creation interface\n");
    IUnknown_Release((IUnknown *) dd);

    IDirectDraw_Release(dd2);
    IDirectDraw_Release(dd4);
    IDirectDraw_Release(dd7);
    IDirectDrawSurface4_Release(dsurface4);
    IDirectDrawSurface2_Release(dsurface2);
}

static void GetDDInterface_7(void)
{
    IDirectDrawSurface7 *dsurface7;
    IDirectDrawSurface4 *dsurface4;
    DDSURFACEDESC2 surface;
    HRESULT ret;
    IDirectDraw2 *dd2;
    IDirectDraw4 *dd4;
    IDirectDraw7 *dd7;
    ULONG ref1, ref2, ref4, ref7;
    void *dd;

    ret = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw2, (void **) &dd2);
    ok(ret == DD_OK, "IDirectDraw7_QueryInterface returned %08x\n", ret);
    ret = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw4, (void **) &dd4);
    ok(ret == DD_OK, "IDirectDraw7_QueryInterface returned %08x\n", ret);
    ret = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw7, (void **) &dd7);
    ok(ret == DD_OK, "IDirectDraw7_QueryInterface returned %08x\n", ret);

    /* Create a surface */
    ZeroMemory(&surface, sizeof(surface));
    surface.dwSize = sizeof(surface);
    surface.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
    surface.dwHeight = 10;
    surface.dwWidth = 10;
    ret = IDirectDraw7_CreateSurface(dd7, &surface, &dsurface7, NULL);
    if(ret != DD_OK)
    {
        ok(FALSE, "IDirectDraw::CreateSurface failed with error %x\n", ret);
        return;
    }
    ret = IDirectDrawSurface7_QueryInterface(dsurface7, &IID_IDirectDrawSurface4, (void **) &dsurface4);
    ok(ret == DD_OK, "IDirectDrawSurface_QueryInterface returned %08x\n", ret);

    ref1 = getref((IUnknown *) lpDD);
    ok(ref1 == 1, "IDirectDraw refcount is %d\n", ref1);
    ref2 = getref((IUnknown *) dd2);
    ok(ref2 == 1, "IDirectDraw2 refcount is %d\n", ref2);
    ref4 = getref((IUnknown *) dd4);
    ok(ref4 == 1, "IDirectDraw4 refcount is %d\n", ref4);
    ref7 = getref((IUnknown *) dd7);
    ok(ref7 == 2, "IDirectDraw7 refcount is %d\n", ref7);

    ret = IDirectDrawSurface7_GetDDInterface(dsurface7, &dd);
    ok(ret == DD_OK, "IDirectDrawSurface7_GetDDInterface returned %08x\n", ret);
    ok(getref((IUnknown *) lpDD) == ref1 + 0, "IDirectDraw refcount was increased by %d\n", getref((IUnknown *) lpDD) - ref1);
    ok(getref((IUnknown *) dd2) == ref2 + 0, "IDirectDraw2 refcount was increased by %d\n", getref((IUnknown *) dd2) - ref2);
    ok(getref((IUnknown *) dd4) == ref4 + 0, "IDirectDraw4 refcount was increased by %d\n", getref((IUnknown *) dd4) - ref4);
    ok(getref((IUnknown *) dd7) == ref7 + 1, "IDirectDraw7 refcount was increased by %d\n", getref((IUnknown *) dd7) - ref7);

    ok(dd == dd7, "Returned interface pointer is not equal to the creation interface\n");
    IUnknown_Release((IUnknown *) dd);

    /* Now test what happens if we QI the surface for some other version - It should still return the creation interface */
    ret = IDirectDrawSurface4_GetDDInterface(dsurface4, &dd);
    ok(ret == DD_OK, "IDirectDrawSurface7_GetDDInterface returned %08x\n", ret);
    ok(getref((IUnknown *) lpDD) == ref1 + 0, "IDirectDraw refcount was increased by %d\n", getref((IUnknown *) lpDD) - ref1);
    ok(getref((IUnknown *) dd2) == ref2 + 0, "IDirectDraw2 refcount was increased by %d\n", getref((IUnknown *) dd2) - ref2);
    ok(getref((IUnknown *) dd4) == ref4 + 0, "IDirectDraw4 refcount was increased by %d\n", getref((IUnknown *) dd4) - ref4);
    ok(getref((IUnknown *) dd7) == ref7 + 1, "IDirectDraw7 refcount was increased by %d\n", getref((IUnknown *) dd7) - ref7);

    ok(dd == dd7, "Returned interface pointer is not equal to the creation interface\n");
    IUnknown_Release((IUnknown *) dd);

    IDirectDraw_Release(dd2);
    IDirectDraw_Release(dd4);
    IDirectDraw_Release(dd7);
    IDirectDrawSurface4_Release(dsurface4);
    IDirectDrawSurface7_Release(dsurface7);
}

static ULONG getRefcount(IUnknown *iface)
{
    IUnknown_AddRef(iface);
    return IUnknown_Release(iface);
}

#define MAXEXPECTED 8  /* Can match up to 8 expected surfaces */
struct enumstruct
{
    IDirectDrawSurface *expected[MAXEXPECTED];
    UINT count;
};

static HRESULT WINAPI enumCB(IDirectDrawSurface *surf, DDSURFACEDESC *desc, void *ctx)
{
    int i;
    BOOL found = FALSE;

    for(i = 0; i < MAXEXPECTED; i++)
    {
        if(((struct enumstruct *)ctx)->expected[i] == surf) found = TRUE;
    }

    ok(found, "Unexpected surface %p enumerated\n", surf);
    ((struct enumstruct *)ctx)->count++;
    IDirectDrawSurface_Release(surf);
    return DDENUMRET_OK;
}

struct compare
{
    DWORD width, height;
    DWORD caps, caps2;
    UINT mipmaps;
};

static HRESULT WINAPI CubeTestPaletteEnum(IDirectDrawSurface7 *surface, DDSURFACEDESC2 *desc, void *context)
{
    HRESULT hr;

    hr = IDirectDrawSurface7_SetPalette(surface, context);
    if (desc->dwWidth == 64) /* This is for first mimpmap */
        ok(hr == DDERR_NOTONMIPMAPSUBLEVEL, "SetPalette returned: %x\n",hr);
    else
        ok(hr == DD_OK, "SetPalette returned: %x\n",hr);

    IDirectDrawSurface7_Release(surface);

    return DDENUMRET_OK;
}

static HRESULT WINAPI CubeTestLvl2Enum(IDirectDrawSurface7 *surface, DDSURFACEDESC2 *desc, void *context)
{
    UINT *mipmaps = context;

    (*mipmaps)++;
    IDirectDrawSurface7_EnumAttachedSurfaces(surface,
                                             context,
                                             CubeTestLvl2Enum);

    return DDENUMRET_OK;
}

static HRESULT WINAPI CubeTestLvl1Enum(IDirectDrawSurface7 *surface, DDSURFACEDESC2 *desc, void *context)
{
    UINT mipmaps = 0;
    UINT *num = context;
    static const struct compare expected[] =
    {
        {
            128, 128,
            DDSCAPS_MIPMAP | DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY | DDSCAPS_COMPLEX,
            DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ,
            7
        },
        {
            128, 128,
            DDSCAPS_MIPMAP | DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY | DDSCAPS_COMPLEX,
            DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ,
            7
        },
        {
            128, 128,
            DDSCAPS_MIPMAP | DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY | DDSCAPS_COMPLEX,
            DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY,
            7
        },
        {
            128, 128,
            DDSCAPS_MIPMAP | DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY | DDSCAPS_COMPLEX,
            DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY,
            7
        },
        {
            128, 128,
            DDSCAPS_MIPMAP | DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY | DDSCAPS_COMPLEX,
            DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX,
            7
        },
        {
            64, 64, /* This is the first mipmap */
            DDSCAPS_MIPMAP | DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY | DDSCAPS_COMPLEX,
            DDSCAPS2_MIPMAPSUBLEVEL | DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX,
            6
        },
    };

    mipmaps = 0;
    IDirectDrawSurface7_EnumAttachedSurfaces(surface,
                                             &mipmaps,
                                             CubeTestLvl2Enum);

    ok(desc->dwWidth == expected[*num].width, "Surface width is %d expected %d\n", desc->dwWidth, expected[*num].width);
    ok(desc->dwHeight == expected[*num].height, "Surface height is %d expected %d\n", desc->dwHeight, expected[*num].height);
    ok(desc->ddsCaps.dwCaps == expected[*num].caps, "Surface caps are %08x expected %08x\n", desc->ddsCaps.dwCaps, expected[*num].caps);
    ok(desc->ddsCaps.dwCaps2 == expected[*num].caps2, "Surface caps2 are %08x expected %08x\n", desc->ddsCaps.dwCaps2, expected[*num].caps2);
    ok(mipmaps == expected[*num].mipmaps, "Surface has %d mipmaps, expected %d\n", mipmaps, expected[*num].mipmaps);

    (*num)++;

    IDirectDrawSurface7_Release(surface);

    return DDENUMRET_OK;
}

static void CubeMapTest(void)
{
    IDirectDraw7 *dd7 = NULL;
    IDirectDrawSurface7 *cubemap = NULL;
    IDirectDrawPalette *palette = NULL;
    DDSURFACEDESC2 ddsd;
    HRESULT hr;
    PALETTEENTRY Table[256];
    int i;
    UINT num = 0;
    UINT ref;
    struct enumstruct ctx;

    for(i=0; i<256; i++)
    {
        Table[i].peRed   = 0xff;
        Table[i].peGreen = 0;
        Table[i].peBlue  = 0;
        Table[i].peFlags = 0;
    }

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw7, (void **) &dd7);
    ok(hr == DD_OK, "IDirectDraw::QueryInterface returned %08x\n", hr);
    if (FAILED(hr)) goto err;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP | DDSCAPS_SYSTEMMEMORY;
    ddsd.ddsCaps.dwCaps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALLFACES;

    /* D3DFMT_R5G6B5 */
    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 16;
    U2(U4(ddsd).ddpfPixelFormat).dwRBitMask = 0xF800;
    U3(U4(ddsd).ddpfPixelFormat).dwGBitMask = 0x07E0;
    U4(U4(ddsd).ddpfPixelFormat).dwBBitMask = 0x001F;

    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &cubemap, NULL);
    if (FAILED(hr))
    {
        skip("Can't create cubemap surface\n");
        goto err;
    }

    hr = IDirectDrawSurface7_GetSurfaceDesc(cubemap, &ddsd);
    ok(hr == DD_OK, "IDirectDrawSurface7_GetSurfaceDesc returned %08x\n", hr);
    ok(ddsd.ddsCaps.dwCaps == (DDSCAPS_MIPMAP | DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY | DDSCAPS_COMPLEX),
       "Root Caps are %08x\n", ddsd.ddsCaps.dwCaps);
    ok(ddsd.ddsCaps.dwCaps2 == (DDSCAPS2_CUBEMAP_POSITIVEX | DDSCAPS2_CUBEMAP),
       "Root Caps2 are %08x\n", ddsd.ddsCaps.dwCaps2);

    IDirectDrawSurface7_EnumAttachedSurfaces(cubemap,
                                             &num,
                                             CubeTestLvl1Enum);
    ok(num == 6, "Surface has %d attachments\n", num);
    IDirectDrawSurface7_Release(cubemap);

    /* What happens if I do not specify any faces? */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP | DDSCAPS_SYSTEMMEMORY;
    ddsd.ddsCaps.dwCaps2 = DDSCAPS2_CUBEMAP;

    /* D3DFMT_R5G6B5 */
    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 16;
    U2(U4(ddsd).ddpfPixelFormat).dwRBitMask = 0xF800;
    U3(U4(ddsd).ddpfPixelFormat).dwGBitMask = 0x07E0;
    U4(U4(ddsd).ddpfPixelFormat).dwBBitMask = 0x001F;

    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &cubemap, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw7::CreateSurface asking for a cube map without faces returned %08x\n", hr);

    /* Cube map faces without a cube map? */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP | DDSCAPS_SYSTEMMEMORY;
    ddsd.ddsCaps.dwCaps2 = DDSCAPS2_CUBEMAP_ALLFACES;

    /* D3DFMT_R5G6B5 */
    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 16;
    U2(U4(ddsd).ddpfPixelFormat).dwRBitMask = 0xF800;
    U3(U4(ddsd).ddpfPixelFormat).dwGBitMask = 0x07E0;
    U4(U4(ddsd).ddpfPixelFormat).dwBBitMask = 0x001F;

    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &cubemap, NULL);
    ok(hr == DDERR_INVALIDCAPS, "IDirectDraw7::CreateSurface returned %08x\n", hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP | DDSCAPS_SYSTEMMEMORY;
    ddsd.ddsCaps.dwCaps2 = DDSCAPS2_CUBEMAP_POSITIVEX;

    /* D3DFMT_R5G6B5 */
    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 16;
    U2(U4(ddsd).ddpfPixelFormat).dwRBitMask = 0xF800;
    U3(U4(ddsd).ddpfPixelFormat).dwGBitMask = 0x07E0;
    U4(U4(ddsd).ddpfPixelFormat).dwBBitMask = 0x001F;

    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &cubemap, NULL);
    ok(hr == DDERR_INVALIDCAPS, "IDirectDraw7::CreateSurface returned %08x\n", hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP | DDSCAPS_SYSTEMMEMORY;
    ddsd.ddsCaps.dwCaps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALLFACES;

    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
    U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 8;

    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &cubemap, NULL);
    if (FAILED(hr))
    {
        skip("Can't create palletized cubemap surface\n");
        goto err;
    }

    hr = IDirectDraw7_CreatePalette(dd7, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, Table, &palette, NULL);
    ok(hr == DD_OK, "CreatePalette failed with %08x\n", hr);

    hr = IDirectDrawSurface7_EnumAttachedSurfaces(cubemap, palette, CubeTestPaletteEnum);
    ok(hr == DD_OK, "EnumAttachedSurfaces failed\n");

    ref = getRefcount((IUnknown *) palette);
    ok(ref == 6, "Refcount is %u, expected 1\n", ref);

    IDirectDrawSurface7_Release(cubemap);

    ref = getRefcount((IUnknown *) palette);
    ok(ref == 1, "Refcount is %u, expected 1\n", ref);

    IDirectDrawPalette_Release(palette);

    /* Make sure everything is cleaned up properly. Use the enumSurfaces test infrastructure */
    memset(&ctx, 0, sizeof(ctx));
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    hr = IDirectDraw_EnumSurfaces(lpDD, DDENUMSURFACES_DOESEXIST | DDENUMSURFACES_ALL, (DDSURFACEDESC *) &ddsd, (void *) &ctx, enumCB);
    ok(hr == DD_OK, "IDirectDraw_EnumSurfaces returned %08x\n", hr);
    ok(ctx.count == 0, "%d surfaces enumerated, expected 0\n", ctx.count);

    err:
    if (dd7) IDirectDraw7_Release(dd7);
}

static void CompressedTest(void)
{
    HRESULT hr;
    IDirectDrawSurface7 *surface;
    DDSURFACEDESC2 ddsd, ddsd2;
    IDirectDraw7 *dd7 = NULL;
    RECT r = { 0, 0, 128, 128 };
    RECT r2 = { 32, 32, 64, 64 };

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw7, (void **) &dd7);
    ok(hr == DD_OK, "IDirectDraw::QueryInterface returned %08x\n", hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_FOURCC;
    U4(ddsd).ddpfPixelFormat.dwFourCC = MAKEFOURCC('D','X','T','1');

    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface, NULL);
    ok(hr == DD_OK, "CreateSurface returned %08x\n", hr);
    if (FAILED(hr))
    {
        skip("failed to create surface\n");
        return;
    }

    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);
    U4(ddsd2).ddpfPixelFormat.dwSize = sizeof(U4(ddsd2).ddpfPixelFormat);
    hr = IDirectDrawSurface7_GetSurfaceDesc(surface, &ddsd2);
    ok(hr == DD_OK, "GetSurfaceDesc returned %08x\n", hr);

    ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
       "Surface desc flags: %08x\n", ddsd2.dwFlags);
    ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
    ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
    ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY),
       "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
    ok(U1(ddsd2).dwLinearSize == 8192, "Linear size is %d\n", U1(ddsd2).dwLinearSize);
    ok(ddsd2.ddsCaps.dwCaps2 == 0, "Caps2: %08x\n", ddsd2.ddsCaps.dwCaps2);
    IDirectDrawSurface7_Release(surface);

    U4(ddsd).ddpfPixelFormat.dwFourCC = MAKEFOURCC('D','X','T','3');
    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface, NULL);
    ok(hr == DD_OK, "CreateSurface returned %08x\n", hr);
    if (FAILED(hr))
    {
        skip("failed to create surface\n");
        return;
    }

    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);
    U4(ddsd2).ddpfPixelFormat.dwSize = sizeof(U4(ddsd2).ddpfPixelFormat);
    hr = IDirectDrawSurface7_GetSurfaceDesc(surface, &ddsd2);
    ok(hr == DD_OK, "GetSurfaceDesc returned %08x\n", hr);

    ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
       "Surface desc flags: %08x\n", ddsd2.dwFlags);
    ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
    ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
    ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY),
       "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
    ok(U1(ddsd2).dwLinearSize == 16384, "Linear size is %d\n", U1(ddsd2).dwLinearSize);
    IDirectDrawSurface7_Release(surface);

    U4(ddsd).ddpfPixelFormat.dwFourCC = MAKEFOURCC('D','X','T','5');
    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface, NULL);
    ok(hr == DD_OK, "CreateSurface returned %08x\n", hr);
    if (FAILED(hr))
    {
        skip("failed to create surface\n");
        return;
    }

    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);
    U4(ddsd2).ddpfPixelFormat.dwSize = sizeof(U4(ddsd2).ddpfPixelFormat);
    hr = IDirectDrawSurface7_GetSurfaceDesc(surface, &ddsd2);
    ok(hr == DD_OK, "GetSurfaceDesc returned %08x\n", hr);

    ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
       "Surface desc flags: %08x\n", ddsd2.dwFlags);
    ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
    ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
    ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY),
       "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
    ok(U1(ddsd2).dwLinearSize == 16384, "Linear size is %d\n", U1(ddsd2).dwLinearSize);
    ok(ddsd2.lpSurface == 0, "Surface memory is at %p, expected NULL\n", ddsd2.lpSurface);

    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);
    U4(ddsd2).ddpfPixelFormat.dwSize = sizeof(U4(ddsd2).ddpfPixelFormat);

    /* Show that the description is not changed when locking the surface. What is really interesting
     * about this is that DDSD_LPSURFACE isn't set.
     */
    hr = IDirectDrawSurface7_Lock(surface, NULL, &ddsd2, DDLOCK_READONLY, 0);
    ok(hr == DD_OK, "Lock returned %08x\n", hr);

    ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
       "Surface desc flags: %08x\n", ddsd2.dwFlags);
    ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
    ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
    ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY),
       "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
    ok(U1(ddsd2).dwLinearSize == 16384, "Linear size is %d\n", U1(ddsd2).dwLinearSize);
    ok(ddsd2.lpSurface != 0, "Surface memory is at NULL\n");

    hr = IDirectDrawSurface7_Unlock(surface, NULL);
    ok(hr == DD_OK, "Unlock returned %08x\n", hr);

    /* Now what about a locking rect?  */
    hr = IDirectDrawSurface7_Lock(surface, &r, &ddsd2, DDLOCK_READONLY, 0);
    ok(hr == DD_OK, "Lock returned %08x\n", hr);

    ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
       "Surface desc flags: %08x\n", ddsd2.dwFlags);
    ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
    ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
    ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY),
       "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
    ok(U1(ddsd2).dwLinearSize == 16384, "Linear size is %d\n", U1(ddsd2).dwLinearSize);
    ok(ddsd2.lpSurface != 0, "Surface memory is at NULL\n");

    hr = IDirectDrawSurface7_Unlock(surface, &r);
    ok(hr == DD_OK, "Unlock returned %08x\n", hr);

    /* Now what about a different locking offset? */
    hr = IDirectDrawSurface7_Lock(surface, &r2, &ddsd2, DDLOCK_READONLY, 0);
    ok(hr == DD_OK, "Lock returned %08x\n", hr);

    ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
       "Surface desc flags: %08x\n", ddsd2.dwFlags);
    ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
    ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
    ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY),
       "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
    ok(U1(ddsd2).dwLinearSize == 16384, "Linear size is %d\n", U1(ddsd2).dwLinearSize);
    ok(ddsd2.lpSurface != 0, "Surface memory is at NULL\n");

    hr = IDirectDrawSurface7_Unlock(surface, &r2);
    ok(hr == DD_OK, "Unlock returned %08x\n", hr);
    IDirectDrawSurface7_Release(surface);

    /* Try this with video memory. A kind of surprise. It still has the LINEARSIZE flag set,
     * but seems to have a pitch instead.
     */
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY;
    U4(ddsd).ddpfPixelFormat.dwFourCC = MAKEFOURCC('D','X','T','1');

    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface, NULL);
    ok(hr == DD_OK || hr == DDERR_NOTEXTUREHW || hr == DDERR_INVALIDPARAMS ||
       broken(hr == DDERR_NODIRECTDRAWHW), "CreateSurface returned %08x\n", hr);

    /* Not supported everywhere */
    if(SUCCEEDED(hr))
    {
        memset(&ddsd2, 0, sizeof(ddsd2));
        ddsd2.dwSize = sizeof(ddsd2);
        U4(ddsd2).ddpfPixelFormat.dwSize = sizeof(U4(ddsd2).ddpfPixelFormat);
        hr = IDirectDrawSurface7_GetSurfaceDesc(surface, &ddsd2);
        ok(hr == DD_OK, "GetSurfaceDesc returned %08x\n", hr);

        ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
        "Surface desc flags: %08x\n", ddsd2.dwFlags);
        ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
        ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
        ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM),
        "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
        /* ATI drivers report a broken linear size, thus no need to clone the exact behaviour. nvidia reports the correct size */
        ok(ddsd2.ddsCaps.dwCaps2 == 0, "Caps2: %08x\n", ddsd2.ddsCaps.dwCaps2);
        IDirectDrawSurface7_Release(surface);

        U4(ddsd).ddpfPixelFormat.dwFourCC = MAKEFOURCC('D','X','T','3');
        hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface, NULL);
        ok(hr == DD_OK, "CreateSurface returned %08x\n", hr);

        memset(&ddsd2, 0, sizeof(ddsd2));
        ddsd2.dwSize = sizeof(ddsd2);
        U4(ddsd2).ddpfPixelFormat.dwSize = sizeof(U4(ddsd2).ddpfPixelFormat);
        hr = IDirectDrawSurface7_GetSurfaceDesc(surface, &ddsd2);
        ok(hr == DD_OK, "GetSurfaceDesc returned %08x\n", hr);

        ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
        "Surface desc flags: %08x\n", ddsd2.dwFlags);
        ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
        ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
        ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM),
        "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
        /* ATI drivers report a broken linear size, thus no need to clone the exact behaviour. nvidia reports the correct size */
        IDirectDrawSurface7_Release(surface);

        U4(ddsd).ddpfPixelFormat.dwFourCC = MAKEFOURCC('D','X','T','5');
        hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface, NULL);
        ok(hr == DD_OK, "CreateSurface returned %08x\n", hr);

        memset(&ddsd2, 0, sizeof(ddsd2));
        ddsd2.dwSize = sizeof(ddsd2);
        U4(ddsd2).ddpfPixelFormat.dwSize = sizeof(U4(ddsd2).ddpfPixelFormat);
        hr = IDirectDrawSurface7_GetSurfaceDesc(surface, &ddsd2);
        ok(hr == DD_OK, "GetSurfaceDesc returned %08x\n", hr);

        ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
        "Surface desc flags: %08x\n", ddsd2.dwFlags);
        ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
        ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
        ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM),
        "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
        /* ATI drivers report a broken linear size, thus no need to clone the exact behaviour. nvidia reports the correct size */
        ok(ddsd2.lpSurface == 0, "Surface memory is at %p, expected NULL\n", ddsd2.lpSurface);

        memset(&ddsd2, 0, sizeof(ddsd2));
        ddsd2.dwSize = sizeof(ddsd2);
        U4(ddsd2).ddpfPixelFormat.dwSize = sizeof(U4(ddsd2).ddpfPixelFormat);

        /* Show that the description is not changed when locking the surface. What is really interesting
        * about this is that DDSD_LPSURFACE isn't set.
        */
        hr = IDirectDrawSurface7_Lock(surface, NULL, &ddsd2, DDLOCK_READONLY, 0);
        ok(hr == DD_OK, "Lock returned %08x\n", hr);

        ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
        "Surface desc flags: %08x\n", ddsd2.dwFlags);
        ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
        ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
        ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM),
        "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
        /* ATI drivers report a broken linear size, thus no need to clone the exact behaviour. nvidia reports the correct size */
        ok(ddsd2.lpSurface != 0, "Surface memory is at NULL\n");

        hr = IDirectDrawSurface7_Unlock(surface, NULL);
        ok(hr == DD_OK, "Unlock returned %08x\n", hr);

        /* Now what about a locking rect?  */
        hr = IDirectDrawSurface7_Lock(surface, &r, &ddsd2, DDLOCK_READONLY, 0);
        ok(hr == DD_OK, "Lock returned %08x\n", hr);

        ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
        "Surface desc flags: %08x\n", ddsd2.dwFlags);
        ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
        ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
        ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM),
        "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
        /* ATI drivers report a broken linear size, thus no need to clone the exact behaviour. nvidia reports the correct size */
        ok(ddsd2.lpSurface != 0, "Surface memory is at NULL\n");

        hr = IDirectDrawSurface7_Unlock(surface, &r);
        ok(hr == DD_OK, "Unlock returned %08x\n", hr);

        /* Now what about a different locking offset? */
        hr = IDirectDrawSurface7_Lock(surface, &r2, &ddsd2, DDLOCK_READONLY, 0);
        ok(hr == DD_OK, "Lock returned %08x\n", hr);

        ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
        "Surface desc flags: %08x\n", ddsd2.dwFlags);
        ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
        ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
        ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM),
        "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
        /* ATI drivers report a broken linear size, thus no need to clone the exact behaviour. nvidia reports the correct size */
        ok(ddsd2.lpSurface != 0, "Surface memory is at NULL\n");

        hr = IDirectDrawSurface7_Unlock(surface, &r2);
        ok(hr == DD_OK, "Unlock returned %08x\n", hr);

        IDirectDrawSurface7_Release(surface);
    }
    else
    {
        skip("Hardware DXTN textures not supported\n");
    }

    /* What happens to managed textures? Interestingly, Windows reports them as being in system
     * memory. The linear size fits again.
     */
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
    U4(ddsd).ddpfPixelFormat.dwFourCC = MAKEFOURCC('D','X','T','1');

    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface, NULL);
    ok(hr == DD_OK || hr == DDERR_NOTEXTUREHW, "CreateSurface returned %08x\n", hr);

    /* Not supported everywhere */
    if(SUCCEEDED(hr))
    {
        memset(&ddsd2, 0, sizeof(ddsd2));
        ddsd2.dwSize = sizeof(ddsd2);
        U4(ddsd2).ddpfPixelFormat.dwSize = sizeof(U4(ddsd2).ddpfPixelFormat);
        hr = IDirectDrawSurface7_GetSurfaceDesc(surface, &ddsd2);
        ok(hr == DD_OK, "GetSurfaceDesc returned %08x\n", hr);

        ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
        "Surface desc flags: %08x\n", ddsd2.dwFlags);
        ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
        ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
        ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY),
        "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
        ok(U1(ddsd2).dwLinearSize == 8192, "Linear size is %d\n", U1(ddsd2).dwLinearSize);
        ok(ddsd2.ddsCaps.dwCaps2 == DDSCAPS2_TEXTUREMANAGE, "Caps2: %08x\n", ddsd2.ddsCaps.dwCaps2);
        IDirectDrawSurface7_Release(surface);

        U4(ddsd).ddpfPixelFormat.dwFourCC = MAKEFOURCC('D','X','T','3');
        hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface, NULL);
        ok(hr == DD_OK, "CreateSurface returned %08x\n", hr);

        memset(&ddsd2, 0, sizeof(ddsd2));
        ddsd2.dwSize = sizeof(ddsd2);
        U4(ddsd2).ddpfPixelFormat.dwSize = sizeof(U4(ddsd2).ddpfPixelFormat);
        hr = IDirectDrawSurface7_GetSurfaceDesc(surface, &ddsd2);
        ok(hr == DD_OK, "GetSurfaceDesc returned %08x\n", hr);

        ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
        "Surface desc flags: %08x\n", ddsd2.dwFlags);
        ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
        ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
        ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY),
        "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
        ok(U1(ddsd2).dwLinearSize == 16384, "Linear size is %d\n", U1(ddsd2).dwLinearSize);
        IDirectDrawSurface7_Release(surface);

        U4(ddsd).ddpfPixelFormat.dwFourCC = MAKEFOURCC('D','X','T','5');
        hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface, NULL);
        ok(hr == DD_OK, "CreateSurface returned %08x\n", hr);

        memset(&ddsd2, 0, sizeof(ddsd2));
        ddsd2.dwSize = sizeof(ddsd2);
        U4(ddsd2).ddpfPixelFormat.dwSize = sizeof(U4(ddsd2).ddpfPixelFormat);
        hr = IDirectDrawSurface7_GetSurfaceDesc(surface, &ddsd2);
        ok(hr == DD_OK, "GetSurfaceDesc returned %08x\n", hr);

        ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
        "Surface desc flags: %08x\n", ddsd2.dwFlags);
        ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
        ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
        ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY),
        "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
        ok(U1(ddsd2).dwLinearSize == 16384, "Linear size is %d\n", U1(ddsd2).dwLinearSize);
        ok(ddsd2.lpSurface == 0, "Surface memory is at %p, expected NULL\n", ddsd2.lpSurface);

        memset(&ddsd2, 0, sizeof(ddsd2));
        ddsd2.dwSize = sizeof(ddsd2);
        U4(ddsd2).ddpfPixelFormat.dwSize = sizeof(U4(ddsd2).ddpfPixelFormat);

        /* Show that the description is not changed when locking the surface. What is really interesting
        * about this is that DDSD_LPSURFACE isn't set.
        */
        hr = IDirectDrawSurface7_Lock(surface, NULL, &ddsd2, DDLOCK_READONLY, 0);
        ok(hr == DD_OK, "Lock returned %08x\n", hr);

        ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
        "Surface desc flags: %08x\n", ddsd2.dwFlags);
        ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
        ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
        ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY),
        "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
        ok(U1(ddsd2).dwLinearSize == 16384, "Linear size is %d\n", U1(ddsd2).dwLinearSize);
        ok(ddsd2.lpSurface != 0, "Surface memory is at NULL\n");

        hr = IDirectDrawSurface7_Unlock(surface, NULL);
        ok(hr == DD_OK, "Unlock returned %08x\n", hr);

        /* Now what about a locking rect?  */
        hr = IDirectDrawSurface7_Lock(surface, &r, &ddsd2, DDLOCK_READONLY, 0);
        ok(hr == DD_OK, "Lock returned %08x\n", hr);

        ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
        "Surface desc flags: %08x\n", ddsd2.dwFlags);
        ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
        ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
        ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY),
        "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
        ok(U1(ddsd2).dwLinearSize == 16384, "\"Linear\" size is %d\n", U1(ddsd2).dwLinearSize);
        ok(ddsd2.lpSurface != 0, "Surface memory is at NULL\n");

        hr = IDirectDrawSurface7_Unlock(surface, &r);
        ok(hr == DD_OK, "Unlock returned %08x\n", hr);

        /* Now what about a different locking offset? */
        hr = IDirectDrawSurface7_Lock(surface, &r2, &ddsd2, DDLOCK_READONLY, 0);
        ok(hr == DD_OK, "Lock returned %08x\n", hr);

        ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
        "Surface desc flags: %08x\n", ddsd2.dwFlags);
        ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
        ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
        ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY),
        "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
        ok(U1(ddsd2).dwLinearSize == 16384, "\"Linear\" size is %d\n", U1(ddsd2).dwLinearSize);
        ok(ddsd2.lpSurface != 0, "Surface memory is at NULL\n");

        hr = IDirectDrawSurface7_Unlock(surface, &r2);
        ok(hr == DD_OK, "Unlock returned %08x\n", hr);

        IDirectDrawSurface7_Release(surface);
    }
    else
    {
        skip("Hardware DXTN textures not supported\n");
    }

    IDirectDraw7_Release(dd7);
}

static void SizeTest(void)
{
    IDirectDrawSurface *dsurface = NULL;
    DDSURFACEDESC desc;
    HRESULT ret;
    HWND window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            100, 100, 160, 160, NULL, NULL, NULL, NULL);

    /* Create an offscreen surface surface without a size */
    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS;
    desc.ddsCaps.dwCaps |= DDSCAPS_OFFSCREENPLAIN;
    ret = IDirectDraw_CreateSurface(lpDD, &desc, &dsurface, NULL);
    ok(ret == DDERR_INVALIDPARAMS, "Creating an offscreen plain surface without a size info returned %08x (dsurface=%p)\n", ret, dsurface);
    if(dsurface)
    {
        trace("Surface at %p\n", dsurface);
        IDirectDrawSurface_Release(dsurface);
        dsurface = NULL;
    }

    /* Create an offscreen surface surface with only a width parameter */
    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH;
    desc.ddsCaps.dwCaps |= DDSCAPS_OFFSCREENPLAIN;
    desc.dwWidth = 128;
    ret = IDirectDraw_CreateSurface(lpDD, &desc, &dsurface, NULL);
    ok(ret == DDERR_INVALIDPARAMS, "Creating an offscreen plain surface without height info returned %08x\n", ret);
    if(dsurface)
    {
        IDirectDrawSurface_Release(dsurface);
        dsurface = NULL;
    }

    /* Create an offscreen surface surface with only a height parameter */
    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT;
    desc.ddsCaps.dwCaps |= DDSCAPS_OFFSCREENPLAIN;
    desc.dwHeight = 128;
    ret = IDirectDraw_CreateSurface(lpDD, &desc, &dsurface, NULL);
    ok(ret == DDERR_INVALIDPARAMS, "Creating an offscreen plain surface without width info returned %08x\n", ret);
    if(dsurface)
    {
        IDirectDrawSurface_Release(dsurface);
        dsurface = NULL;
    }

    /* Test 0 height. */
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    desc.dwWidth = 1;
    desc.dwHeight = 0;
    ret = IDirectDraw_CreateSurface(lpDD, &desc, &dsurface, NULL);
    ok(ret == DDERR_INVALIDPARAMS, "Creating a 0 height surface returned %#x, expected DDERR_INVALIDPARAMS.\n", ret);
    if (SUCCEEDED(ret)) IDirectDrawSurface_Release(dsurface);
    dsurface = NULL;

    /* Test 0 width. */
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    desc.dwWidth = 0;
    desc.dwHeight = 1;
    ret = IDirectDraw_CreateSurface(lpDD, &desc, &dsurface, NULL);
    ok(ret == DDERR_INVALIDPARAMS, "Creating a 0 width surface returned %#x, expected DDERR_INVALIDPARAMS.\n", ret);
    if (SUCCEEDED(ret)) IDirectDrawSurface_Release(dsurface);
    dsurface = NULL;

    /* Sanity check */
    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    desc.ddsCaps.dwCaps |= DDSCAPS_OFFSCREENPLAIN;
    desc.dwHeight = 128;
    desc.dwWidth = 128;
    ret = IDirectDraw_CreateSurface(lpDD, &desc, &dsurface, NULL);
    ok(ret == DD_OK, "Creating an offscreen plain surface with width and height info returned %08x\n", ret);
    if(dsurface)
    {
        IDirectDrawSurface_Release(dsurface);
        dsurface = NULL;
    }

    /* Test a primary surface size */
    ret = IDirectDraw_SetCooperativeLevel(lpDD, window, DDSCL_NORMAL);
    ok(ret == DD_OK, "SetCooperativeLevel failed with %08x\n", ret);

    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS;
    desc.ddsCaps.dwCaps |= DDSCAPS_PRIMARYSURFACE;
    desc.dwHeight = 128; /* Keep them set to  check what happens */
    desc.dwWidth = 128; /* Keep them set to  check what happens */
    ret = IDirectDraw_CreateSurface(lpDD, &desc, &dsurface, NULL);
    ok(ret == DD_OK, "Creating a primary surface without width and height info returned %08x\n", ret);
    if(dsurface)
    {
        ret = IDirectDrawSurface_GetSurfaceDesc(dsurface, &desc);
        ok(ret == DD_OK, "GetSurfaceDesc returned %x\n", ret);

        IDirectDrawSurface_Release(dsurface);
        dsurface = NULL;

        ok(desc.dwFlags & DDSD_WIDTH, "Primary surface doesn't have width set\n");
        ok(desc.dwFlags & DDSD_HEIGHT, "Primary surface doesn't have height set\n");
        ok(desc.dwWidth == GetSystemMetrics(SM_CXSCREEN), "Surface width differs from screen width\n");
        ok(desc.dwHeight == GetSystemMetrics(SM_CYSCREEN), "Surface height differs from screen height\n");
    }
    ret = IDirectDraw_SetCooperativeLevel(lpDD, NULL, DDSCL_NORMAL);
    ok(ret == DD_OK, "SetCooperativeLevel failed with %08x\n", ret);
}

static void BltParamTest(void)
{
    IDirectDrawSurface *surface1 = NULL, *surface2 = NULL;
    DDSURFACEDESC desc;
    HRESULT hr;
    DDBLTFX BltFx;
    RECT valid = {10, 10, 20, 20};
    RECT invalid1 = {20, 10, 10, 20};
    RECT invalid2 = {20, 20, 20, 20};
    RECT invalid3 = {-1, -1, 20, 20};
    RECT invalid4 = {60, 60, 70, 70};

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    desc.ddsCaps.dwCaps |= DDSCAPS_OFFSCREENPLAIN;
    desc.dwHeight = 128;
    desc.dwWidth = 128;
    hr = IDirectDraw_CreateSurface(lpDD, &desc, &surface1, NULL);
    ok(hr == DD_OK, "Creating an offscreen plain surface failed with %08x\n", hr);

    desc.dwHeight = 64;
    desc.dwWidth = 64;
    hr = IDirectDraw_CreateSurface(lpDD, &desc, &surface2, NULL);
    ok(hr == DD_OK, "Creating an offscreen plain surface failed with %08x\n", hr);

    if(0)
    {
        /* This crashes */
        hr = IDirectDrawSurface_BltFast(surface1, 0, 0, NULL, NULL, 0);
        ok(hr == DD_OK, "BltFast from NULL surface returned %08x\n", hr);
    }
    hr = IDirectDrawSurface_BltFast(surface1, 0, 0, surface2, NULL, 0);
    ok(hr == DD_OK, "BltFast from smaller to bigger surface returned %08x\n", hr);
    hr = IDirectDrawSurface_BltFast(surface2, 0, 0, surface1, NULL, 0);
    ok(hr == DDERR_INVALIDRECT, "BltFast from bigger to smaller surface returned %08x\n", hr);
    hr = IDirectDrawSurface_BltFast(surface2, 0, 0, surface1, &valid, 0);
    ok(hr == DD_OK, "BltFast from bigger to smaller surface using a valid rectangle returned %08x\n", hr);
    hr = IDirectDrawSurface_BltFast(surface2, 60, 60, surface1, &valid, 0);
    ok(hr == DDERR_INVALIDRECT, "BltFast with a rectangle resulting in an off-surface write returned %08x\n", hr);
    hr = IDirectDrawSurface_BltFast(surface1, 90, 90, surface2, NULL, 0);
    ok(hr == DDERR_INVALIDRECT, "BltFast with a rectangle resulting in an off-surface write returned %08x\n", hr);

    hr = IDirectDrawSurface_BltFast(surface1, -10, 0, surface2, NULL, 0);
    ok(hr == DDERR_INVALIDRECT, "BltFast with an offset resulting in an off-surface write returned %08x\n", hr);
    hr = IDirectDrawSurface_BltFast(surface1, 0, -10, surface2, NULL, 0);
    ok(hr == DDERR_INVALIDRECT, "BltFast with an offset resulting in an off-surface write returned %08x\n", hr);
    hr = IDirectDrawSurface_BltFast(surface2, 20, 20, surface1, &valid, 0);
    ok(hr == DD_OK, "BltFast from bigger to smaller surface using a valid rectangle and offset returned %08x\n", hr);

    hr = IDirectDrawSurface_BltFast(surface2, 0, 0, surface1, &invalid1, 0);
    ok(hr == DDERR_INVALIDRECT, "BltFast with invalid rectangle 1 returned %08x\n", hr);
    hr = IDirectDrawSurface_BltFast(surface2, 0, 0, surface1, &invalid2, 0);
    ok(hr == DDERR_INVALIDRECT, "BltFast with invalid rectangle 2 returned %08x\n", hr);
    hr = IDirectDrawSurface_BltFast(surface2, 0, 0, surface1, &invalid3, 0);
    ok(hr == DDERR_INVALIDRECT, "BltFast with invalid rectangle 3 returned %08x\n", hr);
    hr = IDirectDrawSurface_BltFast(surface1, 0, 0, surface2, &invalid4, 0);
    ok(hr == DDERR_INVALIDRECT, "BltFast with invalid rectangle 3 returned %08x\n", hr);
    hr = IDirectDrawSurface_BltFast(surface1, 0, 0, surface1, NULL, 0);
    ok(hr == DD_OK, "BltFast blitting a surface onto itself returned %08x\n", hr);

    /* Blt(non-fast) tests */
    memset(&BltFx, 0, sizeof(BltFx));
    BltFx.dwSize = sizeof(BltFx);
    U5(BltFx).dwFillColor = 0xaabbccdd;

    hr = IDirectDrawSurface_Blt(surface1, &valid, NULL, NULL, DDBLT_COLORFILL, &BltFx);
    ok(hr == DD_OK, "IDirectDrawSurface_Blt with a valid rectangle for color fill returned %08x\n", hr);
    hr = IDirectDrawSurface_Blt(surface1, &valid, NULL, &invalid3, DDBLT_COLORFILL, &BltFx);
    ok(hr == DD_OK, "IDirectDrawSurface_Blt with an invalid, unused rectangle returned %08x\n", hr);
    hr = IDirectDrawSurface_Blt(surface2, &invalid1, NULL, NULL, DDBLT_COLORFILL, &BltFx);
    ok(hr == DDERR_INVALIDRECT, "IDirectDrawSurface_Blt with a with invalid rectangle 1 returned %08x\n", hr);
    hr = IDirectDrawSurface_Blt(surface2, &invalid2, NULL, NULL, DDBLT_COLORFILL, &BltFx);
    ok(hr == DDERR_INVALIDRECT, "IDirectDrawSurface_Blt with a with invalid rectangle 2 returned %08x\n", hr);
    hr = IDirectDrawSurface_Blt(surface2, &invalid3, NULL, NULL, DDBLT_COLORFILL, &BltFx);
    ok(hr == DDERR_INVALIDRECT, "IDirectDrawSurface_Blt with a with invalid rectangle 3 returned %08x\n", hr);
    hr = IDirectDrawSurface_Blt(surface2, &invalid4, NULL, NULL, DDBLT_COLORFILL, &BltFx);
    ok(hr == DDERR_INVALIDRECT, "IDirectDrawSurface_Blt with a with invalid rectangle 4 returned %08x\n", hr);

    /* Valid on surface 1 */
    hr = IDirectDrawSurface_Blt(surface1, &invalid4, NULL, NULL, DDBLT_COLORFILL, &BltFx);
    ok(hr == DD_OK, "IDirectDrawSurface_Blt with a subrectangle fill returned %08x\n", hr);

    /* Works - stretched blit */
    hr = IDirectDrawSurface_Blt(surface1, NULL, surface2, NULL, 0, NULL);
    ok(hr == DD_OK, "IDirectDrawSurface_Blt from a smaller to a bigger surface returned %08x\n", hr);
    hr = IDirectDrawSurface_Blt(surface2, NULL, surface1, NULL, 0, NULL);
    ok(hr == DD_OK, "IDirectDrawSurface_Blt from a bigger to a smaller surface %08x\n", hr);

    /* Invalid dest rects in sourced blits */
    hr = IDirectDrawSurface_Blt(surface2, &invalid1, surface1, NULL, 0, NULL);
    ok(hr == DDERR_INVALIDRECT, "IDirectDrawSurface_Blt with a with invalid rectangle 1 returned %08x\n", hr);
    hr = IDirectDrawSurface_Blt(surface2, &invalid2, surface1, NULL, 0, NULL);
    ok(hr == DDERR_INVALIDRECT, "IDirectDrawSurface_Blt with a with invalid rectangle 2 returned %08x\n", hr);
    hr = IDirectDrawSurface_Blt(surface2, &invalid3, surface1, NULL, 0, NULL);
    ok(hr == DDERR_INVALIDRECT, "IDirectDrawSurface_Blt with a with invalid rectangle 3 returned %08x\n", hr);
    hr = IDirectDrawSurface_Blt(surface2, &invalid4, surface1, NULL, 0, NULL);
    ok(hr == DDERR_INVALIDRECT, "IDirectDrawSurface_Blt with a with invalid rectangle 4 returned %08x\n", hr);

    /* Invalid src rects */
    hr = IDirectDrawSurface_Blt(surface2, NULL, surface1, &invalid1, 0, NULL);
    ok(hr == DDERR_INVALIDRECT, "IDirectDrawSurface_Blt with a with invalid rectangle 1 returned %08x\n", hr);
    hr = IDirectDrawSurface_Blt(surface2, NULL, surface1, &invalid2, 0, NULL);
    ok(hr == DDERR_INVALIDRECT, "IDirectDrawSurface_Blt with a with invalid rectangle 2 returned %08x\n", hr);
    hr = IDirectDrawSurface_Blt(surface2, NULL, surface1, &invalid3, 0, NULL);
    ok(hr == DDERR_INVALIDRECT, "IDirectDrawSurface_Blt with a with invalid rectangle 3 returned %08x\n", hr);
    hr = IDirectDrawSurface_Blt(surface1, NULL, surface2, &invalid4, 0, NULL);
    ok(hr == DDERR_INVALIDRECT, "IDirectDrawSurface_Blt with a with invalid rectangle 4 returned %08x\n", hr);

    IDirectDrawSurface_Release(surface1);
    IDirectDrawSurface_Release(surface2);
}

static void PaletteTest(void)
{
    HRESULT hr;
    IDirectDrawSurface *lpSurf = NULL;
    IDirectDrawSurface *backbuffer = NULL;
    DDSCAPS ddscaps;
    DDSURFACEDESC ddsd;
    IDirectDrawPalette *palette = NULL;
    PALETTEENTRY Table[256];
    PALETTEENTRY palEntries[256];
    int i;

    for(i=0; i<256; i++)
    {
        Table[i].peRed   = 0xff;
        Table[i].peGreen = 0;
        Table[i].peBlue  = 0;
        Table[i].peFlags = 0;
    }

    /* Create a 8bit palette without DDPCAPS_ALLOW256 set */
    hr = IDirectDraw_CreatePalette(lpDD, DDPCAPS_8BIT, Table, &palette, NULL);
    ok(hr == DD_OK, "CreatePalette failed with %08x\n", hr);
    if (FAILED(hr)) goto err;
    /* Read back the palette and verify the entries. Without DDPCAPS_ALLOW256 set
    /  entry 0 and 255 should have been overwritten with black and white */
    hr = IDirectDrawPalette_GetEntries(palette , 0, 0, 256, &palEntries[0]);
    ok(hr == DD_OK, "GetEntries failed with %08x\n", hr);
    if(hr == DD_OK)
    {
        ok((palEntries[0].peRed == 0) && (palEntries[0].peGreen == 0) && (palEntries[0].peBlue == 0),
           "Palette entry 0 of a palette without DDPCAPS_ALLOW256 set should be (0,0,0) but it is (%d,%d,%d)\n",
           palEntries[0].peRed, palEntries[0].peGreen, palEntries[0].peBlue);
        ok((palEntries[255].peRed == 255) && (palEntries[255].peGreen == 255) && (palEntries[255].peBlue == 255),
           "Palette entry 255 of a palette without DDPCAPS_ALLOW256 set should be (255,255,255) but it is (%d,%d,%d)\n",
           palEntries[255].peRed, palEntries[255].peGreen, palEntries[255].peBlue);

        /* Entry 1-254 should contain red */
        for(i=1; i<255; i++)
            ok((palEntries[i].peRed == 255) && (palEntries[i].peGreen == 0) && (palEntries[i].peBlue == 0),
               "Palette entry %d should have contained (255,0,0) but was set to (%d,%d,%d)\n",
               i, palEntries[i].peRed, palEntries[i].peGreen, palEntries[i].peBlue);
    }

    /* CreatePalette without DDPCAPS_ALLOW256 ignores entry 0 and 255,
    /  now check we are able to update the entries afterwards. */
    hr = IDirectDrawPalette_SetEntries(palette , 0, 0, 256, &Table[0]);
    ok(hr == DD_OK, "SetEntries failed with %08x\n", hr);
    hr = IDirectDrawPalette_GetEntries(palette , 0, 0, 256, &palEntries[0]);
    ok(hr == DD_OK, "GetEntries failed with %08x\n", hr);
    if(hr == DD_OK)
    {
        ok((palEntries[0].peRed == 0) && (palEntries[0].peGreen == 0) && (palEntries[0].peBlue == 0),
           "Palette entry 0 should have been set to (0,0,0) but it contains (%d,%d,%d)\n",
           palEntries[0].peRed, palEntries[0].peGreen, palEntries[0].peBlue);
        ok((palEntries[255].peRed == 255) && (palEntries[255].peGreen == 255) && (palEntries[255].peBlue == 255),
           "Palette entry 255 should have been set to (255,255,255) but it contains (%d,%d,%d)\n",
           palEntries[255].peRed, palEntries[255].peGreen, palEntries[255].peBlue);
    }
    IDirectDrawPalette_Release(palette);

    /* Create a 8bit palette with DDPCAPS_ALLOW256 set */
    hr = IDirectDraw_CreatePalette(lpDD, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, Table, &palette, NULL);
    ok(hr == DD_OK, "CreatePalette failed with %08x\n", hr);
    if (FAILED(hr)) goto err;

    hr = IDirectDrawPalette_GetEntries(palette , 0, 0, 256, &palEntries[0]);
    ok(hr == DD_OK, "GetEntries failed with %08x\n", hr);
    if(hr == DD_OK)
    {
        /* All entries should contain red */
        for(i=0; i<256; i++)
            ok((palEntries[i].peRed == 255) && (palEntries[i].peGreen == 0) && (palEntries[i].peBlue == 0),
               "Palette entry %d should have contained (255,0,0) but was set to (%d,%d,%d)\n",
               i, palEntries[i].peRed, palEntries[i].peGreen, palEntries[i].peBlue);
    }

    /* Try to set palette to a non-palettized surface */
    ddsd.dwSize = sizeof(ddsd);
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = 800;
    ddsd.dwHeight = 600;
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(ddsd.ddpfPixelFormat).dwRGBBitCount = 32;
    U2(ddsd.ddpfPixelFormat).dwRBitMask = 0xFF0000;
    U3(ddsd.ddpfPixelFormat).dwGBitMask = 0x00FF00;
    U4(ddsd.ddpfPixelFormat).dwBBitMask = 0x0000FF;
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpSurf, NULL);
    ok(hr==DD_OK, "CreateSurface returned: %x\n",hr);
    if (FAILED(hr))
    {
        skip("failed to create surface\n");
        goto err;
    }

    hr = IDirectDrawSurface_SetPalette(lpSurf, palette);
    ok(hr == DDERR_INVALIDPIXELFORMAT, "CreateSurface returned: %x\n",hr);

    IDirectDrawPalette_Release(palette);
    palette = NULL;

    hr = IDirectDrawSurface_GetPalette(lpSurf, &palette);
    ok(hr == DDERR_NOPALETTEATTACHED, "CreateSurface returned: %x\n",hr);

    err:

    if (lpSurf) IDirectDrawSurface_Release(lpSurf);
    if (palette) IDirectDrawPalette_Release(palette);

    hr = IDirectDraw_CreatePalette(lpDD, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, Table, &palette, NULL);
    ok(hr == DD_OK, "CreatePalette failed with %08x\n", hr);

    ddsd.dwSize = sizeof(ddsd);
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_BACKBUFFERCOUNT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_COMPLEX | DDSCAPS_FLIP;
    ddsd.dwWidth = 64;
    ddsd.dwHeight = 64;
    ddsd.dwBackBufferCount = 1;
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
    U1(ddsd.ddpfPixelFormat).dwRGBBitCount = 8;

    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpSurf, NULL);
    ok(hr==DD_OK, "CreateSurface returned: %x\n",hr);
    if (FAILED(hr))
    {
        skip("failed to create surface\n");
        return;
    }

    ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
    hr = IDirectDrawSurface_GetAttachedSurface(lpSurf, &ddscaps, &backbuffer);
    ok(hr == DD_OK, "GetAttachedSurface returned: %x\n",hr);

    hr = IDirectDrawSurface_SetPalette(backbuffer, palette);
    ok(hr == DD_OK, "SetPalette returned: %x\n",hr);

    IDirectDrawPalette_Release(palette);
    palette = NULL;

    hr = IDirectDrawSurface_GetPalette(backbuffer, &palette);
    ok(hr == DD_OK, "CreateSurface returned: %x\n",hr);
    IDirectDrawPalette_Release(palette);

    IDirectDrawSurface_Release(backbuffer);
    IDirectDrawSurface_Release(lpSurf);
}

static void SurfaceCapsTest(void)
{
    DDSURFACEDESC create;
    DDSURFACEDESC desc;
    HRESULT hr;
    IDirectDrawSurface *surface1 = NULL;
    DDSURFACEDESC2 create2, desc2;
    IDirectDrawSurface7 *surface7 = NULL;
    IDirectDraw7 *dd7 = NULL;
    DWORD create_caps[] = {
        DDSCAPS_OFFSCREENPLAIN,
        DDSCAPS_TEXTURE,
        DDSCAPS_TEXTURE | DDSCAPS_ALLOCONLOAD,
        0,
        DDSCAPS_TEXTURE | DDSCAPS_ALLOCONLOAD | DDSCAPS_SYSTEMMEMORY,
        DDSCAPS_PRIMARYSURFACE,
        DDSCAPS_PRIMARYSURFACE | DDSCAPS_SYSTEMMEMORY,
        DDSCAPS_3DDEVICE,
        DDSCAPS_ZBUFFER,
        DDSCAPS_3DDEVICE | DDSCAPS_OFFSCREENPLAIN
    };
    DWORD expected_caps[] = {
        DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
        DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
        DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM | DDSCAPS_ALLOCONLOAD,
        DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
        DDSCAPS_TEXTURE | DDSCAPS_ALLOCONLOAD | DDSCAPS_SYSTEMMEMORY,
        DDSCAPS_PRIMARYSURFACE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM | DDSCAPS_VISIBLE,
        DDSCAPS_PRIMARYSURFACE | DDSCAPS_SYSTEMMEMORY | DDSCAPS_VISIBLE,
        DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
        DDSCAPS_ZBUFFER | DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY,
        DDSCAPS_3DDEVICE | DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM
    };
    UINT i;

    /* Tests various surface flags, what changes do they undergo during
     * surface creation. Forsaken engine expects texture surfaces without
     * memory flag to get a video memory flag right after creation. */

    if (!(ddcaps.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
    {
        skip("DDraw reported no VIDEOMEMORY cap. Broken video driver? Skipping surface caps tests.\n");
        return ;
    }

    for (i = 0; i < sizeof(create_caps) / sizeof(DWORD); i++)
    {
        memset(&create, 0, sizeof(create));
        create.dwSize = sizeof(create);
        create.ddsCaps.dwCaps = create_caps[i];
        create.dwFlags = DDSD_CAPS;

        if (!(create.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
        {
            create.dwFlags |= DDSD_HEIGHT | DDSD_WIDTH;
            create.dwHeight = 128;
            create.dwWidth = 128;
        }

        if (create.ddsCaps.dwCaps & DDSCAPS_ZBUFFER)
        {
            create.dwFlags |= DDSD_PIXELFORMAT;
            create.ddpfPixelFormat.dwSize = sizeof(create.ddpfPixelFormat);
            create.ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
            U1(create.ddpfPixelFormat).dwZBufferBitDepth = 16;
            U3(create.ddpfPixelFormat).dwZBitMask = 0x0000FFFF;
        }

        hr = IDirectDraw_CreateSurface(lpDD, &create, &surface1, NULL);
        ok(hr == DD_OK, "IDirectDraw_CreateSurface failed with %08x\n", hr);

        if (SUCCEEDED(hr))
        {
            memset(&desc, 0, sizeof(desc));
            desc.dwSize = sizeof(DDSURFACEDESC);
            hr = IDirectDrawSurface_GetSurfaceDesc(surface1, &desc);
            ok(hr == DD_OK, "IDirectDrawSurface_GetSurfaceDesc failed with %08x\n", hr);

            ok(desc.ddsCaps.dwCaps == expected_caps[i],
                    "GetSurfaceDesc test %d returned caps %x, expected %x\n",
                    i, desc.ddsCaps.dwCaps, expected_caps[i]);

            IDirectDrawSurface_Release(surface1);
        }
    }

    /* Test for differences in ddraw 7 */
    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw7, (void **) &dd7);
    ok(hr == DD_OK, "IDirectDraw_QueryInterface returned %08x\n", hr);
    if (FAILED(hr))
    {
        skip("Failed to get IDirectDraw7 interface, skipping tests\n");
    }
    else
    {
        for (i = 0; i < sizeof(create_caps) / sizeof(DWORD); i++)
        {
            memset(&create2, 0, sizeof(create2));
            create2.dwSize = sizeof(create2);
            create2.ddsCaps.dwCaps = create_caps[i];
            create2.dwFlags = DDSD_CAPS;

            if (!(create2.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
            {
                create2.dwFlags |= DDSD_HEIGHT | DDSD_WIDTH;
                create2.dwHeight = 128;
                create2.dwWidth = 128;
            }

            if (create2.ddsCaps.dwCaps & DDSCAPS_ZBUFFER)
            {
                create2.dwFlags |= DDSD_PIXELFORMAT;
                U4(create2).ddpfPixelFormat.dwSize = sizeof(U4(create2).ddpfPixelFormat);
                U4(create2).ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
                U1(U4(create2).ddpfPixelFormat).dwZBufferBitDepth = 16;
                U3(U4(create2).ddpfPixelFormat).dwZBitMask = 0x0000FFFF;
            }

            hr = IDirectDraw7_CreateSurface(dd7, &create2, &surface7, NULL);
            ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);

            if (SUCCEEDED(hr))
            {
                memset(&desc2, 0, sizeof(desc2));
                desc2.dwSize = sizeof(DDSURFACEDESC2);
                hr = IDirectDrawSurface7_GetSurfaceDesc(surface7, &desc2);
                ok(hr == DD_OK, "IDirectDrawSurface_GetSurfaceDesc failed with %08x\n", hr);

                ok(desc2.ddsCaps.dwCaps == expected_caps[i],
                        "GetSurfaceDesc test %d returned caps %x, expected %x\n",
                        i, desc2.ddsCaps.dwCaps, expected_caps[i]);

                IDirectDrawSurface7_Release(surface7);
            }
        }

        IDirectDraw7_Release(dd7);
    }

    memset(&create, 0, sizeof(create));
    create.dwSize = sizeof(create);
    create.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    create.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_VIDEOMEMORY;
    create.dwWidth = 64;
    create.dwHeight = 64;
    hr = IDirectDraw_CreateSurface(lpDD, &create, &surface1, NULL);
    ok(hr == DDERR_INVALIDCAPS, "Creating a SYSMEM | VIDMEM surface returned 0x%08x, expected DDERR_INVALIDCAPS\n", hr);
    if(surface1) IDirectDrawSurface_Release(surface1);
}

static BOOL can_create_primary_surface(void)
{
    DDSURFACEDESC ddsd;
    IDirectDrawSurface *surface;
    HRESULT hr;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface, NULL);
    if(FAILED(hr)) return FALSE;
    IDirectDrawSurface_Release(surface);
    return TRUE;
}

static void BackBufferCreateSurfaceTest(void)
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

    const DWORD caps = DDSCAPS_BACKBUFFER;
    const DWORD expected_caps = DDSCAPS_BACKBUFFER | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;

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

    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surf, NULL);
    ok(SUCCEEDED(hr), "IDirectDraw_CreateSurface failed: 0x%08x\n", hr);
    if (surf != NULL)
    {
        hr = IDirectDrawSurface_GetSurfaceDesc(surf, &created_ddsd);
        ok(SUCCEEDED(hr), "IDirectDraw_GetSurfaceDesc failed: 0x%08x\n", hr);
        ok(created_ddsd.ddsCaps.dwCaps == expected_caps,
           "GetSurfaceDesc returned caps %x, expected %x\n", created_ddsd.ddsCaps.dwCaps,
           expected_caps);
        IDirectDrawSurface_Release(surf);
    }

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw2, (void **) &dd2);
    ok(SUCCEEDED(hr), "IDirectDraw_QueryInterface failed: 0x%08x\n", hr);

    hr = IDirectDraw2_CreateSurface(dd2, &ddsd, &surf, NULL);
    ok(hr == DDERR_INVALIDCAPS, "IDirectDraw2_CreateSurface didn't return %x08x, but %x08x\n",
       DDERR_INVALIDCAPS, hr);

    IDirectDraw2_Release(dd2);

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw4, (void **) &dd4);
    ok(SUCCEEDED(hr), "IDirectDraw_QueryInterface failed: 0x%08x\n", hr);

    hr = IDirectDraw4_CreateSurface(dd4, &ddsd2, &surf4, NULL);
    ok(hr == DDERR_INVALIDCAPS, "IDirectDraw4_CreateSurface didn't return %x08x, but %x08x\n",
       DDERR_INVALIDCAPS, hr);

    IDirectDraw4_Release(dd4);

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw7, (void **) &dd7);
    ok(SUCCEEDED(hr), "IDirectDraw_QueryInterface failed: 0x%08x\n", hr);

    hr = IDirectDraw7_CreateSurface(dd7, &ddsd2, &surf7, NULL);
    ok(hr == DDERR_INVALIDCAPS, "IDirectDraw7_CreateSurface didn't return %x08x, but %x08x\n",
       DDERR_INVALIDCAPS, hr);

    IDirectDraw7_Release(dd7);
}

static void BackBufferAttachmentFlipTest(void)
{
    HRESULT hr;
    IDirectDrawSurface *surface1, *surface2, *surface3, *surface4;
    DDSURFACEDESC ddsd;
    HWND window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            100, 100, 160, 160, NULL, NULL, NULL, NULL);

    hr = IDirectDraw_SetCooperativeLevel(lpDD, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "SetCooperativeLevel returned %08x\n", hr);

    /* Perform attachment tests on a back-buffer */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
    ddsd.dwWidth = GetSystemMetrics(SM_CXSCREEN);
    ddsd.dwHeight = GetSystemMetrics(SM_CYSCREEN);
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface2, NULL);
    ok(SUCCEEDED(hr), "CreateSurface returned: %x\n",hr);

    if (surface2 != NULL)
    {
        /* Try a single primary and a two back buffers */
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface1, NULL);
        ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);

        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
        ddsd.dwWidth = GetSystemMetrics(SM_CXSCREEN);
        ddsd.dwHeight = GetSystemMetrics(SM_CYSCREEN);
        hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface3, NULL);
        ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);

        /* This one has a different size */
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
        ddsd.dwWidth = 128;
        ddsd.dwHeight = 128;
        hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface4, NULL);
        ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);

        hr = IDirectDrawSurface_AddAttachedSurface(surface1, surface2);
        todo_wine ok(hr == DD_OK || broken(hr == DDERR_CANNOTATTACHSURFACE),
           "Attaching a back buffer to a front buffer returned %08x\n", hr);
        if(SUCCEEDED(hr))
        {
            /* Try flipping the surfaces */
            hr = IDirectDrawSurface_Flip(surface1, NULL, DDFLIP_WAIT);
            todo_wine ok(hr == DD_OK, "IDirectDrawSurface_Flip returned 0x%08x\n", hr);
            hr = IDirectDrawSurface_Flip(surface2, NULL, DDFLIP_WAIT);
            todo_wine ok(hr == DDERR_NOTFLIPPABLE, "IDirectDrawSurface_Flip returned 0x%08x\n", hr);

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
            /* Try flipping the surfaces */
            hr = IDirectDrawSurface_Flip(surface1, NULL, DDFLIP_WAIT);
            todo_wine ok(hr == DD_OK, "IDirectDrawSurface_Flip returned 0x%08x\n", hr);
            hr = IDirectDrawSurface_Flip(surface2, NULL, DDFLIP_WAIT);
            todo_wine ok(hr == DDERR_NOTFLIPPABLE, "IDirectDrawSurface_Flip returned 0x%08x\n", hr);

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
            /* Try flipping the surfaces */
            hr = IDirectDrawSurface_Flip(surface3, NULL, DDFLIP_WAIT);
            todo_wine ok(hr == DD_OK, "IDirectDrawSurface_Flip returned 0x%08x\n", hr);
            hr = IDirectDrawSurface_Flip(surface2, NULL, DDFLIP_WAIT);
            todo_wine ok(hr == DDERR_NOTFLIPPABLE, "IDirectDrawSurface_Flip returned 0x%08x\n", hr);
            hr = IDirectDrawSurface_Flip(surface1, NULL, DDFLIP_WAIT);
            ok(hr == DDERR_NOTFLIPPABLE, "IDirectDrawSurface_Flip returned 0x%08x\n", hr);

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

    hr =IDirectDraw_SetCooperativeLevel(lpDD, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "SetCooperativeLevel returned %08x\n", hr);

    DestroyWindow(window);
}

static void CreateSurfaceBadCapsSizeTest(void)
{
    DDSURFACEDESC ddsd_ok;
    DDSURFACEDESC ddsd_bad1;
    DDSURFACEDESC ddsd_bad2;
    DDSURFACEDESC ddsd_bad3;
    DDSURFACEDESC ddsd_bad4;
    DDSURFACEDESC2 ddsd2_ok;
    DDSURFACEDESC2 ddsd2_bad1;
    DDSURFACEDESC2 ddsd2_bad2;
    DDSURFACEDESC2 ddsd2_bad3;
    DDSURFACEDESC2 ddsd2_bad4;
    IDirectDrawSurface *surf;
    IDirectDrawSurface4 *surf4;
    IDirectDrawSurface7 *surf7;
    HRESULT hr;
    IDirectDraw2 *dd2;
    IDirectDraw4 *dd4;
    IDirectDraw7 *dd7;

    const DWORD caps = DDSCAPS_OFFSCREENPLAIN;

    memset(&ddsd_ok, 0, sizeof(ddsd_ok));
    ddsd_ok.dwSize = sizeof(ddsd_ok);
    ddsd_ok.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd_ok.dwWidth = 64;
    ddsd_ok.dwHeight = 64;
    ddsd_ok.ddsCaps.dwCaps = caps;
    ddsd_bad1 = ddsd_ok;
    ddsd_bad1.dwSize--;
    ddsd_bad2 = ddsd_ok;
    ddsd_bad2.dwSize++;
    ddsd_bad3 = ddsd_ok;
    ddsd_bad3.dwSize = 0;
    ddsd_bad4 = ddsd_ok;
    ddsd_bad4.dwSize = sizeof(DDSURFACEDESC2);

    memset(&ddsd2_ok, 0, sizeof(ddsd2_ok));
    ddsd2_ok.dwSize = sizeof(ddsd2_ok);
    ddsd2_ok.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd2_ok.dwWidth = 64;
    ddsd2_ok.dwHeight = 64;
    ddsd2_ok.ddsCaps.dwCaps = caps;
    ddsd2_bad1 = ddsd2_ok;
    ddsd2_bad1.dwSize--;
    ddsd2_bad2 = ddsd2_ok;
    ddsd2_bad2.dwSize++;
    ddsd2_bad3 = ddsd2_ok;
    ddsd2_bad3.dwSize = 0;
    ddsd2_bad4 = ddsd2_ok;
    ddsd2_bad4.dwSize = sizeof(DDSURFACEDESC);

    hr = IDirectDraw_CreateSurface(lpDD, &ddsd_ok, &surf, NULL);
    ok(SUCCEEDED(hr), "IDirectDraw_CreateSurface failed: 0x%08x\n", hr);
    IDirectDrawSurface_Release(surf);

    hr = IDirectDraw_CreateSurface(lpDD, &ddsd_bad1, &surf, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd_bad2, &surf, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd_bad3, &surf, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd_bad4, &surf, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw_CreateSurface(lpDD, NULL, &surf, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw2, (void **) &dd2);
    ok(SUCCEEDED(hr), "IDirectDraw_QueryInterface failed: 0x%08x\n", hr);

    hr = IDirectDraw2_CreateSurface(dd2, &ddsd_ok, &surf, NULL);
    ok(SUCCEEDED(hr), "IDirectDraw2_CreateSurface failed: 0x%08x\n", hr);
    IDirectDrawSurface_Release(surf);

    hr = IDirectDraw2_CreateSurface(dd2, &ddsd_bad1, &surf, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw2_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw2_CreateSurface(dd2, &ddsd_bad2, &surf, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw2_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw2_CreateSurface(dd2, &ddsd_bad3, &surf, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw2_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw2_CreateSurface(dd2, &ddsd_bad4, &surf, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw2_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw2_CreateSurface(dd2, NULL, &surf, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw2_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);

    IDirectDraw2_Release(dd2);

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw4, (void **) &dd4);
    ok(SUCCEEDED(hr), "IDirectDraw_QueryInterface failed: 0x%08x\n", hr);

    hr = IDirectDraw4_CreateSurface(dd4, &ddsd2_ok, &surf4, NULL);
    ok(SUCCEEDED(hr), "IDirectDraw4_CreateSurface failed: 0x%08x\n", hr);
    IDirectDrawSurface4_Release(surf4);

    hr = IDirectDraw4_CreateSurface(dd4, &ddsd2_bad1, &surf4, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw4_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw4_CreateSurface(dd4, &ddsd2_bad2, &surf4, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw4_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw4_CreateSurface(dd4, &ddsd2_bad3, &surf4, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw4_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw4_CreateSurface(dd4, &ddsd2_bad4, &surf4, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw4_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw4_CreateSurface(dd4, NULL, &surf4, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw4_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);

    IDirectDraw4_Release(dd4);

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw7, (void **) &dd7);
    ok(SUCCEEDED(hr), "IDirectDraw_QueryInterface failed: 0x%08x\n", hr);

    hr = IDirectDraw7_CreateSurface(dd7, &ddsd2_ok, &surf7, NULL);
    ok(SUCCEEDED(hr), "IDirectDraw7_CreateSurface failed: 0x%08x\n", hr);
    IDirectDrawSurface7_Release(surf7);

    hr = IDirectDraw7_CreateSurface(dd7, &ddsd2_bad1, &surf7, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw7_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw7_CreateSurface(dd7, &ddsd2_bad2, &surf7, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw7_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw7_CreateSurface(dd7, &ddsd2_bad3, &surf7, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw7_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw7_CreateSurface(dd7, &ddsd2_bad4, &surf7, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw7_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw7_CreateSurface(dd7, NULL, &surf7, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw7_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);

    IDirectDraw7_Release(dd7);
}

static void reset_ddsd(DDSURFACEDESC *ddsd)
{
    memset(ddsd, 0, sizeof(*ddsd));
    ddsd->dwSize = sizeof(*ddsd);
}

static void no_ddsd_caps_test(void)
{
    DDSURFACEDESC ddsd;
    HRESULT hr;
    IDirectDrawSurface *surface;

    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface, NULL);
    ok(SUCCEEDED(hr), "IDirectDraw_CreateSurface failed, hr %#x.\n", hr);
    reset_ddsd(&ddsd);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &ddsd);
    IDirectDrawSurface_Release(surface);
    ok(ddsd.dwFlags & DDSD_CAPS, "DDSD_CAPS is not set\n");
    ok(ddsd.ddsCaps.dwCaps & DDSCAPS_OFFSCREENPLAIN, "DDSCAPS_OFFSCREENPLAIN is not set\n");

    reset_ddsd(&ddsd);
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface, NULL);
    ok(SUCCEEDED(hr), "IDirectDraw_CreateSurface failed, hr %#x.\n", hr);
    reset_ddsd(&ddsd);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &ddsd);
    IDirectDrawSurface_Release(surface);
    ok(ddsd.dwFlags & DDSD_CAPS, "DDSD_CAPS is not set\n");
    ok(ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE, "DDSCAPS_OFFSCREENPLAIN is not set\n");

    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_SYSTEMMEMORY;
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface, NULL);
    ok(hr == DDERR_INVALIDCAPS, "IDirectDraw_CreateSurface returned %#x, expected DDERR_INVALIDCAPS.\n", hr);
}

static void dump_format(const DDPIXELFORMAT *fmt)
{
    trace("dwFlags %08x, FourCC %08x, dwZBufferBitDepth %u, stencil %u\n", fmt->dwFlags, fmt->dwFourCC,
          U1(*fmt).dwZBufferBitDepth, U2(*fmt).dwStencilBitDepth);
    trace("dwZBitMask %08x, dwStencilBitMask %08x, dwRGBZBitMask %08x\n", U3(*fmt).dwZBitMask,
          U4(*fmt).dwStencilBitMask, U5(*fmt).dwRGBZBitMask);
}

static void zbufferbitdepth_test(void)
{
    enum zfmt_succeed
    {
        ZFMT_SUPPORTED_ALWAYS,
        ZFMT_SUPPORTED_NEVER,
        ZFMT_SUPPORTED_HWDEPENDENT
    };
    struct
    {
        DWORD depth;
        enum zfmt_succeed supported;
        DDPIXELFORMAT pf;
    }
    test_data[] =
    {
        {
            16, ZFMT_SUPPORTED_ALWAYS,
            {
                sizeof(DDPIXELFORMAT), DDPF_ZBUFFER, 0,
                {16}, {0}, {0x0000ffff}, {0x00000000}, {0x00000000}
            }
        },
        {
            24, ZFMT_SUPPORTED_HWDEPENDENT,
            {
                sizeof(DDPIXELFORMAT), DDPF_ZBUFFER, 0,
                {24}, {0}, {0x00ffffff}, {0x00000000}, {0x00000000}
            }
        },
        {
            32, ZFMT_SUPPORTED_HWDEPENDENT,
            {
                sizeof(DDPIXELFORMAT), DDPF_ZBUFFER, 0,
                {32}, {0}, {0xffffffff}, {0x00000000}, {0x00000000}
            }
        },
        /* Returns DDERR_INVALIDPARAMS instead of DDERR_INVALIDPIXELFORMAT.
         * Disabled for now
        {
            0, ZFMT_SUPPORTED_NEVER
        },
        */
        {
            15, ZFMT_SUPPORTED_NEVER
        },
        {
            28, ZFMT_SUPPORTED_NEVER
        },
        {
            40, ZFMT_SUPPORTED_NEVER
        },
    };

    DDSURFACEDESC ddsd;
    IDirectDrawSurface *surface;
    HRESULT hr;
    unsigned int i;
    DDCAPS caps;

    memset(&caps, 0, sizeof(caps));
    caps.dwSize = sizeof(caps);
    hr = IDirectDraw_GetCaps(lpDD, &caps, NULL);
    ok(SUCCEEDED(hr), "IDirectDraw_GetCaps failed, hr %#x.\n", hr);
    if (!(caps.ddsCaps.dwCaps & DDSCAPS_ZBUFFER))
    {
        skip("Z buffers not supported, skipping DDSD_ZBUFFERBITDEPTH test\n");
        return;
    }

    for (i = 0; i < (sizeof(test_data) / sizeof(*test_data)); i++)
    {
        reset_ddsd(&ddsd);
        ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_ZBUFFERBITDEPTH;
        ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
        ddsd.dwWidth = 256;
        ddsd.dwHeight = 256;
        U2(ddsd).dwZBufferBitDepth = test_data[i].depth;

        hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface, NULL);
        if (test_data[i].supported == ZFMT_SUPPORTED_ALWAYS)
        {
            ok(SUCCEEDED(hr), "IDirectDraw_CreateSurface failed, hr %#x.\n", hr);
        }
        else if (test_data[i].supported == ZFMT_SUPPORTED_NEVER)
        {
            ok(hr == DDERR_INVALIDPIXELFORMAT, "IDirectDraw_CreateSurface returned %#x, expected %x.\n",
               hr, DDERR_INVALIDPIXELFORMAT);
        }
        if (!surface) continue;

        reset_ddsd(&ddsd);
        hr = IDirectDrawSurface_GetSurfaceDesc(surface, &ddsd);
        ok(SUCCEEDED(hr), "IDirectDrawSurface_GetSurfaceDesc failed, hr %#x.\n", hr);
        IDirectDrawSurface_Release(surface);

        ok(ddsd.dwFlags & DDSD_ZBUFFERBITDEPTH, "DDSD_ZBUFFERBITDEPTH is not set\n");
        ok(!(ddsd.dwFlags & DDSD_PIXELFORMAT), "DDSD_PIXELFORMAT is set\n");
        /* Yet the ddpfPixelFormat member contains valid data */
        if (memcmp(&ddsd.ddpfPixelFormat, &test_data[i].pf, ddsd.ddpfPixelFormat.dwSize))
        {
            ok(0, "Unexpected format for depth %u\n", test_data[i].depth);
            dump_format(&ddsd.ddpfPixelFormat);
        }
    }

    /* DDSD_ZBUFFERBITDEPTH vs DDSD_PIXELFORMAT? */
    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_ZBUFFERBITDEPTH;
    ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    ddsd.dwWidth = 256;
    ddsd.dwHeight = 256;
    U2(ddsd).dwZBufferBitDepth = 24;
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
    U1(ddsd.ddpfPixelFormat).dwZBufferBitDepth = 16;
    U3(ddsd.ddpfPixelFormat).dwZBitMask = 0x0000ffff;

    surface = NULL;
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface, NULL);
    ok(SUCCEEDED(hr), "IDirectDraw_CreateSurface failed, hr %#x.\n", hr);
    if (!surface) return;
    reset_ddsd(&ddsd);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &ddsd);
    ok(SUCCEEDED(hr), "IDirectDrawSurface_GetSurfaceDesc failed, hr %#x.\n", hr);
    IDirectDrawSurface_Release(surface);
    ok(U1(ddsd.ddpfPixelFormat).dwZBufferBitDepth == 16, "Expected a 16bpp depth buffer, got %ubpp\n",
       U1(ddsd.ddpfPixelFormat).dwZBufferBitDepth);
    ok(ddsd.dwFlags & DDSD_ZBUFFERBITDEPTH, "DDSD_ZBUFFERBITDEPTH is not set\n");
    ok(!(ddsd.dwFlags & DDSD_PIXELFORMAT), "DDSD_PIXELFORMAT is set\n");
    ok(U2(ddsd).dwZBufferBitDepth == 16, "Expected dwZBufferBitDepth=16, got %u\n",
       U2(ddsd).dwZBufferBitDepth);

    /* DDSD_PIXELFORMAT vs invalid ZBUFFERBITDEPTH */
    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_ZBUFFERBITDEPTH | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    ddsd.dwWidth = 256;
    ddsd.dwHeight = 256;
    U2(ddsd).dwZBufferBitDepth = 40;
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
    U1(ddsd.ddpfPixelFormat).dwZBufferBitDepth = 16;
    U3(ddsd.ddpfPixelFormat).dwZBitMask = 0x0000ffff;
    surface = NULL;
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface, NULL);
    ok(SUCCEEDED(hr), "IDirectDraw_CreateSurface failed, hr %#x.\n", hr);
    if (surface) IDirectDrawSurface_Release(surface);

    /* Create a PIXELFORMAT-only surface, see if ZBUFFERBITDEPTH is set */
    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    ddsd.dwWidth = 256;
    ddsd.dwHeight = 256;
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
    U1(ddsd.ddpfPixelFormat).dwZBufferBitDepth = 16;
    U3(ddsd.ddpfPixelFormat).dwZBitMask = 0x0000ffff;
    surface = NULL;
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface, NULL);
    ok(SUCCEEDED(hr), "IDirectDrawSurface_GetSurfaceDesc failed, hr %#x.\n", hr);
    reset_ddsd(&ddsd);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &ddsd);
    ok(SUCCEEDED(hr), "IDirectDrawSurface_GetSurfaceDesc failed, hr %#x.\n", hr);
    IDirectDrawSurface_Release(surface);
    ok(U1(ddsd.ddpfPixelFormat).dwZBufferBitDepth == 16, "Expected a 16bpp depth buffer, got %ubpp\n",
       U1(ddsd.ddpfPixelFormat).dwZBufferBitDepth);
    ok(ddsd.dwFlags & DDSD_ZBUFFERBITDEPTH, "DDSD_ZBUFFERBITDEPTH is not set\n");
    ok(!(ddsd.dwFlags & DDSD_PIXELFORMAT), "DDSD_PIXELFORMAT is set\n");
    ok(U2(ddsd).dwZBufferBitDepth == 16, "Expected dwZBufferBitDepth=16, got %u\n",
       U2(ddsd).dwZBufferBitDepth);
}

static void test_ddsd(DDSURFACEDESC *ddsd, BOOL expect_pf, BOOL expect_zd, const char *name, DWORD z_bit_depth)
{
    IDirectDrawSurface *surface;
    IDirectDrawSurface7 *surface7;
    HRESULT hr;
    DDSURFACEDESC out;
    DDSURFACEDESC2 out2;

    hr = IDirectDraw_CreateSurface(lpDD, ddsd, &surface, NULL);
    if (hr == DDERR_NOZBUFFERHW)
    {
        skip("Z buffers not supported, skipping Z flag test\n");
        return;
    }
    ok(SUCCEEDED(hr), "IDirectDraw_CreateSurface failed, hr %#x.\n", hr);
    hr = IDirectDrawSurface_QueryInterface(surface, &IID_IDirectDrawSurface7, (void **) &surface7);
    ok(SUCCEEDED(hr), "IDirectDrawSurface_QueryInterface failed, hr %#x.\n", hr);

    reset_ddsd(&out);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &out);
    ok(SUCCEEDED(hr), "IDirectDrawSurface_GetSurfaceDesc failed, hr %#x.\n", hr);
    memset(&out2, 0, sizeof(out2));
    out2.dwSize = sizeof(out2);
    hr = IDirectDrawSurface7_GetSurfaceDesc(surface7, &out2);
    ok(SUCCEEDED(hr), "IDirectDrawSurface_GetSurfaceDesc failed, hr %#x.\n", hr);

    if (expect_pf)
    {
        ok(out.dwFlags & DDSD_PIXELFORMAT, "%s surface: Expected DDSD_PIXELFORMAT to be set\n", name);
        ok(out2.dwFlags & DDSD_PIXELFORMAT,
                "%s surface: Expected DDSD_PIXELFORMAT to be set in DDSURFACEDESC2\n", name);
    }
    else
    {
        ok(!(out.dwFlags & DDSD_PIXELFORMAT), "%s surface: Expected DDSD_PIXELFORMAT not to be set\n", name);
        ok(out2.dwFlags & DDSD_PIXELFORMAT,
                "%s surface: Expected DDSD_PIXELFORMAT to be set in DDSURFACEDESC2\n", name);
    }
    if (expect_zd)
    {
        ok(out.dwFlags & DDSD_ZBUFFERBITDEPTH, "%s surface: Expected DDSD_ZBUFFERBITDEPTH to be set\n", name);
        ok(U2(out).dwZBufferBitDepth == z_bit_depth, "ZBufferBitDepth is %u, expected %u\n",
                U2(out).dwZBufferBitDepth, z_bit_depth);
        ok(!(out2.dwFlags & DDSD_ZBUFFERBITDEPTH),
                "%s surface: Did not expect DDSD_ZBUFFERBITDEPTH to be set in DDSURFACEDESC2\n", name);
        /* dwMipMapCount and dwZBufferBitDepth share the same union */
        ok(U2(out2).dwMipMapCount == 0, "dwMipMapCount is %u, expected 0\n", U2(out2).dwMipMapCount);
    }
    else
    {
        ok(!(out.dwFlags & DDSD_ZBUFFERBITDEPTH), "%s surface: Expected DDSD_ZBUFFERBITDEPTH not to be set\n", name);
        ok(U2(out).dwZBufferBitDepth == 0, "ZBufferBitDepth is %u, expected 0\n", U2(out).dwZBufferBitDepth);
        ok(!(out2.dwFlags & DDSD_ZBUFFERBITDEPTH),
                "%s surface: Did not expect DDSD_ZBUFFERBITDEPTH to be set in DDSURFACEDESC2\n", name);
        ok(U2(out2).dwMipMapCount == 0, "dwMipMapCount is %u, expected 0\n", U2(out2).dwMipMapCount);
    }

    reset_ddsd(&out);
    hr = IDirectDrawSurface_Lock(surface, NULL, &out, 0, NULL);
    if (SUCCEEDED(hr))
    {
        hr = IDirectDrawSurface_Unlock(surface, NULL);
        ok(SUCCEEDED(hr), "IDirectDrawSurface_GetSurfaceDesc failed, hr %#x.\n", hr);

        /* DDSD_ZBUFFERBITDEPTH is never set on Nvidia, but follows GetSurfaceDesc rules on AMD */
        if (!expect_zd)
        {
            ok(!(out.dwFlags & DDSD_ZBUFFERBITDEPTH),
                "Lock %s surface: Expected DDSD_ZBUFFERBITDEPTH not to be set\n", name);
        }

        /* DDSD_PIXELFORMAT follows GetSurfaceDesc rules */
        if (expect_pf)
        {
            ok(out.dwFlags & DDSD_PIXELFORMAT, "%s surface: Expected DDSD_PIXELFORMAT to be set\n", name);
        }
        else
        {
            ok(!(out.dwFlags & DDSD_PIXELFORMAT),
                "Lock %s surface: Expected DDSD_PIXELFORMAT not to be set\n", name);
        }
        if (out.dwFlags & DDSD_ZBUFFERBITDEPTH)
            ok(U2(out).dwZBufferBitDepth == z_bit_depth, "ZBufferBitDepth is %u, expected %u\n",
                    U2(out).dwZBufferBitDepth, z_bit_depth);
        else
            ok(U2(out).dwZBufferBitDepth == 0, "ZBufferBitDepth is %u, expected 0\n", U2(out).dwZBufferBitDepth);
    }

    hr = IDirectDrawSurface7_Lock(surface7, NULL, &out2, 0, NULL);
    ok(SUCCEEDED(hr), "IDirectDrawSurface7_Lock failed, hr %#x.\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IDirectDrawSurface7_Unlock(surface7, NULL);
        ok(SUCCEEDED(hr), "IDirectDrawSurface7_Unlock failed, hr %#x.\n", hr);
        /* DDSD_PIXELFORMAT is always set, DDSD_ZBUFFERBITDEPTH never */
        ok(out2.dwFlags & DDSD_PIXELFORMAT,
                "Lock %s surface: Expected DDSD_PIXELFORMAT to be set in DDSURFACEDESC2\n", name);
        ok(!(out2.dwFlags & DDSD_ZBUFFERBITDEPTH),
                "Lock %s surface: Did not expect DDSD_ZBUFFERBITDEPTH to be set in DDSURFACEDESC2\n", name);
        ok(U2(out2).dwMipMapCount == 0, "dwMipMapCount is %u, expected 0\n", U2(out2).dwMipMapCount);
    }

    IDirectDrawSurface7_Release(surface7);
    IDirectDrawSurface_Release(surface);
}

static void pixelformat_flag_test(void)
{
    DDSURFACEDESC ddsd;
    DDCAPS caps;
    HRESULT hr;

    memset(&caps, 0, sizeof(caps));
    caps.dwSize = sizeof(caps);
    hr = IDirectDraw_GetCaps(lpDD, &caps, NULL);
    ok(SUCCEEDED(hr), "IDirectDraw_GetCaps failed, hr %#x.\n", hr);
    if (!(caps.ddsCaps.dwCaps & DDSCAPS_ZBUFFER))
    {
        skip("Z buffers not supported, skipping DDSD_PIXELFORMAT test\n");
        return;
    }

    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.dwWidth = 64;
    ddsd.dwHeight = 64;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    test_ddsd(&ddsd, TRUE, FALSE, "offscreen plain", ~0U);

    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    test_ddsd(&ddsd, TRUE, FALSE, "primary", ~0U);

    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_ZBUFFERBITDEPTH;
    ddsd.dwWidth = 64;
    ddsd.dwHeight = 64;
    U2(ddsd).dwZBufferBitDepth = 16;
    ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    test_ddsd(&ddsd, FALSE, TRUE, "Z buffer", 16);
}

static BOOL fourcc_supported(DWORD fourcc, DWORD caps)
{
    DDSURFACEDESC ddsd;
    HRESULT hr;
    IDirectDrawSurface *surface;

    reset_ddsd(&ddsd);
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd.dwWidth = 4;
    ddsd.dwHeight = 4;
    ddsd.ddsCaps.dwCaps = caps;
    ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
    ddsd.ddpfPixelFormat.dwFourCC = fourcc;
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface, NULL);
    if (FAILED(hr))
    {
        return FALSE;
    }
    IDirectDrawSurface_Release(surface);
    return TRUE;
}

static void partial_block_lock_test(void)
{
    IDirectDrawSurface7 *surface;
    HRESULT hr;
    DDSURFACEDESC2 ddsd;
    IDirectDraw7 *dd7;
    const struct
    {
        DWORD caps, caps2;
        const char *name;
        BOOL success;
    }
    pools[] =
    {
        {
            DDSCAPS_VIDEOMEMORY, 0,
            "D3DPOOL_DEFAULT", FALSE
        },
        {
            DDSCAPS_SYSTEMMEMORY, 0,
            "D3DPOOL_SYSTEMMEM", TRUE
        },
        {
            0, DDSCAPS2_TEXTUREMANAGE,
            "D3DPOOL_MANAGED", TRUE
        }
    };
    const struct
    {
        DWORD fourcc;
        DWORD caps;
        const char *name;
        unsigned int block_width;
        unsigned int block_height;
    }
    formats[] =
    {
        {MAKEFOURCC('D','X','T','1'), DDSCAPS_TEXTURE, "D3DFMT_DXT1", 4, 4},
        {MAKEFOURCC('D','X','T','2'), DDSCAPS_TEXTURE, "D3DFMT_DXT2", 4, 4},
        {MAKEFOURCC('D','X','T','3'), DDSCAPS_TEXTURE, "D3DFMT_DXT3", 4, 4},
        {MAKEFOURCC('D','X','T','4'), DDSCAPS_TEXTURE, "D3DFMT_DXT4", 4, 4},
        {MAKEFOURCC('D','X','T','5'), DDSCAPS_TEXTURE, "D3DFMT_DXT5", 4, 4},
        /* ATI2N surfaces aren't available in ddraw */
        {MAKEFOURCC('U','Y','V','Y'), DDSCAPS_OVERLAY, "D3DFMT_UYVY", 2, 1},
        {MAKEFOURCC('Y','U','Y','2'), DDSCAPS_OVERLAY, "D3DFMT_YUY2", 2, 1},
    };
    unsigned int i, j;
    RECT rect;

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw7, (void **) &dd7);
    ok(SUCCEEDED(hr), "QueryInterface failed, hr %#x.\n", hr);

    for (i = 0; i < sizeof(formats) / sizeof(formats[0]); i++)
    {
        if (!fourcc_supported(formats[i].fourcc, formats[i].caps | DDSCAPS_VIDEOMEMORY))
        {
            skip("%s surfaces not supported, skipping partial block lock test\n", formats[i].name);
            continue;
        }

        for (j = 0; j < (sizeof(pools) / sizeof(*pools)); j++)
        {
            if (formats[i].caps & DDSCAPS_OVERLAY && !(pools[j].caps & DDSCAPS_VIDEOMEMORY))
                continue;

            memset(&ddsd, 0, sizeof(ddsd));
            ddsd.dwSize = sizeof(ddsd);
            ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
            ddsd.dwWidth = 128;
            ddsd.dwHeight = 128;
            ddsd.ddsCaps.dwCaps = pools[j].caps | formats[i].caps;
            ddsd.ddsCaps.dwCaps2 = pools[j].caps2;
            U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_FOURCC;
            U4(ddsd).ddpfPixelFormat.dwFourCC = formats[i].fourcc;
            hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface, NULL);
            ok(SUCCEEDED(hr), "CreateSurface failed, hr %#x, format %s, pool %s\n",
                hr, formats[i].name, pools[j].name);

            /* All Windows versions allow partial block locks with DDSCAPS_SYSTEMMEMORY and
             * DDSCAPS2_TEXTUREMANAGE, just like in d3d8 and d3d9. Windows XP also allows those locks
             * with DDSCAPS_VIDEOMEMORY. Windows Vista and Windows 7 disallow partial locks of vidmem
             * surfaces, making the ddraw behavior consistent with d3d8 and 9.
             *
             * Mark the Windows XP behavior as broken until we find an application that needs it */
            if (formats[i].block_width > 1)
            {
                SetRect(&rect, formats[i].block_width >> 1, 0, formats[i].block_width, formats[i].block_height);
                hr = IDirectDrawSurface7_Lock(surface, &rect, &ddsd, 0, NULL);
                ok(SUCCEEDED(hr) == pools[j].success || broken(SUCCEEDED(hr)),
                        "Partial block lock %s, expected %s, format %s, pool %s\n",
                        SUCCEEDED(hr) ? "succeeded" : "failed", pools[j].success ? "success" : "failure",
                        formats[i].name, pools[j].name);
                if (SUCCEEDED(hr))
                {
                    hr = IDirectDrawSurface7_Unlock(surface, NULL);
                    ok(SUCCEEDED(hr), "Unlock failed, hr %#x.\n", hr);
                }

                SetRect(&rect, 0, 0, formats[i].block_width >> 1, formats[i].block_height);
                hr = IDirectDrawSurface7_Lock(surface, &rect, &ddsd, 0, NULL);
                ok(SUCCEEDED(hr) == pools[j].success || broken(SUCCEEDED(hr)),
                        "Partial block lock %s, expected %s, format %s, pool %s\n",
                        SUCCEEDED(hr) ? "succeeded" : "failed", pools[j].success ? "success" : "failure",
                        formats[i].name, pools[j].name);
                if (SUCCEEDED(hr))
                {
                    hr = IDirectDrawSurface7_Unlock(surface, NULL);
                    ok(SUCCEEDED(hr), "Unlock failed, hr %#x.\n", hr);
                }
            }

            if (formats[i].block_height > 1)
            {
                SetRect(&rect, 0, formats[i].block_height >> 1, formats[i].block_width, formats[i].block_height);
                hr = IDirectDrawSurface7_Lock(surface, &rect, &ddsd, 0, NULL);
                ok(SUCCEEDED(hr) == pools[j].success || broken(SUCCEEDED(hr)),
                        "Partial block lock %s, expected %s, format %s, pool %s\n",
                        SUCCEEDED(hr) ? "succeeded" : "failed", pools[j].success ? "success" : "failure",
                        formats[i].name, pools[j].name);
                if (SUCCEEDED(hr))
                {
                    hr = IDirectDrawSurface7_Unlock(surface, NULL);
                    ok(SUCCEEDED(hr), "Unlock failed, hr %#x.\n", hr);
                }

                SetRect(&rect, 0, 0, formats[i].block_width, formats[i].block_height >> 1);
                hr = IDirectDrawSurface7_Lock(surface, &rect, &ddsd, 0, NULL);
                ok(SUCCEEDED(hr) == pools[j].success || broken(SUCCEEDED(hr)),
                        "Partial block lock %s, expected %s, format %s, pool %s\n",
                        SUCCEEDED(hr) ? "succeeded" : "failed", pools[j].success ? "success" : "failure",
                        formats[i].name, pools[j].name);
                if (SUCCEEDED(hr))
                {
                    hr = IDirectDrawSurface7_Unlock(surface, NULL);
                    ok(SUCCEEDED(hr), "Unlock failed, hr %#x.\n", hr);
                }
            }

            SetRect(&rect, 0, 0, formats[i].block_width, formats[i].block_height);
            hr = IDirectDrawSurface7_Lock(surface, &rect, &ddsd, 0, NULL);
            ok(SUCCEEDED(hr), "Full block lock returned %08x, expected %08x, format %s, pool %s\n",
                    hr, DD_OK, formats[i].name, pools[j].name);
            if (SUCCEEDED(hr))
            {
                hr = IDirectDrawSurface7_Unlock(surface, NULL);
                ok(SUCCEEDED(hr), "Unlock failed, hr %#x.\n", hr);
            }

            IDirectDrawSurface7_Release(surface);
        }
    }

    IDirectDraw7_Release(dd7);
}

START_TEST(dsurface)
{
    HRESULT ret;
    IDirectDraw4 *dd4;

    HMODULE ddraw_mod = GetModuleHandleA("ddraw.dll");
    pDirectDrawCreateEx = (void *) GetProcAddress(ddraw_mod, "DirectDrawCreateEx");

    if (!CreateDirectDraw())
        return;

    ret = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw4, (void **) &dd4);
    if (ret == E_NOINTERFACE)
    {
        win_skip("DirectDraw4 and higher are not supported\n");
        ReleaseDirectDraw();
        return;
    }
    IDirectDraw_Release(dd4);

    if(!can_create_primary_surface())
    {
        skip("Unable to create primary surface\n");
        return;
    }

    ddcaps.dwSize = sizeof(DDCAPS);
    ret = IDirectDraw_GetCaps(lpDD, &ddcaps, NULL);
    if (ret != DD_OK)
    {
        skip("IDirectDraw_GetCaps failed with %08x\n", ret);
        return;
    }

    GetDDInterface_1();
    GetDDInterface_2();
    GetDDInterface_4();
    GetDDInterface_7();
    CubeMapTest();
    CompressedTest();
    SizeTest();
    BltParamTest();
    PaletteTest();
    SurfaceCapsTest();
    BackBufferCreateSurfaceTest();
    BackBufferAttachmentFlipTest();
    CreateSurfaceBadCapsSizeTest();
    no_ddsd_caps_test();
    zbufferbitdepth_test();
    pixelformat_flag_test();
    partial_block_lock_test();
    ReleaseDirectDraw();
}
