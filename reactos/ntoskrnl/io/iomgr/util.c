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
RtlpGetStackLimits(PULONG_PTR LowLimit,
                   PULONG_PTR HighLimit);

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
    PFILE_FULL_EA_INFORMATION EaBufferEnd;
    ULONG NextEaBufferOffset;
    LONG IntEaLength;

    /* Lenght of the rest. Inital equal to EaLength */
    IntEaLength = EaLength;

    /* Inital EaBuffer equal to EaBuffer */
    EaBufferEnd = EaBuffer;

    /* The rest length of the buffer */
    while (IntEaLength >= FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]))
    {
        /* rest of buffer must greater then the
           sizeof(FILE_FULL_EA_INFORMATION) + buffer */
        NextEaBufferOffset =
            EaBufferEnd->EaNameLength + EaBufferEnd->EaValueLength +
            FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) + 1;

        if (IntEaLength >= NextEaBufferOffset)
        {
            /* is the EaBufferName terminated with zero? */
            if (EaBufferEnd->EaName[EaBufferEnd->EaNameLength]==0)
            {
                /* more EaBuffers ahead */
                if (EaBufferEnd->NextEntryOffset == 0)
                {
                    /* test the rest buffersize */
                    IntEaLength = IntEaLength - NextEaBufferOffset;
                    if (IntEaLength >= 0)
                    {
                        return STATUS_SUCCESS;
                    }
                }
                else
                {
                    /* From the MSDN
                       http://msdn2.microsoft.com/en-us/library/ms795740.aspx
                       For all entries except the last, the value of
                       NextEntryOffset must be greater than zero and
                       must fall on a ULONG boundary
                     */
                    NextEaBufferOffset = ((NextEaBufferOffset + 3) & ~3);
                    if ((EaBufferEnd->NextEntryOffset == NextEaBufferOffset) &&
                        (EaBufferEnd->NextEntryOffset>0))
                    {
                        /* Rest of buffer must be greater then the
                           next offset */
                        IntEaLength =
                            IntEaLength - EaBufferEnd->NextEntryOffset;

                        if (IntEaLength >= 0)
                        {
                            EaBufferEnd = (PFILE_FULL_EA_INFORMATION)
                                ((ULONG_PTR)EaBufferEnd +
                                 EaBufferEnd->NextEntryOffset);
                            continue;
                        }
                    }
                }
            }
        }
        break;
    }

    if (ErrorOffset != NULL)
    {
        /* Calculate the error offset */
        *ErrorOffset = (ULONG)((ULONG_PTR)EaBufferEnd - (ULONG_PTR)EaBuffer);
    }

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
