/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/arcname.c
 * PURPOSE:         Creates ARC names for boot devices
 *
 * PROGRAMMERS:     Eric Kohl (ekohl@rz-online.de)
 */


/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

static NTSTATUS 
STDCALL INIT_FUNCTION 
DiskQueryRoutine(PWSTR ValueName, 
                 ULONG ValueType, 
                 PVOID ValueData, 
                 ULONG ValueLength,
                 PVOID Context,
                 PVOID EntryContext);

static VOID INIT_FUNCTION
IopEnumerateBiosDisks(PLIST_ENTRY ListHead);

static VOID INIT_FUNCTION
IopEnumerateDisks(PLIST_ENTRY ListHead);

static NTSTATUS INIT_FUNCTION
IopAssignArcNamesToDisk(PDEVICE_OBJECT DeviceObject, ULONG RDisk, ULONG DiskNumber);

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, DiskQueryRoutine)
#pragma alloc_text(INIT, IopEnumerateBiosDisks)
#pragma alloc_text(INIT, IopEnumerateDisks)
#pragma alloc_text(INIT, IopAssignArcNamesToDisk)
#pragma alloc_text(INIT, IoCreateArcNames)
#pragma alloc_text(INIT, IoCreateSystemRootLink)
#endif


/* MACROS *******************************************************************/

#define FS_VOLUME_BUFFER_SIZE (MAX_PATH + sizeof(FILE_FS_VOLUME_INFORMATION))

/* FUNCTIONS ****************************************************************/

static 
NTSTATUS
STDCALL
INIT_FUNCTION
DiskQueryRoutine(PWSTR ValueName,
                 ULONG ValueType,
                 PVOID ValueData,
                 ULONG ValueLength,
                 PVOID Context,
                 PVOID EntryContext)
{
  PLIST_ENTRY ListHead = (PLIST_ENTRY)Context;
  PULONG GlobalDiskCount = (PULONG)EntryContext;
  PDISKENTRY DiskEntry;
  UNICODE_STRING NameU;

  if (ValueType == REG_SZ &&
      ValueLength == 20 * sizeof(WCHAR))
    {
      DiskEntry = ExAllocatePool(PagedPool, sizeof(DISKENTRY));
      if (DiskEntry == NULL)
        {
          return STATUS_NO_MEMORY;
        }
      DiskEntry->DiskNumber = (*GlobalDiskCount)++;

      NameU.Buffer = (PWCHAR)ValueData;
      NameU.Length = NameU.MaximumLength = 8 * sizeof(WCHAR);
      RtlUnicodeStringToInteger(&NameU, 16, &DiskEntry->Checksum);

      NameU.Buffer = (PWCHAR)ValueData + 9;
      RtlUnicodeStringToInteger(&NameU, 16, &DiskEntry->Signature);

      InsertTailList(ListHead, &DiskEntry->ListEntry);
    }

  return STATUS_SUCCESS;
}

#define ROOT_NAME   L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System\\MultifunctionAdapter"

static VOID INIT_FUNCTION
IopEnumerateBiosDisks(PLIST_ENTRY ListHead)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  WCHAR Name[255];
  ULONG AdapterCount;
  ULONG ControllerCount;
  ULONG DiskCount;
  NTSTATUS Status;
  ULONG GlobalDiskCount=0;

 
  memset(QueryTable, 0, sizeof(QueryTable));
  QueryTable[0].Name = L"Identifier";
  QueryTable[0].QueryRoutine = DiskQueryRoutine;
  QueryTable[0].EntryContext = (PVOID)&GlobalDiskCount;

  AdapterCount = 0;
  while (1)
    {
      swprintf(Name, L"%s\\%lu", ROOT_NAME, AdapterCount);
      Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                      Name,
                                      &QueryTable[1],
                                      NULL,
                                      NULL);
      if (!NT_SUCCESS(Status))
        {
            break;
        }
        
      swprintf(Name, L"%s\\%lu\\DiskController", ROOT_NAME, AdapterCount);
      Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                      Name,
                                      &QueryTable[1],
                                      NULL,
                                      NULL);
      if (NT_SUCCESS(Status))
        {
          ControllerCount = 0;
          while (1)
            {
              swprintf(Name, L"%s\\%lu\\DiskController\\%lu", ROOT_NAME, AdapterCount, ControllerCount);
              Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                              Name,
                                              &QueryTable[1],
                                              NULL,
                                              NULL);
              if (!NT_SUCCESS(Status))
                {
                    break;
                }
                
              swprintf(Name, L"%s\\%lu\\DiskController\\%lu\\DiskPeripheral", ROOT_NAME, AdapterCount, ControllerCount);
              Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                              Name,
                                              &QueryTable[1],
                                              NULL,
                                              NULL);
              if (NT_SUCCESS(Status))
                {
                  DiskCount = 0;
                  while (1)
                    {
                      swprintf(Name, L"%s\\%lu\\DiskController\\%lu\\DiskPeripheral\\%lu", ROOT_NAME, AdapterCount, ControllerCount, DiskCount);
                      Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                                      Name,
                                                      QueryTable,
                                                      (PVOID)ListHead,
                                                      NULL);
                      if (!NT_SUCCESS(Status))
                        {
                          break;
                        }
                      DiskCount++;
                    }
                }
              ControllerCount++;
            }
        }
      AdapterCount++;
    }
}

