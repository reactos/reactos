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

#include "mshtml_private.h"

typedef struct {
    IOleUndoManager IOleUndoManager_iface;

    LONG ref;
} UndoManager;

static inline UndoManager *impl_from_IOleUndoManager(IOleUndoManager *iface)
{
    return CONTAINING_RECORD(iface, UndoManager, IOleUndoManager_iface);
}

static HRESULT WINAPI OleUndoManager_QueryInterface(IOleUndoManager *iface, REFIID riid, void **ppv)
{
    UndoManager *This = impl_from_IOleUndoManager(iface);

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IOleUndoManager_iface;
    }else if(IsEqualGUID(riid, &IID_IOleUndoManager)) {
        TRACE("(%p)->(IID_IOleUndoManager %p)\n", This, ppv);
        *ppv = &This->IOleUndoManager_iface;
    }else {
        *ppv = NULL;
        FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI OleUndoManager_AddRef(IOleUndoManager *iface)
{
    UndoManager *This = impl_from_IOleUndoManager(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI OleUndoManager_Release(IOleUndoManager *iface)
{
    UndoManager *This = impl_from_IOleUndoManager(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI OleUndoManager_Open(IOleUndoManager *iface, IOleParentUndoUnit *pPUU)
{
    UndoManager *This = impl_from_IOleUndoManager(iface);
    FIXME("(%p)->(%p)\n", This, pPUU);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleUndoManager_Close(IOleUndoManager *iface, IOleParentUndoUnit *pPUU,
        BOOL fCommit)
{
    UndoManager *This = impl_from_IOleUndoManager(iface);
    FIXME("(%p)->(%p %x)\n", This, pPUU, fCommit);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleUndoManager_Add(IOleUndoManager *iface, IOleUndoUnit *pUU)
{
    UndoManager *This = impl_from_IOleUndoManager(iface);
    FIXME("(%p)->(%p)\n", This, pUU);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleUndoManager_GetOpenParentState(IOleUndoManager *iface, DWORD *pdwState)
{
    UndoManager *This = impl_from_IOleUndoManager(iface);
    FIXME("(%p)->(%p)\n", This, pdwState);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleUndoManager_DiscardFrom(IOleUndoManager *iface, IOleUndoUnit *pUU)
{
    UndoManager *This = impl_from_IOleUndoManager(iface);
    FIXME("(%p)->(%p)\n", This, pUU);
    return S_OK;
}

static HRESULT WINAPI OleUndoManager_UndoTo(IOleUndoManager *iface, IOleUndoUnit *pUU)
{
    UndoManager *This = impl_from_IOleUndoManager(iface);
    FIXME("(%p)->(%p)\n", This, pUU);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleUndoManager_RedoTo(IOleUndoManager *iface, IOleUndoUnit *pUU)
{
    UndoManager *This = impl_from_IOleUndoManager(iface);
    FIXME("(%p)->(%p)\n", This, pUU);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleUndoManager_EnumUndoable(IOleUndoManager *iface,
        IEnumOleUndoUnits **ppEnum)
{
    UndoManager *This = impl_from_IOleUndoManager(iface);
    FIXME("(%p)->(%p)\n", This, ppEnum);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleUndoManager_EnumRedoable(IOleUndoManager *iface,
        IEnumOleUndoUnits **ppEnum)
{
    UndoManager *This = impl_from_IOleUndoManager(iface);
    FIXME("(%p)->(%p)\n", This, ppEnum);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleUndoManager_GetLastUndoDescription(IOleUndoManager *iface, BSTR *pBstr)
{
    UndoManager *This = impl_from_IOleUndoManager(iface);
    FIXME("(%p)->(%p)\n", This, pBstr);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleUndoManager_GetLastRedoDescription(IOleUndoManager *iface, BSTR *pBstr)
{
    UndoManager *This = impl_from_IOleUndoManager(iface);
    FIXME("(%p)->(%p)\n", This, pBstr);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleUndoManager_Enable(IOleUndoManager *iface, BOOL fEnable)
{
    UndoManager *This = impl_from_IOleUndoManager(iface);
    FIXME("(%p)->(%x)\n", This, fEnable);
    return E_NOTIMPL;
}

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

    ret->IOleUndoManager_iface.lpVtbl = &OleUndoManagerVtbl;
    ret->ref = 1;

    return &ret->IOleUndoManager_iface;
}

/**********************************************************
 * IServiceProvider implementation
 */

static inline HTMLDocument *impl_from_IServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, IServiceProvider_iface);
}

static HRESULT WINAPI ServiceProvider_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    HTMLDocument *This = impl_from_IServiceProvider(iface);
    return htmldoc_query_interface(This, riid, ppv);
}

static ULONG WINAPI ServiceProvider_AddRef(IServiceProvider *iface)
{
    HTMLDocument *This = impl_from_IServiceProvider(iface);
    return htmldoc_addref(This);
}

static ULONG WINAPI ServiceProvider_Release(IServiceProvider *iface)
{
    HTMLDocument *This = impl_from_IServiceProvider(iface);
    return htmldoc_release(This);
}

static HRESULT WINAPI ServiceProvider_QueryService(IServiceProvider *iface, REFGUID guidService,
        REFIID riid, void **ppv)
{
    HTMLDocument *This = impl_from_IServiceProvider(iface);

    if(IsEqualGUID(&CLSID_CMarkup, guidService)) {
        FIXME("(%p)->(CLSID_CMarkup %s %p)\n", This, debugstr_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    if(IsEqualGUID(&SID_SOleUndoManager, guidService)) {
        TRACE("SID_SOleUndoManager\n");

        if(!This->doc_obj->undomgr)
            This->doc_obj->undomgr = create_undomgr();

        return IOleUndoManager_QueryInterface(This->doc_obj->undomgr, riid, ppv);
    }

    if(IsEqualGUID(&SID_SContainerDispatch, guidService)) {
        TRACE("SID_SContainerDispatch\n");
        return IHTMLDocument2_QueryInterface(&This->IHTMLDocument2_iface, riid, ppv);
    }

    if(IsEqualGUID(&IID_IWindowForBindingUI, guidService)) {
        TRACE("IID_IWindowForBindingUI\n");
        return IWindowForBindingUI_QueryInterface(&This->doc_obj->IWindowForBindingUI_iface, riid, ppv);
    }

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);

    if(This->doc_obj->client) {
        HRESULT hres;

        hres = do_query_service((IUnknown*)This->doc_obj->client, guidService, riid, ppv);
        if(SUCCEEDED(hres))
            return hres;
    }

    FIXME("unknown service %s\n", debugstr_guid(guidService));
    return E_NOINTERFACE;
}

static const IServiceProviderVtbl ServiceProviderVtbl = {
    ServiceProvider_QueryInterface,
    ServiceProvider_AddRef,
    ServiceProvider_Release,
    ServiceProvider_QueryService
};

void HTMLDocument_Service_Init(HTMLDocument *This)
{
    This->IServiceProvider_iface.lpVtbl = &ServiceProviderVtbl;
}
