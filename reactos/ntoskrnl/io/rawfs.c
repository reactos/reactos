/* $Id: rawfs.c,v 1.5 2003/07/11 01:23:14 royce Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/rawfs.c
 * PURPOSE:         Raw filesystem driver
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *                  Created 13/04/2003
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include <reactos/bugcodes.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES *******************************************************************/

typedef struct _RAWFS_GLOBAL_DATA
{
  PDRIVER_OBJECT DriverObject;
  PDEVICE_OBJECT DeviceObject;
  ULONG Flags;
  ERESOURCE VolumeListLock;
  LIST_ENTRY VolumeListHead;
  NPAGED_LOOKASIDE_LIST FcbLookasideList;
  NPAGED_LOOKASIDE_LIST CcbLookasideList;
} RAWFS_GLOBAL_DATA, *PRAWFS_GLOBAL_DATA, VCB, *PVCB;

typedef struct _RAWFS_DEVICE_EXTENSION
{
  KSPIN_LOCK FcbListLock;
  LIST_ENTRY FcbListHead;
  PDEVICE_OBJECT StorageDevice;
  ULONG Flags;
  struct _RAWFS_FCB *VolumeFcb;
  LIST_ENTRY VolumeListEntry;
} RAWFS_DEVICE_EXTENSION, *PRAWFS_DEVICE_EXTENSION;

typedef struct _RAWFS_IRP_CONTEXT
{
   PIRP Irp;
   PDEVICE_OBJECT DeviceObject;
   PRAWFS_DEVICE_EXTENSION DeviceExt;
   ULONG Flags;
   WORK_QUEUE_ITEM WorkQueueItem;
   PIO_STACK_LOCATION Stack;
   UCHAR MajorFunction;
   UCHAR MinorFunction;
   PFILE_OBJECT FileObject;
} RAWFS_IRP_CONTEXT, *PRAWFS_IRP_CONTEXT;

#define IRPCONTEXT_CANWAIT  0x0001

#define FCB_CACHE_INITIALIZED   0x0001
#define FCB_DELETE_PENDING      0x0002
#define FCB_IS_FAT              0x0004
#define FCB_IS_PAGE_FILE        0x0008
#define FCB_IS_VOLUME           0x0010

typedef struct _RAWFS_FCB
{
  /* Start FCB header required by ReactOS/Windows NT */
  FSRTL_COMMON_FCB_HEADER RFCB;
  SECTION_OBJECT_POINTERS SectionObjectPointers;
  ERESOURCE MainResource;
  ERESOURCE PagingIoResource;
  /* End FCB header required by ReactOS/Windows NT */

  /* Reference count */
  LONG RefCount;

  /* List of FCB's for this volume */
  LIST_ENTRY FcbListEntry;

  /* Pointer to the parent fcb */
  struct _RAWFS_FCB* ParentFcb;

  /* Flags for the FCB */
  ULONG Flags;

  /* Pointer to the file object which has initialized the fcb */
  PFILE_OBJECT FileObject;
} RAWFS_FCB, *PRAWFS_FCB;

typedef struct _RAWFS_CCB
{
  LARGE_INTEGER CurrentByteOffset;
} RAWFS_CCB, *PRAWFS_CCB;

/* GLOBALS ******************************************************************/

#define TAG_IRP TAG('R', 'I', 'R', 'P')

static PDRIVER_OBJECT RawFsDriverObject;
static PDEVICE_OBJECT DiskDeviceObject;
static PDEVICE_OBJECT CdromDeviceObject;
static PDEVICE_OBJECT TapeDeviceObject;
static NPAGED_LOOKASIDE_LIST IrpContextLookasideList;
static LONG RawFsQueueCount = 0;

/* FUNCTIONS *****************************************************************/

BOOLEAN
RawFsIsRawFileSystemDeviceObject(IN PDEVICE_OBJECT DeviceObject)
{
  DPRINT("RawFsIsRawFileSystemDeviceObject(DeviceObject %x)\n", DeviceObject);

  if (DeviceObject == DiskDeviceObject)
    return TRUE;
  if (DeviceObject == CdromDeviceObject)
    return TRUE;
  if (DeviceObject == TapeDeviceObject)
    return TRUE;
  return FALSE;
}

static NTSTATUS
RawFsDispatchRequest(IN PRAWFS_IRP_CONTEXT IrpContext);

