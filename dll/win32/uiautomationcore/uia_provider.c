/*
 * Copyright 2022 Connor McAdams for CodeWeavers
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

#include "uia_private.h"
#include "ocidl.h"

#include "wine/debug.h"
#include "initguid.h"
#include "wine/iaccessible2.h"

WINE_DEFAULT_DEBUG_CHANNEL(uiautomation);

DEFINE_GUID(SID_AccFromDAWrapper, 0x33f139ee, 0xe509, 0x47f7, 0xbf,0x39, 0x83,0x76,0x44,0xf7,0x45,0x76);

/* Returns S_OK if flag is set, S_FALSE if it is not. */
static HRESULT msaa_check_acc_state_hres(IAccessible *acc, VARIANT cid, ULONG flag)
{
    HRESULT hr;
    VARIANT v;

    VariantInit(&v);
    hr = IAccessible_get_accState(acc, cid, &v);
    if (SUCCEEDED(hr))
        hr = ((V_VT(&v) == VT_I4) && (V_I4(&v) & flag)) ? S_OK : S_FALSE;

    return hr;
}

static BOOL msaa_check_acc_state(IAccessible *acc, VARIANT cid, ULONG flag)
{
    return msaa_check_acc_state_hres(acc, cid, flag) == S_OK;
}

static HRESULT msaa_acc_get_service(IAccessible *acc, REFGUID sid, REFIID riid, void **service)
{
    IServiceProvider *sp;
    HRESULT hr;

    *service = NULL;
    hr = IAccessible_QueryInterface(acc, &IID_IServiceProvider, (void **)&sp);
    if (FAILED(hr))
        return hr;

    hr = IServiceProvider_QueryService(sp, sid, riid, (void **)service);
    IServiceProvider_Release(sp);
    return hr;
}

static IAccessible2 *msaa_acc_get_ia2(IAccessible *acc)
{
    IAccessible2 *ia2 = NULL;
    HRESULT hr;

    hr = msaa_acc_get_service(acc, &IID_IAccessible2, &IID_IAccessible2, (void **)&ia2);
    if (SUCCEEDED(hr) && ia2)
        return ia2;

    hr = IAccessible_QueryInterface(acc, &IID_IAccessible2, (void **)&ia2);
    if (SUCCEEDED(hr) && ia2)
        return ia2;

    return NULL;
}

static IAccessible *msaa_acc_da_unwrap(IAccessible *acc)
{
    IAccessible *acc2;
    HRESULT hr;

    hr = msaa_acc_get_service(acc, &SID_AccFromDAWrapper, &IID_IAccessible, (void **)&acc2);
    if (SUCCEEDED(hr) && acc2)
        return acc2;

    IAccessible_AddRef(acc);
    return acc;
}

static BOOL msaa_acc_is_oleacc_proxy(IAccessible *acc)
{
    IUnknown *unk;
    HRESULT hr;

    hr = msaa_acc_get_service(acc, &IIS_IsOleaccProxy, &IID_IUnknown, (void **)&unk);
    if (SUCCEEDED(hr) && unk)
    {
        IUnknown_Release(unk);
        return TRUE;
    }

    return FALSE;
}

/*
 * Compare role, state, child count, and location properties of the two
 * IAccessibles. If all four are successfully retrieved and are equal, this is
 * considered a match.
 */
static HRESULT msaa_acc_prop_match(IAccessible *acc, IAccessible *acc2)
{
    BOOL role_match, state_match, child_count_match, location_match;
    LONG child_count[2], left[2], top[2], width[2], height[2];
    VARIANT cid, v, v2;
    HRESULT hr, hr2;

    role_match = state_match = child_count_match = location_match = FALSE;
    variant_init_i4(&cid, CHILDID_SELF);
    hr = IAccessible_get_accRole(acc, cid, &v);
    if (SUCCEEDED(hr) && (V_VT(&v) == VT_I4))
    {
        VariantInit(&v2);
        hr = IAccessible_get_accRole(acc2, cid, &v2);
        if (SUCCEEDED(hr) && (V_VT(&v2) == VT_I4))
        {
            if (V_I4(&v) != V_I4(&v2))
                return E_FAIL;

            role_match = TRUE;
        }
    }

    VariantInit(&v);
    hr = IAccessible_get_accState(acc, cid, &v);
    if (SUCCEEDED(hr) && (V_VT(&v) == VT_I4))
    {
        VariantInit(&v2);
        hr = IAccessible_get_accState(acc2, cid, &v2);
        if (SUCCEEDED(hr) && (V_VT(&v2) == VT_I4))
        {
            if (V_I4(&v) != V_I4(&v2))
                return E_FAIL;

            state_match = TRUE;
        }
    }

    hr = IAccessible_get_accChildCount(acc, &child_count[0]);
    hr2 = IAccessible_get_accChildCount(acc2, &child_count[1]);
    if (SUCCEEDED(hr) && SUCCEEDED(hr2))
    {
        if (child_count[0] != child_count[1])
            return E_FAIL;

        child_count_match = TRUE;
    }

    hr = IAccessible_accLocation(acc, &left[0], &top[0], &width[0], &height[0], cid);
    if (SUCCEEDED(hr))
    {
        hr = IAccessible_accLocation(acc2, &left[1], &top[1], &width[1], &height[1], cid);
        if (SUCCEEDED(hr))
        {
            if ((left[0] != left[1]) || (top[0] != top[1]) || (width[0] != width[1]) ||
                    (height[0] != height[1]))
                return E_FAIL;

            location_match = TRUE;
        }
    }

    if (role_match && state_match && child_count_match && location_match)
        return S_OK;

    return S_FALSE;
}

static BOOL msaa_acc_iface_cmp(IAccessible *acc, IAccessible *acc2)
{
    IUnknown *unk, *unk2;
    BOOL matched;

    acc = msaa_acc_da_unwrap(acc);
    acc2 = msaa_acc_da_unwrap(acc2);
    IAccessible_QueryInterface(acc, &IID_IUnknown, (void**)&unk);
    IAccessible_QueryInterface(acc2, &IID_IUnknown, (void**)&unk2);
    matched = (unk == unk2);

    IAccessible_Release(acc);
    IUnknown_Release(unk);
    IAccessible_Release(acc2);
    IUnknown_Release(unk2);

    return matched;
}

static BOOL msaa_acc_compare(IAccessible *acc, IAccessible *acc2)
{
    IAccessible2 *ia2[2] = { NULL, NULL };
    IUnknown *unk, *unk2;
    BOOL matched = FALSE;
    LONG unique_id[2];
    BSTR name[2];
    VARIANT cid;
    HRESULT hr;

    acc = msaa_acc_da_unwrap(acc);
    acc2 = msaa_acc_da_unwrap(acc2);
    IAccessible_QueryInterface(acc, &IID_IUnknown, (void**)&unk);
    IAccessible_QueryInterface(acc2, &IID_IUnknown, (void**)&unk2);
    if (unk == unk2)
    {
        matched = TRUE;
        goto exit;
    }

    ia2[0] = msaa_acc_get_ia2(acc);
    ia2[1] = msaa_acc_get_ia2(acc2);
    if (!ia2[0] != !ia2[1])
        goto exit;
    if (ia2[0])
    {
        hr = IAccessible2_get_uniqueID(ia2[0], &unique_id[0]);
        if (SUCCEEDED(hr))
        {
            hr = IAccessible2_get_uniqueID(ia2[1], &unique_id[1]);
            if (SUCCEEDED(hr))
            {
                if (unique_id[0] == unique_id[1])
                    matched = TRUE;

                goto exit;
            }
        }
    }

    hr = msaa_acc_prop_match(acc, acc2);
    if (FAILED(hr))
        goto exit;
    if (hr == S_OK)
        matched = TRUE;

    variant_init_i4(&cid, CHILDID_SELF);
    hr = IAccessible_get_accName(acc, cid, &name[0]);
    if (SUCCEEDED(hr))
    {
        hr = IAccessible_get_accName(acc2, cid, &name[1]);
        if (SUCCEEDED(hr))
        {
            if (!name[0] && !name[1])
                matched = TRUE;
            else if (!name[0] || !name[1])
                matched = FALSE;
            else
            {
                if (!wcscmp(name[0], name[1]))
                    matched = TRUE;
                else
                    matched = FALSE;
            }

            SysFreeString(name[1]);
        }

        SysFreeString(name[0]);
    }

exit:
    IUnknown_Release(unk);
    IUnknown_Release(unk2);
    IAccessible_Release(acc);
    IAccessible_Release(acc2);
    if (ia2[0])
        IAccessible2_Release(ia2[0]);
    if (ia2[1])
        IAccessible2_Release(ia2[1]);

    return matched;
}

static HRESULT msaa_acc_get_parent(IAccessible *acc, IAccessible **parent)
{
    IDispatch *disp = NULL;
    HRESULT hr;

    *parent = NULL;
    hr = IAccessible_get_accParent(acc, &disp);
    if (FAILED(hr) || !disp)
        return hr;

    hr = IDispatch_QueryInterface(disp, &IID_IAccessible, (void**)parent);
    IDispatch_Release(disp);
    return hr;
}

