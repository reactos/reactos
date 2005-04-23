/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/iomgr.c
 * PURPOSE:         Initializes the io manager
 * 
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#include "../dbg/kdb.h"
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define TAG_DEVICE_TYPE     TAG('D', 'E', 'V', 'T')
#define TAG_FILE_TYPE       TAG('F', 'I', 'L', 'E')
#define TAG_ADAPTER_TYPE    TAG('A', 'D', 'P', 'T')

/* DATA ********************************************************************/


POBJECT_TYPE EXPORTED IoDeviceObjectType = NULL;
POBJECT_TYPE EXPORTED IoFileObjectType = NULL;
ULONG        EXPORTED IoReadOperationCount = 0;
ULONGLONG    EXPORTED IoReadTransferCount = 0;
ULONG        EXPORTED IoWriteOperationCount = 0;
ULONGLONG    EXPORTED IoWriteTransferCount = 0;
ULONG                 IoOtherOperationCount = 0;
ULONGLONG             IoOtherTransferCount = 0;
KSPIN_LOCK   EXPORTED IoStatisticsLock = 0;

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
   
   if (HandleCount > 1 || FileObject->DeviceObject == NULL)
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
				      &FileObject->Event,
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

     Irp = IoAllocateIrp(FileObject->DeviceObject->StackSize, TRUE);
     if (Irp == NULL)
     {
        /*
         * FIXME: This case should eventually be handled. We should wait
         * until enough memory is available to allocate the IRP.
         */
        ASSERT(FALSE);
     }
   
     Irp->UserEvent = &FileObject->Event;
     Irp->Tail.Overlay.Thread = PsGetCurrentThread();
     Irp->Flags |= IRP_CLOSE_OPERATION;
   
     StackPtr = IoGetNextIrpStackLocation(Irp);
     StackPtr->MajorFunction = IRP_MJ_CLOSE;
     StackPtr->DeviceObject = FileObject->DeviceObject;
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


static NTSTATUS
IopSetDefaultSecurityDescriptor(SECURITY_INFORMATION SecurityInformation,
				PSECURITY_DESCRIPTOR SecurityDescriptor,
				PULONG BufferLength)
{
  ULONG_PTR Current;
  ULONG SidSize;
  ULONG SdSize;
  NTSTATUS Status;

  DPRINT("IopSetDefaultSecurityDescriptor() called\n");

  if (SecurityInformation == 0)
    {
      return STATUS_ACCESS_DENIED;
    }

  SidSize = RtlLengthSid(SeWorldSid);
  SdSize = sizeof(SECURITY_DESCRIPTOR) + (2 * SidSize);

  if (*BufferLength < SdSize)
    {
      *BufferLength = SdSize;
      return STATUS_BUFFER_TOO_SMALL;
    }

  *BufferLength = SdSize;

  Status = RtlCreateSecurityDescriptor(SecurityDescriptor,
				       SECURITY_DESCRIPTOR_REVISION);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  SecurityDescriptor->Control |= SE_SELF_RELATIVE;
  Current = (ULONG_PTR)SecurityDescriptor + sizeof(SECURITY_DESCRIPTOR);

  if (SecurityInformation & OWNER_SECURITY_INFORMATION)
    {
      RtlCopyMemory((PVOID)Current,
		    SeWorldSid,
		    SidSize);
      SecurityDescriptor->Owner = (PSID)((ULONG_PTR)Current - (ULONG_PTR)SecurityDescriptor);
      Current += SidSize;
    }

  if (SecurityInformation & GROUP_SECURITY_INFORMATION)
    {
      RtlCopyMemory((PVOID)Current,
		    SeWorldSid,
		    SidSize);
      SecurityDescriptor->Group = (PSID)((ULONG_PTR)Current - (ULONG_PTR)SecurityDescriptor);
      Current += SidSize;
    }

  if (SecurityInformation & DACL_SECURITY_INFORMATION)
    {
      SecurityDescriptor->Control |= SE_DACL_PRESENT;
    }

  if (SecurityInformation & SACL_SECURITY_INFORMATION)
    {
      SecurityDescriptor->Control |= SE_SACL_PRESENT;
    }

  return STATUS_SUCCESS;
}