/*static */NTSTATUS
RawFsReadDisk(IN PDEVICE_OBJECT pDeviceObject,
  IN PLARGE_INTEGER ReadOffset,
  IN ULONG ReadLength,
  IN OUT PUCHAR Buffer)
{
  IO_STATUS_BLOCK IoStatus;
  NTSTATUS Status;
  KEVENT Event;
  PIRP Irp;

  KeInitializeEvent(&Event, NotificationEvent, FALSE);

  DPRINT("RawFsReadDisk(pDeviceObject %x, Offset %I64x, Length %d, Buffer %x)\n",
	  pDeviceObject, ReadOffset->QuadPart, ReadLength, Buffer);

  DPRINT ("Building synchronous FSD Request...\n");
  Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
    pDeviceObject,
    Buffer,
    ReadLength,
    ReadOffset,
    &Event,
    &IoStatus);
  if (Irp == NULL)
    {
      DPRINT("IoBuildSynchronousFsdRequest() failed\n");
      return STATUS_UNSUCCESSFUL;
    }

  DPRINT("Calling IO Driver... with irp %x\n", Irp);
  Status = IoCallDriver(pDeviceObject, Irp);

  DPRINT("Waiting for IO Operation for %x\n", Irp);
  if (Status == STATUS_PENDING)
    {
      DPRINT("Operation pending\n");
      KeWaitForSingleObject (&Event, Suspended, KernelMode, FALSE, NULL);
      DPRINT("Getting IO Status... for %x\n", Irp);
      Status = IoStatus.Status;
    }

  if (!NT_SUCCESS(Status))
    {
      DPRINT("RawFsReadDisk() failed. Status %x\n", Status);
      DPRINT("(pDeviceObject %x, Offset %I64x, Size %d, Buffer %x\n",
	      pDeviceObject, ReadOffset->QuadPart, ReadLength, Buffer);
      return Status;
    }
  DPRINT("Block request succeeded for %x\n", Irp);
  return STATUS_SUCCESS;
}

/*static */NTSTATUS
RawFsWriteDisk(IN PDEVICE_OBJECT pDeviceObject,
  IN PLARGE_INTEGER WriteOffset,
  IN ULONG WriteLength,
  IN PUCHAR Buffer)
{
  IO_STATUS_BLOCK IoStatus;
  NTSTATUS Status;
  KEVENT Event;
  PIRP Irp;

  DPRINT("RawFsWriteDisk(pDeviceObject %x, Offset %I64x, Size %d, Buffer %x)\n",
	  pDeviceObject, WriteOffset->QuadPart, WriteLength, Buffer);

  KeInitializeEvent(&Event, NotificationEvent, FALSE);

  DPRINT("Building synchronous FSD Request...\n");
  Irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE,
    pDeviceObject,
    Buffer,
    WriteLength,
    WriteOffset, 
    &Event, 
    &IoStatus);
  if (!Irp)
    {
      DPRINT("IoBuildSynchronousFsdRequest()\n");
      return(STATUS_UNSUCCESSFUL);
    }

  DPRINT("Calling IO Driver...\n");
  Status = IoCallDriver(pDeviceObject, Irp);

  DPRINT("Waiting for IO Operation...\n");
  if (Status == STATUS_PENDING)
    {
      KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
      DPRINT("Getting IO Status...\n");
      Status = IoStatus.Status;
    }
  if (!NT_SUCCESS(Status))
    {
      DPRINT("RawFsWriteDisk() failed. Status %x\n", Status);
      DPRINT("(pDeviceObject %x, Offset %I64x, Size %d, Buffer %x\n",
	      pDeviceObject, WriteOffset->QuadPart, WriteLength, Buffer);
      return Status;
    }

  return STATUS_SUCCESS;
}

static NTSTATUS
RawFsBlockDeviceIoControl(IN PDEVICE_OBJECT DeviceObject,
  IN ULONG CtlCode,
  IN PVOID InputBuffer,
  IN ULONG InputBufferSize,
  IN OUT PVOID OutputBuffer, 
  IN OUT PULONG pOutputBufferSize)
{
	ULONG OutputBufferSize = 0;
	KEVENT Event;
	PIRP Irp;
	IO_STATUS_BLOCK IoStatus;
	NTSTATUS Status;

	DPRINT("RawFsBlockDeviceIoControl(DeviceObject %x, CtlCode %x, "
    "InputBuffer %x, InputBufferSize %x, OutputBuffer %x, " 
    "POutputBufferSize %x (%x)\n", DeviceObject, CtlCode, 
    InputBuffer, InputBufferSize, OutputBuffer, pOutputBufferSize, 
    pOutputBufferSize ? *pOutputBufferSize : 0);

	if (pOutputBufferSize)
  	{
  		OutputBufferSize = *pOutputBufferSize;
  	}

	KeInitializeEvent(&Event, NotificationEvent, FALSE);

	DPRINT("Building device I/O control request ...\n");
	Irp = IoBuildDeviceIoControlRequest(CtlCode, 
    DeviceObject, 
    InputBuffer, 
    InputBufferSize, 
    OutputBuffer,
    OutputBufferSize, 
    FALSE, 
    &Event, 
    &IoStatus);
	if (Irp == NULL)
  	{
  		DPRINT("IoBuildDeviceIoControlRequest failed\n");
  		return STATUS_INSUFFICIENT_RESOURCES;
  	}

	DPRINT("Calling IO Driver... with irp %x\n", Irp);
	Status = IoCallDriver(DeviceObject, Irp);

	DPRINT("Waiting for IO Operation for %x\n", Irp);
	if (Status == STATUS_PENDING)
    {
    	DPRINT("Operation pending\n");
    	KeWaitForSingleObject (&Event, Suspended, KernelMode, FALSE, NULL);
    	DPRINT("Getting IO Status... for %x\n", Irp);
    	Status = IoStatus.Status;
    }
	if (OutputBufferSize)
  	{
  		*pOutputBufferSize = OutputBufferSize;
  	}
	DPRINT("Returning Status %x\n", Status);
	return Status;
}

