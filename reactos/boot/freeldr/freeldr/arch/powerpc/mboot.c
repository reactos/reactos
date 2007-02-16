/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Freeloader
 * FILE:            boot/freeldr/freeldr/multiboot.c
 * PURPOSE:         ReactOS Loader
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

#include <freeldr.h>
#include <internal/powerpc/ke.h>

#define NDEBUG
#include <debug.h>

/* Base Addres of Kernel in Physical Memory */
#define KERNEL_BASE_PHYS 0x200000

/* Bits to shift to convert a Virtual Address into an Offset in the Page Table */
#define PFN_SHIFT 12

/* Bits to shift to convert a Virtual Address into an Offset in the Page Directory */
#define PDE_SHIFT 20
#define PDE_SHIFT_PAE 18

  
/* Converts a Relative Address read from the Kernel into a Physical Address */
#define RaToPa(p) \
    (ULONG_PTR)((ULONG_PTR)p + KERNEL_BASE_PHYS)
   
/* Converts a Phsyical Address Pointer into a Page Frame Number */
#define PaPtrToPfn(p) \
    (((ULONG_PTR)&p) >> PFN_SHIFT)
    
/* Converts a Phsyical Address into a Page Frame Number */
#define PaToPfn(p) \
    ((p) >> PFN_SHIFT)

#define STARTUP_BASE                0xF0000000
#define HYPERSPACE_BASE             0xF0800000
#define APIC_BASE                   0xFEC00000
#define KPCR_BASE                   0xFF000000
    
#define LowMemPageTableIndex        0
#define StartupPageTableIndex       (STARTUP_BASE >> 20)    / sizeof(HARDWARE_PTE_X86)
#define HyperspacePageTableIndex    (HYPERSPACE_BASE >> 20) / sizeof(HARDWARE_PTE_X86)
#define KpcrPageTableIndex          (KPCR_BASE >> 20)       / sizeof(HARDWARE_PTE_X86)
#define ApicPageTableIndex          (APIC_BASE >> 20)       / sizeof(HARDWARE_PTE_X86)

#define LowMemPageTableIndexPae     0
#define StartupPageTableIndexPae    (STARTUP_BASE >> 21)
#define HyperspacePageTableIndexPae (HYPERSPACE_BASE >> 21)
#define KpcrPageTableIndexPae       (KPCR_BASE >> 21)
#define ApicPageTableIndexPae       (APIC_BASE >> 21)


#define KernelEntryPoint            (KernelEntry - KERNEL_BASE_PHYS) + KernelBase

/* Load Address of Next Module */
ULONG_PTR NextModuleBase = 0;

/* Currently Opened Module */
VOID *CurrentModule = NULL;

/* Unrelocated Kernel Base in Virtual Memory */
ULONG_PTR KernelBase;

/* Wether PAE is to be used or not */
BOOLEAN PaeModeEnabled;

/* Kernel Entrypoint in Physical Memory */
ULONG_PTR KernelEntry;

/* FUNCTIONS *****************************************************************/

/*++
 * FrLdrStartup 
 * INTERNAL
 *
 *     Prepares the system for loading the Kernel.
 *
 * Params:
 *     Magic - Multiboot Magic
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     None.
 *
 *--*/
VOID
NTAPI
FrLdrStartup(ULONG Magic)
{   
#if 0   
    /* Disable Interrupts */
    KeArchDisableInterrupts();
    
    /* Re-initalize EFLAGS */
    KeArchEraseFlags();
    
    /* Initialize the page directory */
    FrLdrSetupPageDirectory();
#endif
}

/*++
 * FrLdrGetKernelBase 
 * INTERNAL
 *
 *     Gets the Kernel Base to use.
 *
 * Params:
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     Sets both the FreeLdr internal variable as well as the one which
 *     will be used by the Kernel.
 *
 *--*/
VOID
FASTCALL
FrLdrGetKernelBase(VOID)
{
    PCHAR p;
     
    /* Read Command Line */
    p = (PCHAR)LoaderBlock.CommandLine;
    while ((p = strchr(p, '/')) != NULL) {
        
        /* Find "/3GB" */
        if (!strnicmp(p + 1, "3GB", 3)) {
            
            /* Make sure there's nothing following it */
            if (p[4] == ' ' || p[4] == 0) {
                
                /* Use 3GB */
                KernelBase = 0xC0000000;
            }
        }

        p++;
    }
          
    /* Set KernelBase */
    LoaderBlock.KernelBase = KernelBase;
}

