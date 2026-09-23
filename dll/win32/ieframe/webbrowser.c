/*
 * Implementation of IWebBrowser interface for WebBrowser control
 *
 * Copyright 2001 John R. Sheets (for CodeWeavers)
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

#include "ieframe.h"

#include "exdispid.h"
#include "mshtml.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ieframe);

static inline WebBrowser *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, WebBrowser, IUnknown_inner);
}

static HRESULT WINAPI WebBrowser_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    WebBrowser *This = impl_from_IUnknown(iface);

    if (ppv == NULL)
        return E_POINTER;
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IUnknown_inner;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IWebBrowser2_iface;
    }else if(IsEqualGUID(&IID_IWebBrowser, riid)) {
        TRACE("(%p)->(IID_IWebBrowser %p)\n", This, ppv);
        *ppv = &This->IWebBrowser2_iface;
    }else if(IsEqualGUID(&IID_IWebBrowserApp, riid)) {
        TRACE("(%p)->(IID_IWebBrowserApp %p)\n", This, ppv);
        *ppv = &This->IWebBrowser2_iface;
    }else if(IsEqualGUID(&IID_IWebBrowser2, riid)) {
        TRACE("(%p)->(IID_IWebBrowser2 %p)\n", This, ppv);
        *ppv = &This->IWebBrowser2_iface;
    }else if(IsEqualGUID(&IID_IOleObject, riid)) {
        TRACE("(%p)->(IID_IOleObject %p)\n", This, ppv);
        *ppv = &This->IOleObject_iface;
    }else if(IsEqualGUID(&IID_IOleWindow, riid)) {
        TRACE("(%p)->(IID_IOleWindow %p)\n", This, ppv);
        *ppv = &This->IOleInPlaceObject_iface;
    }else if(IsEqualGUID (&IID_IOleInPlaceObject, riid)) {
        TRACE("(%p)->(IID_IOleInPlaceObject %p)\n", This, ppv);
        *ppv = &This->IOleInPlaceObject_iface;
    }else if(IsEqualGUID(&IID_IOleControl, riid)) {
        TRACE("(%p)->(IID_IOleControl %p)\n", This, ppv);
        *ppv = &This->IOleControl_iface;
    }else if(IsEqualGUID(&IID_IPersist, riid)) {
        TRACE("(%p)->(IID_IPersist %p)\n", This, ppv);
        *ppv = &This->IPersistStorage_iface;
    }else if(IsEqualGUID(&IID_IPersistStorage, riid)) {
        TRACE("(%p)->(IID_IPersistStorage %p)\n", This, ppv);
        *ppv = &This->IPersistStorage_iface;
    }else if(IsEqualGUID(&IID_IPersistMemory, riid)) {
        TRACE("(%p)->(IID_IPersistStorage %p)\n", This, ppv);
        *ppv = &This->IPersistMemory_iface;
    }else if(IsEqualGUID (&IID_IPersistStreamInit, riid)) {
        TRACE("(%p)->(IID_IPersistStreamInit %p)\n", This, ppv);
        *ppv = &This->IPersistStreamInit_iface;
    }else if(IsEqualGUID(&IID_IProvideClassInfo, riid)) {
        TRACE("(%p)->(IID_IProvideClassInfo %p)\n", This, ppv);
        *ppv = &This->IProvideClassInfo2_iface;
    }else if(IsEqualGUID(&IID_IProvideClassInfo2, riid)) {
        TRACE("(%p)->(IID_IProvideClassInfo2 %p)\n", This, ppv);
        *ppv = &This->IProvideClassInfo2_iface;
    }else if(IsEqualGUID(&IID_IConnectionPointContainer, riid)) {
        TRACE("(%p)->(IID_IConnectionPointContainer %p)\n", This, ppv);
        *ppv = &This->doc_host.cps.IConnectionPointContainer_iface;
    }else if(IsEqualGUID(&IID_IViewObject, riid)) {
        TRACE("(%p)->(IID_IViewObject %p)\n", This, ppv);
        *ppv = &This->IViewObject2_iface;
    }else if(IsEqualGUID(&IID_IViewObject2, riid)) {
        TRACE("(%p)->(IID_IViewObject2 %p)\n", This, ppv);
        *ppv = &This->IViewObject2_iface;
    }else if(IsEqualGUID(&IID_IOleInPlaceActiveObject, riid)) {
        TRACE("(%p)->(IID_IOleInPlaceActiveObject %p)\n", This, ppv);
        *ppv = &This->IOleInPlaceActiveObject_iface;
    }else if(IsEqualGUID(&IID_IOleCommandTarget, riid)) {
        TRACE("(%p)->(IID_IOleCommandTarget %p)\n", This, ppv);
        *ppv = &This->IOleCommandTarget_iface;
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        *ppv = &This->IServiceProvider_iface;
        TRACE("(%p)->(IID_IServiceProvider %p)\n", This, ppv);
    }else if(IsEqualGUID(&IID_IDataObject, riid)) {
        *ppv = &This->IDataObject_iface;
        TRACE("(%p)->(IID_IDataObject %p)\n", This, ppv);
    }else if(IsEqualGUID(&IID_IQuickActivate, riid)) {
        TRACE("(%p)->(IID_IQuickActivate %p) returning NULL\n", This, ppv);
        return E_NOINTERFACE;
    }else if(IsEqualGUID(&IID_IRunnableObject, riid)) {
        TRACE("(%p)->(IID_IRunnableObject %p) returning NULL\n", This, ppv);
        return E_NOINTERFACE;
    }else if(IsEqualGUID(&IID_IPerPropertyBrowsing, riid)) {
        TRACE("(%p)->(IID_IPerPropertyBrowsing %p) returning NULL\n", This, ppv);
        return E_NOINTERFACE;
    }else if(IsEqualGUID(&IID_IOleCache, riid)) {
        TRACE("(%p)->(IID_IOleCache %p) returning NULL\n", This, ppv);
        return E_NOINTERFACE;
    }else if(IsEqualGUID(&IID_IOleInPlaceSite, riid)) {
        TRACE("(%p)->(IID_IOleInPlaceSite %p) returning NULL\n", This, ppv);
        return E_NOINTERFACE;
    }else if(IsEqualGUID(&IID_IObjectWithSite, riid)) {
        TRACE("(%p)->(IID_IObjectWithSite %p) returning NULL\n", This, ppv);
        return E_NOINTERFACE;
    }else if(IsEqualGUID(&IID_IViewObjectEx, riid)) {
        TRACE("(%p)->(IID_IViewObjectEx %p) returning NULL\n", This, ppv);
        return E_NOINTERFACE;
    }else if(IsEqualGUID(&IID_IOleLink, riid)) {
        TRACE("(%p)->(IID_IOleLink %p) returning NULL\n", This, ppv);
        return E_NOINTERFACE;
    }else if(IsEqualGUID(&IID_IMarshal, riid)) {
        TRACE("(%p)->(IID_IMarshal %p) returning NULL\n", This, ppv);
        return E_NOINTERFACE;
    }else if(IsEqualGUID(&IID_IStdMarshalInfo, riid)) {
        /* This is implemented since IE10 */
        WARN("(%p)->(IID_IStdMarshalInfo %p) returning NULL\n", This, ppv);
        return E_NOINTERFACE;
    }else if(HlinkFrame_QI(&This->hlink_frame, riid, ppv)) {
        return S_OK;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    FIXME("(%p)->(%s %p) interface not supported\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI WebBrowser_AddRef(IUnknown *iface)
{
    WebBrowser *This = impl_from_IUnknown(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%ld\n", This, ref);
    return ref;
}

static ULONG WINAPI WebBrowser_Release(IUnknown *iface)
{
    WebBrowser *This = impl_from_IUnknown(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        if(This->sink)
            IAdviseSink_Release(This->sink);

        if(This->advise_holder)
            IOleAdviseHolder_Release(This->advise_holder);

        if(This->doc_host.document)
            IUnknown_Release(This->doc_host.document);

        DocHost_Release(&This->doc_host);

        WebBrowser_OleObject_Destroy(This);

        free(This);
        unlock_module();
    }

    return ref;
}

static const struct IUnknownVtbl internal_unk_vtbl =
{
    WebBrowser_QueryInterface,
    WebBrowser_AddRef,
    WebBrowser_Release
};

static inline WebBrowser *impl_from_IWebBrowser2(IWebBrowser2 *iface)
{
    return CONTAINING_RECORD(iface, WebBrowser, IWebBrowser2_iface);
}

static HRESULT WINAPI WebBrowser2_QueryInterface(IWebBrowser2 *iface, REFIID riid, LPVOID *ppv)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    return IUnknown_QueryInterface(This->hlink_frame.outer, riid, ppv);
}

static ULONG WINAPI WebBrowser2_AddRef(IWebBrowser2 *iface)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    return IUnknown_AddRef(This->hlink_frame.outer);
}

static ULONG WINAPI WebBrowser2_Release(IWebBrowser2 *iface)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    return IUnknown_Release(This->hlink_frame.outer);
}

