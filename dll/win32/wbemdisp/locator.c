/*
 * Copyright 2013 Hans Leidekker for CodeWeavers
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "initguid.h"
#include "objbase.h"
#include "wmiutils.h"
#include "wbemcli.h"
#include "wbemdisp.h"

#include "wine/debug.h"
#include "wbemdisp_private.h"
#include "wbemdisp_classes.h"

WINE_DEFAULT_DEBUG_CHANNEL(wbemdisp);

struct namedvalueset
{
    ISWbemNamedValueSet ISWbemNamedValueSet_iface;
    LONG refs;

    IWbemContext *context;
};

static struct namedvalueset *unsafe_valueset_impl_from_IDispatch(IDispatch *iface);

static IWbemContext * unsafe_get_context_from_namedvalueset( IDispatch *disp )
{
    struct namedvalueset *valueset = unsafe_valueset_impl_from_IDispatch( disp );
    return valueset ? valueset->context : NULL;
}

struct services;

static HRESULT EnumVARIANT_create( struct services *, IEnumWbemClassObject *, IEnumVARIANT ** );
static HRESULT ISWbemSecurity_create( ISWbemSecurity ** );
static HRESULT SWbemObject_create( struct services *, IWbemClassObject *, ISWbemObject ** );

enum type_id
{
    ISWbemLocator_tid,
    ISWbemObject_tid,
    ISWbemObjectSet_tid,
    ISWbemProperty_tid,
    ISWbemPropertySet_tid,
    ISWbemServices_tid,
    ISWbemSecurity_tid,
    ISWbemNamedValueSet_tid,
    ISWbemNamedValue_tid,
    ISWbemMethodSet_tid,
    ISWbemMethod_tid,
    last_tid
};

static ITypeLib *wbemdisp_typelib;
static ITypeInfo *wbemdisp_typeinfo[last_tid];

static REFIID wbemdisp_tid_id[] =
{
    &IID_ISWbemLocator,
    &IID_ISWbemObject,
    &IID_ISWbemObjectSet,
    &IID_ISWbemProperty,
    &IID_ISWbemPropertySet,
    &IID_ISWbemServices,
    &IID_ISWbemSecurity,
    &IID_ISWbemNamedValueSet,
    &IID_ISWbemNamedValue,
    &IID_ISWbemMethodSet,
    &IID_ISWbemMethod,
};

static HRESULT get_typeinfo( enum type_id tid, ITypeInfo **ret )
{
    HRESULT hr;

    if (!wbemdisp_typelib)
    {
        ITypeLib *typelib;

        hr = LoadRegTypeLib( &LIBID_WbemScripting, 1, 2, LOCALE_SYSTEM_DEFAULT, &typelib );
        if (FAILED( hr ))
        {
            ERR( "LoadRegTypeLib failed: %#lx\n", hr );
            return hr;
        }
        if (InterlockedCompareExchangePointer( (void **)&wbemdisp_typelib, typelib, NULL ))
            ITypeLib_Release( typelib );
    }
    if (!wbemdisp_typeinfo[tid])
    {
        ITypeInfo *typeinfo;

        hr = ITypeLib_GetTypeInfoOfGuid( wbemdisp_typelib, wbemdisp_tid_id[tid], &typeinfo );
        if (FAILED( hr ))
        {
            ERR( "GetTypeInfoOfGuid(%s) failed: %#lx\n", debugstr_guid(wbemdisp_tid_id[tid]), hr );
            return hr;
        }
        if (InterlockedCompareExchangePointer( (void **)(wbemdisp_typeinfo + tid), typeinfo, NULL ))
            ITypeInfo_Release( typeinfo );
    }
    *ret = wbemdisp_typeinfo[tid];
    ITypeInfo_AddRef( *ret );
    return S_OK;
}

struct property
{
    ISWbemProperty ISWbemProperty_iface;
    LONG refs;
    IWbemClassObject *object;
    BSTR name;
};

static inline struct property *impl_from_ISWbemProperty( ISWbemProperty *iface )
{
    return CONTAINING_RECORD( iface, struct property, ISWbemProperty_iface );
}

static ULONG WINAPI property_AddRef( ISWbemProperty *iface )
{
    struct property *property = impl_from_ISWbemProperty( iface );
    return InterlockedIncrement( &property->refs );
}

static ULONG WINAPI property_Release( ISWbemProperty *iface )
{
    struct property *property = impl_from_ISWbemProperty( iface );
    LONG refs = InterlockedDecrement( &property->refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", property );
        IWbemClassObject_Release( property->object );
        SysFreeString( property->name );
        free( property );
    }
    return refs;
}

static HRESULT WINAPI property_QueryInterface( ISWbemProperty *iface, REFIID riid, void **obj )
{
    struct property *property = impl_from_ISWbemProperty( iface );

    TRACE( "%p %s %p\n", property, debugstr_guid(riid), obj );

    if (IsEqualGUID( riid, &IID_ISWbemProperty ) ||
        IsEqualGUID( riid, &IID_IDispatch ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *obj = iface;
    }
    else
    {
        WARN( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }
    ISWbemProperty_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI property_GetTypeInfoCount( ISWbemProperty *iface, UINT *count )
{
    struct property *property = impl_from_ISWbemProperty( iface );
    TRACE( "%p, %p\n", property, count );
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI property_GetTypeInfo( ISWbemProperty *iface, UINT index,
                                            LCID lcid, ITypeInfo **info )
{
    struct property *property = impl_from_ISWbemProperty( iface );
    TRACE( "%p, %u, %#lx, %p\n", property, index, lcid, info );

    return get_typeinfo( ISWbemProperty_tid, info );
}

static HRESULT WINAPI property_GetIDsOfNames( ISWbemProperty *iface, REFIID riid, LPOLESTR *names,
                                              UINT count, LCID lcid, DISPID *dispid )
{
    struct property *property = impl_from_ISWbemProperty( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %s, %p, %u, %#lx, %p\n", property, debugstr_guid(riid), names, count, lcid, dispid );

    if (!names || !count || !dispid) return E_INVALIDARG;

    hr = get_typeinfo( ISWbemProperty_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, names, count, dispid );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI property_Invoke( ISWbemProperty *iface, DISPID member, REFIID riid,
                                       LCID lcid, WORD flags, DISPPARAMS *params,
                                       VARIANT *result, EXCEPINFO *excep_info, UINT *arg_err )
{
    struct property *property = impl_from_ISWbemProperty( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %ld, %s, %#lx, %#x, %p, %p, %p, %p\n", property, member, debugstr_guid(riid),
           lcid, flags, params, result, excep_info, arg_err );

    hr = get_typeinfo( ISWbemProperty_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &property->ISWbemProperty_iface, member, flags,
                               params, result, excep_info, arg_err );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI property_get_Value( ISWbemProperty *iface, VARIANT *value )
{
    struct property *property = impl_from_ISWbemProperty( iface );

    TRACE( "%p %p\n", property, value );

    return IWbemClassObject_Get( property->object, property->name, 0, value, NULL, NULL );
}

static HRESULT WINAPI property_put_Value( ISWbemProperty *iface, VARIANT *varValue )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI property_get_Name( ISWbemProperty *iface, BSTR *strName )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI property_get_IsLocal( ISWbemProperty *iface, VARIANT_BOOL *bIsLocal )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI property_get_Origin( ISWbemProperty *iface, BSTR *strOrigin )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI property_get_CIMType( ISWbemProperty *iface, WbemCimtypeEnum *iCimType )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI property_get_Qualifiers_( ISWbemProperty *iface, ISWbemQualifierSet **objWbemQualifierSet )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI property_get_IsArray( ISWbemProperty *iface, VARIANT_BOOL *bIsArray )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static const ISWbemPropertyVtbl property_vtbl =
{
    property_QueryInterface,
    property_AddRef,
    property_Release,
    property_GetTypeInfoCount,
    property_GetTypeInfo,
    property_GetIDsOfNames,
    property_Invoke,
    property_get_Value,
    property_put_Value,
    property_get_Name,
    property_get_IsLocal,
    property_get_Origin,
    property_get_CIMType,
    property_get_Qualifiers_,
    property_get_IsArray
};

static HRESULT SWbemProperty_create( IWbemClassObject *wbem_object, BSTR name, ISWbemProperty **obj )
{
    struct property *property;

    TRACE( "%p, %p\n", obj, wbem_object );

    if (!(property = malloc( sizeof(*property) ))) return E_OUTOFMEMORY;
    property->ISWbemProperty_iface.lpVtbl = &property_vtbl;
    property->refs = 1;
    property->object = wbem_object;
    IWbemClassObject_AddRef( property->object );
    property->name = SysAllocStringLen( name, SysStringLen( name ) );
    *obj = &property->ISWbemProperty_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}

struct propertyset
{
    ISWbemPropertySet ISWbemPropertySet_iface;
    LONG refs;
    IWbemClassObject *object;
};

static inline struct propertyset *impl_from_ISWbemPropertySet(
    ISWbemPropertySet *iface )
{
    return CONTAINING_RECORD( iface, struct propertyset, ISWbemPropertySet_iface );
}

static ULONG WINAPI propertyset_AddRef( ISWbemPropertySet *iface )
{
    struct propertyset *propertyset = impl_from_ISWbemPropertySet( iface );
    return InterlockedIncrement( &propertyset->refs );
}

static ULONG WINAPI propertyset_Release( ISWbemPropertySet *iface )
{
    struct propertyset *propertyset = impl_from_ISWbemPropertySet( iface );
    LONG refs = InterlockedDecrement( &propertyset->refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", propertyset );
        IWbemClassObject_Release( propertyset->object );
        free( propertyset );
    }
    return refs;
}

static HRESULT WINAPI propertyset_QueryInterface( ISWbemPropertySet *iface,
                                                  REFIID riid, void **obj )
{
    struct propertyset *propertyset = impl_from_ISWbemPropertySet( iface );

    TRACE( "%p %s %p\n", propertyset, debugstr_guid(riid), obj );

    if (IsEqualGUID( riid, &IID_ISWbemPropertySet ) ||
        IsEqualGUID( riid, &IID_IDispatch ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *obj = iface;
    }
    else
    {
        WARN( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }
    ISWbemPropertySet_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI propertyset_GetTypeInfoCount( ISWbemPropertySet *iface, UINT *count )
{
    struct propertyset *propertyset = impl_from_ISWbemPropertySet( iface );
    TRACE( "%p, %p\n", propertyset, count );
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI propertyset_GetTypeInfo( ISWbemPropertySet *iface,
                                               UINT index, LCID lcid, ITypeInfo **info )
{
    struct propertyset *propertyset = impl_from_ISWbemPropertySet( iface );
    TRACE( "%p, %u, %#lx, %p\n", propertyset, index, lcid, info );

    return get_typeinfo( ISWbemPropertySet_tid, info );
}

static HRESULT WINAPI propertyset_GetIDsOfNames( ISWbemPropertySet *iface, REFIID riid, LPOLESTR *names,
                                                 UINT count, LCID lcid, DISPID *dispid )
{
    struct propertyset *propertyset = impl_from_ISWbemPropertySet( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %s, %p, %u, %#lx, %p\n", propertyset, debugstr_guid(riid), names, count, lcid, dispid );

    if (!names || !count || !dispid) return E_INVALIDARG;

    hr = get_typeinfo( ISWbemPropertySet_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, names, count, dispid );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI propertyset_Invoke( ISWbemPropertySet *iface, DISPID member, REFIID riid,
                                          LCID lcid, WORD flags, DISPPARAMS *params,
                                          VARIANT *result, EXCEPINFO *excep_info, UINT *arg_err )
{
    struct propertyset *propertyset = impl_from_ISWbemPropertySet( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %ld, %s, %#lx, %#x, %p, %p, %p, %p\n", propertyset, member, debugstr_guid(riid),
           lcid, flags, params, result, excep_info, arg_err );

    hr = get_typeinfo( ISWbemPropertySet_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &propertyset->ISWbemPropertySet_iface, member, flags,
                               params, result, excep_info, arg_err );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI propertyset_get__NewEnum( ISWbemPropertySet *iface, IUnknown **unk )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI propertyset_Item( ISWbemPropertySet *iface, BSTR name,
                                        LONG flags, ISWbemProperty **prop )
{
    struct propertyset *propertyset = impl_from_ISWbemPropertySet( iface );
    HRESULT hr;
    VARIANT var;

    TRACE( "%p, %s, %#lx, %p\n", propertyset, debugstr_w(name), flags, prop );

    hr = IWbemClassObject_Get( propertyset->object, name, 0, &var, NULL, NULL );
    if (SUCCEEDED(hr))
    {
        hr = SWbemProperty_create( propertyset->object, name, prop );
        VariantClear( &var );
    }
    return hr;
}

static HRESULT WINAPI propertyset_get_Count( ISWbemPropertySet *iface, LONG *count )
{
    struct propertyset *propertyset = impl_from_ISWbemPropertySet( iface );
    HRESULT hr;
    VARIANT val;

    TRACE( "%p, %p\n", propertyset, count );

    hr = IWbemClassObject_Get( propertyset->object, L"__PROPERTY_COUNT", 0, &val, NULL, NULL );
    if (SUCCEEDED(hr))
    {
        *count = V_I4( &val );
    }
    return hr;
}

static HRESULT WINAPI propertyset_Add( ISWbemPropertySet *iface, BSTR name, WbemCimtypeEnum type,
                                       VARIANT_BOOL is_array, LONG flags, ISWbemProperty **prop )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI propertyset_Remove( ISWbemPropertySet *iface, BSTR name, LONG flags )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static const ISWbemPropertySetVtbl propertyset_vtbl =
{
    propertyset_QueryInterface,
    propertyset_AddRef,
    propertyset_Release,
    propertyset_GetTypeInfoCount,
    propertyset_GetTypeInfo,
    propertyset_GetIDsOfNames,
    propertyset_Invoke,
    propertyset_get__NewEnum,
    propertyset_Item,
    propertyset_get_Count,
    propertyset_Add,
    propertyset_Remove
};

static HRESULT SWbemPropertySet_create( IWbemClassObject *wbem_object, ISWbemPropertySet **obj )
{
    struct propertyset *propertyset;

    TRACE( "%p, %p\n", obj, wbem_object );

    if (!(propertyset = malloc( sizeof(*propertyset) ))) return E_OUTOFMEMORY;
    propertyset->ISWbemPropertySet_iface.lpVtbl = &propertyset_vtbl;
    propertyset->refs = 1;
    propertyset->object = wbem_object;
    IWbemClassObject_AddRef( propertyset->object );
    *obj = &propertyset->ISWbemPropertySet_iface;

    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}

struct services
{
    ISWbemServices ISWbemServices_iface;
    LONG refs;
    IWbemServices *services;
};

struct member
{
    BSTR name;
    BOOL is_method;
    DISPID dispid;
    CIMTYPE type;
};

struct object
{
    ISWbemObject ISWbemObject_iface;
    LONG refs;
    IWbemClassObject *object;
    struct member *members;
    UINT nb_members;
    DISPID last_dispid;
    DISPID last_dispid_method;
    struct services *services;
};

struct methodset
{
    ISWbemMethodSet ISWbemMethodSet_iface;
    LONG refs;
    struct object *object;
};

struct method
{
    ISWbemMethod ISWbemMethod_iface;
    LONG refs;
    struct methodset *set;
    WCHAR *name;
};

static struct method *impl_from_ISWbemMethod( ISWbemMethod *iface )
{
    return CONTAINING_RECORD( iface, struct method, ISWbemMethod_iface );
}

static HRESULT WINAPI method_QueryInterface( ISWbemMethod *iface, REFIID riid, void **ppvObject )
{
    struct method *method = impl_from_ISWbemMethod( iface );

    TRACE( "%p %s %p\n", method, debugstr_guid(riid), ppvObject );

    if (IsEqualGUID( riid, &IID_ISWbemMethod ) ||
        IsEqualGUID( riid, &IID_IDispatch ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *ppvObject = iface;
    }
    else
    {
        WARN( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }
    ISWbemMethod_AddRef( iface );
    return S_OK;
}

static ULONG WINAPI method_AddRef( ISWbemMethod *iface )
{
    struct method *method = impl_from_ISWbemMethod( iface );
    return InterlockedIncrement( &method->refs );
}

static ULONG WINAPI method_Release( ISWbemMethod *iface )
{
    struct method *method = impl_from_ISWbemMethod( iface );
    LONG refs = InterlockedDecrement( &method->refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", method );
        ISWbemMethodSet_Release( &method->set->ISWbemMethodSet_iface );
        free( method->name );
        free( method );
    }
    return refs;
}

static HRESULT WINAPI method_GetTypeInfoCount(
    ISWbemMethod *iface,
    UINT *count )
{
    struct method *method = impl_from_ISWbemMethod( iface );

    TRACE( "%p, %p\n", method, count );
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI method_GetTypeInfo(
    ISWbemMethod *iface,
    UINT index,
    LCID lcid,
    ITypeInfo **info )
{
    struct method *method = impl_from_ISWbemMethod( iface );

    TRACE( "%p, %u, %#lx, %p\n", method, index, lcid, info );

    return get_typeinfo( ISWbemMethod_tid, info );
}

static HRESULT WINAPI method_GetIDsOfNames(
    ISWbemMethod *iface,
    REFIID riid,
    LPOLESTR *names,
    UINT count,
    LCID lcid,
    DISPID *dispid )
{
    struct method *method = impl_from_ISWbemMethod( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %s, %p, %u, %#lx, %p\n", method, debugstr_guid(riid), names, count, lcid, dispid );

    if (!names || !count || !dispid) return E_INVALIDARG;

    hr = get_typeinfo( ISWbemMethod_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, names, count, dispid );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI method_Invoke(
    ISWbemMethod *iface,
    DISPID member,
    REFIID riid,
    LCID lcid,
    WORD flags,
    DISPPARAMS *params,
    VARIANT *result,
    EXCEPINFO *excep_info,
    UINT *arg_err )
{
    struct method *method = impl_from_ISWbemMethod( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %ld, %s, %#lx, %#x, %p, %p, %p, %p\n", method, member, debugstr_guid(riid),
           lcid, flags, params, result, excep_info, arg_err );

    hr = get_typeinfo( ISWbemMethod_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &method->ISWbemMethod_iface, member, flags,
                               params, result, excep_info, arg_err );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI method_get_Name(
    ISWbemMethod *iface,
    BSTR *name )
{
    FIXME("\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI method_get_Origin(
    ISWbemMethod *iface,
    BSTR *origin )
{
    FIXME("\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI method_get_InParameters(
    ISWbemMethod *iface,
    ISWbemObject **params )
{
    struct method *method = impl_from_ISWbemMethod( iface );
    IWbemClassObject *in_sign = NULL, *instance;
    HRESULT hr;

    TRACE("%p, %p\n", method, params);

    *params = NULL;

    if (SUCCEEDED(hr = IWbemClassObject_GetMethod( method->set->object->object,
            method->name, 0, &in_sign, NULL )) && in_sign != NULL)
    {
        hr = IWbemClassObject_SpawnInstance( in_sign, 0, &instance );
        IWbemClassObject_Release( in_sign );
        if (SUCCEEDED(hr))
        {
            hr = SWbemObject_create( method->set->object->services, instance, params );
            IWbemClassObject_Release( instance );
        }
    }

    return hr;
}

static HRESULT WINAPI method_get_OutParameters(
    ISWbemMethod *iface,
    ISWbemObject **params )
{
    FIXME("\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI method_get_Qualifiers_(
    ISWbemMethod *iface,
    ISWbemQualifierSet **qualifiers )
{
    FIXME("\n");

    return E_NOTIMPL;
}

static const ISWbemMethodVtbl methodvtbl =
{
    method_QueryInterface,
    method_AddRef,
    method_Release,
    method_GetTypeInfoCount,
    method_GetTypeInfo,
    method_GetIDsOfNames,
    method_Invoke,
    method_get_Name,
    method_get_Origin,
    method_get_InParameters,
    method_get_OutParameters,
    method_get_Qualifiers_,
};

static HRESULT SWbemMethod_create( struct methodset *set, const WCHAR *name, ISWbemMethod **obj )
{
    struct method *method;

    if (!(method = malloc( sizeof(*method) ))) return E_OUTOFMEMORY;

    method->ISWbemMethod_iface.lpVtbl = &methodvtbl;
    method->refs = 1;
    method->set = set;
    ISWbemMethodSet_AddRef( &method->set->ISWbemMethodSet_iface );
    if (!(method->name = wcsdup( name )))
    {
        ISWbemMethod_Release( &method->ISWbemMethod_iface );
        return E_OUTOFMEMORY;
    }

    *obj = &method->ISWbemMethod_iface;

    return S_OK;
}

static struct methodset *impl_from_ISWbemMethodSet( ISWbemMethodSet *iface )
{
    return CONTAINING_RECORD( iface, struct methodset, ISWbemMethodSet_iface );
}

static HRESULT WINAPI methodset_QueryInterface( ISWbemMethodSet *iface, REFIID riid, void **ppvObject )
{
    struct methodset *set = impl_from_ISWbemMethodSet( iface );

    TRACE( "%p %s %p\n", set, debugstr_guid(riid), ppvObject );

    if (IsEqualGUID( riid, &IID_ISWbemMethodSet ) ||
        IsEqualGUID( riid, &IID_IDispatch ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *ppvObject = iface;
    }
    else
    {
        WARN( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }
    ISWbemMethodSet_AddRef( iface );
    return S_OK;
}

static ULONG WINAPI methodset_AddRef( ISWbemMethodSet *iface )
{
    struct methodset *set = impl_from_ISWbemMethodSet( iface );
    return InterlockedIncrement( &set->refs );
}

static ULONG WINAPI methodset_Release( ISWbemMethodSet *iface )
{
    struct methodset *set = impl_from_ISWbemMethodSet( iface );
    LONG refs = InterlockedDecrement( &set->refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", set );
        ISWbemObject_Release( &set->object->ISWbemObject_iface );
        free( set );
    }
    return refs;
}

static HRESULT WINAPI methodset_GetTypeInfoCount(
    ISWbemMethodSet *iface,
    UINT *count )
{
    struct methodset *set = impl_from_ISWbemMethodSet( iface );

    TRACE( "%p, %p\n", set, count );
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI methodset_GetTypeInfo( ISWbemMethodSet *iface,
    UINT index,
    LCID lcid,
    ITypeInfo **info )
{
    struct methodset *set = impl_from_ISWbemMethodSet( iface );

    TRACE( "%p, %u, %#lx, %p\n", set, index, lcid, info );

    return get_typeinfo( ISWbemMethodSet_tid, info );
}

static HRESULT WINAPI methodset_GetIDsOfNames(
    ISWbemMethodSet *iface,
    REFIID riid,
    LPOLESTR *names,
    UINT count,
    LCID lcid,
    DISPID *dispid )
{
    struct methodset *set = impl_from_ISWbemMethodSet( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %s, %p, %u, %#lx, %p\n", set, debugstr_guid(riid), names, count, lcid, dispid );

    if (!names || !count || !dispid) return E_INVALIDARG;

    hr = get_typeinfo( ISWbemMethodSet_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, names, count, dispid );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI methodset_Invoke(
    ISWbemMethodSet *iface,
    DISPID member,
    REFIID riid,
    LCID lcid,
    WORD flags,
    DISPPARAMS *params,
    VARIANT *result,
    EXCEPINFO *excep_info,
    UINT *arg_err )
{
    struct methodset *set = impl_from_ISWbemMethodSet( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %ld, %s, %#lx, %#x, %p, %p, %p, %p\n", set, member, debugstr_guid(riid),
           lcid, flags, params, result, excep_info, arg_err );

    hr = get_typeinfo( ISWbemMethodSet_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &set->ISWbemMethodSet_iface, member, flags,
                               params, result, excep_info, arg_err );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI methodset_get__NewEnum(
    ISWbemMethodSet *iface,
    IUnknown **unk )
{
    FIXME("\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI methodset_Item(
    ISWbemMethodSet *iface,
    BSTR name,
    LONG flags,
    ISWbemMethod **method )
{
    struct methodset *set = impl_from_ISWbemMethodSet( iface );
    IWbemClassObject *in_sign, *out_sign;
    HRESULT hr;

    TRACE("%p, %s, %#lx, %p\n", set, debugstr_w(name), flags, method);

    *method = NULL;

    if (SUCCEEDED(hr = IWbemClassObject_GetMethod( set->object->object,
            name, flags, &in_sign, &out_sign )))
    {
        if (in_sign)
            IWbemClassObject_Release( in_sign );
        if (out_sign)
            IWbemClassObject_Release( out_sign );

        return SWbemMethod_create( set, name, method );
    }

    return hr;
}

static HRESULT WINAPI methodset_get_Count(
    ISWbemMethodSet *iface,
    LONG *count )
{
    FIXME("\n");

    return E_NOTIMPL;
}

static const ISWbemMethodSetVtbl methodsetvtbl =
{
    methodset_QueryInterface,
    methodset_AddRef,
    methodset_Release,
    methodset_GetTypeInfoCount,
    methodset_GetTypeInfo,
    methodset_GetIDsOfNames,
    methodset_Invoke,
    methodset_get__NewEnum,
    methodset_Item,
    methodset_get_Count,
};

static HRESULT SWbemMethodSet_create( struct object *object, ISWbemMethodSet **obj )
{
    struct methodset *set;

    if (!(set = malloc( sizeof(*set) ))) return E_OUTOFMEMORY;

    set->ISWbemMethodSet_iface.lpVtbl = &methodsetvtbl;
    set->refs = 1;
    set->object = object;
    ISWbemObject_AddRef( &object->ISWbemObject_iface );

    *obj = &set->ISWbemMethodSet_iface;

    return S_OK;
}

#define DISPID_BASE         0x1800000
#define DISPID_BASE_METHOD  0x1000000

static inline struct object *impl_from_ISWbemObject(
    ISWbemObject *iface )
{
    return CONTAINING_RECORD( iface, struct object, ISWbemObject_iface );
}

static ULONG WINAPI object_AddRef(
    ISWbemObject *iface )
{
    struct object *object = impl_from_ISWbemObject( iface );
    return InterlockedIncrement( &object->refs );
}

static ULONG WINAPI object_Release(
    ISWbemObject *iface )
{
    struct object *object = impl_from_ISWbemObject( iface );
    LONG refs = InterlockedDecrement( &object->refs );
    if (!refs)
    {
        UINT i;

        TRACE( "destroying %p\n", object );
        IWbemClassObject_Release( object->object );
        ISWbemServices_Release( &object->services->ISWbemServices_iface );
        for (i = 0; i < object->nb_members; i++) SysFreeString( object->members[i].name );
        free( object->members );
        free( object );
    }
    return refs;
}

static HRESULT WINAPI object_QueryInterface(
    ISWbemObject *iface,
    REFIID riid,
    void **ppvObject )
{
    struct object *object = impl_from_ISWbemObject( iface );

    TRACE( "%p %s %p\n", object, debugstr_guid(riid), ppvObject );

    if (IsEqualGUID( riid, &IID_ISWbemObject ) ||
        IsEqualGUID( riid, &IID_IDispatch ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *ppvObject = iface;
    }
    else
    {
        WARN( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }
    ISWbemObject_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI object_GetTypeInfoCount(
    ISWbemObject *iface,
    UINT *count )
{
    struct object *object = impl_from_ISWbemObject( iface );

    TRACE( "%p, %p\n", object, count );
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI object_GetTypeInfo(
    ISWbemObject *iface,
    UINT index,
    LCID lcid,
    ITypeInfo **info )
{
    struct object *object = impl_from_ISWbemObject( iface );
    FIXME( "%p, %u, %#lx, %p\n", object, index, lcid, info );
    return E_NOTIMPL;
}

static BOOL object_reserve_member( struct object *object, unsigned int count, unsigned int *capacity )
{
    unsigned int new_capacity, max_capacity;
    struct member *new_members;

    if (count <= *capacity)
        return TRUE;

    max_capacity = ~0u / sizeof(*object->members);
    if (count > max_capacity)
        return FALSE;

    new_capacity = max(4, *capacity);
    while (new_capacity < count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;
    if (new_capacity < count)
        new_capacity = max_capacity;

    if (!(new_members = realloc( object->members, new_capacity * sizeof(*new_members) )))
        return FALSE;

    object->members = new_members;
    *capacity = new_capacity;

    return TRUE;
}

static HRESULT init_members( struct object *object )
{
    IWbemClassObject *sig_in, *sig_out;
    unsigned int i, capacity = 0, count = 0;
    CIMTYPE type;
    HRESULT hr;
    BSTR name;

    if (object->members) return S_OK;

    hr = IWbemClassObject_BeginEnumeration( object->object, 0 );
    if (SUCCEEDED( hr ))
    {
        while (IWbemClassObject_Next( object->object, 0, &name, NULL, &type, NULL ) == S_OK)
        {
            if (!object_reserve_member( object, count + 1, &capacity )) goto error;
            object->members[count].name      = name;
            object->members[count].is_method = FALSE;
            object->members[count].dispid    = 0;
            object->members[count].type      = type;
            count++;
            TRACE( "added property %s\n", debugstr_w(name) );
        }
        IWbemClassObject_EndEnumeration( object->object );
    }

    hr = IWbemClassObject_BeginMethodEnumeration( object->object, 0 );
    if (SUCCEEDED( hr ))
    {
        while (IWbemClassObject_NextMethod( object->object, 0, &name, &sig_in, &sig_out ) == S_OK)
        {
            if (!object_reserve_member( object, count + 1, &capacity )) goto error;
            object->members[count].name      = name;
            object->members[count].is_method = TRUE;
            object->members[count].dispid    = 0;
            count++;
            if (sig_in) IWbemClassObject_Release( sig_in );
            if (sig_out) IWbemClassObject_Release( sig_out );
            TRACE( "added method %s\n", debugstr_w(name) );
        }
        IWbemClassObject_EndMethodEnumeration( object->object );
    }

    object->nb_members = count;
    TRACE( "added %u members\n", object->nb_members );
    return S_OK;

error:
    for (i = 0; i < count; ++i)
        SysFreeString( object->members[i].name );
    free( object->members );
    object->members = NULL;
    object->nb_members = 0;
    return E_FAIL;
}

static DISPID get_member_dispid( struct object *object, const WCHAR *name )
{
    UINT i;
    for (i = 0; i < object->nb_members; i++)
    {
        if (!wcsicmp( object->members[i].name, name ))
        {
            if (!object->members[i].dispid)
            {
                if (object->members[i].is_method)
                    object->members[i].dispid = ++object->last_dispid_method;
                else
                    object->members[i].dispid = ++object->last_dispid;
            }
            return object->members[i].dispid;
        }
    }
    return DISPID_UNKNOWN;
}

static HRESULT WINAPI object_GetIDsOfNames(
    ISWbemObject *iface,
    REFIID riid,
    LPOLESTR *names,
    UINT count,
    LCID lcid,
    DISPID *dispid )
{
    struct object *object = impl_from_ISWbemObject( iface );
    HRESULT hr;
    UINT i;
    ITypeInfo *typeinfo;

    TRACE( "%p, %s, %p, %u, %#lx, %p\n", object, debugstr_guid(riid), names, count, lcid, dispid );

    if (!names || !count || !dispid) return E_INVALIDARG;

    hr = init_members( object );
    if (FAILED( hr )) return hr;

    hr = get_typeinfo( ISWbemObject_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, names, count, dispid );
        ITypeInfo_Release( typeinfo );
    }
    if (SUCCEEDED(hr)) return hr;

    for (i = 0; i < count; i++)
    {
        if ((dispid[i] = get_member_dispid( object, names[i] )) == DISPID_UNKNOWN) break;
    }
    if (i != count) return DISP_E_UNKNOWNNAME;
    return S_OK;
}

static BSTR get_member_name( struct object *object, DISPID dispid, CIMTYPE *type )
{
    UINT i;
    for (i = 0; i < object->nb_members; i++)
    {
        if (object->members[i].dispid == dispid)
        {
            *type = object->members[i].type;
            return object->members[i].name;
        }
    }
    return NULL;
}

static VARTYPE to_vartype( CIMTYPE type )
{
    switch (type)
    {
    case CIM_BOOLEAN:   return VT_BOOL;

    case CIM_STRING:
    case CIM_REFERENCE:
    case CIM_DATETIME:  return VT_BSTR;

    case CIM_SINT8:     return VT_I1;
    case CIM_UINT8:     return VT_UI1;
    case CIM_SINT16:    return VT_I2;

    case CIM_UINT16:
    case CIM_SINT32:    return VT_I4;

    case CIM_UINT32:    return VT_UI4;

    case CIM_SINT64:    return VT_I8;
    case CIM_UINT64:    return VT_UI8;

    case CIM_REAL32:    return VT_R4;

    default:
        ERR( "unhandled type %lu\n", type );
        break;
    }
    return 0;
}

static HRESULT WINAPI object_Invoke(
    ISWbemObject *iface,
    DISPID member,
    REFIID riid,
    LCID lcid,
    WORD flags,
    DISPPARAMS *params,
    VARIANT *result,
    EXCEPINFO *excep_info,
    UINT *arg_err )
{
    struct object *object = impl_from_ISWbemObject( iface );
    BSTR name;
    ITypeInfo *typeinfo;
    VARTYPE vartype;
    VARIANT value;
    CIMTYPE type;
    HRESULT hr;

    TRACE( "%p, %ld, %s, %#lx, %#x, %p, %p, %p, %p\n", object, member, debugstr_guid(riid),
           lcid, flags, params, result, excep_info, arg_err );

    if (member <= DISPID_BASE_METHOD)
    {
        hr = get_typeinfo( ISWbemObject_tid, &typeinfo );
        if (SUCCEEDED(hr))
        {
            hr = ITypeInfo_Invoke( typeinfo, &object->ISWbemObject_iface, member, flags,
                                   params, result, excep_info, arg_err );
            ITypeInfo_Release( typeinfo );
        }
        return hr;
    }

    if (!(name = get_member_name( object, member, &type )))
        return DISP_E_MEMBERNOTFOUND;

    if (flags == (DISPATCH_METHOD|DISPATCH_PROPERTYGET) ||
        flags == DISPATCH_PROPERTYGET)
    {
        memset( params, 0, sizeof(*params) );
        return IWbemClassObject_Get( object->object, name, 0, result, NULL, NULL );
    }
    else if (flags == DISPATCH_PROPERTYPUT)
    {
        if (!params->cArgs || !params->rgvarg)
        {
            WARN( "Missing put property value\n" );
            return E_INVALIDARG;
        }

        vartype = to_vartype( type );
        V_VT( &value ) = VT_EMPTY;
        if (SUCCEEDED(hr = VariantChangeType( &value, params->rgvarg, 0, vartype )))
        {
            hr = IWbemClassObject_Put( object->object, name, 0, &value, 0 );
            VariantClear( &value );
        }
        return hr;
    }
    else
    {
        FIXME( "flags %#x not supported\n", flags );
        return E_NOTIMPL;
    }
}

static HRESULT WINAPI object_Put_(
    ISWbemObject *iface,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    ISWbemObjectPath **objWbemObjectPath )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI object_PutAsync_(
    ISWbemObject *iface,
    IDispatch *objWbemSink,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    IDispatch *objWbemAsyncContext )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI object_Delete_(
    ISWbemObject *iface,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI object_DeleteAsync_(
    ISWbemObject *iface,
    IDispatch *objWbemSink,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    IDispatch *objWbemAsyncContext )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI object_Instances_(
    ISWbemObject *iface,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    ISWbemObjectSet **objWbemObjectSet )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI object_InstancesAsync_(
    ISWbemObject *iface,
    IDispatch *objWbemSink,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    IDispatch *objWbemAsyncContext )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI object_Subclasses_(
    ISWbemObject *iface,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    ISWbemObjectSet **objWbemObjectSet )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI object_SubclassesAsync_(
    ISWbemObject *iface,
    IDispatch *objWbemSink,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    IDispatch *objWbemAsyncContext )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI object_Associators_(
    ISWbemObject *iface,
    BSTR strAssocClass,
    BSTR strResultClass,
    BSTR strResultRole,
    BSTR strRole,
    VARIANT_BOOL bClassesOnly,
    VARIANT_BOOL bSchemaOnly,
    BSTR strRequiredAssocQualifier,
    BSTR strRequiredQualifier,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    ISWbemObjectSet **objWbemObjectSet )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI object_AssociatorsAsync_(
    ISWbemObject *iface,
    IDispatch *objWbemSink,
    BSTR strAssocClass,
    BSTR strResultClass,
    BSTR strResultRole,
    BSTR strRole,
    VARIANT_BOOL bClassesOnly,
    VARIANT_BOOL bSchemaOnly,
    BSTR strRequiredAssocQualifier,
    BSTR strRequiredQualifier,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    IDispatch *objWbemAsyncContext )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI object_References_(
    ISWbemObject *iface,
    BSTR strResultClass,
    BSTR strRole,
    VARIANT_BOOL bClassesOnly,
    VARIANT_BOOL bSchemaOnly,
    BSTR strRequiredQualifier,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    ISWbemObjectSet **objWbemObjectSet )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI object_ReferencesAsync_(
    ISWbemObject *iface,
    IDispatch *objWbemSink,
    BSTR strResultClass,
    BSTR strRole,
    VARIANT_BOOL bClassesOnly,
    VARIANT_BOOL bSchemaOnly,
    BSTR strRequiredQualifier,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    IDispatch *objWbemAsyncContext )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI object_ExecMethod_(
    ISWbemObject *iface,
    BSTR method,
    IDispatch *in_params,
    LONG flags,
    IDispatch *valueset,
    ISWbemObject **out_params )
{
    struct object *object = impl_from_ISWbemObject( iface );
    VARIANT path;
    HRESULT hr;

    TRACE( "%p, %s, %p, %#lx, %p, %p\n", object, debugstr_w(method), in_params, flags, valueset, out_params );

    V_VT( &path ) = VT_EMPTY;
    hr = IWbemClassObject_Get( object->object, L"__PATH", 0, &path, NULL, NULL );
    if (SUCCEEDED(hr))
    {
        if (V_VT( &path ) != VT_BSTR)
        {
            WARN( "Unexpected object path value type.\n" );
            VariantClear( &path );
            return E_UNEXPECTED;
        }

        hr = ISWbemServices_ExecMethod( &object->services->ISWbemServices_iface, V_BSTR( &path ), method,
                in_params, flags, valueset, out_params );

        VariantClear( &path );
    }

    return hr;
}

static HRESULT WINAPI object_ExecMethodAsync_(
    ISWbemObject *iface,
    IDispatch *objWbemSink,
    BSTR strMethodName,
    IDispatch *objWbemInParameters,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    IDispatch *objWbemAsyncContext )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI object_Clone_(
    ISWbemObject *iface,
    ISWbemObject **objWbemObject )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI object_GetObjectText_(
    ISWbemObject *iface,
    LONG flags,
    BSTR *text )
{
    struct object *object = impl_from_ISWbemObject( iface );

    TRACE( "%p, %#lx, %p\n", object, flags, text );

    return IWbemClassObject_GetObjectText( object->object, flags, text );
}

static HRESULT WINAPI object_SpawnDerivedClass_(
    ISWbemObject *iface,
    LONG iFlags,
    ISWbemObject **objWbemObject )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI object_SpawnInstance_(
    ISWbemObject *iface,
    LONG iFlags,
    ISWbemObject **objWbemObject )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI object_CompareTo_(
    ISWbemObject *iface,
    IDispatch *objWbemObject,
    LONG iFlags,
    VARIANT_BOOL *bResult )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI object_get_Qualifiers_(
    ISWbemObject *iface,
    ISWbemQualifierSet **objWbemQualifierSet )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI object_get_Properties_( ISWbemObject *iface, ISWbemPropertySet **prop_set )
{
    struct object *object = impl_from_ISWbemObject( iface );

    TRACE( "%p, %p\n", object, prop_set );
    return SWbemPropertySet_create( object->object, prop_set );
}

static HRESULT WINAPI object_get_Methods_(
    ISWbemObject *iface,
    ISWbemMethodSet **set )
{
    struct object *object = impl_from_ISWbemObject( iface );

    TRACE( "%p, %p\n", object, set );
    return SWbemMethodSet_create( object, set );
}

static HRESULT WINAPI object_get_Derivation_(
    ISWbemObject *iface,
    VARIANT *strClassNameArray )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI object_get_Path_(
    ISWbemObject *iface,
    ISWbemObjectPath **objWbemObjectPath )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI object_get_Security_(
    ISWbemObject *iface,
    ISWbemSecurity **objWbemSecurity )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static const ISWbemObjectVtbl object_vtbl =
{
    object_QueryInterface,
    object_AddRef,
    object_Release,
    object_GetTypeInfoCount,
    object_GetTypeInfo,
    object_GetIDsOfNames,
    object_Invoke,
    object_Put_,
    object_PutAsync_,
    object_Delete_,
    object_DeleteAsync_,
    object_Instances_,
    object_InstancesAsync_,
    object_Subclasses_,
    object_SubclassesAsync_,
    object_Associators_,
    object_AssociatorsAsync_,
    object_References_,
    object_ReferencesAsync_,
    object_ExecMethod_,
    object_ExecMethodAsync_,
    object_Clone_,
    object_GetObjectText_,
    object_SpawnDerivedClass_,
    object_SpawnInstance_,
    object_CompareTo_,
    object_get_Qualifiers_,
    object_get_Properties_,
    object_get_Methods_,
    object_get_Derivation_,
    object_get_Path_,
    object_get_Security_
};

static struct object *unsafe_object_impl_from_IDispatch(IDispatch *iface)
{
    if (!iface)
        return NULL;
    if (iface->lpVtbl != (IDispatchVtbl *)&object_vtbl)
    {
        FIXME( "External implementations are not supported.\n" );
        return NULL;
    }
    return CONTAINING_RECORD(iface, struct object, ISWbemObject_iface);
}

static HRESULT SWbemObject_create( struct services *services, IWbemClassObject *wbem_object,
        ISWbemObject **obj )
{
    struct object *object;

    TRACE( "%p, %p\n", obj, wbem_object );

    if (!(object = malloc( sizeof(*object) ))) return E_OUTOFMEMORY;
    object->ISWbemObject_iface.lpVtbl = &object_vtbl;
    object->refs = 1;
    object->object = wbem_object;
    IWbemClassObject_AddRef( object->object );
    object->members = NULL;
    object->nb_members = 0;
    object->last_dispid = DISPID_BASE;
    object->last_dispid_method = DISPID_BASE_METHOD;
    object->services = services;
    ISWbemServices_AddRef( &object->services->ISWbemServices_iface );

    *obj = &object->ISWbemObject_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}

struct objectset
{
    ISWbemObjectSet ISWbemObjectSet_iface;
    LONG refs;
    IEnumWbemClassObject *objectenum;
    LONG count;
    struct services *services;
};

static inline struct objectset *impl_from_ISWbemObjectSet(
    ISWbemObjectSet *iface )
{
    return CONTAINING_RECORD( iface, struct objectset, ISWbemObjectSet_iface );
}

static ULONG WINAPI objectset_AddRef(
    ISWbemObjectSet *iface )
{
    struct objectset *objectset = impl_from_ISWbemObjectSet( iface );
    return InterlockedIncrement( &objectset->refs );
}

static ULONG WINAPI objectset_Release(
    ISWbemObjectSet *iface )
{
    struct objectset *objectset = impl_from_ISWbemObjectSet( iface );
    LONG refs = InterlockedDecrement( &objectset->refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", objectset );
        IEnumWbemClassObject_Release( objectset->objectenum );
        ISWbemServices_Release( &objectset->services->ISWbemServices_iface );
        free( objectset );
    }
    return refs;
}

static HRESULT WINAPI objectset_QueryInterface(
    ISWbemObjectSet *iface,
    REFIID riid,
    void **ppvObject )
{
    struct objectset *objectset = impl_from_ISWbemObjectSet( iface );

    TRACE( "%p %s %p\n", objectset, debugstr_guid(riid), ppvObject );

    if (IsEqualGUID( riid, &IID_ISWbemObjectSet ) ||
        IsEqualGUID( riid, &IID_IDispatch ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *ppvObject = iface;
    }
    else
    {
        WARN( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }
    ISWbemObjectSet_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI objectset_GetTypeInfoCount(
    ISWbemObjectSet *iface,
    UINT *count )
{
    struct objectset *objectset = impl_from_ISWbemObjectSet( iface );
    TRACE( "%p, %p\n", objectset, count );
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI objectset_GetTypeInfo(
    ISWbemObjectSet *iface,
    UINT index,
    LCID lcid,
    ITypeInfo **info )
{
    struct objectset *objectset = impl_from_ISWbemObjectSet( iface );
    TRACE( "%p, %u, %#lx, %p\n", objectset, index, lcid, info );

    return get_typeinfo( ISWbemObjectSet_tid, info );
}

static HRESULT WINAPI objectset_GetIDsOfNames(
    ISWbemObjectSet *iface,
    REFIID riid,
    LPOLESTR *names,
    UINT count,
    LCID lcid,
    DISPID *dispid )
{
    struct objectset *objectset = impl_from_ISWbemObjectSet( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %s, %p, %u, %#lx, %p\n", objectset, debugstr_guid(riid), names, count, lcid, dispid );

    if (!names || !count || !dispid) return E_INVALIDARG;

    hr = get_typeinfo( ISWbemObjectSet_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, names, count, dispid );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI objectset_Invoke(
    ISWbemObjectSet *iface,
    DISPID member,
    REFIID riid,
    LCID lcid,
    WORD flags,
    DISPPARAMS *params,
    VARIANT *result,
    EXCEPINFO *excep_info,
    UINT *arg_err )
{
    struct objectset *objectset = impl_from_ISWbemObjectSet( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %ld, %s, %#lx, %#x, %p, %p, %p, %p\n", objectset, member, debugstr_guid(riid),
           lcid, flags, params, result, excep_info, arg_err );

    hr = get_typeinfo( ISWbemObjectSet_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &objectset->ISWbemObjectSet_iface, member, flags,
                               params, result, excep_info, arg_err );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI objectset_get__NewEnum(
    ISWbemObjectSet *iface,
    IUnknown **pUnk )
{
    struct objectset *objectset = impl_from_ISWbemObjectSet( iface );
    IEnumWbemClassObject *objectenum;
    HRESULT hr;

    TRACE( "%p, %p\n", objectset, pUnk );

    hr = IEnumWbemClassObject_Clone( objectset->objectenum, &objectenum );
    if (FAILED( hr )) return hr;

    hr = EnumVARIANT_create( objectset->services, objectenum, (IEnumVARIANT **)pUnk );
    IEnumWbemClassObject_Release( objectenum );
    return hr;
}

static HRESULT WINAPI objectset_Item(
    ISWbemObjectSet *iface,
    BSTR strObjectPath,
    LONG iFlags,
    ISWbemObject **objWbemObject )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI objectset_get_Count(
    ISWbemObjectSet *iface,
    LONG *iCount )
{
    struct objectset *objectset = impl_from_ISWbemObjectSet( iface );

    TRACE( "%p, %p\n", objectset, iCount );

    *iCount = objectset->count;
    return S_OK;
}

static HRESULT WINAPI objectset_get_Security_(
    ISWbemObjectSet *iface,
    ISWbemSecurity **objWbemSecurity )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI objectset_ItemIndex(
    ISWbemObjectSet *iface,
    LONG lIndex,
    ISWbemObject **objWbemObject )
{
    struct objectset *objectset = impl_from_ISWbemObjectSet( iface );
    LONG count;
    HRESULT hr;
    IEnumVARIANT *enum_var;
    VARIANT var;

    TRACE( "%p, %ld, %p\n", objectset, lIndex, objWbemObject );

    *objWbemObject = NULL;
    hr = ISWbemObjectSet_get_Count( iface, &count );
    if (FAILED(hr)) return hr;

    if (lIndex >= count) return WBEM_E_NOT_FOUND;

    hr = ISWbemObjectSet_get__NewEnum( iface, (IUnknown **)&enum_var );
    if (FAILED(hr)) return hr;

    IEnumVARIANT_Reset( enum_var );
    hr = IEnumVARIANT_Skip( enum_var, lIndex );
    if (SUCCEEDED(hr))
        hr = IEnumVARIANT_Next( enum_var, 1, &var, NULL );
    IEnumVARIANT_Release( enum_var );

    if (SUCCEEDED(hr))
    {
        if (V_VT( &var ) == VT_DISPATCH)
            hr = IDispatch_QueryInterface( V_DISPATCH( &var ), &IID_ISWbemObject, (void **)objWbemObject );
        else
            hr = WBEM_E_NOT_FOUND;
        VariantClear( &var );
    }

    return hr;
}

static const ISWbemObjectSetVtbl objectset_vtbl =
{
    objectset_QueryInterface,
    objectset_AddRef,
    objectset_Release,
    objectset_GetTypeInfoCount,
    objectset_GetTypeInfo,
    objectset_GetIDsOfNames,
    objectset_Invoke,
    objectset_get__NewEnum,
    objectset_Item,
    objectset_get_Count,
    objectset_get_Security_,
    objectset_ItemIndex
};

static LONG get_object_count( IEnumWbemClassObject *iter )
{
    LONG count = 0;
    while (IEnumWbemClassObject_Skip( iter, WBEM_INFINITE, 1 ) == S_OK) count++;
    IEnumWbemClassObject_Reset( iter );
    return count;
}

static HRESULT SWbemObjectSet_create( struct services *services, IEnumWbemClassObject *wbem_objectenum,
        ISWbemObjectSet **obj )
{
    struct objectset *objectset;

    TRACE( "%p, %p\n", obj, wbem_objectenum );

    if (!(objectset = malloc( sizeof(*objectset) ))) return E_OUTOFMEMORY;
    objectset->ISWbemObjectSet_iface.lpVtbl = &objectset_vtbl;
    objectset->refs = 1;
    objectset->objectenum = wbem_objectenum;
    IEnumWbemClassObject_AddRef( objectset->objectenum );
    objectset->count = get_object_count( objectset->objectenum );
    objectset->services = services;
    ISWbemServices_AddRef( &services->ISWbemServices_iface );

    *obj = &objectset->ISWbemObjectSet_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}

struct enumvar
{
    IEnumVARIANT IEnumVARIANT_iface;
    LONG refs;
    IEnumWbemClassObject *objectenum;
    struct services *services;
};

static inline struct enumvar *impl_from_IEnumVARIANT(
    IEnumVARIANT *iface )
{
    return CONTAINING_RECORD( iface, struct enumvar, IEnumVARIANT_iface );
}

static ULONG WINAPI enumvar_AddRef(
    IEnumVARIANT *iface )
{
    struct enumvar *enumvar = impl_from_IEnumVARIANT( iface );
    return InterlockedIncrement( &enumvar->refs );
}

static ULONG WINAPI enumvar_Release(
    IEnumVARIANT *iface )
{
    struct enumvar *enumvar = impl_from_IEnumVARIANT( iface );
    LONG refs = InterlockedDecrement( &enumvar->refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", enumvar );
        IEnumWbemClassObject_Release( enumvar->objectenum );
        ISWbemServices_Release( &enumvar->services->ISWbemServices_iface );
        free( enumvar );
    }
    return refs;
}

static HRESULT WINAPI enumvar_QueryInterface(
    IEnumVARIANT *iface,
    REFIID riid,
    void **ppvObject )
{
    struct enumvar *enumvar = impl_from_IEnumVARIANT( iface );

    TRACE( "%p %s %p\n", enumvar, debugstr_guid(riid), ppvObject );

    if (IsEqualGUID( riid, &IID_IEnumVARIANT ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *ppvObject = iface;
    }
    else
    {
        WARN( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }
    IEnumVARIANT_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI enumvar_Next( IEnumVARIANT *iface, ULONG celt, VARIANT *var, ULONG *fetched )
{
    struct enumvar *enumvar = impl_from_IEnumVARIANT( iface );
    IWbemClassObject *obj;
    ULONG count = 0;

    TRACE( "%p, %lu, %p, %p\n", iface, celt, var, fetched );

    if (celt) IEnumWbemClassObject_Next( enumvar->objectenum, WBEM_INFINITE, 1, &obj, &count );
    if (count)
    {
        ISWbemObject *sobj;
        HRESULT hr;

        hr = SWbemObject_create( enumvar->services, obj, &sobj );
        IWbemClassObject_Release( obj );
        if (FAILED( hr )) return hr;

        V_VT( var ) = VT_DISPATCH;
        V_DISPATCH( var ) = (IDispatch *)sobj;
    }
    if (fetched) *fetched = count;
    return (count < celt) ? S_FALSE : S_OK;
}

static HRESULT WINAPI enumvar_Skip( IEnumVARIANT *iface, ULONG celt )
{
    struct enumvar *enumvar = impl_from_IEnumVARIANT( iface );

    TRACE( "%p, %lu\n", iface, celt );

    return IEnumWbemClassObject_Skip( enumvar->objectenum, WBEM_INFINITE, celt );
}

static HRESULT WINAPI enumvar_Reset( IEnumVARIANT *iface )
{
    struct enumvar *enumvar = impl_from_IEnumVARIANT( iface );

    TRACE( "%p\n", iface );

    return IEnumWbemClassObject_Reset( enumvar->objectenum );
}

static HRESULT WINAPI enumvar_Clone( IEnumVARIANT *iface, IEnumVARIANT **penum )
{
    FIXME( "%p, %p\n", iface, penum );
    return E_NOTIMPL;
}

static const struct IEnumVARIANTVtbl enumvar_vtbl =
{
    enumvar_QueryInterface,
    enumvar_AddRef,
    enumvar_Release,
    enumvar_Next,
    enumvar_Skip,
    enumvar_Reset,
    enumvar_Clone
};

static HRESULT EnumVARIANT_create( struct services *services, IEnumWbemClassObject *objectenum,
        IEnumVARIANT **obj )
{
    struct enumvar *enumvar;

    if (!(enumvar = malloc( sizeof(*enumvar) ))) return E_OUTOFMEMORY;
    enumvar->IEnumVARIANT_iface.lpVtbl = &enumvar_vtbl;
    enumvar->refs = 1;
    enumvar->objectenum = objectenum;
    IEnumWbemClassObject_AddRef( enumvar->objectenum );
    enumvar->services = services;
    ISWbemServices_AddRef( &services->ISWbemServices_iface );

    *obj = &enumvar->IEnumVARIANT_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}

static inline struct services *impl_from_ISWbemServices(
    ISWbemServices *iface )
{
    return CONTAINING_RECORD( iface, struct services, ISWbemServices_iface );
}

static ULONG WINAPI services_AddRef(
    ISWbemServices *iface )
{
    struct services *services = impl_from_ISWbemServices( iface );
    return InterlockedIncrement( &services->refs );
}

static ULONG WINAPI services_Release(
    ISWbemServices *iface )
{
    struct services *services = impl_from_ISWbemServices( iface );
    LONG refs = InterlockedDecrement( &services->refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", services );
        IWbemServices_Release( services->services );
        free( services );
    }
    return refs;
}

static HRESULT WINAPI services_QueryInterface(
    ISWbemServices *iface,
    REFIID riid,
    void **ppvObject )
{
    struct services *services = impl_from_ISWbemServices( iface );

    TRACE( "%p %s %p\n", services, debugstr_guid(riid), ppvObject );

    if (IsEqualGUID( riid, &IID_ISWbemServices ) ||
        IsEqualGUID( riid, &IID_IDispatch ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *ppvObject = iface;
    }
    else
    {
        WARN( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }
    ISWbemServices_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI services_GetTypeInfoCount(
    ISWbemServices *iface,
    UINT *count )
{
    struct services *services = impl_from_ISWbemServices( iface );
    TRACE( "%p, %p\n", services, count );

    *count = 1;
    return S_OK;
}

static HRESULT WINAPI services_GetTypeInfo(
    ISWbemServices *iface,
    UINT index,
    LCID lcid,
    ITypeInfo **info )
{
    struct services *services = impl_from_ISWbemServices( iface );
    TRACE( "%p, %u, %#lx, %p\n", services, index, lcid, info );

    return get_typeinfo( ISWbemServices_tid, info );
}

static HRESULT WINAPI services_GetIDsOfNames(
    ISWbemServices *iface,
    REFIID riid,
    LPOLESTR *names,
    UINT count,
    LCID lcid,
    DISPID *dispid )
{
    struct services *services = impl_from_ISWbemServices( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %s, %p, %u, %#lx, %p\n", services, debugstr_guid(riid), names, count, lcid, dispid );

    if (!names || !count || !dispid) return E_INVALIDARG;

    hr = get_typeinfo( ISWbemServices_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, names, count, dispid );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI services_Invoke(
    ISWbemServices *iface,
    DISPID member,
    REFIID riid,
    LCID lcid,
    WORD flags,
    DISPPARAMS *params,
    VARIANT *result,
    EXCEPINFO *excep_info,
    UINT *arg_err )
{
    struct services *services = impl_from_ISWbemServices( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %ld, %s, %#lx, %#x, %p, %p, %p, %p\n", services, member, debugstr_guid(riid),
           lcid, flags, params, result, excep_info, arg_err );

    hr = get_typeinfo( ISWbemServices_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &services->ISWbemServices_iface, member, flags,
                               params, result, excep_info, arg_err );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI services_Get(
    ISWbemServices *iface,
    BSTR strObjectPath,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    ISWbemObject **objWbemObject )
{
    struct services *services = impl_from_ISWbemServices( iface );
    IWbemClassObject *obj;
    HRESULT hr;

    TRACE( "%p, %s, %#lx, %p, %p\n", iface, debugstr_w(strObjectPath), iFlags, objWbemNamedValueSet,
           objWbemObject );

    if (objWbemNamedValueSet) FIXME( "ignoring context\n" );

    hr = IWbemServices_GetObject( services->services, strObjectPath, iFlags, NULL, &obj, NULL );
    if (hr != S_OK) return hr;

    hr = SWbemObject_create( services, obj, objWbemObject );
    IWbemClassObject_Release( obj );
    return hr;
}

static HRESULT WINAPI services_GetAsync(
    ISWbemServices *iface,
    IDispatch *objWbemSink,
    BSTR strObjectPath,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    IDispatch *objWbemAsyncContext )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_Delete(
    ISWbemServices *iface,
    BSTR strObjectPath,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_DeleteAsync(
    ISWbemServices* This,
    IDispatch *objWbemSink,
    BSTR strObjectPath,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    IDispatch *objWbemAsyncContext )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static BSTR build_query_string( const WCHAR *class )
{
    UINT len = lstrlenW(class) + ARRAY_SIZE(L"SELECT * FROM ");
    BSTR ret;

    if (!(ret = SysAllocStringLen( NULL, len ))) return NULL;
    lstrcpyW( ret, L"SELECT * FROM " );
    lstrcatW( ret, class );
    return ret;
}

static HRESULT WINAPI services_InstancesOf(
    ISWbemServices *iface,
    BSTR strClass,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    ISWbemObjectSet **objWbemObjectSet )
{
    BSTR query, wql = SysAllocString( L"WQL" );
    HRESULT hr;

    TRACE( "%p, %s, %#lx, %p, %p\n", iface, debugstr_w(strClass), iFlags, objWbemNamedValueSet,
           objWbemObjectSet );

    if (!(query = build_query_string( strClass )))
    {
        SysFreeString( wql );
        return E_OUTOFMEMORY;
    }
    hr = ISWbemServices_ExecQuery( iface, query, wql, iFlags, objWbemNamedValueSet, objWbemObjectSet );
    SysFreeString( wql );
    SysFreeString( query );
    return hr;
}

static HRESULT WINAPI services_InstancesOfAsync(
    ISWbemServices *iface,
    IDispatch *objWbemSink,
    BSTR strClass,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    IDispatch *objWbemAsyncContext )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_SubclassesOf(
    ISWbemServices *iface,
    BSTR strSuperclass,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    ISWbemObjectSet **objWbemObjectSet )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_SubclassesOfAsync(
    ISWbemServices *iface,
    IDispatch *objWbemSink,
    BSTR strSuperclass,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    IDispatch *objWbemAsyncContext )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_ExecQuery(
    ISWbemServices *iface,
    BSTR strQuery,
    BSTR strQueryLanguage,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    ISWbemObjectSet **objWbemObjectSet )
{
    struct services *services = impl_from_ISWbemServices( iface );
    IEnumWbemClassObject *iter;
    HRESULT hr;

    TRACE( "%p, %s, %s, %#lx, %p, %p\n", iface, debugstr_w(strQuery), debugstr_w(strQueryLanguage),
           iFlags, objWbemNamedValueSet, objWbemObjectSet );

    if (objWbemNamedValueSet) FIXME( "ignoring context\n" );

    hr = IWbemServices_ExecQuery( services->services, strQueryLanguage, strQuery, iFlags, NULL, &iter );
    if (hr != S_OK) return hr;

    hr = SWbemObjectSet_create( services, iter, objWbemObjectSet );
    IEnumWbemClassObject_Release( iter );
    return hr;
}

static HRESULT WINAPI services_ExecQueryAsync(
    ISWbemServices *iface,
    IDispatch *objWbemSink,
    BSTR strQuery,
    BSTR strQueryLanguage,
    LONG lFlags,
    IDispatch *objWbemNamedValueSet,
    IDispatch *objWbemAsyncContext )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_AssociatorsOf(
    ISWbemServices *iface,
    BSTR strObjectPath,
    BSTR strAssocClass,
    BSTR strResultClass,
    BSTR strResultRole,
    BSTR strRole,
    VARIANT_BOOL bClassesOnly,
    VARIANT_BOOL bSchemaOnly,
    BSTR strRequiredAssocQualifier,
    BSTR strRequiredQualifier,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    ISWbemObjectSet **objWbemObjectSet )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_AssociatorsOfAsync(
    ISWbemServices *iface,
    IDispatch *objWbemSink,
    BSTR strObjectPath,
    BSTR strAssocClass,
    BSTR strResultClass,
    BSTR strResultRole,
    BSTR strRole,
    VARIANT_BOOL bClassesOnly,
    VARIANT_BOOL bSchemaOnly,
    BSTR strRequiredAssocQualifier,
    BSTR strRequiredQualifier,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    IDispatch *objWbemAsyncContext )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_ReferencesTo(
    ISWbemServices *iface,
    BSTR strObjectPath,
    BSTR strResultClass,
    BSTR strRole,
    VARIANT_BOOL bClassesOnly,
    VARIANT_BOOL bSchemaOnly,
    BSTR strRequiredQualifier,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    ISWbemObjectSet **objWbemObjectSet )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_ReferencesToAsync(
    ISWbemServices *iface,
    IDispatch *objWbemSink,
    BSTR strObjectPath,
    BSTR strResultClass,
    BSTR strRole,
    VARIANT_BOOL bClassesOnly,
    VARIANT_BOOL bSchemaOnly,
    BSTR strRequiredQualifier,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    IDispatch *objWbemAsyncContext )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_ExecNotificationQuery(
    ISWbemServices *iface,
    BSTR strQuery,
    BSTR strQueryLanguage,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    ISWbemEventSource **objWbemEventSource )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_ExecNotificationQueryAsync(
    ISWbemServices *iface,
    IDispatch *objWbemSink,
    BSTR strQuery,
    BSTR strQueryLanguage,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    IDispatch *objWbemAsyncContext )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_ExecMethod(
    ISWbemServices *iface,
    BSTR path,
    BSTR method,
    IDispatch *in_sparams,
    LONG flags,
    IDispatch *valueset,
    ISWbemObject **out_sparams )
{
    struct services *services = impl_from_ISWbemServices( iface );
    IWbemClassObject *out_params = NULL;
    struct object *in_params;
    IWbemContext *context;
    HRESULT hr;

    TRACE( "%p, %s, %s, %p, %#lx, %p, %p\n", services, debugstr_w(path), debugstr_w(method), in_sparams,
            flags, valueset, out_sparams );

    in_params = unsafe_object_impl_from_IDispatch( in_sparams );
    out_params = NULL;

    context = unsafe_get_context_from_namedvalueset( valueset );

    hr = IWbemServices_ExecMethod( services->services, path, method, flags, context, in_params ? in_params->object : NULL,
            out_sparams ? &out_params : NULL, NULL );

    if (SUCCEEDED(hr) && out_params)
    {
        hr = SWbemObject_create( services, out_params, out_sparams );
        IWbemClassObject_Release( out_params );
    }

    return hr;
}

static HRESULT WINAPI services_ExecMethodAsync(
    ISWbemServices *iface,
    IDispatch *objWbemSink,
    BSTR strObjectPath,
    BSTR strMethodName,
    IDispatch *objWbemInParameters,
    LONG iFlags,
    IDispatch *objWbemNamedValueSet,
    IDispatch *objWbemAsyncContext )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI services_get_Security_(
        ISWbemServices *iface,
        ISWbemSecurity **objWbemSecurity )
{
    TRACE( "%p, %p\n", iface, objWbemSecurity );

    if (!objWbemSecurity)
        return E_INVALIDARG;

    return ISWbemSecurity_create( objWbemSecurity );
}

static const ISWbemServicesVtbl services_vtbl =
{
    services_QueryInterface,
    services_AddRef,
    services_Release,
    services_GetTypeInfoCount,
    services_GetTypeInfo,
    services_GetIDsOfNames,
    services_Invoke,
    services_Get,
    services_GetAsync,
    services_Delete,
    services_DeleteAsync,
    services_InstancesOf,
    services_InstancesOfAsync,
    services_SubclassesOf,
    services_SubclassesOfAsync,
    services_ExecQuery,
    services_ExecQueryAsync,
    services_AssociatorsOf,
    services_AssociatorsOfAsync,
    services_ReferencesTo,
    services_ReferencesToAsync,
    services_ExecNotificationQuery,
    services_ExecNotificationQueryAsync,
    services_ExecMethod,
    services_ExecMethodAsync,
    services_get_Security_
};

static HRESULT SWbemServices_create( IWbemServices *wbem_services, ISWbemServices **obj )
{
    struct services *services;

    TRACE( "%p, %p\n", obj, wbem_services );

    if (!(services = malloc( sizeof(*services) ))) return E_OUTOFMEMORY;
    services->ISWbemServices_iface.lpVtbl = &services_vtbl;
    services->refs = 1;
    services->services = wbem_services;
    IWbemServices_AddRef( services->services );

    *obj = &services->ISWbemServices_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}

struct locator
{
    ISWbemLocator ISWbemLocator_iface;
    LONG refs;
    IWbemLocator *locator;
};

static inline struct locator *impl_from_ISWbemLocator( ISWbemLocator *iface )
{
    return CONTAINING_RECORD( iface, struct locator, ISWbemLocator_iface );
}

static ULONG WINAPI locator_AddRef(
    ISWbemLocator *iface )
{
    struct locator *locator = impl_from_ISWbemLocator( iface );
    return InterlockedIncrement( &locator->refs );
}

static ULONG WINAPI locator_Release(
    ISWbemLocator *iface )
{
    struct locator *locator = impl_from_ISWbemLocator( iface );
    LONG refs = InterlockedDecrement( &locator->refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", locator );
        if (locator->locator)
            IWbemLocator_Release( locator->locator );
        free( locator );
    }
    return refs;
}

static HRESULT WINAPI locator_QueryInterface(
    ISWbemLocator *iface,
    REFIID riid,
    void **ppvObject )
{
    struct locator *locator = impl_from_ISWbemLocator( iface );

    TRACE( "%p, %s, %p\n", locator, debugstr_guid( riid ), ppvObject );

    if (IsEqualGUID( riid, &IID_ISWbemLocator ) ||
        IsEqualGUID( riid, &IID_IDispatch ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *ppvObject = iface;
    }
    else
    {
        WARN( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }
    ISWbemLocator_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI locator_GetTypeInfoCount(
    ISWbemLocator *iface,
    UINT *count )
{
    struct locator *locator = impl_from_ISWbemLocator( iface );

    TRACE( "%p, %p\n", locator, count );
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI locator_GetTypeInfo(
    ISWbemLocator *iface,
    UINT index,
    LCID lcid,
    ITypeInfo **info )
{
    struct locator *locator = impl_from_ISWbemLocator( iface );
    TRACE( "%p, %u, %#lx, %p\n", locator, index, lcid, info );

    return get_typeinfo( ISWbemLocator_tid, info );
}

static HRESULT WINAPI locator_GetIDsOfNames(
    ISWbemLocator *iface,
    REFIID riid,
    LPOLESTR *names,
    UINT count,
    LCID lcid,
    DISPID *dispid )
{
    struct locator *locator = impl_from_ISWbemLocator( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %s, %p, %u, %#lx, %p\n", locator, debugstr_guid(riid), names, count, lcid, dispid );

    if (!names || !count || !dispid) return E_INVALIDARG;

    hr = get_typeinfo( ISWbemLocator_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, names, count, dispid );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI locator_Invoke(
    ISWbemLocator *iface,
    DISPID member,
    REFIID riid,
    LCID lcid,
    WORD flags,
    DISPPARAMS *params,
    VARIANT *result,
    EXCEPINFO *excep_info,
    UINT *arg_err )
{
    struct locator *locator = impl_from_ISWbemLocator( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %ld, %s, %#lx, %#x, %p, %p, %p, %p\n", locator, member, debugstr_guid(riid),
           lcid, flags, params, result, excep_info, arg_err );

    hr = get_typeinfo( ISWbemLocator_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &locator->ISWbemLocator_iface, member, flags,
                               params, result, excep_info, arg_err );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static BSTR build_resource_string( BSTR server, BSTR namespace )
{
    ULONG len, len_server = 0, len_namespace = 0;
    BSTR ret;

    if (server && *server) len_server = lstrlenW( server );
    else len_server = 1;
    if (namespace && *namespace) len_namespace = lstrlenW( namespace );
    else len_namespace = ARRAY_SIZE(L"root\\default") - 1;

    if (!(ret = SysAllocStringLen( NULL, 2 + len_server + 1 + len_namespace ))) return NULL;

    ret[0] = ret[1] = '\\';
    if (server && *server) lstrcpyW( ret + 2, server );
    else ret[2] = '.';

    len = len_server + 2;
    ret[len++] = '\\';

    if (namespace && *namespace) lstrcpyW( ret + len, namespace );
    else lstrcpyW( ret + len, L"root\\default" );
    return ret;
}

static HRESULT WINAPI locator_ConnectServer(
    ISWbemLocator *iface,
    BSTR strServer,
    BSTR strNamespace,
    BSTR strUser,
    BSTR strPassword,
    BSTR strLocale,
    BSTR strAuthority,
    LONG iSecurityFlags,
    IDispatch *objWbemNamedValueSet,
    ISWbemServices **objWbemServices )
{
    struct locator *locator = impl_from_ISWbemLocator( iface );
    IWbemServices *services;
    IWbemContext *context;
    BSTR resource;
    HRESULT hr;

    TRACE( "%p, %s, %s, %s, %p, %s, %s, %#lx, %p, %p\n", iface, debugstr_w(strServer),
           debugstr_w(strNamespace), debugstr_w(strUser), strPassword, debugstr_w(strLocale),
           debugstr_w(strAuthority), iSecurityFlags, objWbemNamedValueSet, objWbemServices );

    if (!locator->locator)
    {
        hr = CoCreateInstance( &CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, &IID_IWbemLocator,
                               (void **)&locator->locator );
        if (hr != S_OK) return hr;
    }

    context = unsafe_get_context_from_namedvalueset( objWbemNamedValueSet );

    if (!(resource = build_resource_string( strServer, strNamespace ))) return E_OUTOFMEMORY;
    hr = IWbemLocator_ConnectServer( locator->locator, resource, strUser, strPassword, strLocale,
                                     iSecurityFlags, strAuthority, context, &services );
    SysFreeString( resource );
    if (hr != S_OK) return hr;

    hr = SWbemServices_create( services, objWbemServices );
    IWbemServices_Release( services );
    return hr;
}

static HRESULT WINAPI locator_get_Security_(
    ISWbemLocator *iface,
    ISWbemSecurity **objWbemSecurity )
{
    TRACE( "%p, %p\n", iface, objWbemSecurity );

    if (!objWbemSecurity)
        return E_INVALIDARG;

    return ISWbemSecurity_create( objWbemSecurity );
}

static const ISWbemLocatorVtbl locator_vtbl =
{
    locator_QueryInterface,
    locator_AddRef,
    locator_Release,
    locator_GetTypeInfoCount,
    locator_GetTypeInfo,
    locator_GetIDsOfNames,
    locator_Invoke,
    locator_ConnectServer,
    locator_get_Security_
};

HRESULT SWbemLocator_create( void **obj )
{
    struct locator *locator;

    TRACE( "%p\n", obj );

    if (!(locator = malloc( sizeof(*locator) ))) return E_OUTOFMEMORY;
    locator->ISWbemLocator_iface.lpVtbl = &locator_vtbl;
    locator->refs = 1;
    locator->locator = NULL;

    *obj = &locator->ISWbemLocator_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}

struct security
{
    ISWbemSecurity ISWbemSecurity_iface;
    LONG refs;
    WbemImpersonationLevelEnum  implevel;
    WbemAuthenticationLevelEnum authlevel;
};

static inline struct security *impl_from_ISWbemSecurity( ISWbemSecurity *iface )
{
    return CONTAINING_RECORD( iface, struct security, ISWbemSecurity_iface );
}

static ULONG WINAPI security_AddRef(
    ISWbemSecurity *iface )
{
    struct security *security = impl_from_ISWbemSecurity( iface );
    return InterlockedIncrement( &security->refs );
}

static ULONG WINAPI security_Release(
    ISWbemSecurity *iface )
{
    struct security *security = impl_from_ISWbemSecurity( iface );
    LONG refs = InterlockedDecrement( &security->refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", security );
        free( security );
    }
    return refs;
}

static HRESULT WINAPI security_QueryInterface(
    ISWbemSecurity *iface,
    REFIID riid,
    void **ppvObject )
{
    struct security *security = impl_from_ISWbemSecurity( iface );
    TRACE( "%p, %s, %p\n", security, debugstr_guid( riid ), ppvObject );

    if (IsEqualGUID( riid, &IID_ISWbemSecurity ) ||
        IsEqualGUID( riid, &IID_IDispatch ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *ppvObject = iface;
    }
    else
    {
        WARN( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }
    ISWbemSecurity_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI security_GetTypeInfoCount(
    ISWbemSecurity *iface,
    UINT *count )
{
    struct security *security = impl_from_ISWbemSecurity( iface );
    TRACE( "%p, %p\n", security, count );

    *count = 1;
    return S_OK;
}

static HRESULT WINAPI security_GetTypeInfo(
    ISWbemSecurity *iface,
    UINT index,
    LCID lcid,
    ITypeInfo **info )
{
    struct security *security = impl_from_ISWbemSecurity( iface );
    TRACE( "%p, %u, %#lx, %p\n", security, index, lcid, info );

    return get_typeinfo( ISWbemSecurity_tid, info );
}

static HRESULT WINAPI security_GetIDsOfNames(
    ISWbemSecurity *iface,
    REFIID riid,
    LPOLESTR *names,
    UINT count,
    LCID lcid,
    DISPID *dispid )
{
    struct security *security = impl_from_ISWbemSecurity( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %s, %p, %u, %#lx, %p\n", security, debugstr_guid(riid), names, count, lcid, dispid );

    if (!names || !count || !dispid) return E_INVALIDARG;

    hr = get_typeinfo( ISWbemSecurity_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, names, count, dispid );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI security_Invoke(
    ISWbemSecurity *iface,
    DISPID member,
    REFIID riid,
    LCID lcid,
    WORD flags,
    DISPPARAMS *params,
    VARIANT *result,
    EXCEPINFO *excep_info,
    UINT *arg_err )
{
    struct security *security = impl_from_ISWbemSecurity( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %ld, %s, %#lx, %#x, %p, %p, %p, %p\n", security, member, debugstr_guid(riid),
           lcid, flags, params, result, excep_info, arg_err );

    hr = get_typeinfo( ISWbemSecurity_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &security->ISWbemSecurity_iface, member, flags,
                               params, result, excep_info, arg_err );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI security_get_ImpersonationLevel(
    ISWbemSecurity *iface,
    WbemImpersonationLevelEnum *impersonation_level )
{
    struct security *security = impl_from_ISWbemSecurity( iface );
    FIXME( "%p, %p: stub\n", security, impersonation_level );

    if (!impersonation_level)
        return E_INVALIDARG;

    *impersonation_level = security->implevel;
    return S_OK;
}

static HRESULT WINAPI security_put_ImpersonationLevel(
    ISWbemSecurity *iface,
    WbemImpersonationLevelEnum impersonation_level )
{
    struct security *security = impl_from_ISWbemSecurity( iface );
    FIXME( "%p, %d: stub\n", security, impersonation_level );

    security->implevel = impersonation_level;
    return S_OK;
}

static HRESULT WINAPI security_get_AuthenticationLevel(
    ISWbemSecurity *iface,
    WbemAuthenticationLevelEnum *authentication_level )
{
    struct security *security = impl_from_ISWbemSecurity( iface );
    FIXME( "%p, %p: stub\n", security, authentication_level );

    if (!authentication_level)
        return E_INVALIDARG;

    *authentication_level = security->authlevel;
    return S_OK;
}

static HRESULT WINAPI security_put_AuthenticationLevel(
    ISWbemSecurity *iface,
    WbemAuthenticationLevelEnum authentication_level )
{
    struct security *security = impl_from_ISWbemSecurity( iface );
    FIXME( "%p, %d: stub\n", security, authentication_level );

    security->authlevel = authentication_level;
    return S_OK;
}

static HRESULT WINAPI security_get_Privileges(
    ISWbemSecurity *iface,
    ISWbemPrivilegeSet **privilege_set )
{
    struct security *security = impl_from_ISWbemSecurity( iface );
    FIXME( "%p, %p: stub\n", security, privilege_set );

    if (!privilege_set)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static const ISWbemSecurityVtbl security_vtbl =
{
    security_QueryInterface,
    security_AddRef,
    security_Release,
    security_GetTypeInfoCount,
    security_GetTypeInfo,
    security_GetIDsOfNames,
    security_Invoke,
    security_get_ImpersonationLevel,
    security_put_ImpersonationLevel,
    security_get_AuthenticationLevel,
    security_put_AuthenticationLevel,
    security_get_Privileges
};

static HRESULT ISWbemSecurity_create( ISWbemSecurity **obj )
{
    struct security *security;

    TRACE( "%p\n", obj );

    if (!(security = malloc( sizeof(*security) ))) return E_OUTOFMEMORY;
    security->ISWbemSecurity_iface.lpVtbl = &security_vtbl;
    security->refs = 1;
    security->implevel = wbemImpersonationLevelImpersonate;
    security->authlevel = wbemAuthenticationLevelPktPrivacy;

    *obj = &security->ISWbemSecurity_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}

struct namedvalue
{
    ISWbemNamedValue ISWbemNamedValue_iface;
    LONG refs;
};

static struct namedvalueset *impl_from_ISWbemNamedValueSet( ISWbemNamedValueSet *iface )
{
    return CONTAINING_RECORD( iface, struct namedvalueset, ISWbemNamedValueSet_iface );
}

static struct namedvalue *impl_from_ISWbemNamedValue( ISWbemNamedValue *iface )
{
    return CONTAINING_RECORD( iface, struct namedvalue, ISWbemNamedValue_iface );
}

static HRESULT WINAPI namedvalue_QueryInterface(
    ISWbemNamedValue *iface,
    REFIID riid,
    void **ppvObject )
{
    struct namedvalue *value = impl_from_ISWbemNamedValue( iface );

    TRACE( "%p, %s, %p\n", value, debugstr_guid( riid ), ppvObject );

    if (IsEqualGUID( riid, &IID_ISWbemNamedValue ) ||
        IsEqualGUID( riid, &IID_IDispatch ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *ppvObject = iface;
    }
    else
    {
        WARN( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }
    ISWbemNamedValue_AddRef( iface );
    return S_OK;
}

static ULONG WINAPI namedvalue_AddRef(
    ISWbemNamedValue *iface )
{
    struct namedvalue *value = impl_from_ISWbemNamedValue( iface );
    return InterlockedIncrement( &value->refs );
}

static ULONG WINAPI namedvalue_Release(
    ISWbemNamedValue *iface )
{
    struct namedvalue *value = impl_from_ISWbemNamedValue( iface );
    LONG refs = InterlockedDecrement( &value->refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", value );
        free( value );
    }
    return refs;
}

static HRESULT WINAPI namedvalue_GetTypeInfoCount(
    ISWbemNamedValue *iface,
    UINT *count )
{
    struct namedvalue *value = impl_from_ISWbemNamedValue( iface );
    TRACE( "%p, %p\n", value, count );

    *count = 1;
    return S_OK;
}

static HRESULT WINAPI namedvalue_GetTypeInfo(
    ISWbemNamedValue *iface,
    UINT index,
    LCID lcid,
    ITypeInfo **info )
{
    struct namedvalue *value = impl_from_ISWbemNamedValue( iface );

    TRACE( "%p, %u, %#lx, %p\n", value, index, lcid, info );

    return get_typeinfo( ISWbemNamedValue_tid, info );
}

static HRESULT WINAPI namedvalue_GetIDsOfNames(
    ISWbemNamedValue *iface,
    REFIID riid,
    LPOLESTR *names,
    UINT count,
    LCID lcid,
    DISPID *dispid )
{
    struct namedvalue *value = impl_from_ISWbemNamedValue( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %s, %p, %u, %#lx, %p\n", value, debugstr_guid(riid), names, count, lcid, dispid );

    if (!names || !count || !dispid) return E_INVALIDARG;

    hr = get_typeinfo( ISWbemNamedValue_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, names, count, dispid );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI namedvalue_Invoke(
    ISWbemNamedValue *iface,
    DISPID member,
    REFIID riid,
    LCID lcid,
    WORD flags,
    DISPPARAMS *params,
    VARIANT *result,
    EXCEPINFO *excep_info,
    UINT *arg_err )
{
    struct namedvalue *set = impl_from_ISWbemNamedValue( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %ld, %s, %#lx, %#x, %p, %p, %p, %p\n", set, member, debugstr_guid(riid),
           lcid, flags, params, result, excep_info, arg_err );

    hr = get_typeinfo( ISWbemNamedValue_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &set->ISWbemNamedValue_iface, member, flags,
                               params, result, excep_info, arg_err );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI namedvalue_get_Value(
    ISWbemNamedValue *iface,
    VARIANT *var )
{
    FIXME("\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI namedvalue_put_Value(
    ISWbemNamedValue *iface,
    VARIANT *var )
{
    FIXME("\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI namedvalue_get_Name(
    ISWbemNamedValue *iface,
    BSTR *name )
{
    FIXME("\n");

    return E_NOTIMPL;
}

static const ISWbemNamedValueVtbl namedvaluevtbl =
{
    namedvalue_QueryInterface,
    namedvalue_AddRef,
    namedvalue_Release,
    namedvalue_GetTypeInfoCount,
    namedvalue_GetTypeInfo,
    namedvalue_GetIDsOfNames,
    namedvalue_Invoke,
    namedvalue_get_Value,
    namedvalue_put_Value,
    namedvalue_get_Name
};

static HRESULT namedvalue_create( ISWbemNamedValue **value )
{
    struct namedvalue *object;

    if (!(object = malloc( sizeof(*object) ))) return E_OUTOFMEMORY;

    object->ISWbemNamedValue_iface.lpVtbl = &namedvaluevtbl;
    object->refs = 1;

    *value = &object->ISWbemNamedValue_iface;

    return S_OK;
}

static HRESULT WINAPI namedvalueset_QueryInterface(
    ISWbemNamedValueSet *iface,
    REFIID riid,
    void **ppvObject )
{
    struct namedvalueset *set = impl_from_ISWbemNamedValueSet( iface );

    TRACE( "%p, %s, %p\n", set, debugstr_guid( riid ), ppvObject );

    if (IsEqualGUID( riid, &IID_ISWbemNamedValueSet ) ||
        IsEqualGUID( riid, &IID_IDispatch ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *ppvObject = iface;
    }
    else
    {
        WARN( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }
    ISWbemNamedValueSet_AddRef( iface );
    return S_OK;
}

static ULONG WINAPI namedvalueset_AddRef(
    ISWbemNamedValueSet *iface )
{
    struct namedvalueset *set = impl_from_ISWbemNamedValueSet( iface );
    return InterlockedIncrement( &set->refs );
}

static ULONG WINAPI namedvalueset_Release(
    ISWbemNamedValueSet *iface )
{
    struct namedvalueset *set = impl_from_ISWbemNamedValueSet( iface );
    LONG refs = InterlockedDecrement( &set->refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", set );
        if (set->context)
            IWbemContext_Release( set->context );
        free( set );
    }
    return refs;
}

static HRESULT WINAPI namedvalueset_GetTypeInfoCount(
    ISWbemNamedValueSet *iface,
    UINT *count )
{
    struct namedvalueset *set = impl_from_ISWbemNamedValueSet( iface );
    TRACE( "%p, %p\n", set, count );

    *count = 1;
    return S_OK;
}

static HRESULT WINAPI namedvalueset_GetTypeInfo(
    ISWbemNamedValueSet *iface,
    UINT index,
    LCID lcid,
    ITypeInfo **info )
{
    struct namedvalueset *set = impl_from_ISWbemNamedValueSet( iface );

    TRACE( "%p, %u, %#lx, %p\n", set, index, lcid, info );

    return get_typeinfo( ISWbemNamedValueSet_tid, info );
}

static HRESULT WINAPI namedvalueset_GetIDsOfNames(
    ISWbemNamedValueSet *iface,
    REFIID riid,
    LPOLESTR *names,
    UINT count,
    LCID lcid,
    DISPID *dispid )
{
    struct namedvalueset *set = impl_from_ISWbemNamedValueSet( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %s, %p, %u, %#lx, %p\n", set, debugstr_guid(riid), names, count, lcid, dispid );

    if (!names || !count || !dispid) return E_INVALIDARG;

    hr = get_typeinfo( ISWbemNamedValueSet_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, names, count, dispid );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI namedvalueset_Invoke(
    ISWbemNamedValueSet *iface,
    DISPID member,
    REFIID riid,
    LCID lcid,
    WORD flags,
    DISPPARAMS *params,
    VARIANT *result,
    EXCEPINFO *excep_info,
    UINT *arg_err )
{
    struct namedvalueset *set = impl_from_ISWbemNamedValueSet( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE( "%p, %ld, %s, %#lx, %#x, %p, %p, %p, %p\n", set, member, debugstr_guid(riid),
           lcid, flags, params, result, excep_info, arg_err );

    hr = get_typeinfo( ISWbemNamedValueSet_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &set->ISWbemNamedValueSet_iface, member, flags,
                               params, result, excep_info, arg_err );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI namedvalueset_get__NewEnum(
    ISWbemNamedValueSet *iface,
    IUnknown **unk )
{
    FIXME("\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI namedvalueset_Item(
    ISWbemNamedValueSet *iface,
    BSTR name,
    LONG flags,
    ISWbemNamedValue **value )
{
    struct namedvalueset *set = impl_from_ISWbemNamedValueSet( iface );
    VARIANT var;
    HRESULT hr;

    TRACE("%p, %s, %#lx, %p\n", set, debugstr_w(name), flags, value);

    if (SUCCEEDED(hr = IWbemContext_GetValue( set->context, name, flags, &var )))
    {
        VariantClear( &var );
        hr = namedvalue_create( value );
    }

    return hr;
}

static HRESULT WINAPI namedvalueset_get_Count(
    ISWbemNamedValueSet *iface,
    LONG *count )
{
    FIXME("\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI namedvalueset_Add(
    ISWbemNamedValueSet *iface,
    BSTR name,
    VARIANT *var,
    LONG flags,
    ISWbemNamedValue **value )
{
    struct namedvalueset *set = impl_from_ISWbemNamedValueSet( iface );
    HRESULT hr;

    TRACE("%p, %s, %s, %#lx, %p\n", set, debugstr_w(name), debugstr_variant(var), flags, value);

    if (!name || !var || !value)
        return WBEM_E_INVALID_PARAMETER;

    if (SUCCEEDED(hr = IWbemContext_SetValue( set->context, name, flags, var )))
    {
        hr = namedvalue_create( value );
    }

    return hr;
}

static HRESULT WINAPI namedvalueset_Remove(
    ISWbemNamedValueSet *iface,
    BSTR name,
    LONG flags )
{
    struct namedvalueset *set = impl_from_ISWbemNamedValueSet( iface );

    TRACE("%p, %s, %#lx\n", set, debugstr_w(name), flags);

    return IWbemContext_DeleteValue( set->context, name, flags );
}

static HRESULT WINAPI namedvalueset_Clone(
    ISWbemNamedValueSet *iface,
    ISWbemNamedValueSet **valueset )
{
    FIXME("\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI namedvalueset_DeleteAll(
    ISWbemNamedValueSet *iface )
{
    struct namedvalueset *set = impl_from_ISWbemNamedValueSet( iface );

    TRACE("%p\n", set);

    return IWbemContext_DeleteAll( set->context );
}

static const ISWbemNamedValueSetVtbl namedvalueset_vtbl =
{
    namedvalueset_QueryInterface,
    namedvalueset_AddRef,
    namedvalueset_Release,
    namedvalueset_GetTypeInfoCount,
    namedvalueset_GetTypeInfo,
    namedvalueset_GetIDsOfNames,
    namedvalueset_Invoke,
    namedvalueset_get__NewEnum,
    namedvalueset_Item,
    namedvalueset_get_Count,
    namedvalueset_Add,
    namedvalueset_Remove,
    namedvalueset_Clone,
    namedvalueset_DeleteAll,
};

static struct namedvalueset *unsafe_valueset_impl_from_IDispatch(IDispatch *iface)
{
    if (!iface)
        return NULL;
    if (iface->lpVtbl != (IDispatchVtbl *)&namedvalueset_vtbl)
    {
        FIXME( "External implementations are not supported.\n" );
        return NULL;
    }
    return CONTAINING_RECORD(iface, struct namedvalueset, ISWbemNamedValueSet_iface);
}

HRESULT SWbemNamedValueSet_create( void **obj )
{
    struct namedvalueset *set;
    HRESULT hr;

    TRACE( "%p\n", obj );

    if (!(set = calloc( 1, sizeof(*set) ))) return E_OUTOFMEMORY;
    set->ISWbemNamedValueSet_iface.lpVtbl = &namedvalueset_vtbl;
    set->refs = 1;

    if (FAILED(hr = CoCreateInstance( &CLSID_WbemContext, NULL, CLSCTX_INPROC_SERVER, &IID_IWbemContext,
            (void **)&set->context )))
    {
        ISWbemNamedValueSet_Release( &set->ISWbemNamedValueSet_iface );
        return hr;
    }

    *obj = &set->ISWbemNamedValueSet_iface;
    TRACE( "returning iface %p\n", *obj );
    return hr;
}
