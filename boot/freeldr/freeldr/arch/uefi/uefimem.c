/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Memory Management Functions
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES ******************************************************************/

#include <uefildr.h>
#include <memops.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(WARNING);

#define NEXT_MEMORY_DESCRIPTOR(Descriptor, DescriptorSize) \
    (EFI_MEMORY_DESCRIPTOR*)((char*)(Descriptor) + (DescriptorSize))
#define EXIT_STACK_SIZE 0x1000
#define UNUSED_MAX_DESCRIPTOR_COUNT 10000

ULONG
AddMemoryDescriptor(
    _Inout_ PFREELDR_MEMORY_DESCRIPTOR List,
    _In_ ULONG MaxCount,
    _In_ PFN_NUMBER BasePage,
    _In_ PFN_NUMBER PageCount,
    _In_ TYPE_OF_MEMORY MemoryType);

/* GLOBALS *******************************************************************/

extern ULONG LoaderPagesSpanned;
extern EFI_SYSTEM_TABLE* GlobalSystemTable;
extern EFI_HANDLE GlobalImageHandle;
extern REACTOS_INTERNAL_BGCONTEXT framebufferData;

EFI_MEMORY_DESCRIPTOR* EfiMemoryMap = NULL;
UINT32 FreeldrDescCount;
PVOID OsLoaderBase;
SIZE_T OsLoaderSize;
EFI_HANDLE PublicBootHandle;
PVOID ExitStack;
PVOID EndofExitStack;
BOOLEAN UefiBootServicesExited = FALSE;  /* Track if boot services have been exited */

void _exituefi(VOID);

/* FUNCTIONS *****************************************************************/

static
VOID
PUEFI_LoadMemoryMap(
    _Out_ UINTN*  LocMapKey,
    _Out_ UINTN*  LocMapSize,
    _Out_ UINTN*  LocDescriptorSize,
    _Out_ UINT32* LocDescriptorVersion)
{
    EFI_STATUS Status;
    UINTN AllocationSize = 0;
    ULONG Count = 0;

    TRACE("PUEFI_LoadMemoryMap: Entry\n");
    
    Status = GlobalSystemTable->BootServices->GetMemoryMap(LocMapSize,
                                                           EfiMemoryMap,
                                                           LocMapKey,
                                                           LocDescriptorSize,
                                                           LocDescriptorVersion);
    TRACE("PUEFI_LoadMemoryMap: Initial GetMemoryMap Status=0x%lx, MapSize=%lu\n", 
          (ULONG)Status, (ULONG)*LocMapSize);

    /* Reallocate and retrieve again the needed memory map size (since memory
     * allocated by AllocatePool() counts in the map), until it's OK. */
    while (Status != EFI_SUCCESS)
    {
        TRACE("PUEFI_LoadMemoryMap: Reallocating buffer, attempt %lu\n", Count + 1);
        
        /* Reallocate the memory map buffer */
        if (EfiMemoryMap)
            GlobalSystemTable->BootServices->FreePool(EfiMemoryMap);

        /* If MapSize never reports the correct size after the first time, increment */
        AllocationSize = *LocMapSize + (*LocDescriptorSize * Count);
        TRACE("PUEFI_LoadMemoryMap: Allocating %lu bytes\n", (ULONG)AllocationSize);
        
        GlobalSystemTable->BootServices->AllocatePool(EfiLoaderData, AllocationSize,
                                                      (VOID**)&EfiMemoryMap);
        Status = GlobalSystemTable->BootServices->GetMemoryMap(LocMapSize,
                                                               EfiMemoryMap,
                                                               LocMapKey,
                                                               LocDescriptorSize,
                                                               LocDescriptorVersion);
        TRACE("PUEFI_LoadMemoryMap: GetMemoryMap Status=0x%lx after realloc\n", (ULONG)Status);
        Count++;
        
        if (Count > 10)
        {
            ERR("PUEFI_LoadMemoryMap: Failed after 10 attempts, giving up\n");
            break;
        }
    }
    
    TRACE("PUEFI_LoadMemoryMap: Exit - MapKey=%lu, MapSize=%lu\n", 
          (ULONG)*LocMapKey, (ULONG)*LocMapSize);
}

