/*
 *  ReactOS Services
 *  Copyright (C) 2015 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Services
 * FILE:             base/services/srvsvc/rpcserver.c
 * PURPOSE:          Server service
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include "winerror.h"
#include "lmerr.h"

WINE_DEFAULT_DEBUG_CHANNEL(srvsvc);

/* FUNCTIONS *****************************************************************/

DWORD
WINAPI
RpcThreadRoutine(
    LPVOID lpParameter)
{
    RPC_STATUS Status;

    Status = RpcServerUseProtseqEpW(L"ncacn_np", 20, L"\\pipe\\srvsvc", NULL);
    if (Status != RPC_S_OK)
    {
        ERR("RpcServerUseProtseqEpW() failed (Status %lx)\n", Status);
        return 0;
    }

    Status = RpcServerRegisterIf(srvsvc_v3_0_s_ifspec, NULL, NULL);
    if (Status != RPC_S_OK)
    {
        ERR("RpcServerRegisterIf() failed (Status %lx)\n", Status);
        return 0;
    }

    Status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, FALSE);
    if (Status != RPC_S_OK)
    {
        ERR("RpcServerListen() failed (Status %lx)\n", Status);
    }

    return 0;
}


void __RPC_FAR * __RPC_USER midl_user_allocate(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}


void __RPC_USER SHARE_DEL_HANDLE_rundown(SHARE_DEL_HANDLE hClientHandle)
{
}


/* Function 0 */
void
__stdcall
Opnum0NotUsedOnWire(void)
{
    UNIMPLEMENTED;
}


/* Function 1 */
void
__stdcall
Opnum1NotUsedOnWire(void)
{
    UNIMPLEMENTED;
}


/* Function 2 */
void
__stdcall
Opnum2NotUsedOnWire(void)
{
    UNIMPLEMENTED;
}


/* Function 3 */
void
__stdcall
Opnum3NotUsedOnWire(void)
{
    UNIMPLEMENTED;
}


/* Function 4 */
void
__stdcall
Opnum4NotUsedOnWire(void)
{
    UNIMPLEMENTED;
}

/* Function 5 */
void
__stdcall
Opnum5NotUsedOnWire(void)
{
    UNIMPLEMENTED;
}


/* Function 6 */
void
__stdcall
Opnum6NotUsedOnWire(void)
{
    UNIMPLEMENTED;
}


/* Function 7 */
void
__stdcall
Opnum7NotUsedOnWire(void)
{
    UNIMPLEMENTED;
}


/* Function 8 */
NET_API_STATUS
__stdcall
NetrConnectionEnum(
    SRVSVC_HANDLE ServerName,
    WCHAR *Qualifier,
    LPCONNECT_ENUM_STRUCT InfoStruct,
    DWORD PreferedMaximumLength,
    DWORD *TotalEntries,
    DWORD *ResumeHandle)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 9 */
NET_API_STATUS
__stdcall
NetrFileEnum(
    SRVSVC_HANDLE ServerName,
    WCHAR *BasePath,
    WCHAR *UserName,
    PFILE_ENUM_STRUCT InfoStruct,
    DWORD PreferedMaximumLength,
    DWORD *TotalEntries,
    DWORD *ResumeHandle)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 10 */
NET_API_STATUS
__stdcall
NetrFileGetInfo(
    SRVSVC_HANDLE ServerName,
    DWORD FileId,
    DWORD Level,
    LPFILE_INFO InfoStruct)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 11 */
NET_API_STATUS
__stdcall
NetrFileClose(
    SRVSVC_HANDLE ServerName,
    DWORD FileId)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 12 */
NET_API_STATUS
__stdcall
NetrSessionEnum(
    SRVSVC_HANDLE ServerName,
    WCHAR *ClientName,
    WCHAR *UserName,
    PSESSION_ENUM_STRUCT InfoStruct,
    DWORD PreferedMaximumLength,
    DWORD *TotalEntries,
    DWORD *ResumeHandle)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 13 */
NET_API_STATUS
__stdcall
NetrSessionDel(
    SRVSVC_HANDLE ServerName,
    WCHAR *ClientName,
    WCHAR *UserName)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 14 */
NET_API_STATUS
__stdcall
NetrShareAdd(
    SRVSVC_HANDLE ServerName,
    DWORD Level,
    LPSHARE_INFO InfoStruct,
    DWORD *ParmErr)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 15 */
NET_API_STATUS
__stdcall
NetrShareEnum(
    SRVSVC_HANDLE ServerName,
    LPSHARE_ENUM_STRUCT InfoStruct,
    DWORD PreferedMaximumLength,
    DWORD *TotalEntries,
    DWORD *ResumeHandle)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 16 */
