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

#define COBJMACROS

#include <assert.h>
#include "oleacc_private.h"
#include "commctrl.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(oleacc);

typedef struct win_class_vtbl win_class_vtbl;
typedef struct {
    IAccessible IAccessible_iface;
    IOleWindow IOleWindow_iface;
    IEnumVARIANT IEnumVARIANT_iface;
    IServiceProvider IServiceProvider_iface;

    LONG ref;

    HWND hwnd;
    HWND enum_pos;
    INT role;

    const win_class_vtbl *vtbl;
} Client;

struct win_class_vtbl {
    void (*init)(Client*);
    HRESULT (*get_state)(Client*, VARIANT, VARIANT*);
    HRESULT (*get_name)(Client*, VARIANT, BSTR*);
    HRESULT (*get_kbd_shortcut)(Client*, VARIANT, BSTR*);
    HRESULT (*get_value)(Client*, VARIANT, BSTR*);
    HRESULT (*put_value)(Client*, VARIANT, BSTR);
};

static HRESULT win_get_name(HWND hwnd, BSTR *name)
{
    WCHAR buf[1024];
    UINT i, len;

    len = SendMessageW(hwnd, WM_GETTEXT, ARRAY_SIZE(buf), (LPARAM)buf);
    if(!len)
        return S_FALSE;

    for(i=0; i<len; i++) {
        if(buf[i] == '&') {
            len--;
            memmove(buf+i, buf+i+1, (len-i)*sizeof(WCHAR));
            break;
        }
    }

    *name = SysAllocStringLen(buf, len);
    return *name ? S_OK : E_OUTOFMEMORY;
}

static HRESULT win_get_kbd_shortcut(HWND hwnd, BSTR *shortcut)
{
    WCHAR buf[1024];
    UINT i, len;

    len = SendMessageW(hwnd, WM_GETTEXT, ARRAY_SIZE(buf), (LPARAM)buf);
    if(!len)
        return S_FALSE;

    for(i=0; i<len; i++) {
        if(buf[i] == '&')
            break;
    }
    if(i+1 >= len)
        return S_FALSE;

    *shortcut = SysAllocString(L"Alt+!");
    if(!*shortcut)
        return E_OUTOFMEMORY;
    (*shortcut)[4] = buf[i+1];
    return S_OK;
}

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
    }else if(IsEqualIID(riid, &IID_IServiceProvider)) {
        *ppv = &This->IServiceProvider_iface;
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

    TRACE("(%p) ref = %lu\n", This, ref);
    return ref;
}

