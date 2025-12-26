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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uiautomation);

static HRESULT get_uia_condition_struct_from_iface(IUIAutomationCondition *condition, struct UiaCondition **cond_struct);

/*
 * IUIAutomationOrCondition interface.
 */
struct uia_or_condition {
    IUIAutomationOrCondition IUIAutomationOrCondition_iface;
    LONG ref;

    IUIAutomationCondition **child_ifaces;
    int child_count;

    struct UiaAndOrCondition condition;
};

static inline struct uia_or_condition *impl_from_IUIAutomationOrCondition(IUIAutomationOrCondition *iface)
{
    return CONTAINING_RECORD(iface, struct uia_or_condition, IUIAutomationOrCondition_iface);
}

static HRESULT WINAPI uia_or_condition_QueryInterface(IUIAutomationOrCondition *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUIAutomationOrCondition) || IsEqualIID(riid, &IID_IUIAutomationCondition) ||
            IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IUIAutomationOrCondition_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI uia_or_condition_AddRef(IUIAutomationOrCondition *iface)
{
    struct uia_or_condition *uia_or_condition = impl_from_IUIAutomationOrCondition(iface);
    ULONG ref = InterlockedIncrement(&uia_or_condition->ref);

    TRACE("%p, refcount %ld\n", uia_or_condition, ref);
    return ref;
}

static ULONG WINAPI uia_or_condition_Release(IUIAutomationOrCondition *iface)
{
    struct uia_or_condition *uia_or_condition = impl_from_IUIAutomationOrCondition(iface);
    ULONG ref = InterlockedDecrement(&uia_or_condition->ref);

    TRACE("%p, refcount %ld\n", uia_or_condition, ref);

    if (!ref)
    {
        if (uia_or_condition->child_ifaces)
        {
            int i;

            for (i = 0; i < uia_or_condition->child_count; i++)
            {
                if (uia_or_condition->child_ifaces[i])
                    IUIAutomationCondition_Release(uia_or_condition->child_ifaces[i]);
            }
        }

        free(uia_or_condition->child_ifaces);
        free(uia_or_condition->condition.ppConditions);
        free(uia_or_condition);
    }

    return ref;
}

static HRESULT WINAPI uia_or_condition_get_ChildCount(IUIAutomationOrCondition *iface, int *child_count)
{
    struct uia_or_condition *uia_or_condition = impl_from_IUIAutomationOrCondition(iface);

    TRACE("%p, %p\n", iface, child_count);

    if (!child_count)
        return E_POINTER;

    *child_count = uia_or_condition->child_count;

    return S_OK;
}

static HRESULT WINAPI uia_or_condition_GetChildrenAsNativeArray(IUIAutomationOrCondition *iface,
        IUIAutomationCondition ***out_children, int *out_children_count)
{
    struct uia_or_condition *uia_or_condition = impl_from_IUIAutomationOrCondition(iface);
    IUIAutomationCondition **children;
    int i;

    TRACE("%p, %p, %p\n", iface, out_children, out_children_count);

    if (!out_children)
        return E_POINTER;

    *out_children = NULL;

    if (!out_children_count)
        return E_POINTER;

    if (!(children = CoTaskMemAlloc(uia_or_condition->child_count * sizeof(*children))))
        return E_OUTOFMEMORY;

    for (i = 0; i < uia_or_condition->child_count; i++)
    {
        children[i] = uia_or_condition->child_ifaces[i];
        IUIAutomationCondition_AddRef(uia_or_condition->child_ifaces[i]);
    }

    *out_children = children;
    *out_children_count = uia_or_condition->child_count;

    return S_OK;
}

static HRESULT WINAPI uia_or_condition_GetChildren(IUIAutomationOrCondition *iface, SAFEARRAY **out_children)
{
    FIXME("%p, %p: stub\n", iface, out_children);
    return E_NOTIMPL;
}

static const IUIAutomationOrConditionVtbl uia_or_condition_vtbl = {
    uia_or_condition_QueryInterface,
    uia_or_condition_AddRef,
    uia_or_condition_Release,
    uia_or_condition_get_ChildCount,
    uia_or_condition_GetChildrenAsNativeArray,
    uia_or_condition_GetChildren,
};

static HRESULT create_uia_or_condition_iface(IUIAutomationCondition **out_cond, IUIAutomationCondition **in_conds,
        int in_cond_count)
{
    struct uia_or_condition *uia_or_condition;
    int i;

    if (!out_cond)
        return E_POINTER;

    *out_cond = NULL;

    uia_or_condition = calloc(1, sizeof(*uia_or_condition));
    if (!uia_or_condition)
        return E_OUTOFMEMORY;

    uia_or_condition->IUIAutomationOrCondition_iface.lpVtbl = &uia_or_condition_vtbl;
    uia_or_condition->ref = 1;

    uia_or_condition->child_ifaces = calloc(in_cond_count, sizeof(*in_conds));
    if (!uia_or_condition->child_ifaces)
    {
        IUIAutomationOrCondition_Release(&uia_or_condition->IUIAutomationOrCondition_iface);
        return E_OUTOFMEMORY;
    }

    uia_or_condition->condition.ppConditions = calloc(in_cond_count, sizeof(*uia_or_condition->condition.ppConditions));
    if (!uia_or_condition->condition.ppConditions)
    {
        IUIAutomationOrCondition_Release(&uia_or_condition->IUIAutomationOrCondition_iface);
        return E_OUTOFMEMORY;
    }

    uia_or_condition->condition.ConditionType = ConditionType_Or;
    uia_or_condition->child_count = uia_or_condition->condition.cConditions = in_cond_count;
    for (i = 0; i < in_cond_count; i++)
    {
        HRESULT hr;

        hr = get_uia_condition_struct_from_iface(in_conds[i], &uia_or_condition->condition.ppConditions[i]);
        if (FAILED(hr))
        {
            IUIAutomationOrCondition_Release(&uia_or_condition->IUIAutomationOrCondition_iface);
            return hr;
        }

        uia_or_condition->child_ifaces[i] = in_conds[i];
        IUIAutomationCondition_AddRef(in_conds[i]);
    }

    *out_cond = (IUIAutomationCondition *)&uia_or_condition->IUIAutomationOrCondition_iface;
    return S_OK;
}

/*
 * IUIAutomationNotCondition interface.
 */
struct uia_not_condition {
    IUIAutomationNotCondition IUIAutomationNotCondition_iface;
    LONG ref;

    IUIAutomationCondition *child_iface;
    struct UiaNotCondition condition;
};

static inline struct uia_not_condition *impl_from_IUIAutomationNotCondition(IUIAutomationNotCondition *iface)
{
    return CONTAINING_RECORD(iface, struct uia_not_condition, IUIAutomationNotCondition_iface);
}

static HRESULT WINAPI uia_not_condition_QueryInterface(IUIAutomationNotCondition *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUIAutomationNotCondition) || IsEqualIID(riid, &IID_IUIAutomationCondition) ||
            IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IUIAutomationNotCondition_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI uia_not_condition_AddRef(IUIAutomationNotCondition *iface)
{
    struct uia_not_condition *uia_not_condition = impl_from_IUIAutomationNotCondition(iface);
    ULONG ref = InterlockedIncrement(&uia_not_condition->ref);

    TRACE("%p, refcount %ld\n", uia_not_condition, ref);
    return ref;
}

static ULONG WINAPI uia_not_condition_Release(IUIAutomationNotCondition *iface)
{
    struct uia_not_condition *uia_not_condition = impl_from_IUIAutomationNotCondition(iface);
    ULONG ref = InterlockedDecrement(&uia_not_condition->ref);

    TRACE("%p, refcount %ld\n", uia_not_condition, ref);
    if (!ref)
    {
        IUIAutomationCondition_Release(uia_not_condition->child_iface);
        free(uia_not_condition);
    }

    return ref;
}

static HRESULT WINAPI uia_not_condition_GetChild(IUIAutomationNotCondition *iface, IUIAutomationCondition **child)
{
    struct uia_not_condition *uia_not_condition = impl_from_IUIAutomationNotCondition(iface);

    TRACE("%p, %p\n", iface, child);

    if (!child)
        return E_POINTER;

    IUIAutomationCondition_AddRef(uia_not_condition->child_iface);
    *child = uia_not_condition->child_iface;

    return S_OK;
}

static const IUIAutomationNotConditionVtbl uia_not_condition_vtbl = {
    uia_not_condition_QueryInterface,
    uia_not_condition_AddRef,
    uia_not_condition_Release,
    uia_not_condition_GetChild,
};

static HRESULT create_uia_not_condition_iface(IUIAutomationCondition **out_cond, IUIAutomationCondition *in_cond)
{
    struct uia_not_condition *uia_not_condition;
    struct UiaCondition *cond_struct;
    HRESULT hr;

    if (!out_cond)
        return E_POINTER;

    *out_cond = NULL;
    hr = get_uia_condition_struct_from_iface(in_cond, &cond_struct);
    if (FAILED(hr))
        return hr;

    uia_not_condition = calloc(1, sizeof(*uia_not_condition));
    if (!uia_not_condition)
        return E_OUTOFMEMORY;

    uia_not_condition->IUIAutomationNotCondition_iface.lpVtbl = &uia_not_condition_vtbl;
    uia_not_condition->condition.ConditionType = ConditionType_Not;
    uia_not_condition->condition.pConditions = cond_struct;
    uia_not_condition->ref = 1;
    uia_not_condition->child_iface = in_cond;
    IUIAutomationCondition_AddRef(in_cond);

    *out_cond = (IUIAutomationCondition *)&uia_not_condition->IUIAutomationNotCondition_iface;
    return S_OK;
}

/*
 * IUIAutomationPropertyCondition interface.
 */
struct uia_property_condition {
    IUIAutomationPropertyCondition IUIAutomationPropertyCondition_iface;
    LONG ref;

    struct UiaPropertyCondition condition;
};

static inline struct uia_property_condition *impl_from_IUIAutomationPropertyCondition(IUIAutomationPropertyCondition *iface)
{
    return CONTAINING_RECORD(iface, struct uia_property_condition, IUIAutomationPropertyCondition_iface);
}

static HRESULT WINAPI uia_property_condition_QueryInterface(IUIAutomationPropertyCondition *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUIAutomationPropertyCondition) || IsEqualIID(riid, &IID_IUIAutomationCondition) ||
            IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IUIAutomationPropertyCondition_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI uia_property_condition_AddRef(IUIAutomationPropertyCondition *iface)
{
    struct uia_property_condition *uia_property_condition = impl_from_IUIAutomationPropertyCondition(iface);
    ULONG ref = InterlockedIncrement(&uia_property_condition->ref);

    TRACE("%p, refcount %ld\n", uia_property_condition, ref);
    return ref;
}

static ULONG WINAPI uia_property_condition_Release(IUIAutomationPropertyCondition *iface)
{
    struct uia_property_condition *uia_property_condition = impl_from_IUIAutomationPropertyCondition(iface);
    ULONG ref = InterlockedDecrement(&uia_property_condition->ref);

    TRACE("%p, refcount %ld\n", uia_property_condition, ref);

    if (!ref)
    {
        VariantClear(&uia_property_condition->condition.Value);
        free(uia_property_condition);
    }

    return ref;
}

static HRESULT WINAPI uia_property_condition_get_PropertyId(IUIAutomationPropertyCondition *iface, PROPERTYID *prop_id)
{
    struct uia_property_condition *uia_property_condition = impl_from_IUIAutomationPropertyCondition(iface);

    TRACE("%p, %p\n", iface, prop_id);

    if (!prop_id)
        return E_POINTER;

    *prop_id = uia_property_condition->condition.PropertyId;

    return S_OK;
}

static HRESULT WINAPI uia_property_condition_get_PropertyValue(IUIAutomationPropertyCondition *iface, VARIANT *val)
{
    struct uia_property_condition *uia_property_condition = impl_from_IUIAutomationPropertyCondition(iface);

    TRACE("%p, %p\n", iface, val);

    if (!val)
        return E_POINTER;

    VariantCopy(val, &uia_property_condition->condition.Value);

    return S_OK;
}

static HRESULT WINAPI uia_property_condition_get_PropertyConditionFlags(IUIAutomationPropertyCondition *iface,
        enum PropertyConditionFlags *flags)
{
    struct uia_property_condition *uia_property_condition = impl_from_IUIAutomationPropertyCondition(iface);

    TRACE("%p, %p\n", iface, flags);

    if (!flags)
        return E_POINTER;

    *flags = uia_property_condition->condition.Flags;

    return S_OK;
}

static const IUIAutomationPropertyConditionVtbl uia_property_condition_vtbl = {
    uia_property_condition_QueryInterface,
    uia_property_condition_AddRef,
    uia_property_condition_Release,
    uia_property_condition_get_PropertyId,
    uia_property_condition_get_PropertyValue,
    uia_property_condition_get_PropertyConditionFlags,
};

static HRESULT create_uia_property_condition_iface(IUIAutomationCondition **out_cond, PROPERTYID prop_id, VARIANT val,
        enum PropertyConditionFlags prop_flags)
{
    const struct uia_prop_info *prop_info = uia_prop_info_from_id(prop_id);
    struct uia_property_condition *uia_property_condition;

    if (!out_cond)
        return E_POINTER;

    *out_cond = NULL;
    if (!prop_info)
        return E_INVALIDARG;

    switch (prop_info->type)
    {
    case UIAutomationType_Bool:
        if (V_VT(&val) != VT_BOOL)
            return E_INVALIDARG;
        break;

    case UIAutomationType_IntArray:
        if (V_VT(&val) != (VT_I4 | VT_ARRAY))
            return E_INVALIDARG;
        break;

    default:
        FIXME("Property condition evaluation for property type %#x unimplemented\n", prop_info->type);
        return E_NOTIMPL;
    }

    uia_property_condition = calloc(1, sizeof(*uia_property_condition));
    if (!uia_property_condition)
        return E_OUTOFMEMORY;

    uia_property_condition->IUIAutomationPropertyCondition_iface.lpVtbl = &uia_property_condition_vtbl;
    uia_property_condition->condition.ConditionType = ConditionType_Property;
    uia_property_condition->condition.PropertyId = prop_id;
    VariantCopy(&uia_property_condition->condition.Value, &val);
    uia_property_condition->condition.Flags = prop_flags;
    uia_property_condition->ref = 1;

    *out_cond = (IUIAutomationCondition *)&uia_property_condition->IUIAutomationPropertyCondition_iface;
    return S_OK;
}

/*
 * IUIAutomationBoolCondition interface.
 */
struct uia_bool_condition {
    IUIAutomationBoolCondition IUIAutomationBoolCondition_iface;
    LONG ref;

    struct UiaCondition condition;
};

static inline struct uia_bool_condition *impl_from_IUIAutomationBoolCondition(IUIAutomationBoolCondition *iface)
{
    return CONTAINING_RECORD(iface, struct uia_bool_condition, IUIAutomationBoolCondition_iface);
}

static HRESULT WINAPI uia_bool_condition_QueryInterface(IUIAutomationBoolCondition *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUIAutomationBoolCondition) || IsEqualIID(riid, &IID_IUIAutomationCondition) ||
            IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IUIAutomationBoolCondition_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI uia_bool_condition_AddRef(IUIAutomationBoolCondition *iface)
{
    struct uia_bool_condition *uia_bool_condition = impl_from_IUIAutomationBoolCondition(iface);
    ULONG ref = InterlockedIncrement(&uia_bool_condition->ref);

    TRACE("%p, refcount %ld\n", uia_bool_condition, ref);
    return ref;
}

static ULONG WINAPI uia_bool_condition_Release(IUIAutomationBoolCondition *iface)
{
    struct uia_bool_condition *uia_bool_condition = impl_from_IUIAutomationBoolCondition(iface);
    ULONG ref = InterlockedDecrement(&uia_bool_condition->ref);

    TRACE("%p, refcount %ld\n", uia_bool_condition, ref);
    if (!ref)
        free(uia_bool_condition);

    return ref;
}

static HRESULT WINAPI uia_bool_condition_get_BooleanValue(IUIAutomationBoolCondition *iface, BOOL *ret_val)
{
    struct uia_bool_condition *uia_bool_condition = impl_from_IUIAutomationBoolCondition(iface);

    TRACE("%p, %p\n", iface, ret_val);

    if (!ret_val)
        return E_POINTER;

    if (uia_bool_condition->condition.ConditionType == ConditionType_True)
        *ret_val = TRUE;
    else
        *ret_val = FALSE;

    return S_OK;
}

static const IUIAutomationBoolConditionVtbl uia_bool_condition_vtbl = {
    uia_bool_condition_QueryInterface,
    uia_bool_condition_AddRef,
    uia_bool_condition_Release,
    uia_bool_condition_get_BooleanValue,
};

static HRESULT create_uia_bool_condition_iface(IUIAutomationCondition **out_cond, enum ConditionType cond_type)
{
    struct uia_bool_condition *uia_bool_condition;

    if (!out_cond)
        return E_POINTER;

    uia_bool_condition = calloc(1, sizeof(*uia_bool_condition));
    if (!uia_bool_condition)
        return E_OUTOFMEMORY;

    uia_bool_condition->IUIAutomationBoolCondition_iface.lpVtbl = &uia_bool_condition_vtbl;
    uia_bool_condition->condition.ConditionType = cond_type;
    uia_bool_condition->ref = 1;

    *out_cond = (IUIAutomationCondition *)&uia_bool_condition->IUIAutomationBoolCondition_iface;
    return S_OK;
}