static VOID INIT_FUNCTION
IopEnumerateDisks(PLIST_ENTRY ListHead)
{
  ULONG i, k;
  PDISKENTRY DiskEntry;
  DISK_GEOMETRY DiskGeometry;
  KEVENT Event;
  PIRP Irp;
  IO_STATUS_BLOCK StatusBlock;
  LARGE_INTEGER PartitionOffset;
  PULONG Buffer;
  WCHAR DeviceNameBuffer[80];
  UNICODE_STRING DeviceName;
  NTSTATUS Status;
  PDEVICE_OBJECT DeviceObject;
  PFILE_OBJECT FileObject;
  BOOLEAN IsRemovableMedia;
  PPARTITION_SECTOR PartitionBuffer = NULL;
  ULONG PartitionBufferSize = 0;


  for (i = 0; i < IoGetConfigurationInformation()->DiskCount; i++)
    {

      swprintf(DeviceNameBuffer,
	       L"\\Device\\Harddisk%lu\\Partition0",
	       i);
      RtlInitUnicodeString(&DeviceName,
			   DeviceNameBuffer);


      Status = IoGetDeviceObjectPointer(&DeviceName,
				        FILE_READ_DATA,
				        &FileObject,
				        &DeviceObject);
      if (!NT_SUCCESS(Status))
        {
	  continue;
	}
      IsRemovableMedia = DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA ? TRUE : FALSE;
      ObDereferenceObject(FileObject);
      if (IsRemovableMedia)
        {
          ObDereferenceObject(DeviceObject);
          continue;
	}
      DiskEntry = ExAllocatePool(PagedPool, sizeof(DISKENTRY));
      if (DiskEntry == NULL)
        {
          KEBUGCHECK(0);
        }
      DiskEntry->DiskNumber = i;
      DiskEntry->DeviceObject = DeviceObject;

      /* determine the sector size */
      KeInitializeEvent(&Event, NotificationEvent, FALSE);
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
          KEBUGCHECK(0);
        }

      Status = IoCallDriver(DeviceObject, Irp);
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
          KEBUGCHECK(0);
        }
      if (PartitionBuffer != NULL && PartitionBufferSize < DiskGeometry.BytesPerSector)
        {
          ExFreePool(PartitionBuffer);
          PartitionBuffer = NULL;
        }
      if (PartitionBuffer == NULL)
        {
          PartitionBufferSize = max(DiskGeometry.BytesPerSector, PAGE_SIZE);
          PartitionBuffer = ExAllocatePool(PagedPool, PartitionBufferSize);
          if (PartitionBuffer == NULL)
            {
              KEBUGCHECK(0);
            }
        }

      /* read the partition sector */
      KeInitializeEvent(&Event, NotificationEvent, FALSE);
      PartitionOffset.QuadPart = 0;
      Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
				         DeviceObject,
				         PartitionBuffer,
				         DiskGeometry.BytesPerSector,
				         &PartitionOffset,
				         &Event,
				         &StatusBlock);
      Status = IoCallDriver(DeviceObject, Irp);
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
          KEBUGCHECK(0);
        }

      /* Calculate the MBR checksum */
      DiskEntry->Checksum = 0;
      Buffer = (PULONG)PartitionBuffer;
      for (k = 0; k < 128; k++)
        {
          DiskEntry->Checksum += Buffer[k];
        }
      DiskEntry->Checksum = ~DiskEntry->Checksum + 1;
      DiskEntry->Signature = PartitionBuffer->Signature;

      InsertTailList(ListHead, &DiskEntry->ListEntry);
    }
  if (PartitionBuffer)
    {
      ExFreePool(PartitionBuffer);
    }
}

