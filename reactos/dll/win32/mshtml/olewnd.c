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

/**********************************************************
 * IOleInPlaceActiveObject implementation
 */

static inline HTMLDocument *impl_from_IOleInPlaceActiveObject(IOleInPlaceActiveObject *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, IOleInPlaceActiveObject_iface);
}

static HRESULT WINAPI OleInPlaceActiveObject_QueryInterface(IOleInPlaceActiveObject *iface, REFIID riid, void **ppv)
{
    HTMLDocument *This = impl_from_IOleInPlaceActiveObject(iface);
    return htmldoc_query_interface(This, riid, ppv);
}

static ULONG WINAPI OleInPlaceActiveObject_AddRef(IOleInPlaceActiveObject *iface)
{
    HTMLDocument *This = impl_from_IOleInPlaceActiveObject(iface);
    return htmldoc_addref(This);
}

static ULONG WINAPI OleInPlaceActiveObject_Release(IOleInPlaceActiveObject *iface)
{
    HTMLDocument *This = impl_from_IOleInPlaceActiveObject(iface);
    return htmldoc_release(This);
}

static HRESULT WINAPI OleInPlaceActiveObject_GetWindow(IOleInPlaceActiveObject *iface, HWND *phwnd)
{
    HTMLDocument *This = impl_from_IOleInPlaceActiveObject(iface);

    TRACE("(%p)->(%p)\n", This, phwnd);

    if(!phwnd)
        return E_INVALIDARG;

    if(!This->doc_obj->in_place_active) {
        *phwnd = NULL;
        return E_FAIL;
    }

    *phwnd = This->doc_obj->hwnd;
    return S_OK;
}

static HRESULT WINAPI OleInPlaceActiveObject_ContextSensitiveHelp(IOleInPlaceActiveObject *iface, BOOL fEnterMode)
{
    HTMLDocument *This = impl_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%x)\n", This, fEnterMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceActiveObject_TranslateAccelerator(IOleInPlaceActiveObject *iface, LPMSG lpmsg)
{
    HTMLDocument *This = impl_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%p)\n", This, lpmsg);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceActiveObject_OnFrameWindowActivate(IOleInPlaceActiveObject *iface,
        BOOL fActivate)
{
    HTMLDocument *This = impl_from_IOleInPlaceActiveObject(iface);

    TRACE("(%p)->(%x)\n", This, fActivate);

    if(This->doc_obj->hostui)
        IDocHostUIHandler_OnFrameWindowActivate(This->doc_obj->hostui, fActivate);

    return S_OK;
}

static HRESULT WINAPI OleInPlaceActiveObject_OnDocWindowActivate(IOleInPlaceActiveObject *iface, BOOL fActivate)
{
    HTMLDocument *This = impl_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%x)\n", This, fActivate);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceActiveObject_ResizeBorder(IOleInPlaceActiveObject *iface, LPCRECT prcBorder,
                                                IOleInPlaceUIWindow *pUIWindow, BOOL fFrameWindow)
{
    HTMLDocument *This = impl_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%p %p %x)\n", This, prcBorder, pUIWindow, fFrameWindow);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceActiveObject_EnableModeless(IOleInPlaceActiveObject *iface, BOOL fEnable)
{
    HTMLDocument *This = impl_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%x)\n", This, fEnable);
    return E_NOTIMPL;
}

static const IOleInPlaceActiveObjectVtbl OleInPlaceActiveObjectVtbl = {
    OleInPlaceActiveObject_QueryInterface,
    OleInPlaceActiveObject_AddRef,
    OleInPlaceActiveObject_Release,
    OleInPlaceActiveObject_GetWindow,
    OleInPlaceActiveObject_ContextSensitiveHelp,
    OleInPlaceActiveObject_TranslateAccelerator,
    OleInPlaceActiveObject_OnFrameWindowActivate,
    OleInPlaceActiveObject_OnDocWindowActivate,
    OleInPlaceActiveObject_ResizeBorder,
    OleInPlaceActiveObject_EnableModeless
};

/**********************************************************
 * IOleInPlaceObjectWindowless implementation
 */

static inline HTMLDocument *impl_from_IOleInPlaceObjectWindowless(IOleInPlaceObjectWindowless *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, IOleInPlaceObjectWindowless_iface);
}

static HRESULT WINAPI OleInPlaceObjectWindowless_QueryInterface(IOleInPlaceObjectWindowless *iface,
        REFIID riid, void **ppv)
{
    HTMLDocument *This = impl_from_IOleInPlaceObjectWindowless(iface);
    return htmldoc_query_interface(This, riid, ppv);
}

static ULONG WINAPI OleInPlaceObjectWindowless_AddRef(IOleInPlaceObjectWindowless *iface)
{
    HTMLDocument *This = impl_from_IOleInPlaceObjectWindowless(iface);
    return htmldoc_addref(This);
}

