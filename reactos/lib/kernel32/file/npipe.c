/* $Id: npipe.c,v 1.11 2002/07/10 15:09:57 ekohl Exp $
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
  PIO_STATUS_BLOCK IoStatusBlock;
  IO_STATUS_BLOCK Iosb;
  HANDLE hEvent;
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
  if ((!NT_SUCCESS(Status) && Status != STATUS_PIPE_CONNECTED) ||
      (Status == STATUS_PENDING))
    {
      SetLastErrorByStatus(Status);
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


BOOL STDCALL
CallNamedPipeA(LPCSTR lpNamedPipeName,
	       LPVOID lpInBuffer,
	       DWORD nInBufferSize,
	       LPVOID lpOutBuffer,
	       DWORD nOutBufferSize,
	       LPDWORD lpBytesRead,
	       DWORD nTimeOut)
{
  UNICODE_STRING PipeName;
  BOOL Result;
  
  RtlCreateUnicodeStringFromAsciiz(&PipeName,
				   (LPSTR)lpNamedPipeName);
  
  Result = CallNamedPipeW(PipeName.Buffer,
			  lpInBuffer,
			  nInBufferSize,
			  lpOutBuffer,
			  nOutBufferSize,
			  lpBytesRead,
			  nTimeOut);
  
  RtlFreeUnicodeString(&PipeName);
  
  return(Result);
}


BOOL STDCALL
CallNamedPipeW(LPCWSTR lpNamedPipeName,
	       LPVOID lpInBuffer,
	       DWORD nInBufferSize,
	       LPVOID lpOutBuffer,
	       DWORD nOutBufferSize,
	       LPDWORD lpBytesRead,
	       DWORD nTimeOut)
{
  HANDLE hPipe = INVALID_HANDLE_VALUE;
  BOOL bRetry = TRUE;
  BOOL bError = FALSE;
  DWORD dwPipeMode;

  while (TRUE)
    {
      hPipe = CreateFileW(lpNamedPipeName,
			  GENERIC_READ | GENERIC_WRITE,
			  FILE_SHARE_READ | FILE_SHARE_WRITE,
			  NULL,
			  OPEN_EXISTING,
			  FILE_ATTRIBUTE_NORMAL,
			  NULL);
      if (hPipe != INVALID_HANDLE_VALUE)
	break;

      if (bRetry == FALSE)
	return(FALSE);

      WaitNamedPipeW(lpNamedPipeName,
		     nTimeOut);

      bRetry = FALSE;
    }

  dwPipeMode = PIPE_READMODE_MESSAGE;
  bError = SetNamedPipeHandleState(hPipe,
				   &dwPipeMode,
				   NULL,
				   NULL);
  if (!bError)
    {
      CloseHandle(hPipe);
      return(FALSE);
    }

  bError = TransactNamedPipe(hPipe,
			     lpInBuffer,
			     nInBufferSize,
			     lpOutBuffer,
			     nOutBufferSize,
			     lpBytesRead,
			     NULL);
  CloseHandle(hPipe);

  return(bError);
}


BOOL STDCALL
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
  FILE_PIPE_LOCAL_INFORMATION LocalInfo;
  FILE_PIPE_INFORMATION PipeInfo;
  IO_STATUS_BLOCK StatusBlock;
  NTSTATUS Status;

  if (lpState != NULL)
    {
      Status = NtQueryInformationFile(hNamedPipe,
				      &StatusBlock,
				      &PipeInfo,
				      sizeof(FILE_PIPE_INFORMATION),
				      FilePipeInformation);
      if (!NT_SUCCESS(Status))
	{
	  SetLastErrorByStatus(Status);
	  return(FALSE);
	}
      *lpState = 0; /* FIXME */
    }

  if (lpCurInstances != NULL)
    {
      Status = NtQueryInformationFile(hNamedPipe,
				      &StatusBlock,
				      &LocalInfo,
				      sizeof(FILE_PIPE_LOCAL_INFORMATION),
				      FilePipeLocalInformation);
      if (!NT_SUCCESS(Status))
	{
	  SetLastErrorByStatus(Status);
	  return(FALSE);
	}
      *lpCurInstances = min(LocalInfo.CurrentInstances, 255);
    }


  /* FIXME: retrieve remaining information */


  return(TRUE);
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