static HRESULT get_uia_condition_struct_from_iface(IUIAutomationCondition *condition, struct UiaCondition **cond_struct)
{
    *cond_struct = NULL;
    if (!condition)
        return E_POINTER;

    if (condition->lpVtbl == (IUIAutomationConditionVtbl *)&uia_bool_condition_vtbl)
    {
        struct uia_bool_condition *cond;

        cond = impl_from_IUIAutomationBoolCondition((IUIAutomationBoolCondition *)condition);
        *cond_struct = &cond->condition;
    }
    else if (condition->lpVtbl == (IUIAutomationConditionVtbl *)&uia_property_condition_vtbl)
    {
        struct uia_property_condition *cond;

        cond = impl_from_IUIAutomationPropertyCondition((IUIAutomationPropertyCondition *)condition);
        *cond_struct = (struct UiaCondition *)&cond->condition;
    }
    else if (condition->lpVtbl == (IUIAutomationConditionVtbl *)&uia_not_condition_vtbl)
    {
        struct uia_not_condition *cond;

        cond = impl_from_IUIAutomationNotCondition((IUIAutomationNotCondition *)condition);
        *cond_struct = (struct UiaCondition *)&cond->condition;
    }
    else if (condition->lpVtbl == (IUIAutomationConditionVtbl *)&uia_or_condition_vtbl)
    {
        struct uia_or_condition *cond;

        cond = impl_from_IUIAutomationOrCondition((IUIAutomationOrCondition *)condition);
        *cond_struct = (struct UiaCondition *)&cond->condition;
    }
    else
        return E_FAIL;

    return S_OK;
}

static HRESULT create_control_view_condition_iface(IUIAutomationCondition **out_condition)
{
    IUIAutomationCondition *prop_cond, *not_cond;
    HRESULT hr;
    VARIANT v;

    if (!out_condition)
        return E_POINTER;

    *out_condition = NULL;

    VariantInit(&v);
    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = VARIANT_FALSE;
    hr = create_uia_property_condition_iface(&prop_cond, UIA_IsControlElementPropertyId, v, PropertyConditionFlags_None);
    if (FAILED(hr))
        return hr;

    hr = create_uia_not_condition_iface(&not_cond, prop_cond);
    if (FAILED(hr))
    {
        IUIAutomationCondition_Release(prop_cond);
        return hr;
    }

    *out_condition = not_cond;

    return S_OK;
}

/*
 * IUIAutomationCacheRequest interface.
 */
struct uia_cache_request {
    IUIAutomationCacheRequest IUIAutomationCacheRequest_iface;
    LONG ref;

    IUIAutomationCondition *view_condition;
    struct UiaCacheRequest cache_req;

    int *prop_ids;
    int prop_ids_count;
    SIZE_T prop_ids_arr_size;
};

static inline struct uia_cache_request *impl_from_IUIAutomationCacheRequest(IUIAutomationCacheRequest *iface)
{
    return CONTAINING_RECORD(iface, struct uia_cache_request, IUIAutomationCacheRequest_iface);
}

static HRESULT WINAPI uia_cache_request_QueryInterface(IUIAutomationCacheRequest *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUIAutomationCacheRequest) || IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IUIAutomationCacheRequest_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI uia_cache_request_AddRef(IUIAutomationCacheRequest *iface)
{
    struct uia_cache_request *uia_cache_request = impl_from_IUIAutomationCacheRequest(iface);
    ULONG ref = InterlockedIncrement(&uia_cache_request->ref);

    TRACE("%p, refcount %ld\n", uia_cache_request, ref);
    return ref;
}

static ULONG WINAPI uia_cache_request_Release(IUIAutomationCacheRequest *iface)
{
    struct uia_cache_request *uia_cache_request = impl_from_IUIAutomationCacheRequest(iface);
    ULONG ref = InterlockedDecrement(&uia_cache_request->ref);

    TRACE("%p, refcount %ld\n", uia_cache_request, ref);

    if (!ref)
    {
        IUIAutomationCondition_Release(uia_cache_request->view_condition);
        free(uia_cache_request->prop_ids);
        free(uia_cache_request);
    }

    return ref;
}

static HRESULT WINAPI uia_cache_request_AddProperty(IUIAutomationCacheRequest *iface, PROPERTYID prop_id)
{
    struct uia_cache_request *uia_cache_request = impl_from_IUIAutomationCacheRequest(iface);
    const struct uia_prop_info *prop_info = uia_prop_info_from_id(prop_id);
    int i;

    TRACE("%p, %d\n", iface, prop_id);

    if (!prop_info)
        return E_INVALIDARG;

    /* Don't add a duplicate property to the cache request. */
    for (i = 0; i < uia_cache_request->prop_ids_count; i++)
    {
        if (uia_cache_request->prop_ids[i] == prop_id)
            return S_OK;
    }

    if (!uia_array_reserve((void **)&uia_cache_request->prop_ids, &uia_cache_request->prop_ids_arr_size,
                uia_cache_request->prop_ids_count + 1, sizeof(*uia_cache_request->prop_ids)))
        return E_OUTOFMEMORY;

    uia_cache_request->prop_ids[uia_cache_request->prop_ids_count] = prop_id;
    uia_cache_request->prop_ids_count++;

    uia_cache_request->cache_req.pProperties = uia_cache_request->prop_ids;
    uia_cache_request->cache_req.cProperties = uia_cache_request->prop_ids_count;

    return S_OK;
}

