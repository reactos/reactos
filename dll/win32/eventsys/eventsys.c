/*
 * Copyright (c) 2026 Terascale Functionalists
 * SPDX-License-Identifier: MIT
 *
 * Minimal COM+ Event System compatibility surface.
 */

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "oaidl.h"
#include "rpcproxy.h"

#include "wine/debug.h"

#include <initguid.h>

WINE_DEFAULT_DEBUG_CHANNEL(eventsys);

DEFINE_GUID(CLSID_CEventSystem, 0x4e14fba2, 0x2e22, 0x11d1, 0x99, 0x64, 0x00, 0xc0, 0x4f, 0xbb, 0xb3, 0x45);
DEFINE_GUID(CLSID_CEventSubscription, 0x7542e960, 0x79c7, 0x11d1, 0x88, 0xf9, 0x00, 0x80, 0xc7, 0xd7, 0x71, 0xbf);
DEFINE_GUID(IID_IEventSystem, 0x4e14fb9f, 0x2e22, 0x11d1, 0x99, 0x64, 0x00, 0xc0, 0x4f, 0xbb, 0xb3, 0x45);
DEFINE_GUID(IID_IEventSubscription, 0x4a6b0e15, 0x2e38, 0x11d1, 0x99, 0x65, 0x00, 0xc0, 0x4f, 0xbb, 0xb3, 0x45);
DEFINE_GUID(IID_IEnumEventObject, 0xf4a07d63, 0x2e25, 0x11d1, 0x99, 0x64, 0x00, 0xc0, 0x4f, 0xbb, 0xb3, 0x45);
DEFINE_GUID(IID_IEventObjectCollection, 0xf89ac270, 0xd4eb, 0x11d1, 0xb6, 0x82, 0x00, 0x80, 0x5f, 0xc7, 0x92, 0x16);

typedef struct IEventSystem IEventSystem;
typedef struct IEventSubscription IEventSubscription;
typedef struct IEnumEventObject IEnumEventObject;
typedef struct IEventObjectCollection IEventObjectCollection;

typedef struct IEventSystemVtbl
{
    HRESULT (WINAPI *QueryInterface)(IEventSystem *iface, REFIID riid, void **object);
    ULONG (WINAPI *AddRef)(IEventSystem *iface);
    ULONG (WINAPI *Release)(IEventSystem *iface);
    HRESULT (WINAPI *GetTypeInfoCount)(IEventSystem *iface, UINT *count);
    HRESULT (WINAPI *GetTypeInfo)(IEventSystem *iface, UINT index, LCID lcid, ITypeInfo **type_info);
    HRESULT (WINAPI *GetIDsOfNames)(IEventSystem *iface, REFIID riid, LPOLESTR *names, UINT name_count,
                                    LCID lcid, DISPID *dispids);
    HRESULT (WINAPI *Invoke)(IEventSystem *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                             DISPPARAMS *params, VARIANT *result, EXCEPINFO *exception, UINT *arg_error);
    HRESULT (WINAPI *Query)(IEventSystem *iface, BSTR progid, BSTR criteria, int *error_index,
                            IUnknown **object);
    HRESULT (WINAPI *Store)(IEventSystem *iface, BSTR progid, IUnknown *object);
    HRESULT (WINAPI *Remove)(IEventSystem *iface, BSTR progid, BSTR criteria, int *error_index);
    HRESULT (WINAPI *get_EventObjectChangeEventClassID)(IEventSystem *iface, BSTR *event_class_id);
    HRESULT (WINAPI *QueryS)(IEventSystem *iface, BSTR progid, BSTR criteria, IUnknown **object);
    HRESULT (WINAPI *RemoveS)(IEventSystem *iface, BSTR progid, BSTR criteria);
} IEventSystemVtbl;

struct IEventSystem
{
    const IEventSystemVtbl *lpVtbl;
};

typedef struct IEnumEventObjectVtbl
{
    HRESULT (WINAPI *QueryInterface)(IEnumEventObject *iface, REFIID riid, void **object);
    ULONG (WINAPI *AddRef)(IEnumEventObject *iface);
    ULONG (WINAPI *Release)(IEnumEventObject *iface);
    HRESULT (WINAPI *Clone)(IEnumEventObject *iface, IEnumEventObject **clone);
    HRESULT (WINAPI *Next)(IEnumEventObject *iface, ULONG requested_count, IUnknown **objects,
                           ULONG *returned_count);
    HRESULT (WINAPI *Reset)(IEnumEventObject *iface);
    HRESULT (WINAPI *Skip)(IEnumEventObject *iface, ULONG skip_count);
} IEnumEventObjectVtbl;

struct IEnumEventObject
{
    const IEnumEventObjectVtbl *lpVtbl;
};

