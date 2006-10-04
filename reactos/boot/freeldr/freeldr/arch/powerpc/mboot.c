/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
 *  Copyright (C) 2005       Alex Ionescu  <alex@relsoft.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <freeldr.h>
#include <of_call.h>
#include "ppcboot.h"
#include "mmu.h"
#include "compat.h"

#define NDEBUG
#include <debug.h>

static PVOID KernelMemory = 0;
extern boot_infos_t BootInfo;

/* Bits to shift to convert a Virtual Address into an Offset in the Page Table */
#define PFN_SHIFT 12

/* Bits to shift to convert a Virtual Address into an Offset in the Page Directory */
#define PDE_SHIFT 22
#define PDE_SHIFT_PAE 18


/* Converts a Relative Address read from the Kernel into a Physical Address */
ULONG RaToPa(ULONG p) {
    return (ULONG)(KernelMemory) + p;
}

/* Converts a Phsyical Address Pointer into a Page Frame Number */
#define PaPtrToPfn(p) \
    (((ULONG_PTR)&p) >> PFN_SHIFT)

/* Converts a Phsyical Address into a Page Frame Number */
#define PaToPfn(p) \
    ((p) >> PFN_SHIFT)

#define STARTUP_BASE                0xC0000000
#define HYPERSPACE_BASE             0xC0400000
#define HYPERSPACE_PAE_BASE         0xC0800000
#define APIC_BASE                   0xFEC00000
#define KPCR_BASE                   0xFF000000

#define LowMemPageTableIndex        0
#define StartupPageTableIndex       (STARTUP_BASE >> 22)
#define HyperspacePageTableIndex    (HYPERSPACE_BASE >> 22)
#define KpcrPageTableIndex          (KPCR_BASE >> 22)
#define ApicPageTableIndex          (APIC_BASE >> 22)

#define LowMemPageTableIndexPae     0
#define StartupPageTableIndexPae    (STARTUP_BASE >> 21)
#define HyperspacePageTableIndexPae (HYPERSPACE_PAE_BASE >> 21)
#define KpcrPageTableIndexPae       (KPCR_BASE >> 21)
#define ApicPageTableIndexPae       (APIC_BASE >> 21)


#define BAT_GRANULARITY             (64 * 1024)
#define KernelMemorySize            (16 * 1024 * 1024)
#define KernelEntryPoint            (KernelEntry - KERNEL_BASE_PHYS) + KernelBase

/* Load Address of Next Module */
ULONG_PTR NextModuleBase = 0;

/* Currently Opened Module */
PLOADER_MODULE CurrentModule = NULL;

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

typedef void (*KernelEntryFn)( void * );

VOID
DrawDigit(boot_infos_t *BootInfo, ULONG Digit, int x, int y)
{
    int i,j,k;

    for( i = 0; i < 7; i++ ) {
	for( j = 0; j < 8; j++ ) {
	    for( k = 0; k < BootInfo->dispDeviceDepth/8; k++ ) {
		SetPhysByte(((ULONG_PTR)BootInfo->dispDeviceBase)+
			    k +
			    (((j+x) * (BootInfo->dispDeviceDepth/8)) +
			     ((i+y) * (BootInfo->dispDeviceRowBytes))),
			    BootInfo->dispFont[Digit][i*8+j] == ' ' ? 0 : 255);
	    }
	}
    }
}

VOID
DrawNumber(boot_infos_t *BootInfo, ULONG Number, int x, int y)
{
    int i;

    for( i = 0; i < 8; i++, Number<<=4 ) {
	DrawDigit(BootInfo,(Number>>28)&0xf,x+(i*8),y);
    }
}

