/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/fs/util.c
 * PURPOSE:         Misc Utility Functions for File System Drivers
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

#define FSRTL_MAX_RESOURCES 16

#define FTTYPE  ((ULONG)'f')
#define FT_BALANCED_READ_MODE \
    CTL_CODE(FTTYPE, 6, METHOD_NEITHER, FILE_ANY_ACCESS)

/* GLOBALS *******************************************************************/

BOOLEAN STDCALL MmIsFileAPagingFile(PFILE_OBJECT FileObject);
static ULONG FsRtlpAllocatedResources = 0;
static PERESOURCE FsRtlpResources;

/* PRIVATE FUNCTIONS**********************************************************/

VOID
STDCALL
INIT_FUNCTION
RtlpInitializeResources(VOID)
{
    ULONG i;

    /* Allocate the Resource Buffer */
    FsRtlpResources = FsRtlAllocatePool(NonPagedPool,
                                        FSRTL_MAX_RESOURCES*sizeof(ERESOURCE));

    /* Initialize the Resources */
    for (i = 0; i < FSRTL_MAX_RESOURCES; i++)
    {
        ExInitializeResource(&FsRtlpResources[i]);
    }
}

/* FUNCTIONS *****************************************************************/

/*++
 * @name FsRtlIsTotalDeviceFailure
 * @implemented NT 4.0
 *
 *     The FsRtlIsTotalDeviceFailure routine checks if an NTSTATUS error code
 *     represents a disk hardware failure.
 *
 * @param NtStatus
 *        The NTSTATUS Code to Test
 *
 * @return TRUE in case of Hardware Failure, FALSE otherwise.
 *
 * @remarks None.
 *
 *--*/
BOOLEAN
STDCALL
FsRtlIsTotalDeviceFailure(IN NTSTATUS NtStatus)
{
    return((NT_SUCCESS(NtStatus)) ||
           (STATUS_CRC_ERROR == NtStatus) ||
           (STATUS_DEVICE_DATA_ERROR == NtStatus) ? FALSE : TRUE);
}

/*++
 * @name FsRtlIsNtstatusExpected
 * @implemented NT 4.0
 *
 *     The FsRtlIsNtstatusExpected routine checks if an NTSTATUS error code
 *     is expected by the File System Support Library.
 *
 * @param NtStatus
 *        The NTSTATUS Code to Test
 *
 * @return TRUE if the Value is Expected, FALSE otherwise.
 *
 * @remarks None.
 *
 *--*/
BOOLEAN
STDCALL
FsRtlIsNtstatusExpected(IN NTSTATUS NtStatus)
{
    return((STATUS_DATATYPE_MISALIGNMENT == NtStatus) ||
           (STATUS_ACCESS_VIOLATION == NtStatus) ||
           (STATUS_ILLEGAL_INSTRUCTION == NtStatus) ||
           (STATUS_INSTRUCTION_MISALIGNMENT == NtStatus)) ? FALSE : TRUE;
}

/*++
 * @name FsRtlIsPagingFile
 * @implemented NT 4.0
 *
 *     The FsRtlIsPagingFile routine checks if the FileObject is a Paging File.
 *
 * @param FileObject
 *        A pointer to the File Object to be tested.
 *
 * @return TRUE if the File is a Paging File, FALSE otherwise.
 *
 * @remarks None.
 *
 *--*/
BOOLEAN
STDCALL
FsRtlIsPagingFile(IN PFILE_OBJECT FileObject)
{
    return MmIsFileAPagingFile(FileObject);
}

