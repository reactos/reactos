/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Freeloader
 * FILE:            boot/freeldr/freeldr/multiboot.c
 * PURPOSE:         ReactOS Loader
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Hartmut Birr - SMP/PAE Code
 */

#include <freeldr.h>
#include <internal/i386/ke.h>

#define NDEBUG
#include <debug.h>


/* Converts a Relative Address read from the Kernel into a Physical Address */
#define RaToPa(p) \
    (ULONG_PTR)((ULONG_PTR)p + KERNEL_BASE_PHYS)


/* Load Address of Next Module */
ULONG_PTR NextModuleBase = 0;

/* Currently Opened Module */
PFRLDR_MODULE CurrentModule = NULL;

/* Unrelocated Kernel Base in Virtual Memory */
ULONG_PTR KernelBase;

/* Kernel Entrypoint in Physical Memory */
ULONG_PTR KernelEntry;


/* FUNCTIONS *****************************************************************/

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
BOOL
STDCALL
FrLdrMapKernel(FILE *KernelImage)
{
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
    Delta = KernelBase - NtHeader->OptionalHeader.ImageBase;

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
    return TRUE;
}

ULONG_PTR
STDCALL
FrLdrLoadModule(FILE *ModuleImage,
                LPSTR ModuleName,
                PULONG ModuleSize)
{
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
}

ULONG_PTR
STDCALL
FrLdrCreateModule(LPSTR ModuleName)
{
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
}

BOOL
STDCALL
FrLdrCloseModule(ULONG_PTR ModuleBase,
                 ULONG ModuleSize)
{
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
    return(FALSE);
}
