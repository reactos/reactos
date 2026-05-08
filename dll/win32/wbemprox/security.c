/*
 * __SystemSecurity implementation
 *
 * Copyright 2014 Vincent Povirk for CodeWeavers
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
#include "iads.h"

#include "wine/debug.h"
#include "wbemprox_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wbemprox);

static HRESULT to_byte_array( void *data, DWORD size, VARIANT *var )
{
    SAFEARRAY *sa;
    void *sadata;
    HRESULT hr;

    if (!(sa = SafeArrayCreateVector( VT_UI1, 0, size ))) return E_OUTOFMEMORY;

    hr = SafeArrayAccessData( sa, &sadata );

    if (SUCCEEDED(hr))
    {
        memcpy( sadata, data, size );

        SafeArrayUnaccessData( sa );
    }
    else
    {
        SafeArrayDestroy( sa );
        return hr;
    }

    set_variant( VT_UI1|VT_ARRAY, 0, sa, var );
    return S_OK;
}

static HRESULT get_sd( SECURITY_DESCRIPTOR **sd, DWORD *size )
{
    BYTE sid_admin_buffer[SECURITY_MAX_SID_SIZE];
    SID *sid_admin = (SID*)sid_admin_buffer;
    BYTE sid_network_buffer[SECURITY_MAX_SID_SIZE];
    SID *sid_network = (SID*)sid_network_buffer;
    BYTE sid_local_buffer[SECURITY_MAX_SID_SIZE];
    SID *sid_local = (SID*)sid_local_buffer;
    BYTE sid_users_buffer[SECURITY_MAX_SID_SIZE];
    SID *sid_users = (SID*)sid_users_buffer;
    BYTE acl_buffer[sizeof(ACL) + 4 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + SECURITY_MAX_SID_SIZE)];
    ACL *acl = (ACL*)acl_buffer;
    DWORD sid_size;
    SECURITY_DESCRIPTOR absolute_sd;
    HRESULT hr = S_OK;

    sid_size = sizeof(sid_admin_buffer);
    CreateWellKnownSid( WinBuiltinAdministratorsSid, NULL, sid_admin, &sid_size );

    sid_size = sizeof(sid_network_buffer);
    CreateWellKnownSid( WinNetworkServiceSid, NULL, sid_network, &sid_size );

    sid_size = sizeof(sid_local_buffer);
    CreateWellKnownSid( WinLocalServiceSid, NULL, sid_local, &sid_size );

    sid_size = sizeof(sid_users_buffer);
    CreateWellKnownSid( WinAuthenticatedUserSid, NULL, sid_users, &sid_size );

    InitializeAcl( acl, sizeof(acl_buffer), ACL_REVISION );

    AddAccessAllowedAceEx( acl, ACL_REVISION, CONTAINER_INHERIT_ACE|INHERITED_ACE,
        ADS_RIGHT_DS_CREATE_CHILD|ADS_RIGHT_DS_DELETE_CHILD|ADS_RIGHT_ACTRL_DS_LIST|ADS_RIGHT_DS_SELF|
        ADS_RIGHT_DS_READ_PROP|ADS_RIGHT_DS_WRITE_PROP|READ_CONTROL|WRITE_DAC,
        sid_admin );

    AddAccessAllowedAceEx( acl, ACL_REVISION, CONTAINER_INHERIT_ACE|INHERITED_ACE,
        ADS_RIGHT_DS_CREATE_CHILD|ADS_RIGHT_DS_DELETE_CHILD|ADS_RIGHT_DS_READ_PROP,
        sid_network );

    AddAccessAllowedAceEx( acl, ACL_REVISION, CONTAINER_INHERIT_ACE|INHERITED_ACE,
        ADS_RIGHT_DS_CREATE_CHILD|ADS_RIGHT_DS_DELETE_CHILD|ADS_RIGHT_DS_READ_PROP,
        sid_local );

    AddAccessAllowedAceEx( acl, ACL_REVISION, CONTAINER_INHERIT_ACE|INHERITED_ACE,
        ADS_RIGHT_DS_CREATE_CHILD|ADS_RIGHT_DS_DELETE_CHILD|ADS_RIGHT_DS_READ_PROP,
        sid_users );

    InitializeSecurityDescriptor( &absolute_sd, SECURITY_DESCRIPTOR_REVISION );

    SetSecurityDescriptorOwner( &absolute_sd, sid_admin, TRUE );
    SetSecurityDescriptorGroup( &absolute_sd, sid_admin, TRUE );
    SetSecurityDescriptorDacl( &absolute_sd, TRUE, acl, TRUE );

    *size = GetSecurityDescriptorLength( &absolute_sd );

    *sd = malloc( *size );
    if (!*sd)
        hr = E_OUTOFMEMORY;

    if (SUCCEEDED(hr))
    {
        if (!MakeSelfRelativeSD(&absolute_sd, *sd, size))
        {
            free( *sd );
            *sd = NULL;
            hr = E_FAIL;
        }
    }

    return hr;
}

HRESULT security_get_sd( IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out )
{
    VARIANT var_sd, retval;
    IWbemClassObject *sig, *out_params = NULL;
    HRESULT hr, ret;
    SECURITY_DESCRIPTOR *sd;
    DWORD sd_size;

    TRACE("%p, %p, %p, %p\n", obj, context, in, out);

    hr = create_signature( WBEMPROX_NAMESPACE_CIMV2, L"__SystemSecurity", L"GetSD", PARAM_OUT, &sig );

    if (SUCCEEDED(hr))
    {
        hr = IWbemClassObject_SpawnInstance( sig, 0, &out_params );

        IWbemClassObject_Release( sig );
    }

    if (SUCCEEDED(hr))
    {
        ret = get_sd( &sd, &sd_size );

        if (SUCCEEDED(ret))
        {
            VariantInit( &var_sd );

            hr = to_byte_array( sd, sd_size, &var_sd );

            if (SUCCEEDED(hr))
                hr = IWbemClassObject_Put( out_params, L"SD", 0, &var_sd, CIM_UINT8|CIM_FLAG_ARRAY );

            free( sd );
            VariantClear( &var_sd );
        }

        if (SUCCEEDED(hr))
        {
            set_variant( VT_UI4, ret, NULL, &retval );
            hr = IWbemClassObject_Put( out_params, L"ReturnValue", 0, &retval, CIM_UINT32 );
        }

        if (SUCCEEDED(hr) && out)
        {
            *out = out_params;
            IWbemClassObject_AddRef( out_params );
        }

        IWbemClassObject_Release( out_params );
    }

    return hr;
}


HRESULT security_set_sd( IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out )
{
    VARIANT retval;
    IWbemClassObject *sig, *out_params = NULL;
    HRESULT hr;

    FIXME("stub\n");

    hr = create_signature( WBEMPROX_NAMESPACE_CIMV2, L"__SystemSecurity", L"SetSD", PARAM_OUT, &sig );

    if (SUCCEEDED(hr))
    {
        hr = IWbemClassObject_SpawnInstance( sig, 0, &out_params );

        IWbemClassObject_Release( sig );
    }

    if (SUCCEEDED(hr))
    {
        set_variant( VT_UI4, S_OK, NULL, &retval );
        hr = IWbemClassObject_Put( out_params, L"ReturnValue", 0, &retval, CIM_UINT32 );

        if (SUCCEEDED(hr) && out)
        {
            *out = out_params;
            IWbemClassObject_AddRef( out_params );
        }

        IWbemClassObject_Release( out_params );
    }

    return hr;
}
