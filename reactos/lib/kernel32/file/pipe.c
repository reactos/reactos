/* $Id$
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
#include "../include/debug.h"

/* GLOBALS ******************************************************************/

ULONG ProcessPipeId = 0;

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
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
   ULONG PipeId;
   ULONG Attributes;
   PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;

   DefaultTimeout.QuadPart = 300000000; /* 30 seconds */

   PipeId = (ULONG)InterlockedIncrement((LONG*)&ProcessPipeId);
   swprintf(Buffer,
	    L"\\Device\\NamedPipe\\Win32Pipes.%08x.%08x",
	    NtCurrentTeb()->Cid.UniqueProcess,
	    PipeId);
   RtlInitUnicodeString (&PipeName,
			 Buffer);

   Attributes = OBJ_CASE_INSENSITIVE;
   if (lpPipeAttributes)
     {
	SecurityDescriptor = lpPipeAttributes->lpSecurityDescriptor;
	if (lpPipeAttributes->bInheritHandle)
	  Attributes |= OBJ_INHERIT;
     }

   InitializeObjectAttributes(&ObjectAttributes,
			      &PipeName,
			      Attributes,
			      NULL,
			      SecurityDescriptor);

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
		       0,
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
