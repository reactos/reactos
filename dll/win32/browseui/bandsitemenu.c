/*
 *	Band site menu
 *
 *	Copyright 2007	Hervé Poussineua
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

#define COBJMACROS

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "shlobj.h"
#include "shobjidl.h"
#include "todo.h"

#include "browseui.h"

WINE_DEFAULT_DEBUG_CHANNEL(browseui);

typedef struct _tagBandSiteMenu {
    const IShellServiceVtbl *vtbl;
    LONG refCount;
} BandSiteMenu;

static const IShellServiceVtbl BandSiteMenuVtbl;

static inline BandSiteMenu *impl_from_IShellService(IShellService *iface)
{
    return (BandSiteMenu *)((char *)iface - FIELD_OFFSET(BandSiteMenu, vtbl));
}

HRESULT WINAPI BandSiteMenu_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut)
{
    BandSiteMenu *This;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    This = CoTaskMemAlloc(sizeof(BandSiteMenu));
    if (This == NULL)
        return E_OUTOFMEMORY;
    ZeroMemory(This, sizeof(*This));
    This->vtbl = &BandSiteMenuVtbl;
    This->refCount = 1;

    TRACE("returning %p\n", This);
    *ppOut = (IUnknown *)This;
    BROWSEUI_refCount++;
    return S_OK;
}

static void WINAPI BandSiteMenu_Destructor(BandSiteMenu *This)
{
    TRACE("destroying %p\n", This);
    CoTaskMemFree(This);
    BROWSEUI_refCount--;
}

static HRESULT WINAPI BandSiteMenu_QueryInterface(IShellService *iface, REFIID iid, LPVOID *ppvOut)
{
    BandSiteMenu *This = impl_from_IShellService(iface);
    *ppvOut = NULL;

    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(iid), ppvOut);

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IShellService))
    {
        *ppvOut = &This->vtbl;
    }

    if (*ppvOut)
    {
        IUnknown_AddRef(iface);
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI BandSiteMenu_AddRef(IShellService *iface)
{
    BandSiteMenu *This = impl_from_IShellService(iface);
    TRACE("(%p)\n", iface);
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI BandSiteMenu_Release(IShellService *iface)
{
    BandSiteMenu *This = impl_from_IShellService(iface);
    ULONG ret;

    TRACE("(%p)\n", iface);

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        BandSiteMenu_Destructor(This);
    return ret;
}

static HRESULT WINAPI BandSiteMenu_SetOwner(IShellService *iface, IUnknown *pOwner)
{
    FIXME("(%p, %p)\n", iface, pOwner);
    return E_NOTIMPL;
}

static const IShellServiceVtbl BandSiteMenuVtbl =
{
    BandSiteMenu_QueryInterface,
    BandSiteMenu_AddRef,
    BandSiteMenu_Release,

    BandSiteMenu_SetOwner,
};
