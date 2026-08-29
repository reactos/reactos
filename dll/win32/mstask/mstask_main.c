/*
 * Copyright (C) 2008 Google (Roy Shea)
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "rpcproxy.h"
#include "taskschd.h"
#include "mstask.h"
#include "mstask_private.h"
#include "atsvc.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mstask);

LONG dll_ref = 0;

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    TRACE("(%s %s %p)\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);

    if (IsEqualGUID(rclsid, &CLSID_CTaskScheduler)) {
        return IClassFactory_QueryInterface((LPCLASSFACTORY)&MSTASK_ClassFactory, iid, ppv);
    }

    FIXME("Not supported class: %s\n", debugstr_guid(rclsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    return dll_ref != 0 ? S_FALSE : S_OK;
}

DWORD WINAPI NetrJobAdd_wrapper(ATSVC_HANDLE server_name, LPAT_INFO info, LPDWORD jobid)
{
    return NetrJobAdd(server_name, info, jobid);
}

DWORD WINAPI NetrJobDel_wrapper(ATSVC_HANDLE server_name, DWORD min_jobid, DWORD max_jobid)
{
    return NetrJobDel(server_name, min_jobid, max_jobid);
}

DWORD WINAPI NetrJobEnum_wrapper(ATSVC_HANDLE server_name, LPAT_ENUM_CONTAINER container,
                                 DWORD max_length, LPDWORD total, LPDWORD resume)
{
    return NetrJobEnum(server_name, container, max_length, total, resume);
}

DWORD WINAPI NetrJobGetInfo_wrapper(ATSVC_HANDLE server_name, DWORD jobid, LPAT_INFO *info)
{
    return NetrJobGetInfo(server_name, jobid, info);
}

void __RPC_FAR *__RPC_USER MIDL_user_allocate(SIZE_T n)
{
    return malloc(n);
}

void __RPC_USER MIDL_user_free(void __RPC_FAR *p)
{
    free(p);
}

handle_t __RPC_USER ATSVC_HANDLE_bind(ATSVC_HANDLE str)
{
    static unsigned char ncalrpc[] = "ncalrpc";
    unsigned char *binding_str;
    handle_t rpc_handle = 0;

    if (RpcStringBindingComposeA(NULL, ncalrpc, NULL, NULL, NULL, &binding_str) == RPC_S_OK)
    {
        RpcBindingFromStringBindingA(binding_str, &rpc_handle);
        RpcStringFreeA(&binding_str);
    }
    return rpc_handle;
}

void __RPC_USER ATSVC_HANDLE_unbind(ATSVC_HANDLE ServerName, handle_t rpc_handle)
{
    RpcBindingFree(&rpc_handle);
}
