/*
 * WebBrowser Implementation
 *
 * Copyright 2005 James Hawkins
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

#include "hhctrl.h"

#define ICOM_THIS_MULTI(impl,field,iface) impl* const This=(impl*)((char*)(iface) - offsetof(impl,field))

typedef struct IOleClientSiteImpl
{
    const IOleClientSiteVtbl *lpVtbl;
    const IOleInPlaceSiteVtbl *lpvtblOleInPlaceSite;
    const IOleInPlaceFrameVtbl *lpvtblOleInPlaceFrame;
    const IDocHostUIHandlerVtbl *lpvtblDocHostUIHandler;

    /* IOleClientSiteImpl data */
    IOleObject *pBrowserObject;
    LONG ref;

    /* IOleInPlaceFrame data */
    HWND hwndWindow;
} IOleClientSiteImpl;

static HRESULT STDMETHODCALLTYPE Site_QueryInterface(IOleClientSite *iface, REFIID riid, void **ppvObj)
{
    ICOM_THIS_MULTI(IOleClientSiteImpl, lpVtbl, iface);
    *ppvObj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IOleClientSite))
    {
        *ppvObj = This;
    }
    else if (IsEqualIID(riid, &IID_IOleInPlaceSite))
    {
        *ppvObj = &(This->lpvtblOleInPlaceSite);
    }
    else if (IsEqualIID(riid, &IID_IDocHostUIHandler))
    {
        *ppvObj = &(This->lpvtblDocHostUIHandler);
    }
    else
        return E_NOINTERFACE;

    return S_OK;
}