typedef struct IEventObjectCollectionVtbl
{
    HRESULT (WINAPI *QueryInterface)(IEventObjectCollection *iface, REFIID riid, void **object);
    ULONG (WINAPI *AddRef)(IEventObjectCollection *iface);
    ULONG (WINAPI *Release)(IEventObjectCollection *iface);
    HRESULT (WINAPI *GetTypeInfoCount)(IEventObjectCollection *iface, UINT *count);
    HRESULT (WINAPI *GetTypeInfo)(IEventObjectCollection *iface, UINT index, LCID lcid, ITypeInfo **type_info);
    HRESULT (WINAPI *GetIDsOfNames)(IEventObjectCollection *iface, REFIID riid, LPOLESTR *names, UINT name_count,
                                    LCID lcid, DISPID *dispids);
    HRESULT (WINAPI *Invoke)(IEventObjectCollection *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                             DISPPARAMS *params, VARIANT *result, EXCEPINFO *exception, UINT *arg_error);
    HRESULT (WINAPI *get__NewEnum)(IEventObjectCollection *iface, IUnknown **enumerator);
    HRESULT (WINAPI *get_Item)(IEventObjectCollection *iface, BSTR object_id, VARIANT *item);
    HRESULT (WINAPI *get_NewEnum)(IEventObjectCollection *iface, IEnumEventObject **enumerator);
    HRESULT (WINAPI *get_Count)(IEventObjectCollection *iface, LONG *count);
    HRESULT (WINAPI *Add)(IEventObjectCollection *iface, VARIANT *item, BSTR object_id);
    HRESULT (WINAPI *Remove)(IEventObjectCollection *iface, BSTR object_id);
} IEventObjectCollectionVtbl;

struct IEventObjectCollection
{
    const IEventObjectCollectionVtbl *lpVtbl;
};

typedef struct IEventSubscriptionVtbl
{
    HRESULT (WINAPI *QueryInterface)(IEventSubscription *iface, REFIID riid, void **object);
    ULONG (WINAPI *AddRef)(IEventSubscription *iface);
    ULONG (WINAPI *Release)(IEventSubscription *iface);
    HRESULT (WINAPI *GetTypeInfoCount)(IEventSubscription *iface, UINT *count);
    HRESULT (WINAPI *GetTypeInfo)(IEventSubscription *iface, UINT index, LCID lcid, ITypeInfo **type_info);
    HRESULT (WINAPI *GetIDsOfNames)(IEventSubscription *iface, REFIID riid, LPOLESTR *names, UINT name_count,
                                    LCID lcid, DISPID *dispids);
    HRESULT (WINAPI *Invoke)(IEventSubscription *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                             DISPPARAMS *params, VARIANT *result, EXCEPINFO *exception, UINT *arg_error);
    HRESULT (WINAPI *get_SubscriptionID)(IEventSubscription *iface, BSTR *value);
    HRESULT (WINAPI *put_SubscriptionID)(IEventSubscription *iface, BSTR value);
    HRESULT (WINAPI *get_SubscriptionName)(IEventSubscription *iface, BSTR *value);
    HRESULT (WINAPI *put_SubscriptionName)(IEventSubscription *iface, BSTR value);
    HRESULT (WINAPI *get_PublisherID)(IEventSubscription *iface, BSTR *value);
    HRESULT (WINAPI *put_PublisherID)(IEventSubscription *iface, BSTR value);
    HRESULT (WINAPI *get_EventClassID)(IEventSubscription *iface, BSTR *value);
    HRESULT (WINAPI *put_EventClassID)(IEventSubscription *iface, BSTR value);
    HRESULT (WINAPI *get_MethodName)(IEventSubscription *iface, BSTR *value);
    HRESULT (WINAPI *put_MethodName)(IEventSubscription *iface, BSTR value);
    HRESULT (WINAPI *get_SubscriberCLSID)(IEventSubscription *iface, BSTR *value);
    HRESULT (WINAPI *put_SubscriberCLSID)(IEventSubscription *iface, BSTR value);
    HRESULT (WINAPI *get_SubscriberInterface)(IEventSubscription *iface, IUnknown **value);
    HRESULT (WINAPI *put_SubscriberInterface)(IEventSubscription *iface, IUnknown *value);
    HRESULT (WINAPI *get_PerUser)(IEventSubscription *iface, BOOL *value);
    HRESULT (WINAPI *put_PerUser)(IEventSubscription *iface, BOOL value);
    HRESULT (WINAPI *get_OwnerSID)(IEventSubscription *iface, BSTR *value);
    HRESULT (WINAPI *put_OwnerSID)(IEventSubscription *iface, BSTR value);
    HRESULT (WINAPI *get_Enabled)(IEventSubscription *iface, BOOL *value);
    HRESULT (WINAPI *put_Enabled)(IEventSubscription *iface, BOOL value);
    HRESULT (WINAPI *get_Description)(IEventSubscription *iface, BSTR *value);
    HRESULT (WINAPI *put_Description)(IEventSubscription *iface, BSTR value);
    HRESULT (WINAPI *get_MachineName)(IEventSubscription *iface, BSTR *value);
    HRESULT (WINAPI *put_MachineName)(IEventSubscription *iface, BSTR value);
    HRESULT (WINAPI *GetPublisherProperty)(IEventSubscription *iface, BSTR name, VARIANT *value);
    HRESULT (WINAPI *PutPublisherProperty)(IEventSubscription *iface, BSTR name, VARIANT *value);
    HRESULT (WINAPI *RemovePublisherProperty)(IEventSubscription *iface, BSTR name);
    HRESULT (WINAPI *GetPublisherPropertyCollection)(IEventSubscription *iface, IEventObjectCollection **collection);
    HRESULT (WINAPI *GetSubscriberProperty)(IEventSubscription *iface, BSTR name, VARIANT *value);
    HRESULT (WINAPI *PutSubscriberProperty)(IEventSubscription *iface, BSTR name, VARIANT *value);
    HRESULT (WINAPI *RemoveSubscriberProperty)(IEventSubscription *iface, BSTR name);
    HRESULT (WINAPI *GetSubscriberPropertyCollection)(IEventSubscription *iface, IEventObjectCollection **collection);
    HRESULT (WINAPI *get_InterfaceID)(IEventSubscription *iface, BSTR *value);
    HRESULT (WINAPI *put_InterfaceID)(IEventSubscription *iface, BSTR value);
} IEventSubscriptionVtbl;

