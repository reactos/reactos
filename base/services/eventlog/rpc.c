/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             services/eventlog/rpc.c
 * PURPOSE:          Event logging service
 * COPYRIGHT:        Copyright 2005 Saveliy Tretiakov            
 */

/* INCLUDES *****************************************************************/

#include "eventlog.h"
#ifdef RPC_ENABLED

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

/* Function 0 */
NTSTATUS ElfrClearELFW(
    /* [in] */  LOGHANDLE Handle,
    /* [in] */  PUNICODE_STRING BackupName)
{
    DPRINT("UNIMPLEMENTED");
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 1 */
NTSTATUS ElfrBackupELFW(
    /* [in] */  LOGHANDLE Handle,
    /* [in] */  PUNICODE_STRING BackupName)
{
    DPRINT("UNIMPLEMENTED");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 2 */
NTSTATUS ElfrCloseEL(
    /* [out][in] */ PLOGHANDLE Handle)
{
    DPRINT("UNIMPLEMENTED");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 3 */
NTSTATUS ElfrDeregisterEventSource(
    /* [out][in] */ PLOGHANDLE Handle)
{
    DPRINT("UNIMPLEMENTED");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 4 */
NTSTATUS ElfrNumberOfRecords(
    /* [in] */  LOGHANDLE Handle,
    /* [out] */ long __RPC_FAR * NumberOfRecords)
{
    DPRINT("UNIMPLEMENTED");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 5 */
NTSTATUS ElfrOldestRecord(
    /* [in] */  LOGHANDLE LogHandle,
    /* [out] */ long __RPC_FAR * OldestRecNumber)
{
    DPRINT("EventLogGetOldestRec UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 6 */
NTSTATUS ElfrChangeNotify(void)
{
    DPRINT("EventLogChangeNotify UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 7 */
NTSTATUS ElfrOpenELW(
    /* [unique][in] */  LPWSTR ServerName,
    /* [in] */  PUNICODE_STRING FileName,
    /* [in] */  PUNICODE_STRING NullStr,
    /* [in] */  long MajorVer,
    /* [in] */  long MinorVer,
    /* [out] */ PLOGHANDLE Handle)
{
    DPRINT("EventLogOpenW UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 8 */
NTSTATUS ElfrRegisterEventSourceW(
    /* [unique][in] */  LPWSTR ServerName,
    /* [in] */  PUNICODE_STRING LogName,
    /* [in] */  PUNICODE_STRING NullStr,
    /* [in] */  long MajorVer,
    /* [in] */  long MinorVer,
    /* [out] */ PLOGHANDLE Handle)
{
    DPRINT("EventLogRegSrcW UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 9 */
NTSTATUS ElfrOpenBELW(
    /* [unique][in] */  LPWSTR ServerName,
    /* [in] */  PUNICODE_STRING BackupName,
    /* [in] */  long MajorVer,
    /* [in] */  long MinorVer,
    /* [out] */ PLOGHANDLE Handle)
{
    DPRINT("EventLogOpenBackupW UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 10 */
NTSTATUS ElfrReadELW(
    /* [in] */  LOGHANDLE Handle,
    /* [in] */  long Flags,
    /* [in] */  long Offset,
    /* [in] */  long BufSize,
    /* [size_is][out] */unsigned char __RPC_FAR * Buffer,
    /* [out] */ long __RPC_FAR * BytesRead,
    /* [out] */ long __RPC_FAR * BytesNeeded)
{
    DPRINT("EventLogReadW UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 11 */
NTSTATUS ElfrReportEventW(
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
    DPRINT("EventLogReportEventW UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 12 */
NTSTATUS ElfrClearELFA(
    /* [in] */          LOGHANDLE Handle,
    /* [unique][in] */  PANSI_STRING BackupName)
{
    DPRINT("EventLogClearA UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 13 */
NTSTATUS ElfrBackupELFA(
    /* [in] */  LOGHANDLE Handle,
    /* [in] */  PANSI_STRING BackupName)
{
    DPRINT("EventLogBackupA UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 14 */
NTSTATUS ElfrOpenELA(
    /* [unique][in] */  LPSTR ServerName,
    /* [in] */  PANSI_STRING LogName,
    /* [in] */  PANSI_STRING NullStr,
    /* [in] */  long MajorVer,
    /* [in] */  long MinorVer,
    /* [out] */ PLOGHANDLE Handle)
{
    UNICODE_STRING logname = { 0 }, servername = { 0 }, StrNull = { 0 };
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

    status = EventLogOpenW(servername.Buffer, logname.Buffer,
                           StrNull, MajorVer, MinorVer, Handle);

    RtlFreeUnicodeString(&servername);
    RtlFreeUnicodeString(&logname);

    return status;
}


/* Function 15 */
NTSTATUS ElfrRegisterEventSourceA(
    /* [unique][in] */  LPSTR ServerName,
    /* [in] */  PANSI_STRING LogName,
    /* [in] */  PANSI_STRING NullStr,
    /* [in] */  long MajorVer,
    /* [in] */  long MinorVer,
    /* [out] */ PLOGHANDLE Handle)
{
    DPRINT("EventLogRegSrcA UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 16 */
NTSTATUS ElfrOpenBELA(
    /* [unique][in] */  LPSTR ServerName,
    /* [in] */  PANSI_STRING BakckupName,
    /* [in] */  long MajorVer,
    /* [in] */  long MinorVer,
    /* [out] */ PLOGHANDLE Handle)
{
    DPRINT("EventLogOpenBackupA UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 17 */
NTSTATUS ElfrReadELA(
    /* [in] */  LOGHANDLE Handle,
    /* [in] */  long Flags,
    /* [in] */  long Offset,
    /* [in] */  long BufSize,
    /* [size_is][out] */unsigned char __RPC_FAR * Buffer,
    /* [out] */ long __RPC_FAR * BytesRead,
    /* [out] */ long __RPC_FAR * BytesNeeded)
{
    DPRINT("EventLogReadA UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 18 */
NTSTATUS ElfrReportEventA(
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
    DPRINT("EventLogReportEventA UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 19 */
NTSTATUS ElfrRegisterClusterSvc(void)
{
    DPRINT("EventLogRegisterClusterSvc UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 20 */
NTSTATUS ElfrDeregisterClusterSvc(void)
{
    DPRINT("EventLogDeregisterClusterSvc UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 21 */
NTSTATUS ElfrWriteClusterEvents(void)
{
    DPRINT("EventLogWriteClusterEvents UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 22 */
NTSTATUS ElfrGetLogInformation(
    /* [in] */  LOGHANDLE Handle,
    /* [in] */  long InfoLevel,
    /* [size_is][out] */unsigned char __RPC_FAR * Buffer,
    /* [in] */  long BufSize,
    /* [out] */ long __RPC_FAR * BytesNeeded)
{
    DPRINT("EventLogGetInfo UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 23 */
NTSTATUS ElfrFlushEL(
    /* [in] */  LOGHANDLE Handle)
{
    DbgPrint("EventLogFlush UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}

#else
void func(handle_t h) { }
#endif  // RPC_ENABLED

void __RPC_FAR *__RPC_USER midl_user_allocate(size_t len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}

void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}
