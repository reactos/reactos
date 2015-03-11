/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         NetAPI DLL
 * FILE:            reactos/dll/win32/netapi32/wksta_new.c
 * PURPOSE:         Workstation service interface code
 *
 * PROGRAMMERS:     Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include "netapi32.h"
#include "wkssvc_c.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

/* FUNCTIONS *****************************************************************/

void __RPC_FAR * __RPC_USER midl_user_allocate(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}


handle_t __RPC_USER
WKSSVC_IDENTIFY_HANDLE_bind(WKSSVC_IDENTIFY_HANDLE pszSystemName)
{
    handle_t hBinding = NULL;
    LPWSTR pszStringBinding;
    RPC_STATUS status;

    TRACE("WKSSVC_IDENTIFY_HANDLE_bind() called\n");

    status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      pszSystemName,
                                      L"\\pipe\\wkssvc",
                                      NULL,
                                      &pszStringBinding);
    if (status)
    {
        TRACE("RpcStringBindingCompose returned 0x%x\n", status);
        return NULL;
    }

    /* Set the binding handle that will be used to bind to the server. */
    status = RpcBindingFromStringBindingW(pszStringBinding,
                                          &hBinding);
    if (status)
    {
        TRACE("RpcBindingFromStringBinding returned 0x%x\n", status);
    }

    status = RpcStringFreeW(&pszStringBinding);
    if (status)
    {
//        TRACE("RpcStringFree returned 0x%x\n", status);
    }

    return hBinding;
}


void __RPC_USER
WKSSVC_IDENTIFY_HANDLE_unbind(WKSSVC_IDENTIFY_HANDLE pszSystemName,
                              handle_t hBinding)
{
    RPC_STATUS status;

    TRACE("WKSSVC_IDENTIFY_HANDLE_unbind() called\n");

    status = RpcBindingFree(&hBinding);
    if (status)
    {
        TRACE("RpcBindingFree returned 0x%x\n", status);
    }
}


handle_t __RPC_USER
WKSSVC_IMPERSONATE_HANDLE_bind(WKSSVC_IMPERSONATE_HANDLE pszSystemName)
{
    handle_t hBinding = NULL;
    LPWSTR pszStringBinding;
    RPC_STATUS status;

    TRACE("WKSSVC_IMPERSONATE_HANDLE_bind() called\n");

    status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      pszSystemName,
                                      L"\\pipe\\wkssvc",
                                      NULL,
                                      &pszStringBinding);
    if (status)
    {
        TRACE("RpcStringBindingCompose returned 0x%x\n", status);
        return NULL;
    }

    /* Set the binding handle that will be used to bind to the server. */
    status = RpcBindingFromStringBindingW(pszStringBinding,
                                          &hBinding);
    if (status)
    {
        TRACE("RpcBindingFromStringBinding returned 0x%x\n", status);
    }

    status = RpcStringFreeW(&pszStringBinding);
    if (status)
    {
//        TRACE("RpcStringFree returned 0x%x\n", status);
    }

    return hBinding;
}


void __RPC_USER
WKSSVC_IMPERSONATE_HANDLE_unbind(WKSSVC_IMPERSONATE_HANDLE pszSystemName,
                                 handle_t hBinding)
{
    RPC_STATUS status;

    TRACE("WKSSVC_IMPERSONATE_HANDLE_unbind() called\n");

    status = RpcBindingFree(&hBinding);
    if (status)
    {
        TRACE("RpcBindingFree returned 0x%x\n", status);
    }
}