#define DIR_FORWARD 0
#define DIR_REVERSE 1
static HRESULT msaa_acc_get_next_child(IAccessible *acc, LONG start_pos, LONG direction,
        IAccessible **child, LONG *child_id, LONG *child_pos, BOOL check_visible)
{
    LONG child_count, cur_pos;
    IDispatch *disp;
    VARIANT cid;
    HRESULT hr;

    *child = NULL;
    *child_id = 0;
    cur_pos = start_pos;
    while (1)
    {
        hr = IAccessible_get_accChildCount(acc, &child_count);
        if (FAILED(hr) || (cur_pos > child_count))
            break;

        variant_init_i4(&cid, cur_pos);
        hr = IAccessible_get_accChild(acc, cid, &disp);
        if (FAILED(hr))
            break;

        if (hr == S_FALSE)
        {
            if (!check_visible || !msaa_check_acc_state(acc, cid, STATE_SYSTEM_INVISIBLE))
            {
                *child = acc;
                *child_id = *child_pos = cur_pos;
                IAccessible_AddRef(acc);
                return S_OK;
            }
        }
        else
        {
            IAccessible *acc_child = NULL;

            hr = IDispatch_QueryInterface(disp, &IID_IAccessible, (void **)&acc_child);
            IDispatch_Release(disp);
            if (FAILED(hr))
                break;

            variant_init_i4(&cid, CHILDID_SELF);
            if (!check_visible || !msaa_check_acc_state(acc_child, cid, STATE_SYSTEM_INVISIBLE))
            {
                *child = acc_child;
                *child_id = CHILDID_SELF;
                *child_pos = cur_pos;
                return S_OK;
            }

            IAccessible_Release(acc_child);
        }

        if (direction == DIR_FORWARD)
            cur_pos++;
        else
            cur_pos--;

        if ((cur_pos > child_count) || (cur_pos <= 0))
            break;
    }

    return hr;
}

static HRESULT msaa_acc_get_child_pos(IAccessible *acc, IAccessible **out_parent,
        LONG *out_child_pos)
{
    LONG child_count, child_id, child_pos, match_pos;
    IAccessible *child, *parent, *match, **children;
    HRESULT hr;
    int i;

    *out_parent = NULL;
    *out_child_pos = 0;
    hr = msaa_acc_get_parent(acc, &parent);
    if (FAILED(hr) || !parent)
        return hr;

    hr = IAccessible_get_accChildCount(parent, &child_count);
    if (FAILED(hr) || !child_count)
    {
        IAccessible_Release(parent);
        return hr;
    }

    children = calloc(child_count, sizeof(*children));
    if (!children)
        return E_OUTOFMEMORY;

    match = NULL;
    for (i = 0; i < child_count; i++)
    {
        hr = msaa_acc_get_next_child(parent, i + 1, DIR_FORWARD, &child, &child_id, &child_pos, FALSE);
        if (FAILED(hr) || !child)
            goto exit;

        if (child != parent)
            children[i] = child;
        else
            IAccessible_Release(child);
    }

    for (i = 0; i < child_count; i++)
    {
        if (!children[i])
            continue;

        if (msaa_acc_compare(acc, children[i]))
        {
            if (!match)
            {
                match = children[i];
                match_pos = i + 1;
            }
            /* Can't have more than one IAccessible match. */
            else
            {
                match = NULL;
                match_pos = 0;
                break;
            }
        }
    }

exit:
    if (match)
    {
        *out_parent = parent;
        *out_child_pos = match_pos;
    }
    else
        IAccessible_Release(parent);

    for (i = 0; i < child_count; i++)
    {
        if (children[i])
            IAccessible_Release(children[i]);
    }

    free(children);

    return hr;
}

static LONG msaa_role_to_uia_control_type(LONG role)
{
    switch (role)
    {
    case ROLE_SYSTEM_TITLEBAR:           return UIA_TitleBarControlTypeId;
    case ROLE_SYSTEM_MENUBAR:            return UIA_MenuBarControlTypeId;
    case ROLE_SYSTEM_SCROLLBAR:          return UIA_ScrollBarControlTypeId;
    case ROLE_SYSTEM_INDICATOR:
    case ROLE_SYSTEM_GRIP:               return UIA_ThumbControlTypeId;
    case ROLE_SYSTEM_APPLICATION:
    case ROLE_SYSTEM_WINDOW:             return UIA_WindowControlTypeId;
    case ROLE_SYSTEM_MENUPOPUP:          return UIA_MenuControlTypeId;
    case ROLE_SYSTEM_TOOLTIP:            return UIA_ToolTipControlTypeId;
    case ROLE_SYSTEM_DOCUMENT:           return UIA_DocumentControlTypeId;
    case ROLE_SYSTEM_PANE:               return UIA_PaneControlTypeId;
    case ROLE_SYSTEM_GROUPING:           return UIA_GroupControlTypeId;
    case ROLE_SYSTEM_SEPARATOR:          return UIA_SeparatorControlTypeId;
    case ROLE_SYSTEM_TOOLBAR:            return UIA_ToolBarControlTypeId;
    case ROLE_SYSTEM_STATUSBAR:          return UIA_StatusBarControlTypeId;
    case ROLE_SYSTEM_TABLE:              return UIA_TableControlTypeId;
    case ROLE_SYSTEM_COLUMNHEADER:
    case ROLE_SYSTEM_ROWHEADER:          return UIA_HeaderControlTypeId;
    case ROLE_SYSTEM_CELL:               return UIA_DataItemControlTypeId;
    case ROLE_SYSTEM_LINK:               return UIA_HyperlinkControlTypeId;
    case ROLE_SYSTEM_LIST:               return UIA_ListControlTypeId;
    case ROLE_SYSTEM_LISTITEM:           return UIA_ListItemControlTypeId;
    case ROLE_SYSTEM_OUTLINE:            return UIA_TreeControlTypeId;
    case ROLE_SYSTEM_OUTLINEITEM:        return UIA_TreeItemControlTypeId;
    case ROLE_SYSTEM_PAGETAB:            return UIA_TabItemControlTypeId;
    case ROLE_SYSTEM_GRAPHIC:            return UIA_ImageControlTypeId;
    case ROLE_SYSTEM_STATICTEXT:         return UIA_TextControlTypeId;
    case ROLE_SYSTEM_TEXT:               return UIA_EditControlTypeId;
    case ROLE_SYSTEM_CLOCK:
    case ROLE_SYSTEM_BUTTONDROPDOWNGRID:
    case ROLE_SYSTEM_PUSHBUTTON:         return UIA_ButtonControlTypeId;
    case ROLE_SYSTEM_CHECKBUTTON:        return UIA_CheckBoxControlTypeId;
    case ROLE_SYSTEM_RADIOBUTTON:        return UIA_RadioButtonControlTypeId;
    case ROLE_SYSTEM_COMBOBOX:           return UIA_ComboBoxControlTypeId;
    case ROLE_SYSTEM_PROGRESSBAR:        return UIA_ProgressBarControlTypeId;
    case ROLE_SYSTEM_SLIDER:             return UIA_SliderControlTypeId;
    case ROLE_SYSTEM_SPINBUTTON:         return UIA_SpinnerControlTypeId;
    case ROLE_SYSTEM_BUTTONMENU:
    case ROLE_SYSTEM_MENUITEM:           return UIA_MenuItemControlTypeId;
    case ROLE_SYSTEM_PAGETABLIST:        return UIA_TabControlTypeId;
    case ROLE_SYSTEM_BUTTONDROPDOWN:
    case ROLE_SYSTEM_SPLITBUTTON:        return UIA_SplitButtonControlTypeId;
    case ROLE_SYSTEM_SOUND:
    case ROLE_SYSTEM_CURSOR:
    case ROLE_SYSTEM_CARET:
    case ROLE_SYSTEM_ALERT:
    case ROLE_SYSTEM_CLIENT:
    case ROLE_SYSTEM_CHART:
    case ROLE_SYSTEM_DIALOG:
    case ROLE_SYSTEM_BORDER:
    case ROLE_SYSTEM_COLUMN:
    case ROLE_SYSTEM_ROW:
    case ROLE_SYSTEM_HELPBALLOON:
    case ROLE_SYSTEM_CHARACTER:
    case ROLE_SYSTEM_PROPERTYPAGE:
    case ROLE_SYSTEM_DROPLIST:
    case ROLE_SYSTEM_DIAL:
    case ROLE_SYSTEM_HOTKEYFIELD:
    case ROLE_SYSTEM_DIAGRAM:
    case ROLE_SYSTEM_ANIMATION:
    case ROLE_SYSTEM_EQUATION:
    case ROLE_SYSTEM_WHITESPACE:
    case ROLE_SYSTEM_IPADDRESS:
    case ROLE_SYSTEM_OUTLINEBUTTON:
        WARN("No UIA control type mapping for MSAA role %ld\n", role);
        break;

    default:
        FIXME("UIA control type mapping unimplemented for MSAA role %ld\n", role);
        break;
    }

    return 0;
}

/*
 * UiaProviderFromIAccessible IRawElementProviderSimple interface.
 */
struct msaa_provider {
    IRawElementProviderSimple IRawElementProviderSimple_iface;
    IRawElementProviderFragment IRawElementProviderFragment_iface;
    IRawElementProviderFragmentRoot IRawElementProviderFragmentRoot_iface;
    ILegacyIAccessibleProvider ILegacyIAccessibleProvider_iface;
    IProxyProviderWinEventHandler IProxyProviderWinEventHandler_iface;
    LONG refcount;

    IAccessible *acc;
    IAccessible2 *ia2;
    VARIANT cid;
    HWND hwnd;
    LONG control_type;

    BOOL root_acc_check_ran;
    BOOL is_root_acc;

    IAccessible *parent;
    INT child_pos;
};

static BOOL msaa_check_root_acc(struct msaa_provider *msaa_prov)
{
    IAccessible *acc;
    HRESULT hr;

    if (msaa_prov->root_acc_check_ran)
        return msaa_prov->is_root_acc;

    msaa_prov->root_acc_check_ran = TRUE;
    if (V_I4(&msaa_prov->cid) != CHILDID_SELF || msaa_prov->parent)
        return FALSE;

    hr = AccessibleObjectFromWindow(msaa_prov->hwnd, OBJID_CLIENT, &IID_IAccessible, (void **)&acc);
    if (FAILED(hr))
        return FALSE;

    if (msaa_acc_compare(msaa_prov->acc, acc))
        msaa_prov->is_root_acc = TRUE;

    IAccessible_Release(acc);
    return msaa_prov->is_root_acc;
}

static inline struct msaa_provider *impl_from_msaa_provider(IRawElementProviderSimple *iface)
{
    return CONTAINING_RECORD(iface, struct msaa_provider, IRawElementProviderSimple_iface);
}

