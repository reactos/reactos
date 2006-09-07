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

static NTSTATUS INIT_FUNCTION
IopCheckCdromDevices(PULONG DeviceNumber);

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, DiskQueryRoutine)
#pragma alloc_text(INIT, IopEnumerateBiosDisks)
#pragma alloc_text(INIT, IopEnumerateDisks)
#pragma alloc_text(INIT, IopAssignArcNamesToDisk)
#pragma alloc_text(INIT, IoCreateArcNames)
#pragma alloc_text(INIT, IopCheckCdromDevices)
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


static NTSTATUS INIT_FUNCTION
IopCheckCdromDevices(PULONG DeviceNumber)
{
  PCONFIGURATION_INFORMATION ConfigInfo;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING DeviceName;
  WCHAR DeviceNameBuffer[MAX_PATH];
  HANDLE Handle;
  ULONG i;
  NTSTATUS Status;
  IO_STATUS_BLOCK IoStatusBlock;
#if 0
  PFILE_FS_VOLUME_INFORMATION FileFsVolume;
  USHORT Buffer[FS_VOLUME_BUFFER_SIZE];

  FileFsVolume = (PFILE_FS_VOLUME_INFORMATION)Buffer;
#endif

  ConfigInfo = IoGetConfigurationInformation();
  for (i = 0; i < ConfigInfo->CdRomCount; i++)
    {
#if 0
      swprintf(DeviceNameBuffer,
	       L"\\Device\\CdRom%lu\\",
	       i);
      RtlInitUnicodeString(&DeviceName,
			   DeviceNameBuffer);

      InitializeObjectAttributes(&ObjectAttributes,
				 &DeviceName,
				 0,
				 NULL,
				 NULL);

      Status = ZwOpenFile(&Handle,
			  FILE_ALL_ACCESS,
			  &ObjectAttributes,
			  &IoStatusBlock,
			  0,
			  0);
      DPRINT("ZwOpenFile()  DeviceNumber %lu  Status %lx\n", i, Status);
      if (NT_SUCCESS(Status))
	{
	  Status = ZwQueryVolumeInformationFile(Handle,
						&IoStatusBlock,
						FileFsVolume,
						FS_VOLUME_BUFFER_SIZE,
						FileFsVolumeInformation);
	  DPRINT("ZwQueryVolumeInformationFile()  Status %lx\n", Status);
	  if (NT_SUCCESS(Status))
	    {
	      DPRINT("VolumeLabel: '%S'\n", FileFsVolume->VolumeLabel);
	      if (_wcsicmp(FileFsVolume->VolumeLabel, L"REACTOS") == 0)
		{
		  ZwClose(Handle);
		  *DeviceNumber = i;
		  return(STATUS_SUCCESS);
		}
	    }
	  ZwClose(Handle);
	}
#endif

      /*
       * Check for 'reactos/ntoskrnl.exe' first...
       */

      swprintf(DeviceNameBuffer,
	       L"\\Device\\CdRom%lu\\reactos\\ntoskrnl.exe",
	       i);
      RtlInitUnicodeString(&DeviceName,
			   DeviceNameBuffer);

      InitializeObjectAttributes(&ObjectAttributes,
				 &DeviceName,
				 0,
				 NULL,
				 NULL);

      Status = ZwOpenFile(&Handle,
			  FILE_ALL_ACCESS,
			  &ObjectAttributes,
			  &IoStatusBlock,
			  0,
			  0);
      DPRINT("ZwOpenFile()  DeviceNumber %lu  Status %lx\n", i, Status);
      if (NT_SUCCESS(Status))
	{
	  DPRINT("Found ntoskrnl.exe on Cdrom%lu\n", i);
	  ZwClose(Handle);
	  *DeviceNumber = i;
	  return(STATUS_SUCCESS);
	}

      /*
       * ...and for 'reactos/system32/ntoskrnl.exe' also.
       */

      swprintf(DeviceNameBuffer,
	       L"\\Device\\CdRom%lu\\reactos\\system32\\ntoskrnl.exe",
	       i);
      RtlInitUnicodeString(&DeviceName,
			   DeviceNameBuffer);

      InitializeObjectAttributes(&ObjectAttributes,
				 &DeviceName,
				 0,
				 NULL,
				 NULL);

      Status = ZwOpenFile(&Handle,
			  FILE_ALL_ACCESS,
			  &ObjectAttributes,
			  &IoStatusBlock,
			  0,
			  0);
      DPRINT("ZwOpenFile()  DeviceNumber %lu  Status %lx\n", i, Status);
      if (NT_SUCCESS(Status))
	{
	  DPRINT("Found ntoskrnl.exe on Cdrom%lu\n", i);
	  ZwClose(Handle);
	  *DeviceNumber = i;
	  return(STATUS_SUCCESS);
	}
    }

  DPRINT("Could not find ntoskrnl.exe\n");
  *DeviceNumber = (ULONG)-1;

  return(STATUS_UNSUCCESSFUL);
}


