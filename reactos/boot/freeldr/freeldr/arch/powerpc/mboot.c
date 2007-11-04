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
#include <elf/elf.h>
#include <elf/reactos.h>
#include <of_call.h>
#include "ppcboot.h"
#include "ppcmmu/mmu.h"
#include "compat.h"
#include "ppcfont.h"

#define NDEBUG
#include <debug.h>

PVOID KernelMemory = 0;
extern boot_infos_t BootInfo;

/* Bits to shift to convert a Virtual Address into an Offset in the Page Table */
#define PFN_SHIFT 12

/* Bits to shift to convert a Virtual Address into an Offset in the Page Directory */
#define PDE_SHIFT 22
#define PDE_SHIFT_PAE 18

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
#define KernelMemorySize            (8 * 1024 * 1024)
#define XROUNDUP(x,n)               ((((ULONG)x) + ((n) - 1)) & (~((n) - 1)))

/* Load Address of Next Module */
ULONG_PTR NextModuleBase = 0;

/* Currently Opened Module */
PLOADER_MODULE CurrentModule = NULL;

/* Unrelocated Kernel Base in Virtual Memory */
ULONG_PTR KernelBase;

/* Wether PAE is to be used or not */
BOOLEAN PaeModeEnabled;

/* Kernel Entrypoint in Physical Memory */
ULONG_PTR KernelEntryPoint;

/* Dummy to bring in memmove */
PVOID memmove_dummy = memmove;

PLOADER_MODULE
NTAPI
LdrGetModuleObject(PCHAR ModuleName);

NTSTATUS
NTAPI
LdrPEFixupImports(IN PVOID DllBase,
                  IN PCHAR DllName);

VOID PpcInitializeMmu(int max);

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

int MmuPageMiss(int trapCode, ppc_trap_frame_t *trap)
{
    int i;
    printf("TRAP %x\n", trapCode);
    for( i = 0; i < 40; i++ )
	printf("r[%d] %x\n", i, trap->gpr[i]);
    printf("HALT!\n");
    while(1);
}

typedef struct _ppc_map_set_t {
    int mapsize;
    int usecount;
    ppc_map_info_t *info;
} ppc_map_set_t;

VOID
NTAPI
FrLdrAddPageMapping(ppc_map_set_t *set, int proc, paddr_t phys, vaddr_t virt)
{
    int j;
    paddr_t page = ROUND_DOWN(phys, (1<<PFN_SHIFT));
    if (virt == 0)
        virt = page;
    else
        virt = ROUND_DOWN(virt, (1<<PFN_SHIFT));

    for( j = 0; j < set->usecount; j++ )
    {
        if(set->info[j].addr == page) return;
    }

    if (!set->mapsize)
    {
        set->mapsize = 0x80;
        set->info = MmAllocateMemory(0x80 * sizeof(*set->info));
    }
    else if (set->mapsize <= set->usecount)
    {
        ppc_map_info_t *newinfo = MmAllocateMemory(set->mapsize * 2 * sizeof(*set->info));
        memcpy(newinfo, set->info, set->mapsize * sizeof(*set->info));
        MmFreeMemory(set->info);
        set->info = newinfo;
        set->mapsize *= 2;
    }
    
    set->info[set->usecount].flags = MMU_ALL_RW;
    set->info[set->usecount].proc = proc;
    set->info[set->usecount].addr = virt;
    set->info[set->usecount].phys = page;
    set->usecount++;
}