static NTSTATUS INIT_FUNCTION
IopAssignArcNamesToDisk(PDEVICE_OBJECT DeviceObject, ULONG RDisk, ULONG DiskNumber)
{
  WCHAR DeviceNameBuffer[80];
  WCHAR ArcNameBuffer[80];
  UNICODE_STRING DeviceName;
  UNICODE_STRING ArcName;
  PDRIVE_LAYOUT_INFORMATION LayoutInfo = NULL;
  NTSTATUS Status;
  ULONG i;
  KEVENT Event;
  PIRP Irp;
  IO_STATUS_BLOCK StatusBlock;
  ULONG PartitionNumber;

  swprintf(DeviceNameBuffer,
	   L"\\Device\\Harddisk%lu\\Partition0",
	   DiskNumber);
  RtlInitUnicodeString(&DeviceName,
		       DeviceNameBuffer);

  swprintf(ArcNameBuffer,
	   L"\\ArcName\\multi(0)disk(0)rdisk(%lu)",
	   RDisk);
  RtlInitUnicodeString(&ArcName,
		       ArcNameBuffer);

  DPRINT("%wZ ==> %wZ\n", &ArcName, &DeviceName);

  Status = IoAssignArcName(&ArcName, &DeviceName);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("IoAssignArcName failed, status=%lx\n", Status);
      return(Status);
    }

  LayoutInfo = ExAllocatePool(PagedPool, 2 * PAGE_SIZE);
  if (LayoutInfo == NULL)
    {
      return STATUS_NO_MEMORY;
    }
  KeInitializeEvent(&Event, NotificationEvent, FALSE);
  Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_LAYOUT,
				      DeviceObject,
				      NULL,
				      0,
				      LayoutInfo,
				      2 * PAGE_SIZE,
				      FALSE,
				      &Event,
				      &StatusBlock);
  if (Irp == NULL)
    {
      ExFreePool(LayoutInfo);
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  Status = IoCallDriver(DeviceObject, Irp);
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
      ExFreePool(LayoutInfo);
      return Status;
    }

  DPRINT("Number of partitions: %u\n", LayoutInfo->PartitionCount);
  
  PartitionNumber = 1;
  for (i = 0; i < LayoutInfo->PartitionCount; i++)
    {
      if (!IsContainerPartition(LayoutInfo->PartitionEntry[i].PartitionType) &&
          LayoutInfo->PartitionEntry[i].PartitionType != PARTITION_ENTRY_UNUSED)
        {
          
          swprintf(DeviceNameBuffer,
	           L"\\Device\\Harddisk%lu\\Partition%lu",
	           DiskNumber,
	           PartitionNumber);
          RtlInitUnicodeString(&DeviceName, DeviceNameBuffer);

          swprintf(ArcNameBuffer,
	           L"\\ArcName\\multi(0)disk(0)rdisk(%lu)partition(%lu)",
	           RDisk,
	           PartitionNumber);
          RtlInitUnicodeString(&ArcName, ArcNameBuffer);

          DPRINT("%wZ ==> %wZ\n", &ArcName, &DeviceName);

          Status = IoAssignArcName(&ArcName, &DeviceName);
          if (!NT_SUCCESS(Status))
            {
              DPRINT1("IoAssignArcName failed, status=%lx\n", Status);
              ExFreePool(LayoutInfo);
	      return(Status);
            }
          PartitionNumber++;
        }
    }
  ExFreePool(LayoutInfo);
  return STATUS_SUCCESS;
}

