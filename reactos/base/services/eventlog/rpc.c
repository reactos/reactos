/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             services/eventlog/rpc.c
 * PURPOSE:          Event logging service
 * COPYRIGHT:        Copyright 2005 Saveliy Tretiakov
 */

/* INCLUDES *****************************************************************/

#include "eventlog.h"

#ifdef __GNUC__
/* HACK as long as idl compiler doesn't support well PUNICODE_STRING args */
#define PANSI_STRING LPSTR
#define PUNICODE_STRING LPWSTR
#define BINDING_HANDLE handle_t BindingHandle,
#else
#define BINDING_HANDLE
#endif

/* FUNCTIONS ****************************************************************/

DWORD STDCALL RpcThreadRoutine(LPVOID lpParameter)
{
    RPC_STATUS Status;

    Status = RpcServerUseProtseqEpW(L"ncacn_np", 20, L"\\pipe\\EventLog", NULL);
    if (Status != RPC_S_OK)
    {
        DPRINT("RpcServerUseProtseqEpW() failed (Status %lx)\n", Status);
        return 0;
    }

#ifdef _MSC_VER
    Status = RpcServerRegisterIf(eventlog_v0_0_s_ifspec, NULL, NULL);
#else
    Status = RpcServerRegisterIf(eventlog_ServerIfHandle, NULL, NULL);
#endif

    if (Status != RPC_S_OK)
    {
        DPRINT("RpcServerRegisterIf() failed (Status %lx)\n", Status);
        return 0;
    }

    Status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, FALSE);

    if (Status != RPC_S_OK)
    {
        DPRINT("RpcServerListen() failed (Status %lx)\n", Status);
    }

    return 0;
}

/* Function 0x00 */
NTSTATUS ElfrClearELFW(
    BINDING_HANDLE
    /* [in] */  LOGHANDLE Handle,
    /* [in] */  PUNICODE_STRING BackupName)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 0x01 */
NTSTATUS ElfrBackupELFW(
    BINDING_HANDLE
    /* [in] */  LOGHANDLE Handle,
    /* [in] */  PUNICODE_STRING BackupName)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 0x02 */
