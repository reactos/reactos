/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/vfat/blockdev.c
 * PURPOSE:          Temporary sector reading support
 * PROGRAMMER:       David Welch (welch@cwcom.net)
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/* FUNCTIONS ***************************************************************/

NTSTATUS
VfatReadSectors (IN PDEVICE_OBJECT pDeviceObject,
		 IN ULONG DiskSector, 
		 IN ULONG SectorCount,
		 IN OUT PUCHAR Buffer)
{
  LARGE_INTEGER sectorNumber;
  PIRP Irp;
  IO_STATUS_BLOCK IoStatus;
  KEVENT event;
  NTSTATUS Status;
  ULONG sectorSize;

  sectorNumber.u.LowPart = DiskSector << 9;
  sectorNumber.u.HighPart = DiskSector >> 23;

  KeInitializeEvent (&event, NotificationEvent, FALSE);
  sectorSize = BLOCKSIZE * SectorCount;

  DPRINT ("VFATReadSector(pDeviceObject %x, DiskSector %d, Buffer %x)\n",
	  pDeviceObject, DiskSector, Buffer);
  DPRINT ("sectorNumber %08lx:%08lx sectorSize %ld\n",
	  (unsigned long int) sectorNumber.u.LowPart,
	  (unsigned long int) sectorNumber.u.HighPart, sectorSize);


  DPRINT ("Building synchronous FSD Request...\n");
  Irp = IoBuildSynchronousFsdRequest (IRP_MJ_READ,
				      pDeviceObject,
				      Buffer,
				      sectorSize,
				      &sectorNumber, 
				      &event, 
				      &IoStatus);

  if (Irp == NULL)
    {
      DPRINT("IoBuildSynchronousFsdRequest failed\n");
      return(STATUS_UNSUCCESSFUL);;
    }

  DPRINT ("Calling IO Driver... with irp %x\n", Irp);
  Status = IoCallDriver (pDeviceObject, Irp);

  DPRINT ("Waiting for IO Operation for %x\n", Irp);
  if (Status == STATUS_PENDING)
    {
      DPRINT ("Operation pending\n");
      KeWaitForSingleObject (&event, Suspended, KernelMode, FALSE, NULL);
      DPRINT ("Getting IO Status... for %x\n", Irp);
      Status = IoStatus.Status;
    }

  if (!NT_SUCCESS (Status))
    {
      DPRINT ("IO failed!!! VFATREadSectors : Error code: %x\n", Status);
      DPRINT ("(pDeviceObject %x, DiskSector %x, Buffer %x, offset 0x%x%x)\n",
	      pDeviceObject, DiskSector, Buffer, sectorNumber.u.HighPart,
	      sectorNumber.u.LowPart);
      return (Status);
    }
  DPRINT ("Block request succeeded for %x\n", Irp);
  return (STATUS_SUCCESS);
}

NTSTATUS
VfatWriteSectors (IN PDEVICE_OBJECT pDeviceObject,
		  IN ULONG DiskSector,
		  IN ULONG SectorCount, 
		  IN PUCHAR Buffer)
{
  LARGE_INTEGER sectorNumber;
  PIRP Irp;
  IO_STATUS_BLOCK IoStatus;
  KEVENT event;
  NTSTATUS Status;
  ULONG sectorSize;

  DPRINT ("VFATWriteSector(pDeviceObject %x, DiskSector %d, Buffer %x)\n",
	  pDeviceObject, DiskSector, Buffer);

  sectorNumber.u.LowPart = DiskSector << 9;
  sectorNumber.u.HighPart = DiskSector >> 23;

  KeInitializeEvent (&event, NotificationEvent, FALSE);

  sectorSize = BLOCKSIZE * SectorCount;

  DPRINT ("Building synchronous FSD Request...\n");
  Irp = IoBuildSynchronousFsdRequest (IRP_MJ_WRITE,
				      pDeviceObject,
				      Buffer,
				      sectorSize,
				      &sectorNumber, 
				      &event, 
				      &IoStatus);

  if (!Irp)
    {
      DPRINT ("WRITE failed!!!\n");
      return (STATUS_UNSUCCESSFUL);
    }

  DPRINT ("Calling IO Driver...\n");
  Status = IoCallDriver (pDeviceObject, Irp);

  DPRINT ("Waiting for IO Operation...\n");
  if (Status == STATUS_PENDING)
    {
      KeWaitForSingleObject (&event, Suspended, KernelMode, FALSE, NULL);
      DPRINT ("Getting IO Status...\n");
      Status = IoStatus.Status;
    }

  if (!NT_SUCCESS (Status))
    {
      DPRINT1 ("IO failed!!! VFATWriteSectors : Error code: %x\n", Status);
      return (Status);
    }

  DPRINT ("Block request succeeded\n");
  return (STATUS_SUCCESS);
}