NTSTATUS INIT_FUNCTION
IoCreateArcNames(VOID)
{
  PCONFIGURATION_INFORMATION ConfigInfo;
  WCHAR DeviceNameBuffer[80];
  WCHAR ArcNameBuffer[80];
  UNICODE_STRING DeviceName;
  UNICODE_STRING ArcName;
  ULONG i, RDiskNumber;
  NTSTATUS Status;
  LIST_ENTRY BiosDiskListHead;
  LIST_ENTRY DiskListHead;
  PLIST_ENTRY Entry;
  PDISKENTRY BiosDiskEntry;
  PDISKENTRY DiskEntry;

  DPRINT("IoCreateArcNames() called\n");

  ConfigInfo = IoGetConfigurationInformation();

  /* create ARC names for floppy drives */
  DPRINT("Floppy drives: %lu\n", ConfigInfo->FloppyCount);
  for (i = 0; i < ConfigInfo->FloppyCount; i++)
    {
      swprintf(DeviceNameBuffer,
	       L"\\Device\\Floppy%lu",
	       i);
      RtlInitUnicodeString(&DeviceName,
			   DeviceNameBuffer);

      swprintf(ArcNameBuffer,
	       L"\\ArcName\\multi(0)disk(0)fdisk(%lu)",
	       i);
      RtlInitUnicodeString(&ArcName,
			   ArcNameBuffer);
      DPRINT("%wZ ==> %wZ\n",
	     &ArcName,
	     &DeviceName);

      Status = IoAssignArcName(&ArcName,
			       &DeviceName);
      if (!NT_SUCCESS(Status))
	return(Status);
    }

  /* create ARC names for hard disk drives */
  InitializeListHead(&BiosDiskListHead);
  InitializeListHead(&DiskListHead);
  IopEnumerateBiosDisks(&BiosDiskListHead);
  IopEnumerateDisks(&DiskListHead);

  RDiskNumber = 0;
  while (!IsListEmpty(&BiosDiskListHead))
    {
      Entry = RemoveHeadList(&BiosDiskListHead);
      BiosDiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);
      Entry = DiskListHead.Flink;
      while (Entry != &DiskListHead)
        {
          DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);
          if (DiskEntry->Checksum == BiosDiskEntry->Checksum &&
              DiskEntry->Signature == BiosDiskEntry->Signature)
            {

              Status = IopAssignArcNamesToDisk(DiskEntry->DeviceObject, RDiskNumber, DiskEntry->DiskNumber);

              RemoveEntryList(&DiskEntry->ListEntry);
              ExFreePool(DiskEntry);
              break;
            }
          Entry = Entry->Flink;
        }
      RDiskNumber++;
      ExFreePool(BiosDiskEntry);
    }

  while (!IsListEmpty(&DiskListHead))
    {
      Entry = RemoveHeadList(&DiskListHead);
      DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);
      ExFreePool(DiskEntry);
    }

  /* create ARC names for cdrom drives */
  DPRINT("CD-ROM drives: %lu\n", ConfigInfo->CdRomCount);
  for (i = 0; i < ConfigInfo->CdRomCount; i++)
    {
      swprintf(DeviceNameBuffer,
	       L"\\Device\\CdRom%lu",
	       i);
      RtlInitUnicodeString(&DeviceName,
			   DeviceNameBuffer);

      swprintf(ArcNameBuffer,
	       L"\\ArcName\\multi(0)disk(0)cdrom(%lu)",
	       i);
      RtlInitUnicodeString(&ArcName,
			   ArcNameBuffer);
      DPRINT("%wZ ==> %wZ\n",
	     &ArcName,
	     &DeviceName);

      Status = IoAssignArcName(&ArcName,
			       &DeviceName);
      if (!NT_SUCCESS(Status))
	return(Status);
    }

  DPRINT("IoCreateArcNames() done\n");

  return(STATUS_SUCCESS);
}

VOID
INIT_FUNCTION
NTAPI
IopApplyRosCdromArcHack(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ULONG DeviceNumber = -1;
    PCONFIGURATION_INFORMATION ConfigInfo;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DeviceName;
    WCHAR Buffer[MAX_PATH];
    CHAR AnsiBuffer[MAX_PATH];
    ULONG i;
    FILE_BASIC_INFORMATION FileInfo;
    NTSTATUS Status;
    PCHAR p, q;

    /* Only ARC Name left - Build full ARC Name */
    p = strstr(LoaderBlock->ArcBootDeviceName, "cdrom");
    if (p)
    {
        /* Get configuration information */
        ConfigInfo = IoGetConfigurationInformation();
        for (i = 0; i < ConfigInfo->CdRomCount; i++)
        {
            /* Try to find the installer */
            swprintf(Buffer, L"\\Device\\CdRom%lu\\reactos\\ntoskrnl.exe", i);
            RtlInitUnicodeString(&DeviceName, Buffer);
            InitializeObjectAttributes(&ObjectAttributes,
                                       &DeviceName,
                                       0,
                                       NULL,
                                       NULL);
            Status = ZwQueryAttributesFile(&ObjectAttributes, &FileInfo);
            if (NT_SUCCESS(Status)) DeviceNumber = i;

            /* Try to find live CD boot */
            swprintf(Buffer,
                     L"\\Device\\CdRom%lu\\reactos\\system32\\ntoskrnl.exe",
                     i);
            RtlInitUnicodeString(&DeviceName, Buffer);
            InitializeObjectAttributes(&ObjectAttributes,
                                       &DeviceName,
                                       0,
                                       NULL,
                                       NULL);
            Status = ZwQueryAttributesFile(&ObjectAttributes, &FileInfo);
            if (NT_SUCCESS(Status)) DeviceNumber = i;
        }

        /* Build the name */
        sprintf(p, "cdrom(%lu)", DeviceNumber);

        /* Adjust original command line */
        q = strchr(p, ')');
        if (q)
        {
            q++;
            strcpy(AnsiBuffer, q);
            sprintf(p, "cdrom(%lu)", DeviceNumber);
            strcat(p, AnsiBuffer);
        }
    }
}

