/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Memory Management Functions (UEFI)
 * NOTE:        Reworked for robust GetMemoryMap handling and safer sizing.
 * COPYRIGHT:   Original: Copyright 2022 Justin Miller
 *              Rework:   2025 Ahmed ARIF arif193@gmail.com
 */

/* INCLUDES ******************************************************************/

#include <uefildr.h>
#include <debug.h>

DBG_DEFAULT_CHANNEL(WARNING);

/* HELPERS *******************************************************************/

#define NEXT_MEMORY_DESCRIPTOR(Descriptor, DescriptorSize) \
    ((EFI_MEMORY_DESCRIPTOR *)((char *)(Descriptor) + (DescriptorSize)))

/* A small, safe extra headroom (in descriptors) to absorb growth caused by
 * allocations we perform *while* fetching the map. */
#define MAP_SLACK_DESCRIPTORS 8

/* When the first probe cannot provide a size, seed to something reasonable. */
#define MAP_FALLBACK_BYTES    (16 * 1024)

/* When building the FreeLdr map we allow a small growth margin to avoid
 * reallocation if firmware adds a couple entries between calls (rare). */
#define FREELDR_EXTRA_DESCS   8

/* EXIT_BOOT_SERVICES stack scratch size (unchanged) */
#define EXIT_STACK_SIZE       0x1000

/* Max “virtual” list size we hand to AddMemoryDescriptor (unchanged semantic) */
#define UNUSED_MAX_DESCRIPTOR_COUNT 10000

/* EXTERNALS *****************************************************************/

extern ULONG LoaderPagesSpanned;
extern EFI_SYSTEM_TABLE *GlobalSystemTable;
extern EFI_HANDLE GlobalImageHandle;
extern REACTOS_INTERNAL_BGCONTEXT framebufferData;

/* Provided elsewhere */
extern char __ImageBase;

/* From your other unit */
extern ULONG
AddMemoryDescriptor(
    _Inout_ PFREELDR_MEMORY_DESCRIPTOR List,
    _In_    ULONG MaxCount,
    _In_    PFN_NUMBER BasePage,
    _In_    PFN_NUMBER PageCount,
    _In_    TYPE_OF_MEMORY MemoryType);

/* GLOBALS *******************************************************************/

EFI_MEMORY_DESCRIPTOR *EfiMemoryMap = NULL;
UINT32                 FreeldrDescCount = 0;

PVOID     OsLoaderBase;
SIZE_T    OsLoaderSize;
EFI_HANDLE PublicBootHandle;

static PVOID ExitStack;
static PVOID EndofExitStack;

/* Declared elsewhere */
void _exituefi(VOID);

/* FORWARD DECLS *************************************************************/

static VOID
PUEFI_LoadMemoryMap(
    _Out_ UINTN  *LocMapKey,
    _Out_ UINTN  *LocMapSize,
    _Out_ UINTN  *LocDescriptorSize,
    _Out_ UINT32 *LocDescriptorVersion);

static VOID
UefiSetMemory(
    _Inout_ PFREELDR_MEMORY_DESCRIPTOR MemoryMap,
    _In_    ULONG_PTR BaseAddress,
    _In_    PFN_COUNT SizeInPages,
    _In_    TYPE_OF_MEMORY MemoryType);

static TYPE_OF_MEMORY
UefiConvertToFreeldrDesc(_In_ EFI_MEMORY_TYPE EfiMemoryType);

/* IMPLEMENTATION ************************************************************/

/* Robust memory-map loader:
 * - Treats EFI_BUFFER_TOO_SMALL as the normal/expected probe result.
 * - Allocates with slack and retries until EFI_SUCCESS.
 * - Trims returned length to a whole number of descriptors. */
