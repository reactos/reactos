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

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#define COBJMACROS

//#ifndef WINE_NATIVEWIN32
# include "windef.h"
# include "winbase.h"
# include "winnls.h"
# include "winerror.h"
# include "wingdi.h"
//#endif


#include "ddraw.h"
#include "d3d.h"

#include "ddraw_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

const GUID IID_IParent = {0xc20e4c88, 0x74e7, 0x4940, {0xba, 0x9f, 0x2e, 0x32, 0x3f, 0x9d, 0xc9, 0x81}};

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
    ICOM_THIS_FROM(IParentImpl, IParent, iface);
    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), obj);

    *obj = NULL;
    if ( IsEqualGUID( &IID_IUnknown, riid ) ||
         IsEqualGUID( &IID_IParent, riid ) )
    {
        *obj = ICOM_INTERFACE(This, IParent);
        IParent_AddRef(ICOM_INTERFACE(This, IParent));
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
    ICOM_THIS_FROM(IParentImpl, IParent, iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %d\n", This, ref - 1);

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
static ULONG WINAPI 
IParentImpl_Release(IParent *iface)
{
    ICOM_THIS_FROM(IParentImpl, IParent, iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %d\n", This, ref);

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
const IParentVtbl IParent_Vtbl =
{
     IParentImpl_QueryInterface,
     IParentImpl_AddRef,
     IParentImpl_Release,
};
