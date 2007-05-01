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
#include "mmu.h"
#include "compat.h"
#include "font.h"

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
ULONG_PTR KernelEntry;

/* Dummy to bring in memmove */
PVOID memmove_dummy = memmove;

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

typedef struct _DIRECTORY_ENTRY {
    ULONG Virt, Phys;
} DIRECTORY_ENTRY;

typedef struct _DIRECTORY_HEADER {
    UINT NumberOfPages;
    UINT DirectoryPage, UsedSpace;
    DIRECTORY_ENTRY *Directory[1];
} DIRECTORY_HEADER;

DIRECTORY_ENTRY *
GetPageMapping( DIRECTORY_HEADER *TranslationMap, ULONG Number ) {
    if( Number >= (ULONG)TranslationMap->NumberOfPages ) return 0;
    else {
	int EntriesPerPage = (1<<PFN_SHIFT)/sizeof(DIRECTORY_ENTRY);
	return &TranslationMap->Directory
	    [Number/EntriesPerPage][Number%EntriesPerPage];
    }
}		

VOID
AddPageMapping( DIRECTORY_HEADER *TranslationMap, 
		ULONG Virt,
		ULONG OldVirt) {
    ULONG Phys;
    DIRECTORY_ENTRY *Entry;

    if( !TranslationMap->DirectoryPage ||
	TranslationMap->UsedSpace >= ((1<<PFN_SHIFT)/sizeof(DIRECTORY_ENTRY)) )
    {
	TranslationMap->Directory
	    [TranslationMap->DirectoryPage] = MmAllocateMemory(1<<PFN_SHIFT);
	TranslationMap->UsedSpace = 0;
	TranslationMap->DirectoryPage++;
	AddPageMapping
	    (TranslationMap, 
	     (ULONG)TranslationMap->Directory
	     [TranslationMap->DirectoryPage-1],0);
    }
    Phys = PpcVirt2phys(Virt,0);
    TranslationMap->NumberOfPages++;
    Entry = GetPageMapping(TranslationMap, TranslationMap->NumberOfPages-1);
    Entry->Virt = OldVirt ? OldVirt : Virt; Entry->Phys = Phys;
    TranslationMap->UsedSpace++;
}

