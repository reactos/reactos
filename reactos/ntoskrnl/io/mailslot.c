/* $Id:$
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/mailslot.c
 * PURPOSE:         No purpose listed.
 * 
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
NtCreateMailslotFile(OUT PHANDLE FileHandle,
		     IN ACCESS_MASK DesiredAccess,
		     IN POBJECT_ATTRIBUTES ObjectAttributes,
		     OUT PIO_STATUS_BLOCK IoStatusBlock,
		     IN ULONG CreateOptions,
		     IN ULONG MailslotQuota,
		     IN ULONG MaxMessageSize,
		     IN PLARGE_INTEGER TimeOut)
{
   MAILSLOT_CREATE_PARAMETERS Buffer;
   
   DPRINT("NtCreateMailslotFile(FileHandle %x, DesiredAccess %x, "
	  "ObjectAttributes %x ObjectAttributes->ObjectName->Buffer %S)\n",
	  FileHandle,DesiredAccess,ObjectAttributes,
	  ObjectAttributes->ObjectName->Buffer);
   
   ASSERT_IRQL(PASSIVE_LEVEL);
   
   if (TimeOut != NULL)
     {
	Buffer.ReadTimeout.QuadPart = TimeOut->QuadPart;
	Buffer.TimeoutSpecified = TRUE;
     }
   else
     {
	Buffer.TimeoutSpecified = FALSE;
     }
   Buffer.MailslotQuota = MailslotQuota;
   Buffer.MaximumMessageSize = MaxMessageSize;

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