static HRESULT WINAPI uia_cache_request_AddPattern(IUIAutomationCacheRequest *iface, PATTERNID pattern_id)
{
    FIXME("%p, %d: stub\n", iface, pattern_id);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_cache_request_Clone(IUIAutomationCacheRequest *iface, IUIAutomationCacheRequest **out_req)
{
    FIXME("%p, %p: stub\n", iface, out_req);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_cache_request_get_TreeScope(IUIAutomationCacheRequest *iface, enum TreeScope *scope)
{
    struct uia_cache_request *uia_cache_request = impl_from_IUIAutomationCacheRequest(iface);

    TRACE("%p, %p\n", iface, scope);

    if (!scope)
        return E_POINTER;

    *scope = uia_cache_request->cache_req.Scope;

    return S_OK;
}

static HRESULT WINAPI uia_cache_request_put_TreeScope(IUIAutomationCacheRequest *iface, enum TreeScope scope)
{
    struct uia_cache_request *uia_cache_request = impl_from_IUIAutomationCacheRequest(iface);

    TRACE("%p, %#x\n", iface, scope);

    if (!scope || (scope & ~TreeScope_Subtree))
        return E_INVALIDARG;

    if ((scope & TreeScope_Children) || (scope & TreeScope_Descendants))
    {
        FIXME("Unimplemented TreeScope %#x\n", scope);
        return E_NOTIMPL;
    }

    uia_cache_request->cache_req.Scope = scope;

    return S_OK;
}

static HRESULT WINAPI uia_cache_request_get_TreeFilter(IUIAutomationCacheRequest *iface,
        IUIAutomationCondition **filter)
{
    struct uia_cache_request *uia_cache_request = impl_from_IUIAutomationCacheRequest(iface);

    TRACE("%p, %p\n", iface, filter);

    if (!filter)
        return E_POINTER;

    IUIAutomationCondition_AddRef(uia_cache_request->view_condition);
    *filter = uia_cache_request->view_condition;

    return S_OK;
}

static HRESULT WINAPI uia_cache_request_put_TreeFilter(IUIAutomationCacheRequest *iface, IUIAutomationCondition *filter)
{
    struct uia_cache_request *uia_cache_request = impl_from_IUIAutomationCacheRequest(iface);
    struct UiaCondition *cond_struct;
    HRESULT hr;

    TRACE("%p, %p\n", iface, filter);

    if (!filter)
        return E_POINTER;

    hr = get_uia_condition_struct_from_iface(filter, &cond_struct);
    if (FAILED(hr))
        return hr;

    uia_cache_request->cache_req.pViewCondition = cond_struct;
    IUIAutomationCondition_Release(uia_cache_request->view_condition);
    uia_cache_request->view_condition = filter;
    IUIAutomationCondition_AddRef(filter);

    return S_OK;
}

static HRESULT WINAPI uia_cache_request_get_AutomationElementMode(IUIAutomationCacheRequest *iface,
        enum AutomationElementMode *mode)
{
    struct uia_cache_request *uia_cache_request = impl_from_IUIAutomationCacheRequest(iface);

    TRACE("%p, %p\n", iface, mode);

    if (!mode)
        return E_POINTER;

    *mode = uia_cache_request->cache_req.automationElementMode;

    return S_OK;
}

static HRESULT WINAPI uia_cache_request_put_AutomationElementMode(IUIAutomationCacheRequest *iface,
        enum AutomationElementMode mode)
{
    struct uia_cache_request *uia_cache_request = impl_from_IUIAutomationCacheRequest(iface);

    TRACE("%p, %d\n", iface, mode);

    if ((mode != AutomationElementMode_Full) && (mode != AutomationElementMode_None))
        return E_INVALIDARG;

    if (mode == AutomationElementMode_None)
    {
        FIXME("AutomationElementMode_None unsupported\n");
        return E_NOTIMPL;
    }

    uia_cache_request->cache_req.automationElementMode = mode;

    return S_OK;
}

static const IUIAutomationCacheRequestVtbl uia_cache_request_vtbl = {
    uia_cache_request_QueryInterface,
    uia_cache_request_AddRef,
    uia_cache_request_Release,
    uia_cache_request_AddProperty,
    uia_cache_request_AddPattern,
    uia_cache_request_Clone,
    uia_cache_request_get_TreeScope,
    uia_cache_request_put_TreeScope,
    uia_cache_request_get_TreeFilter,
    uia_cache_request_put_TreeFilter,
    uia_cache_request_get_AutomationElementMode,
    uia_cache_request_put_AutomationElementMode,
};

static HRESULT create_uia_cache_request_iface(IUIAutomationCacheRequest **out_cache_req)
{
    struct uia_cache_request *uia_cache_request;
    IUIAutomationCondition *view_condition;
    HRESULT hr;

    if (!out_cache_req)
        return E_POINTER;

    *out_cache_req = NULL;
    hr = create_control_view_condition_iface(&view_condition);
    if (FAILED(hr))
        return hr;

    uia_cache_request = calloc(1, sizeof(*uia_cache_request));
    if (!uia_cache_request)
    {
        IUIAutomationCondition_Release(view_condition);
        return E_OUTOFMEMORY;
    }

    uia_cache_request->IUIAutomationCacheRequest_iface.lpVtbl = &uia_cache_request_vtbl;
    uia_cache_request->ref = 1;

    uia_cache_request->view_condition = view_condition;
    get_uia_condition_struct_from_iface(view_condition, &uia_cache_request->cache_req.pViewCondition);
    uia_cache_request->cache_req.Scope = TreeScope_Element;
    uia_cache_request->cache_req.automationElementMode = AutomationElementMode_Full;

    *out_cache_req = &uia_cache_request->IUIAutomationCacheRequest_iface;
    return S_OK;
}

static HRESULT get_uia_cache_request_struct_from_iface(IUIAutomationCacheRequest *cache_request,
        struct UiaCacheRequest **cache_req_struct)
{
    struct uia_cache_request *cache_req_data;

    *cache_req_struct = NULL;
    if (!cache_request)
        return E_POINTER;

    if (cache_request->lpVtbl != &uia_cache_request_vtbl)
        return E_FAIL;

    cache_req_data = impl_from_IUIAutomationCacheRequest(cache_request);
    *cache_req_struct = &cache_req_data->cache_req;

    return S_OK;
}

/*
 * COM API UI Automation event related functions.
 */
static struct uia_com_event_handlers
{
    struct rb_tree handler_event_id_map;
    struct rb_tree handler_map;

    LONG handler_count;
} com_event_handlers;

static CRITICAL_SECTION com_event_handlers_cs;
static CRITICAL_SECTION_DEBUG com_event_handlers_cs_debug =
{
    0, 0, &com_event_handlers_cs,
    { &com_event_handlers_cs_debug.ProcessLocksList, &com_event_handlers_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": com_event_handlers_cs") }
};
static CRITICAL_SECTION com_event_handlers_cs = { &com_event_handlers_cs_debug, -1, 0, 0, 0, 0 };

struct uia_event_handler_event_id_map_entry
{
    struct rb_entry entry;
    int event_id;

    struct list handlers_list;
};

static int uia_com_event_handler_event_id_compare(const void *key, const struct rb_entry *entry)
{
    struct uia_event_handler_event_id_map_entry *event_entry = RB_ENTRY_VALUE(entry, struct uia_event_handler_event_id_map_entry, entry);
    int event_id = *((int *)key);

    return (event_entry->event_id > event_id) - (event_entry->event_id < event_id);
}

struct uia_event_handler_identifier {
    IUnknown *handler_iface;
    SAFEARRAY *runtime_id;
    int event_id;
};

struct uia_event_handler_map_entry
{
    struct rb_entry entry;

    IUnknown *handler_iface;
    SAFEARRAY *runtime_id;
    int event_id;

    struct list handlers_list;

    struct uia_event_handler_event_id_map_entry *handler_event_id_map;
    struct list handler_event_id_map_list_entry;
};

static int uia_com_event_handler_id_compare(const void *key, const struct rb_entry *entry)
{
    struct uia_event_handler_map_entry *map_entry = RB_ENTRY_VALUE(entry, struct uia_event_handler_map_entry, entry);
    struct uia_event_handler_identifier *event_id = (struct uia_event_handler_identifier *)key;

    if (event_id->event_id != map_entry->event_id)
        return (event_id->event_id > map_entry->event_id) - (event_id->event_id < map_entry->event_id);
    else if (event_id->handler_iface != map_entry->handler_iface)
        return (event_id->handler_iface > map_entry->handler_iface) - (event_id->handler_iface < map_entry->handler_iface);
    else if (event_id->runtime_id && map_entry->runtime_id)
        return uia_compare_safearrays(event_id->runtime_id, map_entry->runtime_id, UIAutomationType_IntArray);
    else
        return (event_id->runtime_id > map_entry->runtime_id) - (event_id->runtime_id < map_entry->runtime_id);
}

struct uia_com_event {
    DWORD git_cookie;
    HUIAEVENT event;
    BOOL from_cui8;

    struct rb_tree focus_hwnd_map;
    struct list event_handler_map_list_entry;
    struct uia_event_handler_map_entry *handler_map;
};

static HRESULT uia_com_focus_win_event_callback(struct uia_event *event, void *user_data)
{
    struct uia_node *node = impl_from_IWineUiaNode((IWineUiaNode *)user_data);
    VARIANT v, v2;
    HRESULT hr;

    /* Only match desktop events. */
    if (!event->desktop_subtree_event)
        return S_OK;

    VariantInit(&v);
    VariantInit(&v2);

    hr = create_node_from_node_provider(&node->IWineUiaNode_iface, 0, PROV_METHOD_FLAG_RETURN_NODE_LRES, &v);
    if (FAILED(hr))
    {
        WARN("Failed to create new node lres with hr %#lx\n", hr);
        return hr;
    }

    if (V_VT(&v) == VT_I4)
    {
        hr = IWineUiaEvent_raise_event(&event->IWineUiaEvent_iface, v, v2);
        if (FAILED(hr))
        {
            WARN("raise_event failed with hr %#lx\n", hr);
            uia_node_lresult_release(V_I4(&v));
        }
    }
    VariantClear(&v);

    return hr;
}

static BOOL uia_com_focus_handler_advise_node(struct uia_com_event *event, HUIANODE node, HWND hwnd)
{
    HRESULT hr;

    hr = uia_event_advise_node((struct uia_event *)event->event, node);
    if (FAILED(hr))
    {
        WARN("uia_event_advise_node failed with hr %#lx\n", hr);
        goto exit;
    }

    hr = uia_hwnd_map_add_hwnd(&event->focus_hwnd_map, hwnd);
    if (FAILED(hr))
        WARN("Failed to add hwnd for focus winevent, hr %#lx\n", hr);

exit:
    return SUCCEEDED(hr);
}

static void uia_com_focus_win_event_handler(HUIANODE node, HWND hwnd, struct uia_event_handler_event_id_map_entry *event_id_map)
{
    struct uia_event_handler_map_entry *entry;
    HRESULT hr;
    VARIANT v;

    LIST_FOR_EACH_ENTRY(entry, &event_id_map->handlers_list, struct uia_event_handler_map_entry, handler_event_id_map_list_entry)
    {
        struct uia_com_event *event;

        LIST_FOR_EACH_ENTRY(event, &entry->handlers_list, struct uia_com_event, event_handler_map_list_entry)
        {
            if (uia_hwnd_map_check_hwnd(&event->focus_hwnd_map, hwnd) ||
                    !uia_com_focus_handler_advise_node(event, node, hwnd))
                continue;

            hr = get_focus_from_node_provider((IWineUiaNode *)node, 0, PROV_METHOD_FLAG_RETURN_NODE_LRES, &v);
            if (V_VT(&v) == VT_I4)
            {
                HUIANODE focus_node = NULL;

                hr = uia_node_from_lresult(V_I4(&v), &focus_node, NODE_FLAG_IGNORE_CLIENTSIDE_HWND_PROVS);
                if (SUCCEEDED(hr))
                {
                    hr = uia_event_for_each(UIA_AutomationFocusChangedEventId, uia_com_focus_win_event_callback,
                            (void *)focus_node, TRUE);
                    if (FAILED(hr))
                        WARN("uia_event_for_each on focus_node failed with hr %#lx\n", hr);
                }
                UiaNodeRelease(focus_node);
            }
            VariantClear(&v);
        }
    }

    VariantInit(&v);
    hr = UiaGetPropertyValue(node, UIA_HasKeyboardFocusPropertyId, &v);
    if (SUCCEEDED(hr) && (V_VT(&v) == VT_BOOL && V_BOOL(&v) == VARIANT_TRUE))
    {
        hr = uia_event_for_each(UIA_AutomationFocusChangedEventId, uia_com_focus_win_event_callback, (void *)node, TRUE);
        if (FAILED(hr))
            WARN("uia_event_for_each failed with hr %#lx\n", hr);
    }

    VariantClear(&v);
}

static HRESULT uia_com_focus_win_event_msaa_callback(struct uia_event *event, void *user_data)
{
    struct uia_event_args args = { { EventArgsType_Simple, UIA_AutomationFocusChangedEventId }, 0 };
    HUIANODE node = (HUIANODE)user_data;

    /* Only match desktop events. */
    if (!event->desktop_subtree_event)
        return S_OK;

    return uia_event_invoke(node, NULL, &args, event);
}

static void uia_com_focus_win_event_msaa_handler(HWND hwnd, LONG child_id)
{
    IRawElementProviderFragmentRoot *elroot;
    IRawElementProviderFragment *elfrag;
    IRawElementProviderSimple *elprov;
    HRESULT hr;
    VARIANT v;

    hr = create_msaa_provider_from_hwnd(hwnd, child_id, &elprov);
    if (FAILED(hr))
    {
        WARN("create_msaa_provider_from_hwnd failed with hr %#lx\n", hr);
        return;
    }

    hr = IRawElementProviderSimple_QueryInterface(elprov, &IID_IRawElementProviderFragmentRoot, (void **)&elroot);
    if (FAILED(hr))
        goto exit;

    hr = IRawElementProviderFragmentRoot_GetFocus(elroot, &elfrag);
    IRawElementProviderFragmentRoot_Release(elroot);
    if (FAILED(hr))
        goto exit;

    if (elfrag)
    {
        IRawElementProviderSimple *elprov2;

        hr = IRawElementProviderFragment_QueryInterface(elfrag, &IID_IRawElementProviderSimple, (void **)&elprov2);
        IRawElementProviderFragment_Release(elfrag);
        if (FAILED(hr))
            goto exit;

        IRawElementProviderSimple_Release(elprov);
        elprov = elprov2;
    }

    VariantInit(&v);
    hr = IRawElementProviderSimple_GetPropertyValue(elprov, UIA_HasKeyboardFocusPropertyId, &v);
    if (FAILED(hr))
        goto exit;

    if (V_VT(&v) == VT_BOOL && V_BOOL(&v) == VARIANT_TRUE)
    {
        HUIANODE node;

        hr = create_uia_node_from_elprov(elprov, &node, TRUE, 0);
        if (SUCCEEDED(hr))
        {
            hr = uia_event_for_each(UIA_AutomationFocusChangedEventId, uia_com_focus_win_event_msaa_callback, (void *)node, TRUE);
            if (FAILED(hr))
                WARN("uia_event_for_each failed with hr %#lx\n", hr);
            UiaNodeRelease(node);
        }
    }

exit:
    IRawElementProviderSimple_Release(elprov);
}

HRESULT uia_com_win_event_callback(DWORD event_id, HWND hwnd, LONG obj_id, LONG child_id, DWORD thread_id, DWORD event_time)
{
    LONG handler_count;

    TRACE("%ld, %p, %ld, %ld, %ld, %ld\n", event_id, hwnd, obj_id, child_id, thread_id, event_time);

    EnterCriticalSection(&com_event_handlers_cs);
    handler_count = com_event_handlers.handler_count;
    LeaveCriticalSection(&com_event_handlers_cs);

    if (!handler_count)
        return S_OK;

    switch (event_id)
    {
    case EVENT_OBJECT_SHOW:
    {
        struct uia_event_handler_map_entry *entry;
        SAFEARRAY *rt_id = NULL;
        HUIANODE node;
        HRESULT hr;

        if (obj_id != OBJID_WINDOW || !uia_hwnd_is_visible(hwnd))
            break;

        hr = UiaNodeFromHandle(hwnd, &node);
        if (FAILED(hr))
            return hr;

        hr = UiaGetRuntimeId(node, &rt_id);
        if (FAILED(hr))
        {
            UiaNodeRelease(node);
            return hr;
        }

        EnterCriticalSection(&com_event_handlers_cs);

        RB_FOR_EACH_ENTRY(entry, &com_event_handlers.handler_map, struct uia_event_handler_map_entry, entry)
        {
            struct uia_com_event *event;

            /*
             * Focus change event handlers only listen for EVENT_OBJECT_SHOW
             * on the desktop HWND.
             */
            if ((entry->event_id == UIA_AutomationFocusChangedEventId) && (hwnd != GetDesktopWindow()))
                continue;

            LIST_FOR_EACH_ENTRY(event, &entry->handlers_list, struct uia_com_event, event_handler_map_list_entry)
            {
                hr = uia_event_check_node_within_event_scope((struct uia_event *)event->event, node, rt_id, NULL);
                if (FAILED(hr))
                    WARN("uia_event_check_node_within_scope failed with hr %#lx\n", hr);
                else if (hr == S_OK)
                {
                    hr = uia_event_advise_node((struct uia_event *)event->event, node);
                    if (FAILED(hr))
                        WARN("uia_event_advise_node failed with hr %#lx\n", hr);
                }
            }
        }

        LeaveCriticalSection(&com_event_handlers_cs);

        UiaNodeRelease(node);
        break;
    }

    case EVENT_OBJECT_FOCUS:
    {
        static const int uia_event_id = UIA_AutomationFocusChangedEventId;
        struct rb_entry *rb_entry;
        HRESULT hr;

        if (obj_id != OBJID_CLIENT)
            break;

        EnterCriticalSection(&com_event_handlers_cs);

        if ((rb_entry = rb_get(&com_event_handlers.handler_event_id_map, &uia_event_id)))
        {
            struct uia_event_handler_event_id_map_entry *event_id_map;
            HUIANODE node = NULL;

            event_id_map = RB_ENTRY_VALUE(rb_entry, struct uia_event_handler_event_id_map_entry, entry);
            hr = create_uia_node_from_hwnd(hwnd, &node, NODE_FLAG_IGNORE_CLIENTSIDE_HWND_PROVS);
            if (SUCCEEDED(hr))
                uia_com_focus_win_event_handler(node, hwnd, event_id_map);
            else
                uia_com_focus_win_event_msaa_handler(hwnd, child_id);

            UiaNodeRelease(node);
        }

        LeaveCriticalSection(&com_event_handlers_cs);
        break;
    }

    case EVENT_OBJECT_DESTROY:
    {
        static const int uia_event_id = UIA_AutomationFocusChangedEventId;
        struct rb_entry *rb_entry;

        if (obj_id != OBJID_WINDOW)
            break;

        EnterCriticalSection(&com_event_handlers_cs);

        if ((rb_entry = rb_get(&com_event_handlers.handler_event_id_map, &uia_event_id)))
        {
            struct uia_event_handler_event_id_map_entry *event_id_map;
            struct uia_event_handler_map_entry *entry;

            event_id_map = RB_ENTRY_VALUE(rb_entry, struct uia_event_handler_event_id_map_entry, entry);
            LIST_FOR_EACH_ENTRY(entry, &event_id_map->handlers_list, struct uia_event_handler_map_entry,
                    handler_event_id_map_list_entry)
            {
                struct uia_com_event *event;

                LIST_FOR_EACH_ENTRY(event, &entry->handlers_list, struct uia_com_event, event_handler_map_list_entry)
                {
                    uia_hwnd_map_remove_hwnd(&event->focus_hwnd_map, hwnd);
                }
            }
        }

        LeaveCriticalSection(&com_event_handlers_cs);
        break;
    }

    default:
        break;
    }

    return S_OK;
}

static HRESULT uia_event_handlers_add_handler_to_event_id_map(struct uia_event_handler_map_entry *event_map)
{
    struct uia_event_handler_event_id_map_entry *event_id_map;
    struct rb_entry *rb_entry;

    if ((rb_entry = rb_get(&com_event_handlers.handler_event_id_map, &event_map->event_id)))
        event_id_map = RB_ENTRY_VALUE(rb_entry, struct uia_event_handler_event_id_map_entry, entry);
    else
    {
        if (!(event_id_map = calloc(1, sizeof(*event_id_map))))
            return E_OUTOFMEMORY;

        event_id_map->event_id = event_map->event_id;
        list_init(&event_id_map->handlers_list);
        rb_put(&com_event_handlers.handler_event_id_map, &event_map->event_id, &event_id_map->entry);
    }

    list_add_tail(&event_id_map->handlers_list, &event_map->handler_event_id_map_list_entry);
    event_map->handler_event_id_map = event_id_map;

    return S_OK;
}

static HRESULT uia_event_handlers_add_handler(IUnknown *handler_iface, SAFEARRAY *runtime_id, int event_id,
        struct uia_com_event *event)
{
    struct uia_event_handler_identifier event_ident = { handler_iface, runtime_id, event_id };
    struct uia_event_handler_map_entry *event_map;
    struct rb_entry *rb_entry;
    HRESULT hr = S_OK;

    EnterCriticalSection(&com_event_handlers_cs);

    if (!com_event_handlers.handler_count)
    {
        rb_init(&com_event_handlers.handler_map, uia_com_event_handler_id_compare);
        rb_init(&com_event_handlers.handler_event_id_map, uia_com_event_handler_event_id_compare);
    }

    if ((rb_entry = rb_get(&com_event_handlers.handler_map, &event_ident)))
        event_map = RB_ENTRY_VALUE(rb_entry, struct uia_event_handler_map_entry, entry);
    else
    {
        if (!(event_map = calloc(1, sizeof(*event_map))))
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        hr = SafeArrayCopy(runtime_id, &event_map->runtime_id);
        if (FAILED(hr))
        {
            free(event_map);
            goto exit;
        }

        event_map->event_id = event_id;
        hr = uia_event_handlers_add_handler_to_event_id_map(event_map);
        if (FAILED(hr))
        {
            SafeArrayDestroy(event_map->runtime_id);
            free(event_map);
            goto exit;
        }

        event_map->handler_iface = handler_iface;
        IUnknown_AddRef(event_map->handler_iface);

        list_init(&event_map->handlers_list);
        rb_put(&com_event_handlers.handler_map, &event_ident, &event_map->entry);
    }

    list_add_tail(&event_map->handlers_list, &event->event_handler_map_list_entry);
    event->handler_map = event_map;
    com_event_handlers.handler_count++;
    if (event_id == UIA_AutomationFocusChangedEventId)
    {
        GUITHREADINFO info = { sizeof(info) };

        if (GetGUIThreadInfo(0, &info) && info.hwndFocus)
        {
            HUIANODE node = NULL;

            if (SUCCEEDED(create_uia_node_from_hwnd(info.hwndFocus, &node, NODE_FLAG_IGNORE_CLIENTSIDE_HWND_PROVS)))
                uia_com_focus_handler_advise_node(event, node, info.hwndFocus);
            UiaNodeRelease(node);
        }
    }

exit:
    LeaveCriticalSection(&com_event_handlers_cs);

    return hr;
}

static void uia_event_handler_destroy(struct uia_com_event *event)
{
    list_remove(&event->event_handler_map_list_entry);
    uia_hwnd_map_destroy(&event->focus_hwnd_map);
    if (event->event)
        UiaRemoveEvent(event->event);
    if (event->git_cookie)
        unregister_interface_in_git(event->git_cookie);
    free(event);
}

static void uia_event_handler_map_entry_destroy(struct uia_event_handler_map_entry *entry)
{
    struct uia_com_event *event, *event2;

    LIST_FOR_EACH_ENTRY_SAFE(event, event2, &entry->handlers_list, struct uia_com_event, event_handler_map_list_entry)
    {
        uia_event_handler_destroy(event);
        com_event_handlers.handler_count--;
    }

    list_remove(&entry->handler_event_id_map_list_entry);
    if (list_empty(&entry->handler_event_id_map->handlers_list))
    {
        rb_remove(&com_event_handlers.handler_event_id_map, &entry->handler_event_id_map->entry);
        free(entry->handler_event_id_map);
    }

    rb_remove(&com_event_handlers.handler_map, &entry->entry);
    IUnknown_Release(entry->handler_iface);
    SafeArrayDestroy(entry->runtime_id);
    free(entry);
}

static void uia_event_handlers_remove_handlers(IUnknown *handler_iface, SAFEARRAY *runtime_id, int event_id)
{
    struct uia_event_handler_identifier event_ident = { handler_iface, runtime_id, event_id };
    struct rb_entry *rb_entry;

    EnterCriticalSection(&com_event_handlers_cs);

    if (com_event_handlers.handler_count && (rb_entry = rb_get(&com_event_handlers.handler_map, &event_ident)))
        uia_event_handler_map_entry_destroy(RB_ENTRY_VALUE(rb_entry, struct uia_event_handler_map_entry, entry));

    LeaveCriticalSection(&com_event_handlers_cs);
}

static HRESULT create_uia_element_from_cache_req(IUIAutomationElement **iface, BOOL from_cui8,
        struct UiaCacheRequest *cache_req, LONG start_idx, SAFEARRAY *req_data, BSTR tree_struct);
static HRESULT uia_com_event_callback(struct uia_event *event, struct uia_event_args *args,
        SAFEARRAY *cache_req, BSTR tree_struct)
{
    struct uia_com_event *com_event = (struct uia_com_event *)event->u.clientside.callback_data;
    IUIAutomationElement *elem;
    BSTR tree_struct2;
    HRESULT hr;

    /* Nothing matches the cache request view condition, do nothing. */
    if (!cache_req)
        return S_OK;

    /* create_uia_element_from_cache_req frees the passed in BSTR. */
    tree_struct2 = SysAllocString(tree_struct);
    hr = create_uia_element_from_cache_req(&elem, com_event->from_cui8, &event->u.clientside.cache_req, 0, cache_req,
            tree_struct2);
    if (FAILED(hr))
        return hr;

    switch (event->event_id)
    {
    case UIA_AutomationFocusChangedEventId:
    {
        IUIAutomationFocusChangedEventHandler *handler;

        hr = get_interface_in_git(&IID_IUIAutomationFocusChangedEventHandler, com_event->git_cookie, (IUnknown **)&handler);
        if (SUCCEEDED(hr))
        {
            hr = IUIAutomationFocusChangedEventHandler_HandleFocusChangedEvent(handler, elem);
            IUIAutomationFocusChangedEventHandler_Release(handler);
        }
        break;
    }

    default:
    {
        IUIAutomationEventHandler *handler;

        hr = get_interface_in_git(&IID_IUIAutomationEventHandler, com_event->git_cookie, (IUnknown **)&handler);
        if (SUCCEEDED(hr))
        {
            hr = IUIAutomationEventHandler_HandleAutomationEvent(handler, elem, event->event_id);
            IUIAutomationEventHandler_Release(handler);
        }
        break;
    }
    }
    IUIAutomationElement_Release(elem);

    return hr;
}

/*
 * IUIAutomationElementArray interface.
 */
struct uia_element_array {
    IUIAutomationElementArray IUIAutomationElementArray_iface;
    LONG ref;

    IUIAutomationElement **elements;
    int elements_count;
};

static inline struct uia_element_array *impl_from_IUIAutomationElementArray(IUIAutomationElementArray *iface)
{
    return CONTAINING_RECORD(iface, struct uia_element_array, IUIAutomationElementArray_iface);
}

static HRESULT WINAPI uia_element_array_QueryInterface(IUIAutomationElementArray *iface, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IUIAutomationElementArray))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IUIAutomationElementArray_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI uia_element_array_AddRef(IUIAutomationElementArray *iface)
{
    struct uia_element_array *element = impl_from_IUIAutomationElementArray(iface);
    ULONG ref = InterlockedIncrement(&element->ref);

    TRACE("%p, refcount %ld\n", element, ref);
    return ref;
}

static ULONG WINAPI uia_element_array_Release(IUIAutomationElementArray *iface)
{
    struct uia_element_array *element_arr = impl_from_IUIAutomationElementArray(iface);
    ULONG ref = InterlockedDecrement(&element_arr->ref);

    TRACE("%p, refcount %ld\n", element_arr, ref);
    if (!ref)
    {
        if (element_arr->elements_count)
        {
            int i;

            for (i = 0; i < element_arr->elements_count; i++)
            {
                if (element_arr->elements[i])
                    IUIAutomationElement_Release(element_arr->elements[i]);
            }
        }
        free(element_arr->elements);
        free(element_arr);
    }

    return ref;
}

static HRESULT WINAPI uia_element_array_get_Length(IUIAutomationElementArray *iface, int *out_length)
{
    struct uia_element_array *element_arr = impl_from_IUIAutomationElementArray(iface);

    TRACE("%p, %p\n", iface, out_length);

    if (!out_length)
        return E_POINTER;

    *out_length = element_arr->elements_count;

    return S_OK;
}

static HRESULT WINAPI uia_element_array_GetElement(IUIAutomationElementArray *iface, int idx,
        IUIAutomationElement **out_elem)
{
    struct uia_element_array *element_arr = impl_from_IUIAutomationElementArray(iface);

    TRACE("%p, %p\n", iface, out_elem);

    if (!out_elem)
        return E_POINTER;

    if ((idx < 0) || (idx >= element_arr->elements_count))
        return E_INVALIDARG;

    *out_elem = element_arr->elements[idx];
    IUIAutomationElement_AddRef(element_arr->elements[idx]);

    return S_OK;
}

