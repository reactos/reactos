/* $Id: pipe.c,v 1.8 2003/01/15 21:24:34 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/create.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include <k32.h>

#define NDEBUG
#include <kernel32/kernel32.h>

/* GLOBALS ******************************************************************/

ULONG ProcessPipeId = 0;

/* FUNCTIONS ****************************************************************/

BOOL STDCALL CreatePipe(PHANDLE hReadPipe,
			PHANDLE hWritePipe,
			LPSECURITY_ATTRIBUTES lpPipeAttributes,
			DWORD nSize)
{
   WCHAR Buffer[64];
   UNICODE_STRING PipeName;
   OBJECT_ATTRIBUTES ObjectAttributes;
   IO_STATUS_BLOCK StatusBlock;
   LARGE_INTEGER DefaultTimeout;
   NTSTATUS Status;
   HANDLE ReadPipeHandle;
   HANDLE WritePipeHandle;
   PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;

   DefaultTimeout.QuadPart = 300000000; /* 30 seconds */

   ProcessPipeId++;
   swprintf(Buffer,
	    L"\\Device\\NamedPipe\\Win32Pipes.%08x.%08x",
	    NtCurrentTeb()->Cid.UniqueProcess,
	    ProcessPipeId);
   RtlInitUnicodeString (&PipeName,
			 Buffer);

   if (lpPipeAttributes)
     {
	SecurityDescriptor = lpPipeAttributes->lpSecurityDescriptor;
     }

   InitializeObjectAttributes(&ObjectAttributes,
			      &PipeName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      SecurityDescriptor);
   if (lpPipeAttributes)
   {
      if(lpPipeAttributes->bInheritHandle)
         ObjectAttributes.Attributes |= OBJ_INHERIT;
      if (lpPipeAttributes->lpSecurityDescriptor)
	 ObjectAttributes.SecurityDescriptor = lpPipeAttributes->lpSecurityDescriptor;
   }

   Status = NtCreateNamedPipeFile(&ReadPipeHandle,
				  FILE_GENERIC_READ,
				  &ObjectAttributes,
				  &StatusBlock,
				  FILE_SHARE_READ | FILE_SHARE_WRITE,
				  FILE_CREATE,
				  FILE_SYNCHRONOUS_IO_NONALERT,
				  FALSE,
				  FALSE,
				  FALSE,
				  1,
				  nSize,
				  nSize,
				  &DefaultTimeout);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return FALSE;
     }

   Status = NtOpenFile(&WritePipeHandle,
		       FILE_GENERIC_WRITE,
		       &ObjectAttributes,
		       &StatusBlock,
		       FILE_SHARE_READ | FILE_SHARE_WRITE,
		       FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
   if (!NT_SUCCESS(Status))
     {
	NtClose(ReadPipeHandle);
	SetLastErrorByStatus(Status);
	return FALSE;
     }

   *hReadPipe = ReadPipeHandle;
   *hWritePipe = WritePipeHandle;

   return TRUE;
}

/* EOF */
