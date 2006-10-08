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
NTAPI
IoAcquireCancelSpinLock(PKIRQL Irql)
{
    /* Just acquire the internal lock */
    KeAcquireSpinLock(&CancelSpinLock,Irql);
}

/*
 * @implemented
 */
PVOID
NTAPI
IoGetInitialStack(VOID)
{
    /* Return the initial stack from the TCB */
    return PsGetCurrentThread()->Tcb.InitialStack;
}

/*
 * @implemented
 */
VOID
NTAPI
IoGetStackLimits(OUT PULONG LowLimit,
                 OUT PULONG HighLimit)
{
    /* Return the limits from the TEB... this is wrong! */
    DPRINT1("FIXME: IoGetStackLimits returning B*LLSHIT!\n");
    *LowLimit = (ULONG)NtCurrentTeb()->Tib.StackLimit;
    *HighLimit = (ULONG)NtCurrentTeb()->Tib.StackBase;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
IoIsSystemThread(IN PETHREAD Thread)
{
    /* Call the Ps Function */
    return PsIsSystemThread(Thread);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
IoIsWdmVersionAvailable(IN UCHAR MajorVersion,
                        IN UCHAR MinorVersion)
{
    /* Return support for WDM 1.10 (Windows 2000) */
    if (MajorVersion <= 1 && MinorVersion <= 0x10) return TRUE;
    return FALSE;
}

/*
 * @implemented
 */
PEPROCESS
NTAPI
IoGetCurrentProcess(VOID)
{
    return (PEPROCESS)PsGetCurrentThread()->Tcb.ApcState.Process;
}

/*
 * @implemented
 */
VOID
NTAPI
IoReleaseCancelSpinLock(KIRQL Irql)
{
    /* Release the internal lock */
    KeReleaseSpinLock(&CancelSpinLock,Irql);
}

/*
 * @implemented
 */
PEPROCESS
NTAPI
IoThreadToProcess(IN PETHREAD Thread)
{
    /* Return the thread's process */
    return Thread->ThreadsProcess;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoCheckDesiredAccess(IN OUT PACCESS_MASK DesiredAccess,
                     IN ACCESS_MASK GrantedAccess)
{
    PAGED_CODE();

    /* Map the generic mask */
    RtlMapGenericMask(DesiredAccess,
                      &IoFileObjectType->TypeInfo.GenericMapping);

    /* Fail if the access masks don't grant full access */
    if ((~(*DesiredAccess) & GrantedAccess)) return STATUS_ACCESS_DENIED;
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoCheckEaBufferValidity(IN PFILE_FULL_EA_INFORMATION EaBuffer,
                        IN ULONG EaLength,
                        OUT PULONG ErrorOffset)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoCheckFunctionAccess(IN ACCESS_MASK GrantedAccess,
                      IN UCHAR MajorFunction,
                      IN UCHAR MinorFunction,
                      IN ULONG IoControlCode,
                      IN PVOID ExtraData OPTIONAL,
                      IN PVOID ExtraData2 OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoValidateDeviceIoControlAccess(IN PIRP Irp,
                                IN ULONG RequiredAccess)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
VOID
NTAPI
IoSetDeviceToVerify(IN PETHREAD Thread,
                    IN PDEVICE_OBJECT DeviceObject)
{
    /* Set the pointer in the thread */
    Thread->DeviceToVerify = DeviceObject;
}

/*
 * @implemented
 */
VOID
NTAPI
IoSetHardErrorOrVerifyDevice(IN PIRP Irp,
                             IN PDEVICE_OBJECT DeviceObject)
{
    /* Set the pointer in the IRP */
    Irp->Tail.Overlay.Thread->DeviceToVerify = DeviceObject;
}

/*
 * @implemented
 */
PDEVICE_OBJECT
NTAPI
IoGetDeviceToVerify(IN PETHREAD Thread)
{
    /* Return the pointer that was set with IoSetDeviceToVerify */
    return Thread->DeviceToVerify;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoCheckQuerySetVolumeInformation(IN FS_INFORMATION_CLASS FsInformationClass,
                                 IN ULONG Length,
                                 IN BOOLEAN SetOperation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
/* EOF */
