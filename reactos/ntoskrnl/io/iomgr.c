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

/* DATA ********************************************************************/

POBJECT_TYPE IoDeviceObjectType = NULL;
POBJECT_TYPE IoFileObjectType = NULL;
extern POBJECT_TYPE IoControllerObjectType;
ULONG IoReadOperationCount = 0;
LARGE_INTEGER IoReadTransferCount = {{0, 0}};
ULONG IoWriteOperationCount = 0;
LARGE_INTEGER IoWriteTransferCount = {{0, 0}};
ULONG IoOtherOperationCount = 0;
LARGE_INTEGER IoOtherTransferCount = {{0, 0}};
KSPIN_LOCK IoStatisticsLock = 0;

GENERIC_MAPPING IopFileMapping = {
    FILE_GENERIC_READ,
    FILE_GENERIC_WRITE,
    FILE_GENERIC_EXECUTE,
    FILE_ALL_ACCESS};

static KSPIN_LOCK CancelSpinLock;
extern LIST_ENTRY ShutdownListHead;
extern KSPIN_LOCK ShutdownListLock;
extern NPAGED_LOOKASIDE_LIST IoCompletionPacketLookaside;
extern POBJECT_TYPE IoAdapterObjectType;
NPAGED_LOOKASIDE_LIST IoLargeIrpLookaside;
NPAGED_LOOKASIDE_LIST IoSmallIrpLookaside;

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
IopInitLookasideLists(VOID)
{
    ULONG LargeIrpSize, SmallIrpSize;
    LONG i;
    PKPRCB Prcb;
    PNPAGED_LOOKASIDE_LIST CurrentList = NULL;

    /* Calculate the sizes */
    LargeIrpSize = sizeof(IRP) + (8 * sizeof(IO_STACK_LOCATION));
    SmallIrpSize = sizeof(IRP) + sizeof(IO_STACK_LOCATION);

    /* Initialize the Lookaside List for Large IRPs */
    ExInitializeNPagedLookasideList(&IoLargeIrpLookaside,
                                    NULL,
                                    NULL,
                                    0,
                                    LargeIrpSize,
                                    IO_LARGEIRP,
                                    0);

    /* Initialize the Lookaside List for Small IRPs */
    ExInitializeNPagedLookasideList(&IoSmallIrpLookaside,
                                    NULL,
                                    NULL,
                                    0,
                                    SmallIrpSize,
                                    IO_SMALLIRP,
                                    0);

    /* Initialize the Lookaside List for I\O Completion */
    ExInitializeNPagedLookasideList(&IoCompletionPacketLookaside,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(IO_COMPLETION_PACKET),
                                    IOC_TAG1,
                                    0);

    /* Now allocate the per-processor lists */
    for (i = 0; i < KeNumberProcessors; i++)
    {
        /* Get the PRCB for this CPU */
        Prcb = ((PKPCR)(KPCR_BASE + i * PAGE_SIZE))->Prcb;
        DPRINT("Setting up lookaside for CPU: %x, PRCB: %p\n", i, Prcb);

        /* Set the Large IRP List */
        Prcb->PPLookasideList[LookasideLargeIrpList].L = &IoLargeIrpLookaside.L;
        CurrentList = ExAllocatePoolWithTag(NonPagedPool,
                                            sizeof(NPAGED_LOOKASIDE_LIST),
                                            IO_LARGEIRP_CPU);
        if (CurrentList)
        {
            /* Initialize the Lookaside List for Large IRPs */
            ExInitializeNPagedLookasideList(CurrentList,
                                            NULL,
                                            NULL,
                                            0,
                                            LargeIrpSize,
                                            IO_LARGEIRP_CPU,
                                            0);
        }
        else
        {
            CurrentList = &IoLargeIrpLookaside;
        }
        Prcb->PPLookasideList[LookasideLargeIrpList].P = &CurrentList->L;

        /* Set the Small IRP List */
        Prcb->PPLookasideList[LookasideSmallIrpList].L = &IoSmallIrpLookaside.L;
        CurrentList = ExAllocatePoolWithTag(NonPagedPool,
                                            sizeof(NPAGED_LOOKASIDE_LIST),
                                            IO_SMALLIRP_CPU);
        if (CurrentList)
        {
            /* Initialize the Lookaside List for Large IRPs */
            ExInitializeNPagedLookasideList(CurrentList,
                                            NULL,
                                            NULL,
                                            0,
                                            SmallIrpSize,
                                            IO_SMALLIRP_CPU,
                                            0);
        }
        else
        {
            CurrentList = &IoSmallIrpLookaside;
        }
        Prcb->PPLookasideList[LookasideSmallIrpList].P = &CurrentList->L;

        /* Set the I/O Completion List */
        Prcb->PPLookasideList[LookasideCompletionList].L = &IoCompletionPacketLookaside.L;
        CurrentList = ExAllocatePoolWithTag(NonPagedPool,
                                            sizeof(NPAGED_LOOKASIDE_LIST),
                                            IO_SMALLIRP_CPU);
        if (CurrentList)
        {
            /* Initialize the Lookaside List for Large IRPs */
            ExInitializeNPagedLookasideList(CurrentList,
                                            NULL,
                                            NULL,
                                            0,
                                            SmallIrpSize,
                                            IOC_CPU,
                                            0);
        }
        else
        {
            CurrentList = &IoCompletionPacketLookaside;
        }
        Prcb->PPLookasideList[LookasideCompletionList].P = &CurrentList->L;
    }

    DPRINT("Done allocation\n");
}

