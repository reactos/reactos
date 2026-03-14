/*
 * Copyright 2017 Jacek Caban for CodeWeavers
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

#include "combaseapi.h"
#include "initguid.h"
#include "uia_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uiautomation);

HMODULE huia_module;

struct uia_object_wrapper
{
    IUnknown IUnknown_iface;
    LONG refcount;

    IUnknown *marshaler;
    IUnknown *marshal_object;
};

static struct uia_object_wrapper *impl_uia_object_wrapper_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct uia_object_wrapper, IUnknown_iface);
}

static HRESULT WINAPI uia_object_wrapper_QueryInterface(IUnknown *iface,
        REFIID riid, void **ppv)
{
    struct uia_object_wrapper *wrapper = impl_uia_object_wrapper_from_IUnknown(iface);
    return IUnknown_QueryInterface(wrapper->marshal_object, riid, ppv);
}

static ULONG WINAPI uia_object_wrapper_AddRef(IUnknown *iface)
{
    struct uia_object_wrapper *wrapper = impl_uia_object_wrapper_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&wrapper->refcount);

    TRACE("%p, refcount %ld\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI uia_object_wrapper_Release(IUnknown *iface)
{
    struct uia_object_wrapper *wrapper = impl_uia_object_wrapper_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&wrapper->refcount);

    TRACE("%p, refcount %ld\n", iface, refcount);
    if (!refcount)
    {
        IUnknown_Release(wrapper->marshaler);
        free(wrapper);
    }

    return refcount;
}

static const IUnknownVtbl uia_object_wrapper_vtbl = {
    uia_object_wrapper_QueryInterface,
    uia_object_wrapper_AddRef,
    uia_object_wrapper_Release,
};

/*
 * When passing the ReservedNotSupportedValue/ReservedMixedAttributeValue
 * interface pointers across apartments within the same process, create a free
 * threaded marshaler so that the pointer value is preserved.
 */
static HRESULT create_uia_object_wrapper(IUnknown *reserved, void **ppv)
{
    struct uia_object_wrapper *wrapper;
    HRESULT hr;

    TRACE("%p, %p\n", reserved, ppv);

    wrapper = calloc(1, sizeof(*wrapper));
    if (!wrapper)
        return E_OUTOFMEMORY;

    wrapper->IUnknown_iface.lpVtbl = &uia_object_wrapper_vtbl;
    wrapper->marshal_object = reserved;
    wrapper->refcount = 1;

    if (FAILED(hr = CoCreateFreeThreadedMarshaler(&wrapper->IUnknown_iface, &wrapper->marshaler)))
    {
        free(wrapper);
        return hr;
    }

    hr = IUnknown_QueryInterface(wrapper->marshaler, &IID_IMarshal, ppv);
    IUnknown_Release(&wrapper->IUnknown_iface);

    return hr;
}

/*
 * UiaReservedNotSupportedValue/UiaReservedMixedAttributeValue object.
 */
static HRESULT WINAPI uia_reserved_obj_QueryInterface(IUnknown *iface,
        REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IMarshal))
        return create_uia_object_wrapper(iface, ppv);
    else
        return E_NOINTERFACE;

    return S_OK;
}

static ULONG WINAPI uia_reserved_obj_AddRef(IUnknown *iface)
{
    return 1;
}

static ULONG WINAPI uia_reserved_obj_Release(IUnknown *iface)
{
    return 1;
}

static const IUnknownVtbl uia_reserved_obj_vtbl = {
    uia_reserved_obj_QueryInterface,
    uia_reserved_obj_AddRef,
    uia_reserved_obj_Release,
};

static IUnknown uia_reserved_ns_iface = {&uia_reserved_obj_vtbl};
static IUnknown uia_reserved_ma_iface = {&uia_reserved_obj_vtbl};

/*
 * UiaHostProviderFromHwnd IRawElementProviderSimple interface.
 */