static
VOID
PUEFI_LoadMemoryMap(
    _Out_ UINTN  *LocMapKey,
    _Out_ UINTN  *LocMapSize,
    _Out_ UINTN  *LocDescriptorSize,
    _Out_ UINT32 *LocDescriptorVersion)
{
    EFI_STATUS Status;
    UINTN  MapKey = 0, MapSize = 0, DescSize = 0;
    UINT32 DescVer = 0;

    /* First probe: most firmwares answer with EFI_BUFFER_TOO_SMALL and
       the required size in MapSize. */
    Status = GlobalSystemTable->BootServices->GetMemoryMap(&MapSize,
                                                           NULL,
                                                           &MapKey,
                                                           &DescSize,
                                                           &DescVer);

    // AGENT-MODIFIED: Added detailed status debugging
    TRACE("GetMemoryMap initial probe Status=%lx, EFI_BUFFER_TOO_SMALL=%lx\n", (UINTN)Status, (UINTN)EFI_BUFFER_TOO_SMALL);
    TRACE("MapSize=%lu DescSize=%lu after probe\n", (UINTN)MapSize, (UINTN)DescSize);
    
    if (Status != EFI_BUFFER_TOO_SMALL && Status != EFI_SUCCESS)
    {
        /* Some quirky firmwares return INVALID_PARAMETER if Buffer==NULL.
           Seed a fallback and continue with the loop below. */
        TRACE("GetMemoryMap initial probe abnormal: %lx; seeding default\n", (UINTN)Status);
        if (MapSize == 0)
            MapSize = MAP_FALLBACK_BYTES;
        if (DescSize == 0)
            DescSize = sizeof(EFI_MEMORY_DESCRIPTOR);
        DescVer = 1;
    }

    /* Retry loop: allocate with slack, fetch, grow if needed. */
    for (;;)
    {
        if (DescSize == 0)
            DescSize = sizeof(EFI_MEMORY_DESCRIPTOR);

        UINTN Capacity = MapSize + (DescSize * MAP_SLACK_DESCRIPTORS);

        if (EfiMemoryMap)
        {
            GlobalSystemTable->BootServices->FreePool(EfiMemoryMap);
            EfiMemoryMap = NULL;
        }

        Status = GlobalSystemTable->BootServices->AllocatePool(EfiLoaderData,
                                                               Capacity,
                                                               (VOID **)&EfiMemoryMap);
        if (EFI_ERROR(Status) || !EfiMemoryMap)
        {
            TRACE("AllocatePool(EfiMemoryMap, %lu) failed: %lx\n",
                  (UINTN)Capacity, (UINTN)Status);
            UiMessageBoxCritical("Unable to initialize memory manager.");
            /* Hard stop: callers assume a valid map after return. */
            FrLdrBugCheckWithMessage(0, __FILE__, __LINE__,
                                     "AllocatePool for memory map failed: %lx", (UINTN)Status);
        }

        UINTN TmpSize = Capacity;
        Status = GlobalSystemTable->BootServices->GetMemoryMap(&TmpSize,
                                                               EfiMemoryMap,
                                                               &MapKey,
                                                               &DescSize,
                                                               &DescVer);
        if (Status == EFI_SUCCESS)
        {
            /* Trim to whole descriptors to avoid half-tail. */
            TmpSize -= (TmpSize % DescSize);

            *LocMapKey            = MapKey;
            *LocMapSize           = TmpSize;
            *LocDescriptorSize    = DescSize;
            *LocDescriptorVersion = DescVer;

            TRACE("MapKey=%lx MapSize=%lu DescSize=%lu DescVer=%lu\n",
                  (UINTN)MapKey, (UINTN)TmpSize, (UINTN)DescSize, (UINTN)DescVer);
            return;
        }

        if (Status != EFI_BUFFER_TOO_SMALL)
        {
            TRACE("GetMemoryMap failed: %lx\n", (UINTN)Status);
            UiMessageBoxCritical("Unable to initialize memory manager.");
            FrLdrBugCheckWithMessage(0, __FILE__, __LINE__,
                                     "GetMemoryMap failed: %lx", (UINTN)Status);
        }

        /* Grew meanwhile: try again with the size firmware just returned. */
        MapSize = TmpSize + (DescSize * MAP_SLACK_DESCRIPTORS);
    }
}

static
VOID
UefiSetMemory(
    _Inout_ PFREELDR_MEMORY_DESCRIPTOR MemoryMap,
    _In_    ULONG_PTR BaseAddress,
    _In_    PFN_COUNT SizeInPages,
    _In_    TYPE_OF_MEMORY MemoryType)
{
    /* Convert physical address to PFN */
    const PFN_NUMBER BasePage  = (PFN_NUMBER)(BaseAddress >> EFI_PAGE_SHIFT);
    const PFN_NUMBER PageCount = (PFN_NUMBER)SizeInPages;

    FreeldrDescCount = AddMemoryDescriptor(MemoryMap,
                                           UNUSED_MAX_DESCRIPTOR_COUNT,
                                           BasePage,
                                           PageCount,
                                           MemoryType);
}

