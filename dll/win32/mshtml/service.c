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

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "wine/debug.h"

#include "mshtml_private.h"
#include "htmlscript.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

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

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI OleUndoManager_Release(IOleUndoManager *iface)
{
    UndoManager *This = impl_from_IOleUndoManager(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref)
        free(This);

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
    UndoManager *ret = malloc(sizeof(UndoManager));

    if (!ret) return NULL;

    ret->IOleUndoManager_iface.lpVtbl = &OleUndoManagerVtbl;
    ret->ref = 1;

    return &ret->IOleUndoManager_iface;
}

typedef struct {
    IHTMLEditServices IHTMLEditServices_iface;
    LONG ref;
} editsvcs;

static inline editsvcs *impl_from_IHTMLEditServices(IHTMLEditServices *iface)
{
    return CONTAINING_RECORD(iface, editsvcs, IHTMLEditServices_iface);
}

static HRESULT WINAPI editsvcs_QueryInterface(IHTMLEditServices *iface, REFIID riid, void **ppv)
{
    editsvcs *This = impl_from_IHTMLEditServices(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        *ppv = &This->IHTMLEditServices_iface;
    } else if(IsEqualGUID(riid, &IID_IHTMLEditServices)) {
        *ppv = &This->IHTMLEditServices_iface;
    } else {
        *ppv = NULL;
        FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI editsvcs_AddRef(IHTMLEditServices *iface)
{
    editsvcs *This = impl_from_IHTMLEditServices(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);
    return ref;
}

static ULONG WINAPI editsvcs_Release(IHTMLEditServices *iface)
{
    editsvcs *This = impl_from_IHTMLEditServices(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref)
        free(This);

    return ref;
}

static HRESULT WINAPI editsvcs_AddDesigner(IHTMLEditServices *iface,
    IHTMLEditDesigner *pIDesigner)
{
    editsvcs *This = impl_from_IHTMLEditServices(iface);
    FIXME("(%p)->(%p)\n", This, pIDesigner);
    return E_NOTIMPL;
}

static HRESULT WINAPI editsvcs_RemoveDesigner(IHTMLEditServices *iface,
    IHTMLEditDesigner *pIDesigner)
{
    editsvcs *This = impl_from_IHTMLEditServices(iface);
    FIXME("(%p)->(%p)\n", This, pIDesigner);
    return E_NOTIMPL;
}

static HRESULT WINAPI editsvcs_GetSelectionServices(IHTMLEditServices *iface,
    IMarkupContainer *pIContainer, ISelectionServices **ppSelSvc)
{
    editsvcs *This = impl_from_IHTMLEditServices(iface);
    FIXME("(%p)->(%p,%p)\n", This, pIContainer, ppSelSvc);
    return E_NOTIMPL;
}

static HRESULT WINAPI editsvcs_MoveToSelectionAnchor(IHTMLEditServices *iface,
    IMarkupPointer *pIStartAnchor)
{
    editsvcs *This = impl_from_IHTMLEditServices(iface);
    FIXME("(%p)->(%p)\n", This, pIStartAnchor);
    return E_NOTIMPL;
}

static HRESULT WINAPI editsvcs_MoveToSelectionEnd(IHTMLEditServices *iface,
    IMarkupPointer *pIEndAnchor)
{
    editsvcs *This = impl_from_IHTMLEditServices(iface);
    FIXME("(%p)->(%p)\n", This, pIEndAnchor);
    return E_NOTIMPL;
}

static HRESULT WINAPI editsvcs_SelectRange(IHTMLEditServices *iface,
    IMarkupPointer *pStart, IMarkupPointer *pEnd, SELECTION_TYPE eType)
{
    editsvcs *This = impl_from_IHTMLEditServices(iface);
    FIXME("(%p)->(%p,%p,%#x)\n", This, pStart, pEnd, eType);
    return E_NOTIMPL;
}

static const IHTMLEditServicesVtbl editsvcsVtbl = {
    editsvcs_QueryInterface,
    editsvcs_AddRef,
    editsvcs_Release,
    editsvcs_AddDesigner,
    editsvcs_RemoveDesigner,
    editsvcs_GetSelectionServices,
    editsvcs_MoveToSelectionAnchor,
    editsvcs_MoveToSelectionEnd,
    editsvcs_SelectRange,
};

static IHTMLEditServices *create_editsvcs(void)
{
    editsvcs *ret = malloc(sizeof(*ret));

    if (ret) {
        ret->IHTMLEditServices_iface.lpVtbl = &editsvcsVtbl;
        ret->ref = 1;
        return &ret->IHTMLEditServices_iface;
    }

    return NULL;
}

/**********************************************************
 * IServiceProvider implementation
 */

static inline HTMLDocumentNode *HTMLDocumentNode_from_IServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IServiceProvider_iface);
}