/* IDispatch methods */
static HRESULT WINAPI WebBrowser_GetTypeInfoCount(IWebBrowser2 *iface, UINT *pctinfo)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI WebBrowser_GetTypeInfo(IWebBrowser2 *iface, UINT iTInfo, LCID lcid,
                                     LPTYPEINFO *ppTInfo)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    ITypeInfo *typeinfo;
    HRESULT hres;

    TRACE("(%p)->(%d %ld %p)\n", This, iTInfo, lcid, ppTInfo);

    hres = get_typeinfo(IWebBrowser2_tid, &typeinfo);
    if(FAILED(hres))
        return hres;

    ITypeInfo_AddRef(typeinfo);
    *ppTInfo = typeinfo;
    return S_OK;
}

static HRESULT WINAPI WebBrowser_GetIDsOfNames(IWebBrowser2 *iface, REFIID riid,
                                       LPOLESTR *rgszNames, UINT cNames,
                                       LCID lcid, DISPID *rgDispId)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    ITypeInfo *typeinfo;
    HRESULT hres;

    TRACE("(%p)->(%s %p %d %ld %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    hres = get_typeinfo(IWebBrowser2_tid, &typeinfo);
    if(FAILED(hres))
        return hres;

    return ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
}

static HRESULT WINAPI WebBrowser_Invoke(IWebBrowser2 *iface, DISPID dispIdMember,
                                REFIID riid, LCID lcid, WORD wFlags,
                                DISPPARAMS *pDispParams, VARIANT *pVarResult,
                                EXCEPINFO *pExepInfo, UINT *puArgErr)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    ITypeInfo *typeinfo;
    HRESULT hres;

    TRACE("(%p)->(%ld %s %ld %08x %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    hres = get_typeinfo(IWebBrowser2_tid, &typeinfo);
    if(FAILED(hres))
        return hres;

    return ITypeInfo_Invoke(typeinfo, &This->IWebBrowser2_iface, dispIdMember, wFlags, pDispParams,
            pVarResult, pExepInfo, puArgErr);
}