struct IEventSubscription
{
    const IEventSubscriptionVtbl *lpVtbl;
};

struct event_system
{
    IEventSystem IEventSystem_iface;
    LONG refs;
};

struct event_subscription
{
    IEventSubscription IEventSubscription_iface;
    LONG refs;
    BSTR subscription_id;
    BSTR subscription_name;
    BSTR publisher_id;
    BSTR event_class_id;
    BSTR method_name;
    BSTR subscriber_clsid;
    IUnknown *subscriber_interface;
    BOOL per_user;
    BSTR owner_sid;
    BOOL enabled;
    BSTR description;
    BSTR machine_name;
    BSTR interface_id;
};

struct event_enum
{
    IEnumEventObject IEnumEventObject_iface;
    LONG refs;
};

struct event_collection
{
    IEventObjectCollection IEventObjectCollection_iface;
    LONG refs;
};

struct eventsys_class_factory
{
    IClassFactory IClassFactory_iface;
    HRESULT (*create)(REFIID riid, void **object);
};

static HINSTANCE eventsys_instance;
static LONG module_refs;
static LONG server_locks;

static inline struct event_system *impl_from_IEventSystem(IEventSystem *iface)
{
    return CONTAINING_RECORD(iface, struct event_system, IEventSystem_iface);
}

static inline struct event_subscription *impl_from_IEventSubscription(IEventSubscription *iface)
{
    return CONTAINING_RECORD(iface, struct event_subscription, IEventSubscription_iface);
}

static inline struct event_enum *impl_from_IEnumEventObject(IEnumEventObject *iface)
{
    return CONTAINING_RECORD(iface, struct event_enum, IEnumEventObject_iface);
}

static inline struct event_collection *impl_from_IEventObjectCollection(IEventObjectCollection *iface)
{
    return CONTAINING_RECORD(iface, struct event_collection, IEventObjectCollection_iface);
}

static inline struct eventsys_class_factory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, struct eventsys_class_factory, IClassFactory_iface);
}

static void *eventsys_alloc_zero(SIZE_T size)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

static void eventsys_free(void *memory)
{
    HeapFree(GetProcessHeap(), 0, memory);
}

static HRESULT copy_bstr(BSTR source, BSTR *dest)
{
    if (!dest)
        return E_POINTER;

    *dest = NULL;
    if (!source)
        return S_OK;

    *dest = SysAllocString(source);
    return *dest ? S_OK : E_OUTOFMEMORY;
}

static HRESULT set_bstr(BSTR *slot, BSTR value)
{
    BSTR copy = NULL;

    if (value)
    {
        copy = SysAllocString(value);
        if (!copy)
            return E_OUTOFMEMORY;
    }

    SysFreeString(*slot);
    *slot = copy;
    return S_OK;
}

static HRESULT dispatch_GetTypeInfoCount(UINT *count)
{
    if (!count)
        return E_POINTER;

    *count = 0;
    return S_OK;
}

static HRESULT dispatch_GetTypeInfo(UINT index, LCID lcid, ITypeInfo **type_info)
{
    (void)index;
    (void)lcid;

    if (!type_info)
        return E_POINTER;

    *type_info = NULL;
    return DISP_E_BADINDEX;
}

static HRESULT dispatch_GetIDsOfNames(REFIID riid, LPOLESTR *names, UINT name_count,
                                      LCID lcid, DISPID *dispids)
{
    UINT i;

    (void)riid;
    (void)names;
    (void)lcid;

    if (!dispids)
        return E_POINTER;

    for (i = 0; i < name_count; ++i)
        dispids[i] = DISPID_UNKNOWN;

    return DISP_E_UNKNOWNNAME;
}

