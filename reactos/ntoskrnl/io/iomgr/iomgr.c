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

ULONG IopTraceLevel = 0;

// should go into a proper header
VOID
NTAPI
IoSynchronousInvalidateDeviceRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN DEVICE_RELATION_TYPE Type
);

VOID
NTAPI
IopTimerDispatch(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
);

/* DATA ********************************************************************/

POBJECT_TYPE IoDeviceObjectType = NULL;
POBJECT_TYPE IoFileObjectType = NULL;
extern POBJECT_TYPE IoControllerObjectType;
extern UNICODE_STRING NtSystemRoot;
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
ERESOURCE IopDatabaseResource;
extern ERESOURCE FileSystemListLock;
ERESOURCE IopSecurityResource;
extern KGUARDED_MUTEX FsChangeNotifyListLock;
extern KGUARDED_MUTEX PnpNotifyListLock;
extern LIST_ENTRY IopDiskFsListHead;
extern LIST_ENTRY IopCdRomFsListHead;
extern LIST_ENTRY IopTapeFsListHead;
extern LIST_ENTRY IopNetworkFsListHead;
extern LIST_ENTRY DriverBootReinitListHead;
extern LIST_ENTRY DriverReinitListHead;
extern LIST_ENTRY PnpNotifyListHead;
extern LIST_ENTRY FsChangeNotifyListHead;
extern LIST_ENTRY IopErrorLogListHead;
extern LIST_ENTRY IopTimerQueueHead;
extern KDPC IopTimerDpc;
extern KTIMER IopTimer;
extern KSPIN_LOCK CancelSpinLock;
extern KSPIN_LOCK IoVpbLock;
extern KSPIN_LOCK IoStatisticsLock;
extern KSPIN_LOCK DriverReinitListLock;
extern KSPIN_LOCK DriverBootReinitListLock;
extern KSPIN_LOCK IopLogListLock;
extern KSPIN_LOCK IopTimerLock;

extern PDEVICE_OBJECT IopErrorLogObject;

NPAGED_LOOKASIDE_LIST IoLargeIrpLookaside;
NPAGED_LOOKASIDE_LIST IoSmallIrpLookaside;
NPAGED_LOOKASIDE_LIST IopMdlLookasideList;

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, IoInitSystem)
#endif

/* INIT FUNCTIONS ************************************************************/

VOID
INIT_FUNCTION
NTAPI
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
}

BOOLEAN
INIT_FUNCTION
NTAPI
IopCreateObjectTypes(VOID)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING Name;

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
    if (!NT_SUCCESS(ObCreateObjectType(&Name,
                                       &ObjectTypeInitializer,
                                       NULL,
                                       &IoAdapterObjectType))) return FALSE;

    /* Do the Controller Type */
    RtlInitUnicodeString(&Name, L"Controller");
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(CONTROLLER_OBJECT);
    if (!NT_SUCCESS(ObCreateObjectType(&Name,
                                       &ObjectTypeInitializer,
                                       NULL,
                                       &IoControllerObjectType))) return FALSE;

    /* Do the Device Type. FIXME: Needs Delete Routine! */
    RtlInitUnicodeString(&Name, L"Device");
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(DEVICE_OBJECT);
    ObjectTypeInitializer.ParseProcedure = IopParseDevice;
    ObjectTypeInitializer.SecurityProcedure = IopSecurityFile;
    if (!NT_SUCCESS(ObCreateObjectType(&Name,
                                       &ObjectTypeInitializer,
                                       NULL,
                                       &IoDeviceObjectType))) return FALSE;

    /* Initialize the Driver object type */
    RtlInitUnicodeString(&Name, L"Driver");
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(DRIVER_OBJECT);
    ObjectTypeInitializer.DeleteProcedure = IopDeleteDriver;
    ObjectTypeInitializer.ParseProcedure = NULL;
    ObjectTypeInitializer.SecurityProcedure = NULL;
    if (!NT_SUCCESS(ObCreateObjectType(&Name,
                                       &ObjectTypeInitializer,
                                       NULL,
                                       &IoDriverObjectType))) return FALSE;

    /* Initialize the I/O Completion object type */
    RtlInitUnicodeString(&Name, L"IoCompletion");
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(KQUEUE);
    ObjectTypeInitializer.ValidAccessMask = IO_COMPLETION_ALL_ACCESS;
    ObjectTypeInitializer.InvalidAttributes |= OBJ_PERMANENT;
    ObjectTypeInitializer.GenericMapping = IopCompletionMapping;
    ObjectTypeInitializer.DeleteProcedure = IopDeleteIoCompletion;
    if (!NT_SUCCESS(ObCreateObjectType(&Name,
                                       &ObjectTypeInitializer,
                                       NULL,
                                       &IoCompletionType))) return FALSE;

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
    if (!NT_SUCCESS(ObCreateObjectType(&Name,
                                       &ObjectTypeInitializer,
                                       NULL,
                                       &IoFileObjectType))) return FALSE;

    /* Success */
    return TRUE;
}