HRESULT WINAPI msaa_provider_QueryInterface(IRawElementProviderSimple *iface, REFIID riid, void **ppv)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_provider(iface);

    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IRawElementProviderSimple) || IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IRawElementProviderFragment))
        *ppv = &msaa_prov->IRawElementProviderFragment_iface;
    else if (IsEqualIID(riid, &IID_IRawElementProviderFragmentRoot))
        *ppv = &msaa_prov->IRawElementProviderFragmentRoot_iface;
    else if (IsEqualIID(riid, &IID_ILegacyIAccessibleProvider))
        *ppv = &msaa_prov->ILegacyIAccessibleProvider_iface;
    else if (IsEqualIID(riid, &IID_IProxyProviderWinEventHandler))
        *ppv = &msaa_prov->IProxyProviderWinEventHandler_iface;
    else
        return E_NOINTERFACE;

    IRawElementProviderSimple_AddRef(iface);
    return S_OK;
}

ULONG WINAPI msaa_provider_AddRef(IRawElementProviderSimple *iface)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_provider(iface);
    ULONG refcount = InterlockedIncrement(&msaa_prov->refcount);

    TRACE("%p, refcount %ld\n", iface, refcount);

    return refcount;
}

ULONG WINAPI msaa_provider_Release(IRawElementProviderSimple *iface)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_provider(iface);
    ULONG refcount = InterlockedDecrement(&msaa_prov->refcount);

    TRACE("%p, refcount %ld\n", iface, refcount);

    if (!refcount)
    {
        IAccessible_Release(msaa_prov->acc);
        if (msaa_prov->parent)
            IAccessible_Release(msaa_prov->parent);
        if (msaa_prov->ia2)
            IAccessible2_Release(msaa_prov->ia2);
        free(msaa_prov);
    }

    return refcount;
}

HRESULT WINAPI msaa_provider_get_ProviderOptions(IRawElementProviderSimple *iface,
        enum ProviderOptions *ret_val)
{
    TRACE("%p, %p\n", iface, ret_val);
    *ret_val = ProviderOptions_ServerSideProvider | ProviderOptions_UseComThreading;
    return S_OK;
}

HRESULT WINAPI msaa_provider_GetPatternProvider(IRawElementProviderSimple *iface,
        PATTERNID pattern_id, IUnknown **ret_val)
{
    TRACE("%p, %d, %p\n", iface, pattern_id, ret_val);

    *ret_val = NULL;
    switch (pattern_id)
    {
    case UIA_LegacyIAccessiblePatternId:
        return IRawElementProviderSimple_QueryInterface(iface, &IID_IUnknown, (void **)ret_val);

    default:
        FIXME("Unimplemented patternId %d\n", pattern_id);
        break;
    }

    return S_OK;
}

HRESULT WINAPI msaa_provider_GetPropertyValue(IRawElementProviderSimple *iface,
        PROPERTYID prop_id, VARIANT *ret_val)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_provider(iface);
    HRESULT hr;
    VARIANT v;

    TRACE("%p, %d, %p\n", iface, prop_id, ret_val);

    VariantInit(ret_val);
    VariantInit(&v);
    switch (prop_id)
    {
    case UIA_ProviderDescriptionPropertyId:
        V_VT(ret_val) = VT_BSTR;
        V_BSTR(ret_val) = SysAllocString(L"Wine: MSAA Proxy");
        break;

    case UIA_ControlTypePropertyId:
        if (!msaa_prov->control_type)
        {
            hr = IAccessible_get_accRole(msaa_prov->acc, msaa_prov->cid, &v);
            if (SUCCEEDED(hr) && (V_VT(&v) == VT_I4))
                msaa_prov->control_type = msaa_role_to_uia_control_type(V_I4(&v));
        }

        if (msaa_prov->control_type)
            variant_init_i4(ret_val, msaa_prov->control_type);

        break;

    case UIA_HasKeyboardFocusPropertyId:
        hr = msaa_check_acc_state_hres(msaa_prov->acc, msaa_prov->cid, STATE_SYSTEM_FOCUSED);
        if (FAILED(hr))
            return hr;

        variant_init_bool(ret_val, hr == S_OK);
        break;

    case UIA_IsKeyboardFocusablePropertyId:
        hr = msaa_check_acc_state_hres(msaa_prov->acc, msaa_prov->cid, STATE_SYSTEM_FOCUSABLE);
        if (FAILED(hr))
            return hr;

        variant_init_bool(ret_val, hr == S_OK);
        break;

    case UIA_IsEnabledPropertyId:
        hr = msaa_check_acc_state_hres(msaa_prov->acc, msaa_prov->cid, STATE_SYSTEM_UNAVAILABLE);
        if (FAILED(hr))
            return hr;

        variant_init_bool(ret_val, hr == S_FALSE);
        break;

    case UIA_IsPasswordPropertyId:
        hr = msaa_check_acc_state_hres(msaa_prov->acc, msaa_prov->cid, STATE_SYSTEM_PROTECTED);
        if (FAILED(hr))
            return hr;

        variant_init_bool(ret_val, hr == S_OK);
        break;

    case UIA_NamePropertyId:
    {
        BSTR name;

        hr = IAccessible_get_accName(msaa_prov->acc, msaa_prov->cid, &name);
        if (SUCCEEDED(hr) && name)
        {
            V_VT(ret_val) = VT_BSTR;
            V_BSTR(ret_val) = name;
        }
        break;
    }

    case UIA_IsOffscreenPropertyId:
    {
        RECT rect[2] = { 0 };
        RECT intersect_rect;
        LONG width, height;

        variant_init_bool(ret_val, FALSE);
        if (msaa_check_acc_state(msaa_prov->acc, msaa_prov->cid, STATE_SYSTEM_OFFSCREEN))
        {
            variant_init_bool(ret_val, TRUE);
            break;
        }

        hr = IAccessible_accLocation(msaa_prov->acc, &rect[0].left, &rect[0].top, &width, &height, msaa_prov->cid);
        if (FAILED(hr))
            break;

        rect[0].right = rect[0].left + width;
        rect[0].bottom = rect[0].top + height;
        SetLastError(NOERROR);
        if (!GetClientRect(msaa_prov->hwnd, &rect[1]))
        {
            if (GetLastError() == ERROR_INVALID_WINDOW_HANDLE)
                variant_init_bool(ret_val, TRUE);
            break;
        }

        SetLastError(NOERROR);
        if (!MapWindowPoints(msaa_prov->hwnd, NULL, (POINT *)&rect[1], 2) && GetLastError())
        {
            if (GetLastError() == ERROR_INVALID_WINDOW_HANDLE)
                variant_init_bool(ret_val, TRUE);
            break;
        }

        variant_init_bool(ret_val, !IntersectRect(&intersect_rect, &rect[0], &rect[1]));
        break;
    }

    default:
        FIXME("Unimplemented propertyId %d\n", prop_id);
        break;
    }

    return S_OK;
}

HRESULT WINAPI msaa_provider_get_HostRawElementProvider(IRawElementProviderSimple *iface,
        IRawElementProviderSimple **ret_val)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_provider(iface);

    TRACE("%p, %p\n", iface, ret_val);

    *ret_val = NULL;
    if (msaa_check_root_acc(msaa_prov))
        return UiaHostProviderFromHwnd(msaa_prov->hwnd, ret_val);

    return S_OK;
}

static const IRawElementProviderSimpleVtbl msaa_provider_vtbl = {
    msaa_provider_QueryInterface,
    msaa_provider_AddRef,
    msaa_provider_Release,
    msaa_provider_get_ProviderOptions,
    msaa_provider_GetPatternProvider,
    msaa_provider_GetPropertyValue,
    msaa_provider_get_HostRawElementProvider,
};

/*
 * IRawElementProviderFragment interface for UiaProviderFromIAccessible
 * providers.
 */
static inline struct msaa_provider *impl_from_msaa_fragment(IRawElementProviderFragment *iface)
{
    return CONTAINING_RECORD(iface, struct msaa_provider, IRawElementProviderFragment_iface);
}

static HRESULT WINAPI msaa_fragment_QueryInterface(IRawElementProviderFragment *iface, REFIID riid,
        void **ppv)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_fragment(iface);
    return IRawElementProviderSimple_QueryInterface(&msaa_prov->IRawElementProviderSimple_iface, riid, ppv);
}

static ULONG WINAPI msaa_fragment_AddRef(IRawElementProviderFragment *iface)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_fragment(iface);
    return IRawElementProviderSimple_AddRef(&msaa_prov->IRawElementProviderSimple_iface);
}

static ULONG WINAPI msaa_fragment_Release(IRawElementProviderFragment *iface)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_fragment(iface);
    return IRawElementProviderSimple_Release(&msaa_prov->IRawElementProviderSimple_iface);
}

