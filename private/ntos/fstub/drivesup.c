/*++

Copyright (c) 1990-1998  Microsoft Corporation

Module Name:

    hanfnc.c

Abstract:

    default handlers for hal functions which don't get handlers
    installed by the hal

Author:

    Ken Reneris (kenr) 19-July-1994

Revision History:

    G.Chrysanthakopoulos (georgioc)    
    Added support for removable disk with a BPB,instead of a partition table.
    All changes in HalIoReadParitionTable. Started 01-June-1996

--*/

#include "ntos.h"
#include "zwapi.h"
#include "hal.h"
#include "ntdddisk.h"
#include "haldisp.h"
#include "ntddft.h"
#include "mountmgr.h"
#include "stdio.h"
#include <setupblk.h>

#include "drivesup.h"

//
// Debugging macros and flags
//

ULONG DrivesupDebug = 0;
ULONG DrivesupBreakIn = FALSE;

VOID
DrivesupDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );

#define DebugPrint(x) DrivesupDebugPrint x

//
// Macro definitions
//

#define GET_STARTING_SECTOR( p ) (                  \
        (ULONG) (p->StartingSectorLsb0) +           \
        (ULONG) (p->StartingSectorLsb1 << 8) +      \
        (ULONG) (p->StartingSectorMsb0 << 16) +     \
        (ULONG) (p->StartingSectorMsb1 << 24) )

#define GET_PARTITION_LENGTH( p ) (                 \
        (ULONG) (p->PartitionLengthLsb0) +          \
        (ULONG) (p->PartitionLengthLsb1 << 8) +     \
        (ULONG) (p->PartitionLengthMsb0 << 16) +    \
        (ULONG) (p->PartitionLengthMsb1 << 24) )

//
//  Structure for determing if an 0xaa55 marked sector has a BPB in it.
//

typedef struct _BOOT_SECTOR_INFO {
    UCHAR   JumpByte[1];
    UCHAR   Ignore1[2];
    UCHAR   OemData[8];
    UCHAR   BytesPerSector[2];
    UCHAR   Ignore2[6];
    UCHAR   NumberOfSectors[2];
    UCHAR   MediaByte[1];
    UCHAR   Ignore3[2];
    UCHAR   SectorsPerTrack[2];
    UCHAR   NumberOfHeads[2];
} BOOT_SECTOR_INFO, *PBOOT_SECTOR_INFO;

//
// Strings definitions
//

static PUCHAR DiskPartitionName = "\\Device\\Harddisk%d\\Partition%d";
static PUCHAR RegistryKeyName   = DISK_REGISTRY_KEY;

VOID
HalpCalculateChsValues(
    IN PLARGE_INTEGER PartitionOffset,
    IN PLARGE_INTEGER PartitionLength,
    IN CCHAR ShiftCount,
    IN ULONG SectorsPerTrack,
    IN ULONG NumberOfTracks,
    IN ULONG ConventionalCylinders,
    OUT PPARTITION_DESCRIPTOR PartitionDescriptor
    );

NTSTATUS
HalpQueryPartitionType(
    IN  PUNICODE_STRING             DeviceName,
    IN  PDRIVE_LAYOUT_INFORMATION   DriveLayout,
    OUT PULONG                      PartitionType
    );

NTSTATUS
HalpQueryDriveLayout(
    IN  PUNICODE_STRING             DeviceName,
    OUT PDRIVE_LAYOUT_INFORMATION*  DriveLayout
    );

VOID
FASTCALL
xHalGetPartialGeometry(
    IN PDEVICE_OBJECT DeviceObject,
    IN PULONG ConventionalCylinders,
    IN PLONGLONG DiskSize
    );

NTSTATUS
HalpGetFullGeometry(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDISK_GEOMETRY Geometry,
    OUT PULONGLONG RealSectorCount
    );

BOOLEAN
HalpIsValidPartitionEntry(
    PPARTITION_DESCRIPTOR Entry,
    ULONGLONG MaxOffset,
    ULONGLONG MaxSector
    );

NTSTATUS
HalpNextMountLetter(
    IN  PUNICODE_STRING DeviceName,
    OUT PUCHAR          DriveLetter
    );

UCHAR
HalpNextDriveLetter(
    IN  PUNICODE_STRING DeviceName,
    IN  PSTRING         NtDeviceName,
    OUT PUCHAR          NtSystemPath,
    IN  BOOLEAN         UseHardLinksIfNecessary
    );

VOID
HalpEnableAutomaticDriveLetterAssignment(
    );

NTSTATUS
HalpSetMountLetter(
    IN  PUNICODE_STRING DeviceName,
    IN  UCHAR           DriveLetter
    );

BOOLEAN
HalpIsOldStyleFloppy(
    IN  PUNICODE_STRING DeviceName
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, HalpCalculateChsValues)
#pragma alloc_text(PAGE, HalpQueryPartitionType)
#pragma alloc_text(PAGE, HalpQueryDriveLayout)
#pragma alloc_text(PAGE, HalpNextMountLetter)
#pragma alloc_text(PAGE, HalpNextDriveLetter)
#pragma alloc_text(PAGE, HalpEnableAutomaticDriveLetterAssignment)
#pragma alloc_text(PAGE, HalpSetMountLetter)
#pragma alloc_text(PAGE, xHalIoAssignDriveLetters)
#pragma alloc_text(PAGE, xHalIoReadPartitionTable)
#pragma alloc_text(PAGE, xHalIoSetPartitionInformation)
#pragma alloc_text(PAGE, xHalIoWritePartitionTable)
#pragma alloc_text(PAGE, HalpIsValidPartitionEntry)
#pragma alloc_text(PAGE, HalpGetFullGeometry)
#pragma alloc_text(PAGE, HalpIsOldStyleFloppy)
#endif



VOID
FASTCALL
xHalExamineMBR(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG MBRTypeIdentifier,
    OUT PVOID *Buffer
    )

/*++

Routine Description:

    Given a master boot record type (MBR - the zero'th sector on the disk),
    read the master boot record of a disk.  If the MBR is found to be of that
    type, allocate a structure whose layout is dependant upon that partition
    type, fill with the appropriate values, and return a pointer to that buffer
    in the output parameter.

    The best example for a use of this routine is to support Ontrack
    systems DiskManager software.  Ontrack software lays down a special
    partition describing the entire drive.  The special partition type
    (0x54) will be recognized and a couple of longwords of data will
    be passed back in a buffer for a disk driver to act upon.

Arguments:

    DeviceObject - The device object describing the entire drive.

    SectorSize - The minimum number of bytes that an IO operation can
                 fetch.

    MBRIndentifier - A value that will be searched for in the
                     in the MBR.  This routine will understand
                     the semantics implied by this value.

    Buffer - Pointer to a buffer that returns data according to the
             type of MBR searched for.  If the MBR is not of the
             type asked for, the buffer will not be allocated and this
             pointer will be NULL.  It is the responsibility of the
             caller of HalExamineMBR to deallocate the buffer.  The
             caller should deallocate the memory ASAP.

Return Value:

    None.

--*/

{


    LARGE_INTEGER partitionTableOffset;
    PUCHAR readBuffer = (PUCHAR) NULL;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIRP irp;
    PPARTITION_DESCRIPTOR partitionTableEntry;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG readSize;

    *Buffer = NULL;
    //
    // Determine the size of a read operation to ensure that at least 512
    // bytes are read.  This will guarantee that enough data is read to
    // include an entire partition table.  Note that this code assumes that
    // the actual sector size of the disk (if less than 512 bytes) is a
    // multiple of 2, a fairly reasonable assumption.
    //

    if (SectorSize >= 512) {
        readSize = SectorSize;
    } else {
        readSize = 512;
    }

    //
    // Start at sector 0 of the device.
    //

    partitionTableOffset = RtlConvertUlongToLargeInteger( 0 );

    //
    // Allocate a buffer that will hold the reads.
    //

    readBuffer = ExAllocatePoolWithTag(
                     NonPagedPoolCacheAligned,
                     PAGE_SIZE>readSize?PAGE_SIZE:readSize,
                     'btsF'
                     );

    if (readBuffer == NULL) {
        return;
    }

    //
    // Read record containing partition table.
    //
    // Create a notification event object to be used while waiting for
    // the read request to complete.
    //

    KeInitializeEvent( &event, NotificationEvent, FALSE );

    irp = IoBuildSynchronousFsdRequest( IRP_MJ_READ,
                                        DeviceObject,
                                        readBuffer,
                                        readSize,
                                        &partitionTableOffset,
                                        &event,
                                        &ioStatus );

    if (!irp) {
        ExFreePool(readBuffer);
        return;
    } else {
        PIO_STACK_LOCATION irpStack;
        irpStack = IoGetNextIrpStackLocation(irp);
        irpStack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
    }

    status = IoCallDriver( DeviceObject, irp );

    if (status == STATUS_PENDING) {
        (VOID) KeWaitForSingleObject( &event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER) NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS( status )) {
        ExFreePool(readBuffer);
        return;
    }

    //
    // Check for Boot Record signature.
    //

    if (((PUSHORT) readBuffer)[BOOT_SIGNATURE_OFFSET] != BOOT_RECORD_SIGNATURE) {
        ExFreePool(readBuffer);
        return;
    }

    //
    // Check for DM type partition.
    //

    partitionTableEntry = (PPARTITION_DESCRIPTOR) &(((PUSHORT) readBuffer)[PARTITION_TABLE_OFFSET]);

    if (partitionTableEntry->PartitionType != MBRTypeIdentifier) {

        //
        // The partition type isn't what the caller cares about.
        //
        ExFreePool(readBuffer);

    } else {

        if (partitionTableEntry->PartitionType == 0x54) {

            //
            // Rather than allocate a new piece of memory to return
            // the data - just use the memory allocated for the buffer.
            // We can assume the caller will delete this shortly.
            //

            ((PULONG)readBuffer)[0] = 63;
            *Buffer = readBuffer;

        } else if (partitionTableEntry->PartitionType == 0x55) {

            //
            // EzDrive Parititon.  Simply return the pointer to non-null
            // There is no skewing here.
            //

            *Buffer = readBuffer;

        } else {

            ASSERT(partitionTableEntry->PartitionType == 0x55);

        }

    }

}

VOID
FASTCALL
xHalGetPartialGeometry(
    IN PDEVICE_OBJECT DeviceObject,
    IN PULONG ConventionalCylinders,
    IN PLONGLONG DiskSize
    )

/*++

Routine Description:

    We need this routine to get the number of cylinders that the disk driver
    thinks is on the drive.  We will need this to calculate CHS values
    when we fill in the partition table entries.

Arguments:

    DeviceObject - The device object describing the entire drive.

    ConventionalCylinders - Number of cylinders on the drive.

Return Value:

    None.

--*/

{
    PIRP localIrp;
    PDISK_GEOMETRY diskGeometry;
    PIO_STATUS_BLOCK iosb;
    PKEVENT eventPtr;
    NTSTATUS status;

    *ConventionalCylinders = 0UL;
    *DiskSize = 0UL;

    diskGeometry = ExAllocatePoolWithTag(
                      NonPagedPool,
                      sizeof(DISK_GEOMETRY),
                      'btsF'
                      );

    if (!diskGeometry) {

        return;

    }

    iosb = ExAllocatePoolWithTag(
               NonPagedPool,
               sizeof(IO_STATUS_BLOCK),
               'btsF'
               );

    if (!iosb) {

        ExFreePool(diskGeometry);
        return;

    }

    eventPtr = ExAllocatePoolWithTag(
                   NonPagedPool,
                   sizeof(KEVENT),
                   'btsF'
                   );

    if (!eventPtr) {

        ExFreePool(iosb);
        ExFreePool(diskGeometry);
        return;

    }

    KeInitializeEvent(
        eventPtr,
        NotificationEvent,
        FALSE
        );

    localIrp = IoBuildDeviceIoControlRequest(
                   IOCTL_DISK_GET_DRIVE_GEOMETRY,
                   DeviceObject,
                   NULL,
                   0UL,
                   diskGeometry,
                   sizeof(DISK_GEOMETRY),
                   FALSE,
                   eventPtr,
                   iosb
                   );

    if (!localIrp) {

        ExFreePool(eventPtr);
        ExFreePool(iosb);
        ExFreePool(diskGeometry);
        return;

    }


    //
    // Call the lower level driver, wait for the opertion
    // to finish.
    //

    status = IoCallDriver(
                 DeviceObject,
                 localIrp
                 );

    if (status == STATUS_PENDING) {
        (VOID) KeWaitForSingleObject(
                   eventPtr,
                   Executive,
                   KernelMode,
                   FALSE,
                   (PLARGE_INTEGER) NULL
                   );
        status = iosb->Status;
    }

    if (NT_SUCCESS(status)) {

    //
    // The operation completed successfully.  Get the cylinder
    // count of the drive.
    //

        *ConventionalCylinders = diskGeometry->Cylinders.LowPart;

        //
        // If the count is less than 1024 we can pass that back.  Otherwise
        // send back the 1024
        //

        if (diskGeometry->Cylinders.QuadPart >= (LONGLONG)1024) {

            *ConventionalCylinders = 1024;

        }

        //
        // Calculate disk size from gemotry information
        //

        *DiskSize = diskGeometry->Cylinders.QuadPart *
                    diskGeometry->TracksPerCylinder *
                    diskGeometry->SectorsPerTrack *
                    diskGeometry->BytesPerSector;

    }

    ExFreePool(eventPtr);
    ExFreePool(iosb);
    ExFreePool(diskGeometry);
    return;

}