static
TYPE_OF_MEMORY
UefiConvertToFreeldrDesc(_In_ EFI_MEMORY_TYPE EfiMemoryType)
{
    switch (EfiMemoryType)
    {
        case EfiReservedMemoryType:        return LoaderReserve;
        case EfiLoaderCode:                return LoaderLoadedProgram;
        case EfiLoaderData:                return LoaderLoadedProgram;
        case EfiBootServicesCode:          return LoaderFirmwareTemporary;
        case EfiBootServicesData:          return LoaderFirmwareTemporary;
        case EfiRuntimeServicesCode:       return LoaderFirmwarePermanent;
        case EfiRuntimeServicesData:       return LoaderFirmwarePermanent;
        case EfiConventionalMemory:        return LoaderFree;
        case EfiUnusableMemory:            return LoaderBad;
        case EfiACPIReclaimMemory:         return LoaderFirmwareTemporary;
        case EfiACPIMemoryNVS:             return LoaderReserve;
        case EfiMemoryMappedIO:            return LoaderReserve;
        case EfiMemoryMappedIOPortSpace:   return LoaderReserve;
        default:                           return LoaderReserve;
    }
}

PFREELDR_MEMORY_DESCRIPTOR
UefiMemGetMemoryMap(_Out_ ULONG *MemoryMapSize /* OUT: number of entries */)
{
    EFI_LOADED_IMAGE_PROTOCOL *LoadedImage = NULL;
    EFI_GUID EfiLoadedImageProtocol = EFI_LOADED_IMAGE_PROTOCOL_GUID;

    UINTN MapKey = 0, MapBytes = 0, DescSize = 0;
    UINT32 DescVersion = 0;

    EFI_STATUS Status;
    PFREELDR_MEMORY_DESCRIPTOR FreeldrMem = NULL;

    FreeldrDescCount = 0;
    EfiMemoryMap = NULL;

    /* Identify our image for base/size and the boot device. */
    Status = GlobalSystemTable->BootServices->HandleProtocol(GlobalImageHandle,
                                                             &EfiLoadedImageProtocol,
                                                             (VOID **)&LoadedImage);
    if (EFI_ERROR(Status) || !LoadedImage)
    {
        TRACE("HandleProtocol(LOADED_IMAGE) failed: %lx\n", (UINTN)Status);
        UiMessageBoxCritical("Unable to initialize memory manager.");
        return NULL;
    }

    OsLoaderBase     = LoadedImage->ImageBase;
    OsLoaderSize     = LoadedImage->ImageSize;
    PublicBootHandle = LoadedImage->DeviceHandle;

    TRACE("UefiMemGetMemoryMap: Gather memory map\n");
    PUEFI_LoadMemoryMap(&MapKey, &MapBytes, &DescSize, &DescVersion);

    /* Convert the firmware map into FreeLdr's compact descriptor list. */
    const UINT32 EntryCount = (DescSize ? (UINT32)(MapBytes / DescSize) : 0);

    // AGENT-MODIFIED: Added detailed allocation size debugging
    TRACE("Memory allocation calculation: MapBytes=%lu, DescSize=%lu, EntryCount=%u\n", 
          (UINTN)MapBytes, (UINTN)DescSize, EntryCount);

    /* Compute buffer size carefully (avoid overflow): */
    const SIZE_T each = sizeof(FREELDR_MEMORY_DESCRIPTOR);
    SIZE_T FreeldrEntriesCap = (SIZE_T)EntryCount + FREELDR_EXTRA_DESCS;
    if (FreeldrEntriesCap < EntryCount)  /* overflow check */
        FreeldrEntriesCap = EntryCount;

    SIZE_T FreeldrBytes = each * FreeldrEntriesCap;
    
    // AGENT-MODIFIED: Added calculation result debugging
    TRACE("FreeldrBytes calculation: each=%lu * FreeldrEntriesCap=%lu = %lu\n", 
          (UINTN)each, (UINTN)FreeldrEntriesCap, (UINTN)FreeldrBytes);

    Status = GlobalSystemTable->BootServices->AllocatePool(EfiLoaderData,
                                                           FreeldrBytes,
                                                           (VOID **)&FreeldrMem);
    if (EFI_ERROR(Status) || !FreeldrMem)
    {
        TRACE("AllocatePool(FreeldrMem %lu bytes) failed: %lx\n",
              (UINTN)FreeldrBytes, (UINTN)Status);
        UiMessageBoxCritical("Unable to initialize memory manager.");
        return NULL;
    }

    // AGENT-MODIFIED: Better debug trace before memset
    TRACE("About to memset: FreeldrMem=%p, FreeldrBytes=%lu\n", FreeldrMem, (UINTN)FreeldrBytes);

    /* Zero exactly what we allocated. */
    memset(FreeldrMem, 0, FreeldrBytes);
    
    // AGENT-MODIFIED: Confirm memset succeeded
    TRACE("memset completed successfully\n");

    /* Walk the EFI map and translate. */
    EFI_MEMORY_DESCRIPTOR *MapEntry = (EFI_MEMORY_DESCRIPTOR *)EfiMemoryMap;
    for (UINT32 i = 0; i < EntryCount; ++i)
    {
        TYPE_OF_MEMORY Mt = UefiConvertToFreeldrDesc(MapEntry->Type);

        /* Try to reserve ConventionalMemory so firmware doesn’t reuse it later. */
        if (Mt == LoaderFree)
        {
            EFI_STATUS Res =
                GlobalSystemTable->BootServices->AllocatePages(AllocateAddress,
                                                               EfiLoaderData,
                                                               MapEntry->NumberOfPages,
                                                               &MapEntry->PhysicalStart);
            if (EFI_ERROR(Res))
            {
                /* Could not pin; mark as temporary so we don’t assume ownership. */
                Mt = LoaderFirmwareTemporary;
            }
        }

        /* Track the maximum span of our own image (LoaderLoadedProgram). */
        if (Mt == LoaderLoadedProgram)
        {
            UINTN end = (UINTN)(MapEntry->PhysicalStart +
                                (MapEntry->NumberOfPages << EFI_PAGE_SHIFT));
            PFN_NUMBER last = (PFN_NUMBER)(end >> EFI_PAGE_SHIFT);
            if (last > LoaderPagesSpanned)
                LoaderPagesSpanned = last;
        }

        /* We do not expose LoaderReserve to our allocator. */
        if (Mt != LoaderReserve)
        {
            UefiSetMemory(FreeldrMem,
                          (ULONG_PTR)MapEntry->PhysicalStart,
                          (PFN_COUNT)MapEntry->NumberOfPages,
                          Mt);
        }

        MapEntry = NEXT_MEMORY_DESCRIPTOR(MapEntry, DescSize);
    }

    /* Windows/NT expects page 0 reserved; some UEFI maps leave it free. */
    UefiSetMemory(FreeldrMem, 0, 1, LoaderFirmwarePermanent);

    *MemoryMapSize = FreeldrDescCount;
    return FreeldrMem;
}