static
VOID
UefiSetMemory(
    _Inout_ PFREELDR_MEMORY_DESCRIPTOR MemoryMap,
    _In_ ULONG_PTR BaseAddress,
    _In_ PFN_COUNT SizeInPages,
    _In_ TYPE_OF_MEMORY MemoryType)
{
    ULONG_PTR BasePage, PageCount;

    TRACE("UefiSetMemory: Entry - BaseAddress=0x%lx, SizeInPages=%lu, MemoryType=%d\n",
          (ULONG)BaseAddress, (ULONG)SizeInPages, MemoryType);

    /* Sanity check for corrupted values */
    if (SizeInPages > 0x100000 || BaseAddress > 0xFFFFFFFF)
    {
        ERR("UefiSetMemory: Corrupted descriptor! BaseAddress=0x%lx, SizeInPages=%lu - SKIPPING\n",
            (ULONG)BaseAddress, (ULONG)SizeInPages);
        return;
    }

    BasePage = BaseAddress / EFI_PAGE_SIZE;
    PageCount = SizeInPages;
    
    TRACE("UefiSetMemory: Calling AddMemoryDescriptor with BasePage=0x%lx, PageCount=%lu\n",
          (ULONG)BasePage, (ULONG)PageCount);

    /* Add the memory descriptor */
    FreeldrDescCount = AddMemoryDescriptor(MemoryMap,
                                           UNUSED_MAX_DESCRIPTOR_COUNT,
                                           BasePage,
                                           PageCount,
                                           MemoryType);
    
    TRACE("UefiSetMemory: AddMemoryDescriptor returned FreeldrDescCount=%lu\n", (ULONG)FreeldrDescCount);
}

static
TYPE_OF_MEMORY
UefiConvertToFreeldrDesc(EFI_MEMORY_TYPE EfiMemoryType)
{
    switch (EfiMemoryType)
    {
        case EfiReservedMemoryType:
            return LoaderReserve;
        case EfiLoaderCode:
            return LoaderLoadedProgram;
        case EfiLoaderData:
            return LoaderLoadedProgram;
        case EfiBootServicesCode:
            return LoaderFirmwareTemporary;
        case EfiBootServicesData:
            return LoaderFirmwareTemporary;
        case EfiRuntimeServicesCode:
            return LoaderFirmwarePermanent;
        case EfiRuntimeServicesData:
            return LoaderFirmwarePermanent;
        case EfiConventionalMemory:
            return LoaderFree;
        case EfiUnusableMemory:
            return LoaderBad;
        case EfiACPIReclaimMemory:
            return LoaderFirmwareTemporary;
        case EfiACPIMemoryNVS:
            return LoaderReserve;
        case EfiMemoryMappedIO:
            return LoaderReserve;
        case EfiMemoryMappedIOPortSpace:
            return LoaderReserve;
        default:
            break;
    }
    return LoaderReserve;
}

