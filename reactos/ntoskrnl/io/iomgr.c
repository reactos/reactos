/* $Id: iomgr.c,v 1.34 2003/06/02 10:02:56 ekohl Exp $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 ntoskrnl/io/iomgr.c
 * PURPOSE:              Initializes the io manager
 * PROGRAMMER:           David Welch (welch@mcmail.com)
 * REVISION HISTORY:
 *             29/07/98: Created
 */

/* INCLUDES ****************************************************************/

#include <limits.h>
#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/io.h>
#include <internal/pool.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define TAG_DEVICE_TYPE     TAG('D', 'E', 'V', 'T')
#define TAG_FILE_TYPE       TAG('F', 'I', 'L', 'E')

/* DATA ********************************************************************/


POBJECT_TYPE EXPORTED IoDeviceObjectType = NULL;
POBJECT_TYPE EXPORTED IoFileObjectType = NULL;
ULONG        EXPORTED IoReadOperationCount = 0;	/* FIXME: unknown type */
ULONG        EXPORTED IoReadTransferCount = 0;	/* FIXME: unknown type */
ULONG        EXPORTED IoWriteOperationCount = 0; /* FIXME: unknown type */
ULONG        EXPORTED IoWriteTransferCount = 0;	/* FIXME: unknown type */
ULONG        EXPORTED IoStatisticsLock = 0;	/* FIXME: unknown type */

static GENERIC_MAPPING IopFileMapping = {FILE_GENERIC_READ,
					 FILE_GENERIC_WRITE,
					 FILE_GENERIC_EXECUTE,
					 FILE_ALL_ACCESS};

/* FUNCTIONS ****************************************************************/

VOID STDCALL
IopCloseFile(PVOID ObjectBody,
	     ULONG HandleCount)
{
   PFILE_OBJECT FileObject = (PFILE_OBJECT)ObjectBody;
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   NTSTATUS Status;
   
   DPRINT("IopCloseFile()\n");
   
   if (HandleCount > 0 || FileObject->DeviceObject == NULL)
     {
	return;
     }

#if 0
//NOTE: Allmost certain that the latest changes to I/O Mgr makes this redundant (OriginalFileObject case)
   ObReferenceObjectByPointer(FileObject,
			      STANDARD_RIGHTS_REQUIRED,
			      IoFileObjectType,
			      UserMode);
#endif

   KeResetEvent( &FileObject->Event );
  
   Irp = IoBuildSynchronousFsdRequest(IRP_MJ_CLEANUP,
				      FileObject->DeviceObject,
				      NULL,
				      0,
				      NULL,
				      NULL,
				      NULL);
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->FileObject = FileObject;
   
   Status = IoCallDriver(FileObject->DeviceObject, Irp);
   if (Status == STATUS_PENDING)
   {
      KeWaitForSingleObject(&FileObject->Event, Executive, KernelMode, FALSE, NULL);
   }
}


VOID STDCALL
IopDeleteFile(PVOID ObjectBody)
{
   PFILE_OBJECT FileObject = (PFILE_OBJECT)ObjectBody;
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   NTSTATUS Status;
   
   DPRINT("IopDeleteFile()\n");

   if (FileObject->DeviceObject)
   {
#if 0
//NOTE: Allmost certain that the latest changes to I/O Mgr makes this redundant (OriginalFileObject case)
     
     ObReferenceObjectByPointer(ObjectBody,
			        STANDARD_RIGHTS_REQUIRED,
			        IoFileObjectType,
			        UserMode);
#endif   
     KeResetEvent( &FileObject->Event );
     Irp = IoBuildSynchronousFsdRequest(IRP_MJ_CLOSE,
				        FileObject->DeviceObject,
				        NULL,
				        0,
				        NULL,
				        NULL,
				        NULL);
     StackPtr = IoGetNextIrpStackLocation(Irp);
     StackPtr->FileObject = FileObject;
   
     Status = IoCallDriver(FileObject->DeviceObject, Irp);
     if (Status == STATUS_PENDING)
     {
        KeWaitForSingleObject(&FileObject->Event, Executive, KernelMode, FALSE, NULL);
     }
   }
   
   if (FileObject->FileName.Buffer != NULL)
     {
	ExFreePool(FileObject->FileName.Buffer);
	FileObject->FileName.Buffer = 0;
     }
}


