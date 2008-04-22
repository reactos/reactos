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

#include "docobj.h"
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
    const IOleCommandTargetVtbl *oletargetVtbl;
    LONG refCount;
    INT nObjs;
    struct SubBand *objs;
    IUnknown *pUnkOuter;
} BandSite;

static const IBandSiteVtbl BandSiteVtbl;
static const IWindowEventHandlerVtbl BandSite_EventHandlerVtbl;
static const IDeskBarClientVtbl BandSite_DeskBarVtbl;
static const IOleCommandTargetVtbl BandSite_OleTargetVtbl;

static inline BandSite *impl_from_IWindowEventHandler(IWindowEventHandler *iface)
{
    return (BandSite *)((char *)iface - FIELD_OFFSET(BandSite, eventhandlerVtbl));
}

static inline BandSite *impl_from_IDeskBarClient(IDeskBarClient *iface)
{
    return (BandSite *)((char *)iface - FIELD_OFFSET(BandSite, deskbarVtbl));
}

static inline BandSite *impl_from_IOleCommandTarget(IOleCommandTarget *iface)
{
    return (BandSite *)((char *)iface - FIELD_OFFSET(BandSite, oletargetVtbl));
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

    if (!pUnkOuter)
        return E_POINTER;

    This = CoTaskMemAlloc(sizeof(BandSite));
    if (This == NULL)
        return E_OUTOFMEMORY;
    ZeroMemory(This, sizeof(*This));
    This->pUnkOuter = pUnkOuter;
    IUnknown_AddRef(pUnkOuter);
    This->vtbl = &BandSiteVtbl;
    This->eventhandlerVtbl = &BandSite_EventHandlerVtbl;
    This->deskbarVtbl = &BandSite_DeskBarVtbl;
    This->oletargetVtbl = &BandSite_OleTargetVtbl;
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
    IUnknown_Release(This->pUnkOuter);
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

    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(iid), ppvOut);

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IBandSite))
    {
        *ppvOut = &This->vtbl;
    }
    else if (IsEqualIID(iid, &IID_IWindowEventHandler))
    {
        *ppvOut = &This->eventhandlerVtbl;
    }
    else if (IsEqualIID(iid, &IID_IOleWindow) || IsEqualIID(iid, &IID_IDeskBarClient))
    {
        *ppvOut = &This->deskbarVtbl;
    }
    else if (IsEqualIID(iid, &IID_IOleCommandTarget))
    {
        *ppvOut = &This->oletargetVtbl;
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
    TRACE("(%p)\n", iface);
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI BandSite_Release(IBandSite *iface)
{
    BandSite *This = (BandSite *)iface;
    ULONG ret;

    TRACE("(%p)\n", iface);

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

    TRACE("(%p, %p)\n", iface, punk);

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

    TRACE("(%p, %u, %p)\n", iface, uBand, pdwBandID);

    if (uBand >= This->nObjs)
        return E_FAIL;

    *pdwBandID = This->objs[uBand].dwBandID;
    return S_OK;
}

static HRESULT WINAPI BandSite_QueryBand(IBandSite *iface, DWORD dwBandID, IDeskBand **ppstb, DWORD *pdwState, LPWSTR pszName, int cchName)
{
    BandSite *This = (BandSite *)iface;
    struct SubBand *band;

    TRACE("(%p, %u, %p, %p, %p, %d)\n", iface, dwBandID, ppstb, pdwState, pszName, cchName);

    band = get_band(This, dwBandID);
    if (!band)
        return E_FAIL;

    FIXME("Stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BandSite_SetBandState(IBandSite *iface, DWORD dwBandID, DWORD dwMask, DWORD dwState)
{
    BandSite *This = (BandSite *)iface;
    struct SubBand *band;

    TRACE("(%p, %u, %x, %x)\n", iface, dwBandID, dwMask, dwState);

    band = get_band(This, dwBandID);
    if (!band)
        return E_FAIL;

    FIXME("Stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BandSite_RemoveBand(IBandSite *iface, DWORD dwBandID)
{
    BandSite *This = (BandSite *)iface;
    struct SubBand *band;

    TRACE("(%p, %u)\n", iface, dwBandID);

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

    TRACE("(%p, %u, %s, %p)\n", iface, dwBandID, debugstr_guid(riid), ppv);

    band = get_band(This, dwBandID);
    if (!band)
        return E_FAIL;

    return IUnknown_QueryInterface(band->punk, riid, ppv);
}

static HRESULT WINAPI BandSite_SetBandSiteInfo(IBandSite *iface, const BANDSITEINFO *pbsinfo)
{
    FIXME("(%p, %p)\n", iface, pbsinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI BandSite_GetBandSiteInfo(IBandSite *iface, BANDSITEINFO *pbsinfo)
{
    FIXME("(%p, %p)\n", iface, pbsinfo);
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
    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(iid), ppvOut);
    return BandSite_QueryInterface((IBandSite *)This, iid, ppvOut);
}

static ULONG WINAPI BandSite_IWindowEventHandler_AddRef(IWindowEventHandler *iface)
{
    BandSite *This = impl_from_IWindowEventHandler(iface);
    TRACE("(%p)\n", iface);
    return BandSite_AddRef((IBandSite *)This);
}

static ULONG WINAPI BandSite_IWindowEventHandler_Release(IWindowEventHandler *iface)
{
    BandSite *This = impl_from_IWindowEventHandler(iface);
    TRACE("(%p)\n", iface);
    return BandSite_Release((IBandSite *)This);
}

static HRESULT WINAPI BandSite_ProcessMessage(IWindowEventHandler *iface, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plrResult)
{
    FIXME("(%p, %p, %u, %p, %p, %p)\n", iface, hWnd, uMsg, wParam, lParam, plrResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI BandSite_ContainsWindow(IWindowEventHandler *iface, HWND hWnd)
{
    FIXME("(%p, %p)\n", iface, hWnd);
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
    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(iid), ppvOut);
    return BandSite_QueryInterface((IBandSite *)This, iid, ppvOut);
}

static ULONG WINAPI BandSite_IDeskBarClient_AddRef(IDeskBarClient *iface)
{
    BandSite *This = impl_from_IDeskBarClient(iface);
    TRACE("(%p)\n", iface);
    return BandSite_AddRef((IBandSite *)This);
}

static ULONG WINAPI BandSite_IDeskBarClient_Release(IDeskBarClient *iface)
{
    BandSite *This = impl_from_IDeskBarClient(iface);
    TRACE("(%p)\n", iface);
    return BandSite_Release((IBandSite *)This);
}

static HRESULT WINAPI BandSite_IDeskBarClient_GetWindow(IDeskBarClient *iface, HWND *phWnd)
{
    FIXME("(%p, %p)\n", iface, phWnd);
    return E_NOTIMPL;
}

static HRESULT WINAPI BandSite_IDeskBarClient_ContextSensitiveHelp(IDeskBarClient *iface, BOOL fEnterMode)
{
    FIXME("(%p, %d)\n", iface, fEnterMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI BandSite_IDeskBarClient_SetDeskBarSite(IDeskBarClient *iface, IUnknown *pUnk)
{
    FIXME("(%p, %p)\n", iface, pUnk);
    return E_NOTIMPL;
}

static HRESULT WINAPI BandSite_IDeskBarClient_SetModeDBC(IDeskBarClient *iface, DWORD unknown)
{
    FIXME("(%p, %x)\n", iface, unknown);
    return E_NOTIMPL;
}

static HRESULT WINAPI BandSite_IDeskBarClient_UIActivateDBC(IDeskBarClient *iface, DWORD unknown)
{
    FIXME("(%p, %x)\n", iface, unknown);
    return E_NOTIMPL;
}

static HRESULT WINAPI BandSite_IDeskBarClient_GetSize(IDeskBarClient *iface, DWORD unknown1, LPRECT unknown2)
{
    FIXME("(%p, %x, %p)\n", iface, unknown1, unknown2);
    return E_NOTIMPL;
}

static const IDeskBarClientVtbl BandSite_DeskBarVtbl =
{
    BandSite_IDeskBarClient_QueryInterface,
    BandSite_IDeskBarClient_AddRef,
    BandSite_IDeskBarClient_Release,

    BandSite_IDeskBarClient_GetWindow,
    BandSite_IDeskBarClient_ContextSensitiveHelp,

    BandSite_IDeskBarClient_SetDeskBarSite,
    BandSite_IDeskBarClient_SetModeDBC,
    BandSite_IDeskBarClient_UIActivateDBC,
    BandSite_IDeskBarClient_GetSize,
};

static HRESULT WINAPI BandSite_IOleCommandTarget_QueryInterface(IOleCommandTarget *iface, REFIID iid, LPVOID *ppvOut)
{
    BandSite *This = impl_from_IOleCommandTarget(iface);
    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(iid), ppvOut);
    return BandSite_QueryInterface((IBandSite *)This, iid, ppvOut);
}

static ULONG WINAPI BandSite_IOleCommandTarget_AddRef(IOleCommandTarget *iface)
{
    BandSite *This = impl_from_IOleCommandTarget(iface);
    TRACE("(%p)\n", iface);
    return BandSite_AddRef((IBandSite *)This);
}

static ULONG WINAPI BandSite_IOleCommandTarget_Release(IOleCommandTarget *iface)
{
    BandSite *This = impl_from_IOleCommandTarget(iface);
    TRACE("(%p)\n", iface);
    return BandSite_Release((IBandSite *)This);
}

static HRESULT WINAPI BandSite_IOleCommandTarget_QueryStatus(IOleCommandTarget *iface, const GUID *pguidCmdGroup, DWORD cCmds, OLECMD *prgCmds, OLECMDTEXT *pCmdText)
{
    FIXME("(%p, %p, %u, %p, %p)\n", iface, pguidCmdGroup, cCmds, prgCmds, pCmdText);
    return E_NOTIMPL;
}

static HRESULT WINAPI BandSite_IOleCommandTarget_Exec(IOleCommandTarget *iface, const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    FIXME("(%p, %p, %u, %u, %p, %p)\n", iface, pguidCmdGroup, nCmdID, nCmdExecOpt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static const IOleCommandTargetVtbl BandSite_OleTargetVtbl =
{
    BandSite_IOleCommandTarget_QueryInterface,
    BandSite_IOleCommandTarget_AddRef,
    BandSite_IOleCommandTarget_Release,

    BandSite_IOleCommandTarget_QueryStatus,
    BandSite_IOleCommandTarget_Exec,
};
