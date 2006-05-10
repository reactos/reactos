/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/disk.c
 * PURPOSE:         I/O Support for Hal Disk (Partition Table/MBR) Routines.
 *
 * PROGRAMMERS:     Eric Kohl (ekohl@rz-online.de)
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* LOCAL MACROS and TYPES ***************************************************/

#define AUTO_DRIVE         ((ULONG)-1)

#define PARTITION_MAGIC    0xaa55

#include <pshpack1.h>

typedef struct _REG_DISK_MOUNT_INFO
{
  ULONG Signature;
  LARGE_INTEGER StartingOffset;
} REG_DISK_MOUNT_INFO, *PREG_DISK_MOUNT_INFO;

#include <poppack.h>

typedef enum _DISK_MANAGER
{
  NoDiskManager,
  OntrackDiskManager,
  EZ_Drive
} DISK_MANAGER;

HAL_DISPATCH HalDispatchTable =
{
    HAL_DISPATCH_VERSION,
    (pHalQuerySystemInformation) NULL,	// HalQuerySystemInformation
    (pHalSetSystemInformation) NULL,	// HalSetSystemInformation
    (pHalQueryBusSlots) NULL,			// HalQueryBusSlots
    0,
    (pHalExamineMBR) HalExamineMBR,
    (pHalIoAssignDriveLetters) xHalIoAssignDriveLetters,
    (pHalIoReadPartitionTable) xHalIoReadPartitionTable,
    (pHalIoSetPartitionInformation) xHalIoSetPartitionInformation,
    (pHalIoWritePartitionTable) xHalIoWritePartitionTable,
    (pHalHandlerForBus) NULL,			// HalReferenceHandlerForBus
    (pHalReferenceBusHandler) NULL,		// HalReferenceBusHandler
    (pHalReferenceBusHandler) NULL,		// HalDereferenceBusHandler
    (pHalInitPnpDriver) NULL,               //HalInitPnpDriver;
    (pHalInitPowerManagement) NULL,         //HalInitPowerManagement;
    (pHalGetDmaAdapter) NULL,               //HalGetDmaAdapter;
    (pHalGetInterruptTranslator) NULL,      //HalGetInterruptTranslator;
    (pHalStartMirroring) NULL,              //HalStartMirroring;
    (pHalEndMirroring) NULL,                //HalEndMirroring;
    (pHalMirrorPhysicalMemory) NULL,        //HalMirrorPhysicalMemory;
    (pHalEndOfBoot) NULL,                   //HalEndOfBoot;
    (pHalMirrorVerify) NULL                //HalMirrorVerify;
};

HAL_PRIVATE_DISPATCH HalPrivateDispatchTable =
{
    HAL_PRIVATE_DISPATCH_VERSION,
    (pHalHandlerForBus) NULL,
    (pHalHandlerForConfigSpace) NULL,
    (pHalLocateHiberRanges) NULL,
    (pHalRegisterBusHandler) NULL,
    (pHalSetWakeEnable) NULL,
    (pHalSetWakeAlarm) NULL,
    (pHalTranslateBusAddress) NULL,
    (pHalAssignSlotResources) NULL,
    (pHalHaltSystem) NULL,
    (pHalFindBusAddressTranslation) NULL,
    (pHalResetDisplay) NULL,
    (pHalAllocateMapRegisters) NULL,
    (pKdSetupPciDeviceForDebugging) NULL,
    (pKdReleasePciDeviceForDebugging) NULL,
    (pKdGetAcpiTablePhase0) NULL,
    (pKdCheckPowerButton) NULL,
    (pHalVectorToIDTEntry) NULL,
    (pKdMapPhysicalMemory64) NULL,
    (pKdUnmapVirtualAddress) NULL
};

const WCHAR DiskMountString[] = L"\\DosDevices\\%C:";

/* FUNCTIONS *****************************************************************/

NTSTATUS
FASTCALL
xHalQueryDriveLayout(IN PUNICODE_STRING DeviceName,
		     OUT PDRIVE_LAYOUT_INFORMATION *LayoutInfo)
{
  IO_STATUS_BLOCK StatusBlock;
  DISK_GEOMETRY DiskGeometry;
  PDEVICE_OBJECT DeviceObject = NULL;
  PFILE_OBJECT FileObject;
  KEVENT Event;
  PIRP Irp;
  NTSTATUS Status;

  DPRINT("xHalpQueryDriveLayout %wZ %p\n",
	 DeviceName,
	 LayoutInfo);

  /* Get the drives sector size */
  Status = IoGetDeviceObjectPointer(DeviceName,
				    FILE_READ_DATA,
				    &FileObject,
				    &DeviceObject);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Status %x\n", Status);
      return(Status);
    }

  KeInitializeEvent(&Event,
		    NotificationEvent,
		    FALSE);

  Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_GEOMETRY,
				      DeviceObject,
				      NULL,
				      0,
				      &DiskGeometry,
				      sizeof(DISK_GEOMETRY),
				      FALSE,
				      &Event,
				      &StatusBlock);
  if (Irp == NULL)
    {
      ObDereferenceObject(FileObject);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  Status = IoCallDriver(DeviceObject,
			Irp);
  if (Status == STATUS_PENDING)
    {
      KeWaitForSingleObject(&Event,
			    Executive,
			    KernelMode,
			    FALSE,
			    NULL);
      Status = StatusBlock.Status;
    }
  if (!NT_SUCCESS(Status))
    {
      if (DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
	{
	  DiskGeometry.BytesPerSector = 512;
	}
      else
	{
	  ObDereferenceObject(FileObject);
	  return(Status);
	}
    }

  DPRINT("DiskGeometry.BytesPerSector: %d\n",
	 DiskGeometry.BytesPerSector);

  if (DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
    {
      PDRIVE_LAYOUT_INFORMATION Buffer;

      /* Allocate a partition list for a single entry. */
      Buffer = ExAllocatePool(NonPagedPool,
			      sizeof(DRIVE_LAYOUT_INFORMATION));
      if (Buffer != NULL)
	{
	  RtlZeroMemory(Buffer,
			sizeof(DRIVE_LAYOUT_INFORMATION));
	  Buffer->PartitionCount = 1;
	  *LayoutInfo = Buffer;

	  Status = STATUS_SUCCESS;
	}
      else
        {
	  Status = STATUS_UNSUCCESSFUL;
	}
    }
  else
    {
      /* Read the partition table */
      Status = IoReadPartitionTable(DeviceObject,
				    DiskGeometry.BytesPerSector,
				    FALSE,
				    LayoutInfo);
    }

  ObDereferenceObject(FileObject);

  return(Status);
}


static NTSTATUS
xHalpReadSector (IN PDEVICE_OBJECT DeviceObject,
		 IN ULONG SectorSize,
		 IN PLARGE_INTEGER SectorOffset,
		 IN PVOID Sector)
{
  IO_STATUS_BLOCK StatusBlock;
  KEVENT Event;
  PIRP Irp;
  NTSTATUS Status;

  DPRINT("xHalpReadSector() called\n");

  ASSERT(DeviceObject);
  ASSERT(Sector);

  KeInitializeEvent(&Event,
		    NotificationEvent,
		    FALSE);

  /* Read the sector */
  Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
				     DeviceObject,
				     Sector,
				     SectorSize,
				     SectorOffset,
				     &Event,
				     &StatusBlock);

  Status = IoCallDriver(DeviceObject,
			Irp);
  if (Status == STATUS_PENDING)
    {
      KeWaitForSingleObject(&Event,
			    Executive,
			    KernelMode,
			    FALSE,
			    NULL);
      Status = StatusBlock.Status;
    }

  if (!NT_SUCCESS(Status))
    {
      DPRINT("Reading sector failed (Status 0x%08lx)\n",
	     Status);
      return Status;
    }

  return Status;
}


static NTSTATUS
xHalpWriteSector (IN PDEVICE_OBJECT DeviceObject,
		  IN ULONG SectorSize,
		  IN PLARGE_INTEGER SectorOffset,
		  IN PVOID Sector)
{
  IO_STATUS_BLOCK StatusBlock;
  KEVENT Event;
  PIRP Irp;
  NTSTATUS Status;

  DPRINT("xHalpWriteSector() called\n");

  KeInitializeEvent(&Event,
		    NotificationEvent,
		    FALSE);

  /* Write the sector */
  Irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE,
				     DeviceObject,
				     Sector,
				     SectorSize,
				     SectorOffset,
				     &Event,
				     &StatusBlock);

  Status = IoCallDriver(DeviceObject,
			Irp);
  if (Status == STATUS_PENDING)
    {
      KeWaitForSingleObject(&Event,
			    Executive,
			    KernelMode,
			    FALSE,
			    NULL);
      Status = StatusBlock.Status;
    }

  if (!NT_SUCCESS(Status))
    {
      DPRINT("Writing sector failed (Status 0x%08lx)\n",
	     Status);
    }

  return Status;
}