NET_API_STATUS
NET_API_FUNCTION
NetGetJoinInformation(
    LPCWSTR Server,
    LPWSTR *Name,
    PNETSETUP_JOIN_STATUS type)
{
    NET_API_STATUS status;

    TRACE("NetGetJoinInformation(%s %p %p)\n", wine_dbgstr_w(Server),
          Name, type);

    if (Name == NULL || type == NULL)
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        status = NetrGetJoinInformation((LPWSTR)Server,
                                        Name,
                                        type);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


NET_API_STATUS
WINAPI
NetWkstaGetInfo(
    LPWSTR servername,
    DWORD level,
    LPBYTE *bufptr)
{
    NET_API_STATUS status;

    TRACE("NetWkstaGetInfo(%s, %d, %p)\n", debugstr_w(servername),
          level, bufptr);

    *bufptr = NULL;

    RpcTryExcept
    {
        status = NetrWkstaGetInfo(servername,
                                  level,
                                  (LPWKSTA_INFO)bufptr);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


NET_API_STATUS
WINAPI
NetWkstaSetInfo(
    LPWSTR servername,
    DWORD level,
    LPBYTE buffer,
    LPDWORD parm_err)
{
    NET_API_STATUS status;

    TRACE("NetWkstaSetInfo(%s, %d, %p, %p)\n", debugstr_w(servername),
          level, buffer, parm_err);

    RpcTryExcept
    {
        status = NetrWkstaSetInfo(servername,
                                  level,
                                  (LPWKSTA_INFO)buffer,
                                  parm_err);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


NET_API_STATUS
WINAPI
NetWkstaUserEnum(
    LMSTR servername,
    DWORD level,
    LPBYTE *bufptr,
    DWORD prefmaxlen,
    LPDWORD entriesread,
    LPDWORD totalentries,
    LPDWORD resumehandle)
{
    WKSTA_USER_ENUM_STRUCT UserEnumInfo;
    WKSTA_USER_INFO_0_CONTAINER Level0;
    WKSTA_USER_INFO_1_CONTAINER Level1;
    NET_API_STATUS status;

    TRACE("NetWkstaUserEnum(%s, %d, %p, %d, %p, %p, %p)\n", debugstr_w(servername),
          level, bufptr, prefmaxlen, entriesread, totalentries, resumehandle);

    UserEnumInfo.Level = level;
    switch (level)
    {
        case 0:
            UserEnumInfo.WkstaUserInfo.Level0 = &Level0;
            UserEnumInfo.WkstaUserInfo.Level0->EntriesRead = 0;
            UserEnumInfo.WkstaUserInfo.Level0->Buffer = NULL;
            break;

        case 1:
            UserEnumInfo.WkstaUserInfo.Level1 = &Level1;
            UserEnumInfo.WkstaUserInfo.Level1->EntriesRead = 0;
            UserEnumInfo.WkstaUserInfo.Level1->Buffer = NULL;
            break;

        default:
            return ERROR_INVALID_PARAMETER;
    }

    RpcTryExcept
    {
        status = NetrWkstaUserEnum(servername,
                                   &UserEnumInfo,
                                   prefmaxlen,
                                   totalentries,
                                   resumehandle);
        if (status == NERR_Success || status == ERROR_MORE_DATA)
        {
            switch (level)
            {
                case 0:
                    *bufptr = (LPBYTE)UserEnumInfo.WkstaUserInfo.Level0->Buffer;
                    *entriesread = UserEnumInfo.WkstaUserInfo.Level0->EntriesRead;
                    break;

                case 1:
                    *bufptr = (LPBYTE)UserEnumInfo.WkstaUserInfo.Level1->Buffer;
                    *entriesread = UserEnumInfo.WkstaUserInfo.Level1->EntriesRead;
                    break;
            }
        }
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


NET_API_STATUS
WINAPI
NetWkstaUserGetInfo(
    LPWSTR reserved,
    DWORD level,
    PBYTE *bufptr)
{
    NET_API_STATUS status;

    TRACE("NetWkstaUserGetInfo(%s, %d, %p)\n", debugstr_w(reserved),
          level, bufptr);

    if (reserved != NULL)
        return ERROR_INVALID_PARAMETER;

    *bufptr = NULL;

    RpcTryExcept
    {
        status = NetrWkstaUserGetInfo(NULL,
                                      level,
                                      (LPWKSTA_USER_INFO)bufptr);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


NET_API_STATUS
WINAPI
NetWkstaUserSetInfo(
    LPWSTR reserved,
    DWORD level,
    LPBYTE buf,
    LPDWORD parm_err)
{
    NET_API_STATUS status;

    TRACE("NetWkstaSetInfo(%s, %d, %p, %p)\n", debugstr_w(reserved),
          level, buf, parm_err);

    if (reserved != NULL)
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
       status = NetrWkstaUserSetInfo(NULL,
                                     level,
                                     (LPWKSTA_USER_INFO)&buf,
                                     parm_err);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}

/* EOF */