PFREELDR_MEMORY_DESCRIPTOR
UefiMemGetMemoryMap(ULONG *MemoryMapSize)
{
    EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
    UINT32 DescriptorVersion;
    SIZE_T FreeldrMemMapSize;
    UINTN DescriptorSize;
    EFI_STATUS Status;
    UINTN MapSize;
    UINTN MapKey;
    UINT32 Index;

    EFI_GUID EfiLoadedImageProtocol = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    PFREELDR_MEMORY_DESCRIPTOR FreeldrMem = NULL;
    EFI_MEMORY_DESCRIPTOR* MapEntry = NULL;
    UINT32 EntryCount = 0;
    FreeldrDescCount = 0;

    Status = GlobalSystemTable->BootServices->HandleProtocol(GlobalImageHandle,
                                                             &EfiLoadedImageProtocol,
                                                             (VOID**)&LoadedImage);
    if (Status != EFI_SUCCESS)
    {
        TRACE("Failed to find LoadedImageHandle with status: %d\n", Status);
        UiMessageBoxCritical("Unable to initialize memory manager.");
        return NULL;
    }
    OsLoaderBase = LoadedImage->ImageBase;
    OsLoaderSize = LoadedImage->ImageSize;
    PublicBootHandle = LoadedImage->DeviceHandle;
    EfiMemoryMap = NULL;

    TRACE("UefiMemGetMemoryMap: Gather memory map\n");
    PUEFI_LoadMemoryMap(&MapKey,
                        &MapSize,
                        &DescriptorSize,
                        &DescriptorVersion);

    TRACE("Value of MapKey: %d\n", MapKey);
    TRACE("Value of MapSize: %d\n", MapSize);
    TRACE("Value of DescriptorSize: %d\n", DescriptorSize);
    TRACE("Value of DescriptorVersion: %d\n", DescriptorVersion);

    EntryCount = (MapSize / DescriptorSize);
    TRACE("Calculated EntryCount: %lu\n", (ULONG)EntryCount);

    FreeldrMemMapSize = (sizeof(FREELDR_MEMORY_DESCRIPTOR) * EntryCount);
    TRACE("Allocating FreeldrMemMapSize: %lu bytes\n", (ULONG)FreeldrMemMapSize);
    
    Status = GlobalSystemTable->BootServices->AllocatePool(EfiLoaderData,
                                                           FreeldrMemMapSize,
                                                           (void**)&FreeldrMem);
    if (Status != EFI_SUCCESS)
    {
        TRACE("Failed to allocate pool with status %d\n", Status);
        UiMessageBoxCritical("Unable to initialize memory manager.");
        return NULL;
    }
    
    TRACE("Successfully allocated FreeldrMem at 0x%p\n", FreeldrMem);

    TRACE("About to zero memory at 0x%p, size=%lu\n", FreeldrMem, (ULONG)FreeldrMemMapSize);
    
    /* Use safe memory operation for early boot */
    FrLdrZeroMemory(FreeldrMem, FreeldrMemMapSize);
    
    TRACE("Memory zeroed successfully\n");
    
    MapEntry = EfiMemoryMap;
    TRACE("EfiMemoryMap pointer: 0x%p\n", MapEntry);
    
    TRACE("Starting memory descriptor processing. EntryCount=%lu\n", (ULONG)EntryCount);
    
	for (Index = 0; Index < EntryCount; ++Index)
    {
        TRACE("Processing descriptor %lu/%lu\n", (ULONG)Index, (ULONG)EntryCount);
        TRACE("  MapEntry pointer: 0x%p\n", MapEntry);
        
        /* Check for NULL pointer */
        if (MapEntry == NULL)
        {
            TRACE("ERROR: MapEntry is NULL at index %lu\n", (ULONG)Index);
            break;
        }
        
        TYPE_OF_MEMORY MemoryType = UefiConvertToFreeldrDesc(MapEntry->Type);
        
        TRACE("  Type=%d, PhysAddr=0x%lx, Pages=%lu\n", 
              MapEntry->Type, (ULONG)MapEntry->PhysicalStart, (ULONG)MapEntry->NumberOfPages);
        
        /* For LoaderFree memory, we don't need to call AllocatePages */
        /* since it's already free memory - just track it properly */
        if (MemoryType == LoaderFree)
        {
            /* LoaderFree memory should still be processed by UefiSetMemory below */
            TRACE("  Processing LoaderFree memory (no allocation needed)\n");
        }

        /* Sometimes our loader can be loaded into higher memory than we ever allocate */
        if (MemoryType == LoaderLoadedProgram)
        {
            if (((MapEntry->PhysicalStart + (MapEntry->NumberOfPages * PAGE_SIZE)) >> EFI_PAGE_SHIFT) > LoaderPagesSpanned)
            {
                /* This value needs to be adjusted if this occurs */
                LoaderPagesSpanned = ((MapEntry->PhysicalStart + (MapEntry->NumberOfPages * PAGE_SIZE)) >> EFI_PAGE_SHIFT);
            }
        }

        /* Process all memory types including LoaderReserve */
        TRACE("  Calling UefiSetMemory with MemoryType=%d\n", MemoryType);
        UefiSetMemory(FreeldrMem,
                      MapEntry->PhysicalStart,
                      MapEntry->NumberOfPages,
                      MemoryType);
        TRACE("  UefiSetMemory completed\n");

        TRACE("  Moving to next descriptor. Current=%p, DescriptorSize=%lu\n", MapEntry, (ULONG)DescriptorSize);
        MapEntry = NEXT_MEMORY_DESCRIPTOR(MapEntry, DescriptorSize);
        TRACE("  Next MapEntry pointer: 0x%p\n", MapEntry);
    }
    
    TRACE("Memory descriptor processing completed. Processed %lu entries\n", (ULONG)Index);

    /* Windows expects the first page to be reserved, otherwise it asserts.
     * However it can be just a free page on some UEFI systems. */
    TRACE("Setting first page as LoaderFirmwarePermanent\n");
    UefiSetMemory(FreeldrMem, 0x000000, 1, LoaderFirmwarePermanent);
    
    TRACE("Final FreeldrDescCount: %lu\n", (ULONG)FreeldrDescCount);
    *MemoryMapSize = FreeldrDescCount;
    
    TRACE("UefiMemGetMemoryMap completed successfully, returning 0x%p\n", FreeldrMem);
    return FreeldrMem;
}