static ULONG WINAPI OleInPlaceObjectWindowless_Release(IOleInPlaceObjectWindowless *iface)
{
    HTMLDocument *This = impl_from_IOleInPlaceObjectWindowless(iface);
    return htmldoc_release(This);
}

static HRESULT WINAPI OleInPlaceObjectWindowless_GetWindow(IOleInPlaceObjectWindowless *iface,
        HWND *phwnd)
{
    HTMLDocument *This = impl_from_IOleInPlaceObjectWindowless(iface);
    return IOleInPlaceActiveObject_GetWindow(&This->IOleInPlaceActiveObject_iface, phwnd);
}

static HRESULT WINAPI OleInPlaceObjectWindowless_ContextSensitiveHelp(IOleInPlaceObjectWindowless *iface,
        BOOL fEnterMode)
{
    HTMLDocument *This = impl_from_IOleInPlaceObjectWindowless(iface);
    return IOleInPlaceActiveObject_ContextSensitiveHelp(&This->IOleInPlaceActiveObject_iface, fEnterMode);
}

static HRESULT WINAPI OleInPlaceObjectWindowless_InPlaceDeactivate(IOleInPlaceObjectWindowless *iface)
{
    HTMLDocument *This = impl_from_IOleInPlaceObjectWindowless(iface);

    TRACE("(%p)\n", This);

    if(This->doc_obj->ui_active)
        IOleDocumentView_UIActivate(&This->IOleDocumentView_iface, FALSE);
    This->doc_obj->window_active = FALSE;

    if(!This->doc_obj->in_place_active)
        return S_OK;

    if(This->doc_obj->frame) {
        IOleInPlaceFrame_Release(This->doc_obj->frame);
        This->doc_obj->frame = NULL;
    }

    if(This->doc_obj->hwnd) {
        ShowWindow(This->doc_obj->hwnd, SW_HIDE);
        SetWindowPos(This->doc_obj->hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
    }

    This->doc_obj->focus = FALSE;
    notif_focus(This->doc_obj);

    This->doc_obj->in_place_active = FALSE;
    if(This->doc_obj->ipsite) {
        IOleInPlaceSiteEx *ipsiteex;
        HRESULT hres;

        hres = IOleInPlaceSite_QueryInterface(This->doc_obj->ipsite, &IID_IOleInPlaceSiteEx, (void**)&ipsiteex);
        if(SUCCEEDED(hres)) {
            IOleInPlaceSiteEx_OnInPlaceDeactivateEx(ipsiteex, TRUE);
            IOleInPlaceSiteEx_Release(ipsiteex);
        }else {
            IOleInPlaceSite_OnInPlaceDeactivate(This->doc_obj->ipsite);
        }
    }

    return S_OK;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_UIDeactivate(IOleInPlaceObjectWindowless *iface)
{
    HTMLDocument *This = impl_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_SetObjectRects(IOleInPlaceObjectWindowless *iface,
        LPCRECT lprcPosRect, LPCRECT lprcClipRect)
{
    HTMLDocument *This = impl_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)->(%p %p)\n", This, lprcPosRect, lprcClipRect);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_ReactivateAndUndo(IOleInPlaceObjectWindowless *iface)
{
    HTMLDocument *This = impl_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_OnWindowMessage(IOleInPlaceObjectWindowless *iface,
        UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *lpResult)
{
    HTMLDocument *This = impl_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)->(%u %lu %lu %p)\n", This, msg, wParam, lParam, lpResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_GetDropTarget(IOleInPlaceObjectWindowless *iface,
        IDropTarget **ppDropTarget)
{
    HTMLDocument *This = impl_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)->(%p)\n", This, ppDropTarget);
    return E_NOTIMPL;
}

static const IOleInPlaceObjectWindowlessVtbl OleInPlaceObjectWindowlessVtbl = {
    OleInPlaceObjectWindowless_QueryInterface,
    OleInPlaceObjectWindowless_AddRef,
    OleInPlaceObjectWindowless_Release,
    OleInPlaceObjectWindowless_GetWindow,
    OleInPlaceObjectWindowless_ContextSensitiveHelp,
    OleInPlaceObjectWindowless_InPlaceDeactivate,
    OleInPlaceObjectWindowless_UIDeactivate,
    OleInPlaceObjectWindowless_SetObjectRects,
    OleInPlaceObjectWindowless_ReactivateAndUndo,
    OleInPlaceObjectWindowless_OnWindowMessage,
    OleInPlaceObjectWindowless_GetDropTarget
};

void HTMLDocument_Window_Init(HTMLDocument *This)
{
    This->IOleInPlaceActiveObject_iface.lpVtbl = &OleInPlaceActiveObjectVtbl;
    This->IOleInPlaceObjectWindowless_iface.lpVtbl = &OleInPlaceObjectWindowlessVtbl;
}
