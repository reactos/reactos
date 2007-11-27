/*
 * IWineD3DSurface Implementation of management(non-rendering) functions
 *
 * Copyright 1998 Lionel Ulmer
 * Copyright 2000-2001 TransGaming Technologies Inc.
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006-2007 Stefan Dösinger for CodeWeavers
 * Copyright 2007 Henri Verbeet
 * Copyright 2006-2007 Roderick Colenbrander
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
#include "wined3d_private.h"

#include <assert.h>

WINE_DEFAULT_DEBUG_CHANNEL(d3d_surface);

/* Do NOT define GLINFO_LOCATION in this file. THIS CODE MUST NOT USE IT */

/* *******************************************
   IWineD3DSurface IUnknown parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DBaseSurfaceImpl_QueryInterface(IWineD3DSurface *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    /* Warn ,but be nice about things */
    TRACE("(%p)->(%s,%p)\n", This,debugstr_guid(riid),ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DResource)
        || IsEqualGUID(riid, &IID_IWineD3DSurface)) {
        IUnknown_AddRef((IUnknown*)iface);
        *ppobj = This;
        return S_OK;
        }
        *ppobj = NULL;
        return E_NOINTERFACE;
}

ULONG WINAPI IWineD3DBaseSurfaceImpl_AddRef(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->resource.ref);
    TRACE("(%p) : AddRef increasing from %d\n", This,ref - 1);
    return ref;
}

/* ****************************************************
   IWineD3DSurface IWineD3DResource parts follow
   **************************************************** */
HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetDevice(IWineD3DSurface *iface, IWineD3DDevice** ppDevice) {
    return IWineD3DResourceImpl_GetDevice((IWineD3DResource *)iface, ppDevice);
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetPrivateData(IWineD3DSurface *iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    return IWineD3DResourceImpl_SetPrivateData((IWineD3DResource *)iface, refguid, pData, SizeOfData, Flags);
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetPrivateData(IWineD3DSurface *iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    return IWineD3DResourceImpl_GetPrivateData((IWineD3DResource *)iface, refguid, pData, pSizeOfData);
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_FreePrivateData(IWineD3DSurface *iface, REFGUID refguid) {
    return IWineD3DResourceImpl_FreePrivateData((IWineD3DResource *)iface, refguid);
}

DWORD   WINAPI IWineD3DBaseSurfaceImpl_SetPriority(IWineD3DSurface *iface, DWORD PriorityNew) {
    return IWineD3DResourceImpl_SetPriority((IWineD3DResource *)iface, PriorityNew);
}

DWORD   WINAPI IWineD3DBaseSurfaceImpl_GetPriority(IWineD3DSurface *iface) {
    return IWineD3DResourceImpl_GetPriority((IWineD3DResource *)iface);
}

WINED3DRESOURCETYPE WINAPI IWineD3DBaseSurfaceImpl_GetType(IWineD3DSurface *iface) {
    TRACE("(%p) : calling resourceimpl_GetType\n", iface);
    return IWineD3DResourceImpl_GetType((IWineD3DResource *)iface);
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetParent(IWineD3DSurface *iface, IUnknown **pParent) {
    TRACE("(%p) : calling resourceimpl_GetParent\n", iface);
    return IWineD3DResourceImpl_GetParent((IWineD3DResource *)iface, pParent);
}

/* ******************************************************
   IWineD3DSurface IWineD3DSurface parts follow
   ****************************************************** */

HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetContainer(IWineD3DSurface* iface, REFIID riid, void** ppContainer) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    IWineD3DBase *container = 0;

    TRACE("(This %p, riid %s, ppContainer %p)\n", This, debugstr_guid(riid), ppContainer);

    if (!ppContainer) {
        ERR("Called without a valid ppContainer.\n");
    }

    /** From MSDN:
     * If the surface is created using CreateImageSurface/CreateOffscreenPlainSurface, CreateRenderTarget,
     * or CreateDepthStencilSurface, the surface is considered stand alone. In this case,
     * GetContainer will return the Direct3D device used to create the surface.
     */
    if (This->container) {
        container = This->container;
    } else {
        container = (IWineD3DBase *)This->resource.wineD3DDevice;
    }

    TRACE("Relaying to QueryInterface\n");
    return IUnknown_QueryInterface(container, riid, ppContainer);
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetDesc(IWineD3DSurface *iface, WINED3DSURFACE_DESC *pDesc) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;

    TRACE("(%p) : copying into %p\n", This, pDesc);
    if(pDesc->Format != NULL)             *(pDesc->Format) = This->resource.format;
    if(pDesc->Type != NULL)               *(pDesc->Type)   = This->resource.resourceType;
    if(pDesc->Usage != NULL)              *(pDesc->Usage)              = This->resource.usage;
    if(pDesc->Pool != NULL)               *(pDesc->Pool)               = This->resource.pool;
    if(pDesc->Size != NULL)               *(pDesc->Size)               = This->resource.size;   /* dx8 only */
    if(pDesc->MultiSampleType != NULL)    *(pDesc->MultiSampleType)    = This->currentDesc.MultiSampleType;
    if(pDesc->MultiSampleQuality != NULL) *(pDesc->MultiSampleQuality) = This->currentDesc.MultiSampleQuality;
    if(pDesc->Width != NULL)              *(pDesc->Width)              = This->currentDesc.Width;
    if(pDesc->Height != NULL)             *(pDesc->Height)             = This->currentDesc.Height;
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetBltStatus(IWineD3DSurface *iface, DWORD Flags) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    TRACE("(%p)->(%x)\n", This, Flags);

    switch (Flags)
    {
        case WINEDDGBS_CANBLT:
        case WINEDDGBS_ISBLTDONE:
            return WINED3D_OK;

        default:
            return WINED3DERR_INVALIDCALL;
    }
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetFlipStatus(IWineD3DSurface *iface, DWORD Flags) {
    /* XXX: DDERR_INVALIDSURFACETYPE */

    TRACE("(%p)->(%08x)\n",iface,Flags);
    switch (Flags) {
        case WINEDDGFS_CANFLIP:
        case WINEDDGFS_ISFLIPDONE:
            return WINED3D_OK;

        default:
            return WINED3DERR_INVALIDCALL;
    }
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_IsLost(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    TRACE("(%p)\n", This);

    /* D3D8 and 9 loose full devices, ddraw only surfaces */
    return This->Flags & SFLAG_LOST ? WINED3DERR_DEVICELOST : WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_Restore(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    TRACE("(%p)\n", This);

    /* So far we don't lose anything :) */
    This->Flags &= ~SFLAG_LOST;
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetPalette(IWineD3DSurface *iface, IWineD3DPalette *Pal) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    IWineD3DPaletteImpl *PalImpl = (IWineD3DPaletteImpl *) Pal;
    TRACE("(%p)->(%p)\n", This, Pal);

    if(This->palette != NULL)
        if(This->resource.usage & WINED3DUSAGE_RENDERTARGET)
            This->palette->Flags &= ~WINEDDPCAPS_PRIMARYSURFACE;

    if(PalImpl != NULL) {
        if(This->resource.usage & WINED3DUSAGE_RENDERTARGET) {
            /* Set the device's main palette if the palette
            * wasn't a primary palette before
            */
            if(!(PalImpl->Flags & WINEDDPCAPS_PRIMARYSURFACE)) {
                IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;
                unsigned int i;

                for(i=0; i < 256; i++) {
                    device->palettes[device->currentPalette][i] = PalImpl->palents[i];
                }
            }

            (PalImpl)->Flags |= WINEDDPCAPS_PRIMARYSURFACE;
        }
    }
    This->palette = PalImpl;

    return IWineD3DSurface_RealizePalette(iface);
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetColorKey(IWineD3DSurface *iface, DWORD Flags, WINEDDCOLORKEY *CKey) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    TRACE("(%p)->(%08x,%p)\n", This, Flags, CKey);

    if ((Flags & WINEDDCKEY_COLORSPACE) != 0) {
        FIXME(" colorkey value not supported (%08x) !\n", Flags);
        return WINED3DERR_INVALIDCALL;
    }

    /* Dirtify the surface, but only if a key was changed */
    if(CKey) {
        switch (Flags & ~WINEDDCKEY_COLORSPACE) {
            case WINEDDCKEY_DESTBLT:
                This->DestBltCKey = *CKey;
                This->CKeyFlags |= WINEDDSD_CKDESTBLT;
                break;

            case WINEDDCKEY_DESTOVERLAY:
                This->DestOverlayCKey = *CKey;
                This->CKeyFlags |= WINEDDSD_CKDESTOVERLAY;
                break;

            case WINEDDCKEY_SRCOVERLAY:
                This->SrcOverlayCKey = *CKey;
                This->CKeyFlags |= WINEDDSD_CKSRCOVERLAY;
                break;

            case WINEDDCKEY_SRCBLT:
                This->SrcBltCKey = *CKey;
                This->CKeyFlags |= WINEDDSD_CKSRCBLT;
                break;
        }
    }
    else {
        switch (Flags & ~WINEDDCKEY_COLORSPACE) {
            case WINEDDCKEY_DESTBLT:
                This->CKeyFlags &= ~WINEDDSD_CKDESTBLT;
                break;

            case WINEDDCKEY_DESTOVERLAY:
                This->CKeyFlags &= ~WINEDDSD_CKDESTOVERLAY;
                break;

            case WINEDDCKEY_SRCOVERLAY:
                This->CKeyFlags &= ~WINEDDSD_CKSRCOVERLAY;
                break;

            case WINEDDCKEY_SRCBLT:
                This->CKeyFlags &= ~WINEDDSD_CKSRCBLT;
                break;
        }
    }

    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetPalette(IWineD3DSurface *iface, IWineD3DPalette **Pal) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    TRACE("(%p)->(%p)\n", This, Pal);

    *Pal = (IWineD3DPalette *) This->palette;
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_RealizePalette(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    RGBQUAD col[256];
    IWineD3DPaletteImpl *pal = This->palette;
    unsigned int n;
    TRACE("(%p)\n", This);

    if(This->resource.format == WINED3DFMT_P8 ||
       This->resource.format == WINED3DFMT_A8P8)
    {
        if(!This->Flags & SFLAG_INSYSMEM) {
            FIXME("Palette changed with surface that does not have an up to date system memory copy\n");
        }
        TRACE("Dirtifying surface\n");
        IWineD3DSurface_ModifyLocation(iface, SFLAG_INSYSMEM, TRUE);
    }

    if(This->Flags & SFLAG_DIBSECTION) {
        TRACE("(%p): Updating the hdc's palette\n", This);
        for (n=0; n<256; n++) {
            if(pal) {
                col[n].rgbRed   = pal->palents[n].peRed;
                col[n].rgbGreen = pal->palents[n].peGreen;
                col[n].rgbBlue  = pal->palents[n].peBlue;
            } else {
                IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;
                /* Use the default device palette */
                col[n].rgbRed   = device->palettes[device->currentPalette][n].peRed;
                col[n].rgbGreen = device->palettes[device->currentPalette][n].peGreen;
                col[n].rgbBlue  = device->palettes[device->currentPalette][n].peBlue;
            }
            col[n].rgbReserved = 0;
        }
        SetDIBColorTable(This->hDC, 0, 256, col);
    }

    return WINED3D_OK;
}

DWORD WINAPI IWineD3DBaseSurfaceImpl_GetPitch(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    DWORD ret;
    TRACE("(%p)\n", This);

    /* DXTn formats don't have exact pitches as they are to the new row of blocks,
    where each block is 4x4 pixels, 8 bytes (dxt1) and 16 bytes (dxt2/3/4/5)
    ie pitch = (width/4) * bytes per block                                  */
    if (This->resource.format == WINED3DFMT_DXT1) /* DXT1 is 8 bytes per block */
        ret = ((This->currentDesc.Width + 3) >> 2) << 3;
    else if (This->resource.format == WINED3DFMT_DXT2 || This->resource.format == WINED3DFMT_DXT3 ||
             This->resource.format == WINED3DFMT_DXT4 || This->resource.format == WINED3DFMT_DXT5) /* DXT2/3/4/5 is 16 bytes per block */
        ret = ((This->currentDesc.Width + 3) >> 2) << 4;
    else {
        unsigned char alignment = This->resource.wineD3DDevice->surface_alignment;
        ret = This->bytesPerPixel * This->currentDesc.Width;  /* Bytes / row */
        ret = (ret + alignment - 1) & ~(alignment - 1);
    }
    TRACE("(%p) Returning %d\n", This, ret);
    return ret;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetOverlayPosition(IWineD3DSurface *iface, LONG X, LONG Y) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;

    FIXME("(%p)->(%d,%d) Stub!\n", This, X, Y);

    if(!(This->resource.usage & WINED3DUSAGE_OVERLAY))
    {
        TRACE("(%p): Not an overlay surface\n", This);
        return WINEDDERR_NOTAOVERLAYSURFACE;
    }

    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetOverlayPosition(IWineD3DSurface *iface, LONG *X, LONG *Y) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;

    FIXME("(%p)->(%p,%p) Stub!\n", This, X, Y);

    if(!(This->resource.usage & WINED3DUSAGE_OVERLAY))
    {
        TRACE("(%p): Not an overlay surface\n", This);
        return WINEDDERR_NOTAOVERLAYSURFACE;
    }

    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_UpdateOverlayZOrder(IWineD3DSurface *iface, DWORD Flags, IWineD3DSurface *Ref) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    IWineD3DSurfaceImpl *RefImpl = (IWineD3DSurfaceImpl *) Ref;

    FIXME("(%p)->(%08x,%p) Stub!\n", This, Flags, RefImpl);

    if(!(This->resource.usage & WINED3DUSAGE_OVERLAY))
    {
        TRACE("(%p): Not an overlay surface\n", This);
        return WINEDDERR_NOTAOVERLAYSURFACE;
    }

    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_UpdateOverlay(IWineD3DSurface *iface, RECT *SrcRect, IWineD3DSurface *DstSurface, RECT *DstRect, DWORD Flags, WINEDDOVERLAYFX *FX) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    IWineD3DSurfaceImpl *Dst = (IWineD3DSurfaceImpl *) DstSurface;
    FIXME("(%p)->(%p, %p, %p, %08x, %p)\n", This, SrcRect, Dst, DstRect, Flags, FX);

    if(!(This->resource.usage & WINED3DUSAGE_OVERLAY))
    {
        TRACE("(%p): Not an overlay surface\n", This);
        return WINEDDERR_NOTAOVERLAYSURFACE;
    }

    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetClipper(IWineD3DSurface *iface, IWineD3DClipper *clipper)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    TRACE("(%p)->(%p)\n", This, clipper);

    This->clipper = clipper;
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetClipper(IWineD3DSurface *iface, IWineD3DClipper **clipper)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    TRACE("(%p)->(%p)\n", This, clipper);

    *clipper = This->clipper;
    if(*clipper) {
        IWineD3DClipper_AddRef(*clipper);
    }
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetContainer(IWineD3DSurface *iface, IWineD3DBase *container) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;

    TRACE("This %p, container %p\n", This, container);

    /* We can't keep a reference to the container, since the container already keeps a reference to us. */

    TRACE("Setting container to %p from %p\n", container, This->container);
    This->container = container;

    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetFormat(IWineD3DSurface *iface, WINED3DFORMAT format) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    const StaticPixelFormatDesc *formatEntry = getFormatDescEntry(format, NULL, NULL);

    if (This->resource.format != WINED3DFMT_UNKNOWN) {
        FIXME("(%p) : The format of the surface must be WINED3DFORMAT_UNKNOWN\n", This);
        return WINED3DERR_INVALIDCALL;
    }

    TRACE("(%p) : Setting texture format to (%d,%s)\n", This, format, debug_d3dformat(format));
    if (format == WINED3DFMT_UNKNOWN) {
        This->resource.size = 0;
    } else if (format == WINED3DFMT_DXT1) {
        /* DXT1 is half byte per pixel */
        This->resource.size = ((max(This->pow2Width, 4) * formatEntry->bpp) * max(This->pow2Height, 4)) >> 1;

    } else if (format == WINED3DFMT_DXT2 || format == WINED3DFMT_DXT3 ||
               format == WINED3DFMT_DXT4 || format == WINED3DFMT_DXT5) {
        This->resource.size = ((max(This->pow2Width, 4) * formatEntry->bpp) * max(This->pow2Height, 4));
    } else {
        unsigned char alignment = This->resource.wineD3DDevice->surface_alignment;
        This->resource.size = ((This->pow2Width * formatEntry->bpp) + alignment - 1) & ~(alignment - 1);
        This->resource.size *= This->pow2Height;
    }

    if (format != WINED3DFMT_UNKNOWN) {
        This->bytesPerPixel = formatEntry->bpp;
    } else {
        This->bytesPerPixel = 0;
    }

    This->Flags |= (WINED3DFMT_D16_LOCKABLE == format) ? SFLAG_LOCKABLE : 0;

    This->resource.format = format;

    TRACE("(%p) : Size %d, bytesPerPixel %d\n", This, This->resource.size, This->bytesPerPixel);

    return WINED3D_OK;
}

HRESULT IWineD3DBaseSurfaceImpl_CreateDIBSection(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    int extraline = 0;
    SYSTEM_INFO sysInfo;
    BITMAPINFO* b_info;
    HDC ddc;
    DWORD *masks;
    const StaticPixelFormatDesc *formatEntry = getFormatDescEntry(This->resource.format, NULL, NULL);
    UINT usage;

    switch (This->bytesPerPixel) {
        case 2:
        case 4:
            /* Allocate extra space to store the RGB bit masks. */
            b_info = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BITMAPINFOHEADER) + 3 * sizeof(DWORD));
            break;

        case 3:
            b_info = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BITMAPINFOHEADER));
            break;

        default:
            /* Allocate extra space for a palette. */
            b_info = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                               sizeof(BITMAPINFOHEADER)
                               + sizeof(RGBQUAD)
                               * (1 << (This->bytesPerPixel * 8)));
            break;
    }

    if (!b_info)
        return E_OUTOFMEMORY;

        /* Some apps access the surface in via DWORDs, and do not take the necessary care at the end of the
    * surface. So we need at least extra 4 bytes at the end of the surface. Check against the page size,
    * if the last page used for the surface has at least 4 spare bytes we're safe, otherwise
    * add an extra line to the dib section
        */
    GetSystemInfo(&sysInfo);
    if( ((This->resource.size + 3) % sysInfo.dwPageSize) < 4) {
        extraline = 1;
        TRACE("Adding an extra line to the dib section\n");
    }

    b_info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    /* TODO: Is there a nicer way to force a specific alignment? (8 byte for ddraw) */
    b_info->bmiHeader.biWidth = IWineD3DSurface_GetPitch(iface) / This->bytesPerPixel;
    b_info->bmiHeader.biHeight = -This->currentDesc.Height -extraline;
    b_info->bmiHeader.biSizeImage = ( This->currentDesc.Height + extraline) * IWineD3DSurface_GetPitch(iface);
    b_info->bmiHeader.biPlanes = 1;
    b_info->bmiHeader.biBitCount = This->bytesPerPixel * 8;

    b_info->bmiHeader.biXPelsPerMeter = 0;
    b_info->bmiHeader.biYPelsPerMeter = 0;
    b_info->bmiHeader.biClrUsed = 0;
    b_info->bmiHeader.biClrImportant = 0;

    /* Get the bit masks */
    masks = (DWORD *) &(b_info->bmiColors);
    switch (This->resource.format) {
        case WINED3DFMT_R8G8B8:
            usage = DIB_RGB_COLORS;
            b_info->bmiHeader.biCompression = BI_RGB;
            break;

        case WINED3DFMT_X1R5G5B5:
        case WINED3DFMT_A1R5G5B5:
        case WINED3DFMT_A4R4G4B4:
        case WINED3DFMT_X4R4G4B4:
        case WINED3DFMT_R3G3B2:
        case WINED3DFMT_A8R3G3B2:
        case WINED3DFMT_A2B10G10R10:
        case WINED3DFMT_A8B8G8R8:
        case WINED3DFMT_X8B8G8R8:
        case WINED3DFMT_A2R10G10B10:
        case WINED3DFMT_R5G6B5:
        case WINED3DFMT_A16B16G16R16:
            usage = 0;
            b_info->bmiHeader.biCompression = BI_BITFIELDS;
            masks[0] = formatEntry->redMask;
            masks[1] = formatEntry->greenMask;
            masks[2] = formatEntry->blueMask;
            break;

        default:
            /* Don't know palette */
            b_info->bmiHeader.biCompression = BI_RGB;
            usage = 0;
            break;
    }

    ddc = GetDC(0);
    if (ddc == 0) {
        HeapFree(GetProcessHeap(), 0, b_info);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    TRACE("Creating a DIB section with size %dx%dx%d, size=%d\n", b_info->bmiHeader.biWidth, b_info->bmiHeader.biHeight, b_info->bmiHeader.biBitCount, b_info->bmiHeader.biSizeImage);
    This->dib.DIBsection = CreateDIBSection(ddc, b_info, usage, &This->dib.bitmap_data, 0 /* Handle */, 0 /* Offset */);
    ReleaseDC(0, ddc);

    if (!This->dib.DIBsection) {
        ERR("CreateDIBSection failed!\n");
        HeapFree(GetProcessHeap(), 0, b_info);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    TRACE("DIBSection at : %p\n", This->dib.bitmap_data);
    /* copy the existing surface to the dib section */
    if(This->resource.allocatedMemory) {
        memcpy(This->dib.bitmap_data, This->resource.allocatedMemory, b_info->bmiHeader.biSizeImage);
    } else {
        /* This is to make LockRect read the gl Texture although memory is allocated */
        This->Flags &= ~SFLAG_INSYSMEM;
    }
    This->dib.bitmap_size = b_info->bmiHeader.biSizeImage;

    HeapFree(GetProcessHeap(), 0, b_info);

    /* Now allocate a HDC */
    This->hDC = CreateCompatibleDC(0);
    This->dib.holdbitmap = SelectObject(This->hDC, This->dib.DIBsection);
    TRACE("using wined3d palette %p\n", This->palette);
    SelectPalette(This->hDC,
                  This->palette ? This->palette->hpal : 0,
                  FALSE);

    This->Flags |= SFLAG_DIBSECTION;

    HeapFree(GetProcessHeap(), 0, This->resource.heapMemory);
    This->resource.heapMemory = NULL;

    return WINED3D_OK;
}

/*****************************************************************************
 * _Blt_ColorFill
 *
 * Helper function that fills a memory area with a specific color
 *
 * Params:
 *  buf: memory address to start filling at
 *  width, height: Dimensions of the area to fill
 *  bpp: Bit depth of the surface
 *  lPitch: pitch of the surface
 *  color: Color to fill with
 *
 *****************************************************************************/
static HRESULT
        _Blt_ColorFill(BYTE *buf,
                       int width, int height,
                       int bpp, LONG lPitch,
                       DWORD color)
{
    int x, y;
    LPBYTE first;

    /* Do first row */

#define COLORFILL_ROW(type) \
    { \
    type *d = (type *) buf; \
    for (x = 0; x < width; x++) \
    d[x] = (type) color; \
    break; \
}
    switch(bpp)
    {
        case 1: COLORFILL_ROW(BYTE)
                case 2: COLORFILL_ROW(WORD)
        case 3:
        {
            BYTE *d = (BYTE *) buf;
            for (x = 0; x < width; x++,d+=3)
            {
                d[0] = (color    ) & 0xFF;
                d[1] = (color>> 8) & 0xFF;
                d[2] = (color>>16) & 0xFF;
            }
            break;
        }
        case 4: COLORFILL_ROW(DWORD)
        default:
            FIXME("Color fill not implemented for bpp %d!\n", bpp*8);
            return WINED3DERR_NOTAVAILABLE;
    }

#undef COLORFILL_ROW

    /* Now copy first row */
    first = buf;
    for (y = 1; y < height; y++)
    {
        buf += lPitch;
        memcpy(buf, first, width * bpp);
    }
    return WINED3D_OK;
}

/*****************************************************************************
 * IWineD3DSurface::Blt, SW emulation version
 *
 * Performs blits to a surface, eigher from a source of source-less blts
 * This is the main functionality of DirectDraw
 *
 * Params:
 *  DestRect: Destination rectangle to write to
 *  SrcSurface: Source surface, can be NULL
 *  SrcRect: Source rectangle
 *****************************************************************************/
HRESULT WINAPI
IWineD3DBaseSurfaceImpl_Blt(IWineD3DSurface *iface,
                            RECT *DestRect,
                            IWineD3DSurface *SrcSurface,
                            RECT *SrcRect,
                            DWORD Flags,
                            WINEDDBLTFX *DDBltFx,
                            WINED3DTEXTUREFILTERTYPE Filter)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    IWineD3DSurfaceImpl *Src = (IWineD3DSurfaceImpl *) SrcSurface;
    RECT        xdst,xsrc;
    HRESULT     ret = WINED3D_OK;
    WINED3DLOCKED_RECT  dlock, slock;
    WINED3DFORMAT       dfmt = WINED3DFMT_UNKNOWN, sfmt = WINED3DFMT_UNKNOWN;
    int bpp, srcheight, srcwidth, dstheight, dstwidth, width;
    int x, y;
    const StaticPixelFormatDesc *sEntry, *dEntry;
    LPBYTE dbuf, sbuf;
    TRACE("(%p)->(%p,%p,%p,%x,%p)\n", This, DestRect, Src, SrcRect, Flags, DDBltFx);

    if (TRACE_ON(d3d_surface))
    {
        if (DestRect) TRACE("\tdestrect :%dx%d-%dx%d\n",
            DestRect->left, DestRect->top, DestRect->right, DestRect->bottom);
        if (SrcRect) TRACE("\tsrcrect  :%dx%d-%dx%d\n",
            SrcRect->left, SrcRect->top, SrcRect->right, SrcRect->bottom);
#if 0
        TRACE("\tflags: ");
                      DDRAW_dump_DDBLT(Flags);
                      if (Flags & WINEDDBLT_DDFX)
              {
                      TRACE("\tblitfx: ");
                      DDRAW_dump_DDBLTFX(DDBltFx->dwDDFX);
    }
#endif
    }

    if ( (This->Flags & SFLAG_LOCKED) || ((Src != NULL) && (Src->Flags & SFLAG_LOCKED)))
    {
        WARN(" Surface is busy, returning DDERR_SURFACEBUSY\n");
        return WINEDDERR_SURFACEBUSY;
    }

    if(Filter != WINED3DTEXF_NONE && Filter != WINED3DTEXF_POINT) {
        /* Can happen when d3d9 apps do a StretchRect call which isn't handled in gl */
        FIXME("Filters not supported in software blit\n");
    }

    if (Src == This)
    {
        IWineD3DSurface_LockRect(iface, &dlock, NULL, 0);
        dfmt = This->resource.format;
        slock = dlock;
        sfmt = dfmt;
        sEntry = getFormatDescEntry(sfmt, NULL, NULL);
        dEntry = sEntry;
    }
    else
    {
        if (Src)
        {
            IWineD3DSurface_LockRect(SrcSurface, &slock, NULL, WINED3DLOCK_READONLY);
            sfmt = Src->resource.format;
        }
        sEntry = getFormatDescEntry(sfmt, NULL, NULL);
        dfmt = This->resource.format;
        dEntry = getFormatDescEntry(dfmt, NULL, NULL);
        IWineD3DSurface_LockRect(iface, &dlock,NULL,0);
    }

    if (!DDBltFx || !(DDBltFx->dwDDFX)) Flags &= ~WINEDDBLT_DDFX;

    if (sEntry->isFourcc && dEntry->isFourcc)
    {
        if (sfmt != dfmt)
        {
            FIXME("FOURCC->FOURCC copy only supported for the same type of surface\n");
            ret = WINED3DERR_WRONGTEXTUREFORMAT;
            goto release;
        }
        memcpy(dlock.pBits, slock.pBits, This->resource.size);
        goto release;
    }

    if (sEntry->isFourcc && !dEntry->isFourcc)
    {
        FIXME("DXTC decompression not supported right now\n");
        goto release;
    }

    if (DestRect)
    {
        memcpy(&xdst,DestRect,sizeof(xdst));
    }
    else
    {
        xdst.top    = 0;
        xdst.bottom = This->currentDesc.Height;
        xdst.left   = 0;
        xdst.right  = This->currentDesc.Width;
    }

    if (SrcRect)
    {
        memcpy(&xsrc,SrcRect,sizeof(xsrc));
    }
    else
    {
        if (Src)
        {
            xsrc.top    = 0;
            xsrc.bottom = Src->currentDesc.Height;
            xsrc.left   = 0;
            xsrc.right  = Src->currentDesc.Width;
        }
        else
        {
            memset(&xsrc,0,sizeof(xsrc));
        }
    }

    /* First check for the validity of source / destination rectangles. This was
     * verified using a test application + by MSDN.
     */
    if ((Src != NULL) &&
         ((xsrc.bottom > Src->currentDesc.Height) || (xsrc.bottom < 0) ||
         (xsrc.top     > Src->currentDesc.Height) || (xsrc.top    < 0) ||
         (xsrc.left    > Src->currentDesc.Width)  || (xsrc.left   < 0) ||
         (xsrc.right   > Src->currentDesc.Width)  || (xsrc.right  < 0) ||
         (xsrc.right   < xsrc.left)               || (xsrc.bottom < xsrc.top)))
    {
        WARN("Application gave us bad source rectangle for Blt.\n");
        ret = WINEDDERR_INVALIDRECT;
        goto release;
    }
    /* For the Destination rect, it can be out of bounds on the condition that a clipper
     * is set for the given surface.
     */
    if ((/*This->clipper == NULL*/ TRUE) &&
         ((xdst.bottom  > This->currentDesc.Height) || (xdst.bottom < 0) ||
         (xdst.top      > This->currentDesc.Height) || (xdst.top    < 0) ||
         (xdst.left     > This->currentDesc.Width)  || (xdst.left   < 0) ||
         (xdst.right    > This->currentDesc.Width)  || (xdst.right  < 0) ||
         (xdst.right    < xdst.left)                || (xdst.bottom < xdst.top)))
    {
        WARN("Application gave us bad destination rectangle for Blt without a clipper set.\n");
        ret = WINEDDERR_INVALIDRECT;
        goto release;
    }

    /* Now handle negative values in the rectangles. Warning: only supported for now
    in the 'simple' cases (ie not in any stretching / rotation cases).

    First, the case where nothing is to be done.
    */
    if (((xdst.bottom <= 0) || (xdst.right <= 0)         ||
          (xdst.top    >= (int) This->currentDesc.Height) ||
          (xdst.left   >= (int) This->currentDesc.Width)) ||
          ((Src != NULL) &&
          ((xsrc.bottom <= 0) || (xsrc.right <= 0)     ||
          (xsrc.top >= (int) Src->currentDesc.Height) ||
          (xsrc.left >= (int) Src->currentDesc.Width))  ))
    {
        TRACE("Nothing to be done !\n");
        goto release;
    }

    /* The easy case : the source-less blits.... */
    if (Src == NULL)
    {
        RECT full_rect;
        RECT temp_rect; /* No idea if intersect rect can be the same as one of the source rect */

        full_rect.left   = 0;
        full_rect.top    = 0;
        full_rect.right  = This->currentDesc.Width;
        full_rect.bottom = This->currentDesc.Height;
        IntersectRect(&temp_rect, &full_rect, &xdst);
        xdst = temp_rect;
    }
    else
    {
        /* Only handle clipping on the destination rectangle */
        int clip_horiz = (xdst.left < 0) || (xdst.right  > (int) This->currentDesc.Width );
        int clip_vert  = (xdst.top  < 0) || (xdst.bottom > (int) This->currentDesc.Height);
        if (clip_vert || clip_horiz)
        {
            /* Now check if this is a special case or not... */
            if ((((xdst.bottom - xdst.top ) != (xsrc.bottom - xsrc.top )) && clip_vert ) ||
                   (((xdst.right  - xdst.left) != (xsrc.right  - xsrc.left)) && clip_horiz) ||
                   (Flags & WINEDDBLT_DDFX))
            {
                WARN("Out of screen rectangle in special case. Not handled right now.\n");
                goto release;
            }

            if (clip_horiz)
            {
                if (xdst.left < 0) { xsrc.left -= xdst.left; xdst.left = 0; }
                if (xdst.right > This->currentDesc.Width)
                {
                    xsrc.right -= (xdst.right - (int) This->currentDesc.Width);
                    xdst.right = (int) This->currentDesc.Width;
                }
            }
            if (clip_vert)
            {
                if (xdst.top < 0)
                {
                    xsrc.top -= xdst.top;
                    xdst.top = 0;
                }
                if (xdst.bottom > This->currentDesc.Height)
                {
                    xsrc.bottom -= (xdst.bottom - (int) This->currentDesc.Height);
                    xdst.bottom = (int) This->currentDesc.Height;
                }
            }
            /* And check if after clipping something is still to be done... */
            if ((xdst.bottom <= 0)   || (xdst.right <= 0)       ||
                 (xdst.top   >= (int) This->currentDesc.Height)  ||
                 (xdst.left  >= (int) This->currentDesc.Width)   ||
                 (xsrc.bottom <= 0)   || (xsrc.right <= 0)       ||
                 (xsrc.top >= (int) Src->currentDesc.Height)     ||
                 (xsrc.left >= (int) Src->currentDesc.Width))
            {
                TRACE("Nothing to be done after clipping !\n");
                goto release;
            }
        }
    }

    bpp = This->bytesPerPixel;
    srcheight = xsrc.bottom - xsrc.top;
    srcwidth = xsrc.right - xsrc.left;
    dstheight = xdst.bottom - xdst.top;
    dstwidth = xdst.right - xdst.left;
    width = (xdst.right - xdst.left) * bpp;

    assert(width <= dlock.Pitch);

    dbuf = (BYTE*)dlock.pBits+(xdst.top*dlock.Pitch)+(xdst.left*bpp);

    if (Flags & WINEDDBLT_WAIT)
    {
        Flags &= ~WINEDDBLT_WAIT;
    }
    if (Flags & WINEDDBLT_ASYNC)
    {
        static BOOL displayed = FALSE;
        if (!displayed)
            FIXME("Can't handle WINEDDBLT_ASYNC flag right now.\n");
        displayed = TRUE;
        Flags &= ~WINEDDBLT_ASYNC;
    }
    if (Flags & WINEDDBLT_DONOTWAIT)
    {
        /* WINEDDBLT_DONOTWAIT appeared in DX7 */
        static BOOL displayed = FALSE;
        if (!displayed)
            FIXME("Can't handle WINEDDBLT_DONOTWAIT flag right now.\n");
        displayed = TRUE;
        Flags &= ~WINEDDBLT_DONOTWAIT;
    }

    /* First, all the 'source-less' blits */
    if (Flags & WINEDDBLT_COLORFILL)
    {
        ret = _Blt_ColorFill(dbuf, dstwidth, dstheight, bpp,
                             dlock.Pitch, DDBltFx->u5.dwFillColor);
        Flags &= ~WINEDDBLT_COLORFILL;
    }

    if (Flags & WINEDDBLT_DEPTHFILL)
    {
        FIXME("DDBLT_DEPTHFILL needs to be implemented!\n");
    }
    if (Flags & WINEDDBLT_ROP)
    {
        /* Catch some degenerate cases here */
        switch(DDBltFx->dwROP)
        {
            case BLACKNESS:
                ret = _Blt_ColorFill(dbuf,dstwidth,dstheight,bpp,dlock.Pitch,0);
                break;
                case 0xAA0029: /* No-op */
                    break;
            case WHITENESS:
                ret = _Blt_ColorFill(dbuf,dstwidth,dstheight,bpp,dlock.Pitch,~0);
                break;
                case SRCCOPY: /* well, we do that below ? */
                    break;
            default:
                FIXME("Unsupported raster op: %08x  Pattern: %p\n", DDBltFx->dwROP, DDBltFx->u5.lpDDSPattern);
                goto error;
        }
        Flags &= ~WINEDDBLT_ROP;
    }
    if (Flags & WINEDDBLT_DDROPS)
    {
        FIXME("\tDdraw Raster Ops: %08x  Pattern: %p\n", DDBltFx->dwDDROP, DDBltFx->u5.lpDDSPattern);
    }
    /* Now the 'with source' blits */
    if (Src)
    {
        LPBYTE sbase;
        int sx, xinc, sy, yinc;

        if (!dstwidth || !dstheight) /* hmm... stupid program ? */
            goto release;
        sbase = (BYTE*)slock.pBits+(xsrc.top*slock.Pitch)+xsrc.left*bpp;
        xinc = (srcwidth << 16) / dstwidth;
        yinc = (srcheight << 16) / dstheight;

        if (!Flags)
        {
            /* No effects, we can cheat here */
            if (dstwidth == srcwidth)
            {
                if (dstheight == srcheight)
                {
                    /* No stretching in either direction. This needs to be as
                    * fast as possible */
                    sbuf = sbase;

                    /* check for overlapping surfaces */
                    if (SrcSurface != iface || xdst.top < xsrc.top ||
                        xdst.right <= xsrc.left || xsrc.right <= xdst.left)
                    {
                        /* no overlap, or dst above src, so copy from top downwards */
                        for (y = 0; y < dstheight; y++)
                        {
                            memcpy(dbuf, sbuf, width);
                            sbuf += slock.Pitch;
                            dbuf += dlock.Pitch;
                        }
                    }
                    else if (xdst.top > xsrc.top)  /* copy from bottom upwards */
                    {
                        sbuf += (slock.Pitch*dstheight);
                        dbuf += (dlock.Pitch*dstheight);
                        for (y = 0; y < dstheight; y++)
                        {
                            sbuf -= slock.Pitch;
                            dbuf -= dlock.Pitch;
                            memcpy(dbuf, sbuf, width);
                        }
                    }
                    else /* src and dst overlapping on the same line, use memmove */
                    {
                        for (y = 0; y < dstheight; y++)
                        {
                            memmove(dbuf, sbuf, width);
                            sbuf += slock.Pitch;
                            dbuf += dlock.Pitch;
                        }
                    }
                } else {
                    /* Stretching in Y direction only */
                    for (y = sy = 0; y < dstheight; y++, sy += yinc) {
                        sbuf = sbase + (sy >> 16) * slock.Pitch;
                        memcpy(dbuf, sbuf, width);
                        dbuf += dlock.Pitch;
                    }
                }
            }
            else
            {
                /* Stretching in X direction */
                int last_sy = -1;
                for (y = sy = 0; y < dstheight; y++, sy += yinc)
                {
                    sbuf = sbase + (sy >> 16) * slock.Pitch;

                    if ((sy >> 16) == (last_sy >> 16))
                    {
                        /* this sourcerow is the same as last sourcerow -
                        * copy already stretched row
                        */
                        memcpy(dbuf, dbuf - dlock.Pitch, width);
                    }
                    else
                    {
#define STRETCH_ROW(type) { \
                        type *s = (type *) sbuf, *d = (type *) dbuf; \
                        for (x = sx = 0; x < dstwidth; x++, sx += xinc) \
                        d[x] = s[sx >> 16]; \
                        break; }

                        switch(bpp)
                        {
                            case 1: STRETCH_ROW(BYTE)
                                    case 2: STRETCH_ROW(WORD)
                                            case 4: STRETCH_ROW(DWORD)
                            case 3:
                            {
                                LPBYTE s,d = dbuf;
                                for (x = sx = 0; x < dstwidth; x++, sx+= xinc)
                                {
                                    DWORD pixel;

                                    s = sbuf+3*(sx>>16);
                                    pixel = s[0]|(s[1]<<8)|(s[2]<<16);
                                    d[0] = (pixel    )&0xff;
                                    d[1] = (pixel>> 8)&0xff;
                                    d[2] = (pixel>>16)&0xff;
                                    d+=3;
                                }
                                break;
                            }
                            default:
                                FIXME("Stretched blit not implemented for bpp %d!\n", bpp*8);
                                ret = WINED3DERR_NOTAVAILABLE;
                                goto error;
                        }
#undef STRETCH_ROW
                    }
                    dbuf += dlock.Pitch;
                    last_sy = sy;
                }
            }
        }
        else
        {
            LONG dstyinc = dlock.Pitch, dstxinc = bpp;
            DWORD keylow = 0xFFFFFFFF, keyhigh = 0, keymask = 0xFFFFFFFF;
            DWORD destkeylow = 0x0, destkeyhigh = 0xFFFFFFFF, destkeymask = 0xFFFFFFFF;
            if (Flags & (WINEDDBLT_KEYSRC | WINEDDBLT_KEYDEST | WINEDDBLT_KEYSRCOVERRIDE | WINEDDBLT_KEYDESTOVERRIDE))
            {
                /* The color keying flags are checked for correctness in ddraw */
                if (Flags & WINEDDBLT_KEYSRC)
                {
                    keylow  = Src->SrcBltCKey.dwColorSpaceLowValue;
                    keyhigh = Src->SrcBltCKey.dwColorSpaceHighValue;
                }
                else  if (Flags & WINEDDBLT_KEYSRCOVERRIDE)
                {
                    keylow  = DDBltFx->ddckSrcColorkey.dwColorSpaceLowValue;
                    keyhigh = DDBltFx->ddckSrcColorkey.dwColorSpaceHighValue;
                }

                if (Flags & WINEDDBLT_KEYDEST)
                {
                    /* Destination color keys are taken from the source surface ! */
                    destkeylow  = Src->DestBltCKey.dwColorSpaceLowValue;
                    destkeyhigh = Src->DestBltCKey.dwColorSpaceHighValue;
                }
                else if (Flags & WINEDDBLT_KEYDESTOVERRIDE)
                {
                    destkeylow  = DDBltFx->ddckDestColorkey.dwColorSpaceLowValue;
                    destkeyhigh = DDBltFx->ddckDestColorkey.dwColorSpaceHighValue;
                }

                if(bpp == 1)
                {
                    keymask = 0xff;
                }
                else
                {
                    keymask = sEntry->redMask   |
                            sEntry->greenMask |
                            sEntry->blueMask;
                }
                Flags &= ~(WINEDDBLT_KEYSRC | WINEDDBLT_KEYDEST | WINEDDBLT_KEYSRCOVERRIDE | WINEDDBLT_KEYDESTOVERRIDE);
            }

            if (Flags & WINEDDBLT_DDFX)
            {
                LPBYTE dTopLeft, dTopRight, dBottomLeft, dBottomRight, tmp;
                LONG tmpxy;
                dTopLeft     = dbuf;
                dTopRight    = dbuf+((dstwidth-1)*bpp);
                dBottomLeft  = dTopLeft+((dstheight-1)*dlock.Pitch);
                dBottomRight = dBottomLeft+((dstwidth-1)*bpp);

                if (DDBltFx->dwDDFX & WINEDDBLTFX_ARITHSTRETCHY)
                {
                    /* I don't think we need to do anything about this flag */
                    WARN("Flags=DDBLT_DDFX nothing done for WINEDDBLTFX_ARITHSTRETCHY\n");
                }
                if (DDBltFx->dwDDFX & WINEDDBLTFX_MIRRORLEFTRIGHT)
                {
                    tmp          = dTopRight;
                    dTopRight    = dTopLeft;
                    dTopLeft     = tmp;
                    tmp          = dBottomRight;
                    dBottomRight = dBottomLeft;
                    dBottomLeft  = tmp;
                    dstxinc = dstxinc *-1;
                }
                if (DDBltFx->dwDDFX & WINEDDBLTFX_MIRRORUPDOWN)
                {
                    tmp          = dTopLeft;
                    dTopLeft     = dBottomLeft;
                    dBottomLeft  = tmp;
                    tmp          = dTopRight;
                    dTopRight    = dBottomRight;
                    dBottomRight = tmp;
                    dstyinc = dstyinc *-1;
                }
                if (DDBltFx->dwDDFX & WINEDDBLTFX_NOTEARING)
                {
                    /* I don't think we need to do anything about this flag */
                    WARN("Flags=DDBLT_DDFX nothing done for WINEDDBLTFX_NOTEARING\n");
                }
                if (DDBltFx->dwDDFX & WINEDDBLTFX_ROTATE180)
                {
                    tmp          = dBottomRight;
                    dBottomRight = dTopLeft;
                    dTopLeft     = tmp;
                    tmp          = dBottomLeft;
                    dBottomLeft  = dTopRight;
                    dTopRight    = tmp;
                    dstxinc = dstxinc * -1;
                    dstyinc = dstyinc * -1;
                }
                if (DDBltFx->dwDDFX & WINEDDBLTFX_ROTATE270)
                {
                    tmp          = dTopLeft;
                    dTopLeft     = dBottomLeft;
                    dBottomLeft  = dBottomRight;
                    dBottomRight = dTopRight;
                    dTopRight    = tmp;
                    tmpxy   = dstxinc;
                    dstxinc = dstyinc;
                    dstyinc = tmpxy;
                    dstxinc = dstxinc * -1;
                }
                if (DDBltFx->dwDDFX & WINEDDBLTFX_ROTATE90)
                {
                    tmp          = dTopLeft;
                    dTopLeft     = dTopRight;
                    dTopRight    = dBottomRight;
                    dBottomRight = dBottomLeft;
                    dBottomLeft  = tmp;
                    tmpxy   = dstxinc;
                    dstxinc = dstyinc;
                    dstyinc = tmpxy;
                    dstyinc = dstyinc * -1;
                }
                if (DDBltFx->dwDDFX & WINEDDBLTFX_ZBUFFERBASEDEST)
                {
                    /* I don't think we need to do anything about this flag */
                    WARN("Flags=WINEDDBLT_DDFX nothing done for WINEDDBLTFX_ZBUFFERBASEDEST\n");
                }
                dbuf = dTopLeft;
                Flags &= ~(WINEDDBLT_DDFX);
            }

#define COPY_COLORKEY_FX(type) { \
            type *s, *d = (type *) dbuf, *dx, tmp; \
            for (y = sy = 0; y < dstheight; y++, sy += yinc) { \
            s = (type*)(sbase + (sy >> 16) * slock.Pitch); \
            dx = d; \
            for (x = sx = 0; x < dstwidth; x++, sx += xinc) { \
            tmp = s[sx >> 16]; \
            if (((tmp & keymask) < keylow || (tmp & keymask) > keyhigh) && \
            ((dx[0] & destkeymask) >= destkeylow && (dx[0] & destkeymask) <= destkeyhigh)) { \
            dx[0] = tmp; \
        } \
            dx = (type*)(((LPBYTE)dx)+dstxinc); \
        } \
            d = (type*)(((LPBYTE)d)+dstyinc); \
        } \
            break; }

            switch (bpp) {
                case 1: COPY_COLORKEY_FX(BYTE)
                        case 2: COPY_COLORKEY_FX(WORD)
                                case 4: COPY_COLORKEY_FX(DWORD)
                case 3:
                {
                    LPBYTE s,d = dbuf, dx;
                    for (y = sy = 0; y < dstheight; y++, sy += yinc)
                    {
                        sbuf = sbase + (sy >> 16) * slock.Pitch;
                        dx = d;
                        for (x = sx = 0; x < dstwidth; x++, sx+= xinc)
                        {
                            DWORD pixel, dpixel = 0;
                            s = sbuf+3*(sx>>16);
                            pixel = s[0]|(s[1]<<8)|(s[2]<<16);
                            dpixel = dx[0]|(dx[1]<<8)|(dx[2]<<16);
                            if (((pixel & keymask) < keylow || (pixel & keymask) > keyhigh) &&
                                  ((dpixel & keymask) >= destkeylow || (dpixel & keymask) <= keyhigh))
                            {
                                dx[0] = (pixel    )&0xff;
                                dx[1] = (pixel>> 8)&0xff;
                                dx[2] = (pixel>>16)&0xff;
                            }
                            dx+= dstxinc;
                        }
                        d += dstyinc;
                    }
                    break;
                }
                default:
                    FIXME("%s color-keyed blit not implemented for bpp %d!\n",
                          (Flags & WINEDDBLT_KEYSRC) ? "Source" : "Destination", bpp*8);
                    ret = WINED3DERR_NOTAVAILABLE;
                    goto error;
#undef COPY_COLORKEY_FX
            }
        }
    }

error:
    if (Flags && FIXME_ON(d3d_surface))
    {
        FIXME("\tUnsupported flags: %08x\n", Flags);
    }

release:
    IWineD3DSurface_UnlockRect(iface);
    if (SrcSurface && SrcSurface != iface) IWineD3DSurface_UnlockRect(SrcSurface);
    return ret;
}

/*****************************************************************************
 * IWineD3DSurface::BltFast, SW emulation version
 *
 * This is the software implementation of BltFast, as used by GDI surfaces
 * and as a fallback for OpenGL surfaces. This code is taken from the old
 * DirectDraw code, and was originally written by TransGaming.
 *
 * Params:
 *  dstx:
 *  dsty:
 *  Source: Source surface to copy from
 *  rsrc: Source rectangle
 *  trans: Some Flags
 *
 * Returns:
 *  WINED3D_OK on success
 *
 *****************************************************************************/
HRESULT WINAPI
IWineD3DBaseSurfaceImpl_BltFast(IWineD3DSurface *iface,
                                DWORD dstx,
                                DWORD dsty,
                                IWineD3DSurface *Source,
                                RECT *rsrc,
                                DWORD trans)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    IWineD3DSurfaceImpl *Src = (IWineD3DSurfaceImpl *) Source;

    int                 bpp, w, h, x, y;
    WINED3DLOCKED_RECT  dlock,slock;
    HRESULT             ret = WINED3D_OK;
    RECT                rsrc2;
    RECT                lock_src, lock_dst, lock_union;
    BYTE                *sbuf, *dbuf;
    const StaticPixelFormatDesc *sEntry, *dEntry;

    if (TRACE_ON(d3d_surface))
    {
        TRACE("(%p)->(%d,%d,%p,%p,%08x)\n", This,dstx,dsty,Src,rsrc,trans);

        if (rsrc)
        {
            TRACE("\tsrcrect: %dx%d-%dx%d\n",rsrc->left,rsrc->top,
                  rsrc->right,rsrc->bottom);
        }
        else
        {
            TRACE(" srcrect: NULL\n");
        }
    }

    if ((This->Flags & SFLAG_LOCKED) ||
         ((Src != NULL) && (Src->Flags & SFLAG_LOCKED)))
    {
        WARN(" Surface is busy, returning DDERR_SURFACEBUSY\n");
        return WINEDDERR_SURFACEBUSY;
    }

    if (!rsrc)
    {
        WARN("rsrc is NULL!\n");
        rsrc = &rsrc2;
        rsrc->left = 0;
        rsrc->top = 0;
        rsrc->right = Src->currentDesc.Width;
        rsrc->bottom = Src->currentDesc.Height;
    }

    /* Check source rect for validity. Copied from normal Blt. Fixes Baldur's Gate.*/
    if ((rsrc->bottom > Src->currentDesc.Height) || (rsrc->bottom < 0) ||
         (rsrc->top    > Src->currentDesc.Height) || (rsrc->top    < 0) ||
         (rsrc->left   > Src->currentDesc.Width)  || (rsrc->left   < 0) ||
         (rsrc->right  > Src->currentDesc.Width)  || (rsrc->right  < 0) ||
         (rsrc->right  < rsrc->left)              || (rsrc->bottom < rsrc->top))
    {
        WARN("Application gave us bad source rectangle for BltFast.\n");
        return WINEDDERR_INVALIDRECT;
    }

    h = rsrc->bottom - rsrc->top;
    if (h > This->currentDesc.Height-dsty) h = This->currentDesc.Height-dsty;
    if (h > Src->currentDesc.Height-rsrc->top) h=Src->currentDesc.Height-rsrc->top;
    if (h <= 0) return WINEDDERR_INVALIDRECT;

    w = rsrc->right - rsrc->left;
    if (w > This->currentDesc.Width-dstx) w = This->currentDesc.Width-dstx;
    if (w > Src->currentDesc.Width-rsrc->left) w = Src->currentDesc.Width-rsrc->left;
    if (w <= 0) return WINEDDERR_INVALIDRECT;

    /* Now compute the locking rectangle... */
    lock_src.left = rsrc->left;
    lock_src.top = rsrc->top;
    lock_src.right = lock_src.left + w;
    lock_src.bottom = lock_src.top + h;

    lock_dst.left = dstx;
    lock_dst.top = dsty;
    lock_dst.right = dstx + w;
    lock_dst.bottom = dsty + h;

    bpp = This->bytesPerPixel;

    /* We need to lock the surfaces, or we won't get refreshes when done. */
    if (Src == This)
    {
        int pitch;

        UnionRect(&lock_union, &lock_src, &lock_dst);

        /* Lock the union of the two rectangles */
        ret = IWineD3DSurface_LockRect(iface, &dlock, &lock_union, 0);
        if(ret != WINED3D_OK) goto error;

        pitch = dlock.Pitch;
        slock.Pitch = dlock.Pitch;

        /* Since slock was originally copied from this surface's description, we can just reuse it */
        assert(This->resource.allocatedMemory != NULL);
        sbuf = (BYTE *)This->resource.allocatedMemory + lock_src.top * pitch + lock_src.left * bpp;
        dbuf = (BYTE *)This->resource.allocatedMemory + lock_dst.top * pitch + lock_dst.left * bpp;
        sEntry = getFormatDescEntry(Src->resource.format, NULL, NULL);
        dEntry = sEntry;
    }
    else
    {
        ret = IWineD3DSurface_LockRect(Source, &slock, &lock_src, WINED3DLOCK_READONLY);
        if(ret != WINED3D_OK) goto error;
        ret = IWineD3DSurface_LockRect(iface, &dlock, &lock_dst, 0);
        if(ret != WINED3D_OK) goto error;

        sbuf = slock.pBits;
        dbuf = dlock.pBits;
        TRACE("Dst is at %p, Src is at %p\n", dbuf, sbuf);

        sEntry = getFormatDescEntry(Src->resource.format, NULL, NULL);
        dEntry = getFormatDescEntry(This->resource.format, NULL, NULL);
    }

    /* Handle first the FOURCC surfaces... */
    if (sEntry->isFourcc && dEntry->isFourcc)
    {
        TRACE("Fourcc -> Fourcc copy\n");
        if (trans)
            FIXME("trans arg not supported when a FOURCC surface is involved\n");
        if (dstx || dsty)
            FIXME("offset for destination surface is not supported\n");
        if (Src->resource.format != This->resource.format)
        {
            FIXME("FOURCC->FOURCC copy only supported for the same type of surface\n");
            ret = WINED3DERR_WRONGTEXTUREFORMAT;
            goto error;
        }
        /* FIXME: Watch out that the size is correct for FOURCC surfaces */
        memcpy(dbuf, sbuf, This->resource.size);
        goto error;
    }
    if (sEntry->isFourcc && !dEntry->isFourcc)
    {
        /* TODO: Use the libtxc_dxtn.so shared library to do
         * software decompression
         */
        ERR("DXTC decompression not supported by now\n");
        goto error;
    }

    if (trans & (WINEDDBLTFAST_SRCCOLORKEY | WINEDDBLTFAST_DESTCOLORKEY))
    {
        DWORD keylow, keyhigh;
        TRACE("Color keyed copy\n");
        if (trans & WINEDDBLTFAST_SRCCOLORKEY)
        {
            keylow  = Src->SrcBltCKey.dwColorSpaceLowValue;
            keyhigh = Src->SrcBltCKey.dwColorSpaceHighValue;
        }
        else
        {
            /* I'm not sure if this is correct */
            FIXME("WINEDDBLTFAST_DESTCOLORKEY not fully supported yet.\n");
            keylow  = This->DestBltCKey.dwColorSpaceLowValue;
            keyhigh = This->DestBltCKey.dwColorSpaceHighValue;
        }

#define COPYBOX_COLORKEY(type) { \
        type *d, *s, tmp; \
        s = (type *) sbuf; \
        d = (type *) dbuf; \
        for (y = 0; y < h; y++) { \
        for (x = 0; x < w; x++) { \
        tmp = s[x]; \
        if (tmp < keylow || tmp > keyhigh) d[x] = tmp; \
    } \
        s = (type *)((BYTE *)s + slock.Pitch); \
        d = (type *)((BYTE *)d + dlock.Pitch); \
    } \
        break; \
    }

        switch (bpp) {
            case 1: COPYBOX_COLORKEY(BYTE)
                    case 2: COPYBOX_COLORKEY(WORD)
                            case 4: COPYBOX_COLORKEY(DWORD)
            case 3:
            {
                BYTE *d, *s;
                DWORD tmp;
                s = (BYTE *) sbuf;
                d = (BYTE *) dbuf;
                for (y = 0; y < h; y++)
                {
                    for (x = 0; x < w * 3; x += 3)
                    {
                        tmp = (DWORD)s[x] + ((DWORD)s[x + 1] << 8) + ((DWORD)s[x + 2] << 16);
                        if (tmp < keylow || tmp > keyhigh)
                        {
                            d[x + 0] = s[x + 0];
                            d[x + 1] = s[x + 1];
                            d[x + 2] = s[x + 2];
                        }
                    }
                    s += slock.Pitch;
                    d += dlock.Pitch;
                }
                break;
            }
            default:
                FIXME("Source color key blitting not supported for bpp %d\n",bpp*8);
                ret = WINED3DERR_NOTAVAILABLE;
                goto error;
        }
#undef COPYBOX_COLORKEY
        TRACE("Copy Done\n");
    }
    else
    {
        int width = w * bpp;
        TRACE("NO color key copy\n");
        for (y = 0; y < h; y++)
        {
            /* This is pretty easy, a line for line memcpy */
            memcpy(dbuf, sbuf, width);
            sbuf += slock.Pitch;
            dbuf += dlock.Pitch;
        }
        TRACE("Copy done\n");
    }

