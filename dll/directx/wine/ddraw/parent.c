/*
 * IParent implementation
 *
 * Copyright (c) 2006 Stefan Dösinger
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
 *
 * A universal parent interface for everything in WineD3D that doesn't have
 * a DDraw counterpart
 */

#include "config.h"
#include "wine/port.h"

#include "ddraw_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

/*****************************************************************************
 * IUnknown methods
 *****************************************************************************/

/*****************************************************************************
 * IParent::Queryinterface
 *
 * It can't query any interfaces, and it's not used for anything. So
 * it just returns E_NOINTERFACE
 *
 * Params:
 *  riid: guid of queried interface
 *  obj: returns a pointer to the interface
 *
 * Return values
 *  This implementation always returns E_NOINTERFACE and NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IParentImpl_QueryInterface(IParent *iface,
                           REFIID riid,
                           void **obj)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), obj);

    *obj = NULL;
    if ( IsEqualGUID( &IID_IUnknown, riid ) ||
         IsEqualGUID( &IID_IParent, riid ) )
    {
        *obj = iface;
        IParent_AddRef(iface);
        return DD_OK;
    }
    return E_NOINTERFACE;
}

/*****************************************************************************
 * IParent::AddRef
 *
 * Increases the refcount
 *
 * Params:
 *
 * Return values
 *  The new refcount
 *
 *****************************************************************************/
static ULONG WINAPI
IParentImpl_AddRef(IParent *iface)
{
    IParentImpl *This = (IParentImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u.\n", This, ref);

    return ref;
}


/*****************************************************************************
 * IParent::Release
 *
 * Releases the refcount, and destroys the object if the refcount falls to 0
 * Also releases the child object, if destroyed. That's almost the whole sense
 * of this interface.
 *
 * Params:
 *
 * Return values
 *  The new refcount
 *
 *****************************************************************************/
static ULONG WINAPI IParentImpl_Release(IParent *iface)
{
    IParentImpl *This = (IParentImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", This, ref);

    if (ref == 0)
    {
        TRACE("(%p) Releasing child at %p\n", This, This->child);
        if(This->child)
            IUnknown_Release(This->child);
        HeapFree(GetProcessHeap(), 0, This);
        TRACE("Released\n");
    }
    return ref;
}

/*****************************************************************************
 * The VTable
 *****************************************************************************/
static const struct IParentVtbl ddraw_parent_vtbl =
{
     IParentImpl_QueryInterface,
     IParentImpl_AddRef,
     IParentImpl_Release,
};

void ddraw_parent_init(IParentImpl *parent)
{
    parent->lpVtbl = &ddraw_parent_vtbl;
    parent->ref = 1;
}
