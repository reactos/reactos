/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         NetAPI DLL
 * FILE:            dll/win32/netapi32/obsolete.c
 * PURPOSE:         Obsolete functions
 * PROGRAMMERS:     Eric Kohl (eric.kohl@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "netapi32.h"

#include <lmalert.h>
#include <lmaudit.h>
#include <lmconfig.h>
#include <lmerrlog.h>
#include <lmmsg.h>
#include <lmrepl.h>
#include <lmsvc.h>

/* FUNCTIONS *****************************************************************/

NET_API_STATUS
WINAPI
NetAlertRaise(
    _In_ LPCWSTR AlertEventName,
    _In_ LPVOID Buffer,
    _In_ DWORD BufferSize)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetAlertRaiseEx(
    _In_ LPCWSTR AlertEventName,
    _In_ LPVOID VariableInfo,
    _In_ DWORD VariableInfoSize,
    _In_ LPCWSTR ServiceName)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetAuditClear(
    _In_opt_ LPCWSTR server,
    _In_opt_ LPCWSTR backupfile,
    _In_opt_ LPCWSTR service)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetAuditRead(
    _In_opt_ LPCWSTR server,
    _In_opt_ LPCWSTR service,
    _In_ LPHLOG auditloghandle,
    _In_ DWORD offset,
    _In_opt_ LPDWORD reserved1,
    _In_ DWORD reserved2,
    _In_ DWORD offsetflag,
    _Out_ LPBYTE *bufptr,
    _In_ DWORD prefmaxlen,
    _Out_ LPDWORD bytesread,
    _Out_ LPDWORD totalavailable)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetAuditWrite(
    _In_ DWORD type,
    _In_ LPBYTE buf,
    _In_ DWORD numbytes,
    _In_opt_ LPCWSTR service,
    _In_opt_ LPBYTE reserved)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetConfigGet(
    _In_opt_ LPCWSTR server,
    _In_ LPCWSTR component,
    _In_ LPCWSTR parameter,
    _Out_ LPBYTE *bufptr)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetConfigGetAll(
    _In_opt_ LPCWSTR server,
    _In_ LPCWSTR component,
    _Out_ LPBYTE *bufptr)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetConfigSet(
    _In_opt_ LPCWSTR server,
    _In_opt_ LPCWSTR reserved1,
    _In_ LPCWSTR component,
    _In_ DWORD level,
    _In_ DWORD reserved2,
    _In_ LPBYTE buf,
    _In_ DWORD reserved3)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetErrorLogClear(
    _In_opt_ LPCWSTR UncServerName,
    _In_opt_ LPCWSTR BackupFile,
    _In_opt_ LPBYTE Reserved)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetErrorLogRead(
    _In_opt_ LPCWSTR UncServerName,
    _In_opt_ LPWSTR Reserved1,
    _In_ LPHLOG ErrorLogHandle,
    _In_ DWORD Offset,
    _In_opt_ LPDWORD Reserved2,
    _In_ DWORD Reserved3,
    _In_ DWORD OffsetFlag,
    _Outptr_ LPBYTE *BufPtr,
    _In_ DWORD PrefMaxSize,
    _Out_ LPDWORD BytesRead,
    _Out_ LPDWORD TotalAvailable)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetErrorLogWrite(
    _In_opt_ LPBYTE Reserved1,
    _In_ DWORD Code,
    _In_ LPCWSTR Component,
    _In_ LPBYTE Buffer,
    _In_ DWORD NumBytes,
    _In_ LPBYTE MsgBuf,
    _In_ DWORD StrCount,
    _In_opt_ LPBYTE Reserved2)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetMessageBufferSend(
    _In_ LPCWSTR servername,
    _In_ LPCWSTR msgname,
    _In_ LPCWSTR fromname,
    _In_ LPBYTE buf,
    _In_ DWORD buflen)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetMessageNameAdd(
    _In_ LPCWSTR servername,
    _In_ LPCWSTR msgname)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetMessageNameDel(
    _In_ LPCWSTR servername,
    _In_ LPCWSTR msgname)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetMessageNameEnum(
    _In_ LPCWSTR servername,
    _In_ DWORD level,
    _Out_ LPBYTE *bufptr,
    _In_ DWORD prefmaxlen,
    _Out_ LPDWORD entriesread,
    _Out_ LPDWORD totalentries,
    _Inout_ LPDWORD resume_handle)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetMessageNameGetInfo(
    _In_ LPCWSTR servername,
    _In_ LPCWSTR msgname,
    _In_ DWORD level,
    _Out_ LPBYTE *bufptr)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetReplExportDirAdd(
    _In_opt_ LPCWSTR servername,
    _In_ DWORD level,
    _In_ const LPBYTE buf,
    _Out_opt_ LPDWORD parm_err)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetReplExportDirDel(
    _In_opt_ LPCWSTR servername,
    _In_ LPCWSTR dirname)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetReplExportDirEnum(
    _In_opt_ LPCWSTR servername,
    _In_ DWORD level,
    _Out_ LPBYTE *bufptr,
    _In_ DWORD prefmaxlen,
    _Out_ LPDWORD entriesread,
    _Out_ LPDWORD totalentries,
    _Inout_opt_ LPDWORD resumehandle)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetReplExportDirGetInfo(
    _In_opt_ LPCWSTR servername,
    _In_ LPCWSTR dirname,
    _In_ DWORD level,
    _Out_ LPBYTE *bufptr)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetReplExportDirLock(
    _In_opt_ LPCWSTR servername,
    _In_ LPCWSTR dirname)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetReplExportDirSetInfo(
    _In_opt_ LPCWSTR servername,
    _In_ LPCWSTR dirname,
    _In_ DWORD level,
    _In_ const LPBYTE buf,
    _Out_opt_ LPDWORD parm_err)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetReplExportDirUnlock(
    _In_opt_ LPCWSTR servername,
    _In_ LPCWSTR dirname,
    _In_ DWORD unlockforce)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetReplGetInfo(
    _In_opt_ LPCWSTR servername,
    _In_ DWORD level,
    _Out_ LPBYTE *bufptr)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetReplImportDirAdd(
    _In_opt_ LPCWSTR servername,
    _In_ DWORD level,
    _In_ const LPBYTE buf,
    _Out_opt_ LPDWORD parm_err)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetReplImportDirDel(
    _In_opt_ LPCWSTR servername,
    _In_ LPCWSTR dirname)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetReplImportDirEnum(
    _In_opt_ LPCWSTR servername,
    _In_ DWORD level,
    _Out_ LPBYTE *bufptr,
    _In_ DWORD prefmaxlen,
    _Out_ LPDWORD entriesread,
    _Out_ LPDWORD totalentries,
    _Inout_opt_ LPDWORD resumehandle)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetReplImportDirGetInfo(
    _In_opt_ LPCWSTR servername,
    _In_ LPCWSTR dirname,
    _In_ DWORD level,
    _Out_ LPBYTE *bufptr)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetReplImportDirLock(
    _In_opt_ LPCWSTR servername,
    _In_ LPCWSTR dirname)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetReplImportDirUnlock(
    _In_opt_ LPCWSTR servername,
    _In_ LPCWSTR dirname,
    _In_ DWORD unlockforce)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetReplSetInfo(
    _In_opt_ LPCWSTR servername,
    _In_ DWORD level,
    _In_ const LPBYTE buf,
    _Out_opt_ LPDWORD parm_err)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetServiceControl(
    _In_opt_ LPCWSTR servername,
    _In_ LPCWSTR service,
    _In_ DWORD opcode,
    _In_ DWORD arg,
    _Outptr_ LPBYTE *bufptr)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetServiceEnum(
    _In_opt_ LPCWSTR servername,
    _In_ DWORD level,
    _Outptr_ LPBYTE *bufptr,
    _In_ DWORD prefmaxlen,
    _Out_ LPDWORD entriesread,
    _Out_ LPDWORD totalentries,
    _Inout_opt_ LPDWORD resume_handle)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetServiceGetInfo(
    _In_opt_ LPCWSTR servername,
    _In_ LPCWSTR service,
    _In_ DWORD level,
    _Outptr_ LPBYTE *bufptr)
{
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS
WINAPI
NetServiceInstall(
    _In_opt_ LPCWSTR servername,
    _In_ LPCWSTR service,
    _In_ DWORD argc,
    _In_reads_(argc) LPCWSTR argv[],
    _Outptr_ LPBYTE *bufptr)
{
    return ERROR_NOT_SUPPORTED;
}

/* EOF */