/* IWebBrowser methods */
static HRESULT WINAPI WebBrowser_GoBack(IWebBrowser2 *iface)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    TRACE("(%p)\n", This);
    return go_back(&This->doc_host);
}

static HRESULT WINAPI WebBrowser_GoForward(IWebBrowser2 *iface)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    TRACE("(%p)\n", This);
    return go_forward(&This->doc_host);
}

static HRESULT WINAPI WebBrowser_GoHome(IWebBrowser2 *iface)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    TRACE("(%p)\n", This);
    return go_home(&This->doc_host);
}

static HRESULT WINAPI WebBrowser_GoSearch(IWebBrowser2 *iface)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_Navigate(IWebBrowser2 *iface, BSTR szUrl,
                                  VARIANT *Flags, VARIANT *TargetFrameName,
                                  VARIANT *PostData, VARIANT *Headers)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%s %s %s %s %s)\n", This, debugstr_w(szUrl), debugstr_variant(Flags),
          debugstr_variant(TargetFrameName), debugstr_variant(PostData),
          debugstr_variant(Headers));

    return navigate_url(&This->doc_host, szUrl, Flags, TargetFrameName, PostData, Headers);
}

static HRESULT WINAPI WebBrowser_Refresh(IWebBrowser2 *iface)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)\n", This);

    return refresh_document(&This->doc_host, NULL);
}

static HRESULT WINAPI WebBrowser_Refresh2(IWebBrowser2 *iface, VARIANT *Level)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(Level));

    return refresh_document(&This->doc_host, Level);
}

static HRESULT WINAPI WebBrowser_Stop(IWebBrowser2 *iface)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)\n", This);
    return S_OK;
}

static HRESULT WINAPI WebBrowser_get_Application(IWebBrowser2 *iface, IDispatch **ppDisp)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%p)\n", This, ppDisp);

    if(!ppDisp)
        return E_POINTER;

    *ppDisp = (IDispatch*)&This->IWebBrowser2_iface;
    IDispatch_AddRef(*ppDisp);
    return S_OK;
}

static HRESULT WINAPI WebBrowser_get_Parent(IWebBrowser2 *iface, IDispatch **ppDisp)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, ppDisp);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Container(IWebBrowser2 *iface, IDispatch **ppDisp)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, ppDisp);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Document(IWebBrowser2 *iface, IDispatch **ppDisp)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    IDispatch *disp = NULL;

    TRACE("(%p)->(%p)\n", This, ppDisp);

    if(This->doc_host.document) {
        HRESULT hres;

        hres = IUnknown_QueryInterface(This->doc_host.document, &IID_IDispatch, (void**)&disp);
        if(SUCCEEDED(hres)) {
            IDispatch *html_doc;

            /* Some broken apps cast returned IDispatch to IHTMLDocument2
             * without QueryInterface call */
            hres = IDispatch_QueryInterface(disp, &IID_IHTMLDocument2, (void**)&html_doc);
            if(SUCCEEDED(hres)) {
                IDispatch_Release(disp);
                disp = html_doc;
            }
        }
    }

    *ppDisp = disp;
    return disp ? S_OK : S_FALSE;
}

static HRESULT WINAPI WebBrowser_get_TopLevelContainer(IWebBrowser2 *iface, VARIANT_BOOL *pBool)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pBool);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Type(IWebBrowser2 *iface, BSTR *Type)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, Type);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Left(IWebBrowser2 *iface, LONG *pl)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%p)\n", This, pl);

    *pl = This->pos_rect.left;
    return S_OK;
}

static HRESULT WINAPI WebBrowser_put_Left(IWebBrowser2 *iface, LONG Left)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    RECT rect;

    TRACE("(%p)->(%ld)\n", This, Left);

    if(!This->inplace)
        return E_UNEXPECTED;

    rect = This->pos_rect;
    rect.left = Left;

    /* We don't really change the window position here.
     * We just notify the embedder that he should do so. */
    return IOleInPlaceSiteEx_OnPosRectChange(This->inplace, &rect);
}

static HRESULT WINAPI WebBrowser_get_Top(IWebBrowser2 *iface, LONG *pl)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%p)\n", This, pl);

    *pl = This->pos_rect.top;
    return S_OK;
}

