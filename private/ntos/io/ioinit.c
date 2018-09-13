/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    ioinit.c

Abstract:

    This module contains the code to initialize the I/O system.

Author:

    Darryl E. Havens (darrylh) April 27, 1989

Environment:

    Kernel mode, system initialization code

Revision History:


--*/

#include "iop.h"
#include <setupblk.h>
#include <inbv.h>
#include <ntddstor.h>


//
// Define the default number of I/O stack locations a large IRP should
// have if not specified by the registry.
//

#define DEFAULT_LARGE_IRP_LOCATIONS 8

//
// Define the default number of IRP that can be in progress and allocated
// from a lookaside list.
//

#define DEFAULT_LOOKASIDE_IRP_LIMIT 512

//
// Define the type for driver group name entries in the group list so that
// load order dependencies can be tracked.
//

typedef struct _TREE_ENTRY {
    struct _TREE_ENTRY *Left;
    struct _TREE_ENTRY *Right;
    struct _TREE_ENTRY *Sibling;
    ULONG DriversThisType;
    ULONG DriversLoaded;
    UNICODE_STRING GroupName;
} TREE_ENTRY, *PTREE_ENTRY;

typedef struct _DRIVER_INFORMATION {
    LIST_ENTRY Link;
    PDRIVER_OBJECT DriverObject;
    PBOOT_DRIVER_LIST_ENTRY DataTableEntry;
    HANDLE ServiceHandle;
    USHORT TagPosition;
    BOOLEAN Failed;
    BOOLEAN Processed;
} DRIVER_INFORMATION, *PDRIVER_INFORMATION;

PTREE_ENTRY IopGroupListHead;

//
// I/O Error logging support
//
PVOID IopErrorLogObject = NULL;

//
// Group order table
//

ULONG IopGroupIndex;
PLIST_ENTRY IopGroupTable;

//
// Define a macro for initializing drivers.
//

#define InitializeDriverObject( Object ) {                                 \
    ULONG i;                                                               \
    RtlZeroMemory( Object,                                                 \
                   sizeof( DRIVER_OBJECT ) + sizeof ( DRIVER_EXTENSION )); \
    Object->DriverExtension = (PDRIVER_EXTENSION) (Object + 1);            \
    Object->DriverExtension->DriverObject = Object;                        \
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)                         \
        Object->MajorFunction[i] = IopInvalidDeviceRequest;                \
    Object->Type = IO_TYPE_DRIVER;                                         \
    Object->Size = sizeof( DRIVER_OBJECT );                                \
    }

//
// Define external procedures not in common header files
//

VOID
IopInitializeData(
    VOID
    );