static HRESULT dispatch_Invoke(DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                               DISPPARAMS *params, VARIANT *result, EXCEPINFO *exception,
                               UINT *arg_error)
{
    (void)dispid;
    (void)riid;
    (void)lcid;
    (void)flags;
    (void)params;
    (void)exception;

    if (result)
        VariantInit(result);
    if (arg_error)
        *arg_error = 0;

    return DISP_E_MEMBERNOTFOUND;
}

static HRESULT create_event_collection(IEventObjectCollection **collection);

static HRESULT WINAPI event_enum_QueryInterface(IEnumEventObject *iface, REFIID riid, void **object)
{
    if (!object)
        return E_POINTER;

    *object = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IEnumEventObject))
        *object = iface;

    if (!*object)
        return E_NOINTERFACE;

    iface->lpVtbl->AddRef(iface);
    return S_OK;
}

static ULONG WINAPI event_enum_AddRef(IEnumEventObject *iface)
{
    struct event_enum *enumerator = impl_from_IEnumEventObject(iface);
    return InterlockedIncrement(&enumerator->refs);
}

static ULONG WINAPI event_enum_Release(IEnumEventObject *iface)
{
    struct event_enum *enumerator = impl_from_IEnumEventObject(iface);
    ULONG refs = InterlockedDecrement(&enumerator->refs);

    if (!refs)
    {
        InterlockedDecrement(&module_refs);
        eventsys_free(enumerator);
    }

    return refs;
}

static HRESULT create_event_enum(IEnumEventObject **enumerator);

static HRESULT WINAPI event_enum_Clone(IEnumEventObject *iface, IEnumEventObject **clone)
{
    (void)iface;

    if (!clone)
        return E_POINTER;

    return create_event_enum(clone);
}

static HRESULT WINAPI event_enum_Next(IEnumEventObject *iface, ULONG requested_count,
                                      IUnknown **objects, ULONG *returned_count)
{
    ULONG i;

    (void)iface;

    if (returned_count)
        *returned_count = 0;

    if (requested_count && !objects)
        return E_POINTER;

    for (i = 0; i < requested_count; ++i)
        objects[i] = NULL;

    return S_FALSE;
}

static HRESULT WINAPI event_enum_Reset(IEnumEventObject *iface)
{
    (void)iface;
    return S_OK;
}

static HRESULT WINAPI event_enum_Skip(IEnumEventObject *iface, ULONG skip_count)
{
    (void)iface;
    return skip_count ? S_FALSE : S_OK;
}

static const IEnumEventObjectVtbl event_enum_vtbl =
{
    event_enum_QueryInterface,
    event_enum_AddRef,
    event_enum_Release,
    event_enum_Clone,
    event_enum_Next,
    event_enum_Reset,
    event_enum_Skip
};

static HRESULT create_event_enum(IEnumEventObject **enumerator)
{
    struct event_enum *object;

    if (!enumerator)
        return E_POINTER;

    *enumerator = NULL;

    object = eventsys_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IEnumEventObject_iface.lpVtbl = &event_enum_vtbl;
    object->refs = 1;
    InterlockedIncrement(&module_refs);
    *enumerator = &object->IEnumEventObject_iface;
    return S_OK;
}

static HRESULT WINAPI event_collection_QueryInterface(IEventObjectCollection *iface, REFIID riid, void **object)
{
    if (!object)
        return E_POINTER;

    *object = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IEventObjectCollection))
        *object = iface;

    if (!*object)
        return E_NOINTERFACE;

    iface->lpVtbl->AddRef(iface);
    return S_OK;
}

static ULONG WINAPI event_collection_AddRef(IEventObjectCollection *iface)
{
    struct event_collection *collection = impl_from_IEventObjectCollection(iface);
    return InterlockedIncrement(&collection->refs);
}

static ULONG WINAPI event_collection_Release(IEventObjectCollection *iface)
{
    struct event_collection *collection = impl_from_IEventObjectCollection(iface);
    ULONG refs = InterlockedDecrement(&collection->refs);

    if (!refs)
    {
        InterlockedDecrement(&module_refs);
        eventsys_free(collection);
    }

    return refs;
}

static HRESULT WINAPI event_collection_GetTypeInfoCount(IEventObjectCollection *iface, UINT *count)
{
    (void)iface;
    return dispatch_GetTypeInfoCount(count);
}

static HRESULT WINAPI event_collection_GetTypeInfo(IEventObjectCollection *iface, UINT index, LCID lcid,
                                                   ITypeInfo **type_info)
{
    (void)iface;
    return dispatch_GetTypeInfo(index, lcid, type_info);
}

static HRESULT WINAPI event_collection_GetIDsOfNames(IEventObjectCollection *iface, REFIID riid, LPOLESTR *names,
                                                     UINT name_count, LCID lcid, DISPID *dispids)
{
    (void)iface;
    return dispatch_GetIDsOfNames(riid, names, name_count, lcid, dispids);
}