BOOLEAN
INIT_FUNCTION
NTAPI
IopCreateRootDirectories()
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DirName;
    HANDLE Handle;

    /* Create the '\Driver' object directory */
    RtlInitUnicodeString(&DirName, L"\\Driver");
    InitializeObjectAttributes(&ObjectAttributes,
                               &DirName,
                               OBJ_PERMANENT,
                               NULL,
                               NULL);
    if (!NT_SUCCESS(NtCreateDirectoryObject(&Handle,
                                            DIRECTORY_ALL_ACCESS,
                                            &ObjectAttributes))) return FALSE;
    NtClose(Handle);

    /* Create the '\FileSystem' object directory */
    RtlInitUnicodeString(&DirName, L"\\FileSystem");
    InitializeObjectAttributes(&ObjectAttributes,
                               &DirName,
                               OBJ_PERMANENT,
                               NULL,
                               NULL);
    if (!NT_SUCCESS(NtCreateDirectoryObject(&Handle,
                                            DIRECTORY_ALL_ACCESS,
                                            &ObjectAttributes))) return FALSE;
    NtClose(Handle);

    /* Return success */
    return TRUE;
}

BOOLEAN
INIT_FUNCTION
NTAPI
IopMarkBootPartition(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    STRING DeviceString;
    CHAR Buffer[256];
    UNICODE_STRING DeviceName;
    NTSTATUS Status;
    HANDLE FileHandle;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_OBJECT FileObject;

    /* Build the ARC device name */
    sprintf(Buffer, "\\ArcName\\%s", LoaderBlock->ArcBootDeviceName);
    RtlInitAnsiString(&DeviceString, Buffer);
    Status = RtlAnsiStringToUnicodeString(&DeviceName, &DeviceString, TRUE);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Open it */
    InitializeObjectAttributes(&ObjectAttributes,
                               &DeviceName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = ZwOpenFile(&FileHandle,
                        FILE_READ_ATTRIBUTES,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        0,
                        FILE_NON_DIRECTORY_FILE);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        KeBugCheckEx(INACCESSIBLE_BOOT_DEVICE,
                     (ULONG_PTR)&DeviceName,
                     Status,
                     0,
                     0);
    }

    /* Get the DO */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       0,
                                       IoFileObjectType,
                                       KernelMode,
                                       (PVOID *)&FileObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        RtlFreeUnicodeString(&DeviceName);
        return FALSE;
    }

    /* Mark it as the boot partition */
    FileObject->DeviceObject->Flags |= DO_SYSTEM_BOOT_PARTITION;

    /* Save a copy of the DO for the I/O Error Logger */
    ObReferenceObject(FileObject->DeviceObject);
    IopErrorLogObject = FileObject->DeviceObject;

    /* Cleanup and return success */
    RtlFreeUnicodeString(&DeviceName);
    NtClose(FileHandle);
    ObDereferenceObject(FileObject);
    return TRUE;
}

