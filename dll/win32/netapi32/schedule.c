/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         NetAPI DLL
 * FILE:            reactos/dll/win32/netapi32/schedule.c
 * PURPOSE:         Scheduler service interface code
 *
 * PROGRAMMERS:     Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include "netapi32.h"
#include "atsvc_c.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

/* FUNCTIONS *****************************************************************/

handle_t __RPC_USER
ATSVC_HANDLE_bind(ATSVC_HANDLE pszSystemName)
{
    handle_t hBinding = NULL;
    LPWSTR pszStringBinding;
    RPC_STATUS status;

    TRACE("ATSVC_HANDLE_bind() called\n");

    status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      (RPC_WSTR)pszSystemName,
                                      L"\\pipe\\atsvc",
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
ATSVC_HANDLE_unbind(ATSVC_HANDLE pszSystemName,
                    handle_t hBinding)
{
    RPC_STATUS status;

    TRACE("ATSVC_HANDLE_unbind() called\n");

    status = RpcBindingFree(&hBinding);
    if (status)
    {
        TRACE("RpcBindingFree returned 0x%x\n", status);
    }
}


NET_API_STATUS
WINAPI
NetScheduleJobAdd(
    LPCWSTR ServerName,
    LPBYTE Buffer,
    LPDWORD JobId)
{
    NET_API_STATUS status;

    TRACE("NetScheduleJobAdd(%s, %p, %p)\n", debugstr_w(ServerName),
          Buffer, JobId);

    RpcTryExcept
    {
        status = NetrJobAdd(ServerName,
                            (LPAT_INFO)Buffer,
                            JobId);
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
NetScheduleJobDel(
    LPCWSTR ServerName,
    DWORD MinJobId,
    DWORD MaxJobId)
{
    NET_API_STATUS status;

    TRACE("NetScheduleJobDel(%s, %d, %d)\n", debugstr_w(ServerName),
          MinJobId, MaxJobId);

    RpcTryExcept
    {
        status = NetrJobDel(ServerName,
                            MinJobId,
                            MaxJobId);
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
NetScheduleJobEnum(
    LPCWSTR ServerName,
    LPBYTE *PointerToBuffer,
    DWORD PreferredMaximumLength,
    LPDWORD EntriesRead,
    LPDWORD TotalEntries,
    LPDWORD ResumeHandle)
{
    AT_ENUM_CONTAINER EnumContainer;
    NET_API_STATUS status;

    TRACE("NetScheduleJobEnum(%s, %p, %d, %p, %p, %p)\n", debugstr_w(ServerName),
          PointerToBuffer, PreferredMaximumLength, EntriesRead, TotalEntries, ResumeHandle);

    EnumContainer.EntriesRead = 0;
    EnumContainer.Buffer = NULL;

    RpcTryExcept
    {
        status = NetrJobEnum(ServerName,
                             &EnumContainer,
                             PreferredMaximumLength,
                             TotalEntries,
                             ResumeHandle);
        if (status == NERR_Success || status == ERROR_MORE_DATA)
        {
            *PointerToBuffer = (LPBYTE)EnumContainer.Buffer;
            *EntriesRead = EnumContainer.EntriesRead;
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
NetScheduleJobGetInfo(
    LPCWSTR ServerName,
    DWORD JobId,
    LPBYTE *PointerToBuffer)
{
    NET_API_STATUS status;

    TRACE("NetScheduleJobGetInfo(%s, %d, %p)\n", debugstr_w(ServerName),
          JobId, PointerToBuffer);

    RpcTryExcept
    {
        status = NetrJobGetInfo(ServerName,
                                JobId,
                                (LPAT_INFO *)PointerToBuffer);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}

/* EOF */