struct hwnd_host_provider {
    IRawElementProviderSimple IRawElementProviderSimple_iface;
    LONG refcount;

    HWND hwnd;
};

static inline struct hwnd_host_provider *impl_from_hwnd_host_provider(IRawElementProviderSimple *iface)
{
    return CONTAINING_RECORD(iface, struct hwnd_host_provider, IRawElementProviderSimple_iface);
}

HRESULT WINAPI hwnd_host_provider_QueryInterface(IRawElementProviderSimple *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IRawElementProviderSimple) || IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IRawElementProviderSimple_AddRef(iface);
    return S_OK;
}

ULONG WINAPI hwnd_host_provider_AddRef(IRawElementProviderSimple *iface)
{
    struct hwnd_host_provider *host_prov = impl_from_hwnd_host_provider(iface);
    ULONG refcount = InterlockedIncrement(&host_prov->refcount);

    TRACE("%p, refcount %ld\n", iface, refcount);

    return refcount;
}

ULONG WINAPI hwnd_host_provider_Release(IRawElementProviderSimple *iface)
{
    struct hwnd_host_provider *host_prov = impl_from_hwnd_host_provider(iface);
    ULONG refcount = InterlockedDecrement(&host_prov->refcount);

    TRACE("%p, refcount %ld\n", iface, refcount);

    if (!refcount)
        free(host_prov);

    return refcount;
}

HRESULT WINAPI hwnd_host_provider_get_ProviderOptions(IRawElementProviderSimple *iface,
        enum ProviderOptions *ret_val)
{
    TRACE("%p, %p\n", iface, ret_val);
    *ret_val = ProviderOptions_ServerSideProvider;
    return S_OK;
}

HRESULT WINAPI hwnd_host_provider_GetPatternProvider(IRawElementProviderSimple *iface,
        PATTERNID pattern_id, IUnknown **ret_val)
{
    TRACE("%p, %d, %p\n", iface, pattern_id, ret_val);
    *ret_val = NULL;
    return S_OK;
}

HRESULT WINAPI hwnd_host_provider_GetPropertyValue(IRawElementProviderSimple *iface,
        PROPERTYID prop_id, VARIANT *ret_val)
{
    struct hwnd_host_provider *host_prov = impl_from_hwnd_host_provider(iface);

    TRACE("%p, %d, %p\n", iface, prop_id, ret_val);

    VariantInit(ret_val);
    switch (prop_id)
    {
    case UIA_NativeWindowHandlePropertyId:
        V_VT(ret_val) = VT_I4;
        V_I4(ret_val) = HandleToUlong(host_prov->hwnd);
        break;

    case UIA_ProviderDescriptionPropertyId:
        V_VT(ret_val) = VT_BSTR;
        V_BSTR(ret_val) = SysAllocString(L"Wine: HWND Provider Proxy");
        break;

    default:
        break;
    }

    return S_OK;
}

HRESULT WINAPI hwnd_host_provider_get_HostRawElementProvider(IRawElementProviderSimple *iface,
        IRawElementProviderSimple **ret_val)
{
    TRACE("%p, %p\n", iface, ret_val);
    *ret_val = NULL;
    return S_OK;
}

static const IRawElementProviderSimpleVtbl hwnd_host_provider_vtbl = {
    hwnd_host_provider_QueryInterface,
    hwnd_host_provider_AddRef,
    hwnd_host_provider_Release,
    hwnd_host_provider_get_ProviderOptions,
    hwnd_host_provider_GetPatternProvider,
    hwnd_host_provider_GetPropertyValue,
    hwnd_host_provider_get_HostRawElementProvider,
};

/***********************************************************************
 *          UiaClientsAreListening (uiautomationcore.@)
 */
BOOL WINAPI UiaClientsAreListening(void)
{
    TRACE("()\n");
    return TRUE;
}

/***********************************************************************
 *          UiaGetReservedMixedAttributeValue (uiautomationcore.@)
 */
