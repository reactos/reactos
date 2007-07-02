/*
 * Unit tests for (a few) ddraw surface functions
 *
 * Copyright (C) 2005 Antoine Chavasse (a.chavasse@gmail.com)
 * Copyright (C) 2005 Christian Costa
 * Copyright 2005 Ivan Leo Puoti
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

#include <assert.h>
#include "wine/test.h"
#include "ddraw.h"
#include "unknwn.h"

static LPDIRECTDRAW lpDD = NULL;

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

static void MipMapCreationTest(void)
{
    LPDIRECTDRAWSURFACE lpDDSMipMapTest;
    DDSURFACEDESC ddsd;
    HRESULT rc;

    /* First mipmap creation test: create a surface with DDSCAPS_COMPLEX,
       DDSCAPS_MIPMAP, and DDSD_MIPMAPCOUNT. This create the number of
        requested mipmap levels. */
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_MIPMAPCOUNT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    U2(ddsd).dwMipMapCount = 3;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 32;
    rc = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpDDSMipMapTest, NULL);
    ok(rc==DD_OK,"CreateSurface returned: %x\n",rc);

    /* Check the number of created mipmaps */
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(ddsd);
    rc = IDirectDrawSurface_GetSurfaceDesc(lpDDSMipMapTest, &ddsd);
    ok(rc==DD_OK,"GetSurfaceDesc returned: %x\n",rc);
    ok(ddsd.dwFlags & DDSD_MIPMAPCOUNT,
        "GetSurfaceDesc returned no mipmapcount.\n");
    ok(U2(ddsd).dwMipMapCount == 3, "Incorrect mipmap count: %d.\n",
        U2(ddsd).dwMipMapCount);

    /* Destroy the surface. */
    IDirectDrawSurface_Release(lpDDSMipMapTest);


    /* Second mipmap creation test: create a surface without a mipmap
       count, with DDSCAPS_MIPMAP and without DDSCAPS_COMPLEX.
       This creates a single mipmap level. */
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 32;
    rc = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpDDSMipMapTest, NULL);
    ok(rc==DD_OK,"CreateSurface returned: %x\n",rc);

    /* Check the number of created mipmaps */
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(ddsd);
    rc = IDirectDrawSurface_GetSurfaceDesc(lpDDSMipMapTest, &ddsd);
    ok(rc==DD_OK,"GetSurfaceDesc returned: %x\n",rc);
    ok(ddsd.dwFlags & DDSD_MIPMAPCOUNT,
        "GetSurfaceDesc returned no mipmapcount.\n");
    ok(U2(ddsd).dwMipMapCount == 1, "Incorrect mipmap count: %d.\n",
        U2(ddsd).dwMipMapCount);

    /* Destroy the surface. */
    IDirectDrawSurface_Release(lpDDSMipMapTest);


    /* Third mipmap creation test: create a surface with DDSCAPS_MIPMAP,
        DDSCAPS_COMPLEX and without DDSD_MIPMAPCOUNT.
       It's an undocumented features where a chain of mipmaps, starting from
       he specified size and down to the smallest size, is automatically
       created.
       Anarchy Online needs this feature to work. */
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 32;
    rc = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpDDSMipMapTest, NULL);
    ok(rc==DD_OK,"CreateSurface returned: %x\n",rc);

    /* Check the number of created mipmaps */
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(ddsd);
    rc = IDirectDrawSurface_GetSurfaceDesc(lpDDSMipMapTest, &ddsd);
    ok(rc==DD_OK,"GetSurfaceDesc returned: %x\n",rc);
    ok(ddsd.dwFlags & DDSD_MIPMAPCOUNT,
        "GetSurfaceDesc returned no mipmapcount.\n");
    ok(U2(ddsd).dwMipMapCount == 6, "Incorrect mipmap count: %d.\n",
        U2(ddsd).dwMipMapCount);

    /* Destroy the surface. */
    IDirectDrawSurface_Release(lpDDSMipMapTest);


    /* Fourth mipmap creation test: same as above with a different texture
       size.
       The purpose is to verify that the number of generated mipmaps is
       dependent on the smallest dimension. */
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    ddsd.dwWidth = 32;
    ddsd.dwHeight = 64;
    rc = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpDDSMipMapTest, NULL);
    ok(rc==DD_OK,"CreateSurface returned: %x\n",rc);

    /* Check the number of created mipmaps */
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(ddsd);
    rc = IDirectDrawSurface_GetSurfaceDesc(lpDDSMipMapTest, &ddsd);
    ok(rc==DD_OK,"GetSurfaceDesc returned: %x\n",rc);
    ok(ddsd.dwFlags & DDSD_MIPMAPCOUNT,
        "GetSurfaceDesc returned no mipmapcount.\n");
    ok(U2(ddsd).dwMipMapCount == 6, "Incorrect mipmap count: %d.\n",
        U2(ddsd).dwMipMapCount);

    /* Destroy the surface. */
    IDirectDrawSurface_Release(lpDDSMipMapTest);
}