static ULONG WINAPI Client_Release(IAccessible *iface)
{
    Client *This = impl_from_Client(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref = %lu\n", This, ref);

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
    FIXME("(%p)->(%u %lx %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_GetIDsOfNames(IAccessible *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%s %p %u %lx %p)\n", This, debugstr_guid(riid),
            rgszNames, cNames, lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_Invoke(IAccessible *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%lx %s %lx %x %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
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

static HRESULT WINAPI Client_get_accName(IAccessible *iface, VARIANT id, BSTR *name)
{
    Client *This = impl_from_Client(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_variant(&id), name);

    *name = NULL;
    if(This->vtbl && This->vtbl->get_name)
        return This->vtbl->get_name(This, id, name);

    if(convert_child_id(&id) != CHILDID_SELF || !IsWindow(This->hwnd))
        return E_INVALIDARG;

    return win_get_name(This->hwnd, name);
}

static HRESULT WINAPI Client_get_accValue(IAccessible *iface, VARIANT id, BSTR *value)
{
    Client *This = impl_from_Client(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_variant(&id), value);

    *value = NULL;
    if(This->vtbl && This->vtbl->get_value)
        return This->vtbl->get_value(This, id, value);

    if(convert_child_id(&id) != CHILDID_SELF)
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
    V_I4(pvarRole) = This->role;
    return S_OK;
}

static HRESULT client_get_state(Client *client, VARIANT id, VARIANT *state)
{
    GUITHREADINFO info;
    LONG style;

    if(convert_child_id(&id) != CHILDID_SELF) {
        V_VT(state) = VT_EMPTY;
        return E_INVALIDARG;
    }

    V_VT(state) = VT_I4;
    V_I4(state) = 0;

    style = GetWindowLongW(client->hwnd, GWL_STYLE);
    if(style & WS_DISABLED)
        V_I4(state) |= STATE_SYSTEM_UNAVAILABLE;
    else if(IsWindow(client->hwnd))
        V_I4(state) |= STATE_SYSTEM_FOCUSABLE;

    info.cbSize = sizeof(info);
    if(GetGUIThreadInfo(0, &info) && info.hwndFocus == client->hwnd)
        V_I4(state) |= STATE_SYSTEM_FOCUSED;
    if(!(style & WS_VISIBLE))
        V_I4(state) |= STATE_SYSTEM_INVISIBLE;
    return S_OK;
}

static HRESULT WINAPI Client_get_accState(IAccessible *iface, VARIANT id, VARIANT *state)
{
    Client *This = impl_from_Client(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_variant(&id), state);

    if(This->vtbl && This->vtbl->get_state)
        return This->vtbl->get_state(This, id, state);
    return client_get_state(This, id, state);
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
        VARIANT id, BSTR *shortcut)
{
    Client *This = impl_from_Client(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_variant(&id), shortcut);

    *shortcut = NULL;
    if(This->vtbl && This->vtbl->get_kbd_shortcut)
        return This->vtbl->get_kbd_shortcut(This, id, shortcut);

    if(convert_child_id(&id) != CHILDID_SELF)
        return E_INVALIDARG;

    return win_get_kbd_shortcut(This->hwnd, shortcut);
}

static HRESULT WINAPI Client_get_accFocus(IAccessible *iface, VARIANT *focus)
{
    Client *This = impl_from_Client(iface);
    GUITHREADINFO info;

    TRACE("(%p)->(%p)\n", This, focus);

    V_VT(focus) = VT_EMPTY;
    info.cbSize = sizeof(info);
    if(GetGUIThreadInfo(0, &info) && info.hwndFocus) {
        if(info.hwndFocus == This->hwnd) {
            V_VT(focus) = VT_I4;
            V_I4(focus) = CHILDID_SELF;
        }
        else if(IsChild(This->hwnd, info.hwndFocus)) {
            IDispatch *disp;
            HRESULT hr;

            hr = AccessibleObjectFromWindow(info.hwndFocus, OBJID_WINDOW,
                    &IID_IDispatch, (void**)&disp);
            if(FAILED(hr))
                return hr;
            if(!disp)
                return E_FAIL;

            V_VT(focus) = VT_DISPATCH;
            V_DISPATCH(focus) = disp;
        }
    }

    return S_OK;
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
    FIXME("(%p)->(%lx %s)\n", This, flagsSelect, debugstr_variant(&varID));
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

    pt.x = rect.left;
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
    FIXME("(%p)->(%ld %s %p)\n", This, navDir, debugstr_variant(&varStart), pvarEnd);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_accHitTest(IAccessible *iface,
        LONG xLeft, LONG yTop, VARIANT *pvarID)
{
    Client *This = impl_from_Client(iface);
    HWND child;
    POINT pt;

    TRACE("(%p)->(%ld %ld %p)\n", This, xLeft, yTop, pvarID);

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

static HRESULT WINAPI Client_put_accValue(IAccessible *iface, VARIANT id, BSTR value)
{
    Client *This = impl_from_Client(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_variant(&id), value);

    if(This->vtbl && This->vtbl->put_value)
        return This->vtbl->put_value(This, id, value);

    if(convert_child_id(&id) != CHILDID_SELF)
        return E_INVALIDARG;
    return S_FALSE;
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

    TRACE("(%p)->(%lu %p %p)\n", This, celt, rgVar, pCeltFetched);

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

    TRACE("(%p)->(%lu)\n", This, celt);

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

static inline Client* impl_from_Client_ServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, Client, IServiceProvider_iface);
}

static HRESULT WINAPI Client_ServiceProvider_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    Client *This = impl_from_Client_ServiceProvider(iface);
    return IAccessible_QueryInterface(&This->IAccessible_iface, riid, ppv);
}

static ULONG WINAPI Client_ServiceProvider_AddRef(IServiceProvider *iface)
{
    Client *This = impl_from_Client_ServiceProvider(iface);
    return IAccessible_AddRef(&This->IAccessible_iface);
}

static ULONG WINAPI Client_ServiceProvider_Release(IServiceProvider *iface)
{
    Client *This = impl_from_Client_ServiceProvider(iface);
    return IAccessible_Release(&This->IAccessible_iface);
}

static HRESULT WINAPI Client_ServiceProvider_QueryService(IServiceProvider *iface, REFGUID guid_service,
        REFIID riid, void **ppv)
{
    Client *This = impl_from_Client_ServiceProvider(iface);

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_guid(guid_service), debugstr_guid(riid), ppv);

    *ppv = NULL;
    if (IsEqualIID(guid_service, &IIS_IsOleaccProxy))
        return IAccessible_QueryInterface(&This->IAccessible_iface, riid, ppv);

    return E_INVALIDARG;
}

static const IServiceProviderVtbl ClientServiceProviderVtbl = {
    Client_ServiceProvider_QueryInterface,
    Client_ServiceProvider_AddRef,
    Client_ServiceProvider_Release,
    Client_ServiceProvider_QueryService
};

static void edit_init(Client *client)
{
    client->role = ROLE_SYSTEM_TEXT;
}

static HRESULT edit_get_state(Client *client, VARIANT id, VARIANT *state)
{
    HRESULT hres;
    LONG style;

    hres = client_get_state(client, id, state);
    if(FAILED(hres))
        return hres;

    assert(V_VT(state) == VT_I4);

    style = GetWindowLongW(client->hwnd, GWL_STYLE);
    if(style & ES_READONLY)
        V_I4(state) |= STATE_SYSTEM_READONLY;
    if(style & ES_PASSWORD)
        V_I4(state) |= STATE_SYSTEM_PROTECTED;
    return S_OK;
}

/*
 * Edit control objects have their name property defined by the first static
 * text control preceding them in the order of window creation. If one is not
 * found, the edit has no name property. In the case of the keyboard shortcut
 * property, the first preceding visible static text control is used.
 */
static HWND edit_find_label(HWND hwnd, BOOL visible)
{
    HWND cur;

    for(cur = hwnd; cur; cur = GetWindow(cur, GW_HWNDPREV)) {
        WCHAR class_name[64];

        if(!RealGetWindowClassW(cur, class_name, ARRAY_SIZE(class_name)))
            continue;

        if(!wcsicmp(class_name, WC_STATICW)) {
            if(visible && !(GetWindowLongW(cur, GWL_STYLE) & WS_VISIBLE))
                continue;
            else
                break;
        }
    }

    return cur;
}

static HRESULT edit_get_name(Client *client, VARIANT id, BSTR *name)
{
    HWND label;

    if(convert_child_id(&id) != CHILDID_SELF || !IsWindow(client->hwnd))
        return E_INVALIDARG;

    label = edit_find_label(client->hwnd, FALSE);
    if(!label)
        return S_FALSE;

    return win_get_name(label, name);
}

static HRESULT edit_get_kbd_shortcut(Client *client, VARIANT id, BSTR *shortcut)
{
    HWND label;

    if(convert_child_id(&id) != CHILDID_SELF)
        return E_INVALIDARG;

    label = edit_find_label(client->hwnd, TRUE);
    if(!label)
        return S_FALSE;

    return win_get_kbd_shortcut(label, shortcut);
}

static HRESULT edit_get_value(Client *client, VARIANT id, BSTR *value_out)
{
    WCHAR *buf;
    UINT len;

    if(convert_child_id(&id) != CHILDID_SELF)
        return E_INVALIDARG;

    if(GetWindowLongW(client->hwnd, GWL_STYLE) & ES_PASSWORD)
        return E_ACCESSDENIED;

    len = SendMessageW(client->hwnd, WM_GETTEXTLENGTH, 0, 0);
    buf = heap_alloc_zero((len + 1) * sizeof(*buf));
    if(!buf)
        return E_OUTOFMEMORY;

    SendMessageW(client->hwnd, WM_GETTEXT, len + 1, (LPARAM)buf);
    *value_out = SysAllocString(buf);
    heap_free(buf);
    return S_OK;
}

static HRESULT edit_put_value(Client *client, VARIANT id, BSTR value)
{
    if(convert_child_id(&id) != CHILDID_SELF || !IsWindow(client->hwnd))
        return E_INVALIDARG;

    SendMessageW(client->hwnd, WM_SETTEXT, 0, (LPARAM)value);
    return S_OK;
}

static const win_class_vtbl edit_vtbl = {
    edit_init,
    edit_get_state,
    edit_get_name,
    edit_get_kbd_shortcut,
    edit_get_value,
    edit_put_value,
};

static const struct win_class_data classes[] = {
    {WC_LISTBOXW,           0x10000, TRUE},
    {L"#32768",             0x10001, TRUE}, /* menu */
    {WC_BUTTONW,            0x10002, TRUE},
    {WC_STATICW,            0x10003, TRUE},
    {WC_EDITW,              0x10004, FALSE, &edit_vtbl},
    {WC_COMBOBOXW,          0x10005, TRUE},
    {L"#32770",             0x10006, TRUE}, /* dialog */
    {L"#32771",             0x10007, TRUE}, /* winswitcher */
    {L"MDIClient",          0x10008, TRUE},
    {L"#32769",             0x10009, TRUE}, /* desktop */
    {WC_SCROLLBARW,         0x1000a, TRUE},
    {STATUSCLASSNAMEW,      0x1000b, TRUE},
    {TOOLBARCLASSNAMEW,     0x1000c, TRUE},
    {PROGRESS_CLASSW,       0x1000d, TRUE},
    {ANIMATE_CLASSW,        0x1000e, TRUE},
    {WC_TABCONTROLW,        0x1000f, TRUE},
    {HOTKEY_CLASSW,         0x10010, TRUE},
    {WC_HEADERW,            0x10011, TRUE},
    {TRACKBAR_CLASSW,       0x10012, TRUE},
    {WC_LISTVIEWW,          0x10013, TRUE},
    {UPDOWN_CLASSW,         0x10016, TRUE},
    {TOOLTIPS_CLASSW,       0x10018, TRUE},
    {WC_TREEVIEWW,          0x10019, TRUE},
    {DATETIMEPICK_CLASSW,   0, TRUE},
    {WC_IPADDRESSW,         0, TRUE},
    {L"RICHEDIT",           0x1001c, TRUE},
    {L"RichEdit20A",        0, TRUE},
    {L"RichEdit20W",        0, TRUE},
    {NULL}
};

HRESULT create_client_object(HWND hwnd, const IID *iid, void **obj)
{
    const struct win_class_data *data;
    Client *client;
    HRESULT hres = S_OK;

    if(!IsWindow(hwnd))
        return E_FAIL;

    client = heap_alloc_zero(sizeof(Client));
    if(!client)
        return E_OUTOFMEMORY;

    data = find_class_data(hwnd, classes);

    client->IAccessible_iface.lpVtbl = &ClientVtbl;
    client->IOleWindow_iface.lpVtbl = &ClientOleWindowVtbl;
    client->IEnumVARIANT_iface.lpVtbl = &ClientEnumVARIANTVtbl;
    client->IServiceProvider_iface.lpVtbl = &ClientServiceProviderVtbl;
    client->ref = 1;
    client->hwnd = hwnd;
    client->enum_pos = 0;
    client->role = ROLE_SYSTEM_CLIENT;

    if(data)
        client->vtbl = data->vtbl;
    if(client->vtbl && client->vtbl->init)
        client->vtbl->init(client);

    hres = IAccessible_QueryInterface(&client->IAccessible_iface, iid, obj);
    IAccessible_Release(&client->IAccessible_iface);
    return hres;
}
