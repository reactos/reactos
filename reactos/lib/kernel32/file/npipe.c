/* $Id: npipe.c,v 1.8 2001/10/21 19:06:42 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/npipe.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <windows.h>
#include <kernel32/error.h>
//#include <wchar.h>
//#include <string.h>
#include <limits.h>
#include <napi/npipe.h>

//#define NDEBUG
#include <kernel32/kernel32.h>

/* FUNCTIONS ****************************************************************/

HANDLE STDCALL
CreateNamedPipeA(LPCSTR lpName,
		 DWORD dwOpenMode,
		 DWORD dwPipeMode,
		 DWORD nMaxInstances,
		 DWORD nOutBufferSize,
		 DWORD nInBufferSize,
		 DWORD nDefaultTimeOut,
		 LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
   HANDLE NamedPipeHandle;
   UNICODE_STRING NameU;
   ANSI_STRING NameA;
   
   RtlInitAnsiString(&NameA, (LPSTR)lpName);
   RtlAnsiStringToUnicodeString(&NameU, &NameA, TRUE);
   
   NamedPipeHandle = CreateNamedPipeW(NameU.Buffer,
				      dwOpenMode,
				      dwPipeMode,
				      nMaxInstances,
				      nOutBufferSize,
				      nInBufferSize,
				      nDefaultTimeOut,
				      lpSecurityAttributes);
   
   RtlFreeUnicodeString(&NameU);
   
   return(NamedPipeHandle);
}


HANDLE STDCALL
CreateNamedPipeW(LPCWSTR lpName,
		 DWORD dwOpenMode,
		 DWORD dwPipeMode,
		 DWORD nMaxInstances,
		 DWORD nOutBufferSize,
		 DWORD nInBufferSize,
		 DWORD nDefaultTimeOut,
		 LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
   UNICODE_STRING NamedPipeName;
   BOOL Result;
   NTSTATUS Status;
   OBJECT_ATTRIBUTES ObjectAttributes;
   HANDLE PipeHandle;
   ACCESS_MASK DesiredAccess;
   ULONG CreateOptions;
   ULONG CreateDisposition;
   BOOLEAN WriteModeMessage;
   BOOLEAN ReadModeMessage;
   BOOLEAN NonBlocking;
   IO_STATUS_BLOCK Iosb;
   ULONG ShareAccess;
   LARGE_INTEGER DefaultTimeOut;
   
   Result = RtlDosPathNameToNtPathName_U((LPWSTR)lpName,
					 &NamedPipeName,
					 NULL,
					 NULL);
   if (!Result)
     {
	SetLastError(ERROR_PATH_NOT_FOUND);
	return(INVALID_HANDLE_VALUE);
     }
   
   DPRINT("Pipe name: %wZ\n", &NamedPipeName);
   DPRINT("Pipe name: %S\n", NamedPipeName.Buffer);
   
   InitializeObjectAttributes(&ObjectAttributes,
			      &NamedPipeName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);
   
   DesiredAccess = 0;
   
   ShareAccess = 0;
   
   CreateDisposition = FILE_OPEN_IF;
   
   CreateOptions = 0;
   if (dwOpenMode & FILE_FLAG_WRITE_THROUGH)
     {
	CreateOptions = CreateOptions | FILE_WRITE_THROUGH;
     }
   if (dwOpenMode & FILE_FLAG_OVERLAPPED)
     {
	CreateOptions = CreateOptions | FILE_SYNCHRONOUS_IO_ALERT;
     }
   if (dwOpenMode & PIPE_ACCESS_DUPLEX)
     {
	CreateOptions = CreateOptions | FILE_PIPE_FULL_DUPLEX;
     }
   else if (dwOpenMode & PIPE_ACCESS_INBOUND)
     {
	CreateOptions = CreateOptions | FILE_PIPE_INBOUND;
     }
   else if (dwOpenMode & PIPE_ACCESS_OUTBOUND)
     {
	CreateOptions = CreateOptions | FILE_PIPE_OUTBOUND;
     }
   
   if (dwPipeMode & PIPE_TYPE_BYTE)
     {
	WriteModeMessage = FALSE;
     }
   else if (dwPipeMode & PIPE_TYPE_MESSAGE)
     {
	WriteModeMessage = TRUE;
     }
   else
     {
	WriteModeMessage = FALSE;
     }
   
   if (dwPipeMode & PIPE_READMODE_BYTE)
     {
	ReadModeMessage = FALSE;
     }
   else if (dwPipeMode & PIPE_READMODE_MESSAGE)
     {
	ReadModeMessage = TRUE;
     }
   else
     {
	ReadModeMessage = FALSE;
     }
   
   if (dwPipeMode & PIPE_WAIT)
     {
	NonBlocking = FALSE;
     }
   else if (dwPipeMode & PIPE_NOWAIT)
     {
	NonBlocking = TRUE;
     }
   else
     {
	NonBlocking = FALSE;
     }
   
   if (nMaxInstances >= PIPE_UNLIMITED_INSTANCES)
     {
	nMaxInstances = ULONG_MAX;
     }
   
   DefaultTimeOut.QuadPart = nDefaultTimeOut * -10000;
   
   Status = NtCreateNamedPipeFile(&PipeHandle,
				  DesiredAccess,
				  &ObjectAttributes,
				  &Iosb,
				  ShareAccess,
				  CreateDisposition,
				  CreateOptions,
				  WriteModeMessage,
				  ReadModeMessage,
				  NonBlocking,
				  nMaxInstances,
				  nInBufferSize,
				  nOutBufferSize,
				  &DefaultTimeOut);
   
   RtlFreeUnicodeString(&NamedPipeName);
   
   if (!NT_SUCCESS(Status))
     {
	DPRINT("NtCreateNamedPipe failed (Status %x)!\n", Status);
	SetLastErrorByStatus (Status);
	return(INVALID_HANDLE_VALUE);
     }
   
   return(PipeHandle);
}