NTSTATUS
RawInitialize(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

//
// Define the local procedures
//

BOOLEAN
IopCheckDependencies(
    IN HANDLE KeyHandle
    );

VOID
IopCreateArcNames(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

BOOLEAN
IopCreateObjectTypes(
    VOID
    );

PTREE_ENTRY
IopCreateEntry(
    IN PUNICODE_STRING GroupName
    );

BOOLEAN
IopCreateRootDirectories(
    VOID
    );

VOID
IopFreeGroupTree(
    IN PTREE_ENTRY TreeEntry
    );

NTSTATUS
IopInitializeAttributesAndCreateObject(
    IN PUNICODE_STRING ObjectName,
    IN OUT POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PDRIVER_OBJECT *DriverObject
    );

BOOLEAN
IopInitializeBootDrivers(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    OUT PDRIVER_OBJECT *PreviousDriver
    );

PDRIVER_OBJECT
IopInitializeBuiltinDriver(
    IN PUNICODE_STRING DriverName,
    IN PUNICODE_STRING RegistryPath,
    IN PDRIVER_INITIALIZE DriverInitializeRoutine,
    IN PLDR_DATA_TABLE_ENTRY TableEntry,
    IN BOOLEAN TextModeSetup
    );


BOOLEAN
IopInitializeSingleBootDriver(
    IN  HANDLE KeyHandle,
    IN  PBOOT_DRIVER_LIST_ENTRY BootDriver,
    OUT PUNICODE_STRING DriverName           OPTIONAL
    );

BOOLEAN
IopInitializeSystemDrivers(
    VOID
    );

PTREE_ENTRY
IopLookupGroupName(
    IN PUNICODE_STRING GroupName,
    IN BOOLEAN Insert
    );

BOOLEAN
IopMarkBootPartition(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

BOOLEAN
IopReassignSystemRoot(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    OUT PSTRING NtDeviceName
    );

VOID
IopStoreSystemPartitionInformation(
    IN     PUNICODE_STRING NtSystemPartitionDeviceName,
    IN OUT PUNICODE_STRING OsLoaderPathName
    );

USHORT
IopGetDriverTagPriority (
    IN HANDLE Servicehandle
    );

VOID
IopInsertDriverList (
    IN PLIST_ENTRY ListHead,
    IN PDRIVER_INFORMATION DriverInfo
    );

VOID
IopNotifySetupDevices (
    PDEVICE_NODE DeviceNode
    );

BOOLEAN
IopWaitForBootDevicesStarted (
    IN VOID
    );

BOOLEAN
IopWaitForBootDevicesDeleted (
    IN VOID
    );
VOID
IopSetIoRoutines(
    IN VOID
    );

//
// The following allows the I/O system's initialization routines to be
// paged out of memory.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,IoInitSystem)
#pragma alloc_text(INIT,IopCheckDependencies)
#pragma alloc_text(INIT,IopCreateArcNames)
#pragma alloc_text(INIT,IopCreateEntry)
#pragma alloc_text(INIT,IopCreateObjectTypes)
#pragma alloc_text(INIT,IopCreateRootDirectories)
#pragma alloc_text(INIT,IopFreeGroupTree)
#pragma alloc_text(INIT,IopInitializeAttributesAndCreateObject)
#pragma alloc_text(INIT,IopInitializeBootDrivers)
#pragma alloc_text(INIT,IopInitializeBuiltinDriver)
#pragma alloc_text(INIT,IopInitializeSingleBootDriver)
#pragma alloc_text(INIT,IopInitializeSystemDrivers)
#pragma alloc_text(INIT,IopLookupGroupName)
#pragma alloc_text(INIT,IopMarkBootPartition)
#pragma alloc_text(INIT,IopReassignSystemRoot)
#pragma alloc_text(INIT,IopStoreSystemPartitionInformation)
#pragma alloc_text(INIT,IopInsertDriverList)
#pragma alloc_text(INIT,IopGetDriverTagPriority)
#pragma alloc_text(INIT,IopNotifySetupDevices)
#pragma alloc_text(INIT,IopWaitForBootDevicesStarted)
#pragma alloc_text(INIT,IopWaitForBootDevicesDeleted)
#pragma alloc_text(INIT,IopLoadBootFilterDriver)
#pragma alloc_text(INIT,IopSetIoRoutines)
#endif


BOOLEAN
IoInitSystem(
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This routine initializes the I/O system.

Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block that was
        created by the OS Loader.

Return Value:

    The function value is a BOOLEAN indicating whether or not the I/O system
    was successfully initialized.

--*/

{
    PDRIVER_OBJECT driverObject;
    PDRIVER_OBJECT *nextDriverObject;
    STRING ntDeviceName;
    UCHAR deviceNameBuffer[256];
    ULONG largePacketSize;
    ULONG smallPacketSize;
    ULONG mdlPacketSize;
    ULONG numberOfPackets;
    ULONG poolSize;
    PLIST_ENTRY entry;
    PREINIT_PACKET reinitEntry;
    LARGE_INTEGER deltaTime;
    MM_SYSTEMSIZE systemSize;
    USHORT completionZoneSize;
    USHORT largeIrpZoneSize;
    USHORT smallIrpZoneSize;
    USHORT mdlZoneSize;
    ULONG oldNtGlobalFlag;
    NTSTATUS status;
    ANSI_STRING ansiString;
    UNICODE_STRING eventName;
    UNICODE_STRING startTypeName;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE handle;
    PDEVICE_NODE deviceNode;
    PNPAGED_LOOKASIDE_LIST lookaside;
    ULONG Index;
    PKPRCB prcb;
    ULONG len;
    PKEY_VALUE_PARTIAL_INFORMATION value;
    UCHAR   valueBuffer[32];

    ASSERT( IopQueryOperationLength[FileMaximumInformation] == 0xff );
    ASSERT( IopSetOperationLength[FileMaximumInformation] == 0xff );
    ASSERT( IopQueryOperationAccess[FileMaximumInformation] == 0xffffffff );
    ASSERT( IopSetOperationAccess[FileMaximumInformation] == 0xffffffff );

    ASSERT( IopQueryFsOperationLength[FileFsMaximumInformation] == 0xff );
    ASSERT( IopSetFsOperationLength[FileFsMaximumInformation] == 0xff );
    ASSERT( IopQueryFsOperationAccess[FileFsMaximumInformation] == 0xffffffff );
    ASSERT( IopSetFsOperationAccess[FileFsMaximumInformation] == 0xffffffff );

    //
    // Initialize the I/O database resource, lock, and the file system and
    // network file system queue headers.  Also allocate the cancel spin
    // lock.
    //

    ntDeviceName.Buffer = deviceNameBuffer;
    ntDeviceName.MaximumLength = sizeof(deviceNameBuffer);
    ntDeviceName.Length = 0;

    ExInitializeResource( &IopDatabaseResource );
    ExInitializeResource( &IopSecurityResource );
    KeInitializeSpinLock( &IopDatabaseLock );
    InitializeListHead( &IopDiskFileSystemQueueHead );
    InitializeListHead( &IopCdRomFileSystemQueueHead );
    InitializeListHead( &IopTapeFileSystemQueueHead );
    InitializeListHead( &IopNetworkFileSystemQueueHead );
    InitializeListHead( &IopBootDriverReinitializeQueueHead );
    InitializeListHead( &IopDriverReinitializeQueueHead );
    InitializeListHead( &IopNotifyShutdownQueueHead );
    InitializeListHead( &IopNotifyLastChanceShutdownQueueHead );
    InitializeListHead( &IopFsNotifyChangeQueueHead );
    KeInitializeSpinLock( &IopCancelSpinLock );
    KeInitializeSpinLock( &IopVpbSpinLock );
    KeInitializeSpinLock( &IoStatisticsLock );

    IopSetIoRoutines();
    //
    // Initialize the unique device object number counter used by IoCreateDevice
    // when automatically generating a device object name.
    //
    IopUniqueDeviceObjectNumber = 0;

    //
    // Initialize the large I/O Request Packet (IRP) lookaside list head and the
    // mutex which guards the list.
    //

    if (!IopLargeIrpStackLocations) {
        IopLargeIrpStackLocations = DEFAULT_LARGE_IRP_LOCATIONS;
    }

    systemSize = MmQuerySystemSize();

    switch ( systemSize ) {

    case MmSmallSystem :
        completionZoneSize = 6;
        smallIrpZoneSize = 6;
        largeIrpZoneSize = 8;
        mdlZoneSize = 16;
        IopLookasideIrpLimit = DEFAULT_LOOKASIDE_IRP_LIMIT;
        break;

    case MmMediumSystem :
        completionZoneSize = 24;
        smallIrpZoneSize = 24;
        largeIrpZoneSize = 32;
        mdlZoneSize = 90;
        IopLookasideIrpLimit = DEFAULT_LOOKASIDE_IRP_LIMIT * 2;
        break;

    case MmLargeSystem :
        if (MmIsThisAnNtAsSystem()) {
            completionZoneSize = 96;
            smallIrpZoneSize = 96;
            largeIrpZoneSize = 128;
            mdlZoneSize = 256;
            IopLookasideIrpLimit = DEFAULT_LOOKASIDE_IRP_LIMIT * 4;

        } else {
            completionZoneSize = 32;
            smallIrpZoneSize = 32;
            largeIrpZoneSize = 64;
            mdlZoneSize = 128;
            IopLookasideIrpLimit = DEFAULT_LOOKASIDE_IRP_LIMIT * 3;
        }

        break;
    }

    //
    // Initialize the system I/O completion lookaside list.
    //

    ExInitializeNPagedLookasideList( &IopCompletionLookasideList,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof(IOP_MINI_COMPLETION_PACKET),
                                     ' pcI',
                                     completionZoneSize );

    //
    // Initialize the system large IRP lookaside list.
    //

    largePacketSize = (ULONG) (sizeof( IRP ) + (IopLargeIrpStackLocations * sizeof( IO_STACK_LOCATION )));
    ExInitializeNPagedLookasideList( &IopLargeIrpLookasideList,
                                     NULL,
                                     NULL,
                                     0,
                                     largePacketSize,
                                     'lprI',
                                     largeIrpZoneSize );

    //
    // Initialize the system small IRP lookaside list.
    //

    smallPacketSize = (ULONG) (sizeof( IRP ) + sizeof( IO_STACK_LOCATION ));
    ExInitializeNPagedLookasideList( &IopSmallIrpLookasideList,
                                     NULL,
                                     NULL,
                                     0,
                                     smallPacketSize,
                                     'sprI',
                                     smallIrpZoneSize );

    //
    // Initialize the system MDL lookaside list.
    //

    mdlPacketSize = (ULONG) (sizeof( MDL ) + (IOP_FIXED_SIZE_MDL_PFNS * sizeof( PFN_NUMBER )));
    ExInitializeNPagedLookasideList( &IopMdlLookasideList,
                                     NULL,
                                     NULL,
                                     0,
                                     mdlPacketSize,
                                     ' ldM',
                                     mdlZoneSize );

    //
    // Initialize the per processor nonpaged lookaside lists and descriptors.
    //

    for (Index = 0; Index < (ULONG)KeNumberProcessors; Index += 1) {
        prcb = KiProcessorBlock[Index];

        //
        // Initialize the I/O completion per processor lookaside pointers
        //

        prcb->PPLookasideList[LookasideCompletionList].L = &IopCompletionLookasideList;
        lookaside = (PNPAGED_LOOKASIDE_LIST)ExAllocatePoolWithTag( NonPagedPool,
                                                                   sizeof(NPAGED_LOOKASIDE_LIST),
                                                                   'PpcI');

        if (lookaside != NULL) {
            ExInitializeNPagedLookasideList( lookaside,
                                             NULL,
                                             NULL,
                                             0,
                                             sizeof(IOP_MINI_COMPLETION_PACKET),
                                             'PpcI',
                                             completionZoneSize );

        } else {
            lookaside = &IopCompletionLookasideList;
        }

        prcb->PPLookasideList[LookasideCompletionList].P = lookaside;

        //
        // Initialize the large IRP per processor lookaside pointers.
        //

        prcb->PPLookasideList[LookasideLargeIrpList].L = &IopLargeIrpLookasideList;
        lookaside = (PNPAGED_LOOKASIDE_LIST)ExAllocatePoolWithTag( NonPagedPool,
                                                                   sizeof(NPAGED_LOOKASIDE_LIST),
                                                                   'LprI');

        if (lookaside != NULL) {
            ExInitializeNPagedLookasideList( lookaside,
                                             NULL,
                                             NULL,
                                             0,
                                             largePacketSize,
                                             'LprI',
                                             largeIrpZoneSize );

        } else {
            lookaside = &IopLargeIrpLookasideList;
        }

        prcb->PPLookasideList[LookasideLargeIrpList].P = lookaside;

        //
        // Initialize the small IRP per processor lookaside pointers.
        //

        prcb->PPLookasideList[LookasideSmallIrpList].L = &IopSmallIrpLookasideList;
        lookaside = (PNPAGED_LOOKASIDE_LIST)ExAllocatePoolWithTag( NonPagedPool,
                                                                   sizeof(NPAGED_LOOKASIDE_LIST),
                                                                   'SprI');

        if (lookaside != NULL) {
            ExInitializeNPagedLookasideList( lookaside,
                                             NULL,
                                             NULL,
                                             0,
                                             smallPacketSize,
                                             'SprI',
                                             smallIrpZoneSize);

        } else {
            lookaside = &IopSmallIrpLookasideList;
        }

        prcb->PPLookasideList[LookasideSmallIrpList].P = lookaside;

        //
        // Initialize the MDL per processor lookaside list pointers.
        //

        prcb->PPLookasideList[LookasideMdlList].L = &IopMdlLookasideList;
        lookaside = (PNPAGED_LOOKASIDE_LIST)ExAllocatePoolWithTag( NonPagedPool,
                                                                   sizeof(NPAGED_LOOKASIDE_LIST),
                                                                   'PldM');

        if (lookaside != NULL) {
            ExInitializeNPagedLookasideList( lookaside,
                                             NULL,
                                             NULL,
                                             0,
                                             mdlPacketSize,
                                             'PldM',
                                             mdlZoneSize );

        } else {
            lookaside = &IopMdlLookasideList;
        }

        prcb->PPLookasideList[LookasideMdlList].P = lookaside;
    }

    //
    // Initialize the I/O completion spin lock.
    //

    KeInitializeSpinLock( &IopCompletionLock );

    //
    // Initalize the error log spin locks and log list.
    //

    KeInitializeSpinLock( &IopErrorLogLock );
    KeInitializeSpinLock( &IopErrorLogAllocationLock );
    InitializeListHead( &IopErrorLogListHead );

    //
    // Determine if the Error Log service will ever run this boot.
    //
    InitializeObjectAttributes (&objectAttributes,
                                &CmRegistryMachineSystemCurrentControlSetServicesEventLog,
                                OBJ_CASE_INSENSITIVE,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    status = ZwOpenKey(&handle,
                       KEY_READ,
                       &objectAttributes
                       );

    if (NT_SUCCESS (status)) {
        RtlInitUnicodeString (&startTypeName, L"Start");
        value = (PKEY_VALUE_PARTIAL_INFORMATION) valueBuffer;
        status = NtQueryValueKey (handle,
                                  &startTypeName,
                                  KeyValuePartialInformation,
                                  valueBuffer,
                                  sizeof (valueBuffer),
                                  &len);

        if (NT_SUCCESS (status) && (value->Type == REG_DWORD)) {
            if (SERVICE_DISABLED == (*(PULONG) (value->Data))) {
                //
                // We are disabled for this boot.
                //
                IopErrorLogDisabledThisBoot = TRUE;
            } else {
                IopErrorLogDisabledThisBoot = FALSE;
            }
        } else {
            //
            // Didn't find the value so we are not enabled.
            //
            IopErrorLogDisabledThisBoot = TRUE;
        }
    } else {
        //
        // Didn't find the key so we are not enabled
        //
        IopErrorLogDisabledThisBoot = TRUE;
    }

    //
    // Initialize the registry access semaphore.
    //

    KeInitializeSemaphore( &IopRegistrySemaphore, 1, 1 );

    //
    // Initialize the timer database and start the timer DPC routine firing
    // so that drivers can use it during initialization.
    //

    deltaTime.QuadPart = - 10 * 1000 * 1000;

    KeInitializeSpinLock( &IopTimerLock );
    InitializeListHead( &IopTimerQueueHead );
    KeInitializeDpc( &IopTimerDpc, IopTimerDispatch, NULL );
    KeInitializeTimerEx( &IopTimer, SynchronizationTimer );
    (VOID) KeSetTimerEx( &IopTimer, deltaTime, 1000, &IopTimerDpc );

    //
    // Initialize the IopHardError structure used for informational pop-ups.
    //

    ExInitializeWorkItem( &IopHardError.ExWorkItem,
                          IopHardErrorThread,
                          NULL );

    InitializeListHead( &IopHardError.WorkQueue );

    KeInitializeSpinLock( &IopHardError.WorkQueueSpinLock );

    KeInitializeSemaphore( &IopHardError.WorkQueueSemaphore,
                           0,
                           MAXLONG );

    IopHardError.ThreadStarted = FALSE;

    IopCurrentHardError = NULL;

    //
    // Create the link tracking named event.
    //

    RtlInitUnicodeString( &eventName, L"\\Security\\TRKWKS_EVENT" );
    InitializeObjectAttributes( &objectAttributes,
                                &eventName,
                                OBJ_PERMANENT,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );
    status = NtCreateEvent( &handle,
                            EVENT_ALL_ACCESS,
                            &objectAttributes,
                            NotificationEvent,
                            FALSE );
    if (!NT_SUCCESS( status )) {
#if DBG
        DbgPrint( "IOINIT: NtCreateEvent failed\n" );
#endif
        return FALSE;
    }

    (VOID) ObReferenceObjectByHandle( handle,
                                      0,
                                      ExEventObjectType,
                                      KernelMode,
                                      (PVOID *) &IopLinkTrackingServiceEvent,
                                      NULL );

    KeInitializeEvent( &IopLinkTrackingPacket.Event, NotificationEvent, FALSE );
    KeInitializeEvent(&IopLinkTrackingPortObject, SynchronizationEvent, TRUE );

    KeInitializeSemaphore(&IopProfileChangeSemaphore, 1, 1) ;

    //
    // Create all of the objects for the I/O system.
    //

    if (!IopCreateObjectTypes()) {
#if DBG
        DbgPrint( "IOINIT: IopCreateObjectTypes failed\n" );
#endif
        return FALSE;
    }

    //
    // Create the root directories for the I/O system.
    //

    if (!IopCreateRootDirectories()) {
#if DBG
        DbgPrint( "IOINIT: IopCreateRootDirectories failed\n" );
#endif
        return FALSE;
    }

    //
    // Initialize the resource map
    //

    IopInitializeResourceMap (LoaderBlock);

    //
    // Initialize PlugPlay services phase 0
    //

    status = IopInitializePlugPlayServices(LoaderBlock, 0);
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    //
    // Call Power manager to initialize for drivers
    //

    PoInitDriverServices(0);

    //
    // Call HAL to initialize PnP bus driver
    //

    HalInitPnpDriver();
    deviceNode = IopRootDeviceNode->Child;
    while (deviceNode) {
        if ((deviceNode->Flags & DNF_STARTED) &&
            !(deviceNode->Flags & DNF_LEGACY_DRIVER)) {
            IopInitHalDeviceNode = deviceNode;
            deviceNode->Flags |= DNF_HAL_NODE;
            break;
        }
        deviceNode = deviceNode->Sibling;
    }

    //
    // Call WMI to initialize it and allow it to create its driver object
    // Note that no calls to WMI can occur until it is initialized here.
    //

    WMIInitialize();

    //
    // Save this for use during PnP enumeration -- we NULL it out later
    // before LoaderBlock is reused.
    //

    IopLoaderBlock = (PVOID)LoaderBlock;

    //
    // If this is a remote boot, we need to add a few values to the registry.
    //

    if (IoRemoteBootClient) {
        status = IopAddRemoteBootValuesToRegistry(LoaderBlock);
        if (!NT_SUCCESS(status)) {
            KeBugCheckEx( NETWORK_BOOT_INITIALIZATION_FAILED,
                          1,
                          status,
                          0,
                          0 );
        }
    }

    //
    // Initialize PlugPlay services phase 1 to execute firmware mapper
    //

    status = IopInitializePlugPlayServices(LoaderBlock, 1);
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    //
    // Initialize the drivers loaded by the boot loader (OSLOADER)
    //

    nextDriverObject = &driverObject;
    if (!IopInitializeBootDrivers( LoaderBlock,
                                   nextDriverObject )) {
#if DBG
        DbgPrint( "IOINIT: Initializing boot drivers failed\n" );
#endif // DBG
        return FALSE;
    }

    //
    // Once we have initialized the boot drivers, we don't need the
    // copy of the pointer to the loader block any more.
    //

    IopLoaderBlock = NULL;

    //
    // If this is a remote boot, start the network and assign
    // C: to \Device\LanmanRedirector.
    //

    if (IoRemoteBootClient) {
        status = IopStartNetworkForRemoteBoot(LoaderBlock);
        if (!NT_SUCCESS( status )) {
            KeBugCheckEx( NETWORK_BOOT_INITIALIZATION_FAILED,
                          2,
                          status,
                          0,
                          0 );
        }
    }

    //
    // Save the current value of the NT Global Flags and enable kernel debugger
    // symbol loading while drivers are being loaded so that systems can be
    // debugged regardless of whether they are free or checked builds.
    //

    oldNtGlobalFlag = NtGlobalFlag;

    if (!(NtGlobalFlag & FLG_ENABLE_KDEBUG_SYMBOL_LOAD)) {
        NtGlobalFlag |= FLG_ENABLE_KDEBUG_SYMBOL_LOAD;
    }

    status = PsLocateSystemDll();
    if (!NT_SUCCESS( status )) {
        return FALSE;
    }

    //
    // Initialize the device drivers for the system.
    //

    if (!IopInitializeSystemDrivers()) {
#if DBG
        DbgPrint( "IOINIT: Initializing system drivers failed\n" );
#endif // DBG
        return FALSE;
    }

    //
    // Free the memory allocated to contain the group dependency list.
    //

    if (IopGroupListHead) {
        IopFreeGroupTree( IopGroupListHead );
    }

    //
    // Walk the list of drivers that have requested that they be called again
    // for reinitialization purposes.
    //

    while (entry = ExInterlockedRemoveHeadList( &IopDriverReinitializeQueueHead, &IopDatabaseLock )) {
        reinitEntry = CONTAINING_RECORD( entry, REINIT_PACKET, ListEntry );
        reinitEntry->DriverObject->DriverExtension->Count++;
        reinitEntry->DriverObject->Flags &= ~DRVO_REINIT_REGISTERED;
        reinitEntry->DriverReinitializationRoutine( reinitEntry->DriverObject,
                                                    reinitEntry->Context,
                                                    reinitEntry->DriverObject->DriverExtension->Count );
        ExFreePool( reinitEntry );
    }

    //
    // Reassign \SystemRoot to NT device name path.
    //

    if (!IopReassignSystemRoot( LoaderBlock, &ntDeviceName )) {
        return FALSE;
    }

    //
    // Protect the system partition of an ARC system if necessary
    //

    if (!IopProtectSystemPartition( LoaderBlock )) {
        return(FALSE);
    }

    //
    // Assign DOS drive letters to disks and cdroms and define \SystemRoot.
    //

    ansiString.MaximumLength = NtSystemRoot.MaximumLength / sizeof( WCHAR );
    ansiString.Length = 0;
    ansiString.Buffer = (RtlAllocateStringRoutine)( ansiString.MaximumLength );
    status = RtlUnicodeStringToAnsiString( &ansiString,
                                           &NtSystemRoot,
                                           FALSE
                                         );
    if (!NT_SUCCESS( status )) {
        DbgPrint( "IOINIT: UnicodeToAnsi( %wZ ) failed - %x\n", &NtSystemRoot, status );
        return(FALSE);
    }

    IoAssignDriveLetters( LoaderBlock,
                          &ntDeviceName,
                          ansiString.Buffer,
                          &ansiString );

    status = RtlAnsiStringToUnicodeString( &NtSystemRoot,
                                           &ansiString,
                                           FALSE
                                         );
    if (!NT_SUCCESS( status )) {
        DbgPrint( "IOINIT: AnsiToUnicode( %Z ) failed - %x\n", &ansiString, status );
        return(FALSE);
    }

    //
    // Also restore the NT Global Flags to their original state.
    //

    NtGlobalFlag = oldNtGlobalFlag;

    //
    // Call Power manager to initialize for post-boot drivers
    //
    PoInitDriverServices(1);

    //
    // Indicate that the I/O system successfully initialized itself.
    //

    return TRUE;
}

VOID
IopSetIoRoutines()
{
    if (pIofCallDriver == NULL) {

        pIofCallDriver = IopfCallDriver;
    }

    if (pIofCompleteRequest == NULL) {

        pIofCompleteRequest = IopfCompleteRequest;
    }

    if (pIoAllocateIrp == NULL) {

        pIoAllocateIrp = IopAllocateIrpPrivate;
    }

    if (pIoFreeIrp == NULL) {

        pIoFreeIrp = IopFreeIrp;
    }
}


BOOLEAN
IopCheckDependencies(
    IN HANDLE KeyHandle
    )

/*++

Routine Description:

    This routine gets the "DependOnGroup" field for the specified key node
    and determines whether any driver in the group(s) that this entry is
    dependent on has successfully loaded.

Arguments:

    KeyHandle - Supplies a handle to the key representing the driver in
        question.

Return Value:

    The function value is TRUE if the driver should be loaded, otherwise
    FALSE

--*/

{
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    UNICODE_STRING groupName;
    BOOLEAN load;
    ULONG length;
    PWSTR source;
    PTREE_ENTRY treeEntry;

    //
    // Attempt to obtain the "DependOnGroup" key for the specified driver
    // entry.  If one does not exist, then simply mark this driver as being
    // one to attempt to load.  If it does exist, then check to see whether
    // or not any driver in the groups that it is dependent on has loaded
    // and allow it to load.
    //

    if (!NT_SUCCESS( IopGetRegistryValue( KeyHandle, L"DependOnGroup", &keyValueInformation ))) {
        return TRUE;
    }

    length = keyValueInformation->DataLength;

    source = (PWSTR) ((PUCHAR) keyValueInformation + keyValueInformation->DataOffset);
    load = TRUE;

    while (length) {
        RtlInitUnicodeString( &groupName, source );
        groupName.Length = groupName.MaximumLength;
        treeEntry = IopLookupGroupName( &groupName, FALSE );
        if (treeEntry) {
            if (!treeEntry->DriversLoaded) {
                load = FALSE;
                break;
            }
        }
        length -= groupName.MaximumLength;
        source = (PWSTR) ((PUCHAR) source + groupName.MaximumLength);
    }

    ExFreePool( keyValueInformation );
    return load;
}


VOID
IopCreateArcNames(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    The loader block contains a table of disk signatures and corresponding
    ARC names. Each device that the loader can access will appear in the
    table. This routine opens each disk device in the system, reads the
    signature and compares it to the table. For each match, it creates a
    symbolic link between the nt device name and the ARC name.

    The checksum value provided by the loader is the ULONG sum of all
    elements in the checksum, inverted, plus 1:
    checksum = ~sum + 1;
    This way the sum of all of the elements can be calculated here and
    added to the checksum in the loader block.  If the result is zero, then
    there is a match.

Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block that was
        created by the OS Loader.

Return Value:

    None.

--*/

{
    STRING arcBootDeviceString;
    UCHAR deviceNameBuffer[128];
    STRING deviceNameString;
    UNICODE_STRING deviceNameUnicodeString;
    PDEVICE_OBJECT deviceObject;
    UCHAR arcNameBuffer[128];
    STRING arcNameString;
    UNICODE_STRING arcNameUnicodeString;
    PFILE_OBJECT fileObject;
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    DISK_GEOMETRY diskGeometry;
    PDRIVE_LAYOUT_INFORMATION driveLayout;
    PLIST_ENTRY listEntry;
    PARC_DISK_SIGNATURE diskBlock;
    ULONG diskNumber;
    ULONG partitionNumber;
    PCHAR arcName;
    PULONG buffer;
    PIRP irp;
    KEVENT event;
    LARGE_INTEGER offset;
    ULONG checkSum;
    ULONG i;
    PVOID tmpPtr;
    BOOLEAN useLegacyEnumeration = FALSE;
    BOOLEAN singleBiosDiskFound;
    BOOLEAN bootDiskFound = FALSE;
    PARC_DISK_INFORMATION arcInformation = LoaderBlock->ArcDiskInformation;
    ULONG totalDriverDisksFound = IoGetConfigurationInformation()->DiskCount;
    ULONG totalPnpDisksFound = 0;
    STRING arcSystemDeviceString;
    STRING osLoaderPathString;
    UNICODE_STRING osLoaderPathUnicodeString;
    PWSTR diskList = NULL;
    wchar_t *pDiskNameList;
    STORAGE_DEVICE_NUMBER   pnpDiskDeviceNumber;


    //
    // ask PNP to give us a list with all the currently active disks
    //

    pDiskNameList = diskList;
    pnpDiskDeviceNumber.DeviceNumber = 0xFFFFFFFF;
    status = IoGetDeviceInterfaces(&DiskClassGuid, NULL, 0, &diskList);

    if (!NT_SUCCESS(status)) {

        useLegacyEnumeration = TRUE;
        if (pDiskNameList) {
            *pDiskNameList = L'\0';
        }

    } else {

        //
        // count the number of disks returned
        //

        pDiskNameList = diskList;
        while (*pDiskNameList != L'\0') {

            totalPnpDisksFound++;
            pDiskNameList = pDiskNameList + (wcslen(pDiskNameList) + 1);

        }

        pDiskNameList = diskList;

        //
        // if the disk returned by PNP are not all the disks in the system
        // it means that some legacy driver has generated a disk device object/link.
        // In that case we need to enumerate all pnp disks and then using the legacy
        // for-loop also enumerate the non-pnp disks
        //

        if (totalPnpDisksFound < totalDriverDisksFound) {
            useLegacyEnumeration = TRUE;
        }

    }

    //
    // If a single bios disk was found if there is only a
    // single entry on the disk signature list.
    //

    singleBiosDiskFound = (arcInformation->DiskSignatures.Flink->Flink ==
                           &arcInformation->DiskSignatures) ? (TRUE) : (FALSE);


    //
    // Create hal/loader partition name
    //

    sprintf( arcNameBuffer, "\\ArcName\\%s", LoaderBlock->ArcHalDeviceName );
    RtlInitAnsiString( &arcNameString, arcNameBuffer );
    RtlAnsiStringToUnicodeString (&IoArcHalDeviceName, &arcNameString, TRUE);

    //
    // Create boot partition name
    //

    sprintf( arcNameBuffer, "\\ArcName\\%s", LoaderBlock->ArcBootDeviceName );
    RtlInitAnsiString( &arcNameString, arcNameBuffer );
    RtlAnsiStringToUnicodeString (&IoArcBootDeviceName, &arcNameString, TRUE);
    i = strlen (LoaderBlock->ArcBootDeviceName) + 1;
    IoLoaderArcBootDeviceName = ExAllocatePool (PagedPool, i);
    if (IoLoaderArcBootDeviceName) {
        memcpy (IoLoaderArcBootDeviceName, LoaderBlock->ArcBootDeviceName, i);
    }

    if (singleBiosDiskFound && strstr(LoaderBlock->ArcBootDeviceName, "cdrom")) {
        singleBiosDiskFound = FALSE;
    }

    //
    // Get ARC boot device name from loader block.
    //

    RtlInitAnsiString( &arcBootDeviceString,
                       LoaderBlock->ArcBootDeviceName );

    //
    // Get ARC system device name from loader block.
    //

    RtlInitAnsiString( &arcSystemDeviceString,
                       LoaderBlock->ArcHalDeviceName );

    //
    // If this is a remote boot, create an ArcName for the redirector path.
    //

    if (IoRemoteBootClient) {

        bootDiskFound = TRUE;

        RtlInitAnsiString( &deviceNameString, "\\Device\\LanmanRedirector" );
        status = RtlAnsiStringToUnicodeString( &deviceNameUnicodeString,
                                               &deviceNameString,
                                               TRUE );

        if (NT_SUCCESS( status )) {

            sprintf( arcNameBuffer,
                     "\\ArcName\\%s",
                     LoaderBlock->ArcBootDeviceName );
            RtlInitAnsiString( &arcNameString, arcNameBuffer );
            status = RtlAnsiStringToUnicodeString( &arcNameUnicodeString,
                                                   &arcNameString,
                                                   TRUE );
            if (NT_SUCCESS( status )) {

                //
                // Create symbolic link between NT device name and ARC name.
                //

                IoCreateSymbolicLink( &arcNameUnicodeString,
                                      &deviceNameUnicodeString );
                RtlFreeUnicodeString( &arcNameUnicodeString );

                //
                // We've found the system partition--store it away in the registry
                // to later be transferred to a application-friendly location.
                //
                RtlInitAnsiString( &osLoaderPathString, LoaderBlock->NtHalPathName );
                status = RtlAnsiStringToUnicodeString( &osLoaderPathUnicodeString,
                                                       &osLoaderPathString,
                                                       TRUE );

#if DBG
                if (!NT_SUCCESS( status )) {
                    DbgPrint("IopCreateArcNames: couldn't allocate unicode string for OsLoader path - %x\n", status);
                }
#endif // DBG
                if (NT_SUCCESS( status )) {

                    IopStoreSystemPartitionInformation( &deviceNameUnicodeString,
                                                        &osLoaderPathUnicodeString );

                    RtlFreeUnicodeString( &osLoaderPathUnicodeString );
                }
            }

            RtlFreeUnicodeString( &deviceNameUnicodeString );
        }
    }

    //
    // For each disk in the system do the following:
    // 1. open the device
    // 2. get its geometry
    // 3. read the MBR
    // 4. determine ARC name via disk signature and checksum
    // 5. construct ARC name.
    // In order to deal with the case of disk dissappearing before we get to this point
    // (due to a failed start on one of many disks present in the system) we ask PNP for a list
    // of all the currenttly active disks in the system. If the number of disks returned is
    // less than the IoGetConfigurationInformation()->DiskCount, then we have legacy disks
    // that we need to enumerate in the for loop.
    // In the legacy case, the ending condition for the loop is NOT the total disk on the
    // system but an arbitrary number of the max total legacy disks expected in the system..
    // Additional note: Legacy disks get assigned symbolic links AFTER all pnp enumeration is complete
    //

    totalDriverDisksFound = max(totalPnpDisksFound,totalDriverDisksFound);

    if (useLegacyEnumeration && (totalPnpDisksFound == 0)) {

        //
        // search up to a maximum arbitrary number of legacy disks
        //

        totalDriverDisksFound +=20;
    }

    for (diskNumber = 0;
         diskNumber < totalDriverDisksFound;
         diskNumber++) {

        //
        // Construct the NT name for a disk and obtain a reference.
        //

        if (pDiskNameList && (*pDiskNameList != L'\0')) {

            //
            // retrieve the first symbolic linkname from the PNP disk list
            //

            RtlInitUnicodeString(&deviceNameUnicodeString, pDiskNameList);
            pDiskNameList = pDiskNameList + (wcslen(pDiskNameList) + 1);

            status = IoGetDeviceObjectPointer( &deviceNameUnicodeString,
                                               FILE_READ_ATTRIBUTES,
                                               &fileObject,
                                               &deviceObject );

            if (NT_SUCCESS(status)) {

                //
                // since PNP gave s just asym link we have to retrieve the actual
                // disk number through an IOCTL call to the disk stack.
                // Create IRP for get device number device control.
                //

                irp = IoBuildDeviceIoControlRequest( IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                                     deviceObject,
                                                     NULL,
                                                     0,
                                                     &pnpDiskDeviceNumber,
                                                     sizeof(STORAGE_DEVICE_NUMBER),
                                                     FALSE,
                                                     &event,
                                                     &ioStatusBlock );
                if (!irp) {
                    ObDereferenceObject( fileObject );
                    continue;
                }

                KeInitializeEvent( &event,
                                   NotificationEvent,
                                   FALSE );
                status = IoCallDriver( deviceObject,
                                       irp );

                if (status == STATUS_PENDING) {
                    KeWaitForSingleObject( &event,
                                           Suspended,
                                           KernelMode,
                                           FALSE,
                                           NULL );
                    status = ioStatusBlock.Status;
                }

                if (!NT_SUCCESS( status )) {
                    ObDereferenceObject( fileObject );
                    continue;
                }

            }

            if (useLegacyEnumeration && (*pDiskNameList == L'\0') ) {

                //
                // end of pnp disks
                // if there are any legacy disks following we need to update
                // the total disk found number to cover the maximum disk number
                // a legacy disk could be at. (in a sparse name space)
                //

                if (pnpDiskDeviceNumber.DeviceNumber == 0xFFFFFFFF) {
                    pnpDiskDeviceNumber.DeviceNumber = 0;
                }

                diskNumber = max(diskNumber,pnpDiskDeviceNumber.DeviceNumber);
                totalDriverDisksFound = diskNumber + 20;

            }

        } else {

            sprintf( deviceNameBuffer,
                     "\\Device\\Harddisk%d\\Partition0",
                     diskNumber );
            RtlInitAnsiString( &deviceNameString, deviceNameBuffer );
            status = RtlAnsiStringToUnicodeString( &deviceNameUnicodeString,
                                                   &deviceNameString,
                                                   TRUE );
            if (!NT_SUCCESS( status )) {
                continue;
            }

            status = IoGetDeviceObjectPointer( &deviceNameUnicodeString,
                                               FILE_READ_ATTRIBUTES,
                                               &fileObject,
                                               &deviceObject );

            RtlFreeUnicodeString( &deviceNameUnicodeString );

            //
            // set the pnpDiskNumber value so its not used.
            //

            pnpDiskDeviceNumber.DeviceNumber = 0xFFFFFFFF;

        }


        if (!NT_SUCCESS( status )) {

            continue;
        }

        //
        // Create IRP for get drive geometry device control.
        //

        irp = IoBuildDeviceIoControlRequest( IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                             deviceObject,
                                             NULL,
                                             0,
                                             &diskGeometry,
                                             sizeof(DISK_GEOMETRY),
                                             FALSE,
                                             &event,
                                             &ioStatusBlock );
        if (!irp) {
            ObDereferenceObject( fileObject );
            continue;
        }

        KeInitializeEvent( &event,
                           NotificationEvent,
                           FALSE );
        status = IoCallDriver( deviceObject,
                               irp );

        if (status == STATUS_PENDING) {
            KeWaitForSingleObject( &event,
                                   Suspended,
                                   KernelMode,
                                   FALSE,
                                   NULL );
            status = ioStatusBlock.Status;
        }

        if (!NT_SUCCESS( status )) {
            ObDereferenceObject( fileObject );
            continue;
        }

        //
        // Get partition information for this disk.
        //

        status = IoReadPartitionTable( deviceObject,
                                       diskGeometry.BytesPerSector,
                                       TRUE,
                                       &driveLayout );

        ObDereferenceObject( fileObject );

        if (!NT_SUCCESS( status )) {
            continue;
        }

        //
        // Make sure sector size is at least 512 bytes.
        //

        if (diskGeometry.BytesPerSector < 512) {
            diskGeometry.BytesPerSector = 512;
        }

        //
        // Check to see if EZ Drive is out there on this disk.  If
        // it is then zero out the signature in the drive layout since
        // this will never be written by anyone AND change to offset to
        // actually read sector 1 rather than 0 cause that's what the
        // loader actually did.
        //

        offset.QuadPart = 0;
        HalExamineMBR( deviceObject,
                       diskGeometry.BytesPerSector,
                       (ULONG)0x55,
                       &tmpPtr );

        if (tmpPtr) {

            offset.QuadPart = diskGeometry.BytesPerSector;
            ExFreePool(tmpPtr);
#ifdef _X86_
        } else if (KeI386MachineType & MACHINE_TYPE_PC_9800_COMPATIBLE) {

            //
            //  PC 9800 compatible machines do not have a standard
            //  MBR format and use a different sector for checksuming.
            //

            offset.QuadPart = 512;
#endif //_X86_
        }

        //
        // Allocate buffer for sector read and construct the read request.
        //

        buffer = ExAllocatePool( NonPagedPoolCacheAlignedMustS,
                                 diskGeometry.BytesPerSector );

        if (buffer) {
            irp = IoBuildSynchronousFsdRequest( IRP_MJ_READ,
                                                deviceObject,
                                                buffer,
                                                diskGeometry.BytesPerSector,
                                                &offset,
                                                &event,
                                                &ioStatusBlock );

            if (!irp) {
                ExFreePool(driveLayout);
                ExFreePool(buffer);
                continue;
            }
        } else {
            ExFreePool(driveLayout);
            continue;
        }
        KeInitializeEvent( &event,
                           NotificationEvent,
                           FALSE );
        status = IoCallDriver( deviceObject,
                               irp );
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject( &event,
                                   Suspended,
                                   KernelMode,
                                   FALSE,
                                   NULL );
            status = ioStatusBlock.Status;
        }

        if (!NT_SUCCESS( status )) {
            ExFreePool(driveLayout);
            ExFreePool(buffer);
            continue;
        }

        //
        // Calculate MBR sector checksum.  Only 512 bytes are used.
        //

        checkSum = 0;
        for (i = 0; i < 128; i++) {
            checkSum += buffer[i];
        }

        //
        // For each ARC disk information record in the loader block
        // match the disk signature and checksum to determine its ARC
        // name and construct the NT ARC names symbolic links.
        //

        for (listEntry = arcInformation->DiskSignatures.Flink;
             listEntry != &arcInformation->DiskSignatures;
             listEntry = listEntry->Flink) {

            //
            // Get next record and compare disk signatures.
            //

            diskBlock = CONTAINING_RECORD( listEntry,
                                           ARC_DISK_SIGNATURE,
                                           ListEntry );

            //
            // Compare disk signatures.
            //
            // Or if there is only a single disk drive from
            // both the bios and driver viewpoints then
            // assign an arc name to that drive.
            //

            if ((singleBiosDiskFound && (totalDriverDisksFound == 1)) ||
                (diskBlock->Signature == driveLayout->Signature &&
                 !(diskBlock->CheckSum + checkSum) &&
                 diskBlock->ValidPartitionTable)) {

                //
                // Create unicode device name for physical disk.
                //

                if (pnpDiskDeviceNumber.DeviceNumber == 0xFFFFFFFF) {

                    sprintf( deviceNameBuffer,
                             "\\Device\\Harddisk%d\\Partition0",
                             diskNumber );

                } else {

                    sprintf( deviceNameBuffer,
                             "\\Device\\Harddisk%d\\Partition0",
                             pnpDiskDeviceNumber.DeviceNumber );

                }

                RtlInitAnsiString( &deviceNameString, deviceNameBuffer );
                status = RtlAnsiStringToUnicodeString( &deviceNameUnicodeString,
                                                       &deviceNameString,
                                                       TRUE );
                if (!NT_SUCCESS( status )) {
                    continue;
                }

                //
                // Create unicode ARC name for this partition.
                //

                arcName = diskBlock->ArcName;
                sprintf( arcNameBuffer,
                         "\\ArcName\\%s",
                         arcName );
                RtlInitAnsiString( &arcNameString, arcNameBuffer );
                status = RtlAnsiStringToUnicodeString( &arcNameUnicodeString,
                                                       &arcNameString,
                                                       TRUE );
                if (!NT_SUCCESS( status )) {
                    continue;
                }

                //
                // Create symbolic link between NT device name and ARC name.
                //

                IoCreateSymbolicLink( &arcNameUnicodeString,
                                      &deviceNameUnicodeString );
                RtlFreeUnicodeString( &arcNameUnicodeString );
                RtlFreeUnicodeString( &deviceNameUnicodeString );

                //
                // Create an ARC name for every partition on this disk.
                //

                for (partitionNumber = 0;
                     partitionNumber < driveLayout->PartitionCount;
                     partitionNumber++) {

                    //
                    // Create unicode NT device name.
                    //

                    if (pnpDiskDeviceNumber.DeviceNumber == 0xFFFFFFFF) {

                        sprintf( deviceNameBuffer,
                                 "\\Device\\Harddisk%d\\Partition%d",
                                 diskNumber,
                                 partitionNumber+1 );


                    } else {

                        sprintf( deviceNameBuffer,
                                 "\\Device\\Harddisk%d\\Partition%d",
                                 pnpDiskDeviceNumber.DeviceNumber,
                                 partitionNumber+1 );

                    }

                    RtlInitAnsiString( &deviceNameString, deviceNameBuffer );
                    status = RtlAnsiStringToUnicodeString( &deviceNameUnicodeString,
                                                           &deviceNameString,
                                                           TRUE );
                    if (!NT_SUCCESS( status )) {
                        continue;
                    }

                    //
                    // Create unicode ARC name for this partition and
                    // check to see if this is the boot disk.
                    //

                    sprintf( arcNameBuffer,
                             "%spartition(%d)",
                             arcName,
                             partitionNumber+1 );
                    RtlInitAnsiString( &arcNameString, arcNameBuffer );
                    if (RtlEqualString( &arcNameString,
                                        &arcBootDeviceString,
                                        TRUE )) {
                        bootDiskFound = TRUE;
                    }

                    //
                    // See if this is the system partition.
                    //
                    if (RtlEqualString( &arcNameString,
                                        &arcSystemDeviceString,
                                        TRUE )) {
                        //
                        // We've found the system partition--store it away in the registry
                        // to later be transferred to a application-friendly location.
                        //
                        RtlInitAnsiString( &osLoaderPathString, LoaderBlock->NtHalPathName );
                        status = RtlAnsiStringToUnicodeString( &osLoaderPathUnicodeString,
                                                               &osLoaderPathString,
                                                               TRUE );

#if DBG
                        if (!NT_SUCCESS( status )) {
                            DbgPrint("IopCreateArcNames: couldn't allocate unicode string for OsLoader path - %x\n", status);
                        }
#endif // DBG
                        if (NT_SUCCESS( status )) {

                            IopStoreSystemPartitionInformation( &deviceNameUnicodeString,
                                                                &osLoaderPathUnicodeString );

                            RtlFreeUnicodeString( &osLoaderPathUnicodeString );
                        }
                    }

                    //
                    // Add the NT ARC namespace prefix to the ARC name constructed.
                    //

                    sprintf( arcNameBuffer,
                             "\\ArcName\\%spartition(%d)",
                             arcName,
                             partitionNumber+1 );
                    RtlInitAnsiString( &arcNameString, arcNameBuffer );
                    status = RtlAnsiStringToUnicodeString( &arcNameUnicodeString,
                                                           &arcNameString,
                                                           TRUE );
                    if (!NT_SUCCESS( status )) {
                        continue;
                    }

                    //
                    // Create symbolic link between NT device name and ARC name.
                    //

                    IoCreateSymbolicLink( &arcNameUnicodeString,
                                          &deviceNameUnicodeString );
                    RtlFreeUnicodeString( &arcNameUnicodeString );
                    RtlFreeUnicodeString( &deviceNameUnicodeString );
                }

            } else {

#if DBG
                //
                // Check key indicators to see if this condition may be
                // caused by a viral infection.
                //

                if (diskBlock->Signature == driveLayout->Signature &&
                    (diskBlock->CheckSum + checkSum) != 0 &&
                    diskBlock->ValidPartitionTable) {
                    DbgPrint("IopCreateArcNames: Virus or duplicate disk signatures\n");
                }
#endif
            }
        }

        ExFreePool( driveLayout );
        ExFreePool( buffer );
    }

    if (!bootDiskFound) {

        //
        // Locate the disk block that represents the boot device.
        //

        diskBlock = NULL;
        for (listEntry = arcInformation->DiskSignatures.Flink;
             listEntry != &arcInformation->DiskSignatures;
             listEntry = listEntry->Flink) {

            diskBlock = CONTAINING_RECORD( listEntry,
                                           ARC_DISK_SIGNATURE,
                                           ListEntry );
            if (strcmp( diskBlock->ArcName, LoaderBlock->ArcBootDeviceName ) == 0) {
                break;
            }
            diskBlock = NULL;
        }

        if (diskBlock) {

            //
            // This could be a CdRom boot.  Search all of the NT CdRoms
            // to locate a checksum match on the diskBlock found.  If
            // there is a match, assign the ARC name to the CdRom.
            //

            irp = NULL;
            buffer = ExAllocatePool( NonPagedPoolCacheAlignedMustS,
                                     2048 );
            if (buffer) {

                //
                // Construct the NT names for CdRoms and search each one
                // for a checksum match.  If found, create the ARC Name
                // symbolic link.
                //

                for (diskNumber = 0; TRUE; diskNumber++) {

                    sprintf( deviceNameBuffer,
                             "\\Device\\CdRom%d",
                             diskNumber );

                    RtlInitAnsiString( &deviceNameString, deviceNameBuffer );
                    status = RtlAnsiStringToUnicodeString( &deviceNameUnicodeString,
                                                           &deviceNameString,
                                                           TRUE );
                    if (NT_SUCCESS( status )) {

                        status = IoGetDeviceObjectPointer( &deviceNameUnicodeString,
                                                           FILE_READ_ATTRIBUTES,
                                                           &fileObject,
                                                           &deviceObject );
                        if (!NT_SUCCESS( status )) {

                            //
                            // All CdRoms have been processed.
                            //

                            RtlFreeUnicodeString( &deviceNameUnicodeString );
                            break;
                        }

                        //
                        // Read the block for the checksum calculation.
                        //

                        offset.QuadPart = 0x8000;
                        irp = IoBuildSynchronousFsdRequest( IRP_MJ_READ,
                                                            deviceObject,
                                                            buffer,
                                                            2048,
                                                            &offset,
                                                            &event,
                                                            &ioStatusBlock );
                        checkSum = 0;
                        if (irp) {
                            KeInitializeEvent( &event,
                                               NotificationEvent,
                                               FALSE );
                            status = IoCallDriver( deviceObject,
                                                   irp );
                            if (status == STATUS_PENDING) {
                                KeWaitForSingleObject( &event,
                                                       Suspended,
                                                       KernelMode,
                                                       FALSE,
                                                       NULL );
                                status = ioStatusBlock.Status;
                            }

                            if (NT_SUCCESS( status )) {

                                //
                                // Calculate MBR sector checksum.
                                // 2048 bytes are used.
                                //

                                for (i = 0; i < 2048 / sizeof(ULONG) ; i++) {
                                    checkSum += buffer[i];
                                }
                            }
                        }
                        ObDereferenceObject( fileObject );

                        if (!(diskBlock->CheckSum + checkSum)) {

                            //
                            // This is the boot CdRom.  Create the symlink for
                            // the ARC name from the loader block.
                            //

                            sprintf( arcNameBuffer,
                                     "\\ArcName\\%s",
                                     LoaderBlock->ArcBootDeviceName );
                            RtlInitAnsiString( &arcNameString, arcNameBuffer );
                            status = RtlAnsiStringToUnicodeString( &arcNameUnicodeString,
                                                                   &arcNameString,
                                                                   TRUE );
                            if (NT_SUCCESS( status )) {

                                IoCreateSymbolicLink( &arcNameUnicodeString,
                                                      &deviceNameUnicodeString );
                                RtlFreeUnicodeString( &arcNameUnicodeString );
                            }
                            RtlFreeUnicodeString( &deviceNameUnicodeString );
                            break;
                        }
                        RtlFreeUnicodeString( &deviceNameUnicodeString );
                    }
                }
                ExFreePool(buffer);
            }
        }
    }

    if (diskList) {
        ExFreePool(diskList);
    }
}

GENERIC_MAPPING IopFileMapping = {
    STANDARD_RIGHTS_READ |
        FILE_READ_DATA | FILE_READ_ATTRIBUTES | FILE_READ_EA | SYNCHRONIZE,
    STANDARD_RIGHTS_WRITE |
        FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA | FILE_APPEND_DATA | SYNCHRONIZE,
    STANDARD_RIGHTS_EXECUTE |
        SYNCHRONIZE | FILE_READ_ATTRIBUTES | FILE_EXECUTE,
    FILE_ALL_ACCESS
};

GENERIC_MAPPING IopCompletionMapping = {
    STANDARD_RIGHTS_READ |
        IO_COMPLETION_QUERY_STATE,
    STANDARD_RIGHTS_WRITE |
        IO_COMPLETION_MODIFY_STATE,
    STANDARD_RIGHTS_EXECUTE |
        SYNCHRONIZE,
    IO_COMPLETION_ALL_ACCESS
};

BOOLEAN
IopCreateObjectTypes(
    VOID
    )

/*++

Routine Description:

    This routine creates the object types used by the I/O system and its
    components.  The object types created are:

        Adapter
        Controller
        Device
        Driver
        File
        I/O Completion

Arguments:

    None.

Return Value:

    The function value is a BOOLEAN indicating whether or not the object
    types were successfully created.


--*/

{
    OBJECT_TYPE_INITIALIZER objectTypeInitializer;
    UNICODE_STRING nameString;

    //
    // Initialize the common fields of the Object Type Initializer record
    //

    RtlZeroMemory( &objectTypeInitializer, sizeof( objectTypeInitializer ) );
    objectTypeInitializer.Length = sizeof( objectTypeInitializer );
    objectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    objectTypeInitializer.GenericMapping = IopFileMapping;
    objectTypeInitializer.PoolType = NonPagedPool;
    objectTypeInitializer.ValidAccessMask = FILE_ALL_ACCESS;
    objectTypeInitializer.UseDefaultObject = TRUE;


    //
    // Create the object type for adapter objects.
    //

    RtlInitUnicodeString( &nameString, L"Adapter" );
    // objectTypeInitializer.DefaultNonPagedPoolCharge = sizeof( struct _ADAPTER_OBJECT );
    if (!NT_SUCCESS( ObCreateObjectType( &nameString,
                                      &objectTypeInitializer,
                                      (PSECURITY_DESCRIPTOR) NULL,
                                      &IoAdapterObjectType ))) {
        return FALSE;
    }

#ifdef _PNP_POWER_

    //
    // Create the object type for device helper objects.
    //

    RtlInitUnicodeString( &nameString, L"DeviceHandler" );
    if (!NT_SUCCESS( ObCreateObjectType( &nameString,
                                      &objectTypeInitializer,
                                      (PSECURITY_DESCRIPTOR) NULL,
                                      &IoDeviceHandlerObjectType ))) {
        return FALSE;
    }
    IoDeviceHandlerObjectSize = sizeof(DEVICE_HANDLER_OBJECT);

#endif

    //
    // Create the object type for controller objects.
    //

    RtlInitUnicodeString( &nameString, L"Controller" );
    objectTypeInitializer.DefaultNonPagedPoolCharge = sizeof( CONTROLLER_OBJECT );
    if (!NT_SUCCESS( ObCreateObjectType( &nameString,
                                      &objectTypeInitializer,
                                      (PSECURITY_DESCRIPTOR) NULL,
                                      &IoControllerObjectType ))) {
        return FALSE;
    }

    //
    // Create the object type for device objects.
    //

    RtlInitUnicodeString( &nameString, L"Device" );
    objectTypeInitializer.DefaultNonPagedPoolCharge = sizeof( DEVICE_OBJECT );
    objectTypeInitializer.ParseProcedure = IopParseDevice;
    objectTypeInitializer.DeleteProcedure = IopDeleteDevice;
    objectTypeInitializer.SecurityProcedure = IopGetSetSecurityObject;
    objectTypeInitializer.QueryNameProcedure = (OB_QUERYNAME_METHOD)NULL;
    if (!NT_SUCCESS( ObCreateObjectType( &nameString,
                                      &objectTypeInitializer,
                                      (PSECURITY_DESCRIPTOR) NULL,
                                      &IoDeviceObjectType ))) {
        return FALSE;
    }

    //
    // Create the object type for driver objects.
    //

    RtlInitUnicodeString( &nameString, L"Driver" );
    objectTypeInitializer.DefaultNonPagedPoolCharge = sizeof( DRIVER_OBJECT );
    objectTypeInitializer.ParseProcedure = (OB_PARSE_METHOD) NULL;
    objectTypeInitializer.DeleteProcedure = IopDeleteDriver;
    objectTypeInitializer.SecurityProcedure = (OB_SECURITY_METHOD) NULL;
    objectTypeInitializer.QueryNameProcedure = (OB_QUERYNAME_METHOD)NULL;
    if (!NT_SUCCESS( ObCreateObjectType( &nameString,
                                      &objectTypeInitializer,
                                      (PSECURITY_DESCRIPTOR) NULL,
                                      &IoDriverObjectType ))) {
        return FALSE;
    }

    //
    // Create the object type for I/O completion objects.
    //

    RtlInitUnicodeString( &nameString, L"IoCompletion" );
    objectTypeInitializer.DefaultNonPagedPoolCharge = sizeof( KQUEUE );
    objectTypeInitializer.InvalidAttributes = OBJ_PERMANENT | OBJ_OPENLINK;
    objectTypeInitializer.GenericMapping = IopCompletionMapping;
    objectTypeInitializer.ValidAccessMask = IO_COMPLETION_ALL_ACCESS;
    objectTypeInitializer.DeleteProcedure = IopDeleteIoCompletion;
    if (!NT_SUCCESS( ObCreateObjectType( &nameString,
                                      &objectTypeInitializer,
                                      (PSECURITY_DESCRIPTOR) NULL,
                                      &IoCompletionObjectType ))) {
        return FALSE;
    }

    //
    // Create the object type for file objects.
    //

    RtlInitUnicodeString( &nameString, L"File" );
    objectTypeInitializer.DefaultPagedPoolCharge = IO_FILE_OBJECT_PAGED_POOL_CHARGE;
    objectTypeInitializer.DefaultNonPagedPoolCharge = IO_FILE_OBJECT_NON_PAGED_POOL_CHARGE +
                                                      sizeof( FILE_OBJECT );
    objectTypeInitializer.InvalidAttributes = OBJ_PERMANENT | OBJ_EXCLUSIVE | OBJ_OPENLINK;
    objectTypeInitializer.GenericMapping = IopFileMapping;
    objectTypeInitializer.ValidAccessMask = FILE_ALL_ACCESS;
    objectTypeInitializer.MaintainHandleCount = TRUE;
    objectTypeInitializer.CloseProcedure = IopCloseFile;
    objectTypeInitializer.DeleteProcedure = IopDeleteFile;
    objectTypeInitializer.ParseProcedure = IopParseFile;
    objectTypeInitializer.SecurityProcedure = IopGetSetSecurityObject;
    objectTypeInitializer.QueryNameProcedure = IopQueryName;
    objectTypeInitializer.UseDefaultObject = FALSE;

    if (!NT_SUCCESS( ObCreateObjectType( &nameString,
                                      &objectTypeInitializer,
                                      (PSECURITY_DESCRIPTOR) NULL,
                                      &IoFileObjectType ))) {
        return FALSE;
    }

    return TRUE;
}

PTREE_ENTRY
IopCreateEntry(
    IN PUNICODE_STRING GroupName
    )

/*++

Routine Description:

    This routine creates an entry for the specified group name suitable for
    being inserted into the group name tree.

Arguments:

    GroupName - Specifies the name of the group for the entry.

Return Value:

    The function value is a pointer to the created entry.


--*/

{
    PTREE_ENTRY treeEntry;

    //
    // Allocate and initialize an entry suitable for placing into the group
    // name tree.
    //

    treeEntry = ExAllocatePool( PagedPool,
                                sizeof( TREE_ENTRY ) + GroupName->Length );
    if (!treeEntry) {
        ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
    }

    RtlZeroMemory( treeEntry, sizeof( TREE_ENTRY ) );
    treeEntry->GroupName.Length = GroupName->Length;
    treeEntry->GroupName.MaximumLength = GroupName->Length;
    treeEntry->GroupName.Buffer = (PWCHAR) (treeEntry + 1);
    RtlCopyMemory( treeEntry->GroupName.Buffer,
                   GroupName->Buffer,
                   GroupName->Length );

    return treeEntry;
}

BOOLEAN
IopCreateRootDirectories(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to create the object manager directory objects
    to contain the various device and file system driver objects.

Arguments:

    None.

Return Value:

    The function value is a BOOLEAN indicating whether or not the directory
    objects were successfully created.


--*/

{
    HANDLE handle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING nameString;
    NTSTATUS status;

    //
    // Create the root directory object for the \Driver directory.
    //

    RtlInitUnicodeString( &nameString, L"\\Driver" );
    InitializeObjectAttributes( &objectAttributes,
                                &nameString,
                                OBJ_PERMANENT,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    status = NtCreateDirectoryObject( &handle,
                                      DIRECTORY_ALL_ACCESS,
                                      &objectAttributes );
    if (!NT_SUCCESS( status )) {
        return FALSE;
    } else {
        (VOID) NtClose( handle );
    }

    //
    // Create the root directory object for the \FileSystem directory.
    //

    RtlInitUnicodeString( &nameString, L"\\FileSystem" );

    status = NtCreateDirectoryObject( &handle,
                                      DIRECTORY_ALL_ACCESS,
                                      &objectAttributes );
    if (!NT_SUCCESS( status )) {
        return FALSE;
    } else {
        (VOID) NtClose( handle );
    }

    return TRUE;
}

VOID
IopFreeGroupTree(
    PTREE_ENTRY TreeEntry
    )

/*++

Routine Description:

    This routine is invoked to free a node from the group dependency tree.
    It is invoked the first time with the root of the tree, and thereafter
    recursively to walk the tree and remove the nodes.

Arguments:

    TreeEntry - Supplies a pointer to the node to be freed.

Return Value:

    None.

--*/

{
    //
    // Simply walk the tree in ascending order from the bottom up and free
    // each node along the way.
    //

    if (TreeEntry->Left) {
        IopFreeGroupTree( TreeEntry->Left );
    }

    if (TreeEntry->Sibling) {
        IopFreeGroupTree( TreeEntry->Sibling );
    }

    if (TreeEntry->Right) {
        IopFreeGroupTree( TreeEntry->Right );
    }

    //
    // All of the children and siblings for this node have been freed, so
    // now free this node as well.
    //

    ExFreePool( TreeEntry );
}

NTSTATUS
IopInitializeAttributesAndCreateObject(
    IN PUNICODE_STRING ObjectName,
    IN OUT POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PDRIVER_OBJECT *DriverObject
    )

/*++

Routine Description:

    This routine is invoked to initialize a set of object attributes and
    to create a driver object.

Arguments:

    ObjectName - Supplies the name of the driver object.

    ObjectAttributes - Supplies a pointer to the object attributes structure
        to be initialized.

    DriverObject - Supplies a variable to receive a pointer to the resultant
        created driver object.

Return Value:

    The function value is the final status of the operation.

--*/

{
    NTSTATUS status;

    //
    // Simply initialize the object attributes and create the driver object.
    //

    InitializeObjectAttributes( ObjectAttributes,
                                ObjectName,
                                OBJ_PERMANENT | OBJ_CASE_INSENSITIVE,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    status = ObCreateObject( KeGetPreviousMode(),
                             IoDriverObjectType,
                             ObjectAttributes,
                             KernelMode,
                             (PVOID) NULL,
                             (ULONG) (sizeof( DRIVER_OBJECT ) + sizeof ( DRIVER_EXTENSION )),
                             0,
                             0,
                             (PVOID *)DriverObject );
    return status;
}

BOOLEAN
IopInitializeBootDrivers(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    OUT PDRIVER_OBJECT *PreviousDriver
    )

/*++

Routine Description:

    This routine is invoked to initialize the boot drivers that were loaded
    by the OS Loader.  The list of drivers is provided as part of the loader
    parameter block.

Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block, created
        by the OS Loader.

    Previous Driver - Supplies a variable to receive the address of the
        driver object chain created by initializing the drivers.

Return Value:

    The function value is a BOOLEAN indicating whether or not the boot
    drivers were successfully initialized.

--*/

{
    UNICODE_STRING completeName;
    UNICODE_STRING rawFsName;
    NTSTATUS status;
    PLIST_ENTRY nextEntry;
    PLIST_ENTRY entry;
    PREINIT_PACKET reinitEntry;
    PBOOT_DRIVER_LIST_ENTRY bootDriver;
    HANDLE keyHandle;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    PDRIVER_OBJECT driverObject;
    USHORT i, j;
    PLDR_DATA_TABLE_ENTRY driverEntry;
    PLDR_DATA_TABLE_ENTRY dllEntry;
    UNICODE_STRING groupName;
    PTREE_ENTRY treeEntry;
    PDRIVER_INFORMATION driverInfo;
    START_CONTEXT startContext;
    BOOLEAN moreProcessing;
    BOOLEAN newDevice;
    BOOLEAN textModeSetup = FALSE;
    BOOLEAN bootReinitDriversFound;
    BOOLEAN bootConfigsOK;

    UNREFERENCED_PARAMETER( PreviousDriver );

    //
    // Initialize the built-in RAW file system driver.
    //

    RtlInitUnicodeString( &rawFsName, L"\\FileSystem\\RAW" );
    RtlInitUnicodeString( &completeName, L"" );
    if (!IopInitializeBuiltinDriver( &rawFsName,
                                     &completeName,
                                     RawInitialize,
                                     NULL,
                                     textModeSetup )) {
#if DBG
        DbgPrint( "IOINIT: Failed to initialize RAW filsystem \n" );

#endif

        return FALSE;
    }

    //
    // Determine number of group orders and build a list_entry array to link all the drivers
    // together based on their groups.
    //

    IopGroupIndex = IopGetGroupOrderIndex(NULL);
    if (IopGroupIndex == NO_MORE_GROUP) {
        return FALSE;
    }

    IopGroupTable = (PLIST_ENTRY) ExAllocatePool(PagedPool, IopGroupIndex * sizeof (LIST_ENTRY));
    if (IopGroupTable == NULL) {
        return FALSE;
    }
    for (i = 0; i < IopGroupIndex; i++) {
        InitializeListHead(&IopGroupTable[i]);
    }

    PnpAsyncOk = FALSE;

    //
    // Call DllInitialize for driver dependent DLLs.
    //

    nextEntry = LoaderBlock->LoadOrderListHead.Flink;
    while (nextEntry != &LoaderBlock->LoadOrderListHead) {
        dllEntry = CONTAINING_RECORD(nextEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        if (dllEntry->Flags & LDRP_DRIVER_DEPENDENT_DLL) {
            (VOID)MmCallDllInitialize(dllEntry);
        }
        nextEntry = nextEntry->Flink;
    }
    //
    // Allocate pool to store driver's start information.
    // All the driver info records with the same group value will be linked into a list.
    //

    nextEntry = LoaderBlock->BootDriverListHead.Flink;
    while (nextEntry != &LoaderBlock->BootDriverListHead) {
        bootDriver = CONTAINING_RECORD( nextEntry,
                                        BOOT_DRIVER_LIST_ENTRY,
                                        Link );
        driverEntry = bootDriver->LdrEntry;
        driverInfo = (PDRIVER_INFORMATION) ExAllocatePool(
                        PagedPool, sizeof(DRIVER_INFORMATION));
        if (driverInfo) {
            RtlZeroMemory(driverInfo, sizeof(DRIVER_INFORMATION));
            InitializeListHead(&driverInfo->Link);
            driverInfo->DataTableEntry = bootDriver;

            //
            // Open the driver's registry key to find out if this is a
            // filesystem or a driver.
            //

            status = IopOpenRegistryKeyEx( &keyHandle,
                                           (HANDLE)NULL,
                                           &bootDriver->RegistryPath,
                                           KEY_READ
                                           );
            if (!NT_SUCCESS( status )) {
                ExFreePool(driverInfo);
            } else {
                driverInfo->ServiceHandle = keyHandle;
                j = IopGetGroupOrderIndex(keyHandle);
                if (j == SETUP_RESERVED_GROUP) {

                    textModeSetup = TRUE;

                    //
                    // Special handling for setupdd.sys
                    //

                    status = IopGetDriverNameFromKeyNode( keyHandle,
                                                          &completeName );
                    if (NT_SUCCESS(status)) {
                        driverObject = IopInitializeBuiltinDriver(
                                           &completeName,
                                           &bootDriver->RegistryPath,
                                           (PDRIVER_INITIALIZE) driverEntry->EntryPoint,
                                           driverEntry,
                                           textModeSetup );
                        ExFreePool( completeName.Buffer );
                        NtClose(keyHandle);
                        ExFreePool(driverInfo);
                        if (driverObject) {

                            //
                            // Once we successfully initialized the setupdd.sys, we are ready
                            // to notify it all the root enumerated devices.
                            //

                            IopNotifySetupDevices(IopRootDeviceNode);
                        } else {
                            ExFreePool(IopGroupTable);
                            return FALSE;
                        }
                    }

                } else {
                    driverInfo->TagPosition = IopGetDriverTagPriority(keyHandle);
                    IopInsertDriverList(&IopGroupTable[j], driverInfo);
                }
            }
        }
        nextEntry = nextEntry->Flink;
    }

    //
    // Process each driver base on its group.  The group with lower index number (higher
    // priority) is processed first.
    //

    for (i = 0; i < IopGroupIndex; i++) {
        nextEntry = IopGroupTable[i].Flink;
        while (nextEntry != &IopGroupTable[i]) {

            driverInfo = CONTAINING_RECORD(nextEntry, DRIVER_INFORMATION, Link);
            keyHandle = driverInfo->ServiceHandle;
            bootDriver = driverInfo->DataTableEntry;
            driverEntry = bootDriver->LdrEntry;
            driverInfo->Processed = TRUE;

            //
            // call the driver's driver entry
            //
            // See if this driver has an ObjectName value.  If so, this value
            // overrides the default ("\Driver" or "\FileSystem").
            //

            status = IopGetDriverNameFromKeyNode( keyHandle,
                                                  &completeName );
            if (!NT_SUCCESS( status )) {

#if DBG
                DbgPrint( "IOINIT: Could not get driver name for %wZ\n",
                          &bootDriver->RegistryPath );
#endif // DBG

                driverInfo->Failed = TRUE;
            } else {

                status = IopGetRegistryValue( keyHandle,
                                              L"Group",
                                              &keyValueInformation );
                if (NT_SUCCESS( status )) {

                    if (keyValueInformation->DataLength) {
                        groupName.Length = (USHORT) keyValueInformation->DataLength;
                        groupName.MaximumLength = groupName.Length;
                        groupName.Buffer = (PWSTR) ((PUCHAR) keyValueInformation + keyValueInformation->DataOffset);
                        treeEntry = IopLookupGroupName( &groupName, TRUE );
                    } else {
                        treeEntry = (PTREE_ENTRY) NULL;
                    }
                    ExFreePool( keyValueInformation );
                } else {
                    treeEntry = (PTREE_ENTRY) NULL;
                }

                //
                // Check if this bus has a BOOT config.
                // Buses with BOOT configs allow assignment of BOOT configs on their level.
                //

                bootConfigsOK = TRUE;
                status = IopGetRegistryValue( keyHandle,
                                              L"HasBootConfig",
                                              &keyValueInformation );
                if (NT_SUCCESS(status)) {
                    if (*(PULONG)((PUCHAR)keyValueInformation + keyValueInformation->DataOffset) == 0) {
                        bootConfigsOK = FALSE;
                    }
                    ExFreePool(keyValueInformation);
                }

                driverObject = NULL;
                if (IopCheckDependencies( keyHandle )) {
                    //
                    // The driver may already be initialized by IopInitializeBootFilterDriver
                    // if it is boot filter driver.
                    // If not, initialize it.
                    //

                    driverObject = driverInfo->DriverObject;
                    if (driverObject == NULL) {
                        driverObject = IopInitializeBuiltinDriver(
                                           &completeName,
                                           &bootDriver->RegistryPath,
                                           (PDRIVER_INITIALIZE) driverEntry->EntryPoint,
                                           driverEntry,
                                           textModeSetup);
                        //
                        // Pnp might unload the driver before we get a chance to look at this. So take an extra
                        // reference.
                        //
                        if (driverObject) {
                            ObReferenceObject(driverObject);
                        }
                    }
                }
                if (driverObject) {
                    if (treeEntry) {
                        treeEntry->DriversLoaded++;
                    }
                    driverInfo->DriverObject = driverObject;

                } else {
                    driverInfo->Failed = TRUE;
                }
                ExFreePool( completeName.Buffer );
            }
            if (!driverInfo->Failed) {

                USHORT group;

                IopAddDevicesToBootDriver(driverObject);

                //
                // Scan the hardware tree looking for devices which need
                // resources or starting.
                //

                IopRequestDeviceAction(NULL, ReenumerateBootDevices, NULL, (PNTSTATUS)&bootConfigsOK);

            }

            //
            // Before processing next boot driver, wait for IoRequestDeviceRemoval complete.
            // The driver to be processed may need the resources being released by
            // IoRequestDeviceRemoval.  (For drivers report detected BOOT device if they fail to
            // get the resources in their DriverEntry.  They will fail and we will bugcheck with
            // inaccessible boot device.)
            //

            if (!IopWaitForBootDevicesDeleted()) {
                return FALSE;
            }

            nextEntry = nextEntry->Flink;
        }

        //
        // If we are done with Bus driver group, then it's time to reserved the Hal resources
        // and reserve boot resources
        //

        if (i == BUS_DRIVER_GROUP) {
            if (textModeSetup == FALSE) {
                //
                // BUGBUG - There problems with Async ops, disable for now.
                //
                // PnpAsyncOk = TRUE;
            }

            //
            // Reserve BOOT configs on Internal bus 0.
            //

            IopReserveLegacyBootResources(Internal, 0);
            IopReserveResourcesRoutine = IopAllocateBootResources;
            ASSERT(IopInitHalResources == NULL);
            ASSERT(IopInitReservedResourceList == NULL);
            IopBootConfigsReserved = TRUE;

        }
    }

    //
    // If we started a network boot driver, then imitate what DHCP does
    // in sending IOCTLs.
    //

    if (IoRemoteBootClient) {
        status = IopStartTcpIpForRemoteBoot(LoaderBlock);
        if (!NT_SUCCESS(status)) {
            KeBugCheckEx( NETWORK_BOOT_INITIALIZATION_FAILED,
                          3,
                          status,
                          0,
                          0 );
        }
    }

    //
    // Scan the hardware tree looking for devices which need
    // resources or starting.
    //
    PnPBootDriversLoaded = TRUE;
    IopResourcesReleased = TRUE;

    IopRequestDeviceAction(NULL, RestartEnumeration, NULL, NULL);

    //
    // If start irps are handled asynchronously, we need to make sure all the boot devices
    // started before continue.
    //

    if (!IopWaitForBootDevicesStarted()) {
        return FALSE;
    }

    bootReinitDriversFound = FALSE;

    while (entry = ExInterlockedRemoveHeadList( &IopBootDriverReinitializeQueueHead, &IopDatabaseLock )) {
        bootReinitDriversFound = TRUE;
        reinitEntry = CONTAINING_RECORD( entry, REINIT_PACKET, ListEntry );
        reinitEntry->DriverObject->DriverExtension->Count++;
        reinitEntry->DriverObject->Flags &= ~DRVO_BOOTREINIT_REGISTERED;
        reinitEntry->DriverReinitializationRoutine( reinitEntry->DriverObject,
                                                    reinitEntry->Context,
                                                    reinitEntry->DriverObject->DriverExtension->Count );
        ExFreePool( reinitEntry );
    }

    //
    // If there were any drivers that registered for boot reinitialization, then
    // we need to wait one more time to make sure we catch any additional
    // devices that were created in response to the reinitialization callback.
    //

    if (bootReinitDriversFound && !IopWaitForBootDevicesStarted()) {
        return FALSE;
    }

    //
    // Link NT device names to ARC names now that all of the boot drivers
    // have intialized.
    //

    IopCreateArcNames( LoaderBlock );

    //
    // Find and mark the boot partition device object so that if a subsequent
    // access or mount of the device during initialization occurs, an more
    // bugcheck can be produced that helps the user understand why the system
    // is failing to boot and run properly.  This occurs when either one of the
    // device drivers or the file system fails to load, or when the file system
    // cannot mount the device for some other reason.
    //

    if (!IopMarkBootPartition( LoaderBlock )) {
        return FALSE;
    }

    //
    // BUGBUG - There problems with Async ops, disable for now.
    //
    // PnpAsyncOk = TRUE;
    PnPBootDriversInitialized = TRUE;

    //
    // Go thru every driver that we initialized.  If it supports AddDevice entry and
    // did not create any device object after we start it.  We mark it as failure so
    // text mode setup knows this driver is not needed.
    //

    for (i = 0; i < IopGroupIndex; i++) {
        while (IsListEmpty(&IopGroupTable[i]) == FALSE) {

            BOOLEAN failed;

            nextEntry = RemoveHeadList(&IopGroupTable[i]);
            driverInfo = CONTAINING_RECORD(nextEntry, DRIVER_INFORMATION, Link);
            driverObject = driverInfo->DriverObject;

            if (textModeSetup                    &&
                (driverInfo->Failed == FALSE)    &&
                !IopIsLegacyDriver(driverObject) &&
                (driverObject->DeviceObject == NULL) &&
                !(driverObject->Flags & DRVO_REINIT_REGISTERED)) {

                //
                // If failed is not set and it's not a legacy driver and it has no device object
                // tread it as failure case.
                //

                driverInfo->Failed = TRUE;

                if (!(driverObject->Flags & DRVO_UNLOAD_INVOKED)) {
                    driverObject->Flags |= DRVO_UNLOAD_INVOKED;
                    if (driverObject->DriverUnload) {
                        driverObject->DriverUnload(driverObject);
                    }
                    ObMakeTemporaryObject( driverObject );  // Reference taken while inserting into the object table.
                    ObDereferenceObject(driverObject);      // Reference taken when getting driver object pointer.
                }
            }
            if (driverObject) {
                ObDereferenceObject(driverObject);          // Reference taken specifically for text mode setup.
            }

            if (driverInfo->Failed) {
                driverInfo->DataTableEntry->LdrEntry->Flags |= LDRP_FAILED_BUILTIN_LOAD;
            }
            NtClose(driverInfo->ServiceHandle);
            ExFreePool(driverInfo);
        }
    }

    ExFreePool(IopGroupTable);

    //
    // Initialize the drivers necessary to dump all of physical memory to the
    // disk if the system is configured to do so.
    //


    return TRUE;
}

PDRIVER_OBJECT
IopInitializeBuiltinDriver(
    IN PUNICODE_STRING DriverName,
    IN PUNICODE_STRING RegistryPath,
    IN PDRIVER_INITIALIZE DriverInitializeRoutine,
    IN PLDR_DATA_TABLE_ENTRY DriverEntry,
    IN BOOLEAN TextModeSetup
    )

/*++

Routine Description:

    This routine is invoked to initialize a built-in driver.

Arguments:

    DriverName - Specifies the name to be used in creating the driver object.

    RegistryPath - Specifies the path to be used by the driver to get to
        the registry.

    DriverInitializeRoutine - Specifies the initialization entry point of
        the built-in driver.

    DriverEntry - Specifies the driver data table entry to determine if the
        driver is a wdm driver.

    TextModeSetup - Specifies if this is TextMode setup boot.

Return Value:

    The function returns a pointer to a DRIVER_OBJECT if the built-in
    driver successfully initialized.  Otherwise, a value of NULL is returned.

--*/

{
    HANDLE handle;
    PDRIVER_OBJECT driverObject;
    PDRIVER_OBJECT tmpDriverObject;
    OBJECT_ATTRIBUTES objectAttributes;
    PWSTR buffer;
    NTSTATUS status;
    HANDLE serviceHandle;
    PWSTR pserviceName;
    USHORT serviceNameLength;
    PDRIVER_EXTENSION driverExtension;
    PIMAGE_NT_HEADERS ntHeaders;
    PVOID imageBase;
#if DBG
    LARGE_INTEGER stime, etime;
    ULONG dtime;
#endif
    PLIST_ENTRY entry;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;

    //
    // Begin by creating the driver object.
    //

    status = IopInitializeAttributesAndCreateObject( DriverName,
                                                     &objectAttributes,
                                                     &driverObject );
    if (!NT_SUCCESS( status )) {
        return NULL;
    }

    //
    // Initialize the driver object.
    //

    InitializeDriverObject( driverObject );
    driverObject->DriverInit = DriverInitializeRoutine;

    //
    // Insert the driver object into the object table.
    //

    status = ObInsertObject( driverObject,
                             NULL,
                             FILE_READ_DATA,
                             0,
                             (PVOID *) NULL,
                             &handle );

    if (!NT_SUCCESS( status )) {
        return NULL;
    }

    //
    // Reference the handle and obtain a pointer to the driver object so that
    // the handle can be deleted without the object going away.
    //

    status = ObReferenceObjectByHandle( handle,
                                        0,
                                        IoDriverObjectType,
                                        KernelMode,
                                        (PVOID *) &tmpDriverObject,
                                        (POBJECT_HANDLE_INFORMATION) NULL );
    ASSERT(status == STATUS_SUCCESS);
    //
    // Fill in the DriverSection so the image will be automatically unloaded on
    // failures. We should use the entry from the PsModuleList.
    //

    entry = PsLoadedModuleList.Flink;
    while (entry != &PsLoadedModuleList && DriverEntry) {
        DataTableEntry = CONTAINING_RECORD(entry,
                                           LDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);
        if (RtlEqualString((PSTRING)&DriverEntry->BaseDllName,
                    (PSTRING)&DataTableEntry->BaseDllName,
                    TRUE
                    )) {
            driverObject->DriverSection = DataTableEntry;
            break;
        }
        entry = entry->Flink;
    }

    //
    // The boot process takes a while loading drivers.   Indicate that
    // progress is being made.
    //

    InbvIndicateProgress();

    //
    // Get start and sice for the DriverObject.
    //

    if (DriverEntry) {
        imageBase = DriverEntry->DllBase;
        ntHeaders = RtlImageNtHeader(imageBase);
        driverObject->DriverStart = imageBase;
        driverObject->DriverSize = ntHeaders->OptionalHeader.SizeOfImage;
        if (!(ntHeaders->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_WDM_DRIVER)) {
            driverObject->Flags |= DRVO_LEGACY_DRIVER;
        }
    } else {
        ntHeaders = NULL;
        driverObject->Flags |= DRVO_LEGACY_DRIVER;
    }

    //
    // Save the name of the driver so that it can be easily located by functions
    // such as error logging.
    //

    buffer = ExAllocatePool( PagedPool, DriverName->MaximumLength + 2 );

    if (buffer) {
        driverObject->DriverName.Buffer = buffer;
        driverObject->DriverName.MaximumLength = DriverName->MaximumLength;
        driverObject->DriverName.Length = DriverName->Length;

        RtlCopyMemory( driverObject->DriverName.Buffer,
                       DriverName->Buffer,
                       DriverName->MaximumLength );
        buffer[DriverName->Length >> 1] = (WCHAR) '\0';
    }

    //
    // Save the name of the service key so that it can be easily located by PnP
    // mamager.
    //

    driverExtension = driverObject->DriverExtension;
    if (RegistryPath && RegistryPath->Length != 0) {
        pserviceName = RegistryPath->Buffer + RegistryPath->Length / sizeof (WCHAR) - 1;
        if (*pserviceName == OBJ_NAME_PATH_SEPARATOR) {
            pserviceName--;
        }
        serviceNameLength = 0;
        while (pserviceName != RegistryPath->Buffer) {
            if (*pserviceName == OBJ_NAME_PATH_SEPARATOR) {
                pserviceName++;
                break;
            } else {
                serviceNameLength += sizeof(WCHAR);
                pserviceName--;
            }
        }
        if (pserviceName == RegistryPath->Buffer) {
            serviceNameLength += sizeof(WCHAR);
        }
        buffer = ExAllocatePool( NonPagedPool, serviceNameLength + sizeof(UNICODE_NULL) );

        if (buffer) {
            driverExtension->ServiceKeyName.Buffer = buffer;
            driverExtension->ServiceKeyName.MaximumLength = serviceNameLength + sizeof(UNICODE_NULL);
            driverExtension->ServiceKeyName.Length = serviceNameLength;

            RtlCopyMemory( driverExtension->ServiceKeyName.Buffer,
                           pserviceName,
                           serviceNameLength );
            buffer[driverExtension->ServiceKeyName.Length >> 1] = UNICODE_NULL;
        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
            driverExtension->ServiceKeyName.Buffer = NULL;
            driverExtension->ServiceKeyName.Length = 0;
            goto exit;
        }

        //
        // Prepare driver initialization
        //

        status = IopOpenRegistryKeyEx( &serviceHandle,
                                       NULL,
                                       RegistryPath,
                                       KEY_ALL_ACCESS
                                       );
        if (NT_SUCCESS(status)) {
            status = IopPrepareDriverLoading(&driverExtension->ServiceKeyName,
                                             serviceHandle,
                                             ntHeaders);
            NtClose(serviceHandle);
            if (!NT_SUCCESS(status)) {
                goto exit;
            }
        } else {
            goto exit;
        }
    } else {
        driverExtension->ServiceKeyName.Buffer = NULL;
        driverExtension->ServiceKeyName.MaximumLength = 0;
        driverExtension->ServiceKeyName.Length = 0;
    }

    //
    // Load the Registry information in the appropriate fields of the device
    // object.
    //

    driverObject->HardwareDatabase = &CmRegistryMachineHardwareDescriptionSystemName;

#if DBG
    KeQuerySystemTime (&stime);
#endif

    //
    // Now invoke the driver's initialization routine to initialize itself.
    //

    PERFINFO_DRIVER_INIT( driverObject);

    status = driverObject->DriverInit( driverObject, RegistryPath );

    PERFINFO_DRIVER_INIT_COMPLETE( driverObject);

#if DBG

    //
    // If DriverInit took longer than 5 seconds or the driver did not load,
    // print a message.
    //

    KeQuerySystemTime (&etime);
    dtime  = (ULONG) ((etime.QuadPart - stime.QuadPart) / 1000000);

    if (dtime > 50  ||  !NT_SUCCESS( status )) {
        if (dtime < 10) {
            DbgPrint( "IOINIT: Built-in driver %wZ failed to initialize - %lX\n",
                DriverName, status );

        } else {
            DbgPrint( "IOINIT: Built-in driver %wZ took %d.%ds to ",
                DriverName, dtime/10, dtime%10 );

            if (NT_SUCCESS( status )) {
                DbgPrint ("initialize\n");
            } else {
                DbgPrint ("fail initialization - %lX\n", status);
            }
        }
    }
#endif
exit:
    NtClose( handle );

    //
    // If we load the driver because we think it is a legacy driver and
    // it does not create any device object in its DriverEntry.  We will
    // unload this driver.
    //

    if (NT_SUCCESS(status) && !IopIsLegacyDriver(driverObject)) {
        if (driverObject->DeviceObject == NULL     &&
            driverExtension->ServiceKeyName.Buffer &&
            !IopIsAnyDeviceInstanceEnabled(&driverExtension->ServiceKeyName, NULL, FALSE)) {
            if (TextModeSetup && !(driverObject->Flags & DRVO_REINIT_REGISTERED)) {

                //
                // Clean up but leave driver object.  Because it may be needed later.
                // After boot driver phase completes, we will process all the driver objects
                // which still have no device to control.
                //

                IopDriverLoadingFailed(NULL, &driverObject->DriverExtension->ServiceKeyName);
            }
        } else {

            //
            // Start the devices controlled by the driver and enumerate them
            // At this point, we know there is at least one device controlled by the driver.
            //

            IopDeleteLegacyKey(driverObject);
        }
    }

    if (NT_SUCCESS( status )) {
        IopReadyDeviceObjects( driverObject );
        return driverObject;
    } else {
        if (status != STATUS_PLUGPLAY_NO_DEVICE) {

            //
            // if STATUS_PLUGPLAY_NO_DEVICE, the driver was disable by hardware profile.
            //

            IopDriverLoadingFailed(NULL, &driverObject->DriverExtension->ServiceKeyName);
            ObMakeTemporaryObject( driverObject );
            ObDereferenceObject ( driverObject );
        }
        return NULL;
    }
}



BOOLEAN
IopInitializeSystemDrivers(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to load and initialize all of the drivers that
    are supposed to be loaded during Phase 1 initialization of the I/O
    system.  This is accomplished by calling the Configuration Manager to
    get a NULL-terminated array of handles to the open keys for each driver
    that is to be loaded, and then loading and initializing the driver.

Arguments:

    None.

Return Value:

    The function value is a BOOLEAN indicating whether or not the drivers
    were successfully loaded and initialized.

--*/

{
    BOOLEAN  newDevice, moreProcessing;
    NTSTATUS status;
    PHANDLE driverList;
    PHANDLE savedList;
    HANDLE enumHandle;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    UNICODE_STRING groupName, enumName;
    PTREE_ENTRY treeEntry;
    UNICODE_STRING driverName;
    PDRIVER_OBJECT driverObject;
    START_CONTEXT startContext;

    //
    // Scan the device node tree to process any device which have been enumerated
    // but not been added/started.
    //

    startContext.LoadDriver = TRUE;
    startContext.AddContext.DriverStartType = SERVICE_DEMAND_START;

    IopProcessAddDevices(IopRootDeviceNode, NO_MORE_GROUP, SERVICE_DEMAND_START);

    //
    // Process the whole device tree to assign resources to those devices who
    // have been successfully added to their drivers.
    //

    newDevice = TRUE;
    startContext.AddContext.GroupsToStart = NO_MORE_GROUP;
    startContext.AddContext.GroupToStartNext = NO_MORE_GROUP;
    while (newDevice || moreProcessing) {
        startContext.NewDevice = FALSE;
        moreProcessing = IopProcessAssignResources(IopRootDeviceNode, FALSE, TRUE);

        //
        // Process the whole device tree to start those devices who have been allocated
        // resources and waiting to be started.
        // Note, the IopProcessStartDevices routine may enumerate new devices.
        //

        IopProcessStartDevices(IopRootDeviceNode, &startContext);
        newDevice = startContext.NewDevice;
    }

    //
    // Walk thru the service list to load the remaining system start drivers.
    // (Most likely these drivers are software drivers.)
    //

    //
    // Get the list of drivers that are to be loaded during this phase of
    // system initialization, and invoke each driver in turn.  Ensure that
    // the list really exists, otherwise get out now.
    //

    if (!(driverList = CmGetSystemDriverList())) {
        return TRUE;
    }

    //
    // Walk the entire list, loading each of the drivers if not already loaded,
    // until there are no more drivers in the list.
    //

    for (savedList = driverList; *driverList; driverList++) {

        //
        // Now check if the driver has been loaded already.
        // get the name of the driver object first ...
        //

        status = IopGetDriverNameFromKeyNode( *driverList,
                                              &driverName );
        if (NT_SUCCESS( status )) {

            driverObject = IopReferenceDriverObjectByName(&driverName);
            RtlFreeUnicodeString(&driverName);
            if (driverObject) {

                //
                // Driver was loaded already.  Dereference the driver object
                // and skip it.
                //

                ObDereferenceObject(driverObject);
                continue;
            }
        }

        //
        // Open registry ServiceKeyName\Enum branch to check if the driver was
        // loaded before but failed.
        //

        PiWstrToUnicodeString(&enumName, REGSTR_KEY_ENUM);
        status = IopOpenRegistryKeyEx( &enumHandle,
                                       *driverList,
                                       &enumName,
                                       KEY_READ
                                       );

        if (NT_SUCCESS( status )) {

            ULONG startFailed = 0;

            status = IopGetRegistryValue(enumHandle, L"INITSTARTFAILED", &keyValueInformation);

            if (NT_SUCCESS( status )) {
                if (keyValueInformation->DataLength) {
                    startFailed = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
                }
                ExFreePool( keyValueInformation );
            }
            ZwClose(enumHandle);
            if (startFailed != 0) {
                continue;
            }
        }

        //
        // The driver is not loaded yet.  Load it ...
        //

        status = IopGetRegistryValue( *driverList,
                                      L"Group",
                                      &keyValueInformation );
        if (NT_SUCCESS( status )) {
            if (keyValueInformation->DataLength) {
                groupName.Length = (USHORT) keyValueInformation->DataLength;
                groupName.MaximumLength = groupName.Length;
                groupName.Buffer = (PWSTR) ((PUCHAR) keyValueInformation + keyValueInformation->DataOffset);
                treeEntry = IopLookupGroupName( &groupName, TRUE );
            } else {
                treeEntry = (PTREE_ENTRY) NULL;
            }
            ExFreePool( keyValueInformation );
        } else {
            treeEntry = (PTREE_ENTRY) NULL;
        }

        if (IopCheckDependencies( *driverList )) {
            if (NT_SUCCESS( IopLoadDriver( *driverList, TRUE ) )) {
                if (treeEntry) {
                    treeEntry->DriversLoaded++;
                }
            }
        }

        //
        // The boot process takes a while loading drivers.   Indicate that
        // progress is being made.
        //

        InbvIndicateProgress();

    }

    //
    // Finally, free the pool that was allocated for the list and return
    // an indicator the load operation worked.
    //

    ExFreePool( (PVOID) savedList );

    //
    // Walk the device tree again to enumerate any devices reported after the system drivers
    // started.
    //

    startContext.LoadDriver = TRUE;
    startContext.AddContext.DriverStartType = SERVICE_DEMAND_START;

    IopProcessAddDevices(IopRootDeviceNode, NO_MORE_GROUP, SERVICE_DEMAND_START);

    //
    // Process the whole device tree to assign resources to those devices who
    // have been successfully added to their drivers.
    //

    newDevice = TRUE;
    startContext.AddContext.GroupsToStart = NO_MORE_GROUP;
    startContext.AddContext.GroupToStartNext = NO_MORE_GROUP;
    do {
        startContext.NewDevice = FALSE;
        moreProcessing = IopProcessAssignResources(IopRootDeviceNode, newDevice, TRUE);

        //
        // Process the whole device tree to start those devices who have been allocated
        // resources and waiting to be started.
        // Note, the IopProcessStartDevices routine may enumerate new devices.
        //

        IopProcessStartDevices(IopRootDeviceNode, &startContext);
        newDevice = startContext.NewDevice;
    } while (newDevice || moreProcessing) ;

    //
    // Mark pnp has completed the driver loading for both system and
    // autoload drivers.
    //

    PnPInitialized = TRUE;

    return TRUE;
}

PTREE_ENTRY
IopLookupGroupName(
    IN PUNICODE_STRING GroupName,
    IN BOOLEAN Insert
    )

/*++

Routine Description:

    This routine looks up a group entry in the group load tree and either
    returns a pointer to it, or optionally creates the entry and inserts
    it into the tree.

Arguments:

    GroupName - The name of the group to look up, or insert.

    Insert - Indicates whether or not an entry is to be created and inserted
        into the tree if the name does not already exist.

Return Value:

    The function value is a pointer to the entry for the specified group
    name, or NULL.

--*/

{
    PTREE_ENTRY treeEntry;
    PTREE_ENTRY previousEntry;

    //
    // Begin by determining whether or not there are any entries in the tree
    // whatsoever.  If not, and it is OK to insert, then insert this entry
    // into the tree.
    //

    if (!IopGroupListHead) {
        if (!Insert) {
            return (PTREE_ENTRY) NULL;
        } else {
            IopGroupListHead = IopCreateEntry( GroupName );
            return IopGroupListHead;
        }
    }

    //
    // The tree is not empty, so actually attempt to do a lookup.
    //

    treeEntry = IopGroupListHead;

    for (;;) {
        if (GroupName->Length < treeEntry->GroupName.Length) {
            if (treeEntry->Left) {
                treeEntry = treeEntry->Left;
            } else {
                if (!Insert) {
                    return (PTREE_ENTRY) NULL;
                } else {
                    treeEntry->Left = IopCreateEntry( GroupName );
                    return treeEntry->Left;
                }

            }
        } else if (GroupName->Length > treeEntry->GroupName.Length) {
            if (treeEntry->Right) {
                treeEntry = treeEntry->Right;
            } else {
                if (!Insert) {
                    return (PTREE_ENTRY) NULL;
                } else {
                    treeEntry->Right = IopCreateEntry( GroupName );
                    return treeEntry->Right;
                }
            }
        } else {
            if (!RtlEqualUnicodeString( GroupName, &treeEntry->GroupName, TRUE )) {
                previousEntry = treeEntry;
                while (treeEntry->Sibling) {
                    treeEntry = treeEntry->Sibling;
                    if (RtlEqualUnicodeString( GroupName, &treeEntry->GroupName, TRUE )) {
                        return treeEntry;
                    }
                    previousEntry = previousEntry->Sibling;
                }
                if (!Insert) {
                    return (PTREE_ENTRY) NULL;
                } else {
                    previousEntry->Sibling = IopCreateEntry( GroupName );
                    return previousEntry->Sibling;
                }
            } else {
                return treeEntry;
            }
        }
    }
}

BOOLEAN
IopMarkBootPartition(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This routine is invoked to locate and mark the boot partition device object
    as a boot device so that subsequent operations can fail more cleanly and
    with a better explanation of why the system failed to boot and run properly.

Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block created
        by the OS Loader during the boot process.  This structure contains
        the various system partition and boot device names and paths.

Return Value:

    The function value is TRUE if everything worked, otherwise FALSE.

Notes:

    If the boot partition device object cannot be found, then the system will
    bugcheck.

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    STRING deviceNameString;
    UCHAR deviceNameBuffer[256];
    UNICODE_STRING deviceNameUnicodeString;
    NTSTATUS status;
    HANDLE fileHandle;
    IO_STATUS_BLOCK ioStatus;
    PFILE_OBJECT fileObject;
    CHAR ArcNameFmt[12];

    ArcNameFmt[0] = '\\';
    ArcNameFmt[1] = 'A';
    ArcNameFmt[2] = 'r';
    ArcNameFmt[3] = 'c';
    ArcNameFmt[4] = 'N';
    ArcNameFmt[5] = 'a';
    ArcNameFmt[6] = 'm';
    ArcNameFmt[7] = 'e';
    ArcNameFmt[8] = '\\';
    ArcNameFmt[9] = '%';
    ArcNameFmt[10] = 's';
    ArcNameFmt[11] = '\0';
    //
    // Open the ARC boot device object. The boot device driver should have
    // created the object.
    //

    sprintf( deviceNameBuffer,
             ArcNameFmt,
             LoaderBlock->ArcBootDeviceName );

    RtlInitAnsiString( &deviceNameString, deviceNameBuffer );

    status = RtlAnsiStringToUnicodeString( &deviceNameUnicodeString,
                                           &deviceNameString,
                                           TRUE );

    if (!NT_SUCCESS( status )) {
        return FALSE;
    }

    InitializeObjectAttributes( &objectAttributes,
                                &deviceNameUnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    status = ZwOpenFile( &fileHandle,
                         FILE_READ_ATTRIBUTES,
                         &objectAttributes,
                         &ioStatus,
                         0,
                         FILE_NON_DIRECTORY_FILE );
    if (!NT_SUCCESS( status )) {
        KeBugCheckEx( INACCESSIBLE_BOOT_DEVICE,
                      (ULONG_PTR) &deviceNameUnicodeString,
                      status,
                      0,
                      0 );
    }

    //
    // Convert the file handle into a pointer to the device object itself.
    //

    status = ObReferenceObjectByHandle( fileHandle,
                                        0,
                                        IoFileObjectType,
                                        KernelMode,
                                        (PVOID *) &fileObject,
                                        NULL );
    if (!NT_SUCCESS( status )) {
        RtlFreeUnicodeString( &deviceNameUnicodeString );
        return FALSE;
    }

    //
    // Mark the device object represented by the file object.
    //

    fileObject->DeviceObject->Flags |= DO_SYSTEM_BOOT_PARTITION;


    //
    // Reference the device object and store the reference.
    //
    ObReferenceObject(fileObject->DeviceObject);

    IopErrorLogObject =  fileObject->DeviceObject;

    RtlFreeUnicodeString( &deviceNameUnicodeString );

    //
    // Finally, close the handle and dereference the file object.
    //

    NtClose( fileHandle );
    ObDereferenceObject( fileObject );

    return TRUE;
}

BOOLEAN
IopReassignSystemRoot(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    OUT PSTRING NtDeviceName
    )

/*++

Routine Description:

    This routine is invoked to reassign \SystemRoot from being an ARC path
    name to its NT path name equivalent.  This is done by looking up the
    ARC device name as a symbolic link and determining which NT device object
    is referred to by it.  The link is then replaced with the new name.

Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block created
        by the OS Loader during the boot process.  This structure contains
        the various system partition and boot device names and paths.

    NtDeviceName - Specifies a pointer to a STRING to receive the NT name of
        the device from which the system was booted.

Return Value:

    The function value is a BOOLEAN indicating whether or not the ARC name
    was resolved to an NT name.

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    NTSTATUS status;
    UCHAR deviceNameBuffer[256];
    WCHAR arcNameUnicodeBuffer[64];
    UCHAR arcNameStringBuffer[256];
    STRING deviceNameString;
    STRING arcNameString;
    STRING linkString;
    UNICODE_STRING linkUnicodeString;
    UNICODE_STRING deviceNameUnicodeString;
    UNICODE_STRING arcNameUnicodeString;
    HANDLE linkHandle;

#if DBG

    UCHAR debugBuffer[256];
    STRING debugString;
    UNICODE_STRING debugUnicodeString;

#endif
    CHAR ArcNameFmt[12];

    ArcNameFmt[0] = '\\';
    ArcNameFmt[1] = 'A';
    ArcNameFmt[2] = 'r';
    ArcNameFmt[3] = 'c';
    ArcNameFmt[4] = 'N';
    ArcNameFmt[5] = 'a';
    ArcNameFmt[6] = 'm';
    ArcNameFmt[7] = 'e';
    ArcNameFmt[8] = '\\';
    ArcNameFmt[9] = '%';
    ArcNameFmt[10] = 's';
    ArcNameFmt[11] = '\0';

    //
    // Open the ARC boot device symbolic link object. The boot device
    // driver should have created the object.
    //

    sprintf( deviceNameBuffer,
             ArcNameFmt,
             LoaderBlock->ArcBootDeviceName );

    RtlInitAnsiString( &deviceNameString, deviceNameBuffer );

    status = RtlAnsiStringToUnicodeString( &deviceNameUnicodeString,
                                           &deviceNameString,
                                           TRUE );

    if (!NT_SUCCESS( status )) {
        return FALSE;
    }

    InitializeObjectAttributes( &objectAttributes,
                                &deviceNameUnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    status = NtOpenSymbolicLinkObject( &linkHandle,
                                       SYMBOLIC_LINK_ALL_ACCESS,
                                       &objectAttributes );

    if (!NT_SUCCESS( status )) {

#if DBG

        sprintf( debugBuffer, "IOINIT: unable to resolve %s, Status == %X\n",
                 deviceNameBuffer,
                 status );

        RtlInitAnsiString( &debugString, debugBuffer );

        status = RtlAnsiStringToUnicodeString( &debugUnicodeString,
                                               &debugString,
                                               TRUE );

        if (NT_SUCCESS( status )) {
            ZwDisplayString( &debugUnicodeString );
            RtlFreeUnicodeString( &debugUnicodeString );
        }

#endif // DBG

        RtlFreeUnicodeString( &deviceNameUnicodeString );
        return FALSE;
    }

    //
    // Get handle to \SystemRoot symbolic link.
    //

    arcNameUnicodeString.Buffer = arcNameUnicodeBuffer;
    arcNameUnicodeString.Length = 0;
    arcNameUnicodeString.MaximumLength = sizeof( arcNameUnicodeBuffer );

    status = NtQuerySymbolicLinkObject( linkHandle,
                                        &arcNameUnicodeString,
                                        NULL );

    if (!NT_SUCCESS( status )) {
        return FALSE;
    }

    arcNameString.Buffer = arcNameStringBuffer;
    arcNameString.Length = 0;
    arcNameString.MaximumLength = sizeof( arcNameStringBuffer );

    status = RtlUnicodeStringToAnsiString( &arcNameString,
                                           &arcNameUnicodeString,
                                           FALSE );

    arcNameStringBuffer[arcNameString.Length] = '\0';

    NtClose( linkHandle );
    RtlFreeUnicodeString( &deviceNameUnicodeString );

    RtlInitAnsiString( &linkString, INIT_SYSTEMROOT_LINKNAME );

    status = RtlAnsiStringToUnicodeString( &linkUnicodeString,
                                           &linkString,
                                           TRUE);

    if (!NT_SUCCESS( status )) {
        return FALSE;
    }

    InitializeObjectAttributes( &objectAttributes,
                                &linkUnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    status = NtOpenSymbolicLinkObject( &linkHandle,
                                       SYMBOLIC_LINK_ALL_ACCESS,
                                       &objectAttributes );

    if (!NT_SUCCESS( status )) {
        return FALSE;
    }

    NtMakeTemporaryObject( linkHandle );
    NtClose( linkHandle );

    sprintf( deviceNameBuffer,
             "%Z%s",
             &arcNameString,
             LoaderBlock->NtBootPathName );

    //
    // Get NT device name for \SystemRoot assignment.
    //

    RtlCopyString( NtDeviceName, &arcNameString );

    deviceNameBuffer[strlen(deviceNameBuffer)-1] = '\0';

    RtlInitAnsiString(&deviceNameString, deviceNameBuffer);

    InitializeObjectAttributes( &objectAttributes,
                                &linkUnicodeString,
                                OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                                NULL,
                                NULL );

    status = RtlAnsiStringToUnicodeString( &arcNameUnicodeString,
                                           &deviceNameString,
                                           TRUE);

    if (!NT_SUCCESS( status )) {
        return FALSE;
    }

    status = NtCreateSymbolicLinkObject( &linkHandle,
                                         SYMBOLIC_LINK_ALL_ACCESS,
                                         &objectAttributes,
                                         &arcNameUnicodeString );

    RtlFreeUnicodeString( &arcNameUnicodeString );
    RtlFreeUnicodeString( &linkUnicodeString );
    NtClose( linkHandle );

#if DBG

    if (NT_SUCCESS( status )) {

        sprintf( debugBuffer,
                 "INIT: Reassigned %s => %s\n",
                 INIT_SYSTEMROOT_LINKNAME,
                 deviceNameBuffer );

    } else {

        sprintf( debugBuffer,
                 "INIT: unable to create %s => %s, Status == %X\n",
                 INIT_SYSTEMROOT_LINKNAME,
                 deviceNameBuffer,
                 status );
    }

    RtlInitAnsiString( &debugString, debugBuffer );

    status = RtlAnsiStringToUnicodeString( &debugUnicodeString,
                                           &debugString,
                                           TRUE );

    if (NT_SUCCESS( status )) {

        ZwDisplayString( &debugUnicodeString );
        RtlFreeUnicodeString( &debugUnicodeString );
    }

#endif // DBG

    return TRUE;
}

VOID
IopStoreSystemPartitionInformation(
    IN     PUNICODE_STRING NtSystemPartitionDeviceName,
    IN OUT PUNICODE_STRING OsLoaderPathName
    )

/*++

Routine Description:

    This routine writes two values to the registry (under HKLM\SYSTEM\Setup)--one
    containing the NT device name of the system partition and the other containing
    the path to the OS loader.  These values will later be migrated into a
    Win95-compatible registry location (NT path converted to DOS path), so that
    installation programs (including our own setup) have a rock-solid way of knowing
    what the system partition is, on both ARC and x86.

    ERRORS ENCOUNTERED IN THIS ROUTINE ARE NOT CONSIDERED FATAL.

Arguments:

    NtSystemPartitionDeviceName - supplies the NT device name of the system partition.
        This is the \Device\Harddisk<n>\Partition<m> name, which used to be the actual
        device name, but now is a symbolic link to a name of the form \Device\Volume<x>.
        We open up this symbolic link, and retrieve the name that it points to.  The
        target name is the one we store away in the registry.

    OsLoaderPathName - supplies the path (on the partition specified in the 1st parameter)
        where the OS loader is located.  Upon return, this path will have had its trailing
        backslash removed (if present, and path isn't root).

Return Value:

    None.

--*/

{
    NTSTATUS status;
    HANDLE linkHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE systemHandle, setupHandle;
    UNICODE_STRING nameString, volumeNameString;
    WCHAR voumeNameStringBuffer[256];
    //
    // Declare a unicode buffer big enough to contain the longest string we'll be using.
    // (ANSI string in 'sizeof()' below on purpose--we want the number of chars here.)
    //
    WCHAR nameBuffer[sizeof("SystemPartition")];

    //
    // Both UNICODE_STRING buffers should be NULL-terminated.
    //

    ASSERT( NtSystemPartitionDeviceName->MaximumLength >= NtSystemPartitionDeviceName->Length + sizeof(WCHAR) );
    ASSERT( NtSystemPartitionDeviceName->Buffer[NtSystemPartitionDeviceName->Length / sizeof(WCHAR)] == L'\0' );

    ASSERT( OsLoaderPathName->MaximumLength >= OsLoaderPathName->Length + sizeof(WCHAR) );
    ASSERT( OsLoaderPathName->Buffer[OsLoaderPathName->Length / sizeof(WCHAR)] == L'\0' );

    //
    // Open the NtSystemPartitionDeviceName symbolic link, and find out the volume device
    // it points to.
    //

    InitializeObjectAttributes(&objectAttributes,
                               NtSystemPartitionDeviceName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                              );

    status = NtOpenSymbolicLinkObject(&linkHandle,
                                      SYMBOLIC_LINK_QUERY,
                                      &objectAttributes
                                     );

    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint("IopStoreSystemPartitionInformation: couldn't open symbolic link %wZ - %x\n",
                 NtSystemPartitionDeviceName,
                 status
                );
#endif // DBG
        return;
    }

    volumeNameString.Buffer = voumeNameStringBuffer;
    volumeNameString.Length = 0;
    //
    // Leave room at the end of the buffer for a terminating null, in case we need to add one.
    //
    volumeNameString.MaximumLength = sizeof(voumeNameStringBuffer) - sizeof(WCHAR);

    status = NtQuerySymbolicLinkObject(linkHandle,
                                       &volumeNameString,
                                       NULL
                                      );

    //
    // We don't need the handle to the symbolic link any longer.
    //

    NtClose(linkHandle);

    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint("IopStoreSystemPartitionInformation: couldn't query symbolic link %wZ - %x\n",
                 NtSystemPartitionDeviceName,
                 status
                );
#endif // DBG
        return;
    }

    //
    // Make sure the volume name string is null-terminated.
    //

    volumeNameString.Buffer[volumeNameString.Length / sizeof(WCHAR)] = L'\0';

    //
    // Open HKLM\SYSTEM key.
    //

    status = IopOpenRegistryKeyEx( &systemHandle,
                                   NULL,
                                   &CmRegistryMachineSystemName,
                                   KEY_ALL_ACCESS
                                   );

    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint("IopStoreSystemPartitionInformation: couldn't open \\REGISTRY\\MACHINE\\SYSTEM - %x\n", status);
#endif // DBG
        return;
    }

    //
    // Now open/create the setup subkey.
    //

    ASSERT( sizeof(L"Setup") <= sizeof(nameBuffer) );

    nameBuffer[0] = L'S';
    nameBuffer[1] = L'e';
    nameBuffer[2] = L't';
    nameBuffer[3] = L'u';
    nameBuffer[4] = L'p';
    nameBuffer[5] = L'\0';

    nameString.MaximumLength = sizeof(L"Setup");
    nameString.Length        = sizeof(L"Setup") - sizeof(WCHAR);
    nameString.Buffer        = nameBuffer;

    status = IopCreateRegistryKeyEx( &setupHandle,
                                     systemHandle,
                                     &nameString,
                                     KEY_ALL_ACCESS,
                                     REG_OPTION_NON_VOLATILE,
                                     NULL
                                     );

    NtClose(systemHandle);  // Don't need the handle to the HKLM\System key anymore.

    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint("IopStoreSystemPartitionInformation: couldn't open Setup subkey - %x\n", status);
#endif // DBG
        return;
    }

    ASSERT( sizeof(L"SystemPartition") <= sizeof(nameBuffer) );

    nameBuffer[0]  = L'S';
    nameBuffer[1]  = L'y';
    nameBuffer[2]  = L's';
    nameBuffer[3]  = L't';
    nameBuffer[4]  = L'e';
    nameBuffer[5]  = L'm';
    nameBuffer[6]  = L'P';
    nameBuffer[7]  = L'a';
    nameBuffer[8]  = L'r';
    nameBuffer[9]  = L't';
    nameBuffer[10] = L'i';
    nameBuffer[11] = L't';
    nameBuffer[12] = L'i';
    nameBuffer[13] = L'o';
    nameBuffer[14] = L'n';
    nameBuffer[15] = L'\0';

    nameString.MaximumLength = sizeof(L"SystemPartition");
    nameString.Length        = sizeof(L"SystemPartition") - sizeof(WCHAR);

    
  
    status = NtSetValueKey(setupHandle,
                            &nameString,
                            TITLE_INDEX_VALUE,
                            REG_SZ,
                            volumeNameString.Buffer,
                            volumeNameString.Length + sizeof(WCHAR)
                           );
    

#if DBG
    if (!NT_SUCCESS(status)) {
        DbgPrint("IopStoreSystemPartitionInformation: couldn't write SystemPartition value - %x\n", status);
    }
#endif // DBG

    ASSERT( sizeof(L"OsLoaderPath") <= sizeof(nameBuffer) );

    nameBuffer[0]  = L'O';
    nameBuffer[1]  = L's';
    nameBuffer[2]  = L'L';
    nameBuffer[3]  = L'o';
    nameBuffer[4]  = L'a';
    nameBuffer[5]  = L'd';
    nameBuffer[6]  = L'e';
    nameBuffer[7]  = L'r';
    nameBuffer[8]  = L'P';
    nameBuffer[9]  = L'a';
    nameBuffer[10] = L't';
    nameBuffer[11] = L'h';
    nameBuffer[12] = L'\0';

    nameString.MaximumLength = sizeof(L"OsLoaderPath");
    nameString.Length        = sizeof(L"OsLoaderPath") - sizeof(WCHAR);

    //
    // Strip off the trailing backslash from the path (unless, of course, the path is a
    // single backslash).
    //

    if ((OsLoaderPathName->Length > sizeof(WCHAR)) &&
        (*(PWCHAR)((PCHAR)OsLoaderPathName->Buffer + OsLoaderPathName->Length - sizeof(WCHAR)) == L'\\')) {

        OsLoaderPathName->Length -= sizeof(WCHAR);
        *(PWCHAR)((PCHAR)OsLoaderPathName->Buffer + OsLoaderPathName->Length) = L'\0';
    }

    status = NtSetValueKey(setupHandle,
                           &nameString,
                           TITLE_INDEX_VALUE,
                           REG_SZ,
                           OsLoaderPathName->Buffer,
                           OsLoaderPathName->Length + sizeof(WCHAR)
                           );
#if DBG
    if (!NT_SUCCESS(status)) {
        DbgPrint("IopStoreSystemPartitionInformation: couldn't write OsLoaderPath value - %x\n", status);
    }
#endif // DBG

    NtClose(setupHandle);
}

USHORT
IopGetDriverTagPriority (
    IN HANDLE ServiceHandle
    )

/*++

Routine Description:

    This routine reads the Tag value of a driver and determine the tag's priority
    among its driver group.

Arguments:

    ServiceHandle - specifies the handle of the driver's service key.

Return Value:

    USHORT for priority.

--*/

{
    NTSTATUS status;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation1;
    UNICODE_STRING groupName;
    HANDLE handle;
    USHORT index = (USHORT) -1;
    PULONG groupOrder;
    ULONG count, tag;

    //
    // Open System\CurrentControlSet\Control\GroupOrderList
    //

    PiWstrToUnicodeString(&groupName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\GroupOrderList");
    status = IopOpenRegistryKeyEx( &handle,
                                   NULL,
                                   &groupName,
                                   KEY_READ
                                   );

    if (!NT_SUCCESS( status )) {
        return index;
    }

    //
    // Read service key's Group value
    //

    status = IopGetRegistryValue (ServiceHandle,
                                  L"Group",
                                  &keyValueInformation);
    if (NT_SUCCESS(status)) {

        //
        // Try to read what caller wants.
        //

        if ((keyValueInformation->Type == REG_SZ) &&
            (keyValueInformation->DataLength != 0)) {
            IopRegistryDataToUnicodeString(&groupName,
                                           (PWSTR)KEY_VALUE_DATA(keyValueInformation),
                                           keyValueInformation->DataLength
                                           );
        }
    } else {

        //
        // If we failed to read the Group value, or no Group value...
        //

        NtClose(handle);
        return index;
    }

    //
    // Read service key's Tag value
    //

    status = IopGetRegistryValue (ServiceHandle,
                                  L"Tag",
                                  &keyValueInformation1);
    if (NT_SUCCESS(status)) {

        //
        // Try to read what caller wants.
        //

        if ((keyValueInformation1->Type == REG_DWORD) &&
            (keyValueInformation1->DataLength != 0)) {
            tag = *(PULONG)KEY_VALUE_DATA(keyValueInformation1);
        }
        ExFreePool(keyValueInformation1);
    } else {

        //
        // If we failed to read the Group value, or no Group value...
        //

        ExFreePool(keyValueInformation);
        NtClose(handle);
        return index;
    }

    //
    // Read group order list value for the driver's Group
    //

    status = IopGetRegistryValue (handle,
                                  groupName.Buffer,
                                  &keyValueInformation1);
    ExFreePool(keyValueInformation);
    NtClose(handle);
    if (NT_SUCCESS(status)) {

        //
        // Try to read what caller wants.
        //

        if ((keyValueInformation1->Type == REG_BINARY) &&
            (keyValueInformation1->DataLength != 0)) {
            groupOrder = (PULONG)KEY_VALUE_DATA(keyValueInformation1);
            count = *groupOrder;
            ASSERT((count + 1) * sizeof(ULONG) <= keyValueInformation1->DataLength);
            groupOrder++;
            for (index = 1; index <= count; index++) {
                if (tag == *groupOrder) {
                    break;
                } else {
                    groupOrder++;
                }
            }
        }
        ExFreePool(keyValueInformation1);
    } else {

        //
        // If we failed to read the Group value, or no Group value...
        //

        return index;
    }
    return index;
}

VOID
IopInsertDriverList (
    IN PLIST_ENTRY ListHead,
    IN PDRIVER_INFORMATION DriverInfo
    )

/*++

Routine Description:

    This routine reads the Tag value of a driver and determine the tag's priority
    among its driver group.

Arguments:

    ServiceHandle - specifies the handle of the driver's service key.

Return Value:

    USHORT for priority.

--*/

{
    PLIST_ENTRY nextEntry;
    PDRIVER_INFORMATION info;

    nextEntry = ListHead->Flink;
    while (nextEntry != ListHead) {
        info = CONTAINING_RECORD(nextEntry, DRIVER_INFORMATION, Link);

        //
        // Scan the driver info list to find the driver whose priority is
        // lower than current driver's.
        // (Lower TagPosition value means higher Priority)
        //

        if (info->TagPosition > DriverInfo->TagPosition) {
            break;
        }
        nextEntry = nextEntry->Flink;
    }

    //
    // Insert the Driver info to the front of the nextEntry
    //

    nextEntry = nextEntry->Blink;
    InsertHeadList(nextEntry, &DriverInfo->Link);
}

VOID
IopNotifySetupDevices (
    PDEVICE_NODE DeviceNode
    )

/*++

Routine Description:

    This routine notifies setupdd.sys for all the enumerated devices whose
    service have not been setup.

    This routine only gets executed on textmode setup phase.

Parameters:

    DeviceNode - specifies the root of the subtree to be processed.

Return Value:

    None.

--*/

{
    PDEVICE_NODE deviceNode = DeviceNode->Child;
    PDEVICE_OBJECT deviceObject;
    HANDLE handle;
    UNICODE_STRING unicodeString;
    NTSTATUS status;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;

    while (deviceNode) {
        IopNotifySetupDevices(deviceNode);
        if (deviceNode->ServiceName.Length == 0) {

            //
            // We only notify setupdd the device nodes which do not have service setup yet.
            // It is impossible that at this point, a device has a service setup and
            // setupdd has to change it.
            //

            deviceObject = deviceNode->PhysicalDeviceObject;
            status = IopDeviceObjectToDeviceInstance(deviceObject, &handle, KEY_ALL_ACCESS);
            if (NT_SUCCESS(status)) {

                //
                // Notify setup about the device.
                //

                IopNotifySetupDeviceArrival(deviceObject, handle, TRUE);

                //
                // Finally register the device
                //

                status = PpDeviceRegistration(
                             &deviceNode->InstancePath,
                             TRUE,
                             &unicodeString       // registered ServiceName
                             );

                if (NT_SUCCESS(status)) {
                    deviceNode->ServiceName = unicodeString;
                    if (IopIsDevNodeProblem(deviceNode, CM_PROB_NOT_CONFIGURED)) {
                        IopClearDevNodeProblem(deviceNode);
                    }
                }
                ZwClose(handle);
            }
        }
        deviceNode = deviceNode->Sibling;
    }
}

NTSTATUS
IopLogErrorEvent(
    IN ULONG            SequenceNumber,
    IN ULONG            UniqueErrorValue,
    IN NTSTATUS         FinalStatus,
    IN NTSTATUS         SpecificIOStatus,
    IN ULONG            LengthOfInsert1,
    IN PWCHAR           Insert1,
    IN ULONG            LengthOfInsert2,
    IN PWCHAR           Insert2
    )

/*++

Routine Description:

    This routine allocates an error log entry, copies the supplied data
    to it, and requests that it be written to the error log file.

Arguments:
    SequenceNumber - A value that is unique to an IRP over the life of the irp in
    this driver. - 0 generally means an error not associated with an IRP

    UniqueErrorValue - A unique long word that identifies the particular
    call to this function.

    FinalStatus - The final status given to the irp that was associated
    with this error.  If this log entry is being made during one of
    the retries this value will be STATUS_SUCCESS.

    SpecificIOStatus - The IO status for a particular error.

    LengthOfInsert1 - The length in bytes (including the terminating NULL)
                      of the first insertion string.

    Insert1 - The first insertion string.

    LengthOfInsert2 - The length in bytes (including the terminating NULL)
                      of the second insertion string.  NOTE, there must
                      be a first insertion string for their to be
                      a second insertion string.

    Insert2 - The second insertion string.

Return Value:

    STATUS_SUCCESS - Success
    STATUS_INVALID_HANDLE - Uninitialized error log device object
    STATUS_NO_DATA_DETECTED - NULL Error log entry

--*/

{
    PIO_ERROR_LOG_PACKET errorLogEntry;
    PUCHAR ptrToFirstInsert;
    PUCHAR ptrToSecondInsert;

    if (!IopErrorLogObject) {
        return(STATUS_INVALID_HANDLE);
    }


    errorLogEntry = IoAllocateErrorLogEntry(
                        IopErrorLogObject,
                        (UCHAR)( sizeof(IO_ERROR_LOG_PACKET) +
                                LengthOfInsert1 +
                                LengthOfInsert2) );

   if ( errorLogEntry != NULL ) {

      errorLogEntry->ErrorCode = SpecificIOStatus;
      errorLogEntry->SequenceNumber = SequenceNumber;
      errorLogEntry->MajorFunctionCode = 0;
      errorLogEntry->RetryCount = 0;
      errorLogEntry->UniqueErrorValue = UniqueErrorValue;
      errorLogEntry->FinalStatus = FinalStatus;
      errorLogEntry->DumpDataSize = 0;

      ptrToFirstInsert = (PUCHAR)&errorLogEntry->DumpData[0];

      ptrToSecondInsert = ptrToFirstInsert + LengthOfInsert1;

      if (LengthOfInsert1) {

         errorLogEntry->NumberOfStrings = 1;
         errorLogEntry->StringOffset = (USHORT)(ptrToFirstInsert -
                                                (PUCHAR)errorLogEntry);
         RtlCopyMemory(
                      ptrToFirstInsert,
                      Insert1,
                      LengthOfInsert1
                      );

         if (LengthOfInsert2) {

            errorLogEntry->NumberOfStrings = 2;
            RtlCopyMemory(
                         ptrToSecondInsert,
                         Insert2,
                         LengthOfInsert2
                         );

         } //LenghtOfInsert2

      } // LenghtOfInsert1

      IoWriteErrorLogEntry(errorLogEntry);
      return(STATUS_SUCCESS);

   }  // errorLogEntry != NULL

    return(STATUS_NO_DATA_DETECTED);

} //IopLogErrorEvent

BOOLEAN
IopWaitForBootDevicesStarted (
    IN VOID
    )

/*++

Routine Description:

    This routine waits for enumeration lock to be released for ALL devices.

Arguments:

    None.

Return Value:

    BOOLEAN.

--*/

{
    PDEVICE_NODE deviceNode;
    NTSTATUS status;
    KIRQL oldIrql;

    //
    // Wait on IoInvalidateDeviceRelations event to make sure all the devcies are enumerated
    // before progressing to mark boot partitions.
    //

    status = KeWaitForSingleObject( &PiEnumerationLock,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    NULL );
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    if (PnpAsyncOk) {

        //
        // Perform top-down check to make sure all the devices with Async start and Async Query
        // Device Relations are done.
        //

        deviceNode = IopRootDeviceNode;
        for (; ;) {

            ExAcquireSpinLock(&IopPnPSpinLock, &oldIrql);

            if (deviceNode->Flags & DNF_ASYNC_REQUEST_PENDING) {

                KeClearEvent(&PiEnumerationLock);
                ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);

                //
                // Wait on PiEnumerationLock to be signaled before proceeding.
                // At this point if a device node is marked ASYNC request pending,  this
                // must be an ASYNC start or enumeration which will queue an enumeration
                // request and once the enumeration completes, the PiEnumerationLock
                // will be signaled.
                //

                status = KeWaitForSingleObject( &PiEnumerationLock,
                                                Executive,
                                                KernelMode,
                                                FALSE,
                                                NULL );
                if (!NT_SUCCESS(status)) {
                    return FALSE;
                }
                continue;   // Make sure this device is done.
            } else {
                ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);
            }

            if (deviceNode->Child) {
                deviceNode = deviceNode->Child;
                continue;
            }
            if (deviceNode->Sibling) {
                deviceNode = deviceNode->Sibling;
                continue;
            }

            for (; ;) {
                deviceNode = deviceNode->Parent;

                //
                // If that was the last node to check, then exit loop
                //

                if (deviceNode == IopRootDeviceNode) {
                    goto exit;
                }
                if (deviceNode->Sibling) {
                    deviceNode = deviceNode->Sibling;
                    break;
                }
            }
        }
    exit:
        ;
    }
    return TRUE;
}

BOOLEAN
IopWaitForBootDevicesDeleted (
    IN VOID
    )

/*++

Routine Description:

    This routine waits for IoRequestDeviceRemoval to be completed.

Arguments:

    None.

Return Value:

    BOOLEAN.

--*/

{
    NTSTATUS status;

    //
    // Wait on device removal event to make sure all the deleted devcies are processed
    // before moving on to process next boot driver.
    //

    status = KeWaitForSingleObject( &PiEventQueueEmpty,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    NULL );
    return NT_SUCCESS(status);
}

PDRIVER_OBJECT
IopLoadBootFilterDriver (
    IN PUNICODE_STRING DriverName,
    IN ULONG GroupIndex
    )

/*++

Routine Description:

    This initializes boot filter drivers.

Arguments:

    DriverName - specifies the name of the driver to be initialized.

    GroupIndex - specifies the Driver's group index (could be anything)

Return Value:

    PDRIVER_OBJECT

--*/

{
    PDRIVER_OBJECT driverObject = NULL;
    PLIST_ENTRY nextEntry;
    PDRIVER_INFORMATION driverInfo;
    UNICODE_STRING completeName;
    PBOOT_DRIVER_LIST_ENTRY bootDriver;
    PLDR_DATA_TABLE_ENTRY driverEntry;
    HANDLE keyHandle;
    NTSTATUS status;

    if (IopGroupTable == NULL || GroupIndex >= IopGroupIndex) {

        //
        // If we have not reached the boot driver initialization phase or
        // the filter driver is not a boot driver.
        //

        return driverObject;
    }

    //
    // Go thru every driver that we initialized.  If it supports AddDevice entry and
    // did not create any device object after we start it.  We mark it as failure so
    // text mode setup knows this driver is not needed.
    //

    nextEntry = IopGroupTable[GroupIndex].Flink;
    while (nextEntry != &IopGroupTable[GroupIndex]) {

        driverInfo = CONTAINING_RECORD(nextEntry, DRIVER_INFORMATION, Link);
        if (driverInfo->Processed == FALSE) {

            keyHandle = driverInfo->ServiceHandle;

            status = IopGetDriverNameFromKeyNode( keyHandle,
                                                  &completeName );
            if (NT_SUCCESS( status )) {
                if (RtlEqualUnicodeString(DriverName,
                                          &completeName,
                                          TRUE)) {    // case-insensitive

                    bootDriver = driverInfo->DataTableEntry;
                    driverEntry = bootDriver->LdrEntry;

                    driverObject = IopInitializeBuiltinDriver(
                                       &completeName,
                                       &bootDriver->RegistryPath,
                                       (PDRIVER_INITIALIZE) driverEntry->EntryPoint,
                                       driverEntry,
                                       FALSE);
                    driverInfo->DriverObject = driverObject;
                    driverInfo->Processed = TRUE;
                    //
                    // Pnp might unload the driver before we get a chance to look at this. So take an extra
                    // reference.
                    //
                    if (driverObject) {
                        ObReferenceObject(driverObject);
                    }
                    ExFreePool(completeName.Buffer);
                    break;
                }
                ExFreePool(completeName.Buffer);
            }
        }

        nextEntry = nextEntry->Flink;
    }
    return driverObject;
}
#if 0

VOID
IopCheckClassFiltersForBootDevice (
    PDEVICE_NODE DeviceNode
    )

/*++

Routine Description:

    This routine notifies setupdd.sys for all the enumerated devices whose
    service have not been setup.

    This routine only gets executed on textmode setup phase.

Parameters:

    DeviceNode - specifies the root of the subtree to be processed.

Return Value:

    None.

--*/

{
    PDEVICE_OBJECT deviceObject;
    HANDLE handle;
    UNICODE_STRING unicodeString, unicodeName;
    NTSTATUS status;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;

    if (DeviceNode->ServiceName.Length != 0) {

        deviceObject = DeviceNode->PhysicalDeviceObject;
        status = IopDeviceObjectToDeviceInstance(deviceObject, &handle, KEY_ALL_ACCESS);
        if (NT_SUCCESS(status)) {

            //
            // Check if Class GUID is present.  If yes, add the device
            // instance to its Class Filters' service keys such that the class filter
            // drivers can stay around.
            //

            //
            // Get the class value to locate the class key for this devnode
            //

            status = IopGetRegistryValue(handle,
                                         REGSTR_VALUE_CLASSGUID,
                                         &keyValueInformation);

            if (NT_SUCCESS(status) &&
                ((keyValueInformation->Type == REG_SZ) && (keyValueInformation->DataLength != 0))) {

                HANDLE controlKey, classKey = NULL;
                UNICODE_STRING unicodeClassGuid;
                RTL_QUERY_REGISTRY_TABLE queryTable[2];

                IopRegistryDataToUnicodeString(
                    &unicodeClassGuid,
                    (PWSTR) KEY_VALUE_DATA(keyValueInformation),
                    keyValueInformation->DataLength);

                //
                // Open the class key
                //

                status = IopOpenRegistryKeyEx( &controlKey,
                                               NULL,
                                               &CmRegistryMachineSystemCurrentControlSetControlClass,
                                               KEY_READ
                                               );

                if (!NT_SUCCESS(status)) {
                    classKey = NULL;
                } else {
                    status = IopOpenRegistryKeyEx( &classKey,
                                                   controlKey,
                                                   &unicodeClassGuid,
                                                   KEY_READ
                                                   );

                    NtClose(controlKey);
                    if (!NT_SUCCESS(status)) {
                        classKey = NULL;
                    }
                }
                ExFreePool(keyValueInformation);

                if (classKey) {

                    RTL_QUERY_REGISTRY_TABLE queryTable;

                    RtlZeroMemory(&queryTable, sizeof(queryTable));

                    queryTable.QueryRoutine =
                        (PRTL_QUERY_REGISTRY_ROUTINE) IopAddDeviceInstanceForClassFilters;
                    queryTable.Name = REGSTR_VAL_LOWERFILTERS;
                    RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                           (PWSTR) classKey,
                                           &queryTable,
                                           &deviceNode->InstancePath,
                                           NULL);

                    queryTable.QueryRoutine =
                        (PRTL_QUERY_REGISTRY_ROUTINE) IopAddDeviceInstanceForClassFilters;
                    queryTable.Name = REGSTR_VAL_UPPERFILTERS;
                    RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                           (PWSTR) classKey,
                                           &queryTable,
                                           &deviceNode->InstancePath,
                                           NULL);
                    NtClose(classKey);
                }
            }

            ZwClose(handle);
        }
    }
}

NTSTATUS
IopAddDeviceInstanceForClassFilters(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PWCHAR ValueData,
    IN ULONG ValueLength,
    IN PUNICODE_STRING DeviceInstancePath,
    IN ULONG notUse
    )

/*++

Routine Description:

    This routine add a device instance path to the service's enum subkey.

Arguments:

    ValueName - the name of the value

    ValueType - the type of the value

    ValueData - the data in the value (unicode string data)

    ValueLength - the number of bytes in the value data

    Context - a structure which contains the device node, the context passed
              to IopCallDriverAddDevice and the driver lists for the device
              node.

    EntryContext - the index of the driver list the routine should append
                   nodes to.

Return Value:

    STATUS_SUCCESS if the driver was located and added to the list
    successfully or if there was a non-fatal error while handling the
    driver.

    an error value indicating why the driver could not be added to the list.

--*/

{
    UNICODE_STRING serviceName;
    UNICODE_STRING matchingDeviceInstance;
    UNICODE_STRING unicodeString;
    CHAR unicodeBuffer[20];
    HANDLE serviceEnumHandle;
    ULONG count;
    NTSTATUS status;

    RtlInitUnicodeString(&serviceName, ValueData);
    status = IopOpenServiceEnumKeys(&serviceName,
                                    KEY_ALL_ACCESS,
                                    NULL,
                                    &serviceEnumHandle,
                                    TRUE
                                   );
    if (!NT_SUCCESS(status)) {
        return STATUS_SUCCESS;
    }

    //
    // Now, search through the service's existing list of device instances, to see
    // if this instance has previously been registered.
    //

    status = PiFindDevInstMatch(serviceEnumHandle,
                                DeviceInstancePath,
                                &count,
                                &matchingDeviceInstance);

    if (!NT_SUCCESS(status)) {
        return STATUS_SUCCESS;
    }

    if (!matchingDeviceInstance.Buffer) {

        //
        // Create the value entry and update NextInstance= for the madeup key
        //

        PiUlongToUnicodeString(&unicodeString, unicodeBuffer, 20, count);
        status = ZwSetValueKey(
                    serviceEnumHandle,
                    &unicodeString,
                    TITLE_INDEX_VALUE,
                    REG_SZ,
                    DeviceInstancePath->Buffer,
                    DeviceInstancePath->Length
                    );
        count++;
        PiWstrToUnicodeString(&unicodeString, REGSTR_VALUE_COUNT);
        ZwSetValueKey(
                serviceEnumHandle,
                &unicodeString,
                TITLE_INDEX_VALUE,
                REG_DWORD,
                &count,
                sizeof(count)
                );
        PiWstrToUnicodeString(&unicodeString, REGSTR_VALUE_NEXT_INSTANCE);
        ZwSetValueKey(
                serviceEnumHandle,
                &unicodeString,
                TITLE_INDEX_VALUE,
                REG_DWORD,
                &count,
                sizeof(count)
                );
    } else {
        RtlFreeUnicodeString(&matchingDeviceInstance);
    }
    NtClose(serviceEnumHandle);
    return STATUS_SUCCESS;
}
#endif
