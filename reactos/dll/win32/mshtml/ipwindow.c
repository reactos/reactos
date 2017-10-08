/*
 * Copyright 2010 Jacek Caban for CodeWeavers
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
    IOleInPlaceFrame IOleInPlaceFrame_iface;
    LONG ref;
} InPlaceFrame;

static inline InPlaceFrame *impl_from_IOleInPlaceFrame(IOleInPlaceFrame *iface)
{
    return CONTAINING_RECORD(iface, InPlaceFrame, IOleInPlaceFrame_iface);
}

static HRESULT WINAPI InPlaceFrame_QueryInterface(IOleInPlaceFrame *iface,
                                                  REFIID riid, void **ppv)
{
    InPlaceFrame *This = impl_from_IOleInPlaceFrame(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IOleInPlaceFrame_iface;
    }else if(IsEqualGUID(&IID_IOleWindow, riid)) {
        *ppv = &This->IOleInPlaceFrame_iface;
    }else if(IsEqualGUID(&IID_IOleInPlaceUIWindow, riid)) {
        *ppv = &This->IOleInPlaceFrame_iface;
    }else if(IsEqualGUID(&IID_IOleInPlaceFrame, riid)) {
        *ppv = &This->IOleInPlaceFrame_iface;
    }else {
        WARN("Unsopported interface %s\n", debugstr_mshtml_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI InPlaceFrame_AddRef(IOleInPlaceFrame *iface)
{
    InPlaceFrame *This = impl_from_IOleInPlaceFrame(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI InPlaceFrame_Release(IOleInPlaceFrame *iface)
{
    InPlaceFrame *This = impl_from_IOleInPlaceFrame(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI InPlaceFrame_GetWindow(IOleInPlaceFrame *iface, HWND *phwnd)
{
    InPlaceFrame *This = impl_from_IOleInPlaceFrame(iface);
    FIXME("(%p)->(%p)\n", This, phwnd);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_ContextSensitiveHelp(IOleInPlaceFrame *iface,
                                                        BOOL fEnterMode)
{
    InPlaceFrame *This = impl_from_IOleInPlaceFrame(iface);
    FIXME("(%p)->(%x)\n", This, fEnterMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_GetBorder(IOleInPlaceFrame *iface, LPRECT lprectBorder)
{
    InPlaceFrame *This = impl_from_IOleInPlaceFrame(iface);
    FIXME("(%p)->(%p)\n", This, lprectBorder);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_RequestBorderSpace(IOleInPlaceFrame *iface,
                                                      LPCBORDERWIDTHS pborderwidths)
{
    InPlaceFrame *This = impl_from_IOleInPlaceFrame(iface);
    FIXME("(%p)->(%p)\n", This, pborderwidths);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetBorderSpace(IOleInPlaceFrame *iface,
                                                  LPCBORDERWIDTHS pborderwidths)
{
    InPlaceFrame *This = impl_from_IOleInPlaceFrame(iface);
    FIXME("(%p)->(%p)\n", This, pborderwidths);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetActiveObject(IOleInPlaceFrame *iface,
        IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    InPlaceFrame *This = impl_from_IOleInPlaceFrame(iface);
    FIXME("(%p)->(%p %s)\n", This, pActiveObject, debugstr_w(pszObjName));
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_InsertMenus(IOleInPlaceFrame *iface, HMENU hmenuShared,
        LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    InPlaceFrame *This = impl_from_IOleInPlaceFrame(iface);
    FIXME("(%p)->(%p %p)\n", This, hmenuShared, lpMenuWidths);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetMenu(IOleInPlaceFrame *iface, HMENU hmenuShared,
        HOLEMENU holemenu, HWND hwndActiveObject)
{
    InPlaceFrame *This = impl_from_IOleInPlaceFrame(iface);
    FIXME("(%p)->(%p %p %p)\n", This, hmenuShared, holemenu, hwndActiveObject);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_RemoveMenus(IOleInPlaceFrame *iface, HMENU hmenuShared)
{
    InPlaceFrame *This = impl_from_IOleInPlaceFrame(iface);
    FIXME("(%p)->(%p)\n", This, hmenuShared);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetStatusText(IOleInPlaceFrame *iface,
                                                 LPCOLESTR pszStatusText)
{
    InPlaceFrame *This = impl_from_IOleInPlaceFrame(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pszStatusText));
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_EnableModeless(IOleInPlaceFrame *iface, BOOL fEnable)
{
    InPlaceFrame *This = impl_from_IOleInPlaceFrame(iface);
    FIXME("(%p)->(%x)\n", This, fEnable);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_TranslateAccelerator(IOleInPlaceFrame *iface, LPMSG lpmsg,
                                                        WORD wID)
{
    InPlaceFrame *This = impl_from_IOleInPlaceFrame(iface);
    FIXME("(%p)->(%p %d)\n", This, lpmsg, wID);
    return E_NOTIMPL;
}

static const IOleInPlaceFrameVtbl OleInPlaceFrameVtbl = {
    InPlaceFrame_QueryInterface,
    InPlaceFrame_AddRef,
    InPlaceFrame_Release,
    InPlaceFrame_GetWindow,
    InPlaceFrame_ContextSensitiveHelp,
    InPlaceFrame_GetBorder,
    InPlaceFrame_RequestBorderSpace,
    InPlaceFrame_SetBorderSpace,
    InPlaceFrame_SetActiveObject,
    InPlaceFrame_InsertMenus,
    InPlaceFrame_SetMenu,
    InPlaceFrame_RemoveMenus,
    InPlaceFrame_SetStatusText,
    InPlaceFrame_EnableModeless,
    InPlaceFrame_TranslateAccelerator
};

HRESULT create_ip_frame(IOleInPlaceFrame **ret)
{
    InPlaceFrame *frame;

    frame = heap_alloc_zero(sizeof(*frame));
    if(!frame)
        return E_OUTOFMEMORY;

    frame->IOleInPlaceFrame_iface.lpVtbl = &OleInPlaceFrameVtbl;
    frame->ref = 1;

    *ret = &frame->IOleInPlaceFrame_iface;
    return S_OK;
}

typedef struct {
    IOleInPlaceUIWindow IOleInPlaceUIWindow_iface;
    LONG ref;
} InPlaceUIWindow;

static inline InPlaceUIWindow *impl_from_IOleInPlaceUIWindow(IOleInPlaceUIWindow *iface)
{
    return CONTAINING_RECORD(iface, InPlaceUIWindow, IOleInPlaceUIWindow_iface);
}

static HRESULT WINAPI InPlaceUIWindow_QueryInterface(IOleInPlaceUIWindow *iface, REFIID riid, void **ppv)
{
    InPlaceUIWindow *This = impl_from_IOleInPlaceUIWindow(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IOleInPlaceUIWindow_iface;
    }else if(IsEqualGUID(&IID_IOleWindow, riid)) {
        *ppv = &This->IOleInPlaceUIWindow_iface;
    }else if(IsEqualGUID(&IID_IOleInPlaceUIWindow, riid)) {
        *ppv = &This->IOleInPlaceUIWindow_iface;
    }else {
        WARN("Unsopported interface %s\n", debugstr_mshtml_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI InPlaceUIWindow_AddRef(IOleInPlaceUIWindow *iface)
{
    InPlaceUIWindow *This = impl_from_IOleInPlaceUIWindow(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI InPlaceUIWindow_Release(IOleInPlaceUIWindow *iface)
{
    InPlaceUIWindow *This = impl_from_IOleInPlaceUIWindow(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI InPlaceUIWindow_GetWindow(IOleInPlaceUIWindow *iface, HWND *phwnd)
{
    InPlaceUIWindow *This = impl_from_IOleInPlaceUIWindow(iface);
    FIXME("(%p)->(%p)\n", This, phwnd);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceUIWindow_ContextSensitiveHelp(IOleInPlaceUIWindow *iface,
        BOOL fEnterMode)
{
    InPlaceUIWindow *This = impl_from_IOleInPlaceUIWindow(iface);
    FIXME("(%p)->(%x)\n", This, fEnterMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceUIWindow_GetBorder(IOleInPlaceUIWindow *iface, LPRECT lprectBorder)
{
    InPlaceUIWindow *This = impl_from_IOleInPlaceUIWindow(iface);
    FIXME("(%p)->(%p)\n", This, lprectBorder);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceUIWindow_RequestBorderSpace(IOleInPlaceUIWindow *iface,
        LPCBORDERWIDTHS pborderwidths)
{
    InPlaceUIWindow *This = impl_from_IOleInPlaceUIWindow(iface);
    FIXME("(%p)->(%p)\n", This, pborderwidths);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceUIWindow_SetBorderSpace(IOleInPlaceUIWindow *iface,
        LPCBORDERWIDTHS pborderwidths)
{
    InPlaceUIWindow *This = impl_from_IOleInPlaceUIWindow(iface);
    FIXME("(%p)->(%p)\n", This, pborderwidths);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceUIWindow_SetActiveObject(IOleInPlaceUIWindow *iface,
        IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    InPlaceUIWindow *This = impl_from_IOleInPlaceUIWindow(iface);
    FIXME("(%p)->(%p %s)\n", This, pActiveObject, debugstr_w(pszObjName));
    return E_NOTIMPL;
}

static const IOleInPlaceUIWindowVtbl OleInPlaceUIWindowVtbl = {
    InPlaceUIWindow_QueryInterface,
    InPlaceUIWindow_AddRef,
    InPlaceUIWindow_Release,
    InPlaceUIWindow_GetWindow,
    InPlaceUIWindow_ContextSensitiveHelp,
    InPlaceUIWindow_GetBorder,
    InPlaceUIWindow_RequestBorderSpace,
    InPlaceUIWindow_SetBorderSpace,
    InPlaceUIWindow_SetActiveObject,
};

HRESULT create_ip_window(IOleInPlaceUIWindow **ret)
{
    InPlaceUIWindow *uiwindow;

    uiwindow = heap_alloc_zero(sizeof(*uiwindow));
    if(!uiwindow)
        return E_OUTOFMEMORY;

    uiwindow->IOleInPlaceUIWindow_iface.lpVtbl = &OleInPlaceUIWindowVtbl;
    uiwindow->ref = 1;

    *ret = &uiwindow->IOleInPlaceUIWindow_iface;
    return S_OK;
}