VOID
NTAPI
FrLdrStartup(ULONG Magic)
{
    KernelEntryFn KernelEntryAddress = 
	(KernelEntryFn)(KernelEntry + KernelBase);
    ULONG_PTR i, j;
    DIRECTORY_HEADER *TranslationMap = MmAllocateMemory(4<<PFN_SHIFT);
    ULONG_PTR stack = ((ULONG_PTR)TranslationMap)+(4<<PFN_SHIFT);
    boot_infos_t *LocalBootInfo = (boot_infos_t *)TranslationMap;
    TranslationMap = (DIRECTORY_HEADER *)
	(((PCHAR)&LocalBootInfo[1]) + sizeof(BootDigits));
    memcpy(&LocalBootInfo[1], BootDigits, sizeof(BootDigits));
    *LocalBootInfo = BootInfo;
    LocalBootInfo->dispFont = (font_char *)&LocalBootInfo[1];
    ULONG_PTR KernelVirtAddr, NewMapSdr = 
	(ULONG_PTR)MmAllocateMemory(128*1024);
    DIRECTORY_ENTRY *Entry;
    LoaderBlock.ArchExtra = (ULONG)LocalBootInfo;
    int msr = GetMSR();

    printf("Translation map: %x (msr %x)\n", TranslationMap, msr);
    NewMapSdr = XROUNDUP(NewMapSdr,64*1024);
    printf("New SDR1 value will be %x\n", NewMapSdr);

    memset(TranslationMap,0,sizeof(*TranslationMap));

    printf("Translation map zeroed\n");
    /* Add the page containing the page directory */
    for( i = (ULONG_PTR)TranslationMap; i < stack; i += (1<<PFN_SHIFT) ) {
	AddPageMapping( TranslationMap, i, 0 );
    }

    /* Map freeldr space 0xe00000 ... 0xe60000 */
    for( i = 0xe00000; i < 0xe80000; i += (1<<PFN_SHIFT) ) {
	AddPageMapping( TranslationMap, i, 0 );
    }

    /* Map kernel space 0x80000000 ... */
    for( i = (ULONG)KernelMemory; 
	 i < (ULONG)KernelMemory + KernelMemorySize; 
	 i += (1<<PFN_SHIFT),j++ ) {
	KernelVirtAddr = LoaderBlock.KernelBase + (i - (ULONG)KernelMemory);
	AddPageMapping( TranslationMap, i, KernelVirtAddr );
    }

    printf("Built map of %d pages\n", TranslationMap->NumberOfPages);
    printf("Local boot info at %x\n", LocalBootInfo);

    /* 
     * Stuff page table entries for the page containing this code,
     * The pages containing the page table entries, and finally the kernel
     * pages.
     * 
     * When done, we'll be able to flop over to kernel land! 
     */
    for( i = 0, Entry = GetPageMapping( TranslationMap, i );
	 Entry;
	 i++, Entry = GetPageMapping( TranslationMap, i ) ) {
	DrawNumber(LocalBootInfo,Entry->Virt,10,90);
	DrawNumber(LocalBootInfo,Entry->Phys,100,90);
	InsertPageEntry( Entry->Virt, Entry->Phys, 
			 (i & 0x3ff) >> 3, NewMapSdr );
    }

    /* Tell them we're booting */
    DrawNumber(LocalBootInfo,(ULONG)&LoaderBlock,10,100);

    SetSDR1( NewMapSdr );
    msr |= 0x30;
    printf("About to set msr (%x)!!!\n", msr);
    __asm__("mtmsr %0" : : "r" (msr) );
    DrawNumber(LocalBootInfo,(ULONG)KernelEntryAddress,100,100);
    KernelEntryAddress( (void*)&LoaderBlock );
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
static VOID
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

    printf("Loading file (elf at %x)\n", KernelAddr);

    /* Load the first 1024 bytes of the kernel image so we can read the PE header */
    if (!FsReadFile(KernelImage, sizeof(ehdr), NULL, &ehdr)) {

        /* Fail if we couldn't read */
	printf("Couldn't read the elf header\n");
        return FALSE;
    }

    printf("Elf header: (%c%c%c type %d machine %d version %d entry %x shoff %x shentsize %d shnum %d)\n", 
	   ehdr.e_ident[1], ehdr.e_ident[2], ehdr.e_ident[3],
	   ehdr.e_type,
	   ehdr.e_machine,
	   ehdr.e_version,
	   ehdr.e_entry,
	   ehdr.e_shoff,
	   ehdr.e_shentsize,
	   ehdr.e_shnum);

    /* Start by getting elf headers */
    phsize = ehdr.e_phentsize;
    phnum = ehdr.e_phnum;
    shsize = ehdr.e_shentsize;
    shnum = ehdr.e_shnum;
    sptr = (PCHAR)MmAllocateMemory(shnum * shsize);

    /* Read section headers */
    FsSetFilePointer(KernelImage,  ehdr.e_shoff);
    FsReadFile(KernelImage, shsize * shnum, NULL, sptr);

    printf("Loaded section headers\n");

    /* Now we'll get the PE Header */
    for( i = 0; i < shnum; i++ ) 
    {
	shdr = ELF_SECTION(i);
	shdr->sh_addr = 0;

	/* Find the PE Header */
	if (shdr->sh_type == TYPE_PEHEADER)
	{
	    FsSetFilePointer(KernelImage, shdr->sh_offset);
	    FsReadFile(KernelImage, shdr->sh_size, NULL, KernelMemory);
	    ImageHeader = (PIMAGE_DOS_HEADER)KernelMemory;
	    NtHeader = (PIMAGE_NT_HEADERS)((PCHAR)KernelMemory + SWAPD(ImageHeader->e_lfanew));
	    printf("NtHeader at %x\n", SWAPD(ImageHeader->e_lfanew));
	    printf("SectionAlignment %x\n", 
		   SWAPD(NtHeader->OptionalHeader.SectionAlignment));
	    SectionAddr = ROUND_UP
		(shdr->sh_size, SWAPD(NtHeader->OptionalHeader.SectionAlignment));
	    printf("Header ends at %x\n", SectionAddr);
	    break;
	}
    }

    if(i == shnum) 
    {
	printf("No peheader section encountered :-(\n");
	return 0;
    }

    /* Save the Image Base */
    NtHeader->OptionalHeader.ImageBase = SWAPD(KernelAddr);

    /* Load the file image */
    Section = COFF_FIRST_SECTION(NtHeader);
    SectionCount = SWAPW(NtHeader->FileHeader.NumberOfSections);

    printf("Section headers at %x\n", ((PCHAR)Section) - ((PCHAR)KernelAddr));

    /* Walk each section */
    for (i=0; i < SectionCount; i++, Section++)
    {
	printf("Section %d (NT Header) is elf section %d\n",
	       i, SWAPD(Section->PointerToRawData));
	shdr = ELF_SECTION(SWAPD(Section->PointerToRawData));

	shdr->sh_addr = SectionAddr = SWAPD(Section->VirtualAddress);
	
	if (shdr->sh_type != SHT_NOBITS)
	{
	    /* Content area */
	    printf("Loading section %d at %x\n", i, KernelAddr + SectionAddr);
	    FsSetFilePointer(KernelImage, shdr->sh_offset);
	    FsReadFile(KernelImage, shdr->sh_size, NULL, KernelMemory + SectionAddr);
	} 
	else
	{
	    /* Zero it out */
	    printf("BSS section %d at %x\n", i, KernelAddr + SectionAddr);
	    memset(KernelMemory + SectionAddr, 0, 
		   ROUND_UP(shdr->sh_size, 
			    SWAPD(NtHeader->OptionalHeader.SectionAlignment)));
        }
    }

    ImageSize = SWAPD(NtHeader->OptionalHeader.SizeOfImage);
    KernelEntry = SWAPD(NtHeader->OptionalHeader.AddressOfEntryPoint);
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
	targetSection = shdr->sh_info - 1;

	if (!ELF_SECTION(targetSection)->sh_addr) continue;

	printf("Found reloc section %d (symbols %d target %d) with %d relocs\n",
	       i, shdr->sh_link, shdr->sh_info, numreloc);
	
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
	    S = symbol.st_value + KernelAddr + ELF_SECTION(symbol.st_shndx)->sh_addr;
	    A = reloc.r_addend;
	    P = reloc.r_offset + KernelAddr + ELF_SECTION(targetSection)->sh_addr;
	    
	    Target32 = 
		(ULONG*)(((PCHAR)KernelMemory) + ELF_SECTION(targetSection)->sh_addr);
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
		printf("reloc[%d]: (type %x sym %d val %d) off %x add %x\n", 
		       j,
		       ELF32_R_TYPE(reloc.r_info),
		       ELF32_R_SYM(reloc.r_info), 
		       symbol.st_value,
		       reloc.r_offset, reloc.r_addend);	    
		break;
	    }
	}

	MmFreeMemory(SymbolSection);
	MmFreeMemory(RelocSection);
    }

    MmFreeMemory(sptr);

    ModuleData = &reactos_modules[LoaderBlock.ModsCount];
    ModuleData->ModStart = (ULONG)KernelMemory;
    /* Increase the next Load Base */
    NextModuleBase = ROUND_UP((ULONG)KernelMemory + ImageSize, PAGE_SIZE);
    ModuleData->ModEnd = NextModuleBase;
    ModuleData->String = (ULONG)MmAllocateMemory(strlen(ImageName)+1);
    strcpy((PCHAR)ModuleData->String, ImageName);
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
    printf("Kernel Memory @%x\n", (int)KernelMemory);

    return FrLdrMapModule(KernelImage, "ntoskrnl.exe", KernelMemory, KernelBase);
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

    printf("Module size %x len %x name %s\n", 
	   ModuleData->ModStart,
	   ModuleData->ModEnd - ModuleData->ModStart,
	   ModuleName);

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