static const IUIAutomationElementArrayVtbl uia_element_array_vtbl = {
    uia_element_array_QueryInterface,
    uia_element_array_AddRef,
    uia_element_array_Release,
    uia_element_array_get_Length,
    uia_element_array_GetElement,
};

static HRESULT create_uia_element_array_iface(IUIAutomationElementArray **iface, int elements_count)
{
    struct uia_element_array *element_arr = calloc(1, sizeof(*element_arr));

    *iface = NULL;
    if (!element_arr)
        return E_OUTOFMEMORY;

    element_arr->IUIAutomationElementArray_iface.lpVtbl = &uia_element_array_vtbl;
    element_arr->ref = 1;
    element_arr->elements_count = elements_count;
    if (!(element_arr->elements = calloc(elements_count, sizeof(*element_arr->elements))))
    {
        free(element_arr);
        return E_OUTOFMEMORY;
    }

    *iface = &element_arr->IUIAutomationElementArray_iface;
    return S_OK;
}

struct uia_cache_property {
    int prop_id;
    VARIANT prop_val;
};

/*
 * IUIAutomationElement interface.
 */
struct uia_element {
    IUIAutomationElement9 IUIAutomationElement9_iface;
    LONG ref;

    BOOL from_cui8;
    HUIANODE node;

    struct uia_cache_property *cached_props;
    int cached_props_count;

    IUnknown *marshal;
};

static inline struct uia_element *impl_from_IUIAutomationElement9(IUIAutomationElement9 *iface)
{
    return CONTAINING_RECORD(iface, struct uia_element, IUIAutomationElement9_iface);
}

static HRESULT WINAPI uia_element_QueryInterface(IUIAutomationElement9 *iface, REFIID riid, void **ppv)
{
    struct uia_element *element = impl_from_IUIAutomationElement9(iface);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IUIAutomationElement) || (element->from_cui8 &&
            (IsEqualIID(riid, &IID_IUIAutomationElement2) || IsEqualIID(riid, &IID_IUIAutomationElement3) ||
            IsEqualIID(riid, &IID_IUIAutomationElement4) || IsEqualIID(riid, &IID_IUIAutomationElement5) ||
            IsEqualIID(riid, &IID_IUIAutomationElement6) || IsEqualIID(riid, &IID_IUIAutomationElement7) ||
            IsEqualIID(riid, &IID_IUIAutomationElement8) || IsEqualIID(riid, &IID_IUIAutomationElement9))))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IMarshal))
        return IUnknown_QueryInterface(element->marshal, riid, ppv);
    else
        return E_NOINTERFACE;

    IUIAutomationElement9_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI uia_element_AddRef(IUIAutomationElement9 *iface)
{
    struct uia_element *element = impl_from_IUIAutomationElement9(iface);
    ULONG ref = InterlockedIncrement(&element->ref);

    TRACE("%p, refcount %ld\n", element, ref);
    return ref;
}

static ULONG WINAPI uia_element_Release(IUIAutomationElement9 *iface)
{
    struct uia_element *element = impl_from_IUIAutomationElement9(iface);
    ULONG ref = InterlockedDecrement(&element->ref);

    TRACE("%p, refcount %ld\n", element, ref);
    if (!ref)
    {
        if (element->cached_props_count)
        {
            int i;

            for (i = 0; i < element->cached_props_count; i++)
                VariantClear(&element->cached_props[i].prop_val);
        }

        IUnknown_Release(element->marshal);
        free(element->cached_props);
        UiaNodeRelease(element->node);
        free(element);
    }

    return ref;
}

static HRESULT WINAPI uia_element_SetFocus(IUIAutomationElement9 *iface)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_GetRuntimeId(IUIAutomationElement9 *iface, SAFEARRAY **runtime_id)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_FindFirst(IUIAutomationElement9 *iface, enum TreeScope scope,
        IUIAutomationCondition *condition, IUIAutomationElement **found)
{
    IUIAutomationCacheRequest *cache_req;
    HRESULT hr;

    TRACE("%p, %#x, %p, %p\n", iface, scope, condition, found);

    if (!found)
        return E_POINTER;

    *found = NULL;
    hr = create_uia_cache_request_iface(&cache_req);
    if (FAILED(hr))
        return hr;

    hr = IUIAutomationElement9_FindFirstBuildCache(iface, scope, condition, cache_req, found);
    IUIAutomationCacheRequest_Release(cache_req);

    return hr;
}

static HRESULT WINAPI uia_element_FindAll(IUIAutomationElement9 *iface, enum TreeScope scope,
        IUIAutomationCondition *condition, IUIAutomationElementArray **found)
{
    IUIAutomationCacheRequest *cache_req;
    HRESULT hr;

    TRACE("%p, %#x, %p, %p\n", iface, scope, condition, found);

    if (!found)
        return E_POINTER;

    *found = NULL;
    hr = create_uia_cache_request_iface(&cache_req);
    if (FAILED(hr))
        return hr;

    hr = IUIAutomationElement9_FindAllBuildCache(iface, scope, condition, cache_req, found);
    IUIAutomationCacheRequest_Release(cache_req);

    return hr;
}

static HRESULT set_find_params_struct(struct UiaFindParams *params, IUIAutomationCondition *cond, int scope,
        BOOL find_first)
{
    HRESULT hr;

    hr = get_uia_condition_struct_from_iface(cond, &params->pFindCondition);
    if (FAILED(hr))
        return hr;

    if (!scope || (scope & (~TreeScope_Subtree)))
        return E_INVALIDARG;

    params->FindFirst = find_first;
    if (scope & TreeScope_Element)
        params->ExcludeRoot = FALSE;
    else
        params->ExcludeRoot = TRUE;

    if (scope & TreeScope_Descendants)
        params->MaxDepth = -1;
    else if (scope & TreeScope_Children)
        params->MaxDepth = 1;
    else
        params->MaxDepth = 0;

    return S_OK;
}

static HRESULT WINAPI uia_element_FindFirstBuildCache(IUIAutomationElement9 *iface, enum TreeScope scope,
        IUIAutomationCondition *condition, IUIAutomationCacheRequest *cache_req, IUIAutomationElement **found)
{
    struct uia_element *element = impl_from_IUIAutomationElement9(iface);
    LONG lbound_offsets, lbound_tree_structs, elems_count, offset_idx;
    struct UiaFindParams find_params = { 0 };
    struct UiaCacheRequest *cache_req_struct;
    SAFEARRAY *sa, *tree_structs, *offsets;
    IUIAutomationElement *elem;
    BSTR tree_struct_str;
    HRESULT hr;

    TRACE("%p, %#x, %p, %p, %p\n", iface, scope, condition, cache_req, found);

    if (!found)
        return E_POINTER;

    *found = elem = NULL;
    hr = get_uia_cache_request_struct_from_iface(cache_req, &cache_req_struct);
    if (FAILED(hr))
        return hr;

    hr = set_find_params_struct(&find_params, condition, scope, TRUE);
    if (FAILED(hr))
        return hr;

    sa = offsets = tree_structs = NULL;
    hr = UiaFind(element->node, &find_params, cache_req_struct, &sa, &offsets, &tree_structs);
    if (FAILED(hr) || !sa)
        goto exit;

    hr = get_safearray_bounds(tree_structs, &lbound_tree_structs, &elems_count);
    if (FAILED(hr))
        goto exit;

    hr = SafeArrayGetElement(tree_structs, &lbound_tree_structs, &tree_struct_str);
    if (FAILED(hr))
        goto exit;

    hr = get_safearray_bounds(offsets, &lbound_offsets, &elems_count);
    if (FAILED(hr))
        goto exit;

    hr = SafeArrayGetElement(offsets, &lbound_offsets, &offset_idx);
    if (FAILED(hr))
        goto exit;

    hr = create_uia_element_from_cache_req(&elem, element->from_cui8, cache_req_struct, offset_idx, sa, tree_struct_str);
    if (SUCCEEDED(hr))
        *found = elem;

exit:
    SafeArrayDestroy(tree_structs);
    SafeArrayDestroy(offsets);
    SafeArrayDestroy(sa);

    return hr;
}

static HRESULT WINAPI uia_element_FindAllBuildCache(IUIAutomationElement9 *iface, enum TreeScope scope,
        IUIAutomationCondition *condition, IUIAutomationCacheRequest *cache_req, IUIAutomationElementArray **found)
{
    struct uia_element *element = impl_from_IUIAutomationElement9(iface);
    LONG lbound_offsets, lbound_tree_structs, elems_count;
    struct uia_element_array *element_array_data;
    struct UiaFindParams find_params = { 0 };
    struct UiaCacheRequest *cache_req_struct;
    SAFEARRAY *sa, *tree_structs, *offsets;
    IUIAutomationElementArray *array;
    HRESULT hr;
    int i;

    TRACE("%p, %#x, %p, %p, %p\n", iface, scope, condition, cache_req, found);

    if (!found)
        return E_POINTER;

    *found = array = NULL;
    hr = get_uia_cache_request_struct_from_iface(cache_req, &cache_req_struct);
    if (FAILED(hr))
        return hr;

    hr = set_find_params_struct(&find_params, condition, scope, FALSE);
    if (FAILED(hr))
        return hr;

    sa = offsets = tree_structs = NULL;
    hr = UiaFind(element->node, &find_params, cache_req_struct, &sa, &offsets, &tree_structs);
    if (FAILED(hr) || !sa)
        goto exit;

    hr = get_safearray_bounds(tree_structs, &lbound_tree_structs, &elems_count);
    if (FAILED(hr))
        goto exit;

    hr = get_safearray_bounds(offsets, &lbound_offsets, &elems_count);
    if (FAILED(hr))
        goto exit;

    hr = create_uia_element_array_iface(&array, elems_count);
    if (FAILED(hr))
        goto exit;

    element_array_data = impl_from_IUIAutomationElementArray(array);
    for (i = 0; i < elems_count; i++)
    {
        BSTR tree_struct_str;
        LONG offset_idx, idx;

        idx = lbound_offsets + i;
        hr = SafeArrayGetElement(offsets, &idx, &offset_idx);
        if (FAILED(hr))
            goto exit;

        idx = lbound_tree_structs + i;
        hr = SafeArrayGetElement(tree_structs, &idx, &tree_struct_str);
        if (FAILED(hr))
            goto exit;

        hr = create_uia_element_from_cache_req(&element_array_data->elements[i], element->from_cui8,
                cache_req_struct, offset_idx, sa, tree_struct_str);
        if (FAILED(hr))
            goto exit;
    }

    *found = array;

exit:
    if (FAILED(hr) && array)
        IUIAutomationElementArray_Release(array);

    SafeArrayDestroy(tree_structs);
    SafeArrayDestroy(offsets);
    SafeArrayDestroy(sa);

    return hr;
}

static HRESULT WINAPI uia_element_BuildUpdatedCache(IUIAutomationElement9 *iface, IUIAutomationCacheRequest *cache_req,
        IUIAutomationElement **updated_elem)
{
    struct uia_element *element = impl_from_IUIAutomationElement9(iface);
    struct UiaCacheRequest *cache_req_struct;
    IUIAutomationElement *cache_elem;
    BSTR tree_struct;
    SAFEARRAY *sa;
    HRESULT hr;

    TRACE("%p, %p, %p\n", iface, cache_req, updated_elem);

    if (!updated_elem)
        return E_POINTER;

    *updated_elem = NULL;
    hr = get_uia_cache_request_struct_from_iface(cache_req, &cache_req_struct);
    if (FAILED(hr))
        return hr;

    hr = UiaGetUpdatedCache(element->node, cache_req_struct, NormalizeState_None, NULL, &sa, &tree_struct);
    if (FAILED(hr))
        return hr;

    hr = create_uia_element_from_cache_req(&cache_elem, element->from_cui8, cache_req_struct, 0, sa, tree_struct);
    if (SUCCEEDED(hr))
        *updated_elem = cache_elem;

    SafeArrayDestroy(sa);
    return S_OK;
}