static void SrcColorKey32BlitTest(void)
{
    LPDIRECTDRAWSURFACE lpSrc;
    LPDIRECTDRAWSURFACE lpDst;
    DDSURFACEDESC ddsd;
    DDSURFACEDESC ddsd2;
    DDCOLORKEY DDColorKey;
    LPDWORD lpData;
    HRESULT rc;
    DDBLTFX fx;

    ddsd2.dwSize = sizeof(ddsd2);
    ddsd2.ddpfPixelFormat.dwSize = sizeof(ddsd2.ddpfPixelFormat);

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
    rc = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpDst, NULL);
    ok(rc==DD_OK,"CreateSurface returned: %x\n",rc);

    ddsd.dwFlags |= DDSD_CKSRCBLT;
    ddsd.ddckCKSrcBlt.dwColorSpaceLowValue = 0xFF00FF;
    ddsd.ddckCKSrcBlt.dwColorSpaceHighValue = 0xFF00FF;
    rc = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpSrc, NULL);
    ok(rc==DD_OK,"CreateSurface returned: %x\n",rc);
    
    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    lpData = (LPDWORD)ddsd2.lpSurface;
    lpData[0] = 0xCCCCCCCC;
    lpData[1] = 0xCCCCCCCC;
    lpData[2] = 0xCCCCCCCC;
    lpData[3] = 0xCCCCCCCC;
    rc = IDirectDrawSurface_Unlock(lpDst, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    rc = IDirectDrawSurface_Lock(lpSrc, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    lpData = (LPDWORD)ddsd2.lpSurface;
    lpData[0] = 0x77010203;
    lpData[1] = 0x00010203;
    lpData[2] = 0x77FF00FF;
    lpData[3] = 0x00FF00FF;
    rc = IDirectDrawSurface_Unlock(lpSrc, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYSRC, NULL);

    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    lpData = (LPDWORD)ddsd2.lpSurface;
    ok((lpData[0]==0x77010203)&&(lpData[1]==0x00010203)&&(lpData[2]==0xCCCCCCCC)&&(lpData[3]==0xCCCCCCCC),
       "Destination data after blitting is not correct\n");
    rc = IDirectDrawSurface_Unlock(lpDst, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    /* Also test SetColorKey */
    IDirectDrawSurface_GetColorKey(lpSrc, DDCKEY_SRCBLT, &DDColorKey);
    ok(DDColorKey.dwColorSpaceLowValue == 0xFF00FF && DDColorKey.dwColorSpaceHighValue == 0xFF00FF,
       "GetColorKey does not return the colorkey used at surface creation\n");

    DDColorKey.dwColorSpaceLowValue = 0x00FF00;
    DDColorKey.dwColorSpaceHighValue = 0x00FF00;
    IDirectDrawSurface_SetColorKey(lpSrc, DDCKEY_SRCBLT, &DDColorKey);

    DDColorKey.dwColorSpaceLowValue = 0;
    DDColorKey.dwColorSpaceHighValue = 0;
    IDirectDrawSurface_GetColorKey(lpSrc, DDCKEY_SRCBLT, &DDColorKey);
    ok(DDColorKey.dwColorSpaceLowValue == 0x00FF00 && DDColorKey.dwColorSpaceHighValue == 0x00FF00,
       "GetColorKey does not return the colorkey set with SetColorKey\n");

    ddsd.ddckCKSrcBlt.dwColorSpaceLowValue = 0;
    ddsd.ddckCKSrcBlt.dwColorSpaceHighValue = 0;
    IDirectDrawSurface_GetSurfaceDesc(lpSrc, &ddsd);
    ok(ddsd.ddckCKSrcBlt.dwColorSpaceLowValue == 0x00FF00 && ddsd.ddckCKSrcBlt.dwColorSpaceHighValue == 0x00FF00,
       "GetSurfaceDesc does not return the colorkey set with SetColorKey\n");

    IDirectDrawSurface_Release(lpSrc);
    IDirectDrawSurface_Release(lpDst);

    /* start with a new set of surfaces to test the color keying parameters to blit */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CKSRCBLT | DDSD_CKDESTBLT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = 800;
    ddsd.dwHeight = 600;
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(ddsd.ddpfPixelFormat).dwRGBBitCount = 32;
    U2(ddsd.ddpfPixelFormat).dwRBitMask = 0xFF0000;
    U3(ddsd.ddpfPixelFormat).dwGBitMask = 0x00FF00;
    U4(ddsd.ddpfPixelFormat).dwBBitMask = 0x0000FF;
    ddsd.ddckCKDestBlt.dwColorSpaceLowValue = 0xFF0000;
    ddsd.ddckCKDestBlt.dwColorSpaceHighValue = 0xFF0000;
    ddsd.ddckCKSrcBlt.dwColorSpaceLowValue = 0x00FF00;
    ddsd.ddckCKSrcBlt.dwColorSpaceHighValue = 0x00FF00;
    rc = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpDst, NULL);
    ok(rc==DD_OK || rc == DDERR_NOCOLORKEYHW,"CreateSurface returned: %x\n",rc);
    if(FAILED(rc))
    {
        skip("Failed to create surface\n");
        return;
    }

    /* start with a new set of surfaces to test the color keying parameters to blit */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CKSRCBLT | DDSD_CKDESTBLT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = 800;
    ddsd.dwHeight = 600;
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(ddsd.ddpfPixelFormat).dwRGBBitCount = 32;
    U2(ddsd.ddpfPixelFormat).dwRBitMask = 0xFF0000;
    U3(ddsd.ddpfPixelFormat).dwGBitMask = 0x00FF00;
    U4(ddsd.ddpfPixelFormat).dwBBitMask = 0x0000FF;
    ddsd.ddckCKSrcBlt.dwColorSpaceLowValue = 0x0000FF;
    ddsd.ddckCKSrcBlt.dwColorSpaceHighValue = 0x0000FF;
    ddsd.ddckCKDestBlt.dwColorSpaceLowValue = 0x000000;
    ddsd.ddckCKDestBlt.dwColorSpaceHighValue = 0x000000;
    rc = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpSrc, NULL);
    ok(rc==DD_OK || rc == DDERR_NOCOLORKEYHW,"CreateSurface returned: %x\n",rc);
    if(FAILED(rc))
    {
        skip("Failed to create surface\n");
        IDirectDrawSurface_Release(lpDst);
        return;
    }

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    fx.ddckSrcColorkey.dwColorSpaceHighValue = 0x110000;
    fx.ddckSrcColorkey.dwColorSpaceLowValue = 0x110000;
    fx.ddckDestColorkey.dwColorSpaceHighValue = 0x001100;
    fx.ddckDestColorkey.dwColorSpaceLowValue = 0x001100;

    rc = IDirectDrawSurface_Lock(lpSrc, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    lpData = (LPDWORD)ddsd2.lpSurface;
    lpData[0] = 0x000000FF; /* Applies to src blt key in src surface */
    lpData[1] = 0x00000000; /* Applies to dst blt key in src surface */
    lpData[2] = 0x00FF0000; /* Dst color key in dst surface */
    lpData[3] = 0x0000FF00; /* Src color key in dst surface */
    lpData[4] = 0x00001100; /* Src color key in ddbltfx */
    lpData[5] = 0x00110000; /* Dst color key in ddbltfx */
    rc = IDirectDrawSurface_Unlock(lpSrc, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    lpData = (LPDWORD)ddsd2.lpSurface;
    lpData[0] = 0x55555555;
    lpData[1] = 0x55555555;
    lpData[2] = 0x55555555;
    lpData[3] = 0x55555555;
    lpData[4] = 0x55555555;
    lpData[5] = 0x55555555;
    rc = IDirectDrawSurface_Unlock(lpDst, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    /* Test a blit without keying */
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, 0, &fx);
    ok(rc == DD_OK, "IDirectDrawSurface_Blt returned %08x\n", rc);

    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    lpData = (LPDWORD)ddsd2.lpSurface;
    /* Should have copied src data unmodified to dst */
    ok(lpData[0] == 0x000000FF &&
       lpData[1] == 0x00000000 &&
       lpData[2] == 0x00FF0000 &&
       lpData[3] == 0x0000FF00 &&
       lpData[4] == 0x00001100 &&
       lpData[5] == 0x00110000, "Surface data after unkeyed blit does not match\n");

    lpData[0] = 0x55555555;
    lpData[1] = 0x55555555;
    lpData[2] = 0x55555555;
    lpData[3] = 0x55555555;
    lpData[4] = 0x55555555;
    lpData[5] = 0x55555555;
    rc = IDirectDrawSurface_Unlock(lpDst, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    /* Src key */
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYSRC, &fx);
    ok(rc == DD_OK, "IDirectDrawSurface_Blt returned %08x\n", rc);

    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    lpData = (LPDWORD)ddsd2.lpSurface;

    ok(lpData[0] == 0x55555555 && /* Here the src key applied */
       lpData[1] == 0x00000000 &&
       lpData[2] == 0x00FF0000 &&
       lpData[3] == 0x0000FF00 &&
       lpData[4] == 0x00001100 &&
       lpData[5] == 0x00110000, "Surface data after srckey blit does not match\n");

    lpData[0] = 0x55555555;
    lpData[1] = 0x55555555;
    lpData[2] = 0x55555555;
    lpData[3] = 0x55555555;
    lpData[4] = 0x55555555;
    lpData[5] = 0x55555555;
    rc = IDirectDrawSurface_Unlock(lpDst, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    /* Src override */
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYSRCOVERRIDE, &fx);
    ok(rc == DD_OK, "IDirectDrawSurface_Blt returned %08x\n", rc);

    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    lpData = (LPDWORD)ddsd2.lpSurface;

    ok(lpData[0] == 0x000000FF &&
       lpData[1] == 0x00000000 &&
       lpData[2] == 0x00FF0000 &&
       lpData[3] == 0x0000FF00 &&
       lpData[4] == 0x00001100 &&
       lpData[5] == 0x55555555, /* Override key applies here */
       "Surface data after src override key blit does not match\n");

    lpData[0] = 0x55555555;
    lpData[1] = 0x55555555;
    lpData[2] = 0x55555555;
    lpData[3] = 0x55555555;
    lpData[4] = 0x55555555;
    lpData[5] = 0x55555555;
    rc = IDirectDrawSurface_Unlock(lpDst, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    /* Src override AND src key. That is not supposed to work */
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYSRC | DDBLT_KEYSRCOVERRIDE, &fx);
    ok(rc == DDERR_INVALIDPARAMS, "IDirectDrawSurface_Blt returned %08x\n", rc);

    /* Verify that the destination is unchanged */
    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    lpData = (LPDWORD)ddsd2.lpSurface;

    ok(lpData[0] == 0x55555555 &&
       lpData[1] == 0x55555555 &&
       lpData[2] == 0x55555555 &&
       lpData[3] == 0x55555555 &&
       lpData[4] == 0x55555555 &&
       lpData[5] == 0x55555555, /* Override key applies here */
       "Surface data after src key blit with override does not match\n");

    lpData[0] = 0x00FF0000; /* Dest key in dst surface */
    lpData[1] = 0x00FF0000; /* Dest key in dst surface */
    lpData[2] = 0x00001100; /* Dest key in override */
    lpData[3] = 0x00001100; /* Dest key in override */
    lpData[4] = 0x00000000; /* Dest key in src surface */
    lpData[5] = 0x00000000; /* Dest key in src surface */
    rc = IDirectDrawSurface_Unlock(lpDst, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    /* Dest key blit */
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYDEST, &fx);
    ok(rc == DD_OK, "IDirectDrawSurface_Blt returned %08x\n", rc);

    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    lpData = (LPDWORD)ddsd2.lpSurface;

    /* DirectDraw uses the dest blit key from the SOURCE surface ! */
    ok(lpData[0] == 0x00ff0000 &&
       lpData[1] == 0x00ff0000 &&
       lpData[2] == 0x00001100 &&
       lpData[3] == 0x00001100 &&
       lpData[4] == 0x00001100 && /* Key applies here */
       lpData[5] == 0x00110000,   /* Key applies here */
       "Surface data after dest key blit does not match\n");

    lpData[0] = 0x00FF0000; /* Dest key in dst surface */
    lpData[1] = 0x00FF0000; /* Dest key in dst surface */
    lpData[2] = 0x00001100; /* Dest key in override */
    lpData[3] = 0x00001100; /* Dest key in override */
    lpData[4] = 0x00000000; /* Dest key in src surface */
    lpData[5] = 0x00000000; /* Dest key in src surface */
    rc = IDirectDrawSurface_Unlock(lpDst, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    /* Dest override key blit */
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYDESTOVERRIDE, &fx);
    ok(rc == DD_OK, "IDirectDrawSurface_Blt returned %08x\n", rc);

    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    lpData = (LPDWORD)ddsd2.lpSurface;

    ok(lpData[0] == 0x00FF0000 &&
       lpData[1] == 0x00FF0000 &&
       lpData[2] == 0x00FF0000 && /* Key applies here */
       lpData[3] == 0x0000FF00 && /* Key applies here */
       lpData[4] == 0x00000000 &&
       lpData[5] == 0x00000000,
       "Surface data after dest key override blit does not match\n");

    lpData[0] = 0x00FF0000; /* Dest key in dst surface */
    lpData[1] = 0x00FF0000; /* Dest key in dst surface */
    lpData[2] = 0x00001100; /* Dest key in override */
    lpData[3] = 0x00001100; /* Dest key in override */
    lpData[4] = 0x00000000; /* Dest key in src surface */
    lpData[5] = 0x00000000; /* Dest key in src surface */
    rc = IDirectDrawSurface_Unlock(lpDst, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    /* Dest override key blit. Supposed to fail too */
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYDEST | DDBLT_KEYDESTOVERRIDE, &fx);
    ok(rc == DDERR_INVALIDPARAMS, "IDirectDrawSurface_Blt returned %08x\n", rc);

    /* Check for unchanged data */
    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    lpData = (LPDWORD)ddsd2.lpSurface;

    ok(lpData[0] == 0x00FF0000 &&
       lpData[1] == 0x00FF0000 &&
       lpData[2] == 0x00001100 && /* Key applies here */
       lpData[3] == 0x00001100 && /* Key applies here */
       lpData[4] == 0x00000000 &&
       lpData[5] == 0x00000000,
       "Surface data with dest key and dest override does not match\n");

    lpData[0] = 0x00FF0000; /* Dest key in dst surface */
    lpData[1] = 0x00FF0000; /* Dest key in dst surface */
    lpData[2] = 0x00001100; /* Dest key in override */
    lpData[3] = 0x00001100; /* Dest key in override */
    lpData[4] = 0x00000000; /* Dest key in src surface */
    lpData[5] = 0x00000000; /* Dest key in src surface */
    rc = IDirectDrawSurface_Unlock(lpDst, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    /* Modify the source data a bit to give some more conclusive results */
    rc = IDirectDrawSurface_Lock(lpSrc, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    lpData = (LPDWORD)ddsd2.lpSurface;
    lpData[5] = 0x000000FF; /* Applies to src blt key in src surface */
    rc = IDirectDrawSurface_Unlock(lpSrc, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    /* Source and destination key */
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYDEST | DDBLT_KEYSRC, &fx);
    ok(rc == DD_OK, "IDirectDrawSurface_Blt returned %08x\n", rc);

    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    lpData = (LPDWORD)ddsd2.lpSurface;

    ok(lpData[0] == 0x00FF0000 && /* Masked by Destination key */
       lpData[1] == 0x00FF0000 && /* Masked by Destination key */
       lpData[2] == 0x00001100 && /* Masked by Destination key */
       lpData[3] == 0x00001100 && /* Masked by Destination key */
       lpData[4] == 0x00001100 && /* Allowed by destination key, not masked by source key */
       lpData[5] == 0x00000000,   /* Allowed by dst key, but masked by source key */
       "Surface data with src key and dest key blit does not match\n");

    lpData[0] = 0x00FF0000; /* Dest key in dst surface */
    lpData[1] = 0x00FF0000; /* Dest key in dst surface */
    lpData[2] = 0x00001100; /* Dest key in override */
    lpData[3] = 0x00001100; /* Dest key in override */
    lpData[4] = 0x00000000; /* Dest key in src surface */
    lpData[5] = 0x00000000; /* Dest key in src surface */
    rc = IDirectDrawSurface_Unlock(lpDst, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    /* Override keys without ddbltfx parameter fail */
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYDESTOVERRIDE, NULL);
    ok(rc == DDERR_INVALIDPARAMS, "IDirectDrawSurface_Blt returned %08x\n", rc);
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYSRCOVERRIDE, NULL);
    ok(rc == DDERR_INVALIDPARAMS, "IDirectDrawSurface_Blt returned %08x\n", rc);

    /* Try blitting without keys in the source surface*/
    rc = IDirectDrawSurface_SetColorKey(lpSrc, DDCKEY_SRCBLT, NULL);
    ok(rc == DD_OK, "SetColorKey returned %x\n", rc);
    rc = IDirectDrawSurface_SetColorKey(lpSrc, DDCKEY_DESTBLT, NULL);
    ok(rc == DD_OK, "SetColorKey returned %x\n", rc);

    /* That fails now. Do not bother to check that the data is unmodified */
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYSRC, &fx);
    ok(rc == DDERR_INVALIDPARAMS, "IDirectDrawSurface_Blt returned %08x\n", rc);

    /* Dest key blit still works. Which key is used this time??? */
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYDEST, &fx);
    ok(rc == DD_OK, "IDirectDrawSurface_Blt returned %08x\n", rc);

    /* With korrectly passed override keys no key in the surface is needed.
     * Again, the result was checked before, no need to do that again
     */
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYDESTOVERRIDE, &fx);
    ok(rc == DD_OK, "IDirectDrawSurface_Blt returned %08x\n", rc);
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYSRCOVERRIDE, &fx);
    ok(rc == DD_OK, "IDirectDrawSurface_Blt returned %08x\n", rc);

    IDirectDrawSurface_Release(lpSrc);
    IDirectDrawSurface_Release(lpDst);
}