VOID
NTAPI
FrLdrStartup(ULONG Magic)
{
    KernelEntryFn KernelEntryAddress = 
	(KernelEntryFn)(KernelEntry + KernelBase);
    register ULONG_PTR i asm("r4"), j asm("r5");
    UINT NeededPTE = ((UINT)KernelMemory) >> PFN_SHIFT;
    UINT NeededMemory = 
	(2 * sizeof(ULONG_PTR) * (NeededPTE + NeededPTE / 512)) + 
	sizeof(BootInfo) + sizeof(BootDigits);
    UINT NeededPages = ROUND_UP(NeededMemory,(1<<PFN_SHIFT));
    register PULONG_PTR TranslationMap asm("r6") = 
	MmAllocateMemory(NeededMemory);
    boot_infos_t *LocalBootInfo = (boot_infos_t *)TranslationMap;
    TranslationMap = (PULONG_PTR)
	(((PCHAR)&LocalBootInfo[1]) + sizeof(BootDigits));
    memcpy(&LocalBootInfo[1], BootDigits, sizeof(BootDigits));
    *LocalBootInfo = BootInfo;
    LocalBootInfo->dispFont = (font_char *)&LocalBootInfo[1];
    
    TranslationMap[0] = (ULONG_PTR)FrLdrStartup;
    for( i = 1; i < NeededPages; i++ )
    {
	TranslationMap[i*2] = NeededMemory+(i<<PFN_SHIFT);
    }

    for( j = 0; j < KernelMemorySize>>PFN_SHIFT; j++ )
    {
	TranslationMap[(i+j)*2] = ((UINT)(KernelMemory+(i<<PFN_SHIFT)));
    }

    for( i = 0; i < j; i++ ) 
    {
	TranslationMap[(i*2)+1] = PpcVirt2phys(TranslationMap[i*2],0);
    }

    printf("Built map of %d pages\n", j);

    /* 
     * Stuff page table entries for the page containing this code,
     * The pages containing the page table entries, and finally the kernel
     * pages.
     * 
     * When done, we'll be able to flop over to kernel land! 
     */
    for( i = 0; i < j; i++ )
    {
	DrawNumber(LocalBootInfo,i,10,90);
	DrawNumber(LocalBootInfo,TranslationMap[i*2],10,100);
	DrawNumber(LocalBootInfo,TranslationMap[i*2+1],10,110);

	InsertPageEntry
	    (TranslationMap[i*2],
	     TranslationMap[(i*2)+1],
	     (i>>10));
    }

    /* Tell them we're booting */
    DrawNumber(LocalBootInfo,0x1cabba9e,10,120);
    DrawNumber(LocalBootInfo,(ULONG)KernelEntryAddress,100,120);

    KernelEntryAddress( (void*)LocalBootInfo );
    while(1);
}

/*++
 * FrLdrSetupPae
 * INTERNAL
 *
 *     Configures PAE on a MP System, and sets the PDBR if it's supported, or if
 *     the system is UP.
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
FASTCALL
FrLdrSetupPae(ULONG Magic)
{
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
static VOID
FASTCALL
FrLdrGetKernelBase(VOID)
{
    PCHAR p;

    /* Set KernelBase */
    LoaderBlock.KernelBase = KernelBase;

    /* Read Command Line */
    p = (PCHAR)LoaderBlock.CommandLine;
    while ((p = strchr(p, '/')) != NULL) {

        /* Find "/3GB" */
        if (!_strnicmp(p + 1, "3GB", 3)) {

            /* Make sure there's nothing following it */
            if (p[4] == ' ' || p[4] == 0) {

                /* Use 3GB */
                KernelBase = 0xE0000000;
                LoaderBlock.KernelBase = 0xC0000000;
            }
        }

        p++;
    }
}

/*++
 * FrLdrGetPaeMode
 * INTERNAL
 *
 *     Determines whether PAE mode shoudl be enabled or not.
 *
 * Params:
 *     None.
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     None.
 *
 *--*/