VOID
INIT_FUNCTION
IoInit (VOID)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DirName;
    UNICODE_STRING LinkName = RTL_CONSTANT_STRING(L"\\DosDevices");
    HANDLE Handle;

    IopInitDriverImplementation();

    DPRINT("Creating Device Object Type\n");

    /* Initialize the Driver object type  */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"Device");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(DEVICE_OBJECT);
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.ValidAccessMask = FILE_ALL_ACCESS;
    ObjectTypeInitializer.UseDefaultObject = TRUE;
    ObjectTypeInitializer.GenericMapping = IopFileMapping;
    ObpCreateTypeObject(&ObjectTypeInitializer, &Name, &IoDeviceObjectType);

    /* Do the Adapter Type */
    RtlInitUnicodeString(&Name, L"Adapter");
    ObpCreateTypeObject(&ObjectTypeInitializer, &Name, &IoAdapterObjectType);

    /* Do the Controller Type */
    RtlInitUnicodeString(&Name, L"Controller");
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(CONTROLLER_OBJECT);
    ObpCreateTypeObject(&ObjectTypeInitializer, &Name, &IoControllerObjectType);

    /* Initialize the File object type  */
    RtlInitUnicodeString(&Name, L"File");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(FILE_OBJECT);
    ObjectTypeInitializer.CloseProcedure = IopCloseFile;
    ObjectTypeInitializer.DeleteProcedure = IopDeleteFile;
    ObjectTypeInitializer.SecurityProcedure = IopSecurityFile;
    ObjectTypeInitializer.QueryNameProcedure = IopQueryNameFile;
    ObpCreateTypeObject(&ObjectTypeInitializer, &Name, &IoFileObjectType);

  /*
   * Create the '\Driver' object directory
   */
  RtlInitUnicodeString(&DirName, L"\\Driver");
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
  RtlInitUnicodeString(&DirName,
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
  RtlInitUnicodeString(&DirName,
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
  RtlInitUnicodeString(&DirName,
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
  RtlInitUnicodeString(&DirName,
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
  IopInitLookasideLists();

  /*
   * Create link from '\DosDevices' to '\??' directory
   */
  RtlInitUnicodeString(&DirName,
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

  PnpInit2();

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

  Status = IopStartDevice(DeviceNode);
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
    DPRINT("CommandLine: %s\n", (PCHAR)KeLoaderBlock.CommandLine);
    Status = IoCreateSystemRootLink((PCHAR)KeLoaderBlock.CommandLine);
    if (!NT_SUCCESS(Status)) {
        CPRINT("IoCreateSystemRootLink FAILED: (0x%x) - ", Status);
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