static HRESULT WINAPI WebBrowser_put_Top(IWebBrowser2 *iface, LONG Top)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    RECT rect;

    TRACE("(%p)->(%ld)\n", This, Top);

    if(!This->inplace)
        return E_UNEXPECTED;

    rect = This->pos_rect;
    rect.top = Top;

    /* We don't really change the window position here.
     * We just notify the embedder that he should do so. */
    return IOleInPlaceSiteEx_OnPosRectChange(This->inplace, &rect);
}

static HRESULT WINAPI WebBrowser_get_Width(IWebBrowser2 *iface, LONG *pl)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%p)\n", This, pl);

    *pl = This->pos_rect.right - This->pos_rect.left;
    return S_OK;
}

static HRESULT WINAPI WebBrowser_put_Width(IWebBrowser2 *iface, LONG Width)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    RECT rect;

    TRACE("(%p)->(%ld)\n", This, Width);

    if(!This->inplace)
        return E_UNEXPECTED;

    rect = This->pos_rect;
    rect.right = rect.left+Width;

    /* We don't really change the window size here.
     * We just notify the embedder that he should do so. */
   return IOleInPlaceSiteEx_OnPosRectChange(This->inplace, &rect);
}

static HRESULT WINAPI WebBrowser_get_Height(IWebBrowser2 *iface, LONG *pl)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%p)\n", This, pl);

    *pl = This->pos_rect.bottom - This->pos_rect.top;
    return S_OK;
}

static HRESULT WINAPI WebBrowser_put_Height(IWebBrowser2 *iface, LONG Height)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    RECT rect;

    TRACE("(%p)->(%ld)\n", This, Height);

    if(!This->inplace)
        return E_UNEXPECTED;

    rect = This->pos_rect;
    rect.bottom = rect.top+Height;

    /* We don't really change the window size here.
     * We just notify the embedder that he should do so. */
    return IOleInPlaceSiteEx_OnPosRectChange(This->inplace, &rect);
}

static HRESULT WINAPI WebBrowser_get_LocationName(IWebBrowser2 *iface, BSTR *LocationName)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    IHTMLDocument2 *doc;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, LocationName);

    if(This->doc_host.document &&
            SUCCEEDED(IUnknown_QueryInterface(This->doc_host.document, &IID_IHTMLDocument2, (void **)&doc))) {
        hres = IHTMLDocument2_get_title(doc, LocationName);
        if(hres == S_OK && !SysStringLen(*LocationName)) {
            SysFreeString(*LocationName);
            hres = get_location_url(&This->doc_host, LocationName);
        }
        IHTMLDocument2_Release(doc);
    } else {
        *LocationName = SysAllocString(L"");
        hres = S_FALSE;
    }

    return hres;
}

static HRESULT WINAPI WebBrowser_get_LocationURL(IWebBrowser2 *iface, BSTR *LocationURL)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%p)\n", This, LocationURL);

    return get_location_url(&This->doc_host, LocationURL);
}

static HRESULT WINAPI WebBrowser_get_Busy(IWebBrowser2 *iface, VARIANT_BOOL *pBool)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%p)\n", This, pBool);

    *pBool = This->doc_host.busy;
    return S_OK;
}

static HRESULT WINAPI WebBrowser_Quit(IWebBrowser2 *iface)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)\n", This);

    /* It's a InternetExplorer specific method, we have nothing to do here. */
    return E_FAIL;
}

static HRESULT WINAPI WebBrowser_ClientToWindow(IWebBrowser2 *iface, int *pcx, int *pcy)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p %p)\n", This, pcx, pcy);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_PutProperty(IWebBrowser2 *iface, BSTR szProperty, VARIANT vtValue)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_w(szProperty), debugstr_variant(&vtValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_GetProperty(IWebBrowser2 *iface, BSTR szProperty, VARIANT *pvtValue)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_w(szProperty), debugstr_variant(pvtValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Name(IWebBrowser2 *iface, BSTR *Name)
{
    static const WCHAR sName[] = {'M','i','c','r','o','s','o','f','t',' ','W','e','b',
                                  ' ','B','r','o','w','s','e','r',' ','C','o','n','t','r','o','l',0};
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%p)\n", This, Name);

    *Name = SysAllocString(sName);

    return S_OK;
}

static HRESULT WINAPI WebBrowser_get_HWND(IWebBrowser2 *iface, SHANDLE_PTR *pHWND)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%p)\n", This, pHWND);

    /* WebBrowser control never has a frame window (in opposition to InternetExplorer) */
    *pHWND = 0;
    return E_FAIL;
}

static HRESULT WINAPI WebBrowser_get_FullName(IWebBrowser2 *iface, BSTR *FullName)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, FullName);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Path(IWebBrowser2 *iface, BSTR *Path)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, Path);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Visible(IWebBrowser2 *iface, VARIANT_BOOL *pBool)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%p)\n", This, pBool);

    *pBool = This->visible;
    return S_OK;
}

static HRESULT WINAPI WebBrowser_put_Visible(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    VARIANTARG arg;
    DISPPARAMS dispparams = {&arg, NULL, 1, 0};

    TRACE("(%p)->(%x)\n", This, Value);

    This->visible = Value;

    V_VT(&arg) = VT_BOOL;
    V_BOOL(&arg) = Value;
    call_sink(This->doc_host.cps.wbe2, DISPID_ONVISIBLE, &dispparams);

    return S_OK;
}

