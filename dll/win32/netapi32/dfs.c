/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         NetAPI DLL
 * FILE:            reactos/dll/win32/netapi32/dfs.c
 * PURPOSE:         Distributed File System Service interface code
 * PROGRAMMERS:     Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include "netapi32.h"

#include <rpc.h>
#include "netdfs_c.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

/* FUNCTIONS *****************************************************************/

NET_API_STATUS
WINAPI
NetDfsAdd(
    _In_ LPWSTR DfsEntryPath,
    _In_ LPWSTR ServerName,
    _In_ LPWSTR PathName,
    _In_opt_ LPWSTR Comment,
    _In_ DWORD Flags)
{
#if 0
    NET_API_STATUS Status;

    TRACE("NetDfsAdd(%s %s %s %s 0x%lx)\n",
          debugstr_w(DfsEntryPath), debugstr_w(ServerName), debugstr_w(PathName),
          debugstr_w(Comment), Flags);

    if (DfsEntryPath == NULL ||
        *DfsEntryPath == UNICODE_NULL ||
        ServerName == NULL ||
        *ServerName == UNICODE_NULL ||
        PathName == NULL ||
        *PathName == UNICODE_NULL)
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        Status = NetrDfsAdd(DfsEntryPath,
                            ServerName,
                            PathName,
                            Comment,
                            Flags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
#endif
    UNIMPLEMENTED;
    return 0;
}


NET_API_STATUS
WINAPI
NetDfsAddFtRoot(
    _In_ LPWSTR ServerName,
    _In_ LPWSTR RootShare,
    _In_ LPWSTR FtDfsName,
    _In_opt_ LPWSTR Comment,
    _In_ DWORD Flags)
{
    UNIMPLEMENTED;
    return 0;
}


NET_API_STATUS
WINAPI
NetDfsAddStdRoot(
    _In_ LPWSTR ServerName,
    _In_ LPWSTR RootShare,
    _In_opt_ LPWSTR Comment,
    _In_ DWORD Flags)
{
    UNIMPLEMENTED;
    return 0;
}


NET_API_STATUS
WINAPI
NetDfsAddStdRootForced(
    _In_ LPWSTR ServerName,
    _In_ LPWSTR RootShare,
    _In_opt_ LPWSTR Comment,
    _In_ LPWSTR Store)
{
    UNIMPLEMENTED;
    return 0;
}


NET_API_STATUS
WINAPI
NetDfsEnum(
    _In_ LPWSTR DfsName,
    _In_ DWORD Level,
    _In_ DWORD PrefMaxLen,
    _Out_ LPBYTE *Buffer,
    _Out_ LPDWORD EntriesRead,
    _Inout_ LPDWORD ResumeHandle)
{
    UNIMPLEMENTED;
    return 0;
}


NET_API_STATUS
WINAPI
NetDfsGetClientInfo(
    _In_ LPWSTR DfsEntryPath,
    _In_opt_ LPWSTR ServerName,
    _In_opt_ LPWSTR ShareName,
    _In_ DWORD Level,
    _Out_ LPBYTE *Buffer)
{
    UNIMPLEMENTED;
    return 0;
}


NET_API_STATUS
WINAPI
NetDfsGetDcAddress(
    _In_ LPWSTR ServerName,
    _Out_ LPWSTR *DcIpAddress,
    _Out_ PBOOLEAN IsRoot,
    _Out_ PULONG Timeout)
{
    UNIMPLEMENTED;
    return 0;
}


NET_API_STATUS
WINAPI
NetDfsGetFtContainerSecurity(
    _In_ LPWSTR DomainName,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_ PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
    _Out_ LPDWORD lpcbSecurityDescriptor)
{
    UNIMPLEMENTED;
    return 0;
}


NET_API_STATUS
WINAPI
NetDfsGetInfo(
    _In_ LPWSTR DfsEntryPath,
    _In_opt_ LPWSTR ServerName,
    _In_opt_ LPWSTR ShareName,
    _In_ DWORD Level,
    _Out_ LPBYTE *Buffer)
{
    UNIMPLEMENTED;
    return 0;
}


NET_API_STATUS
WINAPI
NetDfsGetSecurity(
    _In_ LPWSTR DfsEntryPath,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_ PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
    _Out_ LPDWORD lpcbSecurityDescriptor)
{
    UNIMPLEMENTED;
    return 0;
}


NET_API_STATUS
WINAPI
NetDfsGetStdContainerSecurity(
    _In_ LPWSTR MachineName,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_ PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
    _Out_ LPDWORD lpcbSecurityDescriptor)
{
    UNIMPLEMENTED;
    return 0;
}


/* NetDfsManagerGetConfigInfo */


NET_API_STATUS
WINAPI
NetDfsManagerInitialize(
    _In_ LPWSTR ServerName,
    _Reserved_ DWORD Flags)
{
    UNIMPLEMENTED;
    return 0;
}


/* NetDfsManagerSendSiteInfo */


NET_API_STATUS
WINAPI
NetDfsMove(
    _In_ LPWSTR Path,
    _In_ LPWSTR NewPath,
    _In_ ULONG Flags)
{
    UNIMPLEMENTED;
    return 0;
}


NET_API_STATUS
WINAPI
NetDfsRemove(
    _In_ LPWSTR DfsEntryPath,
    _In_opt_ LPWSTR ServerName,
    _In_opt_ LPWSTR ShareName)
{
    UNIMPLEMENTED;
    return 0;
}


NET_API_STATUS
WINAPI
NetDfsRemoveFtRoot(
    _In_ LPWSTR ServerName,
    _In_ LPWSTR RootShare,
    _In_ LPWSTR FtDfsName,
    _Reserved_ DWORD Flags)
{
    UNIMPLEMENTED;
    return 0;
}


NET_API_STATUS
WINAPI
NetDfsRemoveFtRootForced(
    _In_ LPWSTR DomainName,
    _In_ LPWSTR ServerName,
    _In_ LPWSTR RootShare,
    _In_ LPWSTR FtDfsName,
    _Reserved_ DWORD Flags)
{
    UNIMPLEMENTED;
    return 0;
}


NET_API_STATUS
WINAPI
NetDfsRemoveStdRoot(
    _In_ LPWSTR ServerName,
    _In_ LPWSTR RootShare,
    _In_ DWORD Flags)
{
    UNIMPLEMENTED;
    return 0;
}


NET_API_STATUS
WINAPI
NetDfsRename(
    _In_ LPWSTR Path,
    _In_ LPWSTR NewPath)
{
    UNIMPLEMENTED;
    return 0;
}


NET_API_STATUS
WINAPI
NetDfsSetClientInfo(
    _In_ LPWSTR DfsEntryPath,
    _In_opt_ LPWSTR ServerName,
    _In_opt_ LPWSTR ShareName,
    _In_ DWORD Level,
    _In_ LPBYTE Buffer)
{
    UNIMPLEMENTED;
    return 0;
}


NET_API_STATUS
WINAPI
NetDfsSetFtContainerSecurity(
    _In_ LPWSTR DomainName,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    UNIMPLEMENTED;
    return 0;
}


NET_API_STATUS
WINAPI
NetDfsSetInfo(
    _In_ LPWSTR DfsEntryPath,
    _In_opt_ LPWSTR ServerName,
    _In_opt_ LPWSTR ShareName,
    _In_ DWORD Level,
    _In_ LPBYTE Buffer)
{
    UNIMPLEMENTED;
    return 0;
}


NET_API_STATUS
WINAPI
NetDfsSetSecurity(
    _In_ LPWSTR DfsEntryPath,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    UNIMPLEMENTED;
    return 0;
}


NET_API_STATUS
WINAPI
NetDfsSetStdContainerSecurity(
    _In_ LPWSTR MachineName,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    UNIMPLEMENTED;
    return 0;
}

/* EOF */
