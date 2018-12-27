/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/iomgr/util.c
 * PURPOSE:         I/O Utility Functions
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Aleksey Bragin (aleksey@reactos.org)
 *                  Daniel Zimmerman (netzimme@aim.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

VOID
NTAPI
RtlpGetStackLimits(PULONG_PTR StackBase,
                   PULONG_PTR StackLimit);

/* PRIVATE FUNCTIONS *********************************************************/

NTSTATUS
NTAPI
IoComputeDesiredAccessFileObject(IN PFILE_OBJECT FileObject,
                                 IN PACCESS_MASK DesiredAccess)
{
    /* Assume failure */
    *DesiredAccess = 0;

    /* First check we really have a FileObject */
    if (OBJECT_TO_OBJECT_HEADER(FileObject)->Type != IoFileObjectType)
    {
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    /* Then compute desired access:
     * Check if the handle has either FILE_WRITE_DATA or FILE_APPEND_DATA was
     * granted. However, if this is a named pipe, make sure we don't ask for
     * FILE_APPEND_DATA as it interferes with the FILE_CREATE_PIPE_INSTANCE
     * access right!
     */
    *DesiredAccess = ((!(FileObject->Flags & FO_NAMED_PIPE) ? FILE_APPEND_DATA : 0) | FILE_WRITE_DATA);

    return STATUS_SUCCESS;
}

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID
NTAPI
IoAcquireCancelSpinLock(OUT PKIRQL Irql)
{
    /* Just acquire the internal lock */
    *Irql = KeAcquireQueuedSpinLock(LockQueueIoCancelLock);
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
IoGetStackLimits(OUT PULONG_PTR LowLimit,
                 OUT PULONG_PTR HighLimit)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    ULONG_PTR DpcStack = (ULONG_PTR)(Prcb->DpcStack);
    volatile ULONG_PTR StackAddress;

    /* Save our stack address so we always know it's valid */
    StackAddress = (ULONG_PTR)(&StackAddress);

    /* Get stack values */
    RtlpGetStackLimits(LowLimit, HighLimit);

    /* Check if we're outside the stack */
    if ((StackAddress < *LowLimit) || (StackAddress > *HighLimit))
    {
        /* Check if we may be in a DPC */
        if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
        {
            /* Check if we really are in a DPC */
            if ((Prcb->DpcRoutineActive) &&
                (StackAddress <= DpcStack) &&
                (StackAddress >= DpcStack - KERNEL_STACK_SIZE))
            {
                /* Use the DPC stack limits */
                *HighLimit = DpcStack;
                *LowLimit = DpcStack - KERNEL_STACK_SIZE;
            }
        }
    }
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
    /* Return support for WDM 1.30 (Windows Server 2003) */
    if (MajorVersion <= 1 && MinorVersion <= 0x30) return TRUE;
    return FALSE;
}

/*
 * @implemented
 */
PEPROCESS
NTAPI
IoGetCurrentProcess(VOID)
{
    /* Return the current thread's process */
    return (PEPROCESS)PsGetCurrentThread()->Tcb.ApcState.Process;
}

/*
 * @implemented
 */
VOID
NTAPI
IoReleaseCancelSpinLock(IN KIRQL Irql)
{
    /* Release the internal lock */
    KeReleaseQueuedSpinLock(LockQueueIoCancelLock, Irql);
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
 * @implemented
 */
NTSTATUS
NTAPI
IoCheckEaBufferValidity(IN PFILE_FULL_EA_INFORMATION EaBuffer,
                        IN ULONG EaLength,
                        OUT PULONG ErrorOffset)
{
    ULONG NextEntryOffset;
    UCHAR EaNameLength;
    ULONG ComputedLength;
    PFILE_FULL_EA_INFORMATION Current;

    PAGED_CODE();

    /* We will browse all the entries */
    for (Current = EaBuffer; ; Current = (PFILE_FULL_EA_INFORMATION)((ULONG_PTR)Current + NextEntryOffset))
    {
        /* Check that we have enough bits left for the current entry */
        if (EaLength < FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName))
        {
            goto FailPath;
        }

        EaNameLength = Current->EaNameLength;
        ComputedLength = Current->EaValueLength + EaNameLength + FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName) + 1;
        /* Check that we have enough bits left for storing the name and its value */
        if (EaLength < ComputedLength)
        {
            goto FailPath;
        }

        /* Make sure the name is null terminated */
        if (Current->EaName[EaNameLength] != ANSI_NULL)
        {
            goto FailPath;
        }

        /* Get the next entry offset */
        NextEntryOffset = Current->NextEntryOffset;
        /* If it's 0, it's a termination case */
        if (NextEntryOffset == 0)
        {
            /* If we don't overflow! */
            if ((LONG)(EaLength - ComputedLength) < 0)
            {
                goto FailPath;
            }

            break;
        }

        /* Compare the next offset we computed with the provided one, they must match */
        if (ALIGN_UP_BY(ComputedLength, sizeof(ULONG)) != NextEntryOffset)
        {
            goto FailPath;
        }

        /* Check next entry offset value is positive */
        if ((LONG)NextEntryOffset < 0)
        {
            goto FailPath;
        }

        /* Compute the remaining bits */
        EaLength -= NextEntryOffset;
        /* We must have bits left */
        if (EaLength < 0)
        {
            goto FailPath;
        }

        /* Move to the next entry */
    }

    /* If we end here, everything went OK */
    return STATUS_SUCCESS;

FailPath:
    /* If we end here, we failed, set failed offset */
    *ErrorOffset = (ULONG_PTR)Current - (ULONG_PTR)EaBuffer;
    return STATUS_EA_LIST_INCONSISTENT;
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
