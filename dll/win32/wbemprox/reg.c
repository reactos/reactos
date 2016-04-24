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

#include "wbemprox_private.h"

#include <winreg.h>

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

static HRESULT enum_key( HKEY root, const WCHAR *subkey, VARIANT *names, VARIANT *retval )
{
    HKEY hkey;
    HRESULT hr = S_OK;
    WCHAR buf[256];
    BSTR *strings, *tmp;
    DWORD count = 2, len = sizeof(buf)/sizeof(buf[0]);
    LONG res, i = 0;

    TRACE("%p, %s\n", root, debugstr_w(subkey));

    if (!(strings = heap_alloc( count * sizeof(BSTR) ))) return E_OUTOFMEMORY;
    if ((res = RegOpenKeyExW( root, subkey, 0, KEY_ENUMERATE_SUB_KEYS, &hkey )))
    {
        set_variant( VT_UI4, res, NULL, retval );
        heap_free( strings );
        return S_OK;
    }
    for (;;)
    {
        if (i >= count)
        {
            count *= 2;
            if (!(tmp = heap_realloc( strings, count * sizeof(BSTR) )))
            {
                RegCloseKey( hkey );
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
            hr = ERROR_OUTOFMEMORY;
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
    heap_free( strings );
    return hr;
}

HRESULT reg_enum_key( IWbemClassObject *obj, IWbemClassObject *in, IWbemClassObject **out )
{
    VARIANT defkey, subkey, names, retval;
    IWbemClassObject *sig, *out_params = NULL;
    HRESULT hr;

    TRACE("%p, %p\n", in, out);

    hr = IWbemClassObject_Get( in, param_defkeyW, 0, &defkey, NULL, NULL );
    if (hr != S_OK) return hr;
    hr = IWbemClassObject_Get( in, param_subkeynameW, 0, &subkey, NULL, NULL );
    if (hr != S_OK) return hr;

    hr = create_signature( class_stdregprovW, method_enumkeyW, PARAM_OUT, &sig );
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
    hr = enum_key( (HKEY)(INT_PTR)V_I4(&defkey), V_BSTR(&subkey), &names, &retval );
    if (hr != S_OK) goto done;
    if (out_params)
    {
        if (!V_UI4( &retval ))
        {
            hr = IWbemClassObject_Put( out_params, param_namesW, 0, &names, CIM_STRING|CIM_FLAG_ARRAY );
            if (hr != S_OK) goto done;
        }
        hr = IWbemClassObject_Put( out_params, param_returnvalueW, 0, &retval, CIM_UINT32 );
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

static HRESULT enum_values( HKEY root, const WCHAR *subkey, VARIANT *names, VARIANT *types, VARIANT *retval )
{
    HKEY hkey = NULL;
    HRESULT hr = S_OK;
    BSTR *value_names = NULL;
    DWORD count, buflen, len, *value_types = NULL;
    LONG res, i = 0;
    WCHAR *buf = NULL;

    TRACE("%p, %s\n", root, debugstr_w(subkey));

    if ((res = RegOpenKeyExW( root, subkey, 0, KEY_QUERY_VALUE, &hkey ))) goto done;
    if ((res = RegQueryInfoKeyW( hkey, NULL, NULL, NULL, NULL, NULL, NULL, &count, &buflen, NULL, NULL, NULL )))
        goto done;

    hr = E_OUTOFMEMORY;
    if (!(buf = heap_alloc( (buflen + 1) * sizeof(WCHAR) ))) goto done;
    if (!(value_names = heap_alloc( count * sizeof(BSTR) ))) goto done;
    if (!(value_types = heap_alloc( count * sizeof(DWORD) ))) goto done;

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
            hr = ERROR_OUTOFMEMORY;
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
    heap_free( value_names );
    heap_free( value_types );
    heap_free( buf );
    return hr;
}

HRESULT reg_enum_values( IWbemClassObject *obj, IWbemClassObject *in, IWbemClassObject **out )
{
    VARIANT defkey, subkey, names, types, retval;
    IWbemClassObject *sig, *out_params = NULL;
    HRESULT hr;

    TRACE("%p, %p\n", in, out);

    hr = IWbemClassObject_Get( in, param_defkeyW, 0, &defkey, NULL, NULL );
    if (hr != S_OK) return hr;
    hr = IWbemClassObject_Get( in, param_subkeynameW, 0, &subkey, NULL, NULL );
    if (hr != S_OK) return hr;

    hr = create_signature( class_stdregprovW, method_enumvaluesW, PARAM_OUT, &sig );
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
    hr = enum_values( (HKEY)(INT_PTR)V_I4(&defkey), V_BSTR(&subkey), &names, &types, &retval );
    if (hr != S_OK) goto done;
    if (out_params)
    {
        if (!V_UI4( &retval ))
        {
            hr = IWbemClassObject_Put( out_params, param_namesW, 0, &names, CIM_STRING|CIM_FLAG_ARRAY );
            if (hr != S_OK) goto done;
            hr = IWbemClassObject_Put( out_params, param_typesW, 0, &types, CIM_SINT32|CIM_FLAG_ARRAY );
            if (hr != S_OK) goto done;
        }
        hr = IWbemClassObject_Put( out_params, param_returnvalueW, 0, &retval, CIM_UINT32 );
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

static HRESULT get_stringvalue( HKEY root, const WCHAR *subkey, const WCHAR *name, VARIANT *value, VARIANT *retval )
{
    HRESULT hr = S_OK;
    BSTR str = NULL;
    DWORD size;
    LONG res;

    TRACE("%p, %s, %s\n", root, debugstr_w(subkey), debugstr_w(name));

    if ((res = RegGetValueW( root, subkey, name, RRF_RT_REG_SZ, NULL, NULL, &size ))) goto done;
    if (!(str = SysAllocStringLen( NULL, size / sizeof(WCHAR) - 1 )))
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    if (!(res = RegGetValueW( root, subkey, name, RRF_RT_REG_SZ, NULL, str, &size )))
        set_variant( VT_BSTR, 0, str, value );

done:
    set_variant( VT_UI4, res, NULL, retval );
    if (res) SysFreeString( str );
    return hr;
}

HRESULT reg_get_stringvalue( IWbemClassObject *obj, IWbemClassObject *in, IWbemClassObject **out )
{
    VARIANT defkey, subkey, name, value, retval;
    IWbemClassObject *sig, *out_params = NULL;
    HRESULT hr;

    TRACE("%p, %p\n", in, out);

    hr = IWbemClassObject_Get( in, param_defkeyW, 0, &defkey, NULL, NULL );
    if (hr != S_OK) return hr;
    hr = IWbemClassObject_Get( in, param_subkeynameW, 0, &subkey, NULL, NULL );
    if (hr != S_OK) return hr;
    hr = IWbemClassObject_Get( in, param_valuenameW, 0, &name, NULL, NULL );
    if (hr != S_OK) return hr;

    hr = create_signature( class_stdregprovW, method_getstringvalueW, PARAM_OUT, &sig );
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
    hr = get_stringvalue( (HKEY)(INT_PTR)V_I4(&defkey), V_BSTR(&subkey), V_BSTR(&name), &value, &retval );
    if (hr != S_OK) goto done;
    if (out_params)
    {
        if (!V_UI4( &retval ))
        {
            hr = IWbemClassObject_Put( out_params, param_valueW, 0, &value, CIM_STRING );
            if (hr != S_OK) goto done;
        }
        hr = IWbemClassObject_Put( out_params, param_returnvalueW, 0, &retval, CIM_UINT32 );
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