NTSTATUS STDCALL
IopQueryNameFile(PVOID ObjectBody,
		 POBJECT_NAME_INFORMATION ObjectNameInfo,
		 ULONG Length,
		 PULONG ReturnLength)
{
  POBJECT_NAME_INFORMATION LocalInfo;
  PFILE_NAME_INFORMATION FileNameInfo;
  PFILE_OBJECT FileObject;
  ULONG LocalReturnLength;
  NTSTATUS Status;

  DPRINT ("IopQueryNameFile() called\n");

  FileObject = (PFILE_OBJECT)ObjectBody;

  LocalInfo = ExAllocatePool (NonPagedPool,
			      sizeof(OBJECT_NAME_INFORMATION) +
				MAX_PATH * sizeof(WCHAR));
  if (LocalInfo == NULL)
    return STATUS_INSUFFICIENT_RESOURCES;

  Status = ObQueryNameString (FileObject->DeviceObject->Vpb->RealDevice,
			      LocalInfo,
			      MAX_PATH * sizeof(WCHAR),
			      &LocalReturnLength);
  if (!NT_SUCCESS (Status))
    {
      ExFreePool (LocalInfo);
      return Status;
    }
  DPRINT ("Device path: %wZ\n", &LocalInfo->Name);

  Status = RtlAppendUnicodeStringToString (&ObjectNameInfo->Name,
					   &LocalInfo->Name);

  ExFreePool (LocalInfo);

  FileNameInfo = ExAllocatePool (NonPagedPool,
				 MAX_PATH * sizeof(WCHAR) + sizeof(ULONG));
  if (FileNameInfo == NULL)
    return STATUS_INSUFFICIENT_RESOURCES;

  Status = IoQueryFileInformation (FileObject,
				   FileNameInformation,
				   MAX_PATH * sizeof(WCHAR) + sizeof(ULONG),
				   FileNameInfo,
				   NULL);
  if (Status != STATUS_SUCCESS)
    {
      ExFreePool (FileNameInfo);
      return Status;
    }

  Status = RtlAppendUnicodeToString (&ObjectNameInfo->Name,
				     FileNameInfo->FileName);

  DPRINT ("Total path: %wZ\n", &ObjectNameInfo->Name);

  ExFreePool (FileNameInfo);

  return Status;
}


