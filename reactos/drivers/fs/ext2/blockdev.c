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
#include <string.h>
#include <internal/string.h>

#define NDEBUG
#include <internal/debug.h>

#include "ext2fs.h"

/* FUNCTIONS ***************************************************************/

BOOLEAN Ext2ReadSectors(IN PDEVICE_OBJECT pDeviceObject,
			IN ULONG	DiskSector,
                        IN ULONG        SectorCount,
			IN PVOID	Buffer)
{
    LARGE_INTEGER   sectorNumber;
    PIRP            irp;
    IO_STATUS_BLOCK ioStatus;
    KEVENT          event;
    NTSTATUS        status;
    ULONG           sectorSize;
    int j;
   
   DPRINT("VFATReadSector(pDeviceObject %x, DiskSector %d, Buffer %x)\n",
   	    pDeviceObject,DiskSector,Buffer);

    SET_LARGE_INTEGER_HIGH_PART(sectorNumber, 0);
    SET_LARGE_INTEGER_LOW_PART(sectorNumber, DiskSector * BLOCKSIZE);

DPRINT("DiskSector:%ld BLKSZ:%ld sectorNumber:%ld:%ld\n", 
       (unsigned long) DiskSector,
       (unsigned long) BLOCKSIZE,
       (unsigned long) GET_LARGE_INTEGER_HIGH_PART(sectorNumber),
       (unsigned long) GET_LARGE_INTEGER_LOW_PART(sectorNumber));

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    sectorSize = BLOCKSIZE*SectorCount;


    DPRINT("Building synchronous FSD Request...\n");
    irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
                                       pDeviceObject,
                                       Buffer,
                                       sectorSize,
                                       &sectorNumber,
                                       &event,
                                       &ioStatus );

    if (!irp) {
        DbgPrint("READ failed!!!\n");
        return FALSE;
    }

    DPRINT("Calling IO Driver...\n");
    status = IoCallDriver(pDeviceObject,
                          irp);

    DPRINT("Waiting for IO Operation...\n");
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);
        DPRINT("Getting IO Status...\n");
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        DbgPrint("IO failed!!! Error code: %d(%x)\n", status, status);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN VFATWriteSectors(IN PDEVICE_OBJECT pDeviceObject,
		     	 IN ULONG	DiskSector,
                         IN ULONG       SectorCount,
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
   
   DPRINT("VFATWriteSector(pDeviceObject %x, DiskSector %d, Buffer %x)\n",
   	    pDeviceObject,DiskSector,Buffer);

    SET_LARGE_INTEGER_HIGH_PART(sectorNumber, 0);
    SET_LARGE_INTEGER_LOW_PART(sectorNumber, DiskSector * BLOCKSIZE);

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    sectorSize = BLOCKSIZE*SectorCount;


    DPRINT("Building synchronous FSD Request...\n");
    irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE,
                                       pDeviceObject,
                                       Buffer,
                                       sectorSize,
                                       &sectorNumber,
                                       &event,
                                       &ioStatus );

    if (!irp) {
        DbgPrint("WRITE failed!!!\n");
        return FALSE;
    }

    DPRINT("Calling IO Driver...\n");
    status = IoCallDriver(pDeviceObject,
                          irp);

    DPRINT("Waiting for IO Operation...\n");
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);
        DPRINT("Getting IO Status...\n");
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        DbgPrint("IO failed!!! Error code: %d(%x)\n", status, status);
        return FALSE;
    }


    ExFreePool(mbr);
    DPRINT("Block request succeeded\n");
    return TRUE;
}