static HRESULT WINAPI msaa_fragment_Navigate(IRawElementProviderFragment *iface,
        enum NavigateDirection direction, IRawElementProviderFragment **ret_val)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_fragment(iface);
    LONG child_count, child_id, child_pos;
    IRawElementProviderSimple *elprov;
    IAccessible *acc;
    HRESULT hr;

    TRACE("%p, %d, %p\n", iface, direction, ret_val);

    *ret_val = NULL;
    switch (direction)
    {
    case NavigateDirection_Parent:
        if (msaa_check_root_acc(msaa_prov))
            break;

        if (V_I4(&msaa_prov->cid) == CHILDID_SELF)
        {
            hr = msaa_acc_get_parent(msaa_prov->acc, &acc);
            if (FAILED(hr) || !acc)
                break;
        }
        else
            acc = msaa_prov->acc;

        hr = create_msaa_provider(acc, CHILDID_SELF, NULL, FALSE, FALSE, &elprov);
        if (SUCCEEDED(hr))
        {
            struct msaa_provider *prov = impl_from_msaa_provider(elprov);
            *ret_val = &prov->IRawElementProviderFragment_iface;
        }

        if (acc != msaa_prov->acc)
            IAccessible_Release(acc);

        break;

    case NavigateDirection_FirstChild:
    case NavigateDirection_LastChild:
        if (V_I4(&msaa_prov->cid) != CHILDID_SELF)
            break;

        hr = IAccessible_get_accChildCount(msaa_prov->acc, &child_count);
        if (FAILED(hr) || !child_count)
            break;

        if (direction == NavigateDirection_FirstChild)
            hr = msaa_acc_get_next_child(msaa_prov->acc, 1, DIR_FORWARD, &acc, &child_id,
                    &child_pos, TRUE);
        else
            hr = msaa_acc_get_next_child(msaa_prov->acc, child_count, DIR_REVERSE, &acc, &child_id,
                    &child_pos, TRUE);

        if (FAILED(hr) || !acc)
            break;

        hr = create_msaa_provider(acc, child_id, NULL, FALSE, FALSE, &elprov);
        if (SUCCEEDED(hr))
        {
            struct msaa_provider *prov = impl_from_msaa_provider(elprov);

            *ret_val = &prov->IRawElementProviderFragment_iface;
            prov->parent = msaa_prov->acc;
            IAccessible_AddRef(msaa_prov->acc);
            if (acc != msaa_prov->acc)
                prov->child_pos = child_pos;
            else
                prov->child_pos = child_id;
        }
        IAccessible_Release(acc);

        break;

    case NavigateDirection_NextSibling:
    case NavigateDirection_PreviousSibling:
        if (msaa_check_root_acc(msaa_prov))
            break;

        if (!msaa_prov->parent)
        {
            if (V_I4(&msaa_prov->cid) != CHILDID_SELF)
            {
                msaa_prov->parent = msaa_prov->acc;
                IAccessible_AddRef(msaa_prov->acc);
                msaa_prov->child_pos = V_I4(&msaa_prov->cid);
            }
            else
            {
                hr = msaa_acc_get_child_pos(msaa_prov->acc, &acc, &child_pos);
                if (FAILED(hr) || !acc)
                    break;
                msaa_prov->parent = acc;
                msaa_prov->child_pos = child_pos;
            }
        }

        if (direction == NavigateDirection_NextSibling)
            hr = msaa_acc_get_next_child(msaa_prov->parent, msaa_prov->child_pos + 1, DIR_FORWARD,
                    &acc, &child_id, &child_pos, TRUE);
        else
            hr = msaa_acc_get_next_child(msaa_prov->parent, msaa_prov->child_pos - 1, DIR_REVERSE,
                    &acc, &child_id, &child_pos, TRUE);

        if (FAILED(hr) || !acc)
            break;

        hr = create_msaa_provider(acc, child_id, NULL, FALSE, FALSE, &elprov);
        if (SUCCEEDED(hr))
        {
            struct msaa_provider *prov = impl_from_msaa_provider(elprov);

            *ret_val = &prov->IRawElementProviderFragment_iface;
            prov->parent = msaa_prov->parent;
            IAccessible_AddRef(msaa_prov->parent);
            if (acc != msaa_prov->acc)
                prov->child_pos = child_pos;
            else
                prov->child_pos = child_id;
        }
        IAccessible_Release(acc);

        break;

    default:
        FIXME("Invalid NavigateDirection %d\n", direction);
        return E_INVALIDARG;
    }

    return S_OK;
}

static HRESULT WINAPI msaa_fragment_GetRuntimeId(IRawElementProviderFragment *iface,
        SAFEARRAY **ret_val)
{
    FIXME("%p, %p: stub!\n", iface, ret_val);
    *ret_val = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI msaa_fragment_get_BoundingRectangle(IRawElementProviderFragment *iface,
        struct UiaRect *ret_val)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_fragment(iface);
    LONG left, top, width, height;
    HRESULT hr;

    TRACE("%p, %p\n", iface, ret_val);

    memset(ret_val, 0, sizeof(*ret_val));

    /*
     * If this IAccessible is at the root of its HWND, the BaseHwnd provider
     * will supply the bounding rectangle.
     */
    if (msaa_check_root_acc(msaa_prov))
        return S_OK;

    if (msaa_check_acc_state(msaa_prov->acc, msaa_prov->cid, STATE_SYSTEM_OFFSCREEN))
        return S_OK;

    hr = IAccessible_accLocation(msaa_prov->acc, &left, &top, &width, &height, msaa_prov->cid);
    if (FAILED(hr))
        return hr;

    ret_val->left = left;
    ret_val->top = top;
    ret_val->width = width;
    ret_val->height = height;

    return S_OK;
}

static HRESULT WINAPI msaa_fragment_GetEmbeddedFragmentRoots(IRawElementProviderFragment *iface,
        SAFEARRAY **ret_val)
{
    TRACE("%p, %p\n", iface, ret_val);
    *ret_val = NULL;
    return S_OK;
}

static HRESULT WINAPI msaa_fragment_SetFocus(IRawElementProviderFragment *iface)
{
    FIXME("%p: stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI msaa_fragment_get_FragmentRoot(IRawElementProviderFragment *iface,
        IRawElementProviderFragmentRoot **ret_val)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_fragment(iface);
    IRawElementProviderSimple *elprov;
    IAccessible *acc;
    HRESULT hr;

    TRACE("%p, %p\n", iface, ret_val);

    *ret_val = NULL;
    hr = AccessibleObjectFromWindow(msaa_prov->hwnd, OBJID_CLIENT, &IID_IAccessible, (void **)&acc);
    if (FAILED(hr) || !acc)
        return hr;

    hr = create_msaa_provider(acc, CHILDID_SELF, msaa_prov->hwnd, TRUE, TRUE, &elprov);
    IAccessible_Release(acc);
    if (FAILED(hr))
        return hr;

    hr = IRawElementProviderSimple_QueryInterface(elprov, &IID_IRawElementProviderFragmentRoot, (void **)ret_val);
    IRawElementProviderSimple_Release(elprov);

    return hr;
}

static const IRawElementProviderFragmentVtbl msaa_fragment_vtbl = {
    msaa_fragment_QueryInterface,
    msaa_fragment_AddRef,
    msaa_fragment_Release,
    msaa_fragment_Navigate,
    msaa_fragment_GetRuntimeId,
    msaa_fragment_get_BoundingRectangle,
    msaa_fragment_GetEmbeddedFragmentRoots,
    msaa_fragment_SetFocus,
    msaa_fragment_get_FragmentRoot,
};

/*
 * IRawElementProviderFragmentRoot interface for UiaProviderFromIAccessible
 * providers.
 */
static inline struct msaa_provider *impl_from_msaa_fragment_root(IRawElementProviderFragmentRoot *iface)
{
    return CONTAINING_RECORD(iface, struct msaa_provider, IRawElementProviderFragmentRoot_iface);
}

static HRESULT WINAPI msaa_fragment_root_QueryInterface(IRawElementProviderFragmentRoot *iface, REFIID riid,
        void **ppv)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_fragment_root(iface);
    return IRawElementProviderSimple_QueryInterface(&msaa_prov->IRawElementProviderSimple_iface, riid, ppv);
}

static ULONG WINAPI msaa_fragment_root_AddRef(IRawElementProviderFragmentRoot *iface)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_fragment_root(iface);
    return IRawElementProviderSimple_AddRef(&msaa_prov->IRawElementProviderSimple_iface);
}

static ULONG WINAPI msaa_fragment_root_Release(IRawElementProviderFragmentRoot *iface)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_fragment_root(iface);
    return IRawElementProviderSimple_Release(&msaa_prov->IRawElementProviderSimple_iface);
}

static HRESULT WINAPI msaa_fragment_root_ElementProviderFromPoint(IRawElementProviderFragmentRoot *iface,
        double x, double y, IRawElementProviderFragment **ret_val)
{
    FIXME("%p, %f, %f, %p: stub!\n", iface, x, y, ret_val);
    *ret_val = NULL;
    return E_NOTIMPL;
}

static HRESULT msaa_acc_get_focus(struct msaa_provider *prov, struct msaa_provider **out_prov)
{
    IRawElementProviderSimple *elprov;
    IAccessible *focus_acc = NULL;
    INT focus_cid = CHILDID_SELF;
    HWND hwnd = NULL;
    HRESULT hr;
    VARIANT v;

    *out_prov = NULL;

    if (V_I4(&prov->cid) != CHILDID_SELF)
        return S_OK;

    VariantInit(&v);
    hr = IAccessible_get_accFocus(prov->acc, &v);
    if (FAILED(hr) || (V_VT(&v) != VT_I4 && V_VT(&v) != VT_DISPATCH))
    {
        VariantClear(&v);
        return hr;
    }

    if (V_VT(&v) == VT_I4)
    {
        IDispatch *disp = NULL;

        if (V_I4(&v) == CHILDID_SELF)
            return S_OK;

        hr = IAccessible_get_accChild(prov->acc, v, &disp);
        if (FAILED(hr))
            return hr;

        if (hr == S_FALSE)
        {
            hwnd = prov->hwnd;
            focus_acc = prov->acc;
            IAccessible_AddRef(focus_acc);
            focus_cid = V_I4(&v);
        }
        else if (disp)
        {
            V_VT(&v) = VT_DISPATCH;
            V_DISPATCH(&v) = disp;
        }
        else
            return E_FAIL;
    }

    if (V_VT(&v) == VT_DISPATCH)
    {
        hr = IDispatch_QueryInterface(V_DISPATCH(&v), &IID_IAccessible, (void **)&focus_acc);
        VariantClear(&v);
        if (FAILED(hr))
            return hr;

        hr = WindowFromAccessibleObject(focus_acc, &hwnd);
        if (FAILED(hr) || !hwnd)
        {
            IAccessible_Release(focus_acc);
            return hr;
        }
    }

    hr = create_msaa_provider(focus_acc, focus_cid, hwnd, FALSE, FALSE, &elprov);
    IAccessible_Release(focus_acc);
    if (SUCCEEDED(hr))
        *out_prov = impl_from_msaa_provider(elprov);

    return hr;
}