/*++
 * FrLdrSetupPageDirectory 
 * INTERNAL
 *
 *     Sets up the ReactOS Startup Page Directory.
 *
 * Params:
 *     None.
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     We are setting PDEs, but using the equvivalent (for our purpose) PTE structure.
 *     As such, please note that PageFrameNumber == PageEntryNumber.
 *
 *--*/
VOID
FASTCALL
FrLdrSetupPageDirectory(VOID)
{
#if 0
    PPAGE_DIRECTORY_X86 PageDir;
    PPAGE_DIRECTORY_TABLE_X64 PageDirTablePae;
    PPAGE_DIRECTORY_X64 PageDirPae;
    ULONG KernelPageTableIndex;
    ULONG i;

    if (PaeModeEnabled) {
    
        /* Get the Kernel Table Index */
        KernelPageTableIndex = (KernelBase >> 21);

        /* Get the Startup Page Directory Table */
        PageDirTablePae = (PPAGE_DIRECTORY_TABLE_X64)&startup_pagedirectorytable_pae;

        /* Get the Startup Page Directory */
        PageDirPae = (PPAGE_DIRECTORY_X64)&startup_pagedirectory_pae;

        /* Set the Startup page directory table */
        for (i = 0; i < 4; i++)
        {
            PageDirTablePae->Pde[i].Valid = 1;
            PageDirTablePae->Pde[i].PageFrameNumber = PaPtrToPfn(startup_pagedirectory_pae) + i;
        }

        /* Set up the Low Memory PDE */
        for (i = 0; i < 2; i++)
        {
            PageDirPae->Pde[LowMemPageTableIndexPae + i].Valid = 1;
            PageDirPae->Pde[LowMemPageTableIndexPae + i].Write = 1;
            PageDirPae->Pde[LowMemPageTableIndexPae + i].PageFrameNumber = PaPtrToPfn(lowmem_pagetable_pae) + i;
        }

        /* Set up the Kernel PDEs */
        for (i = 0; i < 3; i++)
        {
            PageDirPae->Pde[KernelPageTableIndex + i].Valid = 1;
            PageDirPae->Pde[KernelPageTableIndex + i].Write = 1;
            PageDirPae->Pde[KernelPageTableIndex + i].PageFrameNumber = PaPtrToPfn(kernel_pagetable_pae) + i;
        }
    
        /* Set up the Startup PDE */
        for (i = 0; i < 4; i++)
        {
            PageDirPae->Pde[StartupPageTableIndexPae + i].Valid = 1;
            PageDirPae->Pde[StartupPageTableIndexPae + i].Write = 1;
            PageDirPae->Pde[StartupPageTableIndexPae + i].PageFrameNumber = PaPtrToPfn(startup_pagedirectory_pae) + i;
        }
    
        /* Set up the Hyperspace PDE */
        for (i = 0; i < 2; i++)
        {
            PageDirPae->Pde[HyperspacePageTableIndexPae + i].Valid = 1;
            PageDirPae->Pde[HyperspacePageTableIndexPae + i].Write = 1;
            PageDirPae->Pde[HyperspacePageTableIndexPae + i].PageFrameNumber = PaPtrToPfn(hyperspace_pagetable_pae) + i;
        }
    
        /* Set up the Apic PDE */   
        for (i = 0; i < 2; i++)
        {
            PageDirPae->Pde[ApicPageTableIndexPae + i].Valid = 1;
            PageDirPae->Pde[ApicPageTableIndexPae + i].Write = 1;
            PageDirPae->Pde[ApicPageTableIndexPae + i].PageFrameNumber = PaPtrToPfn(apic_pagetable_pae) + i;
        }
        
        /* Set up the KPCR PDE */
        PageDirPae->Pde[KpcrPageTableIndexPae].Valid = 1;
        PageDirPae->Pde[KpcrPageTableIndexPae].Write = 1;
        PageDirPae->Pde[KpcrPageTableIndexPae].PageFrameNumber = PaPtrToPfn(kpcr_pagetable_pae);
    
        /* Set up Low Memory PTEs */
        PageDirPae = (PPAGE_DIRECTORY_X64)&lowmem_pagetable_pae;
        for (i=0; i<1024; i++) {
        
            PageDirPae->Pde[i].Valid = 1;
            PageDirPae->Pde[i].Write = 1;
            PageDirPae->Pde[i].Owner = 1;
            PageDirPae->Pde[i].PageFrameNumber = i;
        }
            
        /* Set up Kernel PTEs */
        PageDirPae = (PPAGE_DIRECTORY_X64)&kernel_pagetable_pae;
        for (i=0; i<1536; i++) {
        
            PageDirPae->Pde[i].Valid = 1;
            PageDirPae->Pde[i].Write = 1;
            PageDirPae->Pde[i].PageFrameNumber = PaToPfn(KERNEL_BASE_PHYS) + i;
        }
        
        /* Set up APIC PTEs */
        PageDirPae = (PPAGE_DIRECTORY_X64)&apic_pagetable_pae;
        PageDirPae->Pde[0].Valid = 1;
        PageDirPae->Pde[0].Write = 1;
        PageDirPae->Pde[0].CacheDisable = 1;
        PageDirPae->Pde[0].WriteThrough = 1;
        PageDirPae->Pde[0].PageFrameNumber = PaToPfn(APIC_BASE);
        PageDirPae->Pde[0x200].Valid = 1;
        PageDirPae->Pde[0x200].Write = 1;
        PageDirPae->Pde[0x200].CacheDisable = 1;
        PageDirPae->Pde[0x200].WriteThrough = 1;
        PageDirPae->Pde[0x200].PageFrameNumber = PaToPfn(APIC_BASE + KERNEL_BASE_PHYS);
    
        /* Set up KPCR PTEs */
        PageDirPae = (PPAGE_DIRECTORY_X64)&kpcr_pagetable_pae;
        PageDirPae->Pde[0].Valid = 1;
        PageDirPae->Pde[0].Write = 1;
        PageDirPae->Pde[0].PageFrameNumber = 1;      
    
    } else {
            
        /* Get the Kernel Table Index */
        KernelPageTableIndex = (KernelBase >> PDE_SHIFT) / sizeof(HARDWARE_PTE_X86);
        
        /* Get the Startup Page Directory */
        PageDir = (PPAGE_DIRECTORY_X86)&startup_pagedirectory;
        
        /* Set up the Low Memory PDE */
        PageDir->Pde[LowMemPageTableIndex].Valid = 1;
        PageDir->Pde[LowMemPageTableIndex].Write = 1;
        PageDir->Pde[LowMemPageTableIndex].PageFrameNumber = PaPtrToPfn(lowmem_pagetable);
    
        /* Set up the Kernel PDEs */
        PageDir->Pde[KernelPageTableIndex].Valid = 1;
        PageDir->Pde[KernelPageTableIndex].Write = 1;
        PageDir->Pde[KernelPageTableIndex].PageFrameNumber = PaPtrToPfn(kernel_pagetable);
        PageDir->Pde[KernelPageTableIndex + 1].Valid = 1;
        PageDir->Pde[KernelPageTableIndex + 1].Write = 1;
        PageDir->Pde[KernelPageTableIndex + 1].PageFrameNumber = PaPtrToPfn(kernel_pagetable + 4096);
        
        /* Set up the Startup PDE */
        PageDir->Pde[StartupPageTableIndex].Valid = 1;
        PageDir->Pde[StartupPageTableIndex].Write = 1;
        PageDir->Pde[StartupPageTableIndex].PageFrameNumber = PaPtrToPfn(startup_pagedirectory);
        
        /* Set up the Hyperspace PDE */
        PageDir->Pde[HyperspacePageTableIndex].Valid = 1;
        PageDir->Pde[HyperspacePageTableIndex].Write = 1;
        PageDir->Pde[HyperspacePageTableIndex].PageFrameNumber = PaPtrToPfn(hyperspace_pagetable);
    
        /* Set up the Apic PDE */   
        PageDir->Pde[ApicPageTableIndex].Valid = 1;
        PageDir->Pde[ApicPageTableIndex].Write = 1;
        PageDir->Pde[ApicPageTableIndex].PageFrameNumber = PaPtrToPfn(apic_pagetable);
        
        /* Set up the KPCR PDE */
        PageDir->Pde[KpcrPageTableIndex].Valid = 1;
        PageDir->Pde[KpcrPageTableIndex].Write = 1;
        PageDir->Pde[KpcrPageTableIndex].PageFrameNumber = PaPtrToPfn(kpcr_pagetable);
    
        /* Set up Low Memory PTEs */
        PageDir = (PPAGE_DIRECTORY_X86)&lowmem_pagetable;
        for (i=0; i<1024; i++) {
        
            PageDir->Pde[i].Valid = 1;
            PageDir->Pde[i].Write = 1;
            PageDir->Pde[i].Owner = 1;
            PageDir->Pde[i].PageFrameNumber = PaToPfn(i * PAGE_SIZE);
        }
        
        /* Set up Kernel PTEs */
        PageDir = (PPAGE_DIRECTORY_X86)&kernel_pagetable;
        for (i=0; i<1536; i++) {
        
            PageDir->Pde[i].Valid = 1;
            PageDir->Pde[i].Write = 1;
            PageDir->Pde[i].PageFrameNumber = PaToPfn(KERNEL_BASE_PHYS + i * PAGE_SIZE);
        }
        
        /* Set up APIC PTEs */
        PageDir = (PPAGE_DIRECTORY_X86)&apic_pagetable;
        PageDir->Pde[0].Valid = 1;
        PageDir->Pde[0].Write = 1;
        PageDir->Pde[0].CacheDisable = 1;
        PageDir->Pde[0].WriteThrough = 1;
        PageDir->Pde[0].PageFrameNumber = PaToPfn(APIC_BASE);
        PageDir->Pde[0x200].Valid = 1;
        PageDir->Pde[0x200].Write = 1;
        PageDir->Pde[0x200].CacheDisable = 1;
        PageDir->Pde[0x200].WriteThrough = 1;
        PageDir->Pde[0x200].PageFrameNumber = PaToPfn(APIC_BASE + KERNEL_BASE_PHYS);
    
        /* Set up KPCR PTEs */
        PageDir = (PPAGE_DIRECTORY_X86)&kpcr_pagetable;
        PageDir->Pde[0].Valid = 1;
        PageDir->Pde[0].Write = 1;
        PageDir->Pde[0].PageFrameNumber = 1;      
    }
    return;
#endif
}

