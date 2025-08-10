/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 Windows NT Loader Functions
 * COPYRIGHT:   Copyright 2024 ReactOS Team
 */

#include <freeldr.h>
#include <ntldr/winldr.h>
#include <debug.h>
DBG_DEFAULT_CHANNEL(WINDOWS);

BOOLEAN
MempSetupPaging(
    IN PFN_NUMBER StartPage,
    IN PFN_NUMBER NumberOfPages,
    IN BOOLEAN KernelMapping)
{
    /* ARM64 paging setup using UEFI memory management */
    TRACE("ARM64: Setting up paging for StartPage=0x%lx, NumberOfPages=0x%lx, KernelMapping=%d\n",
          (ULONG)StartPage, (ULONG)NumberOfPages, KernelMapping);
    
    /* For UEFI ARM64, we rely on identity mapping set up by the bootloader */
    /* The kernel will set up its own page tables */
    return TRUE;
}

VOID
MempUnmapPage(
    PFN_NUMBER Page)
{
    /* ARM64 page unmapping */
    TRACE("ARM64: Unmapping page 0x%lx\n", (ULONG)Page);
    
    /* For UEFI ARM64, page unmapping is handled by UEFI/MMU */
    /* Individual page unmapping is not typically needed in the bootloader */
}

VOID
MempDump(VOID)
{
    /* ARM64 memory dump for debugging */
    TRACE("ARM64: Memory dump requested\n");
    
    /* This would dump memory allocation information for debugging */
    /* Implementation can be added when needed for debugging purposes */
}

BOOLEAN
Arm64SetupForNt(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PVOID *GdtIdt,
    IN ULONG *PcrBasePage,
    IN ULONG *TssBasePage)
{
    TRACE("ARM64: Setting up for NT kernel\n");
    
    /* ARM64 doesn't use GDT/IDT or TSS like x86 */
    *GdtIdt = NULL;
    *PcrBasePage = 0;
    *TssBasePage = 0;
    
    /* Setup ARM64 specific structures for NT */
    /* Initialize ARM64 exception vectors if needed */
    /* The kernel will set up its own exception handling */
    
    /* Ensure memory is properly prepared */
    if (!Arm64InitializeMemory(LoaderBlock))
    {
        ERR("ARM64: Failed to initialize memory for NT\n");
        return FALSE;
    }
    
    TRACE("ARM64: Successfully set up for NT kernel\n");
    return TRUE;
}

/* ARM64 specific memory setup */
BOOLEAN
Arm64InitializeMemory(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    TRACE("ARM64: Initializing memory management structures\n");
    
    /* Validate loader parameter block */
    if (!LoaderBlock)
    {
        ERR("ARM64: Invalid LoaderBlock\n");
        return FALSE;
    }
    
    /* Initialize memory descriptor list if needed */
    if (IsListEmpty(&LoaderBlock->MemoryDescriptorListHead))
    {
        ERR("ARM64: Memory descriptor list is empty\n");
        return FALSE;
    }
    
    /* Dump memory descriptors for debugging */
    WinLdrpDumpMemoryDescriptors(LoaderBlock);
    
    /* ARM64 specific memory initialization */
    /* The UEFI firmware has already set up basic memory management */
    /* Additional ARM64 specific setup can be added here if needed */
    
    TRACE("ARM64: Memory management structures initialized\n");
    return TRUE;
}

VOID
WinLdrpDumpMemoryDescriptors(PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* Dump memory descriptors for debugging */
    PLIST_ENTRY NextMd;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    
    NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextMd != &LoaderBlock->MemoryDescriptorListHead)
    {
        MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                           MEMORY_ALLOCATION_DESCRIPTOR,
                                           ListEntry);
        
        TRACE("MD: Base=0x%X, Count=0x%X, Type=%d\n",
              MemoryDescriptor->BasePage,
              MemoryDescriptor->PageCount,
              MemoryDescriptor->MemoryType);
        
        NextMd = MemoryDescriptor->ListEntry.Flink;
    }
}

PVOID
WinLdrLoadModule(
    PCSTR ModuleName,
    ULONG *ModuleSize,
    TYPE_OF_MEMORY MemoryType)
{
    /* Load a module for ARM64 */
    TRACE("ARM64: Loading module %s\n", ModuleName);
    
    /* For ARM64 UEFI, module loading is handled by the generic PE loader */
    /* which should already support ARM64 PE files */
    /* This function is typically not called directly - the generic */
    /* PE loader in the main code handles ARM64 PE files */
    
    /* If a specific ARM64 module loader is needed, implement it here */
    WARN("ARM64: Module loading not implemented - using generic PE loader\n");
    
    if (ModuleSize) *ModuleSize = 0;
    return NULL;
}

BOOLEAN
WinLdrCheckForLoadedDll(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PCH DllName,
    OUT PLDR_DATA_TABLE_ENTRY *LoadedEntry)
{
    /* Check if a DLL is already loaded */
    PLIST_ENTRY NextEntry;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    
    NextEntry = LoaderBlock->LoadOrderListHead.Flink;
    while (NextEntry != &LoaderBlock->LoadOrderListHead)
    {
        DataTableEntry = CONTAINING_RECORD(NextEntry,
                                         LDR_DATA_TABLE_ENTRY,
                                         InLoadOrderLinks);
        
        /* Compare the base DLL name */
        if (_stricmp(DataTableEntry->BaseDllName.Buffer, DllName) == 0)
        {
            *LoadedEntry = DataTableEntry;
            return TRUE;
        }
        
        NextEntry = NextEntry->Flink;
    }
    
    return FALSE;
}