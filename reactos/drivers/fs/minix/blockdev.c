/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/minix/minix.c
 * PURPOSE:          Minix FSD
 * PROGRAMMER:       David Welch (welch@mcmail.com)
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <string.h>
#include <internal/string.h>

#define NDEBUG
#include <internal/debug.h>

#include "minix.h"

/* FUNCTIONS ***************************************************************/

BOOLEAN MinixReadSector(IN PDEVICE_OBJECT pDeviceObject,
			IN ULONG	DiskSector,
			IN UCHAR*	Buffer)
{
    LARGE_INTEGER   sectorNumber;
    PIRP            irp;
    IO_STATUS_BLOCK ioStatus;
    KEVENT          event;
    NTSTATUS        status;
    ULONG           sectorSize;
    PULONG          mbr;
   
    DPRINT("MinixReadSector(pDeviceObject %x, DiskSector %d, Buffer %x)\n",
           pDeviceObject,DiskSector,Buffer);
   
    sectorNumber.u.HighPart = 0;
    sectorNumber.u.LowPart = DiskSector * BLOCKSIZE;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    sectorSize = BLOCKSIZE;

    mbr = ExAllocatePool(NonPagedPool, sectorSize);

    if (!mbr) {
        return FALSE;
    }


    irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
                                       pDeviceObject,
                                       mbr,
                                       sectorSize,
                                       &sectorNumber,
                                       &event,
                                       &ioStatus );

    if (!irp) {
        ExFreePool(mbr);
        return FALSE;
    }

    status = IoCallDriver(pDeviceObject,
                          irp);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        ExFreePool(mbr);
        return FALSE;
    }

    RtlCopyMemory(Buffer,mbr,sectorSize);

    ExFreePool(mbr);
    return TRUE;
}

BOOLEAN MinixWriteSector(IN PDEVICE_OBJECT pDeviceObject,
			IN ULONG	DiskSector,
			IN UCHAR*	Buffer)
{
    LARGE_INTEGER   sectorNumber;
    PIRP            irp;
    IO_STATUS_BLOCK ioStatus;
    KEVENT          event;
    NTSTATUS        status;
    ULONG           sectorSize;
    
    DPRINT("MinixWriteSector(pDeviceObject %x, DiskSector %d, Buffer %x)\n",
           pDeviceObject,DiskSector,Buffer);
   
    sectorNumber.u.HighPart = 0;
    sectorNumber.u.LowPart = DiskSector * BLOCKSIZE;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    sectorSize = BLOCKSIZE;

    irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE,
                                       pDeviceObject,
                                       Buffer,
                                       sectorSize,
                                       &sectorNumber,
                                       &event,
                                       &ioStatus );


    status = IoCallDriver(pDeviceObject,
                          irp);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    return TRUE;
}