static HRESULT WINAPI event_collection_Invoke(IEventObjectCollection *iface, DISPID dispid, REFIID riid, LCID lcid,
                                              WORD flags, DISPPARAMS *params, VARIANT *result,
                                              EXCEPINFO *exception, UINT *arg_error)
{
    (void)iface;
    return dispatch_Invoke(dispid, riid, lcid, flags, params, result, exception, arg_error);
}

static HRESULT WINAPI event_collection_get__NewEnum(IEventObjectCollection *iface, IUnknown **enumerator)
{
    IEnumEventObject *event_enum;
    HRESULT hr;

    (void)iface;

    if (!enumerator)
        return E_POINTER;

    *enumerator = NULL;

    hr = create_event_enum(&event_enum);
    if (FAILED(hr))
        return hr;

    *enumerator = (IUnknown *)event_enum;
    return S_OK;
}

static HRESULT WINAPI event_collection_get_Item(IEventObjectCollection *iface, BSTR object_id, VARIANT *item)
{
    (void)iface;
    (void)object_id;

    if (!item)
        return E_POINTER;

    VariantInit(item);
    return S_FALSE;
}

static HRESULT WINAPI event_collection_get_NewEnum(IEventObjectCollection *iface, IEnumEventObject **enumerator)
{
    (void)iface;
    return create_event_enum(enumerator);
}

static HRESULT WINAPI event_collection_get_Count(IEventObjectCollection *iface, LONG *count)
{
    (void)iface;

    if (!count)
        return E_POINTER;

    *count = 0;
    return S_OK;
}

static HRESULT WINAPI event_collection_Add(IEventObjectCollection *iface, VARIANT *item, BSTR object_id)
{
    (void)iface;
    (void)item;
    (void)object_id;
    return S_OK;
}

static HRESULT WINAPI event_collection_Remove(IEventObjectCollection *iface, BSTR object_id)
{
    (void)iface;
    (void)object_id;
    return S_OK;
}

static const IEventObjectCollectionVtbl event_collection_vtbl =
{
    event_collection_QueryInterface,
    event_collection_AddRef,
    event_collection_Release,
    event_collection_GetTypeInfoCount,
    event_collection_GetTypeInfo,
    event_collection_GetIDsOfNames,
    event_collection_Invoke,
    event_collection_get__NewEnum,
    event_collection_get_Item,
    event_collection_get_NewEnum,
    event_collection_get_Count,
    event_collection_Add,
    event_collection_Remove
};

static HRESULT create_event_collection(IEventObjectCollection **collection)
{
    struct event_collection *object;

    if (!collection)
        return E_POINTER;

    *collection = NULL;

    object = eventsys_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IEventObjectCollection_iface.lpVtbl = &event_collection_vtbl;
    object->refs = 1;
    InterlockedIncrement(&module_refs);
    *collection = &object->IEventObjectCollection_iface;
    return S_OK;
}

static HRESULT WINAPI event_system_QueryInterface(IEventSystem *iface, REFIID riid, void **object)
{
    if (!object)
        return E_POINTER;

    *object = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IEventSystem))
        *object = iface;

    if (!*object)
        return E_NOINTERFACE;

    iface->lpVtbl->AddRef(iface);
    return S_OK;
}

static ULONG WINAPI event_system_AddRef(IEventSystem *iface)
{
    struct event_system *system = impl_from_IEventSystem(iface);
    return InterlockedIncrement(&system->refs);
}

static ULONG WINAPI event_system_Release(IEventSystem *iface)
{
    struct event_system *system = impl_from_IEventSystem(iface);
    ULONG refs = InterlockedDecrement(&system->refs);

    if (!refs)
    {
        InterlockedDecrement(&module_refs);
        eventsys_free(system);
    }

    return refs;
}

static HRESULT WINAPI event_system_GetTypeInfoCount(IEventSystem *iface, UINT *count)
{
    (void)iface;
    return dispatch_GetTypeInfoCount(count);
}

static HRESULT WINAPI event_system_GetTypeInfo(IEventSystem *iface, UINT index, LCID lcid, ITypeInfo **type_info)
{
    (void)iface;
    return dispatch_GetTypeInfo(index, lcid, type_info);
}

static HRESULT WINAPI event_system_GetIDsOfNames(IEventSystem *iface, REFIID riid, LPOLESTR *names,
                                                 UINT name_count, LCID lcid, DISPID *dispids)
{
    (void)iface;
    return dispatch_GetIDsOfNames(riid, names, name_count, lcid, dispids);
}

static HRESULT WINAPI event_system_Invoke(IEventSystem *iface, DISPID dispid, REFIID riid, LCID lcid,
                                          WORD flags, DISPPARAMS *params, VARIANT *result,
                                          EXCEPINFO *exception, UINT *arg_error)
{
    (void)iface;
    return dispatch_Invoke(dispid, riid, lcid, flags, params, result, exception, arg_error);
}