BOOLEAN
INIT_FUNCTION
NTAPI
IoInitSystem(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    LARGE_INTEGER ExpireTime;
    NTSTATUS Status;
    CHAR Buffer[256];
    ANSI_STRING NtBootPath, RootString;

    /* Initialize empty NT Boot Path */
    RtlInitEmptyAnsiString(&NtBootPath, Buffer, sizeof(Buffer));

    /* Initialize the lookaside lists */
    IopInitLookasideLists();

    /* Initialize all locks and lists */
    ExInitializeResource(&IopDatabaseResource);
    ExInitializeResource(&FileSystemListLock);
    ExInitializeResource(&IopSecurityResource);
    KeInitializeGuardedMutex(&FsChangeNotifyListLock);
    KeInitializeGuardedMutex(&PnpNotifyListLock);
    InitializeListHead(&IopDiskFsListHead);
    InitializeListHead(&IopCdRomFsListHead);
    InitializeListHead(&IopTapeFsListHead);
    InitializeListHead(&IopNetworkFsListHead);
    InitializeListHead(&DriverBootReinitListHead);
    InitializeListHead(&DriverReinitListHead);
    InitializeListHead(&PnpNotifyListHead);
    InitializeListHead(&ShutdownListHead);
    InitializeListHead(&FsChangeNotifyListHead);
    InitializeListHead(&IopErrorLogListHead);
    KeInitializeSpinLock(&CancelSpinLock);
    KeInitializeSpinLock(&IoVpbLock);
    KeInitializeSpinLock(&IoStatisticsLock);
    KeInitializeSpinLock(&DriverReinitListLock);
    KeInitializeSpinLock(&DriverBootReinitListLock);
    KeInitializeSpinLock(&ShutdownListLock);
    KeInitializeSpinLock(&IopLogListLock);

    /* Initialize Timer List Lock */
    KeInitializeSpinLock(&IopTimerLock);

    /* Initialize Timer List */
    InitializeListHead(&IopTimerQueueHead);

    /* Initialize the DPC/Timer which will call the other Timer Routines */
    ExpireTime.QuadPart = -10000000;
    KeInitializeDpc(&IopTimerDpc, IopTimerDispatch, NULL);
    KeInitializeTimerEx(&IopTimer, SynchronizationTimer);
    KeSetTimerEx(&IopTimer, ExpireTime, 1000, &IopTimerDpc);

    /* Create Object Types */
    if (!IopCreateObjectTypes()) return FALSE;

    /* Create Object Directories */
    if (!IopCreateRootDirectories()) return FALSE;

    /* Initialize PnP manager */
    PnpInit();

    /* Create the group driver list */
    IoCreateDriverList();

    /* Load boot start drivers */
    IopInitializeBootDrivers();

    /* Call back drivers that asked for */
    IopReinitializeBootDrivers();

    /* Initialize PnP root relations */
    IoSynchronousInvalidateDeviceRelations(IopRootDeviceNode->
                                           PhysicalDeviceObject,
                                           BusRelations);

    /* Create ARC names for boot devices */
    IopCreateArcNames(LoaderBlock);

    /* Mark the system boot partition */
    if (!IopMarkBootPartition(LoaderBlock)) return FALSE;

#ifndef _WINKD_
    /* Read KDB Data */
    KdbInit();

    /* I/O is now setup for disk access, so phase 3 */
    KdInitSystem(3, LoaderBlock);
#endif

    /* Load services for devices found by PnP manager */
    IopInitializePnpServices(IopRootDeviceNode, FALSE);

    /* Load system start drivers */
    IopInitializeSystemDrivers();

    /* Destroy the group driver list */
    IoDestroyDriverList();

    /* Reinitialize drivers that requested it */
    IopReinitializeDrivers();

    /* Convert SystemRoot from ARC to NT path */
    Status = IopReassignSystemRoot(LoaderBlock, &NtBootPath);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Set the ANSI_STRING for the root path */
    RootString.MaximumLength = NtSystemRoot.MaximumLength / sizeof(WCHAR);
    RootString.Length = 0;
    RootString.Buffer = ExAllocatePoolWithTag(PagedPool,
                                              RootString.MaximumLength,
                                              TAG_IO);

    /* Convert the path into the ANSI_STRING */
    Status = RtlUnicodeStringToAnsiString(&RootString, &NtSystemRoot, FALSE);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Assign drive letters */
    IoAssignDriveLetters(LoaderBlock,
                         &NtBootPath,
                         (PUCHAR)RootString.Buffer,
                         &RootString);

    /* Update system root */
    Status = RtlAnsiStringToUnicodeString(&NtSystemRoot, &RootString, FALSE);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Load the System DLL and its Entrypoints */
    if (!NT_SUCCESS(PsLocateSystemDll())) return FALSE;

    /* Return success */
    return TRUE;
}

/* EOF */
