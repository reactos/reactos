/* IWineD3DClipper implementation
 *
 * Copyright 2000 (c) Marcus Meissner
 * Copyright 2000 (c) TransGaming Technologies Inc.
 * Copyright 2006 (c) Stefan Dösinger
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
#include <stdio.h>
#ifdef HAVE_FLOAT_H
# include <float.h>
#endif
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

static HRESULT WINAPI IWineD3DClipperImpl_QueryInterface(IWineD3DClipper *iface, REFIID riid, void **Obj)
{
    IWineD3DClipperImpl *This = (IWineD3DClipperImpl *)iface;

    TRACE("(%p)->(%p,%p)\n", This, riid, Obj);
    if (IsEqualGUID(&IID_IUnknown, riid)
        || IsEqualGUID(&IID_IWineD3DClipper, riid))
    {
        *Obj = iface;
        IWineD3DClipper_AddRef(iface);
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

static ULONG WINAPI IWineD3DClipperImpl_AddRef(IWineD3DClipper *iface )
{
    IWineD3DClipperImpl *This = (IWineD3DClipperImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->() incrementing from %u.\n", This, ref - 1);

    return ref;
}

static ULONG WINAPI IWineD3DClipperImpl_Release(IWineD3DClipper *iface)
{
    IWineD3DClipperImpl *This = (IWineD3DClipperImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->() decrementing from %u.\n", This, ref + 1);

    if (ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This);
        return 0;
    }
    else return ref;
}

static HRESULT WINAPI IWineD3DClipperImpl_GetParent(IWineD3DClipper *iface, IUnknown **Parent)
{
    IWineD3DClipperImpl *This = (IWineD3DClipperImpl *)iface;
    TRACE("(%p)->(%p)\n", This, Parent);

    *Parent = This->Parent;
    IUnknown_AddRef(*Parent);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DClipperImpl_SetHwnd(IWineD3DClipper *iface, DWORD Flags, HWND hWnd)
{
    IWineD3DClipperImpl *This = (IWineD3DClipperImpl *)iface;

    TRACE("(%p)->(0x%08x,%p)\n", This, Flags, hWnd);
    if( Flags )
    {
        FIXME("Flags = 0x%08x, not supported.\n",Flags);
        return WINED3DERR_INVALIDCALL;
    }

    This->hWnd = hWnd;
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DClipperImpl_GetClipList(IWineD3DClipper *iface, RECT *Rect, RGNDATA *ClipList, DWORD *Size)
{
    IWineD3DClipperImpl *This = (IWineD3DClipperImpl *)iface;
    TRACE("(%p,%p,%p,%p)\n", This, Rect, ClipList, Size);

    if (This->hWnd)
    {
        HDC hDC = GetDCEx(This->hWnd, NULL, DCX_WINDOW);
        if (hDC)
        {
            HRGN hRgn = CreateRectRgn(0,0,0,0);
            if (GetRandomRgn(hDC, hRgn, SYSRGN))
            {
                if (GetVersion() & 0x80000000)
                {
                    /* map region to screen coordinates */
                    POINT org;
                    GetDCOrgEx( hDC, &org );
                    OffsetRgn( hRgn, org.x, org.y );
                }
                if (Rect)
                {
                    HRGN hRgnClip = CreateRectRgn(Rect->left, Rect->top,
                        Rect->right, Rect->bottom);
                    CombineRgn(hRgn, hRgn, hRgnClip, RGN_AND);
                    DeleteObject(hRgnClip);
                }
                *Size = GetRegionData(hRgn, *Size, ClipList);
            }
            DeleteObject(hRgn);
            ReleaseDC(This->hWnd, hDC);
        }
        return WINED3D_OK;
    }
    else
    {
        static int warned = 0;
        if (warned++ < 10)
            FIXME("(%p,%p,%p,%p),stub!\n",This,Rect,ClipList,Size);
        if (Size) *Size=0;
        return WINEDDERR_NOCLIPLIST;
    }
}

static HRESULT WINAPI IWineD3DClipperImpl_SetClipList(IWineD3DClipper *iface, RGNDATA *rgn, DWORD Flags)
{
    IWineD3DClipperImpl *This = (IWineD3DClipperImpl *)iface;
    static int warned = 0;

    if (warned++ < 10 || rgn == NULL)
        FIXME("(%p,%p,%d),stub!\n",This,rgn,Flags);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DClipperImpl_GetHwnd(IWineD3DClipper *iface, HWND *hwnd)
{
    IWineD3DClipperImpl *This = (IWineD3DClipperImpl *)iface;
    TRACE("(%p)->(%p)\n", This, hwnd);

    *hwnd = This->hWnd;
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DClipperImpl_IsClipListChanged(IWineD3DClipper *iface, BOOL *changed)
{
    IWineD3DClipperImpl *This = (IWineD3DClipperImpl *)iface;
    FIXME("(%p)->(%p),stub!\n",This,changed);

    /* XXX What is safest? */
    *changed = FALSE;

    return WINED3D_OK;
}

static const IWineD3DClipperVtbl IWineD3DClipper_Vtbl =
{
    IWineD3DClipperImpl_QueryInterface,
    IWineD3DClipperImpl_AddRef,
    IWineD3DClipperImpl_Release,
    IWineD3DClipperImpl_GetParent,
    IWineD3DClipperImpl_GetClipList,
    IWineD3DClipperImpl_GetHwnd,
    IWineD3DClipperImpl_IsClipListChanged,
    IWineD3DClipperImpl_SetClipList,
    IWineD3DClipperImpl_SetHwnd
};

IWineD3DClipper* WINAPI WineDirect3DCreateClipper(IUnknown *Parent)
{
    IWineD3DClipperImpl *obj;

    TRACE("Creating clipper, parent %p\n", Parent);
    obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*obj));
    if(!obj)
    {
        ERR("Out of memory when trying to allocate a WineD3D Clipper\n");
        return NULL;
    }

    obj->lpVtbl = &IWineD3DClipper_Vtbl;
    obj->Parent = Parent;

    IWineD3DClipper_AddRef((IWineD3DClipper *)obj);
    return (IWineD3DClipper *) obj;
}
