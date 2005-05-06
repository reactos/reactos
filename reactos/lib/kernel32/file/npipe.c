/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/npipe.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
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


/*
 * @implemented
 */
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
   ULONG WriteModeMessage;
   ULONG ReadModeMessage;
   ULONG NonBlocking;
   IO_STATUS_BLOCK Iosb;
   ULONG ShareAccess, Attributes;
   LARGE_INTEGER DefaultTimeOut;
   PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;

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

   Attributes = OBJ_CASE_INSENSITIVE;
   if(lpSecurityAttributes)
     {
       SecurityDescriptor = lpSecurityAttributes->lpSecurityDescriptor;
       if(lpSecurityAttributes->bInheritHandle)
          Attributes |= OBJ_INHERIT;
     }

   InitializeObjectAttributes(&ObjectAttributes,
			      &NamedPipeName,
			      Attributes,
			      NULL,
			      SecurityDescriptor);

   DesiredAccess = 0;
   ShareAccess = 0;
   CreateDisposition = FILE_OPEN_IF;
   CreateOptions = 0;
   if (dwOpenMode & FILE_FLAG_WRITE_THROUGH)
     {
	CreateOptions = CreateOptions | FILE_WRITE_THROUGH;
     }
   if (!(dwOpenMode & FILE_FLAG_OVERLAPPED))
     {
	CreateOptions = CreateOptions | FILE_SYNCHRONOUS_IO_NONALERT;
     }
   if (dwOpenMode & PIPE_ACCESS_DUPLEX)
     {
	CreateOptions = CreateOptions | FILE_PIPE_FULL_DUPLEX;
	DesiredAccess |= (FILE_GENERIC_READ | FILE_GENERIC_WRITE);
     }
   else if (dwOpenMode & PIPE_ACCESS_INBOUND)
     {
	CreateOptions = CreateOptions | FILE_PIPE_INBOUND;
	DesiredAccess |= FILE_GENERIC_READ;
     }
   else if (dwOpenMode & PIPE_ACCESS_OUTBOUND)
     {
	CreateOptions = CreateOptions | FILE_PIPE_OUTBOUND;
	DesiredAccess |= FILE_GENERIC_WRITE;
     }

   if (dwPipeMode & PIPE_TYPE_BYTE)
     {
	WriteModeMessage = FILE_PIPE_BYTE_STREAM_MODE;
     }
   else if (dwPipeMode & PIPE_TYPE_MESSAGE)
     {
	WriteModeMessage = FILE_PIPE_MESSAGE_MODE;
     }
   else
     {
	WriteModeMessage = FILE_PIPE_BYTE_STREAM_MODE;
     }

   if (dwPipeMode & PIPE_READMODE_BYTE)
     {
	ReadModeMessage = FILE_PIPE_BYTE_STREAM_MODE;
     }
   else if (dwPipeMode & PIPE_READMODE_MESSAGE)
     {
	ReadModeMessage = FILE_PIPE_MESSAGE_MODE;
     }
   else
     {
	ReadModeMessage = FILE_PIPE_BYTE_STREAM_MODE;
     }

   if (dwPipeMode & PIPE_WAIT)
     {
	NonBlocking = FILE_PIPE_QUEUE_OPERATION;
     }
   else if (dwPipeMode & PIPE_NOWAIT)
     {
	NonBlocking = FILE_PIPE_COMPLETE_OPERATION;
     }
   else
     {
	NonBlocking = FILE_PIPE_QUEUE_OPERATION;
     }

   if (nMaxInstances >= PIPE_UNLIMITED_INSTANCES)
     {
	nMaxInstances = ULONG_MAX;
     }

   DefaultTimeOut.QuadPart = nDefaultTimeOut * -10000LL;

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
	return INVALID_HANDLE_VALUE;
     }

   return PipeHandle;
}


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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
		       FILE_SHARE_READ | FILE_SHARE_WRITE,
		       FILE_SYNCHRONOUS_IO_NONALERT);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus (Status);
	return(FALSE);
     }
   
   WaitPipe.Timeout.QuadPart = nTimeOut * -10000LL;
   
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


/*
 * @implemented
 */
