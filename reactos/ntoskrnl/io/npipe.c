/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/bug.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL NtCreateNamedPipeFile(OUT PHANDLE NamedPipeFileHandle,
				       IN ACCESS_MASK DesiredAccess,               
				       IN POBJECT_ATTRIBUTES ObjectAttributes,     
				       OUT PIO_STATUS_BLOCK IoStatusBlock,    
				       IN ULONG FileAttributes,
				       IN ULONG ShareAccess,
				       IN ULONG OpenMode,  
				       IN ULONG PipeType, 
				       IN ULONG PipeRead, 
				       IN ULONG PipeWait, 
				       IN ULONG MaxInstances,
				       IN ULONG InBufferSize,
				       IN ULONG OutBufferSize,
				       IN PLARGE_INTEGER TimeOut)
{
   return(ZwCreateNamedPipeFile(NamedPipeFileHandle,
				DesiredAccess,
				ObjectAttributes,
				IoStatusBlock,
				FileAttributes,
				ShareAccess,
				OpenMode,
				PipeType,
				PipeRead,
				PipeWait,
				MaxInstances,
				InBufferSize,
				OutBufferSize,
				TimeOut));
}

NTSTATUS STDCALL ZwCreateNamedPipeFile(OUT PHANDLE NamedPipeFileHandle,
				       IN ACCESS_MASK DesiredAccess,               
				       IN POBJECT_ATTRIBUTES ObjectAttributes,     
				       OUT PIO_STATUS_BLOCK IoStatusBlock,    
				       IN ULONG FileAttributes,
				       IN ULONG ShareAccess,
				       IN ULONG OpenMode,  
				       IN ULONG PipeType, 
				       IN ULONG PipeRead, 
				       IN ULONG PipeWait, 
				       IN ULONG MaxInstances,
				       IN ULONG InBufferSize,
				       IN ULONG OutBufferSize,
				       IN PLARGE_INTEGER TimeOut)
{
   UNIMPLEMENTED;
}