BOOL STDCALL
PeekNamedPipe(HANDLE hNamedPipe,
	      LPVOID lpBuffer,
	      DWORD nBufferSize,
	      LPDWORD lpBytesRead,
	      LPDWORD lpTotalBytesAvail,
	      LPDWORD lpBytesLeftThisMessage)
{
  PFILE_PIPE_PEEK_BUFFER Buffer;
  IO_STATUS_BLOCK Iosb;
  ULONG BufferSize;
  NTSTATUS Status;

  BufferSize = nBufferSize + sizeof(FILE_PIPE_PEEK_BUFFER);
  Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
			   0,
			   BufferSize);

  Status = NtFsControlFile(hNamedPipe,
			   NULL,
			   NULL,
			   NULL,
			   &Iosb,
			   FSCTL_PIPE_PEEK,
			   NULL,
			   0,
			   Buffer,
			   BufferSize);
  if (Status == STATUS_PENDING)
    {
      Status = NtWaitForSingleObject(hNamedPipe,
				     FALSE,
				     NULL);
      if (NT_SUCCESS(Status))
	Status = Iosb.Status;
    }

  if (Status == STATUS_BUFFER_OVERFLOW)
    {
      Status = STATUS_SUCCESS;
    }

  if (!NT_SUCCESS(Status))
    {
      RtlFreeHeap(RtlGetProcessHeap(),
		  0,
		  Buffer);
      SetLastErrorByStatus(Status);
      return(FALSE);
    }

  if (lpTotalBytesAvail != NULL)
    {
      *lpTotalBytesAvail = Buffer->ReadDataAvailable;
    }

  if (lpBytesRead != NULL)
    {
      *lpBytesRead = Iosb.Information - sizeof(FILE_PIPE_PEEK_BUFFER);
    }

  if (lpBytesLeftThisMessage != NULL)
    {
      *lpBytesLeftThisMessage = Buffer->MessageLength -
	(Iosb.Information - sizeof(FILE_PIPE_PEEK_BUFFER));
    }

  if (lpBuffer != NULL)
    {
      memcpy(lpBuffer, Buffer->Data,
	     min(nBufferSize, Iosb.Information - sizeof(FILE_PIPE_PEEK_BUFFER)));
    }

  RtlFreeHeap(RtlGetProcessHeap(),
	      0,
	      Buffer);

  return(TRUE);
}


BOOL STDCALL
TransactNamedPipe(HANDLE hNamedPipe,
		  LPVOID lpInBuffer,
		  DWORD nInBufferSize,
		  LPVOID lpOutBuffer,
		  DWORD nOutBufferSize,
		  LPDWORD lpBytesRead,
		  LPOVERLAPPED lpOverlapped)
{
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS Status;

  if (lpOverlapped == NULL)
    {
      Status = NtFsControlFile(hNamedPipe,
			       NULL,
			       NULL,
			       NULL,
			       &IoStatusBlock,
			       FSCTL_PIPE_TRANSCEIVE,
			       lpInBuffer,
			       nInBufferSize,
			       lpOutBuffer,
			       nOutBufferSize);
      if (Status == STATUS_PENDING)
	{
	  NtWaitForSingleObject(hNamedPipe,
				0,
				FALSE);
	  Status = IoStatusBlock.Status;
	}
      if (NT_SUCCESS(Status))
	{
	  *lpBytesRead = IoStatusBlock.Information;
	}
    }
  else
    {
      lpOverlapped->Internal = STATUS_PENDING;

      Status = NtFsControlFile(hNamedPipe,
			       lpOverlapped->hEvent,
			       NULL,
			       NULL,
			       (PIO_STATUS_BLOCK)lpOverlapped,
			       FSCTL_PIPE_TRANSCEIVE,
			       lpInBuffer,
			       nInBufferSize,
			       lpOutBuffer,
			       nOutBufferSize);
    }

  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }

  return(TRUE);
}

/* EOF */
