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

ULONG IopTraceLevel = IO_IRP_DEBUG;

// should go into a proper header
VOID
NTAPI
IoSynchronousInvalidateDeviceRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN DEVICE_RELATION_TYPE Type
);


/* DATA ********************************************************************/

POBJECT_TYPE IoDeviceObjectType = NULL;
POBJECT_TYPE IoFileObjectType = NULL;
extern POBJECT_TYPE IoControllerObjectType;
BOOLEAN IoCountOperations;
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

extern LIST_ENTRY ShutdownListHead;
extern KSPIN_LOCK ShutdownListLock;
extern NPAGED_LOOKASIDE_LIST IoCompletionPacketLookaside;
extern POBJECT_TYPE IoAdapterObjectType;
NPAGED_LOOKASIDE_LIST IoLargeIrpLookaside;
NPAGED_LOOKASIDE_LIST IoSmallIrpLookaside;
NPAGED_LOOKASIDE_LIST IopMdlLookasideList;

VOID INIT_FUNCTION IopInitLookasideLists(VOID);

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, IoInitCancelHandling)
#pragma alloc_text(INIT, IoInitShutdownNotification)
#pragma alloc_text(INIT, IopInitLookasideLists)
#pragma alloc_text(INIT, IoInit)
#pragma alloc_text(INIT, IoInit2)
#pragma alloc_text(INIT, IoInit3)
#endif

/* INIT FUNCTIONS ************************************************************/