VOID
UefiExitBootServices(VOID)
{
    UINTN MapKey;
    UINTN MapSize;
    EFI_STATUS Status;
    UINTN DescriptorSize;
    UINT32 DescriptorVersion;
    ULONG Attempts;
    volatile ULONG Step = 0;

    Step = 1;
    TRACE("UefiExitBootServices: Entry (Step %lu)\n", Step);
    
    Step = 2;
    TRACE("UefiExitBootServices: GlobalSystemTable=%p (Step %lu)\n", GlobalSystemTable, Step);
    TRACE("UefiExitBootServices: GlobalImageHandle=%p (Step %lu)\n", GlobalImageHandle, Step);
    
    if (GlobalSystemTable == NULL)
    {
        ERR("FATAL: GlobalSystemTable is NULL! Cannot exit boot services\n");
        return;
    }
    
    Step = 3;
    TRACE("UefiExitBootServices: BootServices=%p (Step %lu)\n", GlobalSystemTable->BootServices, Step);
    
    if (GlobalSystemTable->BootServices == NULL)
    {
        ERR("FATAL: BootServices is NULL! Cannot exit boot services\n");
        return;
    }
    
    Step = 4;
    TRACE("UefiExitBootServices: All pointers valid, proceeding (Step %lu)\n", Step);
    
    /* Try up to 5 times to exit boot services.
     * The memory map can change between getting it and calling ExitBootServices,
     * so we need to retry with a fresh map key if it fails. */
    for (Attempts = 0; Attempts < 5; Attempts++)
    {
        Step = 10 + Attempts;
        TRACE("UefiExitBootServices: Attempt %lu of 5 (Step %lu)\n", Attempts + 1, Step);
        
        /* Get a fresh memory map and key */
        TRACE("UefiExitBootServices: Calling PUEFI_LoadMemoryMap\n");
        
        MapKey = 0;
        MapSize = 0;
        DescriptorSize = 0;
        DescriptorVersion = 0;
        
        PUEFI_LoadMemoryMap(&MapKey,
                            &MapSize,
                            &DescriptorSize,
                            &DescriptorVersion);
        
        TRACE("UefiExitBootServices: After LoadMemoryMap - MapKey=%lu, MapSize=%lu\n", 
              (ULONG)MapKey, (ULONG)MapSize);

        if (MapKey == 0 || MapSize == 0)
        {
            ERR("UefiExitBootServices: Invalid map key or size!\n");
            continue;
        }

        /* Try to exit boot services */
        TRACE("UefiExitBootServices: About to call BootServices->ExitBootServices\n");
        TRACE("UefiExitBootServices: GlobalImageHandle=%p, MapKey=%lu\n", 
              GlobalImageHandle, (ULONG)MapKey);
        
        Status = GlobalSystemTable->BootServices->ExitBootServices(GlobalImageHandle, MapKey);
        
        TRACE("UefiExitBootServices: ExitBootServices returned Status=0x%lx\n", (ULONG)Status);
        
        if (Status == EFI_SUCCESS)
        {
            TRACE("Successfully exited boot services on attempt %lu\n", Attempts + 1);
            UefiBootServicesExited = TRUE;  /* Mark that boot services have been exited */
            return;
        }
        
        /* If we got an invalid parameter error, the map key is stale - retry */
        if (Status == EFI_INVALID_PARAMETER)
        {
            TRACE("ExitBootServices failed with stale map key on attempt %lu, retrying...\n", Attempts + 1);
            continue;
        }
        
        /* For other errors, bail out */
        TRACE("ExitBootServices failed with unexpected status 0x%lx on attempt %lu\n", (ULONG)Status, Attempts + 1);
        break;
    }

    /* If we get here, we failed to exit boot services */
    TRACE("Failed to exit boot services after %lu attempts with final status: 0x%lx\n", Attempts, (ULONG)Status);
    FrLdrBugCheckWithMessage(EXIT_BOOTSERVICES_FAILURE,
                             __FILE__,
                             __LINE__,
                             "Failed to exit boot services after %lu attempts with status: 0x%lx",
                             Attempts, (ULONG)Status);
}