VOID
UefiExitBootServices(VOID)
{
    EFI_STATUS Status;
    UINTN MapKey = 0, MapBytes = 0, DescSize = 0;
    UINT32 DescVersion = 0;

    TRACE("Attempting to exit boot services\n");

    /* Per spec, fetch a *fresh* map/key immediately before ExitBootServices. */
    PUEFI_LoadMemoryMap(&MapKey, &MapBytes, &DescSize, &DescVersion);

    Status = GlobalSystemTable->BootServices->ExitBootServices(GlobalImageHandle, MapKey);

    /* Spec permits one retry: refetch key and try again. */
    if (EFI_ERROR(Status))
    {
        TRACE("ExitBootServices first attempt failed: %lx, retrying\n", (UINTN)Status);
        PUEFI_LoadMemoryMap(&MapKey, &MapBytes, &DescSize, &DescVersion);
        Status = GlobalSystemTable->BootServices->ExitBootServices(GlobalImageHandle, MapKey);
    }

    if (EFI_ERROR(Status))
    {
        TRACE("Failed to exit boot services: %lx\n", (UINTN)Status);
        FrLdrBugCheckWithMessage(EXIT_BOOTSERVICES_FAILURE,
                                 __FILE__,
                                 __LINE__,
                                 "ExitBootServices failed: %lx",
                                 (UINTN)Status);
    }
    else
    {
        TRACE("Exited boot services\n");
    }
}

VOID
UefiPrepareForReactOS(VOID)
{
    _exituefi();
}