HRESULT WINAPI UiaGetReservedMixedAttributeValue(IUnknown **value)
{
    TRACE("(%p)\n", value);

    if (!value)
        return E_INVALIDARG;

    *value = &uia_reserved_ma_iface;

    return S_OK;
}

/***********************************************************************
 *          UiaGetReservedNotSupportedValue (uiautomationcore.@)
 */
HRESULT WINAPI UiaGetReservedNotSupportedValue(IUnknown **value)
{
    TRACE("(%p)\n", value);

    if (!value)
        return E_INVALIDARG;

    *value = &uia_reserved_ns_iface;

    return S_OK;
}

/***********************************************************************
 *          UiaRaiseAutomationPropertyChangedEvent (uiautomationcore.@)
 */
HRESULT WINAPI UiaRaiseAutomationPropertyChangedEvent(IRawElementProviderSimple *provider, PROPERTYID id, VARIANT old, VARIANT new)
{
    FIXME("(%p, %d, %s, %s): stub\n", provider, id, debugstr_variant(&old), debugstr_variant(&new));
    return S_OK;
}

/***********************************************************************
 *          UiaRaiseStructureChangedEvent (uiautomationcore.@)
 */
HRESULT WINAPI UiaRaiseStructureChangedEvent(IRawElementProviderSimple *provider, enum StructureChangeType struct_change_type,
        int *runtime_id, int runtime_id_len)
{
    FIXME("(%p, %d, %p, %d): stub\n", provider, struct_change_type, runtime_id, runtime_id_len);
    return S_OK;
}

/***********************************************************************
 *          UiaRaiseAsyncContentLoadedEvent (uiautomationcore.@)
 */
HRESULT WINAPI UiaRaiseAsyncContentLoadedEvent(IRawElementProviderSimple *provider,
        enum AsyncContentLoadedState async_content_loaded_state, double percent_complete)
{
    FIXME("(%p, %d, %f): stub\n", provider, async_content_loaded_state, percent_complete);
    return S_OK;
}

/***********************************************************************
 *          UiaRaiseTextEditTextChangedEvent (uiautomationcore.@)
 */
HRESULT WINAPI UiaRaiseTextEditTextChangedEvent(IRawElementProviderSimple *provider,
        enum TextEditChangeType text_edit_change_type, SAFEARRAY *changed_data)
{
    FIXME("(%p, %d, %p): stub\n", provider, text_edit_change_type, changed_data);
    return S_OK;
}

/***********************************************************************
 *          UiaRaiseNotificationEvent (uiautomationcore.@)
 */
HRESULT WINAPI UiaRaiseNotificationEvent(IRawElementProviderSimple *provider, enum NotificationKind notification_kind,
        enum NotificationProcessing notification_processing, BSTR display_str, BSTR activity_id)
{
    FIXME("(%p, %d, %d, %s, %s): stub\n", provider, notification_kind, notification_processing,
            debugstr_w(display_str), debugstr_w(activity_id));
    return S_OK;
}

/***********************************************************************
 *          UiaRaiseChangesEvent (uiautomationcore.@)
 */
HRESULT WINAPI UiaRaiseChangesEvent(IRawElementProviderSimple *provider, int event_id_count,
        struct UiaChangeInfo *uia_changes)
{
    FIXME("(%p, %d, %p): stub\n", provider, event_id_count, uia_changes);
    return S_OK;
}

HRESULT WINAPI UiaHostProviderFromHwnd(HWND hwnd, IRawElementProviderSimple **provider)
{
    struct hwnd_host_provider *host_prov;

    TRACE("(%p, %p)\n", hwnd, provider);

    if (provider)
        *provider = NULL;

    if (!IsWindow(hwnd) || !provider)
        return E_INVALIDARG;

    host_prov = calloc(1, sizeof(*host_prov));
    if (!host_prov)
        return E_OUTOFMEMORY;

    host_prov->IRawElementProviderSimple_iface.lpVtbl = &hwnd_host_provider_vtbl;
    host_prov->refcount = 1;
    host_prov->hwnd = hwnd;
    *provider = &host_prov->IRawElementProviderSimple_iface;

    return S_OK;
}

