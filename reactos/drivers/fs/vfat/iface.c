/* $Id: iface.c,v 1.45 2000/12/29 23:17:12 dwelch Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/vfat/iface.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 * UPDATE HISTORY:
 *    ??           Created
 *   24-10-1998   Fixed bugs in long filename support
 *                Fixed a bug that prevented unsuccessful file open requests 
 *                being reported
 *                Now works with long filenames that span over a sector 
 *                boundary
 *   28-10-1998   Reads entire FAT into memory
 *                VFatReadSector modified to read in more than one sector at a
 *                time
 *   7-11-1998    Fixed bug that assumed that directory data could be 
 *                fragmented
 *   8-12-1998    Added FAT32 support
 *                Added initial writability functions
 *                WARNING: DO NOT ATTEMPT TO TEST WRITABILITY FUNCTIONS!!!
 *   12-12-1998   Added basic support for FILE_STANDARD_INFORMATION request
 *
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <wchar.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/* GLOBALS *****************************************************************/

static PDRIVER_OBJECT VfatDriverObject;

/* FUNCTIONS ****************************************************************/

BOOLEAN 
VfatHasFileSystem (PDEVICE_OBJECT DeviceToMount)
/*
 * FUNCTION: Tests if the device contains a filesystem that can be mounted 
 *           by this fsd
 */
{
  BootSector *Boot;

  Boot = ExAllocatePool (NonPagedPool, 512);

  VFATReadSectors (DeviceToMount, 0, 1, (UCHAR *) Boot);

  DPRINT ("Boot->SysType %.5s\n", Boot->SysType);
  if (strncmp (Boot->SysType, "FAT12", 5) == 0 ||
      strncmp (Boot->SysType, "FAT16", 5) == 0 ||
      strncmp (((struct _BootSector32 *) (Boot))->SysType, "FAT32", 5) == 0)
    {
      ExFreePool (Boot);
      return (TRUE);
    }
  ExFreePool (Boot);
  return (FALSE);
}

NTSTATUS
VfatMountDevice (PDEVICE_EXTENSION DeviceExt, PDEVICE_OBJECT DeviceToMount)
/*
 * FUNCTION: Mounts the device
 */
{
  DPRINT ("Mounting VFAT device...");
  DPRINT ("DeviceExt %x\n", DeviceExt);

  DeviceExt->Boot = ExAllocatePool (NonPagedPool, 512);
  VFATReadSectors (DeviceToMount, 0, 1, (UCHAR *) DeviceExt->Boot);

  DPRINT ("DeviceExt->Boot->BytesPerSector %x\n",
	  DeviceExt->Boot->BytesPerSector);

  DeviceExt->FATStart = DeviceExt->Boot->ReservedSectors;
  DeviceExt->rootDirectorySectors =
    (DeviceExt->Boot->RootEntries * 32) / DeviceExt->Boot->BytesPerSector;
  DeviceExt->rootStart =
    DeviceExt->FATStart +
    DeviceExt->Boot->FATCount * DeviceExt->Boot->FATSectors;
  DeviceExt->dataStart =
    DeviceExt->rootStart + DeviceExt->rootDirectorySectors;
  DeviceExt->FATEntriesPerSector = DeviceExt->Boot->BytesPerSector / 32;
  DeviceExt->BytesPerCluster = DeviceExt->Boot->SectorsPerCluster *
    DeviceExt->Boot->BytesPerSector;

  if (strncmp (DeviceExt->Boot->SysType, "FAT12", 5) == 0)
    {
      DeviceExt->FatType = FAT12;
    }
  else
    if (strncmp
	(((struct _BootSector32 *) (DeviceExt->Boot))->SysType, "FAT32",
	 5) == 0)
    {
      DeviceExt->FatType = FAT32;
      DeviceExt->rootDirectorySectors = DeviceExt->Boot->SectorsPerCluster;
      DeviceExt->rootStart =
	DeviceExt->FATStart + DeviceExt->Boot->FATCount
	* ((struct _BootSector32 *) (DeviceExt->Boot))->FATSectors32;
      DeviceExt->dataStart = DeviceExt->rootStart;
    }
  else
    {
      DeviceExt->FatType = FAT16;
    }

  // with FAT32 it's not a good idea to load always fat in memory
  // because on a 8GB partition with 2 KO clusters, the fat = 8 MO
  if (DeviceExt->FatType != FAT32)
    {
      DeviceExt->FAT =
	ExAllocatePool (NonPagedPool,
			BLOCKSIZE * DeviceExt->Boot->FATSectors);
      VFATReadSectors (DeviceToMount, DeviceExt->FATStart,
		       DeviceExt->Boot->FATSectors, (UCHAR *) DeviceExt->FAT);
    }
  return STATUS_SUCCESS;
}