static HRESULT WINAPI event_system_Query(IEventSystem *iface, BSTR progid, BSTR criteria, int *error_index,
                                         IUnknown **object)
{
    IEventObjectCollection *collection;
    HRESULT hr;

    (void)iface;
    (void)progid;
    (void)criteria;

    if (!object)
        return E_POINTER;

    if (error_index)
        *error_index = -1;

    *object = NULL;
    hr = create_event_collection(&collection);
    if (FAILED(hr))
        return hr;

    *object = (IUnknown *)collection;
    return S_OK;
}

static HRESULT WINAPI event_system_Store(IEventSystem *iface, BSTR progid, IUnknown *object)
{
    (void)iface;
    (void)progid;
    (void)object;
    return S_OK;
}

static HRESULT WINAPI event_system_Remove(IEventSystem *iface, BSTR progid, BSTR criteria, int *error_index)
{
    (void)iface;
    (void)progid;
    (void)criteria;

    if (error_index)
        *error_index = -1;

    return S_OK;
}

static HRESULT WINAPI event_system_get_EventObjectChangeEventClassID(IEventSystem *iface, BSTR *event_class_id)
{
    (void)iface;

    if (!event_class_id)
        return E_POINTER;

    *event_class_id = SysAllocStringLen(NULL, 0);
    return *event_class_id ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI event_system_QueryS(IEventSystem *iface, BSTR progid, BSTR criteria, IUnknown **object)
{
    (void)iface;
    (void)progid;
    (void)criteria;

    if (!object)
        return E_POINTER;

    *object = NULL;
    return S_FALSE;
}

static HRESULT WINAPI event_system_RemoveS(IEventSystem *iface, BSTR progid, BSTR criteria)
{
    (void)iface;
    (void)progid;
    (void)criteria;
    return S_OK;
}

static const IEventSystemVtbl event_system_vtbl =
{
    event_system_QueryInterface,
    event_system_AddRef,
    event_system_Release,
    event_system_GetTypeInfoCount,
    event_system_GetTypeInfo,
    event_system_GetIDsOfNames,
    event_system_Invoke,
    event_system_Query,
    event_system_Store,
    event_system_Remove,
    event_system_get_EventObjectChangeEventClassID,
    event_system_QueryS,
    event_system_RemoveS
};

static HRESULT create_event_system(REFIID riid, void **object)
{
    struct event_system *system;
    HRESULT hr;

    if (!object)
        return E_POINTER;

    *object = NULL;

    system = eventsys_alloc_zero(sizeof(*system));
    if (!system)
        return E_OUTOFMEMORY;

    system->IEventSystem_iface.lpVtbl = &event_system_vtbl;
    system->refs = 1;
    InterlockedIncrement(&module_refs);

    hr = event_system_QueryInterface(&system->IEventSystem_iface, riid, object);
    event_system_Release(&system->IEventSystem_iface);
    return hr;
}

static HRESULT WINAPI event_subscription_QueryInterface(IEventSubscription *iface, REFIID riid, void **object)
{
    if (!object)
        return E_POINTER;

    *object = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IEventSubscription))
        *object = iface;

    if (!*object)
        return E_NOINTERFACE;

    iface->lpVtbl->AddRef(iface);
    return S_OK;
}

static ULONG WINAPI event_subscription_AddRef(IEventSubscription *iface)
{
    struct event_subscription *subscription = impl_from_IEventSubscription(iface);
    return InterlockedIncrement(&subscription->refs);
}

static ULONG WINAPI event_subscription_Release(IEventSubscription *iface)
{
    struct event_subscription *subscription = impl_from_IEventSubscription(iface);
    ULONG refs = InterlockedDecrement(&subscription->refs);

    if (!refs)
    {
        SysFreeString(subscription->subscription_id);
        SysFreeString(subscription->subscription_name);
        SysFreeString(subscription->publisher_id);
        SysFreeString(subscription->event_class_id);
        SysFreeString(subscription->method_name);
        SysFreeString(subscription->subscriber_clsid);
        if (subscription->subscriber_interface)
            IUnknown_Release(subscription->subscriber_interface);
        SysFreeString(subscription->owner_sid);
        SysFreeString(subscription->description);
        SysFreeString(subscription->machine_name);
        SysFreeString(subscription->interface_id);
        InterlockedDecrement(&module_refs);
        eventsys_free(subscription);
    }

    return refs;
}

static HRESULT WINAPI event_subscription_GetTypeInfoCount(IEventSubscription *iface, UINT *count)
{
    (void)iface;
    return dispatch_GetTypeInfoCount(count);
}

static HRESULT WINAPI event_subscription_GetTypeInfo(IEventSubscription *iface, UINT index, LCID lcid,
                                                     ITypeInfo **type_info)
{
    (void)iface;
    return dispatch_GetTypeInfo(index, lcid, type_info);
}

