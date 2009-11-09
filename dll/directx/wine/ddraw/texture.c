/* Direct3D Texture
 * Copyright (c) 1998 Lionel ULMER
 * Copyright (c) 2006 Stefan DÖSINGER
 *
 * This file contains the implementation of interface Direct3DTexture2.
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
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "wine/exception.h"

#include "ddraw.h"
#include "d3d.h"

#include "ddraw_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d7);
WINE_DECLARE_DEBUG_CHANNEL(ddraw_thunk);

/*****************************************************************************
 * IUnknown interfaces. They are thunks to IDirectDrawSurface7
 *****************************************************************************/
static HRESULT WINAPI
Thunk_IDirect3DTextureImpl_2_QueryInterface(IDirect3DTexture2 *iface,
                                            REFIID riid,
                                            void **obj)
{
    IDirectDrawSurfaceImpl *This = surface_from_texture2(iface);
    TRACE("(%p)->(%s,%p) thunking to IDirectDrawSurface7 interface.\n", This, debugstr_guid(riid), obj);
    return IDirectDrawSurface7_QueryInterface((IDirectDrawSurface7 *)This, riid, obj);
}

static HRESULT WINAPI
Thunk_IDirect3DTextureImpl_1_QueryInterface(IDirect3DTexture *iface,
                                            REFIID riid,
                                            void **obj)
{
    IDirectDrawSurfaceImpl *This = surface_from_texture1(iface);
    TRACE("(%p)->(%s,%p) thunking to IDirectDrawSurface7 interface.\n", This, debugstr_guid(riid), obj);

    return IDirectDrawSurface7_QueryInterface((IDirectDrawSurface7 *)This, riid, obj);
}

static ULONG WINAPI
Thunk_IDirect3DTextureImpl_2_AddRef(IDirect3DTexture2 *iface)
{
    IDirectDrawSurfaceImpl *This = surface_from_texture2(iface);
    TRACE("(%p)->() thunking to IDirectDrawSurface7 interface.\n", This);

    return IDirectDrawSurface7_AddRef((IDirectDrawSurface7 *)This);
}

static ULONG WINAPI
Thunk_IDirect3DTextureImpl_1_AddRef(IDirect3DTexture *iface)
{
    IDirectDrawSurfaceImpl *This = surface_from_texture1(iface);
    TRACE("(%p)->() thunking to IDirectDrawSurface7 interface.\n", This);

    return IDirectDrawSurface7_AddRef((IDirectDrawSurface7 *)This);
}

static ULONG WINAPI
Thunk_IDirect3DTextureImpl_2_Release(IDirect3DTexture2 *iface)
{
    IDirectDrawSurfaceImpl *This = surface_from_texture2(iface);
    TRACE("(%p)->() thunking to IDirectDrawSurface7 interface.\n", This);

    return IDirectDrawSurface7_Release((IDirectDrawSurface7 *)This);
}


static ULONG WINAPI
Thunk_IDirect3DTextureImpl_1_Release(IDirect3DTexture *iface)
{
    IDirectDrawSurfaceImpl *This = surface_from_texture1(iface);
    TRACE("(%p)->() thunking to IDirectDrawSurface7 interface.\n", This);

    return IDirectDrawSurface7_Release((IDirectDrawSurface7 *)This);
}

/*****************************************************************************
 * IDirect3DTexture interface
 *****************************************************************************/

/*****************************************************************************
 * IDirect3DTexture1::Initialize
 *
 * The sdk says it's not implemented
 *
 * Params:
 *  ?
 *
 * Returns
 *  DDERR_UNSUPPORTED
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DTextureImpl_1_Initialize(IDirect3DTexture *iface,
                                  IDirect3DDevice *Direct3DDevice,
                                  IDirectDrawSurface *DDSurface)
{
    TRACE("(%p)->(%p,%p) Not implemented\n", iface, Direct3DDevice, DDSurface);
    return DDERR_UNSUPPORTED; /* Unchecked */
}