VOID FASTCALL
HalExamineMBR(IN PDEVICE_OBJECT DeviceObject,
	      IN ULONG SectorSize,
	      IN ULONG MBRTypeIdentifier,
	      OUT PVOID *Buffer)
{
  LARGE_INTEGER SectorOffset;
  PPARTITION_SECTOR Sector;
  NTSTATUS Status;

  DPRINT("HalExamineMBR()\n");

  *Buffer = NULL;

  if (SectorSize < 512)
    SectorSize = 512;
  if (SectorSize > 4096)
    SectorSize = 4096;

  Sector = (PPARTITION_SECTOR) ExAllocatePool (PagedPool,
					       SectorSize);
  if (Sector == NULL)
    {
      DPRINT ("Partition sector allocation failed\n");
      return;
    }

#if defined(__GNUC__)
  SectorOffset.QuadPart = 0LL;
#else
  SectorOffset.QuadPart = 0;
#endif
  Status = xHalpReadSector (DeviceObject,
			    SectorSize,
			    &SectorOffset,
			    (PVOID)Sector);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("xHalpReadSector() failed (Status %lx)\n", Status);
      ExFreePool(Sector);
      return;
    }

  if (Sector->Magic != PARTITION_MAGIC)
    {
      DPRINT("Invalid MBR magic value\n");
      ExFreePool(Sector);
      return;
    }

  if (Sector->Partition[0].PartitionType != MBRTypeIdentifier)
    {
      DPRINT("Invalid MBRTypeIdentifier\n");
      ExFreePool(Sector);
      return;
    }

  if (Sector->Partition[0].PartitionType == 0x54)
    {
      /* Found 'Ontrack Disk Manager'. Shift all sectors by 63 */
      DPRINT("Found 'Ontrack Disk Manager'!\n");
      *((PULONG)Sector) = 63;
    }

  *Buffer = (PVOID)Sector;
}


static BOOLEAN
HalpAssignDrive(IN PUNICODE_STRING PartitionName,
		IN ULONG DriveNumber,
		IN UCHAR DriveType,
                IN ULONG Signature,
                IN LARGE_INTEGER StartingOffset,
                IN HANDLE hKey)
{
  WCHAR DriveNameBuffer[16];
  UNICODE_STRING DriveName;
  ULONG i;
  NTSTATUS Status;
  REG_DISK_MOUNT_INFO DiskMountInfo;

  DPRINT("HalpAssignDrive()\n");

  if ((DriveNumber != AUTO_DRIVE) && (DriveNumber < 26))
    {
      /* Force assignment */
      if ((ObSystemDeviceMap->DriveMap & (1 << DriveNumber)) != 0)
	{
	  DbgPrint("Drive letter already used!\n");
	  return FALSE;
	}
    }
  else
    {
      /* Automatic assignment */
      DriveNumber = AUTO_DRIVE;

      for (i = 2; i < 26; i++)
	{
	  if ((ObSystemDeviceMap->DriveMap & (1 << i)) == 0)
	    {
	      DriveNumber = i;
	      break;
	    }
	}

      if (DriveNumber == AUTO_DRIVE)
	{
	  DbgPrint("No drive letter available!\n");
	  return FALSE;
	}
    }

  DPRINT("DriveNumber %d\n", DriveNumber);

  /* Update the System Device Map */
  ObSystemDeviceMap->DriveMap |= (1 << DriveNumber);
  ObSystemDeviceMap->DriveType[DriveNumber] = DriveType;

  /* Build drive name */
  swprintf(DriveNameBuffer,
	   L"\\??\\%C:",
	   'A' + DriveNumber);
  RtlInitUnicodeString(&DriveName,
		       DriveNameBuffer);

  DPRINT("  %wZ ==> %wZ\n",
	 &DriveName,
	 PartitionName);

  /* Create symbolic link */
  Status = IoCreateSymbolicLink(&DriveName,
		                PartitionName);

  if (hKey &&
      DriveType == DOSDEVICE_DRIVE_FIXED && 
      Signature)
    {
      DiskMountInfo.Signature = Signature;
      DiskMountInfo.StartingOffset = StartingOffset;
      swprintf(DriveNameBuffer, DiskMountString, L'A' + DriveNumber);
      RtlInitUnicodeString(&DriveName, DriveNameBuffer);

      Status = ZwSetValueKey(hKey,
                             &DriveName,
                             0,
                             REG_BINARY,
                             &DiskMountInfo,
                             sizeof(DiskMountInfo));
      if (!NT_SUCCESS(Status))
        {
          DPRINT1("ZwCreateValueKey failed for %wZ, status=%x\n", &DriveName, Status);
        }
    }
  return TRUE;
}

ULONG
xHalpGetRDiskCount(VOID)
{
  NTSTATUS Status;
  UNICODE_STRING ArcName;
  PWCHAR ArcNameBuffer;
  OBJECT_ATTRIBUTES ObjectAttributes;
  HANDLE DirectoryHandle;
  POBJECT_DIRECTORY_INFORMATION DirectoryInfo;
  ULONG Skip;
  ULONG ResultLength;
  ULONG CurrentRDisk;
  ULONG RDiskCount;
  BOOLEAN First = TRUE;
  ULONG Count;
  
  DirectoryInfo = ExAllocatePool(PagedPool, 2 * PAGE_SIZE);
  if (DirectoryInfo == NULL)
    {
      return 0;
    }

  RtlInitUnicodeString(&ArcName, L"\\ArcName");
  InitializeObjectAttributes(&ObjectAttributes,
			     &ArcName,
			     0,
			     NULL,
			     NULL);
   
  Status = ZwOpenDirectoryObject (&DirectoryHandle,
		                  SYMBOLIC_LINK_ALL_ACCESS,
		                  &ObjectAttributes);
   if (!NT_SUCCESS(Status))
     {
       DPRINT1("ZwOpenDirectoryObject for %wZ failed, status=%lx\n", &ArcName, Status);
       ExFreePool(DirectoryInfo);
       return 0;
     }

   RDiskCount = 0;
   Skip = 0;
   while (NT_SUCCESS(Status))
     {
       Status = NtQueryDirectoryObject (DirectoryHandle,
			                DirectoryInfo,
			                2 * PAGE_SIZE,
			                FALSE,
                                        First,
			                &Skip,
			                &ResultLength);
       First = FALSE;
       if (NT_SUCCESS(Status))
         {
           Count = 0;
           while (DirectoryInfo[Count].ObjectName.Buffer)
             {
               DPRINT("Count %x\n", Count);
               DirectoryInfo[Count].ObjectName.Buffer[DirectoryInfo[Count].ObjectName.Length / sizeof(WCHAR)] = 0;
               ArcNameBuffer = DirectoryInfo[Count].ObjectName.Buffer;
               if (DirectoryInfo[Count].ObjectName.Length >= sizeof(L"multi(0)disk(0)rdisk(0)") - sizeof(WCHAR) && 
                   !_wcsnicmp(ArcNameBuffer, L"multi(0)disk(0)rdisk(", (sizeof(L"multi(0)disk(0)rdisk(") - sizeof(WCHAR)) / sizeof(WCHAR)))
                 {
                   DPRINT("%S\n", ArcNameBuffer);
                   ArcNameBuffer += (sizeof(L"multi(0)disk(0)rdisk(") - sizeof(WCHAR)) / sizeof(WCHAR);
                   CurrentRDisk = 0;
                   while (iswdigit(*ArcNameBuffer))
                     {
                       CurrentRDisk = CurrentRDisk * 10 + *ArcNameBuffer - L'0';
                       ArcNameBuffer++;
                     }
                   if (!_wcsicmp(ArcNameBuffer, L")") &&
                       CurrentRDisk >= RDiskCount)
                     {
                       RDiskCount = CurrentRDisk + 1;
                     }
                 }
               Count++;
             }
         }
     }
   ExFreePool(DirectoryInfo);
   return RDiskCount;
}
  
