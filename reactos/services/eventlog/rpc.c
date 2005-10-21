/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS
 * FILE:             services/eventlog/rpc.c
 * PURPOSE:          Event logging service
 * PROGRAMMER:       Saveliy Tretiakov (savelity@mail.ru)
 */
 
#include "eventlog.h"

DWORD STDCALL RpcThreadRoutine(LPVOID lpParameter)
{
    RPC_STATUS Status;

    Status = RpcServerUseProtseqEpW(L"ncacn_np",
                                    20,
                                    L"\\pipe\\EventLog",
                                    NULL);
    if(Status != RPC_S_OK)
    {
        DPRINT("RpcServerUseProtseqEpW() failed (Status %lx)\n", Status);
        return 0;
    }

    Status = RpcServerRegisterIf(eventlog_ServerIfHandle, NULL, NULL);
    
    if(Status != RPC_S_OK)
    {
        DPRINT("RpcServerRegisterIf() failed (Status %lx)\n", Status);
        return 0;
    }

    Status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, FALSE);
    
    if(Status != RPC_S_OK)
    {
        DPRINT("RpcServerListen() failed (Status %lx)\n", Status);
    }

    return 0;
}

/* Function 0 */
NTSTATUS EventLogClearW(
	handle_t BindingHandle,
	LOGHANDLE Handle,
	wchar_t *BackupName)
{
    DPRINT("EventLogClearW UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 1 */
NTSTATUS EventLogBackupW(
	handle_t BindingHandle,
	LOGHANDLE Handle,
	wchar_t *FileName)
{
    DPRINT("EventLogBackupW UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}

	
/* Function 2 */
NTSTATUS EventLogClose(
	handle_t BindingHandle,
	PLOGHANDLE Handle)
{
    DPRINT("EventLogClose UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}
   

/* Function 3 */
NTSTATUS EventLogUnregSrc(
	handle_t BindingHandle,
	PLOGHANDLE Handle)
{
    DPRINT("EventLogUnregSrc UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 4 */
NTSTATUS EventLogRecordsNumber(
	handle_t BindingHandle,
	LOGHANDLE Handle,
	unsigned long *RecordsNumber)
{
    DPRINT("EventLogRecordsNumber UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 5 */
NTSTATUS EventLogGetOldestRec(
	handle_t BindingHandle,
	LOGHANDLE Handle,
	unsigned long *OldestRecNumber)
{
    DPRINT("EventLogGetOldestRec UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* FIXME */
NTSTATUS Unknown6(handle_t BindingHandle)
{
    DPRINT("Unknown6() called\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 7 */
NTSTATUS EventLogOpenW(
	handle_t BindingHandle,
	LPWSTR ServerName,
	wchar_t *FileName, 
	wchar_t *NullStr, 
	unsigned long MajorVer,
	unsigned long MinorVer,
	PLOGHANDLE Handle)
{
    DPRINT("EventLogOpenW UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}

		
/* Function 8 */
NTSTATUS EventLogRegSrcW(
	handle_t BindingHandle,
	LPWSTR ServerName,
	wchar_t *LogName, 
	wchar_t *NullStr, 
	unsigned long MajorVer,
	unsigned long MinorVer,
	PLOGHANDLE Handle)
{
    DPRINT("EventLogRegSrcW UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}

		
/* Function 9 */
NTSTATUS EventLogOpenBackupW(
	handle_t BindingHandle,
	LPWSTR ServerName,
	wchar_t *BackupName, 
	unsigned long MajorVer,
	unsigned long MinorVer,
	PLOGHANDLE Handle)
{
    DPRINT("EventLogOpenBackupW UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 10 */
NTSTATUS EventLogReadW(
	handle_t BindingHandle,
	LOGHANDLE Handle,
	unsigned long Flags,
	unsigned long Offset,
	unsigned long BufSize,
	unsigned char *Buffer,
	unsigned long *BytesRead,
	unsigned long *BytesNeeded)
{
    DPRINT("EventLogReadW UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 11 */
NTSTATUS EventLogReportEventW(
	handle_t BindingHandle,
	LOGHANDLE Handle,
	unsigned long Time,
	unsigned short Type,
	unsigned short Category,
	unsigned long ID,
	unsigned short NumStrings,
	unsigned long DataSize,
	wchar_t *ComputerName,
	unsigned char *SID,
	wchar_t *Strings,
	unsigned char *Data,
	unsigned short Flags)
{
    DPRINT("EventLogReportEventW UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}

		
/* Function 12 */
NTSTATUS EventLogClearA(
	handle_t BindingHandle,
	LOGHANDLE Handle,
	char *BackupName)
{
    DPRINT("EventLogClearA UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 13 */
NTSTATUS EventLogBackupA(
	handle_t BindingHandle,
	LOGHANDLE Handle,
	char *BackupName)
{
    DPRINT("EventLogBackupA UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 14 */
NTSTATUS EventLogOpenA(
	handle_t BindingHandle,
	LPSTR ServerName,
	char *LogName, 
	char *NullStr, 
	unsigned long MajorVer,
	unsigned long MinorVer,
	PLOGHANDLE Handle)
{
    DPRINT("EventLogOpenA UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 15 */
NTSTATUS EventLogRegSrcA(
	handle_t BindingHandle,
	LPSTR ServerName,
	char *LogName, 
	char *NullStr,
	unsigned long MajorVer,
	unsigned long MinorVer,
	PLOGHANDLE Handle)
{
    DPRINT("EventLogRegSrcA UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 16 */
NTSTATUS EventLogOpenBackupA(
	handle_t BindingHandle,
	LPSTR ServerName,
	char *BackupName, 
	unsigned long MajorVer,
	unsigned long MinorVer,
	PLOGHANDLE Handle)
{
    DPRINT("EventLogOpenBackupA UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 17 */
NTSTATUS EventLogReadA(
	handle_t BindingHandle,
	LOGHANDLE Handle,
	unsigned long Flags,
	unsigned long Offset,
	unsigned long BufSize,
	unsigned char *Buffer,
	unsigned long *BytesRead,
	unsigned long *BytesNeeded)
{
    DPRINT("EventLogReadA UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 18 */
NTSTATUS EventLogReportEventA(
	handle_t BindingHandle,
	LOGHANDLE Handle,
	unsigned long Time,
	unsigned short Type,
	unsigned short Category,
	unsigned long ID,
	unsigned short NumStrings,
	unsigned long DataSize,
	char *ComputerName,
	unsigned char *SID,
	char* Strings,
	unsigned char *Data,
	unsigned short Flags)
{
    DPRINT("EventLogReportEventA UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* FIXME */
NTSTATUS Unknown19(handle_t BindingHandle)
{
    DPRINT("Unknown19 called\n");
    return STATUS_NOT_IMPLEMENTED;
}
	
	
/* FIXME */
NTSTATUS Unknown20(handle_t BindingHandle)
{
    DPRINT("Unknown20 called\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* FIXME */
NTSTATUS Unknown21(handle_t BindingHandle)
{
    DPRINT("Unknown21 called\n");
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 22 */
NTSTATUS EventLogGetInfo(
	handle_t BindingHandle,
	LOGHANDLE Handle,
	unsigned long InfoLevel,
	unsigned char *Buffer,
	unsigned long BufSize,
	unsigned long *BytesNeeded)
{
    DPRINT("EventLogGetInfo UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}


void __RPC_FAR * __RPC_USER midl_user_allocate(size_t len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}