static HRESULT WINAPI event_subscription_GetIDsOfNames(IEventSubscription *iface, REFIID riid,
                                                       LPOLESTR *names, UINT name_count, LCID lcid,
                                                       DISPID *dispids)
{
    (void)iface;
    return dispatch_GetIDsOfNames(riid, names, name_count, lcid, dispids);
}

static HRESULT WINAPI event_subscription_Invoke(IEventSubscription *iface, DISPID dispid, REFIID riid,
                                                LCID lcid, WORD flags, DISPPARAMS *params, VARIANT *result,
                                                EXCEPINFO *exception, UINT *arg_error)
{
    (void)iface;
    return dispatch_Invoke(dispid, riid, lcid, flags, params, result, exception, arg_error);
}

#define SUBSCRIPTION_BSTR_ACCESSORS(name, field)                                               \
static HRESULT WINAPI event_subscription_get_##name(IEventSubscription *iface, BSTR *value)     \
{                                                                                               \
    struct event_subscription *subscription = impl_from_IEventSubscription(iface);               \
    return copy_bstr(subscription->field, value);                                                \
}                                                                                               \
                                                                                                \
static HRESULT WINAPI event_subscription_put_##name(IEventSubscription *iface, BSTR value)       \
{                                                                                               \
    struct event_subscription *subscription = impl_from_IEventSubscription(iface);               \
    return set_bstr(&subscription->field, value);                                                \
}

SUBSCRIPTION_BSTR_ACCESSORS(SubscriptionID, subscription_id)
SUBSCRIPTION_BSTR_ACCESSORS(SubscriptionName, subscription_name)
SUBSCRIPTION_BSTR_ACCESSORS(PublisherID, publisher_id)
SUBSCRIPTION_BSTR_ACCESSORS(EventClassID, event_class_id)
SUBSCRIPTION_BSTR_ACCESSORS(MethodName, method_name)
SUBSCRIPTION_BSTR_ACCESSORS(SubscriberCLSID, subscriber_clsid)
SUBSCRIPTION_BSTR_ACCESSORS(OwnerSID, owner_sid)
SUBSCRIPTION_BSTR_ACCESSORS(Description, description)
SUBSCRIPTION_BSTR_ACCESSORS(MachineName, machine_name)
SUBSCRIPTION_BSTR_ACCESSORS(InterfaceID, interface_id)

static HRESULT WINAPI event_subscription_get_SubscriberInterface(IEventSubscription *iface, IUnknown **value)
{
    struct event_subscription *subscription = impl_from_IEventSubscription(iface);

    if (!value)
        return E_POINTER;

    *value = subscription->subscriber_interface;
    if (*value)
        IUnknown_AddRef(*value);

    return S_OK;
}

static HRESULT WINAPI event_subscription_put_SubscriberInterface(IEventSubscription *iface, IUnknown *value)
{
    struct event_subscription *subscription = impl_from_IEventSubscription(iface);

    if (value)
        IUnknown_AddRef(value);
    if (subscription->subscriber_interface)
        IUnknown_Release(subscription->subscriber_interface);

    subscription->subscriber_interface = value;
    return S_OK;
}

static HRESULT WINAPI event_subscription_get_PerUser(IEventSubscription *iface, BOOL *value)
{
    struct event_subscription *subscription = impl_from_IEventSubscription(iface);

    if (!value)
        return E_POINTER;

    *value = subscription->per_user;
    return S_OK;
}

static HRESULT WINAPI event_subscription_put_PerUser(IEventSubscription *iface, BOOL value)
{
    struct event_subscription *subscription = impl_from_IEventSubscription(iface);
    subscription->per_user = !!value;
    return S_OK;
}

static HRESULT WINAPI event_subscription_get_Enabled(IEventSubscription *iface, BOOL *value)
{
    struct event_subscription *subscription = impl_from_IEventSubscription(iface);

    if (!value)
        return E_POINTER;

    *value = subscription->enabled;
    return S_OK;
}

static HRESULT WINAPI event_subscription_put_Enabled(IEventSubscription *iface, BOOL value)
{
    struct event_subscription *subscription = impl_from_IEventSubscription(iface);
    subscription->enabled = !!value;
    return S_OK;
}

static HRESULT WINAPI event_subscription_GetProperty(IEventSubscription *iface, BSTR name, VARIANT *value)
{
    (void)iface;
    (void)name;

    if (!value)
        return E_POINTER;

    VariantInit(value);
    return S_OK;
}

static HRESULT WINAPI event_subscription_PutProperty(IEventSubscription *iface, BSTR name, VARIANT *value)
{
    (void)iface;
    (void)name;
    (void)value;
    return S_OK;
}

static HRESULT WINAPI event_subscription_RemoveProperty(IEventSubscription *iface, BSTR name)
{
    (void)iface;
    (void)name;
    return S_OK;
}

static HRESULT WINAPI event_subscription_GetPropertyCollection(IEventSubscription *iface,
                                                               IEventObjectCollection **collection)
{
    (void)iface;
    return create_event_collection(collection);
}