/*++
 * @name FsRtlNormalizeNtstatus
 * @implemented NT 4.0
 *
 *     The FsRtlNormalizeNtstatus routine normalizes an NTSTATUS error code.
 *
 * @param NtStatusToNormalize
 *        The NTSTATUS error code to Normalize.
 *
 * @param NormalizedNtStatus
 *        The NTSTATUS error code to return if the NtStatusToNormalize is not
 *        a proper expected error code by the File System Library.
 *
 * @return NtStatusToNormalize if it is an expected value, otherwise
 *         NormalizedNtStatus.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
STDCALL
FsRtlNormalizeNtstatus(IN NTSTATUS NtStatusToNormalize,
                       IN NTSTATUS NormalizedNtStatus)
{
    return(TRUE == FsRtlIsNtstatusExpected(NtStatusToNormalize)) ?
           NtStatusToNormalize : NormalizedNtStatus;
}

/*++
 * @name FsRtlAllocateResource
 * @implemented NT 4.0
 *
 *     The FsRtlAllocateResource routine returns a pre-initialized ERESOURCE
 *     for use by a File System Driver.
 *
 * @return A Pointer to a pre-initialized ERESOURCE.
 *
 * @remarks The File System Library only provides up to 16 Resources.
 *
 *--*/
PERESOURCE
STDCALL
FsRtlAllocateResource(VOID)
{
    /* Return a pre-allocated ERESOURCE */
    return &FsRtlpResources[FsRtlpAllocatedResources++ & FSRTL_MAX_RESOURCES];
}

/*++
 * @name FsRtlBalanceReads
 * @implemented NT 4.0
 *
 *     The FsRtlBalanceReads routine sends an IRP to an FTDISK Driver
 *     requesting the driver to balance read requests across a mirror set.
 *
 * @param TargetDevice
 *        A pointer to an FTDISK Device Object.
 *
 * @return The NTSTATUS error code returned by the FTDISK Driver.
 *
 * @remarks FTDISK is a Software RAID Implementation.
 *
 *--*/
NTSTATUS
STDCALL
FsRtlBalanceReads(PDEVICE_OBJECT TargetDevice)
{
    PIRP Irp;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    /* Initialize the Local Event */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* Build the special IOCTL */
    Irp = IoBuildDeviceIoControlRequest(FT_BALANCED_READ_MODE,
                                        TargetDevice,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);

    /* Send it */
    Status = IoCallDriver(TargetDevice, Irp);

    /* Wait if needed */
    if (Status == STATUS_PENDING)
    {
        Status = KeWaitForSingleObject(&Event,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);
        /* Return Status */
        Status = IoStatusBlock.Status;
    }

    return Status;
}

/*++
 * @name FsRtlPostPagingFileStackOverflow
 * @unimplemented NT 4.0
 *
 *     The FsRtlPostPagingFileStackOverflow routine
 *
 * @param Context
 *
 * @param Event
 *
 * @param StackOverflowRoutine
 *
 * @return
 *
 * @remarks None.
 *
 *--*/
VOID
STDCALL
FsRtlPostPagingFileStackOverflow(IN PVOID Context,
                                 IN PKEVENT Event,
                                 IN PFSRTL_STACK_OVERFLOW_ROUTINE StackOverflowRoutine)
{
    UNIMPLEMENTED;
}

/*++
 * @name FsRtlPostStackOverflow
 * @unimplemented NT 4.0
 *
 *     The FsRtlPostStackOverflow routine
 *
 * @param Context
 *
 * @param Event
 *
 * @param StackOverflowRoutine
 *
 * @return
 *
 * @remarks None.
 *
 *--*/
VOID
STDCALL
FsRtlPostStackOverflow(IN PVOID Context,
                       IN PKEVENT Event,
                       IN PFSRTL_STACK_OVERFLOW_ROUTINE StackOverflowRoutine)
{
    UNIMPLEMENTED;
}

/*++
 * @name FsRtlSyncVolumes
 * @implemented NT 4.0
 *
 *     The FsRtlSyncVolumes routine is deprecated.
 *
 * @return Always returns STATUS_SUCCESS.
 *
 * @remarks Deprecated.
 *
 *--*/
NTSTATUS
STDCALL
FsRtlSyncVolumes(DWORD Unknown0,
                 DWORD Unknown1,
                 DWORD Unknown2)
{
    return STATUS_SUCCESS;
}

/* EOF */