static HRESULT WINAPI DocNodeServiceProvider_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IServiceProvider(iface);
    return IHTMLDOMNode_QueryInterface(&This->node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI DocNodeServiceProvider_AddRef(IServiceProvider *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IServiceProvider(iface);
    return IHTMLDOMNode_AddRef(&This->node.IHTMLDOMNode_iface);
}

static ULONG WINAPI DocNodeServiceProvider_Release(IServiceProvider *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IServiceProvider(iface);
    return IHTMLDOMNode_Release(&This->node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI DocNodeServiceProvider_QueryService(IServiceProvider *iface, REFGUID guidService,
        REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IServiceProvider(iface);

    if(IsEqualGUID(&IID_IActiveScriptSite, guidService)) {
        IActiveScriptSite *site;

        TRACE("IID_IActiveScriptSite\n");

        if(!This->window) {
            FIXME("No window\n");
            return E_NOTIMPL;
        }

        if(!(site = get_first_script_site(This->window)))
            return E_OUTOFMEMORY;
        return IActiveScriptSite_QueryInterface(site, riid, ppv);
    }

    if(IsEqualGUID(&SID_SInternetHostSecurityManager, guidService)) {
        TRACE("SID_SInternetHostSecurityManager\n");
        return IInternetHostSecurityManager_QueryInterface(&This->IInternetHostSecurityManager_iface, riid, ppv);
    }

    if(IsEqualGUID(&SID_SContainerDispatch, guidService)) {
        TRACE("SID_SContainerDispatch\n");
        return IHTMLDocument2_QueryInterface(&This->IHTMLDocument2_iface, riid, ppv);
    }

    return IServiceProvider_QueryService(&This->doc_obj->IServiceProvider_iface, guidService, riid, ppv);
}

static const IServiceProviderVtbl DocNodeServiceProviderVtbl = {
    DocNodeServiceProvider_QueryInterface,
    DocNodeServiceProvider_AddRef,
    DocNodeServiceProvider_Release,
    DocNodeServiceProvider_QueryService
};

static inline HTMLDocumentObj *HTMLDocumentObj_from_IServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IServiceProvider_iface);
}

static HRESULT WINAPI DocObjServiceProvider_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IServiceProvider(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI DocObjServiceProvider_AddRef(IServiceProvider *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IServiceProvider(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI DocObjServiceProvider_Release(IServiceProvider *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IServiceProvider(iface);
    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI DocObjServiceProvider_QueryService(IServiceProvider *iface, REFGUID guidService,
        REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IServiceProvider(iface);

    if(IsEqualGUID(&CLSID_CMarkup, guidService)) {
        FIXME("(%p)->(CLSID_CMarkup %s %p)\n", This, debugstr_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    if(IsEqualGUID(&SID_SOleUndoManager, guidService)) {
        TRACE("SID_SOleUndoManager\n");

        if(!This->undomgr)
            This->undomgr = create_undomgr();

        if (!This->undomgr)
            return E_OUTOFMEMORY;

        return IOleUndoManager_QueryInterface(This->undomgr, riid, ppv);
    }

    if(IsEqualGUID(&SID_SInternetHostSecurityManager, guidService)) {
        TRACE("SID_SInternetHostSecurityManager\n");
        return IInternetHostSecurityManager_QueryInterface(&This->doc_node->IInternetHostSecurityManager_iface, riid, ppv);
    }

    if(IsEqualGUID(&SID_SContainerDispatch, guidService)) {
        TRACE("SID_SContainerDispatch\n");
        return IHTMLDocument2_QueryInterface(&This->IHTMLDocument2_iface, riid, ppv);
    }

    if(IsEqualGUID(&IID_IWindowForBindingUI, guidService)) {
        TRACE("IID_IWindowForBindingUI\n");
        return IWindowForBindingUI_QueryInterface(&This->IWindowForBindingUI_iface, riid, ppv);
    }

    if(IsEqualGUID(&SID_SHTMLEditServices, guidService)) {
        TRACE("SID_SHTMLEditServices\n");

        if(!This->editsvcs)
            This->editsvcs = create_editsvcs();

        if (!This->editsvcs)
            return E_OUTOFMEMORY;

        return IHTMLEditServices_QueryInterface(This->editsvcs, riid, ppv);
    }

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);

    if(This->client) {
        HRESULT hres;

        hres = do_query_service((IUnknown*)This->client, guidService, riid, ppv);
        if(SUCCEEDED(hres))
            return hres;
    }

    FIXME("unknown service %s\n", debugstr_guid(guidService));
    return E_NOINTERFACE;
}

static const IServiceProviderVtbl DocObjServiceProviderVtbl = {
    DocObjServiceProvider_QueryInterface,
    DocObjServiceProvider_AddRef,
    DocObjServiceProvider_Release,
    DocObjServiceProvider_QueryService
};

void HTMLDocumentNode_Service_Init(HTMLDocumentNode *This)
{
    This->IServiceProvider_iface.lpVtbl = &DocNodeServiceProviderVtbl;
}

void HTMLDocumentObj_Service_Init(HTMLDocumentObj *This)
{
    This->IServiceProvider_iface.lpVtbl = &DocObjServiceProviderVtbl;
}