NET_API_STATUS
__stdcall
NetrShareGetInfo(
    SRVSVC_HANDLE ServerName,
    WCHAR *NetName,
    DWORD Level,
    LPSHARE_INFO InfoStruct)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 17 */
NET_API_STATUS
__stdcall
NetrShareSetInfo(
    SRVSVC_HANDLE ServerName,
    WCHAR *NetName,
    DWORD Level,
    LPSHARE_INFO ShareInfo,
    DWORD *ParmErr)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 18 */
NET_API_STATUS
__stdcall
NetrShareDel(
    SRVSVC_HANDLE ServerName,
    WCHAR *NetName,
    DWORD Reserved)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 19 */
NET_API_STATUS
__stdcall
NetrShareDelSticky(
    SRVSVC_HANDLE ServerName,
    WCHAR *NetName,
    DWORD Reserved)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 20 */
NET_API_STATUS
__stdcall
NetrShareCheck(
    SRVSVC_HANDLE ServerName,
    WCHAR *Device,
    DWORD *Type)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 21 */
NET_API_STATUS
__stdcall
NetrServerGetInfo(
    SRVSVC_HANDLE ServerName,
    DWORD Level,
    LPSERVER_INFO InfoStruct)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 22 */
NET_API_STATUS
__stdcall
NetrServerSetInfo(
    SRVSVC_HANDLE ServerName,
    DWORD Level,
    LPSERVER_INFO ServerInfo,
    DWORD *ParmErr)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 23 */
NET_API_STATUS
__stdcall
NetrServerDiskEnum(
    SRVSVC_HANDLE ServerName,
    DWORD Level,
    DISK_ENUM_CONTAINER *DiskInfoStruct,
    DWORD PreferedMaximumLength,
    DWORD *TotalEntries,
    DWORD *ResumeHandle)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 24 */
NET_API_STATUS
__stdcall
NetrServerStatisticsGet(
    SRVSVC_HANDLE ServerName,
    WCHAR *Service,
    DWORD Level,
    DWORD Options,
    LPSTAT_SERVER_0 *InfoStruct)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 25 */
NET_API_STATUS
__stdcall
NetrServerTransportAdd(
    SRVSVC_HANDLE ServerName,
    DWORD Level,
    LPSERVER_TRANSPORT_INFO_0 Buffer)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 26 */
NET_API_STATUS
__stdcall
NetrServerTransportEnum(
    SRVSVC_HANDLE ServerName,
    LPSERVER_XPORT_ENUM_STRUCT InfoStruct,
    DWORD PreferedMaximumLength,
    DWORD *TotalEntries,
    DWORD *ResumeHandle)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 27 */
NET_API_STATUS
__stdcall
NetrServerTransportDel(
    SRVSVC_HANDLE ServerName,
    DWORD Level,
    LPSERVER_TRANSPORT_INFO_0 Buffer)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 28 */
NET_API_STATUS
__stdcall
NetrRemoteTOD(
    SRVSVC_HANDLE ServerName,
    LPTIME_OF_DAY_INFO *BufferPtr)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 29 */
void
__stdcall
Opnum29NotUsedOnWire(void)
{
    UNIMPLEMENTED;
}


/* Function 30 */
NET_API_STATUS
__stdcall
NetprPathType(
    SRVSVC_HANDLE ServerName,
    WCHAR *PathName,
    DWORD *PathType,
    DWORD Flags)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 31 */
NET_API_STATUS
__stdcall
NetprPathCanonicalize(
    SRVSVC_HANDLE ServerName,
    WCHAR *PathName,
    unsigned char *Outbuf,
    DWORD OutbufLen,
    WCHAR *Prefix,
    DWORD *PathType,
    DWORD Flags)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 32 */
long
__stdcall
NetprPathCompare(
    SRVSVC_HANDLE ServerName,
    WCHAR *PathName1,
    WCHAR *PathName2,
    DWORD PathType,
    DWORD Flags)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 33 */
NET_API_STATUS
__stdcall
NetprNameValidate(
    SRVSVC_HANDLE ServerName,
    WCHAR * Name,
    DWORD NameType,
    DWORD Flags)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 34 */
NET_API_STATUS
__stdcall
NetprNameCanonicalize(
    SRVSVC_HANDLE ServerName,
    WCHAR *Name,
    WCHAR *Outbuf,
    DWORD OutbufLen,
    DWORD NameType,
    DWORD Flags)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 35 */