VOID
FASTCALL
FrLdrGetPaeMode(VOID)
{
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
    PIMAGE_DOS_HEADER ImageHeader;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_SECTION_HEADER Section;
    ULONG SectionCount;
    ULONG ImageSize;
    ULONG_PTR SourceSection;
    ULONG_PTR TargetSection;
    ULONG SectionSize;
    INT i;
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

    /* Save the Image Size */
    ImageSize = NtHeader->OptionalHeader.SizeOfImage;

    /* Free the Header */
    MmFreeMemory(ImageHeader);

    /* Set the file pointer to zero */
    FsSetFilePointer(KernelImage, 0);

    /* Allocate kernel memory */
    KernelMemory = MmAllocateMemory(KernelMemorySize);

    printf("Kernel Memory @%x\n", (int)KernelMemory);

    /* Save Entrypoint */
    KernelEntry = NtHeader->OptionalHeader.AddressOfEntryPoint;

    /* Load the file image */
    FsReadFile(KernelImage, ImageSize, NULL, KernelMemory);

    /* Reload the NT Header */
    NtHeader = (PIMAGE_NT_HEADERS)((PCHAR)KernelMemory + ImageHeader->e_lfanew);

    /* Load the first section */
    Section = IMAGE_FIRST_SECTION(NtHeader);
    SectionCount = NtHeader->FileHeader.NumberOfSections - 1;

    /* Now go to the last section */
    Section += SectionCount;

    /* Walk each section backwards */
    for (i=(INT)SectionCount; i >= 0; i--, Section--) {

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
    RelocationDir = (PIMAGE_BASE_RELOCATION)(KernelMemory + RelocationDDir->VirtualAddress);
    RelocationEnd = (PIMAGE_BASE_RELOCATION)((ULONG_PTR)RelocationDir + RelocationDDir->Size);

    /* Calculate Difference between Real Base and Compiled Base*/
    Delta = KernelBase - NtHeader->OptionalHeader.ImageBase;

    /* Determine how far we shoudl relocate */
    MaxAddress = (ULONG)KernelMemory + ImageSize;

    /* Relocate until we've processed all the blocks */
    while (RelocationDir < RelocationEnd && RelocationDir->SizeOfBlock > 0) {

        /* See how many Relocation Blocks we have */
        Count = (RelocationDir->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(USHORT);

        /* Calculate the Address of this Directory */
        Address = (ULONG)KernelMemory + RelocationDir->VirtualAddress;

        /* Calculate the Offset of the Type */
        TypeOffset = (PUSHORT)(RelocationDir + 1);

        for (i = 0; i < (INT)Count; i++) {

            ShortPtr = (PUSHORT)(Address + (*TypeOffset & 0xFFF));

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
    NextModuleBase = ROUND_UP((ULONG)KernelMemory + ImageSize, PAGE_SIZE);

    /* Return Success */
    return TRUE;
}

ULONG_PTR
NTAPI
FrLdrLoadModule(FILE *ModuleImage,
                LPCSTR ModuleName,
                PULONG ModuleSize)
{
    ULONG LocalModuleSize;
    PLOADER_MODULE ModuleData;
    LPSTR NameBuffer;
    LPSTR TempName;

    /* Get current module data structure and module name string array */
    ModuleData = &reactos_modules[LoaderBlock.ModsCount];

    /* Get only the Module Name */
    do {

        TempName = strchr(ModuleName, '\\');

        if(TempName) {
            ModuleName = TempName + 1;
        }

    } while(TempName);
    NameBuffer = reactos_module_strings[LoaderBlock.ModsCount];

    /* Get Module Size */
    LocalModuleSize = FsGetFileSize(ModuleImage);

    /* Fill out Module Data Structure */
    ModuleData->ModStart = NextModuleBase;
    ModuleData->ModEnd = NextModuleBase + LocalModuleSize;

    /* Save name */
    strcpy(NameBuffer, ModuleName);
    ModuleData->String = (ULONG_PTR)NameBuffer;

    /* Load the file image */
    FsReadFile(ModuleImage, LocalModuleSize, NULL, (PVOID)NextModuleBase);

    /* Move to next memory block and increase Module Count */
    NextModuleBase = ROUND_UP(ModuleData->ModEnd, PAGE_SIZE);
    LoaderBlock.ModsCount++;

    /* Return Module Size if required */
    if (ModuleSize != NULL) {
        *ModuleSize = LocalModuleSize;
    }

    return(ModuleData->ModStart);
}

ULONG_PTR
NTAPI
FrLdrCreateModule(LPCSTR ModuleName)
{
    PLOADER_MODULE ModuleData;
    LPSTR NameBuffer;

    /* Get current module data structure and module name string array */
    ModuleData = &reactos_modules[LoaderBlock.ModsCount];
    NameBuffer = reactos_module_strings[LoaderBlock.ModsCount];

    /* Set up the structure */
    ModuleData->ModStart = NextModuleBase;
    ModuleData->ModEnd = -1;

    /* Copy the name */
    strcpy(NameBuffer, ModuleName);
    ModuleData->String = (ULONG_PTR)NameBuffer;

    /* Set the current Module */
    CurrentModule = ModuleData;

    /* Return Module Base Address */
    return(ModuleData->ModStart);
}

BOOLEAN
NTAPI
FrLdrCloseModule(ULONG_PTR ModuleBase,
                 ULONG ModuleSize)
{
    PLOADER_MODULE ModuleData = CurrentModule;

    /* Make sure a module is opened */
    if (ModuleData) {

        /* Make sure this is the right module and that it hasn't been closed */
        if ((ModuleBase == ModuleData->ModStart) && (ModuleData->ModEnd == (ULONG_PTR)-1)) {

            /* Close the Module */
            ModuleData->ModEnd = ModuleData->ModStart + ModuleSize;

            /* Set the next Module Base and increase the number of modules */
            NextModuleBase = ROUND_UP(ModuleData->ModEnd, PAGE_SIZE);
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