static void QueryInterface(void)
{
    LPDIRECTDRAWSURFACE dsurface;
    DDSURFACEDESC surface;
    LPVOID object;
    HRESULT ret;

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

    /* Call IUnknown::QueryInterface */
    ret = IDirectDrawSurface_QueryInterface(dsurface, 0, &object);
    ok(ret == DDERR_INVALIDPARAMS, "IDirectDrawSurface::QueryInterface returned %x\n", ret);

    IDirectDrawSurface_Release(dsurface);
}

/* The following tests test which interface is returned by IDirectDrawSurfaceX::GetDDInterface.
 * It uses refcounts to test that and compares the interface addresses. Partially fits here, and
 * partially in the refcount test
 */

static unsigned long getref(IUnknown *iface)
{
    IUnknown_AddRef(iface);
    return IUnknown_Release(iface);
}

static void GetDDInterface_1(void)
{
    LPDIRECTDRAWSURFACE dsurface;
    LPDIRECTDRAWSURFACE2 dsurface2;
    DDSURFACEDESC surface;
    HRESULT ret;
    IDirectDraw2 *dd2;
    IDirectDraw4 *dd4;
    IDirectDraw7 *dd7;
    unsigned long ref1, ref2, ref4, ref7;
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
    ok(ref1 == 1, "IDirectDraw refcount is %ld\n", ref1);
    ref2 = getref((IUnknown *) dd2);
    ok(ref2 == 1, "IDirectDraw2 refcount is %ld\n", ref2);
    ref4 = getref((IUnknown *) dd4);
    ok(ref4 == 1, "IDirectDraw4 refcount is %ld\n", ref4);
    ref7 = getref((IUnknown *) dd7);
    ok(ref7 == 1, "IDirectDraw7 refcount is %ld\n", ref7);


    ret = IDirectDrawSurface2_GetDDInterface(dsurface2, &dd);
    ok(ret == DD_OK, "IDirectDrawSurface7_GetDDInterface returned %08x\n", ret);
    ok(getref((IUnknown *) lpDD) == ref1 + 1, "IDirectDraw refcount was increased by %ld\n", getref((IUnknown *) lpDD) - ref1);
    ok(getref((IUnknown *) dd2) == ref2 + 0, "IDirectDraw2 refcount was increased by %ld\n", getref((IUnknown *) dd2) - ref2);
    ok(getref((IUnknown *) dd4) == ref4 + 0, "IDirectDraw4 refcount was increased by %ld\n", getref((IUnknown *) dd4) - ref4);
    ok(getref((IUnknown *) dd7) == ref7 + 0, "IDirectDraw7 refcount was increased by %ld\n", getref((IUnknown *) dd7) - ref7);

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
    LPDIRECTDRAWSURFACE dsurface;
    LPDIRECTDRAWSURFACE2 dsurface2;
    DDSURFACEDESC surface;
    HRESULT ret;
    IDirectDraw2 *dd2;
    IDirectDraw4 *dd4;
    IDirectDraw7 *dd7;
    unsigned long ref1, ref2, ref4, ref7;
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
    ok(ref1 == 1, "IDirectDraw refcount is %ld\n", ref1);
    ref2 = getref((IUnknown *) dd2);
    ok(ref2 == 1, "IDirectDraw2 refcount is %ld\n", ref2);
    ref4 = getref((IUnknown *) dd4);
    ok(ref4 == 1, "IDirectDraw4 refcount is %ld\n", ref4);
    ref7 = getref((IUnknown *) dd7);
    ok(ref7 == 1, "IDirectDraw7 refcount is %ld\n", ref7);


    ret = IDirectDrawSurface2_GetDDInterface(dsurface2, &dd);
    ok(ret == DD_OK, "IDirectDrawSurface7_GetDDInterface returned %08x\n", ret);
    ok(getref((IUnknown *) lpDD) == ref1 + 0, "IDirectDraw refcount was increased by %ld\n", getref((IUnknown *) lpDD) - ref1);
    ok(getref((IUnknown *) dd2) == ref2 + 1, "IDirectDraw2 refcount was increased by %ld\n", getref((IUnknown *) dd2) - ref2);
    ok(getref((IUnknown *) dd4) == ref4 + 0, "IDirectDraw4 refcount was increased by %ld\n", getref((IUnknown *) dd4) - ref4);
    ok(getref((IUnknown *) dd7) == ref7 + 0, "IDirectDraw7 refcount was increased by %ld\n", getref((IUnknown *) dd7) - ref7);

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
    LPDIRECTDRAWSURFACE2 dsurface2;
    LPDIRECTDRAWSURFACE4 dsurface4;
    DDSURFACEDESC2 surface;
    HRESULT ret;
    IDirectDraw2 *dd2;
    IDirectDraw4 *dd4;
    IDirectDraw7 *dd7;
    unsigned long ref1, ref2, ref4, ref7;
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
    ok(ref1 == 1, "IDirectDraw refcount is %ld\n", ref1);
    ref2 = getref((IUnknown *) dd2);
    ok(ref2 == 1, "IDirectDraw2 refcount is %ld\n", ref2);
    ref4 = getref((IUnknown *) dd4);
    ok(ref4 == 2, "IDirectDraw4 refcount is %ld\n", ref4);
    ref7 = getref((IUnknown *) dd7);
    ok(ref7 == 1, "IDirectDraw7 refcount is %ld\n", ref7);

    ret = IDirectDrawSurface4_GetDDInterface(dsurface4, &dd);
    ok(ret == DD_OK, "IDirectDrawSurface7_GetDDInterface returned %08x\n", ret);
    ok(getref((IUnknown *) lpDD) == ref1 + 0, "IDirectDraw refcount was increased by %ld\n", getref((IUnknown *) lpDD) - ref1);
    ok(getref((IUnknown *) dd2) == ref2 + 0, "IDirectDraw2 refcount was increased by %ld\n", getref((IUnknown *) dd2) - ref2);
    ok(getref((IUnknown *) dd4) == ref4 + 1, "IDirectDraw4 refcount was increased by %ld\n", getref((IUnknown *) dd4) - ref4);
    ok(getref((IUnknown *) dd7) == ref7 + 0, "IDirectDraw7 refcount was increased by %ld\n", getref((IUnknown *) dd7) - ref7);

    ok(dd == dd4, "Returned interface pointer is not equal to the creation interface\n");
    IUnknown_Release((IUnknown *) dd);

    /* Now test what happens if we QI the surface for some other version - It should still return the creation interface */
    ret = IDirectDrawSurface2_GetDDInterface(dsurface2, &dd);
    ok(ret == DD_OK, "IDirectDrawSurface7_GetDDInterface returned %08x\n", ret);
    ok(getref((IUnknown *) lpDD) == ref1 + 0, "IDirectDraw refcount was increased by %ld\n", getref((IUnknown *) lpDD) - ref1);
    ok(getref((IUnknown *) dd2) == ref2 + 0, "IDirectDraw2 refcount was increased by %ld\n", getref((IUnknown *) dd2) - ref2);
    ok(getref((IUnknown *) dd4) == ref4 + 1, "IDirectDraw4 refcount was increased by %ld\n", getref((IUnknown *) dd4) - ref4);
    ok(getref((IUnknown *) dd7) == ref7 + 0, "IDirectDraw7 refcount was increased by %ld\n", getref((IUnknown *) dd7) - ref7);

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
    LPDIRECTDRAWSURFACE4 dsurface4;
    LPDIRECTDRAWSURFACE7 dsurface7;
    DDSURFACEDESC2 surface;
    HRESULT ret;
    IDirectDraw2 *dd2;
    IDirectDraw4 *dd4;
    IDirectDraw7 *dd7;
    unsigned long ref1, ref2, ref4, ref7;
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
    ok(ref1 == 1, "IDirectDraw refcount is %ld\n", ref1);
    ref2 = getref((IUnknown *) dd2);
    ok(ref2 == 1, "IDirectDraw2 refcount is %ld\n", ref2);
    ref4 = getref((IUnknown *) dd4);
    ok(ref4 == 1, "IDirectDraw4 refcount is %ld\n", ref4);
    ref7 = getref((IUnknown *) dd7);
    ok(ref7 == 2, "IDirectDraw7 refcount is %ld\n", ref7);

    ret = IDirectDrawSurface7_GetDDInterface(dsurface7, &dd);
    ok(ret == DD_OK, "IDirectDrawSurface7_GetDDInterface returned %08x\n", ret);
    ok(getref((IUnknown *) lpDD) == ref1 + 0, "IDirectDraw refcount was increased by %ld\n", getref((IUnknown *) lpDD) - ref1);
    ok(getref((IUnknown *) dd2) == ref2 + 0, "IDirectDraw2 refcount was increased by %ld\n", getref((IUnknown *) dd2) - ref2);
    ok(getref((IUnknown *) dd4) == ref4 + 0, "IDirectDraw4 refcount was increased by %ld\n", getref((IUnknown *) dd4) - ref4);
    ok(getref((IUnknown *) dd7) == ref7 + 1, "IDirectDraw7 refcount was increased by %ld\n", getref((IUnknown *) dd7) - ref7);

    ok(dd == dd7, "Returned interface pointer is not equal to the creation interface\n");
    IUnknown_Release((IUnknown *) dd);

    /* Now test what happens if we QI the surface for some other version - It should still return the creation interface */
    ret = IDirectDrawSurface4_GetDDInterface(dsurface4, &dd);
    ok(ret == DD_OK, "IDirectDrawSurface7_GetDDInterface returned %08x\n", ret);
    ok(getref((IUnknown *) lpDD) == ref1 + 0, "IDirectDraw refcount was increased by %ld\n", getref((IUnknown *) lpDD) - ref1);
    ok(getref((IUnknown *) dd2) == ref2 + 0, "IDirectDraw2 refcount was increased by %ld\n", getref((IUnknown *) dd2) - ref2);
    ok(getref((IUnknown *) dd4) == ref4 + 0, "IDirectDraw4 refcount was increased by %ld\n", getref((IUnknown *) dd4) - ref4);
    ok(getref((IUnknown *) dd7) == ref7 + 1, "IDirectDraw7 refcount was increased by %ld\n", getref((IUnknown *) dd7) - ref7);

    ok(dd == dd7, "Returned interface pointer is not equal to the creation interface\n");
    IUnknown_Release((IUnknown *) dd);

    IDirectDraw_Release(dd2);
    IDirectDraw_Release(dd4);
    IDirectDraw_Release(dd7);
    IDirectDrawSurface4_Release(dsurface4);
    IDirectDrawSurface7_Release(dsurface7);
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

static void EnumTest(void)
{
    HRESULT rc;
    DDSURFACEDESC ddsd;
    IDirectDrawSurface *surface;
    struct enumstruct ctx;

    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_MIPMAPCOUNT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    U2(ddsd).dwMipMapCount = 3;
    ddsd.dwWidth = 32;
    ddsd.dwHeight = 32;
    rc = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface, NULL);
    ok(rc==DD_OK,"CreateSurface returned: %x\n",rc);

    memset(&ctx, 0, sizeof(ctx));
    ctx.expected[0] = surface;
    rc = IDirectDrawSurface_GetAttachedSurface(ctx.expected[0], &ddsd.ddsCaps, &ctx.expected[1]);
    ok(rc == DD_OK, "GetAttachedSurface returned %08x\n", rc);
    rc = IDirectDrawSurface_GetAttachedSurface(ctx.expected[1], &ddsd.ddsCaps, &ctx.expected[2]);
    ok(rc == DD_OK, "GetAttachedSurface returned %08x\n", rc);
    rc = IDirectDrawSurface_GetAttachedSurface(ctx.expected[2], &ddsd.ddsCaps, &ctx.expected[3]);
    ok(rc == DDERR_NOTFOUND, "GetAttachedSurface returned %08x\n", rc);
    ctx.count = 0;

    rc = IDirectDraw_EnumSurfaces(lpDD, DDENUMSURFACES_DOESEXIST | DDENUMSURFACES_ALL, &ddsd, (void *) &ctx, enumCB);
    ok(rc == DD_OK, "IDirectDraw_EnumSurfaces returned %08x\n", rc);
    ok(ctx.count == 3, "%d surfaces enumerated, expected 3\n", ctx.count);

    IDirectDrawSurface_Release(ctx.expected[2]);
    IDirectDrawSurface_Release(ctx.expected[1]);
    IDirectDrawSurface_Release(surface);
}

START_TEST(dsurface)
{
    if (!CreateDirectDraw())
        return;
    MipMapCreationTest();
    SrcColorKey32BlitTest();
    QueryInterface();
    GetDDInterface_1();
    GetDDInterface_2();
    GetDDInterface_4();
    GetDDInterface_7();
    EnumTest();
    ReleaseDirectDraw();
}