static PRAWFS_FCB
RawFsNewFCB(IN PRAWFS_GLOBAL_DATA pGlobalData)
{
  PRAWFS_FCB Fcb;

  Fcb = ExAllocateFromNPagedLookasideList(&pGlobalData->FcbLookasideList);
  memset(Fcb, 0, sizeof(RAWFS_FCB));
  ExInitializeResourceLite(&Fcb->PagingIoResource);
  ExInitializeResourceLite(&Fcb->MainResource);
//  FsRtlInitializeFileLock(&Fcb->FileLock, NULL, NULL); 
  return Fcb;
}

static VOID
RawFsDestroyFCB(IN PRAWFS_GLOBAL_DATA pGlobalData, IN PRAWFS_FCB pFcb)
{
  //FsRtlUninitializeFileLock(&pFcb->FileLock); 
  ExDeleteResourceLite(&pFcb->PagingIoResource);
  ExDeleteResourceLite(&pFcb->MainResource);
  ExFreeToNPagedLookasideList(&pGlobalData->FcbLookasideList, pFcb);
}

static PRAWFS_CCB
RawFsNewCCB(PRAWFS_GLOBAL_DATA pGlobalData)
{
  PRAWFS_CCB Ccb;

  Ccb = ExAllocateFromNPagedLookasideList(&pGlobalData->CcbLookasideList);
  memset(Ccb, 0, sizeof(RAWFS_CCB));
  return Ccb;
}

/*static */VOID
RawFsDestroyCCB(PRAWFS_GLOBAL_DATA pGlobalData, PRAWFS_CCB pCcb)
{
  ExFreeToNPagedLookasideList(&pGlobalData->CcbLookasideList, pCcb);
}

static PRAWFS_IRP_CONTEXT
RawFsAllocateIrpContext(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
  PRAWFS_GLOBAL_DATA GlobalData;
  PRAWFS_IRP_CONTEXT IrpContext;
  UCHAR MajorFunction;

  DPRINT("RawFsAllocateIrpContext(DeviceObject %x, Irp %x)\n", DeviceObject, Irp);

  assert(DeviceObject);
  assert(Irp);

  GlobalData = (PRAWFS_GLOBAL_DATA) DeviceObject->DeviceExtension;
  IrpContext = ExAllocateFromNPagedLookasideList(&IrpContextLookasideList);
  if (IrpContext)
    {
      RtlZeroMemory(IrpContext, sizeof(IrpContext));
      IrpContext->Irp = Irp;
      IrpContext->DeviceObject = DeviceObject;
      IrpContext->DeviceExt = DeviceObject->DeviceExtension;
      IrpContext->Stack = IoGetCurrentIrpStackLocation(Irp);
      assert(IrpContext->Stack);
      MajorFunction = IrpContext->MajorFunction = IrpContext->Stack->MajorFunction;
      IrpContext->MinorFunction = IrpContext->Stack->MinorFunction;
      IrpContext->FileObject = IrpContext->Stack->FileObject;
      if (MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL ||
        MajorFunction == IRP_MJ_DEVICE_CONTROL ||
        MajorFunction == IRP_MJ_SHUTDOWN)
        {
          IrpContext->Flags |= IRPCONTEXT_CANWAIT;
        }
      else if (MajorFunction != IRP_MJ_CLEANUP &&
        MajorFunction != IRP_MJ_CLOSE &&
        IoIsOperationSynchronous(Irp))
        {
          IrpContext->Flags |= IRPCONTEXT_CANWAIT;
        }
    }
  return IrpContext;
}