VOID
HalpCalculateChsValues(
    IN PLARGE_INTEGER PartitionOffset,
    IN PLARGE_INTEGER PartitionLength,
    IN CCHAR ShiftCount,
    IN ULONG SectorsPerTrack,
    IN ULONG NumberOfTracks,
    IN ULONG ConventionalCylinders,
    OUT PPARTITION_DESCRIPTOR PartitionDescriptor
    )

/*++

Routine Description:

    This routine will determine the cylinder, head, and sector (CHS) values
    that should be placed in a partition table entry, given the partition's
    location on the disk and its size.  The values calculated are packed into
    int13 format -- the high two bits of the sector byte contain bits 8 and 9
    of the 10 bit cylinder value, the low 6 bits of the sector byte contain
    the 6 bit sector value;  the cylinder byte contains the low 8 bits
    of the cylinder value; and the head byte contains the 8-bit head value.
    Both the start and end CHS values are calculated.

Arguments:

    PartitionOffset - Byte offset of the partition, relative to the entire
        physical disk.

    PartitionLength - Size in bytes of the partition.

    ShiftCount - Shift count to convert from byte counts to sector counts.

    SectorsPerTrack - Number of sectors in a track on the media on which
        the partition resides.

    NumberOfTracks - Number of tracks in a cylinder on the media on which
        the partition resides.

    ConventionalCylinders - The "normalized" disk cylinders.  We will never
        set the cylinders greater than this.

    PartitionDescriptor - Structure to be filled in with the start and
        end CHS values.  Other fields in the structure are not referenced
        or modified.

Return Value:

    None.

Note:

    The Cylinder and Head values are 0-based but the Sector value is 1-based.

    If the start or end cylinder overflows 10 bits (ie, > 1023), CHS values
    will be set to all 1's.

    No checking is done on the SectorsPerTrack and NumberOfTrack values.

--*/

{
    ULONG startSector, sectorCount, endSector;
    ULONG sectorsPerCylinder;
    ULONG remainder;
    ULONG startC, startH, startS, endC, endH, endS;
    LARGE_INTEGER tempInt;

    PAGED_CODE();

    //
    // Calculate the number of sectors in a cylinder.  This is the
    // number of heads multiplied by the number of sectors per track.
    //

    sectorsPerCylinder = SectorsPerTrack * NumberOfTracks;

    //
    // Convert byte offset/count to sector offset/count.
    //

    tempInt.QuadPart = PartitionOffset->QuadPart >> ShiftCount;
    startSector = tempInt.LowPart;

    tempInt.QuadPart = PartitionLength->QuadPart >> ShiftCount;
    sectorCount = tempInt.LowPart;

    endSector = startSector + sectorCount - 1;

    startC = startSector / sectorsPerCylinder;
    endC   = endSector   / sectorsPerCylinder;

    if (!ConventionalCylinders) {

        ConventionalCylinders = 1024;

    }

    //
    // Set these values so that win95 is happy.
    //

    if (startC >= ConventionalCylinders) {

        startC = ConventionalCylinders - 1;

    }

    if (endC >= ConventionalCylinders) {

        endC = ConventionalCylinders - 1;

    }

    //
    // Calculate the starting track and sector.
    //

    remainder = startSector % sectorsPerCylinder;
    startH = remainder / SectorsPerTrack;
    startS = remainder % SectorsPerTrack;

    //
    // Calculate the ending track and sector.
    //

    remainder = endSector % sectorsPerCylinder;
    endH = remainder / SectorsPerTrack;
    endS = remainder % SectorsPerTrack;

    //
    // Pack the result into the caller's structure.
    //

    // low 8 bits of the cylinder => C value

    PartitionDescriptor->StartingCylinderMsb = (UCHAR) startC;
    PartitionDescriptor->EndingCylinderMsb   = (UCHAR) endC;

    // 8 bits of head value => H value

    PartitionDescriptor->StartingTrack = (UCHAR) startH;
    PartitionDescriptor->EndingTrack   = (UCHAR) endH;

    // bits 8-9 of cylinder and 6 bits of the sector => S value

    PartitionDescriptor->StartingCylinderLsb = (UCHAR) (((startS + 1) & 0x3f)
                                                        | ((startC >> 2) & 0xc0));

    PartitionDescriptor->EndingCylinderLsb = (UCHAR) (((endS + 1) & 0x3f)
                                                        | ((endC >> 2) & 0xc0));
}


#define BOOTABLE_PARTITION  0
#define PRIMARY_PARTITION   1
#define LOGICAL_PARTITION   2
#define FT_PARTITION        3
#define OTHER_PARTITION     4

NTSTATUS
HalpQueryPartitionType(
    IN  PUNICODE_STRING             DeviceName,
    IN  PDRIVE_LAYOUT_INFORMATION   DriveLayout,
    OUT PULONG                      PartitionType
    )