static HRESULT WINAPI msaa_fragment_root_GetFocus(IRawElementProviderFragmentRoot *iface,
        IRawElementProviderFragment **ret_val)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_fragment_root(iface);
    struct msaa_provider *prov, *prov2;
    IRawElementProviderSimple *elprov;
    HRESULT hr;

    TRACE("%p, %p\n", iface, ret_val);

    *ret_val = NULL;
    if (V_I4(&msaa_prov->cid) != CHILDID_SELF)
        return S_OK;

    hr = create_msaa_provider(msaa_prov->acc, CHILDID_SELF, msaa_prov->hwnd, FALSE, FALSE, &elprov);
    if (FAILED(hr))
        return hr;

    prov = impl_from_msaa_provider(elprov);
    while (SUCCEEDED(msaa_acc_get_focus(prov, &prov2)))
    {
        if (!prov2 || (msaa_check_acc_state_hres(prov2->acc, prov2->cid, STATE_SYSTEM_INVISIBLE) != S_FALSE) ||
                ((V_I4(&prov2->cid) == CHILDID_SELF) && msaa_acc_iface_cmp(prov->acc, prov2->acc)))
        {
            if (prov2)
                IRawElementProviderSimple_Release(&prov2->IRawElementProviderSimple_iface);

            if (msaa_acc_iface_cmp(prov->acc, msaa_prov->acc) && V_I4(&prov->cid) == CHILDID_SELF)
            {
                IRawElementProviderSimple_Release(&prov->IRawElementProviderSimple_iface);
                return S_OK;
            }
            break;
        }

        IRawElementProviderSimple_Release(&prov->IRawElementProviderSimple_iface);
        prov = prov2;
    }

    hr = IRawElementProviderSimple_QueryInterface(&prov->IRawElementProviderSimple_iface, &IID_IRawElementProviderFragment, (void **)ret_val);
    IRawElementProviderSimple_Release(&prov->IRawElementProviderSimple_iface);
    return hr;
}

static const IRawElementProviderFragmentRootVtbl msaa_fragment_root_vtbl = {
    msaa_fragment_root_QueryInterface,
    msaa_fragment_root_AddRef,
    msaa_fragment_root_Release,
    msaa_fragment_root_ElementProviderFromPoint,
    msaa_fragment_root_GetFocus,
};

/*
 * ILegacyIAccessibleProvider interface for UiaProviderFromIAccessible
 * providers.
 */
static inline struct msaa_provider *impl_from_msaa_acc_provider(ILegacyIAccessibleProvider *iface)
{
    return CONTAINING_RECORD(iface, struct msaa_provider, ILegacyIAccessibleProvider_iface);
}

static HRESULT WINAPI msaa_acc_provider_QueryInterface(ILegacyIAccessibleProvider *iface, REFIID riid, void **ppv)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_acc_provider(iface);
    return IRawElementProviderSimple_QueryInterface(&msaa_prov->IRawElementProviderSimple_iface, riid, ppv);
}

static ULONG WINAPI msaa_acc_provider_AddRef(ILegacyIAccessibleProvider *iface)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_acc_provider(iface);
    return IRawElementProviderSimple_AddRef(&msaa_prov->IRawElementProviderSimple_iface);
}

static ULONG WINAPI msaa_acc_provider_Release(ILegacyIAccessibleProvider *iface)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_acc_provider(iface);
    return IRawElementProviderSimple_Release(&msaa_prov->IRawElementProviderSimple_iface);
}

static HRESULT WINAPI msaa_acc_provider_Select(ILegacyIAccessibleProvider *iface, LONG select_flags)
{
    FIXME("%p, %#lx: stub!\n", iface, select_flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI msaa_acc_provider_DoDefaultAction(ILegacyIAccessibleProvider *iface)
{
    FIXME("%p: stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI msaa_acc_provider_SetValue(ILegacyIAccessibleProvider *iface, LPCWSTR val)
{
    FIXME("%p, %p<%s>: stub!\n", iface, val, debugstr_w(val));
    return E_NOTIMPL;
}

static HRESULT WINAPI msaa_acc_provider_GetIAccessible(ILegacyIAccessibleProvider *iface,
        IAccessible **out_acc)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_acc_provider(iface);

    TRACE("%p, %p\n", iface, out_acc);

    *out_acc = NULL;
    if (msaa_acc_is_oleacc_proxy(msaa_prov->acc))
        return S_OK;

    return IAccessible_QueryInterface(msaa_prov->acc, &IID_IAccessible, (void **)out_acc);
}

static HRESULT WINAPI msaa_acc_provider_get_ChildId(ILegacyIAccessibleProvider *iface, int *out_cid)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_acc_provider(iface);

    TRACE("%p, %p\n", iface, out_cid);
    *out_cid = V_I4(&msaa_prov->cid);

    return S_OK;
}

static HRESULT WINAPI msaa_acc_provider_get_Name(ILegacyIAccessibleProvider *iface, BSTR *out_name)
{
    FIXME("%p, %p: stub!\n", iface, out_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI msaa_acc_provider_get_Value(ILegacyIAccessibleProvider *iface, BSTR *out_value)
{
    FIXME("%p, %p: stub!\n", iface, out_value);
    return E_NOTIMPL;
}

static HRESULT WINAPI msaa_acc_provider_get_Description(ILegacyIAccessibleProvider *iface,
        BSTR *out_description)
{
    FIXME("%p, %p: stub!\n", iface, out_description);
    return E_NOTIMPL;
}

static HRESULT WINAPI msaa_acc_provider_get_Role(ILegacyIAccessibleProvider *iface, DWORD *out_role)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_acc_provider(iface);
    HRESULT hr;
    VARIANT v;

    TRACE("%p, %p\n", iface, out_role);

    *out_role = 0;
    VariantInit(&v);
    hr = IAccessible_get_accRole(msaa_prov->acc, msaa_prov->cid, &v);
    if (SUCCEEDED(hr) && V_VT(&v) == VT_I4)
        *out_role = V_I4(&v);

    return S_OK;
}

static HRESULT WINAPI msaa_acc_provider_get_State(ILegacyIAccessibleProvider *iface, DWORD *out_state)
{
    FIXME("%p, %p: stub!\n", iface, out_state);
    return E_NOTIMPL;
}

static HRESULT WINAPI msaa_acc_provider_get_Help(ILegacyIAccessibleProvider *iface, BSTR *out_help)
{
    FIXME("%p, %p: stub!\n", iface, out_help);
    return E_NOTIMPL;
}

static HRESULT WINAPI msaa_acc_provider_get_KeyboardShortcut(ILegacyIAccessibleProvider *iface,
        BSTR *out_kbd_shortcut)
{
    FIXME("%p, %p: stub!\n", iface, out_kbd_shortcut);
    return E_NOTIMPL;
}

static HRESULT WINAPI msaa_acc_provider_GetSelection(ILegacyIAccessibleProvider *iface,
        SAFEARRAY **out_selected)
{
    FIXME("%p, %p: stub!\n", iface, out_selected);
    return E_NOTIMPL;
}

static HRESULT WINAPI msaa_acc_provider_get_DefaultAction(ILegacyIAccessibleProvider *iface,
        BSTR *out_default_action)
{
    FIXME("%p, %p: stub!\n", iface, out_default_action);
    return E_NOTIMPL;
}

static const ILegacyIAccessibleProviderVtbl msaa_acc_provider_vtbl = {
    msaa_acc_provider_QueryInterface,
    msaa_acc_provider_AddRef,
    msaa_acc_provider_Release,
    msaa_acc_provider_Select,
    msaa_acc_provider_DoDefaultAction,
    msaa_acc_provider_SetValue,
    msaa_acc_provider_GetIAccessible,
    msaa_acc_provider_get_ChildId,
    msaa_acc_provider_get_Name,
    msaa_acc_provider_get_Value,
    msaa_acc_provider_get_Description,
    msaa_acc_provider_get_Role,
    msaa_acc_provider_get_State,
    msaa_acc_provider_get_Help,
    msaa_acc_provider_get_KeyboardShortcut,
    msaa_acc_provider_GetSelection,
    msaa_acc_provider_get_DefaultAction,
};

/*
 * IProxyProviderWinEventHandler interface for UiaProviderFromIAccessible
 * providers.
 */
static inline struct msaa_provider *impl_from_msaa_winevent_handler(IProxyProviderWinEventHandler *iface)
{
    return CONTAINING_RECORD(iface, struct msaa_provider, IProxyProviderWinEventHandler_iface);
}

static HRESULT WINAPI msaa_winevent_handler_QueryInterface(IProxyProviderWinEventHandler *iface, REFIID riid,
        void **ppv)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_winevent_handler(iface);
    return IRawElementProviderSimple_QueryInterface(&msaa_prov->IRawElementProviderSimple_iface, riid, ppv);
}

static ULONG WINAPI msaa_winevent_handler_AddRef(IProxyProviderWinEventHandler *iface)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_winevent_handler(iface);
    return IRawElementProviderSimple_AddRef(&msaa_prov->IRawElementProviderSimple_iface);
}

static ULONG WINAPI msaa_winevent_handler_Release(IProxyProviderWinEventHandler *iface)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_winevent_handler(iface);
    return IRawElementProviderSimple_Release(&msaa_prov->IRawElementProviderSimple_iface);
}

static HRESULT WINAPI msaa_winevent_handler_RespondToWinEvent(IProxyProviderWinEventHandler *iface, DWORD event_id,
        HWND hwnd, LONG objid, LONG cid, IProxyProviderWinEventSink *event_sink)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_winevent_handler(iface);
    HRESULT hr;

    TRACE("%p, %ld, %p, %ld, %ld, %p\n", iface, event_id, hwnd, objid, cid, event_sink);

    switch (event_id)
    {
    case EVENT_SYSTEM_ALERT:
        hr = IProxyProviderWinEventSink_AddAutomationEvent(event_sink, &msaa_prov->IRawElementProviderSimple_iface,
                UIA_SystemAlertEventId);
        if (FAILED(hr))
            WARN("AddAutomationEvent failed with hr %#lx\n", hr);
        break;

    case EVENT_OBJECT_REORDER:
    case EVENT_OBJECT_SELECTION:
    case EVENT_OBJECT_NAMECHANGE:
    case EVENT_OBJECT_VALUECHANGE:
    case EVENT_OBJECT_HELPCHANGE:
    case EVENT_OBJECT_INVOKED:
        FIXME("WinEvent %ld currently unimplemented\n", event_id);
        return E_NOTIMPL;

    default:
        break;
    }

    return S_OK;
}

