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

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
NtCreateNamedPipeFile(PHANDLE FileHandleUnsafe,
		      ACCESS_MASK DesiredAccess,
		      POBJECT_ATTRIBUTES ObjectAttributesUnsafe,
		      PIO_STATUS_BLOCK IoStatusBlockUnsafe,
		      ULONG ShareAccess,
		      ULONG CreateDisposition,
		      ULONG CreateOptions,
		      ULONG NamedPipeType,
		      ULONG ReadMode,
		      ULONG CompletionMode,
		      ULONG MaximumInstances,
		      ULONG InboundQuota,
		      ULONG OutboundQuota,
		      PLARGE_INTEGER DefaultTimeoutUnsafe)
{
  NAMED_PIPE_CREATE_PARAMETERS Buffer;
  KPROCESSOR_MODE PreviousMode;
  NTSTATUS Status;
  HANDLE FileHandle;
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;

  DPRINT("NtCreateNamedPipeFile(FileHandle %x, DesiredAccess %x, "
	 "ObjectAttributes %x ObjectAttributes->ObjectName->Buffer %S)\n",
	 FileHandle,DesiredAccess,ObjectAttributes,
	 ObjectAttributes->ObjectName->Buffer);

  ASSERT_IRQL(PASSIVE_LEVEL);

  if (DefaultTimeoutUnsafe != NULL)
    {
      if (UserMode == PreviousMode)
        {
          Status = STATUS_SUCCESS;
          _SEH_TRY
            {
              ProbeForRead(DefaultTimeoutUnsafe,
                           sizeof(LARGE_INTEGER),
                           sizeof(LARGE_INTEGER));
              Buffer.DefaultTimeout.QuadPart = DefaultTimeoutUnsafe->QuadPart;
            }
          _SEH_HANDLE
            {
              Status = _SEH_GetExceptionCode();
            }
          _SEH_END;
        }
      else
        {
          Buffer.DefaultTimeout.QuadPart = DefaultTimeoutUnsafe->QuadPart;
        }
      Buffer.TimeoutSpecified = TRUE;
    }
  else
    {
      Buffer.TimeoutSpecified = FALSE;
    }
  Buffer.NamedPipeType = NamedPipeType;
  Buffer.ReadMode = ReadMode;
  Buffer.CompletionMode = CompletionMode;
  Buffer.MaximumInstances = MaximumInstances;
  Buffer.InboundQuota = InboundQuota;
  Buffer.OutboundQuota = OutboundQuota;

  PreviousMode = ExGetPreviousMode();
  if (KernelMode == PreviousMode)
    {
      return IoCreateFile(FileHandleUnsafe,
                          DesiredAccess,
                          ObjectAttributesUnsafe,
                          IoStatusBlockUnsafe,
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
                        ShareAccess,
                        CreateDisposition,
                        CreateOptions,
                        NULL,
                        0,
                        CreateFileTypeNamedPipe,
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
