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
    NET_API_STATUS
    __stdcall
    status = NetrDfsAdd(DfsEntryPath,
                        ServerName,
                        PathName,
                        Comment,
                        Flags);
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
    _Out_ LPDWORD ResumeHandle)
{
    UNIMPLEMENTED;
    return 0;
}


NET_API_STATUS
WINAPI
NetDfsGetClientInfo(
    _In_ LPWSTR DfsEntryPath,
    _In_ LPWSTR ServerName,
    _In_ LPWSTR ShareName,
    _In_ DWORD Level,
    _Out_ LPBYTE *Buffer)
{
    UNIMPLEMENTED;
    return 0;
}


/* NetDfsGetDcAddress */


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


/* EOF */