NTSTATUS
xHalpGetDiskNumberFromRDisk(ULONG RDisk, PULONG DiskNumber)
{
  WCHAR NameBuffer[80];
  UNICODE_STRING ArcName;
  UNICODE_STRING LinkName;
  OBJECT_ATTRIBUTES ObjectAttributes;
  HANDLE LinkHandle;
  NTSTATUS Status;

  swprintf(NameBuffer,
	   L"\\ArcName\\multi(0)disk(0)rdisk(%lu)",
	   RDisk);

  RtlInitUnicodeString(&ArcName, NameBuffer);
  InitializeObjectAttributes(&ObjectAttributes,
			     &ArcName,
			     0,
			     NULL,
			     NULL);
  Status = ZwOpenSymbolicLinkObject(&LinkHandle,
                                    SYMBOLIC_LINK_ALL_ACCESS,
				    &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ZwOpenSymbolicLinkObject failed for %wZ, status=%lx\n", &ArcName, Status);
      return Status;
    }
  
  LinkName.Buffer = NameBuffer;
  LinkName.Length = 0;
  LinkName.MaximumLength = sizeof(NameBuffer);
  Status = ZwQuerySymbolicLinkObject(LinkHandle,
                                     &LinkName,
                                     NULL);
  ZwClose(LinkHandle);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ZwQuerySymbolicLinkObject failed, status=%lx\n", Status);
      return Status;
    }
  if (LinkName.Length < sizeof(L"\\Device\\Harddisk0\\Partition0") - sizeof(WCHAR) ||
      LinkName.Length >= sizeof(NameBuffer))
    {
      return STATUS_UNSUCCESSFUL;
    }

  NameBuffer[LinkName.Length / sizeof(WCHAR)] = 0;
  if (_wcsnicmp(NameBuffer, L"\\Device\\Harddisk", (sizeof(L"\\Device\\Harddisk") - sizeof(WCHAR)) / sizeof(WCHAR)))
    {
      return STATUS_UNSUCCESSFUL;
    }
  LinkName.Buffer += (sizeof(L"\\Device\\Harddisk") - sizeof(WCHAR)) / sizeof(WCHAR);

  if (!iswdigit(*LinkName.Buffer))
    {
      return STATUS_UNSUCCESSFUL;
    }
  *DiskNumber = 0;
  while (iswdigit(*LinkName.Buffer))
    {
      *DiskNumber = *DiskNumber * 10 + *LinkName.Buffer - L'0';
      LinkName.Buffer++;
    }
  if (_wcsicmp(LinkName.Buffer, L"\\Partition0"))
    {
      return STATUS_UNSUCCESSFUL;
    }
  return STATUS_SUCCESS;
}