BOOL STDCALL
WaitNamedPipeA(LPCSTR lpNamedPipeName,
	       DWORD nTimeOut)
{
   BOOL r;
   UNICODE_STRING NameU;
   ANSI_STRING NameA;
   
   RtlInitAnsiString(&NameA, (LPSTR)lpNamedPipeName);
   RtlAnsiStringToUnicodeString(&NameU, &NameA, TRUE);
   
   r = WaitNamedPipeW(NameU.Buffer, nTimeOut);
   
   RtlFreeUnicodeString(&NameU);
   
   return(r);
}


BOOL STDCALL
WaitNamedPipeW(LPCWSTR lpNamedPipeName,
	       DWORD nTimeOut)
{
   UNICODE_STRING NamedPipeName;
   BOOL r;
   NTSTATUS Status;
   OBJECT_ATTRIBUTES ObjectAttributes;
   NPFS_WAIT_PIPE WaitPipe;
   HANDLE FileHandle;
   IO_STATUS_BLOCK Iosb;
   
   r = RtlDosPathNameToNtPathName_U((LPWSTR)lpNamedPipeName,
				    &NamedPipeName,
				    NULL,
				    NULL);
   
   if (!r)
     {
	return(FALSE);
     }
   
   InitializeObjectAttributes(&ObjectAttributes,
			      &NamedPipeName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);
   Status = NtOpenFile(&FileHandle,
		       FILE_GENERIC_READ,
		       &ObjectAttributes,
		       &Iosb,
		       0,
		       FILE_SYNCHRONOUS_IO_ALERT);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus (Status);
	return(FALSE);
     }
   
   WaitPipe.Timeout.QuadPart = nTimeOut * -10000;
   
   Status = NtFsControlFile(FileHandle,
			    NULL,
			    NULL,
			    NULL,
			    &Iosb,
			    FSCTL_PIPE_WAIT,
			    &WaitPipe,
			    sizeof(WaitPipe),
			    NULL,
			    0);
   NtClose(FileHandle);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus (Status);
	return(FALSE);
     }
   
   return(TRUE);
}