static VOID
RawFsFreeIrpContext(IN PRAWFS_IRP_CONTEXT IrpContext)
{
  DPRINT("RawFsFreeIrpContext(IrpContext %x)\n", IrpContext);

  assert(IrpContext);

  ExFreeToNPagedLookasideList(&IrpContextLookasideList, IrpContext);
}

static VOID
STDCALL RawFsDoRequest(PVOID IrpContext)
{
  ULONG Count;

  DPRINT("RawFsDoRequest(IrpContext %x), MajorFunction %x, %d\n",
    IrpContext, ((PRAWFS_IRP_CONTEXT) IrpContext)->MajorFunction, Count);

  Count = InterlockedDecrement(&RawFsQueueCount);
  RawFsDispatchRequest((PRAWFS_IRP_CONTEXT) IrpContext);
}

static NTSTATUS
RawFsQueueRequest(PRAWFS_IRP_CONTEXT IrpContext)
{
  ULONG Count;

  DPRINT("RawFsQueueRequest (IrpContext %x), %d\n", IrpContext, Count);

  assert(IrpContext != NULL);
  assert(IrpContext->Irp != NULL);

  Count = InterlockedIncrement(&RawFsQueueCount);

  IrpContext->Flags |= IRPCONTEXT_CANWAIT;
  IoMarkIrpPending (IrpContext->Irp);
  ExInitializeWorkItem (&IrpContext->WorkQueueItem, RawFsDoRequest, IrpContext);
  ExQueueWorkItem(&IrpContext->WorkQueueItem, CriticalWorkQueue);
  return STATUS_PENDING;
}