BOOL STDCALL
ConnectNamedPipe(IN HANDLE hNamedPipe,
                 IN LPOVERLAPPED lpOverlapped)
{
   NTSTATUS Status;
   
   if (lpOverlapped != NULL)
     {
        PVOID ApcContext;
        
        lpOverlapped->Internal = STATUS_PENDING;
        ApcContext = (((ULONG_PTR)lpOverlapped->hEvent & 0x1) ? NULL : lpOverlapped);
        
        Status = NtFsControlFile(hNamedPipe,
                                 lpOverlapped->hEvent,
                                 NULL,
                                 ApcContext,
                                 (PIO_STATUS_BLOCK)lpOverlapped,
                                 FSCTL_PIPE_LISTEN,
                                 NULL,
                                 0,
                                 NULL,
                                 0);

        /* return FALSE in case of failure and pending operations! */
        if (!NT_SUCCESS(Status) || Status == STATUS_PENDING)
          {
             SetLastErrorByStatus(Status);
             return FALSE;
          }
     }
   else
     {
        IO_STATUS_BLOCK Iosb;
        
        Status = NtFsControlFile(hNamedPipe,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &Iosb,
                                 FSCTL_PIPE_LISTEN,
                                 NULL,
                                 0,
                                 NULL,
                                 0);

        /* wait in case operation is pending */
        if (Status == STATUS_PENDING)
          {
             Status = NtWaitForSingleObject(hNamedPipe,
                                            FALSE,
                                            NULL);
             if (NT_SUCCESS(Status))
               {
                  Status = Iosb.Status;
               }
          }

        if (!NT_SUCCESS(Status))
          {
             SetLastErrorByStatus(Status);
             return FALSE;
          }
     }

   return TRUE;
}


/*
 * @implemented
 */
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
	SetState.Timeout.QuadPart = (*lpCollectDataTimeout) * -10000LL;
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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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


/*
 * @unimplemented
 */
BOOL STDCALL
GetNamedPipeHandleStateW(HANDLE hNamedPipe,
			 LPDWORD lpState,
			 LPDWORD lpCurInstances,
			 LPDWORD lpMaxCollectionCount,
			 LPDWORD lpCollectDataTimeout,
			 LPWSTR lpUserName,
			 DWORD nMaxUserNameSize)
{
  IO_STATUS_BLOCK StatusBlock;
  NTSTATUS Status;

  if (lpState != NULL)
  {
    FILE_PIPE_INFORMATION PipeInfo;
    
    Status = NtQueryInformationFile(hNamedPipe,
                                    &StatusBlock,
                                    &PipeInfo,
                                    sizeof(FILE_PIPE_INFORMATION),
                                    FilePipeInformation);
    if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return FALSE;
    }

    *lpState = ((PipeInfo.CompletionMode != FILE_PIPE_QUEUE_OPERATION) ? PIPE_NOWAIT : PIPE_WAIT);
    *lpState |= ((PipeInfo.ReadMode != FILE_PIPE_BYTE_STREAM_MODE) ? PIPE_READMODE_MESSAGE : PIPE_READMODE_BYTE);
  }

  if(lpCurInstances != NULL)
  {
    FILE_PIPE_LOCAL_INFORMATION LocalInfo;
    
    Status = NtQueryInformationFile(hNamedPipe,
                                    &StatusBlock,
                                    &LocalInfo,
                                    sizeof(FILE_PIPE_LOCAL_INFORMATION),
                                    FilePipeLocalInformation);
    if(!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return FALSE;
    }

    *lpCurInstances = min(LocalInfo.CurrentInstances, PIPE_UNLIMITED_INSTANCES);
  }

  if(lpMaxCollectionCount != NULL || lpCollectDataTimeout != NULL)
  {
    FILE_PIPE_REMOTE_INFORMATION RemoteInfo;
    
    Status = NtQueryInformationFile(hNamedPipe,
                                    &StatusBlock,
                                    &RemoteInfo,
                                    sizeof(FILE_PIPE_REMOTE_INFORMATION),
                                    FilePipeRemoteInformation);
    if(!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return FALSE;
    }

    if(lpMaxCollectionCount != NULL)
    {
      *lpMaxCollectionCount = RemoteInfo.MaximumCollectionCount;
    }
    
    if(lpCollectDataTimeout != NULL)
    {
      /* FIXME */
      *lpCollectDataTimeout = 0;
    }
  }
  
  if(lpUserName != NULL)
  {
    /* FIXME - open the thread token, call ImpersonateNamedPipeClient() and
               retreive the user name with GetUserName(), revert the impersonation
               and finally restore the thread token */
  }

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
GetNamedPipeHandleStateA(HANDLE hNamedPipe,
			 LPDWORD lpState,
			 LPDWORD lpCurInstances,
			 LPDWORD lpMaxCollectionCount,
			 LPDWORD lpCollectDataTimeout,
			 LPSTR lpUserName,
			 DWORD nMaxUserNameSize)
{
  UNICODE_STRING UserNameW;
  ANSI_STRING UserNameA;
  BOOL Ret;
  
  if(lpUserName != NULL)
  {
    UserNameW.Length = 0;
    UserNameW.MaximumLength = nMaxUserNameSize * sizeof(WCHAR);
    UserNameW.Buffer = HeapAlloc(GetCurrentProcess(), 0, UserNameW.MaximumLength);
    
    UserNameA.Buffer = lpUserName;
    UserNameA.Length = 0;
    UserNameA.MaximumLength = nMaxUserNameSize;
  }
  
  Ret = GetNamedPipeHandleStateW(hNamedPipe,
                                 lpState,
                                 lpCurInstances,
                                 lpMaxCollectionCount,
                                 lpCollectDataTimeout,
                                 UserNameW.Buffer,
                                 nMaxUserNameSize);

  if(Ret && lpUserName != NULL)
  {
    NTSTATUS Status = RtlUnicodeStringToAnsiString(&UserNameA, &UserNameW, FALSE);
    if(!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      Ret = FALSE;
    }
  }
  
  if(UserNameW.Buffer != NULL)
  {
    HeapFree(GetCurrentProcess(), 0, UserNameW.Buffer);
  }
  
  return Ret;
}