static const IProxyProviderWinEventHandlerVtbl msaa_winevent_handler_vtbl = {
    msaa_winevent_handler_QueryInterface,
    msaa_winevent_handler_AddRef,
    msaa_winevent_handler_Release,
    msaa_winevent_handler_RespondToWinEvent,
};

HRESULT create_msaa_provider(IAccessible *acc, LONG child_id, HWND hwnd, BOOL root_acc_known,
        BOOL is_root_acc, IRawElementProviderSimple **elprov)
{
    struct msaa_provider *msaa_prov = calloc(1, sizeof(*msaa_prov));

    if (!msaa_prov)
        return E_OUTOFMEMORY;

    msaa_prov->IRawElementProviderSimple_iface.lpVtbl = &msaa_provider_vtbl;
    msaa_prov->IRawElementProviderFragment_iface.lpVtbl = &msaa_fragment_vtbl;
    msaa_prov->IRawElementProviderFragmentRoot_iface.lpVtbl = &msaa_fragment_root_vtbl;
    msaa_prov->ILegacyIAccessibleProvider_iface.lpVtbl = &msaa_acc_provider_vtbl;
    msaa_prov->IProxyProviderWinEventHandler_iface.lpVtbl = &msaa_winevent_handler_vtbl;
    msaa_prov->refcount = 1;
    variant_init_i4(&msaa_prov->cid, child_id);
    msaa_prov->acc = acc;
    IAccessible_AddRef(acc);
    msaa_prov->ia2 = msaa_acc_get_ia2(acc);

    if (!hwnd)
    {
        HRESULT hr;

        hr = WindowFromAccessibleObject(acc, &msaa_prov->hwnd);
        if (FAILED(hr))
            WARN("WindowFromAccessibleObject failed with hr %#lx\n", hr);
    }
    else
        msaa_prov->hwnd = hwnd;

    if (root_acc_known)
    {
        msaa_prov->root_acc_check_ran = TRUE;
        msaa_prov->is_root_acc = is_root_acc;
    }

    *elprov = &msaa_prov->IRawElementProviderSimple_iface;

    return S_OK;
}

/***********************************************************************
 *          UiaProviderFromIAccessible (uiautomationcore.@)
 */
HRESULT WINAPI UiaProviderFromIAccessible(IAccessible *acc, LONG child_id, DWORD flags,
        IRawElementProviderSimple **elprov)
{
    HWND hwnd = NULL;
    HRESULT hr;

    TRACE("(%p, %ld, %#lx, %p)\n", acc, child_id, flags, elprov);

    if (elprov)
        *elprov = NULL;

    if (!elprov)
        return E_POINTER;
    if (!acc)
        return E_INVALIDARG;

    if (flags != UIA_PFIA_DEFAULT)
    {
        FIXME("unsupported flags %#lx\n", flags);
        return E_NOTIMPL;
    }

    if (msaa_acc_is_oleacc_proxy(acc))
    {
        WARN("Cannot wrap an oleacc proxy IAccessible!\n");
        return E_INVALIDARG;
    }

    hr = WindowFromAccessibleObject(acc, &hwnd);
    if (FAILED(hr))
       return hr;
    if (!hwnd)
        return E_FAIL;

    return create_msaa_provider(acc, child_id, hwnd, FALSE, FALSE, elprov);
}

static HRESULT uia_get_hr_for_last_error(void)
{
    DWORD last_err = GetLastError();

    switch (last_err)
    {
    case ERROR_INVALID_WINDOW_HANDLE:
        return UIA_E_ELEMENTNOTAVAILABLE;

    case ERROR_TIMEOUT:
        return UIA_E_TIMEOUT;

    default:
        return E_FAIL;
    }
}

#define UIA_DEFAULT_MSG_TIMEOUT 10000
static HRESULT uia_send_message_timeout(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT timeout, LRESULT *lres)
{
    *lres = 0;
    if (!SendMessageTimeoutW(hwnd, msg, wparam, lparam, SMTO_NORMAL, timeout, (PDWORD_PTR)lres))
        return uia_get_hr_for_last_error();

    return S_OK;
}

static HRESULT get_uia_control_type_for_hwnd(HWND hwnd, int *control_type)
{
    LONG_PTR style, ex_style;

    *control_type = 0;
    if ((ex_style = GetWindowLongPtrW(hwnd, GWL_EXSTYLE)) & WS_EX_APPWINDOW)
    {
        *control_type = UIA_WindowControlTypeId;
        return S_OK;
    }

    SetLastError(NO_ERROR);
    if (!(style = GetWindowLongPtrW(hwnd, GWL_STYLE)) && (GetLastError() != NO_ERROR))
        return uia_get_hr_for_last_error();

    /*
     * Non-caption HWNDs that are popups or tool windows aren't considered full
     * windows, only panes.
     */
    if (((style & WS_CAPTION) != WS_CAPTION) && ((ex_style & WS_EX_TOOLWINDOW) || (style & WS_POPUP)))
    {
        *control_type = UIA_PaneControlTypeId;
        return S_OK;
    }

    /* Non top-level HWNDs are considered panes as well. */
    if (!uia_is_top_level_hwnd(hwnd))
        *control_type = UIA_PaneControlTypeId;
    else
        *control_type = UIA_WindowControlTypeId;

    return S_OK;
}

/*
 * Default ProviderType_BaseHwnd IRawElementProviderSimple interface.
 */
struct base_hwnd_provider {
    IRawElementProviderSimple IRawElementProviderSimple_iface;
    IRawElementProviderFragment IRawElementProviderFragment_iface;
    LONG refcount;

    HWND hwnd;
};

static inline struct base_hwnd_provider *impl_from_base_hwnd_provider(IRawElementProviderSimple *iface)
{
    return CONTAINING_RECORD(iface, struct base_hwnd_provider, IRawElementProviderSimple_iface);
}

