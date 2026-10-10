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
#include "resource.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(htmlhelp);

static inline WebBrowserContainer *impl_from_IOleClientSite(IOleClientSite *iface)
{
    return CONTAINING_RECORD(iface, WebBrowserContainer, IOleClientSite_iface);
}

static HRESULT STDMETHODCALLTYPE Site_QueryInterface(IOleClientSite *iface, REFIID riid, void **ppvObj)
{
    WebBrowserContainer *This = impl_from_IOleClientSite(iface);

    if (IsEqualIID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppvObj);
        *ppvObj = &This->IOleClientSite_iface;
    }else if(IsEqualIID(riid, &IID_IOleClientSite)) {
        TRACE("(%p)->(IID_IOleClientSite %p)\n", This, ppvObj);
        *ppvObj = &This->IOleClientSite_iface;
    }else if (IsEqualIID(riid, &IID_IOleInPlaceSite)) {
        TRACE("(%p)->(IID_IOleInPlaceSite %p)\n", This, ppvObj);
        *ppvObj = &This->IOleInPlaceSite_iface;
    }else if (IsEqualIID(riid, &IID_IOleInPlaceFrame)) {
        TRACE("(%p)->(IID_IOleInPlaceFrame %p)\n", This, ppvObj);
        *ppvObj = &This->IOleInPlaceSite_iface;
    }else if (IsEqualIID(riid, &IID_IDocHostUIHandler)) {
        TRACE("(%p)->(IID_IDocHostUIHandler %p)\n", This, ppvObj);
        *ppvObj = &This->IDocHostUIHandler_iface;
    }else {
        TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObj);
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppvObj);
    return S_OK;
}

