/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/include/bl.h
 * PURPOSE:         Main Boot Library Header
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
*/

#ifndef _BL_H
#define _BL_H

/* INCLUDES ******************************************************************/

/* C Headers */
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>

/* NT Base Headers */
#include <ntifs.h>

/* NDK Headers */
#include <ntndk.h>

/* UEFI Headers */
#include <Uefi.h>
#include <DevicePath.h>
#include <LoadedImage.h>

VOID
EarlyPrint(_In_ PWCHAR Format, ...);

/* DEFINES *******************************************************************/

#define BL_APPLICATION_FLAG_CONVERTED_FROM_EFI          0x01

#define BL_APP_ENTRY_SIGNATURE                          "BTAPENT"

#define BOOT_APPLICATION_SIGNATURE_1                    'TOOB'
#define BOOT_APPLICATION_SIGNATURE_2                    ' PPA'

#define BOOT_MEMORY_TRANSLATION_TYPE_PHYSICAL           0
#define BOOT_MEMORY_TRANSLATION_TYPE_VIRTUAL            1

#define BOOT_APPLICATION_VERSION                        2
#define BL_MEMORY_DATA_VERSION                          1
#define BL_RETURN_ARGUMENTS_VERSION                     1
#define BL_FIRMWARE_DESCRIPTOR_VERSION                  2

#define BL_APPLICATION_ENTRY_FLAG_NO_GUID               0x01

#define BL_CONTEXT_PAGING_ON                            1
#define BL_CONTEXT_INTERRUPTS_ON                        2

#define BL_MM_FLAG_USE_FIRMWARE_FOR_MEMORY_MAP_BUFFERS  0x01
#define BL_MM_FLAG_REQUEST_COALESCING                   0x02

#define BL_MM_ADD_DESCRIPTOR_COALESCE_FLAG              0x01
#define BL_MM_ADD_DESCRIPTOR_TRUNCATE_FLAG              0x02
#define BL_MM_ADD_DESCRIPTOR_NEVER_COALESCE_FLAG        0x10
#define BL_MM_ADD_DESCRIPTOR_NEVER_TRUNCATE_FLAG        0x20
#define BL_MM_ADD_DESCRIPTOR_UPDATE_LIST_POINTER_FLAG   0x2000

#define BL_MM_DESCRIPTOR_REQUIRES_COALESCING_FLAG       0x2000000
#define BL_MM_DESCRIPTOR_REQUIRES_UPDATING_FLAG         0x4000000

#define BL_MM_REMOVE_VIRTUAL_REGION_FLAG                0x80000000

#define BL_LIBRARY_FLAG_REINITIALIZE                    0x02
#define BL_LIBRARY_FLAG_REINITIALIZE_ALL                0x04
#define BL_LIBRARY_FLAG_INITIALIZATION_COMPLETED        0x20

#define BL_MEMORY_CLASS_SHIFT                           28

/* ENUMERATIONS **************************************************************/

typedef enum _BL_MEMORY_DESCRIPTOR_TYPE
{
    BlMdPhysical,
    BlMdVirtual,
} BL_MEMORY_DESCRIPTOR_TYPE;

typedef enum _BL_TRANSLATION_TYPE
{
    BlNone,
    BlVirtual,
    BlPae,
    BlMax
} BL_TRANSLATION_TYPE;

typedef enum _BL_ARCH_MODE
{
    BlProtectedMode,
    BlRealMode
} BL_ARCH_MODE;

//
// Boot Device Types
//
typedef enum _BL_DEVICE_TYPE
{
    LocalDevice = 0,
    PartitionDevice = 2,
    UdpDevice = 4,
    HardDiskDevice = 6
} BL_DEVICE_TYPE;

//
// Local Device Types
//
typedef enum _BL_LOCAL_DEVICE_TYPE
{
    FloppyDevice = 1,
    CdRomDevice = 2,
    RamDiskDevice = 3,
} BL_LOCAL_DEVICE_TYPE;

//
// Partition types
//
typedef enum _BL_PARTITION_TYPE
{
    GptPartition,
    MbrPartition,
    RawPartition,
} BL_PARTITION_TYPE;

//
// File Path Types
//
typedef enum _BL_PATH_TYPE
{
    EfiPath = 4
} BL_PATH_TYPE;

//
// Classes of Memory
//
typedef enum _BL_MEMORY_CLASS
{
    BlLoaderClass = 0xD,
    BlApplicationClass,
    BlSystemClass
} BL_MEMORY_CLASS;