VOID
INIT_FUNCTION
IoInitCancelHandling(VOID)
{
    extern KSPIN_LOCK CancelSpinLock;
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
    ULONG LargeIrpSize, SmallIrpSize, MdlSize;
    LONG i;
    PKPRCB Prcb;
    PNPAGED_LOOKASIDE_LIST CurrentList = NULL;

    /* Calculate the sizes */
    LargeIrpSize = sizeof(IRP) + (8 * sizeof(IO_STACK_LOCATION));
    SmallIrpSize = sizeof(IRP) + sizeof(IO_STACK_LOCATION);
    MdlSize = sizeof(MDL) + (23 * sizeof(PFN_NUMBER));

    /* Initialize the Lookaside List for Large IRPs */
    ExInitializeNPagedLookasideList(&IoLargeIrpLookaside,
                                    NULL,
                                    NULL,
                                    0,
                                    LargeIrpSize,
                                    IO_LARGEIRP,
                                    64);

    /* Initialize the Lookaside List for Small IRPs */
    ExInitializeNPagedLookasideList(&IoSmallIrpLookaside,
                                    NULL,
                                    NULL,
                                    0,
                                    SmallIrpSize,
                                    IO_SMALLIRP,
                                    32);

    /* Initialize the Lookaside List for I\O Completion */
    ExInitializeNPagedLookasideList(&IoCompletionPacketLookaside,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(IO_COMPLETION_PACKET),
                                    IOC_TAG1,
                                    32);

    /* Initialize the Lookaside List for MDLs */
    ExInitializeNPagedLookasideList(&IopMdlLookasideList,
                                    NULL,
                                    NULL,
                                    0,
                                    MdlSize,
                                    TAG_MDL,
                                    128);

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
                                            64);
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
            /* Initialize the Lookaside List for Small IRPs */
            ExInitializeNPagedLookasideList(CurrentList,
                                            NULL,
                                            NULL,
                                            0,
                                            SmallIrpSize,
                                            IO_SMALLIRP_CPU,
                                            32);
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
                                            sizeof(IO_COMPLETION_PACKET),
                                            IO_SMALLIRP_CPU,
                                            32);
        }
        else
        {
            CurrentList = &IoCompletionPacketLookaside;
        }
        Prcb->PPLookasideList[LookasideCompletionList].P = &CurrentList->L;

        /* Set the MDL Completion List */
        Prcb->PPLookasideList[LookasideMdlList].L = &IopMdlLookasideList.L;
        CurrentList = ExAllocatePoolWithTag(NonPagedPool,
                                            sizeof(NPAGED_LOOKASIDE_LIST),
                                            TAG_MDL);
        if (CurrentList)
        {
            /* Initialize the Lookaside List for MDLs */
            ExInitializeNPagedLookasideList(CurrentList,
                                            NULL,
                                            NULL,
                                            0,
                                            SmallIrpSize,
                                            TAG_MDL,
                                            128);
        }
        else
        {
            CurrentList = &IopMdlLookasideList;
        }
        Prcb->PPLookasideList[LookasideMdlList].P = &CurrentList->L;
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

    /* Initialize default settings */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    ObjectTypeInitializer.ValidAccessMask = FILE_ALL_ACCESS;
    ObjectTypeInitializer.UseDefaultObject = TRUE;
    ObjectTypeInitializer.GenericMapping = IopFileMapping;

    /* Do the Adapter Type */
    RtlInitUnicodeString(&Name, L"Adapter");
    ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &IoAdapterObjectType);

    /* Do the Controller Type */
    RtlInitUnicodeString(&Name, L"Controller");
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(CONTROLLER_OBJECT);
    ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &IoControllerObjectType);

    /* Do the Device Type */
    RtlInitUnicodeString(&Name, L"Device");
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(DEVICE_OBJECT);
    ObjectTypeInitializer.ParseProcedure = IopParseDevice;
    ObjectTypeInitializer.SecurityProcedure = IopSecurityFile;
    ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &IoDeviceObjectType);

    /* Initialize the Driver object type */
    RtlInitUnicodeString(&Name, L"Driver");
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(DRIVER_OBJECT);
    ObjectTypeInitializer.DeleteProcedure = IopDeleteDriver;
    ObjectTypeInitializer.ParseProcedure = NULL;
    ObjectTypeInitializer.SecurityProcedure = NULL;
    ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &IoDriverObjectType);

    /* Initialize the I/O Completion object type */
    RtlInitUnicodeString(&Name, L"IoCompletion");
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(KQUEUE);
    ObjectTypeInitializer.ValidAccessMask = IO_COMPLETION_ALL_ACCESS;
    ObjectTypeInitializer.InvalidAttributes |= OBJ_PERMANENT;
    ObjectTypeInitializer.GenericMapping = IopCompletionMapping;
    ObjectTypeInitializer.DeleteProcedure = IopDeleteIoCompletion;
    ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &IoCompletionType);

    /* Initialize the File object type  */
    RtlInitUnicodeString(&Name, L"File");
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(FILE_OBJECT);
    ObjectTypeInitializer.InvalidAttributes |= OBJ_EXCLUSIVE;
    ObjectTypeInitializer.MaintainHandleCount = TRUE;
    ObjectTypeInitializer.ValidAccessMask = FILE_ALL_ACCESS;
    ObjectTypeInitializer.GenericMapping = IopFileMapping;
    ObjectTypeInitializer.CloseProcedure = IopCloseFile;
    ObjectTypeInitializer.DeleteProcedure = IopDeleteFile;
    ObjectTypeInitializer.SecurityProcedure = IopSecurityFile;
    ObjectTypeInitializer.QueryNameProcedure = IopQueryNameFile;
    ObjectTypeInitializer.ParseProcedure = IopParseFile;
    ObjectTypeInitializer.UseDefaultObject = FALSE;
    ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &IoFileObjectType);

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
  IopInitDriverImplementation();
  IoInitCancelHandling();
  IoInitFileSystemImplementation();
  IoInitVpbImplementation();
  IoInitShutdownNotification();
  IopInitPnpNotificationImplementation();
  IopInitErrorLog();
  IopInitTimerImplementation();
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
  LDR_DATA_TABLE_ENTRY ModuleObject;
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

  ModuleObject.DllBase = NULL;
  ModuleObject.SizeOfImage = 0;
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
  IoSynchronousInvalidateDeviceRelations(
    IopRootDeviceNode->PhysicalDeviceObject,
    BusRelations);

     /* Start boot logging */
    IopInitBootLog(BootLog);

    /* Load boot start drivers */
    IopInitializeBootDrivers();

    /* Call back drivers that asked for */
    IopReinitializeBootDrivers();
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
    KdInitSystem(3, (PROS_LOADER_PARAMETER_BLOCK)&KeLoaderBlock);

    /* Load services for devices found by PnP manager */
    IopInitializePnpServices(IopRootDeviceNode, FALSE);

    /* Load system start drivers */
    IopInitializeSystemDrivers();
    IoDestroyDriverList();

    /* Reinitialize drivers that requested it */
    IopReinitializeDrivers();

    /* Stop boot logging */
    IopStopBootLog();

    /* Assign drive letters */
    IoAssignDriveLetters((PLOADER_PARAMETER_BLOCK)&KeLoaderBlock,
                         NULL,
                         NULL,
                         NULL);
}

/* EOF */
