/*
 * StdRegProv implementation
 *
 * Copyright 2012 Hans Leidekker for CodeWeavers
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
#include "wbemcli.h"

#include "wine/debug.h"
#include "wbemprox_private.h"

#ifdef __REACTOS__
#include <winreg.h>
#define RRF_SUBKEY_WOW6464KEY 0x00010000
#define RRF_SUBKEY_WOW6432KEY 0x00020000
#endif

WINE_DEFAULT_DEBUG_CHANNEL(wbemprox);

static HRESULT to_bstr_array( BSTR *strings, DWORD count, VARIANT *var )
{
    SAFEARRAY *sa;
    HRESULT hr;
    LONG i;

    if (!(sa = SafeArrayCreateVector( VT_BSTR, 0, count ))) return E_OUTOFMEMORY;
    for (i = 0; i < count; i++)
    {
        if ((hr = SafeArrayPutElement( sa, &i, strings[i] )) != S_OK)
        {
            SafeArrayDestroy( sa );
            return hr;
        }
    }
    set_variant( VT_BSTR|VT_ARRAY, 0, sa, var );
    return S_OK;
}

static void free_bstr_array( BSTR *strings, DWORD count )
{
    while (count--)
        SysFreeString( *(strings++) );
}

static HRESULT to_i4_array( DWORD *values, DWORD count, VARIANT *var )
{
    SAFEARRAY *sa;
    HRESULT hr;
    LONG i;

    if (!(sa = SafeArrayCreateVector( VT_I4, 0, count ))) return E_OUTOFMEMORY;
    for (i = 0; i < count; i++)
    {
        if ((hr = SafeArrayPutElement( sa, &i, &values[i] )) != S_OK)
        {
            SafeArrayDestroy( sa );
            return hr;
        }
    }
    set_variant( VT_I4|VT_ARRAY, 0, sa, var );
    return S_OK;
}

static unsigned int get_access_mask( IWbemContext *context )
{
    VARIANT value;

    if (!context) return 0;

    V_VT( &value ) = VT_EMPTY;
    if (FAILED(IWbemContext_GetValue( context, L"__ProviderArchitecture", 0, &value )))
        return 0;

    if (FAILED(VariantChangeType( &value, &value, 0, VT_I4 )))
    {
        VariantClear( &value );
        return 0;
    }

    if (V_I4( &value ) == 32)
        return KEY_WOW64_32KEY;
    else if (V_I4( &value ) == 64)
        return KEY_WOW64_64KEY;

    return 0;
}

static HRESULT create_key( HKEY root, const WCHAR *subkey, IWbemContext *context, VARIANT *retval )
{
    LONG res;
    HKEY hkey;

    TRACE("%p, %s\n", root, debugstr_w(subkey));

    res = RegCreateKeyExW( root, subkey, 0, NULL, 0, get_access_mask( context ), NULL, &hkey, NULL );
    set_variant( VT_UI4, res, NULL, retval );
    if (!res)
    {
        RegCloseKey( hkey );
        return S_OK;
    }
    return HRESULT_FROM_WIN32( res );
}

HRESULT reg_create_key( IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out )
{
    VARIANT defkey, subkey, retval;
    IWbemClassObject *sig, *out_params = NULL;
    HRESULT hr;

    TRACE("%p, %p, %p, %p\n", obj, context, in, out);

    hr = IWbemClassObject_Get( in, L"hDefKey", 0, &defkey, NULL, NULL );
    if (hr != S_OK) return hr;
    hr = IWbemClassObject_Get( in, L"sSubKeyName", 0, &subkey, NULL, NULL );
    if (hr != S_OK) return hr;

    hr = create_signature( WBEMPROX_NAMESPACE_CIMV2, L"StdRegProv", L"CreateKey", PARAM_OUT, &sig );
    if (hr != S_OK)
    {
        VariantClear( &subkey );
        return hr;
    }
    if (out)
    {
        hr = IWbemClassObject_SpawnInstance( sig, 0, &out_params );
        if (hr != S_OK)
        {
            VariantClear( &subkey );
            IWbemClassObject_Release( sig );
            return hr;
        }
    }
    hr = create_key( (HKEY)(INT_PTR)V_I4(&defkey), V_BSTR(&subkey), context, &retval );
    if (hr == S_OK && out_params)
        hr = IWbemClassObject_Put( out_params, L"ReturnValue", 0, &retval, CIM_UINT32 );

    VariantClear( &subkey );
    IWbemClassObject_Release( sig );
    if (hr == S_OK && out)
    {
        *out = out_params;
        IWbemClassObject_AddRef( out_params );
    }
    if (out_params) IWbemClassObject_Release( out_params );
    return hr;
}

static HRESULT enum_key( HKEY root, const WCHAR *subkey, VARIANT *names, IWbemContext *context, VARIANT *retval )
{
    HKEY hkey;
    HRESULT hr = S_OK;
    WCHAR buf[256];
    BSTR *strings, *tmp;
    DWORD count = 2, len = ARRAY_SIZE( buf );
    LONG res, i = 0;

    TRACE("%p, %s\n", root, debugstr_w(subkey));

    if (!(strings = malloc( count * sizeof(BSTR) ))) return E_OUTOFMEMORY;
    if ((res = RegOpenKeyExW( root, subkey, 0, KEY_ENUMERATE_SUB_KEYS | get_access_mask( context ), &hkey )))
    {
        set_variant( VT_UI4, res, NULL, retval );
        free( strings );
        return S_OK;
    }
    for (;;)
    {
        if (i >= count)
        {
            count *= 2;
            if (!(tmp = realloc( strings, count * sizeof(BSTR) )))
            {
                RegCloseKey( hkey );
                free( strings );
                return E_OUTOFMEMORY;
            }
            strings = tmp;
        }
        if ((res = RegEnumKeyW( hkey, i, buf, len )) == ERROR_NO_MORE_ITEMS)
        {
            if (i) res = ERROR_SUCCESS;
            break;
        }
        if (res) break;
        if (!(strings[i] = SysAllocString( buf )))
        {
            for (i--; i >= 0; i--) SysFreeString( strings[i] );
            hr = E_OUTOFMEMORY;
            break;
        }
        i++;
    }
    if (hr == S_OK && !res)
    {
        hr = to_bstr_array( strings, i, names );
        free_bstr_array( strings, i );
    }
    set_variant( VT_UI4, res, NULL, retval );
    RegCloseKey( hkey );
    free( strings );
    return hr;
}

HRESULT reg_enum_key( IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out )
{
    VARIANT defkey, subkey, names, retval;
    IWbemClassObject *sig, *out_params = NULL;
    HRESULT hr;

    TRACE("%p, %p, %p, %p\n", obj, context, in, out);

    hr = IWbemClassObject_Get( in, L"hDefKey", 0, &defkey, NULL, NULL );
    if (hr != S_OK) return hr;
    hr = IWbemClassObject_Get( in, L"sSubKeyName", 0, &subkey, NULL, NULL );
    if (hr != S_OK) return hr;

    hr = create_signature( WBEMPROX_NAMESPACE_CIMV2, L"StdRegProv", L"EnumKey", PARAM_OUT, &sig );
    if (hr != S_OK)
    {
        VariantClear( &subkey );
        return hr;
    }
    if (out)
    {
        hr = IWbemClassObject_SpawnInstance( sig, 0, &out_params );
        if (hr != S_OK)
        {
            VariantClear( &subkey );
            IWbemClassObject_Release( sig );
            return hr;
        }
    }
    VariantInit( &names );
    hr = enum_key( (HKEY)(INT_PTR)V_I4(&defkey), V_BSTR(&subkey), &names, context, &retval );
    if (hr != S_OK) goto done;
    if (out_params)
    {
        if (!V_UI4( &retval ))
        {
            hr = IWbemClassObject_Put( out_params, L"sNames", 0, &names, CIM_STRING|CIM_FLAG_ARRAY );
            if (hr != S_OK) goto done;
        }
        hr = IWbemClassObject_Put( out_params, L"ReturnValue", 0, &retval, CIM_UINT32 );
    }

done:
    VariantClear( &names );
    VariantClear( &subkey );
    IWbemClassObject_Release( sig );
    if (hr == S_OK && out)
    {
        *out = out_params;
        IWbemClassObject_AddRef( out_params );
    }
    if (out_params) IWbemClassObject_Release( out_params );
    return hr;
}

static HRESULT enum_values( HKEY root, const WCHAR *subkey, VARIANT *names, VARIANT *types, IWbemContext *context,
                            VARIANT *retval )
{
    HKEY hkey = NULL;
    HRESULT hr = S_OK;
    BSTR *value_names = NULL;
    DWORD count, buflen, len, *value_types = NULL;
    LONG res, i = 0;
    WCHAR *buf = NULL;

    TRACE("%p, %s\n", root, debugstr_w(subkey));

    if ((res = RegOpenKeyExW( root, subkey, 0, KEY_QUERY_VALUE | get_access_mask( context ), &hkey ))) goto done;
    if ((res = RegQueryInfoKeyW( hkey, NULL, NULL, NULL, NULL, NULL, NULL, &count, &buflen, NULL, NULL, NULL )))
        goto done;

    hr = E_OUTOFMEMORY;
    if (!(buf = malloc( (buflen + 1) * sizeof(WCHAR) ))) goto done;
    if (!(value_names = malloc( count * sizeof(BSTR) ))) goto done;
    if (!(value_types = malloc( count * sizeof(DWORD) ))) goto done;

    hr = S_OK;
    for (;;)
    {
        len = buflen + 1;
        res = RegEnumValueW( hkey, i, buf, &len, NULL, &value_types[i], NULL, NULL );
        if (res == ERROR_NO_MORE_ITEMS)
        {
            if (i) res = ERROR_SUCCESS;
            break;
        }
        if (res) break;
        if (!(value_names[i] = SysAllocString( buf )))
        {
            for (i--; i >= 0; i--) SysFreeString( value_names[i] );
            hr = E_OUTOFMEMORY;
            break;
        }
        i++;
    }
    if (hr == S_OK && !res)
    {
        hr = to_bstr_array( value_names, i, names );
        free_bstr_array( value_names, i );
        if (hr == S_OK) hr = to_i4_array( value_types, i, types );
    }

done:
    set_variant( VT_UI4, res, NULL, retval );
    RegCloseKey( hkey );
    free( value_names );
    free( value_types );
    free( buf );
    return hr;
}

HRESULT reg_enum_values( IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out )
{
    VARIANT defkey, subkey, names, types, retval;
    IWbemClassObject *sig, *out_params = NULL;
    HRESULT hr;

    TRACE("%p, %p, %p, %p\n", obj, context, in, out);

    hr = IWbemClassObject_Get( in, L"hDefKey", 0, &defkey, NULL, NULL );
    if (hr != S_OK) return hr;
    hr = IWbemClassObject_Get( in, L"sSubKeyName", 0, &subkey, NULL, NULL );
    if (hr != S_OK) return hr;

    hr = create_signature( WBEMPROX_NAMESPACE_CIMV2, L"StdRegProv", L"EnumValues", PARAM_OUT, &sig );
    if (hr != S_OK)
    {
        VariantClear( &subkey );
        return hr;
    }
    if (out)
    {
        hr = IWbemClassObject_SpawnInstance( sig, 0, &out_params );
        if (hr != S_OK)
        {
            VariantClear( &subkey );
            IWbemClassObject_Release( sig );
            return hr;
        }
    }
    VariantInit( &names );
    VariantInit( &types );
    hr = enum_values( (HKEY)(INT_PTR)V_I4(&defkey), V_BSTR(&subkey), &names, &types, context, &retval );
    if (hr != S_OK) goto done;
    if (out_params)
    {
        if (!V_UI4( &retval ))
        {
            hr = IWbemClassObject_Put( out_params, L"sNames", 0, &names, CIM_STRING|CIM_FLAG_ARRAY );
            if (hr != S_OK) goto done;
            hr = IWbemClassObject_Put( out_params, L"Types", 0, &types, CIM_SINT32|CIM_FLAG_ARRAY );
            if (hr != S_OK) goto done;
        }
        hr = IWbemClassObject_Put( out_params, L"ReturnValue", 0, &retval, CIM_UINT32 );
    }

done:
    VariantClear( &types );
    VariantClear( &names );
    VariantClear( &subkey );
    IWbemClassObject_Release( sig );
    if (hr == S_OK && out)
    {
        *out = out_params;
        IWbemClassObject_AddRef( out_params );
    }
    if (out_params) IWbemClassObject_Release( out_params );
    return hr;
}

static HRESULT get_stringvalue( HKEY root, const WCHAR *subkey, const WCHAR *name, VARIANT *value,
                                IWbemContext *context, VARIANT *retval )
{
    DWORD size, mask, flags = RRF_RT_REG_SZ;
    HRESULT hr = S_OK;
    BSTR str = NULL;
    LONG res;

    TRACE("%p, %s, %s\n", root, debugstr_w(subkey), debugstr_w(name));

    mask = get_access_mask( context );

    if (mask & KEY_WOW64_64KEY)
        flags |= RRF_SUBKEY_WOW6464KEY;
    else if (mask & KEY_WOW64_32KEY)
        flags |= RRF_SUBKEY_WOW6432KEY;

    if ((res = RegGetValueW( root, subkey, name, flags, NULL, NULL, &size ))) goto done;
    if (!(str = SysAllocStringLen( NULL, size / sizeof(WCHAR) - 1 ))) return E_OUTOFMEMORY;
    if (!(res = RegGetValueW( root, subkey, name, flags, NULL, str, &size ))) set_variant( VT_BSTR, 0, str, value );

done:
    set_variant( VT_UI4, res, NULL, retval );
    if (res) SysFreeString( str );
    return hr;
}

HRESULT reg_get_stringvalue( IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out )
{
    VARIANT defkey, subkey, name, value, retval;
    IWbemClassObject *sig, *out_params = NULL;
    HRESULT hr;

    TRACE("%p, %p, %p, %p\n", obj, context, in, out);

    hr = IWbemClassObject_Get( in, L"hDefKey", 0, &defkey, NULL, NULL );
    if (hr != S_OK) return hr;
    hr = IWbemClassObject_Get( in, L"sSubKeyName", 0, &subkey, NULL, NULL );
    if (hr != S_OK) return hr;
    hr = IWbemClassObject_Get( in, L"sValueName", 0, &name, NULL, NULL );
    if (hr != S_OK)
    {
        VariantClear( &subkey );
        return hr;
    }

    hr = create_signature( WBEMPROX_NAMESPACE_CIMV2, L"StdRegProv", L"GetStringValue", PARAM_OUT, &sig );
    if (hr != S_OK)
    {
        VariantClear( &name );
        VariantClear( &subkey );
        return hr;
    }
    if (out)
    {
        hr = IWbemClassObject_SpawnInstance( sig, 0, &out_params );
        if (hr != S_OK)
        {
            VariantClear( &name );
            VariantClear( &subkey );
            IWbemClassObject_Release( sig );
            return hr;
        }
    }
    VariantInit( &value );
    hr = get_stringvalue( (HKEY)(INT_PTR)V_I4(&defkey), V_BSTR(&subkey), V_BSTR(&name), &value, context, &retval );
    if (hr != S_OK) goto done;
    if (out_params)
    {
        if (!V_UI4( &retval ))
        {
            hr = IWbemClassObject_Put( out_params, L"sValue", 0, &value, CIM_STRING );
            if (hr != S_OK) goto done;
        }
        hr = IWbemClassObject_Put( out_params, L"ReturnValue", 0, &retval, CIM_UINT32 );
    }

done:
    VariantClear( &name );
    VariantClear( &subkey );
    IWbemClassObject_Release( sig );
    if (hr == S_OK && out)
    {
        *out = out_params;
        IWbemClassObject_AddRef( out_params );
    }
    if (out_params) IWbemClassObject_Release( out_params );
    return hr;
}

static HRESULT to_ui1_array( BYTE *value, DWORD size, VARIANT *var )
{
    SAFEARRAY *sa;
    HRESULT hr;
    LONG i;

    if (!(sa = SafeArrayCreateVector( VT_UI1, 0, size ))) return E_OUTOFMEMORY;
    for (i = 0; i < size; i++)
    {
        if ((hr = SafeArrayPutElement( sa, &i, &value[i] )) != S_OK)
        {
            SafeArrayDestroy( sa );
            return hr;
        }
    }
    set_variant( VT_UI1|VT_ARRAY, 0, sa, var );
    return S_OK;
}

static HRESULT get_binaryvalue( HKEY root, const WCHAR *subkey, const WCHAR *name, VARIANT *value,
                                IWbemContext *context, VARIANT *retval )
{
    DWORD size, mask, flags = RRF_RT_REG_BINARY;
    HRESULT hr = S_OK;
    BYTE *buf = NULL;
    LONG res;

    TRACE("%p, %s, %s\n", root, debugstr_w(subkey), debugstr_w(name));

    mask = get_access_mask( context );

    if (mask & KEY_WOW64_64KEY)
        flags |= RRF_SUBKEY_WOW6464KEY;
    else if (mask & KEY_WOW64_32KEY)
        flags |= RRF_SUBKEY_WOW6432KEY;

    if ((res = RegGetValueW( root, subkey, name, flags, NULL, NULL, &size ))) goto done;
    if (!(buf = malloc( size ))) return E_OUTOFMEMORY;
    if (!(res = RegGetValueW( root, subkey, name, flags, NULL, buf, &size ))) hr = to_ui1_array( buf, size, value );

done:
    set_variant( VT_UI4, res, NULL, retval );
    free( buf );
    return hr;
}

HRESULT reg_get_binaryvalue( IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out )
{
    VARIANT defkey, subkey, name, value, retval;
    IWbemClassObject *sig, *out_params = NULL;
    HRESULT hr;

    TRACE("%p, %p, %p, %p\n", obj, context, in, out);

    hr = IWbemClassObject_Get( in, L"hDefKey", 0, &defkey, NULL, NULL );
    if (hr != S_OK) return hr;
    hr = IWbemClassObject_Get( in, L"sSubKeyName", 0, &subkey, NULL, NULL );
    if (hr != S_OK) return hr;
    hr = IWbemClassObject_Get( in, L"sValueName", 0, &name, NULL, NULL );
    if (hr != S_OK)
    {
        VariantClear( &subkey );
        return hr;
    }

    hr = create_signature( WBEMPROX_NAMESPACE_CIMV2, L"StdRegProv", L"GetBinaryValue", PARAM_OUT, &sig );
    if (hr != S_OK)
    {
        VariantClear( &name );
        VariantClear( &subkey );
        return hr;
    }
    if (out)
    {
        hr = IWbemClassObject_SpawnInstance( sig, 0, &out_params );
        if (hr != S_OK)
        {
            VariantClear( &name );
            VariantClear( &subkey );
            IWbemClassObject_Release( sig );
            return hr;
        }
    }
    VariantInit( &value );
    hr = get_binaryvalue( (HKEY)(INT_PTR)V_I4(&defkey), V_BSTR(&subkey), V_BSTR(&name), &value, context, &retval );
    if (hr != S_OK) goto done;
    if (out_params)
    {
        if (!V_UI4( &retval ))
        {
            hr = IWbemClassObject_Put( out_params, L"uValue", 0, &value, CIM_UINT8|CIM_FLAG_ARRAY );
            if (hr != S_OK) goto done;
        }
        hr = IWbemClassObject_Put( out_params, L"ReturnValue", 0, &retval, CIM_UINT32 );
    }

done:
    VariantClear( &name );
    VariantClear( &subkey );
    IWbemClassObject_Release( sig );
    if (hr == S_OK && out)
    {
        *out = out_params;
        IWbemClassObject_AddRef( out_params );
    }
    if (out_params) IWbemClassObject_Release( out_params );
    return hr;
}

static void set_stringvalue( HKEY root, const WCHAR *subkey, const WCHAR *name, const WCHAR *value,
                             IWbemContext *context, VARIANT *retval )
{
    HKEY hkey;
    LONG res;

    TRACE("%p, %s, %s, %s\n", root, debugstr_w(subkey), debugstr_w(name), debugstr_w(value));

    if ((res = RegOpenKeyExW( root, subkey, 0, KEY_SET_VALUE | get_access_mask( context ), &hkey )))
    {
        set_variant( VT_UI4, res, NULL, retval );
        return;
    }

    res = RegSetKeyValueW( hkey, NULL, name, REG_SZ, value, (lstrlenW( value ) + 1) * sizeof(*value) );
    set_variant( VT_UI4, res, NULL, retval );
    RegCloseKey( hkey );
}

HRESULT reg_set_stringvalue( IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out )
{
    VARIANT defkey, subkey, name, value, retval;
    IWbemClassObject *sig, *out_params = NULL;
    HRESULT hr;

    TRACE("%p, %p, %p, %p\n", obj, context, in, out);

    hr = IWbemClassObject_Get( in, L"hDefKey", 0, &defkey, NULL, NULL );
    if (hr != S_OK) return hr;
    hr = IWbemClassObject_Get( in, L"sSubKeyName", 0, &subkey, NULL, NULL );
    if (hr != S_OK) return hr;
    hr = IWbemClassObject_Get( in, L"sValueName", 0, &name, NULL, NULL );
    if (hr != S_OK)
    {
        VariantClear( &subkey );
        return hr;
    }
    hr = IWbemClassObject_Get( in, L"sValue", 0, &value, NULL, NULL );
    if (hr != S_OK)
    {
        VariantClear( &name );
        VariantClear( &subkey );
        return hr;
    }

    hr = create_signature( WBEMPROX_NAMESPACE_CIMV2, L"StdRegProv", L"SetStringValue", PARAM_OUT, &sig );
    if (hr != S_OK)
    {
        VariantClear( &name );
        VariantClear( &subkey );
        VariantClear( &value );
        return hr;
    }
    if (out)
    {
        hr = IWbemClassObject_SpawnInstance( sig, 0, &out_params );
        if (hr != S_OK)
        {
            VariantClear( &name );
            VariantClear( &subkey );
            VariantClear( &value );
            IWbemClassObject_Release( sig );
            return hr;
        }
    }

    set_stringvalue( (HKEY)(INT_PTR)V_I4(&defkey), V_BSTR(&subkey), V_BSTR(&name), V_BSTR(&value), context, &retval );
    if (out_params)
        hr = IWbemClassObject_Put( out_params, L"ReturnValue", 0, &retval, CIM_UINT32 );

    VariantClear( &name );
    VariantClear( &subkey );
    VariantClear( &value );
    IWbemClassObject_Release( sig );
    if (hr == S_OK && out)
    {
        *out = out_params;
        IWbemClassObject_AddRef( out_params );
    }
    if (out_params) IWbemClassObject_Release( out_params );
    return hr;
}

static void set_dwordvalue( HKEY root, const WCHAR *subkey, const WCHAR *name, DWORD value, IWbemContext *context,
                            VARIANT *retval )
{
    HKEY hkey;
    LONG res;

    TRACE( "%p, %s, %s, %#lx\n", root, debugstr_w(subkey), debugstr_w(name), value );

    if ((res = RegOpenKeyExW( root, subkey, 0, KEY_SET_VALUE | get_access_mask( context ), &hkey )))
    {
        set_variant( VT_UI4, res, NULL, retval );
        return;
    }

    res = RegSetKeyValueW( hkey, NULL, name, REG_DWORD, &value, sizeof(value) );
    set_variant( VT_UI4, res, NULL, retval );
    RegCloseKey( hkey );
}

HRESULT reg_set_dwordvalue( IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out )
{
    VARIANT defkey, subkey, name, value, retval;
    IWbemClassObject *sig, *out_params = NULL;
    HRESULT hr;

    TRACE("%p, %p, %p, %p\n", obj, context, in, out);

    hr = IWbemClassObject_Get( in, L"hDefKey", 0, &defkey, NULL, NULL );
    if (hr != S_OK) return hr;
    hr = IWbemClassObject_Get( in, L"sSubKeyName", 0, &subkey, NULL, NULL );
    if (hr != S_OK) return hr;
    hr = IWbemClassObject_Get( in, L"sValueName", 0, &name, NULL, NULL );
    if (hr != S_OK)
    {
        VariantClear( &subkey );
        return hr;
    }
    hr = IWbemClassObject_Get( in, L"uValue", 0, &value, NULL, NULL );
    if (hr != S_OK)
    {
        VariantClear( &name );
        VariantClear( &subkey );
        return hr;
    }

    hr = create_signature( WBEMPROX_NAMESPACE_CIMV2, L"StdRegProv", L"SetDWORDValue", PARAM_OUT, &sig );
    if (hr != S_OK)
    {
        VariantClear( &name );
        VariantClear( &subkey );
        return hr;
    }
    if (out)
    {
        hr = IWbemClassObject_SpawnInstance( sig, 0, &out_params );
        if (hr != S_OK)
        {
            VariantClear( &name );
            VariantClear( &subkey );
            IWbemClassObject_Release( sig );
            return hr;
        }
    }
    set_dwordvalue( (HKEY)(INT_PTR)V_I4(&defkey), V_BSTR(&subkey), V_BSTR(&name), V_UI4(&value), context, &retval );
    if (out_params)
        hr = IWbemClassObject_Put( out_params, L"ReturnValue", 0, &retval, CIM_UINT32 );

    VariantClear( &name );
    VariantClear( &subkey );
    IWbemClassObject_Release( sig );
    if (hr == S_OK && out)
    {
        *out = out_params;
        IWbemClassObject_AddRef( out_params );
    }
    if (out_params) IWbemClassObject_Release( out_params );
    return hr;
}

static void delete_key( HKEY root, const WCHAR *subkey, IWbemContext *context, VARIANT *retval )
{
    LONG res;

    TRACE("%p, %s\n", root, debugstr_w(subkey));

    res = RegDeleteKeyExW( root, subkey, get_access_mask( context ), 0 );
    set_variant( VT_UI4, res, NULL, retval );
}

HRESULT reg_delete_key( IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out )
{
    VARIANT defkey, subkey, retval;
    IWbemClassObject *sig, *out_params = NULL;
    HRESULT hr;

    TRACE("%p, %p, %p, %p\n", obj, context, in, out);

    hr = IWbemClassObject_Get( in, L"hDefKey", 0, &defkey, NULL, NULL );
    if (hr != S_OK) return hr;
    hr = IWbemClassObject_Get( in, L"sSubKeyName", 0, &subkey, NULL, NULL );
    if (hr != S_OK) return hr;

    hr = create_signature( WBEMPROX_NAMESPACE_CIMV2, L"StdRegProv", L"DeleteKey", PARAM_OUT, &sig );
    if (hr != S_OK)
    {
        VariantClear( &subkey );
        return hr;
    }
    if (out)
    {
        hr = IWbemClassObject_SpawnInstance( sig, 0, &out_params );
        if (hr != S_OK)
        {
            VariantClear( &subkey );
            IWbemClassObject_Release( sig );
            return hr;
        }
    }
    delete_key( (HKEY)(INT_PTR)V_I4(&defkey), V_BSTR(&subkey), context, &retval );
    if (out_params)
        hr = IWbemClassObject_Put( out_params, L"ReturnValue", 0, &retval, CIM_UINT32 );

    VariantClear( &subkey );
    IWbemClassObject_Release( sig );
    if (hr == S_OK && out)
    {
        *out = out_params;
        IWbemClassObject_AddRef( out_params );
    }
    if (out_params) IWbemClassObject_Release( out_params );
    return hr;
}