/*
 * @implemented
 */
BOOL STDCALL
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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
BOOL STDCALL
TransactNamedPipe(IN HANDLE hNamedPipe,
                  IN LPVOID lpInBuffer,
                  IN DWORD nInBufferSize,
                  OUT LPVOID lpOutBuffer,
                  IN DWORD nOutBufferSize,
                  OUT LPDWORD lpBytesRead  OPTIONAL,
                  IN LPOVERLAPPED lpOverlapped  OPTIONAL)
{
   NTSTATUS Status;
   
   if (lpBytesRead != NULL)
     {
        *lpBytesRead = 0;
     }

   if (lpOverlapped != NULL)
     {
        PVOID ApcContext;

        ApcContext = (((ULONG_PTR)lpOverlapped->hEvent & 0x1) ? NULL : lpOverlapped);
        lpOverlapped->Internal = STATUS_PENDING;

        Status = NtFsControlFile(hNamedPipe,
                                 lpOverlapped->hEvent,
                                 NULL,
                                 ApcContext,
                                 (PIO_STATUS_BLOCK)lpOverlapped,
                                 FSCTL_PIPE_TRANSCEIVE,
                                 lpInBuffer,
                                 nInBufferSize,
                                 lpOutBuffer,
                                 nOutBufferSize);

        /* return FALSE in case of failure and pending operations! */
        if (!NT_SUCCESS(Status) || Status == STATUS_PENDING)
          {
             SetLastErrorByStatus(Status);
             return FALSE;
          }

        if (lpBytesRead != NULL)
          {
             *lpBytesRead = lpOverlapped->InternalHigh;
          }
     }
   else
     {
        IO_STATUS_BLOCK Iosb;
        
        Status = NtFsControlFile(hNamedPipe,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &Iosb,
                                 FSCTL_PIPE_TRANSCEIVE,
                                 lpInBuffer,
                                 nInBufferSize,
                                 lpOutBuffer,
                                 nOutBufferSize);

        /* wait in case operation is pending */
        if (Status == STATUS_PENDING)
          {
             Status = NtWaitForSingleObject(hNamedPipe,
                                            FALSE,
                                            NULL);
             if (NT_SUCCESS(Status))
               {
                  Status = Iosb.Status;
               }
          }

        if (NT_SUCCESS(Status))
          {
             /* lpNumberOfBytesRead must not be NULL here, in fact Win doesn't
                check that case either and crashes (only after the operation
                completed) */
             *lpBytesRead = Iosb.Information;
          }
        else
          {
             SetLastErrorByStatus(Status);
             return FALSE;
          }
     }

   return TRUE;
}

/* EOF */
