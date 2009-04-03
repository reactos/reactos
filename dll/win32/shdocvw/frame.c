/*
 * Copyright 2005 Jacek Caban for CodeWeavers
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

#include "wine/debug.h"
#include "shdocvw.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

#define INPLACEFRAME_THIS(iface) DEFINE_THIS(DocHost, OleInPlaceFrame, iface)

static HRESULT WINAPI InPlaceFrame_QueryInterface(IOleInPlaceFrame *iface,
                                                  REFIID riid, void **ppv)
{
    DocHost *This = INPLACEFRAME_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = INPLACEFRAME(This);
    }else if(IsEqualGUID(&IID_IOleInPlaceUIWindow, riid)) {
        TRACE("(%p)->(IID_IOleInPlaceUIWindow %p)\n", This, ppv);
        *ppv = INPLACEFRAME(This);
    }else if(IsEqualGUID(&IID_IOleInPlaceFrame, riid)) {
        TRACE("(%p)->(IID_IOleInPlaceFrame %p)\n", This, ppv);
        *ppv = INPLACEFRAME(This);
    }

    if(*ppv) {
        IOleInPlaceFrame_AddRef(INPLACEFRAME(This));
        return S_OK;
    }

    WARN("Unsopported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI InPlaceFrame_AddRef(IOleInPlaceFrame *iface)
{
    DocHost *This = INPLACEFRAME_THIS(iface);
    return IOleClientSite_AddRef(CLIENTSITE(This));
}

static ULONG WINAPI InPlaceFrame_Release(IOleInPlaceFrame *iface)
{
    DocHost *This = INPLACEFRAME_THIS(iface);
    return IOleClientSite_Release(CLIENTSITE(This));
}

static HRESULT WINAPI InPlaceFrame_GetWindow(IOleInPlaceFrame *iface, HWND *phwnd)
{
    DocHost *This = INPLACEFRAME_THIS(iface);
    FIXME("(%p)->(%p)\n", This, phwnd);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_ContextSensitiveHelp(IOleInPlaceFrame *iface,
                                                        BOOL fEnterMode)
{
    DocHost *This = INPLACEFRAME_THIS(iface);
    FIXME("(%p)->(%x)\n", This, fEnterMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_GetBorder(IOleInPlaceFrame *iface, LPRECT lprectBorder)
{
    DocHost *This = INPLACEFRAME_THIS(iface);
    FIXME("(%p)->(%p)\n", This, lprectBorder);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_RequestBorderSpace(IOleInPlaceFrame *iface,
                                                      LPCBORDERWIDTHS pborderwidths)
{
    DocHost *This = INPLACEFRAME_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pborderwidths);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetBorderSpace(IOleInPlaceFrame *iface,
                                                  LPCBORDERWIDTHS pborderwidths)
{
    DocHost *This = INPLACEFRAME_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pborderwidths);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetActiveObject(IOleInPlaceFrame *iface,
        IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    DocHost *This = INPLACEFRAME_THIS(iface);
    FIXME("(%p)->(%p %s)\n", This, pActiveObject, debugstr_w(pszObjName));
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_InsertMenus(IOleInPlaceFrame *iface, HMENU hmenuShared,
        LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    DocHost *This = INPLACEFRAME_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, hmenuShared, lpMenuWidths);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetMenu(IOleInPlaceFrame *iface, HMENU hmenuShared,
        HOLEMENU holemenu, HWND hwndActiveObject)
{
    DocHost *This = INPLACEFRAME_THIS(iface);
    FIXME("(%p)->(%p %p %p)\n", This, hmenuShared, holemenu, hwndActiveObject);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_RemoveMenus(IOleInPlaceFrame *iface, HMENU hmenuShared)
{
    DocHost *This = INPLACEFRAME_THIS(iface);
    FIXME("(%p)->(%p)\n", This, hmenuShared);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetStatusText(IOleInPlaceFrame *iface,
                                                 LPCOLESTR pszStatusText)
{
    DocHost *This = INPLACEFRAME_THIS(iface);
    FIXME("(%p)->(%p)\n", This, debugstr_w(pszStatusText));
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_EnableModeless(IOleInPlaceFrame *iface, BOOL fEnable)
{
    DocHost *This = INPLACEFRAME_THIS(iface);
    FIXME("(%p)->(%x)\n", This, fEnable);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_TranslateAccelerator(IOleInPlaceFrame *iface, LPMSG lpmsg,
                                                        WORD wID)
{
    DocHost *This = INPLACEFRAME_THIS(iface);
    FIXME("(%p)->(%p %d)\n", This, lpmsg, wID);
    return E_NOTIMPL;
}

#undef INPLACEFRAME_THIS

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

void DocHost_Frame_Init(DocHost *This)
{
    This->lpOleInPlaceFrameVtbl = &OleInPlaceFrameVtbl;
}
