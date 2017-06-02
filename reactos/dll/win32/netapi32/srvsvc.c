/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         NetAPI DLL
 * FILE:            reactos/dll/win32/netapi32/srvsvc.c
 * PURPOSE:         Server service interface code
 * PROGRAMMERS:     Eric Kohl <eric.kohl@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include "netapi32.h"
#include "srvsvc_c.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

/* FUNCTIONS *****************************************************************/

handle_t __RPC_USER
SRVSVC_HANDLE_bind(SRVSVC_HANDLE pszSystemName)
{
    handle_t hBinding = NULL;
    LPWSTR pszStringBinding;
    RPC_STATUS status;

    TRACE("SRVSVC_HANDLE_bind() called\n");

    status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      (RPC_WSTR)pszSystemName,
                                      L"\\pipe\\srvsvc",
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
SRVSVC_HANDLE_unbind(SRVSVC_HANDLE pszSystemName,
                     handle_t hBinding)
{
    RPC_STATUS status;

    TRACE("SRVSVC_HANDLE_unbind() called\n");

    status = RpcBindingFree(&hBinding);
    if (status)
    {
        TRACE("RpcBindingFree returned 0x%x\n", status);
    }
}


NET_API_STATUS
WINAPI
NetRemoteTOD(
    _In_ LPCWSTR UncServerName,
    _Out_ LPBYTE *BufferPtr)
{
    NET_API_STATUS status;

    TRACE("NetRemoteTOD(%s, %p)\n", debugstr_w(UncServerName),
          BufferPtr);

    *BufferPtr = NULL;

    RpcTryExcept
    {
        status = NetrRemoteTOD((SRVSVC_HANDLE)UncServerName,
                               (LPTIME_OF_DAY_INFO *)BufferPtr);
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
NetShareAdd(
    _In_ LMSTR servername,
    _In_ DWORD level,
    _In_ LPBYTE buf,
    _Out_ LPDWORD parm_err)
{
    NET_API_STATUS status;

    TRACE("NetShareAdd(%s %lu %p %p)\n",
          debugstr_w(servername), level, buf, parm_err);

    if (level != 2 && level != 502 && level != 503)
        return ERROR_INVALID_LEVEL;

    RpcTryExcept
    {
        status = NetrShareAdd(servername,
                              level,
                              (LPSHARE_INFO)&buf,
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
NetShareCheck(
    _In_ LMSTR servername,
    _In_ LMSTR device,
    _Out_ LPDWORD type)
{
    NET_API_STATUS status;

    TRACE("NetShareCheck(%s %s %p)\n",
          debugstr_w(servername), debugstr_w(device), type);

    RpcTryExcept
    {
        status = NetrShareCheck(servername,
                                device,
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
NetShareDel(
    _In_ LMSTR servername,
    _In_ LMSTR netname,
    _In_ DWORD reserved)
{
    NET_API_STATUS status;

    TRACE("NetShareDel(%s %s %lu)\n",
          debugstr_w(servername), debugstr_w(netname), reserved);

    if (netname == NULL || (*netname == 0) || reserved != 0)
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        status = NetrShareDel(servername,
                              netname,
                              reserved);
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
NetShareDelSticky(
    _In_ LMSTR servername,
    _In_ LMSTR netname,
    _In_ DWORD reserved)
{
    NET_API_STATUS status;

    TRACE("NetShareDelSticky(%s %s %lu)\n",
          debugstr_w(servername), debugstr_w(netname), reserved);

    if (netname == NULL || (*netname == 0) || reserved != 0)
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        status = NetrShareDelSticky(servername,
                                    netname,
                                    reserved);
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
NetShareEnum(
    _In_ LMSTR servername,
    _In_ DWORD level,
    _Out_ LPBYTE *bufptr,
    _In_ DWORD prefmaxlen,
    _Out_ LPDWORD entriesread,
    _Out_ LPDWORD totalentries,
    _Inout_ LPDWORD resume_handle)
{
    SHARE_ENUM_STRUCT EnumStruct;
    SHARE_INFO_0_CONTAINER Level0Container = {0, NULL};
    SHARE_INFO_1_CONTAINER Level1Container = {0, NULL};
    SHARE_INFO_2_CONTAINER Level2Container = {0, NULL};
    SHARE_INFO_502_CONTAINER Level502Container = {0, NULL};
    NET_API_STATUS status;

    TRACE("NetShareEnum(%s %lu %p %lu %p %p %p)\n",
          debugstr_w(servername), level, bufptr, prefmaxlen,
          entriesread, totalentries, resume_handle);

    if (level > 2 && level != 502)
        return ERROR_INVALID_LEVEL;

    *bufptr = NULL;
    *entriesread = 0;
    *totalentries = 0;

    EnumStruct.Level = level;
    switch (level)
    {
        case 0:
            EnumStruct.ShareInfo.Level0 = &Level0Container;
            break;

        case 1:
            EnumStruct.ShareInfo.Level1 = &Level1Container;
            break;

        case 2:
            EnumStruct.ShareInfo.Level2 = &Level2Container;
            break;

        case 502:
            EnumStruct.ShareInfo.Level502 = &Level502Container;
            break;
    }

    RpcTryExcept
    {
        status = NetrShareEnum(servername,
                               &EnumStruct,
                               prefmaxlen,
                               totalentries,
                               resume_handle);

        switch (level)
        {
            case 0:
                if (EnumStruct.ShareInfo.Level0->Buffer != NULL)
                {
                    *bufptr = (LPBYTE)EnumStruct.ShareInfo.Level0->Buffer;
                    *entriesread = EnumStruct.ShareInfo.Level0->EntriesRead;
                }
                break;

            case 1:
                if (EnumStruct.ShareInfo.Level1->Buffer != NULL)
                {
                    *bufptr = (LPBYTE)EnumStruct.ShareInfo.Level1->Buffer;
                    *entriesread = EnumStruct.ShareInfo.Level1->EntriesRead;
                }
                break;

            case 2:
                if (EnumStruct.ShareInfo.Level2->Buffer != NULL)
                {
                    *bufptr = (LPBYTE)EnumStruct.ShareInfo.Level2->Buffer;
                    *entriesread = EnumStruct.ShareInfo.Level2->EntriesRead;
                }
                break;

            case 502:
                if (EnumStruct.ShareInfo.Level502->Buffer != NULL)
                {
                    *bufptr = (LPBYTE)EnumStruct.ShareInfo.Level502->Buffer;
                    *entriesread = EnumStruct.ShareInfo.Level502->EntriesRead;
                }
                break;
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
NetShareEnumSticky(
    _In_ LMSTR servername,
    _In_ DWORD level,
    _Out_ LPBYTE *bufptr,
    _In_ DWORD prefmaxlen,
    _Out_ LPDWORD entriesread,
    _Out_ LPDWORD totalentries,
    _Inout_ LPDWORD resume_handle)
{
    SHARE_ENUM_STRUCT EnumStruct;
    SHARE_INFO_0_CONTAINER Level0Container = {0, NULL};
    SHARE_INFO_1_CONTAINER Level1Container = {0, NULL};
    SHARE_INFO_2_CONTAINER Level2Container = {0, NULL};
    SHARE_INFO_502_CONTAINER Level502Container = {0, NULL};
    NET_API_STATUS status;

    TRACE("NetShareEnumSticky(%s %lu %p %lu %p %p %p)\n",
          debugstr_w(servername), level, bufptr, prefmaxlen,
          entriesread, totalentries, resume_handle);

    if (level > 2 && level != 502)
        return ERROR_INVALID_LEVEL;

    *bufptr = NULL;
    *entriesread = 0;
    *totalentries = 0;

    EnumStruct.Level = level;
    switch (level)
    {
        case 0:
            EnumStruct.ShareInfo.Level0 = &Level0Container;
            break;

        case 1:
            EnumStruct.ShareInfo.Level1 = &Level1Container;
            break;

        case 2:
            EnumStruct.ShareInfo.Level2 = &Level2Container;
            break;

        case 502:
            EnumStruct.ShareInfo.Level502 = &Level502Container;
            break;
    }

    RpcTryExcept
    {
        status = NetrShareEnum(servername,
                               (LPSHARE_ENUM_STRUCT)&EnumStruct,
                               prefmaxlen,
                               totalentries,
                               resume_handle);

        switch (level)
        {
            case 0:
                if (EnumStruct.ShareInfo.Level0->Buffer != NULL)
                {
                    *bufptr = (LPBYTE)EnumStruct.ShareInfo.Level0->Buffer;
                    *entriesread = EnumStruct.ShareInfo.Level0->EntriesRead;
                }
                break;

            case 1:
                if (EnumStruct.ShareInfo.Level1->Buffer != NULL)
                {
                    *bufptr = (LPBYTE)EnumStruct.ShareInfo.Level1->Buffer;
                    *entriesread = EnumStruct.ShareInfo.Level1->EntriesRead;
                }
                break;

            case 2:
                if (EnumStruct.ShareInfo.Level2->Buffer != NULL)
                {
                    *bufptr = (LPBYTE)EnumStruct.ShareInfo.Level2->Buffer;
                    *entriesread = EnumStruct.ShareInfo.Level2->EntriesRead;
                }
                break;

            case 502:
                if (EnumStruct.ShareInfo.Level502->Buffer != NULL)
                {
                    *bufptr = (LPBYTE)EnumStruct.ShareInfo.Level502->Buffer;
                    *entriesread = EnumStruct.ShareInfo.Level502->EntriesRead;
                }
                break;
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
NetShareGetInfo(
    _In_ LMSTR servername,
    _In_ LMSTR netname,
    _In_ DWORD level,
    _Out_ LPBYTE *bufptr)
{
    NET_API_STATUS status;

    TRACE("NetShareGetInfo(%s %s %lu %p)\n",
          debugstr_w(servername), debugstr_w(netname), level, bufptr);

    if (level > 2 && level != 502 && level != 1005)
        return ERROR_INVALID_LEVEL;

    if (netname == NULL || *netname == 0)
        return ERROR_INVALID_PARAMETER;

    *bufptr = NULL;

    RpcTryExcept
    {
        status = NetrShareGetInfo(servername,
                                  netname,
                                  level,
                                  (LPSHARE_INFO)bufptr);
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
NetShareSetInfo(
    _In_  LPWSTR servername,
    _In_  LPWSTR netname,
    _In_  DWORD level,
    _In_  LPBYTE buf,
    _Out_ LPDWORD parm_err)
{
    NET_API_STATUS status;

    TRACE("NetShareSetInfo(%s %s %lu %p %p)\n",
          debugstr_w(servername), debugstr_w(netname), level, buf, parm_err);

    if (level != 2 && level != 502 && level != 503 && level != 1004 &&
        level != 1005 && level != 1006 && level != 1501)
        return ERROR_INVALID_LEVEL;

    RpcTryExcept
    {
        status = NetrShareSetInfo(servername,
                                  netname,
                                  level,
                                  (LPSHARE_INFO)&buf,
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