static HRESULT WINAPI WebBrowser_get_StatusBar(IWebBrowser2 *iface, VARIANT_BOOL *pBool)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%p)\n", This, pBool);

    *pBool = This->status_bar;
    return S_OK;
}

static HRESULT WINAPI WebBrowser_put_StatusBar(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    VARIANTARG arg;
    DISPPARAMS dispparams = {&arg, NULL, 1, 0};

    TRACE("(%p)->(%x)\n", This, Value);

    This->status_bar = Value ? VARIANT_TRUE : VARIANT_FALSE;

    /* In opposition to InternetExplorer, all we should do here is
     * inform the embedder about the status bar change. */

    V_VT(&arg) = VT_BOOL;
    V_BOOL(&arg) = Value;
    call_sink(This->doc_host.cps.wbe2, DISPID_ONSTATUSBAR, &dispparams);

    return S_OK;
}

static HRESULT WINAPI WebBrowser_get_StatusText(IWebBrowser2 *iface, BSTR *StatusText)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, StatusText);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_StatusText(IWebBrowser2 *iface, BSTR StatusText)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(StatusText));
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_ToolBar(IWebBrowser2 *iface, int *Value)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%p)\n", This, Value);

    *Value = This->tool_bar;
    return S_OK;
}

static HRESULT WINAPI WebBrowser_put_ToolBar(IWebBrowser2 *iface, int Value)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    VARIANTARG arg;
    DISPPARAMS dispparams = {&arg, NULL, 1, 0};

    TRACE("(%p)->(%x)\n", This, Value);

    This->tool_bar = Value ? VARIANT_TRUE : VARIANT_FALSE;

    /* In opposition to InternetExplorer, all we should do here is
     * inform the embedder about the tool bar change. */

    V_VT(&arg) = VT_BOOL;
    V_BOOL(&arg) = This->tool_bar;
    call_sink(This->doc_host.cps.wbe2, DISPID_ONTOOLBAR, &dispparams);

    return S_OK;
}

static HRESULT WINAPI WebBrowser_get_MenuBar(IWebBrowser2 *iface, VARIANT_BOOL *Value)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%p)\n", This, Value);

    *Value = This->menu_bar;
    return S_OK;
}

static HRESULT WINAPI WebBrowser_put_MenuBar(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    VARIANTARG arg;
    DISPPARAMS dispparams = {&arg, NULL, 1, 0};

    TRACE("(%p)->(%x)\n", This, Value);

    This->menu_bar = Value ? VARIANT_TRUE : VARIANT_FALSE;

    /* In opposition to InternetExplorer, all we should do here is
     * inform the embedder about the menu bar change. */

    V_VT(&arg) = VT_BOOL;
    V_BOOL(&arg) = Value;
    call_sink(This->doc_host.cps.wbe2, DISPID_ONMENUBAR, &dispparams);

    return S_OK;
}

static HRESULT WINAPI WebBrowser_get_FullScreen(IWebBrowser2 *iface, VARIANT_BOOL *pbFullScreen)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%p)\n", This, pbFullScreen);

    *pbFullScreen = This->full_screen;
    return S_OK;
}

static HRESULT WINAPI WebBrowser_put_FullScreen(IWebBrowser2 *iface, VARIANT_BOOL bFullScreen)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    VARIANTARG arg;
    DISPPARAMS dispparams = {&arg, NULL, 1, 0};

    /* In opposition to InternetExplorer, all we should do here is
     * inform the embedder about the fullscreen change. */

    TRACE("(%p)->(%x)\n", This, bFullScreen);

    This->full_screen = bFullScreen ? VARIANT_TRUE : VARIANT_FALSE;

    V_VT(&arg) = VT_BOOL;
    V_BOOL(&arg) = bFullScreen;
    call_sink(This->doc_host.cps.wbe2, DISPID_ONFULLSCREEN, &dispparams);

    return S_OK;
}

static HRESULT WINAPI WebBrowser_Navigate2(IWebBrowser2 *iface, VARIANT *URL, VARIANT *Flags,
        VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    LPCWSTR url;

    TRACE("(%p)->(%s %s %s %s %s)\n", This, debugstr_variant(URL), debugstr_variant(Flags),
          debugstr_variant(TargetFrameName), debugstr_variant(PostData), debugstr_variant(Headers));

    if(!This->client)
        return E_FAIL;

    if(!URL)
        return S_OK;

    switch (V_VT(URL))
    {
    case VT_BSTR:
        url = V_BSTR(URL);
        break;
    case VT_BSTR|VT_BYREF:
        url = *V_BSTRREF(URL);
        break;
    default:
        FIXME("Unsupported V_VT(URL) %d\n", V_VT(URL));
        return E_INVALIDARG;
    }

    return navigate_url(&This->doc_host, url, Flags, TargetFrameName, PostData, Headers);
}

