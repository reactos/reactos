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

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
NtCreateMailslotFile(OUT PHANDLE FileHandleUnsafe,
		     IN ACCESS_MASK DesiredAccess,
		     IN POBJECT_ATTRIBUTES ObjectAttributesUnsafe,
		     OUT PIO_STATUS_BLOCK IoStatusBlockUnsafe,
		     IN ULONG CreateOptions,
		     IN ULONG MailslotQuota,
		     IN ULONG MaxMessageSize,
		     IN PLARGE_INTEGER TimeOutUnsafe)
{
   MAILSLOT_CREATE_PARAMETERS Buffer;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status;
   HANDLE FileHandle;
   OBJECT_ATTRIBUTES ObjectAttributes;
   IO_STATUS_BLOCK IoStatusBlock;
   
   DPRINT("NtCreateMailslotFile(FileHandle %x, DesiredAccess %x, "
	  "ObjectAttributes %x ObjectAttributes->ObjectName->Buffer %S)\n",
	  FileHandle,DesiredAccess,ObjectAttributes,
	  ObjectAttributes->ObjectName->Buffer);
   
   ASSERT_IRQL(PASSIVE_LEVEL);
   
   if (TimeOutUnsafe != NULL)
     {
        if (UserMode == PreviousMode)
          {
             Status = STATUS_SUCCESS;
             _SEH_TRY
               {
                  ProbeForRead(TimeOutUnsafe,
                               sizeof(LARGE_INTEGER),
                               sizeof(LARGE_INTEGER));
                  Buffer.ReadTimeout.QuadPart = TimeOutUnsafe->QuadPart;
               }
             _SEH_HANDLE
               {
                  Status = _SEH_GetExceptionCode();
               }
             _SEH_END;
          }
        else
          {
             Buffer.ReadTimeout.QuadPart = TimeOutUnsafe->QuadPart;
          }
        Buffer.TimeoutSpecified = TRUE;
     }
   else
     {
        Buffer.TimeoutSpecified = FALSE;
     }
   Buffer.MailslotQuota = MailslotQuota;
   Buffer.MaximumMessageSize = MaxMessageSize;

   PreviousMode = ExGetPreviousMode();
   if (KernelMode == PreviousMode)
     {
        return IoCreateFile(FileHandleUnsafe,
                            DesiredAccess,
                            ObjectAttributesUnsafe,
                            IoStatusBlockUnsafe,
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

   Status = RtlCaptureObjectAttributes(&ObjectAttributes,
                                       PreviousMode,
                                       PagedPool,
                                       FALSE,
                                       ObjectAttributesUnsafe);
   if (! NT_SUCCESS(Status))
     {
        return Status;
     }

   Status = IoCreateFile(&FileHandle,
                         DesiredAccess,
                         &ObjectAttributes,
                         &IoStatusBlock,
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
   if (! NT_SUCCESS(Status))
     {
        return Status;
     }

   _SEH_TRY
     {
        ProbeForWrite(FileHandleUnsafe,
                      sizeof(HANDLE),
                      sizeof(ULONG));
        *FileHandleUnsafe = FileHandle;
        ProbeForWrite(IoStatusBlockUnsafe,
                      sizeof(IO_STATUS_BLOCK),
                      sizeof(ULONG));
        *IoStatusBlockUnsafe = IoStatusBlock;
     }
   _SEH_HANDLE
     {
        Status = _SEH_GetExceptionCode();
     }
   _SEH_END;

   return Status;
}

/* EOF */