VOID
NTAPI
FrLdrStartup(ULONG Magic)
{
    ULONG_PTR i, tmp;
    PCHAR ModHeader;
    boot_infos_t *LocalBootInfo = &BootInfo;
    LocalBootInfo->dispFont = (font_char *)&LocalBootInfo[1];
    LoaderBlock.ArchExtra = (ULONG)LocalBootInfo;
    ppc_map_set_t memmap = { };

    for(i = 0; i < LoaderBlock.ModsCount; i++)
    {
	ModHeader = ((PCHAR)reactos_modules[i].ModStart);
	if(ModHeader[0] == 'M' && ModHeader[1] == 'Z')
	    LdrPEFixupImports
		((PVOID)reactos_modules[i].ModStart,
		 (PCHAR)reactos_modules[i].String);
        else /* Make RVA */
        {
            reactos_modules[i].ModStart -= (ULONG_PTR)KernelMemory;
            reactos_modules[i].ModEnd   -= (ULONG_PTR)KernelMemory;
        }
    }

    /* We don't use long longs, but longs for the addresses in the 
     * ADDRESS_RANGE structure.  Swap the quad halves of our memory
     * map.
     */
    for( i = 0; 
         i < reactos_memory_map_descriptor_size / sizeof(reactos_memory_map[0]); 
         i++ )
    {
        tmp = reactos_memory_map[i].base_addr_high;
        reactos_memory_map[i].base_addr_high = reactos_memory_map[i].base_addr_low;
        reactos_memory_map[i].base_addr_low = tmp;
        tmp = reactos_memory_map[i].length_high;
        reactos_memory_map[i].length_high = reactos_memory_map[i].length_low;
        reactos_memory_map[i].length_low = tmp;
    }

    PpcInitializeMmu(0);

    /* We'll use vsid 1 for freeldr (expendable) */
    MmuAllocVsid(1, 0xff);
    MmuSetVsid(0, 8, 1);

    MmuAllocVsid(0, 0xff00);
    MmuSetVsid(8, 16, 0);

    /* Map kernel space 0x80000000 ... */
    for( i = (ULONG)KernelMemory;
	 i < (ULONG)KernelMemory + KernelMemorySize;
	 i += (1<<PFN_SHIFT) ) {
        
        FrLdrAddPageMapping(&memmap, 0, i, KernelBase + i - (ULONG)KernelMemory);
    }

    /* Map module name strings */
    for( i = 0; i < LoaderBlock.ModsCount; i++ )
    {
        FrLdrAddPageMapping(&memmap, 1, (ULONG)reactos_modules[i].String, 0);
    }

    /* Map memory zones */
    FrLdrAddPageMapping(&memmap, 1, (vaddr_t)&reactos_memory_map_descriptor_size, 0);
    FrLdrAddPageMapping(&memmap, 1, (vaddr_t)&LoaderBlock, 0);

    MmuMapPage(memmap.info, memmap.usecount);

    MmuTurnOn((KernelEntryFn)KernelEntryPoint, (void*)&LoaderBlock);

    /* Nothing more */
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
VOID
FASTCALL
FrLdrGetKernelBase(VOID)
{
    PCHAR p;

    /* Default kernel base at 2GB */
    KernelBase = 0x80000000;

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
 * FrLdrMapModule
 * INTERNAL
 *
 *     Loads the indicated elf image as PE.  The target will appear to be
 *     a PE image whose ImageBase has ever been KernelAddr.
 *
 * Params:
 *     Image -- File to load
 *     ImageName -- Name of image for the modules list
 *     MemLoadAddr -- Freeldr address of module
 *     KernelAddr -- Kernel address of module
 *--*/
#define ELF_SECTION(n) ((Elf32_Shdr*)(sptr + (n * shsize)))
#define COFF_FIRST_SECTION(h) ((PIMAGE_SECTION_HEADER) ((DWORD)h+FIELD_OFFSET(IMAGE_NT_HEADERS,OptionalHeader)+(SWAPW(((PIMAGE_NT_HEADERS)(h))->FileHeader.SizeOfOptionalHeader))))

BOOLEAN
NTAPI
FrLdrMapModule(FILE *KernelImage, PCHAR ImageName, PCHAR MemLoadAddr, ULONG KernelAddr)
{
    PIMAGE_DOS_HEADER ImageHeader = 0;
    PIMAGE_NT_HEADERS NtHeader = 0;
    PIMAGE_SECTION_HEADER Section;
    ULONG SectionCount;
    ULONG ImageSize;
    INT i, j;
    PLOADER_MODULE ModuleData;
    int phsize, phnum, shsize, shnum, relsize, SectionAddr = 0;
    PCHAR sptr;
    Elf32_Ehdr ehdr;
    Elf32_Shdr *shdr;
    LPSTR TempName;

    TempName = strrchr(ImageName, '\\');
    if(TempName) TempName++; else TempName = (LPSTR)ImageName;
    ModuleData = LdrGetModuleObject(TempName);

    if(ModuleData)
    {
	return TRUE;
    }

    if(!KernelAddr)
	KernelAddr = (ULONG)NextModuleBase - (ULONG)KernelMemory + KernelBase;
    if(!MemLoadAddr)
	MemLoadAddr = (PCHAR)NextModuleBase;

    ModuleData = &reactos_modules[LoaderBlock.ModsCount];
    //printf("Loading file (elf at %x)\n", KernelAddr);

    /* Load the first 1024 bytes of the kernel image so we can read the PE header */
    if (!FsReadFile(KernelImage, sizeof(ehdr), NULL, &ehdr)) {

        /* Fail if we couldn't read */
	printf("Couldn't read the elf header\n");
        return FALSE;
    }

    /* Start by getting elf headers */
    phsize = ehdr.e_phentsize;
    phnum = ehdr.e_phnum;
    shsize = ehdr.e_shentsize;
    shnum = ehdr.e_shnum;
    sptr = (PCHAR)MmAllocateMemory(shnum * shsize);

    /* Read section headers */
    FsSetFilePointer(KernelImage,  ehdr.e_shoff);
    FsReadFile(KernelImage, shsize * shnum, NULL, sptr);

    /* Now we'll get the PE Header */
    for( i = 0; i < shnum; i++ )
    {
	shdr = ELF_SECTION(i);
	shdr->sh_addr = 0;

	/* Find the PE Header */
	if (shdr->sh_type == TYPE_PEHEADER)
	{
	    FsSetFilePointer(KernelImage, shdr->sh_offset);
	    FsReadFile(KernelImage, shdr->sh_size, NULL, MemLoadAddr);
	    ImageHeader = (PIMAGE_DOS_HEADER)MemLoadAddr;
	    NtHeader = (PIMAGE_NT_HEADERS)((PCHAR)MemLoadAddr + SWAPD(ImageHeader->e_lfanew));
#if 0
	    printf("NtHeader at %x\n", SWAPD(ImageHeader->e_lfanew));
	    printf("SectionAlignment %x\n",
		   SWAPD(NtHeader->OptionalHeader.SectionAlignment));
	    SectionAddr = ROUND_UP
		(shdr->sh_size, SWAPD(NtHeader->OptionalHeader.SectionAlignment));
	    printf("Header ends at %x\n", SectionAddr);
#endif
	    break;
	}
    }

    if(i == shnum)
    {
	printf("No peheader section encountered :-(\n");
	return 0;
    }
#if 0
    else
    {
        printf("DOS SIG: %s\n", (PCHAR)MemLoadAddr);
    }
#endif

    /* Save the Image Base */
    NtHeader->OptionalHeader.ImageBase = SWAPD(KernelAddr);

    /* Load the file image */
    Section = COFF_FIRST_SECTION(NtHeader);
    SectionCount = SWAPW(NtHeader->FileHeader.NumberOfSections);

    /* Walk each section */
    for (i=0; i < SectionCount; i++, Section++)
    {
	shdr = ELF_SECTION((SWAPD(Section->PointerToRawData)+1));

	shdr->sh_addr = SectionAddr = SWAPD(Section->VirtualAddress);
	shdr->sh_addr += KernelAddr;

	Section->PointerToRawData = SWAPD((Section->VirtualAddress - KernelAddr));

	if (shdr->sh_type != SHT_NOBITS)
	{
	    /* Content area */
	    printf("Loading section %d at %x (real: %x:%d)\n", i, KernelAddr + SectionAddr, MemLoadAddr+SectionAddr, shdr->sh_size);
	    FsSetFilePointer(KernelImage, shdr->sh_offset);
	    FsReadFile(KernelImage, shdr->sh_size, NULL, MemLoadAddr + SectionAddr);
	}
	else
	{
	    /* Zero it out */
	    printf("BSS section %d at %x\n", i, KernelAddr + SectionAddr);
	    memset(MemLoadAddr + SectionAddr, 0,
		   ROUND_UP(shdr->sh_size,
			    SWAPD(NtHeader->OptionalHeader.SectionAlignment)));
        }
    }

    ImageSize = SWAPD(NtHeader->OptionalHeader.SizeOfImage);
    printf("Total image size is %x\n", ImageSize);

    /* Handle relocation sections */
    for (i = 0; i < shnum; i++) {
	Elf32_Rela reloc = { };
	ULONG *Target32;
	USHORT *Target16;
	int numreloc, relstart, targetSection;
	Elf32_Sym symbol;
	PCHAR RelocSection, SymbolSection;

	shdr = ELF_SECTION(i);
	/* Only relocs here */
	if((shdr->sh_type != SHT_REL) &&
	   (shdr->sh_type != SHT_RELA)) continue;

	relstart = shdr->sh_offset;
	relsize = shdr->sh_type == SHT_RELA ? 12 : 8;
	numreloc = shdr->sh_size / relsize;
	targetSection = shdr->sh_info;

	if (!ELF_SECTION(targetSection)->sh_addr) continue;

	RelocSection = MmAllocateMemory(shdr->sh_size);
	FsSetFilePointer(KernelImage, relstart);
	FsReadFile(KernelImage, shdr->sh_size, NULL, RelocSection);

	/* Get the symbol section */
	shdr = ELF_SECTION(shdr->sh_link);

	SymbolSection = MmAllocateMemory(shdr->sh_size);
	FsSetFilePointer(KernelImage, shdr->sh_offset);
	FsReadFile(KernelImage, shdr->sh_size, NULL, SymbolSection);

	for(j = 0; j < numreloc; j++)
	{
	    ULONG S,A,P;

	    /* Get the reloc */
	    memcpy(&reloc, RelocSection + (j * relsize), sizeof(reloc));

	    /* Get the symbol */
	    memcpy(&symbol, SymbolSection + (ELF32_R_SYM(reloc.r_info) * sizeof(symbol)), sizeof(symbol));

	    /* Compute addends */
	    S = symbol.st_value + ELF_SECTION(symbol.st_shndx)->sh_addr;
	    A = reloc.r_addend;
	    P = reloc.r_offset + ELF_SECTION(targetSection)->sh_addr;

#if 0
	    printf("Symbol[%d] %d -> %d(%x:%x) -> %x(+%x)@%x\n",
		   ELF32_R_TYPE(reloc.r_info),
		   ELF32_R_SYM(reloc.r_info),
		   symbol.st_shndx,
		   ELF_SECTION(symbol.st_shndx)->sh_addr,
		   symbol.st_value,
		   S,
		   A,
		   P);
#endif

	    Target32 = (ULONG*)(((PCHAR)MemLoadAddr) + (P - KernelAddr));
	    Target16 = (USHORT *)Target32;

	    switch (ELF32_R_TYPE(reloc.r_info))
	    {
	    case R_PPC_NONE:
		break;
	    case R_PPC_ADDR32:
		*Target32 = S + A;
		break;
	    case R_PPC_REL32:
		*Target32 = S + A - P;
		break;
	    case R_PPC_UADDR32: /* Special: Treat as RVA */
		*Target32 = S + A - KernelAddr;
		break;
	    case R_PPC_ADDR24:
		*Target32 = (ADDR24_MASK & (S+A)) | (*Target32 & ~ADDR24_MASK);
		break;
	    case R_PPC_REL24:
		*Target32 = (ADDR24_MASK & (S+A-P)) | (*Target32 & ~ADDR24_MASK);
		break;
	    case R_PPC_ADDR16_LO:
		*Target16 = S + A;
		break;
	    case R_PPC_ADDR16_HA:
		*Target16 = (S + A + 0x8000) >> 16;
		break;
	    default:
		break;
	    }

#if 0
	    printf("reloc[%d:%x]: (type %x sym %d val %d) off %x add %x (old %x new %x)\n",
		   j,
		   ((ULONG)Target32) - ((ULONG)MemLoadAddr),
		   ELF32_R_TYPE(reloc.r_info),
		   ELF32_R_SYM(reloc.r_info),
		   symbol.st_value,
		   reloc.r_offset, reloc.r_addend,
		   x, *Target32);
#endif
	}

	MmFreeMemory(SymbolSection);
	MmFreeMemory(RelocSection);
    }

    MmFreeMemory(sptr);

    ModuleData->ModStart = (ULONG)MemLoadAddr;
    /* Increase the next Load Base */
    NextModuleBase = ROUND_UP((ULONG)MemLoadAddr + ImageSize, PAGE_SIZE);
    ModuleData->ModEnd = NextModuleBase;
    ModuleData->String = (ULONG)MmAllocateMemory(strlen(ImageName)+1);
    strcpy((PCHAR)ModuleData->String, ImageName);
#if 0
    printf("Module %s (%x-%x) next at %x\n",
	   ModuleData->String,
	   ModuleData->ModStart,
	   ModuleData->ModEnd,
	   NextModuleBase);
#endif
    LoaderBlock.ModsCount++;

    /* Return Success */
    return TRUE;
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
    /* Get Kernel Base */
    FrLdrGetKernelBase();

    /* Allocate kernel memory */
    KernelMemory = MmAllocateMemory(KernelMemorySize);

    return FrLdrMapModule(KernelImage, "ntoskrnl.exe", KernelMemory, KernelBase);
}

ULONG_PTR
NTAPI
FrLdrLoadModule(FILE *ModuleImage,
                LPCSTR ModuleName,
                PULONG ModuleSize)
{
    ULONG LocalModuleSize;
    ULONG_PTR ThisModuleBase = NextModuleBase;
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

    return ThisModuleBase;
}

PVOID
NTAPI
FrLdrMapImage(IN FILE *Image, IN PCHAR ShortName, IN ULONG ImageType)
{
    PVOID Result = NULL;

    printf("Loading image %s (type %d)\n", ShortName, ImageType);

    if (ImageType == 1)
    {
        if(FrLdrMapKernel(Image))
            Result = (PVOID)KernelMemory;
    }
    else
    {
        PVOID ModuleBase = (PVOID)NextModuleBase;

        if(FrLdrMapModule(Image, ShortName, 0, 0))
            Result = ModuleBase;   
    }
    return Result;
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