static HRESULT WINAPI WebBrowser_QueryStatusWB(IWebBrowser2 *iface, OLECMDID cmdID, OLECMDF *pcmdf)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    IOleCommandTarget *target = NULL;
    OLECMD ole_command[1];
    HRESULT hres;

    TRACE("(%p)->(%d %p)\n", This, cmdID, pcmdf);

    if (!pcmdf)
        return E_POINTER;
    ole_command[0].cmdID = cmdID;
    ole_command[0].cmdf = *pcmdf;

    if (This->container)
    {
        hres = IOleContainer_QueryInterface(This->container, &IID_IOleCommandTarget, (LPVOID*)&target);
        if(FAILED(hres))
            target = NULL;
    }
    if (!target && This->doc_host.document)
    {
        hres = IUnknown_QueryInterface(This->doc_host.document, &IID_IOleCommandTarget, (LPVOID*)&target);
        if(FAILED(hres))
            target = NULL;
    }

    if (!target)
        return E_UNEXPECTED;

    hres = IOleCommandTarget_QueryStatus(target, NULL, 1, ole_command, NULL);
    if (SUCCEEDED(hres))
        *pcmdf = ole_command[0].cmdf;
    if (hres == OLECMDERR_E_NOTSUPPORTED)
    {
        *pcmdf = 0;
        hres = S_OK;
    }
    IOleCommandTarget_Release(target);

    return hres;
}

static HRESULT WINAPI WebBrowser_ExecWB(IWebBrowser2 *iface, OLECMDID cmdID,
        OLECMDEXECOPT cmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    IOleCommandTarget *target = NULL;
    HRESULT hres;

    TRACE("(%p)->(%d %d %s %p)\n", This, cmdID, cmdexecopt, debugstr_variant(pvaIn), pvaOut);

    if(This->container)
    {
        hres = IOleContainer_QueryInterface(This->container, &IID_IOleCommandTarget, (LPVOID*)&target);
        if(FAILED(hres))
            target = NULL;
    }
    if(!target && This->doc_host.document)
    {
        hres = IUnknown_QueryInterface(This->doc_host.document, &IID_IOleCommandTarget, (LPVOID*)&target);
        if(FAILED(hres))
            target = NULL;
    }

    if(!target)
        return E_UNEXPECTED;

    hres = IOleCommandTarget_Exec(target, NULL, cmdID, cmdexecopt, pvaIn, pvaOut);
    IOleCommandTarget_Release(target);

    return hres;
}

static HRESULT WINAPI WebBrowser_ShowBrowserBar(IWebBrowser2 *iface, VARIANT *pvaClsid,
        VARIANT *pvarShow, VARIANT *pvarSize)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%s %s %s)\n", This, debugstr_variant(pvaClsid), debugstr_variant(pvarShow),
          debugstr_variant(pvarSize));
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_ReadyState(IWebBrowser2 *iface, READYSTATE *lpReadyState)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%p)\n", This, lpReadyState);

    *lpReadyState = This->doc_host.ready_state;
    return S_OK;
}

static HRESULT WINAPI WebBrowser_get_Offline(IWebBrowser2 *iface, VARIANT_BOOL *pbOffline)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%p)\n", This, pbOffline);

    *pbOffline = This->doc_host.offline;
    return S_OK;
}

static HRESULT WINAPI WebBrowser_put_Offline(IWebBrowser2 *iface, VARIANT_BOOL bOffline)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%x)\n", This, bOffline);

    This->doc_host.offline = bOffline ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI WebBrowser_get_Silent(IWebBrowser2 *iface, VARIANT_BOOL *pbSilent)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%p)\n", This, pbSilent);

    *pbSilent = This->doc_host.silent;
    return S_OK;
}

static HRESULT WINAPI WebBrowser_put_Silent(IWebBrowser2 *iface, VARIANT_BOOL bSilent)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%x)\n", This, bSilent);

    This->doc_host.silent = bSilent ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI WebBrowser_get_RegisterAsBrowser(IWebBrowser2 *iface,
        VARIANT_BOOL *pbRegister)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    FIXME("(%p)->(%p)\n", This, pbRegister);

    *pbRegister = This->register_browser;
    return S_OK;
}

static HRESULT WINAPI WebBrowser_put_RegisterAsBrowser(IWebBrowser2 *iface,
        VARIANT_BOOL bRegister)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    FIXME("(%p)->(%x)\n", This, bRegister);

    This->register_browser = bRegister ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI WebBrowser_get_RegisterAsDropTarget(IWebBrowser2 *iface,
        VARIANT_BOOL *pbRegister)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    FIXME("(%p)->(%p)\n", This, pbRegister);

    if(!pbRegister)
        return E_INVALIDARG;

    *pbRegister=0;
    return S_OK;
}

static HRESULT WINAPI WebBrowser_put_RegisterAsDropTarget(IWebBrowser2 *iface,
        VARIANT_BOOL bRegister)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%x)\n", This, bRegister);
    return S_OK;
}

static HRESULT WINAPI WebBrowser_get_TheaterMode(IWebBrowser2 *iface, VARIANT_BOOL *pbRegister)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%p)\n", This, pbRegister);

    *pbRegister = This->theater_mode;
    return S_OK;
}