VOID IoInit (VOID)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING DirName;
  UNICODE_STRING LinkName;
  HANDLE Handle;

  IopInitDriverImplementation();
  
  /*
   * Register iomgr types: DeviceObjectType
   */
  IoDeviceObjectType = ExAllocatePool (NonPagedPool,
				       sizeof (OBJECT_TYPE));
  
  IoDeviceObjectType->Tag = TAG_DEVICE_TYPE;
  IoDeviceObjectType->TotalObjects = 0;
  IoDeviceObjectType->TotalHandles = 0;
  IoDeviceObjectType->MaxObjects = ULONG_MAX;
  IoDeviceObjectType->MaxHandles = ULONG_MAX;
  IoDeviceObjectType->PagedPoolCharge = 0;
  IoDeviceObjectType->NonpagedPoolCharge = sizeof (DEVICE_OBJECT);
  IoDeviceObjectType->Mapping = &IopFileMapping;
  IoDeviceObjectType->Dump = NULL;
  IoDeviceObjectType->Open = NULL;
  IoDeviceObjectType->Close = NULL;
  IoDeviceObjectType->Delete = NULL;
  IoDeviceObjectType->Parse = NULL;
  IoDeviceObjectType->Security = NULL;
  IoDeviceObjectType->QueryName = NULL;
  IoDeviceObjectType->OkayToClose = NULL;
  IoDeviceObjectType->Create = IopCreateDevice;
  IoDeviceObjectType->DuplicationNotify = NULL;
  
  RtlInitUnicodeStringFromLiteral(&IoDeviceObjectType->TypeName, L"Device");

  /*
   * Register iomgr types: FileObjectType
   * (alias DriverObjectType)
   */
  IoFileObjectType = ExAllocatePool (NonPagedPool, sizeof (OBJECT_TYPE));
  
  IoFileObjectType->Tag = TAG_FILE_TYPE;
  IoFileObjectType->TotalObjects = 0;
  IoFileObjectType->TotalHandles = 0;
  IoFileObjectType->MaxObjects = ULONG_MAX;
  IoFileObjectType->MaxHandles = ULONG_MAX;
  IoFileObjectType->PagedPoolCharge = 0;
  IoFileObjectType->NonpagedPoolCharge = sizeof(FILE_OBJECT);
  IoFileObjectType->Mapping = &IopFileMapping;
  IoFileObjectType->Dump = NULL;
  IoFileObjectType->Open = NULL;
  IoFileObjectType->Close = IopCloseFile;
  IoFileObjectType->Delete = IopDeleteFile;
  IoFileObjectType->Parse = NULL;
  IoFileObjectType->Security = NULL;
  IoFileObjectType->QueryName = IopQueryNameFile;
  IoFileObjectType->OkayToClose = NULL;
  IoFileObjectType->Create = IopCreateFile;
  IoFileObjectType->DuplicationNotify = NULL;
  
  RtlInitUnicodeStringFromLiteral(&IoFileObjectType->TypeName, L"File");

  /*
   * Create the '\Driver' object directory
   */
  RtlInitUnicodeStringFromLiteral(&DirName, L"\\Driver");
  InitializeObjectAttributes(&ObjectAttributes,
			     &DirName,
			     0,
			     NULL,
			     NULL);
  NtCreateDirectoryObject(&Handle,
			  0,
			  &ObjectAttributes);

  /*
   * Create the '\FileSystem' object directory
   */
  RtlInitUnicodeStringFromLiteral(&DirName,
		       L"\\FileSystem");
  InitializeObjectAttributes(&ObjectAttributes,
			     &DirName,
			     0,
			     NULL,
			     NULL);
  NtCreateDirectoryObject(&Handle,
			  0,
			  &ObjectAttributes);

  /*
   * Create the '\Device' directory
   */
  RtlInitUnicodeStringFromLiteral(&DirName,
		       L"\\Device");
  InitializeObjectAttributes(&ObjectAttributes,
			     &DirName,
			     0,
			     NULL,
			     NULL);
  ZwCreateDirectoryObject(&Handle,
			  0,
			  &ObjectAttributes);

  /*
   * Create the '\??' directory
   */
  RtlInitUnicodeStringFromLiteral(&DirName,
		       L"\\??");
  InitializeObjectAttributes(&ObjectAttributes,
			     &DirName,
			     0,
			     NULL,
			     NULL);
  ZwCreateDirectoryObject(&Handle,
			  0,
			  &ObjectAttributes);

  /*
   * Create the '\ArcName' directory
   */
  RtlInitUnicodeStringFromLiteral(&DirName,
		       L"\\ArcName");
  InitializeObjectAttributes(&ObjectAttributes,
			     &DirName,
			     0,
			     NULL,
			     NULL);
  ZwCreateDirectoryObject(&Handle,
			  0,
			  &ObjectAttributes);

  /*
   * Initialize remaining subsubsystem
   */
  IoInitCancelHandling();
  IoInitFileSystemImplementation();
  IoInitVpbImplementation();
  IoInitShutdownNotification();

  /*
   * Create link from '\DosDevices' to '\??' directory
   */
  RtlInitUnicodeStringFromLiteral(&LinkName,
		       L"\\DosDevices");
  RtlInitUnicodeStringFromLiteral(&DirName,
		       L"\\??");
  IoCreateSymbolicLink(&LinkName,
		       &DirName);

  /*
   * Initialize PnP manager
   */
  PnpInit();
}

VOID IoInit2(VOID)
{
  PDEVICE_NODE DeviceNode;
  NTSTATUS Status;

  /* Initialize raw filesystem driver */

  /* Use IopRootDeviceNode for now */
  Status = IopCreateDeviceNode(IopRootDeviceNode,
    NULL,
    &DeviceNode);
  if (!NT_SUCCESS(Status))
    {
      CPRINT("IopCreateDeviceNode() failed with status (%x)\n", Status);
      return;
    }

  Status = IopInitializeDriver(RawFsDriverEntry,
    DeviceNode,
    TRUE);
  if (!NT_SUCCESS(Status))
    {
      IopFreeDeviceNode(DeviceNode);
      CPRINT("IopInitializeDriver() failed with status (%x)\n", Status);
      return;
    }
}

PGENERIC_MAPPING STDCALL
IoGetFileObjectGenericMapping(VOID)
{
  return(&IopFileMapping);
}

/* EOF */
