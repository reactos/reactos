/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/bug.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
		    Changed NtCreateMailslotFile
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
NtCreateMailslotFile(OUT PHANDLE FileHandle,
		     IN ACCESS_MASK DesiredAccess,
		     IN POBJECT_ATTRIBUTES ObjectAttributes,
		     OUT PIO_STATUS_BLOCK IoStatusBlock,
		     IN ULONG CreateOptions,
		     IN ULONG Param,			/* FIXME: ??? */
		     IN ULONG MaxMessageSize,
		     IN PLARGE_INTEGER TimeOut)
{
   IO_MAILSLOT_CREATE_BUFFER Buffer;
   
   DPRINT("NtCreateMailslotFile(FileHandle %x, DesiredAccess %x, "
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
   Buffer.Param = Param;			/* FIXME: ??? */
   Buffer.MaxMessageSize = MaxMessageSize;

   return IoCreateFile(FileHandle,
		       DesiredAccess,
		       ObjectAttributes,
		       IoStatusBlock,
		       NULL,
		       FILE_ATTRIBUTE_NORMAL,
		       FILE_SHARE_READ | FILE_SHARE_WRITE,
		       FILE_CREATE,
		       CreateOptions,
		       NULL,
		       0,
		       CreateFileTypeMailslot,
		       (PVOID)&Buffer,
		       0);
}

/* EOF */