static NTSTATUS
RawFsClose(IN PRAWFS_IRP_CONTEXT IrpContext)
{
  DPRINT("RawFsClose(IrpContext %x)\n", IrpContext);
  UNIMPLEMENTED
  return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS
RawFsCreateFile(IN PRAWFS_IRP_CONTEXT IrpContext)
{
  PRAWFS_DEVICE_EXTENSION DeviceExt;
  PRAWFS_GLOBAL_DATA GlobalData;
  PIO_STACK_LOCATION IoSp;
  PFILE_OBJECT FileObject;
  ULONG RequestedDisposition;
  ULONG RequestedOptions;
  PRAWFS_FCB pFcb;
  PRAWFS_CCB pCcb;

  GlobalData = (PRAWFS_GLOBAL_DATA) IrpContext->DeviceObject->DeviceExtension;
  IoSp = IoGetCurrentIrpStackLocation(IrpContext->Irp);
  RequestedDisposition = ((IoSp->Parameters.Create.Options >> 24) & 0xff);
  RequestedOptions = IoSp->Parameters.Create.Options & FILE_VALID_OPTION_FLAGS;
  FileObject = IoSp->FileObject;
  DeviceExt = IrpContext->DeviceObject->DeviceExtension;

  if (FileObject->FileName.Length == 0 && 
      FileObject->RelatedFileObject == NULL)
    {
      /* This a open operation for the volume itself */
      if (RequestedDisposition == FILE_CREATE
    	    || RequestedDisposition == FILE_OVERWRITE_IF
    	    || RequestedDisposition == FILE_SUPERSEDE)
      	{
      	  return STATUS_ACCESS_DENIED;
      	}
      if (RequestedOptions & FILE_DIRECTORY_FILE)
      	{
      	  return STATUS_NOT_A_DIRECTORY;
      	}
      pFcb = DeviceExt->VolumeFcb;
      pCcb = RawFsNewCCB(GlobalData);
      if (pCcb == NULL)
      	{
      	  return (STATUS_INSUFFICIENT_RESOURCES);
      	}

      FileObject->Flags |= FO_FCB_IS_VALID;
      FileObject->SectionObjectPointer = &pFcb->SectionObjectPointers;
      FileObject->FsContext = pFcb;
      FileObject->FsContext2 = pCcb;
      pFcb->RefCount++;

      IrpContext->Irp->IoStatus.Information = FILE_OPENED;
      return(STATUS_SUCCESS);
    }

  /* This filesystem driver only supports volume access */
  return(STATUS_INVALID_PARAMETER);
}

static NTSTATUS
RawFsCreate(IN PRAWFS_IRP_CONTEXT IrpContext)
{
  NTSTATUS Status;

  DPRINT("RawFsCreate(IrpContext %x)\n", IrpContext);

  assert(IrpContext);
  
  if (RawFsIsRawFileSystemDeviceObject(IrpContext->DeviceObject))
    {
      /* DeviceObject represents FileSystem instead of logical volume */
      DPRINT("RawFsCreate() called with file system\n");
      IrpContext->Irp->IoStatus.Information = FILE_OPENED;
      IrpContext->Irp->IoStatus.Status = STATUS_SUCCESS;
      IoCompleteRequest(IrpContext->Irp, IO_DISK_INCREMENT);
      RawFsFreeIrpContext(IrpContext);
      return STATUS_SUCCESS;
    }

  if (!(IrpContext->Flags & IRPCONTEXT_CANWAIT))
    {
      return RawFsQueueRequest(IrpContext);
    }

  IrpContext->Irp->IoStatus.Information = 0;

  Status = RawFsCreateFile(IrpContext);

  IrpContext->Irp->IoStatus.Status = Status;
  IoCompleteRequest(IrpContext->Irp, 
    NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT);
  RawFsFreeIrpContext(IrpContext);

  return Status;
}

static NTSTATUS
RawFsRead(IN PRAWFS_IRP_CONTEXT IrpContext)
{
  DPRINT("RawFsRead(IrpContext %x)\n", IrpContext);
  UNIMPLEMENTED
  return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS
RawFsWrite(IN PRAWFS_IRP_CONTEXT IrpContext)
{
  DPRINT("RawFsWrite(IrpContext %x)\n", IrpContext);
  UNIMPLEMENTED
  return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS
RawFsMount(IN PRAWFS_IRP_CONTEXT IrpContext)
{
  PRAWFS_GLOBAL_DATA GlobalData;
  PDEVICE_OBJECT DeviceObject = NULL;
  PRAWFS_DEVICE_EXTENSION DeviceExt = NULL;
  PRAWFS_FCB VolumeFcb = NULL;
  PRAWFS_FCB Fcb = NULL;
  PARTITION_INFORMATION PartitionInfo;
  DISK_GEOMETRY DiskGeometry;
  LARGE_INTEGER VolumeSize;
  NTSTATUS Status;
  ULONG Size;

  DPRINT("RawFsMount(IrpContext %x)\n", IrpContext);

  assert(IrpContext);

  if (!RawFsIsRawFileSystemDeviceObject(IrpContext->DeviceObject))
    {
      Status = STATUS_INVALID_DEVICE_REQUEST;
      DPRINT("Not for me\n");
      goto ByeBye;
    }

  GlobalData = (PRAWFS_GLOBAL_DATA) IrpContext->DeviceObject->DeviceExtension;

  Status = IoCreateDevice(GlobalData->DriverObject,
    sizeof(RAWFS_DEVICE_EXTENSION),
    NULL,
    FILE_DEVICE_FILE_SYSTEM,
    0,
    FALSE,
    &DeviceObject);
  if (!NT_SUCCESS(Status))
    {
      goto ByeBye;
    }

  DeviceObject->Flags |= DO_DIRECT_IO;
  DeviceExt = (PVOID) DeviceObject->DeviceExtension;
  RtlZeroMemory(DeviceExt, sizeof(RAWFS_DEVICE_EXTENSION));
  
  /* Use same vpb as device disk */
  DeviceObject->Vpb = IrpContext->Stack->Parameters.MountVolume.DeviceObject->Vpb;
  DeviceExt->StorageDevice = IrpContext->Stack->Parameters.MountVolume.DeviceObject;
  DeviceExt->StorageDevice->Vpb->DeviceObject = DeviceObject;
  DeviceExt->StorageDevice->Vpb->RealDevice = DeviceExt->StorageDevice;
  DeviceExt->StorageDevice->Vpb->Flags |= VPB_MOUNTED;
  DeviceObject->StackSize = DeviceExt->StorageDevice->StackSize + 1;
  DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
  
  KeInitializeSpinLock(&DeviceExt->FcbListLock);
  InitializeListHead(&DeviceExt->FcbListHead);

  /* First try getting harddisk geometry then try getting CD-ROM geometry */
  Size = sizeof(DISK_GEOMETRY);
  Status = RawFsBlockDeviceIoControl(
    IrpContext->Stack->Parameters.MountVolume.DeviceObject,
    IOCTL_DISK_GET_DRIVE_GEOMETRY,
    NULL,
    0,
    &DiskGeometry,
    &Size);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("RawFsBlockDeviceIoControl failed with status 0x%.08x\n", Status);
      goto ByeBye;
    }
  if (DiskGeometry.MediaType == FixedMedia)
    {
      // We have found a hard disk
      Size = sizeof(PARTITION_INFORMATION);
      Status = RawFsBlockDeviceIoControl(
        IrpContext->Stack->Parameters.MountVolume.DeviceObject,
        IOCTL_DISK_GET_PARTITION_INFO,
        NULL,
        0,
        &PartitionInfo,
        &Size);
      if (!NT_SUCCESS(Status))
        {
          DPRINT("RawFsBlockDeviceIoControl() failed (%x)\n", Status);
          goto ByeBye;
        }
#ifndef NDEBUG
      DbgPrint("Partition Information:\n");
      DbgPrint("StartingOffset      %u\n", PartitionInfo.StartingOffset.QuadPart);
      DbgPrint("PartitionLength     %u\n", PartitionInfo.PartitionLength.QuadPart);
      DbgPrint("HiddenSectors       %u\n", PartitionInfo.HiddenSectors);
      DbgPrint("PartitionNumber     %u\n", PartitionInfo.PartitionNumber);
      DbgPrint("PartitionType       %u\n", PartitionInfo.PartitionType);
      DbgPrint("BootIndicator       %u\n", PartitionInfo.BootIndicator);
      DbgPrint("RecognizedPartition %u\n", PartitionInfo.RecognizedPartition);
      DbgPrint("RewritePartition    %u\n", PartitionInfo.RewritePartition);
#endif
      VolumeSize.QuadPart = PartitionInfo.PartitionLength.QuadPart;
    }
  else if (DiskGeometry.MediaType > Unknown && DiskGeometry.MediaType <= RemovableMedia)
    {
      Status = STATUS_UNRECOGNIZED_VOLUME;
      goto ByeBye;
    }

  VolumeFcb = RawFsNewFCB(GlobalData);
  if (VolumeFcb == NULL)
    {
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto ByeBye;
    }

  VolumeFcb->Flags = FCB_IS_VOLUME;
  VolumeFcb->RFCB.FileSize.QuadPart = VolumeSize.QuadPart;
  VolumeFcb->RFCB.ValidDataLength.QuadPart = VolumeFcb->RFCB.FileSize.QuadPart;
  VolumeFcb->RFCB.AllocationSize.QuadPart = VolumeFcb->RFCB.FileSize.QuadPart;
  DeviceExt->VolumeFcb = VolumeFcb;

  ExAcquireResourceExclusiveLite(&GlobalData->VolumeListLock, TRUE);
  InsertHeadList(&GlobalData->VolumeListHead, &DeviceExt->VolumeListEntry);
  ExReleaseResourceLite(&GlobalData->VolumeListLock);

  /* No serial number */
  DeviceObject->Vpb->SerialNumber = 0;

  /* Set volume label (no label) */
  *(DeviceObject->Vpb->VolumeLabel) = 0;
  DeviceObject->Vpb->VolumeLabelLength = 0;

  Status = STATUS_SUCCESS;

ByeBye:
  if (!NT_SUCCESS(Status))
    {
      DPRINT("RAWFS: RawFsMount() Status 0x%.08x\n", Status);

      if (Fcb)
        RawFsDestroyFCB(GlobalData, Fcb);
      if (DeviceObject)
        IoDeleteDevice(DeviceObject);
      if (VolumeFcb)
        RawFsDestroyFCB(GlobalData, VolumeFcb);
    }
  return Status;
}

static NTSTATUS
RawFsFileSystemControl(IN PRAWFS_IRP_CONTEXT IrpContext)
{
  NTSTATUS Status;

  DPRINT("RawFsFileSystemControl(IrpContext %x)\n", IrpContext);
  
  assert (IrpContext);

  switch (IrpContext->MinorFunction)
    {
      case IRP_MN_USER_FS_REQUEST:
        DPRINT("RawFs FSC: IRP_MN_USER_FS_REQUEST\n");
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;

      case IRP_MN_MOUNT_VOLUME:
        DPRINT("RawFs FSC: IRP_MN_MOUNT_VOLUME\n");
        Status = RawFsMount(IrpContext);
        break;

      case IRP_MN_VERIFY_VOLUME:
        DPRINT("RawFs FSC: IRP_MN_VERIFY_VOLUME\n");
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;

      default:
        DPRINT("RawFs FSC: MinorFunction %d\n", IrpContext->MinorFunction);
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

  IrpContext->Irp->IoStatus.Status = Status;
  IrpContext->Irp->IoStatus.Information = 0;

  IoCompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
  RawFsFreeIrpContext(IrpContext);
  return Status;
}

static NTSTATUS
RawFsQueryInformation(IN PRAWFS_IRP_CONTEXT IrpContext)
{
  DPRINT("RawFsQueryInformation(IrpContext %x)\n", IrpContext);
  UNIMPLEMENTED
  return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS
RawFsSetInformation(IN PRAWFS_IRP_CONTEXT IrpContext)
{
  DPRINT("RawFsSetInformation(IrpContext %x)\n", IrpContext);
  UNIMPLEMENTED
  return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS
RawFsDirectoryControl(IN PRAWFS_IRP_CONTEXT IrpContext)
{
  DPRINT("RawFsDirectoryControl(IrpContext %x)\n", IrpContext);
  UNIMPLEMENTED
  return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS
RawFsQueryVolumeInformation(IN PRAWFS_IRP_CONTEXT IrpContext)
{
  DPRINT("RawFsQueryVolumeInformation(IrpContext %x)\n", IrpContext);
  UNIMPLEMENTED
  return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS
RawFsSetVolumeInformation(IN PRAWFS_IRP_CONTEXT IrpContext)
{
  DPRINT("RawFsSetVolumeInformation(IrpContext %x)\n", IrpContext);
  UNIMPLEMENTED
  return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS
RawFsLockControl(IN PRAWFS_IRP_CONTEXT IrpContext)
{
  DPRINT("RawFsLockControl(IrpContext %x)\n", IrpContext);
  UNIMPLEMENTED
  return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS
RawFsCleanup(IN PRAWFS_IRP_CONTEXT IrpContext)
{
  DPRINT("RawFsCleanup(IrpContext %x)\n", IrpContext);
  UNIMPLEMENTED
  return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS
RawFsFlush(IN PRAWFS_IRP_CONTEXT IrpContext)
{
  DPRINT("RawFsFlush(IrpContext %x)\n", IrpContext);
  UNIMPLEMENTED
  return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS STDCALL
RawFsShutdown(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
  DPRINT("RawFsShutdown(DeviceObject %x, Irp %x)\n", DeviceObject, Irp);

  /*
   * Note: Do NOT call UNIMPLEMENTED here!
   * This function must finish in order to shutdown ReactOS properly!
   */

  return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS
RawFsDispatchRequest(IN PRAWFS_IRP_CONTEXT IrpContext)
{
  DPRINT("RawFsDispatchRequest(IrpContext %x), MajorFunction %x\n",
    IrpContext, IrpContext->MajorFunction);

  assert (IrpContext);

  switch (IrpContext->MajorFunction)
    {
      case IRP_MJ_CLOSE:
        return RawFsClose(IrpContext);
      case IRP_MJ_CREATE:
        return RawFsCreate (IrpContext);
      case IRP_MJ_READ:
        return RawFsRead (IrpContext);
      case IRP_MJ_WRITE:
        return RawFsWrite (IrpContext);
      case IRP_MJ_FILE_SYSTEM_CONTROL:
        return RawFsFileSystemControl(IrpContext);
      case IRP_MJ_QUERY_INFORMATION:
        return RawFsQueryInformation (IrpContext);
      case IRP_MJ_SET_INFORMATION:
        return RawFsSetInformation (IrpContext);
      case IRP_MJ_DIRECTORY_CONTROL:
        return RawFsDirectoryControl(IrpContext);
      case IRP_MJ_QUERY_VOLUME_INFORMATION:
        return RawFsQueryVolumeInformation(IrpContext);
      case IRP_MJ_SET_VOLUME_INFORMATION:
        return RawFsSetVolumeInformation(IrpContext);
      case IRP_MJ_LOCK_CONTROL:
        return RawFsLockControl(IrpContext);
      case IRP_MJ_CLEANUP:
        return RawFsCleanup(IrpContext);
      case IRP_MJ_FLUSH_BUFFERS:
        return RawFsFlush(IrpContext);
      default:
        DPRINT1("Unexpected major function %x\n", IrpContext->MajorFunction);
        IrpContext->Irp->IoStatus.Status = STATUS_DRIVER_INTERNAL_ERROR;
        IoCompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
        RawFsFreeIrpContext(IrpContext);
        return STATUS_DRIVER_INTERNAL_ERROR;
    }
}

static NTSTATUS STDCALL
RawFsBuildRequest(IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
{
  PRAWFS_IRP_CONTEXT IrpContext;
  NTSTATUS Status;

  DPRINT("RawFsBuildRequest(DeviceObject %x, Irp %x)\n", DeviceObject, Irp);

  assert(DeviceObject);
  assert(Irp);

  IrpContext = RawFsAllocateIrpContext(DeviceObject, Irp);
  if (IrpContext == NULL)
    {
      Status = STATUS_INSUFFICIENT_RESOURCES;
      Irp->IoStatus.Status = Status;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
  else
    {
      if (KeGetCurrentIrql() <= PASSIVE_LEVEL)
        {
          FsRtlEnterFileSystem();
        }
      else
        {
          DPRINT1("RawFs is entered at irql = %d\n", KeGetCurrentIrql());
        }
      Status = RawFsDispatchRequest(IrpContext);
      if (KeGetCurrentIrql() <= PASSIVE_LEVEL)
        {
          FsRtlExitFileSystem();
        }
    }
  return Status;
}

NTSTATUS STDCALL
RawFsDriverEntry(IN PDRIVER_OBJECT DriverObject,
  IN PUNICODE_STRING RegistryPath)
{
  PRAWFS_GLOBAL_DATA DeviceData;
  NTSTATUS Status;

  RawFsDriverObject = DriverObject;

  Status = IoCreateDevice(DriverObject,
    sizeof(RAWFS_GLOBAL_DATA),
    NULL,
    FILE_DEVICE_DISK_FILE_SYSTEM,
    0,
    FALSE,
    &DiskDeviceObject);
  if (!NT_SUCCESS(Status))
    {
      CPRINT("IoCreateDevice() failed with status 0x%.08x\n", Status);
      KeBugCheck(PHASE1_INITIALIZATION_FAILED);
      return(Status);
    }
  DeviceData = DiskDeviceObject->DeviceExtension;
  RtlZeroMemory(DeviceData, sizeof(RAWFS_GLOBAL_DATA));
  DeviceData->DriverObject = DriverObject;
  DeviceData->DeviceObject = DiskDeviceObject;
  DiskDeviceObject->Flags |= DO_DIRECT_IO;


  Status = IoCreateDevice(DriverObject,
    sizeof(RAWFS_GLOBAL_DATA),
    NULL,
    FILE_DEVICE_CD_ROM_FILE_SYSTEM,
    0,
    FALSE,
    &CdromDeviceObject);
  if (!NT_SUCCESS(Status))
    {
      CPRINT("IoCreateDevice() failed with status 0x%.08x\n", Status);
      KeBugCheck(PHASE1_INITIALIZATION_FAILED);
      return(Status);
    }
  DeviceData = CdromDeviceObject->DeviceExtension;
  RtlZeroMemory (DeviceData, sizeof(RAWFS_GLOBAL_DATA));
  DeviceData->DriverObject = DriverObject;
  DeviceData->DeviceObject = CdromDeviceObject;
  CdromDeviceObject->Flags |= DO_DIRECT_IO;


  Status = IoCreateDevice(DriverObject,
    sizeof(RAWFS_GLOBAL_DATA),
    NULL,
    FILE_DEVICE_TAPE_FILE_SYSTEM,
    0,
    FALSE,
    &TapeDeviceObject);
  if (!NT_SUCCESS(Status))
    {
      CPRINT("IoCreateDevice() failed with status 0x%.08x\n", Status);
      KeBugCheck(PHASE1_INITIALIZATION_FAILED);
      return(Status);
    }
  DeviceData = TapeDeviceObject->DeviceExtension;
  RtlZeroMemory (DeviceData, sizeof(RAWFS_GLOBAL_DATA));
  DeviceData->DriverObject = DriverObject;
  DeviceData->DeviceObject = TapeDeviceObject;
  TapeDeviceObject->Flags |= DO_DIRECT_IO;


  DriverObject->MajorFunction[IRP_MJ_CLOSE] = (PDRIVER_DISPATCH) RawFsBuildRequest;
  DriverObject->MajorFunction[IRP_MJ_CREATE] = (PDRIVER_DISPATCH) RawFsBuildRequest;
  DriverObject->MajorFunction[IRP_MJ_READ] = (PDRIVER_DISPATCH) RawFsBuildRequest;
  DriverObject->MajorFunction[IRP_MJ_WRITE] = (PDRIVER_DISPATCH) RawFsBuildRequest;
  DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = (PDRIVER_DISPATCH) RawFsBuildRequest;
  DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = (PDRIVER_DISPATCH) RawFsBuildRequest;
  DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = (PDRIVER_DISPATCH) RawFsBuildRequest;
  DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] = (PDRIVER_DISPATCH) RawFsBuildRequest;
  DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = (PDRIVER_DISPATCH) RawFsBuildRequest;
  DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION] = (PDRIVER_DISPATCH) RawFsBuildRequest;
  DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = (PDRIVER_DISPATCH) RawFsShutdown;
  DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL] = (PDRIVER_DISPATCH) RawFsBuildRequest;
  DriverObject->MajorFunction[IRP_MJ_CLEANUP] = (PDRIVER_DISPATCH) RawFsBuildRequest;
  DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = (PDRIVER_DISPATCH) RawFsBuildRequest;
  DriverObject->DriverUnload = NULL;

  ExInitializeNPagedLookasideList(&IrpContextLookasideList,
    NULL, NULL, 0, sizeof(RAWFS_IRP_CONTEXT), TAG_IRP, 0);


  IoRegisterFileSystem(DiskDeviceObject);
  IoRegisterFileSystem(CdromDeviceObject);
  IoRegisterFileSystem(TapeDeviceObject);

  return STATUS_SUCCESS;
}

/* EOF */
