/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/npipe.c
 * PURPOSE:         Named pipe helper function
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
NTSTATUS STDCALL
NtCreateNamedPipeFile(PHANDLE FileHandle,
		      ACCESS_MASK DesiredAccess,
		      POBJECT_ATTRIBUTES ObjectAttributes,
		      PIO_STATUS_BLOCK IoStatusBlock,
		      ULONG ShareAccess,
		      ULONG CreateDisposition,
		      ULONG CreateOptions,
		      BOOLEAN WriteModeMessage,
		      BOOLEAN ReadModeMessage,
		      BOOLEAN NonBlocking,
		      ULONG MaxInstances,
		      ULONG InBufferSize,
		      ULONG OutBufferSize,
		      PLARGE_INTEGER TimeOut)
{
   IO_PIPE_CREATE_BUFFER Buffer;
   
   DPRINT("NtCreateNamedPipeFile(FileHandle %x, DesiredAccess %x, "
	  "ObjectAttributes %x ObjectAttributes->ObjectName->Buffer %S)\n",
	  FileHandle,DesiredAccess,ObjectAttributes,
	  ObjectAttributes->ObjectName->Buffer);
   
   assert_irql(PASSIVE_LEVEL);
   
   if (TimeOut != NULL)
     {
	Buffer.TimeOut.QuadPart = TimeOut->QuadPart;
     }
   else
     {
	Buffer.TimeOut.QuadPart = 0;
     }
   Buffer.WriteModeMessage = WriteModeMessage;
   Buffer.ReadModeMessage = ReadModeMessage;
   Buffer.NonBlocking = NonBlocking;
   Buffer.MaxInstances = MaxInstances;
   Buffer.InBufferSize = InBufferSize;
   Buffer.OutBufferSize = OutBufferSize;
   
   return IoCreateFile(FileHandle,
		       DesiredAccess,
		       ObjectAttributes,
		       IoStatusBlock,
		       NULL,
		       FILE_ATTRIBUTE_NORMAL,
		       ShareAccess,
		       CreateDisposition,
		       CreateOptions,
		       NULL,
		       0,
		       CreateFileTypeNamedPipe,
		       (PVOID)&Buffer,
		       0);
}

/* EOF */