NTSTATUS ElfrCloseEL(
    BINDING_HANDLE
    /* [out][in] */ PLOGHANDLE Handle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 0x03 */
NTSTATUS ElfrDeregisterEventSource(
    BINDING_HANDLE
    /* [out][in] */ PLOGHANDLE Handle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 0x04 */
NTSTATUS ElfrNumberOfRecords(
    BINDING_HANDLE
    /* [in] */  LOGHANDLE Handle,
    /* [out] */ long __RPC_FAR * NumberOfRecords)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 0x05 */
NTSTATUS ElfrOldestRecord(
    BINDING_HANDLE
    /* [in] */  LOGHANDLE LogHandle,
    /* [out] */ long __RPC_FAR * OldestRecNumber)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 0x06 */
NTSTATUS ElfrChangeNotify(void)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 0x07 */
NTSTATUS ElfrOpenELW(
    BINDING_HANDLE
    /* [unique][in] */  LPWSTR ServerName,
    /* [in] */  PUNICODE_STRING FileName,
    /* [in] */  PUNICODE_STRING NullStr,
    /* [in] */  long MajorVer,
    /* [in] */  long MinorVer,
    /* [out] */ PLOGHANDLE Handle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 0x08 */
NTSTATUS ElfrRegisterEventSourceW(
    BINDING_HANDLE
    /* [unique][in] */  LPWSTR ServerName,
    /* [in] */  PUNICODE_STRING LogName,
    /* [in] */  PUNICODE_STRING NullStr,
    /* [in] */  long MajorVer,
    /* [in] */  long MinorVer,
    /* [out] */ PLOGHANDLE Handle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 0x09 */
NTSTATUS ElfrOpenBELW(
    BINDING_HANDLE
    /* [unique][in] */  LPWSTR ServerName,
    /* [in] */  PUNICODE_STRING BackupName,
    /* [in] */  long MajorVer,
    /* [in] */  long MinorVer,
    /* [out] */ PLOGHANDLE Handle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 0x0a */
NTSTATUS ElfrReadELW(
    BINDING_HANDLE
    /* [in] */  LOGHANDLE Handle,
    /* [in] */  long Flags,
    /* [in] */  long Offset,
    /* [in] */  long BufSize,
    /* [size_is][out] */unsigned char __RPC_FAR * Buffer,
    /* [out] */ long __RPC_FAR * BytesRead,
    /* [out] */ long __RPC_FAR * BytesNeeded)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 0x0b */
NTSTATUS ElfrReportEventW(
    BINDING_HANDLE
    /* [in] */  LOGHANDLE Handle,
    /* [in] */  long Time,
    /* [in] */  short Type,
    /* [in] */  short Category,
    /* [in] */  long ID,
    /* [in] */  short NumStrings,
    /* [in] */  long DataSize,
    /* [in] */  PUNICODE_STRING ComputerName,
    /* [unique][in] */          unsigned char __RPC_FAR * SID,
    /* [unique][size_is][in] */ PUNICODE_STRING __RPC_FAR Strings[],
    /* [unique][size_is][in] */ unsigned char __RPC_FAR * Data,
    /* [in] */                  short Flags,
    /* [unique][out][in] */     long __RPC_FAR * unknown1,
    /* [unique][out][in] */     long __RPC_FAR * unknown2)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 0x0c */
NTSTATUS ElfrClearELFA(
    BINDING_HANDLE
    /* [in] */          LOGHANDLE Handle,
    /* [unique][in] */  PANSI_STRING BackupName)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 0x0d */
NTSTATUS ElfrBackupELFA(
    BINDING_HANDLE
    /* [in] */  LOGHANDLE Handle,
    /* [in] */  PANSI_STRING BackupName)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 0x0e */
NTSTATUS ElfrOpenELA(
    BINDING_HANDLE
    /* [unique][in] */  LPSTR ServerName,
    /* [in] */  PANSI_STRING LogName,
    /* [in] */  PANSI_STRING NullStr,
    /* [in] */  long MajorVer,
    /* [in] */  long MinorVer,
    /* [out] */ PLOGHANDLE Handle)
{
    UNICODE_STRING logname = { 0, }, servername = { 0, }, StrNull = { 0, };
    NTSTATUS status;

    if (LogName && !RtlCreateUnicodeStringFromAsciiz(&logname, LogName))
    {
        return STATUS_NO_MEMORY;
    }

    if (ServerName &&
        !RtlCreateUnicodeStringFromAsciiz(&servername, ServerName))
    {
        RtlFreeUnicodeString(&logname);
        return STATUS_NO_MEMORY;
    }

    status = ElfrOpenELW(BindingHandle, servername.Buffer, logname.Buffer,
                         StrNull.Buffer, MajorVer, MinorVer, Handle);

    RtlFreeUnicodeString(&servername);
    RtlFreeUnicodeString(&logname);

    return status;
}


/* Function 0x0f */
NTSTATUS ElfrRegisterEventSourceA(
    BINDING_HANDLE
    /* [unique][in] */  LPSTR ServerName,
    /* [in] */  PANSI_STRING LogName,
    /* [in] */  PANSI_STRING NullStr,
    /* [in] */  long MajorVer,
    /* [in] */  long MinorVer,
    /* [out] */ PLOGHANDLE Handle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 0x10 */
NTSTATUS ElfrOpenBELA(
    BINDING_HANDLE
    /* [unique][in] */  LPSTR ServerName,
    /* [in] */  PANSI_STRING BakckupName,
    /* [in] */  long MajorVer,
    /* [in] */  long MinorVer,
    /* [out] */ PLOGHANDLE Handle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 0x11 */
NTSTATUS ElfrReadELA(
    BINDING_HANDLE
    /* [in] */  LOGHANDLE Handle,
    /* [in] */  long Flags,
    /* [in] */  long Offset,
    /* [in] */  long BufSize,
    /* [size_is][out] */unsigned char __RPC_FAR * Buffer,
    /* [out] */ long __RPC_FAR * BytesRead,
    /* [out] */ long __RPC_FAR * BytesNeeded)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 0x12 */
NTSTATUS ElfrReportEventA(
    BINDING_HANDLE
    /* [in] */  LOGHANDLE Handle,
    /* [in] */  long Time,
    /* [in] */  short Type,
    /* [in] */  short Category,
    /* [in] */  long ID,
    /* [in] */  short NumStrings,
    /* [in] */  long DataSize,
    /* [in] */  PANSI_STRING ComputerName,
    /* [unique][in] */  unsigned char __RPC_FAR * SID,
    /* [unique][size_is][in] */ PANSI_STRING __RPC_FAR Strings[],
    /* [unique][size_is][in] */ unsigned char __RPC_FAR * Data,
    /* [in] */                  short Flags,
    /* [unique][out][in] */     long __RPC_FAR * unknown1,
    /* [unique][out][in] */     long __RPC_FAR * unknown2)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 0x13 */
NTSTATUS ElfrRegisterClusterSvc(void)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 0x14 */
NTSTATUS ElfrDeregisterClusterSvc(void)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 0x15 */
NTSTATUS ElfrWriteClusterEvents(void)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 0x16 */
NTSTATUS ElfrGetLogInformation(
    BINDING_HANDLE
    /* [in] */  LOGHANDLE Handle,
    /* [in] */  long InfoLevel,
    /* [size_is][out] */unsigned char __RPC_FAR * Buffer,
    /* [in] */  long BufSize,
    /* [out] */ long __RPC_FAR * BytesNeeded)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 0x17 */
NTSTATUS ElfrFlushEL(
    BINDING_HANDLE
    /* [in] */  LOGHANDLE Handle)
{
    DbgPrint("EventLogFlush UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}

void __RPC_FAR *__RPC_USER midl_user_allocate(size_t len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}

void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}