NTSTATUS STDCALL
IopSecurityFile(PVOID ObjectBody,
		SECURITY_OPERATION_CODE OperationCode,
		SECURITY_INFORMATION SecurityInformation,
		PSECURITY_DESCRIPTOR SecurityDescriptor,
		PULONG BufferLength)
{
  IO_STATUS_BLOCK IoStatusBlock;
  PIO_STACK_LOCATION StackPtr;
  PFILE_OBJECT FileObject;
  PIRP Irp;
  NTSTATUS Status;

  DPRINT("IopSecurityFile() called\n");

  FileObject = (PFILE_OBJECT)ObjectBody;

  switch (OperationCode)
    {
      case SetSecurityDescriptor:
	DPRINT("Set security descriptor\n");
	KeResetEvent(&FileObject->Event);
	Irp = IoBuildSynchronousFsdRequest(IRP_MJ_SET_SECURITY,
					   FileObject->DeviceObject,
					   NULL,
					   0,
					   NULL,
					   &FileObject->Event,
					   &IoStatusBlock);

	StackPtr = IoGetNextIrpStackLocation(Irp);
	StackPtr->FileObject = FileObject;

	StackPtr->Parameters.SetSecurity.SecurityInformation = SecurityInformation;
	StackPtr->Parameters.SetSecurity.SecurityDescriptor = SecurityDescriptor;

	Status = IoCallDriver(FileObject->DeviceObject, Irp);
	if (Status == STATUS_PENDING)
	  {
	    KeWaitForSingleObject(&FileObject->Event,
				  Executive,
				  KernelMode,
				  FALSE,
				  NULL);
	    Status = IoStatusBlock.Status;
	  }

	if (Status == STATUS_INVALID_DEVICE_REQUEST)
	  {
	    Status = STATUS_SUCCESS;
	  }
	return Status;

      case QuerySecurityDescriptor:
	DPRINT("Query security descriptor\n");
	KeResetEvent(&FileObject->Event);
	Irp = IoBuildSynchronousFsdRequest(IRP_MJ_QUERY_SECURITY,
					   FileObject->DeviceObject,
					   NULL,
					   0,
					   NULL,
					   &FileObject->Event,
					   &IoStatusBlock);

	Irp->UserBuffer = SecurityDescriptor;

	StackPtr = IoGetNextIrpStackLocation(Irp);
	StackPtr->FileObject = FileObject;

	StackPtr->Parameters.QuerySecurity.SecurityInformation = SecurityInformation;
	StackPtr->Parameters.QuerySecurity.Length = *BufferLength;

	Status = IoCallDriver(FileObject->DeviceObject, Irp);
	if (Status == STATUS_PENDING)
	  {
	    KeWaitForSingleObject(&FileObject->Event,
				  Executive,
				  KernelMode,
				  FALSE,
				  NULL);
	    Status = IoStatusBlock.Status;
	  }

	if (Status == STATUS_INVALID_DEVICE_REQUEST)
	  {
	    Status = IopSetDefaultSecurityDescriptor(SecurityInformation,
						     SecurityDescriptor,
						     BufferLength);
	  }
	else
	  {
	    /* FIXME: Is this correct?? */
	    *BufferLength = IoStatusBlock.Information;
	  }
	return Status;

      case DeleteSecurityDescriptor:
	DPRINT("Delete security descriptor\n");
	return STATUS_SUCCESS;

      case AssignSecurityDescriptor:
	DPRINT("Assign security descriptor\n");
	return STATUS_SUCCESS;
    }

  return STATUS_UNSUCCESSFUL;
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


VOID INIT_FUNCTION
IoInit (VOID)
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
  IoDeviceObjectType->PeakObjects = 0;
  IoDeviceObjectType->PeakHandles = 0;
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
  IoDeviceObjectType->Create = NULL;
  IoDeviceObjectType->DuplicationNotify = NULL;
  
  RtlInitUnicodeString(&IoDeviceObjectType->TypeName, L"Device");

  ObpCreateTypeObject(IoDeviceObjectType);

  /*
   * Register iomgr types: FileObjectType
   * (alias DriverObjectType)
   */
  IoFileObjectType = ExAllocatePool (NonPagedPool, sizeof (OBJECT_TYPE));
  
  IoFileObjectType->Tag = TAG_FILE_TYPE;
  IoFileObjectType->TotalObjects = 0;
  IoFileObjectType->TotalHandles = 0;
  IoFileObjectType->PeakObjects = 0;
  IoFileObjectType->PeakHandles = 0;
  IoFileObjectType->PagedPoolCharge = 0;
  IoFileObjectType->NonpagedPoolCharge = sizeof(FILE_OBJECT);
  IoFileObjectType->Mapping = &IopFileMapping;
  IoFileObjectType->Dump = NULL;
  IoFileObjectType->Open = NULL;
  IoFileObjectType->Close = IopCloseFile;
  IoFileObjectType->Delete = IopDeleteFile;
  IoFileObjectType->Parse = NULL;
  IoFileObjectType->Security = IopSecurityFile;
  IoFileObjectType->QueryName = IopQueryNameFile;
  IoFileObjectType->OkayToClose = NULL;
  IoFileObjectType->Create = IopCreateFile;
  IoFileObjectType->DuplicationNotify = NULL;
  
  RtlInitUnicodeString(&IoFileObjectType->TypeName, L"File");

  ObpCreateTypeObject(IoFileObjectType);
  
    /*
   * Register iomgr types: AdapterObjectType
   */
  IoAdapterObjectType = ExAllocatePool (NonPagedPool,
				       sizeof (OBJECT_TYPE));
  RtlZeroMemory(IoAdapterObjectType, sizeof(OBJECT_TYPE));
  IoAdapterObjectType->Tag = TAG_ADAPTER_TYPE;
  IoAdapterObjectType->PeakObjects = 0;
  IoAdapterObjectType->PeakHandles = 0;
  IoDeviceObjectType->Mapping = &IopFileMapping;
  RtlInitUnicodeString(&IoAdapterObjectType->TypeName, L"Adapter");
  ObpCreateTypeObject(IoAdapterObjectType);

  /*
   * Create the '\Driver' object directory
   */
  RtlRosInitUnicodeStringFromLiteral(&DirName, L"\\Driver");
  InitializeObjectAttributes(&ObjectAttributes,
			     &DirName,
			     0,
			     NULL,
			     NULL);
  ZwCreateDirectoryObject(&Handle,
			  0,
			  &ObjectAttributes);

  /*
   * Create the '\FileSystem' object directory
   */
  RtlRosInitUnicodeStringFromLiteral(&DirName,
		       L"\\FileSystem");
  InitializeObjectAttributes(&ObjectAttributes,
			     &DirName,
			     0,
			     NULL,
			     NULL);
  ZwCreateDirectoryObject(&Handle,
			  0,
			  &ObjectAttributes);

  /*
   * Create the '\Device' directory
   */
  RtlRosInitUnicodeStringFromLiteral(&DirName,
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
  RtlRosInitUnicodeStringFromLiteral(&DirName,
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
  RtlRosInitUnicodeStringFromLiteral(&DirName,
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
  IopInitErrorLog();
  IopInitTimerImplementation();
  IopInitIoCompletionImplementation();

  /*
   * Create link from '\DosDevices' to '\??' directory
   */
  RtlRosInitUnicodeStringFromLiteral(&LinkName,
		       L"\\DosDevices");
  RtlRosInitUnicodeStringFromLiteral(&DirName,
		       L"\\??");
  IoCreateSymbolicLink(&LinkName,
		       &DirName);

  /*
   * Initialize PnP manager
   */
  PnpInit();
}


VOID 
INIT_FUNCTION
IoInit2(BOOLEAN BootLog)
{
  PDEVICE_NODE DeviceNode;
  PDRIVER_OBJECT DriverObject;
  MODULE_OBJECT ModuleObject;
  NTSTATUS Status;

  IoCreateDriverList();
          
  KeInitializeSpinLock (&IoStatisticsLock);

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

  ModuleObject.Base = NULL;
  ModuleObject.Length = 0;
  ModuleObject.EntryPoint = RawFsDriverEntry;

  Status = IopInitializeDriverModule(
    DeviceNode,
    &ModuleObject,
    &DeviceNode->ServiceName,
    TRUE,
    &DriverObject);
  if (!NT_SUCCESS(Status))
    {
      IopFreeDeviceNode(DeviceNode);
      CPRINT("IopInitializeDriver() failed with status (%x)\n", Status);
      return;
    }

  Status = IopInitializeDevice(DeviceNode, DriverObject);
  if (!NT_SUCCESS(Status))
    {
      IopFreeDeviceNode(DeviceNode);
      CPRINT("IopInitializeDevice() failed with status (%x)\n", Status);
      return;
    }

  /*
   * Initialize PnP root releations
   */
  IopInvalidateDeviceRelations(
    IopRootDeviceNode,
    BusRelations);
  
     /* Start boot logging */
    IopInitBootLog(BootLog);
  
    /* Load boot start drivers */
    IopInitializeBootDrivers();
}

VOID
STDCALL
INIT_FUNCTION
IoInit3(VOID)
{
    NTSTATUS Status;
    
    /* Create ARC names for boot devices */
    IoCreateArcNames();

    /* Create the SystemRoot symbolic link */
    CPRINT("CommandLine: %s\n", (PCHAR)KeLoaderBlock.CommandLine);
    Status = IoCreateSystemRootLink((PCHAR)KeLoaderBlock.CommandLine);
    if (!NT_SUCCESS(Status)) {
        DbgPrint("IoCreateSystemRootLink FAILED: (0x%x) - ", Status);
        DbgPrintErrorMessage (Status);
        KEBUGCHECK(INACCESSIBLE_BOOT_DEVICE);
    }

    /* Start Profiling on a Debug Build */
#if defined(KDBG)
    KdbInit();
#endif /* KDBG */

    /* I/O is now setup for disk access, so start the debugging logger thread. */
    if (KdDebugState & KD_DEBUG_FILELOG) DebugLogInit2();

    /* Load services for devices found by PnP manager */
    IopInitializePnpServices(IopRootDeviceNode, FALSE);

    /* Load system start drivers */
    IopInitializeSystemDrivers();
    IoDestroyDriverList();

    /* Stop boot logging */
    IopStopBootLog();

    /* Assign drive letters */
    IoAssignDriveLetters((PLOADER_PARAMETER_BLOCK)&KeLoaderBlock,
                         NULL,
                         NULL,
                         NULL);    
}

/*
 * @implemented
 */
PGENERIC_MAPPING STDCALL
IoGetFileObjectGenericMapping(VOID)
{
  return(&IopFileMapping);
}

/* EOF */