static ULONG STDMETHODCALLTYPE Site_AddRef(IOleClientSite *iface)
{
    WebBrowserContainer *This = impl_from_IOleClientSite(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG STDMETHODCALLTYPE Site_Release(IOleClientSite *iface)
{
    WebBrowserContainer *This = impl_from_IOleClientSite(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        if(This->ole_obj)
            IOleObject_Release(This->ole_obj);
        if(This->web_browser)
            IWebBrowser2_Release(This->web_browser);
        free(This);
    }

    return ref;
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

static const IOleClientSiteVtbl OleClientSiteVtbl =
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

static inline WebBrowserContainer *impl_from_IDocHostUIHandler(IDocHostUIHandler *iface)
{
    return CONTAINING_RECORD(iface, WebBrowserContainer, IDocHostUIHandler_iface);
}

static HRESULT STDMETHODCALLTYPE UI_QueryInterface(IDocHostUIHandler *iface, REFIID riid, LPVOID *ppvObj)
{
    WebBrowserContainer *This = impl_from_IDocHostUIHandler(iface);

    return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, riid, ppvObj);
}

static ULONG STDMETHODCALLTYPE UI_AddRef(IDocHostUIHandler *iface)
{
    WebBrowserContainer *This = impl_from_IDocHostUIHandler(iface);

    return IOleClientSite_AddRef(&This->IOleClientSite_iface);
}

static ULONG STDMETHODCALLTYPE UI_Release(IDocHostUIHandler * iface)
{
    WebBrowserContainer *This = impl_from_IDocHostUIHandler(iface);

    return IOleClientSite_Release(&This->IOleClientSite_iface);
}

static HRESULT STDMETHODCALLTYPE UI_ShowContextMenu(IDocHostUIHandler *iface, DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved)
{
    WebBrowserContainer *This = impl_from_IDocHostUIHandler(iface);
    DWORD cmdid, menu_id = 0;
    HMENU menu, submenu;

    TRACE("(%p)->(%ld %s)\n", This, dwID, wine_dbgstr_point(ppt));

    menu = LoadMenuW(hhctrl_hinstance, MAKEINTRESOURCEW(MENU_WEBBROWSER));
    if (!menu)
        return S_OK;

    /* FIXME: Support more menu types. */
    if(dwID == CONTEXT_MENU_TEXTSELECT)
        menu_id = 1;

    submenu = GetSubMenu(menu, menu_id);

    cmdid = TrackPopupMenu(submenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
            ppt->x, ppt->y, 0, This->hwndWindow, NULL);
    DestroyMenu(menu);

    switch(cmdid) {
    case IDTB_BACK:
        DoPageAction(This, WB_GOBACK);
        break;
    case IDTB_FORWARD:
        DoPageAction(This, WB_GOFORWARD);
        break;
    case MIID_SELECTALL:
        IWebBrowser2_ExecWB(This->web_browser, OLECMDID_SELECTALL, 0, NULL, NULL);
        break;
    case MIID_VIEWSOURCE:
        FIXME("View source\n");
        break;
    case IDTB_PRINT:
        DoPageAction(This, WB_PRINT);
        break;
    case IDTB_REFRESH:
        DoPageAction(This, WB_REFRESH);
        break;
    case MIID_PROPERTIES:
        FIXME("Properties\n");
        break;
    case MIID_COPY:
        IWebBrowser2_ExecWB(This->web_browser, OLECMDID_COPY, 0, NULL, NULL);
        break;
    case MIID_PASTE:
        IWebBrowser2_ExecWB(This->web_browser, OLECMDID_PASTE, 0, NULL, NULL);
        break;
    case MIID_CUT:
        IWebBrowser2_ExecWB(This->web_browser, OLECMDID_CUT, 0, NULL, NULL);
        break;
    }

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

static const IDocHostUIHandlerVtbl DocHostUIHandlerVtbl =
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

static inline WebBrowserContainer *impl_from_IOleInPlaceSite(IOleInPlaceSite *iface)
{
    return CONTAINING_RECORD(iface, WebBrowserContainer, IOleInPlaceSite_iface);
}

static HRESULT STDMETHODCALLTYPE InPlace_QueryInterface(IOleInPlaceSite *iface, REFIID riid, LPVOID *ppvObj)
{
    WebBrowserContainer *This = impl_from_IOleInPlaceSite(iface);

    return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, riid, ppvObj);
}

static ULONG STDMETHODCALLTYPE InPlace_AddRef(IOleInPlaceSite *iface)
{
    WebBrowserContainer *This = impl_from_IOleInPlaceSite(iface);

    return IOleClientSite_AddRef(&This->IOleClientSite_iface);
}

static ULONG STDMETHODCALLTYPE InPlace_Release(IOleInPlaceSite *iface)
{
    WebBrowserContainer *This = impl_from_IOleInPlaceSite(iface);

    return IOleClientSite_Release(&This->IOleClientSite_iface);
}

static HRESULT STDMETHODCALLTYPE InPlace_GetWindow(IOleInPlaceSite *iface, HWND *lphwnd)
{
    WebBrowserContainer *This = impl_from_IOleInPlaceSite(iface);

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
    WebBrowserContainer *This = impl_from_IOleInPlaceSite(iface);

    *lplpFrame = &This->IOleInPlaceFrame_iface;
    IOleInPlaceFrame_AddRef(&This->IOleInPlaceFrame_iface);

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
    WebBrowserContainer *This = impl_from_IOleInPlaceSite(iface);
    IOleInPlaceObject *inplace;

    if (IOleObject_QueryInterface(This->ole_obj, &IID_IOleInPlaceObject,
                                  (void **)&inplace) == S_OK)
    {
        IOleInPlaceObject_SetObjectRects(inplace, lprcPosRect, lprcPosRect);
        IOleInPlaceObject_Release(inplace);
    }

    return S_OK;
}

static const IOleInPlaceSiteVtbl OleInPlaceSiteVtbl =
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

static inline WebBrowserContainer *impl_from_IOleInPlaceFrame(IOleInPlaceFrame *iface)
{
    return CONTAINING_RECORD(iface, WebBrowserContainer, IOleInPlaceFrame_iface);
}

static HRESULT STDMETHODCALLTYPE Frame_QueryInterface(IOleInPlaceFrame *iface, REFIID riid, LPVOID *ppvObj)
{
    WebBrowserContainer *This = impl_from_IOleInPlaceFrame(iface);

    return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, riid, ppvObj);
}

static ULONG STDMETHODCALLTYPE Frame_AddRef(IOleInPlaceFrame *iface)
{
    WebBrowserContainer *This = impl_from_IOleInPlaceFrame(iface);

    return IOleClientSite_AddRef(&This->IOleClientSite_iface);
}

static ULONG STDMETHODCALLTYPE Frame_Release(IOleInPlaceFrame *iface)
{
    WebBrowserContainer *This = impl_from_IOleInPlaceFrame(iface);

    return IOleClientSite_Release(&This->IOleClientSite_iface);
}

static HRESULT STDMETHODCALLTYPE Frame_GetWindow(IOleInPlaceFrame *iface, HWND *lphwnd)
{
    WebBrowserContainer *This = impl_from_IOleInPlaceFrame(iface);

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

static const IOleInPlaceFrameVtbl OleInPlaceFrameVtbl =
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
    WebBrowserContainer *container;
    IOleInPlaceObject *inplace;
    HRESULT hr;
    RECT rc;

    container = calloc(1, sizeof(*container));
    if (!container)
        return FALSE;

    container->IOleClientSite_iface.lpVtbl = &OleClientSiteVtbl;
    container->IOleInPlaceSite_iface.lpVtbl = &OleInPlaceSiteVtbl;
    container->IOleInPlaceFrame_iface.lpVtbl = &OleInPlaceFrameVtbl;
    container->IDocHostUIHandler_iface.lpVtbl = &DocHostUIHandlerVtbl;
    container->ref = 1;
    container->hwndWindow = hwndParent;

    info->web_browser = container;

    hr = OleCreate(&CLSID_WebBrowser, &IID_IOleObject, OLERENDER_DRAW, 0,
                   &container->IOleClientSite_iface, &MyIStorage,
                   (void **)&container->ole_obj);

    if (FAILED(hr)) goto error;

    GetClientRect(hwndParent, &rc);

    hr = OleSetContainedObject((struct IUnknown *)container->ole_obj, TRUE);
    if (FAILED(hr)) goto error;

    hr = IOleObject_DoVerb(container->ole_obj, OLEIVERB_SHOW, NULL,
                           &container->IOleClientSite_iface, -1, hwndParent, &rc);
    if (FAILED(hr)) goto error;

    hr = IOleObject_QueryInterface(container->ole_obj, &IID_IOleInPlaceObject, (void**)&inplace);
    if (FAILED(hr)) goto error;

    IOleInPlaceObject_SetObjectRects(inplace, &rc, &rc);
    IOleInPlaceObject_Release(inplace);

    hr = IOleObject_QueryInterface(container->ole_obj, &IID_IWebBrowser2, (void **)&container->web_browser);
    if (SUCCEEDED(hr))
        return TRUE;

error:
    ReleaseWebBrowser(info);
    return FALSE;
}

void ReleaseWebBrowser(HHInfo *info)
{
    WebBrowserContainer *container = info->web_browser;
    HRESULT hres;

    if(!container)
        return;

    if(container->ole_obj) {
        IOleInPlaceSite *inplace;

        hres = IOleObject_QueryInterface(container->ole_obj, &IID_IOleInPlaceSite, (void**)&inplace);
        if(SUCCEEDED(hres)) {
            IOleInPlaceSite_OnInPlaceDeactivate(inplace);
            IOleInPlaceSite_Release(inplace);
        }

        IOleObject_SetClientSite(container->ole_obj, NULL);
    }

    info->web_browser = NULL;
    IOleClientSite_Release(&container->IOleClientSite_iface);
}

void ResizeWebBrowser(HHInfo *info, DWORD dwWidth, DWORD dwHeight)
{
    if (!info->web_browser)
        return;

    IWebBrowser2_put_Width(info->web_browser->web_browser, dwWidth);
    IWebBrowser2_put_Height(info->web_browser->web_browser, dwHeight);
}

void DoPageAction(WebBrowserContainer *container, DWORD dwAction)
{
    if (!container || !container->web_browser)
        return;

    switch (dwAction)
    {
        case WB_GOBACK:
            IWebBrowser2_GoBack(container->web_browser);
            break;
        case WB_GOFORWARD:
            IWebBrowser2_GoForward(container->web_browser);
            break;
        case WB_GOHOME:
            IWebBrowser2_GoHome(container->web_browser);
            break;
        case WB_SEARCH:
            IWebBrowser2_GoSearch(container->web_browser);
            break;
        case WB_REFRESH:
            IWebBrowser2_Refresh(container->web_browser);
            break;
        case WB_STOP:
            IWebBrowser2_Stop(container->web_browser);
            break;
        case WB_PRINT:
            IWebBrowser2_ExecWB(container->web_browser, OLECMDID_PRINT, OLECMDEXECOPT_DONTPROMPTUSER, 0, 0);
            break;
    }
}
