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

#define NDEBUG
#include <internal/debug.h>

#include "vfat.h"

/* FUNCTIONS ***************************************************************/

BOOLEAN VFATReadSectors(IN PDEVICE_OBJECT pDeviceObject,
                        IN ULONG    DiskSector,
                        IN ULONG        SectorCount,
			IN UCHAR*	Buffer)
{
    LARGE_INTEGER   sectorNumber;
    PIRP            irp;
    IO_STATUS_BLOCK ioStatus;
    KEVENT          event;
    NTSTATUS        status;
    ULONG           sectorSize;
   
    sectorNumber.LowPart = DiskSector << 9;
    sectorNumber.HighPart = DiskSector >> 23;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    sectorSize = BLOCKSIZE * SectorCount;


    DPRINT("VFATReadSector(pDeviceObject %x, DiskSector %d, Buffer %x)\n",
           pDeviceObject,
           DiskSector,
           Buffer);
    DPRINT("sectorNumber %08lx:%08lx sectorSize %ld\n", 
           (unsigned long int)sectorNumber.LowPart,
           (unsigned long int)sectorNumber.HighPart,
           sectorSize);


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
        DbgPrint("IO failed!!! VFATREadSectors : Error code: %x\n", status);
        DbgPrint("(pDeviceObject %x, DiskSector %x, Buffer %x, offset 0x%x%x)\n",
                 pDeviceObject,
                 DiskSector,
                 Buffer,
                 sectorNumber.HighPart,
                 sectorNumber.LowPart);
        return FALSE;
    }
    DPRINT("Block request succeeded\n");
    return TRUE;
}

BOOLEAN VFATWriteSectors(IN PDEVICE_OBJECT pDeviceObject,
                 IN ULONG   DiskSector,
                         IN ULONG       SectorCount,
			 IN UCHAR*	Buffer)
{
    LARGE_INTEGER   sectorNumber;
    PIRP            irp;
    IO_STATUS_BLOCK ioStatus;
    KEVENT          event;
    NTSTATUS        status;
    ULONG           sectorSize;
   
    DPRINT("VFATWriteSector(pDeviceObject %x, DiskSector %d, Buffer %x)\n",
           pDeviceObject,DiskSector,Buffer);

    sectorNumber.LowPart = DiskSector << 9;
    sectorNumber.HighPart = DiskSector >> 23;

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
        DbgPrint("IO failed!!! VFATWriteSectors : Error code: %x\n", status);
        return FALSE;
    }

    DPRINT("Block request succeeded\n");
    return TRUE;
}