{
    NTSTATUS                status;
    PFILE_OBJECT            fileObject;
    PDEVICE_OBJECT          deviceObject;
    KEVENT                  event;
    PIRP                    irp;
    PARTITION_INFORMATION   partInfo;
    IO_STATUS_BLOCK         ioStatus;
    ULONG                   i;

    status = IoGetDeviceObjectPointer(DeviceName,
                                      FILE_READ_ATTRIBUTES,
                                      &fileObject, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    deviceObject = IoGetAttachedDeviceReference(fileObject->DeviceObject);
    ObDereferenceObject(fileObject);

    if ((deviceObject->Characteristics&FILE_REMOVABLE_MEDIA) ||
        !DriveLayout) {

        ObDereferenceObject(deviceObject);
        *PartitionType = LOGICAL_PARTITION;
        return STATUS_SUCCESS;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_PARTITION_INFO,
                                        deviceObject, NULL, 0, &partInfo,
                                        sizeof(partInfo), FALSE, &event,
                                        &ioStatus);
    if (!irp) {
        ObDereferenceObject(deviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(deviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    ObDereferenceObject(deviceObject);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (!IsRecognizedPartition(partInfo.PartitionType)) {
        *PartitionType = OTHER_PARTITION;
        return STATUS_SUCCESS;
    }

    if (partInfo.PartitionType&0x80) {
        *PartitionType = FT_PARTITION;
        return STATUS_SUCCESS;
    }

    for (i = 0; i < 4; i++) {
        if (partInfo.StartingOffset.QuadPart ==
            DriveLayout->PartitionEntry[i].StartingOffset.QuadPart) {

            if (partInfo.BootIndicator) {
                *PartitionType = BOOTABLE_PARTITION;
            } else {
                *PartitionType = PRIMARY_PARTITION;
            }

            return STATUS_SUCCESS;
        }
    }

    *PartitionType = LOGICAL_PARTITION;
    return STATUS_SUCCESS;
}


NTSTATUS
HalpQueryDriveLayout(
    IN  PUNICODE_STRING             DeviceName,
    OUT PDRIVE_LAYOUT_INFORMATION*  DriveLayout
    )

{
    NTSTATUS        status;
    PFILE_OBJECT    fileObject;
    PDEVICE_OBJECT  deviceObject;
    KEVENT          event;
    PIRP            irp;
    DISK_GEOMETRY   geometry;
    IO_STATUS_BLOCK ioStatus;

    status = IoGetDeviceObjectPointer(DeviceName, FILE_READ_ATTRIBUTES,
                                      &fileObject, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    deviceObject = IoGetAttachedDeviceReference(fileObject->DeviceObject);
    ObDereferenceObject(fileObject);

    if (deviceObject->Characteristics&FILE_REMOVABLE_MEDIA) {
        ObDereferenceObject(deviceObject);
        return STATUS_NO_MEDIA;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                        deviceObject, NULL, 0, &geometry,
                                        sizeof(geometry), FALSE, &event,
                                        &ioStatus);
    if (!irp) {
        ObDereferenceObject(deviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(deviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        ObDereferenceObject(deviceObject);
        return status;
    }

    status = IoReadPartitionTable(deviceObject, geometry.BytesPerSector,
                                  FALSE, DriveLayout);

    ObDereferenceObject(deviceObject);

    return status;
}


NTSTATUS
HalpNextMountLetter(
    IN  PUNICODE_STRING DeviceName,
    OUT PUCHAR          DriveLetter
    )

/*++

Routine Description:

    This routine gives the device the next available drive letter.

Arguments:

    DeviceName  - Supplies the device name.

    DriveLetter - Returns the drive letter assigned or 0.

Return Value:

    NTSTATUS

--*/

{
    UNICODE_STRING                      name;
    PFILE_OBJECT                        fileObject;
    PDEVICE_OBJECT                      deviceObject;
    PMOUNTMGR_DRIVE_LETTER_TARGET       input;
    KEVENT                              event;
    PIRP                                irp;
    MOUNTMGR_DRIVE_LETTER_INFORMATION   output;
    IO_STATUS_BLOCK                     ioStatus;
    NTSTATUS                            status;

    RtlInitUnicodeString(&name, MOUNTMGR_DEVICE_NAME);
    status = IoGetDeviceObjectPointer(&name, FILE_READ_ATTRIBUTES, &fileObject,
                                      &deviceObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    input = ExAllocatePoolWithTag(PagedPool,
                                  (sizeof(MOUNTMGR_DRIVE_LETTER_TARGET) +
                                   DeviceName->Length),
                                  'btsF'
                                 );

    if (!input) {
        ObDereferenceObject(fileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    input->DeviceNameLength = DeviceName->Length;
    RtlCopyMemory(input->DeviceName, DeviceName->Buffer, DeviceName->Length);

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER,
                                        deviceObject, input,
                                        sizeof(MOUNTMGR_DRIVE_LETTER_TARGET) +
                                        DeviceName->Length, &output,
                                        sizeof(output), FALSE, &event,
                                        &ioStatus);
    if (!irp) {
        ExFreePool(input);
        ObDereferenceObject(fileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(deviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    ExFreePool(input);
    ObDereferenceObject(fileObject);

    *DriveLetter = output.CurrentDriveLetter;

    return status;
}

UCHAR
HalpNextDriveLetter(
    IN  PUNICODE_STRING DeviceName,
    IN  PSTRING         NtDeviceName,
    OUT PUCHAR          NtSystemPath,
    IN  BOOLEAN         UseHardLinksIfNecessary
    )

/*++

Routine Description:

    This routine gives the device the next available drive letter.

Arguments:

    DeviceName      - Supplies the device name.

    NtDeviceName    - Supplies the NT device name.

    NtSystemPath    - Supplies the NT system path.

Return Value:

    The drive letter assigned or 0.

--*/

{
    NTSTATUS        status;
    UCHAR           firstDriveLetter, driveLetter;
    WCHAR           name[40];
    UNICODE_STRING  symName;
    UNICODE_STRING  unicodeString, floppyPrefix, cdromPrefix;

    status = HalpNextMountLetter(DeviceName, &driveLetter);
    if (NT_SUCCESS(status)) {
        return driveLetter;
    }

    if (!NtDeviceName || !NtSystemPath) {
        return 0xFF;
    }

    RtlInitUnicodeString(&floppyPrefix, L"\\Device\\Floppy");
    RtlInitUnicodeString(&cdromPrefix, L"\\Device\\CdRom");
    if (RtlPrefixUnicodeString(&floppyPrefix, DeviceName, TRUE)) {
        firstDriveLetter = 'A';
    } else if (RtlPrefixUnicodeString(&cdromPrefix, DeviceName, TRUE)) {
        firstDriveLetter = 'D';
    } else {
        firstDriveLetter = 'C';
    }

    for (driveLetter = firstDriveLetter; driveLetter <= 'Z'; driveLetter++) {
        status = HalpSetMountLetter(DeviceName, driveLetter);
        if (NT_SUCCESS(status)) {
            RtlAnsiStringToUnicodeString(&unicodeString, NtDeviceName, TRUE);
            if (RtlEqualUnicodeString(&unicodeString, DeviceName, TRUE)) {
                NtSystemPath[0] = driveLetter;
            }
            RtlFreeUnicodeString(&unicodeString);
            return driveLetter;
        }
    }

    if (!UseHardLinksIfNecessary) {
        return 0;
    }

    for (driveLetter = firstDriveLetter; driveLetter <= 'Z'; driveLetter++) {
        swprintf(name, L"\\DosDevices\\%c:", driveLetter);
        RtlInitUnicodeString(&symName, name);
        status = IoCreateSymbolicLink(&symName, DeviceName);
        if (NT_SUCCESS(status)) {
            RtlAnsiStringToUnicodeString(&unicodeString, NtDeviceName, TRUE);
            if (RtlEqualUnicodeString(&unicodeString, DeviceName, TRUE)) {
                NtSystemPath[0] = driveLetter;
            }
            RtlFreeUnicodeString(&unicodeString);
            return driveLetter;
        }
    }

    return 0;
}


VOID
HalpEnableAutomaticDriveLetterAssignment(
    )

/*++

Routine Description:

    This routine enables automatic drive letter assignment by the mount
    point manager.

Arguments:

    None.

Return Value:

    None.

--*/

{
    UNICODE_STRING  name;
    PFILE_OBJECT    fileObject;
    PDEVICE_OBJECT  deviceObject;
    KEVENT          event;
    PIRP            irp;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS        status;

    RtlInitUnicodeString(&name, MOUNTMGR_DEVICE_NAME);
    status = IoGetDeviceObjectPointer(&name, FILE_READ_ATTRIBUTES, &fileObject,
                                      &deviceObject);
    if (!NT_SUCCESS(status)) {
        return;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTMGR_AUTO_DL_ASSIGNMENTS,
                                        deviceObject, NULL, 0, NULL, 0, FALSE,
                                        &event, &ioStatus);
    if (!irp) {
        ObDereferenceObject(fileObject);
        return;
    }

    status = IoCallDriver(deviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    ObDereferenceObject(fileObject);
}


NTSTATUS
HalpSetMountLetter(
    IN  PUNICODE_STRING DeviceName,
    IN  UCHAR           DriveLetter
    )

/*++

Routine Description:

    This routine sets the drive letter for the given device.

Arguments:

    DeviceName  - Supplies the device name.

    DriveLetter - Supplies the drive letter.

Return Value:

    NTSTATUS

--*/

{
    WCHAR                           dosBuffer[30];
    UNICODE_STRING                  dosName;
    ULONG                           createPointSize;
    PMOUNTMGR_CREATE_POINT_INPUT    createPoint;
    UNICODE_STRING                  name;
    NTSTATUS                        status;
    PFILE_OBJECT                    fileObject;
    PDEVICE_OBJECT                  deviceObject;
    KEVENT                          event;
    PIRP                            irp;
    IO_STATUS_BLOCK                 ioStatus;

    swprintf(dosBuffer, L"\\DosDevices\\%c:", DriveLetter);
    RtlInitUnicodeString(&dosName, dosBuffer);

    createPointSize = sizeof(MOUNTMGR_CREATE_POINT_INPUT) +
                      dosName.Length + DeviceName->Length;

    createPoint = (PMOUNTMGR_CREATE_POINT_INPUT)
                  ExAllocatePoolWithTag(PagedPool, createPointSize, 'btsF');
    if (!createPoint) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    createPoint->SymbolicLinkNameOffset = sizeof(MOUNTMGR_CREATE_POINT_INPUT);
    createPoint->SymbolicLinkNameLength = dosName.Length;
    createPoint->DeviceNameOffset = createPoint->SymbolicLinkNameOffset +
                                    createPoint->SymbolicLinkNameLength;
    createPoint->DeviceNameLength = DeviceName->Length;

    RtlCopyMemory((PCHAR) createPoint + createPoint->SymbolicLinkNameOffset,
                  dosName.Buffer, dosName.Length);
    RtlCopyMemory((PCHAR) createPoint + createPoint->DeviceNameOffset,
                  DeviceName->Buffer, DeviceName->Length);

    RtlInitUnicodeString(&name, MOUNTMGR_DEVICE_NAME);
    status = IoGetDeviceObjectPointer(&name, FILE_READ_ATTRIBUTES, &fileObject,
                                      &deviceObject);
    if (!NT_SUCCESS(status)) {
        ExFreePool(createPoint);
        return status;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTMGR_CREATE_POINT,
                                        deviceObject, createPoint,
                                        createPointSize, NULL, 0, FALSE,
                                        &event, &ioStatus);
    if (!irp) {
        ObDereferenceObject(fileObject);
        ExFreePool(createPoint);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(deviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    ObDereferenceObject(fileObject);
    ExFreePool(createPoint);

    return status;
}



BOOLEAN
HalpIsOldStyleFloppy(
    IN  PUNICODE_STRING DeviceName
    )

/*++

Routine Description:

    This routine determines whether or not the given device is an old style
    floppy.  That is, a floppy controlled by a traditional floppy controller.
    These floppies have precedent in the drive letter ordering.

Arguments:

    DeviceName  - Supplies the device name.

Return Value:

    FALSE   - The given device is not an old style floppy.

    TRUE    - The given device is an old style floppy.

--*/

{
    PFILE_OBJECT    fileObject;
    PDEVICE_OBJECT  deviceObject;
    KEVENT          event;
    PIRP            irp;
    MOUNTDEV_NAME   name;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS        status;
    
    PAGED_CODE();

    status = IoGetDeviceObjectPointer(DeviceName, FILE_READ_ATTRIBUTES,
                                      &fileObject, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }
    deviceObject = IoGetAttachedDeviceReference(fileObject->DeviceObject);
    ObDereferenceObject(fileObject);


    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                        deviceObject, NULL, 0, &name,
                                        sizeof(name), FALSE, &event,
                                        &ioStatus);
    if (!irp) {
        ObDereferenceObject(deviceObject);
        return FALSE;
    }

    status = IoCallDriver(deviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    ObDereferenceObject(deviceObject);

    if (status == STATUS_BUFFER_OVERFLOW) {
        return FALSE;
    }

    return TRUE;
}


VOID
FASTCALL
xHalIoAssignDriveLetters(
    IN struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
    IN PSTRING NtDeviceName,
    OUT PUCHAR NtSystemPath,
    OUT PSTRING NtSystemPathString
    )

/*++

Routine Description:

    This routine assigns DOS drive letters to eligible disk partitions
    and CDROM drives. It also maps the partition containing the NT
    boot path to \SystemRoot. In NT, objects are built for all partition
    types except 0 (unused) and 5 (extended). But drive letters are assigned
    only to recognized partition types (1, 4, 6, 7, e).

    Drive letter assignment is done in several stages:

        1) For each CdRom:
            Determine if sticky letters are assigned and reserve the letter.

        2) For each disk:
            Determine how many primary partitions and which is bootable.
            Determine which partitions already have 'sticky letters'
                and create their symbolic links.
            Create a bit map for each disk that idicates which partitions
                require default drive letter assignments.

        3) For each disk:
            Assign default drive letters for the bootable
                primary partition or the first nonbootable primary partition.

        4) For each disk:
            Assign default drive letters for the partitions in
                extended volumes.

        5) For each disk:
            Assign default drive letters for the remaining (ENHANCED)
                primary partitions.

        6) Assign A: and B: to the first two floppies in the system if they
            exist. Then assign remaining floppies next available drive letters.

        7) Assign drive letters to CdRoms (either sticky or default).

Arguments:

    LoaderBlock - pointer to a loader parameter block.

    NtDeviceName - pointer to the boot device name string used
            to resolve NtSystemPath.

Return Value:

    None.

--*/

{
    PUCHAR ntName;
    STRING ansiString;
    UNICODE_STRING unicodeString;
    PUCHAR ntPhysicalName;
    STRING ansiPhysicalString;
    UNICODE_STRING unicodePhysicalString;
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    PCONFIGURATION_INFORMATION configurationInformation;
    ULONG diskCount;
    ULONG floppyCount;
    HANDLE deviceHandle;
    IO_STATUS_BLOCK ioStatusBlock;
    ULONG diskNumber;
    ULONG i, j;
    UCHAR driveLetter;
    WCHAR deviceNameBuffer[50];
    UNICODE_STRING deviceName, floppyPrefix, cdromPrefix;
    PDRIVE_LAYOUT_INFORMATION layout;
    BOOLEAN bootable;
    ULONG partitionType;
    ULONG skip;
    ULONG diskCountIncrement;
    ULONG actualDiskCount = 0;

    PAGED_CODE();

    //
    // Get the count of devices from the registry.
    //

    configurationInformation = IoGetConfigurationInformation();

    diskCount = configurationInformation->DiskCount;
    floppyCount = configurationInformation->FloppyCount;

    //
    // Allocate general NT name buffer.
    //

    ntName = ExAllocatePoolWithTag( NonPagedPool, 128, 'btsF');

    ntPhysicalName = ExAllocatePoolWithTag( NonPagedPool, 64, 'btsF');

    if (ntName == NULL || ntPhysicalName == NULL) {

        KeBugCheck( ASSIGN_DRIVE_LETTERS_FAILED );

    }

    //
    // If we're doing a remote boot, set NtSystemPath appropriately.
    //

    if (IoRemoteBootClient) {

        PUCHAR p;
        PUCHAR q;

        //
        // If this is a remote boot setup boot, NtBootPathName is of the
        // form \<server>\<share>\setup\<install-directory>\<platform>.
        // We want the root of the X: drive to be the root of the install
        // directory.
        //
        // If this is a normal remote boot, NtBootPathName is of the form
        // \<server>\<share>\images\<machine>\winnt. We want the root of
        // the X: drive to be the root of the machine directory.
        //
        // Thus in either case, we need to remove all but the last element
        // of the path.
        //
        // Find the beginning of the last element of the path (including
        // the leading backslash).
        //

        p = strrchr( LoaderBlock->NtBootPathName, '\\' );   // find last separator
        q = NULL;
        if ( (p != NULL) && (*(p+1) == 0) ) {

            //
            // NtBootPathName ends with a backslash, so we need to back up
            // to the previous backslash.
            //

            q = p;
            *q = 0;
            p = strrchr( LoaderBlock->NtBootPathName, '\\' );   // find last separator
            *q = '\\';
        }
        if ( p == NULL ) {
            KeBugCheck( ASSIGN_DRIVE_LETTERS_FAILED );
        }

        //
        // Set NtSystemPath to X:\<last element of path>. Note that the symbolic
        // link for X: is created in io\ioinit.c\IopInitializeBootDrivers.
        //
        // Note that we use X: for the textmode setup phase of a remote
        // installation. But for a true remote boot, we use C:.
        //

#if defined(REMOTE_BOOT)
        if ((LoaderBlock->SetupLoaderBlock->Flags & (SETUPBLK_FLAGS_REMOTE_INSTALL |
                                                     SETUPBLK_FLAGS_SYSPREP_INSTALL)) == 0) {
            NtSystemPath[0] = 'C';
        } else
#endif
        {
            NtSystemPath[0] = 'X';
        }
        NtSystemPath[1] = ':';
        strcpy(&NtSystemPath[2], p );
        if ( q != NULL ) {
            NtSystemPath[strlen(NtSystemPath)-1] = '\0'; // remove trailing backslash
        }
        RtlInitString(NtSystemPathString, NtSystemPath);

    }

    //
    // For each disk ...
    //

    diskCountIncrement = 0;
    for (diskNumber = 0; diskNumber < diskCount; diskNumber++) {

        //
        // Create ANSI name string for physical disk.
        //

        sprintf( ntName, DiskPartitionName, diskNumber, 0 );

        //
        // Convert to unicode string.
        //

        RtlInitAnsiString( &ansiString, ntName );

        RtlAnsiStringToUnicodeString( &unicodeString, &ansiString, TRUE );

        InitializeObjectAttributes( &objectAttributes,
                                    &unicodeString,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL );

        //
        // Open device by name.
        //

        status = ZwOpenFile( &deviceHandle,
                             FILE_READ_DATA | SYNCHRONIZE,
                             &objectAttributes,
                             &ioStatusBlock,
                             FILE_SHARE_READ,
                             FILE_SYNCHRONOUS_IO_NONALERT );

        if (NT_SUCCESS( status )) {

            //
            // The device was successfully opened.  Generate a DOS device name
            // for the drive itself.
            //

            sprintf( ntPhysicalName, "\\DosDevices\\PhysicalDrive%d", diskNumber );

            RtlInitAnsiString( &ansiPhysicalString, ntPhysicalName );

            RtlAnsiStringToUnicodeString( &unicodePhysicalString, &ansiPhysicalString, TRUE );

            IoCreateSymbolicLink( &unicodePhysicalString, &unicodeString );

            RtlFreeUnicodeString( &unicodePhysicalString );

            ZwClose(deviceHandle);

            actualDiskCount = diskNumber + 1;
        }

        RtlFreeUnicodeString( &unicodeString );

        if (!NT_SUCCESS( status )) {

#if DBG
            DbgPrint( "IoAssignDriveLetters: Failed to open %s\n", ntName );
#endif // DBG

            //
            // This may be a sparse name space.  Try going farther but
            // not forever.
            //

            if (diskCountIncrement < 50) {
                diskCountIncrement++;
                diskCount++;
            }
        }

    } // end for diskNumber ...

    ExFreePool( ntName );
    ExFreePool( ntPhysicalName );

    diskCount -= diskCountIncrement;
    if (actualDiskCount > diskCount) {
        diskCount = actualDiskCount;
    }

    for (i = 0; i < diskCount; i++) {

        swprintf(deviceNameBuffer, L"\\Device\\Harddisk%d\\Partition0", i);
        RtlInitUnicodeString(&deviceName, deviceNameBuffer);

        status = HalpQueryDriveLayout(&deviceName, &layout);
        if (!NT_SUCCESS(status)) {
            layout = NULL;
        }

        bootable = FALSE;
        for (j = 1; ; j++) {

            swprintf(deviceNameBuffer, L"\\Device\\Harddisk%d\\Partition%d",
                     i, j);
            RtlInitUnicodeString(&deviceName, deviceNameBuffer);

            status = HalpQueryPartitionType(&deviceName, layout,
                                            &partitionType);
            if (!NT_SUCCESS(status)) {
                break;
            }

            if (partitionType != BOOTABLE_PARTITION) {
                continue;
            }

            bootable = TRUE;

            HalpNextDriveLetter(&deviceName, NtDeviceName, NtSystemPath, FALSE);
            break;
        }

        if (bootable) {
            if (layout) {
                ExFreePool(layout);
            }
            continue;
        }

        for (j = 1; ; j++) {

            swprintf(deviceNameBuffer, L"\\Device\\Harddisk%d\\Partition%d",
                     i, j);
            RtlInitUnicodeString(&deviceName, deviceNameBuffer);

            status = HalpQueryPartitionType(&deviceName, layout,
                                            &partitionType);
            if (!NT_SUCCESS(status)) {
                break;
            }

            if (partitionType != PRIMARY_PARTITION) {
                continue;
            }

            HalpNextDriveLetter(&deviceName, NtDeviceName, NtSystemPath, FALSE);
            break;
        }

        if (layout) {
            ExFreePool(layout);
        }
    }

    for (i = 0; i < diskCount; i++) {

        swprintf(deviceNameBuffer, L"\\Device\\Harddisk%d\\Partition0", i);
        RtlInitUnicodeString(&deviceName, deviceNameBuffer);

        status = HalpQueryDriveLayout(&deviceName, &layout);
        if (!NT_SUCCESS(status)) {
            layout = NULL;
        }

        for (j = 1; ; j++) {

            swprintf(deviceNameBuffer, L"\\Device\\Harddisk%d\\Partition%d",
                     i, j);
            RtlInitUnicodeString(&deviceName, deviceNameBuffer);

            status = HalpQueryPartitionType(&deviceName, layout,
                                            &partitionType);
            if (!NT_SUCCESS(status)) {
                break;
            }

            if (partitionType != LOGICAL_PARTITION) {
                continue;
            }

            HalpNextDriveLetter(&deviceName, NtDeviceName, NtSystemPath, FALSE);
        }

        if (layout) {
            ExFreePool(layout);
        }
    }

    for (i = 0; i < diskCount; i++) {

        swprintf(deviceNameBuffer, L"\\Device\\Harddisk%d\\Partition0", i);
        RtlInitUnicodeString(&deviceName, deviceNameBuffer);

        status = HalpQueryDriveLayout(&deviceName, &layout);
        if (!NT_SUCCESS(status)) {
            layout = NULL;
        }

        skip = 0;
        for (j = 1; ; j++) {

            swprintf(deviceNameBuffer, L"\\Device\\Harddisk%d\\Partition%d",
                     i, j);
            RtlInitUnicodeString(&deviceName, deviceNameBuffer);

            status = HalpQueryPartitionType(&deviceName, layout,
                                            &partitionType);
            if (!NT_SUCCESS(status)) {
                break;
            }

            if (partitionType == BOOTABLE_PARTITION) {
                skip = j;
            } else if (partitionType == PRIMARY_PARTITION) {
                if (!skip) {
                    skip = j;
                }
            }
        }

        for (j = 1; ; j++) {

            if (j == skip) {
                continue;
            }

            swprintf(deviceNameBuffer, L"\\Device\\Harddisk%d\\Partition%d",
                     i, j);
            RtlInitUnicodeString(&deviceName, deviceNameBuffer);

            status = HalpQueryPartitionType(&deviceName, layout,
                                            &partitionType);
            if (!NT_SUCCESS(status)) {
                break;
            }

            if (partitionType != PRIMARY_PARTITION &&
                partitionType != FT_PARTITION) {

                continue;
            }

            HalpNextDriveLetter(&deviceName, NtDeviceName, NtSystemPath, FALSE);
        }

        if (layout) {
            ExFreePool(layout);
        }
    }

    for (i = 0; i < floppyCount; i++) {

        swprintf(deviceNameBuffer, L"\\Device\\Floppy%d", i);
        RtlInitUnicodeString(&deviceName, deviceNameBuffer);

        if (!HalpIsOldStyleFloppy(&deviceName)) {
            continue;
        }

        HalpNextDriveLetter(&deviceName, NtDeviceName, NtSystemPath, TRUE);
    }

    for (i = 0; i < floppyCount; i++) {

        swprintf(deviceNameBuffer, L"\\Device\\Floppy%d", i);
        RtlInitUnicodeString(&deviceName, deviceNameBuffer);

        if (HalpIsOldStyleFloppy(&deviceName)) {
            continue;
        }

        HalpNextDriveLetter(&deviceName, NtDeviceName, NtSystemPath, TRUE);
    }

    for (i = 0; i < configurationInformation->CdRomCount; i++) {

        swprintf(deviceNameBuffer, L"\\Device\\CdRom%d", i);
        RtlInitUnicodeString(&deviceName, deviceNameBuffer);

        HalpNextDriveLetter(&deviceName, NtDeviceName, NtSystemPath, TRUE);
    }

    if (!IoRemoteBootClient) {
        RtlAnsiStringToUnicodeString(&unicodeString, NtDeviceName, TRUE);
        driveLetter = HalpNextDriveLetter(&unicodeString, NULL, NULL, TRUE);
        if (driveLetter) {
            if (driveLetter != 0xFF) {
                NtSystemPath[0] = driveLetter;
            }
        } else {
            RtlInitUnicodeString(&floppyPrefix, L"\\Device\\Floppy");
            RtlInitUnicodeString(&cdromPrefix, L"\\Device\\CdRom");
            if (RtlPrefixUnicodeString(&floppyPrefix, &unicodeString, TRUE)) {
                driveLetter = 'A';
            } else if (RtlPrefixUnicodeString(&cdromPrefix, &unicodeString, TRUE)) {
                driveLetter = 'D';
            } else {
                driveLetter = 'C';
            }
            for (; driveLetter <= 'Z'; driveLetter++) {
                status = HalpSetMountLetter(&unicodeString, driveLetter);
                if (NT_SUCCESS(status)) {
                    NtSystemPath[0] = driveLetter;
                }
            }
        }
        RtlFreeUnicodeString(&unicodeString);
    }

    HalpEnableAutomaticDriveLetterAssignment();

} // end IoAssignDriveLetters()


NTSTATUS
FASTCALL
xHalIoReadPartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN BOOLEAN ReturnRecognizedPartitions,
    OUT struct _DRIVE_LAYOUT_INFORMATION **PartitionBuffer
    )

/*++

Routine Description:

    This routine walks the disk reading the partition tables and creates
    an entry in the partition list buffer for each partition.

    The algorithm used by this routine is two-fold:

        1)  Read each partition table and for each valid, recognized
            partition found, to build a descriptor in a partition list.
            Extended partitions are located in order to find other
            partition tables, but no descriptors are built for these.
            The partition list is built in nonpaged pool that is allocated
            by this routine.  It is the caller's responsibility to free
            this pool after it has gathered the appropriate information
            from the list.

        2)  Read each partition table and for each and every entry, build
            a descriptor in the partition list.  Extended partitions are
            located to find each partition table on the disk, and entries
            are built for these as well.  The partition list is build in
            nonpaged pool that is allocated by this routine.  It is the
            caller's responsibility to free this pool after it has copied
            the information back to its caller.

    The first algorithm is used when the ReturnRecognizedPartitions flag
    is set.  This is used to determine how many partition device objects
    the device driver is to create, and where each lives on the drive.

    The second algorithm is used when the ReturnRecognizedPartitions flag
    is clear.  This is used to find all of the partition tables and their
    entries for a utility such as fdisk, that would like to revamp where
    the partitions live.

Arguments:

    DeviceObject - Pointer to device object for this disk.

    SectorSize - Sector size on the device.

    ReturnRecognizedPartitions - A flag indicated whether only recognized
        partition descriptors are to be returned, or whether all partition
        entries are to be returned.

    PartitionBuffer - Pointer to the pointer of the buffer in which the list
        of partition will be stored.

Return Value:

    The functional value is STATUS_SUCCESS if at least one sector table was
    read.

Notes:

    It is the responsibility of the caller to deallocate the partition list
    buffer allocated by this routine.

--*/

{
    ULONG partitionBufferSize = PARTITION_BUFFER_SIZE;
    PDRIVE_LAYOUT_INFORMATION newPartitionBuffer = NULL;

    LONG partitionTableCounter = -1;

    DISK_GEOMETRY diskGeometry;
    ULONGLONG endSector;
    ULONGLONG maxSector;
    ULONGLONG maxOffset;

    LARGE_INTEGER partitionTableOffset;
    LARGE_INTEGER volumeStartOffset;
    LARGE_INTEGER tempInt;
    BOOLEAN primaryPartitionTable;
    LONG partitionNumber;
    PUCHAR readBuffer = (PUCHAR) NULL;
    KEVENT event;

    IO_STATUS_BLOCK ioStatus;
    PIRP irp;
    PPARTITION_DESCRIPTOR partitionTableEntry;
    CCHAR partitionEntry;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG readSize;
    PPARTITION_INFORMATION partitionInfo;
    BOOLEAN foundEZHooker = FALSE;

    BOOLEAN mbrSignatureFound = FALSE;
    BOOLEAN emptyPartitionTable = TRUE;

    PAGED_CODE();

    //
    // Create the buffer that will be passed back to the driver containing
    // the list of partitions on the disk.
    //

    *PartitionBuffer = ExAllocatePoolWithTag( NonPagedPool,
                                              partitionBufferSize,
                                              'btsF' );

    if (*PartitionBuffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Determine the size of a read operation to ensure that at least 512
    // bytes are read.  This will guarantee that enough data is read to
    // include an entire partition table.  Note that this code assumes that
    // the actual sector size of the disk (if less than 512 bytes) is a
    // multiple of 2, a fairly reasonable assumption.
    //

    if (SectorSize >= 512) {
        readSize = SectorSize;
    } else {
        readSize = 512;
    }

    //
    // Look to see if this is an EZDrive Disk.  If it is then get the
    // real parititon table at 1.
    //

    {

        PVOID buff;

        HalExamineMBR(
            DeviceObject,
            readSize,
            (ULONG)0x55,
            &buff
            );

        if (buff) {

            foundEZHooker = TRUE;
            ExFreePool(buff);
            partitionTableOffset.QuadPart = 512;

        } else {

            partitionTableOffset.QuadPart = 0;

        }

    }

    //
    // Get the drive size so we can verify that the partition table is 
    // correct.
    //

    status = HalpGetFullGeometry(DeviceObject, 
                                 &diskGeometry, 
                                 &maxOffset);

    if(!NT_SUCCESS(status)) {
        ExFreePool(*PartitionBuffer);
        *PartitionBuffer = NULL;
        return status;
    }

    //
    // Partition offsets need to fit on the disk or we're not going to 
    // expose them.  Partition ends are generally very very sloppy so we 
    // need to allow some slop.  Adding in a cylinders worth isn't enough 
    // so now we'll assume that all partitions end within 2x of the real end
    // of the disk.
    //

    endSector = maxOffset;

    maxSector = maxOffset * 2;

    DebugPrint((2, "MaxOffset = %#I64x, maxSector = %#I64x\n", 
                maxOffset, maxSector));

    //
    // Indicate that the primary partition table is being read and
    // processed.
    //

    primaryPartitionTable = TRUE;

    //
    // The partitions in this volume have their start sector as 0.
    //

    volumeStartOffset.QuadPart = 0;

    //
    // Initialize the number of partitions in the list.
    //

    partitionNumber = -1;

    //
    // Allocate a buffer that will hold the reads.
    //

    readBuffer = ExAllocatePoolWithTag( NonPagedPoolCacheAligned,
                                        PAGE_SIZE,
                                        'btsF' );

    if (readBuffer == NULL) {
        ExFreePool( *PartitionBuffer );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Read each partition table, create an object for the partition(s)
    // it represents, and then if there is a link entry to another
    // partition table, repeat.
    //

    do {

        BOOLEAN tableIsValid;
        ULONG containerPartitionCount;

        tableIsValid = TRUE;

        //
        // Read record containing partition table.
        //
        // Create a notification event object to be used while waiting for
        // the read request to complete.
        //

        KeInitializeEvent( &event, NotificationEvent, FALSE );

        //
        // Zero out the buffer we're reading into.  In case we get back 
        // STATUS_NO_DATA_DETECTED we'll be prepared.
        //

        RtlZeroMemory(readBuffer, readSize);

        irp = IoBuildSynchronousFsdRequest( IRP_MJ_READ,
                                            DeviceObject,
                                            readBuffer,
                                            readSize,
                                            &partitionTableOffset,
                                            &event,
                                            &ioStatus );

        if (!irp) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        } else {
            PIO_STACK_LOCATION irpStack;
            irpStack = IoGetNextIrpStackLocation(irp);
            irpStack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
        }

        status = IoCallDriver( DeviceObject, irp );

        if (status == STATUS_PENDING) {
            (VOID) KeWaitForSingleObject( &event,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          (PLARGE_INTEGER) NULL);
            status = ioStatus.Status;
        }

        //
        // Special case - if we got a blank-check reading the sector then 
        // pretend it was just successful so we can deal with superfloppies
        // where noone bothered to write anything to the non-filesystem sectors
        //

        if(status == STATUS_NO_DATA_DETECTED) {
            status = STATUS_SUCCESS;
        }

        if (!NT_SUCCESS( status )) {
            break;
        }

        //
        // If EZDrive is hooking the MBR then we found the first partition table
        // in sector 1 rather than 0.  However that partition table is relative
        // to sector zero.  So, Even though we got it from one, reset the partition
        // offset to 0.
        //

        if (foundEZHooker && (partitionTableOffset.QuadPart == 512)) {

            partitionTableOffset.QuadPart = 0;

        }

        //
        // Check for Boot Record signature.
        //

        if (((PUSHORT) readBuffer)[BOOT_SIGNATURE_OFFSET] != BOOT_RECORD_SIGNATURE) {

            DebugPrint((1, "xHalIoReadPT: No 0xaa55 found in partition table %d\n",
                        partitionTableCounter + 1));

            break;

        } else {
            mbrSignatureFound = TRUE;
        }

        //
        // Copy NTFT disk signature to buffer
        //

        if (partitionTableOffset.QuadPart == 0) {
            (*PartitionBuffer)->Signature =  ((PULONG) readBuffer)[PARTITION_TABLE_OFFSET/2-1];
        }

        partitionTableEntry = (PPARTITION_DESCRIPTOR) &(((PUSHORT) readBuffer)[PARTITION_TABLE_OFFSET]);

        //
        // Keep count of partition tables in case we have an extended partition;
        //

        partitionTableCounter++;

        //
        // First create the objects corresponding to the entries in this
        // table that are not link entries or are unused.
        //

        DebugPrint((2, "Partition Table %d:\n", partitionTableCounter));

        for (partitionEntry = 1, containerPartitionCount = 0;
             partitionEntry <= NUM_PARTITION_TABLE_ENTRIES;
             partitionEntry++, partitionTableEntry++) {

            DebugPrint((2, "Partition Entry %d,%d: type %#x %s\n", 
                        partitionTableCounter,
                        partitionEntry,
                        partitionTableEntry->PartitionType,
                        (partitionTableEntry->ActiveFlag) ? "Active" : ""));

            DebugPrint((2, "\tOffset %#08lx for %#08lx Sectors\n", 
                        GET_STARTING_SECTOR(partitionTableEntry),
                        GET_PARTITION_LENGTH(partitionTableEntry)));
            //
            // Do a quick pass over the entry to see if this table is valid.
            // It's only fatal if the master partition table is invalid.
            //

            if((HalpIsValidPartitionEntry(partitionTableEntry, 
                                          maxOffset,
                                          maxSector) == FALSE) && 
               (partitionTableCounter == 0)) {

                ASSERT(DrivesupBreakIn == FALSE);
                DrivesupBreakIn = FALSE;
                tableIsValid = FALSE;
                break;

            } 
            //
            // Only one container partition is allowed per table - any more 
            // and it's invalid.
            //

            if(IsContainerPartition(partitionTableEntry->PartitionType)) {

                containerPartitionCount++;

                if(containerPartitionCount != 1) {

                    DebugPrint((1, "Multiple container partitions found in "
                                   "partition table %d\n - table is invalid\n",
                                partitionTableCounter));
                    tableIsValid = FALSE;
                    break;
                }

            }

            if(emptyPartitionTable) {

                if((GET_STARTING_SECTOR(partitionTableEntry) != 0) ||
                   (GET_PARTITION_LENGTH(partitionTableEntry) != 0)) {

                    //
                    // There's a valid, non-empty partition here. The table
                    // is not empty.
                    //

                    emptyPartitionTable = FALSE;
                }
            }

            //
            // If the partition entry is not used or not recognized, skip
            // it.  Note that this is only done if the caller wanted only
            // recognized partition descriptors returned.
            //

            if (ReturnRecognizedPartitions) {

                //
                // Check if partition type is 0 (unused) or 5/f (extended).
                // The definition of recognized partitions has broadened
                // to include any partition type other than 0 or 5/f.
                //

                if ((partitionTableEntry->PartitionType == PARTITION_ENTRY_UNUSED) ||
                    IsContainerPartition(partitionTableEntry->PartitionType)) {

                    continue;
                }
            }

            //
            // Bump up to the next partition entry.
            //

            partitionNumber++;

            if (((partitionNumber * sizeof( PARTITION_INFORMATION )) + 
                 sizeof( DRIVE_LAYOUT_INFORMATION )) > 
                (ULONG) partitionBufferSize) {

                //
                // The partition list is too small to contain all of the
                // entries, so create a buffer that is twice as large to
                // store the partition list and copy the old buffer into
                // the new one.
                //

                newPartitionBuffer = ExAllocatePoolWithTag( NonPagedPool,
                                                            partitionBufferSize << 1,
                                                            'btsF' );

                if (newPartitionBuffer == NULL) {
                    --partitionNumber;
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                RtlMoveMemory( newPartitionBuffer,
                               *PartitionBuffer,
                               partitionBufferSize );

                ExFreePool( *PartitionBuffer );

                //
                // Reassign the new buffer to the return parameter and
                // reset the size of the buffer.
                //

                *PartitionBuffer = newPartitionBuffer;
                partitionBufferSize <<= 1;
            }

            //
            // Describe this partition table entry in the partition list
            // entry being built for the driver.  This includes writing
            // the partition type, starting offset of the partition, and
            // the length of the partition.
            //

            partitionInfo = &(*PartitionBuffer)->PartitionEntry[partitionNumber];

            partitionInfo->PartitionType = partitionTableEntry->PartitionType;

            partitionInfo->RewritePartition = FALSE;

            if (partitionTableEntry->PartitionType != PARTITION_ENTRY_UNUSED) {
                LONGLONG startOffset;

                partitionInfo->BootIndicator =
                    partitionTableEntry->ActiveFlag & PARTITION_ACTIVE_FLAG ?
                        (BOOLEAN) TRUE : (BOOLEAN) FALSE;

                if (IsContainerPartition(partitionTableEntry->PartitionType)) {
                    partitionInfo->RecognizedPartition = FALSE;
                    startOffset = volumeStartOffset.QuadPart;
                } else {
                    partitionInfo->RecognizedPartition = TRUE;
                    startOffset = partitionTableOffset.QuadPart;
                }

                partitionInfo->StartingOffset.QuadPart = startOffset +
                    UInt32x32To64(GET_STARTING_SECTOR(partitionTableEntry),
                                  SectorSize);
                tempInt.QuadPart = (partitionInfo->StartingOffset.QuadPart -
                                   startOffset) / SectorSize;
                partitionInfo->HiddenSectors = tempInt.LowPart;

                partitionInfo->PartitionLength.QuadPart =
                    UInt32x32To64(GET_PARTITION_LENGTH(partitionTableEntry),
                                  SectorSize);

            } else {

                //
                // Partitions that are not used do not describe any part
                // of the disk.  These types are recorded in the partition
                // list buffer when the caller requested all of the entries
                // be returned.  Simply zero out the remaining fields in
                // the entry.
                //

                partitionInfo->BootIndicator = FALSE;
                partitionInfo->RecognizedPartition = FALSE;
                partitionInfo->StartingOffset.QuadPart = 0;
                partitionInfo->PartitionLength.QuadPart = 0;
                partitionInfo->HiddenSectors = 0;
            }

        }

        DebugPrint((2, "\n"));

        //
        // If an error occurred, leave the routine now.
        //

        if (!NT_SUCCESS( status )) {
            break;
        }

        if(tableIsValid == FALSE) {
            
            //
            // Invalidate this partition table and stop looking for new ones.
            // we'll build the partition list based on the ones we found 
            // previously.
            //

            partitionTableCounter--;
            break;
        }

        //
        // Now check to see if there are any link entries in this table,
        // and if so, set up the sector address of the next partition table.
        // There can only be one link entry in each partition table, and it
        // will point to the next table.
        //

        partitionTableEntry = (PPARTITION_DESCRIPTOR) &(((PUSHORT) readBuffer)[PARTITION_TABLE_OFFSET]);

        //
        // Assume that the link entry is empty.
        //

        partitionTableOffset.QuadPart = 0;

        for (partitionEntry = 1;
             partitionEntry <= NUM_PARTITION_TABLE_ENTRIES;
             partitionEntry++, partitionTableEntry++) {

            if (IsContainerPartition(partitionTableEntry->PartitionType)) {

                //
                // Obtain the address of the next partition table on the
                // disk.  This is the number of hidden sectors added to
                // the beginning of the extended partition (in the case of
                // logical drives), since all logical drives are relative
                // to the extended partition.  The VolumeStartSector will
                // be zero if this is the primary parition table.
                //

                partitionTableOffset.QuadPart = volumeStartOffset.QuadPart +
                    UInt32x32To64(GET_STARTING_SECTOR(partitionTableEntry),
                                  SectorSize);

                //
                // Set the VolumeStartSector to be the begining of the
                // second partition (extended partition) because all of
                // the offsets to the partition tables of the logical drives
                // are relative to this extended partition.
                //

                if (primaryPartitionTable) {
                    volumeStartOffset = partitionTableOffset;
                }

                //
                // Update the maximum sector to be the end of the container 
                // partition.
                //

                maxSector = GET_PARTITION_LENGTH(partitionTableEntry);

                DebugPrint((2, "MaxSector now = %#08lx\n", maxSector));

                //
                // There is only ever one link entry per partition table,
                // exit the loop once it has been found.
                //

                break;
            }
        }


        //
        // All the other partitions will be logical drives.
        //

        primaryPartitionTable = FALSE;


    } while (partitionTableOffset.HighPart | partitionTableOffset.LowPart);

    //
    // Detect super-floppy media attempt #1.
    // If the media is removable and has an 0xaa55 signature on it and 
    // is empty then check to see if we can recognize the BPB.  If we recognize
    // a jump-byte at the beginning of the media then it's a super floppy.  If
    // we don't then it's an unpartitioned disk.
    // 

    DebugPrint((4, "xHalIoReadPartitionTable: RM %d  PTC %d  MBR %d  EPT %d\n", 
                diskGeometry.MediaType,
                partitionTableCounter,
                mbrSignatureFound,
                emptyPartitionTable));

    if((diskGeometry.MediaType == RemovableMedia) &&
       (partitionTableCounter == 0) &&
       (mbrSignatureFound == TRUE) &&
       (emptyPartitionTable == TRUE)) {

        PBOOT_SECTOR_INFO bootSector = (PBOOT_SECTOR_INFO) readBuffer;

        if((bootSector->JumpByte[0] == 0xeb) ||
           (bootSector->JumpByte[0] == 0xe9)) {

            //
            // We've got a superfloppy of some sort.
            //

            DebugPrint((1, "xHalIoReadPartitionTable: Jump byte %#x found "
                           "along with empty partition table - disk is a "
                           "super floppy and has no valid MBR\n", 
                        bootSector->JumpByte));

            partitionTableCounter = -1;
        }
    }

    //
    // If the partition table count is still -1 then we didn't find any 
    // valid partition records.  In this case we'll build a partition list 
    // that contiains one partition spanning the entire disk.
    //

    if(partitionTableCounter == -1) {

        if((mbrSignatureFound == TRUE) ||
           (diskGeometry.MediaType == RemovableMedia)) {

            //
            // Either we found a signature but the partition layout was 
            // invalid (for all disks) or we didn't find a signature but this
            // is a removable disk.  Either of these two cases makes a 
            // superfloppy.
            //

            DebugPrint((1, "xHalIoReadPartitionTable: Drive %#p has no valid MBR. " 
                           "Make it into a super-floppy\n", DeviceObject));
    
            DebugPrint((1, "xHalIoReadPartitionTable: Drive has %#08lx sectors "
                           "and is %#016I64x bytes large\n", 
                        endSector, endSector * diskGeometry.BytesPerSector));
    
            if (endSector > 0) {
    
                partitionInfo = &(*PartitionBuffer)->PartitionEntry[0];
    
                partitionInfo->RewritePartition = FALSE;
                partitionInfo->RecognizedPartition = TRUE;
                partitionInfo->PartitionType = PARTITION_FAT_16;
                partitionInfo->BootIndicator = FALSE;
    
                partitionInfo->HiddenSectors = 0;
    
                partitionInfo->StartingOffset.QuadPart = 0;
    
                partitionInfo->PartitionLength.QuadPart = 
                    (endSector * diskGeometry.BytesPerSector);
    
                (*PartitionBuffer)->Signature = 1;
    
                partitionNumber = 0;
            }
        } else {

            //
            // We found no partitions.  Make sure the partition count is -1
            // so that we setup a zeroed-out partition table below.
            //

            partitionNumber = -1;
        }
    } 

    //
    // Fill in the first field in the PartitionBuffer. This field indicates how
    // many partition entries there are in the PartitionBuffer.
    //

    (*PartitionBuffer)->PartitionCount = ++partitionNumber;

    if (!partitionNumber) {

        //
        // Zero out disk signature.
        //

        (*PartitionBuffer)->Signature = 0;
    }

    //
    // Deallocate read buffer if it was allocated it.
    //

    if (readBuffer != NULL) {
        ExFreePool( readBuffer );
    }

    if (!NT_SUCCESS(status)) {
        ExFreePool(*PartitionBuffer);
        *PartitionBuffer = NULL;
    }
    return status;
}

NTSTATUS
FASTCALL
xHalIoSetPartitionInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG PartitionNumber,
    IN ULONG PartitionType
    )

/*++

Routine Description:

    This routine is invoked when a disk device driver is asked to set the
    partition type in a partition table entry via an I/O control code.  This
    control code is generally issued by the format utility just after it
    has formatted the partition.  The format utility performs the I/O control
    function on the partition and the driver passes the address of the base
    physical device object and the number of the partition associated with
    the device object that the format utility has open.  If this routine
    returns success, then the disk driver should updates its notion of the
    partition type for this partition in its device extension.

Arguments:

    DeviceObject - Pointer to the base physical device object for the device
        on which the partition type is to be set.

    SectorSize - Supplies the size of a sector on the disk in bytes.

    PartitionNumber - Specifies the partition number on the device whose
        partition type is to be changed.

    PartitionType - Specifies the new type for the partition.

Return Value:

    The function value is the final status of the operation.

Notes:

    This routine is synchronous.  Therefore, it MUST be invoked by the disk
    driver's dispatch routine, or by a disk driver's thread.  Likewise, all
    users, FSP threads, etc., must be prepared to enter a wait state when
    issuing the I/O control code to set the partition type for the device.

    Note also that this routine assumes that the partition number passed
    in by the disk driver actually exists since the driver itself supplies
    this parameter.

    Finally, note that this routine may NOT be invoked at APC_LEVEL.  It
    must be invoked at PASSIVE_LEVEL.  This is due to the fact that this
    routine uses a kernel event object to synchronize I/O completion on the
    device.  The event cannot be set to the signaled state without queueing
    the I/O system's special kernel APC routine for I/O completion and
    executing it.  (This rules is a bit esoteric since it only holds true
    if the device driver returns something other than STATUS_PENDING, which
    it will probably never do.)

--*/

{

#define GET_STARTING_SECTOR( p ) (                  \
        (ULONG) (p->StartingSectorLsb0) +           \
        (ULONG) (p->StartingSectorLsb1 << 8) +      \
        (ULONG) (p->StartingSectorMsb0 << 16) +     \
        (ULONG) (p->StartingSectorMsb1 << 24) )

    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS status;
    LARGE_INTEGER partitionTableOffset;
    LARGE_INTEGER volumeStartOffset;
    PUCHAR buffer = (PUCHAR) NULL;
    ULONG transferSize;
    ULONG partitionNumber;
    ULONG partitionEntry;
    PPARTITION_DESCRIPTOR partitionTableEntry;
    BOOLEAN primaryPartitionTable;
    BOOLEAN foundEZHooker = FALSE;

    PAGED_CODE();

    //
    // Begin by determining the size of the buffer required to read and write
    // the partition information to/from the disk.  This is done to ensure
    // that at least 512 bytes are read, thereby guaranteeing that enough data
    // is read to include an entire partition table.  Note that this code
    // assumes that the actual sector size of the disk (if less than 512
    // bytes) is a multiple of 2, a
    // fairly reasonable assumption.
    //

    if (SectorSize >= 512) {
        transferSize = SectorSize;
    } else {
        transferSize = 512;
    }


    //
    // Look to see if this is an EZDrive Disk.  If it is then get the
    // real parititon table at 1.
    //

    {

        PVOID buff;

        HalExamineMBR(
            DeviceObject,
            transferSize,
            (ULONG)0x55,
            &buff
            );

        if (buff) {

            foundEZHooker = TRUE;
            ExFreePool(buff);
            partitionTableOffset.QuadPart = 512;

        } else {

            partitionTableOffset.QuadPart = 0;

        }

    }


    //
    // The partitions in this primary partition have their start sector 0.
    //

    volumeStartOffset.QuadPart = 0;

    //
    // Indicate that the table being read and processed is the primary partition
    // table.
    //

    primaryPartitionTable = TRUE;

    //
    // Initialize the number of partitions found thus far.
    //

    partitionNumber = 0;

    //
    // Allocate a buffer that will hold the read/write data.
    //

    buffer = ExAllocatePoolWithTag( NonPagedPoolCacheAligned, PAGE_SIZE, 'btsF');
    if (buffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize a kernel event to use in synchronizing device requests
    // with I/O completion.
    //

    KeInitializeEvent( &event, NotificationEvent, FALSE );

    //
    // Read each partition table scanning for the partition table entry that
    // the caller wishes to modify.
    //

    do {

        //
        // Read the record containing the partition table.
        //

        (VOID) KeResetEvent( &event );

        irp = IoBuildSynchronousFsdRequest( IRP_MJ_READ,
                                            DeviceObject,
                                            buffer,
                                            transferSize,
                                            &partitionTableOffset,
                                            &event,
                                            &ioStatus );

        if (!irp) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        } else {
            PIO_STACK_LOCATION irpStack;
            irpStack = IoGetNextIrpStackLocation(irp);
            irpStack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
        }

        status = IoCallDriver( DeviceObject, irp );

        if (status == STATUS_PENDING) {
            (VOID) KeWaitForSingleObject( &event,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          (PLARGE_INTEGER) NULL );
            status = ioStatus.Status;
        }

        if (!NT_SUCCESS( status )) {
            break;
        }

        //
        // If EZDrive is hooking the MBR then we found the first partition table
        // in sector 1 rather than 0.  However that partition table is relative
        // to sector zero.  So, Even though we got it from one, reset the partition
        // offset to 0.
        //

        if (foundEZHooker && (partitionTableOffset.QuadPart == 512)) {

            partitionTableOffset.QuadPart = 0;

        }

        //
        // Check for a valid Boot Record signature in the partition table
        // record.
        //

        if (((PUSHORT) buffer)[BOOT_SIGNATURE_OFFSET] != BOOT_RECORD_SIGNATURE) {
            status = STATUS_BAD_MASTER_BOOT_RECORD;
            break;
        }

        partitionTableEntry = (PPARTITION_DESCRIPTOR) &(((PUSHORT) buffer)[PARTITION_TABLE_OFFSET]);

        //
        // Scan the partition entries in this partition table to determine if
        // any of the entries are the desired entry.  Each entry in each
        // table must be scanned in the same order as in IoReadPartitionTable
        // so that the partition table entry cooresponding to the driver's
        // notion of the partition number can be located.
        //

        for (partitionEntry = 1;
            partitionEntry <= NUM_PARTITION_TABLE_ENTRIES;
            partitionEntry++, partitionTableEntry++) {


            //
            // If the partition entry is empty or for an extended, skip it.
            //

            if ((partitionTableEntry->PartitionType == PARTITION_ENTRY_UNUSED) ||
                IsContainerPartition(partitionTableEntry->PartitionType)) {
                continue;
            }

            //
            // A valid partition entry that is recognized has been located.
            // Bump the count and check to see if this entry is the desired
            // entry.
            //

            partitionNumber++;

            if (partitionNumber == PartitionNumber) {

                //
                // This is the desired partition that is to be changed.  Simply
                // overwrite the partition type and write the entire partition
                // buffer back out to the disk.
                //

                partitionTableEntry->PartitionType = (UCHAR) PartitionType;

                (VOID) KeResetEvent( &event );

                irp = IoBuildSynchronousFsdRequest( IRP_MJ_WRITE,
                                                    DeviceObject,
                                                    buffer,
                                                    transferSize,
                                                    &partitionTableOffset,
                                                    &event,
                                                    &ioStatus );

                if (!irp) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                } else {
                    PIO_STACK_LOCATION irpStack;
                    irpStack = IoGetNextIrpStackLocation(irp);
                    irpStack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
                }

                status = IoCallDriver( DeviceObject, irp );

                if (status == STATUS_PENDING) {
                    (VOID) KeWaitForSingleObject( &event,
                                                  Executive,
                                                  KernelMode,
                                                  FALSE,
                                                  (PLARGE_INTEGER) NULL );
                    status = ioStatus.Status;
                }

                break;
            }
        }

        //
        // If all of the entries in the current buffer were scanned and the
        // desired entry was not found, then continue.  Otherwise, leave the
        // routine.
        //

        if (partitionEntry <= NUM_PARTITION_TABLE_ENTRIES) {
            break;
        }

        //
        // Now scan the current buffer to locate an extended partition entry
        // in the table so that its partition information can be read.  There
        // can only be one extended partition entry in each partition table,
        // and it will point to the next table.
        //

        partitionTableEntry = (PPARTITION_DESCRIPTOR) &(((PUSHORT) buffer)[PARTITION_TABLE_OFFSET]);

        for (partitionEntry = 1;
            partitionEntry <= NUM_PARTITION_TABLE_ENTRIES;
            partitionEntry++, partitionTableEntry++) {

            if (IsContainerPartition(partitionTableEntry->PartitionType)) {

                //
                // Obtain the address of the next partition table on the disk.
                // This is the number of hidden sectors added to the beginning
                // of the extended partition (in the case of logical drives),
                // since all logical drives are relative to the extended
                // partition.  The starting offset of the volume will be zero
                // if this is the primary partition table.
                //

                partitionTableOffset.QuadPart = volumeStartOffset.QuadPart +
                    UInt32x32To64(GET_STARTING_SECTOR(partitionTableEntry),
                                  SectorSize);

                //
                // Set the starting offset of the volume to be the beginning of
                // the second partition (the extended partition) because all of
                // the offsets to the partition tables of the logical drives
                // are relative to this extended partition.
                //

                if (primaryPartitionTable) {
                    volumeStartOffset = partitionTableOffset;
                }

                break;
            }
        }

        //
        // Ensure that a partition entry was located that was an extended
        // partition, otherwise the desired partition will never be found.
        //

        if (partitionEntry > NUM_PARTITION_TABLE_ENTRIES) {
            status = STATUS_BAD_MASTER_BOOT_RECORD;
            break;
        }

        //
        // All the other partitions will be logical drives.
        //

        primaryPartitionTable = FALSE;

    } while (partitionNumber < PartitionNumber);

    //
    // If a data buffer was successfully allocated, deallocate it now.
    //

    if (buffer != NULL) {
        ExFreePool( buffer );
    }

    return status;
}

NTSTATUS
FASTCALL
xHalIoWritePartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG SectorsPerTrack,
    IN ULONG NumberOfHeads,
    IN struct _DRIVE_LAYOUT_INFORMATION *PartitionBuffer
    )

/*++

Routine Description:

    This routine walks the disk writing the partition tables from
    the entries in the partition list buffer for each partition.

    Applications that create and delete partitions should issue a
    IoReadPartitionTable call with the 'return recognized partitions'
    boolean set to false to get a full description of the system.

    Then the drive layout structure can be modified by the application to
    reflect the new configuration of the disk and then is written back
    to the disk using this routine.

Arguments:

    DeviceObject - Pointer to device object for this disk.

    SectorSize - Sector size on the device.

    SectorsPerTrack - Track size on the device.

    NumberOfHeads - Same as tracks per cylinder.

    PartitionBuffer - Pointer drive layout buffer.

Return Value:

    The functional value is STATUS_SUCCESS if all writes are completed
    without error.

--*/

{
typedef struct _PARTITION_TABLE {
    PARTITION_INFORMATION PartitionEntry[4];
} PARTITION_TABLE, *PPARTITION_TABLE;

typedef struct _DISK_LAYOUT {
    ULONG TableCount;
    ULONG Signature;
    PARTITION_TABLE PartitionTable[1];
} DISK_LAYOUT, *PDISK_LAYOUT;

typedef struct _PTE {
    UCHAR ActiveFlag;               // Bootable or not
    UCHAR StartingTrack;            // Not used
    USHORT StartingCylinder;        // Not used
    UCHAR PartitionType;            // 12 bit FAT, 16 bit FAT etc.
    UCHAR EndingTrack;              // Not used
    USHORT EndingCylinder;          // Not used
    ULONG StartingSector;           // Hidden sectors
    ULONG PartitionLength;          // Sectors in this partition
} PTE;
typedef PTE UNALIGNED *PPTE;

//
// This macro has the effect of Bit = log2(Data)
//

#define WHICH_BIT(Data, Bit) {                      \
    for (Bit = 0; Bit < 32; Bit++) {                \
        if ((Data >> Bit) == 1) {                   \
            break;                                  \
        }                                           \
    }                                               \
}

    ULONG writeSize;
    PUSHORT writeBuffer = NULL;
    PPTE partitionEntry;
    PPARTITION_TABLE partitionTable;
    CCHAR shiftCount;
    LARGE_INTEGER partitionTableOffset;
    LARGE_INTEGER nextRecordOffset;
    ULONG partitionTableCount;
    ULONG partitionEntryCount;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIRP irp;
    BOOLEAN rewritePartition = FALSE;
    NTSTATUS status = STATUS_SUCCESS;
    LARGE_INTEGER tempInt;
    BOOLEAN foundEZHooker = FALSE;
    ULONG conventionalCylinders;
    LONGLONG diskSize;

    BOOLEAN isSuperFloppy = FALSE;

    //
    // Cast to a structure that is easier to use.
    //

    PDISK_LAYOUT diskLayout = (PDISK_LAYOUT) PartitionBuffer;

    //
    // Ensure that no one is calling this function illegally.
    //

    PAGED_CODE();

    //
    // Determine the size of a write operation to ensure that at least 512
    // bytes are written.  This will guarantee that enough data is written to
    // include an entire partition table.  Note that this code assumes that
    // the actual sector size of the disk (if less than 512 bytes) is a
    // multiple of 2, a fairly reasonable assumption.
    //

    if (SectorSize >= 512) {
        writeSize = SectorSize;
    } else {
        writeSize = 512;
    }

    xHalGetPartialGeometry( DeviceObject,
                            &conventionalCylinders,
                            &diskSize );

    //
    // Look to see if this is an EZDrive Disk.  If it is then get the
    // real partititon table at 1.
    //

    {

        PVOID buff;

        HalExamineMBR(
            DeviceObject,
            writeSize,
            (ULONG)0x55,
            &buff
            );

        if (buff) {

            foundEZHooker = TRUE;
            ExFreePool(buff);
            partitionTableOffset.QuadPart = 512;

        } else {

            partitionTableOffset.QuadPart = 0;

        }

    }

    //
    // Initialize starting variables.
    //

    nextRecordOffset.QuadPart = 0;

    //
    // Calculate shift count for converting between byte and sector.
    //

    WHICH_BIT( SectorSize, shiftCount );

    //
    // Check to see if this device is partitioned (or is being partitioned) 
    // as a floppy.  Floppys have a single partititon with hidden sector count 
    // and partition offset equal to zero.  If the disk is being partitioned
    // like this then we need to be sure not to write an MBR signature or 
    // an NTFT signature to the media.
    //
    // NOTE: this is only to catch ourself when someone tries to write the 
    // existing partition table back to disk.  Any changes to the table will 
    // result in a real MBR being written out.
    //

    if(PartitionBuffer->PartitionCount == 1) {
       
        PPARTITION_INFORMATION partitionEntry = PartitionBuffer->PartitionEntry;

        if((partitionEntry->StartingOffset.QuadPart == 0) &&
           (partitionEntry->HiddenSectors == 0)) {

            isSuperFloppy = TRUE;

            //
            // This would indeed appear to be an attempt to format a floppy.
            // Make sure the other parameters match the defaut values we 
            // provide in ReadParititonTable.  If they don't then fail 
            // the write operation.
            //

            if((partitionEntry->PartitionNumber != 0) ||
               (partitionEntry->PartitionType != PARTITION_FAT_16) ||
               (partitionEntry->BootIndicator == TRUE)) {

                return STATUS_INVALID_PARAMETER;
            }

            if(partitionEntry->RewritePartition == TRUE) {
                rewritePartition = TRUE;
            }

            foundEZHooker = FALSE;
        }
    }

    //
    // Convert partition count to partition table or boot sector count.
    //

    diskLayout->TableCount =
        (PartitionBuffer->PartitionCount +
        NUM_PARTITION_TABLE_ENTRIES - 1) /
        NUM_PARTITION_TABLE_ENTRIES;

    //
    // Allocate a buffer for the sector writes.
    //

    writeBuffer = ExAllocatePoolWithTag( NonPagedPoolCacheAligned, PAGE_SIZE, 'btsF');

    if (writeBuffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Point to the partition table entries in write buffer.
    //

    partitionEntry = (PPTE) &writeBuffer[PARTITION_TABLE_OFFSET];

    for (partitionTableCount = 0;
         partitionTableCount < diskLayout->TableCount;
         partitionTableCount++) {

        UCHAR   partitionType;

        //
        // the first partition table is in the mbr (physical sector 0).
        // other partition tables are in ebr's within the extended partition.
        //

        BOOLEAN mbr = (BOOLEAN) (!partitionTableCount);
        LARGE_INTEGER extendedPartitionOffset;

        //
        // Read the boot record that's already there into the write buffer
        // and save its boot code area if the signature is valid.  This way
        // we don't clobber any boot code that might be there already.
        //

        KeInitializeEvent( &event, NotificationEvent, FALSE );

        irp = IoBuildSynchronousFsdRequest( IRP_MJ_READ,
                                        DeviceObject,
                                        writeBuffer,
                                        writeSize,
                                        &partitionTableOffset,
                                        &event,
                                        &ioStatus );

        if (!irp) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        } else {
            PIO_STACK_LOCATION irpStack;
            irpStack = IoGetNextIrpStackLocation(irp);
            irpStack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
        }

        status = IoCallDriver( DeviceObject, irp );

        if (status == STATUS_PENDING) {
            (VOID) KeWaitForSingleObject( &event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER) NULL);
            status = ioStatus.Status;
        }

        if (!NT_SUCCESS( status )) {
            break;
        }

        //
        // If EZDrive is hooking the MBR then we found the first partition table
        // in sector 1 rather than 0.  However that partition table is relative
        // to sector zero.  So, Even though we got it from one, reset the partition
        // offset to 0.
        //

        if (foundEZHooker && (partitionTableOffset.QuadPart == 512)) {

            partitionTableOffset.QuadPart = 0;

        }

        if(isSuperFloppy == FALSE) {

            //
            // Write signature to last word of boot sector.
            //
    
            writeBuffer[BOOT_SIGNATURE_OFFSET] = BOOT_RECORD_SIGNATURE;
    
            //
            // Write NTFT disk signature if it changed and this is the MBR.
            //
    
            rewritePartition = FALSE;
            if (partitionTableOffset.QuadPart == 0) {
    
                if (((PULONG)writeBuffer)[PARTITION_TABLE_OFFSET/2-1] !=
                    PartitionBuffer->Signature) {
    
                    ((PULONG) writeBuffer)[PARTITION_TABLE_OFFSET/2-1] =
                        PartitionBuffer->Signature;
                    rewritePartition = TRUE;
                }
            }

            //
            // Get pointer to first partition table.
            //
    
            partitionTable = &diskLayout->PartitionTable[partitionTableCount];
    
            //
            // Walk table to determine whether this boot record has changed
            // and update partition table in write buffer in case it needs
            // to be written out to disk.
            //
    
            for (partitionEntryCount = 0;
                 partitionEntryCount < NUM_PARTITION_TABLE_ENTRIES;
                 partitionEntryCount++) {
    
                partitionType =
                        partitionTable->PartitionEntry[partitionEntryCount].PartitionType;
    
                //
                // If the rewrite ISN'T true then copy then just leave the data
                // alone that is in the on-disk table.
                //
    
                if (partitionTable->PartitionEntry[partitionEntryCount].RewritePartition) {
    
                    //
                    // This boot record needs to be written back to disk.
                    //
    
                    rewritePartition = TRUE;
    
                    //
                    // Copy partition type from user buffer to write buffer.
                    //
    
                    partitionEntry[partitionEntryCount].PartitionType =
                        partitionTable->PartitionEntry[partitionEntryCount].PartitionType;
    
                    //
                    // Copy the partition active flag.
                    //
    
                    partitionEntry[partitionEntryCount].ActiveFlag =
                        partitionTable->PartitionEntry[partitionEntryCount].BootIndicator ?
                        (UCHAR) PARTITION_ACTIVE_FLAG : (UCHAR) 0;
    
                    if (partitionType != PARTITION_ENTRY_UNUSED) {
    
                        LARGE_INTEGER sectorOffset;
    
                        //
                        // Calculate partition offset.
                        // If in the mbr or the entry is not a link entry, partition offset
                        // is sectors past last boot record.  Otherwise (not in the mbr and
                        // entry is a link entry), partition offset is sectors past start
                        // of extended partition.
                        //
    
                        if (mbr || !IsContainerPartition(partitionType)) {
                            tempInt.QuadPart = partitionTableOffset.QuadPart;
                        } else {
                            tempInt.QuadPart = extendedPartitionOffset.QuadPart;
                        }
    
                        sectorOffset.QuadPart =
                            partitionTable->PartitionEntry[partitionEntryCount].StartingOffset.QuadPart -
                            tempInt.QuadPart;
    
                        tempInt.QuadPart = sectorOffset.QuadPart >> shiftCount;
                        partitionEntry[partitionEntryCount].StartingSector = tempInt.LowPart;
    
                        //
                        // Calculate partition length.
                        //
    
                        tempInt.QuadPart = partitionTable->PartitionEntry[partitionEntryCount].PartitionLength.QuadPart >> shiftCount;
                        partitionEntry[partitionEntryCount].PartitionLength = tempInt.LowPart;
    
                        //
                        // Fill in CHS values
                        //
    
                        HalpCalculateChsValues(
                            &partitionTable->PartitionEntry[partitionEntryCount].StartingOffset,
                            &partitionTable->PartitionEntry[partitionEntryCount].PartitionLength,
                            shiftCount,
                            SectorsPerTrack,
                            NumberOfHeads,
                            conventionalCylinders,
                            (PPARTITION_DESCRIPTOR) &partitionEntry[partitionEntryCount]);
    
                    } else {
    
                        //
                        // Zero out partition entry fields in case an entry
                        // was deleted.
                        //
    
                        partitionEntry[partitionEntryCount].StartingSector = 0;
                        partitionEntry[partitionEntryCount].PartitionLength = 0;
                        partitionEntry[partitionEntryCount].StartingTrack = 0;
                        partitionEntry[partitionEntryCount].EndingTrack = 0;
                        partitionEntry[partitionEntryCount].StartingCylinder = 0;
                        partitionEntry[partitionEntryCount].EndingCylinder = 0;
                    }
    
                }
    
                if (IsContainerPartition(partitionType)) {
    
                    //
                    // Save next record offset.
                    //
    
                    nextRecordOffset =
                        partitionTable->PartitionEntry[partitionEntryCount].StartingOffset;
                }
    
            } // end for partitionEntryCount ...

        } else {

            //
            // If there's an 0xaa55 in the MBR signature, clear it out.  
            //

            //
            // BUGBUG - don't do this.
            //

            if(writeBuffer[BOOT_SIGNATURE_OFFSET] == BOOT_RECORD_SIGNATURE) {
                // writeBuffer[BOOT_SIGNATURE_OFFSET] += 0x1111;
            }
        }
    
        if (rewritePartition == TRUE) {

            rewritePartition = FALSE;

            //
            // Create a notification event object to be used while waiting for
            // the write request to complete.
            //

            KeInitializeEvent( &event, NotificationEvent, FALSE );

            if (foundEZHooker && (partitionTableOffset.QuadPart == 0)) {

                partitionTableOffset.QuadPart = 512;

            }
            irp = IoBuildSynchronousFsdRequest( IRP_MJ_WRITE,
                                            DeviceObject,
                                            writeBuffer,
                                            writeSize,
                                            &partitionTableOffset,
                                            &event,
                                            &ioStatus );

            if (!irp) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            } else {
                PIO_STACK_LOCATION irpStack;
                irpStack = IoGetNextIrpStackLocation(irp);
                irpStack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
            }

            status = IoCallDriver( DeviceObject, irp );

            if (status == STATUS_PENDING) {
                (VOID) KeWaitForSingleObject( &event,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          (PLARGE_INTEGER) NULL);
                status = ioStatus.Status;
            }

            if (!NT_SUCCESS( status )) {
                break;
            }


            if (foundEZHooker && (partitionTableOffset.QuadPart == 512)) {

                partitionTableOffset.QuadPart = 0;

            }

        } // end if (reWrite ...

        //
        // Update partitionTableOffset to next boot record offset
        //

        partitionTableOffset = nextRecordOffset;
        if(mbr) {
            extendedPartitionOffset = nextRecordOffset;
        }

    } // end for partitionTableCount ...

    //
    // Deallocate write buffer if it was allocated it.
    //

    if (writeBuffer != NULL) {
        ExFreePool( writeBuffer );
    }

    return status;
}


BOOLEAN
HalpIsValidPartitionEntry(
    PPARTITION_DESCRIPTOR Entry,
    ULONGLONG MaxOffset,
    ULONGLONG MaxSector
    )
{
    ULONGLONG endingSector;

    PAGED_CODE();

    if(Entry->PartitionType == PARTITION_ENTRY_UNUSED) {

        //
        // Unused partition entries are always valid.
        //

        return TRUE;

    }

    //
    // Container partition entries and normal partition entries are valid iff 
    // the partition they describe can possibly fit on the disk.  We add 
    // the base sector, the sector offset of the partition and the partition 
    // length.  If they exceed the sector count then this partition entry 
    // is considered invalid.
    //

    endingSector = GET_STARTING_SECTOR(Entry) + 
                   GET_PARTITION_LENGTH(Entry);

    if(endingSector > MaxSector) {

        DebugPrint((1, "HalpIsValidPartitionEntry: entry is invalid\n"));
        DebugPrint((1, "\tHalpIsValidPartitionEntry: offset %#08lx\n", 
                    GET_STARTING_SECTOR(Entry)));
        DebugPrint((1, "\tHalpIsValidPartitionEntry: length %#08lx\n", 
                    GET_PARTITION_LENGTH(Entry)));
        DebugPrint((1, "\tHalpIsValidPartitionEntry: end %#I64x\n", endingSector));
        DebugPrint((1, "\tHalpIsValidPartitionEntry: max %#I64x\n", MaxSector));

        return FALSE;
    } else if(GET_STARTING_SECTOR(Entry) > MaxOffset) {

        DebugPrint((1, "HalpIsValidPartitionEntry: entry is invalid\n"));
        DebugPrint((1, "\tHalpIsValidPartitionEntry: offset %#08lx\n", 
                    GET_STARTING_SECTOR(Entry)));
        DebugPrint((1, "\tHalpIsValidPartitionEntry: length %#08lx\n", 
                    GET_PARTITION_LENGTH(Entry)));
        DebugPrint((1, "\tHalpIsValidPartitionEntry: end %#I64x\n", endingSector));
        DebugPrint((1, "\tHalpIsValidPartitionEntry: maxOffset %#I64x\n", MaxOffset));
        
        return FALSE;
    }

    return TRUE;
}


NTSTATUS
FASTCALL
xHalIoClearPartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG SectorsPerTrack,
    IN ULONG NumberOfHeads
    )

/*++

Routine Description:

    This routine walks the disk writing the partition tables from
    the entries in the partition list buffer for each partition.

    Applications that create and delete partitions should issue a
    IoReadPartitionTable call with the 'return recognized partitions'
    boolean set to false to get a full description of the system.

    Then the drive layout structure can be modified by the application to
    reflect the new configuration of the disk and then is written back
    to the disk using this routine.

Arguments:

    DeviceObject - Pointer to device object for this disk.

    SectorSize - Sector size on the device.

    SectorsPerTrack - Track size on the device.

    NumberOfHeads - Same as tracks per cylinder.

Return Value:

    The functional value is STATUS_SUCCESS if all writes are completed
    without error.

--*/

{
typedef struct _PARTITION_TABLE {
    PARTITION_INFORMATION PartitionEntry[4];
} PARTITION_TABLE, *PPARTITION_TABLE;

typedef struct _DISK_LAYOUT {
    ULONG TableCount;
    ULONG Signature;
    PARTITION_TABLE PartitionTable[1];
} DISK_LAYOUT, *PDISK_LAYOUT;

typedef struct _PTE {
    UCHAR ActiveFlag;               // Bootable or not
    UCHAR StartingTrack;            // Not used
    USHORT StartingCylinder;        // Not used
    UCHAR PartitionType;            // 12 bit FAT, 16 bit FAT etc.
    UCHAR EndingTrack;              // Not used
    USHORT EndingCylinder;          // Not used
    ULONG StartingSector;           // Hidden sectors
    ULONG PartitionLength;          // Sectors in this partition
} PTE;
typedef PTE UNALIGNED *PPTE;

//
// This macro has the effect of Bit = log2(Data)
//

#define WHICH_BIT(Data, Bit) {                      \
    for (Bit = 0; Bit < 32; Bit++) {                \
        if ((Data >> Bit) == 1) {                   \
            break;                                  \
        }                                           \
    }                                               \
}

    ULONG writeSize;
    PUSHORT writeBuffer = NULL;
    PPARTITION_TABLE partitionTable;
    CCHAR shiftCount;
    LARGE_INTEGER partitionTableOffset;
    LARGE_INTEGER nextRecordOffset;
    ULONG partitionTableCount;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIRP irp;
    BOOLEAN rewritePartition = FALSE;
    NTSTATUS status = STATUS_SUCCESS;
    LARGE_INTEGER tempInt;
    BOOLEAN foundEZHooker = FALSE;
    ULONG conventionalCylinders;
    LONGLONG diskSize;

    BOOLEAN isSuperFloppy = FALSE;

    //
    // Ensure that no one is calling this function illegally.
    //

    PAGED_CODE();

    //
    // Determine the size of a write operation to ensure that at least 512
    // bytes are written.  This will guarantee that enough data is written to
    // include an entire partition table.  Note that this code assumes that
    // the actual sector size of the disk (if less than 512 bytes) is a
    // multiple of 2, a fairly reasonable assumption.
    //

    if (SectorSize >= 512) {
        writeSize = SectorSize;
    } else {
        writeSize = 512;
    }

    xHalGetPartialGeometry( DeviceObject,
                            &conventionalCylinders,
                            &diskSize );

    //
    // Look to see if this is an EZDrive Disk.  If it is then get the
    // real partititon table at 1.
    //

    {

        PVOID buff;

        HalExamineMBR(
            DeviceObject,
            writeSize,
            (ULONG)0x55,
            &buff
            );

        if (buff) {

            foundEZHooker = TRUE;
            ExFreePool(buff);
            partitionTableOffset.QuadPart = 512;

        } else {

            partitionTableOffset.QuadPart = 0;

        }

    }

    //
    // Calculate shift count for converting between byte and sector.
    //

    WHICH_BIT( SectorSize, shiftCount );

    //
    // Allocate a buffer for the sector writes.
    //

    writeBuffer = ExAllocatePoolWithTag( NonPagedPoolCacheAligned, PAGE_SIZE, 'btsF');

    if (writeBuffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Read the boot record that's already there into the write buffer
    // and save its boot code area if the signature is valid.  This way
    // we don't clobber any boot code that might be there already.
    //

    KeInitializeEvent( &event, SynchronizationEvent, FALSE );

    irp = IoBuildSynchronousFsdRequest( IRP_MJ_READ,
                                        DeviceObject,
                                        writeBuffer,
                                        writeSize,
                                        &partitionTableOffset,
                                        &event,
                                        &ioStatus );

    if (!irp) {
        ExFreePool(writeBuffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver( DeviceObject, irp );

    if (status == STATUS_PENDING) {
        (VOID) KeWaitForSingleObject( &event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  (PLARGE_INTEGER) NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS( status )) {
        ExFreePool(writeBuffer);
        return status;
    }

    //
    // If there's an 0xaa55 in the MBR signature, clear it out.  
    //

    if(writeBuffer[BOOT_SIGNATURE_OFFSET] == BOOT_RECORD_SIGNATURE) {
        writeBuffer[BOOT_SIGNATURE_OFFSET] += 0x1111;
    }
    
    irp = IoBuildSynchronousFsdRequest( IRP_MJ_WRITE,
                                    DeviceObject,
                                    writeBuffer,
                                    writeSize,
                                    &partitionTableOffset,
                                    &event,
                                    &ioStatus );

    if (!irp) {
        ExFreePool(writeBuffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    } else {
        PIO_STACK_LOCATION irpStack;
        irpStack = IoGetNextIrpStackLocation(irp);
        irpStack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
    }

    status = IoCallDriver( DeviceObject, irp );

    if (status == STATUS_PENDING) {
        (VOID) KeWaitForSingleObject( &event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  (PLARGE_INTEGER) NULL);
        status = ioStatus.Status;
    }

    //
    // Deallocate write buffer if it was allocated it.
    //

    ExFreePool( writeBuffer );

    return status;
}

// ENDEND


NTSTATUS
HalpGetFullGeometry(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDISK_GEOMETRY Geometry,
    OUT PULONGLONG RealSectorCount
    )

/*++

Routine Description:

    We need this routine to get the number of cylinders that the disk driver
    thinks is on the drive.  We will need this to calculate CHS values
    when we fill in the partition table entries.

Arguments:

    DeviceObject - The device object describing the entire drive.

    Geometry - The geometry of the drive
    
    RealSectorCount - the actual number of sectors reported by the drive (
                      this may be less than the size computed by the geometry)

Return Value:

    None.

--*/

{
    PIRP localIrp;
    IO_STATUS_BLOCK iosb;
    PKEVENT eventPtr;
    NTSTATUS status;

    PAGED_CODE();

    eventPtr = ExAllocatePoolWithTag(
                   NonPagedPool,
                   sizeof(KEVENT),
                   'btsF'
                   );

    if (!eventPtr) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent(
        eventPtr,
        NotificationEvent,
        FALSE
        );

    localIrp = IoBuildDeviceIoControlRequest(
                   IOCTL_DISK_GET_DRIVE_GEOMETRY,
                   DeviceObject,
                   NULL,
                   0UL,
                   Geometry,
                   sizeof(DISK_GEOMETRY),
                   FALSE,
                   eventPtr,
                   &iosb
                   );

    if (!localIrp) {
        ExFreePool(eventPtr);
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // Call the lower level driver, wait for the opertion
    // to finish.
    //

    status = IoCallDriver(
                 DeviceObject,
                 localIrp
                 );

    if (status == STATUS_PENDING) {
        (VOID) KeWaitForSingleObject(
                   eventPtr,
                   Executive,
                   KernelMode,
                   FALSE,
                   (PLARGE_INTEGER) NULL
                   );
        status = iosb.Status;
    }

    if(NT_SUCCESS(status)) {

        PARTITION_INFORMATION partitionInfo;
    
        localIrp = IoBuildDeviceIoControlRequest(
                       IOCTL_DISK_GET_PARTITION_INFO,
                       DeviceObject,
                       NULL,
                       0UL,
                       &partitionInfo,
                       sizeof(PARTITION_INFORMATION),
                       FALSE,
                       eventPtr,
                       &iosb
                       );
    
        if (!localIrp) {
            ExFreePool(eventPtr);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    
    
        //
        // Call the lower level driver, wait for the opertion
        // to finish.
        //
    
        status = IoCallDriver(
                     DeviceObject,
                     localIrp
                     );
    
        if (status == STATUS_PENDING) {
            (VOID) KeWaitForSingleObject(
                       eventPtr,
                       Executive,
                       KernelMode,
                       FALSE,
                       (PLARGE_INTEGER) NULL
                       );
            status = iosb.Status;
        }

        if(NT_SUCCESS(status)) {
            *RealSectorCount = (partitionInfo.PartitionLength.QuadPart / 
                                Geometry->BytesPerSector);
        }
    }

    ExFreePool(eventPtr);
    return status;
}


VOID
DrivesupDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )
/*++

Routine Description:

    Debug print for all SCSI drivers

Arguments:

    Debug print level between 0 and 3, with 3 being the most verbose.

Return Value:

    None

--*/

{
    UCHAR DrivesupBuffer[256];

    va_list ap;

    va_start(ap, DebugMessage);


    if (DebugPrintLevel <= DrivesupDebug) {

        _vsnprintf(DrivesupBuffer, 256, DebugMessage, ap);

        DbgPrint(DrivesupBuffer);
    }

    va_end(ap);

} // end DrivesupDebugPrint()