/***********************************************************************
 *          DllMain (uiautomationcore.@)
 */
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, void *reserved)
{
    TRACE("(%p, %ld, %p)\n", hinst, reason, reserved);

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinst);
        huia_module = hinst;
        break;

    default:
        break;
    }

    return TRUE;
}

/* UIAutomation ClassFactory */
struct uia_cf {
    IClassFactory IClassFactory_iface;
    LONG ref;

    const GUID *clsid;
};

static struct uia_cf *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, struct uia_cf, IClassFactory_iface);
}

static HRESULT WINAPI uia_cf_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IClassFactory) || IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IClassFactory_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI uia_cf_AddRef(IClassFactory *iface)
{
    struct uia_cf *cf = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedIncrement(&cf->ref);

    TRACE("%p, refcount %ld\n", cf, ref);

    return ref;
}

static ULONG WINAPI uia_cf_Release(IClassFactory *iface)
{
    struct uia_cf *cf = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedDecrement(&cf->ref);

    TRACE("%p, refcount %ld\n", cf, ref);

    if (!ref)
        free(cf);

    return ref;
}

static HRESULT WINAPI uia_cf_CreateInstance(IClassFactory *iface, IUnknown *pouter, REFIID riid, void **ppv)
{
    struct uia_cf *cf = impl_from_IClassFactory(iface);
    IUnknown *obj = NULL;
    HRESULT hr;

    TRACE("%p, %p, %s, %p\n", iface, pouter, debugstr_guid(riid), ppv);

    *ppv = NULL;
    if (pouter)
        return CLASS_E_NOAGGREGATION;

    if (IsEqualGUID(cf->clsid, &CLSID_CUIAutomation))
        hr = create_uia_iface(&obj, FALSE);
    else if (IsEqualGUID(cf->clsid, &CLSID_CUIAutomation8))
        hr = create_uia_iface(&obj, TRUE);
    else
        return E_NOINTERFACE;

    if (SUCCEEDED(hr))
    {
        hr = IUnknown_QueryInterface(obj, riid, ppv);
        IUnknown_Release(obj);
    }

    return hr;
}

static HRESULT WINAPI uia_cf_LockServer(IClassFactory *iface, BOOL do_lock)
{
    FIXME("%p, %d: stub\n", iface, do_lock);
    return S_OK;
}

static const IClassFactoryVtbl uia_cf_Vtbl =
{
    uia_cf_QueryInterface,
    uia_cf_AddRef,
    uia_cf_Release,
    uia_cf_CreateInstance,
    uia_cf_LockServer
};

static inline HRESULT create_uia_cf(REFCLSID clsid, REFIID riid, void **ppv)
{
    struct uia_cf *cf = calloc(1, sizeof(*cf));
    HRESULT hr;

    *ppv = NULL;
    if (!cf)
        return E_OUTOFMEMORY;

    cf->IClassFactory_iface.lpVtbl = &uia_cf_Vtbl;
    cf->clsid = clsid;
    cf->ref = 1;

    hr = IClassFactory_QueryInterface(&cf->IClassFactory_iface, riid, ppv);
    IClassFactory_Release(&cf->IClassFactory_iface);

    return hr;
}

/***********************************************************************
 *          DllGetClassObject (uiautomationcore.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **ppv)
{
    TRACE("(%s, %s, %p)\n", debugstr_guid(clsid), debugstr_guid(riid), ppv);

    if (IsEqualGUID(clsid, &CLSID_CUIAutomation) || IsEqualGUID(clsid, &CLSID_CUIAutomation8))
        return create_uia_cf(clsid, riid, ppv);

    return CLASS_E_CLASSNOTAVAILABLE;
}