static HRESULT WINAPI base_hwnd_provider_QueryInterface(IRawElementProviderSimple *iface, REFIID riid, void **ppv)
{
    struct base_hwnd_provider *base_hwnd_prov = impl_from_base_hwnd_provider(iface);

    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IRawElementProviderSimple) || IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IRawElementProviderFragment))
        *ppv = &base_hwnd_prov->IRawElementProviderFragment_iface;
    else
        return E_NOINTERFACE;

    IRawElementProviderSimple_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI base_hwnd_provider_AddRef(IRawElementProviderSimple *iface)
{
    struct base_hwnd_provider *base_hwnd_prov = impl_from_base_hwnd_provider(iface);
    ULONG refcount = InterlockedIncrement(&base_hwnd_prov->refcount);

    TRACE("%p, refcount %ld\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI base_hwnd_provider_Release(IRawElementProviderSimple *iface)
{
    struct base_hwnd_provider *base_hwnd_prov = impl_from_base_hwnd_provider(iface);
    ULONG refcount = InterlockedDecrement(&base_hwnd_prov->refcount);

    TRACE("%p, refcount %ld\n", iface, refcount);

    if (!refcount)
        free(base_hwnd_prov);

    return refcount;
}

static HRESULT WINAPI base_hwnd_provider_get_ProviderOptions(IRawElementProviderSimple *iface,
        enum ProviderOptions *ret_val)
{
    TRACE("%p, %p\n", iface, ret_val);
    *ret_val = ProviderOptions_ClientSideProvider;
    return S_OK;
}

static HRESULT WINAPI base_hwnd_provider_GetPatternProvider(IRawElementProviderSimple *iface,
        PATTERNID pattern_id, IUnknown **ret_val)
{
    FIXME("%p, %d, %p: stub\n", iface, pattern_id, ret_val);
    *ret_val = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI base_hwnd_provider_GetPropertyValue(IRawElementProviderSimple *iface,
        PROPERTYID prop_id, VARIANT *ret_val)
{
    struct base_hwnd_provider *base_hwnd_prov = impl_from_base_hwnd_provider(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %d, %p\n", iface, prop_id, ret_val);

    VariantInit(ret_val);
    if (!IsWindow(base_hwnd_prov->hwnd))
        return UIA_E_ELEMENTNOTAVAILABLE;

    switch (prop_id)
    {
    case UIA_ProviderDescriptionPropertyId:
        V_VT(ret_val) = VT_BSTR;
        V_BSTR(ret_val) = SysAllocString(L"Wine: HWND Proxy");
        break;

    case UIA_NativeWindowHandlePropertyId:
        V_VT(ret_val) = VT_I4;
        V_I4(ret_val) = HandleToUlong(base_hwnd_prov->hwnd);
        break;

    case UIA_ProcessIdPropertyId:
    {
        DWORD pid;

        if (!GetWindowThreadProcessId(base_hwnd_prov->hwnd, &pid))
            return UIA_E_ELEMENTNOTAVAILABLE;

        V_VT(ret_val) = VT_I4;
        V_I4(ret_val) = pid;
        break;
    }

    case UIA_ClassNamePropertyId:
    {
        WCHAR buf[256] = { 0 };

        if (!GetClassNameW(base_hwnd_prov->hwnd, buf, ARRAY_SIZE(buf)))
            hr = uia_get_hr_for_last_error();
        else
        {
            V_VT(ret_val) = VT_BSTR;
            V_BSTR(ret_val) = SysAllocString(buf);
        }
        break;
    }

    case UIA_NamePropertyId:
    {
        LRESULT lres;

        V_VT(ret_val) = VT_BSTR;
        V_BSTR(ret_val) = SysAllocString(L"");
        hr = uia_send_message_timeout(base_hwnd_prov->hwnd, WM_GETTEXTLENGTH, 0, 0, UIA_DEFAULT_MSG_TIMEOUT, &lres);
        if (FAILED(hr) || !lres)
            break;

        if (!SysReAllocStringLen(&V_BSTR(ret_val), NULL, lres))
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        hr = uia_send_message_timeout(base_hwnd_prov->hwnd, WM_GETTEXT, SysStringLen(V_BSTR(ret_val)) + 1,
                (LPARAM)V_BSTR(ret_val), UIA_DEFAULT_MSG_TIMEOUT, &lres);
        break;
    }

    case UIA_ControlTypePropertyId:
    {
        int control_type;

        hr = get_uia_control_type_for_hwnd(base_hwnd_prov->hwnd, &control_type);
        if (SUCCEEDED(hr))
        {
            V_VT(ret_val) = VT_I4;
            V_I4(ret_val) = control_type;
        }
        break;
    }

    default:
        break;
    }

    if (FAILED(hr))
        VariantClear(ret_val);

    return hr;
}

static HRESULT WINAPI base_hwnd_provider_get_HostRawElementProvider(IRawElementProviderSimple *iface,
        IRawElementProviderSimple **ret_val)
{
    TRACE("%p, %p\n", iface, ret_val);
    *ret_val = NULL;
    return S_OK;
}

static const IRawElementProviderSimpleVtbl base_hwnd_provider_vtbl = {
    base_hwnd_provider_QueryInterface,
    base_hwnd_provider_AddRef,
    base_hwnd_provider_Release,
    base_hwnd_provider_get_ProviderOptions,
    base_hwnd_provider_GetPatternProvider,
    base_hwnd_provider_GetPropertyValue,
    base_hwnd_provider_get_HostRawElementProvider,
};

/*
 * IRawElementProviderFragment interface for default ProviderType_BaseHwnd
 * providers.
 */
static inline struct base_hwnd_provider *impl_from_base_hwnd_fragment(IRawElementProviderFragment *iface)
{
    return CONTAINING_RECORD(iface, struct base_hwnd_provider, IRawElementProviderFragment_iface);
}

static HRESULT WINAPI base_hwnd_fragment_QueryInterface(IRawElementProviderFragment *iface, REFIID riid,
        void **ppv)
{
    struct base_hwnd_provider *base_hwnd_prov = impl_from_base_hwnd_fragment(iface);
    return IRawElementProviderSimple_QueryInterface(&base_hwnd_prov->IRawElementProviderSimple_iface, riid, ppv);
}

static ULONG WINAPI base_hwnd_fragment_AddRef(IRawElementProviderFragment *iface)
{
    struct base_hwnd_provider *base_hwnd_prov = impl_from_base_hwnd_fragment(iface);
    return IRawElementProviderSimple_AddRef(&base_hwnd_prov->IRawElementProviderSimple_iface);
}

static ULONG WINAPI base_hwnd_fragment_Release(IRawElementProviderFragment *iface)
{
    struct base_hwnd_provider *base_hwnd_prov = impl_from_base_hwnd_fragment(iface);
    return IRawElementProviderSimple_Release(&base_hwnd_prov->IRawElementProviderSimple_iface);
}

static HRESULT WINAPI base_hwnd_fragment_Navigate(IRawElementProviderFragment *iface,
        enum NavigateDirection direction, IRawElementProviderFragment **ret_val)
{
    struct base_hwnd_provider *base_hwnd_prov = impl_from_base_hwnd_fragment(iface);
    IRawElementProviderSimple *elprov = NULL;
    HRESULT hr = S_OK;

    TRACE("%p, %d, %p\n", iface, direction, ret_val);

    *ret_val = NULL;

    switch (direction)
    {
    case NavigateDirection_Parent:
    {
        HWND parent, owner;

        /*
         * Top level owned windows have their owner window as a parent instead
         * of the desktop window.
         */
        if (uia_is_top_level_hwnd(base_hwnd_prov->hwnd) && (owner = GetWindow(base_hwnd_prov->hwnd, GW_OWNER)))
            parent = owner;
        else
            parent = GetAncestor(base_hwnd_prov->hwnd, GA_PARENT);

        if (parent)
            hr = create_base_hwnd_provider(parent, &elprov);
        break;
    }

    case NavigateDirection_FirstChild:
    case NavigateDirection_LastChild:
    case NavigateDirection_PreviousSibling:
    case NavigateDirection_NextSibling:
        FIXME("Unimplemented NavigateDirection %d\n", direction);
        return E_NOTIMPL;

    default:
        FIXME("Invalid NavigateDirection %d\n", direction);
        return E_INVALIDARG;
    }

    if (elprov)
    {
        hr = IRawElementProviderSimple_QueryInterface(elprov, &IID_IRawElementProviderFragment, (void **)ret_val);
        IRawElementProviderSimple_Release(elprov);
    }

    return hr;
}

static HRESULT WINAPI base_hwnd_fragment_GetRuntimeId(IRawElementProviderFragment *iface,
        SAFEARRAY **ret_val)
{
    FIXME("%p, %p: stub!\n", iface, ret_val);
    *ret_val = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI base_hwnd_fragment_get_BoundingRectangle(IRawElementProviderFragment *iface,
        struct UiaRect *ret_val)
{
    struct base_hwnd_provider *base_hwnd_prov = impl_from_base_hwnd_fragment(iface);
    RECT rect = { 0 };

    TRACE("%p, %p\n", iface, ret_val);

    memset(ret_val, 0, sizeof(*ret_val));

    /* Top level minimized window - Return empty rect. */
    if (uia_is_top_level_hwnd(base_hwnd_prov->hwnd) && IsIconic(base_hwnd_prov->hwnd))
        return S_OK;

    if (!GetWindowRect(base_hwnd_prov->hwnd, &rect))
        return uia_get_hr_for_last_error();

    ret_val->left = rect.left;
    ret_val->top = rect.top;
    ret_val->width = (rect.right - rect.left);
    ret_val->height = (rect.bottom - rect.top);

    return S_OK;
}

static HRESULT WINAPI base_hwnd_fragment_GetEmbeddedFragmentRoots(IRawElementProviderFragment *iface,
        SAFEARRAY **ret_val)
{
    FIXME("%p, %p: stub!\n", iface, ret_val);
    *ret_val = NULL;
    return S_OK;
}

static HRESULT WINAPI base_hwnd_fragment_SetFocus(IRawElementProviderFragment *iface)
{
    FIXME("%p: stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI base_hwnd_fragment_get_FragmentRoot(IRawElementProviderFragment *iface,
        IRawElementProviderFragmentRoot **ret_val)
{
    FIXME("%p, %p: stub!\n", iface, ret_val);
    *ret_val = NULL;
    return S_OK;
}

static const IRawElementProviderFragmentVtbl base_hwnd_fragment_vtbl = {
    base_hwnd_fragment_QueryInterface,
    base_hwnd_fragment_AddRef,
    base_hwnd_fragment_Release,
    base_hwnd_fragment_Navigate,
    base_hwnd_fragment_GetRuntimeId,
    base_hwnd_fragment_get_BoundingRectangle,
    base_hwnd_fragment_GetEmbeddedFragmentRoots,
    base_hwnd_fragment_SetFocus,
    base_hwnd_fragment_get_FragmentRoot,
};

HRESULT create_base_hwnd_provider(HWND hwnd, IRawElementProviderSimple **elprov)
{
    struct base_hwnd_provider *base_hwnd_prov;

    *elprov = NULL;

    if (!hwnd)
        return E_INVALIDARG;

    if (!IsWindow(hwnd))
        return UIA_E_ELEMENTNOTAVAILABLE;

    if (!(base_hwnd_prov = calloc(1, sizeof(*base_hwnd_prov))))
        return E_OUTOFMEMORY;

    base_hwnd_prov->IRawElementProviderSimple_iface.lpVtbl = &base_hwnd_provider_vtbl;
    base_hwnd_prov->IRawElementProviderFragment_iface.lpVtbl = &base_hwnd_fragment_vtbl;
    base_hwnd_prov->refcount = 1;
    base_hwnd_prov->hwnd = hwnd;
    *elprov = &base_hwnd_prov->IRawElementProviderSimple_iface;

    return S_OK;
}

/*
 * UI Automation provider thread functions.
 */
struct uia_provider_thread
{
    struct rb_tree node_map;
    struct list nodes_list;
    HANDLE hthread;
    HWND hwnd;
    LONG ref;
};

static struct uia_provider_thread provider_thread;
static CRITICAL_SECTION provider_thread_cs;
static CRITICAL_SECTION_DEBUG provider_thread_cs_debug =
{
    0, 0, &provider_thread_cs,
    { &provider_thread_cs_debug.ProcessLocksList, &provider_thread_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": provider_thread_cs") }
};
static CRITICAL_SECTION provider_thread_cs = { &provider_thread_cs_debug, -1, 0, 0, 0, 0 };

struct uia_provider_thread_map_entry
{
    struct rb_entry entry;

    SAFEARRAY *runtime_id;
    struct list nodes_list;
};

static int uia_runtime_id_compare(const void *key, const struct rb_entry *entry)
{
    struct uia_provider_thread_map_entry *prov_entry = RB_ENTRY_VALUE(entry, struct uia_provider_thread_map_entry, entry);
    return uia_compare_safearrays(prov_entry->runtime_id, (SAFEARRAY *)key, UIAutomationType_IntArray);
}

void uia_provider_thread_remove_node(HUIANODE node)
{
    struct uia_node *node_data = impl_from_IWineUiaNode((IWineUiaNode *)node);

    TRACE("Removing node %p\n", node);

    EnterCriticalSection(&provider_thread_cs);

    list_remove(&node_data->prov_thread_list_entry);
    list_init(&node_data->prov_thread_list_entry);
    if (!list_empty(&node_data->node_map_list_entry))
    {
        list_remove(&node_data->node_map_list_entry);
        list_init(&node_data->node_map_list_entry);
        if (list_empty(&node_data->map->nodes_list))
        {
            rb_remove(&provider_thread.node_map, &node_data->map->entry);
            SafeArrayDestroy(node_data->map->runtime_id);
            free(node_data->map);
        }
        node_data->map = NULL;
    }

    LeaveCriticalSection(&provider_thread_cs);
}

static void uia_provider_thread_disconnect_node(SAFEARRAY *sa)
{
    struct rb_entry *rb_entry;

    EnterCriticalSection(&provider_thread_cs);

    /* Provider thread hasn't been started, no nodes to disconnect. */
    if (!provider_thread.ref)
        goto exit;

    rb_entry = rb_get(&provider_thread.node_map, sa);
    if (rb_entry)
    {
        struct uia_provider_thread_map_entry *prov_map;
        struct list *cursor, *cursor2;
        struct uia_node *node_data;

        prov_map = RB_ENTRY_VALUE(rb_entry, struct uia_provider_thread_map_entry, entry);
        LIST_FOR_EACH_SAFE(cursor, cursor2, &prov_map->nodes_list)
        {
            node_data = LIST_ENTRY(cursor, struct uia_node, node_map_list_entry);

            list_remove(cursor);
            list_remove(&node_data->prov_thread_list_entry);
            list_init(&node_data->prov_thread_list_entry);
            list_init(&node_data->node_map_list_entry);
            node_data->map = NULL;

            IWineUiaNode_disconnect(&node_data->IWineUiaNode_iface);
        }

        rb_remove(&provider_thread.node_map, &prov_map->entry);
        SafeArrayDestroy(prov_map->runtime_id);
        free(prov_map);
    }

exit:
    LeaveCriticalSection(&provider_thread_cs);
}

static HRESULT uia_provider_thread_add_node(HUIANODE node, SAFEARRAY *rt_id)
{
    struct uia_node *node_data = impl_from_IWineUiaNode((IWineUiaNode *)node);
    int prov_type = get_node_provider_type_at_idx(node_data, 0);
    struct uia_provider *prov_data;
    HRESULT hr = S_OK;

    prov_data = impl_from_IWineUiaProvider(node_data->prov[prov_type]);
    node_data->nested_node = prov_data->return_nested_node = prov_data->refuse_hwnd_node_providers = TRUE;

    TRACE("Adding node %p\n", node);

    EnterCriticalSection(&provider_thread_cs);
    list_add_tail(&provider_thread.nodes_list, &node_data->prov_thread_list_entry);

    /* If we have a runtime ID, create an entry in the rb tree. */
    if (rt_id)
    {
        struct uia_provider_thread_map_entry *prov_map;
        struct rb_entry *rb_entry;

        if ((rb_entry = rb_get(&provider_thread.node_map, rt_id)))
            prov_map = RB_ENTRY_VALUE(rb_entry, struct uia_provider_thread_map_entry, entry);
        else
        {
            prov_map = calloc(1, sizeof(*prov_map));
            if (!prov_map)
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }

            hr = SafeArrayCopy(rt_id, &prov_map->runtime_id);
            if (FAILED(hr))
            {
                free(prov_map);
                goto exit;
            }
            list_init(&prov_map->nodes_list);
            rb_put(&provider_thread.node_map, prov_map->runtime_id, &prov_map->entry);
        }

        list_add_tail(&prov_map->nodes_list, &node_data->node_map_list_entry);
        node_data->map = prov_map;
    }

exit:
    LeaveCriticalSection(&provider_thread_cs);

    return hr;
}

#define WM_GET_OBJECT_UIA_NODE (WM_USER + 1)
#define WM_UIA_PROVIDER_THREAD_STOP (WM_USER + 2)
static LRESULT CALLBACK uia_provider_thread_msg_proc(HWND hwnd, UINT msg, WPARAM wparam,
        LPARAM lparam)
{
    switch (msg)
    {
    case WM_GET_OBJECT_UIA_NODE:
    {
        SAFEARRAY *rt_id = (SAFEARRAY *)wparam;
        HUIANODE node = (HUIANODE)lparam;
        LRESULT lr;

        if (FAILED(uia_provider_thread_add_node(node, rt_id)))
        {
            WARN("Failed to add node %p to provider thread list.\n", node);
            return 0;
        }

        /*
         * LresultFromObject returns an index into the global atom string table,
         * which has a valid range of 0xc000-0xffff.
         */
        lr = LresultFromObject(&IID_IWineUiaNode, 0, (IUnknown *)node);
        if ((lr > 0xffff) || (lr < 0xc000))
        {
            WARN("Got invalid lresult %Ix\n", lr);
            lr = 0;
        }

        return lr;
    }

    default:
        break;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

static DWORD WINAPI uia_provider_thread_proc(void *arg)
{
    HANDLE initialized_event = arg;
    HWND hwnd;
    MSG msg;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hwnd = CreateWindowW(L"Message", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
    if (!hwnd)
    {
        WARN("CreateWindow failed: %ld\n", GetLastError());
        CoUninitialize();
        FreeLibraryAndExitThread(huia_module, 1);
    }

    SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)uia_provider_thread_msg_proc);
    provider_thread.hwnd = hwnd;

    /* Initialization complete, thread can now process window messages. */
    SetEvent(initialized_event);
    TRACE("Provider thread started.\n");
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        if (msg.message == WM_UIA_PROVIDER_THREAD_STOP)
            break;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    TRACE("Shutting down UI Automation provider thread.\n");

    DestroyWindow(hwnd);
    CoUninitialize();
    FreeLibraryAndExitThread(huia_module, 0);
}

static BOOL uia_start_provider_thread(void)
{
    BOOL started = TRUE;

    EnterCriticalSection(&provider_thread_cs);
    if (++provider_thread.ref == 1)
    {
        HANDLE ready_event;
        HANDLE events[2];
        HMODULE hmodule;
        DWORD wait_obj;

        /* Increment DLL reference count. */
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                (const WCHAR *)uia_start_provider_thread, &hmodule);

        list_init(&provider_thread.nodes_list);
        rb_init(&provider_thread.node_map, uia_runtime_id_compare);
        events[0] = ready_event = CreateEventW(NULL, FALSE, FALSE, NULL);
        if (!(provider_thread.hthread = CreateThread(NULL, 0, uia_provider_thread_proc,
                ready_event, 0, NULL)))
        {
            FreeLibrary(hmodule);
            started = FALSE;
            goto exit;
        }

        events[1] = provider_thread.hthread;
        wait_obj = WaitForMultipleObjects(2, events, FALSE, INFINITE);
        if (wait_obj != WAIT_OBJECT_0)
        {
            CloseHandle(provider_thread.hthread);
            started = FALSE;
        }

exit:
        CloseHandle(ready_event);
        if (!started)
        {
            WARN("Failed to start provider thread\n");
            memset(&provider_thread, 0, sizeof(provider_thread));
        }
    }

    LeaveCriticalSection(&provider_thread_cs);
    return started;
}

void uia_stop_provider_thread(void)
{
    EnterCriticalSection(&provider_thread_cs);
    if (!--provider_thread.ref)
    {
        PostMessageW(provider_thread.hwnd, WM_UIA_PROVIDER_THREAD_STOP, 0, 0);
        CloseHandle(provider_thread.hthread);
        if (!list_empty(&provider_thread.nodes_list))
            ERR("Provider thread shutdown with nodes still in the list\n");
        memset(&provider_thread, 0, sizeof(provider_thread));
    }
    LeaveCriticalSection(&provider_thread_cs);
}

/*
 * Pass our IWineUiaNode interface to the provider thread for marshaling. UI
 * Automation has to work regardless of whether or not COM is initialized on
 * the thread calling UiaReturnRawElementProvider.
 */
LRESULT uia_lresult_from_node(HUIANODE huianode)
{
    SAFEARRAY *rt_id;
    LRESULT lr = 0;
    HRESULT hr;

    hr = UiaGetRuntimeId(huianode, &rt_id);
    if (SUCCEEDED(hr) && uia_start_provider_thread())
        lr = SendMessageW(provider_thread.hwnd, WM_GET_OBJECT_UIA_NODE, (WPARAM)rt_id, (LPARAM)huianode);

    if (FAILED(hr))
        WARN("UiaGetRuntimeId failed with hr %#lx\n", hr);

    /*
     * LresultFromObject increases refcnt by 1. If LresultFromObject
     * failed or wasn't called, this is expected to release the node.
     */
    UiaNodeRelease(huianode);
    SafeArrayDestroy(rt_id);
    return lr;
}

/***********************************************************************
 *          UiaReturnRawElementProvider (uiautomationcore.@)
 */
LRESULT WINAPI UiaReturnRawElementProvider(HWND hwnd, WPARAM wparam,
        LPARAM lparam, IRawElementProviderSimple *elprov)
{
    HUIANODE node;
    HRESULT hr;

    TRACE("(%p, %Ix, %#Ix, %p)\n", hwnd, wparam, lparam, elprov);

    if (!wparam && !lparam && !elprov)
    {
        FIXME("UIA-to-MSAA bridge not implemented, no provider map to free.\n");
        return 0;
    }

    if (lparam != UiaRootObjectId)
    {
        FIXME("Unsupported object id %Id, ignoring.\n", lparam);
        return 0;
    }

    hr = create_uia_node_from_elprov(elprov, &node, FALSE, 0);
    if (FAILED(hr))
    {
        WARN("Failed to create HUIANODE with hr %#lx\n", hr);
        return 0;
    }

    return uia_lresult_from_node(node);
}

/***********************************************************************
 *          UiaDisconnectProvider (uiautomationcore.@)
 */
HRESULT WINAPI UiaDisconnectProvider(IRawElementProviderSimple *elprov)
{
    SAFEARRAY *sa;
    HUIANODE node;
    HRESULT hr;

    TRACE("(%p)\n", elprov);

    hr = create_uia_node_from_elprov(elprov, &node, FALSE, 0);
    if (FAILED(hr))
        return hr;

    hr = UiaGetRuntimeId(node, &sa);
    UiaNodeRelease(node);
    if (FAILED(hr))
        return hr;

    if (!sa)
        return E_INVALIDARG;

    uia_provider_thread_disconnect_node(sa);

    SafeArrayDestroy(sa);

    return S_OK;
}
