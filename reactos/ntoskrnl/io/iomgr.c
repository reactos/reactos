/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/io/iomgr.c
 * PURPOSE:         I/O Manager Initialization and Misc Utility Functions
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
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

GENERIC_MAPPING IopFileMapping = {
    FILE_GENERIC_READ,
    FILE_GENERIC_WRITE,
    FILE_GENERIC_EXECUTE,
    FILE_ALL_ACCESS};

static KSPIN_LOCK CancelSpinLock;
extern LIST_ENTRY ShutdownListHead;
extern KSPIN_LOCK ShutdownListLock;

/* INIT FUNCTIONS ************************************************************/

VOID
INIT_FUNCTION
IoInitCancelHandling(VOID)
{
    KeInitializeSpinLock(&CancelSpinLock);
}

VOID
INIT_FUNCTION
IoInitShutdownNotification (VOID)
{
   InitializeListHead(&ShutdownListHead);
   KeInitializeSpinLock(&ShutdownListLock);
}

VOID
INIT_FUNCTION
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
  IopInitPnpNotificationImplementation();
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
        KEBUGCHECK(INACCESSIBLE_BOOT_DEVICE);
    }

    /* Read KDB Data */
    KdbInit();

    /* I/O is now setup for disk access, so phase 3 */
    KdInitSystem(3, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);

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

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID
STDCALL
IoAcquireCancelSpinLock(PKIRQL Irql)
{
   KeAcquireSpinLock(&CancelSpinLock,Irql);
}

/*
 * @implemented
 */
PVOID
STDCALL
IoGetInitialStack(VOID)
{
    return(PsGetCurrentThread()->Tcb.InitialStack);
}

/*
 * @implemented
 */
VOID
STDCALL
IoGetStackLimits(OUT PULONG LowLimit,
                 OUT PULONG HighLimit)
{
    *LowLimit = (ULONG)NtCurrentTeb()->Tib.StackLimit;
    *HighLimit = (ULONG)NtCurrentTeb()->Tib.StackBase;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
IoIsSystemThread(IN PETHREAD Thread)
{
    /* Call the Ps Function */
    return PsIsSystemThread(Thread);
}

/*
 * @implemented
 */
BOOLEAN STDCALL
IoIsWdmVersionAvailable(IN UCHAR MajorVersion,
                        IN UCHAR MinorVersion)
{
   if (MajorVersion <= 1 && MinorVersion <= 10)
      return TRUE;
   return FALSE;
}

/*
 * @implemented
 */
VOID
STDCALL
IoReleaseCancelSpinLock(KIRQL Irql)
{
   KeReleaseSpinLock(&CancelSpinLock,Irql);
}

/*
 * @implemented
 */
PEPROCESS
STDCALL
IoThreadToProcess(IN PETHREAD Thread)
{
    return(Thread->ThreadsProcess);
}

/* EOF */
