/*
 * Copyright 2014 Piotr Caban for CodeWeavers
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

#include "oleacc_private.h"

typedef struct {
    IAccessible IAccessible_iface;
    IOleWindow IOleWindow_iface;
    IEnumVARIANT IEnumVARIANT_iface;

    LONG ref;

    HWND hwnd;
    HWND enum_pos;
} Client;

static inline Client* impl_from_Client(IAccessible *iface)
{
    return CONTAINING_RECORD(iface, Client, IAccessible_iface);
}

static HRESULT WINAPI Client_QueryInterface(IAccessible *iface, REFIID riid, void **ppv)
{
    Client *This = impl_from_Client(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    if(IsEqualIID(riid, &IID_IAccessible) ||
            IsEqualIID(riid, &IID_IDispatch) ||
            IsEqualIID(riid, &IID_IUnknown)) {
        *ppv = iface;
    }else if(IsEqualIID(riid, &IID_IOleWindow)) {
        *ppv = &This->IOleWindow_iface;
    }else if(IsEqualIID(riid, &IID_IEnumVARIANT)) {
        *ppv = &This->IEnumVARIANT_iface;
    }else {
        WARN("no interface: %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IAccessible_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI Client_AddRef(IAccessible *iface)
{
    Client *This = impl_from_Client(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref = %u\n", This, ref);
    return ref;
}

static ULONG WINAPI Client_Release(IAccessible *iface)
{
    Client *This = impl_from_Client(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref = %u\n", This, ref);

    if(!ref)
        heap_free(This);
    return ref;
}

static HRESULT WINAPI Client_GetTypeInfoCount(IAccessible *iface, UINT *pctinfo)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_GetTypeInfo(IAccessible *iface,
        UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%u %x %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_GetIDsOfNames(IAccessible *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%s %p %u %x %p)\n", This, debugstr_guid(riid),
            rgszNames, cNames, lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_Invoke(IAccessible *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%x %s %x %x %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_get_accParent(IAccessible *iface, IDispatch **ppdispParent)
{
    Client *This = impl_from_Client(iface);

    TRACE("(%p)->(%p)\n", This, ppdispParent);

    return AccessibleObjectFromWindow(This->hwnd, OBJID_WINDOW,
            &IID_IDispatch, (void**)ppdispParent);
}

static HRESULT WINAPI Client_get_accChildCount(IAccessible *iface, LONG *pcountChildren)
{
    Client *This = impl_from_Client(iface);
    HWND cur;

    TRACE("(%p)->(%p)\n", This, pcountChildren);

    *pcountChildren = 0;
    for(cur = GetWindow(This->hwnd, GW_CHILD); cur; cur = GetWindow(cur, GW_HWNDNEXT))
        (*pcountChildren)++;

    return S_OK;
}

static HRESULT WINAPI Client_get_accChild(IAccessible *iface,
        VARIANT varChildID, IDispatch **ppdispChild)
{
    Client *This = impl_from_Client(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_variant(&varChildID), ppdispChild);

    *ppdispChild = NULL;
    return E_INVALIDARG;
}

static HRESULT WINAPI Client_get_accName(IAccessible *iface, VARIANT varID, BSTR *pszName)
{
    Client *This = impl_from_Client(iface);
    WCHAR name[1024];
    UINT i, len;

    TRACE("(%p)->(%s %p)\n", This, debugstr_variant(&varID), pszName);

    *pszName = NULL;
    if(convert_child_id(&varID) != CHILDID_SELF || !IsWindow(This->hwnd))
        return E_INVALIDARG;

    len = SendMessageW(This->hwnd, WM_GETTEXT, sizeof(name)/sizeof(WCHAR), (LPARAM)name);
    if(!len)
        return S_FALSE;

    for(i=0; i<len; i++) {
        if(name[i] == '&') {
            len--;
            memmove(name+i, name+i+1, (len-i)*sizeof(WCHAR));
            break;
        }
    }

    *pszName = SysAllocStringLen(name, len);
    return *pszName ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI Client_get_accValue(IAccessible *iface, VARIANT varID, BSTR *pszValue)
{
    Client *This = impl_from_Client(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_variant(&varID), pszValue);

    *pszValue = NULL;
    if(convert_child_id(&varID) != CHILDID_SELF)
        return E_INVALIDARG;
    return S_FALSE;
}

static HRESULT WINAPI Client_get_accDescription(IAccessible *iface,
        VARIANT varID, BSTR *pszDescription)
{
    Client *This = impl_from_Client(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_variant(&varID), pszDescription);

    *pszDescription = NULL;
    if(convert_child_id(&varID) != CHILDID_SELF)
        return E_INVALIDARG;
    return S_FALSE;
}

static HRESULT WINAPI Client_get_accRole(IAccessible *iface, VARIANT varID, VARIANT *pvarRole)
{
    Client *This = impl_from_Client(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_variant(&varID), pvarRole);

    if(convert_child_id(&varID) != CHILDID_SELF) {
        V_VT(pvarRole) = VT_EMPTY;
        return E_INVALIDARG;
    }

    V_VT(pvarRole) = VT_I4;
    V_I4(pvarRole) = ROLE_SYSTEM_CLIENT;
    return S_OK;
}

static HRESULT WINAPI Client_get_accState(IAccessible *iface, VARIANT varID, VARIANT *pvarState)
{
    Client *This = impl_from_Client(iface);
    LONG style;

    TRACE("(%p)->(%s %p)\n", This, debugstr_variant(&varID), pvarState);

    if(convert_child_id(&varID) != CHILDID_SELF) {
        V_VT(pvarState) = VT_EMPTY;
        return E_INVALIDARG;
    }

    V_VT(pvarState) = VT_I4;
    V_I4(pvarState) = 0;

    style = GetWindowLongW(This->hwnd, GWL_STYLE);
    if(style & WS_DISABLED)
        V_I4(pvarState) |= STATE_SYSTEM_UNAVAILABLE;
    else if(IsWindow(This->hwnd))
        V_I4(pvarState) |= STATE_SYSTEM_FOCUSABLE;
    if(GetFocus() == This->hwnd)
        V_I4(pvarState) |= STATE_SYSTEM_FOCUSED;
    if(!(style & WS_VISIBLE))
        V_I4(pvarState) |= STATE_SYSTEM_INVISIBLE;
    return S_OK;
}

static HRESULT WINAPI Client_get_accHelp(IAccessible *iface, VARIANT varID, BSTR *pszHelp)
{
    Client *This = impl_from_Client(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_variant(&varID), pszHelp);

    *pszHelp = NULL;
    if(convert_child_id(&varID) != CHILDID_SELF)
        return E_INVALIDARG;
    return S_FALSE;
}

static HRESULT WINAPI Client_get_accHelpTopic(IAccessible *iface,
        BSTR *pszHelpFile, VARIANT varID, LONG *pidTopic)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%p %s %p)\n", This, pszHelpFile, debugstr_variant(&varID), pidTopic);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_get_accKeyboardShortcut(IAccessible *iface,
        VARIANT varID, BSTR *pszKeyboardShortcut)
{
    static const WCHAR shortcut_fmt[] = {'A','l','t','+','!',0};
    Client *This = impl_from_Client(iface);
    WCHAR name[1024];
    UINT i, len;

    TRACE("(%p)->(%s %p)\n", This, debugstr_variant(&varID), pszKeyboardShortcut);

    *pszKeyboardShortcut = NULL;
    if(convert_child_id(&varID) != CHILDID_SELF)
        return E_INVALIDARG;

    len = SendMessageW(This->hwnd, WM_GETTEXT, sizeof(name)/sizeof(WCHAR), (LPARAM)name);
    for(i=0; i<len; i++) {
        if(name[i] == '&')
            break;
    }
    if(i+1 >= len)
        return S_FALSE;

    *pszKeyboardShortcut = SysAllocString(shortcut_fmt);
    if(!*pszKeyboardShortcut)
        return E_OUTOFMEMORY;

    (*pszKeyboardShortcut)[4] = name[i+1];
    return S_OK;
}

static HRESULT WINAPI Client_get_accFocus(IAccessible *iface, VARIANT *pvarID)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%p)\n", This, pvarID);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_get_accSelection(IAccessible *iface, VARIANT *pvarID)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%p)\n", This, pvarID);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_get_accDefaultAction(IAccessible *iface,
        VARIANT varID, BSTR *pszDefaultAction)
{
    Client *This = impl_from_Client(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_variant(&varID), pszDefaultAction);

    *pszDefaultAction = NULL;
    if(convert_child_id(&varID) != CHILDID_SELF)
        return E_INVALIDARG;
    return S_FALSE;
}

static HRESULT WINAPI Client_accSelect(IAccessible *iface, LONG flagsSelect, VARIANT varID)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%x %s)\n", This, flagsSelect, debugstr_variant(&varID));
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_accLocation(IAccessible *iface, LONG *pxLeft,
        LONG *pyTop, LONG *pcxWidth, LONG *pcyHeight, VARIANT varID)
{
    Client *This = impl_from_Client(iface);
    RECT rect;
    POINT pt;

    TRACE("(%p)->(%p %p %p %p %s)\n", This, pxLeft, pyTop,
            pcxWidth, pcyHeight, debugstr_variant(&varID));

    *pxLeft = *pyTop = *pcxWidth = *pcyHeight = 0;
    if(convert_child_id(&varID) != CHILDID_SELF)
        return E_INVALIDARG;

    if(!GetClientRect(This->hwnd, &rect))
        return S_OK;

    pt.x = rect.left,
    pt.y = rect.top;
    MapWindowPoints(This->hwnd, NULL, &pt, 1);
    *pxLeft = pt.x;
    *pyTop = pt.y;

    pt.x = rect.right;
    pt.y = rect.bottom;
    MapWindowPoints(This->hwnd, NULL, &pt, 1);
    *pcxWidth = pt.x - *pxLeft;
    *pcyHeight = pt.y - *pyTop;
    return S_OK;
}

static HRESULT WINAPI Client_accNavigate(IAccessible *iface,
        LONG navDir, VARIANT varStart, VARIANT *pvarEnd)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%d %s %p)\n", This, navDir, debugstr_variant(&varStart), pvarEnd);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_accHitTest(IAccessible *iface,
        LONG xLeft, LONG yTop, VARIANT *pvarID)
{
    Client *This = impl_from_Client(iface);
    HWND child;
    POINT pt;

    TRACE("(%p)->(%d %d %p)\n", This, xLeft, yTop, pvarID);

    V_VT(pvarID) = VT_I4;
    V_I4(pvarID) = 0;

    pt.x = xLeft;
    pt.y = yTop;
    if(!IsWindowVisible(This->hwnd) || !ScreenToClient(This->hwnd, &pt))
        return S_OK;

    child = ChildWindowFromPointEx(This->hwnd, pt, CWP_SKIPINVISIBLE);
    if(!child || child==This->hwnd)
        return S_OK;

    V_VT(pvarID) = VT_DISPATCH;
    return AccessibleObjectFromWindow(child, OBJID_WINDOW,
            &IID_IDispatch, (void**)&V_DISPATCH(pvarID));
}

static HRESULT WINAPI Client_accDoDefaultAction(IAccessible *iface, VARIANT varID)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&varID));
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_put_accName(IAccessible *iface, VARIANT varID, BSTR pszName)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_variant(&varID), debugstr_w(pszName));
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_put_accValue(IAccessible *iface, VARIANT varID, BSTR pszValue)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_variant(&varID), debugstr_w(pszValue));
    return E_NOTIMPL;
}

static const IAccessibleVtbl ClientVtbl = {
    Client_QueryInterface,
    Client_AddRef,
    Client_Release,
    Client_GetTypeInfoCount,
    Client_GetTypeInfo,
    Client_GetIDsOfNames,
    Client_Invoke,
    Client_get_accParent,
    Client_get_accChildCount,
    Client_get_accChild,
    Client_get_accName,
    Client_get_accValue,
    Client_get_accDescription,
    Client_get_accRole,
    Client_get_accState,
    Client_get_accHelp,
    Client_get_accHelpTopic,
    Client_get_accKeyboardShortcut,
    Client_get_accFocus,
    Client_get_accSelection,
    Client_get_accDefaultAction,
    Client_accSelect,
    Client_accLocation,
    Client_accNavigate,
    Client_accHitTest,
    Client_accDoDefaultAction,
    Client_put_accName,
    Client_put_accValue
};

static inline Client* impl_from_Client_OleWindow(IOleWindow *iface)
{
    return CONTAINING_RECORD(iface, Client, IOleWindow_iface);
}

static HRESULT WINAPI Client_OleWindow_QueryInterface(IOleWindow *iface, REFIID riid, void **ppv)
{
    Client *This = impl_from_Client_OleWindow(iface);
    return IAccessible_QueryInterface(&This->IAccessible_iface, riid, ppv);
}

static ULONG WINAPI Client_OleWindow_AddRef(IOleWindow *iface)
{
    Client *This = impl_from_Client_OleWindow(iface);
    return IAccessible_AddRef(&This->IAccessible_iface);
}

static ULONG WINAPI Client_OleWindow_Release(IOleWindow *iface)
{
    Client *This = impl_from_Client_OleWindow(iface);
    return IAccessible_Release(&This->IAccessible_iface);
}

static HRESULT WINAPI Client_OleWindow_GetWindow(IOleWindow *iface, HWND *phwnd)
{
    Client *This = impl_from_Client_OleWindow(iface);

    TRACE("(%p)->(%p)\n", This, phwnd);

    *phwnd = This->hwnd;
    return S_OK;
}

static HRESULT WINAPI Client_OleWindow_ContextSensitiveHelp(IOleWindow *iface, BOOL fEnterMode)
{
    Client *This = impl_from_Client_OleWindow(iface);
    FIXME("(%p)->(%x)\n", This, fEnterMode);
    return E_NOTIMPL;
}

static const IOleWindowVtbl ClientOleWindowVtbl = {
    Client_OleWindow_QueryInterface,
    Client_OleWindow_AddRef,
    Client_OleWindow_Release,
    Client_OleWindow_GetWindow,
    Client_OleWindow_ContextSensitiveHelp
};

static inline Client* impl_from_Client_EnumVARIANT(IEnumVARIANT *iface)
{
    return CONTAINING_RECORD(iface, Client, IEnumVARIANT_iface);
}

static HRESULT WINAPI Client_EnumVARIANT_QueryInterface(IEnumVARIANT *iface, REFIID riid, void **ppv)
{
    Client *This = impl_from_Client_EnumVARIANT(iface);
    return IAccessible_QueryInterface(&This->IAccessible_iface, riid, ppv);
}

static ULONG WINAPI Client_EnumVARIANT_AddRef(IEnumVARIANT *iface)
{
    Client *This = impl_from_Client_EnumVARIANT(iface);
    return IAccessible_AddRef(&This->IAccessible_iface);
}

static ULONG WINAPI Client_EnumVARIANT_Release(IEnumVARIANT *iface)
{
    Client *This = impl_from_Client_EnumVARIANT(iface);
    return IAccessible_Release(&This->IAccessible_iface);
}

static HRESULT WINAPI Client_EnumVARIANT_Next(IEnumVARIANT *iface,
        ULONG celt, VARIANT *rgVar, ULONG *pCeltFetched)
{
    Client *This = impl_from_Client_EnumVARIANT(iface);
    HWND cur = This->enum_pos, next;
    ULONG fetched = 0;
    HRESULT hr;

    TRACE("(%p)->(%u %p %p)\n", This, celt, rgVar, pCeltFetched);

    if(!celt) {
        if(pCeltFetched)
            *pCeltFetched = 0;
        return S_OK;
    }

    if(!This->enum_pos)
        next = GetWindow(This->hwnd, GW_CHILD);
    else
        next = GetWindow(This->enum_pos, GW_HWNDNEXT);

    while(next) {
        cur = next;

        V_VT(rgVar+fetched) = VT_DISPATCH;
        hr = AccessibleObjectFromWindow(cur, OBJID_WINDOW,
                &IID_IDispatch, (void**)&V_DISPATCH(rgVar+fetched));
        if(FAILED(hr)) {
            V_VT(rgVar+fetched) = VT_EMPTY;
            while(fetched > 0) {
                VariantClear(rgVar+fetched-1);
                fetched--;
            }
            if(pCeltFetched)
                *pCeltFetched = 0;
            return hr;
        }
        fetched++;
        if(fetched == celt)
            break;

        next = GetWindow(cur, GW_HWNDNEXT);
    }

    This->enum_pos = cur;
    if(pCeltFetched)
        *pCeltFetched = fetched;
    return celt == fetched ? S_OK : S_FALSE;
}

static HRESULT WINAPI Client_EnumVARIANT_Skip(IEnumVARIANT *iface, ULONG celt)
{
    Client *This = impl_from_Client_EnumVARIANT(iface);
    HWND next;

    TRACE("(%p)->(%u)\n", This, celt);

    while(celt) {
        if(!This->enum_pos)
            next = GetWindow(This->hwnd, GW_CHILD);
        else
            next = GetWindow(This->enum_pos, GW_HWNDNEXT);
        if(!next)
            return S_FALSE;

        This->enum_pos = next;
        celt--;
    }

    return S_OK;
}

static HRESULT WINAPI Client_EnumVARIANT_Reset(IEnumVARIANT *iface)
{
    Client *This = impl_from_Client_EnumVARIANT(iface);

    TRACE("(%p)\n", This);

    This->enum_pos = 0;
    return S_OK;
}

static HRESULT WINAPI Client_EnumVARIANT_Clone(IEnumVARIANT *iface, IEnumVARIANT **ppEnum)
{
    Client *This = impl_from_Client_EnumVARIANT(iface);
    FIXME("(%p)->(%p)\n", This, ppEnum);
    return E_NOTIMPL;
}

static const IEnumVARIANTVtbl ClientEnumVARIANTVtbl = {
    Client_EnumVARIANT_QueryInterface,
    Client_EnumVARIANT_AddRef,
    Client_EnumVARIANT_Release,
    Client_EnumVARIANT_Next,
    Client_EnumVARIANT_Skip,
    Client_EnumVARIANT_Reset,
    Client_EnumVARIANT_Clone
};

HRESULT create_client_object(HWND hwnd, const IID *iid, void **obj)
{
    Client *client;
    HRESULT hres;

    if(!IsWindow(hwnd))
        return E_FAIL;

    client = heap_alloc_zero(sizeof(Client));
    if(!client)
        return E_OUTOFMEMORY;

    client->IAccessible_iface.lpVtbl = &ClientVtbl;
    client->IOleWindow_iface.lpVtbl = &ClientOleWindowVtbl;
    client->IEnumVARIANT_iface.lpVtbl = &ClientEnumVARIANTVtbl;
    client->ref = 1;
    client->hwnd = hwnd;
    client->enum_pos = 0;

    hres = IAccessible_QueryInterface(&client->IAccessible_iface, iid, obj);
    IAccessible_Release(&client->IAccessible_iface);
    return hres;
}