static HRESULT WINAPI WebBrowser_put_TheaterMode(IWebBrowser2 *iface, VARIANT_BOOL bRegister)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    VARIANTARG arg;
    DISPPARAMS dispparams = {&arg, NULL, 1, 0};

    TRACE("(%p)->(%x)\n", This, bRegister);

    This->theater_mode = bRegister ? VARIANT_TRUE : VARIANT_FALSE;

    /* In opposition to InternetExplorer, all we should do here is
     * inform the embedder about the theater mode change. */

    V_VT(&arg) = VT_BOOL;
    V_BOOL(&arg) = bRegister;
    call_sink(This->doc_host.cps.wbe2, DISPID_ONTHEATERMODE, &dispparams);

    return S_OK;
}

static HRESULT WINAPI WebBrowser_get_AddressBar(IWebBrowser2 *iface, VARIANT_BOOL *Value)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%p)\n", This, Value);

    *Value = This->address_bar;
    return S_OK;
}

static HRESULT WINAPI WebBrowser_put_AddressBar(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    VARIANTARG arg;
    DISPPARAMS dispparams = {&arg, NULL, 1, 0};

    TRACE("(%p)->(%x)\n", This, Value);

    This->address_bar = Value ? VARIANT_TRUE : VARIANT_FALSE;

    /* In opposition to InternetExplorer, all we should do here is
     * inform the embedder about the address bar change. */

    V_VT(&arg) = VT_BOOL;
    V_BOOL(&arg) = Value;
    call_sink(This->doc_host.cps.wbe2, DISPID_ONADDRESSBAR, &dispparams);

    return S_OK;
}

static HRESULT WINAPI WebBrowser_get_Resizable(IWebBrowser2 *iface, VARIANT_BOOL *Value)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%p)\n", This, Value);

    /* It's InternetExplorer object's method. We have nothing to do here. */
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_Resizable(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    WebBrowser *This = impl_from_IWebBrowser2(iface);
    VARIANTARG arg;
    DISPPARAMS dispparams = {&arg, NULL, 1, 0};

    TRACE("(%p)->(%x)\n", This, Value);

    /* In opposition to InternetExplorer, all we should do here is
     * inform the embedder about the resizable change. */

    V_VT(&arg) = VT_BOOL;
    V_BOOL(&arg) = Value;
    call_sink(This->doc_host.cps.wbe2, DISPID_WINDOWSETRESIZABLE, &dispparams);

    return S_OK;
}

static const IWebBrowser2Vtbl WebBrowser2Vtbl =
{
    WebBrowser2_QueryInterface,
    WebBrowser2_AddRef,
    WebBrowser2_Release,
    WebBrowser_GetTypeInfoCount,
    WebBrowser_GetTypeInfo,
    WebBrowser_GetIDsOfNames,
    WebBrowser_Invoke,
    WebBrowser_GoBack,
    WebBrowser_GoForward,
    WebBrowser_GoHome,
    WebBrowser_GoSearch,
    WebBrowser_Navigate,
    WebBrowser_Refresh,
    WebBrowser_Refresh2,
    WebBrowser_Stop,
    WebBrowser_get_Application,
    WebBrowser_get_Parent,
    WebBrowser_get_Container,
    WebBrowser_get_Document,
    WebBrowser_get_TopLevelContainer,
    WebBrowser_get_Type,
    WebBrowser_get_Left,
    WebBrowser_put_Left,
    WebBrowser_get_Top,
    WebBrowser_put_Top,
    WebBrowser_get_Width,
    WebBrowser_put_Width,
    WebBrowser_get_Height,
    WebBrowser_put_Height,
    WebBrowser_get_LocationName,
    WebBrowser_get_LocationURL,
    WebBrowser_get_Busy,
    WebBrowser_Quit,
    WebBrowser_ClientToWindow,
    WebBrowser_PutProperty,
    WebBrowser_GetProperty,
    WebBrowser_get_Name,
    WebBrowser_get_HWND,
    WebBrowser_get_FullName,
    WebBrowser_get_Path,
    WebBrowser_get_Visible,
    WebBrowser_put_Visible,
    WebBrowser_get_StatusBar,
    WebBrowser_put_StatusBar,
    WebBrowser_get_StatusText,
    WebBrowser_put_StatusText,
    WebBrowser_get_ToolBar,
    WebBrowser_put_ToolBar,
    WebBrowser_get_MenuBar,
    WebBrowser_put_MenuBar,
    WebBrowser_get_FullScreen,
    WebBrowser_put_FullScreen,
    WebBrowser_Navigate2,
    WebBrowser_QueryStatusWB,
    WebBrowser_ExecWB,
    WebBrowser_ShowBrowserBar,
    WebBrowser_get_ReadyState,
    WebBrowser_get_Offline,
    WebBrowser_put_Offline,
    WebBrowser_get_Silent,
    WebBrowser_put_Silent,
    WebBrowser_get_RegisterAsBrowser,
    WebBrowser_put_RegisterAsBrowser,
    WebBrowser_get_RegisterAsDropTarget,
    WebBrowser_put_RegisterAsDropTarget,
    WebBrowser_get_TheaterMode,
    WebBrowser_put_TheaterMode,
    WebBrowser_get_AddressBar,
    WebBrowser_put_AddressBar,
    WebBrowser_get_Resizable,
    WebBrowser_put_Resizable
};

static inline WebBrowser *impl_from_IServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, WebBrowser, IServiceProvider_iface);
}

