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

HRESULT process_get_owner( IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out )
{
    VARIANT user, domain, retval;
    IWbemClassObject *sig, *out_params = NULL;
    HRESULT hr;

    TRACE("%p, %p, %p, %p\n", obj, context, in, out);

    hr = create_signature( WBEMPROX_NAMESPACE_CIMV2, L"Win32_Process", L"GetOwner", PARAM_OUT, &sig );
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
            hr = IWbemClassObject_Put( out_params, L"User", 0, &user, CIM_STRING );
            if (hr != S_OK) goto done;
            hr = IWbemClassObject_Put( out_params, L"Domain", 0, &domain, CIM_STRING );
            if (hr != S_OK) goto done;
        }
        hr = IWbemClassObject_Put( out_params, L"ReturnValue", 0, &retval, CIM_UINT32 );
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

HRESULT process_create( IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out )
{
    VARIANT command_line, current_directory, startup_info;
    HRESULT ret = WBEM_E_INVALID_PARAMETER, hr;
    IWbemClassObject *sig, *out_params = NULL;
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    UINT32 error;
    VARIANT v;
    BOOL bret;
    CIMTYPE type;

    FIXME("%p, %p, %p, %p stub\n", obj, context, in, out);

    *out = NULL;

    if ((hr = create_signature( WBEMPROX_NAMESPACE_CIMV2, L"Win32_Process", L"Create", PARAM_OUT, &sig ))) return hr;

    VariantInit( &command_line );
    VariantInit( &current_directory );
    VariantInit( &startup_info );

    if (FAILED(hr = IWbemClassObject_Get( in, L"CommandLine", 0, &command_line, &type, NULL ))
            || V_VT( &command_line ) != VT_BSTR)
        WARN( "invalid CommandLine, hr %#lx, type %u\n", hr, V_VT( &command_line ));
    else
        TRACE( "CommandLine %s.\n", debugstr_w( V_BSTR( &command_line )));

    if (FAILED(hr = IWbemClassObject_Get( in, L"CurrentDirectory", 0, &current_directory, &type, NULL ))
            || V_VT( &current_directory ) != VT_BSTR)
        WARN("invalid CurrentDirectory, hr %#lx, type %u\n", hr, V_VT( &current_directory ));
    else
        TRACE( "CurrentDirectory %s.\n", debugstr_w( V_BSTR( &current_directory )));

    if (SUCCEEDED(IWbemClassObject_Get( in, L"ProcessStartupInformation", 0, &startup_info, &type, NULL ))
            && V_VT( &startup_info ) == VT_UNKNOWN && V_UNKNOWN( &startup_info ))
        FIXME( "ProcessStartupInformation is not implemented, vt_type %u, type %lu, val %p\n",
                V_VT( &startup_info ), type, V_UNKNOWN( &startup_info ));

    if (out && (hr = IWbemClassObject_SpawnInstance( sig, 0, &out_params )))
    {
        ret = hr;
        goto done;
    }

    memset( &si, 0, sizeof(si) );
    si.cb = sizeof(si);

    if (V_VT( &command_line ) == VT_BSTR && V_BSTR( &command_line ))
    {
        bret = CreateProcessW( NULL, V_BSTR( &command_line ), NULL, NULL, FALSE, 0L,
                V_VT( &current_directory ) == VT_BSTR ? V_BSTR( &current_directory ) : NULL,
                NULL, &si, &pi );
        TRACE( "CreateProcessW ret %d, GetLastError() %lu\n", bret, GetLastError() );
        if (bret)
        {
            CloseHandle( pi.hThread );
            CloseHandle( pi.hProcess );
            error = 0;
        }
        else
        {
            switch (GetLastError())
            {
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
                error = 9;
                break;
            case ERROR_ACCESS_DENIED:
                error = 2;
                break;
            default:
                error = 8;
                break;
            }
        }
    }
    else
    {
        bret = FALSE;
        error = 21;
    }

    if (out)
    {
        VariantInit( &v );

        V_VT( &v ) = VT_UI4;
        V_UI4( &v ) = pi.dwProcessId;

        if (bret && (ret = IWbemClassObject_Put( out_params, L"ProcessId", 0, &v, 0 ))) goto done;

        V_UI4( &v ) = error;
        if ((ret = IWbemClassObject_Put( out_params, L"ReturnValue", 0, &v, 0 ))) goto done;

        *out = out_params;
        IWbemClassObject_AddRef( out_params );
    }
    ret = S_OK;

done:
    IWbemClassObject_Release( sig );
    if (out_params) IWbemClassObject_Release( out_params );
    VariantClear( &command_line );
    VariantClear( &current_directory );
    VariantClear( &startup_info );
    TRACE( "ret %#lx\n", ret );
    return ret;
}
