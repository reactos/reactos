/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/util.c
 * PURPOSE:         I/O Utility Functions
 * PROGRAMMERS:     <UNKNOWN>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* DATA **********************************************************************/

KSPIN_LOCK CancelSpinLock;

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID
STDCALL
IoAcquireCancelSpinLock(PKIRQL Irql)
{
   KeAcquireSpinLock(&CancelSpinLock,Irql);
}

/*
 * @implemented
 */
PVOID
STDCALL
IoGetInitialStack(VOID)
{
    return(PsGetCurrentThread()->Tcb.InitialStack);
}

/*
 * @implemented
 */
VOID
STDCALL
IoGetStackLimits(OUT PULONG LowLimit,
                 OUT PULONG HighLimit)
{
    *LowLimit = (ULONG)NtCurrentTeb()->Tib.StackLimit;
    *HighLimit = (ULONG)NtCurrentTeb()->Tib.StackBase;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
IoIsSystemThread(IN PETHREAD Thread)
{
    /* Call the Ps Function */
    return PsIsSystemThread(Thread);
}

/*
 * @implemented
 */
BOOLEAN STDCALL
IoIsWdmVersionAvailable(IN UCHAR MajorVersion,
                        IN UCHAR MinorVersion)
{
   /* MinorVersion = 0x20 : WinXP
                     0x10 : Win2k
                     0x5  : WinMe
                     <0x5 : Win98

      We report Win2k now
      */
   if (MajorVersion <= 1 && MinorVersion <= 0x10)
      return TRUE;
   return FALSE;
}

/*
 * @implemented
 */
VOID
STDCALL
IoReleaseCancelSpinLock(KIRQL Irql)
{
   KeReleaseSpinLock(&CancelSpinLock,Irql);
}

/*
 * @implemented
 */
PEPROCESS
STDCALL
IoThreadToProcess(IN PETHREAD Thread)
{
    return(Thread->ThreadsProcess);
}

/*
 * @implemented
 */
NTSTATUS STDCALL
IoCheckDesiredAccess(IN OUT PACCESS_MASK DesiredAccess,
		     IN ACCESS_MASK GrantedAccess)
{
  PAGED_CODE();

  RtlMapGenericMask(DesiredAccess,
		    &IoFileObjectType->TypeInfo.GenericMapping);

  if ((~(*DesiredAccess) & GrantedAccess) != 0)
    return STATUS_ACCESS_DENIED;
  else
    return STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
IoCheckEaBufferValidity(IN PFILE_FULL_EA_INFORMATION EaBuffer,
			IN ULONG EaLength,
			OUT PULONG ErrorOffset)
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
IoCheckFunctionAccess(IN ACCESS_MASK GrantedAccess,
		      IN UCHAR MajorFunction,
		      IN UCHAR MinorFunction,
		      IN ULONG IoControlCode,
		      IN PVOID ExtraData OPTIONAL,
		      IN PVOID ExtraData2 OPTIONAL)
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoValidateDeviceIoControlAccess(IN  PIRP Irp,
                                IN  ULONG RequiredAccess)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
VOID STDCALL
IoSetDeviceToVerify(IN PETHREAD Thread,
		    IN PDEVICE_OBJECT DeviceObject)
{
  Thread->DeviceToVerify = DeviceObject;
}


/*
 * @implemented
 */
VOID STDCALL
IoSetHardErrorOrVerifyDevice(IN PIRP Irp,
			     IN PDEVICE_OBJECT DeviceObject)
{
  Irp->Tail.Overlay.Thread->DeviceToVerify = DeviceObject;
}

/*
 * @implemented
 */
PDEVICE_OBJECT STDCALL
IoGetDeviceToVerify(IN PETHREAD Thread)
/*
 * FUNCTION: Returns a pointer to the device, representing a removable-media
 * device, that is the target of the given thread's I/O request
 */
{
  return(Thread->DeviceToVerify);
}



/* EOF */
