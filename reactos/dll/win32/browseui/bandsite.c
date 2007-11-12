/*
 *	Rebar band site
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

#include <stdarg.h>

#define COBJMACROS

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "shlwapi.h"
#include "winerror.h"
#include "objbase.h"

#include "shlguid.h"
#include "shlobj.h"
#include "shobjidl.h"
#include "todo.h"
#include "undoc.h"

#include "wine/unicode.h"

#include "browseui.h"

WINE_DEFAULT_DEBUG_CHANNEL(browseui);

struct SubBand {
    DWORD dwBandID;
    IUnknown *punk;
};

typedef struct tagBandSite {
    const IBandSiteVtbl *vtbl;
    const IWindowEventHandlerVtbl *eventhandlerVtbl;
    const IDeskBarClientVtbl *deskbarVtbl;
    LONG refCount;
    INT nObjs;
    struct SubBand *objs;
} BandSite;

static const IBandSiteVtbl BandSiteVtbl;
static const IWindowEventHandlerVtbl BandSite_EventHandlerVtbl;
static const IDeskBarClientVtbl BandSite_DeskBarVtbl;

static inline BandSite *impl_from_IWindowEventHandler(IWindowEventHandler *iface)
{
    return (BandSite *)((char *)iface - FIELD_OFFSET(BandSite, eventhandlerVtbl));
}

static inline BandSite *impl_from_IDeskBarClient(IDeskBarClient *iface)
{
    return (BandSite *)((char *)iface - FIELD_OFFSET(BandSite, deskbarVtbl));
}

static struct SubBand *get_band(BandSite *This, DWORD dwBandID)
{
    INT i;

    for (i = 0; i < This->nObjs; i++)
        if (This->objs[i].dwBandID == dwBandID)
            return &This->objs[i];
    return NULL;
}

static void release_obj(struct SubBand *obj)
{
    IUnknown_Release(obj->punk);
}

HRESULT WINAPI BandSite_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut)
{
    BandSite *This;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    This = CoTaskMemAlloc(sizeof(BandSite));
    if (This == NULL)
        return E_OUTOFMEMORY;
    ZeroMemory(This, sizeof(*This));
    This->vtbl = &BandSiteVtbl;
    This->eventhandlerVtbl = &BandSite_EventHandlerVtbl;
    This->refCount = 1;

    TRACE("returning %p\n", This);
    *ppOut = (IUnknown *)This;
    BROWSEUI_refCount++;
    return S_OK;
}

static void WINAPI BandSite_Destructor(BandSite *This)
{
    int i;
    TRACE("destroying %p\n", This);
    for (i = 0; i < This->nObjs; i++)
        release_obj(&This->objs[i]);
    CoTaskMemFree(This->objs);
    CoTaskMemFree(This);
    BROWSEUI_refCount--;
}

static HRESULT WINAPI BandSite_QueryInterface(IBandSite *iface, REFIID iid, LPVOID *ppvOut)
{
    BandSite *This = (BandSite *)iface;
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IBandSite))
    {
        *ppvOut = This;
    }
    else if (IsEqualIID(iid, &IID_IWindowEventHandler))
    {
        *ppvOut = &This->eventhandlerVtbl;
    }
    else if (IsEqualIID(iid, &IID_IOleWindow) || IsEqualIID(iid, &IID_IDeskBarClient))
    {
        *ppvOut = &This->deskbarVtbl;
    }

    if (*ppvOut)
    {
        IUnknown_AddRef(iface);
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI BandSite_AddRef(IBandSite *iface)
{
    BandSite *This = (BandSite *)iface;
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI BandSite_Release(IBandSite *iface)
{
    BandSite *This = (BandSite *)iface;
    ULONG ret;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        BandSite_Destructor(This);
    return ret;
}

static HRESULT WINAPI BandSite_AddBand(IBandSite *iface, IUnknown *punk)
{
    BandSite *This = (BandSite *)iface;
    struct SubBand *newObjs, *current;
    DWORD freeID;

    newObjs = CoTaskMemAlloc((This->nObjs + 1) * sizeof(struct SubBand));
    if (!newObjs)
        return E_OUTOFMEMORY;
    CopyMemory(newObjs, This->objs, This->nObjs * sizeof(struct SubBand));
    current = &newObjs[This->nObjs];

    freeID = This->nObjs;
    while (get_band(This, freeID) != NULL)
        freeID--;
    current->dwBandID = freeID;
    current->punk = punk;

    CoTaskMemFree(This->objs);
    This->objs = newObjs;
    This->nObjs++;

    return S_OK;
}

static HRESULT WINAPI BandSite_EnumBands(IBandSite *iface, UINT uBand, DWORD *pdwBandID)
{
    BandSite *This = (BandSite *)iface;

    if (uBand >= This->nObjs)
        return E_FAIL;

    *pdwBandID = This->objs[uBand].dwBandID;
    return S_OK;
}

static HRESULT WINAPI BandSite_QueryBand(IBandSite *iface, DWORD dwBandID, IDeskBand **ppstb, DWORD *pdwState, LPWSTR pszName, int cchName)
{
    BandSite *This = (BandSite *)iface;
    struct SubBand *band;

    band = get_band(This, dwBandID);
    if (!band)
        return E_FAIL;

    return E_NOTIMPL;
}

static HRESULT WINAPI BandSite_SetBandState(IBandSite *iface, DWORD dwBandID, DWORD dwMask, DWORD dwState)
{
    BandSite *This = (BandSite *)iface;
    struct SubBand *band;

    band = get_band(This, dwBandID);
    if (!band)
        return E_FAIL;

    return E_NOTIMPL;
}

static HRESULT WINAPI BandSite_RemoveBand(IBandSite *iface, DWORD dwBandID)
{
    BandSite *This = (BandSite *)iface;
    struct SubBand *band;

    band = get_band(This, dwBandID);
    if (!band)
        return E_FAIL;

    This->nObjs--;
    MoveMemory(band, band + 1, (This->objs - band - 1) * sizeof(struct SubBand));
    return S_OK;
}

static HRESULT WINAPI BandSite_GetBandObject(IBandSite *iface, DWORD dwBandID, REFIID riid, VOID **ppv)
{
    BandSite *This = (BandSite *)iface;
    struct SubBand *band;

    band = get_band(This, dwBandID);
    if (!band)
        return E_FAIL;

    return IUnknown_QueryInterface(band->punk, riid, ppv);
}

static HRESULT WINAPI BandSite_SetBandSiteInfo(IBandSite *iface, const BANDSITEINFO *pbsinfo)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI BandSite_GetBandSiteInfo(IBandSite *iface, BANDSITEINFO *pbsinfo)
{
    return E_NOTIMPL;
}

static const IBandSiteVtbl BandSiteVtbl =
{
    BandSite_QueryInterface,
    BandSite_AddRef,
    BandSite_Release,

    BandSite_AddBand,
    BandSite_EnumBands,
    BandSite_QueryBand,
    BandSite_SetBandState,
    BandSite_RemoveBand,
    BandSite_GetBandObject,
    BandSite_SetBandSiteInfo,
    BandSite_GetBandSiteInfo,
};

static HRESULT WINAPI BandSite_IWindowEventHandler_QueryInterface(IWindowEventHandler *iface, REFIID iid, LPVOID *ppvOut)
{
    BandSite *This = impl_from_IWindowEventHandler(iface);
    return BandSite_QueryInterface((IBandSite *)This, iid, ppvOut);
}

static ULONG WINAPI BandSite_IWindowEventHandler_AddRef(IWindowEventHandler *iface)
{
    BandSite *This = impl_from_IWindowEventHandler(iface);
    return BandSite_AddRef((IBandSite *)This);
}

static ULONG WINAPI BandSite_IWindowEventHandler_Release(IWindowEventHandler *iface)
{
    BandSite *This = impl_from_IWindowEventHandler(iface);
    return BandSite_Release((IBandSite *)This);
}

static HRESULT WINAPI BandSite_ProcessMessage(IWindowEventHandler *iface, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plrResult)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI BandSite_ContainsWindow(IWindowEventHandler *iface, HWND hWnd)
{
    return E_NOTIMPL;
}

static const IWindowEventHandlerVtbl BandSite_EventHandlerVtbl =
{
    BandSite_IWindowEventHandler_QueryInterface,
    BandSite_IWindowEventHandler_AddRef,
    BandSite_IWindowEventHandler_Release,

    BandSite_ProcessMessage,
    BandSite_ContainsWindow,
};

static HRESULT WINAPI BandSite_IDeskBarClient_QueryInterface(IDeskBarClient *iface, REFIID iid, LPVOID *ppvOut)
{
    BandSite *This = impl_from_IDeskBarClient(iface);
    return BandSite_QueryInterface((IBandSite *)This, iid, ppvOut);
}

static ULONG WINAPI BandSite_IDeskBarClient_AddRef(IDeskBarClient *iface)
{
    BandSite *This = impl_from_IDeskBarClient(iface);
    return BandSite_AddRef((IBandSite *)This);
}

static ULONG WINAPI BandSite_IDeskBarClient_Release(IDeskBarClient *iface)
{
    BandSite *This = impl_from_IDeskBarClient(iface);
    return BandSite_Release((IBandSite *)This);
}

static HRESULT WINAPI BandSite_IDeskBarClient_GetWindow(IDeskBarClient *iface, HWND *phWnd)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI BandSite_IDeskBarClient_ContextSensitiveHelp(IDeskBarClient *iface, BOOL fEnterMode)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI BandSite_SetDeskBarSite(IDeskBarClient *iface, IUnknown *pUnk)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI BandSite_SetModeDBC(IDeskBarClient *iface, DWORD unknown)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI BandSite_UIActivateDBC(IDeskBarClient *iface, DWORD unknown)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI BandSite_GetSize(IDeskBarClient *iface, DWORD unknown1, LPRECT unknown2)
{
    return E_NOTIMPL;
}

static const IDeskBarClientVtbl BandSite_DeskBarVtbl =
{
    BandSite_IDeskBarClient_QueryInterface,
    BandSite_IDeskBarClient_AddRef,
    BandSite_IDeskBarClient_Release,

    BandSite_IDeskBarClient_GetWindow,
    BandSite_IDeskBarClient_ContextSensitiveHelp,

    BandSite_SetDeskBarSite,
    BandSite_SetModeDBC,
    BandSite_UIActivateDBC,
    BandSite_GetSize,
};