/*****************************************************************************
 * IDirect3DTexture2::PaletteChanged
 *
 * Informs the texture about a palette change
 *
 * Params:
 *  Start: Start index of the change
 *  Count: The number of changed entries
 *
 * Returns
 *  D3D_OK, because it's a stub
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DTextureImpl_PaletteChanged(IDirect3DTexture2 *iface,
                                         DWORD Start,
                                         DWORD Count)
{
    IDirectDrawSurfaceImpl *This = surface_from_texture2(iface);
    FIXME("(%p)->(%08x,%08x): stub!\n", This, Start, Count);
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DTextureImpl_1_PaletteChanged(IDirect3DTexture *iface,
                                            DWORD Start,
                                            DWORD Count)
{
    IDirectDrawSurfaceImpl *This = surface_from_texture1(iface);
    TRACE("(%p)->(%08x,%08x) thunking to IDirect3DTexture2 interface.\n", This, Start, Count);

    return IDirect3DTexture2_PaletteChanged((IDirect3DTexture2 *)&This->IDirect3DTexture2_vtbl, Start, Count);
}


/*****************************************************************************
 * IDirect3DTexture::Unload
 *
 * DX5 SDK: "The IDirect3DTexture2::Unload method is not implemented
 *
 *
 * Returns:
 *  DDERR_UNSUPPORTED
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DTextureImpl_1_Unload(IDirect3DTexture *iface)
{
    IDirectDrawSurfaceImpl *This = surface_from_texture1(iface);
    TRACE("(%p)->(): not implemented!\n", This);
    return DDERR_UNSUPPORTED;
}

/*****************************************************************************
 * IDirect3DTexture2::GetHandle
 *
 * Returns handle for the texture. At the moment, the interface
 * to the IWineD3DTexture is used.
 *
 * Params:
 *  Direct3DDevice2: Device this handle is assigned to
 *  Handle: Address to store the handle at.
 *
 * Returns:
 *  D3D_OK
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DTextureImpl_GetHandle(IDirect3DTexture2 *iface,
                                    IDirect3DDevice2 *Direct3DDevice2,
                                    D3DTEXTUREHANDLE *lpHandle)
{
    IDirectDrawSurfaceImpl *This = surface_from_texture2(iface);
    IDirect3DDeviceImpl *d3d = device_from_device2(Direct3DDevice2);

    TRACE("(%p)->(%p,%p)\n", This, d3d, lpHandle);

    EnterCriticalSection(&ddraw_cs);
    if(!This->Handle)
    {
        This->Handle = IDirect3DDeviceImpl_CreateHandle(d3d);
        if(This->Handle)
        {
            d3d->Handles[This->Handle - 1].ptr = This;
            d3d->Handles[This->Handle - 1].type = DDrawHandle_Texture;
        }
    }
    *lpHandle = This->Handle;

    TRACE(" returning handle %08x.\n", *lpHandle);

    LeaveCriticalSection(&ddraw_cs);
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DTextureImpl_1_GetHandle(IDirect3DTexture *iface,
                                       LPDIRECT3DDEVICE lpDirect3DDevice,
                                       LPD3DTEXTUREHANDLE lpHandle)
{
    IDirectDrawSurfaceImpl *This = surface_from_texture1(iface);
    IDirect3DDeviceImpl *d3d = device_from_device1(lpDirect3DDevice);
    IDirect3DTexture2 *d3d_texture2 = (IDirect3DTexture2 *)&This->IDirect3DTexture2_vtbl;
    IDirect3DDevice2 *d3d_device2 = (IDirect3DDevice2 *)&d3d->IDirect3DDevice2_vtbl;

    TRACE_(ddraw_thunk)("(%p)->(%p,%p) thunking to IDirect3DTexture2 interface.\n", This, d3d, lpHandle);

    return IDirect3DTexture2_GetHandle(d3d_texture2, d3d_device2, lpHandle);
}


/*****************************************************************************
 * get_sub_mimaplevel
 *
 * Helper function that returns the next mipmap level
 *
 * tex_ptr: Surface of which to return the next level
 *
 *****************************************************************************/
static IDirectDrawSurfaceImpl *
get_sub_mimaplevel(IDirectDrawSurfaceImpl *tex_ptr)
{
    /* Now go down the mipmap chain to the next surface */
    static DDSCAPS2 mipmap_caps = { DDSCAPS_MIPMAP | DDSCAPS_TEXTURE, 0, 0, {0} };
    LPDIRECTDRAWSURFACE7 next_level;
    IDirectDrawSurfaceImpl *surf_ptr;
    HRESULT hr;

    hr = IDirectDrawSurface7_GetAttachedSurface((IDirectDrawSurface7 *)tex_ptr, &mipmap_caps, &next_level);
    if (FAILED(hr)) return NULL;

    surf_ptr = (IDirectDrawSurfaceImpl *)next_level;
    IDirectDrawSurface7_Release(next_level);

    return surf_ptr;
}