static HRESULT WINAPI WBServiceProvider_QueryInterface(IServiceProvider *iface,
            REFIID riid, LPVOID *ppv)
{
    WebBrowser *This = impl_from_IServiceProvider(iface);
    return IUnknown_QueryInterface(This->hlink_frame.outer, riid, ppv);
}

static ULONG WINAPI WBServiceProvider_AddRef(IServiceProvider *iface)
{
    WebBrowser *This = impl_from_IServiceProvider(iface);
    return IUnknown_AddRef(This->hlink_frame.outer);
}

static ULONG WINAPI WBServiceProvider_Release(IServiceProvider *iface)
{
    WebBrowser *This = impl_from_IServiceProvider(iface);
    return IUnknown_Release(This->hlink_frame.outer);
}

static HRESULT STDMETHODCALLTYPE WBServiceProvider_QueryService(IServiceProvider *iface,
            REFGUID guidService, REFIID riid, void **ppv)
{
    WebBrowser *This = impl_from_IServiceProvider(iface);

    if(IsEqualGUID(&SID_SHTMLWindow, riid)) {
        TRACE("(%p)->(SID_SHTMLWindow)\n", This);
        return IHTMLWindow2_QueryInterface(&This->doc_host.html_window.IHTMLWindow2_iface, riid, ppv);
    }

    if(IsEqualGUID(&IID_IBrowserService2, riid)) {
        TRACE("(%p)->(IID_IBrowserService2 return E_FAIL)\n", This);
        *ppv = NULL;
        return E_FAIL;
    }

    FIXME("(%p)->(%s, %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);
    *ppv = NULL;
    return E_NOINTERFACE;
}

static const IServiceProviderVtbl ServiceProviderVtbl =
{
    WBServiceProvider_QueryInterface,
    WBServiceProvider_AddRef,
    WBServiceProvider_Release,
    WBServiceProvider_QueryService
};

static inline WebBrowser *impl_from_DocHost(DocHost *iface)
{
    return CONTAINING_RECORD(iface, WebBrowser, doc_host);
}

static ULONG WebBrowser_addref(DocHost *iface)
{
    WebBrowser *This = impl_from_DocHost(iface);
    return IUnknown_AddRef(This->hlink_frame.outer);
}

static ULONG WebBrowser_release(DocHost *iface)
{
    WebBrowser *This = impl_from_DocHost(iface);
    return IUnknown_Release(This->hlink_frame.outer);
}

static void DocHostContainer_get_docobj_rect(DocHost *This, RECT *rc)
{
    GetClientRect(This->frame_hwnd, rc);
}

static HRESULT DocHostContainer_set_status_text(DocHost *This, const WCHAR *text)
{
    return E_NOTIMPL;
}

static void DocHostContainer_on_command_state_change(DocHost *This, LONG command, BOOL enable)
{
}

static void DocHostContainer_set_url(DocHost *This, const WCHAR *url)
{
}

static const IDocHostContainerVtbl DocHostContainerVtbl = {
    WebBrowser_addref,
    WebBrowser_release,
    DocHostContainer_get_docobj_rect,
    DocHostContainer_set_status_text,
    DocHostContainer_on_command_state_change,
    DocHostContainer_set_url
};

static HRESULT create_webbrowser(int version, IUnknown *outer, REFIID riid, void **ppv)
{
    WebBrowser *ret;
    HRESULT hres;

    TRACE("(%p %s %p) version=%d\n", outer, debugstr_guid(riid), ppv, version);

    if (outer && !IsEqualIID(riid, &IID_IUnknown))
        return CLASS_E_NOAGGREGATION;

    ret = calloc(1, sizeof(WebBrowser));

    ret->IUnknown_inner.lpVtbl = &internal_unk_vtbl;
    ret->IWebBrowser2_iface.lpVtbl = &WebBrowser2Vtbl;
    ret->IServiceProvider_iface.lpVtbl = &ServiceProviderVtbl;
    ret->ref = 1;
    ret->version = version;

    HlinkFrame_Init(&ret->hlink_frame, outer ? outer :  &ret->IUnknown_inner, &ret->doc_host);
    DocHost_Init(&ret->doc_host, &ret->IWebBrowser2_iface, &DocHostContainerVtbl);

    ret->visible = VARIANT_TRUE;
    ret->menu_bar = VARIANT_TRUE;
    ret->address_bar = VARIANT_TRUE;
    ret->status_bar = VARIANT_TRUE;
    ret->tool_bar = VARIANT_TRUE;

    WebBrowser_OleObject_Init(ret);
    WebBrowser_ViewObject_Init(ret);
    WebBrowser_Persist_Init(ret);
    WebBrowser_ClassInfo_Init(ret);

    lock_module();

    hres = IUnknown_QueryInterface(&ret->IUnknown_inner, riid, ppv);

    IUnknown_Release(&ret->IUnknown_inner);
    return hres;
}

HRESULT WINAPI WebBrowserV1_Create(IClassFactory *iface, IUnknown *pOuter, REFIID riid, void **ppv)
{
    return create_webbrowser(1, pOuter, riid, ppv);
}

HRESULT WINAPI WebBrowser_Create(IClassFactory *iface, IUnknown *pOuter, REFIID riid, void **ppv)
{
    return create_webbrowser(2, pOuter, riid, ppv);
}