BOOL STDCALL
ConnectNamedPipe(HANDLE hNamedPipe,
		 LPOVERLAPPED lpOverlapped)
{
   IO_STATUS_BLOCK Iosb;
   HANDLE hEvent;
   PIO_STATUS_BLOCK IoStatusBlock;
   NTSTATUS Status;
   
   if (lpOverlapped != NULL)
     {
	lpOverlapped->Internal = STATUS_PENDING;
	hEvent = lpOverlapped->hEvent;
	IoStatusBlock = (PIO_STATUS_BLOCK)lpOverlapped;
     }
   else
     {
	IoStatusBlock = &Iosb;
	hEvent = NULL;
     }
   
   Status = NtFsControlFile(hNamedPipe,
			    hEvent,
			    NULL,
			    NULL,
			    IoStatusBlock,
			    FSCTL_PIPE_LISTEN,
			    NULL,
			    0,
			    NULL,
			    0);
   if ((lpOverlapped == NULL) && (Status == STATUS_PENDING))
     {
	Status = NtWaitForSingleObject(hNamedPipe,
				       FALSE,
				       NULL);
	if (!NT_SUCCESS(Status))
	  {
	     SetLastErrorByStatus(Status);
	     return(FALSE);
	  }
	Status = Iosb.Status;
     }
   if (!NT_SUCCESS(Status) || (Status == STATUS_PENDING))
     {
	SetLastErrorByStatus (Status);
	return(FALSE);
     }
   return(TRUE);
}


BOOL STDCALL
SetNamedPipeHandleState(HANDLE hNamedPipe,
			LPDWORD lpMode,
			LPDWORD lpMaxCollectionCount,
			LPDWORD lpCollectDataTimeout)
{
   NPFS_GET_STATE GetState;
   NPFS_SET_STATE SetState;
   IO_STATUS_BLOCK Iosb;
   NTSTATUS Status;

   Status = NtFsControlFile(hNamedPipe,
			    NULL,
			    NULL,
			    NULL,
			    &Iosb,
			    FSCTL_PIPE_GET_STATE,
			    NULL,
			    0,
			    &GetState,
			    sizeof(NPFS_GET_STATE));
   if (Status == STATUS_PENDING)
     {
	Status = NtWaitForSingleObject(hNamedPipe,
				       FALSE,
				       NULL);
	if (!NT_SUCCESS(Status))
	  {
	     SetLastErrorByStatus(Status);
	     return(FALSE);
	  }
     }

   if (lpMode != NULL)
     {
	if ((*lpMode) & PIPE_READMODE_MESSAGE)
	  {
	     SetState.ReadModeMessage = TRUE;
	  }
	else
	  {
	     SetState.ReadModeMessage = FALSE;
	  }
	if ((*lpMode) & PIPE_NOWAIT)
	  {
	     SetState.NonBlocking = TRUE;
	  }
	else
	  {
	     SetState.NonBlocking = FALSE;
	  }
	SetState.WriteModeMessage = GetState.WriteModeMessage;
     }
   else
     {
	SetState.ReadModeMessage = GetState.ReadModeMessage;
	SetState.WriteModeMessage = GetState.WriteModeMessage;
	SetState.NonBlocking = SetState.NonBlocking;
     }
   
   if (lpMaxCollectionCount != NULL)
     {
	SetState.InBufferSize = *lpMaxCollectionCount;
     }
   else
     {
	SetState.InBufferSize = GetState.InBufferSize;
     }
   
   SetState.OutBufferSize = GetState.OutBufferSize;
   
   if (lpCollectDataTimeout != NULL)
     {
	SetState.Timeout.QuadPart = (*lpCollectDataTimeout) * -10000;
     }
   else
     {
	SetState.Timeout = GetState.Timeout;
     }

   Status = NtFsControlFile(hNamedPipe,
			    NULL,
			    NULL,
			    NULL,
			    &Iosb,
			    FSCTL_PIPE_SET_STATE,
			    &SetState,
			    sizeof(NPFS_SET_STATE),
			    NULL,
			    0);
   if (Status == STATUS_PENDING)
     {
	Status = NtWaitForSingleObject(hNamedPipe,
				       FALSE,
				       NULL);
	if (!NT_SUCCESS(Status))
	  {
	     SetLastErrorByStatus(Status);
	     return(FALSE);
	  }
     }

  return(TRUE);
}


WINBOOL STDCALL
CallNamedPipeA(LPCSTR lpNamedPipeName,
	       LPVOID lpInBuffer,
	       DWORD nInBufferSize,
	       LPVOID lpOutBuffer,
	       DWORD nOutBufferSize,
	       LPDWORD lpBytesRead,
	       DWORD nTimeOut)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL STDCALL