long
__stdcall
NetprNameCompare(
    SRVSVC_HANDLE ServerName,
    WCHAR *Name1,
    WCHAR *Name2,
    DWORD NameType,
    DWORD Flags)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 36 */
NET_API_STATUS
__stdcall
NetrShareEnumSticky(
    SRVSVC_HANDLE ServerName,
    LPSHARE_ENUM_STRUCT InfoStruct,
    DWORD PreferedMaximumLength,
    DWORD *TotalEntries,
    DWORD *ResumeHandle)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 37 */
NET_API_STATUS
__stdcall
NetrShareDelStart(
    SRVSVC_HANDLE ServerName,
    WCHAR *NetName,
    DWORD Reserved,
    PSHARE_DEL_HANDLE ContextHandle)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 38 */
NET_API_STATUS
__stdcall
NetrShareDelCommit(
    PSHARE_DEL_HANDLE ContextHandle)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 39 */
DWORD
__stdcall
NetrpGetFileSecurity(
    SRVSVC_HANDLE ServerName,
    WCHAR *ShareName,
    WCHAR *lpFileName,
    SECURITY_INFORMATION RequestedInformation,
    PADT_SECURITY_DESCRIPTOR *SecurityDescriptor)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 40 */
DWORD
__stdcall
NetrpSetFileSecurity(
    SRVSVC_HANDLE ServerName,
    WCHAR *ShareName,
    WCHAR *lpFileName,
    SECURITY_INFORMATION SecurityInformation,
    PADT_SECURITY_DESCRIPTOR SecurityDescriptor)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 41 */
NET_API_STATUS
__stdcall
NetrServerTransportAddEx(
    SRVSVC_HANDLE ServerName,
    DWORD Level,
    LPTRANSPORT_INFO Buffer)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 42 */
void
__stdcall
Opnum42NotUsedOnWire(void)
{
    UNIMPLEMENTED;
}


/* Function 43 */
NET_API_STATUS
__stdcall
NetrDfsGetVersion(
    SRVSVC_HANDLE ServerName,
    DWORD *Version)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 44 */
NET_API_STATUS
__stdcall
NetrDfsCreateLocalPartition(
    SRVSVC_HANDLE ServerName,
    WCHAR *ShareName,
    GUID *EntryUid,
    WCHAR *EntryPrefix,
    WCHAR *ShortName,
    LPNET_DFS_ENTRY_ID_CONTAINER RelationInfo,
    int Force)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 45 */
NET_API_STATUS
__stdcall
NetrDfsDeleteLocalPartition(
    SRVSVC_HANDLE ServerName,
    GUID *Uid,
    WCHAR *Prefix)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 46 */
NET_API_STATUS
__stdcall
NetrDfsSetLocalVolumeState(
    SRVSVC_HANDLE ServerName,
    GUID *Uid,
    WCHAR *Prefix,
    unsigned long State)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 47 */
void
__stdcall
Opnum47NotUsedOnWire(void)
{
    UNIMPLEMENTED;
}


/* Function 48 */
NET_API_STATUS
__stdcall
NetrDfsCreateExitPoint(
    SRVSVC_HANDLE ServerName,
    GUID *Uid,
    WCHAR *Prefix,
    unsigned long Type,
    DWORD ShortPrefixLen,
    WCHAR *ShortPrefix)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 49 */
NET_API_STATUS
__stdcall
NetrDfsDeleteExitPoint(
    SRVSVC_HANDLE ServerName,
    GUID *Uid,
    WCHAR *Prefix,
    unsigned long Type)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 50 */
NET_API_STATUS
__stdcall
NetrDfsModifyPrefix(
    SRVSVC_HANDLE ServerName,
    GUID *Uid,
    WCHAR *Prefix)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 51 */
NET_API_STATUS
__stdcall
NetrDfsFixLocalVolume(
    SRVSVC_HANDLE ServerName,
    WCHAR *VolumeName,
    unsigned long EntryType,
    unsigned long ServiceType,
    WCHAR *StgId,
    GUID *EntryUid,
    WCHAR *EntryPrefix,
    LPNET_DFS_ENTRY_ID_CONTAINER RelationInfo,
    unsigned long CreateDisposition)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 52 */
NET_API_STATUS
__stdcall
NetrDfsManagerReportSiteInfo(
    SRVSVC_HANDLE ServerName,
    LPDFS_SITELIST_INFO *ppSiteInfo)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 53 */
NET_API_STATUS
__stdcall
NetrServerTransportDelEx(
    SRVSVC_HANDLE ServerName,
    DWORD Level,
    LPTRANSPORT_INFO Buffer)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/* EOF */