//
// Types of Memory
//
typedef enum _BL_MEMORY_TYPE
{
    //
    // Loader Memory
    //
    BlLoaderMemory = 0xD0000002,
    BlLoaderHeap = 0xD0000005,
    BlLoaderPageDirectory = 0xD0000006,
    BlLoaderReferencePage = 0xD0000007,
    BlLoaderRamDisk = 0xD0000008,
    BlLoaderData = 0xD000000A,
    BlLoaderSelfMap = 0xD000000F,

    //
    // Application Memory
    //
    BlApplicationData = 0xE0000004,

    //
    // System Memory
    //
    BlConventionalMemory = 0xF0000001,
    BlUnusableMemory = 0xF0000002,
    BlReservedMemory = 0xF0000003,
    BlEfiBootMemory = 0xF0000004,
    BlEfiRuntimeMemory = 0xF0000006,
    BlAcpiReclaimMemory = 0xF0000008,
    BlAcpiNvsMemory = 0xF0000009,
    BlDeviceIoMemory = 0xF000000A,
    BlDevicePortMemory = 0xF000000B,
    BlPalMemory = 0xF000000C,
} BL_MEMORY_TYPE;

typedef enum _BL_MEMORY_ATTR
{
    BlMemoryUncached = 1,
    BlMemoryWriteCombined = 2,
    BlMemoryWriteThrough = 4,
    BlMemoryWriteBack = 8,
    BlMemoryUncachedExported = 0x10,
    BlMemoryWriteProtected = 0x100,
    BlMemoryReadProtected = 0x200,
    BlMemoryExecuteProtected = 0x400,
    BlMemoryRuntime = 0x1000000
} BL_MEMORY_ATTR;

/* DATA STRUCTURES ***********************************************************/

typedef struct _BL_LIBRARY_PARAMETERS
{
    ULONG LibraryFlags;
    ULONG TranslationType;
    ULONG MinimumAllocationCount;
    ULONG MinimumHeapSize;
    ULONG HeapAllocationAttributes;
    PWCHAR ApplicationBaseDirectory;
    ULONG DescriptorCount;
    PWCHAR FontBaseDirectory;
} BL_LIBRARY_PARAMETERS, *PBL_LIBRARY_PARAMETERS;

/* This should eventually go into a more public header */
typedef struct _BOOT_APPLICATION_PARAMETER_BLOCK
{
    /* This header tells the library what image we're dealing with */
    ULONG Signature[2];
    ULONG Version;
    ULONG Size;
    ULONG ImageType;
    ULONG MemoryTranslationType;

    /* Where is the image located */
    ULONGLONG ImageBase;
    ULONG ImageSize;

    /* Offset to BL_MEMORY_DATA */
    ULONG MemoryDataOffset;

    /* Offset to BL_APPLICATION_ENTRY */
    ULONG AppEntryOffset;

    /* Offset to BL_DEVICE_DESCRPIPTOR */
    ULONG BootDeviceOffset;

    /* Offset to BL_FIRMWARE_PARAMETERS */
    ULONG FirmwareParametersOffset;

    /* Offset to BL_RETURN_ARGUMENTS */
    ULONG ReturnArgumentsOffset;
} BOOT_APPLICATION_PARAMETER_BLOCK, *PBOOT_APPLICATION_PARAMETER_BLOCK;

typedef struct _BL_MEMORY_DATA
{
    ULONG Version;
    ULONG MdListOffset;
    ULONG DescriptorCount;
    ULONG DescriptorSize;
    ULONG DescriptorOffset;
} BL_MEMORY_DATA, *PBL_MEMORY_DATA;

typedef struct _BL_FIRMWARE_DESCRIPTOR
{
    ULONG Version;
    ULONG Unknown;
    EFI_HANDLE ImageHandle;
    EFI_SYSTEM_TABLE *SystemTable;
} BL_FIRMWARE_DESCRIPTOR, *PBL_FIRMWARE_DESCRIPTOR;

typedef struct _BL_RETURN_ARGUMENTS
{
    ULONG Version;
    ULONG ReturnArgumentData[6];
} BL_RETURN_ARGUMENTS, *PBL_RETURN_ARGUMENTS;

typedef struct _BL_MEMORY_DESCRIPTOR
{
    LIST_ENTRY ListEntry;
    union
    {
        struct
        {
            ULONGLONG BasePage;
            ULONGLONG VirtualPage;
        };
        struct
        {
            ULONGLONG BaseAddress;
            ULONGLONG VirtualAddress;
        };
    };
    ULONGLONG PageCount;
    ULONG Flags;
    BL_MEMORY_TYPE Type;
} BL_MEMORY_DESCRIPTOR, *PBL_MEMORY_DESCRIPTOR;

