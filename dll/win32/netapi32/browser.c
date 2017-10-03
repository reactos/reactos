/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         NetAPI DLL
 * FILE:            dll/win32/netapi32/browser.c
 * PURPOSE:         Computer Browser service interface code
 * PROGRAMMERS:     Eric Kohl (eric.kohl@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "netapi32.h"

#include <rpc.h>
#include <lmbrowsr.h>
#include <lmserver.h>
#include "browser_c.h"


WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

/* FUNCTIONS *****************************************************************/

handle_t __RPC_USER
BROWSER_IDENTIFY_HANDLE_bind(BROWSER_IDENTIFY_HANDLE pszSystemName)
{
    handle_t hBinding = NULL;
    LPWSTR pszStringBinding;
    RPC_STATUS status;

    TRACE("BROWSER_IDENTIFY_HANDLE_bind() called\n");

    status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      pszSystemName,
                                      L"\\pipe\\browser",
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
BROWSER_IDENTIFY_HANDLE_unbind(BROWSER_IDENTIFY_HANDLE pszSystemName,
                               handle_t hBinding)
{
    RPC_STATUS status;

    TRACE("BROWSER_IDENTIFY_HANDLE_unbind() called\n");

    status = RpcBindingFree(&hBinding);
    if (status)
    {
        TRACE("RpcBindingFree returned 0x%x\n", status);
    }
}


NET_API_STATUS
WINAPI
I_BrowserDebugTrace(
    _In_opt_ LPWSTR ServerName,
    _In_ PCHAR Buffer)
{
    NET_API_STATUS status;

    TRACE("I_BrowserDebugTrace(%s %s)\n",
          debugstr_w(ServerName), Buffer);

    RpcTryExcept
    {
        status = I_BrowserrDebugTrace(ServerName,
                                      Buffer);
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
I_BrowserQueryEmulatedDomains(
    _In_opt_ LPWSTR ServerName,
    _Out_ PBROWSER_EMULATED_DOMAIN *EmulatedDomains,
    _Out_ LPDWORD EntriesRead)
{
    BROWSER_EMULATED_DOMAIN_CONTAINER Container = {0, NULL};
    NET_API_STATUS status;

    TRACE("I_BrowserQueryEmulatedDomains(%s %p %p)\n",
          debugstr_w(ServerName), EmulatedDomains, EntriesRead);

    *EmulatedDomains = NULL;
    *EntriesRead = 0;

    RpcTryExcept
    {
        status = I_BrowserrQueryEmulatedDomains(ServerName,
                                                &Container);

        if (status == NERR_Success)
        {
            *EmulatedDomains = (PBROWSER_EMULATED_DOMAIN)Container.Buffer;
            *EntriesRead = Container.EntriesRead;
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
I_BrowserQueryOtherDomains(
    _In_opt_ LPCWSTR ServerName,
    _Out_ LPBYTE *BufPtr,
    _Out_ LPDWORD EntriesRead,
    _Out_ LPDWORD TotalEntries)
{
    SERVER_INFO_100_CONTAINER Level100Container = {0, NULL};
    SERVER_ENUM_STRUCT EnumStruct;
    NET_API_STATUS status;

    TRACE("I_BrowserQueryOtherDomains(%s %p %p %p)\n",
          debugstr_w(ServerName), BufPtr, EntriesRead, TotalEntries);

    EnumStruct.Level = 100;
    EnumStruct.ServerInfo.Level100 = &Level100Container;

    RpcTryExcept
    {
        status = I_BrowserrQueryOtherDomains((PWSTR)ServerName,
                                             &EnumStruct,
                                             TotalEntries);

        if (status == NERR_Success || status == ERROR_MORE_DATA)
        {
            *BufPtr = (LPBYTE)EnumStruct.ServerInfo.Level100->Buffer;
            *EntriesRead = EnumStruct.ServerInfo.Level100->EntriesRead;
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
I_BrowserServerEnum(
    _In_opt_ LPCWSTR ServerName,
    _In_opt_ LPCWSTR Transport,
    _In_opt_ LPCWSTR ClientName,
    _In_ DWORD Level,
    _Out_ LPBYTE *BufPtr,
    _In_ DWORD PrefMaxLen,
    _Out_ LPDWORD EntriesRead,
    _Out_ LPDWORD TotalEntries,
    _In_ DWORD ServerType,
    _In_opt_ LPCWSTR Domain,
    _Inout_opt_ LPDWORD ResumeHandle)
{
    SERVER_INFO_100_CONTAINER Level100Container = {0, NULL};
    SERVER_ENUM_STRUCT EnumStruct;
    NET_API_STATUS status;

    TRACE("I_BrowserServerEnum(%s %s %s %lu %p %lu %p %p %lu %s %p)\n",
          debugstr_w(ServerName), debugstr_w(Transport), debugstr_w(ClientName),
          Level, BufPtr, PrefMaxLen, EntriesRead, TotalEntries, ServerType,
          debugstr_w(Domain), ResumeHandle);

    EnumStruct.Level = 100;
    EnumStruct.ServerInfo.Level100 = &Level100Container;

    RpcTryExcept
    {
        status = I_BrowserrServerEnum((PWSTR)ServerName,
                                      (PWSTR)Transport,
                                      (PWSTR)ClientName,
                                      &EnumStruct,
                                      PrefMaxLen,
                                      TotalEntries,
                                      ServerType,
                                      (PWSTR)Domain,
                                      ResumeHandle);

        if (status == NERR_Success || status == ERROR_MORE_DATA)
        {
            *BufPtr = (LPBYTE)EnumStruct.ServerInfo.Level100->Buffer;
            *EntriesRead = EnumStruct.ServerInfo.Level100->EntriesRead;
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
I_BrowserQueryStatistics(
    _In_opt_ LPCWSTR ServerName,
    _Inout_ LPBROWSER_STATISTICS *Statistics)
{
    NET_API_STATUS status;

    TRACE("I_BrowserQueryStatistics(%s %p)\n",
          debugstr_w(ServerName), Statistics);

    RpcTryExcept
    {
        status = I_BrowserrQueryStatistics((PWSTR)ServerName,
                                           Statistics);
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
I_BrowserResetStatistics(
    _In_opt_ LPCWSTR ServerName)
{
    NET_API_STATUS status;

    TRACE("I_BrowserResetStatistics(%s)\n",
          debugstr_w(ServerName));

    RpcTryExcept
    {
        status = I_BrowserrResetStatistics((PWSTR)ServerName);
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
I_BrowserResetNetlogonState(
    _In_ LPCWSTR ServerName)
{
    NET_API_STATUS status;

    TRACE("I_BrowserResetNetlogonState(%s)\n",
          debugstr_w(ServerName));

    RpcTryExcept
    {
        status = I_BrowserrResetNetlogonState((PWSTR)ServerName);
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
I_BrowserSetNetlogonState(
    _In_ LPWSTR ServerName,
    _In_ LPWSTR DomainName,
    _In_ LPWSTR EmulatedServerName,
    _In_ DWORD Role)
{
    NET_API_STATUS status;

    TRACE("I_BrowserSetNetlogonState(%s %s %s %lu)\n",
          debugstr_w(ServerName), debugstr_w(ServerName),
          debugstr_w(EmulatedServerName), Role);

    RpcTryExcept
    {
        status = I_BrowserrSetNetlogonState(ServerName,
                                            DomainName,
                                            EmulatedServerName,
                                            Role);
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
NetBrowserStatisticsGet(
    _In_ LPWSTR ServerName,
    _In_ DWORD Level,
    _Out_ LPBYTE *Buffer)
{
    BROWSER_STATISTICS_STRUCT StatisticsStruct;
    BROWSER_STATISTICS_100_CONTAINER Level100Container = {0, NULL};
    BROWSER_STATISTICS_101_CONTAINER Level101Container = {0, NULL};
    NET_API_STATUS status;

    TRACE("NetBrowserStatisticsGet(%s %lu %p)\n",
          debugstr_w(ServerName), Level, Buffer);

    if (Level != 100 && Level != 101)
        return ERROR_INVALID_LEVEL;

    StatisticsStruct.Level = Level;
    switch (Level)
    {
        case 100:
            StatisticsStruct.Statistics.Level100 = &Level100Container;
            break;

        case 101:
            StatisticsStruct.Statistics.Level101 = &Level101Container;
            break;
    }

    RpcTryExcept
    {
        status = NetrBrowserStatisticsGet(ServerName,
                                          Level,
                                          &StatisticsStruct);

        switch (Level)
        {
            case 100:
                if (StatisticsStruct.Statistics.Level100->Buffer != NULL)
                {
                    *Buffer = (LPBYTE)StatisticsStruct.Statistics.Level100->Buffer;
                }
                break;

            case 101:
                if (StatisticsStruct.Statistics.Level101->Buffer != NULL)
                {
                    *Buffer = (LPBYTE)StatisticsStruct.Statistics.Level101->Buffer;
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
NetServerEnum(
    _In_opt_ LMCSTR servername,
    _In_ DWORD level,
    _Out_ LPBYTE *bufptr,
    _In_ DWORD prefmaxlen,
    _Out_ LPDWORD entriesread,
    _Out_ LPDWORD totalentries,
    _In_ DWORD servertype,
    _In_opt_ LMCSTR domain,
    _Inout_opt_ LPDWORD resume_handle)
{
    TRACE("NetServerEnum(%s %lu %p %lu %p %p %lu %s %p)\n",
          debugstr_w(servername), level, bufptr, prefmaxlen, entriesread,
          totalentries, servertype, debugstr_w(domain), resume_handle);

    if (resume_handle != NULL)
        *resume_handle = 0;

    return NetServerEnumEx(servername,
                           level,
                           bufptr,
                           prefmaxlen,
                           entriesread,
                           totalentries,
                           servertype,
                           domain,
                           NULL);
}


NET_API_STATUS
WINAPI
NetServerEnumEx(
    _In_opt_ LMCSTR ServerName,
    _In_ DWORD Level,
    _Out_ LPBYTE *Bufptr,
    _In_ DWORD PrefMaxlen,
    _Out_ LPDWORD EntriesRead,
    _Out_ LPDWORD totalentries,
    _In_ DWORD servertype,
    _In_opt_ LMCSTR domain,
    _In_opt_ LMCSTR FirstNameToReturn)
{
    FIXME("NetServerEnumEx(%s %lu %p %lu %p %p %lu %s %s)\n",
           debugstr_w(ServerName), Level, Bufptr, PrefMaxlen, EntriesRead, totalentries,
           servertype, debugstr_w(domain), debugstr_w(FirstNameToReturn));

    return ERROR_NO_BROWSER_SERVERS_FOUND;
}

/* EOF */