static const IEventSubscriptionVtbl event_subscription_vtbl =
{
    event_subscription_QueryInterface,
    event_subscription_AddRef,
    event_subscription_Release,
    event_subscription_GetTypeInfoCount,
    event_subscription_GetTypeInfo,
    event_subscription_GetIDsOfNames,
    event_subscription_Invoke,
    event_subscription_get_SubscriptionID,
    event_subscription_put_SubscriptionID,
    event_subscription_get_SubscriptionName,
    event_subscription_put_SubscriptionName,
    event_subscription_get_PublisherID,
    event_subscription_put_PublisherID,
    event_subscription_get_EventClassID,
    event_subscription_put_EventClassID,
    event_subscription_get_MethodName,
    event_subscription_put_MethodName,
    event_subscription_get_SubscriberCLSID,
    event_subscription_put_SubscriberCLSID,
    event_subscription_get_SubscriberInterface,
    event_subscription_put_SubscriberInterface,
    event_subscription_get_PerUser,
    event_subscription_put_PerUser,
    event_subscription_get_OwnerSID,
    event_subscription_put_OwnerSID,
    event_subscription_get_Enabled,
    event_subscription_put_Enabled,
    event_subscription_get_Description,
    event_subscription_put_Description,
    event_subscription_get_MachineName,
    event_subscription_put_MachineName,
    event_subscription_GetProperty,
    event_subscription_PutProperty,
    event_subscription_RemoveProperty,
    event_subscription_GetPropertyCollection,
    event_subscription_GetProperty,
    event_subscription_PutProperty,
    event_subscription_RemoveProperty,
    event_subscription_GetPropertyCollection,
    event_subscription_get_InterfaceID,
    event_subscription_put_InterfaceID
};

static HRESULT create_event_subscription(REFIID riid, void **object)
{
    struct event_subscription *subscription;
    HRESULT hr;

    if (!object)
        return E_POINTER;

    *object = NULL;

    subscription = eventsys_alloc_zero(sizeof(*subscription));
    if (!subscription)
        return E_OUTOFMEMORY;

    subscription->IEventSubscription_iface.lpVtbl = &event_subscription_vtbl;
    subscription->refs = 1;
    subscription->enabled = TRUE;
    InterlockedIncrement(&module_refs);

    hr = event_subscription_QueryInterface(&subscription->IEventSubscription_iface, riid, object);
    event_subscription_Release(&subscription->IEventSubscription_iface);
    return hr;
}

static HRESULT WINAPI eventsys_cf_QueryInterface(IClassFactory *iface, REFIID riid, void **object)
{
    if (!object)
        return E_POINTER;

    *object = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IClassFactory))
        *object = iface;

    if (!*object)
        return E_NOINTERFACE;

    IClassFactory_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI eventsys_cf_AddRef(IClassFactory *iface)
{
    (void)iface;
    return 2;
}

static ULONG WINAPI eventsys_cf_Release(IClassFactory *iface)
{
    (void)iface;
    return 1;
}

static HRESULT WINAPI eventsys_cf_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **object)
{
    struct eventsys_class_factory *factory = impl_from_IClassFactory(iface);

    if (!object)
        return E_POINTER;

    *object = NULL;

    if (outer)
        return CLASS_E_NOAGGREGATION;

    return factory->create(riid, object);
}

static HRESULT WINAPI eventsys_cf_LockServer(IClassFactory *iface, BOOL lock)
{
    (void)iface;

    if (lock)
        InterlockedIncrement(&server_locks);
    else
        InterlockedDecrement(&server_locks);

    return S_OK;
}

static const IClassFactoryVtbl eventsys_cf_vtbl =
{
    eventsys_cf_QueryInterface,
    eventsys_cf_AddRef,
    eventsys_cf_Release,
    eventsys_cf_CreateInstance,
    eventsys_cf_LockServer
};

static struct eventsys_class_factory event_system_cf =
{
    { &eventsys_cf_vtbl },
    create_event_system
};

static struct eventsys_class_factory event_subscription_cf =
{
    { &eventsys_cf_vtbl },
    create_event_subscription
};

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    (void)reserved;

    if (reason == DLL_PROCESS_ATTACH)
    {
        eventsys_instance = instance;
        DisableThreadLibraryCalls(instance);
    }

    return TRUE;
}

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **object)
{
    if (!object)
        return E_POINTER;

    *object = NULL;

    if (IsEqualGUID(clsid, &CLSID_CEventSystem))
        return IClassFactory_QueryInterface(&event_system_cf.IClassFactory_iface, riid, object);

    if (IsEqualGUID(clsid, &CLSID_CEventSubscription))
        return IClassFactory_QueryInterface(&event_subscription_cf.IClassFactory_iface, riid, object);

    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    return module_refs || server_locks ? S_FALSE : S_OK;
}

HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources(eventsys_instance);
}

HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources(eventsys_instance);
}
