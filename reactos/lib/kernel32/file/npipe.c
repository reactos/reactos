/* $Id: npipe.c,v 1.2 2000/05/14 09:31:01 ea Exp $
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
#include <windows.h>
#include <wchar.h>
#include <string.h>
#include <ntdll/rtl.h>

#include <kernel32/kernel32.h>

/* FUNCTIONS ****************************************************************/

HANDLE STDCALL CreateNamedPipeA(LPCSTR lpName,
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

HANDLE STDCALL CreateNamedPipeW(LPCWSTR lpName,
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
	return(INVALID_HANDLE_VALUE);
     }
   
   InitializeObjectAttributes(&ObjectAttributes,
			      &NamedPipeName,
			      0,
			      NULL,
			      NULL);
   
   DesiredAccess = 0;
   
   ShareAccess = 0;
   
   CreateDisposition = FILE_OPEN_IF;
   
   CreateOptions = 0;   
   if (dwOpenMode & FILE_FLAG_WRITE_THROUGH)
     {
	CreateOptions = CreateOptions | FILE_FLAG_WRITE_THROUGH;
     }
   if (dwOpenMode & FILE_FLAG_OVERLAPPED)
     {
	CreateOptions = CreateOptions | FILE_SYNCHRONOUS_IO_ALERT;
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
   
   DefaultTimeOut.QuadPart = nDefaultTimeOut * 1000 * 1000;
     
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
   if (!NT_SUCCESS(Status))
     {
	SetLastError(RtlNtStatusToDosError(Status));
	return(INVALID_HANDLE_VALUE);
     }
   return(PipeHandle);
}

BOOL STDCALL WaitNamedPipeA(LPCSTR lpNamedPipeName,
		    DWORD nTimeOut)
{
   BOOL r;
   UNICODE_STRING NameU;
   ANSI_STRING NameA;
   
   RtlInitAnsiString(&NameA, lpNamedPipeName);
   RtlAnsiStringToUnicodeString(&NameU, &NameA, TRUE);
   
   r = WaitNamedPipeW(NameU.Buffer, nTimeOut);
   
   RtlFreeUnicodeString(&NameU);
   
   return(r);
}

BOOL STDCALL WaitNamedPipeW(LPCWSTR lpNamedPipeName,
		    DWORD nTimeOut)
{
   UNICODE_STRING NamedPipeName;
   BOOL r;
   NTSTATUS Status;
   OBJECT_ATTRIBUTES ObjectAttributes;
   NPFS_WAIT_PIPE WaitPipe;
   HANDLE FileHandle;
   IO_STATUS_BLOCK Iosb;
   
   r = RtlDosPathNameToNtPathName_U(lpNamedPipeName,
				    &NamedPipeName,
				    NULL,
				    NULL);
   
   if (!r)
     {
	return(FALSE);
     }
   
   InitializeObjectAttributes(&ObjectAttributes,
			      &NamedPipeName,
			      0,
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
	SetLastError(RtlNtStatusToDosError(Status));
	return(FALSE);
     }
   
   WaitPipe.Timeout.QuadPart = nTimeOut * 1000 * 1000;
   
   Status = NtFsControlFile(FileHandle,
			    NULL,
			    NULL,
			    NULL,
			    &Iosb,
			    FSCTL_WAIT_PIPE,
			    &WaitPipe,
			    sizeof(WaitPipe),
			    NULL,
			    0);
   if (!NT_SUCCESS(Status))
     {
	SetLastError(RtlNtStatusToDosError(Status));
	return(FALSE);
     }
   
   NtClose(FileHandle);
   return(TRUE);
}

BOOL STDCALL ConnectNamedPipe(HANDLE hNamedPipe,
		      LPOVERLAPPED lpOverLapped)
{
   NPFS_LISTEN ListenPipe;
   IO_STATUS_BLOCK Iosb;
   HANDLE hEvent;
   PIO_STATUS_BLOCK IoStatusBlock;
   NTSTATUS Status;
   
   if (lpOverLapped != NULL) 
     {
	lpOverLapped->Internal = STATUS_PENDING;
	hEvent = lpOverLapped->hEvent;
	IoStatusBlock = (PIO_STATUS_BLOCK)lpOverLapped;
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
			    FSCTL_LISTEN,
			    &ListenPipe,
			    sizeof(ListenPipe),
			    NULL,
			    0);
   if (!NT_SUCCESS(Status))
     {
	SetLastError(RtlNtStatusToDosError(Status));
	return(FALSE);
     }
   return(TRUE);
}

BOOL STDCALL SetNamedPipeHandleState(HANDLE hNamedPipe,
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
			    FSCTL_GET_STATE,
			    NULL,
			    0,
			    &GetState,
			    sizeof(GetState));
   if (!NT_SUCCESS(Status))
     {
	SetLastError(RtlNtStatusToDosError(Status));
	return(FALSE);
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
	SetState.Timeout.QuadPart = (*lpCollectDataTimeout) * 1000 * 1000;
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
			    FSCTL_SET_STATE,
			    &SetState,
			    sizeof(SetState),
			    NULL,
			    0);
   if (!NT_SUCCESS(Status))
     {
	SetLastError(RtlNtStatusToDosError(Status));
	return(FALSE);
     }
   return(TRUE);
}

/* EOF */