static ULONG STDMETHODCALLTYPE Site_AddRef(IOleClientSite *iface)
{
    ICOM_THIS_MULTI(IOleClientSiteImpl, lpVtbl, iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG STDMETHODCALLTYPE Site_Release(IOleClientSite *iface)
{
    ICOM_THIS_MULTI(IOleClientSiteImpl, lpVtbl, iface);
    LONG refCount = InterlockedDecrement(&This->ref);

    if (refCount)
        return refCount;

    hhctrl_free(This);
    return 0;
}

static HRESULT STDMETHODCALLTYPE Site_SaveObject(IOleClientSite *iface)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Site_GetMoniker(IOleClientSite *iface, DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Site_GetContainer(IOleClientSite *iface, LPOLECONTAINER *ppContainer)
{
    *ppContainer = NULL;

    return E_NOINTERFACE;
}

static HRESULT STDMETHODCALLTYPE Site_ShowObject(IOleClientSite *iface)
{
    return NOERROR;
}

static HRESULT STDMETHODCALLTYPE Site_OnShowWindow(IOleClientSite *iface, BOOL fShow)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Site_RequestNewObjectLayout(IOleClientSite *iface)
{
    return E_NOTIMPL;
}

static const IOleClientSiteVtbl MyIOleClientSiteTable =
{
    Site_QueryInterface,
    Site_AddRef,
    Site_Release,
    Site_SaveObject,
    Site_GetMoniker,
    Site_GetContainer,
    Site_ShowObject,
    Site_OnShowWindow,
    Site_RequestNewObjectLayout
};

static HRESULT STDMETHODCALLTYPE UI_QueryInterface(IDocHostUIHandler *iface, REFIID riid, LPVOID *ppvObj)
{
    ICOM_THIS_MULTI(IOleClientSiteImpl, lpvtblDocHostUIHandler, iface);
    return Site_QueryInterface((IOleClientSite *)This, riid, ppvObj);
}

static ULONG STDMETHODCALLTYPE UI_AddRef(IDocHostUIHandler *iface)
{
    return 1;
}

static ULONG STDMETHODCALLTYPE UI_Release(IDocHostUIHandler * iface)
{
    return 2;
}

static HRESULT STDMETHODCALLTYPE UI_ShowContextMenu(IDocHostUIHandler *iface, DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE UI_GetHostInfo(IDocHostUIHandler *iface, DOCHOSTUIINFO *pInfo)
{
    pInfo->cbSize = sizeof(DOCHOSTUIINFO);
    pInfo->dwFlags = DOCHOSTUIFLAG_NO3DBORDER;
    pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE UI_ShowUI(IDocHostUIHandler *iface, DWORD dwID, IOleInPlaceActiveObject *pActiveObject, IOleCommandTarget *pCommandTarget, IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE UI_HideUI(IDocHostUIHandler *iface)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE UI_UpdateUI(IDocHostUIHandler *iface)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE UI_EnableModeless(IDocHostUIHandler *iface, BOOL fEnable)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE UI_OnDocWindowActivate(IDocHostUIHandler *iface, BOOL fActivate)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE UI_OnFrameWindowActivate(IDocHostUIHandler *iface, BOOL fActivate)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE UI_ResizeBorder(IDocHostUIHandler *iface, LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE UI_TranslateAccelerator(IDocHostUIHandler *iface, LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID)
{
    return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE UI_GetOptionKeyPath(IDocHostUIHandler *iface, LPOLESTR *pchKey, DWORD dw)
{
    return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE UI_GetDropTarget(IDocHostUIHandler *iface, IDropTarget *pDropTarget, IDropTarget **ppDropTarget)
{
    return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE UI_GetExternal(IDocHostUIHandler *iface, IDispatch **ppDispatch)
{
    *ppDispatch = NULL;
    return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE UI_TranslateUrl(IDocHostUIHandler *iface, DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
{
    *ppchURLOut = NULL;
    return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE UI_FilterDataObject(IDocHostUIHandler *iface, IDataObject *pDO, IDataObject **ppDORet)
{
    *ppDORet = NULL;
    return S_FALSE;
}

static const IDocHostUIHandlerVtbl MyIDocHostUIHandlerTable =
{
    UI_QueryInterface,
    UI_AddRef,
    UI_Release,
    UI_ShowContextMenu,
    UI_GetHostInfo,
    UI_ShowUI,
    UI_HideUI,
    UI_UpdateUI,
    UI_EnableModeless,
    UI_OnDocWindowActivate,
    UI_OnFrameWindowActivate,
    UI_ResizeBorder,
    UI_TranslateAccelerator,
    UI_GetOptionKeyPath,
    UI_GetDropTarget,
    UI_GetExternal,
    UI_TranslateUrl,
    UI_FilterDataObject
};

static HRESULT STDMETHODCALLTYPE InPlace_QueryInterface(IOleInPlaceSite *iface, REFIID riid, LPVOID *ppvObj)
{
    ICOM_THIS_MULTI(IOleClientSiteImpl, lpvtblOleInPlaceSite, iface);
    return Site_QueryInterface((IOleClientSite *)This, riid, ppvObj);
}

static ULONG STDMETHODCALLTYPE InPlace_AddRef(IOleInPlaceSite *iface)
{
    return 1;
}

static ULONG STDMETHODCALLTYPE InPlace_Release(IOleInPlaceSite *iface)
{
    return 2;
}

static HRESULT STDMETHODCALLTYPE InPlace_GetWindow(IOleInPlaceSite *iface, HWND *lphwnd)
{
    ICOM_THIS_MULTI(IOleClientSiteImpl, lpvtblOleInPlaceSite, iface);
    *lphwnd = This->hwndWindow;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE InPlace_ContextSensitiveHelp(IOleInPlaceSite *iface, BOOL fEnterMode)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE InPlace_CanInPlaceActivate(IOleInPlaceSite *iface)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE InPlace_OnInPlaceActivate(IOleInPlaceSite *iface)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE InPlace_OnUIActivate(IOleInPlaceSite *iface)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE InPlace_GetWindowContext(IOleInPlaceSite *iface, LPOLEINPLACEFRAME *lplpFrame, LPOLEINPLACEUIWINDOW *lplpDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    ICOM_THIS_MULTI(IOleClientSiteImpl, lpvtblOleInPlaceSite, iface);
    *lplpFrame = (LPOLEINPLACEFRAME)&This->lpvtblOleInPlaceFrame;
    *lplpDoc = NULL;

    lpFrameInfo->fMDIApp = FALSE;
    lpFrameInfo->hwndFrame = This->hwndWindow;
    lpFrameInfo->haccel = NULL;
    lpFrameInfo->cAccelEntries = 0;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE InPlace_Scroll(IOleInPlaceSite *iface, SIZE scrollExtent)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE InPlace_OnUIDeactivate(IOleInPlaceSite *iface, BOOL fUndoable)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE InPlace_OnInPlaceDeactivate(IOleInPlaceSite *iface)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE InPlace_DiscardUndoState(IOleInPlaceSite *iface)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE InPlace_DeactivateAndUndo(IOleInPlaceSite *iface)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE InPlace_OnPosRectChange(IOleInPlaceSite *iface, LPCRECT lprcPosRect)
{
    ICOM_THIS_MULTI(IOleClientSiteImpl, lpvtblOleInPlaceSite, iface);
    IOleInPlaceObject *inplace;

    if (!IOleObject_QueryInterface(This->pBrowserObject, &IID_IOleInPlaceObject, (void **)&inplace))
        IOleInPlaceObject_SetObjectRects(inplace, lprcPosRect, lprcPosRect);

    return S_OK;
}

static const IOleInPlaceSiteVtbl MyIOleInPlaceSiteTable =
{
    InPlace_QueryInterface,
    InPlace_AddRef,
    InPlace_Release,
    InPlace_GetWindow,
    InPlace_ContextSensitiveHelp,
    InPlace_CanInPlaceActivate,
    InPlace_OnInPlaceActivate,
    InPlace_OnUIActivate,
    InPlace_GetWindowContext,
    InPlace_Scroll,
    InPlace_OnUIDeactivate,
    InPlace_OnInPlaceDeactivate,
    InPlace_DiscardUndoState,
    InPlace_DeactivateAndUndo,
    InPlace_OnPosRectChange
};

static HRESULT STDMETHODCALLTYPE Frame_QueryInterface(IOleInPlaceFrame *iface, REFIID riid, LPVOID *ppvObj)
{
    return E_NOTIMPL;
}

static ULONG STDMETHODCALLTYPE Frame_AddRef(IOleInPlaceFrame *iface)
{
    return 1;
}

static ULONG STDMETHODCALLTYPE Frame_Release(IOleInPlaceFrame *iface)
{
    return 2;
}

static HRESULT STDMETHODCALLTYPE Frame_GetWindow(IOleInPlaceFrame *iface, HWND *lphwnd)
{
    ICOM_THIS_MULTI(IOleClientSiteImpl, lpvtblOleInPlaceFrame, iface);
    *lphwnd = This->hwndWindow;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE Frame_ContextSensitiveHelp(IOleInPlaceFrame *iface, BOOL fEnterMode)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Frame_GetBorder(IOleInPlaceFrame *iface, LPRECT lprectBorder)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Frame_RequestBorderSpace(IOleInPlaceFrame *iface, LPCBORDERWIDTHS pborderwidths)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Frame_SetBorderSpace(IOleInPlaceFrame *iface, LPCBORDERWIDTHS pborderwidths)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Frame_SetActiveObject(IOleInPlaceFrame *iface, IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE Frame_InsertMenus(IOleInPlaceFrame *iface, HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Frame_SetMenu(IOleInPlaceFrame *iface, HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE Frame_RemoveMenus(IOleInPlaceFrame *iface, HMENU hmenuShared)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Frame_SetStatusText(IOleInPlaceFrame *iface, LPCOLESTR pszStatusText)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE Frame_EnableModeless(IOleInPlaceFrame *iface, BOOL fEnable)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE Frame_TranslateAccelerator(IOleInPlaceFrame *iface, LPMSG lpmsg, WORD wID)
{
    return E_NOTIMPL;
}

static const IOleInPlaceFrameVtbl MyIOleInPlaceFrameTable =
{
    Frame_QueryInterface,
    Frame_AddRef,
    Frame_Release,
    Frame_GetWindow,
    Frame_ContextSensitiveHelp,
    Frame_GetBorder,
    Frame_RequestBorderSpace,
    Frame_SetBorderSpace,
    Frame_SetActiveObject,
    Frame_InsertMenus,
    Frame_SetMenu,
    Frame_RemoveMenus,
    Frame_SetStatusText,
    Frame_EnableModeless,
    Frame_TranslateAccelerator
};

static HRESULT STDMETHODCALLTYPE Storage_QueryInterface(IStorage *This, REFIID riid, LPVOID *ppvObj)
{
    return E_NOTIMPL;
}

static ULONG STDMETHODCALLTYPE Storage_AddRef(IStorage *This)
{
    return 1;
}

static ULONG STDMETHODCALLTYPE Storage_Release(IStorage *This)
{
    return 2;
}

static HRESULT STDMETHODCALLTYPE Storage_CreateStream(IStorage *This, const WCHAR *pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStream **ppstm)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Storage_OpenStream(IStorage *This, const WCHAR * pwcsName, void *reserved1, DWORD grfMode, DWORD reserved2, IStream **ppstm)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Storage_CreateStorage(IStorage *This, const WCHAR *pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStorage **ppstg)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Storage_OpenStorage(IStorage *This, const WCHAR * pwcsName, IStorage * pstgPriority, DWORD grfMode, SNB snbExclude, DWORD reserved, IStorage **ppstg)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Storage_CopyTo(IStorage *This, DWORD ciidExclude, IID const *rgiidExclude, SNB snbExclude,IStorage *pstgDest)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Storage_MoveElementTo(IStorage *This, const OLECHAR *pwcsName,IStorage * pstgDest, const OLECHAR *pwcsNewName, DWORD grfFlags)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Storage_Commit(IStorage *This, DWORD grfCommitFlags)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Storage_Revert(IStorage *This)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Storage_EnumElements(IStorage *This, DWORD reserved1, void *reserved2, DWORD reserved3, IEnumSTATSTG **ppenum)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Storage_DestroyElement(IStorage *This, const OLECHAR *pwcsName)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Storage_RenameElement(IStorage *This, const WCHAR *pwcsOldName, const WCHAR *pwcsNewName)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Storage_SetElementTimes(IStorage *This, const WCHAR *pwcsName, FILETIME const *pctime, FILETIME const *patime, FILETIME const *pmtime)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Storage_SetClass(IStorage *This, REFCLSID clsid)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE Storage_SetStateBits(IStorage *This, DWORD grfStateBits, DWORD grfMask)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Storage_Stat(IStorage *This, STATSTG *pstatstg, DWORD grfStatFlag)
{
    return E_NOTIMPL;
}

static const IStorageVtbl MyIStorageTable =
{
    Storage_QueryInterface,
    Storage_AddRef,
    Storage_Release,
    Storage_CreateStream,
    Storage_OpenStream,
    Storage_CreateStorage,
    Storage_OpenStorage,
    Storage_CopyTo,
    Storage_MoveElementTo,
    Storage_Commit,
    Storage_Revert,
    Storage_EnumElements,
    Storage_DestroyElement,
    Storage_RenameElement,
    Storage_SetElementTimes,
    Storage_SetClass,
    Storage_SetStateBits,
    Storage_Stat
};

static IStorage MyIStorage = { &MyIStorageTable };

BOOL InitWebBrowser(HHInfo *info, HWND hwndParent)
{
    IOleClientSiteImpl *iOleClientSiteImpl;
    IOleInPlaceObject *inplace;
    IOleObject *browserObject;
    IWebBrowser2 *webBrowser2;
    HRESULT hr;
    RECT rc;

    iOleClientSiteImpl = hhctrl_alloc_zero(sizeof(IOleClientSiteImpl));
    if (!iOleClientSiteImpl)
        return FALSE;

    iOleClientSiteImpl->ref = 1;
    iOleClientSiteImpl->lpVtbl = &MyIOleClientSiteTable;
    iOleClientSiteImpl->lpvtblOleInPlaceSite = &MyIOleInPlaceSiteTable;
    iOleClientSiteImpl->lpvtblOleInPlaceFrame = &MyIOleInPlaceFrameTable;
    iOleClientSiteImpl->hwndWindow = hwndParent;
    iOleClientSiteImpl->lpvtblDocHostUIHandler = &MyIDocHostUIHandlerTable;

    hr = OleCreate(&CLSID_WebBrowser, &IID_IOleObject, OLERENDER_DRAW, 0,
                   (IOleClientSite *)iOleClientSiteImpl, &MyIStorage,
                   (void **)&browserObject);

    info->client_site = (IOleClientSite *)iOleClientSiteImpl;
    info->wb_object = browserObject;

    if (FAILED(hr)) goto error;

    /* make the browser object accessible to the IOleClientSite implementation */
    iOleClientSiteImpl->pBrowserObject = browserObject;

    GetClientRect(hwndParent, &rc);

    hr = OleSetContainedObject((struct IUnknown *)browserObject, TRUE);
    if (FAILED(hr)) goto error;

    hr = IOleObject_DoVerb(browserObject, OLEIVERB_SHOW, NULL,
                           (IOleClientSite *)iOleClientSiteImpl,
                           -1, hwndParent, &rc);
    if (FAILED(hr)) goto error;

    hr = IOleObject_QueryInterface(browserObject, &IID_IOleInPlaceObject, (void**)&inplace);
    if (FAILED(hr)) goto error;

    IOleInPlaceObject_SetObjectRects(inplace, &rc, &rc);
    IOleInPlaceObject_Release(inplace);

    hr = IOleObject_QueryInterface(browserObject, &IID_IWebBrowser2,
                                   (void **)&webBrowser2);
    if (SUCCEEDED(hr))
    {
        info->web_browser = webBrowser2;
        return TRUE;
    }

error:
    ReleaseWebBrowser(info);
    hhctrl_free(iOleClientSiteImpl);

    return FALSE;
}

void ReleaseWebBrowser(HHInfo *info)
{
    HRESULT hres;

    if (info->web_browser)
    {
        IWebBrowser2_Release(info->web_browser);
        info->web_browser = NULL;
    }

    if (info->client_site)
    {
        IOleClientSite_Release(info->client_site);
        info->client_site = NULL;
    }

    if(info->wb_object) {
        IOleInPlaceSite *inplace;

        hres = IOleObject_QueryInterface(info->wb_object, &IID_IOleInPlaceSite, (void**)&inplace);
        if(SUCCEEDED(hres)) {
            IOleInPlaceSite_OnInPlaceDeactivate(inplace);
            IOleInPlaceSite_Release(inplace);
        }

        IOleObject_SetClientSite(info->wb_object, NULL);

        IOleObject_Release(info->wb_object);
        info->wb_object = NULL;
    }
}

void ResizeWebBrowser(HHInfo *info, DWORD dwWidth, DWORD dwHeight)
{
    if (!info->web_browser)
        return;

    IWebBrowser2_put_Width(info->web_browser, dwWidth);
    IWebBrowser2_put_Height(info->web_browser, dwHeight);
}

void DoPageAction(HHInfo *info, DWORD dwAction)
{
    IWebBrowser2 *pWebBrowser2 = info->web_browser;

    if (!pWebBrowser2)
        return;

    switch (dwAction)
    {
        case WB_GOBACK:
            IWebBrowser2_GoBack(pWebBrowser2);
            break;
        case WB_GOFORWARD:
            IWebBrowser2_GoForward(pWebBrowser2);
            break;
        case WB_GOHOME:
            IWebBrowser2_GoHome(pWebBrowser2);
            break;
        case WB_SEARCH:
            IWebBrowser2_GoSearch(pWebBrowser2);
            break;
        case WB_REFRESH:
            IWebBrowser2_Refresh(pWebBrowser2);
        case WB_STOP:
            IWebBrowser2_Stop(pWebBrowser2);
    }
}