error:
    if (Src == This)
    {
        IWineD3DSurface_UnlockRect(iface);
    }
    else
    {
        IWineD3DSurface_UnlockRect(iface);
        IWineD3DSurface_UnlockRect(Source);
    }

    return ret;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_LockRect(IWineD3DSurface *iface, WINED3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;

    TRACE("(%p) : rect@%p flags(%08x), output lockedRect@%p, memory@%p\n",
          This, pRect, Flags, pLockedRect, This->resource.allocatedMemory);

    pLockedRect->Pitch = IWineD3DSurface_GetPitch(iface);

    if (NULL == pRect)
    {
        pLockedRect->pBits = This->resource.allocatedMemory;
        This->lockedRect.left   = 0;
        This->lockedRect.top    = 0;
        This->lockedRect.right  = This->currentDesc.Width;
        This->lockedRect.bottom = This->currentDesc.Height;

        TRACE("Locked Rect (%p) = l %d, t %d, r %d, b %d\n",
              &This->lockedRect, This->lockedRect.left, This->lockedRect.top,
              This->lockedRect.right, This->lockedRect.bottom);
    }
    else
    {
        TRACE("Lock Rect (%p) = l %d, t %d, r %d, b %d\n",
              pRect, pRect->left, pRect->top, pRect->right, pRect->bottom);

        /* DXTn textures are based on compressed blocks of 4x4 pixels, each
         * 16 bytes large (8 bytes in case of DXT1). Because of that Pitch has
         * slightly different meaning compared to regular textures. For DXTn
         * textures Pitch is the size of a row of blocks, 4 high and "width"
         * long. The x offset is calculated differently as well, since moving 4
         * pixels to the right actually moves an entire 4x4 block to right, ie
         * 16 bytes (8 in case of DXT1). */
        if (This->resource.format == WINED3DFMT_DXT1)
        {
            pLockedRect->pBits = This->resource.allocatedMemory + (pLockedRect->Pitch * pRect->top / 4) + (pRect->left * 2);
        }
        else if (This->resource.format == WINED3DFMT_DXT2 || This->resource.format == WINED3DFMT_DXT3 ||
                    This->resource.format == WINED3DFMT_DXT4 || This->resource.format == WINED3DFMT_DXT5)
        {
            pLockedRect->pBits = This->resource.allocatedMemory + (pLockedRect->Pitch * pRect->top / 4) + (pRect->left * 4);
        }
        else
        {
            pLockedRect->pBits = This->resource.allocatedMemory +
                    (pLockedRect->Pitch * pRect->top) +
                    (pRect->left * This->bytesPerPixel);
        }
        This->lockedRect.left   = pRect->left;
        This->lockedRect.top    = pRect->top;
        This->lockedRect.right  = pRect->right;
        This->lockedRect.bottom = pRect->bottom;
    }

    /* No dirtifying is needed for this surface implementation */
    TRACE("returning memory@%p, pitch(%d)\n", pLockedRect->pBits, pLockedRect->Pitch);

    return WINED3D_OK;
}

void WINAPI IWineD3DBaseSurfaceImpl_BindTexture(IWineD3DSurface *iface) {
    ERR("Should not be called on base texture\n");
    return;
}
