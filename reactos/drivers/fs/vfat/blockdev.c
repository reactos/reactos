/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/vfat/blockdev.c
 * PURPOSE:          Temporary sector reading support
 * PROGRAMMER:       David Welch (welch@mcmail.com)
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/string.h>

#define NDEBUG
#include <internal/debug.h>

#include "vfat.h"

/* FUNCTIONS ***************************************************************/

BOOLEAN VFATReadSector(IN PDEVICE_OBJECT pDeviceObject,
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
    int j;
   
   DPRINT("VFATReadSector(pDeviceObject %x, DiskSector %d, Buffer %x)\n",
   	    pDeviceObject,DiskSector,Buffer);

    sectorNumber.HighPart = 0;
    sectorNumber.LowPart = DiskSector * BLOCKSIZE;

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
        DbgPrint("READ failed!!!\n");
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
        DbgPrint("IO failed!!! Error code: %d\n", status);
        ExFreePool(mbr);
        return FALSE;
    }

   RtlCopyMemory(Buffer,mbr,sectorSize);

    ExFreePool(mbr);
    DPRINT("Block request succeeded\n");
    return TRUE;
}
