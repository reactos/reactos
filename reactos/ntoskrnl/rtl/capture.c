/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/capture.c
 * PURPOSE:         Helper routines for system calls.
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                02/09/01: Created
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#define TAG_CAPT  TAG('C', 'A', 'P', 'T')

/* FUNCTIONS *****************************************************************/

NTSTATUS
RtlCaptureUnicodeString(OUT PUNICODE_STRING Dest,
	                IN KPROCESSOR_MODE CurrentMode,
	                IN POOL_TYPE PoolType,
	                IN BOOLEAN CaptureIfKernel,
			IN PUNICODE_STRING UnsafeSrc)
{
  UNICODE_STRING Src;
  NTSTATUS Status = STATUS_SUCCESS;
  
  ASSERT(Dest != NULL);
  
  /*
   * Copy the source string structure to kernel space.
   */
  
  if(CurrentMode == UserMode)
  {
    _SEH_TRY
    {
      ProbeForRead(UnsafeSrc,
                   sizeof(UNICODE_STRING),
                   sizeof(ULONG));
      Src = *UnsafeSrc;
      if(Src.Length > 0)
      {
        ProbeForRead(Src.Buffer,
                     Src.Length,
                     sizeof(WCHAR));
      }
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    
    if(!NT_SUCCESS(Status))
    {
      return Status;
    }
  }
  else if(!CaptureIfKernel)
  {
    /* just copy the UNICODE_STRING structure, the pointers are considered valid */
    *Dest = *UnsafeSrc;
    return STATUS_SUCCESS;
  }
  else
  {
    /* capture the string even though it is considered to be valid */
    Src = *UnsafeSrc;
  }
  
  /*
   * Initialize the destination string.
   */
  Dest->Length = Src.Length;
  if(Src.Length > 0)
  {
    Dest->MaximumLength = Src.Length + sizeof(WCHAR);
    Dest->Buffer = ExAllocatePoolWithTag(PoolType, Dest->MaximumLength, TAG_CAPT);
    if (Dest->Buffer == NULL)
    {
      Dest->Length = Dest->MaximumLength = 0;
      Dest->Buffer = NULL;
      return STATUS_INSUFFICIENT_RESOURCES;
    }
    /*
     * Copy the source string to kernel space.
     */
    _SEH_TRY
    {
      RtlCopyMemory(Dest->Buffer, Src.Buffer, Src.Length);
      Dest->Buffer[Src.Length / sizeof(WCHAR)] = L'\0';
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    
    if(!NT_SUCCESS(Status))
    {
      ExFreePool(Dest->Buffer);
      Dest->Buffer = NULL;
      Dest->Length = Dest->MaximumLength = 0;
    }
  }
  else
  {
    Dest->MaximumLength = 0;
    Dest->Buffer = NULL;
  }
  
  return Status;
}

VOID
RtlReleaseCapturedUnicodeString(IN PUNICODE_STRING CapturedString,
	                        IN KPROCESSOR_MODE CurrentMode,
	                        IN BOOLEAN CaptureIfKernel)
{
  if(CurrentMode != KernelMode || CaptureIfKernel )
  {
    RtlFreeUnicodeString(CapturedString);
  }
}

NTSTATUS
RtlCaptureAnsiString(PANSI_STRING Dest,
		     PANSI_STRING UnsafeSrc)
{
  PANSI_STRING Src; 
  NTSTATUS Status;
  
  /*
   * Copy the source string structure to kernel space.
   */
  Status = MmCopyFromCaller(&Src, UnsafeSrc, sizeof(ANSI_STRING));
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  /*
   * Initialize the destination string.
   */
  Dest->Length = Src->Length;
  Dest->MaximumLength = Src->MaximumLength;
  Dest->Buffer = ExAllocatePoolWithTag(NonPagedPool, Dest->MaximumLength, TAG_CAPT);
  if (Dest->Buffer == NULL)
    {
      return(Status);
    }

  /*
   * Copy the source string to kernel space.
   */
  Status = MmCopyFromCaller(Dest->Buffer, Src->Buffer, Dest->Length);
  if (!NT_SUCCESS(Status))
    {
      ExFreePool(Dest->Buffer);
      return(Status);
    }

  return(STATUS_SUCCESS);
}

static NTSTATUS
CaptureSID(OUT PSID *Dest,
           IN KPROCESSOR_MODE PreviousMode,
           IN POOL_TYPE PoolType,
           IN PSID UnsafeSrc)
{
  SID Src;
  ULONG Length;
  NTSTATUS Status = STATUS_SUCCESS;
  
  ASSERT(Dest != NULL);

  if(UserMode == PreviousMode)
  {  
    _SEH_TRY
    {
      ProbeForRead(UnsafeSrc,
                   sizeof(SID),
                   sizeof(ULONG));
      RtlCopyMemory(&Src, UnsafeSrc, sizeof(SID));
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    
    if(!NT_SUCCESS(Status))
    {
      return Status;
    }
  }
  else
  {
    /* capture even though it is considered to be valid */
    RtlCopyMemory(&Src, UnsafeSrc, sizeof(SID));
  }

  if(SID_REVISION != Src.Revision)
  {
    return STATUS_INVALID_PARAMETER;
  }

  Length = RtlLengthSid(&Src);
  *Dest = ExAllocatePoolWithTag(PoolType, Length, TAG_CAPT);
  if(NULL == *Dest)
  {
    return STATUS_NO_MEMORY;
  }

  if(UserMode == PreviousMode)
  {  
    _SEH_TRY
    {
      ProbeForRead(UnsafeSrc,
                   Length,
                   sizeof(ULONG));
      RtlCopyMemory(*Dest, UnsafeSrc, Length);
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    
    if(!NT_SUCCESS(Status))
    {
      return Status;
    }
  }
  else
  {
    RtlCopyMemory(*Dest, UnsafeSrc, Length);
  }

  return Status;
}

static NTSTATUS
CaptureACL(OUT PACL *Dest,
           IN KPROCESSOR_MODE PreviousMode,
           IN POOL_TYPE PoolType,
           IN PACL UnsafeSrc)
{
  ACL Src;
  ULONG Length;
  NTSTATUS Status = STATUS_SUCCESS;
  
  ASSERT(Dest != NULL);

  if(UserMode == PreviousMode)
  {  
    _SEH_TRY
    {
      ProbeForRead(UnsafeSrc,
                   sizeof(ACL),
                   sizeof(ULONG));
      RtlCopyMemory(&Src, UnsafeSrc, sizeof(ACL));
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    
    if(!NT_SUCCESS(Status))
    {
      return Status;
    }
  }
  else
  {
    /* capture even though it is considered to be valid */
    RtlCopyMemory(&Src, UnsafeSrc, sizeof(ACL));
  }

  if(Src.AclRevision < MIN_ACL_REVISION || MAX_ACL_REVISION < Src.AclRevision)
  {
    return STATUS_INVALID_PARAMETER;
  }

  Length = Src.AclSize;
  *Dest = ExAllocatePoolWithTag(PoolType, Length, TAG_CAPT);
  if(NULL == *Dest)
  {
    return STATUS_NO_MEMORY;
  }

  if(UserMode == PreviousMode)
  {  
    _SEH_TRY
    {
      ProbeForRead(UnsafeSrc,
                   Length,
                   sizeof(ULONG));
      RtlCopyMemory(*Dest, UnsafeSrc, Length);
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    
    if(!NT_SUCCESS(Status))
    {
      return Status;
    }
  }
  else
  {
    RtlCopyMemory(*Dest, UnsafeSrc, Length);
  }

  return Status;
}

NTSTATUS
RtlCaptureSecurityDescriptor(OUT PSECURITY_DESCRIPTOR Dest,
                             IN KPROCESSOR_MODE PreviousMode,
                             IN POOL_TYPE PoolType,
                             IN BOOLEAN CaptureIfKernel,
                             IN PSECURITY_DESCRIPTOR UnsafeSrc)
{
  SECURITY_DESCRIPTOR Src;
  NTSTATUS Status = STATUS_SUCCESS;
  
  ASSERT(Dest != NULL);
  
  /*
   * Copy the object attributes to kernel space.
   */
  
  if(PreviousMode == UserMode)
  {
    _SEH_TRY
    {
      ProbeForRead(UnsafeSrc,
                   sizeof(SECURITY_DESCRIPTOR),
                   sizeof(ULONG));
      Src = *UnsafeSrc;
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    
    if(!NT_SUCCESS(Status))
    {
      return Status;
    }
  }
  else if(!CaptureIfKernel)
  {
    /* just copy the structure, the pointers are considered valid */
    *Dest = *UnsafeSrc;
    return STATUS_SUCCESS;
  }
  else
  {
    /* capture the object attributes even though it is considered to be valid */
    Src = *UnsafeSrc;
  }

  if(SECURITY_DESCRIPTOR_REVISION1 != Src.Revision)
  {
    return STATUS_INVALID_PARAMETER;
  }

  Dest->Revision = Src.Revision;  
  Dest->Sbz1 = Src.Sbz1;
  Dest->Control = Src.Control;
  Status = CaptureSID(&Dest->Owner, PreviousMode, PoolType, Src.Owner);
  if(!NT_SUCCESS(Status))
  {
    return Status;
  }
  Status = CaptureSID(&Dest->Group, PreviousMode, PoolType, Src.Group);
  if(!NT_SUCCESS(Status))
  {
    if(NULL != Dest->Owner)
    {
      ExFreePool(Dest->Owner);
    }
    return Status;
  }
  Status = CaptureACL(&Dest->Sacl, PreviousMode, PoolType, Src.Sacl);
  if(!NT_SUCCESS(Status))
  {
    if(NULL != Dest->Group)
    {
      ExFreePool(Dest->Group);
    }
    if(NULL != Dest->Owner)
    {
      ExFreePool(Dest->Owner);
    }
    return Status;
  }
  Status = CaptureACL(&Dest->Dacl, PreviousMode, PoolType, Src.Dacl);
  if(!NT_SUCCESS(Status))
  {
    if(NULL != Dest->Sacl)
    {
      ExFreePool(Dest->Sacl);
    }
    if(NULL != Dest->Group)
    {
      ExFreePool(Dest->Group);
    }
    if(NULL != Dest->Owner)
    {
      ExFreePool(Dest->Owner);
    }
    return Status;
  }

  return Status;
}

VOID
RtlReleaseCapturedSecurityDescriptor(IN PSECURITY_DESCRIPTOR CapturedSecurityDescriptor,
                                     IN KPROCESSOR_MODE PreviousMode,
                                     IN BOOLEAN CaptureIfKernel) 
{
  ASSERT(SECURITY_DESCRIPTOR_REVISION1 == CapturedSecurityDescriptor->Revision);

  if(PreviousMode == KernelMode && !CaptureIfKernel)
  {
    return;
  }

  if(NULL != CapturedSecurityDescriptor->Dacl)
  {
    ExFreePool(CapturedSecurityDescriptor->Dacl);
  }
  if(NULL != CapturedSecurityDescriptor->Sacl)
  {
    ExFreePool(CapturedSecurityDescriptor->Sacl);
  }
  if(NULL != CapturedSecurityDescriptor->Group)
  {
    ExFreePool(CapturedSecurityDescriptor->Group);
  }
  if(NULL != CapturedSecurityDescriptor->Owner)
  {
    ExFreePool(CapturedSecurityDescriptor->Owner);
  }
}

NTSTATUS
RtlCaptureObjectAttributes(OUT POBJECT_ATTRIBUTES Dest,
                           IN KPROCESSOR_MODE PreviousMode,
                           IN POOL_TYPE PoolType,
                           IN BOOLEAN CaptureIfKernel,
                           IN POBJECT_ATTRIBUTES UnsafeSrc)
{
  OBJECT_ATTRIBUTES Src;
  NTSTATUS Status = STATUS_SUCCESS;
  
  ASSERT(Dest != NULL);
  
  /*
   * Copy the object attributes to kernel space.
   */
  
  if(PreviousMode == UserMode)
  {
    _SEH_TRY
    {
      ProbeForRead(UnsafeSrc,
                   sizeof(OBJECT_ATTRIBUTES),
                   sizeof(ULONG));
      Src = *UnsafeSrc;
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    
    if(!NT_SUCCESS(Status))
    {
      return Status;
    }
  }
  else if(!CaptureIfKernel)
  {
    /* just copy the structure, the pointers are considered valid */
    *Dest = *UnsafeSrc;
    return STATUS_SUCCESS;
  }
  else
  {
    /* capture the object attributes even though it is considered to be valid */
    Src = *UnsafeSrc;
  }

  if(Src.Length < sizeof(OBJECT_ATTRIBUTES) || NULL == Src.ObjectName)
  {
    return STATUS_INVALID_PARAMETER;
  }

  Dest->Length = sizeof(OBJECT_ATTRIBUTES);
  Dest->RootDirectory = Src.RootDirectory;
  Dest->ObjectName = ExAllocatePoolWithTag(PoolType, sizeof(UNICODE_STRING), TAG_CAPT);
  if(NULL == Dest->ObjectName)
  {
    return STATUS_NO_MEMORY;
  }
  Status = RtlCaptureUnicodeString(Dest->ObjectName,
                                   PreviousMode,
                                   PoolType,
                                   CaptureIfKernel,
                                   Src.ObjectName);
  if(!NT_SUCCESS(Status))
  {
    ExFreePool(Dest->ObjectName);
    return Status;
  }
  Dest->Attributes = Src.Attributes;
  if(NULL == Src.SecurityDescriptor)
  {
    Dest->SecurityDescriptor = NULL;
  }
  else
  {
    Dest->SecurityDescriptor = ExAllocatePoolWithTag(PoolType, sizeof(SECURITY_DESCRIPTOR), TAG_CAPT);
    if(NULL == Dest->SecurityDescriptor)
    {
      RtlReleaseCapturedUnicodeString(Dest->ObjectName,
                                      PreviousMode,
                                      CaptureIfKernel);
      ExFreePool(Dest->ObjectName);
      return STATUS_NO_MEMORY;
    }
    Status = RtlCaptureSecurityDescriptor(Dest->SecurityDescriptor,
                                          PreviousMode,
                                          PoolType,
                                          CaptureIfKernel,
                                          Src.SecurityDescriptor);
    if(!NT_SUCCESS(Status))
    {
      ExFreePool(Dest->SecurityDescriptor);
      RtlReleaseCapturedUnicodeString(Dest->ObjectName,
                                      PreviousMode,
                                      CaptureIfKernel);
      ExFreePool(Dest->ObjectName);
      return Status;
    }
  }
  if(NULL == Src.SecurityQualityOfService)
  {
    Dest->SecurityQualityOfService = NULL;
  }
  else
  {
    Dest->SecurityQualityOfService = ExAllocatePoolWithTag(PoolType, sizeof(SECURITY_QUALITY_OF_SERVICE), TAG_CAPT);
    if(NULL == Dest->SecurityQualityOfService)
    {
      Status = STATUS_NO_MEMORY;
    }
    else
    {
      /*
       * Copy the data to kernel space.
       */
      _SEH_TRY
      {
        RtlCopyMemory(Dest->SecurityQualityOfService,
                      Src.SecurityQualityOfService,
                      sizeof(SECURITY_QUALITY_OF_SERVICE));
      }
      _SEH_HANDLE
      {
        Status = _SEH_GetExceptionCode();
      }
      _SEH_END;
      if(!NT_SUCCESS(Status))
      {
        ExFreePool(Dest->SecurityQualityOfService);
      }
    }
    if(!NT_SUCCESS(Status))
    {
      if(NULL != Dest->SecurityDescriptor)
      {
        RtlReleaseCapturedSecurityDescriptor(Dest->SecurityDescriptor,
                                             PreviousMode,
                                             CaptureIfKernel);
        ExFreePool(Dest->SecurityDescriptor);
      }
      RtlReleaseCapturedUnicodeString(Dest->ObjectName,
                                      PreviousMode,
                                      CaptureIfKernel);
      ExFreePool(Dest->ObjectName);
      return Status;
    }
  }
  
  return Status;
}

VOID
RtlReleaseCapturedObjectAttributes(IN POBJECT_ATTRIBUTES CapturedObjectAttributes,
                                   IN KPROCESSOR_MODE PreviousMode,
                                   IN BOOLEAN CaptureIfKernel) 
{
  ASSERT(NULL != CapturedObjectAttributes->ObjectName);

  if(PreviousMode == KernelMode && !CaptureIfKernel)
  {
    return;
  }

  if(NULL != CapturedObjectAttributes->SecurityQualityOfService)
  {
    ExFreePool(CapturedObjectAttributes->SecurityQualityOfService);
  }
  if(NULL != CapturedObjectAttributes->SecurityDescriptor)
  {
    RtlReleaseCapturedSecurityDescriptor(CapturedObjectAttributes->SecurityDescriptor,
                                         PreviousMode,
                                         CaptureIfKernel);
    ExFreePool(CapturedObjectAttributes->SecurityDescriptor);
  }
  RtlReleaseCapturedUnicodeString(CapturedObjectAttributes->ObjectName,
                                  PreviousMode,
                                  CaptureIfKernel);
  ExFreePool(CapturedObjectAttributes->ObjectName);
}

/*
 * @unimplemented
 */
VOID
STDCALL
RtlCaptureContext (
	OUT PCONTEXT ContextRecord
	)
{
	UNIMPLEMENTED;
}

/*
* @unimplemented
*/
USHORT
STDCALL
RtlCaptureStackBackTrace (
	IN ULONG FramesToSkip,
	IN ULONG FramesToCapture,
	OUT PVOID *BackTrace,
	OUT PULONG BackTraceHash OPTIONAL
	)
{
	UNIMPLEMENTED;
	return 0;
}

/* EOF */