/*****************************************************************************
 * IDirect3DTexture2::Load
 *
 * Loads a texture created with the DDSCAPS_ALLOCONLOAD
 *
 * This function isn't relayed to WineD3D because the whole interface is
 * implemented in DDraw only. For speed improvements a implementation which
 * takes OpenGL more into account could be placed into WineD3D.
 *
 * Params:
 *  D3DTexture2: Address of the texture to load
 *
 * Returns:
 *  D3D_OK on success
 *  D3DERR_TEXTURE_LOAD_FAILED.
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DTextureImpl_Load(IDirect3DTexture2 *iface,
                          IDirect3DTexture2 *D3DTexture2)
{
    IDirectDrawSurfaceImpl *This = surface_from_texture2(iface);
    IDirectDrawSurfaceImpl *src_ptr = surface_from_texture2(D3DTexture2);
    HRESULT ret_value = D3D_OK;
    if(src_ptr == This)
    {
        TRACE("copying surface %p to surface %p, why?\n", src_ptr, This);
        return ret_value;
    }

    TRACE("(%p)->(%p)\n", This, src_ptr);
    EnterCriticalSection(&ddraw_cs);

    if (((src_ptr->surface_desc.ddsCaps.dwCaps & DDSCAPS_MIPMAP) != (This->surface_desc.ddsCaps.dwCaps & DDSCAPS_MIPMAP)) ||
        (src_ptr->surface_desc.u2.dwMipMapCount != This->surface_desc.u2.dwMipMapCount))
    {
        ERR("Trying to load surfaces with different mip-map counts !\n");
    }

    while(1)
    {
        IWineD3DPalette *wine_pal, *wine_pal_src;
        IDirectDrawPalette *pal = NULL, *pal_src = NULL;
        DDSURFACEDESC *src_d, *dst_d;

        TRACE(" copying surface %p to surface %p (mipmap level %d)\n", src_ptr, This, src_ptr->mipmap_level);

        /* Suppress the ALLOCONLOAD flag */
        This->surface_desc.ddsCaps.dwCaps &= ~DDSCAPS_ALLOCONLOAD;

        /* Get the palettes */
        ret_value = IWineD3DSurface_GetPalette(This->WineD3DSurface, &wine_pal);
        if( ret_value != D3D_OK)
        {
            ERR("IWineD3DSurface::GetPalette failed! This is unexpected\n");
            LeaveCriticalSection(&ddraw_cs);
            return D3DERR_TEXTURE_LOAD_FAILED;
        }
        if(wine_pal)
        {
            ret_value = IWineD3DPalette_GetParent(wine_pal, (IUnknown **) &pal);
            if(ret_value != D3D_OK)
            {
                ERR("IWineD3DPalette::GetParent failed! This is unexpected\n");
                LeaveCriticalSection(&ddraw_cs);
                return D3DERR_TEXTURE_LOAD_FAILED;
            }
        }

        ret_value = IWineD3DSurface_GetPalette(src_ptr->WineD3DSurface, &wine_pal_src);
        if( ret_value != D3D_OK)
        {
            ERR("IWineD3DSurface::GetPalette failed! This is unexpected\n");
            LeaveCriticalSection(&ddraw_cs);
            return D3DERR_TEXTURE_LOAD_FAILED;
        }
        if(wine_pal_src)
        {
            ret_value = IWineD3DPalette_GetParent(wine_pal_src, (IUnknown **) &pal_src);
            if(ret_value != D3D_OK)
            {
                ERR("IWineD3DPalette::GetParent failed! This is unexpected\n");
                if (pal) IDirectDrawPalette_Release(pal);
                LeaveCriticalSection(&ddraw_cs);
                return D3DERR_TEXTURE_LOAD_FAILED;
            }
        }

        if (pal_src != NULL)
        {
            PALETTEENTRY palent[256];

            if (pal == NULL)
            {
                IDirectDrawPalette_Release(pal_src);
                LeaveCriticalSection(&ddraw_cs);
                return DDERR_NOPALETTEATTACHED;
            }
            IDirectDrawPalette_GetEntries(pal_src, 0, 0, 256, palent);
            IDirectDrawPalette_SetEntries(pal, 0, 0, 256, palent);
        }

        if (pal) IDirectDrawPalette_Release(pal);
        if (pal_src) IDirectDrawPalette_Release(pal_src);

        /* Copy one surface on the other */
        dst_d = (DDSURFACEDESC *)&(This->surface_desc);
        src_d = (DDSURFACEDESC *)&(src_ptr->surface_desc);

        if ((src_d->dwWidth != dst_d->dwWidth) || (src_d->dwHeight != dst_d->dwHeight))
        {
            /* Should also check for same pixel format, u1.lPitch, ... */
            ERR("Error in surface sizes\n");
            LeaveCriticalSection(&ddraw_cs);
            return D3DERR_TEXTURE_LOAD_FAILED;
        }
        else
        {
            WINED3DLOCKED_RECT pSrcRect, pDstRect;

            /* LPDIRECT3DDEVICE2 d3dd = (LPDIRECT3DDEVICE2) This->D3Ddevice; */
            /* I should put a macro for the calculus of bpp */

            /* Copy also the ColorKeying stuff */
            if (src_d->dwFlags & DDSD_CKSRCBLT)
            {
                dst_d->dwFlags |= DDSD_CKSRCBLT;
                dst_d->ddckCKSrcBlt.dwColorSpaceLowValue = src_d->ddckCKSrcBlt.dwColorSpaceLowValue;
                dst_d->ddckCKSrcBlt.dwColorSpaceHighValue = src_d->ddckCKSrcBlt.dwColorSpaceHighValue;
            }

            /* Copy the main memory texture into the surface that corresponds to the OpenGL
              texture object. */

            ret_value = IWineD3DSurface_LockRect(src_ptr->WineD3DSurface, &pSrcRect, NULL, 0);
            if(ret_value != D3D_OK)
            {
                ERR(" (%p) Locking the source surface failed\n", This);
                LeaveCriticalSection(&ddraw_cs);
                return D3DERR_TEXTURE_LOAD_FAILED;
            }

            ret_value = IWineD3DSurface_LockRect(This->WineD3DSurface, &pDstRect, NULL, 0);
            if(ret_value != D3D_OK)
            {
                ERR(" (%p) Locking the destination surface failed\n", This);
                IWineD3DSurface_UnlockRect(src_ptr->WineD3DSurface);
                LeaveCriticalSection(&ddraw_cs);
                return D3DERR_TEXTURE_LOAD_FAILED;
            }

            if (This->surface_desc.u4.ddpfPixelFormat.dwFlags & DDPF_FOURCC)
                memcpy(pDstRect.pBits, pSrcRect.pBits, src_ptr->surface_desc.u1.dwLinearSize);
            else
                memcpy(pDstRect.pBits, pSrcRect.pBits, pSrcRect.Pitch * src_d->dwHeight);

            IWineD3DSurface_UnlockRect(src_ptr->WineD3DSurface);
            IWineD3DSurface_UnlockRect(This->WineD3DSurface);
        }

        if (src_ptr->surface_desc.ddsCaps.dwCaps & DDSCAPS_MIPMAP)
        {
            src_ptr = get_sub_mimaplevel(src_ptr);
        }
        else
        {
            src_ptr = NULL;
        }
        if (This->surface_desc.ddsCaps.dwCaps & DDSCAPS_MIPMAP)
        {
            This = get_sub_mimaplevel(This);
        }
        else
        {
            This = NULL;
        }

        if ((src_ptr == NULL) || (This == NULL))
        {
            if (src_ptr != This)
            {
                ERR(" Loading surface with different mipmap structure !!!\n");
            }
            break;
        }
    }

    LeaveCriticalSection(&ddraw_cs);
    return ret_value;
}

