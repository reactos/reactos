/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dumpctl.c

Abstract:

    This module contains the code to dump memory to disk after a crash.

Author:

    Darryl E. Havens (darrylh) 17-dec-1993

Environment:

    Kernel mode

Revision History:


--*/

#include "iop.h"
#include "ntddft.h"
#include <inbv.h>
#include <windef.h>

//
// Processor specific macros.
//

#if defined (i386)

#define PROGRAM_COUNTER(_context)   ((_context)->Eip)
#define STACK_POINTER(_context)     ((_context)->Esp)
#define CURRENT_IMAGE_TYPE()        IMAGE_FILE_MACHINE_I386
#define PaeEnabled() X86PaeEnabled()

#elif defined (ALPHA)

#define PROGRAM_COUNTER(_context)   ((_context)->Fir)
#define STACK_POINTER(_context)     ((_context)->IntSp)
#define CURRENT_IMAGE_TYPE()        IMAGE_FILE_MACHINE_ALPHA
#define PaeEnabled() (FALSE)

#elif defined (_IA64_)

#define PROGRAM_COUNTER(_context)   ((_context)->StIIP)
#define STACK_POINTER(_context)     ((_context)->IntSp)
#define CURRENT_IMAGE_TYPE()        IMAGE_FILE_MACHINE_IA64
#define PaeEnabled() (FALSE)

#else

#error ("unknown processor type")

#endif

//
// min3(_a,_b,_c)
//
// Same as min() but takes 3 parameters.
//

#define min3(_a,_b,_c) ( min ( min ((_a), (_b)), min ((_a), (_c))) )


//
// Global variables
//

extern PVOID MmPfnDatabase;
extern PFN_NUMBER MmHighestPossiblePhysicalPage;

NTSTATUS IopFinalCrashDumpStatus         = -1;
ULONG    IopCrashDumpStateChange         =  0;
BOOLEAN  IopDumpFileContainsNewDump      = FALSE;

//
// Max dump transfer sizes
//

#define IO_DUMP_MAXIMUM_TRANSFER_SIZE   ( 1024 * 64 )
#define IO_DUMP_MINIMUM_TRANSFER_SIZE   ( 1024 * 32 )
#define IO_DUMP_MINIMUM_FILE_SIZE       ( PAGE_SIZE * 256 )
#define MAX_UNICODE_LENGTH              ( 512 )

#define DEFAULT_DRIVER_PATH             L"\\SystemRoot\\System32\\Drivers\\"
#define DEFAULT_DUMP_DRIVER             L"\\SystemRoot\\System32\\Drivers\\diskdump.sys"
#define SCSIPORT_DRIVER_NAME            L"scsiport.sys"
#define MAX_TRIAGE_STACK_SIZE           ( 16 * 1024 )
#define DEFAULT_TRIAGE_DUMP_FLAGS (0xFFFFFFFF)


//
// Function prototypes
//


NTSTATUS
IopWriteTriageDump(
    IN ULONG FieldsToWrite,
    IN PDUMP_DRIVER_WRITE WriteRoutine,
    IN OUT PLARGE_INTEGER Mcb,
    IN OUT PMDL Mdl,
    IN ULONG DiverTransferSize,
    IN PCONTEXT Context,
    IN LPBYTE Buffer,
    IN ULONG BufferSize,
    IN ULONG ServicePackBuild,
    IN ULONG TriageOptions
    );

NTSTATUS
IopWriteSummaryDump(
    IN PRTL_BITMAP PageMap,
    IN PDUMP_DRIVER_WRITE WriteRoutine,
    IN PANSI_STRING ProgressMessage,
    IN PUCHAR MessageBuffer,
    IN OUT PLARGE_INTEGER Mcb,
    IN ULONG DiverTransferSize
    );

NTSTATUS
IopWriteToDisk(
    IN PVOID Buffer,
    IN ULONG WriteLength,
    IN PDUMP_DRIVER_WRITE DriverWriteRoutine,
    IN OUT PLARGE_INTEGER * Mcb,
    IN OUT PMDL Mdl,
    IN ULONG DriverTransferSize
    );
    
VOID
IopMapPhysicalMemory(
    IN OUT PMDL Mdl,
    IN ULONG_PTR MemoryAddress,
    IN PPHYSICAL_MEMORY_RUN PhysicalMemoryRun,
    IN ULONG Length
    );

NTSTATUS
IopLoadDumpDriver (
    IN OUT PDUMP_STACK_CONTEXT  DumpStack,
    IN PWCHAR DriverNameString,
    IN PWCHAR NewBaseNameString
    );

NTSTATUS
IoSetCrashDumpState(
    IN SYSTEM_CRASH_STATE_INFORMATION *pDumpState
    );

PSUMMARY_DUMP_HEADER
IopInitializeSummaryDump(
    IN PDUMP_CONTROL_BLOCK pDcb
    );

NTSTATUS
IopWriteSummaryHeader(
    IN PSUMMARY_DUMP_HEADER    pSummaryHeader,
    IN PDUMP_DRIVER_WRITE      pfWrite,
    IN OUT PLARGE_INTEGER *    pMcbBuffer,
    IN OUT PMDL                pMdl,
    IN ULONG                   dwWriteSize,
    IN ULONG                   dwLength
    );

VOID
IopMapVirtualToPhysicalMdl(
    IN OUT PMDL pMdl,
    IN ULONG_PTR dwMemoryAddress,
    IN ULONG    dwLength
    );

ULONG
IopCreateSummaryDump (
    IN PSUMMARY_DUMP_HEADER pHeader
    );

VOID
IopDeleteNonExistentMemory(
    PSUMMARY_DUMP_HEADER        pHeader,
    PPHYSICAL_MEMORY_DESCRIPTOR MmPhysicalMemoryBlock
    );


NTSTATUS
IopGetDumpStack (
    IN PWCHAR                         ModulePrefix,
    OUT PDUMP_STACK_CONTEXT           *pDumpStack,
    IN PUNICODE_STRING                pUniDeviceName,
    IN PWSTR                          pDumpDriverName,
    IN DEVICE_USAGE_NOTIFICATION_TYPE UsageType,
    IN ULONG                          IgnoreDeviceUsageFailure
    );

BOOLEAN
IopInitializeDCB(
    );

LARGE_INTEGER
IopCalculateRequiredDumpSpace(
    IN ULONG            dwDmpFlags,
    IN ULONG            dwHeaderSize,
    IN PFN_NUMBER       dwMaxPages,
    IN PFN_NUMBER       dwMaxSummaryPages
    );

NTSTATUS
IopCompleteDumpInitialization(
    IN HANDLE     FileHandle
    );

#if DBG

VOID
IopDebugPrint(
    ULONG  DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );

#define IoDebugPrint(X) IopDebugPrint X

#else

#define IoDebugPrint(X)

#endif //DBG


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,IoGetDumpStack)
#pragma alloc_text(PAGE,IopLoadDumpDriver)
#pragma alloc_text(PAGE,IoFreeDumpStack)
#pragma alloc_text(PAGE,IoGetCrashDumpInformation)
#pragma alloc_text(PAGE,IoGetCrashDumpStateInformation)
#pragma alloc_text(PAGE,IoSetCrashDumpState)
#endif


#if defined (i386)

//
// Functions 
//


BOOL
X86PaeEnabled(
    )

/*++

Routine Description:

    Is PAE currently enabled?
    
Return Values:

    Return TRUE if PAE is enabled in the CR4 register, FALSE otherwise.

--*/
    
{
    ULONG Reg_Cr4;
    
    _asm {
        _emit 0Fh
        _emit 20h
        _emit 0E0h  ;; mov eax, cr4
        mov Reg_Cr4, eax
    }

    return (Reg_Cr4 & CR4_PAE ? TRUE : FALSE);
}

#endif

        

BOOLEAN
IopIsAddressRangeValid(
    IN PVOID VirtualAddress,
    IN SIZE_T Length
    )

/*++

Routine Description:

    Validate a range of addresses.

Arguments:

    Virtual Address - Beginning of of memory block to validate.

    Length - Length of memory block to validate.

Return Value:

    TRUE - Address range is valid.

    FALSE - Address range is not valid.

--*/

{
    UINT_PTR Va;
    ULONG Pages;

    Va = (UINT_PTR) PAGE_ALIGN (VirtualAddress);
    Pages = COMPUTE_PAGES_SPANNED (VirtualAddress, Length);

    while (Pages) {

        if (!MmIsAddressValid ( (LPVOID) Va)) {
            return FALSE;
        }

        Va += PAGE_SIZE;
        Pages--;
    }

    return TRUE;
}
    


NTSTATUS
IoGetDumpStack (
    IN PWCHAR                          ModulePrefix,
    OUT PDUMP_STACK_CONTEXT          * pDumpStack,
    IN  DEVICE_USAGE_NOTIFICATION_TYPE UsageType,
    IN  ULONG                          IgnoreDeviceUsageFailure
    )
/*++

Routine Description:

    This routine loads a dump stack instance and returns an allocated
    context structure to track the loaded dumps stack.

Arguments:

    ModePrefix      - The prefix to prepent to BaseName during the load
                      operation.  This allows loading the same drivers
                      multiple times with different virtual names and
                      linkages.

    pDumpStack      - The returned dump stack context structure

    UsageType       - The Device Notification Usage Type for this file, that
                      this routine will send as to the device object once the
                      file has been successfully created and initialized.

    IgnoreDeviceUsageFailure - If the Device Usage Notification Irp fails, allow
                      this to succeed anyway.

Return Value:

    Status

--*/
{

    PAGED_CODE();
    return IopGetDumpStack(ModulePrefix,
                           pDumpStack,
                           &IoArcBootDeviceName,
                           DEFAULT_DUMP_DRIVER,
                           UsageType,
                           IgnoreDeviceUsageFailure
                           );
}



NTSTATUS
IopGetDumpStack (
    IN PWCHAR                         ModulePrefix,
    OUT PDUMP_STACK_CONTEXT         * pDumpStack,
    IN PUNICODE_STRING                pUniDeviceName,
    IN PWCHAR                         pDumpDriverName,
    IN DEVICE_USAGE_NOTIFICATION_TYPE UsageType,
    IN ULONG                          IgnoreDeviceUsageFailure
    )
