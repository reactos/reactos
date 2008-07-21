/*
 * Copyright 2005 Jacek Caban
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
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct {
    const IOleUndoManagerVtbl  *lpOleUndoManagerVtbl;

    LONG ref;
} UndoManager;

#define UNDOMGR(x)  ((IOleUndoManager*)  &(x)->lpOleUndoManagerVtbl)

#define UNDOMGR_THIS(iface) DEFINE_THIS(UndoManager, OleUndoManager, iface)

static HRESULT WINAPI OleUndoManager_QueryInterface(IOleUndoManager *iface, REFIID riid, void **ppv)
{
    UndoManager *This = UNDOMGR_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = UNDOMGR(This);
    }else if(IsEqualGUID(riid, &IID_IOleUndoManager)) {
        TRACE("(%p)->(IID_IOleUndoManager %p)\n", This, ppv);
        *ppv = UNDOMGR(This);
    }


    FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI OleUndoManager_AddRef(IOleUndoManager *iface)
{
    UndoManager *This = UNDOMGR_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI OleUndoManager_Release(IOleUndoManager *iface)
{
    UndoManager *This = UNDOMGR_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI OleUndoManager_Open(IOleUndoManager *iface, IOleParentUndoUnit *pPUU)
{
    UndoManager *This = UNDOMGR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pPUU);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleUndoManager_Close(IOleUndoManager *iface, IOleParentUndoUnit *pPUU,
        BOOL fCommit)
{
    UndoManager *This = UNDOMGR_THIS(iface);
    FIXME("(%p)->(%p %x)\n", This, pPUU, fCommit);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleUndoManager_Add(IOleUndoManager *iface, IOleUndoUnit *pUU)
{
    UndoManager *This = UNDOMGR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pUU);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleUndoManager_GetOpenParentState(IOleUndoManager *iface, DWORD *pdwState)
{
    UndoManager *This = UNDOMGR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pdwState);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleUndoManager_DiscardFrom(IOleUndoManager *iface, IOleUndoUnit *pUU)
{
    UndoManager *This = UNDOMGR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pUU);
    return S_OK;
}

static HRESULT WINAPI OleUndoManager_UndoTo(IOleUndoManager *iface, IOleUndoUnit *pUU)
{
    UndoManager *This = UNDOMGR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pUU);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleUndoManager_RedoTo(IOleUndoManager *iface, IOleUndoUnit *pUU)
{
    UndoManager *This = UNDOMGR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pUU);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleUndoManager_EnumUndoable(IOleUndoManager *iface,
        IEnumOleUndoUnits **ppEnum)
{
    UndoManager *This = UNDOMGR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppEnum);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleUndoManager_EnumRedoable(IOleUndoManager *iface,
        IEnumOleUndoUnits **ppEnum)
{
    UndoManager *This = UNDOMGR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppEnum);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleUndoManager_GetLastUndoDescription(IOleUndoManager *iface, BSTR *pBstr)
{
    UndoManager *This = UNDOMGR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pBstr);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleUndoManager_GetLastRedoDescription(IOleUndoManager *iface, BSTR *pBstr)
{
    UndoManager *This = UNDOMGR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pBstr);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleUndoManager_Enable(IOleUndoManager *iface, BOOL fEnable)
{
    UndoManager *This = UNDOMGR_THIS(iface);
    FIXME("(%p)->(%x)\n", This, fEnable);
    return E_NOTIMPL;
}

#undef UNDOMGR_THIS

static const IOleUndoManagerVtbl OleUndoManagerVtbl = {
    OleUndoManager_QueryInterface,
    OleUndoManager_AddRef,
    OleUndoManager_Release,
    OleUndoManager_Open,
    OleUndoManager_Close,
    OleUndoManager_Add,
    OleUndoManager_GetOpenParentState,
    OleUndoManager_DiscardFrom,
    OleUndoManager_UndoTo,
    OleUndoManager_RedoTo,
    OleUndoManager_EnumUndoable,
    OleUndoManager_EnumRedoable,
    OleUndoManager_GetLastUndoDescription,
    OleUndoManager_GetLastRedoDescription,
    OleUndoManager_Enable
};

static IOleUndoManager *create_undomgr(void)
{
    UndoManager *ret = heap_alloc(sizeof(UndoManager));

    ret->lpOleUndoManagerVtbl = &OleUndoManagerVtbl;
    ret->ref = 1;

    return UNDOMGR(ret);
}

/**********************************************************
 * IServiceProvider implementation
 */

#define SERVPROV_THIS(iface) DEFINE_THIS(HTMLDocument, ServiceProvider, iface)

static HRESULT WINAPI ServiceProvider_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    HTMLDocument *This = SERVPROV_THIS(iface);
    return IHTMLDocument2_QueryInterface(HTMLDOC(This), riid, ppv);
}

static ULONG WINAPI ServiceProvider_AddRef(IServiceProvider *iface)
{
    HTMLDocument *This = SERVPROV_THIS(iface);
    return IHTMLDocument2_AddRef(HTMLDOC(This));
}

static ULONG WINAPI ServiceProvider_Release(IServiceProvider *iface)
{
    HTMLDocument *This = SERVPROV_THIS(iface);
    return IHTMLDocument_Release(HTMLDOC(This));
}

static HRESULT WINAPI ServiceProvider_QueryService(IServiceProvider *iface, REFGUID guidService,
        REFIID riid, void **ppv)
{
    HTMLDocument *This = SERVPROV_THIS(iface);
    
    if(IsEqualGUID(&CLSID_CMarkup, guidService)) {
        FIXME("(%p)->(CLSID_CMarkup %s %p)\n", This, debugstr_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    if(IsEqualGUID(&IID_IOleUndoManager, riid)) {
        TRACE("(%p)->(IID_IOleUndoManager %p)\n", This, ppv);

        if(!This->undomgr)
            This->undomgr = create_undomgr();

        IOleUndoManager_AddRef(This->undomgr);
        *ppv = This->undomgr;
        return S_OK;
    }

    FIXME("(%p)->(%s %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);
    
    return E_UNEXPECTED;
}

static const IServiceProviderVtbl ServiceProviderVtbl = {
    ServiceProvider_QueryInterface,
    ServiceProvider_AddRef,
    ServiceProvider_Release,
    ServiceProvider_QueryService
};

void HTMLDocument_Service_Init(HTMLDocument *This)
{
    This->lpServiceProviderVtbl = &ServiceProviderVtbl;

    This->undomgr = NULL;
}
