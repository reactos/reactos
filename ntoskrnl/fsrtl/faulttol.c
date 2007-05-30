/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/faulttol.c
 * PURPOSE:         Provides Fault Tolerance support for File System Drivers
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include "ntddft.h"
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

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
NTAPI
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

    /* Return the status */
    return Status;
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
NTAPI
FsRtlSyncVolumes(ULONG Unknown0,
                 ULONG Unknown1,
                 ULONG Unknown2)
{
    /* Always return success */
    return STATUS_SUCCESS;
}