NTSTATUS 
VfatMount (PDEVICE_OBJECT DeviceToMount)
/*
 * FUNCTION: Mount the filesystem
 */
{
  PDEVICE_OBJECT DeviceObject;
  PDEVICE_EXTENSION DeviceExt;

  IoCreateDevice (VfatDriverObject,
		  sizeof (DEVICE_EXTENSION),
		  NULL, FILE_DEVICE_FILE_SYSTEM, 0, FALSE, &DeviceObject);
  DeviceObject->Flags = DeviceObject->Flags | DO_DIRECT_IO;
  DeviceExt = (PVOID) DeviceObject->DeviceExtension;
  // use same vpb as device disk
  DeviceObject->Vpb = DeviceToMount->Vpb;
  VfatMountDevice (DeviceExt, DeviceToMount);
  DeviceObject->Vpb->Flags |= VPB_MOUNTED;
  DeviceExt->StorageDevice = IoAttachDeviceToDeviceStack (DeviceObject,
							  DeviceToMount);
  ExInitializeResourceLite (&DeviceExt->DirResource);
  ExInitializeResourceLite (&DeviceExt->FatResource);

  KeInitializeSpinLock (&DeviceExt->FcbListLock);
  InitializeListHead (&DeviceExt->FcbListHead);

  /* read serial number */
  if (DeviceExt->FatType == FAT12 || DeviceExt->FatType == FAT16)
    DeviceObject->Vpb->SerialNumber =
      ((struct _BootSector *) (DeviceExt->Boot))->VolumeID;
  else if (DeviceExt->FatType == FAT32)
    DeviceObject->Vpb->SerialNumber =
      ((struct _BootSector32 *) (DeviceExt->Boot))->VolumeID;

  /* read volume label */
  ReadVolumeLabel (DeviceExt, DeviceObject->Vpb);

  return (STATUS_SUCCESS);
}

NTSTATUS STDCALL
VfatFileSystemControl (PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: File system control
 */
{
  PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation (Irp);
//   PVPB       vpb = Stack->Parameters.Mount.Vpb;
  PDEVICE_OBJECT DeviceToMount = Stack->Parameters.Mount.DeviceObject;
  NTSTATUS Status;

//   DPRINT("VFAT FSC\n");
  DbgPrint ("VFAT FSC\n");

  /* FIXME: should make sure that this is actually a mount request!  */

  if (VfatHasFileSystem (DeviceToMount))
    {
      DPRINT ("VFAT: Recognized volume\n");
      Status = VfatMount (DeviceToMount);
    }
  else
    {
      DPRINT ("VFAT: Unrecognized Volume\n");
      Status = STATUS_UNRECOGNIZED_VOLUME;
    }

  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = 0;

  IoCompleteRequest (Irp, IO_NO_INCREMENT);
  return (Status);
}

NTSTATUS STDCALL
DriverEntry (PDRIVER_OBJECT _DriverObject, PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Called by the system to initalize the driver
 * ARGUMENTS:
 *           DriverObject = object describing this driver
 *           RegistryPath = path to our configuration entries
 * RETURNS: Success or failure
 */
{
  PDEVICE_OBJECT DeviceObject;
  NTSTATUS ret;
  UNICODE_STRING DeviceName;

  DbgPrint ("VFAT 0.0.6\n");

  VfatDriverObject = _DriverObject;

  RtlInitUnicodeString (&DeviceName, L"\\Device\\Vfat");
  ret = IoCreateDevice (VfatDriverObject, 0, &DeviceName,
			FILE_DEVICE_FILE_SYSTEM, 0, FALSE, &DeviceObject);
  if (ret != STATUS_SUCCESS)
    {
      return (ret);
    }

  DeviceObject->Flags = DO_DIRECT_IO;
  VfatDriverObject->MajorFunction[IRP_MJ_CLOSE] = VfatClose;
  VfatDriverObject->MajorFunction[IRP_MJ_CREATE] = VfatCreate;
  VfatDriverObject->MajorFunction[IRP_MJ_READ] = VfatRead;
  VfatDriverObject->MajorFunction[IRP_MJ_WRITE] = VfatWrite;
  VfatDriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] =
    VfatFileSystemControl;
  VfatDriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] =
    VfatQueryInformation;
  VfatDriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] =
    VfatSetInformation;
  VfatDriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] =
    VfatDirectoryControl;
  VfatDriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] =
    VfatQueryVolumeInformation;
  VfatDriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = VfatShutdown;

  VfatDriverObject->DriverUnload = NULL;

  IoRegisterFileSystem (DeviceObject);

  return (STATUS_SUCCESS);
}
