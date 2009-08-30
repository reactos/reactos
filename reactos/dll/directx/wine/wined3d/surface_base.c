/*
 * IWineD3DSurface Implementation of management(non-rendering) functions
 *
 * Copyright 1998 Lionel Ulmer
 * Copyright 2000-2001 TransGaming Technologies Inc.
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006-2008 Stefan Dösinger for CodeWeavers
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

/* See also float_16_to_32() in wined3d_private.h */
static inline unsigned short float_32_to_16(const float *in)
{
    int exp = 0;
    float tmp = fabs(*in);
    unsigned int mantissa;
    unsigned short ret;

    /* Deal with special numbers */
    if(*in == 0.0) return 0x0000;
    if(isnan(*in)) return 0x7C01;
    if(isinf(*in)) return (*in < 0.0 ? 0xFC00 : 0x7c00);

    if(tmp < pow(2, 10)) {
        do
        {
            tmp = tmp * 2.0;
            exp--;
        }while(tmp < pow(2, 10));
    } else if(tmp >= pow(2, 11)) {
        do
        {
            tmp /= 2.0;
            exp++;
        }while(tmp >= pow(2, 11));
    }

    mantissa = (unsigned int) tmp;
    if(tmp - mantissa >= 0.5) mantissa++; /* round to nearest, away from zero */

    exp += 10;  /* Normalize the mantissa */
    exp += 15;  /* Exponent is encoded with excess 15 */

    if(exp > 30) { /* too big */
        ret = 0x7c00; /* INF */
    } else if(exp <= 0) {
        /* exp == 0: Non-normalized mantissa. Returns 0x0000 (=0.0) for too small numbers */
        while(exp <= 0) {
            mantissa = mantissa >> 1;
            exp++;
        }
        ret = mantissa & 0x3ff;
    } else {
        ret = (exp << 10) | (mantissa & 0x3ff);
    }

    ret |= ((*in < 0.0 ? 1 : 0) << 15); /* Add the sign */
    return ret;
}


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
    return resource_get_device((IWineD3DResource *)iface, ppDevice);
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetPrivateData(IWineD3DSurface *iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    return resource_set_private_data((IWineD3DResource *)iface, refguid, pData, SizeOfData, Flags);
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetPrivateData(IWineD3DSurface *iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    return resource_get_private_data((IWineD3DResource *)iface, refguid, pData, pSizeOfData);
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_FreePrivateData(IWineD3DSurface *iface, REFGUID refguid) {
    return resource_free_private_data((IWineD3DResource *)iface, refguid);
}

DWORD   WINAPI IWineD3DBaseSurfaceImpl_SetPriority(IWineD3DSurface *iface, DWORD PriorityNew) {
    return resource_set_priority((IWineD3DResource *)iface, PriorityNew);
}

DWORD   WINAPI IWineD3DBaseSurfaceImpl_GetPriority(IWineD3DSurface *iface) {
    return resource_get_priority((IWineD3DResource *)iface);
}

WINED3DRESOURCETYPE WINAPI IWineD3DBaseSurfaceImpl_GetType(IWineD3DSurface *iface) {
    TRACE("(%p) : calling resourceimpl_GetType\n", iface);
    return resource_get_type((IWineD3DResource *)iface);
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetParent(IWineD3DSurface *iface, IUnknown **pParent) {
    TRACE("(%p) : calling resourceimpl_GetParent\n", iface);
    return resource_get_parent((IWineD3DResource *)iface, pParent);
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

    /* Standalone surfaces return the device as container. */
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
    if(pDesc->Format != NULL)             *(pDesc->Format) = This->resource.format_desc->format;
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

    if(This->palette == PalImpl) {
        TRACE("Nop palette change\n");
        return WINED3D_OK;
    }

    if(This->palette != NULL)
        if(This->resource.usage & WINED3DUSAGE_RENDERTARGET)
            This->palette->Flags &= ~WINEDDPCAPS_PRIMARYSURFACE;

    This->palette = PalImpl;

    if(PalImpl != NULL) {
        if(This->resource.usage & WINED3DUSAGE_RENDERTARGET) {
            (PalImpl)->Flags |= WINEDDPCAPS_PRIMARYSURFACE;
        }

        return IWineD3DSurface_RealizePalette(iface);
    }
    else return WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetColorKey(IWineD3DSurface *iface, DWORD Flags, const WINEDDCOLORKEY *CKey)
{
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

DWORD WINAPI IWineD3DBaseSurfaceImpl_GetPitch(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    WINED3DFORMAT format = This->resource.format_desc->format;
    DWORD ret;
    TRACE("(%p)\n", This);

    /* DXTn formats don't have exact pitches as they are to the new row of blocks,
    where each block is 4x4 pixels, 8 bytes (dxt1) and 16 bytes (dxt2/3/4/5)
    ie pitch = (width/4) * bytes per block                                  */
    if (format == WINED3DFMT_DXT1) /* DXT1 is 8 bytes per block */
        ret = ((This->currentDesc.Width + 3) >> 2) << 3;
    else if (format == WINED3DFMT_DXT2 || format == WINED3DFMT_DXT3 ||
             format == WINED3DFMT_DXT4 || format == WINED3DFMT_DXT5) /* DXT2/3/4/5 is 16 bytes per block */
        ret = ((This->currentDesc.Width + 3) >> 2) << 4;
    else {
        unsigned char alignment = This->resource.wineD3DDevice->surface_alignment;
        ret = This->resource.format_desc->byte_count * This->currentDesc.Width;  /* Bytes / row */
        ret = (ret + alignment - 1) & ~(alignment - 1);
    }
    TRACE("(%p) Returning %d\n", This, ret);
    return ret;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetOverlayPosition(IWineD3DSurface *iface, LONG X, LONG Y) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    LONG w, h;

    TRACE("(%p)->(%d,%d) Stub!\n", This, X, Y);

    if(!(This->resource.usage & WINED3DUSAGE_OVERLAY))
    {
        TRACE("(%p): Not an overlay surface\n", This);
        return WINEDDERR_NOTAOVERLAYSURFACE;
    }

    w = This->overlay_destrect.right - This->overlay_destrect.left;
    h = This->overlay_destrect.bottom - This->overlay_destrect.top;
    This->overlay_destrect.left = X;
    This->overlay_destrect.top = Y;
    This->overlay_destrect.right = X + w;
    This->overlay_destrect.bottom = Y + h;

    IWineD3DSurface_DrawOverlay(iface);

    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetOverlayPosition(IWineD3DSurface *iface, LONG *X, LONG *Y) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    HRESULT hr;

    TRACE("(%p)->(%p,%p)\n", This, X, Y);

    if(!(This->resource.usage & WINED3DUSAGE_OVERLAY))
    {
        TRACE("(%p): Not an overlay surface\n", This);
        return WINEDDERR_NOTAOVERLAYSURFACE;
    }
    if(This->overlay_dest == NULL) {
        *X = 0; *Y = 0;
        hr = WINEDDERR_OVERLAYNOTVISIBLE;
    } else {
        *X = This->overlay_destrect.left;
        *Y = This->overlay_destrect.top;
        hr = WINED3D_OK;
    }

    TRACE("Returning 0x%08x, position %d, %d\n", hr, *X, *Y);
    return hr;
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

HRESULT WINAPI IWineD3DBaseSurfaceImpl_UpdateOverlay(IWineD3DSurface *iface, const RECT *SrcRect,
        IWineD3DSurface *DstSurface, const RECT *DstRect, DWORD Flags, const WINEDDOVERLAYFX *FX)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    IWineD3DSurfaceImpl *Dst = (IWineD3DSurfaceImpl *) DstSurface;
    TRACE("(%p)->(%p, %p, %p, %08x, %p)\n", This, SrcRect, Dst, DstRect, Flags, FX);

    if(!(This->resource.usage & WINED3DUSAGE_OVERLAY))
    {
        WARN("(%p): Not an overlay surface\n", This);
        return WINEDDERR_NOTAOVERLAYSURFACE;
    } else if(!DstSurface) {
        WARN("(%p): Dest surface is NULL\n", This);
        return WINED3DERR_INVALIDCALL;
    }

    if(SrcRect) {
        This->overlay_srcrect = *SrcRect;
    } else {
        This->overlay_srcrect.left = 0;
        This->overlay_srcrect.top = 0;
        This->overlay_srcrect.right = This->currentDesc.Width;
        This->overlay_srcrect.bottom = This->currentDesc.Height;
    }

    if(DstRect) {
        This->overlay_destrect = *DstRect;
    } else {
        This->overlay_destrect.left = 0;
        This->overlay_destrect.top = 0;
        This->overlay_destrect.right = Dst ? Dst->currentDesc.Width : 0;
        This->overlay_destrect.bottom = Dst ? Dst->currentDesc.Height : 0;
    }

    if(This->overlay_dest && (This->overlay_dest != Dst || Flags & WINEDDOVER_HIDE)) {
        list_remove(&This->overlay_entry);
    }

    if(Flags & WINEDDOVER_SHOW) {
        if(This->overlay_dest != Dst) {
            This->overlay_dest = Dst;
            list_add_tail(&Dst->overlays, &This->overlay_entry);
        }
    } else if(Flags & WINEDDOVER_HIDE) {
        /* tests show that the rectangles are erased on hide */
        This->overlay_srcrect.left   = 0; This->overlay_srcrect.top     = 0;
        This->overlay_srcrect.right  = 0; This->overlay_srcrect.bottom  = 0;
        This->overlay_destrect.left  = 0; This->overlay_destrect.top    = 0;
        This->overlay_destrect.right = 0; This->overlay_destrect.bottom = 0;
        This->overlay_dest = NULL;
    }

    IWineD3DSurface_DrawOverlay(iface);

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
    const struct GlPixelFormatDesc *format_desc = getFormatDescEntry(format,
            &This->resource.wineD3DDevice->adapter->gl_info);

    if (This->resource.format_desc->format != WINED3DFMT_UNKNOWN)
    {
        FIXME("(%p) : The format of the surface must be WINED3DFORMAT_UNKNOWN\n", This);
        return WINED3DERR_INVALIDCALL;
    }

    TRACE("(%p) : Setting texture format to (%d,%s)\n", This, format, debug_d3dformat(format));
    if (format == WINED3DFMT_UNKNOWN) {
        This->resource.size = 0;
    } else if (format == WINED3DFMT_DXT1) {
        /* DXT1 is half byte per pixel */
        This->resource.size = ((max(This->pow2Width, 4) * format_desc->byte_count) * max(This->pow2Height, 4)) >> 1;

    } else if (format == WINED3DFMT_DXT2 || format == WINED3DFMT_DXT3 ||
               format == WINED3DFMT_DXT4 || format == WINED3DFMT_DXT5) {
        This->resource.size = ((max(This->pow2Width, 4) * format_desc->byte_count) * max(This->pow2Height, 4));
    } else {
        unsigned char alignment = This->resource.wineD3DDevice->surface_alignment;
        This->resource.size = ((This->pow2Width * format_desc->byte_count) + alignment - 1) & ~(alignment - 1);
        This->resource.size *= This->pow2Height;
    }

    This->Flags |= (WINED3DFMT_D16_LOCKABLE == format) ? SFLAG_LOCKABLE : 0;

    This->resource.format_desc = format_desc;

    TRACE("(%p) : Size %d, bytesPerPixel %d\n", This, This->resource.size, format_desc->byte_count);

    return WINED3D_OK;
}

HRESULT IWineD3DBaseSurfaceImpl_CreateDIBSection(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    const struct GlPixelFormatDesc *format_desc = This->resource.format_desc;
    int extraline = 0;
    SYSTEM_INFO sysInfo;
    BITMAPINFO* b_info;
    HDC ddc;
    DWORD *masks;
    UINT usage;

    switch (format_desc->byte_count)
    {
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
                    sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1 << (format_desc->byte_count * 8)));
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
    b_info->bmiHeader.biWidth = IWineD3DSurface_GetPitch(iface) / format_desc->byte_count;
    b_info->bmiHeader.biHeight = -This->currentDesc.Height -extraline;
    b_info->bmiHeader.biSizeImage = ( This->currentDesc.Height + extraline) * IWineD3DSurface_GetPitch(iface);
    b_info->bmiHeader.biPlanes = 1;
    b_info->bmiHeader.biBitCount = format_desc->byte_count * 8;

    b_info->bmiHeader.biXPelsPerMeter = 0;
    b_info->bmiHeader.biYPelsPerMeter = 0;
    b_info->bmiHeader.biClrUsed = 0;
    b_info->bmiHeader.biClrImportant = 0;

    /* Get the bit masks */
    masks = (DWORD *)b_info->bmiColors;
    switch (This->resource.format_desc->format)
    {
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
        case WINED3DFMT_R10G10B10A2_UNORM:
        case WINED3DFMT_R8G8B8A8_UNORM:
        case WINED3DFMT_X8B8G8R8:
        case WINED3DFMT_A2R10G10B10:
        case WINED3DFMT_R5G6B5:
        case WINED3DFMT_R16G16B16A16_UNORM:
            usage = 0;
            b_info->bmiHeader.biCompression = BI_BITFIELDS;
            masks[0] = format_desc->red_mask;
            masks[1] = format_desc->green_mask;
            masks[2] = format_desc->blue_mask;
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
        memcpy(This->dib.bitmap_data, This->resource.allocatedMemory,  This->currentDesc.Height * IWineD3DSurface_GetPitch(iface));
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

static void convert_r32_float_r16_float(const BYTE *src, BYTE *dst, DWORD pitch_in, DWORD pitch_out,
                              unsigned int w, unsigned int h)
{
    unsigned int x, y;
    const float *src_f;
    unsigned short *dst_s;

    TRACE("Converting %dx%d pixels, pitches %d %d\n", w, h, pitch_in, pitch_out);
    for(y = 0; y < h; y++) {
        src_f = (const float *)(src + y * pitch_in);
        dst_s = (unsigned short *) (dst + y * pitch_out);
        for(x = 0; x < w; x++) {
            dst_s[x] = float_32_to_16(src_f + x);
        }
    }
}

static void convert_r5g6b5_x8r8g8b8(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    static const unsigned char convert_5to8[] =
    {
        0x00, 0x08, 0x10, 0x19, 0x21, 0x29, 0x31, 0x3a,
        0x42, 0x4a, 0x52, 0x5a, 0x63, 0x6b, 0x73, 0x7b,
        0x84, 0x8c, 0x94, 0x9c, 0xa5, 0xad, 0xb5, 0xbd,
        0xc5, 0xce, 0xd6, 0xde, 0xe6, 0xef, 0xf7, 0xff,
    };
    static const unsigned char convert_6to8[] =
    {
        0x00, 0x04, 0x08, 0x0c, 0x10, 0x14, 0x18, 0x1c,
        0x20, 0x24, 0x28, 0x2d, 0x31, 0x35, 0x39, 0x3d,
        0x41, 0x45, 0x49, 0x4d, 0x51, 0x55, 0x59, 0x5d,
        0x61, 0x65, 0x69, 0x6d, 0x71, 0x75, 0x79, 0x7d,
        0x82, 0x86, 0x8a, 0x8e, 0x92, 0x96, 0x9a, 0x9e,
        0xa2, 0xa6, 0xaa, 0xae, 0xb2, 0xb6, 0xba, 0xbe,
        0xc2, 0xc6, 0xca, 0xce, 0xd2, 0xd7, 0xdb, 0xdf,
        0xe3, 0xe7, 0xeb, 0xef, 0xf3, 0xf7, 0xfb, 0xff,
    };
    unsigned int x, y;

    TRACE("Converting %ux%u pixels, pitches %u %u\n", w, h, pitch_in, pitch_out);

    for (y = 0; y < h; ++y)
    {
        const WORD *src_line = (const WORD *)(src + y * pitch_in);
        DWORD *dst_line = (DWORD *)(dst + y * pitch_out);
        for (x = 0; x < w; ++x)
        {
            WORD pixel = src_line[x];
            dst_line[x] = 0xff000000
                    | convert_5to8[(pixel & 0xf800) >> 11] << 16
                    | convert_6to8[(pixel & 0x07e0) >> 5] << 8
                    | convert_5to8[(pixel & 0x001f)];
        }
    }
}

struct d3dfmt_convertor_desc {
    WINED3DFORMAT from, to;
    void (*convert)(const BYTE *src, BYTE *dst, DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h);
};

static const struct d3dfmt_convertor_desc convertors[] =
{
    {WINED3DFMT_R32_FLOAT,  WINED3DFMT_R16_FLOAT,   convert_r32_float_r16_float},
    {WINED3DFMT_R5G6B5,     WINED3DFMT_X8R8G8B8,    convert_r5g6b5_x8r8g8b8},
};

static inline const struct d3dfmt_convertor_desc *find_convertor(WINED3DFORMAT from, WINED3DFORMAT to)
{
    unsigned int i;
    for(i = 0; i < (sizeof(convertors) / sizeof(convertors[0])); i++) {
        if(convertors[i].from == from && convertors[i].to == to) {
            return &convertors[i];
        }
    }
    return NULL;
}

/*****************************************************************************
 * surface_convert_format
 *
 * Creates a duplicate of a surface in a different format. Is used by Blt to
 * blit between surfaces with different formats
 *
 * Parameters
 *  source: Source surface
 *  fmt: Requested destination format
 *
 *****************************************************************************/
static IWineD3DSurfaceImpl *surface_convert_format(IWineD3DSurfaceImpl *source, WINED3DFORMAT to_fmt) {
    IWineD3DSurface *ret = NULL;
    const struct d3dfmt_convertor_desc *conv;
    WINED3DLOCKED_RECT lock_src, lock_dst;
    HRESULT hr;

    conv = find_convertor(source->resource.format_desc->format, to_fmt);
    if(!conv) {
        FIXME("Cannot find a conversion function from format %s to %s\n",
              debug_d3dformat(source->resource.format_desc->format), debug_d3dformat(to_fmt));
        return NULL;
    }

    IWineD3DDevice_CreateSurface((IWineD3DDevice *)source->resource.wineD3DDevice,
            source->currentDesc.Width, source->currentDesc.Height, to_fmt, TRUE /* lockable */,
            TRUE /* discard  */, 0 /* level */, &ret, WINED3DRTYPE_SURFACE, 0 /* usage */,
            WINED3DPOOL_SCRATCH, WINED3DMULTISAMPLE_NONE /* TODO: Multisampled conversion */,
            0 /* MultiSampleQuality */, IWineD3DSurface_GetImplType((IWineD3DSurface *) source), NULL /* parent */);
    if(!ret) {
        ERR("Failed to create a destination surface for conversion\n");
        return NULL;
    }

    memset(&lock_src, 0, sizeof(lock_src));
    memset(&lock_dst, 0, sizeof(lock_dst));

    hr = IWineD3DSurface_LockRect((IWineD3DSurface *) source, &lock_src, NULL, WINED3DLOCK_READONLY);
    if(FAILED(hr)) {
        ERR("Failed to lock the source surface\n");
        IWineD3DSurface_Release(ret);
        return NULL;
    }
    hr = IWineD3DSurface_LockRect(ret, &lock_dst, NULL, WINED3DLOCK_READONLY);
    if(FAILED(hr)) {
        ERR("Failed to lock the dest surface\n");
        IWineD3DSurface_UnlockRect((IWineD3DSurface *) source);
        IWineD3DSurface_Release(ret);
        return NULL;
    }

    conv->convert(lock_src.pBits, lock_dst.pBits, lock_src.Pitch, lock_dst.Pitch,
                  source->currentDesc.Width, source->currentDesc.Height);

    IWineD3DSurface_UnlockRect(ret);
    IWineD3DSurface_UnlockRect((IWineD3DSurface *) source);

    return (IWineD3DSurfaceImpl *) ret;
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
            BYTE *d = buf;
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
HRESULT WINAPI IWineD3DBaseSurfaceImpl_Blt(IWineD3DSurface *iface, const RECT *DestRect, IWineD3DSurface *SrcSurface,
        const RECT *SrcRect, DWORD Flags, const WINEDDBLTFX *DDBltFx, WINED3DTEXTUREFILTERTYPE Filter)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    IWineD3DSurfaceImpl *Src = (IWineD3DSurfaceImpl *) SrcSurface;
    RECT        xdst,xsrc;
    HRESULT     ret = WINED3D_OK;
    WINED3DLOCKED_RECT  dlock, slock;
    int bpp, srcheight, srcwidth, dstheight, dstwidth, width;
    const struct GlPixelFormatDesc *sEntry, *dEntry;
    int x, y;
    const BYTE *sbuf;
    BYTE *dbuf;
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

    /* First check for the validity of source / destination rectangles. This was
     * verified using a test application + by MSDN.
     */
    if ((Src != NULL) && (SrcRect != NULL) &&
         ((SrcRect->bottom > Src->currentDesc.Height)||(SrcRect->bottom < 0) ||
         (SrcRect->top     > Src->currentDesc.Height)||(SrcRect->top    < 0) ||
         (SrcRect->left    > Src->currentDesc.Width) ||(SrcRect->left   < 0) ||
         (SrcRect->right   > Src->currentDesc.Width) ||(SrcRect->right  < 0) ||
         (SrcRect->right   < SrcRect->left)          ||(SrcRect->bottom < SrcRect->top)))
    {
        WARN("Application gave us bad source rectangle for Blt.\n");
        return WINEDDERR_INVALIDRECT;
    }
    /* For the Destination rect, it can be out of bounds on the condition that a clipper
     * is set for the given surface.
     */
    if ((/*This->clipper == NULL*/ TRUE) && (DestRect) &&
         ((DestRect->bottom > This->currentDesc.Height)||(DestRect->bottom < 0) ||
         (DestRect->top     > This->currentDesc.Height)||(DestRect->top    < 0) ||
         (DestRect->left    > This->currentDesc.Width) ||(DestRect->left   < 0) ||
         (DestRect->right   > This->currentDesc.Width) ||(DestRect->right  < 0) ||
         (DestRect->right   < DestRect->left)          ||(DestRect->bottom < DestRect->top)))
    {
        WARN("Application gave us bad destination rectangle for Blt without a clipper set.\n");
        return WINEDDERR_INVALIDRECT;
    }

    /* Now handle negative values in the rectangles. Warning: only supported for now
    in the 'simple' cases (ie not in any stretching / rotation cases).

    First, the case where nothing is to be done.
    */
    if ((DestRect && ((DestRect->bottom <= 0) || (DestRect->right <= 0)  ||
          (DestRect->top    >= (int) This->currentDesc.Height) ||
          (DestRect->left   >= (int) This->currentDesc.Width))) ||
          ((Src != NULL) && (SrcRect != NULL) &&
          ((SrcRect->bottom <= 0) || (SrcRect->right <= 0)     ||
          (SrcRect->top >= (int) Src->currentDesc.Height) ||
          (SrcRect->left >= (int) Src->currentDesc.Width))  ))
    {
        TRACE("Nothing to be done !\n");
        return  WINED3D_OK;
    }

    if (DestRect)
    {
        xdst = *DestRect;
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
        xsrc = *SrcRect;
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

    /* The easy case : the source-less blits.... */
    if (Src == NULL && DestRect)
    {
        RECT full_rect;
        RECT temp_rect; /* No idea if intersect rect can be the same as one of the source rect */

        full_rect.left   = 0;
        full_rect.top    = 0;
        full_rect.right  = This->currentDesc.Width;
        full_rect.bottom = This->currentDesc.Height;
        IntersectRect(&temp_rect, &full_rect, DestRect);
        xdst = temp_rect;
    }
    else if (DestRect)
    {
        /* Only handle clipping on the destination rectangle */
        int clip_horiz = (DestRect->left < 0) || (DestRect->right  > (int) This->currentDesc.Width );
        int clip_vert  = (DestRect->top  < 0) || (DestRect->bottom > (int) This->currentDesc.Height);
        if (clip_vert || clip_horiz)
        {
            /* Now check if this is a special case or not... */
            if ((((DestRect->bottom - DestRect->top ) != (xsrc.bottom - xsrc.top )) && clip_vert ) ||
                   (((DestRect->right  - DestRect->left) != (xsrc.right  - xsrc.left)) && clip_horiz) ||
                   (Flags & WINEDDBLT_DDFX))
            {
                WARN("Out of screen rectangle in special case. Not handled right now.\n");
                return  WINED3D_OK;
            }

            if (clip_horiz)
            {
                if (DestRect->left < 0) { xsrc.left -= DestRect->left; xdst.left = 0; }
                if (DestRect->right > This->currentDesc.Width)
                {
                    xsrc.right -= (DestRect->right - (int) This->currentDesc.Width);
                    xdst.right = (int) This->currentDesc.Width;
                }
            }
            if (clip_vert)
            {
                if (DestRect->top < 0)
                {
                    xsrc.top -= DestRect->top;
                    xdst.top = 0;
                }
                if (DestRect->bottom > This->currentDesc.Height)
                {
                    xsrc.bottom -= (DestRect->bottom - (int) This->currentDesc.Height);
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
                return  WINED3D_OK;
            }
        }
    }

    if (Src == This)
    {
        IWineD3DSurface_LockRect(iface, &dlock, NULL, 0);
        slock = dlock;
        sEntry = This->resource.format_desc;
        dEntry = sEntry;
    }
    else
    {
        dEntry = This->resource.format_desc;
        if (Src)
        {
            if (This->resource.format_desc->format != Src->resource.format_desc->format)
            {
                Src = surface_convert_format(Src, dEntry->format);
                if(!Src) {
                    /* The conv function writes a FIXME */
                    WARN("Cannot convert source surface format to dest format\n");
                    goto release;
                }
            }
            IWineD3DSurface_LockRect((IWineD3DSurface *) Src, &slock, NULL, WINED3DLOCK_READONLY);
            sEntry = Src->resource.format_desc;
        }
        else
        {
            sEntry = dEntry;
        }
        if (DestRect)
            IWineD3DSurface_LockRect(iface, &dlock, &xdst, 0);
        else
            IWineD3DSurface_LockRect(iface, &dlock, NULL, 0);
    }

    if (!DDBltFx || !(DDBltFx->dwDDFX)) Flags &= ~WINEDDBLT_DDFX;

    if (sEntry->Flags & dEntry->Flags & WINED3DFMT_FLAG_FOURCC)
    {
        if (!DestRect || Src == This)
        {
            memcpy(dlock.pBits, slock.pBits, This->resource.size);
            goto release;
        }
    }

    bpp = This->resource.format_desc->byte_count;
    srcheight = xsrc.bottom - xsrc.top;
    srcwidth = xsrc.right - xsrc.left;
    dstheight = xdst.bottom - xdst.top;
    dstwidth = xdst.right - xdst.left;
    width = (xdst.right - xdst.left) * bpp;

    assert(width <= dlock.Pitch);

    if (DestRect && Src != This)
        dbuf = dlock.pBits;
    else
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
        const BYTE *sbase;
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
                    if (Src != This || xdst.top < xsrc.top ||
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
                        const type *s = (const type *)sbuf; \
                        type *d = (type *)dbuf; \
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
                                const BYTE *s;
                                BYTE *d = dbuf;
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
                    keymask = sEntry->red_mask
                            | sEntry->green_mask
                            | sEntry->blue_mask;
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
            const type *s; \
            type *d = (type *)dbuf, *dx, tmp; \
            for (y = sy = 0; y < dstheight; y++, sy += yinc) { \
            s = (const type*)(sbase + (sy >> 16) * slock.Pitch); \
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
                    const BYTE *s;
                    BYTE *d = dbuf, *dx;
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
    if (Src && Src != This) IWineD3DSurface_UnlockRect((IWineD3DSurface *) Src);
    /* Release the converted surface if any */
    if (Src && SrcSurface != (IWineD3DSurface *) Src) IWineD3DSurface_Release((IWineD3DSurface *) Src);
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
HRESULT WINAPI IWineD3DBaseSurfaceImpl_BltFast(IWineD3DSurface *iface, DWORD dstx, DWORD dsty,
        IWineD3DSurface *Source, const RECT *rsrc, DWORD trans)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    IWineD3DSurfaceImpl *Src = (IWineD3DSurfaceImpl *) Source;

    int                 bpp, w, h, x, y;
    WINED3DLOCKED_RECT  dlock,slock;
    HRESULT             ret = WINED3D_OK;
    RECT                rsrc2;
    RECT                lock_src, lock_dst, lock_union;
    const BYTE          *sbuf;
    BYTE                *dbuf;
    const struct GlPixelFormatDesc *sEntry, *dEntry;

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
            (Src->Flags & SFLAG_LOCKED))
    {
        WARN(" Surface is busy, returning DDERR_SURFACEBUSY\n");
        return WINEDDERR_SURFACEBUSY;
    }

    if (!rsrc)
    {
        WARN("rsrc is NULL!\n");
        rsrc2.left = 0;
        rsrc2.top = 0;
        rsrc2.right = Src->currentDesc.Width;
        rsrc2.bottom = Src->currentDesc.Height;
        rsrc = &rsrc2;
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

    bpp = This->resource.format_desc->byte_count;

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
        sbuf = This->resource.allocatedMemory + lock_src.top * pitch + lock_src.left * bpp;
        dbuf = This->resource.allocatedMemory + lock_dst.top * pitch + lock_dst.left * bpp;
        sEntry = Src->resource.format_desc;
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

        sEntry = Src->resource.format_desc;
        dEntry = This->resource.format_desc;
    }

    /* Handle first the FOURCC surfaces... */
    if (sEntry->Flags & dEntry->Flags & WINED3DFMT_FLAG_FOURCC)
    {
        UINT block_width;
        UINT block_height;
        UINT block_byte_size;

        TRACE("Fourcc -> Fourcc copy\n");
        if (trans)
            FIXME("trans arg not supported when a FOURCC surface is involved\n");
        if (dstx || dsty)
            FIXME("offset for destination surface is not supported\n");
        if (Src->resource.format_desc->format != This->resource.format_desc->format)
        {
            FIXME("FOURCC->FOURCC copy only supported for the same type of surface\n");
            ret = WINED3DERR_WRONGTEXTUREFORMAT;
            goto error;
        }

        if (This->resource.format_desc->format == WINED3DFMT_DXT1)
        {
            block_width = 4;
            block_height = 4;
            block_byte_size = 8;
        }
        else if (This->resource.format_desc->format == WINED3DFMT_DXT2
                || This->resource.format_desc->format == WINED3DFMT_DXT3
                || This->resource.format_desc->format == WINED3DFMT_DXT4
                || This->resource.format_desc->format == WINED3DFMT_DXT5)
        {
            block_width = 4;
            block_height = 4;
            block_byte_size = 16;
        }
        else
        {
            FIXME("Unsupported FourCC format %s.\n", debug_d3dformat(This->resource.format_desc->format));
            block_width = 1;
            block_height = 1;
            block_byte_size = This->resource.format_desc->byte_count;
        }

        for (y = 0; y < h; y += block_height)
        {
            memcpy(dbuf, sbuf, (w / block_width) * block_byte_size);
            dbuf += dlock.Pitch;
            sbuf += slock.Pitch;
        }

        goto error;
    }
    if ((sEntry->Flags & WINED3DFMT_FLAG_FOURCC) && !(dEntry->Flags & WINED3DFMT_FLAG_FOURCC))
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
        const type *s = (const type *)sbuf; \
        type *d = (type *)dbuf; \
        type tmp; \
        for (y = 0; y < h; y++) { \
        for (x = 0; x < w; x++) { \
        tmp = s[x]; \
        if (tmp < keylow || tmp > keyhigh) d[x] = tmp; \
    } \
        s = (const type *)((const BYTE *)s + slock.Pitch); \
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
                const BYTE *s;
                BYTE *d;
                DWORD tmp;
                s = sbuf;
                d = dbuf;
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
        INT sbufpitch, dbufpitch;

        TRACE("NO color key copy\n");
        /* Handle overlapping surfaces */
        if (sbuf < dbuf)
        {
            sbuf += (h - 1) * slock.Pitch;
            dbuf += (h - 1) * dlock.Pitch;
            sbufpitch = -slock.Pitch;
            dbufpitch = -dlock.Pitch;
        }
        else
        {
            sbufpitch = slock.Pitch;
            dbufpitch = dlock.Pitch;
        }
        for (y = 0; y < h; y++)
        {
            /* This is pretty easy, a line for line memcpy */
            memmove(dbuf, sbuf, width);
            sbuf += sbufpitch;
            dbuf += dbufpitch;
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
        if (This->resource.format_desc->format == WINED3DFMT_DXT1)
        {
            pLockedRect->pBits = This->resource.allocatedMemory + (pLockedRect->Pitch * pRect->top / 4) + (pRect->left * 2);
        }
        else if (This->resource.format_desc->format == WINED3DFMT_DXT2
                || This->resource.format_desc->format == WINED3DFMT_DXT3
                || This->resource.format_desc->format == WINED3DFMT_DXT4
                || This->resource.format_desc->format == WINED3DFMT_DXT5)
        {
            pLockedRect->pBits = This->resource.allocatedMemory + (pLockedRect->Pitch * pRect->top / 4) + (pRect->left * 4);
        }
        else
        {
            pLockedRect->pBits = This->resource.allocatedMemory +
                    (pLockedRect->Pitch * pRect->top) +
                    (pRect->left * This->resource.format_desc->byte_count);
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

void WINAPI IWineD3DBaseSurfaceImpl_BindTexture(IWineD3DSurface *iface, BOOL srgb) {
    ERR("Should not be called on base texture\n");
    return;
}

/* TODO: think about moving this down to resource? */
const void *WINAPI IWineD3DBaseSurfaceImpl_GetData(IWineD3DSurface *iface)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;

    /* This should only be called for sysmem textures, it may be a good idea
     * to extend this to all pools at some point in the future  */
    if (This->resource.pool != WINED3DPOOL_SYSTEMMEM)
    {
        FIXME("(%p) Attempting to get system memory for a non-system memory texture\n", iface);
    }
    return This->resource.allocatedMemory;
}