/*++

Routine Description:

    This routine loads a dump stack instance and returns an allocated
    context structure to track the loaded dumps stack.

Arguments:

    ModePrefix      - The prefix to prepent to BaseName during the load
                      operation.  This allows loading the same drivers
                      multiple times with different virtual names and
                      linkages.

    pDumpStack      - The returned dump stack context structure

    pDeviceName     - The name of the target dump device

    pDumpDriverName - The name of the target dump driver

    UsageType       - The Device Notification Usage Type for this file, that
                      this routine will send as to the device object once the
                      file has been successfully created and initialized.

    IgnoreDeviceUsageFailure - If the Device Usage Notification Irp fails, allow
                      this to succeed anyway.

Return Value:

    Status

--*/
{
    PDUMP_STACK_CONTEXT         DumpStack;
    PUCHAR                      Buffer;
    PUCHAR                      PartitionName;
    ANSI_STRING                 AnsiString;
    UNICODE_STRING              TempName;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    NTSTATUS                    Status;
    HANDLE                      DeviceHandle;
    SCSI_ADDRESS                ScsiAddress;
    BOOLEAN                     ScsiDump;
    PARTITION_INFORMATION       PartitionInfo;
    PFILE_OBJECT                FileObject;
    PDEVICE_OBJECT              DeviceObject;
    PINITIALIZATION_CONTEXT     DumpInit;
    PDUMP_POINTERS              DumpPointers;
    UNICODE_STRING              DriverName;
    PDRIVER_OBJECT              DriverObject;
    PIRP                        Irp;
    PIO_STACK_LOCATION          IrpSp;
    IO_STATUS_BLOCK             IoStatus;
    PWCHAR                      DumpName, NameOffset;
    KEVENT                      Event;
    PVOID                       p1;
    PHYSICAL_ADDRESS            pa;
    ULONG                       i;
    IO_STACK_LOCATION           irpSp;
    ULONG                       information;

    IoDebugPrint((2,"IopGetDumpStack: Prefix:%ws stk: %x device:%ws driver:%ws\n",
                ModulePrefix, pDumpStack, pUniDeviceName->Buffer,pDumpDriverName));

    ASSERT (DeviceUsageTypeUndefined != UsageType);

    DumpStack = ExAllocatePoolWithTag (
                    NonPagedPool,
                    sizeof (DUMP_STACK_CONTEXT) + sizeof (DUMP_POINTERS),
                    'pmuD'
                    );

    if (!DumpStack) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(DumpStack, sizeof(DUMP_STACK_CONTEXT)+sizeof(DUMP_POINTERS));
    DumpInit = &DumpStack->Init;
    DumpPointers = (PDUMP_POINTERS) (DumpStack + 1);
    DumpStack->DumpPointers = DumpPointers;
    InitializeListHead (&DumpStack->DriverList);
    DumpName = NULL;

    //
    // Allocate scratch buffer
    //

    Buffer = ExAllocatePoolWithTag (PagedPool, PAGE_SIZE, 'pmuD');
    if (!Buffer) {
        ExFreePool (DumpStack);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!KeGetBugMessageText(BUGCODE_PSS_CRASH_INIT, &DumpStack->InitMsg) ||
        !KeGetBugMessageText(BUGCODE_PSS_CRASH_PROGRESS, &DumpStack->ProgMsg) ||
        !KeGetBugMessageText(BUGCODE_PSS_CRASH_DONE, &DumpStack->DoneMsg)) {
            Status = STATUS_UNSUCCESSFUL;
            goto Done;
    }

    InitializeObjectAttributes(
        &ObjectAttributes,
        pUniDeviceName,
        0,
        NULL,
        NULL
        );

    Status = ZwOpenFile(
              &DeviceHandle,
              FILE_READ_DATA | SYNCHRONIZE,
              &ObjectAttributes,
              &IoStatus,
              FILE_SHARE_READ | FILE_SHARE_WRITE,
              FILE_NON_DIRECTORY_FILE
              );

    if (!NT_SUCCESS(Status)) {
        IoDebugPrint ((0,
                       "IODUMP: Could not open boot device partition, %s\n",
                       Buffer
                       ));
        goto Done;
    }

    //
    // Check to see whether or not the system was booted from a SCSI device.
    //

    Status = ZwDeviceIoControlFile (
                    DeviceHandle,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatus,
                    IOCTL_SCSI_GET_ADDRESS,
                    NULL,
                    0,
                    &ScsiAddress,
                    sizeof( SCSI_ADDRESS )
                    );

    if (Status == STATUS_PENDING) {
        ZwWaitForSingleObject (
            DeviceHandle,
            FALSE,
            NULL
            );

        Status = IoStatus.Status;
    }

    ScsiDump = (BOOLEAN) (NT_SUCCESS(Status));

    //
    // If SCSI then allocate storage to contain the target address information.
    //

    DumpInit->TargetAddress = NULL;

    if (ScsiDump) {

        DumpInit->TargetAddress = ExAllocatePoolWithTag (
                                    NonPagedPool,
                                    sizeof (SCSI_ADDRESS),
                                    'pmuD'
                                    );
        //
        // It is ok If the allocation fails. The scsi dump driver will scan
        // all devices if the targetaddress information does not exist
        //

        if (DumpInit->TargetAddress) {
            RtlCopyMemory(DumpInit->TargetAddress,&ScsiAddress,sizeof(SCSI_ADDRESS));
        }
    }

    //
    // Determine the disk signature for the device from which the system was
    // booted and get the partition offset.
    //

    Status = ZwDeviceIoControlFile(
                    DeviceHandle,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatus,
                    IOCTL_DISK_GET_PARTITION_INFO,
                    NULL,
                    0,
                    &PartitionInfo,
                    sizeof( PARTITION_INFORMATION )
                    );

    if (Status == STATUS_PENDING) {
        ZwWaitForSingleObject (
            DeviceHandle,
            FALSE,
            NULL
            );

        Status = IoStatus.Status;
    }

    IoDebugPrint((2,"Partition Type = %x\n",PartitionInfo.PartitionType));
    IoDebugPrint((2,"Boot Indicator = %x\n",PartitionInfo.BootIndicator));

    Status = ZwDeviceIoControlFile(
                    DeviceHandle,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatus,
                    IOCTL_DISK_GET_DRIVE_LAYOUT,
                    NULL,
                    0,
                    Buffer,
                    PAGE_SIZE
                    );

    if (Status == STATUS_PENDING) {
        ZwWaitForSingleObject (
            DeviceHandle,
            FALSE,
            NULL
            );

        Status = IoStatus.Status;
    }

    DumpInit->DiskSignature = ((PDRIVE_LAYOUT_INFORMATION) Buffer)->Signature;

    //
    // Get the adapter object and base mapping registers for the disk from
    // the disk driver.  These will be used to call the HAL once the system
    // system has crashed, since it is not possible at that point to recreate
    // them from scratch.
    //

    ObReferenceObjectByHandle (
            DeviceHandle,
            0,
            IoFileObjectType,
            KernelMode,
            (PVOID *) &FileObject,
            NULL
            );


    DeviceObject = IoGetRelatedDeviceObject (FileObject);

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    Irp = IoBuildDeviceIoControlRequest(
                IOCTL_SCSI_GET_DUMP_POINTERS,
                DeviceObject,
                NULL,
                0,
                DumpPointers,
                sizeof (DUMP_POINTERS),
                FALSE,
                &Event,
                &IoStatus
                );

    if (!Irp) {
        ObDereferenceObject (FileObject);
        ZwClose (DeviceHandle);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    IrpSp = IoGetNextIrpStackLocation (Irp);

    IrpSp->FileObject = FileObject;

    Status = IoCallDriver( DeviceObject, Irp );

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    if (!NT_SUCCESS(Status)  ||  IoStatus.Information < FIELD_OFFSET(DUMP_POINTERS, DeviceObject)) {

        IoDebugPrint ((0,
                       "IODUMP: Could not get dump pointers; error = %x, length %x\n",
                        Status,
                        IoStatus.Information
                        ));
        ObDereferenceObject (FileObject);
//      NtClose (DeviceHandle);
        ZwClose (DeviceHandle);
        goto Done;
    }
    DumpStack->PointersLength = (ULONG) IoStatus.Information;

    //
    // If the driver returned a pointer to a device object, that is the
    // object for the dump driver  (non-scsi case)
    //

    DeviceObject = (PDEVICE_OBJECT) DumpPointers->DeviceObject;
    if (DeviceObject) {
        DriverObject = DeviceObject->DriverObject;

        //
        // Loop through the name of the driver looking for the end of the name,
        // which is the name of the dump image.
        //

        DumpName = DriverObject->DriverName.Buffer;
        while ( NameOffset = wcsstr( DumpName, L"\\" )) {
            DumpName = ++NameOffset;
        }

        ScsiDump = FALSE;
    }

    //
    // Release the handle, but keep the reference to the file object as it
    // will be needed at free dump dump driver time
    //

    DumpStack->FileObject = FileObject;
    ZwClose (DeviceHandle);

    //
    // Fill in some DumpInit results
    //

    DumpInit->Length             = sizeof (INITIALIZATION_CONTEXT);
    DumpInit->StallRoutine       = &KeStallExecutionProcessor;
    DumpInit->AdapterObject      = DumpPointers->AdapterObject;
    DumpInit->MappedRegisterBase = DumpPointers->MappedRegisterBase;
    DumpInit->PortConfiguration  = DumpPointers->DumpData;

    DumpStack->ModulePrefix      = ModulePrefix;
    DumpStack->PartitionOffset   = PartitionInfo.StartingOffset;
    DumpStack->UsageType         = DeviceUsageTypeUndefined;

    //
    // The minimum common buffer size is IO_DUMP_COMMON_BUFFER_SIZE (compatability)
    // This is used by the dump driver for SRB extension, CachedExtension, and sense buffer
    //
    if (DumpPointers->CommonBufferSize < IO_DUMP_COMMON_BUFFER_SIZE) {
        DumpPointers->CommonBufferSize = IO_DUMP_COMMON_BUFFER_SIZE;
    }
    DumpInit->CommonBufferSize    = DumpPointers->CommonBufferSize;

    //
    // Allocate the required common buffers
    //

    if (DumpPointers->AllocateCommonBuffers) {
        pa.QuadPart = 0x1000000 - 1;
        for (i=0; i < 2; i++) {
            if (DumpInit->AdapterObject) {

#if !defined(NO_LEGACY_DRIVERS)
                p1 = HalAllocateCommonBuffer(
                    DumpInit->AdapterObject,
                    DumpPointers->CommonBufferSize,
                    &pa,
                    FALSE
                    );
                
#else
                p1 = (*((PDMA_ADAPTER)DumpInit->AdapterObject)->DmaOperations->
                      AllocateCommonBuffer)(
                          (PDMA_ADAPTER)DumpInit->AdapterObject,
                          DumpPointers->CommonBufferSize,
                          &pa,
                          FALSE
                          );
                
#endif // NO_LEGACY_DRIVERS
                        
            } else {
                p1 = MmAllocateContiguousMemory (
                        DumpPointers->CommonBufferSize,
                        pa
                        );

                if (!p1) {
                    p1 = MmAllocateNonCachedMemory (DumpPointers->CommonBufferSize);
                }
                pa = MmGetPhysicalAddress(p1);
            }

            if (!p1) {
                IoDebugPrint ((0, "IODUMP: Could not allocate common buffers for dump\n"));
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Done;
            }

            DumpInit->CommonBuffer[i] = p1;
            DumpInit->PhysicalAddress[i] = pa;
        }
    }

    //
    // Determine whether or not the system booted from SCSI.
    //

    if (ScsiDump) {

        //
        // Load the boot disk and port driver to be used by the various
        // miniports for writing memory to the disk.
        //

        Status = IopLoadDumpDriver (
                        DumpStack,
                        pDumpDriverName,
                        SCSIPORT_DRIVER_NAME
                        );

        if (!NT_SUCCESS(Status)) {

            IopLogErrorEvent(0,9,STATUS_SUCCESS,IO_DUMP_DRIVER_LOAD_FAILURE,0,NULL,0,NULL);
            goto Done;
        }

        //
        // The disk and port dump driver has been loaded.  Load the appropriate
        // miniport driver as well so that the boot device can be accessed.
        //

        DriverName.Length = 0;
        DriverName.Buffer = (PVOID) Buffer;
        DriverName.MaximumLength = PAGE_SIZE;


        //
        // The system was booted from SCSI. Get the name of the appropriate
        // miniport driver and load it.
        //

        sprintf(Buffer, "\\Device\\ScsiPort%d", ScsiAddress.PortNumber );
        RtlInitAnsiString( &AnsiString, Buffer );
        RtlAnsiStringToUnicodeString( &TempName, &AnsiString, TRUE );
        InitializeObjectAttributes(
                    &ObjectAttributes,
                    &TempName,
                    0,
                    NULL,
                    NULL
                    );

        Status = ZwOpenFile(
                    &DeviceHandle,
                    FILE_READ_ATTRIBUTES,
                    &ObjectAttributes,
                    &IoStatus,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_NON_DIRECTORY_FILE
                    );

        RtlFreeUnicodeString( &TempName );
        if (!NT_SUCCESS( Status )) {
            IoDebugPrint ((0,
                           "IODUMP: Could not open SCSI port %d, error = %x\n",
                           ScsiAddress.PortNumber,
                           Status
                           ));
            goto Done;
        }

        //
        // Convert the file handle into a pointer to the device object, and
        // get the name of the driver from its driver object.
        //

        ObReferenceObjectByHandle(
                    DeviceHandle,
                    0,
                    IoFileObjectType,
                    KernelMode,
                    (PVOID *) &FileObject,
                    NULL
                    );

        DriverObject = FileObject->DeviceObject->DriverObject;
        ObDereferenceObject( FileObject );
        ZwClose( DeviceHandle );
        //
        // Loop through the name of the driver looking for the end of the name,
        // which is the name of the miniport image.
        //

        DumpName = DriverObject->DriverName.Buffer;
        while ( NameOffset = wcsstr( DumpName, L"\\" )) {
            DumpName = ++NameOffset;
        }
    }

    //
    // Load the dump driver
    //

    if (!DumpName) {
        Status = STATUS_NOT_SUPPORTED;
        goto Done;
    }

    swprintf ((PWCHAR) Buffer, L"\\SystemRoot\\System32\\Drivers\\%s.sys", DumpName);
    Status = IopLoadDumpDriver (
                    DumpStack,
                    (PWCHAR) Buffer,
                    NULL
                    );
    if (!NT_SUCCESS(Status)) {

        IopLogErrorEvent(0,10,STATUS_SUCCESS,IO_DUMP_DRIVER_LOAD_FAILURE,0,NULL,0,NULL);
        goto Done;
    }

    //
    // Claim the file as part of specific device usage path.
    //

    FileObject = DumpStack->FileObject;
    DeviceObject = IoGetRelatedDeviceObject (FileObject);

    RtlZeroMemory (&irpSp, sizeof (IO_STACK_LOCATION));

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_DEVICE_USAGE_NOTIFICATION;
    irpSp.Parameters.UsageNotification.Type = UsageType;
    irpSp.Parameters.UsageNotification.InPath = TRUE;
    irpSp.FileObject = FileObject;

    Status = IopSynchronousCall (DeviceObject, &irpSp, (VOID **) &information);
    ASSERT (0 == information);

    if (!NT_SUCCESS(Status) && IgnoreDeviceUsageFailure) {
        IoDebugPrint ((0,
                       "IopGetDumpStack: DEVICE_USAGE_NOTIFICATION "
                       "Error ignored (%x)\n",
                       Status));
        Status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(Status)) {
        DumpStack->UsageType         = UsageType;
    }

Done:
    if (NT_SUCCESS(Status)) {
        *pDumpStack = DumpStack;
    } else {
        IoFreeDumpStack (DumpStack);
    }
    ExFreePool (Buffer);
    return Status;
}



NTSTATUS
IopLoadDumpDriver (
    IN OUT PDUMP_STACK_CONTEXT  DumpStack,
    IN PWCHAR DriverNameString,
    IN PWCHAR NewBaseNameString OPTIONAL
    )
/*++

Routine Description:

    Worker function for IoGetDumpStack to load a particular driver into
    the current DumpStack being created

Arguments:

    DumpStack           - Dump driver stack being built

    DriverNameString    - The string name of the driver to load

    NewBaseNameString   - The modified basename of the driver once loaded

Return Value:

    Status

--*/
{
    NTSTATUS                Status;
    PDUMP_STACK_IMAGE       DumpImage;
    PLDR_DATA_TABLE_ENTRY   ImageLdrInfo;
    UNICODE_STRING          DriverName;
    UNICODE_STRING          BaseName;
    UNICODE_STRING          Prefix;
    PUNICODE_STRING         LoadBaseName;

    //
    // Allocate space to track this dump driver
    //

    DumpImage = ExAllocatePoolWithTag (
                        NonPagedPool,
                        sizeof (DUMP_STACK_IMAGE),
                        'pmuD'
                        );

    if (!DumpImage) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Load the system image
    //

    RtlInitUnicodeString (&DriverName, DriverNameString);
    RtlInitUnicodeString (&Prefix, DumpStack->ModulePrefix);
    LoadBaseName = NULL;
    if (NewBaseNameString) {
        LoadBaseName = &BaseName;
        RtlInitUnicodeString (&BaseName, NewBaseNameString);
        BaseName.MaximumLength = Prefix.Length + BaseName.Length;
        BaseName.Buffer = ExAllocatePoolWithTag (
                            NonPagedPool,
                            BaseName.MaximumLength,
                            'pmuD'
                            );


        if (!BaseName.Buffer) {
            ExFreePool (DumpImage);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        BaseName.Length = 0;
        RtlAppendUnicodeStringToString (&BaseName, &Prefix);
        RtlAppendUnicodeToString (&BaseName, NewBaseNameString);
    }

    Status = MmLoadAndLockSystemImage(
                &DriverName,
                &Prefix,
                LoadBaseName,
                &DumpImage->Image,
                &DumpImage->ImageBase
                );

    if (NewBaseNameString) {
        ExFreePool (BaseName.Buffer);
    }

    if (!NT_SUCCESS (Status)) {
        IoDebugPrint ((0,
                       "IODUMP: Could not load %wZ; error = %x\n",
                       &DriverName,
                       Status));
        ExFreePool (DumpImage);
        return Status;
    }

    //
    // Put this driver on the list of drivers to be processed at crash time
    //

    DumpImage->SizeOfImage = DumpImage->Image->SizeOfImage;
    InsertTailList (&DumpStack->DriverList, &DumpImage->Link);
    return STATUS_SUCCESS;
}


ULONG
IopGetDumpControlBlockCheck (
    IN PDUMP_CONTROL_BLOCK  Dcb
    )
/*++

Routine Description:

    Return the current checksum total for the Dcb

Arguments:

    DumpStack           - Dump driver stack to checksum

Return Value:

    Checksum value

--*/
{
    ULONG                   Check;
    PLIST_ENTRY             Link;
    PDUMP_STACK_IMAGE       DumpImage;
    PMAPPED_ADDRESS         MappedAddress;
    PDUMP_STACK_CONTEXT     DumpStack;


    //
    // Check the DCB, memory descriptor array, and the FileDescriptorArray
    //

    Check = PoSimpleCheck(0, Dcb, sizeof(DUMP_CONTROL_BLOCK));
    Check = PoSimpleCheck(
                Check,
                Dcb->MemoryDescriptor,
                Dcb->MemoryDescriptorLength
                );

    Check = PoSimpleCheck(Check, Dcb->FileDescriptorArray, Dcb->FileDescriptorSize);

    DumpStack = Dcb->DumpStack;
    if (DumpStack) {

        //
        // Include the dump stack context structure, and dump driver images
        //

        Check = PoSimpleCheck(Check, DumpStack, sizeof(DUMP_STACK_CONTEXT));
        Check = PoSimpleCheck(Check, DumpStack->DumpPointers, DumpStack->PointersLength);

        for (Link = DumpStack->DriverList.Flink;
             Link != &DumpStack->DriverList;
             Link = Link->Flink) {

            DumpImage = CONTAINING_RECORD(Link, DUMP_STACK_IMAGE, Link);
            Check = PoSimpleCheck(Check, DumpImage, sizeof(DUMP_STACK_IMAGE));
            Check = PoSimpleCheck(Check, DumpImage->ImageBase, DumpImage->SizeOfImage);
        }

        //
        // Include the mapped addresses
        //
        // If this is non-null it is treated as a PMAPPED_ADDRESS * (see scsiport and atdisk)
        //
        if (DumpStack->Init.MappedRegisterBase != NULL) {
            MappedAddress = *(PMAPPED_ADDRESS *)DumpStack->Init.MappedRegisterBase;
        } else {
            MappedAddress = NULL;
        }

        while (MappedAddress) {
            Check = PoSimpleCheck (Check, MappedAddress, sizeof(MAPPED_ADDRESS));
            MappedAddress = MappedAddress->NextMappedAddress;
        }
    }

    return Check;
}


NTSTATUS
IoInitializeDumpStack (
    IN PDUMP_STACK_CONTEXT  DumpStack,
    IN PUCHAR               MessageBuffer OPTIONAL
    )
/*++

Routine Description:

    Initialize the dump driver stack referenced by DumpStack to perform IO.

Arguments:

    DumpStack   - Dump driver stack being initialized

Return Value:

    Status

--*/
{

    PINITIALIZATION_CONTEXT     DumpInit;
    PLIST_ENTRY                 Link;
    ULONG                       Check;
    NTSTATUS                    Status;
    PDRIVER_INITIALIZE          DriverInit;
    PDUMP_STACK_IMAGE           DumpImage;


    DumpInit = &DumpStack->Init;

    //
    // Verify checksum on DumpStack structure
    //

    // BUGBUG: later

    //
    // Initializes the dump drivers
    //

    for (Link = DumpStack->DriverList.Flink;
         Link != &DumpStack->DriverList;
         Link = Link->Flink) {

        DumpImage = CONTAINING_RECORD(Link, DUMP_STACK_IMAGE, Link);

        //
        // Call this driver's driver init.  Only the first driver gets the
        // dump initialization context
        //

        DriverInit = DumpImage->Image->EntryPoint;
        Status = DriverInit (NULL, (PUNICODE_STRING) DumpInit);
        DumpInit = NULL;

        if (!NT_SUCCESS(Status)) {
            IoDebugPrint ((0,
                           "IODUMP: Unable to initialize driver; error = %x\n",
                           Status
                           ));
            return Status;
        }
    }

    DumpInit = &DumpStack->Init;

    //
    // Display string we are starting
    //

    if (MessageBuffer) {
        InbvDisplayString (MessageBuffer);
    }

    //
    // Open the partition from which the system was booted.
    // This returns TRUE if the disk w/the appropriate signature was found,
    // otherwise a NULL, in which case there is no way to continue.
    //

    if (!DumpInit->OpenRoutine (DumpStack->PartitionOffset)) {
        IoDebugPrint (( 0, "IODUMP: Could not find/open partition offset\n" ));
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}


VOID
IoGetDumpHiberRanges (
    IN PVOID                    HiberContext,
    IN PDUMP_STACK_CONTEXT      DumpStack
    )
/*++

Routine Description:

    Adds the dump driver stack storage to the hibernate range list,
    to inform the hibernate procedure which pages need cloned,
    discarded or not checksumed as they are in use by the dump
    stack.

Arguments:

    HiberContext        - Pointer to the hiber context structure

    DumpStack           - Dump driver stack being initialized

Return Value:

    None

--*/
{
    PDUMP_POINTERS              DumpPointers;
    PDUMP_STACK_IMAGE           DumpImage;
    PLIST_ENTRY                 Link;

    DumpPointers = DumpStack->DumpPointers;

    //
    // Report the common buffer
    //

    if (DumpPointers->CommonBufferVa) {
        PoSetHiberRange (
            HiberContext,
            PO_MEM_CL_OR_NCHK,
            DumpPointers->CommonBufferVa,
            DumpPointers->CommonBufferSize,
            'fubc'
            );
    }

    //
    // Dump the entire image of the dump drivers
    //

    for (Link = DumpStack->DriverList.Flink;
         Link != &DumpStack->DriverList;
         Link = Link->Flink) {

        DumpImage = CONTAINING_RECORD(Link, DUMP_STACK_IMAGE, Link);

        PoSetHiberRange (
            HiberContext,
            PO_MEM_CL_OR_NCHK,
            DumpImage->ImageBase,
            DumpImage->SizeOfImage,
            'gmID'
            );
    }
}


VOID
IoFreeDumpStack (
    IN PDUMP_STACK_CONTEXT     DumpStack
    )
/*++

Routine Description:

    Free the dump driver stack referenced by DumpStack

Arguments:

    DumpStack           - Dump driver stack being initialized

Return Value:

    None

--*/
{
    PINITIALIZATION_CONTEXT     DumpInit;
    PDUMP_STACK_IMAGE           DumpImage;
    PDEVICE_OBJECT              DeviceObject;
    PIO_STACK_LOCATION          IrpSp;
    IO_STATUS_BLOCK             IoStatus;
    PIRP                        Irp;
    KEVENT                      Event;
    NTSTATUS                    Status;
    ULONG                       i;
    PFILE_OBJECT                FileObject;
    IO_STACK_LOCATION           irpSp;
    ULONG                       information;

    PAGED_CODE();
    DumpInit = &DumpStack->Init;

    //
    // Release the claim to this file as a specific device usage path.
    //

    FileObject = DumpStack->FileObject;
    if (FileObject) {
        DeviceObject = IoGetRelatedDeviceObject (FileObject);

        RtlZeroMemory (&irpSp, sizeof (IO_STACK_LOCATION));

        irpSp.MajorFunction = IRP_MJ_PNP;
        irpSp.MinorFunction = IRP_MN_DEVICE_USAGE_NOTIFICATION;
        irpSp.Parameters.UsageNotification.Type = DumpStack->UsageType;
        irpSp.Parameters.UsageNotification.InPath = FALSE;
        irpSp.FileObject = FileObject;

        if (DeviceUsageTypeUndefined != DumpStack->UsageType) {
            Status = IopSynchronousCall (DeviceObject, &irpSp, (VOID **) &information);
            ASSERT (0 == information);
        } else {
            Status = STATUS_SUCCESS;
        }
    }

    //
    // Free any common buffers which where allocated
    //

    for (i=0; i < 2; i++) {
        if (DumpInit->CommonBuffer[i]) {
            if (DumpInit->AdapterObject) {

#if !defined(NO_LEGACY_DRIVERS)
                HalFreeCommonBuffer (
                    DumpInit->AdapterObject,
                    ((PDUMP_POINTERS)DumpStack->DumpPointers)->CommonBufferSize,
                    DumpInit->PhysicalAddress[i],
                    DumpInit->CommonBuffer[i],
                    FALSE
                    );
#else
                (*((PDMA_ADAPTER)DumpInit->AdapterObject)->DmaOperations->
                 FreeCommonBuffer )(
                     (PDMA_ADAPTER)DumpInit->AdapterObject,
                     ((PDUMP_POINTERS)DumpStack->DumpPointers)->CommonBufferSize,
                     DumpInit->PhysicalAddress[i],
                     DumpInit->CommonBuffer[i],
                     FALSE
                     );

#endif // NO_LEGACY_DRIVERS

                
            } else {
                MmFreeContiguousMemory (DumpInit->CommonBuffer[i]);
            }
        }
        DumpInit->CommonBuffer[i] = NULL;
    }

    //
    // Unload the dump drivers
    //

    while (!IsListEmpty(&DumpStack->DriverList)) {
        DumpImage = CONTAINING_RECORD(DumpStack->DriverList.Blink, DUMP_STACK_IMAGE, Link);
        RemoveEntryList (&DumpImage->Link);
        MmUnloadSystemImage (DumpImage->Image);
        ExFreePool (DumpImage);
    }

    //
    // Inform the driver stack that the dump registartion is over
    //

    if (DumpStack->FileObject) {
        DeviceObject = IoGetRelatedDeviceObject ((PFILE_OBJECT) DumpStack->FileObject);

        KeInitializeEvent( &Event, NotificationEvent, FALSE );
        Irp = IoBuildDeviceIoControlRequest(
                    IOCTL_SCSI_FREE_DUMP_POINTERS,
                    DeviceObject,
                    DumpStack->DumpPointers,
                    sizeof (DUMP_POINTERS),
                    NULL,
                    0,
                    FALSE,
                    &Event,
                    &IoStatus
                    );

        IrpSp = IoGetNextIrpStackLocation (Irp);
        IrpSp->FileObject = DumpStack->FileObject;

        Status = IoCallDriver( DeviceObject, Irp );

        if (Status == STATUS_PENDING) {
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = IoStatus.Status;
        }
        ObDereferenceObject( DumpStack->FileObject );
    }
    //
    // Free the target address if it exists
    //
    if (DumpStack->Init.TargetAddress) {
        ExFreePool( DumpStack->Init.TargetAddress);
    }
    //
    // Free the dump stack context
    //

    ExFreePool (DumpStack);
}


NTSTATUS
IopInitializeDumpSpaceAndType(
    IN PDUMP_CONTROL_BLOCK dcb,
    IN OUT PULONG block,
    IN PSUMMARY_DUMP_HEADER pSummaryHeader
    )
{
    LARGE_INTEGER Space;
    
    Space.QuadPart = 0;

    if (dcb->Flags & DCB_TRIAGE_DUMP_ENABLED) {

        //
        // Fixed size dump for triage-dumps.
        //
        
        block [ DH_DUMP_TYPE ] = DUMP_TYPE_TRIAGE;
        Space.QuadPart = TRIAGE_DUMP_SIZE;


    } else if (dcb->Flags & DCB_SUMMARY_DUMP_ENABLED) {

        block [ DH_DUMP_TYPE ] = DUMP_TYPE_SUMMARY;

        Space = IopCalculateRequiredDumpSpace(
                                dcb->Flags,
                                dcb->HeaderSize,
                                dcb->MemoryDescriptor->NumberOfPages,
                                pSummaryHeader->Pages
                                );
    } else {

        if (dcb->Flags & DCB_DUMP_HEADER_ENABLED) {
            block [ DH_DUMP_TYPE ] = DUMP_TYPE_HEADER;
        }

        Space = IopCalculateRequiredDumpSpace(
                                dcb->Flags,
                                dcb->HeaderSize,
                                dcb->MemoryDescriptor->NumberOfPages,
                                dcb->MemoryDescriptor->NumberOfPages
                                );
    }

    //
    // If the calculated size is larger than the pagefile, truncate it to
    // the pagefile size.
    //

    if (Space.QuadPart > dcb->DumpFileSize.QuadPart) {
        Space.QuadPart = dcb->DumpFileSize.QuadPart;
    }
    
    block [ DH_REQUIRED_DUMP_SPACE ] = Space.LowPart;
    block [ DH_REQUIRED_DUMP_SPACE + 1 ] = Space.HighPart;

    IoDebugPrint((2,"IODUMP: dcb File Size %x Block Size %x\n",
                    block [DH_REQUIRED_DUMP_SPACE],
                    (ULONG)dcb->DumpFileSize.LowPart
                    ));

    return STATUS_SUCCESS;
}



BOOLEAN
IoWriteCrashDump(
    IN ULONG BugCheckCode,
    IN ULONG_PTR BugCheckParameter1,
    IN ULONG_PTR BugCheckParameter2,
    IN ULONG_PTR BugCheckParameter3,
    IN ULONG_PTR BugCheckParameter4,
    IN PVOID ContextSave
    )

/*++

Routine Description:

    This routine checks to see whether or not crash dumps are enabled and, if
    so, writes all of physical memory to the system disk's paging file.

Arguments:

    BugCheckCode/ParameterN - Code and parameters w/which BugCheck was called.

Return Value:

    None.

--*/

{
    PDUMP_CONTROL_BLOCK dcb;
    PDUMP_STACK_CONTEXT dumpStack;
    PDUMP_DRIVER_WRITE write;
    PDUMP_DRIVER_FINISH finishUp;
    PDUMP_HEADER header;
    EXCEPTION_RECORD exception;
    PCONTEXT context = ContextSave;
    PULONG block;
    LARGE_INTEGER diskByteOffset;
    PPFN_NUMBER page;
    PFN_NUMBER localMdl[(sizeof( MDL )/sizeof(PFN_NUMBER)) + 17];
    PMDL mdl;
    PLARGE_INTEGER mcb;
    ULONG_PTR memoryAddress;
    ULONG byteOffset;
    ULONG byteCount;
    ULONG bytesRemaining;
    NTSTATUS status;
    UCHAR messageBuffer[128];
    PFN_NUMBER ActualPages;
    PSUMMARY_DUMP_HEADER pSummaryHeader;
    ULONG dwTransferSize;
    LARGE_INTEGER requiredDumpSpace;
    ULONG_PTR DirBasePage;

    //
    // Begin by determining whether or not crash dumps are enabled.  If not,
    // check to see whether or not auto-rebooting is enabled.  If not, return
    // immediately since there is nothing to do.
    //

    dcb = IopDumpControlBlock;
    if (!dcb) {
        return FALSE;
    }

    if (dcb->Flags & DCB_DUMP_ENABLED || dcb->Flags & DCB_SUMMARY_ENABLED) {

        IopFinalCrashDumpStatus = STATUS_PENDING;

        //
        // A dump is to be written to the paging file.  Ensure that all of the
        // descriptor data for what needs to be done is valid, otherwise it
        // could be that part of the reason for the bugcheck is that this data
        // was corrupted.  Or, it could be that no paging file was found yet,
        // or any number of other situations.
        //

        if (IopGetDumpControlBlockCheck(dcb) != IopDumpControlBlockChecksum) {
            IoDebugPrint (( 0,
                           "CRASHDUMP: Disk dump routine returning due to DCB integrity error\n"
                           "           No dump will be created\n" ));
            IopFinalCrashDumpStatus = STATUS_UNSUCCESSFUL;
            return FALSE;
        }

        //
        // Message  that we are starting the crashdump
        //

        dumpStack = dcb->DumpStack;
        sprintf( messageBuffer, "%Z\n", &dumpStack->InitMsg );

        //
        // Initialize the dump stack
        //

        status = IoInitializeDumpStack (dumpStack, messageBuffer);
        if (!NT_SUCCESS( status )) {
            IopFinalCrashDumpStatus = STATUS_UNSUCCESSFUL;
            return FALSE;
        }

        //
        // Record the dump driver's entry points.
        //

        write = dumpStack->Init.WriteRoutine;
        finishUp = dumpStack->Init.FinishRoutine;


        dwTransferSize = dumpStack->Init.MaximumTransferSize;

        if ( ( !dwTransferSize ) || ( dwTransferSize > IO_DUMP_MAXIMUM_TRANSFER_SIZE ) ) {
            dwTransferSize = IO_DUMP_MINIMUM_TRANSFER_SIZE;
        }

        IoDebugPrint((2,"CRASHDUMP: Maximum Transfer Size = %x\n",dwTransferSize));


        //
        // The boot partition was found, so put together a dump file header
        // and write it to the disk.
        //

        block = dcb->HeaderPage;
        header = (PDUMP_HEADER) block;

        RtlFillMemoryUlong( header, PAGE_SIZE, 'EGAP' );
        header->ValidDump = 'PMUD';
        header->BugCheckCode = BugCheckCode;
        header->BugCheckParameter1 = BugCheckParameter1;
        header->BugCheckParameter2 = BugCheckParameter2;
        header->BugCheckParameter3 = BugCheckParameter3;
        header->BugCheckParameter4 = BugCheckParameter4;
#if defined (i386)
        //
        // Add the current page directory table page - don't use the directory
        // table base for the crashing process as we have switched cr3 on
        // stack overflow crashes, etc.
        //
    
        _asm {
            mov     eax, cr3
            mov     DirBasePage, eax
        }
        header->DirectoryTableBase = DirBasePage;
#else
        header->DirectoryTableBase = KeGetCurrentThread()->ApcState.Process->DirectoryTableBase[0];
#endif
        header->PfnDataBase = MmPfnDatabase;
        header->PsLoadedModuleList = &PsLoadedModuleList;
        header->PsActiveProcessHead = &PsActiveProcessHead;
        header->NumberProcessors = dcb->NumberProcessors;
        header->MajorVersion = dcb->MajorVersion;
        header->MinorVersion = dcb->MinorVersion;
        header->KdDebuggerDataBlock = KdGetDataBlock();
        header->PaeEnabled = PaeEnabled ();

        header->MachineImageType = CURRENT_IMAGE_TYPE ();

        if (!(dcb->Flags & DCB_DUMP_ENABLED)) {
            dcb->MemoryDescriptor->NumberOfPages = 1;
        }

        strcpy( header->VersionUser, dcb->VersionUser );

        RtlCopyMemory( &block[DH_PHYSICAL_MEMORY_BLOCK],
                       dcb->MemoryDescriptor,
                       sizeof( PHYSICAL_MEMORY_DESCRIPTOR ) +
                       ((dcb->MemoryDescriptor->NumberOfRuns - 1) *
                       sizeof( PHYSICAL_MEMORY_RUN )) );

        RtlCopyMemory( &block[DH_CONTEXT_RECORD],
                       context,
                       sizeof( CONTEXT ) );

        exception.ExceptionCode = STATUS_BREAKPOINT;
        exception.ExceptionRecord = (PEXCEPTION_RECORD) NULL;
        exception.NumberParameters = 0;
        exception.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
        exception.ExceptionAddress = (PVOID) PROGRAM_COUNTER (context);

        RtlCopyMemory( &block[DH_EXCEPTION_RECORD],
                       &exception,
                       sizeof( EXCEPTION_RECORD ) );

        //
        // Init dump type to FULL
        //
        
        block[DH_DUMP_TYPE] = DUMP_TYPE_FULL;

        //
        // Set the timestamp
        //
#ifdef _WIN64
        RtlCopyMemory( &block[DH_CRASH_DUMP_TIMESTAMP],
                       (PCHAR)( &SharedUserData->SystemLowTime),
                       sizeof( LARGE_INTEGER) );
#else
        RtlCopyMemory( &block[DH_CRASH_DUMP_TIMESTAMP],
                       (PCHAR)( &SharedUserData->SystemTime),
                       sizeof( LARGE_INTEGER) );
#endif

        //
        // Set the Required dump size in the dump header. In the case of
        // a summary dump the file allocation size can be significantly larger
        // then the amount of used space.
        //
        
        RtlZeroMemory( &block[DH_REQUIRED_DUMP_SPACE], sizeof(LARGE_INTEGER));

        if (dcb->Flags & DCB_DUMP_ENABLED) {
        
            //
            // If summary dump try to create the dump header
            //
            
            if ( (dcb->Flags & DCB_SUMMARY_DUMP_ENABLED) ) {

                //
                // Initialize the summary dump
                //

                pSummaryHeader = IopInitializeSummaryDump(dcb);

                if ( !pSummaryHeader ) {

                    //
                    // No summary dump header so return.
                    //

                    IoDebugPrint((1,"IoWriteCrashDump: Error Null summary dump header\n"));

                    IopFinalCrashDumpStatus = STATUS_UNSUCCESSFUL;

                    return FALSE;
                }
            }

            IopInitializeDumpSpaceAndType (dcb, block, pSummaryHeader);
        }

        //
        // All of the pieces of the header file have been generated.  Before
        // mapping or writing anything to the disk, the I- & D-stream caches
        // must be flushed so that page color coherency is kept.  Sweep both
        // caches now.
        //

        KeSweepCurrentDcache();
        KeSweepCurrentIcache();

        //
        // Create MDL for dump.
        //

        mdl = (PMDL) &localMdl[0];
        MmCreateMdl( mdl, NULL, PAGE_SIZE );
        mdl->MdlFlags |= MDL_PAGES_LOCKED;

        mcb = dcb->FileDescriptorArray;

        page = MmGetMdlPfnArray(mdl);
        *page = dcb->HeaderPfn;
        mdl->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;

        bytesRemaining = PAGE_SIZE;
        memoryAddress = (ULONG_PTR) dcb->HeaderPage;

        //
        // All of the pieces of the header file have been generated.  Write
        // the header page to the paging file, using the appropriate drivers,
        // etc.
        //

        IoDebugPrint((2,"IoWriteCrashDump: Writing dump header to disk\n"));

        while (bytesRemaining) {

            if (mcb[0].QuadPart <= bytesRemaining) {
                byteCount = mcb[0].LowPart;
            } else {
                byteCount = bytesRemaining;
            }

            mdl->ByteCount = byteCount;
            mdl->ByteOffset = (ULONG)(memoryAddress & (PAGE_SIZE - 1));
            mdl->MappedSystemVa = (PVOID) memoryAddress;
            mdl->StartVa = PAGE_ALIGN ((PVOID)memoryAddress);

            //
            // Write to disk.
            //

            if (!NT_SUCCESS( write( &mcb[1], mdl ) )) {
                IopFinalCrashDumpStatus = STATUS_UNSUCCESSFUL;
                return FALSE;
            }

            //
            // Adjust bytes remaining.
            //

            bytesRemaining -= byteCount;
            memoryAddress += byteCount;
            mcb[0].QuadPart = mcb[0].QuadPart - byteCount;
            mcb[1].QuadPart = mcb[1].QuadPart + byteCount;

            if (!mcb[0].QuadPart) {
                mcb += 2;
            }
        }

        IoDebugPrint((2,"IoWriteCrashDump: Header Page written\n"));


        //
        // If only requesting a header dump, we are now done.
        //
        
        if (dcb->Flags & DCB_DUMP_HEADER_ENABLED) {
            IoDebugPrint((2,"IoWriteCrashDump: Only Dumping Dump Header\n"));
            goto FinishDump;
        }

        //
        // The header page has been written. If this is a triage-dump, write
        // the dump information and bail. Otherwise, fall through and do the
        // full or summary dump.
        //

        if (dcb->Flags & DCB_TRIAGE_DUMP_ENABLED) {
            status = IopWriteTriageDump (dcb->TriageDumpFlags,
                                       write,
                                       mcb,
                                       mdl,
                                       dwTransferSize,
                                       context,
                                       dcb->TriageDumpBuffer,
                                       dcb->TriageDumpBufferSize - PAGE_SIZE,
                                       dcb->BuildNumber,
                                       dcb->Flags
                                       );
                                       
            if (!NT_SUCCESS (status)) {
                IoDebugPrint((1, "IoWriteCrashDump: Failed to write triage-dump\n"));
                IopFinalCrashDumpStatus = STATUS_UNSUCCESSFUL;
                return FALSE;
            }

            IoDebugPrint((1, "IoWriteCrashDump: Successfully wrote triage-dump\n"));
            goto FinishDump;
        }

        //
        // The header page has been written to the paging file.  If a full dump
        // of all of physical memory is to be written, write it now.
        //

        if (dcb->Flags & DCB_DUMP_ENABLED) {

            PFN_NUMBER pagesDoneSoFar = 0;
            ULONG currentPercentage = 0;
            ULONG maximumPercentage = 0;


            //
            // Actual Pages is the number of pages to dump.
            //

            ActualPages = dcb->MemoryDescriptor->NumberOfPages;

            if (dcb->Flags & DCB_SUMMARY_DUMP_ENABLED) {

                PRTL_BITMAP PageMap;

                ASSERT ( pSummaryHeader != NULL );

                PageMap = (PRTL_BITMAP)(pSummaryHeader + 1);
                ASSERT (PageMap != NULL);

                //
                // At this point the dump header header has been sucessfully
                // written. Write the summary dump header.
                //
                
                status = IopWriteSummaryHeader(
                                     pSummaryHeader,
                                     write,
                                     &mcb,
                                     mdl,
                                     dwTransferSize,
                                     (dcb->HeaderSize - PAGE_SIZE)
                                     );
                                     
                if (!NT_SUCCESS (status)) {
                    IoDebugPrint((1,"IoWriteCrashDump: Error writing summary dump header\n"));
                    IopFinalCrashDumpStatus = status;
                    return FALSE;
                }

                IoDebugPrint((2,"IoWriteCrashDump: Sucessfully wrote summary dump header\n"));

                ActualPages = pSummaryHeader->Pages;

            }

            IoDebugPrint((2,"IoWriteCrashDump: Writing Memory Dump\n"));

            //
            // Set the virtual file offset and initialize loop variables and
            // constants.
            //

            memoryAddress = (ULONG_PTR)dcb->MemoryDescriptor->Run[0].BasePage * PAGE_SIZE;

            if ( dcb->Flags & DCB_SUMMARY_DUMP_ENABLED ) {

                PRTL_BITMAP BitMap;
                
                BitMap = (PRTL_BITMAP)(pSummaryHeader+1);

                status = IopWriteSummaryDump (
                                        BitMap,
                                        write,
                                        &dumpStack->ProgMsg,
                                        messageBuffer,
                                        mcb,
                                        dwTransferSize
                                        );

                if (!NT_SUCCESS (status)) {
                    IoDebugPrint((1, "IoWriteCrashDump: Failed to write triage-dump\n"));
                    IopFinalCrashDumpStatus = STATUS_UNSUCCESSFUL;
                    return FALSE;
                }

                IoDebugPrint((1, "IoWriteCrashDump: Successfully wrote triage-dump\n"));
                goto FinishDump;
            }

            //
            // Now loop, writing all of physical memory to the paging file.
            //

            while (mcb[0].QuadPart) {

                diskByteOffset = mcb[1];

                //
                // Calculate byte offset;
                //

                byteOffset = (ULONG)(memoryAddress & (PAGE_SIZE - 1));

                if (dwTransferSize <= mcb[0].QuadPart) {
                    byteCount = dwTransferSize - byteOffset;
                } else {
                    byteCount = mcb[0].LowPart;
                }
                pagesDoneSoFar += byteCount / PAGE_SIZE;

                currentPercentage = (ULONG)((pagesDoneSoFar * 100) /
                                    ActualPages);

                if (currentPercentage > maximumPercentage) {

                    maximumPercentage = currentPercentage;
                    //
                    // Update message on screen.
                    //

                    sprintf( messageBuffer, "%Z: %3d\r", &dumpStack->ProgMsg, maximumPercentage );
                    InbvDisplayString( messageBuffer );

                }

                //
                // Map the physical memory and write it to the
                // current segment of the file.
                //

                IopMapPhysicalMemory( mdl,
                                   memoryAddress,
                                   &dcb->MemoryDescriptor->Run[0],
                                   byteCount
                                   );


                //
                // Write the next segment.
                //

                if (!NT_SUCCESS( write( &diskByteOffset, mdl ) )) {
                    IopFinalCrashDumpStatus = STATUS_UNSUCCESSFUL;
                    return FALSE;
                }

                //
                // Adjust pointers for next part.
                //

                memoryAddress += byteCount;
                mcb[0].QuadPart = mcb[0].QuadPart - byteCount;
                mcb[1].QuadPart = mcb[1].QuadPart + byteCount;

                if (!mcb[0].QuadPart) {
                    mcb += 2;
                }

                if (pagesDoneSoFar >= ActualPages) {
                    break;

                }

            }

            IoDebugPrint((2,"IoWriteCrashDump: memory dump written\n"));
        }
        
FinishDump:

        sprintf( messageBuffer, "%Z", &dumpStack->DoneMsg );
        InbvDisplayString( messageBuffer );

        //
        // Sweep the cache so the debugger will work.
        //

        KeSweepCurrentDcache();
        KeSweepCurrentIcache();

        //
        // Have the dump flush the adapter and disk caches.
        //

        finishUp();

        //
        // Indicate to the debugger that the dump has been successfully
        // written.
        //

        IopFinalCrashDumpStatus = STATUS_SUCCESS;
    }

    //
    // Check to see whether or not auto-reboots are enabled and, if so,
    // reboot now.
    //

    if (dcb->Flags & DCB_AUTO_REBOOT) {
        IoDebugPrint (( 0,  "IODUMP: Autorebooting\n" ));
        KeReturnToFirmware( HalRebootRoutine );
    }

    return TRUE;
}



VOID
IopMapPhysicalMemory(
    IN OUT PMDL Mdl,
    IN ULONG_PTR MemoryAddress,
    IN PPHYSICAL_MEMORY_RUN PhysicalMemoryRun,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine is invoked to fill in the specified MDL (Memory Descriptor
    List) w/the appropriate information to map the specified memory address
    range.

Arguments:

    Mdl - Address of the MDL to be filled in.

    MemoryAddress - Pseudo-virtual address being mapped.

    PhysicalMemoryRun - Base address of the physical memory run list.

    Length - Length of transfer to be mapped.

Return Value:

    None.

--*/

{
    PPHYSICAL_MEMORY_RUN pmr = PhysicalMemoryRun;
    PPFN_NUMBER page;
    PFN_NUMBER pages;
    PFN_NUMBER base;
    PFN_NUMBER currentBase;

    //
    // Begin by determining the base physical page of the start of the address
    // range and filling in the MDL appropriately.
    //
    Mdl->StartVa = PAGE_ALIGN( (PVOID) (MemoryAddress) );
    Mdl->ByteOffset = (ULONG)(MemoryAddress & (PAGE_SIZE - 1));
    Mdl->ByteCount = Length;

    //
    // Get the page frame index for the base address.
    //

    base = (PFN_NUMBER) ((ULONG_PTR)(Mdl->StartVa) >> PAGE_SHIFT);
    pages = COMPUTE_PAGES_SPANNED(MemoryAddress, Length);
    currentBase = pmr->BasePage;
    page = MmGetMdlPfnArray(Mdl);

    //
    // Map all of the pages for this transfer until there are no more remaining
    // to be mapped.
    //

    while (pages) {

        //
        // Find the memory run that maps the beginning of this transfer.
        //

        while (currentBase + pmr->PageCount <= base) {
            currentBase += pmr->PageCount;
            pmr++;
        }

        //
        // The current memory run maps the start of this transfer.  Capture
        // the base page for the start of the transfer.
        //

        *page++ = pmr->BasePage + (PFN_NUMBER)(base++ - currentBase);
        pages--;
    }

    //
    // All of the PFNs for the address range have been filled in so map the
    // physical memory into virtual address space.
    //

    MmMapMemoryDumpMdl( Mdl );
}



VOID
IopAddPageToPageMap(
    IN ULONG dwMaxPage,
    IN PRTL_BITMAP pBitMapHeader,
    IN ULONG dwPageFrameIndex,
    IN ULONG dwNumberOfPages
    )
{
    //
    // Sometimes we get PFNs that are out of range. Just ignore them.
    //

    if (dwPageFrameIndex >= dwMaxPage) {
        return;
    }

    RtlSetBits (pBitMapHeader, dwPageFrameIndex, dwNumberOfPages);
}



VOID
IopRemovePageFromPageMap(
    IN ULONG dwMaxPage,
    IN PRTL_BITMAP pBitMapHeader,
    IN ULONG dwPageFrameIndex,
    IN ULONG dwNumberOfPages
    )
{
    //
    // Sometimes we get PFNs that are out of range. Just ignore them.
    //

    if (dwPageFrameIndex >= dwMaxPage) {
        return;
    }
    
    IoDebugPrint( (6, "RtlClearBits max:%x pfn:%x, pages:%x\n",(dwMaxPage), (dwPageFrameIndex), (dwNumberOfPages) )); \
    RtlClearBits (pBitMapHeader, dwPageFrameIndex, dwNumberOfPages);

}



NTKERNELAPI
NTSTATUS
IoGetCrashDumpInformation(
    OUT PSYSTEM_CRASH_DUMP_INFORMATION pCrashDumpInfo
    )

/*++

Routine Description:

    This function checks to see if an open crash dump section exists and
    if so creates a handle to the section and returns that value
    in the CrashDumpInformation structure.


Arguments:

    CrashInfo - Supplies a pointer to the crash dump information
                structure.

Return Value:

    Status of the operation.  A handle value of zero indicates no
    crash dump was located.

--*/
{

    if (!IopDumpControlBlock) {
        return STATUS_NOT_FOUND;
    }

    //
    // NB: The section object is for direct dump support, which has been
    // removed.
    //
    
    pCrashDumpInfo->hDumpSection = NULL;
    return STATUS_SUCCESS;
}



NTKERNELAPI
NTSTATUS
IoGetCrashDumpStateInformation(
    OUT PSYSTEM_CRASH_STATE_INFORMATION pCrashDumpState
    )

/*++

Routine Description:

    This routine returns sets the state variable for direct dump (fast dump)


Arguments:

    CrashInfo - Supplies a pointer to the crash dump state information
                structure.

Return Value:

    Status of the operation.  A handle value of zero indicates no
    crash dump was located.

--*/

{
    pCrashDumpState->ValidDirectDump = FALSE;

    return STATUS_SUCCESS;
}


NTSTATUS
IoSetDumpRange(
    IN PVOID   DumpContext,
    IN PVOID   StartVa,
    IN ULONG_PTR Pages,
    IN BOOLEAN IsPhysicalAddress
    )

/*++

Routine Description:

    This routine includes this range of memory in the dump

Arguments:

    DumpContext - dump context
    
    StartVa - Starting VA
    
    Pages - The number of pages to include
    
    IsPhysicalAddress - true if direct physical address translation
    
Return Value:

    STATUS_SUCCESS - On success.
    
    NTSTATUS - Error.

--*/
{
    PCHAR                   Va;
    PRTL_BITMAP             pBitMapHeader;
    PHYSICAL_ADDRESS        phyAddr;
    PSUMMARY_DUMP_HEADER    pHeader;
    BOOLEAN                 AllPagesSet;


    Va = StartVa;
    pHeader = (PSUMMARY_DUMP_HEADER) DumpContext;
    pBitMapHeader = (PRTL_BITMAP)(pHeader + 1);
    AllPagesSet = TRUE;

    //
    // Win64 can have really large page addresses.  This dump code does
    // not handle that yet.  Note that before this assert is removed
    // the casts of Pages to ULONG must be removed.
    //

    ASSERT(Pages <= MAXULONG);

    //
    // IsPhysicalAddress indicates that the va is virtually / physically
    // contiguous.
    //
    
    if (IsPhysicalAddress) {

        phyAddr = MmGetPhysicalAddress (Va);
        IopAddPageToPageMap (pHeader->BitmapSize,
                             pBitMapHeader,
                             (ULONG)(phyAddr.QuadPart >> PAGE_SHIFT),
                             (ULONG) Pages
                             );

    } else {
    
        //
        // Not physically contiguous.
        //
        
        while (Pages) {

            //
            // Only do a translation for valid pages.
            //
            
            if ( MmIsAddressValid(Va) ) {

                //
                // Get the physical mapping. Note: this does not require a lock
                //
                
                phyAddr = MmGetPhysicalAddress (Va);

                IopAddPageToPageMap (pHeader->BitmapSize,
                                     pBitMapHeader,
                                     (ULONG)(phyAddr.QuadPart >> PAGE_SHIFT),
                                     1);

                if (phyAddr.QuadPart >> PAGE_SHIFT > pHeader->BitmapSize) {
                    AllPagesSet = FALSE;
                }
            }

            Va +=  PAGE_SIZE;
            Pages--;
        }
    }

    if (AllPagesSet) {
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_ADDRESS;
}


NTSTATUS
IoFreeDumpRange(
    IN PVOID   DumpContext,
    IN PVOID   StartVa,
    IN ULONG_PTR Pages,
    IN BOOLEAN IsPhysicalAddress
    )
/*++

Routine Description:

    This routine excludes this range of memory in the dump.

Arguments:

    DumpContext - dump context
    
    StartVa - Starting VA
    
    Pages - The number of pages to include
    
    IsPhysicalAddress - true if direct physical address translation
    
Return Value:

    STATUS_SUCCESS - On success.
    
    NTSTATUS - Error.

--*/
{
    PCHAR                   Va;
    PRTL_BITMAP             pBitMapHeader;
    PHYSICAL_ADDRESS        phyAddr;
    PSUMMARY_DUMP_HEADER    pHeader;

    //
    // Round to page size.
    //
    
    Va = StartVa;
    pHeader = (PSUMMARY_DUMP_HEADER)DumpContext;
    pBitMapHeader = (PRTL_BITMAP)(pHeader + 1);

    //
    // Win64 can have really large page addresses.  This dump code does
    // not handle that yet.  Note that before this assert is removed
    // the casts of Pages to ULONG must be removed.
    //

    ASSERT (Pages <= MAXULONG);

    if (IsPhysicalAddress) {

        phyAddr = MmGetPhysicalAddress(Va);

        IopRemovePageFromPageMap (pHeader->BitmapSize,
                                  pBitMapHeader,
                                  (ULONG)(phyAddr.QuadPart >> PAGE_SHIFT),
                                  (ULONG) Pages
                                  );
    } else {

        while (Pages) {

            //
            // Only do a translation for valid pages.
            //

            if ( MmIsAddressValid (Va) ) {
                phyAddr = MmGetPhysicalAddress (Va);

                IoDebugPrint((3,"IoFreeDumpRange:Va: %x Pages: %x IsPhysical: %x phyAddr %I64x\n",
                    StartVa,Pages,IsPhysicalAddress, phyAddr.QuadPart));

                IopRemovePageFromPageMap (pHeader->BitmapSize,
                                          pBitMapHeader,
                                          (ULONG)(phyAddr.QuadPart >> PAGE_SHIFT),
                                          1);

            }

            Va += PAGE_SIZE;
            Pages--;
        }
    }

    return STATUS_SUCCESS;
}



LARGE_INTEGER
IopCalculateRequiredDumpSpace(
    IN ULONG   dwDmpFlags,
    IN ULONG   dwHeaderSize,
    IN PFN_NUMBER   dwMaxPages,
    IN PFN_NUMBER   dwMaxSummaryPages
    )

/*++

Routine Description:

    This routine is used to calcuate required dump space

        1. Crash dump summary must be at least 1 page in length.

        2. Summary dump must be large enough for kernel memory plus header,
           plus summary header.

        3. Full dump must be large enough for header plus all physical memory.

Arguments:

    dwDmpFlags - Dump Control Block (DCB) flags.

    dwHeaderSize - The size of the dump header.

    dwMaxPages - All physical memory.

    dwMaxSummaryPages - Maximum pages in summary dump.

Return Value:

    Size of the dump file

--*/
{
    LARGE_INTEGER maxMemorySize;

    //
    // Dump header or dump summary.
    //
    
    if ( (dwDmpFlags & DCB_DUMP_HEADER_ENABLED) ||
         ( !( dwDmpFlags & DCB_DUMP_ENABLED ) &&
         ( dwDmpFlags & DCB_SUMMARY_ENABLED ) ) ) {
         
        maxMemorySize.QuadPart = IO_DUMP_MINIMUM_FILE_SIZE;
        return maxMemorySize;
    }

    if (dwDmpFlags & DCB_TRIAGE_DUMP_ENABLED) {

        maxMemorySize.QuadPart = TRIAGE_DUMP_SIZE;
        return maxMemorySize;
    }

    if (dwDmpFlags & DCB_SUMMARY_DUMP_ENABLED) {
        ULONG summaryHeaderSize;
        ULONG dwGB;

        maxMemorySize.QuadPart  = (dwMaxSummaryPages) * PAGE_SIZE;

        //
        // If biased then max kernel memory is 1GB otherwise it is 2GB
        //
        
        dwGB = 1024 * 1024 * 1024;

        if (maxMemorySize.QuadPart >  (2 * dwGB) ) {
            if (MmVirtualBias) {
                maxMemorySize.QuadPart = dwGB;
            } else {
                maxMemorySize.QuadPart = (2 * dwGB);
            }
        }

        //
        // Calculate the summary header size. Includes the header plus bitmap
        //
        summaryHeaderSize = (ULONG) ROUND_TO_PAGES(
                                sizeof(SUMMARY_DUMP_HEADER) +
                                sizeof(RTL_BITMAP) +
                                ( ( (maxMemorySize.QuadPart >> PAGE_SHIFT ) >> 5 ) << 2 ) +
                                dwHeaderSize
                                );

        maxMemorySize.QuadPart+= summaryHeaderSize;

        return maxMemorySize;

    }

    //
    // Full memory dump is #pages * pagesize plus 1 page for the dump header.
    //
    
    maxMemorySize.QuadPart = (dwMaxPages * PAGE_SIZE) + dwHeaderSize;

    return maxMemorySize;

}



//
// Triage-dump support routines.
//


NTSTATUS
IopGetLoadedDriverInfo(
    OUT ULONG * lpDriverCount,
    OUT ULONG * lpSizeOfStringData
    )

/*++

Routine Description:

    Get information about all loaded drivers.

Arguments:

    lpDriverCount - Buffer to return the count of all the drivers that are
                    currently loaded in the system.

    lpSizeOfStringData - Buffer to return the sum of the sizes of all driver
                    name strings (FullDllName). This does not include the size
                    of the UNICODE_STRING structure or a trailing NULL byte.

Return Values:

    NTSTATUS

--*/

{
    ULONG DriverCount = 0;
    ULONG SizeOfStringData = 0;
    PLIST_ENTRY NextEntry;
    PLDR_DATA_TABLE_ENTRY DriverEntry;


    NextEntry = PsLoadedModuleList.Flink;
    while (NextEntry != &PsLoadedModuleList) {

        DriverEntry = CONTAINING_RECORD (NextEntry,
                                         LDR_DATA_TABLE_ENTRY,
                                         InLoadOrderLinks
                                         );

        if (!IopIsAddressRangeValid (DriverEntry, sizeof (*DriverEntry)) ||
            !IopIsAddressRangeValid (DriverEntry->BaseDllName.Buffer,
                                     DriverEntry->BaseDllName.Length)) {

            return STATUS_UNSUCCESSFUL;
        }
        
        DriverCount++;
        SizeOfStringData += DriverEntry->FullDllName.Length;
        NextEntry = NextEntry->Flink;
    }

    *lpDriverCount = DriverCount;
    *lpSizeOfStringData = SizeOfStringData;

    return STATUS_SUCCESS;
}

#define DmpPoolStringSize(DumpString)\
        (sizeof (DUMP_STRING) + sizeof (WCHAR) * ( DumpString->Length + 1 ))
        
#define DmpNextPoolString(DumpString)                                       \
        (PDUMP_STRING) (                                                    \
            ALIGN_UP_POINTER(                                               \
                ((LPBYTE) DumpString) + DmpPoolStringSize (DumpString),     \
                ULONGLONG                                                   \
                )                                                           \
            )

#define ALIGN_8(_x) ALIGN_UP(_x, DWORDLONG)


#ifndef IndexByByte
#define IndexByByte(Pointer, Index) (&(((BYTE*) (Pointer)) [Index]))
#endif

        
NTSTATUS
IopWriteDriverList(
    IN ULONG_PTR BufferAddress,
    IN ULONG BufferSize,
    IN ULONG DriverListOffset,
    IN ULONG StringPoolOffset
    )

/*++

Routine Description:

    Write the triage dump driver list to the buffer.

Arguments:

    BufferAddress - The address of the buffer.

    BufferSize - The size of the buffer.

    DriverListOffset - The offset within the buffer where the driver list
        should be written.

    StringPoolOffset - The offset within the buffer where the driver list's
        string pool should start. If there are no other strings for the triage
        dump other than driver name strings, this will be the string pool
        offset.

Return Value:

    NTSTATUS

--*/

{
    ULONG i = 0;
    PLIST_ENTRY NextEntry;
    PLDR_DATA_TABLE_ENTRY DriverEntry;
    PDUMP_DRIVER_ENTRY DumpImageArray;
    PDUMP_STRING DumpStringName = NULL;
    PIMAGE_NT_HEADERS NtHeaders;

    ASSERT (DriverListOffset != 0);
    ASSERT (StringPoolOffset != 0);
    

    DumpImageArray = (PDUMP_DRIVER_ENTRY) (BufferAddress + DriverListOffset);
    DumpStringName = (PDUMP_STRING) (BufferAddress + StringPoolOffset);

    NextEntry = PsLoadedModuleList.Flink;
    
    while (NextEntry != &PsLoadedModuleList) {

        DriverEntry = CONTAINING_RECORD (NextEntry,
                                        LDR_DATA_TABLE_ENTRY,
                                        InLoadOrderLinks);

        //
        // Verify the memory is valid before reading anything from it.
        //
        
        if (!IopIsAddressRangeValid (DriverEntry, sizeof (*DriverEntry)) ||
            !IopIsAddressRangeValid (DriverEntry->BaseDllName.Buffer,
                                     DriverEntry->BaseDllName.Length)) {

            return STATUS_UNSUCCESSFUL;
        }
            
        //
        // Build the entry in the string pool. We guarantee all strings are
        // NULL terminated as well as length prefixed.
        //

        DumpStringName->Length = DriverEntry->BaseDllName.Length / 2;
        RtlCopyMemory (DumpStringName->Buffer,
                       DriverEntry->BaseDllName.Buffer,
                       DumpStringName->Length * sizeof (WCHAR)
                       );

        DumpStringName->Buffer[ DumpStringName->Length ] = '\000';

        RtlCopyMemory (&DumpImageArray [i].LdrEntry,
                       DriverEntry,
                       sizeof (DumpImageArray [i].LdrEntry)
                       );

        //
        // Add the time/date stamp.
        //

        DumpImageArray[i].LdrEntry.TimeDateStamp = 0;
        DumpImageArray[i].LdrEntry.SizeOfImage = 0;

        if ( MmDbgReadCheck (DriverEntry->DllBase ) != NULL ) {

            NtHeaders = RtlImageNtHeader (DriverEntry->DllBase);
            ASSERT ( NtHeaders );
            DumpImageArray[i].LdrEntry.TimeDateStamp = NtHeaders->FileHeader.TimeDateStamp;
            DumpImageArray[i].LdrEntry.SizeOfImage = NtHeaders->OptionalHeader.SizeOfImage;
        }
        
        DumpImageArray [i].DriverNameOffset =
                (ULONG)((ULONG_PTR) DumpStringName - BufferAddress);

        i++;
        DumpStringName = DmpNextPoolString (DumpStringName);
        NextEntry = NextEntry->Flink;
    }

    return STATUS_SUCCESS;
}



NTSTATUS
IopWriteTriageDump(
    IN ULONG Fields,
    IN PDUMP_DRIVER_WRITE DriverWriteRoutine,
    IN OUT PLARGE_INTEGER Mcb,
    IN OUT PMDL Mdl,
    IN ULONG DriverTransferSize,
    IN PCONTEXT Context,
    IN BYTE* Buffer,
    IN ULONG BufferSize,
    IN ULONG ServicePackBuild,
    IN ULONG TriageOptions
    )

/*++

Routine Description:

    Write the Triage-Dump to the MCB.

Arguments:

    Fields - The set of fields that should be written.
    
    DriverWriteRoutine - The write routine for the driver.

    Mcb - Message Control Block where the data is to be written.

    Mdl - A MDL descrbing the data to be written (??).

    DriverTransferSize - The maximum transfer size for the driver.

    Context - The context.

    Buffer - The buffer to use as a scratch buffer.

    BufferSize - The size of the buffer.

    ServicePackBuild - Service Pack BuildNumber.

    TriageOptions - Triage Options.

Return Values:

    STATUS_SUCCESS - On success.

    NTSTATUS - Otherwise.

Comments:

    This function assumes that exactly one header page was written.

--*/

{
    ULONG SizeOfSection;
    ULONG SizeOfStringData;
    ULONG DriverCount = 0;
    LPVOID Address = NULL;
    ULONG BytesToWrite = 0;
    ULONG_PTR BufferAddress = 0;
    NTSTATUS Status;
    ULONG_PTR Offset;
    ULONG_PTR StartOfStackRegion = 0;
    PTRIAGE_DUMP_HEADER TriageDumpHeader = NULL;
    PLDR_DATA_TABLE_ENTRY DriverEntry = NULL;
    PDUMP_DRIVER_ENTRY DumpImageArray = NULL;
    PDUMP_STRING DumpStringName = NULL;
    PETHREAD Thread = NULL;

    IoDebugPrint ((0, "[IopWriteTriageDump] BufferSize = %#x ServicePackBuild = %d\n",
                   BufferSize,
                   ServicePackBuild));

    //
    // Setup the triage-dump header.
    //

    if (BufferSize < sizeof (TRIAGE_DUMP_HEADER) + sizeof (DWORD)) {
        return STATUS_NO_MEMORY;
    }
    
    TriageDumpHeader = (PTRIAGE_DUMP_HEADER) Buffer;
    RtlZeroMemory (TriageDumpHeader, sizeof (*TriageDumpHeader));

    //
    // The normal dump header is of size PAGE_SIZE.
    //
    
    TriageDumpHeader->SizeOfDump = BufferSize + PAGE_SIZE;

    //
    // Adjust the BufferSize so we can write the final status DWORD at the
    // end.
    //
    
    BufferSize -= sizeof (DWORD);
    RtlZeroMemory (IndexByByte (Buffer, BufferSize), sizeof (DWORD));

    TriageDumpHeader->ValidOffset = ( TriageDumpHeader->SizeOfDump - sizeof (ULONG) );
    TriageDumpHeader->ContextOffset = DH_CONTEXT_RECORD * sizeof (ULONG);
    TriageDumpHeader->ExceptionOffset = DH_EXCEPTION_RECORD * sizeof (ULONG);
    TriageDumpHeader->BrokenDriverOffset = 0;
    TriageDumpHeader->ServicePackBuild = ServicePackBuild;
    TriageDumpHeader->TriageOptions = TriageOptions;

    Offset = ALIGN_8 (PAGE_SIZE + sizeof (TRIAGE_DUMP_HEADER));

    //
    // Set the Mm Offset, if necessary.
    //

    SizeOfSection = ALIGN_8 (MmSizeOfTriageInformation());

    if (Offset + SizeOfSection < BufferSize) {
        TriageDumpHeader->MmOffset = (ULONG)Offset;
        Offset += SizeOfSection;
    }

    //
    // Set the Unloaded Drivers Offset, if necessary.
    //

    SizeOfSection = ALIGN_8 (MmSizeOfUnloadedDriverInformation());

    if (Offset + SizeOfSection < BufferSize) {
        TriageDumpHeader->UnloadedDriversOffset = (ULONG)Offset;
        Offset += SizeOfSection;
    }

    //
    // Set the Prcb Offset, if necessary.
    //

    if (Fields & TRIAGE_DUMP_PRCB) {
        SizeOfSection = ALIGN_8 (sizeof (KPRCB));

        if (Offset + SizeOfSection < BufferSize) {
            TriageDumpHeader->PrcbOffset = (ULONG)Offset;
            Offset += SizeOfSection;
        }
    }

    //
    // Set the Process Offset, if necessary.
    //

    if (Fields & TRIAGE_DUMP_PROCESS) {
        SizeOfSection = ALIGN_8 (sizeof (EPROCESS));

        if (Offset + SizeOfSection < BufferSize) {
            TriageDumpHeader->ProcessOffset = (ULONG)Offset;
            Offset += SizeOfSection;
        }
    }

    //
    // Set the Thread Offset, if necessary.
    //
    
    if (Fields & TRIAGE_DUMP_THREAD) {
        SizeOfSection = ALIGN_8 (sizeof (ETHREAD));

        if (Offset + SizeOfSection < BufferSize) {
            TriageDumpHeader->ThreadOffset = (ULONG)Offset;
            Offset += SizeOfSection;
        }
    }

    //
    // Set the CallStack Offset, if necessary.
    //

    Thread = PsGetCurrentThread ();

    if (Fields & TRIAGE_DUMP_STACK) {

        //
        // If there is a stack, calculate its size.
        //
        
        if (Thread->Tcb.KernelStackResident) {

            ULONG_PTR StackBase;

            StackBase = (ULONG_PTR) Thread->Tcb.InitialStack;

            ASSERT ( StackBase > STACK_POINTER (Context));
            
            //
            // There is a valid stack. Note that we limit the size of
            // the triage dump stack to MAX_TRIAGE_STACK_SIZE (currently
            // 16 KB).
            //

            StartOfStackRegion = (ULONG_PTR) STACK_POINTER (Context);
            SizeOfSection = (ULONG) min ( StackBase -  (ULONG_PTR) STACK_POINTER (Context),
                                  MAX_TRIAGE_STACK_SIZE
                                  );

            ASSERT ( StartOfStackRegion + SizeOfSection <= StackBase);
                
        } else {

            //
            // There is not a valid stack.
            //

            SizeOfSection = 0;
        }

        if (SizeOfSection && Offset + SizeOfSection < BufferSize) {
            TriageDumpHeader->CallStackOffset = (ULONG)Offset;
            TriageDumpHeader->SizeOfCallStack = SizeOfSection;
            TriageDumpHeader->BaseOfStack = (ULONG) StartOfStackRegion;
            Offset += SizeOfSection;
        }
    }

    //
    // Set the Driver List Offset, if necessary.
    //
    
    Status = IopGetLoadedDriverInfo (&DriverCount, &SizeOfStringData);

    if (NT_SUCCESS (Status) && Fields & TRIAGE_DUMP_DRIVER_LIST) {
        SizeOfSection = ALIGN_8 (DriverCount * sizeof (DUMP_DRIVER_ENTRY));

        if (SizeOfSection && (Offset + SizeOfSection < BufferSize)) {
            TriageDumpHeader->DriverListOffset = (ULONG)Offset;
            TriageDumpHeader->DriverCount = DriverCount;
            Offset += SizeOfSection;
        }

    } else {

        SizeOfSection = 0;
        SizeOfStringData = 0;
    }
            
    //
    // Set the String Pool offset.
    //

    SizeOfSection = ALIGN_8 (SizeOfStringData +
                        DriverCount * (sizeof (WCHAR) + sizeof (DUMP_STRING)));

    if (SizeOfSection && (Offset + SizeOfSection < BufferSize)) {
        TriageDumpHeader->StringPoolOffset = (ULONG)Offset;
        TriageDumpHeader->StringPoolSize = SizeOfSection;
        Offset += SizeOfSection;
    }


    BytesToWrite = (ULONG)Offset;
    BufferAddress = ((ULONG_PTR) Buffer) - PAGE_SIZE ;

    //
    // Write the Mm information.
    //

    if (TriageDumpHeader->MmOffset) {

        Address = (LPVOID) (BufferAddress + TriageDumpHeader->MmOffset);
        MmWriteTriageInformation (Address);
    }

    if (TriageDumpHeader->UnloadedDriversOffset) {

        Address = (LPVOID) (BufferAddress + TriageDumpHeader->UnloadedDriversOffset);
        MmWriteUnloadedDriverInformation (Address);
    }

    //
    // Write the PRCB.
    //

    if (TriageDumpHeader->PrcbOffset) {

        Address = (LPVOID) (BufferAddress + TriageDumpHeader->PrcbOffset);
        RtlCopyMemory (Address,
                       KeGetCurrentPrcb (),
                       sizeof (KPRCB)
                       );
    }

    //
    // Write the EPROCESS.
    //
    
    if (TriageDumpHeader->ProcessOffset) {
    
        Address = (LPVOID) (BufferAddress + TriageDumpHeader->ProcessOffset);
        RtlCopyMemory (Address,
                       PsGetCurrentProcess (),
                       sizeof (EPROCESS)
                       );
    }

    //
    // Write the ETHREAD.
    //
    
    if (TriageDumpHeader->ThreadOffset) {
    
        Address = (LPVOID) (BufferAddress + TriageDumpHeader->ThreadOffset);
        RtlCopyMemory (Address,
                       KeGetCurrentThread (),
                       sizeof (ETHREAD)
                       );
    }

    //
    // Write the Call Stack.
    //

    if (TriageDumpHeader->CallStackOffset) {

        ASSERT (Thread);
        ASSERT (TriageDumpHeader->SizeOfCallStack != 0);

        Address = (LPVOID) (BufferAddress + TriageDumpHeader->CallStackOffset);
        RtlCopyMemory (Address,
                       (PVOID) StartOfStackRegion,
                       TriageDumpHeader->SizeOfCallStack
                       );
    }

    //
    // Write the Driver List.
    //
    
    if (TriageDumpHeader->DriverListOffset &&
        TriageDumpHeader->StringPoolOffset) {

        Status = IopWriteDriverList (BufferAddress,
                                     BufferSize,
                                     TriageDumpHeader->DriverListOffset,
                                     TriageDumpHeader->StringPoolOffset
                                     );

        if (!NT_SUCCESS (Status)) {
            TriageDumpHeader->DriverListOffset = 0;
        }
    }

    ASSERT (BytesToWrite < BufferSize);
    ASSERT (ALIGN_UP (BytesToWrite, PAGE_SIZE) < BufferSize);

    IoDebugPrint ((3, "TRIAGEDUMP: Calling IopWriteToDisk with:\n"
                   "\tBytesToWrite = %#x\n"
                   "\tBytesToWrite (Aligned) = %#x\n"
                   "\tBufferSize = %#x\n",
                    BytesToWrite,
                    ALIGN_UP (BytesToWrite, PAGE_SIZE),
                    BufferSize));


    //
    // Write the valid status to the end of the dump.
    //

    *((ULONG *)IndexByByte (Buffer, BufferSize)) = TRIAGE_DUMP_VALID ;

    //
    // Re-adjust the buffer size.
    //
    
    BufferSize += sizeof (DWORD);

    //
    // NOTE: This routine writes the entire buffer, even if it is not
    // all required.
    //

    Status = IopWriteToDisk (Buffer,
                             BufferSize,
                             DriverWriteRoutine,
                             &Mcb,
                             Mdl,
                             DriverTransferSize
                             );

    return Status;
}



NTSTATUS
IopWritePageToDisk(
    IN PDUMP_DRIVER_WRITE DriverWrite,
    IN OUT PLARGE_INTEGER * McbBuffer,
    IN OUT ULONG DriverTransferSize,
    IN PFN_NUMBER PageFrameIndex
    )

/*++

Routine Description:

    Write the page described by PageFrameIndex to the disk/file (DriverWrite,
    McbBuffer) and update the MCB buffer to reflect the new position in the
    file.
    
Arguments:

    DriverWrite - The driver write routine.

    McbBuffer - A pointer to the MCB array. This array is terminated by
            a zero-length MCB entry. On success, this pointer is updated
            to reflect the new position in the MCB array.

            NB: MCB[0] is the size and MCB[1] is the offset.

    DriverTransferSize - The maximum transfer size for this driver.

    PageFrameIndex - The page to be written.

Return Values:

    NTSTATUS code.

--*/

{
    NTSTATUS Status;
    PFN_NUMBER MdlHack [ (sizeof (MDL) / sizeof (PFN_NUMBER)) + 1];
    PPFN_NUMBER PfnArray;
    PLARGE_INTEGER Mcb;
    ULONG ByteCount;
    ULONG ByteOffset;
    ULONG BytesToWrite;
    PMDL TempMdl;


    ASSERT ( DriverWrite );
    ASSERT ( McbBuffer );
    ASSERT ( DriverTransferSize && DriverTransferSize >= PAGE_SIZE );

    //
    // Initialization
    //

    TempMdl = (PMDL) &MdlHack;
    Mcb = *McbBuffer;
    BytesToWrite = PAGE_SIZE;


    //
    // Initialze the MDL to point to this page.
    //
    
    MmInitializeMdl (TempMdl, NULL, PAGE_SIZE);

    PfnArray = MmGetMdlPfnArray ( TempMdl );
    PfnArray[0] = PageFrameIndex;


    //
    // We loop for the cases when the space remaining in this block (Mcb [0])
    // is less than one page. Generally the Mcb will be large enough to hold
    // the entire page and this loop will only be executed once. When Mcb[0]
    // is less than a page, we will write the first part of the page to this
    // Mcb then increment the Mcb and write the remaining part to the next
    // page.
    //
    
    ByteOffset = 0;

    while ( BytesToWrite ) {

        ASSERT ( Mcb[0].QuadPart != 0 );
        
        ByteCount = (ULONG) min3 ((LONGLONG) BytesToWrite,
                                  (LONGLONG) DriverTransferSize,
                                  Mcb[0].QuadPart
                                  );

        ASSERT ( ByteCount != 0 );

        //
        // Update the MDL byte count and byte offset. 
        //
        
        TempMdl->ByteCount = ByteCount;
        TempMdl->ByteOffset = ByteOffset;

        //
        // Map the MDL. The flags are updated to show that MappedSsytemVa is
        // valid, which should probably be done in MmMapMemoryDumpMdl.
        //
        
        MmMapMemoryDumpMdl ( TempMdl );
        TempMdl->MdlFlags |= ( MDL_PAGES_LOCKED | MDL_MAPPED_TO_SYSTEM_VA );
        TempMdl->StartVa = PAGE_ALIGN (TempMdl->MappedSystemVa);
        
        Status = DriverWrite ( &Mcb[1], TempMdl );


        if (!NT_SUCCESS (Status)) {
            return Status;
        }

        BytesToWrite -= ByteCount;
        ByteOffset += ByteCount;
        
        Mcb[0].QuadPart -= ByteCount;
        Mcb[1].QuadPart += ByteCount;


        //
        // If there is no more room for this MCB, go to the next one.
        //
        
        if ( Mcb[0].QuadPart == 0 ) {

            Mcb += 2;

            //
            // We have filled up all the space in the paging file. 
            //
            
            if ( Mcb[0].QuadPart == 0) {
                return STATUS_END_OF_FILE;
            }
        }
    }

    *McbBuffer = Mcb;

    return Status;
}


NTSTATUS
IopWriteSummaryDump(
    IN PRTL_BITMAP PageMap,
    IN PDUMP_DRIVER_WRITE DriverWriteRoutine,
    IN PANSI_STRING ProgressMessage,
    IN PUCHAR MessageBuffer,
    IN OUT PLARGE_INTEGER Mcb,
    IN OUT ULONG DriverTransferSize
    )

/*++

Routine Description:

    Write a summary dump to the disk.

Arguments:


    PageMap - A bitmap of the pages that need to be written.

    DriverWriteRoutine - The driver's write routine.

    ProgressMessage - The "Percent Complete" message.

    MessageBuffer - A message buffer we can use to update percentage complete
            status.

    Mcb - Message Control Block where the data is to be written.

    DriverTransferSize - The maximum transfer size for the driver.

Return Values:

    NTSTATUS code.

--*/

{
    PVOID Va;
    PFN_NUMBER PageFrameIndex;
    PHYSICAL_ADDRESS PhysicalAddress;
    NTSTATUS Status;

    ULONG WriteCount;
    ULONG MaxWriteCount;
    ULONG Step;
    

    ASSERT ( DriverWriteRoutine != NULL );
    ASSERT ( Mcb != NULL );
    ASSERT ( DriverTransferSize != 0 );
    

    MaxWriteCount = RtlNumberOfSetBits ( PageMap );
    Step = MaxWriteCount / 100;

    IoDebugPrint ((1, "IODUMP: Summary Dump\n"
                     "  Writing %x pages to disk from a total of %x\n",
                  MaxWriteCount,
                  PageMap->SizeOfBitMap));

    //
    // Loop over all pages in the system and write those that are set
    // in the bitmap.
    //

    WriteCount = 0;
    for ( PageFrameIndex = 0;
          PageFrameIndex < PageMap->SizeOfBitMap;
          PageFrameIndex++) {


        //
        // If this page needs to be included in the dump file.
        //
        
        if ( RtlCheckBit (PageMap, PageFrameIndex) ) {

            if (++WriteCount % Step == 0) {

                sprintf (MessageBuffer,
                         "%Z: %3d\r",
                         ProgressMessage,
                         (WriteCount * 100) / MaxWriteCount);

                InbvDisplayString ( MessageBuffer );
            }

            ASSERT ( WriteCount <= MaxWriteCount );

            //
            // Write the page to disk.
            //
            
            Status = IopWritePageToDisk (
                            DriverWriteRoutine,
                            &Mcb,
                            DriverTransferSize,
                            PageFrameIndex
                            );
            
            if (!NT_SUCCESS (Status)) {

                return STATUS_UNSUCCESSFUL;
            }
        }
    }

    return STATUS_SUCCESS;
}


PSUMMARY_DUMP_HEADER
IopInitializeSummaryDump(
    IN PDUMP_CONTROL_BLOCK pDcb
    )
/*++

Routine Description:

    This routine creates a summary dump header. In particular it initializes
    a bitmap that contains a map of kernel memory.

Arguments:

    PDUMP_CONTROL_BLOCK - A pointer to the dump control block.

Return Value:

    Non-NULL - A pointer to the summary dump header

    NULL - Error

--*/
{
    PULONG                  pdwBlock;
    PSUMMARY_DUMP_HEADER    pSummaryHeader;
    ULONG                   dwActualPages;

    //
    // Get the dump header page.
    //

    pdwBlock = pDcb->HeaderPage;

    //
    // The summary dump starts 1 page after the header.
    //

    pSummaryHeader = (PSUMMARY_DUMP_HEADER)&pdwBlock[ DH_SUMMARY_DUMP_RECORD ];

    //
    // Fill the header with signatures.
    //
    RtlFillMemoryUlong( pSummaryHeader,
                        sizeof(SUMMARY_DUMP_HEADER),
                        'PMDS' );

    //
    // Set the size and valid signature.
    //
    
    pSummaryHeader->BitmapSize = (ULONG)( MmPhysicalMemoryBlock->Run[MmPhysicalMemoryBlock->NumberOfRuns-1].BasePage  +
                                      MmPhysicalMemoryBlock->Run[MmPhysicalMemoryBlock->NumberOfRuns-1].PageCount );
    pSummaryHeader->ValidDump = 'PMUD';

    //
    // Construct the kernel memory bitmap.
    //
    
    dwActualPages = IopCreateSummaryDump(pSummaryHeader);

    IoDebugPrint((2,"[IopInitializeSummaryDump]: Kernel Pages = %x\n",dwActualPages));

    if (!dwActualPages) {
        return NULL;
    }

    //
    // Set the actual number of physical pages in the summary dump
    //
    
    pSummaryHeader->Pages = dwActualPages;
    pSummaryHeader->HeaderSize = pDcb->HeaderSize;

    return pSummaryHeader;
}



NTSTATUS
IopWriteSummaryHeader(
    IN PSUMMARY_DUMP_HEADER    pSummaryHeader,
    IN PDUMP_DRIVER_WRITE      pfWrite,
    IN OUT PLARGE_INTEGER *    McbBuffer,
    IN OUT PMDL                pMdl,
    IN ULONG                   dwWriteSize,
    IN ULONG                   dwLength
    )
/*++

Routine Description:

    Write the summary dump header to the dump file.

Arguments:

    pSummaryHeader - pointer to the summary dump bitmap

    pfWrite - dump driver write function

    McbBuffer - pointer to the MCB array.

    pMdl - Pointer to an MDL

    dwWriteSize - the max transfer size for the dump driver

    dwLength - the length of this transfer

Return Value:

    Updated MCB

--*/
{
    NTSTATUS Status;
    ULONG       dwBytesRemaining;
    ULONG_PTR   dwMemoryAddress;
    ULONG       dwByteOffset;
    ULONG       dwByteCount;
    PLARGE_INTEGER pMcb;

    dwBytesRemaining = dwLength;
    dwMemoryAddress = (ULONG_PTR) pSummaryHeader;
    pMcb = *McbBuffer;

    IoDebugPrint (( 0, "IoWriteCrashDump: Writing SUMMARY dump header to disk\n" ));

    while (dwBytesRemaining) {

        // Calculate byte offset
        dwByteOffset = (ULONG)(dwMemoryAddress & (PAGE_SIZE - 1));

        //
        // See if the number of bytes to write is greator than the crash
        // drives max transfer.
        //
        
        if (dwBytesRemaining <= dwWriteSize) {
            dwByteCount = dwBytesRemaining;
        } else {
            dwByteCount = dwWriteSize;
        }

        //
        // If the byteCount is greater than the remaining mcb then correct it.
        //
        
        if (dwByteCount > pMcb[0].QuadPart) {
            dwByteCount = pMcb[0].LowPart;
        }

        pMdl->ByteCount      = dwByteCount;
        pMdl->ByteOffset     = dwByteOffset;
        pMdl->MappedSystemVa = (PVOID) dwMemoryAddress;

        //
        // Get the actual physical frame and create an mdl.
        //
        
        IopMapVirtualToPhysicalMdl(pMdl, dwMemoryAddress, dwByteCount);

        //
        // Write to disk.
        //
        Status = pfWrite( &pMcb[1], pMdl );

        if ( !NT_SUCCESS (Status) ) {
            return Status;
        }

        //
        // Adjust bytes remaining.
        //

        dwBytesRemaining -= dwByteCount;
        dwMemoryAddress  += dwByteCount;

        pMcb[0].QuadPart = pMcb[0].QuadPart - dwByteCount;
        pMcb[1].QuadPart = pMcb[1].QuadPart + dwByteCount;

        if (pMcb[0].QuadPart == 0) {
            pMcb += 2;
        }

        if (pMcb[0].QuadPart == 0) {
            return STATUS_END_OF_FILE;
        }
    }

    IoDebugPrint((2, "IoWriteCrashDump: Writing SUMMARY dump header to disk\n" ));

    *McbBuffer = pMcb;
    return STATUS_SUCCESS;
}



NTSTATUS
IopWriteToDisk(
    IN PVOID Buffer,
    IN ULONG WriteLength,
    IN PDUMP_DRIVER_WRITE DriverWriteRoutine,
    IN OUT PLARGE_INTEGER * McbBuffer,
    IN OUT PMDL Mdl,
    IN ULONG DriverTransferSize
    )
/*++

Routine Description:

    Write the summary dump header to the dump file.

Arguments:

    Buffer - Pointer to the buffer to write.

    WriteLength - The length of this transfer.

    DriverWriteRoutine - Dump driver write function.

    McbBuffer - Pointer to the dump file Mapped Control Block.

    Mdl - Pointer to an MDL.

    DriverTransferSize - The max transfer size for the dump driver.


Return Value:


--*/
{
    ULONG dwBytesRemaining;
    ULONG_PTR dwMemoryAddress;
    ULONG dwByteOffset;
    ULONG dwByteCount;
    PLARGE_INTEGER Mcb;

    ASSERT (Buffer);
    ASSERT (WriteLength);
    ASSERT (DriverWriteRoutine);
    ASSERT (McbBuffer && *McbBuffer);
    ASSERT (Mdl);
    ASSERT (DriverTransferSize >= IO_DUMP_MINIMUM_TRANSFER_SIZE &&
            DriverTransferSize <= IO_DUMP_MAXIMUM_TRANSFER_SIZE);
    

    Mcb = *McbBuffer;
    dwBytesRemaining = WriteLength;
    dwMemoryAddress = (ULONG_PTR) Buffer;

    IoDebugPrint(( 2, "IoWriteToDisk: Writing %d bytes to disk.\n", WriteLength ));

    while (dwBytesRemaining) {

        ASSERT (Mcb [0].QuadPart != 0);
        ASSERT (IopDumpControlBlock->FileDescriptorArray <= Mcb &&
                (LPBYTE) Mcb < (LPBYTE) IopDumpControlBlock->FileDescriptorArray +
                               IopDumpControlBlock->FileDescriptorSize
                );
        
        dwByteOffset = BYTE_OFFSET (dwMemoryAddress);

        //
        // See if the number of bytes to write is greator than the crash
        // drives max transfer.
        //

        dwByteCount = min ( dwBytesRemaining, DriverTransferSize );

        //
        // If the byteCount is greater than the remaining mcb then correct it.
        //
        
        if (dwByteCount > Mcb[0].QuadPart) {
            dwByteCount = Mcb[0].LowPart;
        }

        Mdl->ByteCount = dwByteCount;
        Mdl->ByteOffset = dwByteOffset;
        Mdl->MappedSystemVa = (PVOID) dwMemoryAddress;

        //
        // Get the actual physical frame and create an mdl.
        //
        
        IopMapVirtualToPhysicalMdl(Mdl, dwMemoryAddress, dwByteCount);

        if (!NT_SUCCESS( DriverWriteRoutine ( &Mcb[1], Mdl ) )) {
            IoDebugPrint ((1, "IopWriteToDisk: Failed write.\n"));
            return STATUS_UNSUCCESSFUL;
        }

        //
        // Adjust bytes remaining.
        //

        ASSERT (dwBytesRemaining >= dwByteCount);
        ASSERT (dwByteCount != 0);
        
        dwBytesRemaining -= dwByteCount;
        dwMemoryAddress  += dwByteCount;

        Mcb[0].QuadPart -= dwByteCount;
        Mcb[1].QuadPart += dwByteCount;
        
        if (Mcb[0].QuadPart == 0) {
            Mcb += 2;
        }
    }

    IoDebugPrint ((2, "IopWriteToDisk: Successfully wrote %d bytes.\n", WriteLength));

    *McbBuffer = Mcb;
    return STATUS_SUCCESS;
}


VOID
IopMapVirtualToPhysicalMdl(
    IN OUT PMDL pMdl,
    IN ULONG_PTR dwMemoryAddress,
    IN ULONG    dwLength
    )
{
    PPFN_NUMBER  pdwPage;
    ULONG   dwPages;
    ULONG   dwBase;
    ULONG   dwCurrentBase;
    ULONG_PTR dwBaseVa;
    PHYSICAL_ADDRESS PhysicalAddress;

    //
    // Begin by determining the base physical page of the start of the address
    // range and filling in the MDL appropriately.
    //

    pMdl->StartVa = PAGE_ALIGN((PVOID)dwMemoryAddress);
    pMdl->ByteOffset= (ULONG)(dwMemoryAddress & (PAGE_SIZE - 1));
    pMdl->ByteCount = dwLength;
    dwBaseVa = dwMemoryAddress & ~(PAGE_SIZE -1);
    pMdl->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;

    //
    // Compute the number of pages spanned
    //

    dwPages = COMPUTE_PAGES_SPANNED( dwMemoryAddress, dwLength );
    pdwPage = MmGetMdlPfnArray(pMdl);

    //
    // Map all of the pages for this transfer until there are no more remaining
    // to be mapped.
    //
    IoDebugPrint((3,"MapVirtualToPhysical: VA: %08x off: %08x Len:%08x base: %08x \n",pMdl->StartVa,pMdl->ByteOffset,pMdl->ByteCount,dwBaseVa));

    while (dwPages) {
        PhysicalAddress = MmGetPhysicalAddress( (PVOID)dwBaseVa );
        IoDebugPrint( (3,"Physical Frame: %08x Adr: %08x\n",( PhysicalAddress.QuadPart >> PAGE_SHIFT),dwBaseVa ) );
        *pdwPage++  = (PFN_NUMBER)(PhysicalAddress.QuadPart >> PAGE_SHIFT);
        dwBaseVa    +=PAGE_SIZE;
        dwPages--;
    }

    //
    // All of the PFNs for the address range have been filled in so map the
    // physical memory into virtual address space using crash dump PTE.
    //

//    MmMapMemoryDumpMdl( pMdl );
}



ULONG
IopCreateSummaryDump (
    IN PSUMMARY_DUMP_HEADER pHeader
    )
/*++

Routine Description:

    This routine determines the kernel memory and data structures to include
    in the summary memory dump.

    NOTE: This function uses MmGetPhysicalAddress. MmGetPhysicalAddress does
    not acquire any locks. It uses a set of macros for translation.


Arguments:

    pHeader - Pointer to the dump header

    pDcb - Dump control block
    
Return Value:

    Status

--*/
{
    PRTL_BITMAP             pBitMapHeader = (PRTL_BITMAP)(pHeader+1);
    ULONG                   Pages;
    PCHAR                   VA;
    ULONG                   UserPages;
    PHYSICAL_ADDRESS        PhyAddr;
    ULONG                   i;
    PULONG                  pBitMapBuffer;
    PUCHAR                  pBitMapBytes;
    LARGE_INTEGER           dumpFileSize;
    PULONG                  block;
    ULONG                   numDumpPages;

    //
    // The bitmap is the last structure in the header. The
    // buffer is allocated directly after the bitmap header.
    //
    // Initialize Bit Map, set the size and buffer address.
    //
    
    pBitMapHeader->SizeOfBitMap = pHeader->BitmapSize;
    pBitMapBuffer = (PULONG)( pBitMapHeader + 1);
    pBitMapHeader->Buffer = pBitMapBuffer;
    pBitMapBytes = (PUCHAR)pBitMapBuffer;

    //
    // Clear all bits
    //

    RtlClearAllBits (pBitMapHeader);

    //
    // Have MM initialize the kernel memory to dump
    //
    
    MmSetKernelDumpRange(pHeader);

    //
    // Exclude non-existent memory
    //
    
    IopDeleteNonExistentMemory(pHeader, MmPhysicalMemoryBlock);


    Pages = RtlNumberOfSetBits ( pBitMapHeader );
    IoDebugPrint((2,"IopLocalSetBits =       %x\n",Pages));

    //
    // See If we have room to Include user va for the current process
    //
    
    block = IopDumpControlBlock->HeaderPage;
    dumpFileSize.LowPart= block [DH_REQUIRED_DUMP_SPACE];
    dumpFileSize.HighPart= block [DH_REQUIRED_DUMP_SPACE + 1];

    dumpFileSize.QuadPart -= IopDumpControlBlock->HeaderSize;

    //
    // Total physical pages will not exceed 2<<32 for sometime
    //
    
    numDumpPages= (ULONG)(dumpFileSize.QuadPart >> PAGE_SHIFT);
    IoDebugPrint ((2,"IopCreateSummaryDump: numDumpPages: %x Pages : %x\n", numDumpPages,Pages));

    //
    // Only copy user virtual if there is enough room in the dump file.
    //
    
    UserPages = 0;

    if (Pages < numDumpPages ) {

        //
        // Stride through user VA and include all valid pages
        //

        for (VA = 0; VA < (PCHAR)MmHighestUserAddress; VA+=PAGE_SIZE ) {

            if ( MmIsAddressValid( VA ) ) {

                if ( NT_SUCCESS(IoSetDumpRange(pHeader,VA, 1, FALSE ) ) ) {

                    UserPages++;

                    //
                    // Break if there is no more room in the dump file
                    //

                    if (UserPages+Pages >= numDumpPages ) {
                        break;
                    }
                }
            }
        }

    }

    Pages+= UserPages;

    IoDebugPrint((2,"IopCreateSummaryDump number of user mode pages = %x\n",UserPages));

    IoDebugPrint((2, "SummaryDump: Dump SummaryDumpHeader\n"));
    IoDebugPrint((2, "SdBitmapSize      =%x\n", pHeader->BitmapSize));
    IoDebugPrint((2, "SdAllBits         =%x\n", Pages));
    IoDebugPrint((2, "&SdBitmapBuffer[0]=%x\n", pBitMapHeader->Buffer));


    return Pages;

}


VOID
IopDeleteNonExistentMemory(
    PSUMMARY_DUMP_HEADER pHeader,
    PPHYSICAL_MEMORY_DESCRIPTOR MmPhysicalMemoryBlock
    )
/*++

Routine Description:

    This deletes non existent memory. Non existent memory is defined as the
    unassigned memory between memory runs. For example, memory between
    (run[0].base + size) and run[1].base.
    
Arguments:

    pHeader - This is a pointer to the summary dump header.
    
    dwStartingIndex - The starting bit index in the bitmap (starting page).

    dwMemoryAddress - Pseudo-virtual address being mapped.

    dwByteCount - The number of bytes to copy.

    dwMaxBitmapBit - The limit or the highest possible valid bit in the bitmap.

    pMdl - The MDL to create.

Return Value:

    (none)
    
--*/
{
    PPHYSICAL_MEMORY_RUN    Run;
    ULONG                   NumberOfRuns;
    ULONG                   i;
    PFN_NUMBER              CurrentPageFrame, NextPageFrame;
    PRTL_BITMAP             pBitMapHeader;

    NumberOfRuns            = MmPhysicalMemoryBlock->NumberOfRuns;

    Run = &MmPhysicalMemoryBlock->Run[0];

    pBitMapHeader = (PRTL_BITMAP)(pHeader + 1);
    i = 0;

    CurrentPageFrame = 1;

    NextPageFrame = Run->BasePage;

    //
    // Remove the gap from 0 till the first run.
    //
    
    if (NextPageFrame > CurrentPageFrame) {

        IoDebugPrint( (2,"DeleteNonExistentMemory: %x - %x\n",
            CurrentPageFrame,
            (NextPageFrame - CurrentPageFrame) ) );

        //
        // FIXFIX - handle page frame numbers greater than 32 bits.
        //

        IopRemovePageFromPageMap (pHeader->BitmapSize,
                                  pBitMapHeader,
                                  (ULONG)CurrentPageFrame,
                                  (ULONG)(NextPageFrame-CurrentPageFrame)
                                  );

    }

    //
    // Remove the gaps between runs.
    //
    
    while (i < NumberOfRuns - 1) {
    
        CurrentPageFrame = Run->BasePage + Run->PageCount;
        IoDebugPrint((2, "Run[%x]: Base=%x, Pages=%x\n", i, Run->BasePage, Run->PageCount));

        i++;
        Run++;

        //
        // Get the next starting BasePage.
        //
        
        NextPageFrame = Run->BasePage;

        if (NextPageFrame != CurrentPageFrame) {

            IoDebugPrint((2, "DeleteNonExistentMemory: %x - %x\n", NextPageFrame, CurrentPageFrame));

            //
            // FIXFIX - handle page frame numbers greater than 32 bits.
            //

            IopRemovePageFromPageMap (pHeader->BitmapSize,
                                      pBitMapHeader,
                                      (ULONG)CurrentPageFrame,
                                      (ULONG)(NextPageFrame - CurrentPageFrame)
                                      );
        }
    }
}


NTSTATUS
IopCompleteDumpInitialization(
    IN HANDLE     FileHandle
    )

/*++

Routine Description:

    This routine is invoked after the dump file is created.

    The purpose is to obtain the retrieval pointers so that they can then be
    used later to write the dump. The last step is to checksum the
    IopDumpControlBlock.

    Fields in the IopDumpControlBlock are updated if necessary and the
    IopDumpControlBlockChecksum is initialized.

    This is the final step in dump initialization.

Arguments:

    FileHandle - Handle to the dump file just created.

Return Value:

    STATUS_SUCCESS - Indicates success.

    Other NTSTATUS - Failure.

--*/

{
    NTSTATUS Status;
    NTSTATUS ErrorToLog;
    ULONG i;
    LARGE_INTEGER RequestedFileSize;
    PLARGE_INTEGER mcb;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_STANDARD_INFORMATION StandardFileInfo;

    Status = STATUS_UNSUCCESSFUL;
    ErrorToLog = STATUS_SUCCESS;    // No error
    FileObject = NULL;
    DeviceObject = NULL;

    Status = ObReferenceObjectByHandle( FileHandle,
                                        0,
                                        IoFileObjectType,
                                        KernelMode,
                                        (PVOID *) &FileObject,
                                        NULL
                                        );

    if ( !NT_SUCCESS (Status) ) {
        ASSERT (FALSE);
        goto Cleanup;
    }


    DeviceObject = FileObject->DeviceObject;

    //
    // If this device object represents the boot partition then query
    // the retrieval pointers for the file.
    //

    if ( !(DeviceObject->Flags & DO_SYSTEM_BOOT_PARTITION) ) {
         
        KdPrint (("IODUMP: Cannot dump to pagefile on non-system partition.\n"));
        goto Cleanup;
    }

    Status = ZwQueryInformationFile(
                                FileHandle,
                                &IoStatusBlock,
                                &StandardFileInfo,
                                sizeof (StandardFileInfo),
                                FileStandardInformation
                                );

    if (Status == STATUS_PENDING) {
        Status = KeWaitForSingleObject( &FileObject->Event,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL
                                        );
        Status = IoStatusBlock.Status;
    }

    if ( !NT_SUCCESS (Status) ) {
        KdPrint (("CRASHDUMP: Failed ZwQueryInformationFile\n"));
        goto Cleanup;
    }

    //
    // Do not ask for more space than is in the pagefile.
    //

    RequestedFileSize = IopDumpControlBlock->DumpFileSize;
    
    if (RequestedFileSize.QuadPart > StandardFileInfo.EndOfFile.QuadPart) {
        RequestedFileSize = StandardFileInfo.EndOfFile;
    }

    Status = ZwFsControlFile(
                        FileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        FSCTL_QUERY_RETRIEVAL_POINTERS,
                        &RequestedFileSize,
                        sizeof( LARGE_INTEGER ),
                        &mcb,
                        sizeof( PVOID )
                        );

    if (Status == STATUS_PENDING) {
        Status = KeWaitForSingleObject( &FileObject->Event,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL );
        Status = IoStatusBlock.Status;
    }


    //
    // NOTE: If you fail here, put a BP on ntfs!NtfsQueryRetrievalPointers
    // or FatQueryRetrievalPointers and see why you failed.
    //
    
    if ( !NT_SUCCESS (Status) ) {
        KdPrint (("CRASHDUMP: ZwFsControlFile returnd %d\n", Status));
        ErrorToLog = IO_DUMP_POINTER_FAILURE;
        goto Cleanup;
    }
    
    //
    // This paging file is on the system boot partition, and
    // the retrieval pointers for the file were just successfully
    // queried.  Walk the MCB to size it, and then checksum it.
    //

    for (i = 0; mcb [i].QuadPart; i++) {
        NOTHING;
    }

    //
    // Write back fields of the IopDumpControlBlock.
    //
    
    IopDumpControlBlock->FileDescriptorArray = mcb;
    IopDumpControlBlock->FileDescriptorSize = (i + 1) * sizeof (LARGE_INTEGER);
    IopDumpControlBlock->DumpFileSize = RequestedFileSize;
    IopDumpControlBlockChecksum = IopGetDumpControlBlockCheck ( IopDumpControlBlock );

    Status = STATUS_SUCCESS;

Cleanup:

    if (Status != STATUS_SUCCESS &&
        ErrorToLog != STATUS_SUCCESS ) {
        
        IopLogErrorEvent (0,
                          4,
                          STATUS_SUCCESS,
                          ErrorToLog,
                          0,
                          NULL,
                          0,
                          NULL
                          );
    }

    DeviceObject = NULL;
    
    if ( FileObject ) {
        ObDereferenceObject( FileObject );
        FileObject = NULL;
    }

    return Status;

}


VOID
IopFreeDCB(
    BOOLEAN FreeDCB
    )

/*++

Routine Description:

    Free dump control block storage.

Arguments:

    FreeDCB - Implictly free storage for the dump control block.

Return Value:

    None

--*/
{
    PDUMP_CONTROL_BLOCK dcb;
    NTSTATUS            dwStatus;
    BOOLEAN             ShouldFreeDCB;

    dcb = IopDumpControlBlock;


    if (dcb) {

        if (dcb->MemoryDescriptor) {
             ExFreePool (dcb->MemoryDescriptor);
             dcb->MemoryDescriptor      = NULL;
        }

        if (dcb->HeaderPage) {
            ExFreePool (dcb->HeaderPage);
            dcb->HeaderPage             = NULL;
        }

        if (dcb->FileDescriptorArray) {
            ExFreePool (dcb->FileDescriptorArray);
            dcb->FileDescriptorArray    = NULL;
        }

        if (dcb->DumpStack) {
            IoFreeDumpStack (dcb->DumpStack);
            dcb->DumpStack              = NULL;
        }

        ShouldFreeDCB = FreeDCB | !( dcb->Flags & DCB_AUTO_REBOOT);

        //
        // Disable all options that require dump file access
        //
        
        dcb->Flags = dcb->Flags & DCB_AUTO_REBOOT;

        IoDebugPrint((2,"[IopFreeDCB] Should Free DCB = %s\n",(ShouldFreeDCB ? "TRUE" : "FALSE")));

        if (ShouldFreeDCB) {
            IopDumpControlBlock = NULL;
            ExFreePool( dcb );
            IoDebugPrint((2,"[IopFreeDCB]: Freeing Dump Control Block\n"));
        }
    }

    IoDebugPrint((2,"[IopFreeDCB]: CRASH DUMP DISABLED\n"));

}



NTSTATUS
IoSetCrashDumpState(
    IN SYSTEM_CRASH_STATE_INFORMATION *pDumpState
    )

/*++

Routine Description:

    Disable the current crash dump. Optionally, configure a new crash dump.

Arguments:

    pDumpState - If ValidDirectDump = TRUE  enable a new dump
                                      FALSE Disable crash dump

Return Value:

    STATUS_SUCCESS - The operation was successful

    W32ERROR - Otherwise

--*/
{
    NTSTATUS    Status;
    ULONG       crashConfigurationInProgress;

    //
    // Check the sentinal
    //

    crashConfigurationInProgress = InterlockedExchange(&IopCrashDumpStateChange,1);

    if (crashConfigurationInProgress) {
        return STATUS_SUCCESS;
    }

    //
    // If crash dumps disabled return success
    //

    if (!pDumpState->ValidDirectDump) {
        IopFreeDCB(FALSE);
        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    InterlockedExchange(&IopCrashDumpStateChange,0);

    return Status;
}


VOID
IopReadDumpRegistry(
    OUT PULONG dumpControl,
    OUT PULONG numberOfHeaderPages,
    OUT PULONG autoReboot,
    OUT PULONG dumpFileSize
    )
/*++

Routine Description:

    This routine reads the dump parameters from the registry.

Arguments:

    dumpControl - Supplies a pointer to the dumpControl flags to set.

Return Value:

    None.

--*/

{
    HANDLE                      keyHandle;
    HANDLE                      crashHandle;
    LOGICAL                     crashHandleOpened;
    UNICODE_STRING              keyName;
    NTSTATUS                    status;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    PDUMP_CONTROL_BLOCK         dcb;
    ULONG                       handleValue;

    *dumpControl = 0;
    *autoReboot = 0;
    *dumpFileSize = 0;

    *numberOfHeaderPages = 1;       // Dump header

    //
    // Begin by opening the path to the control for dumping memory.  Note
    // that if it does not exist, then no dumps will occur.
    //

    crashHandleOpened = FALSE;

    RtlInitUnicodeString( &keyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control" );

    status = IopOpenRegistryKey( &keyHandle,
                                 (HANDLE) NULL,
                                 &keyName,
                                 KEY_READ,
                                 FALSE );

    if (!NT_SUCCESS( status )) {
        return;
    }

    RtlInitUnicodeString( &keyName, L"CrashControl" );
    status = IopOpenRegistryKey( &crashHandle,
                                 keyHandle,
                                 &keyName,
                                 KEY_READ,
                                 FALSE );

    NtClose( keyHandle );

    if (!NT_SUCCESS( status )) {
        return;
    }

    crashHandleOpened = TRUE;

    //
    // Now get the value of the crash control to determine whether or not
    // dumping is enabled.
    //

    status = IopGetRegistryValue( crashHandle,
                                  L"CrashDumpEnabled",
                                  &keyValueInformation );

    if (NT_SUCCESS (status)) {

        if (keyValueInformation->DataLength) {

            handleValue = * ((PULONG) ((PUCHAR) keyValueInformation + keyValueInformation->DataOffset));
            ExFreePool( keyValueInformation );

            if (handleValue) {

                *dumpControl |= DCB_DUMP_ENABLED;

                //
                // If handleValue equals summary dump or the amount of physical
                // memory is greater than 2GB, then enable summary dump
                //

                if ( handleValue == 3 ) {

                    *dumpControl |= DCB_TRIAGE_DUMP_ENABLED;
                    
                } else if ( handleValue == 4 ) {

                    *dumpControl |= ( DCB_TRIAGE_DUMP_ENABLED | DCB_TRIAGE_DUMP_ACT_UPON_ENABLED );
                    
                } else if ( ( handleValue == 2 ) ||
                     ( MmPhysicalMemoryBlock->NumberOfPages >= ( 0x80000000 >> PAGE_SHIFT ) ) ) {
                    
                    *dumpControl |= DCB_SUMMARY_DUMP_ENABLED;

                    IoDebugPrint((2,"IopInitializeDCB: SUMMARY DUMP ENABLED \n"));

                    //
                    // Allocate enough storage for the dump header, summary
                    // dump header and bitmap.
                    //

                    *numberOfHeaderPages = BYTES_TO_PAGES(
                                            PAGE_SIZE +
                                            ( ( MmPhysicalMemoryBlock->NumberOfPages >> 5) << 2 ) +
                                            sizeof(SUMMARY_DUMP_HEADER)
                                            );

                    IoDebugPrint((2,"IopInitializeDCB: NumberOfHeader Pages = %x\n",numberOfHeaderPages));

                }
            }
        }
    }

    status = IopGetRegistryValue( crashHandle,
                                  L"LogEvent",
                                  &keyValueInformation );

    if (NT_SUCCESS( status )) {
         if (keyValueInformation->DataLength) {
            handleValue = * ((PULONG) ((PUCHAR) keyValueInformation + keyValueInformation->DataOffset));
            ExFreePool( keyValueInformation);
            if (handleValue) {
                *dumpControl |= DCB_SUMMARY_ENABLED;
            }
        }
    }

    status = IopGetRegistryValue( crashHandle,
                                  L"SendAlert",
                                  &keyValueInformation );

    if (NT_SUCCESS( status )) {
         if (keyValueInformation->DataLength) {
            handleValue = * ((PULONG) ((PUCHAR) keyValueInformation + keyValueInformation->DataOffset));
            ExFreePool( keyValueInformation);
            if (handleValue) {
                *dumpControl |= DCB_SUMMARY_ENABLED;
            }
        }
    }

    //
    // Now determine whether or not automatic reboot is enabled.
    //

    status = IopGetRegistryValue( crashHandle,
                                  L"AutoReboot",
                                  &keyValueInformation );


    if (NT_SUCCESS( status )) {
        if (keyValueInformation->DataLength) {
            *autoReboot = * ((PULONG) ((PUCHAR) keyValueInformation + keyValueInformation->DataOffset));
        }
        ExFreePool( keyValueInformation );
    }

    //
    // If we aren't auto rebooting or crashing then return now.
    //

    if (*dumpControl == 0 && *autoReboot == 0) {
        if (crashHandleOpened == TRUE) {
            NtClose( crashHandle );
        }
        return;
    }

    status = IopGetRegistryValue( crashHandle,
                                  L"DumpFileSize",
                                  &keyValueInformation );

    if (NT_SUCCESS( status )) {
        if (keyValueInformation->DataLength) {
            *dumpFileSize = * ((PULONG) ((PUCHAR) keyValueInformation + keyValueInformation->DataOffset));
        }

        ExFreePool( keyValueInformation );
    }
    return;
}


BOOLEAN
IopInitializeDCB(
    )
/*++

Routine Description:

    This routine initializes the Dump Control Block (DCB). It allocates the
    DCB and reads the crashdump parameters from the registry.

Arguments:


Return Value:

    The final function value is TRUE if everything worked, else FALSE.

--*/

{
    HANDLE                      keyHandle;
    HANDLE                      crashHandle;
    LOGICAL                     crashHandleOpened;
    UNICODE_STRING              keyName;
    NTSTATUS                    status;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    PDUMP_CONTROL_BLOCK         dcb;
    ULONG                       dumpControl;
    ULONG                       handleValue;
    ULONG                       autoReboot;
    PCHAR                       partitionName;
    ULONG                       dcbSize;
    LARGE_INTEGER               page;
    ULONG                       numberOfHeaderPages;
    ULONG                       dumpFileSize;

    //
    // Read all the registry default values first.
    //

    IopReadDumpRegistry ( &dumpControl,
                          &numberOfHeaderPages,
                          &autoReboot,
                          &dumpFileSize);

    //
    // If we aren't crashing or auto rebooting then return now.
    //

    if (dumpControl == 0 && autoReboot == 0) {

        //
        // At some point, we will conditionally on system size, type, etc,
        // set dump defaults like the below and skip over the return.
        //
        //    *dumpControl = (DCB_DUMP_ENABLED | DCB_TRIAGE_DUMP_ENABLED);
        //    *autoReboot = 1;
        //    *dumpFileSize = ?
        //

        return TRUE;
    }

    if (dumpControl & DCB_TRIAGE_DUMP_ENABLED) {
        dumpControl &= ~DCB_SUMMARY_ENABLED;
        dumpFileSize = TRIAGE_DUMP_SIZE;
    }

    //
    // Allocate and initialize the structures necessary to describe and control
    // the post-bugcheck code.
    //

    dcbSize = sizeof( DUMP_CONTROL_BLOCK ) + sizeof( MINIPORT_NODE );
    dcb = ExAllocatePoolWithTag( NonPagedPool, dcbSize, 'pmuD' );
    if (!dcb) {
        IoDebugPrint((1, "IopInitializeDCB: Not enough pool to allocate DCB0\n" ));
        IopLogErrorEvent(0,1,STATUS_SUCCESS,IO_DUMP_INITIALIZATION_FAILURE,0,NULL,0,NULL);
        return FALSE;
    }

    RtlZeroMemory( dcb, dcbSize );
    dcb->Type = IO_TYPE_DCB;
    dcb->Size = (USHORT) dcbSize;
    dcb->Flags = (UCHAR) (dumpControl | (autoReboot ? DCB_AUTO_REBOOT : 0));
    dcb->NumberProcessors = KeNumberProcessors;
    dcb->ProcessorArchitecture = KeProcessorArchitecture;
    dcb->MinorVersion = (USHORT) NtBuildNumber;
    dcb->MajorVersion = (USHORT) ((NtBuildNumber >> 28) & 0xfffffff);
    dcb->BuildNumber = CmNtCSDVersion;
    dcb->TriageDumpFlags = DEFAULT_TRIAGE_DUMP_FLAGS;

    dcb->DumpFileSize.QuadPart = dumpFileSize;

    //
    // Allocate memory descriptors.
    //

    dcb->MemoryDescriptorLength = sizeof( PHYSICAL_MEMORY_DESCRIPTOR ) - sizeof( PHYSICAL_MEMORY_RUN ) +
                                  (MmPhysicalMemoryBlock->NumberOfRuns * sizeof( PHYSICAL_MEMORY_RUN ));
    dcb->MemoryDescriptor = ExAllocatePoolWithTag (
                                    NonPagedPool,
                                    dcb->MemoryDescriptorLength,
                                    'pmuD'
                                    );
    if (!dcb->MemoryDescriptor) {
        ExFreePool (dcb);

        IoDebugPrint((1, "IopInitializeDCB: Not enough pool to allocate DCB1\n" ));
        IopLogErrorEvent(0,1,STATUS_SUCCESS,IO_DUMP_INITIALIZATION_FAILURE,0,NULL,0,NULL);
        return FALSE;
    }

    RtlCopyMemory (
        dcb->MemoryDescriptor,
        MmPhysicalMemoryBlock,
        dcb->MemoryDescriptorLength
        );

    //
    // Allocate header page.
    //

    dcb->HeaderSize = numberOfHeaderPages * PAGE_SIZE;
    dcb->HeaderPage = ExAllocatePoolWithTag( NonPagedPool, dcb->HeaderSize, 'pmuD' );

    if (!dcb->HeaderPage) {
        ExFreePool (dcb->MemoryDescriptor);
        ExFreePool (dcb);
        IoDebugPrint((1, "IopInitializeDCB: Not enough pool to allocate DCB2\n" ));
        IopLogErrorEvent(0,1,STATUS_SUCCESS,IO_DUMP_INITIALIZATION_FAILURE,0,NULL,0,NULL);
        return FALSE;
    }
    page = MmGetPhysicalAddress( dcb->HeaderPage );
    dcb->HeaderPfn = (ULONG)(page.QuadPart >> PAGE_SHIFT);


    //
    // Allocate the triage-dump buffer.
    //
    
    if (dumpControl & DCB_TRIAGE_DUMP_ENABLED) {
    
        dcb->TriageDumpBuffer = ExAllocatePoolWithTag ( NonPagedPool,
                                                      dumpFileSize,
                                                      'pmuD'
                                                      );

        if (!dcb->TriageDumpBuffer) {
            ExFreePool (dcb->HeaderPage);
            ExFreePool (dcb->MemoryDescriptor);
            ExFreePool (dcb);
            IoDebugPrint((1, "IopInitializeDCB: Not enough pool to allocate DCB3\n" ));
            IopLogErrorEvent(0,1,STATUS_SUCCESS,IO_DUMP_INITIALIZATION_FAILURE,0,NULL,0,NULL);
            return FALSE;
        }

        dcb->TriageDumpBufferSize = dumpFileSize;
    }
    
    IopDumpControlBlock = dcb;

    return TRUE;
}


BOOLEAN
IopConfigureCrashDump(
    IN HANDLE hPageFile
    )
/*++

Routine Description:

    This routine configures the system for crash dump. The following things
    are done:
    
        1. Initialize the dump control block and init registry crashdump
           parameters.

        2. Configure either page or fast dump.

        3. Complete dump file initialization.

    This routine is called as each page file is created. A return value of
    TRUE tells the caller (i.e., NtCreatePagingFiles, IoPageFileCreated)
    that crash dump has been configured.


Arguments:

       hPageFile - Handle to the paging file

Return Value:

    TRUE - Configuration complete (or crash dump not enabled).

    FALSE - Error, retry PageFile is not on boot partition.

--*/
{
    ULONG           dwStatus;
    PFILE_OBJECT    fileObject;
    PDEVICE_OBJECT  deviceObject;

    //
    // Only Init DCB Once.
    //
    
    if (!IopDumpControlBlock) {
        if (!IopInitializeDCB()) {
            return TRUE;
        }
    }

    //
    // Return crash dump not enabled
    //
    if (!IopDumpControlBlock){
        return TRUE;
    }

    //
    //  Only autoreboot?
    //
    
    if ( !( IopDumpControlBlock->Flags & (DCB_DUMP_ENABLED | DCB_SUMMARY_ENABLED) ) ) {
        return TRUE;
    }

    //
    // Configure the paging file for crash dump.
    //

    IoDebugPrint((2,"[IoPageFileCreated]: Page file dump\n"));


    dwStatus = ObReferenceObjectByHandle(
                                        hPageFile,
                                        0,
                                        IoFileObjectType,
                                        KernelMode,
                                        (PVOID *) &fileObject,
                                        NULL
                                        );

    if (!NT_SUCCESS( dwStatus )) {
        IoDebugPrint((1,"[IoPageFileCreated]: ObReferenceObjectByHandle for Paging file failed status = %x\n",dwStatus));
        goto error_return;
    }

    //
    // Get a pointer to the device object for this file.  Note that it
    // cannot go away, since there is an open handle to it, so it is
    // OK to dereference it and then use it.
    //

    deviceObject = fileObject->DeviceObject;

    ObDereferenceObject( fileObject );

    //
    // If this device object does not represents the boot partition return
    // FALSE so MM will try again.
    //
    
    if ( ! (deviceObject->Flags & DO_SYSTEM_BOOT_PARTITION) ) {
        return FALSE;
    }

    //
    // Load paging file dump stack
    //
    
    dwStatus = IoGetDumpStack (L"dump_",
                               &IopDumpControlBlock->DumpStack,
                               DeviceUsageTypeDumpFile,
                               FALSE);

    if (!NT_SUCCESS(dwStatus)) {
        IoDebugPrint((1, "IoPageFileCreated: Could not load dump stack status = %x\n",dwStatus) );
        goto error_return;
    }

    IopDumpControlBlock->DumpStack->Init.CrashDump = TRUE;

    IopDumpControlBlock->DumpStack->Init.MemoryBlock = ExAllocatePoolWithTag (
                                                NonPagedPool,
                                                IO_DUMP_MEMORY_BLOCK_PAGES * PAGE_SIZE,
                                                'pmuD'
                                                );

    if (!IopDumpControlBlock->DumpStack->Init.MemoryBlock) {
        dwStatus = STATUS_NO_MEMORY;
        goto error_return;
    }


    //
    // Calculate the amount of space required for the dump
    //
    IopDumpControlBlock->DumpFileSize =IopCalculateRequiredDumpSpace(
                                                    IopDumpControlBlock->Flags,
                                                    IopDumpControlBlock->HeaderSize,
                                                    IopDumpControlBlock->MemoryDescriptor->NumberOfPages,
                                                    IopDumpControlBlock->MemoryDescriptor->NumberOfPages
                                                    );


    //
    // Complete dump initialization
    //

    dwStatus = IopCompleteDumpInitialization(hPageFile);

error_return:

    //
    // The BOOT partition paging file could not be configured.
    //   1. Log an error message
    //   2. Return TRUE so that MM does not try again
    //

    if (!NT_SUCCESS(dwStatus)) {
        IoDebugPrint((1,"IopPageFileCreated: Page File dump init FAILED status = %x\n",dwStatus));

        IopLogErrorEvent(0,3,STATUS_SUCCESS,IO_DUMP_PAGE_CONFIG_FAILED,0,NULL,0,NULL);

        IopFreeDCB(FALSE);

    }

    return TRUE;
}


//
// Debugging routines.
//

#if DBG

ULONG IopDebugLevel = 0;
UCHAR IopBuffer[128];


VOID
IopDebugPrint(
    ULONG Level,
    PCHAR Format,
    ...
    )
{
    va_list ap;

    va_start(ap, Format);

    if (IopDebugLevel >= Level) {
        vsprintf(IopBuffer, Format, ap);
        DbgPrint(IopBuffer);
    }

    va_end(ap);
}

#endif //DBG
