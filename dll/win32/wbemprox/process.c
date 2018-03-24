/*
 * Win32_Process methods implementation
 *
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

#include "config.h"
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wbemcli.h"

#include "wine/debug.h"
#include "wbemprox_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wbemprox);

static HRESULT get_owner( VARIANT *user, VARIANT *domain, VARIANT *retval )
{
    DWORD len;
    UINT error = 8;

    len = 0;
    GetUserNameW( NULL, &len );
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) goto done;
    if (!(V_BSTR( user ) = SysAllocStringLen( NULL, len - 1 ))) goto done;
    if (!GetUserNameW( V_BSTR( user ), &len )) goto done;
    V_VT( user ) = VT_BSTR;

    len = 0;
    GetComputerNameW( NULL, &len );
    if (GetLastError() != ERROR_BUFFER_OVERFLOW) goto done;
    if (!(V_BSTR( domain ) = SysAllocStringLen( NULL, len - 1 ))) goto done;
    if (!GetComputerNameW( V_BSTR( domain ), &len )) goto done;
    V_VT( domain ) = VT_BSTR;

    error = 0;

done:
    if (error)
    {
        VariantClear( user );
        VariantClear( domain );
    }
    set_variant( VT_UI4, error, NULL, retval );
    return S_OK;
}

HRESULT process_get_owner( IWbemClassObject *obj, IWbemClassObject *in, IWbemClassObject **out )
{
    VARIANT user, domain, retval;
    IWbemClassObject *sig, *out_params = NULL;
    HRESULT hr;

    TRACE("%p, %p, %p\n", obj, in, out);

    hr = create_signature( class_processW, method_getownerW, PARAM_OUT, &sig );
    if (hr != S_OK) return hr;

    if (out)
    {
        hr = IWbemClassObject_SpawnInstance( sig, 0, &out_params );
        if (hr != S_OK)
        {
            IWbemClassObject_Release( sig );
            return hr;
        }
    }
    VariantInit( &user );
    VariantInit( &domain );
    hr = get_owner( &user, &domain, &retval );
    if (hr != S_OK) goto done;
    if (out_params)
    {
        if (!V_UI4( &retval ))
        {
            hr = IWbemClassObject_Put( out_params, param_userW, 0, &user, CIM_STRING );
            if (hr != S_OK) goto done;
            hr = IWbemClassObject_Put( out_params, param_domainW, 0, &domain, CIM_STRING );
            if (hr != S_OK) goto done;
        }
        hr = IWbemClassObject_Put( out_params, param_returnvalueW, 0, &retval, CIM_UINT32 );
    }

done:
    VariantClear( &user );
    VariantClear( &domain );
    IWbemClassObject_Release( sig );
    if (hr == S_OK && out)
    {
        *out = out_params;
        IWbemClassObject_AddRef( out_params );
    }
    if (out_params) IWbemClassObject_Release( out_params );
    return hr;
}