VOID FASTCALL
xHalIoAssignDriveLetters(IN PROS_LOADER_PARAMETER_BLOCK LoaderBlock,
			 IN PSTRING NtDeviceName,
			 OUT PUCHAR NtSystemPath,
			 OUT PSTRING NtSystemPathString)
{
  PDRIVE_LAYOUT_INFORMATION *LayoutArray;
  PCONFIGURATION_INFORMATION ConfigInfo;
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK StatusBlock;
  UNICODE_STRING UnicodeString1;
  UNICODE_STRING UnicodeString2;
  HANDLE FileHandle;
  PWSTR Buffer1;
  PWSTR Buffer2;
  ULONG i, j, k;
  ULONG DiskNumber;
  ULONG RDisk;
  NTSTATUS Status;
  HANDLE hKey;
  ULONG Length;
  PKEY_VALUE_PARTIAL_INFORMATION PartialInformation;
  PREG_DISK_MOUNT_INFO DiskMountInfo;
  ULONG RDiskCount;

  DPRINT("xHalIoAssignDriveLetters()\n");

  ConfigInfo = IoGetConfigurationInformation();

  RDiskCount = xHalpGetRDiskCount();

  DPRINT1("RDiskCount %d\n", RDiskCount);

  Buffer1 = (PWSTR)ExAllocatePool(PagedPool,
				  64 * sizeof(WCHAR));
  Buffer2 = (PWSTR)ExAllocatePool(PagedPool,
				  32 * sizeof(WCHAR));
				  
  PartialInformation = (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePool(PagedPool,
                                                                      sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(REG_DISK_MOUNT_INFO));

  DiskMountInfo = (PREG_DISK_MOUNT_INFO) PartialInformation->Data;
  
  /* Open or Create the 'MountedDevices' key */
  RtlInitUnicodeString(&UnicodeString1, L"\\Registry\\Machine\\SYSTEM\\MountedDevices");
  InitializeObjectAttributes(&ObjectAttributes,
                             &UnicodeString1,
                             OBJ_CASE_INSENSITIVE,
                             NULL,
                             NULL);
  Status = ZwOpenKey(&hKey,
                     KEY_ALL_ACCESS,
                     &ObjectAttributes);
  if (!NT_SUCCESS(Status)) 
    {
      Status = ZwCreateKey(&hKey, 
                           KEY_ALL_ACCESS,
                           &ObjectAttributes,
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           NULL);
    }
  if (!NT_SUCCESS(Status))
    {
      hKey = NULL;
      DPRINT1("ZwCreateKey failed for %wZ, status=%x\n", &UnicodeString1, Status);
    }

  /* Create PhysicalDrive links */
  DPRINT("Physical disk drives: %d\n", ConfigInfo->DiskCount);
  for (i = 0; i < ConfigInfo->DiskCount; i++)
    {
      swprintf(Buffer1,
	       L"\\Device\\Harddisk%d\\Partition0",
	        i);
      RtlInitUnicodeString(&UnicodeString1,
			   Buffer1);

      InitializeObjectAttributes(&ObjectAttributes,
				 &UnicodeString1,
				 0,
				 NULL,
				 NULL);

      Status = ZwOpenFile(&FileHandle,
			  0x10001,
			  &ObjectAttributes,
			  &StatusBlock,
			  1,
			  FILE_SYNCHRONOUS_IO_NONALERT);
      if (NT_SUCCESS(Status))
	{
	  ZwClose(FileHandle);

	  swprintf(Buffer2,
		   L"\\??\\PhysicalDrive%d",
		   i);
	  RtlInitUnicodeString(&UnicodeString2,
			       Buffer2);

	  DPRINT("Creating link: %S ==> %S\n",
		 Buffer2,
		 Buffer1);

	  IoCreateSymbolicLink(&UnicodeString2,
			       &UnicodeString1);
	}
    }

  /* Initialize layout array */
  LayoutArray = ExAllocatePool(NonPagedPool,
			       ConfigInfo->DiskCount * sizeof(PDRIVE_LAYOUT_INFORMATION));
  RtlZeroMemory(LayoutArray,
		ConfigInfo->DiskCount * sizeof(PDRIVE_LAYOUT_INFORMATION));
  for (i = 0; i < ConfigInfo->DiskCount; i++)
    {
      swprintf(Buffer1,
	       L"\\Device\\Harddisk%d\\Partition0",
	       i);
      RtlInitUnicodeString(&UnicodeString1,
			   Buffer1);

      Status = xHalQueryDriveLayout(&UnicodeString1,
				    &LayoutArray[i]);
      if (!NT_SUCCESS(Status))
	{
	  DbgPrint("xHalQueryDriveLayout() failed (Status = 0x%lx)\n",
		   Status);
	  LayoutArray[i] = NULL;
	  continue;
	}
      /* We don't use the RewritePartition value while mounting the disks. 
       * We use this value for marking pre-assigned (registry) partitions.
       */
      for (j = 0; j < LayoutArray[i]->PartitionCount; j++)
        {
          LayoutArray[i]->PartitionEntry[j].RewritePartition = FALSE;
        }
    }

#ifndef NDEBUG
  /* Dump layout array */
  for (i = 0; i < ConfigInfo->DiskCount; i++)
    {
      DPRINT("Harddisk %d:\n",
	     i);

      if (LayoutArray[i] == NULL)
	continue;

      DPRINT("Logical partitions: %d\n",
	     LayoutArray[i]->PartitionCount);

      for (j = 0; j < LayoutArray[i]->PartitionCount; j++)
	{
	  DPRINT("  %d: nr:%x boot:%x type:%x startblock:%I64u count:%I64u\n",
		 j,
		 LayoutArray[i]->PartitionEntry[j].PartitionNumber,
		 LayoutArray[i]->PartitionEntry[j].BootIndicator,
		 LayoutArray[i]->PartitionEntry[j].PartitionType,
		 LayoutArray[i]->PartitionEntry[j].StartingOffset.QuadPart,
		 LayoutArray[i]->PartitionEntry[j].PartitionLength.QuadPart);
	}
    }
#endif

  /* Assign pre-assigned (registry) partitions */
  if (hKey)
    {
      for (k = 2; k < 26; k++)
        {
          swprintf(Buffer1, DiskMountString, L'A' + k);
          RtlInitUnicodeString(&UnicodeString1, Buffer1);
          Status = ZwQueryValueKey(hKey,
                                   &UnicodeString1,
                                   KeyValuePartialInformation,
                                   PartialInformation,
                                   sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(REG_DISK_MOUNT_INFO),
                                   &Length);
          if (NT_SUCCESS(Status) && 
              PartialInformation->Type == REG_BINARY &&
              PartialInformation->DataLength == sizeof(REG_DISK_MOUNT_INFO))
            {
              DPRINT("%wZ => %08x:%08x%08x\n", &UnicodeString1, DiskMountInfo->Signature, 
                     DiskMountInfo->StartingOffset.u.HighPart, DiskMountInfo->StartingOffset.u.LowPart);
                {
                  BOOLEAN Found = FALSE;
                  for (i = 0; i < ConfigInfo->DiskCount; i++)
                    {
                      DPRINT("%x\n", LayoutArray[i]->Signature);
                      if (LayoutArray[i] &&
                          LayoutArray[i]->Signature &&
                          LayoutArray[i]->Signature == DiskMountInfo->Signature)
                        {
                          for (j = 0; j < LayoutArray[i]->PartitionCount; j++)
                            {
                              if (LayoutArray[i]->PartitionEntry[j].StartingOffset.QuadPart == DiskMountInfo->StartingOffset.QuadPart)
                                {
                                  if (IsRecognizedPartition(LayoutArray[i]->PartitionEntry[j].PartitionType) &&
                                      LayoutArray[i]->PartitionEntry[j].RewritePartition == FALSE)
                                    {
                    	              swprintf(Buffer2,
		                               L"\\Device\\Harddisk%d\\Partition%d",
                                               i,
		                               LayoutArray[i]->PartitionEntry[j].PartitionNumber);
	                              RtlInitUnicodeString(&UnicodeString2,
				                           Buffer2);

	                              /* Assign drive */
	                              DPRINT("  %wZ\n", &UnicodeString2);
	                              Found = HalpAssignDrive(&UnicodeString2,
			                                      k,
			                                      DOSDEVICE_DRIVE_FIXED,
                                                              DiskMountInfo->Signature,
                                                              DiskMountInfo->StartingOffset,
                                                              NULL);
                                      /* Mark the partition as assigned */
                                      LayoutArray[i]->PartitionEntry[j].RewritePartition = TRUE;
                                    }
                                  break;
                                }
                            }
                        }
                    }
                  if (Found == FALSE)
                    {
                      /* We didn't find a partition for this entry, remove them. */
                      Status = ZwDeleteValueKey(hKey, &UnicodeString1);
                    }
                }
            }
        }
    }

  /* Assign bootable partition on first harddisk */
  DPRINT("Assigning bootable primary partition on first harddisk:\n");
  if (RDiskCount > 0)
    {
      Status = xHalpGetDiskNumberFromRDisk(0, &DiskNumber);
      if (NT_SUCCESS(Status) &&
          DiskNumber < ConfigInfo->DiskCount &&
          LayoutArray[DiskNumber])
        {
          /* Search for bootable partition */
          for (j = 0; j < PARTITION_TBL_SIZE && j < LayoutArray[DiskNumber]->PartitionCount; j++)
	    {
	      if ((LayoutArray[DiskNumber]->PartitionEntry[j].BootIndicator == TRUE) &&
	          IsRecognizedPartition(LayoutArray[DiskNumber]->PartitionEntry[j].PartitionType))
                {
                  if (LayoutArray[DiskNumber]->PartitionEntry[j].RewritePartition == FALSE)
                    {
	              swprintf(Buffer2,
		               L"\\Device\\Harddisk%lu\\Partition%d",
                               DiskNumber,
		               LayoutArray[DiskNumber]->PartitionEntry[j].PartitionNumber);
	              RtlInitUnicodeString(&UnicodeString2,
				           Buffer2);

                      /* Assign drive */
                      DPRINT("  %wZ\n", &UnicodeString2);
	              HalpAssignDrive(&UnicodeString2,
			              AUTO_DRIVE,
			              DOSDEVICE_DRIVE_FIXED,
                                      LayoutArray[DiskNumber]->Signature,
                                      LayoutArray[DiskNumber]->PartitionEntry[j].StartingOffset,
                                      hKey);
                      /* Mark the partition as assigned */
                      LayoutArray[DiskNumber]->PartitionEntry[j].RewritePartition = TRUE;
                    }
                  break;
		}
	    }
	}
    }

  /* Assign remaining primary partitions */
  DPRINT("Assigning remaining primary partitions:\n");
  for (RDisk = 0; RDisk < RDiskCount; RDisk++)
    {
      Status = xHalpGetDiskNumberFromRDisk(RDisk, &DiskNumber);
      if (NT_SUCCESS(Status) &&
          DiskNumber < ConfigInfo->DiskCount &&
          LayoutArray[DiskNumber])
        {
          /* Search for primary partitions */
          for (j = 0; (j < PARTITION_TBL_SIZE) && (j < LayoutArray[DiskNumber]->PartitionCount); j++)
	    {
	      if (LayoutArray[DiskNumber]->PartitionEntry[j].RewritePartition == FALSE &&
                  IsRecognizedPartition(LayoutArray[DiskNumber]->PartitionEntry[j].PartitionType))
	        {
                  swprintf(Buffer2,
		           L"\\Device\\Harddisk%d\\Partition%d",
		           DiskNumber,
		           LayoutArray[DiskNumber]->PartitionEntry[j].PartitionNumber);
	          RtlInitUnicodeString(&UnicodeString2,
				       Buffer2);

	          /* Assign drive */
	          DPRINT("  %wZ\n",
		         &UnicodeString2);
	          HalpAssignDrive(&UnicodeString2,
			          AUTO_DRIVE,
			          DOSDEVICE_DRIVE_FIXED,
                                  LayoutArray[DiskNumber]->Signature,
                                  LayoutArray[DiskNumber]->PartitionEntry[j].StartingOffset,
                                  hKey);
                  /* Mark the partition as assigned */
                  LayoutArray[DiskNumber]->PartitionEntry[j].RewritePartition = TRUE;
		}
	    }
	}
    }

  /* Assign extended (logical) partitions */
  DPRINT("Assigning extended (logical) partitions:\n");
  for (RDisk = 0; RDisk < RDiskCount; RDisk++)
    {
      Status = xHalpGetDiskNumberFromRDisk(RDisk, &DiskNumber);
      if (NT_SUCCESS(Status) &&
          DiskNumber < ConfigInfo->DiskCount &&
          LayoutArray[DiskNumber])
	{
	  /* Search for extended partitions */
	  for (j = PARTITION_TBL_SIZE; j < LayoutArray[DiskNumber]->PartitionCount; j++)
	    {
	      if (IsRecognizedPartition(LayoutArray[DiskNumber]->PartitionEntry[j].PartitionType) &&
                  LayoutArray[DiskNumber]->PartitionEntry[j].RewritePartition == FALSE &&
		  LayoutArray[DiskNumber]->PartitionEntry[j].PartitionNumber != 0)
		{
                  swprintf(Buffer2,
			   L"\\Device\\Harddisk%d\\Partition%d",
			   DiskNumber,
			   LayoutArray[DiskNumber]->PartitionEntry[j].PartitionNumber);
		  RtlInitUnicodeString(&UnicodeString2,
				       Buffer2);

		  /* Assign drive */
		  DPRINT("  %wZ\n",
			 &UnicodeString2);
		  HalpAssignDrive(&UnicodeString2,
				  AUTO_DRIVE,
				  DOSDEVICE_DRIVE_FIXED,
                                  LayoutArray[DiskNumber]->Signature,
                                  LayoutArray[DiskNumber]->PartitionEntry[j].StartingOffset,
                                  hKey);
                  /* Mark the partition as assigned */
                  LayoutArray[DiskNumber]->PartitionEntry[j].RewritePartition = TRUE;
		}
	    }
	}
    }

  /* Assign remaining primary partitions without an arc-name */
  DPRINT("Assigning remaining primary partitions:\n");
  for (DiskNumber = 0; DiskNumber < ConfigInfo->DiskCount; DiskNumber++)
    {
      if (LayoutArray[DiskNumber])
        {
          /* Search for primary partitions */
          for (j = 0; (j < PARTITION_TBL_SIZE) && (j < LayoutArray[DiskNumber]->PartitionCount); j++)
	    {
	      if (LayoutArray[DiskNumber]->PartitionEntry[j].RewritePartition == FALSE &&
                  IsRecognizedPartition(LayoutArray[DiskNumber]->PartitionEntry[j].PartitionType))
	        {
                  swprintf(Buffer2,
		           L"\\Device\\Harddisk%d\\Partition%d",
		           DiskNumber,
		           LayoutArray[DiskNumber]->PartitionEntry[j].PartitionNumber);
	          RtlInitUnicodeString(&UnicodeString2,
				       Buffer2);

	          /* Assign drive */
	          DPRINT("  %wZ\n",
		         &UnicodeString2);
	          HalpAssignDrive(&UnicodeString2,
			          AUTO_DRIVE,
			          DOSDEVICE_DRIVE_FIXED,
                                  LayoutArray[DiskNumber]->Signature,
                                  LayoutArray[DiskNumber]->PartitionEntry[j].StartingOffset,
                                  hKey);
                  /* Mark the partition as assigned */
                  LayoutArray[DiskNumber]->PartitionEntry[j].RewritePartition = TRUE;
		}
	    }
	}
    }

  /* Assign extended (logical) partitions without an arc-name */
  DPRINT("Assigning extended (logical) partitions:\n");
  for (DiskNumber = 0; DiskNumber < ConfigInfo->DiskCount; DiskNumber++)
    {
      if (LayoutArray[DiskNumber])
	{
	  /* Search for extended partitions */
	  for (j = PARTITION_TBL_SIZE; j < LayoutArray[DiskNumber]->PartitionCount; j++)
	    {
	      if (IsRecognizedPartition(LayoutArray[DiskNumber]->PartitionEntry[j].PartitionType) &&
                  LayoutArray[DiskNumber]->PartitionEntry[j].RewritePartition == FALSE &&
		  LayoutArray[DiskNumber]->PartitionEntry[j].PartitionNumber != 0)
		{
                  swprintf(Buffer2,
			   L"\\Device\\Harddisk%d\\Partition%d",
			   DiskNumber,
			   LayoutArray[DiskNumber]->PartitionEntry[j].PartitionNumber);
		  RtlInitUnicodeString(&UnicodeString2,
				       Buffer2);

		  /* Assign drive */
		  DPRINT("  %wZ\n",
			 &UnicodeString2);
		  HalpAssignDrive(&UnicodeString2,
				  AUTO_DRIVE,
				  DOSDEVICE_DRIVE_FIXED,
                                  LayoutArray[DiskNumber]->Signature,
                                  LayoutArray[DiskNumber]->PartitionEntry[j].StartingOffset,
                                  hKey);
                  /* Mark the partition as assigned */
                  LayoutArray[DiskNumber]->PartitionEntry[j].RewritePartition = TRUE;
		}
	    }
	}
    }

  /* Assign removable disk drives */
  DPRINT("Assigning removable disk drives:\n");
  for (i = 0; i < ConfigInfo->DiskCount; i++)
    {
      if (LayoutArray[i])
        {
          /* Search for virtual partitions */
          if (LayoutArray[i]->PartitionCount == 1 &&
	      LayoutArray[i]->PartitionEntry[0].PartitionType == 0)
	    {
	      swprintf(Buffer2,
		       L"\\Device\\Harddisk%d\\Partition1",
		       i);
	      RtlInitUnicodeString(&UnicodeString2,
			           Buffer2);

	      /* Assign drive */
	      DPRINT("  %wZ\n",
		     &UnicodeString2);
	      HalpAssignDrive(&UnicodeString2,
			      AUTO_DRIVE,
			      DOSDEVICE_DRIVE_REMOVABLE,
                              0,
                              RtlConvertLongToLargeInteger(0),
                              hKey);
	    }
	}
    }

  /* Free layout array */
  for (i = 0; i < ConfigInfo->DiskCount; i++)
    {
      if (LayoutArray[i] != NULL)
	ExFreePool(LayoutArray[i]);
    }
  ExFreePool(LayoutArray);

  /* Assign floppy drives */
  DPRINT("Floppy drives: %d\n", ConfigInfo->FloppyCount);
  for (i = 0; i < ConfigInfo->FloppyCount; i++)
    {
      swprintf(Buffer1,
	       L"\\Device\\Floppy%d",
	       i);
      RtlInitUnicodeString(&UnicodeString1,
			   Buffer1);

      /* Assign drive letters A: or B: or first free drive letter */
      DPRINT("  %wZ\n",
	     &UnicodeString1);
      HalpAssignDrive(&UnicodeString1,
		      (i < 2) ? i : AUTO_DRIVE,
		      DOSDEVICE_DRIVE_REMOVABLE,
                      0,
                      RtlConvertLongToLargeInteger(0),
                      hKey);
    }

  /* Assign cdrom drives */
  DPRINT("CD-Rom drives: %d\n", ConfigInfo->CdRomCount);
  for (i = 0; i < ConfigInfo->CdRomCount; i++)
    {
      swprintf(Buffer1,
	       L"\\Device\\CdRom%d",
	       i);
      RtlInitUnicodeString(&UnicodeString1,
			   Buffer1);

      /* Assign first free drive letter */
      DPRINT("  %wZ\n", &UnicodeString1);
      HalpAssignDrive(&UnicodeString1,
		      AUTO_DRIVE,
		      DOSDEVICE_DRIVE_CDROM,
                      0,
                      RtlConvertLongToLargeInteger(0),
                      hKey);
    }

  /* Anything else to do? */

  ExFreePool(PartialInformation);
  ExFreePool(Buffer2);
  ExFreePool(Buffer1);
  if (hKey)
    {
      ZwClose(hKey);
    }
}


NTSTATUS FASTCALL
xHalIoReadPartitionTable(PDEVICE_OBJECT DeviceObject,
			 ULONG SectorSize,
			 BOOLEAN ReturnRecognizedPartitions,
			 PDRIVE_LAYOUT_INFORMATION *PartitionBuffer)
{
  LARGE_INTEGER RealPartitionOffset;
  ULONGLONG PartitionOffset;
#if defined(__GNUC__)
  ULONGLONG nextPartitionOffset = 0LL;
#else
  ULONGLONG nextPartitionOffset = 0;
#endif
  ULONGLONG containerOffset;
  NTSTATUS Status;
  PPARTITION_SECTOR PartitionSector;
  PDRIVE_LAYOUT_INFORMATION LayoutBuffer;
  ULONG i;
  ULONG Count = 0;
  ULONG Number = 1;
  BOOLEAN ExtendedFound = FALSE;
  PVOID MbrBuffer;
  DISK_MANAGER DiskManager = NoDiskManager;

  DPRINT("xHalIoReadPartitionTable(%p %lu %x %p)\n",
	 DeviceObject,
	 SectorSize,
	 ReturnRecognizedPartitions,
	 PartitionBuffer);

  *PartitionBuffer = NULL;

  /* Check sector size */
  if (SectorSize < 512)
    SectorSize = 512;
  if (SectorSize > 4096)
    SectorSize = 4096;

  /* Check for 'Ontrack Disk Manager' */
  HalExamineMBR(DeviceObject,
		SectorSize,
		0x54,
		&MbrBuffer);
  if (MbrBuffer != NULL)
    {
      DPRINT("Found 'Ontrack Disk Manager'\n");
      DiskManager = OntrackDiskManager;
      ExFreePool(MbrBuffer);
    }

  /* Check for 'EZ-Drive' */
  HalExamineMBR(DeviceObject,
		SectorSize,
		0x55,
		&MbrBuffer);
  if (MbrBuffer != NULL)
    {
      DPRINT("Found 'EZ-Drive'\n");
      DiskManager = EZ_Drive;
      ExFreePool(MbrBuffer);
    }

  PartitionSector = (PPARTITION_SECTOR)ExAllocatePool(PagedPool,
						      SectorSize);
  if (PartitionSector == NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  LayoutBuffer = (PDRIVE_LAYOUT_INFORMATION)ExAllocatePool(NonPagedPool,
							   0x1000);
  if (LayoutBuffer == NULL)
    {
      ExFreePool(PartitionSector);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  RtlZeroMemory(LayoutBuffer,
		0x1000);

#if defined(__GNUC__)
  PartitionOffset = 0ULL;
  containerOffset = 0ULL;
#else
  PartitionOffset = 0;
  containerOffset = 0;
#endif

  do
    {
      DPRINT("PartitionOffset: %I64u\n", PartitionOffset / SectorSize);

      /* Handle disk managers */
      if (DiskManager == OntrackDiskManager)
	{
	  /* Shift offset by 63 sectors */
	  RealPartitionOffset.QuadPart = PartitionOffset + (ULONGLONG)(63 * SectorSize);
	}
#if defined(__GNUC__)
      else if (DiskManager == EZ_Drive && PartitionOffset == 0ULL)
#else
      else if (DiskManager == EZ_Drive && PartitionOffset == 0)
#endif
	{
	  /* Use sector 1 instead of sector 0 */
	  RealPartitionOffset.QuadPart = (ULONGLONG)SectorSize;
	}
      else
	{
	  RealPartitionOffset.QuadPart = PartitionOffset;
	}

      DPRINT ("RealPartitionOffset: %I64u\n",
	      RealPartitionOffset.QuadPart / SectorSize);

      Status = xHalpReadSector (DeviceObject,
				SectorSize,
				&RealPartitionOffset,
				PartitionSector);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT ("Failed to read partition table sector (Status = 0x%08lx)\n",
		  Status);
	  ExFreePool (PartitionSector);
	  ExFreePool (LayoutBuffer);
	  return Status;
	}

      /* Check the boot sector id */
      DPRINT("Magic %x\n", PartitionSector->Magic);
      if (PartitionSector->Magic != PARTITION_MAGIC)
	{
	  DPRINT ("Invalid partition sector magic\n");
	  ExFreePool (PartitionSector);
	  *PartitionBuffer = LayoutBuffer;
	  return STATUS_SUCCESS;
	}

#ifndef NDEBUG
      for (i = 0; i < PARTITION_TBL_SIZE; i++)
	{
	  DPRINT1("  %d: flags:%2x type:%x start:%d:%d:%d end:%d:%d:%d stblk:%d count:%d\n",
		  i,
		  PartitionSector->Partition[i].BootFlags,
		  PartitionSector->Partition[i].PartitionType,
		  PartitionSector->Partition[i].StartingHead,
		  PartitionSector->Partition[i].StartingSector & 0x3f,
		  (((PartitionSector->Partition[i].StartingSector) & 0xc0) << 2) +
		     PartitionSector->Partition[i].StartingCylinder,
		  PartitionSector->Partition[i].EndingHead,
		  PartitionSector->Partition[i].EndingSector & 0x3f,
		  (((PartitionSector->Partition[i].EndingSector) & 0xc0) << 2) +
		     PartitionSector->Partition[i].EndingCylinder,
		  PartitionSector->Partition[i].StartingBlock,
		  PartitionSector->Partition[i].SectorCount);
	}
#endif

#if defined(__GNUC__)
      if (PartitionOffset == 0ULL)
#else
      if (PartitionOffset == 0)
#endif
	{
	  LayoutBuffer->Signature = PartitionSector->Signature;
	  DPRINT("Disk signature: %lx\n", LayoutBuffer->Signature);
	}

      ExtendedFound = FALSE;

      for (i = 0; i < PARTITION_TBL_SIZE; i++)
	{
	  if (IsContainerPartition(PartitionSector->Partition[i].PartitionType))
	    {
	      ExtendedFound = TRUE;
	      if ((ULONGLONG) containerOffset == (ULONGLONG) 0)
		{
		  containerOffset = PartitionOffset;
		}
	      nextPartitionOffset = (ULONGLONG) containerOffset +
		(ULONGLONG) PartitionSector->Partition[i].StartingBlock *
		(ULONGLONG) SectorSize;
	    }

	  if ((ReturnRecognizedPartitions == FALSE) ||
	       ((ReturnRecognizedPartitions == TRUE) &&
	        !IsContainerPartition(PartitionSector->Partition[i].PartitionType) &&
                PartitionSector->Partition[i].PartitionType != PARTITION_ENTRY_UNUSED))
	    {
	      /* handle normal partition */
	      DPRINT("Partition %u: Normal Partition\n", i);
	      Count = LayoutBuffer->PartitionCount;
	      DPRINT("Logical Partition %u\n", Count);

	      if (PartitionSector->Partition[i].StartingBlock == 0)
		{
		  LayoutBuffer->PartitionEntry[Count].StartingOffset.QuadPart = 0;
		}
	      else if (IsContainerPartition(PartitionSector->Partition[i].PartitionType))
		{
		  LayoutBuffer->PartitionEntry[Count].StartingOffset.QuadPart =
		    (ULONGLONG) containerOffset +
		    (ULONGLONG) PartitionSector->Partition[i].StartingBlock *
		    (ULONGLONG) SectorSize;
		}
	      else
		{
		  LayoutBuffer->PartitionEntry[Count].StartingOffset.QuadPart =
		    (ULONGLONG)PartitionOffset +
		    ((ULONGLONG)PartitionSector->Partition[i].StartingBlock * (ULONGLONG)SectorSize);
		}
	      LayoutBuffer->PartitionEntry[Count].PartitionLength.QuadPart =
		(ULONGLONG)PartitionSector->Partition[i].SectorCount * (ULONGLONG)SectorSize;
	      LayoutBuffer->PartitionEntry[Count].HiddenSectors =
		PartitionSector->Partition[i].StartingBlock;

	      if (!IsContainerPartition(PartitionSector->Partition[i].PartitionType) &&
                  PartitionSector->Partition[i].PartitionType != PARTITION_ENTRY_UNUSED)
		{
                  LayoutBuffer->PartitionEntry[Count].RecognizedPartition = TRUE;
                  /* WinXP returns garbage as PartitionNumber */
		  LayoutBuffer->PartitionEntry[Count].PartitionNumber = Number;
		  Number++;
		}
	      else
		{
                  LayoutBuffer->PartitionEntry[Count].RecognizedPartition = FALSE;
		  LayoutBuffer->PartitionEntry[Count].PartitionNumber = 0;
		}

	      LayoutBuffer->PartitionEntry[Count].PartitionType =
		PartitionSector->Partition[i].PartitionType;
	      LayoutBuffer->PartitionEntry[Count].BootIndicator =
		(PartitionSector->Partition[i].BootFlags & 0x80)?TRUE:FALSE;
	      LayoutBuffer->PartitionEntry[Count].RewritePartition = FALSE;

	      DPRINT(" %ld: nr: %d boot: %1x type: %x start: 0x%I64x count: 0x%I64x rec: %d\n",
		     Count,
		     LayoutBuffer->PartitionEntry[Count].PartitionNumber,
		     LayoutBuffer->PartitionEntry[Count].BootIndicator,
		     LayoutBuffer->PartitionEntry[Count].PartitionType,
		     LayoutBuffer->PartitionEntry[Count].StartingOffset.QuadPart,
		     LayoutBuffer->PartitionEntry[Count].PartitionLength.QuadPart,
		     LayoutBuffer->PartitionEntry[Count].RecognizedPartition);

	      LayoutBuffer->PartitionCount++;
	    }
	}

      PartitionOffset = nextPartitionOffset;
    }
  while (ExtendedFound == TRUE);

  *PartitionBuffer = LayoutBuffer;
  ExFreePool(PartitionSector);

  return(STATUS_SUCCESS);
}


NTSTATUS FASTCALL
xHalIoSetPartitionInformation(IN PDEVICE_OBJECT DeviceObject,
			      IN ULONG SectorSize,
			      IN ULONG PartitionNumber,
			      IN ULONG PartitionType)
{
  PPARTITION_SECTOR PartitionSector;
  LARGE_INTEGER RealPartitionOffset;
  ULONGLONG PartitionOffset;
#if defined(__GNUC__)
  ULONGLONG nextPartitionOffset = 0LL;
#else
  ULONGLONG nextPartitionOffset = 0;
#endif
  ULONGLONG containerOffset;
  NTSTATUS Status;
  ULONG i;
  ULONG Number = 1;
  BOOLEAN ExtendedFound = FALSE;
  DISK_MANAGER DiskManager = NoDiskManager;

  DPRINT ("xHalIoSetPartitionInformation(%p %lu %lu %lu)\n",
	  DeviceObject,
	  SectorSize,
	  PartitionNumber,
	  PartitionType);

  /* Check sector size */
  if (SectorSize < 512)
    SectorSize = 512;
  if (SectorSize > 4096)
    SectorSize = 4096;

  /* Check for 'Ontrack Disk Manager' */
  HalExamineMBR (DeviceObject,
		 SectorSize,
		 0x54,
		 (PVOID*)(PVOID)&PartitionSector);
  if (PartitionSector != NULL)
    {
      DPRINT ("Found 'Ontrack Disk Manager'\n");
      DiskManager = OntrackDiskManager;
      ExFreePool (PartitionSector);
    }

  /* Check for 'EZ-Drive' */
  HalExamineMBR (DeviceObject,
		 SectorSize,
		 0x55,
		 (PVOID*)(PVOID)&PartitionSector);
  if (PartitionSector != NULL)
    {
      DPRINT ("Found 'EZ-Drive'\n");
      DiskManager = EZ_Drive;
      ExFreePool (PartitionSector);
    }

  /* Allocate partition sector */
  PartitionSector = (PPARTITION_SECTOR) ExAllocatePool (PagedPool,
							SectorSize);
  if (PartitionSector == NULL)
    {
      return STATUS_INSUFFICIENT_RESOURCES;
    }

#if defined(__GNUC__)
  PartitionOffset = 0ULL;
  containerOffset = 0ULL;
#else
  PartitionOffset = 0;
  containerOffset = 0;
#endif

  do
    {
      DPRINT ("PartitionOffset: %I64u\n", PartitionOffset / SectorSize);

      /* Handle disk managers */
      if (DiskManager == OntrackDiskManager)
	{
	  /* Shift offset by 63 sectors */
	  RealPartitionOffset.QuadPart = PartitionOffset + (ULONGLONG)(63 * SectorSize);
	}
#if defined(__GNUC__)
      else if (DiskManager == EZ_Drive && PartitionOffset == 0ULL)
#else
      else if (DiskManager == EZ_Drive && PartitionOffset == 0)
#endif
	{
	  /* Use sector 1 instead of sector 0 */
	  RealPartitionOffset.QuadPart = (ULONGLONG)SectorSize;
	}
      else
	{
	  RealPartitionOffset.QuadPart = PartitionOffset;
	}

      DPRINT ("RealPartitionOffset: %I64u\n",
	      RealPartitionOffset.QuadPart / SectorSize);

      Status = xHalpReadSector (DeviceObject,
				SectorSize,
				&RealPartitionOffset,
				PartitionSector);
      if (!NT_SUCCESS (Status))
	{
	  DPRINT ("Failed to read partition table sector (Status = 0x%08lx)\n",
		  Status);
	  ExFreePool (PartitionSector);
	  return Status;
	}

      /* Check the boot sector id */
      DPRINT("Magic %x\n", PartitionSector->Magic);
      if (PartitionSector->Magic != PARTITION_MAGIC)
	{
	  DPRINT ("Invalid partition sector magic\n");
	  ExFreePool (PartitionSector);
	  return STATUS_UNSUCCESSFUL;
	}

#ifndef NDEBUG
      for (i = 0; i < PARTITION_TBL_SIZE; i++)
	{
	  DPRINT1("  %d: flags:%2x type:%x start:%d:%d:%d end:%d:%d:%d stblk:%d count:%d\n",
		  i,
		  PartitionSector->Partition[i].BootFlags,
		  PartitionSector->Partition[i].PartitionType,
		  PartitionSector->Partition[i].StartingHead,
		  PartitionSector->Partition[i].StartingSector & 0x3f,
		  (((PartitionSector->Partition[i].StartingSector) & 0xc0) << 2) +
		     PartitionSector->Partition[i].StartingCylinder,
		  PartitionSector->Partition[i].EndingHead,
		  PartitionSector->Partition[i].EndingSector & 0x3f,
		  (((PartitionSector->Partition[i].EndingSector) & 0xc0) << 2) +
		     PartitionSector->Partition[i].EndingCylinder,
		  PartitionSector->Partition[i].StartingBlock,
		  PartitionSector->Partition[i].SectorCount);
	}
#endif

      ExtendedFound = FALSE;
      for (i = 0; i < PARTITION_TBL_SIZE; i++)
	{
	  if (IsContainerPartition (PartitionSector->Partition[i].PartitionType))
	    {
	      ExtendedFound = TRUE;
#if defined(__GNUC__)
	      if (containerOffset == 0ULL)
#else
	      if (containerOffset == 0)
#endif
		{
		  containerOffset = PartitionOffset;
		}
	      nextPartitionOffset = containerOffset +
		(ULONGLONG) PartitionSector->Partition[i].StartingBlock *
		(ULONGLONG) SectorSize;
	    }

	  /* Handle recognized partition */
	  if (IsRecognizedPartition (PartitionSector->Partition[i].PartitionType))
	    {
	      if (Number == PartitionNumber)
		{
		  /* Set partition type */
		  PartitionSector->Partition[i].PartitionType = PartitionType;

		  /* Write partition sector */
		  Status = xHalpWriteSector (DeviceObject,
					     SectorSize,
					     &RealPartitionOffset,
					     PartitionSector);
		  if (!NT_SUCCESS(Status))
		    {
		      DPRINT1("xHalpWriteSector() failed (Status %lx)\n", Status);
		    }

		  ExFreePool (PartitionSector);
		  return Status;
		}
	      Number++;
	    }
	}

      PartitionOffset = nextPartitionOffset;
    }
  while (ExtendedFound == TRUE);

  ExFreePool(PartitionSector);

  return STATUS_UNSUCCESSFUL;
}


NTSTATUS FASTCALL
xHalIoWritePartitionTable(IN PDEVICE_OBJECT DeviceObject,
			  IN ULONG SectorSize,
			  IN ULONG SectorsPerTrack,
			  IN ULONG NumberOfHeads,
			  IN PDRIVE_LAYOUT_INFORMATION PartitionBuffer)
{
  PPARTITION_SECTOR PartitionSector;
  LARGE_INTEGER RealPartitionOffset;
  ULONGLONG PartitionOffset;
#if defined(__GNUC__)
  ULONGLONG NextPartitionOffset = 0LL;
#else
  ULONGLONG NextPartitionOffset = 0;
#endif
  ULONGLONG ContainerOffset;
  BOOLEAN ContainerEntry;
  DISK_MANAGER DiskManager;
  ULONG i;
  ULONG j;
  ULONG StartBlock;
  ULONG SectorCount;
  ULONG StartCylinder;
  ULONG StartSector;
  ULONG StartHead;
  ULONG EndCylinder;
  ULONG EndSector;
  ULONG EndHead;
  ULONG lba;
  ULONG x;
  NTSTATUS Status;

  DPRINT ("xHalIoWritePartitionTable(%p %lu %lu %lu %p)\n",
	  DeviceObject,
	  SectorSize,
	  SectorsPerTrack,
	  NumberOfHeads,
	  PartitionBuffer);

  ASSERT(DeviceObject);
  ASSERT(PartitionBuffer);

  DiskManager = NoDiskManager;

  /* Check sector size */
  if (SectorSize < 512)
    SectorSize = 512;
  if (SectorSize > 4096)
    SectorSize = 4096;

  /* Check for 'Ontrack Disk Manager' */
  HalExamineMBR (DeviceObject,
		 SectorSize,
		 0x54,
		 (PVOID*)(PVOID)&PartitionSector);
  if (PartitionSector != NULL)
    {
      DPRINT ("Found 'Ontrack Disk Manager'\n");
      DiskManager = OntrackDiskManager;
      ExFreePool (PartitionSector);
    }

  /* Check for 'EZ-Drive' */
  HalExamineMBR (DeviceObject,
		 SectorSize,
		 0x55,
		 (PVOID*)(PVOID)&PartitionSector);
  if (PartitionSector != NULL)
    {
      DPRINT ("Found 'EZ-Drive'\n");
      DiskManager = EZ_Drive;
      ExFreePool (PartitionSector);
    }

  /* Allocate partition sector */
  PartitionSector = (PPARTITION_SECTOR)ExAllocatePool(PagedPool,
						      SectorSize);
  if (PartitionSector == NULL)
    {
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  Status = STATUS_SUCCESS;
#if defined(__GNUC__)
  PartitionOffset = 0ULL;
  ContainerOffset = 0ULL;
#else
  PartitionOffset = 0;
  ContainerOffset = 0;
#endif
  for (i = 0; i < PartitionBuffer->PartitionCount; i += 4)
    {
      DPRINT ("PartitionOffset: %I64u\n", PartitionOffset);
      DPRINT ("ContainerOffset: %I64u\n", ContainerOffset);

      /* Handle disk managers */
      if (DiskManager == OntrackDiskManager)
	{
	  /* Shift offset by 63 sectors */
	  RealPartitionOffset.QuadPart = PartitionOffset + (ULONGLONG)(63 * SectorSize);
	}
#if defined(__GNUC__)
      else if (DiskManager == EZ_Drive && PartitionOffset == 0ULL)
#else
      else if (DiskManager == EZ_Drive && PartitionOffset == 0)
#endif
	{
	  /* Use sector 1 instead of sector 0 */
	  RealPartitionOffset.QuadPart = (ULONGLONG)SectorSize;
	}
      else
	{
	  RealPartitionOffset.QuadPart = PartitionOffset;
	}

      /* Write modified partition tables */
      if (PartitionBuffer->PartitionEntry[i].RewritePartition == TRUE ||
	  PartitionBuffer->PartitionEntry[i + 1].RewritePartition == TRUE ||
	  PartitionBuffer->PartitionEntry[i + 2].RewritePartition == TRUE ||
	  PartitionBuffer->PartitionEntry[i + 3].RewritePartition == TRUE)
	{
	  /* Read partition sector */
	  Status = xHalpReadSector (DeviceObject,
				    SectorSize,
				    &RealPartitionOffset,
				    PartitionSector);
	  if (!NT_SUCCESS(Status))
	    {
	      DPRINT1 ("xHalpReadSector() failed (Status %lx)\n", Status);
	      break;
	    }

	  /* Initialize a new partition sector */
	  if (PartitionSector->Magic != PARTITION_MAGIC)
	    {
	      /* Create empty partition sector */
	      RtlZeroMemory (PartitionSector,
			     SectorSize);
	      PartitionSector->Magic = PARTITION_MAGIC;
	    }

          PartitionSector->Signature = PartitionBuffer->Signature;
	  /* Update partition sector entries */
	  for (j = 0; j < 4; j++)
	    {
	      if (PartitionBuffer->PartitionEntry[i + j].RewritePartition == TRUE)
		{
		  /* Set partition boot flag */
		  if (PartitionBuffer->PartitionEntry[i + j].BootIndicator)
		    {
		      PartitionSector->Partition[j].BootFlags |= 0x80;
		    }
		  else
		    {
		      PartitionSector->Partition[j].BootFlags &= ~0x80;
		    }

		  /* Set partition type */
		  PartitionSector->Partition[j].PartitionType =
		    PartitionBuffer->PartitionEntry[i + j].PartitionType;

		  /* Set partition data */
#if defined(__GNUC__)
		  if (PartitionBuffer->PartitionEntry[i + j].StartingOffset.QuadPart == 0ULL &&
		      PartitionBuffer->PartitionEntry[i + j].PartitionLength.QuadPart == 0ULL)
#else
		  if (PartitionBuffer->PartitionEntry[i + j].StartingOffset.QuadPart == 0 &&
		      PartitionBuffer->PartitionEntry[i + j].PartitionLength.QuadPart == 0)
#endif
		    {
		      PartitionSector->Partition[j].StartingBlock = 0;
		      PartitionSector->Partition[j].SectorCount = 0;
		      PartitionSector->Partition[j].StartingCylinder = 0;
		      PartitionSector->Partition[j].StartingHead = 0;
		      PartitionSector->Partition[j].StartingSector = 0;
		      PartitionSector->Partition[j].EndingCylinder = 0;
		      PartitionSector->Partition[j].EndingHead = 0;
		      PartitionSector->Partition[j].EndingSector = 0;
		    }
		  else
		    {
		      /*
		       * CHS formulas:
		       * x = LBA DIV SectorsPerTrack
		       * cylinder = (x DIV NumberOfHeads) % 1024
		       * head = x MOD NumberOfHeads
		       * sector = (LBA MOD SectorsPerTrack) + 1
		       */

		      /* Compute starting CHS values */
		      lba = (ULONG)((PartitionBuffer->PartitionEntry[i + j].StartingOffset.QuadPart) / SectorSize);
		      x = lba / SectorsPerTrack;
		      StartCylinder = (x / NumberOfHeads) %1024;
		      StartHead = x % NumberOfHeads;
		      StartSector = (lba % SectorsPerTrack) + 1;
		      DPRINT ("StartingOffset (LBA:%d  C:%d  H:%d  S:%d)\n",
			      lba, StartCylinder, StartHead, StartSector);

		      /* Compute ending CHS values */
		      lba = (ULONG)((PartitionBuffer->PartitionEntry[i + j].StartingOffset.QuadPart +
			     (PartitionBuffer->PartitionEntry[i + j].PartitionLength.QuadPart - 1)) / SectorSize);
		      x = lba / SectorsPerTrack;
		      EndCylinder = (x / NumberOfHeads) % 1024;
		      EndHead = x % NumberOfHeads;
		      EndSector = (lba % SectorsPerTrack) + 1;
		      DPRINT ("EndingOffset (LBA:%d  C:%d  H:%d  S:%d)\n",
			      lba, EndCylinder, EndHead, EndSector);

		      /* Set starting CHS values */
		      PartitionSector->Partition[j].StartingCylinder = StartCylinder & 0xff;
		      PartitionSector->Partition[j].StartingHead = StartHead;
		      PartitionSector->Partition[j].StartingSector =
			((StartCylinder & 0x0300) >> 2) + (StartSector & 0x3f);

		      /* Set ending CHS values */
		      PartitionSector->Partition[j].EndingCylinder = EndCylinder & 0xff;
		      PartitionSector->Partition[j].EndingHead = EndHead;
		      PartitionSector->Partition[j].EndingSector =
			((EndCylinder & 0x0300) >> 2) + (EndSector & 0x3f);

		      /* Calculate start sector and sector count */
		      if (IsContainerPartition (PartitionBuffer->PartitionEntry[i + j].PartitionType))
		        {
		          StartBlock =
			    (PartitionBuffer->PartitionEntry[i + j].StartingOffset.QuadPart - ContainerOffset) / SectorSize;
			}
		      else
		        {
		          StartBlock =
			    (PartitionBuffer->PartitionEntry[i + j].StartingOffset.QuadPart - NextPartitionOffset) / SectorSize;
		        }
		      SectorCount =
			PartitionBuffer->PartitionEntry[i + j].PartitionLength.QuadPart / SectorSize;
		      DPRINT ("LBA (StartBlock:%lu  SectorCount:%lu)\n",
			      StartBlock, SectorCount);

		      /* Set start sector and sector count */
		      PartitionSector->Partition[j].StartingBlock = StartBlock;
		      PartitionSector->Partition[j].SectorCount = SectorCount;
		    }
		}
	    }

	  /* Write partition sector */
	  Status = xHalpWriteSector (DeviceObject,
				     SectorSize,
				     &RealPartitionOffset,
				     PartitionSector);
	  if (!NT_SUCCESS(Status))
	    {
	      DPRINT1("xHalpWriteSector() failed (Status %lx)\n", Status);
	      break;
	    }
	}

      ContainerEntry = FALSE;
      for (j = 0; j < 4; j++)
	{
	  if (IsContainerPartition (PartitionBuffer->PartitionEntry[i + j].PartitionType))
	    {
	      ContainerEntry = TRUE;
	      NextPartitionOffset =
		PartitionBuffer->PartitionEntry[i + j].StartingOffset.QuadPart;

#if defined(__GNUC__)
	      if (ContainerOffset == 0ULL)
#else
	      if (ContainerOffset == 0)
#endif
		{
		  ContainerOffset = NextPartitionOffset;
		}
	    }
	}

      if (ContainerEntry == FALSE)
	{
	  DPRINT ("No container entry in partition sector!\n");
	  break;
	}

      PartitionOffset = NextPartitionOffset;
    }

  ExFreePool (PartitionSector);

  return Status;
}

/*
 * @unimplemented
 *
STDCALL
VOID
IoAssignDriveLetters(
   		IN PLOADER_PARAMETER_BLOCK   	 LoaderBlock,
		IN PSTRING  	NtDeviceName,
		OUT PUCHAR  	NtSystemPath,
		OUT PSTRING  	NtSystemPathString
    )
{
	UNIMPLEMENTED;
*/

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoCreateDisk(
    IN PDEVICE_OBJECT DeviceObject,
    IN struct _CREATE_DISK* Disk
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoGetBootDiskInformation(
    IN OUT PBOOTDISK_INFORMATION BootDiskInformation,
    IN ULONG Size
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoReadDiskSignature(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG BytesPerSector,
    OUT PDISK_SIGNATURE Signature
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoReadPartitionTableEx(
    IN PDEVICE_OBJECT DeviceObject,
    IN struct _DRIVE_LAYOUT_INFORMATION_EX** DriveLayout
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoSetPartitionInformationEx(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG PartitionNumber,
    IN struct _SET_PARTITION_INFORMATION_EX* PartitionInfo
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoSetSystemPartition(
    PUNICODE_STRING VolumeNameString
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoVerifyPartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN FixErrors
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoVolumeDeviceToDosName(
    IN  PVOID           VolumeDeviceObject,
    OUT PUNICODE_STRING DosName
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoWritePartitionTableEx(
    IN PDEVICE_OBJECT DeviceObject,
    IN struct _DRIVE_LAYOUT_INFORMATION_EX* DriveLayfout
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS FASTCALL
IoReadPartitionTable(PDEVICE_OBJECT DeviceObject,
		     ULONG SectorSize,
		     BOOLEAN ReturnRecognizedPartitions,
		     PDRIVE_LAYOUT_INFORMATION *PartitionBuffer)
{
  return(HalIoReadPartitionTable(DeviceObject,
				 SectorSize,
				 ReturnRecognizedPartitions,
				 PartitionBuffer));
}


NTSTATUS FASTCALL
IoSetPartitionInformation(PDEVICE_OBJECT DeviceObject,
			  ULONG SectorSize,
			  ULONG PartitionNumber,
			  ULONG PartitionType)
{
  return(HalIoSetPartitionInformation(DeviceObject,
				      SectorSize,
				      PartitionNumber,
				      PartitionType));
}


NTSTATUS FASTCALL
IoWritePartitionTable(PDEVICE_OBJECT DeviceObject,
		      ULONG SectorSize,
		      ULONG SectorsPerTrack,
		      ULONG NumberOfHeads,
		      PDRIVE_LAYOUT_INFORMATION PartitionBuffer)
{
  return(HalIoWritePartitionTable(DeviceObject,
				  SectorSize,
				  SectorsPerTrack,
				  NumberOfHeads,
				  PartitionBuffer));
}
/* EOF */