static HRESULT WINAPI uia_element_GetCurrentPropertyValue(IUIAutomationElement9 *iface, PROPERTYID prop_id,
        VARIANT *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT create_uia_element(IUIAutomationElement **iface, BOOL from_cui8, HUIANODE node);
static HRESULT get_element_variant_from_node_variant(VARIANT *var, BOOL from_cui8, int prop_type)
{
    HUIANODE node;
    HRESULT hr;

    /* ReservedNotSupported interface, just return it. */
    if (V_VT(var) == VT_UNKNOWN)
        return S_OK;

    if (prop_type & UIAutomationType_Array)
    {
        struct uia_element_array *elem_arr_data;
        IUIAutomationElementArray *elem_arr;
        LONG idx, lbound, elems, i;

        hr = get_safearray_bounds(V_ARRAY(var), &lbound, &elems);
        if (FAILED(hr))
        {
            VariantClear(var);
            return hr;
        }

        hr = create_uia_element_array_iface(&elem_arr, elems);
        if (FAILED(hr))
        {
            VariantClear(var);
            return hr;
        }

        elem_arr_data = impl_from_IUIAutomationElementArray(elem_arr);
        for (i = 0; i < elems; i++)
        {
            idx = lbound + i;
            hr = SafeArrayGetElement(V_ARRAY(var), &idx, &node);
            if (FAILED(hr))
                break;

            hr = create_uia_element(&elem_arr_data->elements[i], from_cui8, node);
            if (FAILED(hr))
            {
                UiaNodeRelease(node);
                break;
            }
        }

        VariantClear(var);
        if (SUCCEEDED(hr))
        {
            V_VT(var) = VT_UNKNOWN;
            V_UNKNOWN(var) = (IUnknown *)elem_arr;
        }
        else
            IUIAutomationElementArray_Release(elem_arr);
    }
    else
    {
        IUIAutomationElement *out_elem;

        hr = UiaHUiaNodeFromVariant(var, &node);
        VariantClear(var);
        if (FAILED(hr))
            return hr;

        hr = create_uia_element(&out_elem, from_cui8, node);
        if (SUCCEEDED(hr))
        {
            V_VT(var) = VT_UNKNOWN;
            V_UNKNOWN(var) = (IUnknown *)out_elem;
        }
        else
            UiaNodeRelease(node);
    }

    return hr;
}

static HRESULT WINAPI uia_element_GetCurrentPropertyValueEx(IUIAutomationElement9 *iface, PROPERTYID prop_id,
        BOOL ignore_default, VARIANT *ret_val)
{
    const struct uia_prop_info *prop_info = uia_prop_info_from_id(prop_id);
    struct uia_element *element = impl_from_IUIAutomationElement9(iface);
    HRESULT hr;

    TRACE("%p, %d, %d, %p\n", iface, prop_id, ignore_default, ret_val);

    if (!ret_val)
        return E_POINTER;

    VariantInit(ret_val);
    if (!prop_info)
        return E_INVALIDARG;

    if (!ignore_default)
        FIXME("Default values currently unimplemented\n");

    hr = UiaGetPropertyValue(element->node, prop_id, ret_val);
    if (FAILED(hr))
        return hr;

    if ((prop_info->type == UIAutomationType_Element) || (prop_info->type == UIAutomationType_ElementArray))
        hr = get_element_variant_from_node_variant(ret_val, element->from_cui8, prop_info->type);

    return hr;
}

static HRESULT WINAPI uia_element_GetCachedPropertyValue(IUIAutomationElement9 *iface, PROPERTYID prop_id,
        VARIANT *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static int __cdecl uia_cached_property_id_compare(const void *a, const void *b)
{
    const PROPERTYID *prop_id = a;
    const struct uia_cache_property *cache_prop = b;

    return ((*prop_id) > cache_prop->prop_id) - ((*prop_id) < cache_prop->prop_id);
}

static HRESULT WINAPI uia_element_GetCachedPropertyValueEx(IUIAutomationElement9 *iface, PROPERTYID prop_id,
        BOOL ignore_default, VARIANT *ret_val)
{
    struct uia_element *element = impl_from_IUIAutomationElement9(iface);
    struct uia_cache_property *cache_prop = NULL;

    TRACE("%p, %d, %d, %p\n", iface, prop_id, ignore_default, ret_val);

    if (!ret_val)
        return E_POINTER;

    VariantInit(ret_val);
    if (!uia_prop_info_from_id(prop_id) || !element->cached_props)
        return E_INVALIDARG;

    if (!ignore_default)
        FIXME("Default values currently unimplemented\n");

    if (!(cache_prop = bsearch(&prop_id, element->cached_props, element->cached_props_count, sizeof(*cache_prop),
            uia_cached_property_id_compare)))
        return E_INVALIDARG;

    VariantCopy(ret_val, &cache_prop->prop_val);
    return S_OK;
}

static HRESULT WINAPI uia_element_GetCurrentPatternAs(IUIAutomationElement9 *iface, PATTERNID pattern_id,
        REFIID riid, void **out_pattern)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_GetCachedPatternAs(IUIAutomationElement9 *iface, PATTERNID pattern_id,
        REFIID riid, void **out_pattern)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_GetCurrentPattern(IUIAutomationElement9 *iface, PATTERNID pattern_id,
        IUnknown **out_pattern)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_GetCachedPattern(IUIAutomationElement9 *iface, PATTERNID pattern_id,
        IUnknown **patternObject)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_GetCachedParent(IUIAutomationElement9 *iface, IUIAutomationElement **parent)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_GetCachedChildren(IUIAutomationElement9 *iface,
        IUIAutomationElementArray **children)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentProcessId(IUIAutomationElement9 *iface, int *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static void uia_elem_get_control_type(VARIANT *v, CONTROLTYPEID *ret_val)
{
    const struct uia_control_type_info *info = NULL;

    *ret_val = UIA_CustomControlTypeId;
    if (V_VT(v) != VT_I4)
        return;

    if ((info = uia_control_type_info_from_id(V_I4(v))))
        *ret_val = info->control_type_id;
    else
        WARN("Provider returned invalid control type ID %ld\n", V_I4(v));
}

static HRESULT WINAPI uia_element_get_CurrentControlType(IUIAutomationElement9 *iface, CONTROLTYPEID *ret_val)
{
    struct uia_element *element = impl_from_IUIAutomationElement9(iface);
    HRESULT hr;
    VARIANT v;

    TRACE("%p, %p\n", iface, ret_val);

    VariantInit(&v);
    hr = UiaGetPropertyValue(element->node, UIA_ControlTypePropertyId, &v);
    uia_elem_get_control_type(&v, ret_val);
    VariantClear(&v);

    return hr;
}

static HRESULT WINAPI uia_element_get_CurrentLocalizedControlType(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentName(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    struct uia_element *element = impl_from_IUIAutomationElement9(iface);
    HRESULT hr;
    VARIANT v;

    TRACE("%p, %p\n", iface, ret_val);

    VariantInit(&v);
    hr = UiaGetPropertyValue(element->node, UIA_NamePropertyId, &v);
    if (SUCCEEDED(hr) && V_VT(&v) == VT_BSTR && V_BSTR(&v))
        *ret_val = SysAllocString(V_BSTR(&v));
    else
        *ret_val = SysAllocString(L"");

    VariantClear(&v);
    return hr;
}

static HRESULT WINAPI uia_element_get_CurrentAcceleratorKey(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentAccessKey(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentHasKeyboardFocus(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentIsKeyboardFocusable(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentIsEnabled(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentAutomationId(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentClassName(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentHelpText(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentCulture(IUIAutomationElement9 *iface, int *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentIsControlElement(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentIsContentElement(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentIsPassword(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentNativeWindowHandle(IUIAutomationElement9 *iface, UIA_HWND *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentItemType(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentIsOffscreen(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentOrientation(IUIAutomationElement9 *iface, enum OrientationType *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentFrameworkId(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentIsRequiredForForm(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentItemStatus(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static void uia_variant_rect_to_rect(VARIANT *v, RECT *ret_val)
{
    double *vals;
    HRESULT hr;

    memset(ret_val, 0, sizeof(*ret_val));
    if (V_VT(v) != (VT_R8 | VT_ARRAY))
        return;

    hr = SafeArrayAccessData(V_ARRAY(v), (void **)&vals);
    if (FAILED(hr))
    {
        WARN("SafeArrayAccessData failed with hr %#lx\n", hr);
        return;
    }

    ret_val->left = vals[0];
    ret_val->top = vals[1];
    ret_val->right = ret_val->left + vals[2];
    ret_val->bottom = ret_val->top + vals[3];

    hr = SafeArrayUnaccessData(V_ARRAY(v));
    if (FAILED(hr))
        WARN("SafeArrayUnaccessData failed with hr %#lx\n", hr);
}

static HRESULT WINAPI uia_element_get_CurrentBoundingRectangle(IUIAutomationElement9 *iface, RECT *ret_val)
{
    struct uia_element *element = impl_from_IUIAutomationElement9(iface);
    HRESULT hr;
    VARIANT v;

    TRACE("%p, %p\n", element, ret_val);

    VariantInit(&v);
    hr = UiaGetPropertyValue(element->node, UIA_BoundingRectanglePropertyId, &v);
    uia_variant_rect_to_rect(&v, ret_val);

    VariantClear(&v);
    return hr;
}

static HRESULT WINAPI uia_element_get_CurrentLabeledBy(IUIAutomationElement9 *iface, IUIAutomationElement **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentAriaRole(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentAriaProperties(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentIsDataValidForForm(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentControllerFor(IUIAutomationElement9 *iface,
        IUIAutomationElementArray **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentDescribedBy(IUIAutomationElement9 *iface,
        IUIAutomationElementArray **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentFlowsTo(IUIAutomationElement9 *iface, IUIAutomationElementArray **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentProviderDescription(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedProcessId(IUIAutomationElement9 *iface, int *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedControlType(IUIAutomationElement9 *iface, CONTROLTYPEID *ret_val)
{
    struct uia_element *element = impl_from_IUIAutomationElement9(iface);
    const int prop_id = UIA_ControlTypePropertyId;
    struct uia_cache_property *cache_prop = NULL;

    TRACE("%p, %p\n", iface, ret_val);

    if (!ret_val)
        return E_POINTER;

    if (!(cache_prop = bsearch(&prop_id, element->cached_props, element->cached_props_count, sizeof(*cache_prop),
            uia_cached_property_id_compare)))
        return E_INVALIDARG;

    uia_elem_get_control_type(&cache_prop->prop_val, ret_val);
    return S_OK;
}

static HRESULT WINAPI uia_element_get_CachedLocalizedControlType(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedName(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    struct uia_element *element = impl_from_IUIAutomationElement9(iface);
    struct uia_cache_property *cache_prop = NULL;
    const int prop_id = UIA_NamePropertyId;

    TRACE("%p, %p\n", iface, ret_val);

    if (!ret_val)
        return E_POINTER;

    if (!(cache_prop = bsearch(&prop_id, element->cached_props, element->cached_props_count, sizeof(*cache_prop),
            uia_cached_property_id_compare)))
        return E_INVALIDARG;

    if ((V_VT(&cache_prop->prop_val) == VT_BSTR) && V_BSTR(&cache_prop->prop_val))
        *ret_val = SysAllocString(V_BSTR(&cache_prop->prop_val));
    else
        *ret_val = SysAllocString(L"");

    return S_OK;
}

static HRESULT WINAPI uia_element_get_CachedAcceleratorKey(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedAccessKey(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedHasKeyboardFocus(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    struct uia_element *element = impl_from_IUIAutomationElement9(iface);
    const int prop_id = UIA_HasKeyboardFocusPropertyId;
    struct uia_cache_property *cache_prop = NULL;

    TRACE("%p, %p\n", iface, ret_val);

    if (!ret_val)
        return E_POINTER;

    if (!(cache_prop = bsearch(&prop_id, element->cached_props, element->cached_props_count, sizeof(*cache_prop),
            uia_cached_property_id_compare)))
        return E_INVALIDARG;

    *ret_val = ((V_VT(&cache_prop->prop_val) == VT_BOOL) && (V_BOOL(&cache_prop->prop_val) == VARIANT_TRUE));
    return S_OK;
}

static HRESULT WINAPI uia_element_get_CachedIsKeyboardFocusable(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    struct uia_element *element = impl_from_IUIAutomationElement9(iface);
    const int prop_id = UIA_IsKeyboardFocusablePropertyId;
    struct uia_cache_property *cache_prop = NULL;

    TRACE("%p, %p\n", iface, ret_val);

    if (!ret_val)
        return E_POINTER;

    if (!(cache_prop = bsearch(&prop_id, element->cached_props, element->cached_props_count, sizeof(*cache_prop),
            uia_cached_property_id_compare)))
        return E_INVALIDARG;

    *ret_val = ((V_VT(&cache_prop->prop_val) == VT_BOOL) && (V_BOOL(&cache_prop->prop_val) == VARIANT_TRUE));
    return S_OK;
}

static HRESULT WINAPI uia_element_get_CachedIsEnabled(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedAutomationId(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedClassName(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedHelpText(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedCulture(IUIAutomationElement9 *iface, int *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedIsControlElement(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedIsContentElement(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedIsPassword(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedNativeWindowHandle(IUIAutomationElement9 *iface, UIA_HWND *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedItemType(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedIsOffscreen(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedOrientation(IUIAutomationElement9 *iface,
        enum OrientationType *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedFrameworkId(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedIsRequiredForForm(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedItemStatus(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedBoundingRectangle(IUIAutomationElement9 *iface, RECT *ret_val)
{
    struct uia_element *element = impl_from_IUIAutomationElement9(iface);
    const int prop_id = UIA_BoundingRectanglePropertyId;
    struct uia_cache_property *cache_prop = NULL;

    TRACE("%p, %p\n", iface, ret_val);

    if (!ret_val)
        return E_POINTER;

    if (!(cache_prop = bsearch(&prop_id, element->cached_props, element->cached_props_count, sizeof(*cache_prop),
            uia_cached_property_id_compare)))
        return E_INVALIDARG;

    uia_variant_rect_to_rect(&cache_prop->prop_val, ret_val);
    return S_OK;
}

static HRESULT WINAPI uia_element_get_CachedLabeledBy(IUIAutomationElement9 *iface, IUIAutomationElement **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedAriaRole(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedAriaProperties(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedIsDataValidForForm(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedControllerFor(IUIAutomationElement9 *iface,
        IUIAutomationElementArray **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedDescribedBy(IUIAutomationElement9 *iface,
        IUIAutomationElementArray **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedFlowsTo(IUIAutomationElement9 *iface, IUIAutomationElementArray **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedProviderDescription(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_GetClickablePoint(IUIAutomationElement9 *iface, POINT *clickable, BOOL *got_clickable)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentOptimizeForVisualContent(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedOptimizeForVisualContent(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentLiveSetting(IUIAutomationElement9 *iface, enum LiveSetting *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedLiveSetting(IUIAutomationElement9 *iface, enum LiveSetting *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentFlowsFrom(IUIAutomationElement9 *iface,
        IUIAutomationElementArray **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedFlowsFrom(IUIAutomationElement9 *iface, IUIAutomationElementArray **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_ShowContextMenu(IUIAutomationElement9 *iface)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentIsPeripheral(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedIsPeripheral(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentPositionInSet(IUIAutomationElement9 *iface, int *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentSizeOfSet(IUIAutomationElement9 *iface, int *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentLevel(IUIAutomationElement9 *iface, int *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentAnnotationTypes(IUIAutomationElement9 *iface, SAFEARRAY **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentAnnotationObjects(IUIAutomationElement9 *iface,
        IUIAutomationElementArray **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedPositionInSet(IUIAutomationElement9 *iface, int *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedSizeOfSet(IUIAutomationElement9 *iface, int *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedLevel(IUIAutomationElement9 *iface, int *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedAnnotationTypes(IUIAutomationElement9 *iface, SAFEARRAY **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedAnnotationObjects(IUIAutomationElement9 *iface,
        IUIAutomationElementArray **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentLandmarkType(IUIAutomationElement9 *iface, LANDMARKTYPEID *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentLocalizedLandmarkType(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedLandmarkType(IUIAutomationElement9 *iface, LANDMARKTYPEID *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedLocalizedLandmarkType(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentFullDescription(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedFullDescription(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_FindFirstWithOptions(IUIAutomationElement9 *iface, enum TreeScope scope,
        IUIAutomationCondition *condition, enum TreeTraversalOptions traversal_opts, IUIAutomationElement *root,
        IUIAutomationElement **found)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_FindAllWithOptions(IUIAutomationElement9 *iface, enum TreeScope scope,
        IUIAutomationCondition *condition, enum TreeTraversalOptions traversal_opts, IUIAutomationElement *root,
        IUIAutomationElementArray **found)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_FindFirstWithOptionsBuildCache(IUIAutomationElement9 *iface, enum TreeScope scope,
        IUIAutomationCondition *condition, IUIAutomationCacheRequest *cache_req,
        enum TreeTraversalOptions traversal_opts, IUIAutomationElement *root, IUIAutomationElement **found)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_FindAllWithOptionsBuildCache(IUIAutomationElement9 *iface, enum TreeScope scope,
        IUIAutomationCondition *condition, IUIAutomationCacheRequest *cache_req,
        enum TreeTraversalOptions traversal_opts, IUIAutomationElement *root, IUIAutomationElementArray **found)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_GetCurrentMetadataValue(IUIAutomationElement9 *iface, int target_id,
        METADATAID metadata_id, VARIANT *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentHeadingLevel(IUIAutomationElement9 *iface, HEADINGLEVELID *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedHeadingLevel(IUIAutomationElement9 *iface, HEADINGLEVELID *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentIsDialog(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedIsDialog(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static const IUIAutomationElement9Vtbl uia_element_vtbl = {
    uia_element_QueryInterface,
    uia_element_AddRef,
    uia_element_Release,
    uia_element_SetFocus,
    uia_element_GetRuntimeId,
    uia_element_FindFirst,
    uia_element_FindAll,
    uia_element_FindFirstBuildCache,
    uia_element_FindAllBuildCache,
    uia_element_BuildUpdatedCache,
    uia_element_GetCurrentPropertyValue,
    uia_element_GetCurrentPropertyValueEx,
    uia_element_GetCachedPropertyValue,
    uia_element_GetCachedPropertyValueEx,
    uia_element_GetCurrentPatternAs,
    uia_element_GetCachedPatternAs,
    uia_element_GetCurrentPattern,
    uia_element_GetCachedPattern,
    uia_element_GetCachedParent,
    uia_element_GetCachedChildren,
    uia_element_get_CurrentProcessId,
    uia_element_get_CurrentControlType,
    uia_element_get_CurrentLocalizedControlType,
    uia_element_get_CurrentName,
    uia_element_get_CurrentAcceleratorKey,
    uia_element_get_CurrentAccessKey,
    uia_element_get_CurrentHasKeyboardFocus,
    uia_element_get_CurrentIsKeyboardFocusable,
    uia_element_get_CurrentIsEnabled,
    uia_element_get_CurrentAutomationId,
    uia_element_get_CurrentClassName,
    uia_element_get_CurrentHelpText,
    uia_element_get_CurrentCulture,
    uia_element_get_CurrentIsControlElement,
    uia_element_get_CurrentIsContentElement,
    uia_element_get_CurrentIsPassword,
    uia_element_get_CurrentNativeWindowHandle,
    uia_element_get_CurrentItemType,
    uia_element_get_CurrentIsOffscreen,
    uia_element_get_CurrentOrientation,
    uia_element_get_CurrentFrameworkId,
    uia_element_get_CurrentIsRequiredForForm,
    uia_element_get_CurrentItemStatus,
    uia_element_get_CurrentBoundingRectangle,
    uia_element_get_CurrentLabeledBy,
    uia_element_get_CurrentAriaRole,
    uia_element_get_CurrentAriaProperties,
    uia_element_get_CurrentIsDataValidForForm,
    uia_element_get_CurrentControllerFor,
    uia_element_get_CurrentDescribedBy,
    uia_element_get_CurrentFlowsTo,
    uia_element_get_CurrentProviderDescription,
    uia_element_get_CachedProcessId,
    uia_element_get_CachedControlType,
    uia_element_get_CachedLocalizedControlType,
    uia_element_get_CachedName,
    uia_element_get_CachedAcceleratorKey,
    uia_element_get_CachedAccessKey,
    uia_element_get_CachedHasKeyboardFocus,
    uia_element_get_CachedIsKeyboardFocusable,
    uia_element_get_CachedIsEnabled,
    uia_element_get_CachedAutomationId,
    uia_element_get_CachedClassName,
    uia_element_get_CachedHelpText,
    uia_element_get_CachedCulture,
    uia_element_get_CachedIsControlElement,
    uia_element_get_CachedIsContentElement,
    uia_element_get_CachedIsPassword,
    uia_element_get_CachedNativeWindowHandle,
    uia_element_get_CachedItemType,
    uia_element_get_CachedIsOffscreen,
    uia_element_get_CachedOrientation,
    uia_element_get_CachedFrameworkId,
    uia_element_get_CachedIsRequiredForForm,
    uia_element_get_CachedItemStatus,
    uia_element_get_CachedBoundingRectangle,
    uia_element_get_CachedLabeledBy,
    uia_element_get_CachedAriaRole,
    uia_element_get_CachedAriaProperties,
    uia_element_get_CachedIsDataValidForForm,
    uia_element_get_CachedControllerFor,
    uia_element_get_CachedDescribedBy,
    uia_element_get_CachedFlowsTo,
    uia_element_get_CachedProviderDescription,
    uia_element_GetClickablePoint,
    uia_element_get_CurrentOptimizeForVisualContent,
    uia_element_get_CachedOptimizeForVisualContent,
    uia_element_get_CurrentLiveSetting,
    uia_element_get_CachedLiveSetting,
    uia_element_get_CurrentFlowsFrom,
    uia_element_get_CachedFlowsFrom,
    uia_element_ShowContextMenu,
    uia_element_get_CurrentIsPeripheral,
    uia_element_get_CachedIsPeripheral,
    uia_element_get_CurrentPositionInSet,
    uia_element_get_CurrentSizeOfSet,
    uia_element_get_CurrentLevel,
    uia_element_get_CurrentAnnotationTypes,
    uia_element_get_CurrentAnnotationObjects,
    uia_element_get_CachedPositionInSet,
    uia_element_get_CachedSizeOfSet,
    uia_element_get_CachedLevel,
    uia_element_get_CachedAnnotationTypes,
    uia_element_get_CachedAnnotationObjects,
    uia_element_get_CurrentLandmarkType,
    uia_element_get_CurrentLocalizedLandmarkType,
    uia_element_get_CachedLandmarkType,
    uia_element_get_CachedLocalizedLandmarkType,
    uia_element_get_CurrentFullDescription,
    uia_element_get_CachedFullDescription,
    uia_element_FindFirstWithOptions,
    uia_element_FindAllWithOptions,
    uia_element_FindFirstWithOptionsBuildCache,
    uia_element_FindAllWithOptionsBuildCache,
    uia_element_GetCurrentMetadataValue,
    uia_element_get_CurrentHeadingLevel,
    uia_element_get_CachedHeadingLevel,
    uia_element_get_CurrentIsDialog,
    uia_element_get_CachedIsDialog,
};

static HRESULT create_uia_element(IUIAutomationElement **iface, BOOL from_cui8, HUIANODE node)
{
    struct uia_element *element = calloc(1, sizeof(*element));
    HRESULT hr;

    *iface = NULL;
    if (!element)
        return E_OUTOFMEMORY;

    element->IUIAutomationElement9_iface.lpVtbl = &uia_element_vtbl;
    element->ref = 1;
    element->from_cui8 = from_cui8;
    element->node = node;

    hr = CoCreateFreeThreadedMarshaler((IUnknown *)&element->IUIAutomationElement9_iface, &element->marshal);
    if (FAILED(hr))
    {
        free(element);
        return hr;
    }

    *iface = (IUIAutomationElement *)&element->IUIAutomationElement9_iface;
    return S_OK;
}

static int __cdecl uia_compare_cache_props(const void *a, const void *b)
{
    struct uia_cache_property *prop1 = (struct uia_cache_property *)a;
    struct uia_cache_property *prop2 = (struct uia_cache_property *)b;

    return (prop1->prop_id > prop2->prop_id) - (prop1->prop_id < prop2->prop_id);
}

static HRESULT create_uia_element_from_cache_req(IUIAutomationElement **iface, BOOL from_cui8,
        struct UiaCacheRequest *cache_req, LONG start_idx, SAFEARRAY *req_data, BSTR tree_struct)
{
    IUIAutomationElement *element = NULL;
    struct uia_element *elem_data;
    HUIANODE node;
    LONG idx[2];
    HRESULT hr;
    VARIANT v;

    *iface = NULL;

    VariantInit(&v);
    idx[0] = start_idx;
    idx[1] = 0;
    hr = SafeArrayGetElement(req_data, idx, &v);
    if (FAILED(hr))
        goto exit;

    hr = UiaHUiaNodeFromVariant(&v, &node);
    if (FAILED(hr))
        goto exit;
    VariantClear(&v);

    hr = create_uia_element(&element, from_cui8, node);
    if (FAILED(hr))
        goto exit;

    elem_data = impl_from_IUIAutomationElement9((IUIAutomationElement9 *)element);
    if (cache_req->cProperties)
    {
        LONG i;

        elem_data->cached_props = calloc(cache_req->cProperties, sizeof(*elem_data->cached_props));
        if (!elem_data->cached_props)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        elem_data->cached_props_count = cache_req->cProperties;
        for (i = 0; i < cache_req->cProperties; i++)
        {
            const struct uia_prop_info *prop_info = uia_prop_info_from_id(cache_req->pProperties[i]);

            elem_data->cached_props[i].prop_id = prop_info->prop_id;

            idx[0] = start_idx;
            idx[1] = 1 + i;
            hr = SafeArrayGetElement(req_data, idx, &elem_data->cached_props[i].prop_val);
            if (FAILED(hr))
                goto exit;

            if ((prop_info->type == UIAutomationType_Element) || (prop_info->type == UIAutomationType_ElementArray))
            {
                hr = get_element_variant_from_node_variant(&elem_data->cached_props[i].prop_val, from_cui8,
                        prop_info->type);
                if (FAILED(hr))
                    goto exit;
            }
        }

        /*
         * Sort the array of cached properties by property ID so that we can
         * access the values with bsearch.
         */
        qsort(elem_data->cached_props, elem_data->cached_props_count, sizeof(*elem_data->cached_props),
                uia_compare_cache_props);
    }

    *iface = element;

exit:
    if (FAILED(hr))
    {
        WARN("Failed to create element from cache request, hr %#lx\n", hr);
        if (element)
            IUIAutomationElement_Release(element);
    }

    SysFreeString(tree_struct);
    return hr;
}

/*
 * IUIAutomationTreeWalker interface.
 */
struct uia_tree_walker {
    IUIAutomationTreeWalker IUIAutomationTreeWalker_iface;
    LONG ref;

    IUIAutomationCacheRequest *default_cache_req;
    IUIAutomationCondition *nav_cond;
    struct UiaCondition *cond_struct;
};

static inline struct uia_tree_walker *impl_from_IUIAutomationTreeWalker(IUIAutomationTreeWalker *iface)
{
    return CONTAINING_RECORD(iface, struct uia_tree_walker, IUIAutomationTreeWalker_iface);
}

static HRESULT WINAPI uia_tree_walker_QueryInterface(IUIAutomationTreeWalker *iface, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IUIAutomationTreeWalker) || IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IUIAutomationTreeWalker_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI uia_tree_walker_AddRef(IUIAutomationTreeWalker *iface)
{
    struct uia_tree_walker *tree_walker = impl_from_IUIAutomationTreeWalker(iface);
    ULONG ref = InterlockedIncrement(&tree_walker->ref);

    TRACE("%p, refcount %ld\n", tree_walker, ref);
    return ref;
}

static ULONG WINAPI uia_tree_walker_Release(IUIAutomationTreeWalker *iface)
{
    struct uia_tree_walker *tree_walker = impl_from_IUIAutomationTreeWalker(iface);
    ULONG ref = InterlockedDecrement(&tree_walker->ref);

    TRACE("%p, refcount %ld\n", tree_walker, ref);
    if (!ref)
    {
        if (tree_walker->default_cache_req)
            IUIAutomationCacheRequest_Release(tree_walker->default_cache_req);
        IUIAutomationCondition_Release(tree_walker->nav_cond);
        free(tree_walker);
    }

    return ref;
}

static HRESULT WINAPI uia_tree_walker_GetParentElement(IUIAutomationTreeWalker *iface, IUIAutomationElement *elem,
        IUIAutomationElement **parent)
{
    struct uia_tree_walker *tree_walker = impl_from_IUIAutomationTreeWalker(iface);

    TRACE("%p, %p, %p\n", iface, elem, parent);

    return IUIAutomationTreeWalker_GetParentElementBuildCache(iface, elem, tree_walker->default_cache_req, parent);
}

static HRESULT WINAPI uia_tree_walker_GetFirstChildElement(IUIAutomationTreeWalker *iface, IUIAutomationElement *elem,
        IUIAutomationElement **first)
{
    struct uia_tree_walker *tree_walker = impl_from_IUIAutomationTreeWalker(iface);

    TRACE("%p, %p, %p\n", iface, elem, first);

    return IUIAutomationTreeWalker_GetFirstChildElementBuildCache(iface, elem, tree_walker->default_cache_req, first);
}

static HRESULT WINAPI uia_tree_walker_GetLastChildElement(IUIAutomationTreeWalker *iface, IUIAutomationElement *elem,
        IUIAutomationElement **last)
{
    struct uia_tree_walker *tree_walker = impl_from_IUIAutomationTreeWalker(iface);

    TRACE("%p, %p, %p\n", iface, elem, last);

    return IUIAutomationTreeWalker_GetLastChildElementBuildCache(iface, elem, tree_walker->default_cache_req, last);
}

static HRESULT WINAPI uia_tree_walker_GetNextSiblingElement(IUIAutomationTreeWalker *iface, IUIAutomationElement *elem,
        IUIAutomationElement **next)
{
    struct uia_tree_walker *tree_walker = impl_from_IUIAutomationTreeWalker(iface);

    TRACE("%p, %p, %p\n", iface, elem, next);

    return IUIAutomationTreeWalker_GetNextSiblingElementBuildCache(iface, elem, tree_walker->default_cache_req, next);
}

static HRESULT WINAPI uia_tree_walker_GetPreviousSiblingElement(IUIAutomationTreeWalker *iface,
        IUIAutomationElement *elem, IUIAutomationElement **prev)
{
    struct uia_tree_walker *tree_walker = impl_from_IUIAutomationTreeWalker(iface);

    TRACE("%p, %p, %p\n", iface, elem, prev);

    return IUIAutomationTreeWalker_GetPreviousSiblingElementBuildCache(iface, elem, tree_walker->default_cache_req, prev);
}

static HRESULT WINAPI uia_tree_walker_NormalizeElement(IUIAutomationTreeWalker *iface, IUIAutomationElement *elem,
        IUIAutomationElement **normalized)
{
    FIXME("%p, %p, %p: stub\n", iface, elem, normalized);
    return E_NOTIMPL;
}

static HRESULT uia_tree_walker_navigate(IUIAutomationTreeWalker *walker, IUIAutomationCacheRequest *cache_req,
        IUIAutomationElement *start_elem, int nav_dir, IUIAutomationElement **out_elem)
{
    struct uia_tree_walker *tree_walker = impl_from_IUIAutomationTreeWalker(walker);
    struct UiaCacheRequest *cache_req_struct;
    struct uia_element *element;
    BSTR tree_struct = NULL;
    SAFEARRAY *sa = NULL;
    HRESULT hr;

    if (!out_elem)
        return E_POINTER;

    *out_elem = NULL;
    if (!start_elem)
        return E_POINTER;

    hr = get_uia_cache_request_struct_from_iface(cache_req, &cache_req_struct);
    if (FAILED(hr))
        return hr;

    element = impl_from_IUIAutomationElement9((IUIAutomationElement9 *)start_elem);
    hr = UiaNavigate(element->node, nav_dir, tree_walker->cond_struct, cache_req_struct, &sa, &tree_struct);
    if (SUCCEEDED(hr) && sa)
    {
        hr = create_uia_element_from_cache_req(out_elem, element->from_cui8, cache_req_struct, 0, sa, tree_struct);
        tree_struct = NULL;
    }

    SysFreeString(tree_struct);
    SafeArrayDestroy(sa);
    return hr;
}

static HRESULT WINAPI uia_tree_walker_GetParentElementBuildCache(IUIAutomationTreeWalker *iface,
        IUIAutomationElement *elem, IUIAutomationCacheRequest *cache_req, IUIAutomationElement **parent)
{
    TRACE("%p, %p, %p, %p\n", iface, elem, cache_req, parent);

    return uia_tree_walker_navigate(iface, cache_req, elem, NavigateDirection_Parent, parent);
}

static HRESULT WINAPI uia_tree_walker_GetFirstChildElementBuildCache(IUIAutomationTreeWalker *iface,
        IUIAutomationElement *elem, IUIAutomationCacheRequest *cache_req, IUIAutomationElement **first)
{
    TRACE("%p, %p, %p, %p\n", iface, elem, cache_req, first);

    return uia_tree_walker_navigate(iface, cache_req, elem, NavigateDirection_FirstChild, first);
}

static HRESULT WINAPI uia_tree_walker_GetLastChildElementBuildCache(IUIAutomationTreeWalker *iface,
        IUIAutomationElement *elem, IUIAutomationCacheRequest *cache_req, IUIAutomationElement **last)
{
    TRACE("%p, %p, %p, %p\n", iface, elem, cache_req, last);

    return uia_tree_walker_navigate(iface, cache_req, elem, NavigateDirection_LastChild, last);
}

static HRESULT WINAPI uia_tree_walker_GetNextSiblingElementBuildCache(IUIAutomationTreeWalker *iface,
        IUIAutomationElement *elem, IUIAutomationCacheRequest *cache_req, IUIAutomationElement **next)
{
    TRACE("%p, %p, %p, %p\n", iface, elem, cache_req, next);

    return uia_tree_walker_navigate(iface, cache_req, elem, NavigateDirection_NextSibling, next);
}

static HRESULT WINAPI uia_tree_walker_GetPreviousSiblingElementBuildCache(IUIAutomationTreeWalker *iface,
        IUIAutomationElement *elem, IUIAutomationCacheRequest *cache_req, IUIAutomationElement **prev)
{
    TRACE("%p, %p, %p, %p\n", iface, elem, cache_req, prev);

    return uia_tree_walker_navigate(iface, cache_req, elem, NavigateDirection_PreviousSibling, prev);
}

static HRESULT WINAPI uia_tree_walker_NormalizeElementBuildCache(IUIAutomationTreeWalker *iface,
        IUIAutomationElement *elem, IUIAutomationCacheRequest *cache_req, IUIAutomationElement **normalized)
{
    FIXME("%p, %p, %p, %p: stub\n", iface, elem, cache_req, normalized);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_tree_walker_get_Condition(IUIAutomationTreeWalker *iface,
        IUIAutomationCondition **condition)
{
    struct uia_tree_walker *tree_walker = impl_from_IUIAutomationTreeWalker(iface);

    TRACE("%p, %p\n", iface, condition);

    if (!condition)
        return E_POINTER;

    IUIAutomationCondition_AddRef(tree_walker->nav_cond);
    *condition = tree_walker->nav_cond;

    return S_OK;
}

static const IUIAutomationTreeWalkerVtbl uia_tree_walker_vtbl = {
    uia_tree_walker_QueryInterface,
    uia_tree_walker_AddRef,
    uia_tree_walker_Release,
    uia_tree_walker_GetParentElement,
    uia_tree_walker_GetFirstChildElement,
    uia_tree_walker_GetLastChildElement,
    uia_tree_walker_GetNextSiblingElement,
    uia_tree_walker_GetPreviousSiblingElement,
    uia_tree_walker_NormalizeElement,
    uia_tree_walker_GetParentElementBuildCache,
    uia_tree_walker_GetFirstChildElementBuildCache,
    uia_tree_walker_GetLastChildElementBuildCache,
    uia_tree_walker_GetNextSiblingElementBuildCache,
    uia_tree_walker_GetPreviousSiblingElementBuildCache,
    uia_tree_walker_NormalizeElementBuildCache,
    uia_tree_walker_get_Condition,
};

static HRESULT create_uia_tree_walker(IUIAutomationTreeWalker **out_tree_walker, IUIAutomationCondition *nav_cond)
{
    struct uia_tree_walker *tree_walker;
    struct UiaCondition *cond_struct;
    HRESULT hr;

    if (!out_tree_walker)
        return E_POINTER;

    *out_tree_walker = NULL;
    hr = get_uia_condition_struct_from_iface(nav_cond, &cond_struct);
    if (FAILED(hr))
        return hr;

    tree_walker = calloc(1, sizeof(*tree_walker));
    if (!tree_walker)
        return E_OUTOFMEMORY;

    tree_walker->IUIAutomationTreeWalker_iface.lpVtbl = &uia_tree_walker_vtbl;
    tree_walker->ref = 1;
    tree_walker->nav_cond = nav_cond;
    IUIAutomationCondition_AddRef(nav_cond);
    tree_walker->cond_struct = cond_struct;

    hr = create_uia_cache_request_iface(&tree_walker->default_cache_req);
    if (FAILED(hr))
    {
        IUIAutomationTreeWalker_Release(&tree_walker->IUIAutomationTreeWalker_iface);
        return hr;
    }

    *out_tree_walker = &tree_walker->IUIAutomationTreeWalker_iface;
    return S_OK;
}

/*
 * IUIAutomation interface.
 */
struct uia_iface {
    IUIAutomation6 IUIAutomation6_iface;
    LONG ref;

    BOOL is_cui8;
};

static inline struct uia_iface *impl_from_IUIAutomation6(IUIAutomation6 *iface)
{
    return CONTAINING_RECORD(iface, struct uia_iface, IUIAutomation6_iface);
}

static HRESULT WINAPI uia_iface_QueryInterface(IUIAutomation6 *iface, REFIID riid, void **ppv)
{
    struct uia_iface *uia_iface = impl_from_IUIAutomation6(iface);

    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUIAutomation) || IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else if (uia_iface->is_cui8 &&
            (IsEqualIID(riid, &IID_IUIAutomation2) ||
            IsEqualIID(riid, &IID_IUIAutomation3) ||
            IsEqualIID(riid, &IID_IUIAutomation4) ||
            IsEqualIID(riid, &IID_IUIAutomation5) ||
            IsEqualIID(riid, &IID_IUIAutomation6)))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IUIAutomation6_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI uia_iface_AddRef(IUIAutomation6 *iface)
{
    struct uia_iface *uia_iface = impl_from_IUIAutomation6(iface);
    ULONG ref = InterlockedIncrement(&uia_iface->ref);

    TRACE("%p, refcount %ld\n", uia_iface, ref);
    return ref;
}

static ULONG WINAPI uia_iface_Release(IUIAutomation6 *iface)
{
    struct uia_iface *uia_iface = impl_from_IUIAutomation6(iface);
    ULONG ref = InterlockedDecrement(&uia_iface->ref);

    TRACE("%p, refcount %ld\n", uia_iface, ref);
    if (!ref)
        free(uia_iface);
    return ref;
}

static HRESULT WINAPI uia_iface_CompareElements(IUIAutomation6 *iface, IUIAutomationElement *elem1,
        IUIAutomationElement *elem2, BOOL *match)
{
    FIXME("%p, %p, %p, %p: stub\n", iface, elem1, elem2, match);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_CompareRuntimeIds(IUIAutomation6 *iface, SAFEARRAY *rt_id1, SAFEARRAY *rt_id2,
        BOOL *match)
{
    FIXME("%p, %p, %p, %p: stub\n", iface, rt_id1, rt_id2, match);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_GetRootElement(IUIAutomation6 *iface, IUIAutomationElement **root)
{
    struct uia_iface *uia_iface = impl_from_IUIAutomation6(iface);
    HUIANODE node;
    HRESULT hr;

    TRACE("%p, %p\n", iface, root);

    if (!root)
        return E_POINTER;

    *root = NULL;
    hr = UiaGetRootNode(&node);
    if (FAILED(hr))
        return hr;

    return create_uia_element(root, uia_iface->is_cui8, node);
}

static HRESULT WINAPI uia_iface_ElementFromHandle(IUIAutomation6 *iface, UIA_HWND hwnd, IUIAutomationElement **out_elem)
{
    struct uia_iface *uia_iface = impl_from_IUIAutomation6(iface);
    HUIANODE node;
    HRESULT hr;

    TRACE("%p, %p, %p\n", iface, hwnd, out_elem);

    hr = UiaNodeFromHandle((HWND)hwnd, &node);
    if (FAILED(hr))
        return hr;

    return create_uia_element(out_elem, uia_iface->is_cui8, node);
}

static HRESULT WINAPI uia_iface_ElementFromPoint(IUIAutomation6 *iface, POINT pt, IUIAutomationElement **out_elem)
{
    FIXME("%p, %s, %p: stub\n", iface, wine_dbgstr_point(&pt), out_elem);
    return E_NOTIMPL;
}

static HRESULT uia_get_focused_element(IUIAutomation6 *iface, IUIAutomationCacheRequest *cache_req,
        BOOL use_default_cache_req, IUIAutomationElement **out_elem)
{
    struct uia_iface *uia_iface = impl_from_IUIAutomation6(iface);
    struct UiaCacheRequest *cache_req_struct;
    BSTR tree_struct;
    SAFEARRAY *sa;
    HRESULT hr;

    if (!out_elem)
        return E_POINTER;

    *out_elem = NULL;
    if (use_default_cache_req)
    {
        hr = create_uia_cache_request_iface(&cache_req);
        if (FAILED(hr))
            return hr;
    }

    hr = get_uia_cache_request_struct_from_iface(cache_req, &cache_req_struct);
    if (FAILED(hr))
        goto exit;

    hr = UiaNodeFromFocus(cache_req_struct, &sa, &tree_struct);
    if (SUCCEEDED(hr))
    {
        if (!sa)
        {
            /*
             * Failure to get a focused element returns E_FAIL from the BuildCache
             * method, but UIA_E_ELEMENTNOTAVAILABLE from the default cache
             * request method.
             */
            hr = use_default_cache_req ? UIA_E_ELEMENTNOTAVAILABLE : E_FAIL;
            SysFreeString(tree_struct);
            goto exit;
        }

        hr = create_uia_element_from_cache_req(out_elem, uia_iface->is_cui8, cache_req_struct, 0, sa, tree_struct);
        SafeArrayDestroy(sa);
    }

exit:
    if (use_default_cache_req)
        IUIAutomationCacheRequest_Release(cache_req);

    return hr;
}

static HRESULT WINAPI uia_iface_GetFocusedElement(IUIAutomation6 *iface, IUIAutomationElement **out_elem)
{
    TRACE("%p, %p\n", iface, out_elem);

    return uia_get_focused_element(iface, NULL, TRUE, out_elem);
}

static HRESULT WINAPI uia_iface_GetRootElementBuildCache(IUIAutomation6 *iface, IUIAutomationCacheRequest *cache_req,
        IUIAutomationElement **out_root)
{
    FIXME("%p, %p, %p: stub\n", iface, cache_req, out_root);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_ElementFromHandleBuildCache(IUIAutomation6 *iface, UIA_HWND hwnd,
        IUIAutomationCacheRequest *cache_req, IUIAutomationElement **out_elem)
{
    FIXME("%p, %p, %p, %p: stub\n", iface, hwnd, cache_req, out_elem);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_ElementFromPointBuildCache(IUIAutomation6 *iface, POINT pt,
        IUIAutomationCacheRequest *cache_req, IUIAutomationElement **out_elem)
{
    FIXME("%p, %s, %p, %p: stub\n", iface, wine_dbgstr_point(&pt), cache_req, out_elem);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_GetFocusedElementBuildCache(IUIAutomation6 *iface,
        IUIAutomationCacheRequest *cache_req, IUIAutomationElement **out_elem)
{
    TRACE("%p, %p, %p\n", iface, cache_req, out_elem);

    return uia_get_focused_element(iface, cache_req, FALSE, out_elem);
}

static HRESULT WINAPI uia_iface_CreateTreeWalker(IUIAutomation6 *iface, IUIAutomationCondition *cond,
        IUIAutomationTreeWalker **out_walker)
{
    TRACE("%p, %p, %p\n", iface, cond, out_walker);

    return create_uia_tree_walker(out_walker, cond);
}

static HRESULT WINAPI uia_iface_get_ControlViewWalker(IUIAutomation6 *iface, IUIAutomationTreeWalker **out_walker)
{
    FIXME("%p, %p: stub\n", iface, out_walker);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_get_ContentViewWalker(IUIAutomation6 *iface, IUIAutomationTreeWalker **out_walker)
{
    FIXME("%p, %p: stub\n", iface, out_walker);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_get_RawViewWalker(IUIAutomation6 *iface, IUIAutomationTreeWalker **out_walker)
{
    FIXME("%p, %p: stub\n", iface, out_walker);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_get_RawViewCondition(IUIAutomation6 *iface, IUIAutomationCondition **out_condition)
{
    TRACE("%p, %p\n", iface, out_condition);

    return create_uia_bool_condition_iface(out_condition, ConditionType_True);
}

static HRESULT WINAPI uia_iface_get_ControlViewCondition(IUIAutomation6 *iface, IUIAutomationCondition **out_condition)
{
    TRACE("%p, %p\n", iface, out_condition);

    return create_control_view_condition_iface(out_condition);
}

static HRESULT WINAPI uia_iface_get_ContentViewCondition(IUIAutomation6 *iface, IUIAutomationCondition **out_condition)
{
    FIXME("%p, %p: stub\n", iface, out_condition);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_CreateCacheRequest(IUIAutomation6 *iface, IUIAutomationCacheRequest **out_cache_req)
{
    HRESULT hr;

    TRACE("%p, %p\n", iface, out_cache_req);

    hr = create_uia_cache_request_iface(out_cache_req);
    if (FAILED(hr))
        return hr;

    hr = IUIAutomationCacheRequest_AddProperty(*out_cache_req, UIA_RuntimeIdPropertyId);
    if (FAILED(hr))
    {
        IUIAutomationCacheRequest_Release(*out_cache_req);
        *out_cache_req = NULL;
    }

    return hr;
}

static HRESULT WINAPI uia_iface_CreateTrueCondition(IUIAutomation6 *iface, IUIAutomationCondition **out_condition)
{
    TRACE("%p, %p\n", iface, out_condition);

    return create_uia_bool_condition_iface(out_condition, ConditionType_True);
}

static HRESULT WINAPI uia_iface_CreateFalseCondition(IUIAutomation6 *iface, IUIAutomationCondition **out_condition)
{
    TRACE("%p, %p\n", iface, out_condition);

    return create_uia_bool_condition_iface(out_condition, ConditionType_False);
}

static HRESULT WINAPI uia_iface_CreatePropertyCondition(IUIAutomation6 *iface, PROPERTYID prop_id, VARIANT val,
        IUIAutomationCondition **out_condition)
{
    TRACE("%p, %d, %s, %p\n", iface, prop_id, debugstr_variant(&val), out_condition);

    return create_uia_property_condition_iface(out_condition, prop_id, val, PropertyConditionFlags_None);
}

static HRESULT WINAPI uia_iface_CreatePropertyConditionEx(IUIAutomation6 *iface, PROPERTYID prop_id, VARIANT val,
        enum PropertyConditionFlags flags, IUIAutomationCondition **out_condition)
{
    FIXME("%p, %d, %s, %#x, %p: stub\n", iface, prop_id, debugstr_variant(&val), flags, out_condition);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_CreateAndCondition(IUIAutomation6 *iface, IUIAutomationCondition *cond1,
        IUIAutomationCondition *cond2, IUIAutomationCondition **out_condition)
{
    FIXME("%p, %p, %p, %p: stub\n", iface, cond1, cond2, out_condition);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_CreateAndConditionFromArray(IUIAutomation6 *iface, SAFEARRAY *conds,
        IUIAutomationCondition **out_condition)
{
    FIXME("%p, %p, %p: stub\n", iface, conds, out_condition);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_CreateAndConditionFromNativeArray(IUIAutomation6 *iface, IUIAutomationCondition **conds,
        int conds_count, IUIAutomationCondition **out_condition)
{
    FIXME("%p, %p, %d, %p: stub\n", iface, conds, conds_count, out_condition);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_CreateOrCondition(IUIAutomation6 *iface, IUIAutomationCondition *cond1,
        IUIAutomationCondition *cond2, IUIAutomationCondition **out_condition)
{
    IUIAutomationCondition *cond_arr[2] = { cond1, cond2 };

    TRACE("%p, %p, %p, %p\n", iface, cond1, cond2, out_condition);

    return create_uia_or_condition_iface(out_condition, cond_arr, ARRAY_SIZE(cond_arr));
}

static HRESULT WINAPI uia_iface_CreateOrConditionFromArray(IUIAutomation6 *iface, SAFEARRAY *conds,
        IUIAutomationCondition **out_condition)
{
    FIXME("%p, %p, %p: stub\n", iface, conds, out_condition);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_CreateOrConditionFromNativeArray(IUIAutomation6 *iface, IUIAutomationCondition **conds,
        int conds_count, IUIAutomationCondition **out_condition)
{
    FIXME("%p, %p, %d, %p: stub\n", iface, conds, conds_count, out_condition);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_CreateNotCondition(IUIAutomation6 *iface, IUIAutomationCondition *cond,
        IUIAutomationCondition **out_condition)
{
    TRACE("%p, %p, %p\n", iface, cond, out_condition);

    return create_uia_not_condition_iface(out_condition, cond);
}

static HRESULT uia_add_com_event_handler(IUIAutomation6 *iface, EVENTID event_id, IUIAutomationElement *elem,
        enum TreeScope scope, IUIAutomationCacheRequest *cache_req, REFIID handler_riid, IUnknown *handler_unk)
{
    struct UiaCacheRequest *cache_req_struct;
    struct uia_com_event *com_event = NULL;
    SAFEARRAY *runtime_id = NULL;
    struct uia_element *element;
    IUnknown *handler_iface;
    HRESULT hr;

    element = impl_from_IUIAutomationElement9((IUIAutomationElement9 *)elem);
    hr = UiaGetRuntimeId(element->node, &runtime_id);
    if (FAILED(hr))
        return hr;

    if (!cache_req)
    {
        hr = create_uia_cache_request_iface(&cache_req);
        if (FAILED(hr))
            goto exit;
    }
    else
        IUIAutomationCacheRequest_AddRef(cache_req);

    hr = get_uia_cache_request_struct_from_iface(cache_req, &cache_req_struct);
    if (FAILED(hr))
        goto exit;

    if (!(com_event = calloc(1, sizeof(*com_event))))
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    com_event->from_cui8 = element->from_cui8;
    list_init(&com_event->event_handler_map_list_entry);
    uia_hwnd_map_init(&com_event->focus_hwnd_map);

    hr = IUnknown_QueryInterface(handler_unk, handler_riid, (void **)&handler_iface);
    if (FAILED(hr))
        goto exit;

    hr = register_interface_in_git(handler_iface, handler_riid, &com_event->git_cookie);
    IUnknown_Release(handler_iface);
    if (FAILED(hr))
        goto exit;

    hr = uia_add_clientside_event(element->node, event_id, scope, NULL, 0, cache_req_struct, runtime_id,
            uia_com_event_callback, (void *)com_event, &com_event->event);
    if (FAILED(hr))
        goto exit;

    if (!uia_clientside_event_start_event_thread((struct uia_event *)com_event->event))
        WARN("Failed to start event thread, WinEvents may not be delivered.\n");

    hr = uia_event_handlers_add_handler(handler_unk, runtime_id, event_id, com_event);

exit:
    if (FAILED(hr) && com_event)
        uia_event_handler_destroy(com_event);
    if (cache_req)
        IUIAutomationCacheRequest_Release(cache_req);
    SafeArrayDestroy(runtime_id);

    return hr;
}

static HRESULT WINAPI uia_iface_AddAutomationEventHandler(IUIAutomation6 *iface, EVENTID event_id,
        IUIAutomationElement *elem, enum TreeScope scope, IUIAutomationCacheRequest *cache_req,
        IUIAutomationEventHandler *handler)
{
    IUnknown *handler_unk;
    HRESULT hr;

    TRACE("%p, %d, %p, %#x, %p, %p\n", iface, event_id, elem, scope, cache_req, handler);

    if (!elem || !handler)
        return E_POINTER;

    if (event_id == UIA_AutomationFocusChangedEventId)
        return E_INVALIDARG;

    hr = IUIAutomationEventHandler_QueryInterface(handler, &IID_IUnknown, (void **)&handler_unk);
    if (FAILED(hr))
        return hr;

    hr = uia_add_com_event_handler(iface, event_id, elem, scope, cache_req, &IID_IUIAutomationEventHandler, handler_unk);
    IUnknown_Release(handler_unk);

    return hr;
}

static HRESULT uia_remove_com_event_handler(EVENTID event_id, IUIAutomationElement *elem, IUnknown *handler_unk)
{
    struct uia_element *element;
    SAFEARRAY *runtime_id;
    HRESULT hr;

    element = impl_from_IUIAutomationElement9((IUIAutomationElement9 *)elem);
    hr = UiaGetRuntimeId(element->node, &runtime_id);
    if (FAILED(hr) || !runtime_id)
        return hr;

    uia_event_handlers_remove_handlers(handler_unk, runtime_id, event_id);
    SafeArrayDestroy(runtime_id);

    return S_OK;
}

static HRESULT WINAPI uia_iface_RemoveAutomationEventHandler(IUIAutomation6 *iface, EVENTID event_id,
        IUIAutomationElement *elem, IUIAutomationEventHandler *handler)
{
    IUnknown *handler_unk;
    HRESULT hr;

    TRACE("%p, %d, %p, %p\n", iface, event_id, elem, handler);

    if (!elem || !handler)
        return S_OK;

    hr = IUIAutomationEventHandler_QueryInterface(handler, &IID_IUnknown, (void **)&handler_unk);
    if (FAILED(hr))
        return hr;

    hr = uia_remove_com_event_handler(event_id, elem, handler_unk);
    IUnknown_Release(handler_unk);

    return hr;
}

static HRESULT WINAPI uia_iface_AddPropertyChangedEventHandlerNativeArray(IUIAutomation6 *iface,
        IUIAutomationElement *elem, enum TreeScope scope, IUIAutomationCacheRequest *cache_req,
        IUIAutomationPropertyChangedEventHandler *handler, PROPERTYID *props, int props_count)
{
    FIXME("%p, %p, %#x, %p, %p, %p, %d: stub\n", iface, elem, scope, cache_req, handler, props, props_count);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_AddPropertyChangedEventHandler(IUIAutomation6 *iface,
        IUIAutomationElement *elem, enum TreeScope scope, IUIAutomationCacheRequest *cache_req,
        IUIAutomationPropertyChangedEventHandler *handler, SAFEARRAY *props)
{
    FIXME("%p, %p, %#x, %p, %p, %p: stub\n", iface, elem, scope, cache_req, handler, props);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_RemovePropertyChangedEventHandler(IUIAutomation6 *iface,
        IUIAutomationElement *elem, IUIAutomationPropertyChangedEventHandler *handler)
{
    FIXME("%p, %p, %p: stub\n", iface, elem, handler);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_AddStructureChangedEventHandler(IUIAutomation6 *iface,
        IUIAutomationElement *elem, enum TreeScope scope, IUIAutomationCacheRequest *cache_req,
        IUIAutomationStructureChangedEventHandler *handler)
{
    FIXME("%p, %p, %#x, %p, %p: stub\n", iface, elem, scope, cache_req, handler);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_RemoveStructureChangedEventHandler(IUIAutomation6 *iface,
        IUIAutomationElement *elem, IUIAutomationStructureChangedEventHandler *handler)
{
    FIXME("%p, %p, %p: stub\n", iface, elem, handler);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_AddFocusChangedEventHandler(IUIAutomation6 *iface,
        IUIAutomationCacheRequest *cache_req, IUIAutomationFocusChangedEventHandler *handler)
{
    IUIAutomationElement *elem;
    IUnknown *handler_unk;
    HRESULT hr;

    TRACE("%p, %p, %p\n", iface, cache_req, handler);

    if (!handler)
        return E_POINTER;

    hr = IUIAutomationFocusChangedEventHandler_QueryInterface(handler, &IID_IUnknown, (void **)&handler_unk);
    if (FAILED(hr))
        return hr;

    hr = IUIAutomation6_GetRootElement(iface, &elem);
    if (FAILED(hr))
    {
        IUnknown_Release(handler_unk);
        return hr;
    }

    hr = uia_add_com_event_handler(iface, UIA_AutomationFocusChangedEventId, elem, TreeScope_Subtree, cache_req,
            &IID_IUIAutomationFocusChangedEventHandler, handler_unk);
    IUIAutomationElement_Release(elem);
    IUnknown_Release(handler_unk);

    return hr;
}

static HRESULT WINAPI uia_iface_RemoveFocusChangedEventHandler(IUIAutomation6 *iface,
        IUIAutomationFocusChangedEventHandler *handler)
{
    IUIAutomationElement *elem;
    IUnknown *handler_unk;
    HRESULT hr;

    TRACE("%p, %p\n", iface, handler);

    hr = IUIAutomationFocusChangedEventHandler_QueryInterface(handler, &IID_IUnknown, (void **)&handler_unk);
    if (FAILED(hr))
        return hr;

    hr = IUIAutomation6_GetRootElement(iface, &elem);
    if (FAILED(hr))
    {
        IUnknown_Release(handler_unk);
        return hr;
    }

    hr = uia_remove_com_event_handler(UIA_AutomationFocusChangedEventId, elem, handler_unk);
    IUIAutomationElement_Release(elem);
    IUnknown_Release(handler_unk);

    return hr;
}

static HRESULT WINAPI uia_iface_RemoveAllEventHandlers(IUIAutomation6 *iface)
{
    struct uia_event_handler_map_entry *entry, *cursor;

    TRACE("%p\n", iface);

    EnterCriticalSection(&com_event_handlers_cs);
    if (!com_event_handlers.handler_count)
        goto exit;

    RB_FOR_EACH_ENTRY_DESTRUCTOR(entry, cursor, &com_event_handlers.handler_map, struct uia_event_handler_map_entry, entry)
    {
        uia_event_handler_map_entry_destroy(entry);
    }

exit:
    LeaveCriticalSection(&com_event_handlers_cs);

    return S_OK;
}

static HRESULT WINAPI uia_iface_IntNativeArrayToSafeArray(IUIAutomation6 *iface, int *arr, int arr_count,
        SAFEARRAY **out_sa)
{
    HRESULT hr = S_OK;
    SAFEARRAY *sa;
    int *sa_arr;

    TRACE("%p, %p, %d, %p\n", iface, arr, arr_count, out_sa);

    if (!out_sa || !arr || !arr_count)
        return E_INVALIDARG;

    *out_sa = NULL;
    if (!(sa = SafeArrayCreateVector(VT_I4, 0, arr_count)))
        return E_OUTOFMEMORY;

    hr = SafeArrayAccessData(sa, (void **)&sa_arr);
    if (FAILED(hr))
        goto exit;

    memcpy(sa_arr, arr, sizeof(*arr) * arr_count);
    hr = SafeArrayUnaccessData(sa);
    if (SUCCEEDED(hr))
        *out_sa = sa;

exit:
    if (FAILED(hr))
        SafeArrayDestroy(sa);

    return hr;
}

static HRESULT WINAPI uia_iface_IntSafeArrayToNativeArray(IUIAutomation6 *iface, SAFEARRAY *sa, int **out_arr,
        int *out_arr_count)
{
    LONG lbound, elems;
    int *arr, *sa_arr;
    VARTYPE vt;
    HRESULT hr;

    TRACE("%p, %p, %p, %p\n", iface, sa, out_arr, out_arr_count);

    if (!sa || !out_arr || !out_arr_count)
        return E_INVALIDARG;

    *out_arr = NULL;
    hr = SafeArrayGetVartype(sa, &vt);
    if (FAILED(hr))
        return hr;

    if (vt != VT_I4)
        return E_INVALIDARG;

    hr = get_safearray_bounds(sa, &lbound, &elems);
    if (FAILED(hr))
        return hr;

    if (!(arr = CoTaskMemAlloc(elems * sizeof(*arr))))
        return E_OUTOFMEMORY;

    hr = SafeArrayAccessData(sa, (void **)&sa_arr);
    if (FAILED(hr))
        goto exit;

    memcpy(arr, sa_arr, sizeof(*arr) * elems);
    hr = SafeArrayUnaccessData(sa);
    if (FAILED(hr))
        goto exit;

    *out_arr = arr;
    *out_arr_count = elems;

exit:
    if (FAILED(hr))
        CoTaskMemFree(arr);

    return hr;
}

static HRESULT WINAPI uia_iface_RectToVariant(IUIAutomation6 *iface, RECT rect, VARIANT *out_var)
{
    FIXME("%p, %s, %p: stub\n", iface, wine_dbgstr_rect(&rect), out_var);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_VariantToRect(IUIAutomation6 *iface, VARIANT var, RECT *out_rect)
{
    FIXME("%p, %s, %p: stub\n", iface, debugstr_variant(&var), out_rect);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_SafeArrayToRectNativeArray(IUIAutomation6 *iface, SAFEARRAY *sa, RECT **out_rect_arr,
        int *out_rect_arr_count)
{
    FIXME("%p, %p, %p, %p: stub\n", iface, sa, out_rect_arr, out_rect_arr_count);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_CreateProxyFactoryEntry(IUIAutomation6 *iface, IUIAutomationProxyFactory *factory,
        IUIAutomationProxyFactoryEntry **out_entry)
{
    FIXME("%p, %p, %p: stub\n", iface, factory, out_entry);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_get_ProxyFactoryMapping(IUIAutomation6 *iface,
        IUIAutomationProxyFactoryMapping **out_factory_map)
{
    FIXME("%p, %p: stub\n", iface, out_factory_map);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_GetPropertyProgrammaticName(IUIAutomation6 *iface, PROPERTYID prop_id, BSTR *out_name)
{
    FIXME("%p, %d, %p: stub\n", iface, prop_id, out_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_GetPatternProgrammaticName(IUIAutomation6 *iface, PATTERNID pattern_id, BSTR *out_name)
{
    FIXME("%p, %d, %p: stub\n", iface, pattern_id, out_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_PollForPotentialSupportedPatterns(IUIAutomation6 *iface, IUIAutomationElement *elem,
        SAFEARRAY **out_pattern_ids, SAFEARRAY **out_pattern_names)
{
    FIXME("%p, %p, %p, %p: stub\n", iface, elem, out_pattern_ids, out_pattern_names);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_PollForPotentialSupportedProperties(IUIAutomation6 *iface, IUIAutomationElement *elem,
        SAFEARRAY **out_prop_ids, SAFEARRAY **out_prop_names)
{
    FIXME("%p, %p, %p, %p: stub\n", iface, elem, out_prop_ids, out_prop_names);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_CheckNotSupported(IUIAutomation6 *iface, VARIANT in_val, BOOL *match)
{
    IUnknown *unk;

    TRACE("%p, %s, %p\n", iface, debugstr_variant(&in_val), match);

    *match = FALSE;
    UiaGetReservedNotSupportedValue(&unk);
    if (V_VT(&in_val) == VT_UNKNOWN && (V_UNKNOWN(&in_val) == unk))
        *match = TRUE;

    return S_OK;
}

static HRESULT WINAPI uia_iface_get_ReservedNotSupportedValue(IUIAutomation6 *iface, IUnknown **out_unk)
{
    TRACE("%p, %p\n", iface, out_unk);

    return UiaGetReservedNotSupportedValue(out_unk);
}

static HRESULT WINAPI uia_iface_get_ReservedMixedAttributeValue(IUIAutomation6 *iface, IUnknown **out_unk)
{
    TRACE("%p, %p\n", iface, out_unk);

    return UiaGetReservedMixedAttributeValue(out_unk);
}

static HRESULT WINAPI uia_iface_ElementFromIAccessible(IUIAutomation6 *iface, IAccessible *acc, int cid,
        IUIAutomationElement **out_elem)
{
    FIXME("%p, %p, %d, %p: stub\n", iface, acc, cid, out_elem);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_ElementFromIAccessibleBuildCache(IUIAutomation6 *iface, IAccessible *acc, int cid,
        IUIAutomationCacheRequest *cache_req, IUIAutomationElement **out_elem)
{
    FIXME("%p, %p, %d, %p, %p: stub\n", iface, acc, cid, cache_req, out_elem);
    return E_NOTIMPL;
}

/* IUIAutomation2 methods */
static HRESULT WINAPI uia_iface_get_AutoSetFocus(IUIAutomation6 *iface, BOOL *out_auto_set_focus)
{
    FIXME("%p, %p: stub\n", iface, out_auto_set_focus);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_put_AutoSetFocus(IUIAutomation6 *iface, BOOL auto_set_focus)
{
    FIXME("%p, %d: stub\n", iface, auto_set_focus);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_get_ConnectionTimeout(IUIAutomation6 *iface, DWORD *out_timeout)
{
    FIXME("%p, %p: stub\n", iface, out_timeout);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_put_ConnectionTimeout(IUIAutomation6 *iface, DWORD timeout)
{
    FIXME("%p, %ld: stub\n", iface, timeout);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_get_TransactionTimeout(IUIAutomation6 *iface, DWORD *out_timeout)
{
    FIXME("%p, %p: stub\n", iface, out_timeout);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_put_TransactionTimeout(IUIAutomation6 *iface, DWORD timeout)
{
    FIXME("%p, %ld: stub\n", iface, timeout);
    return E_NOTIMPL;
}

/* IUIAutomation3 methods */
static HRESULT WINAPI uia_iface_AddTextEditTextChangedEventHandler(IUIAutomation6 *iface, IUIAutomationElement *elem,
        enum TreeScope scope, enum TextEditChangeType change_type, IUIAutomationCacheRequest *cache_req,
        IUIAutomationTextEditTextChangedEventHandler *handler)
{
    FIXME("%p, %p, %#x, %d, %p, %p: stub\n", iface, elem, scope, change_type, cache_req, handler);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_RemoveTextEditTextChangedEventHandler(IUIAutomation6 *iface, IUIAutomationElement *elem,
        IUIAutomationTextEditTextChangedEventHandler *handler)
{
    FIXME("%p, %p, %p: stub\n", iface, elem, handler);
    return E_NOTIMPL;
}

/* IUIAutomation4 methods */
static HRESULT WINAPI uia_iface_AddChangesEventHandler(IUIAutomation6 *iface, IUIAutomationElement *elem,
        enum TreeScope scope, int *change_types, int change_types_count, IUIAutomationCacheRequest *cache_req,
        IUIAutomationChangesEventHandler *handler)
{
    FIXME("%p, %p, %#x, %p, %d, %p, %p: stub\n", iface, elem, scope, change_types, change_types_count, cache_req,
            handler);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_RemoveChangesEventHandler(IUIAutomation6 *iface, IUIAutomationElement *elem,
        IUIAutomationChangesEventHandler *handler)
{
    FIXME("%p, %p, %p: stub\n", iface, elem, handler);
    return E_NOTIMPL;
}

/* IUIAutomation5 methods */
static HRESULT WINAPI uia_iface_AddNotificationEventHandler(IUIAutomation6 *iface, IUIAutomationElement *elem,
        enum TreeScope scope, IUIAutomationCacheRequest *cache_req, IUIAutomationNotificationEventHandler *handler)
{
    FIXME("%p, %p, %#x, %p, %p: stub\n", iface, elem, scope, cache_req, handler);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_RemoveNotificationEventHandler(IUIAutomation6 *iface, IUIAutomationElement *elem,
        IUIAutomationNotificationEventHandler *handler)
{
    FIXME("%p, %p, %p: stub\n", iface, elem, handler);
    return E_NOTIMPL;
}

/* IUIAutomation6 methods */
static HRESULT WINAPI uia_iface_CreateEventHandlerGroup(IUIAutomation6 *iface,
        IUIAutomationEventHandlerGroup **out_handler_group)
{
    FIXME("%p, %p: stub\n", iface, out_handler_group);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_AddEventHandlerGroup(IUIAutomation6 *iface, IUIAutomationElement *elem,
        IUIAutomationEventHandlerGroup *handler_group)
{
    FIXME("%p, %p, %p: stub\n", iface, elem, handler_group);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_RemoveEventHandlerGroup(IUIAutomation6 *iface, IUIAutomationElement *elem,
        IUIAutomationEventHandlerGroup *handler_group)
{
    FIXME("%p, %p, %p: stub\n", iface, elem, handler_group);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_get_ConnectionRecoveryBehavior(IUIAutomation6 *iface,
        enum ConnectionRecoveryBehaviorOptions *out_conn_recovery_opts)
{
    FIXME("%p, %p: stub\n", iface, out_conn_recovery_opts);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_put_ConnectionRecoveryBehavior(IUIAutomation6 *iface,
        enum ConnectionRecoveryBehaviorOptions conn_recovery_opts)
{
    FIXME("%p, %#x: stub\n", iface, conn_recovery_opts);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_get_CoalesceEvents(IUIAutomation6 *iface,
        enum CoalesceEventsOptions *out_coalesce_events_opts)
{
    FIXME("%p, %p: stub\n", iface, out_coalesce_events_opts);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_put_CoalesceEvents(IUIAutomation6 *iface,
        enum CoalesceEventsOptions coalesce_events_opts)
{
    FIXME("%p, %#x: stub\n", iface, coalesce_events_opts);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_AddActiveTextPositionChangedEventHandler(IUIAutomation6 *iface,
        IUIAutomationElement *elem, enum TreeScope scope, IUIAutomationCacheRequest *cache_req,
        IUIAutomationActiveTextPositionChangedEventHandler *handler)
{
    FIXME("%p, %p, %#x, %p, %p: stub\n", iface, elem, scope, cache_req, handler);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_RemoveActiveTextPositionChangedEventHandler(IUIAutomation6 *iface,
        IUIAutomationElement *elem, IUIAutomationActiveTextPositionChangedEventHandler *handler)
{
    FIXME("%p, %p, %p\n", iface, elem, handler);
    return E_NOTIMPL;
}

static const IUIAutomation6Vtbl uia_iface_vtbl = {
    uia_iface_QueryInterface,
    uia_iface_AddRef,
    uia_iface_Release,
    /* IUIAutomation methods */
    uia_iface_CompareElements,
    uia_iface_CompareRuntimeIds,
    uia_iface_GetRootElement,
    uia_iface_ElementFromHandle,
    uia_iface_ElementFromPoint,
    uia_iface_GetFocusedElement,
    uia_iface_GetRootElementBuildCache,
    uia_iface_ElementFromHandleBuildCache,
    uia_iface_ElementFromPointBuildCache,
    uia_iface_GetFocusedElementBuildCache,
    uia_iface_CreateTreeWalker,
    uia_iface_get_ControlViewWalker,
    uia_iface_get_ContentViewWalker,
    uia_iface_get_RawViewWalker,
    uia_iface_get_RawViewCondition,
    uia_iface_get_ControlViewCondition,
    uia_iface_get_ContentViewCondition,
    uia_iface_CreateCacheRequest,
    uia_iface_CreateTrueCondition,
    uia_iface_CreateFalseCondition,
    uia_iface_CreatePropertyCondition,
    uia_iface_CreatePropertyConditionEx,
    uia_iface_CreateAndCondition,
    uia_iface_CreateAndConditionFromArray,
    uia_iface_CreateAndConditionFromNativeArray,
    uia_iface_CreateOrCondition,
    uia_iface_CreateOrConditionFromArray,
    uia_iface_CreateOrConditionFromNativeArray,
    uia_iface_CreateNotCondition,
    uia_iface_AddAutomationEventHandler,
    uia_iface_RemoveAutomationEventHandler,
    uia_iface_AddPropertyChangedEventHandlerNativeArray,
    uia_iface_AddPropertyChangedEventHandler,
    uia_iface_RemovePropertyChangedEventHandler,
    uia_iface_AddStructureChangedEventHandler,
    uia_iface_RemoveStructureChangedEventHandler,
    uia_iface_AddFocusChangedEventHandler,
    uia_iface_RemoveFocusChangedEventHandler,
    uia_iface_RemoveAllEventHandlers,
    uia_iface_IntNativeArrayToSafeArray,
    uia_iface_IntSafeArrayToNativeArray,
    uia_iface_RectToVariant,
    uia_iface_VariantToRect,
    uia_iface_SafeArrayToRectNativeArray,
    uia_iface_CreateProxyFactoryEntry,
    uia_iface_get_ProxyFactoryMapping,
    uia_iface_GetPropertyProgrammaticName,
    uia_iface_GetPatternProgrammaticName,
    uia_iface_PollForPotentialSupportedPatterns,
    uia_iface_PollForPotentialSupportedProperties,
    uia_iface_CheckNotSupported,
    uia_iface_get_ReservedNotSupportedValue,
    uia_iface_get_ReservedMixedAttributeValue,
    uia_iface_ElementFromIAccessible,
    uia_iface_ElementFromIAccessibleBuildCache,
    /* IUIAutomation2 methods */
    uia_iface_get_AutoSetFocus,
    uia_iface_put_AutoSetFocus,
    uia_iface_get_ConnectionTimeout,
    uia_iface_put_ConnectionTimeout,
    uia_iface_get_TransactionTimeout,
    uia_iface_put_TransactionTimeout,
    /* IUIAutomation3 methods */
    uia_iface_AddTextEditTextChangedEventHandler,
    uia_iface_RemoveTextEditTextChangedEventHandler,
    /* IUIAutomation4 methods */
    uia_iface_AddChangesEventHandler,
    uia_iface_RemoveChangesEventHandler,
    /* IUIAutomation5 methods */
    uia_iface_AddNotificationEventHandler,
    uia_iface_RemoveNotificationEventHandler,
    /* IUIAutomation6 methods */
    uia_iface_CreateEventHandlerGroup,
    uia_iface_AddEventHandlerGroup,
    uia_iface_RemoveEventHandlerGroup,
    uia_iface_get_ConnectionRecoveryBehavior,
    uia_iface_put_ConnectionRecoveryBehavior,
    uia_iface_get_CoalesceEvents,
    uia_iface_put_CoalesceEvents,
    uia_iface_AddActiveTextPositionChangedEventHandler,
    uia_iface_RemoveActiveTextPositionChangedEventHandler,
};

HRESULT create_uia_iface(IUnknown **iface, BOOL is_cui8)
{
    struct uia_iface *uia;

    uia = calloc(1, sizeof(*uia));
    if (!uia)
        return E_OUTOFMEMORY;

    uia->IUIAutomation6_iface.lpVtbl = &uia_iface_vtbl;
    uia->is_cui8 = is_cui8;
    uia->ref = 1;

    *iface = (IUnknown *)&uia->IUIAutomation6_iface;
    return S_OK;
}