static HRESULT WINAPI
Thunk_IDirect3DTextureImpl_1_Load(IDirect3DTexture *iface,
                                  IDirect3DTexture *D3DTexture)
{
    IDirectDrawSurfaceImpl *This = surface_from_texture1(iface);
    IDirectDrawSurfaceImpl *Texture = surface_from_texture1(D3DTexture);
    TRACE("(%p)->(%p) thunking to IDirect3DTexture2 interface.\n", This, Texture);

    return IDirect3DTexture2_Load((IDirect3DTexture2 *)&This->IDirect3DTexture2_vtbl,
            D3DTexture ? (IDirect3DTexture2 *)&surface_from_texture1(D3DTexture)->IDirect3DTexture2_vtbl : NULL);
}

/*****************************************************************************
 * The VTables
 *****************************************************************************/
const IDirect3DTexture2Vtbl IDirect3DTexture2_Vtbl =
{
    Thunk_IDirect3DTextureImpl_2_QueryInterface,
    Thunk_IDirect3DTextureImpl_2_AddRef,
    Thunk_IDirect3DTextureImpl_2_Release,
    IDirect3DTextureImpl_GetHandle,
    IDirect3DTextureImpl_PaletteChanged,
    IDirect3DTextureImpl_Load,
};


const IDirect3DTextureVtbl IDirect3DTexture1_Vtbl =
{
    Thunk_IDirect3DTextureImpl_1_QueryInterface,
    Thunk_IDirect3DTextureImpl_1_AddRef,
    Thunk_IDirect3DTextureImpl_1_Release,
    IDirect3DTextureImpl_1_Initialize,
    Thunk_IDirect3DTextureImpl_1_GetHandle,
    Thunk_IDirect3DTextureImpl_1_PaletteChanged,
    Thunk_IDirect3DTextureImpl_1_Load,
    IDirect3DTextureImpl_1_Unload,
};