typedef struct _BL_BCD_OPTION
{
    ULONG Type;
    ULONG DataOffset;
    ULONG DataSize;
    ULONG ListOffset;
    ULONG NextEntryOffset;
    ULONG Empty;
} BL_BCD_OPTION, *PBL_BCD_OPTION;

typedef struct _BL_APPLICATION_ENTRY
{
    CHAR Signature[8];
    ULONG Flags;
    GUID Guid;
    ULONG Unknown[4];
    BL_BCD_OPTION BcdData;
} BL_APPLICATION_ENTRY, *PBL_APPLICATION_ENTRY;

typedef struct _BL_HARDDISK_DEVICE
{
    ULONG PartitionType;
    union
    {
        struct
        {
            ULONG PartitionSignature;
        } Mbr;

        struct
        {
            GUID PartitionSignature;
        } Gpt;

        struct
        {
            ULONG DiskNumber;
        } Raw;
    };
} BL_HARDDISK_DEVICE;

typedef struct _BL_LOCAL_DEVICE
{
    ULONG Type;
    union
    {
        struct
        {
            ULONG DriveNumber;
        } FloppyDisk;

        BL_HARDDISK_DEVICE HardDisk;

        struct
        {
            PHYSICAL_ADDRESS ImageBase;
            LARGE_INTEGER ImageSize;
            ULONG ImageOffset;
        } RamDisk;
    };
} BL_LOCAL_DEVICE;

typedef struct _BL_DEVICE_DESCRIPTOR
{
    ULONG Size;
    ULONG Flags;
    DEVICE_TYPE DeviceType;
    ULONG Unknown;
    union
    {
        BL_LOCAL_DEVICE Local;

        struct
        {
            ULONG Unknown;
        } Remote;

        struct
        {
            union
            {
                ULONG PartitionNumber;
            } Mbr;

            union
            {
                GUID PartitionGuid;
            } Gpt;

            BL_LOCAL_DEVICE Disk;
        } Partition;
    };
} BL_DEVICE_DESCRIPTOR, *PBL_DEVICE_DESCRIPTOR;

typedef struct _BL_FILE_PATH_DESCRIPTOR
{
    ULONG Version;
    ULONG Length;
    ULONG PathType;
    UCHAR Path[ANYSIZE_ARRAY];
} BL_FILE_PATH_DESCRIPTOR, *PBL_FILE_PATH_DESCRIPTOR;

typedef struct _BL_WINDOWS_LOAD_OPTIONS
{
    CHAR Signature[8];
    ULONG Version;
    ULONG Length;
    ULONG OsPathOffset;
    WCHAR LoadOptions[ANYSIZE_ARRAY];
} BL_WINDOWS_LOAD_OPTIONS, *PBL_WINDOWS_LOAD_OPTIONS;

typedef struct _BL_ARCH_CONTEXT
{
    BL_ARCH_MODE Mode;
    BL_TRANSLATION_TYPE TranslationType;
    ULONG ContextFlags;
} BL_ARCH_CONTEXT, *PBL_ARCH_CONTEXT;

typedef struct _BL_MEMORY_DESCRIPTOR_LIST
{
    LIST_ENTRY ListHead;
    PLIST_ENTRY First;
    PLIST_ENTRY This;
    ULONG Type;
} BL_MEMORY_DESCRIPTOR_LIST, *PBL_MEMORY_DESCRIPTOR_LIST;

typedef struct _BL_ADDRESS_RANGE
{
    ULONGLONG Minimum;
    ULONGLONG Maximum;
} BL_ADDRESS_RANGE, *PBL_ADDRESS_RANGE;

/* INLINE ROUTINES ***********************************************************/

FORCEINLINE
VOID
BlSetupDefaultParameters (
    _Out_ PBL_LIBRARY_PARAMETERS LibraryParameters
    )
{
    BL_LIBRARY_PARAMETERS DefaultParameters =
    {
        0x20,
        BlVirtual,
        1024,
        2 * 1024 * 1024,
        0,
        NULL,
        0,
        NULL
    };

    /* Copy the defaults */
    RtlCopyMemory(LibraryParameters, &DefaultParameters, sizeof(*LibraryParameters));
}