VOID
UefiPrepareForReactOS(VOID)
{
    volatile ULONG Checkpoint = 0;
    
    /* Use volatile to prevent optimization */
    Checkpoint = 1;
    TRACE("UefiPrepareForReactOS: Entry (Checkpoint %lu)\n", Checkpoint);
    
    Checkpoint = 2;
    TRACE("UefiPrepareForReactOS: GlobalSystemTable=%p (Checkpoint %lu)\n", GlobalSystemTable, Checkpoint);
    
    if (GlobalSystemTable == NULL)
    {
        ERR("FATAL: GlobalSystemTable is NULL!\n");
        return;
    }
    
    Checkpoint = 3;
    TRACE("UefiPrepareForReactOS: BootServices=%p (Checkpoint %lu)\n", 
          GlobalSystemTable->BootServices, Checkpoint);
    
    if (GlobalSystemTable->BootServices == NULL)
    {
        ERR("FATAL: BootServices is NULL!\n");
        return;
    }
    
    /* Memory barrier to ensure all previous writes are visible */
    MemoryBarrier();
    
    Checkpoint = 4;
    TRACE("UefiPrepareForReactOS: About to call UefiExitBootServices (Checkpoint %lu)\n", Checkpoint);
    TRACE("*** CALLING UefiExitBootServices DIRECTLY - NO STACK SWITCH ***\n");
    
    /* Direct call - no exception handling available in bootloader */
    UefiExitBootServices();
    
    Checkpoint = 5;
    TRACE("UefiPrepareForReactOS: Returned from UefiExitBootServices (Checkpoint %lu)\n", Checkpoint);
    
    /* Mark that boot services have been exited */
    UefiBootServicesExited = TRUE;
    
    Checkpoint = 6;
    TRACE("UefiPrepareForReactOS: Boot services marked as exited (Checkpoint %lu)\n", Checkpoint);
    TRACE("UefiPrepareForReactOS: Exit successfully\n");
}