NTSTATUS
NTAPI
IopReassignSystemRoot(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                      OUT PANSI_STRING NtBootPath)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    CHAR Buffer[256], AnsiBuffer[256];
    WCHAR ArcNameBuffer[64];
    ANSI_STRING TargetString, ArcString, TempString;
    UNICODE_STRING LinkName, TargetName, ArcName;
    HANDLE LinkHandle;

    /* Check if this is a CD-ROM boot */
    IopApplyRosCdromArcHack(LoaderBlock);

    /* Create the Unicode name for the current ARC boot device */
    sprintf(Buffer, "\\ArcName\\%s", LoaderBlock->ArcBootDeviceName);
    RtlInitAnsiString(&TargetString, Buffer);
    Status = RtlAnsiStringToUnicodeString(&TargetName, &TargetString, TRUE);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Initialize the attributes and open the link */
    InitializeObjectAttributes(&ObjectAttributes,
                               &TargetName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenSymbolicLinkObject(&LinkHandle,
                                      SYMBOLIC_LINK_ALL_ACCESS,
                                      &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        /* We failed, free the string */
        RtlFreeUnicodeString(&TargetName);
        return FALSE;
    }

    /* Query the current \\SystemRoot */
    ArcName.Buffer = ArcNameBuffer;
    ArcName.Length = 0;
    ArcName.MaximumLength = sizeof(ArcNameBuffer);
    Status = NtQuerySymbolicLinkObject(LinkHandle, &ArcName, NULL);
    if (!NT_SUCCESS(Status))
    {
        /* We failed, free the string */
        RtlFreeUnicodeString(&TargetName);
        return FALSE;
    }

    /* Convert it to Ansi */
    ArcString.Buffer = AnsiBuffer;
    ArcString.Length = 0;
    ArcString.MaximumLength = sizeof(AnsiBuffer);
    Status = RtlUnicodeStringToAnsiString(&ArcString, &ArcName, FALSE);
    AnsiBuffer[ArcString.Length] = ANSI_NULL;

    /* Close the link handle and free the name */
    ObCloseHandle(LinkHandle, KernelMode);
    RtlFreeUnicodeString(&TargetName);

    /* Setup the system root name again */
    RtlInitAnsiString(&TempString, "\\SystemRoot");
    Status = RtlAnsiStringToUnicodeString(&LinkName, &TempString, TRUE);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Open the symbolic link for it */
    InitializeObjectAttributes(&ObjectAttributes,
                               &LinkName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenSymbolicLinkObject(&LinkHandle,
                                      SYMBOLIC_LINK_ALL_ACCESS,
                                      &ObjectAttributes);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Destroy it */
    NtMakeTemporaryObject(LinkHandle);
    ObCloseHandle(LinkHandle, KernelMode);

    /* Now create the new name for it */
    sprintf(Buffer, "%s%s", ArcString.Buffer, LoaderBlock->NtBootPathName);

    /* Copy it into the passed parameter and null-terminate it */
    RtlCopyString(NtBootPath, &ArcString);
    Buffer[strlen(Buffer) - 1] = ANSI_NULL;

    /* Setup the Unicode-name for the new symbolic link value */
    RtlInitAnsiString(&TargetString, Buffer);
    InitializeObjectAttributes(&ObjectAttributes,
                               &LinkName,
                               OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                               NULL,
                               NULL);
    Status = RtlAnsiStringToUnicodeString(&ArcName, &TargetString, TRUE);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Create it */
    Status = NtCreateSymbolicLinkObject(&LinkHandle,
                                        SYMBOLIC_LINK_ALL_ACCESS,
                                        &ObjectAttributes,
                                        &ArcName);

    /* Free all the strings and close the handle and return success */
    RtlFreeUnicodeString(&ArcName);
    RtlFreeUnicodeString(&LinkName);
    ObCloseHandle(LinkHandle, KernelMode);
    return TRUE;
}

/* EOF */