FORCEINLINE
VOID
MmMdInitializeListHead (
    _In_ PBL_MEMORY_DESCRIPTOR_LIST List
    )
{
    /* Initialize the list */
    InitializeListHead(&List->ListHead);
    List->First = &List->ListHead;
    List->This = NULL;
}

/* INITIALIZATION ROUTINES ***************************************************/

NTSTATUS
BlInitializeLibrary(
    _In_ PBOOT_APPLICATION_PARAMETER_BLOCK BootAppParameters,
    _In_ PBL_LIBRARY_PARAMETERS LibraryParameters
    );

NTSTATUS
BlpArchInitialize (
    _In_ ULONG Phase
    );

NTSTATUS
BlpFwInitialize (
    _In_ ULONG Phase,
    _In_ PBL_FIRMWARE_DESCRIPTOR FirmwareParameters
    );

NTSTATUS
BlpMmInitialize (
    _In_ PBL_MEMORY_DATA MemoryData,
    _In_ BL_TRANSLATION_TYPE TranslationType,
    _In_ PBL_LIBRARY_PARAMETERS LibraryParameters
    );

/* UTILITY ROUTINES **********************************************************/

EFI_STATUS
EfiGetEfiStatusCode(
    _In_ NTSTATUS Status
    );

NTSTATUS
EfiGetNtStatusCode (
    _In_ EFI_STATUS EfiStatus
    );

/* BCD ROUTINES **************************************************************/

ULONG
BlGetBootOptionSize (
    _In_ PBL_BCD_OPTION BcdOption
    );

/* CONTEXT ROUTINES **********************************************************/

VOID
BlpArchSwitchContext (
    _In_ BL_ARCH_MODE NewMode
    );

/* MEMORY MANAGER ROUTINES ***************************************************/

NTSTATUS
MmBaInitialize (
    VOID
    );

NTSTATUS
MmPaInitialize (
    _In_ PBL_MEMORY_DATA MemoryData,
    _In_ ULONG MinimumPages
    );

NTSTATUS
MmArchInitialize (
    _In_ ULONG Phase,
    _In_ PBL_MEMORY_DATA MemoryData,
    _In_ BL_TRANSLATION_TYPE TranslationType,
    _In_ BL_TRANSLATION_TYPE LibraryTranslationType
    );

NTSTATUS
MmHaInitialize (
    _In_ ULONG HeapSize,
    _In_ ULONG HeapAttributes
    );

VOID
MmMdInitialize (
    _In_ ULONG Phase,
    _In_ PBL_LIBRARY_PARAMETERS LibraryParameters
    );

NTSTATUS
MmFwGetMemoryMap (
    _Out_ PBL_MEMORY_DESCRIPTOR_LIST MemoryMap,
    _In_ ULONG Flags
    );

VOID
MmMdFreeList(
    _In_ PBL_MEMORY_DESCRIPTOR_LIST MdList
    );

PBL_MEMORY_DESCRIPTOR
MmMdInitByteGranularDescriptor (
    _In_ ULONG Flags,
    _In_ BL_MEMORY_TYPE Type,
    _In_ ULONGLONG BasePage,
    _In_ ULONGLONG VirtualPage,
    _In_ ULONGLONG PageCount
    );

NTSTATUS
MmMdAddDescriptorToList (
    _In_ PBL_MEMORY_DESCRIPTOR_LIST MdList,
    _In_ PBL_MEMORY_DESCRIPTOR MemoryDescriptor,
    _In_ ULONG Flags
    );

NTSTATUS
MmMdRemoveRegionFromMdlEx (
    __in PBL_MEMORY_DESCRIPTOR_LIST MdList,
    __in ULONG Flags,
    __in ULONGLONG BasePage,
    __in ULONGLONG PageCount,
    __in PBL_MEMORY_DESCRIPTOR_LIST NewMdList
    );

NTSTATUS
MmPapAllocatePagesInRange (
    _Inout_ PULONG PhysicalAddress,
    _In_ BL_MEMORY_TYPE MemoryType,
    _In_ ULONGLONG Pages,
    _In_ ULONG Attributes,
    _In_ ULONG Alignment,
    _In_opt_ PBL_ADDRESS_RANGE Range,
    _In_ ULONG Type
    );

extern ULONG MmDescriptorCallTreeCount;
extern ULONG BlpApplicationFlags;
extern BL_LIBRARY_PARAMETERS BlpLibraryParameters;
extern BL_TRANSLATION_TYPE  MmTranslationType;
extern PBL_ARCH_CONTEXT CurrentExecutionContext;
extern PBL_DEVICE_DESCRIPTOR BlpBootDevice;

#endif