CallNamedPipeW(LPCWSTR lpNamedPipeName,
	       LPVOID lpInBuffer,
	       DWORD nInBufferSize,
	       LPVOID lpOutBuffer,
	       DWORD nOutBufferSize,
	       LPDWORD lpBytesRead,
	       DWORD nTimeOut)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL STDCALL
DisconnectNamedPipe(HANDLE hNamedPipe)
{
   IO_STATUS_BLOCK Iosb;
   NTSTATUS Status;

   Status = NtFsControlFile(hNamedPipe,
			    NULL,
			    NULL,
			    NULL,
			    &Iosb,
			    FSCTL_PIPE_DISCONNECT,
			    NULL,
			    0,
			    NULL,
			    0);
   if (Status == STATUS_PENDING)
     {
	Status = NtWaitForSingleObject(hNamedPipe,
				       FALSE,
				       NULL);
	if (!NT_SUCCESS(Status))
	  {
	     SetLastErrorByStatus(Status);
	     return(FALSE);
	  }
     }

   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return(FALSE);
     }
   return(TRUE);
}


WINBOOL STDCALL
GetNamedPipeHandleStateW(HANDLE hNamedPipe,
			 LPDWORD lpState,
			 LPDWORD lpCurInstances,
			 LPDWORD lpMaxCollectionCount,
			 LPDWORD lpCollectDataTimeout,
			 LPWSTR lpUserName,
			 DWORD nMaxUserNameSize)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL STDCALL
GetNamedPipeHandleStateA(HANDLE hNamedPipe,
			 LPDWORD lpState,
			 LPDWORD lpCurInstances,
			 LPDWORD lpMaxCollectionCount,
			 LPDWORD lpCollectDataTimeout,
			 LPSTR lpUserName,
			 DWORD nMaxUserNameSize)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL STDCALL
GetNamedPipeInfo(HANDLE hNamedPipe,
		 LPDWORD lpFlags,
		 LPDWORD lpOutBufferSize,
		 LPDWORD lpInBufferSize,
		 LPDWORD lpMaxInstances)
{
  FILE_PIPE_LOCAL_INFORMATION PipeLocalInformation;
  IO_STATUS_BLOCK StatusBlock;
  NTSTATUS Status;
  
  Status = NtQueryInformationFile(hNamedPipe,
				  &StatusBlock,
				  &PipeLocalInformation,
				  sizeof(FILE_PIPE_LOCAL_INFORMATION),
				  FilePipeLocalInformation);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }
  
  if (lpFlags != NULL)
    {
      *lpFlags = (PipeLocalInformation.NamedPipeEnd == FILE_PIPE_SERVER_END) ? PIPE_SERVER_END : PIPE_CLIENT_END;
      *lpFlags |= (PipeLocalInformation.NamedPipeType == 1) ? PIPE_TYPE_MESSAGE : PIPE_TYPE_BYTE;
    }
  
  if (lpOutBufferSize != NULL)
    *lpOutBufferSize = PipeLocalInformation.OutboundQuota;
  
  if (lpInBufferSize != NULL)
    *lpInBufferSize = PipeLocalInformation.InboundQuota;
  
  if (lpMaxInstances != NULL)
    {
      if (PipeLocalInformation.MaximumInstances >= 255)
	*lpMaxInstances = PIPE_UNLIMITED_INSTANCES;
      else
	*lpMaxInstances = PipeLocalInformation.MaximumInstances;
    }
  
  return(TRUE);
}


WINBOOL STDCALL
PeekNamedPipe(HANDLE hNamedPipe,
	      LPVOID lpBuffer,
	      DWORD nBufferSize,
	      LPDWORD lpBytesRead,
	      LPDWORD lpTotalBytesAvail,
	      LPDWORD lpBytesLeftThisMessage)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL STDCALL
TransactNamedPipe(HANDLE hNamedPipe,
		  LPVOID lpInBuffer,
		  DWORD nInBufferSize,
		  LPVOID lpOutBuffer,
		  DWORD nOutBufferSize,
		  LPDWORD lpBytesRead,
		  LPOVERLAPPED lpOverlapped)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/* EOF */
