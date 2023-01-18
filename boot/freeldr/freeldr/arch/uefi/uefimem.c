/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Memory Management Functions
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES ******************************************************************/

#include <uefildr.h>

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

    Status = GlobalSystemTable->BootServices->GetMemoryMap(LocMapSize,
                                                           EfiMemoryMap,
                                                           LocMapKey,
                                                           LocDescriptorSize,
                                                           LocDescriptorVersion);

    /* Reallocate and retrieve again the needed memory map size (since memory
     * allocated by AllocatePool() counts in the map), until it's OK. */
    while (Status != EFI_SUCCESS)
    {
        /* Reallocate the memory map buffer */
        if (EfiMemoryMap)
            GlobalSystemTable->BootServices->FreePool(EfiMemoryMap);

        /* If MapSize never reports the correct size after the first time, increment */
        AllocationSize = *LocMapSize + (*LocDescriptorSize * Count);
        GlobalSystemTable->BootServices->AllocatePool(EfiLoaderData, AllocationSize,
                                                      (VOID**)&EfiMemoryMap);
        Status = GlobalSystemTable->BootServices->GetMemoryMap(LocMapSize,
                                                               EfiMemoryMap,
                                                               LocMapKey,
                                                               LocDescriptorSize,
                                                               LocDescriptorVersion);
        Count++;
    }
}

static
VOID
UefiSetMemory(
    _Inout_ PFREELDR_MEMORY_DESCRIPTOR MemoryMap,
    _In_ ULONG_PTR BaseAddress,
    _In_ PFN_COUNT Size,
    _In_ TYPE_OF_MEMORY MemoryType)
{
    ULONG_PTR BasePage, PageCount;

    BasePage = BaseAddress / EFI_PAGE_SIZE;
    PageCount = Size;

    /* Add the memory descriptor */
    FreeldrDescCount = AddMemoryDescriptor(MemoryMap,
                                           UNUSED_MAX_DESCRIPTOR_COUNT,
                                           BasePage,
                                           PageCount,
                                           MemoryType);
}

VOID
ReserveMemory(
    _Inout_ PFREELDR_MEMORY_DESCRIPTOR MemoryMap,
    _In_ ULONG_PTR BaseAddress,
    _In_ PFN_NUMBER Size,
    _In_ TYPE_OF_MEMORY MemoryType,
    _In_ PCHAR Usage)
{
    ULONG_PTR BasePage, PageCount;
    ULONG i;

    BasePage = BaseAddress / PAGE_SIZE;
    PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(BaseAddress, Size);

    for (i = 0; i < FreeldrDescCount; i++)
    {
        /* Check for conflicting descriptor */
        if ((MemoryMap[i].BasePage < BasePage + PageCount) &&
            (MemoryMap[i].BasePage + MemoryMap[i].PageCount > BasePage))
        {
            /* Check if the memory is free */
            if (MemoryMap[i].MemoryType != LoaderFree)
            {
                FrLdrBugCheckWithMessage(
                    MEMORY_INIT_FAILURE,
                    __FILE__,
                    __LINE__,
                    "Failed to reserve memory in the range 0x%Ix - 0x%Ix for %s",
                    BaseAddress,
                    Size,
                    Usage);
            }
        }
    }

    /* Add the memory descriptor */
    FreeldrDescCount = AddMemoryDescriptor(MemoryMap,
                                           UNUSED_MAX_DESCRIPTOR_COUNT,
                                           BasePage,
                                           PageCount,
                                           MemoryType);
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

    FreeldrMemMapSize = (sizeof(FREELDR_MEMORY_DESCRIPTOR) * EntryCount);
    Status = GlobalSystemTable->BootServices->AllocatePool(EfiLoaderData,
                                                           FreeldrMemMapSize,
                                                           (void**)&FreeldrMem);
    if (Status != EFI_SUCCESS)
    {
        TRACE("Failed to allocate pool with status %d\n", Status);
        UiMessageBoxCritical("Unable to initialize memory manager.");
        return NULL;
    }

    RtlZeroMemory(FreeldrMem, FreeldrMemMapSize);
    MapEntry = EfiMemoryMap;
	for (Index = 0; Index < EntryCount; ++Index)
    {
        TYPE_OF_MEMORY MemoryType = UefiConvertToFreeldrDesc(MapEntry->Type);
        if (MemoryType == LoaderFree)
        {
            Status = GlobalSystemTable->BootServices->AllocatePages(AllocateAddress,
                                                                    EfiLoaderData,
                                                                    MapEntry->NumberOfPages,
                                                                    &MapEntry->PhysicalStart);
            if (Status != EFI_SUCCESS)
            {
                /* We failed to reserve the page, so change its type */
                MemoryType = LoaderFirmwareTemporary;
            }
        }

        UefiSetMemory(FreeldrMem,
                      MapEntry->PhysicalStart,
                      MapEntry->NumberOfPages,
                      MemoryType);

        MapEntry = NEXT_MEMORY_DESCRIPTOR(MapEntry, DescriptorSize);
    }

    *MemoryMapSize = FreeldrDescCount;
    return FreeldrMem;
}

static VOID
UefiExitBootServices(VOID)
{
    UINTN MapKey;
    UINTN MapSize;
    EFI_STATUS Status;
    UINTN DescriptorSize;
    UINT32 DescriptorVersion;

    TRACE("Attempting to exit bootsevices\n");
    PUEFI_LoadMemoryMap(&MapKey,
                        &MapSize,
                        &DescriptorSize,
                        &DescriptorVersion);

    Status = GlobalSystemTable->BootServices->ExitBootServices(GlobalImageHandle, MapKey);
    /* UEFI spec demands twice! */
    if (Status != EFI_SUCCESS)
        Status = GlobalSystemTable->BootServices->ExitBootServices(GlobalImageHandle, MapKey);

    if (Status != EFI_SUCCESS)
    {
        TRACE("Failed to exit boot services with status: %d\n", Status);
        FrLdrBugCheckWithMessage(EXIT_BOOTSERVICES_FAILURE,
                                 __FILE__,
                                 __LINE__,
                                 "Failed to exit boot services with status: %d",
                                 Status);
    }
    else
    {
        TRACE("Exited bootservices\n");
    }
}

VOID
UefiPrepareForReactOS(VOID)
{
    UefiExitBootServices();
    ExitStack = MmAllocateMemoryWithType(EXIT_STACK_SIZE, LoaderOsloaderStack);
    EndofExitStack = (PVOID)((ULONG_PTR)ExitStack + EXIT_STACK_SIZE);
}