/*++
 * FrLdrMapKernel 
 * INTERNAL
 *
 *     Maps the Kernel into memory, does PE Section Mapping, initalizes the
 *     uninitialized data sections, and relocates the image.
 *
 * Params:
 *     KernelImage - FILE Structure representing the ntoskrnl image file.
 *
 * Returns:
 *     TRUE if the Kernel was mapped.
 *
 * Remarks:
 *     None.
 *
 *--*/
BOOLEAN
NTAPI
FrLdrMapKernel(FILE *KernelImage)
{
#if 0
    PIMAGE_DOS_HEADER ImageHeader;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_SECTION_HEADER Section;
    ULONG SectionCount;
    ULONG ImageSize;
    ULONG_PTR SourceSection;
    ULONG_PTR TargetSection;
    ULONG SectionSize;
    LONG i;
    PIMAGE_DATA_DIRECTORY RelocationDDir;
    PIMAGE_BASE_RELOCATION RelocationDir, RelocationEnd;
    ULONG Count;
    ULONG_PTR Address, MaxAddress;
    PUSHORT TypeOffset;
    ULONG_PTR Delta;
    PUSHORT ShortPtr;
    PULONG LongPtr;

    /* Allocate 1024 bytes for PE Header */
    ImageHeader = (PIMAGE_DOS_HEADER)MmAllocateMemory(1024);
    
    /* Make sure it was succesful */
    if (ImageHeader == NULL) {
        
        return FALSE;
    }

    /* Load the first 1024 bytes of the kernel image so we can read the PE header */
    if (!FsReadFile(KernelImage, 1024, NULL, ImageHeader)) {

        /* Fail if we couldn't read */
        MmFreeMemory(ImageHeader);
        return FALSE;
    }

    /* Now read the MZ header to get the offset to the PE Header */
    NtHeader = (PIMAGE_NT_HEADERS)((PCHAR)ImageHeader + ImageHeader->e_lfanew);
     
    /* Get Kernel Base */
    KernelBase = NtHeader->OptionalHeader.ImageBase;  
    FrLdrGetKernelBase();
    
    /* Save Entrypoint */
    KernelEntry = RaToPa(NtHeader->OptionalHeader.AddressOfEntryPoint);
    
    /* Save the Image Size */
    ImageSize = NtHeader->OptionalHeader.SizeOfImage;
     
    /* Free the Header */
    MmFreeMemory(ImageHeader);

    /* Set the file pointer to zero */
    FsSetFilePointer(KernelImage, 0);

    /* Load the file image */
    FsReadFile(KernelImage, ImageSize, NULL, (PVOID)KERNEL_BASE_PHYS);
    
    /* Reload the NT Header */
    NtHeader = (PIMAGE_NT_HEADERS)((PCHAR)KERNEL_BASE_PHYS + ImageHeader->e_lfanew);
    
    /* Load the first section */
    Section = IMAGE_FIRST_SECTION(NtHeader);
    SectionCount = NtHeader->FileHeader.NumberOfSections - 1;
    
    /* Now go to the last section */
    Section += SectionCount;
    
    /* Walk each section backwards */    
    for (i=SectionCount; i >= 0; i--, Section--) {
    
        /* Get the disk location and the memory location, and the size */          
        SourceSection = RaToPa(Section->PointerToRawData);
        TargetSection = RaToPa(Section->VirtualAddress);
        SectionSize = Section->SizeOfRawData;
   
        /* If the section is already mapped correctly, go to the next */
        if (SourceSection == TargetSection) continue;
        
        /* Load it into memory */
        memmove((PVOID)TargetSection, (PVOID)SourceSection, SectionSize);
        
        /* Check for unitilizated data */
        if (Section->SizeOfRawData < Section->Misc.VirtualSize) {
            
            /* Zero it out */
            memset((PVOID)RaToPa(Section->VirtualAddress + Section->SizeOfRawData), 
                   0,
                   Section->Misc.VirtualSize - Section->SizeOfRawData);
        }
    }
       
    /* Get the Relocation Data Directory */
    RelocationDDir = &NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    
    /* Get the Relocation Section Start and End*/
    RelocationDir = (PIMAGE_BASE_RELOCATION)(KERNEL_BASE_PHYS + RelocationDDir->VirtualAddress);
    RelocationEnd = (PIMAGE_BASE_RELOCATION)((ULONG_PTR)RelocationDir + RelocationDDir->Size);
   
    /* Calculate Difference between Real Base and Compiled Base*/
    Delta = KernelBase - NtHeader->OptionalHeader.ImageBase;;
    
    /* Determine how far we shoudl relocate */
    MaxAddress = KERNEL_BASE_PHYS + ImageSize;
    
    /* Relocate until we've processed all the blocks */
    while (RelocationDir < RelocationEnd && RelocationDir->SizeOfBlock > 0) {
        
        /* See how many Relocation Blocks we have */
        Count = (RelocationDir->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(USHORT);
        
        /* Calculate the Address of this Directory */
        Address = KERNEL_BASE_PHYS + RelocationDir->VirtualAddress;
        
        /* Calculate the Offset of the Type */
        TypeOffset = (PUSHORT)(RelocationDir + 1);

        for (i = 0; i < Count; i++) {
            
            ShortPtr = (PUSHORT)(Address + (*TypeOffset & 0xFFF));

            /* Don't relocate after the end of the loaded driver */
            if ((ULONG_PTR)ShortPtr >= MaxAddress) break;

            switch (*TypeOffset >> 12) {
                
                case IMAGE_REL_BASED_ABSOLUTE:
                    break;

                case IMAGE_REL_BASED_HIGH:
                    *ShortPtr += HIWORD(Delta);
                    break;

                case IMAGE_REL_BASED_LOW:
                    *ShortPtr += LOWORD(Delta);
                    break;

                case IMAGE_REL_BASED_HIGHLOW:
                    LongPtr = (PULONG)ShortPtr;
                    *LongPtr += Delta;
                    break;
            }
            
            TypeOffset++;
        }
        
        /* Move to the next Relocation Table */
        RelocationDir = (PIMAGE_BASE_RELOCATION)((ULONG_PTR)RelocationDir + RelocationDir->SizeOfBlock);
    }
    
    /* Increase the next Load Base */
    NextModuleBase = ROUND_UP(KERNEL_BASE_PHYS + ImageSize, PAGE_SIZE);

    /* Return Success */
#endif
    return TRUE;
}

ULONG_PTR
NTAPI
FrLdrLoadModule(FILE *ModuleImage, 
                LPCSTR ModuleName, 
                PULONG ModuleSize)
{
#if 0
    ULONG LocalModuleSize;
    PFRLDR_MODULE ModuleData;
    LPSTR NameBuffer;
    LPSTR TempName;

    /* Get current module data structure and module name string array */
    ModuleData = &multiboot_modules[LoaderBlock.ModsCount];
    
    /* Get only the Module Name */
    do {
        
        TempName = strchr(ModuleName, '\\');
        
        if(TempName) {
            ModuleName = TempName + 1;
        }

    } while(TempName);
    NameBuffer = multiboot_module_strings[LoaderBlock.ModsCount];

    /* Get Module Size */
    LocalModuleSize = FsGetFileSize(ModuleImage);
    
    /* Fill out Module Data Structure */
    ModuleData->ModuleStart = NextModuleBase;
    ModuleData->ModuleEnd = NextModuleBase + LocalModuleSize;
    
    /* Save name */
    strcpy(NameBuffer, ModuleName);
    ModuleData->ModuleName = NameBuffer;

    /* Load the file image */
    FsReadFile(ModuleImage, LocalModuleSize, NULL, (PVOID)NextModuleBase);

    /* Move to next memory block and increase Module Count */
    NextModuleBase = ROUND_UP(ModuleData->ModuleEnd, PAGE_SIZE);
    LoaderBlock.ModsCount++;

    /* Return Module Size if required */
    if (ModuleSize != NULL) {
        *ModuleSize = LocalModuleSize;
    }

    return(ModuleData->ModuleStart);
#else
    return 0;
#endif
}

ULONG_PTR
NTAPI
FrLdrCreateModule(LPCSTR ModuleName)
{
#if 0
    PFRLDR_MODULE ModuleData;
    LPSTR NameBuffer;

    /* Get current module data structure and module name string array */
    ModuleData = &multiboot_modules[LoaderBlock.ModsCount];
    NameBuffer = multiboot_module_strings[LoaderBlock.ModsCount];

    /* Set up the structure */
    ModuleData->ModuleStart = NextModuleBase;
    ModuleData->ModuleEnd = -1;
    
    /* Copy the name */
    strcpy(NameBuffer, ModuleName);
    ModuleData->ModuleName = NameBuffer;

    /* Set the current Module */
    CurrentModule = ModuleData;

    /* Return Module Base Address */
    return(ModuleData->ModuleStart);
#else
    return 0;
#endif
}

BOOLEAN
NTAPI
FrLdrCloseModule(ULONG_PTR ModuleBase, 
                 ULONG ModuleSize)
{
#if 0
    PFRLDR_MODULE ModuleData = CurrentModule;

    /* Make sure a module is opened */
    if (ModuleData) {
    
        /* Make sure this is the right module and that it hasn't been closed */
        if ((ModuleBase == ModuleData->ModuleStart) && (ModuleData->ModuleEnd == -1)) {
        
            /* Close the Module */
            ModuleData->ModuleEnd = ModuleData->ModuleStart + ModuleSize;

            /* Set the next Module Base and increase the number of modules */
            NextModuleBase = ROUND_UP(ModuleData->ModuleEnd, PAGE_SIZE);
            LoaderBlock.ModsCount++;
            
            /* Close the currently opened module */
            CurrentModule = NULL;

            /* Success */
            return(TRUE);
        }
    }

    /* Failure path */
#endif
    return(FALSE);
}
