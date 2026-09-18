/*
 * SystemRestore implementation
 *
 * Copyright 2020 Dmitry Timoshkov
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

WINE_DEFAULT_DEBUG_CHANNEL(wbemprox);

HRESULT sysrestore_create( IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out )
{
    FIXME("stub\n");
    return S_OK;
}

HRESULT sysrestore_disable( IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out )
{
    FIXME("stub\n");
    return S_OK;
}

HRESULT sysrestore_enable( IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out )
{
    VARIANT drive, retval;
    IWbemClassObject *sig, *out_params = NULL;
    HRESULT hr;

    TRACE("%p, %p, %p, %p\n", obj, context, in, out);

    hr = IWbemClassObject_Get( in, L"Drive", 0, &drive, NULL, NULL );
    if (hr != S_OK) return hr;

    hr = create_signature( WBEMPROX_NAMESPACE_CIMV2, L"SystemRestore", L"Enable", PARAM_OUT, &sig );
    if (hr != S_OK)
    {
        VariantClear( &drive );
        return hr;
    }

    if (out)
    {
        hr = IWbemClassObject_SpawnInstance( sig, 0, &out_params );
        if (hr != S_OK)
        {
            VariantClear( &drive );
            IWbemClassObject_Release( sig );
            return hr;
        }
    }

    FIXME("%s: stub\n", wine_dbgstr_variant(&drive));

    VariantClear( &drive );
    IWbemClassObject_Release( sig );

    if (out_params)
    {
        set_variant( VT_UI4, ERROR_SUCCESS, NULL, &retval );
        hr = IWbemClassObject_Put( out_params, L"ReturnValue", 0, &retval, CIM_UINT32 );
    }

    if (hr == S_OK && out)
    {
        *out = out_params;
        IWbemClassObject_AddRef( out_params );
    }
    if (out_params) IWbemClassObject_Release( out_params );

    return hr;
}

HRESULT sysrestore_get_last_status( IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out )
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

HRESULT sysrestore_restore( IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out )
{
    FIXME("stub\n");
    return S_OK;
}