NTSTATUS INIT_FUNCTION
IoCreateSystemRootLink(PCHAR ParameterLine)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  UNICODE_STRING LinkName = RTL_CONSTANT_STRING(L"\\SystemRoot");
  UNICODE_STRING DeviceName;
  UNICODE_STRING ArcName;
  UNICODE_STRING BootPath;
  PCHAR ParamBuffer;
  PWCHAR ArcNameBuffer;
  PCHAR p;
  NTSTATUS Status;
  ULONG Length;
  HANDLE Handle;

  /* Create local parameter line copy */
  ParamBuffer = ExAllocatePool(PagedPool, 256);
  strcpy(ParamBuffer, (char *)ParameterLine);

  DPRINT("%s\n", ParamBuffer);
  /* Format: <arc_name>\<path> [options...] */

  /* cut options off */
  p = strchr(ParamBuffer, ' ');
  if (p)
    *p = 0;
  DPRINT("%s\n", ParamBuffer);

  /* extract path */
  p = strchr(ParamBuffer, '\\');
  if (p)
    {
      DPRINT("Boot path: %s\n", p);
      RtlCreateUnicodeStringFromAsciiz(&BootPath, p);
      *p = 0;
    }
  else
    {
      DPRINT("Boot path: %s\n", "\\");
      RtlCreateUnicodeStringFromAsciiz(&BootPath, "\\");
    }
  DPRINT("ARC name: %s\n", ParamBuffer);

  p = strstr(ParamBuffer, "cdrom");
  if (p != NULL)
    {
      ULONG DeviceNumber;

      DPRINT("Booting from CD-ROM!\n");
      Status = IopCheckCdromDevices(&DeviceNumber);
      if (!NT_SUCCESS(Status))
	{
	  CPRINT("Failed to find setup disk!\n");
	  return(Status);
	}

      sprintf(p, "cdrom(%lu)", DeviceNumber);

      DPRINT("New ARC name: %s\n", ParamBuffer);

      /* Adjust original command line */
      p = strstr(ParameterLine, "cdrom");
      if (p != NULL);
	{
	  char temp[256];
	  char *q;

	  q = strchr(p, ')');
	  if (q != NULL)
	    {

	      q++;
	      strcpy(temp, q);
	      sprintf(p, "cdrom(%lu)", DeviceNumber);
	      strcat(p, temp);
	    }
	}
    }

  /* Only arc name left - build full arc name */
  ArcNameBuffer = ExAllocatePool(PagedPool, 256 * sizeof(WCHAR));
  swprintf(ArcNameBuffer,
	   L"\\ArcName\\%S", ParamBuffer);
  RtlInitUnicodeString(&ArcName, ArcNameBuffer);
  DPRINT("Arc name: %wZ\n", &ArcName);

  /* free ParamBuffer */
  ExFreePool(ParamBuffer);

  /* allocate device name string */
  DeviceName.Length = 0;
  DeviceName.MaximumLength = 256 * sizeof(WCHAR);
  DeviceName.Buffer = ExAllocatePool(PagedPool, 256 * sizeof(WCHAR));

  InitializeObjectAttributes(&ObjectAttributes,
			     &ArcName,
			     OBJ_OPENLINK,
			     NULL,
			     NULL);

  Status = ZwOpenSymbolicLinkObject(&Handle,
				    SYMBOLIC_LINK_ALL_ACCESS,
				    &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString(&BootPath);
      ExFreePool(DeviceName.Buffer);
      CPRINT("ZwOpenSymbolicLinkObject() '%wZ' failed (Status %x)\n",
	     &ArcName,
	     Status);
      ExFreePool(ArcName.Buffer);

      return(Status);
    }
  ExFreePool(ArcName.Buffer);

  Status = ZwQuerySymbolicLinkObject(Handle,
				     &DeviceName,
				     &Length);
  ZwClose (Handle);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString(&BootPath);
      ExFreePool(DeviceName.Buffer);
      CPRINT("ZwQuerySymbolicObject() failed (Status %x)\n",
	     Status);

      return(Status);
    }
  DPRINT("Length: %lu DeviceName: %wZ\n", Length, &DeviceName);

  RtlAppendUnicodeStringToString(&DeviceName,
				 &BootPath);

  RtlFreeUnicodeString(&BootPath);
  DPRINT("DeviceName: %wZ\n", &DeviceName);

  /* create the '\SystemRoot' link */
  Status = IoCreateSymbolicLink(&LinkName,
				&DeviceName);
  ExFreePool(DeviceName.Buffer);
  if (!NT_SUCCESS(Status))
    {
      CPRINT("IoCreateSymbolicLink() failed (Status %x)\n",
	     Status);

      return(Status);
    }

  /* Check whether '\SystemRoot'(LinkName) can be opened */
  InitializeObjectAttributes(&ObjectAttributes,
			     &LinkName,
			     0,
			     NULL,
			     NULL);

  Status = ZwOpenFile(&Handle,
		      FILE_ALL_ACCESS,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      0,
		      0);
  if (!NT_SUCCESS(Status))
    {
      CPRINT("ZwOpenFile() failed to open '\\SystemRoot' (Status %x)\n",
	     Status);
      return(Status);
    }

  ZwClose(Handle);

  return(STATUS_SUCCESS);
}

/* EOF */
