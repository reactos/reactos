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
#define _NTSYSTEM_
#include <freeldr.h>

#define NDEBUG
#include <debug.h>
#undef DbgPrint

/* Base Addres of Kernel in Physical Memory */
#define KERNEL_BASE_PHYS 0x200000

/* Bits to shift to convert a Virtual Address into an Offset in the Page Table */
#define PFN_SHIFT 12

/* Bits to shift to convert a Virtual Address into an Offset in the Page Directory */
#define PDE_SHIFT 22
#define PDE_SHIFT_PAE 18


/* Converts a Relative Address read from the Kernel into a Physical Address */
#define RaToPa(p) \
    (ULONG_PTR)((ULONG_PTR)p + KERNEL_BASE_PHYS)

/* Converts a Physical Address Pointer into a Page Frame Number */
#define PaPtrToPfn(p) \
    (((ULONG_PTR)&p) >> PFN_SHIFT)

/* Converts a Physical Address into a Page Frame Number */
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


#define KernelEntryPoint            (KernelEntry - KERNEL_BASE_PHYS) + KernelBase

/* Load Address of Next Module */
ULONG_PTR NextModuleBase = 0;

/* Currently Opened Module */
PLOADER_MODULE CurrentModule = NULL;

/* Unrelocated Kernel Base in Virtual Memory */
ULONG_PTR KernelBase;

/* Whether PAE is to be used or not */
BOOLEAN PaeModeEnabled;

/* Kernel Entrypoint in Physical Memory */
ULONG_PTR KernelEntry;

typedef struct _HARDWARE_PTE_X64 {
    ULONG Valid             : 1;
    ULONG Write             : 1;
    ULONG Owner             : 1;
    ULONG WriteThrough      : 1;
    ULONG CacheDisable      : 1;
    ULONG Accessed          : 1;
    ULONG Dirty             : 1;
    ULONG LargePage         : 1;
    ULONG Global            : 1;
    ULONG CopyOnWrite       : 1;
    ULONG Prototype         : 1;
    ULONG reserved          : 1;
    ULONG PageFrameNumber   : 20;
    ULONG reserved2         : 31;
    ULONG NoExecute         : 1;
} HARDWARE_PTE_X64, *PHARDWARE_PTE_X64;

typedef struct _PAGE_DIRECTORY_X86 {
    HARDWARE_PTE Pde[1024];
} PAGE_DIRECTORY_X86, *PPAGE_DIRECTORY_X86;

typedef struct _PAGE_DIRECTORY_X64 {
    HARDWARE_PTE_X64 Pde[2048];
} PAGE_DIRECTORY_X64, *PPAGE_DIRECTORY_X64;

typedef struct _PAGE_DIRECTORY_TABLE_X64 {
    HARDWARE_PTE_X64 Pde[4];
} PAGE_DIRECTORY_TABLE_X64, *PPAGE_DIRECTORY_TABLE_X64;

/* Page Directory and Tables for non-PAE Systems */
extern PAGE_DIRECTORY_X86 startup_pagedirectory;
extern PAGE_DIRECTORY_X86 lowmem_pagetable;
extern PAGE_DIRECTORY_X86 kernel_pagetable;
extern ULONG_PTR hyperspace_pagetable;
extern ULONG_PTR _pae_pagedirtable;
extern PAGE_DIRECTORY_X86 apic_pagetable;
extern PAGE_DIRECTORY_X86 kpcr_pagetable;

/* Page Directory and Tables for PAE Systems */
extern PAGE_DIRECTORY_TABLE_X64 startup_pagedirectorytable_pae;
extern PAGE_DIRECTORY_X64 startup_pagedirectory_pae;
extern PAGE_DIRECTORY_X64 lowmem_pagetable_pae;
extern PAGE_DIRECTORY_X64 kernel_pagetable_pae;
extern ULONG_PTR hyperspace_pagetable_pae;
extern ULONG_PTR pagedirtable_pae;
extern PAGE_DIRECTORY_X64 apic_pagetable_pae;
extern PAGE_DIRECTORY_X64 kpcr_pagetable_pae;

extern CHAR szHalName[1024];

PIMAGE_BASE_RELOCATION
NTAPI
LdrProcessRelocationBlockLongLong(IN ULONG_PTR Address,
                                  IN ULONG Count,
                                  IN PUSHORT TypeOffset,
                                  IN LONGLONG Delta);

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
    /* Disable Interrupts */
    _disable();

    /* Re-initalize EFLAGS */
    Ke386EraseFlags();

    /* Get the PAE Mode */
    FrLdrGetPaeMode();

    /* Initialize the page directory */
    FrLdrSetupPageDirectory();

    /* Initialize Paging, Write-Protection and Load NTOSKRNL */
    FrLdrSetupPae(Magic);
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
    ULONG_PTR PageDirectoryBaseAddress = (ULONG_PTR)&startup_pagedirectory;
    ASMCODE PagedJump;

    if (PaeModeEnabled)
    {
        PageDirectoryBaseAddress = (ULONG_PTR)&startup_pagedirectorytable_pae;

        /* Enable PAE */
        __writecr4(__readcr4() | X86_CR4_PAE);
    }

    /* Set the PDBR */
    __writecr3(PageDirectoryBaseAddress);

    /* Enable Paging and Write Protect*/
    __writecr0(__readcr0() | X86_CR0_PG | X86_CR0_WP);

    /* Jump to Kernel */
    PagedJump = (ASMCODE)(PVOID)(KernelEntryPoint);
    PagedJump(Magic, &LoaderBlock);
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
 *     Determines whether PAE mode should be enabled or not.
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
    BOOLEAN PaeModeSupported;

    PaeModeSupported = FALSE;
    PaeModeEnabled = FALSE;

    if (CpuidSupported() & 1)
    {
       ULONG eax, ebx, ecx, FeatureBits;
       GetCpuid(1, &eax, &ebx, &ecx, &FeatureBits);
       if (FeatureBits & X86_FEATURE_PAE)
       {
          PaeModeSupported = TRUE;
       }
    }

    if (PaeModeSupported)
    {
       PCHAR p;

       /* Read Command Line */
       p = (PCHAR)LoaderBlock.CommandLine;
       while ((p = strchr(p, '/')) != NULL) {

          p++;
          /* Find "PAE" */
          if (!_strnicmp(p, "PAE", 3)) {

              /* Make sure there's nothing following it */
              if (p[3] == ' ' || p[3] == 0) {

                  /* Use Pae */
                  PaeModeEnabled = TRUE;
                  break;
              }
          }
       }
    }
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
        KernelPageTableIndex = KernelBase >> PDE_SHIFT;

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
}

PVOID
NTAPI
LdrPEGetExportByName(PVOID BaseAddress,
                     PUCHAR SymbolName,
                     USHORT Hint)
{
    PIMAGE_EXPORT_DIRECTORY ExportDir;
    PULONG * ExFunctions;
    PULONG * ExNames;
    USHORT * ExOrdinals;
    PVOID ExName;
    ULONG Ordinal;
    PVOID Function;
    LONG minn, maxn, mid, res;
    ULONG ExportDirSize;

    /* HAL and NTOS use a virtual address, switch it to physical mode */
    if ((ULONG_PTR)BaseAddress & 0x80000000)
    {
        BaseAddress = (PVOID)((ULONG_PTR)BaseAddress - KSEG0_BASE + 0x200000);
    }

    ExportDir = (PIMAGE_EXPORT_DIRECTORY)
        RtlImageDirectoryEntryToData(BaseAddress,
                                     TRUE,
                                     IMAGE_DIRECTORY_ENTRY_EXPORT,
                                     &ExportDirSize);
    if (!ExportDir)
    {
        DbgPrint("LdrPEGetExportByName(): no export directory!\n");
        return NULL;
    }

    /* The symbol names may be missing entirely */
    if (!ExportDir->AddressOfNames)
    {
        DbgPrint("LdrPEGetExportByName(): symbol names missing entirely\n");
        return NULL;
    }

    /*
    * Get header pointers
    */
    ExNames = (PULONG *)RVA(BaseAddress, ExportDir->AddressOfNames);
    ExOrdinals = (USHORT *)RVA(BaseAddress, ExportDir->AddressOfNameOrdinals);
    ExFunctions = (PULONG *)RVA(BaseAddress, ExportDir->AddressOfFunctions);

    /*
    * Check the hint first
    */
    if (Hint < ExportDir->NumberOfNames)
    {
        ExName = RVA(BaseAddress, ExNames[Hint]);
        if (strcmp(ExName, (PCHAR)SymbolName) == 0)
        {
            Ordinal = ExOrdinals[Hint];
            Function = RVA(BaseAddress, ExFunctions[Ordinal]);
            if ((ULONG_PTR)Function >= (ULONG_PTR)ExportDir &&
                (ULONG_PTR)Function < (ULONG_PTR)ExportDir + ExportDirSize)
            {
                Function = NULL;
                if (Function == NULL)
                {
                    DbgPrint("LdrPEGetExportByName(): failed to find %s\n",SymbolName);
                }
                return Function;
            }

            if (Function != NULL) return Function;
        }
    }

    /*
    * Binary search
    */
    minn = 0;
    maxn = ExportDir->NumberOfNames - 1;
    while (minn <= maxn)
    {
        mid = (minn + maxn) / 2;

        ExName = RVA(BaseAddress, ExNames[mid]);
        res = strcmp(ExName, (PCHAR)SymbolName);
        if (res == 0)
        {
            Ordinal = ExOrdinals[mid];
            Function = RVA(BaseAddress, ExFunctions[Ordinal]);
            if ((ULONG_PTR)Function >= (ULONG_PTR)ExportDir &&
                (ULONG_PTR)Function < (ULONG_PTR)ExportDir + ExportDirSize)
            {
                Function = NULL;
                if (Function == NULL)
                {
                    DbgPrint("LdrPEGetExportByName(): failed to find %s\n",SymbolName);
                }
                return Function;
            }
            if (Function != NULL)
            {
                return Function;
            }
        }
        else if (res > 0)
        {
            maxn = mid - 1;
        }
        else
        {
            minn = mid + 1;
        }
    }

    ExName = RVA(BaseAddress, ExNames[mid]);
    DbgPrint("LdrPEGetExportByName(): failed to find %s\n",SymbolName);
    return (PVOID)NULL;
}

NTSTATUS
NTAPI
LdrPEProcessImportDirectoryEntry(PVOID DriverBase,
                                 PLOADER_MODULE LoaderModule,
                                 PIMAGE_IMPORT_DESCRIPTOR ImportModuleDirectory)
{
    PVOID* ImportAddressList;
    PULONG FunctionNameList;

    if (ImportModuleDirectory == NULL || ImportModuleDirectory->Name == 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* Get the import address list. */
    ImportAddressList = (PVOID*)RVA(DriverBase, ImportModuleDirectory->FirstThunk);

    /* Get the list of functions to import. */
    if (ImportModuleDirectory->OriginalFirstThunk != 0)
    {
        FunctionNameList = (PULONG)RVA(DriverBase, ImportModuleDirectory->OriginalFirstThunk);
    }
    else
    {
        FunctionNameList = (PULONG)RVA(DriverBase, ImportModuleDirectory->FirstThunk);
    }

    /* Walk through function list and fixup addresses. */
    while (*FunctionNameList != 0L)
    {
        if ((*FunctionNameList) & 0x80000000)
        {
            DbgPrint("Failed to import ordinal from %s\n", LoaderModule->String);
            return STATUS_UNSUCCESSFUL;
        }
        else
        {
            IMAGE_IMPORT_BY_NAME *pe_name;
            pe_name = RVA(DriverBase, *FunctionNameList);
            *ImportAddressList = LdrPEGetExportByName((PVOID)LoaderModule->ModStart, pe_name->Name, pe_name->Hint);

            /* Fixup the address to be virtual */
            *ImportAddressList = (PVOID)((ULONG_PTR)*ImportAddressList + (KSEG0_BASE - 0x200000));

            //DbgPrint("Looked for: %s and found: %p\n", pe_name->Name, *ImportAddressList);
            if ((*ImportAddressList) == NULL)
            {
                DbgPrint("Failed to import %s from %s\n", pe_name->Name, LoaderModule->String);
                return STATUS_UNSUCCESSFUL;
            }
        }
        ImportAddressList++;
        FunctionNameList++;
    }
    return STATUS_SUCCESS;
}

PLOADER_MODULE
NTAPI
LdrGetModuleObject(PCHAR ModuleName)
{
    ULONG i;

    for (i = 0; i < LoaderBlock.ModsCount; i++)
    {
        if (!_stricmp((PCHAR)reactos_modules[i].String, ModuleName))
        {
            return &reactos_modules[i];
        }
    }

    return NULL;
}

BOOLEAN
NTAPI
FrLdrLoadHal(PCHAR szFileName, INT nPos);

NTSTATUS
NTAPI
LdrPEGetOrLoadModule(PCHAR ModuleName,
                     PCHAR ImportedName,
                     PLOADER_MODULE* ImportedModule)
{
    NTSTATUS Status = STATUS_SUCCESS;

    *ImportedModule = LdrGetModuleObject(ImportedName);
    if (*ImportedModule == NULL)
    {
        /*
         * For now, we only support import-loading the HAL.
         * Later, FrLdrLoadDriver should be made to share the same
         * code, and we'll just call it instead.
         */
        if (!_stricmp(ImportedName, "hal.dll"))
        {
            /* Load the HAL */
            FrLdrLoadHal(szHalName, 10);

            /* Return the new module */
            *ImportedModule = LdrGetModuleObject(ImportedName);
            if (*ImportedModule == NULL)
            {
                DbgPrint("Error loading import: %s\n", ImportedName);
                return STATUS_UNSUCCESSFUL;
            }
        }
        else
        {
            DbgPrint("Don't yet support loading new modules from imports\n");
            Status = STATUS_NOT_IMPLEMENTED;
        }
    }

    return Status;
}

NTSTATUS
NTAPI
LdrPEFixupImports(IN PVOID DllBase,
                  IN PCHAR DllName)
{
    PIMAGE_IMPORT_DESCRIPTOR ImportModuleDirectory;
    PCHAR ImportedName;
    NTSTATUS Status;
    PLOADER_MODULE ImportedModule;
    ULONG Size;

    /*  Process each import module  */
    ImportModuleDirectory = (PIMAGE_IMPORT_DESCRIPTOR)
        RtlImageDirectoryEntryToData(DllBase,
                                     TRUE,
                                     IMAGE_DIRECTORY_ENTRY_IMPORT,
                                     &Size);
    while (ImportModuleDirectory->Name)
    {
        /*  Check to make sure that import lib is kernel  */
        ImportedName = (PCHAR) DllBase + ImportModuleDirectory->Name;
        //DbgPrint("Processing imports for file: %s into file: %s\n", DllName, ImportedName);

        Status = LdrPEGetOrLoadModule(DllName, ImportedName, &ImportedModule);
        if (!NT_SUCCESS(Status)) return Status;

        //DbgPrint("Import Base: %p\n", ImportedModule->ModStart);
        Status = LdrPEProcessImportDirectoryEntry(DllBase, ImportedModule, ImportModuleDirectory);
        if (!NT_SUCCESS(Status)) return Status;

        //DbgPrint("Imports for file: %s into file: %s complete\n", DllName, ImportedName);
        ImportModuleDirectory++;
    }

    return STATUS_SUCCESS;
}

VOID
NTAPI
FrLdrMapImage(IN PIMAGE_NT_HEADERS NtHeader,
              IN PVOID Base)
{
    PIMAGE_SECTION_HEADER Section;
    ULONG SectionCount, SectionSize;
    PVOID SourceSection, TargetSection;
    INT i;

    /* Load the first section */
    Section = IMAGE_FIRST_SECTION(NtHeader);
    SectionCount = NtHeader->FileHeader.NumberOfSections - 1;

    /* Now go to the last section */
    Section += SectionCount;

    /* Walk each section backwards */
    for (i = SectionCount; i >= 0; i--, Section--)
    {
        /* Get the disk location and the memory location, and the size */
        SourceSection = RVA(Base, Section->PointerToRawData);
        TargetSection = RVA(Base, Section->VirtualAddress);
        SectionSize = Section->SizeOfRawData;

        /* If the section is already mapped correctly, go to the next */
        if (SourceSection == TargetSection) continue;

        /* Load it into memory */
        RtlMoveMemory(TargetSection, SourceSection, SectionSize);

        /* Check for uninitialized data */
        if (Section->SizeOfRawData < Section->Misc.VirtualSize)
        {
            /* Zero it out */
            RtlZeroMemory(RVA(Base, Section->VirtualAddress +
                                    Section->SizeOfRawData),
                          Section->Misc.VirtualSize - Section->SizeOfRawData);
        }
    }
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
    PIMAGE_NT_HEADERS NtHeader;
    ULONG ImageSize;
    PVOID LoadBase;

    /* Set the virtual (image) and physical (load) addresses */
    LoadBase = (PVOID)KERNEL_BASE_PHYS;

    /* Load the first 1024 bytes of the kernel image so we can read the PE header */
    if (!FsReadFile(KernelImage, 1024, NULL, LoadBase)) return FALSE;

    /* Now read the MZ header to get the offset to the PE Header */
    NtHeader = RtlImageNtHeader(LoadBase);

    /* Get Kernel Base */
    KernelBase = NtHeader->OptionalHeader.ImageBase;
    FrLdrGetKernelBase();

    /* Save Entrypoint */
    KernelEntry = RaToPa(NtHeader->OptionalHeader.AddressOfEntryPoint);

    /* Save the Image Size */
    ImageSize = NtHeader->OptionalHeader.SizeOfImage;

    /* Set the file pointer to zero */
    FsSetFilePointer(KernelImage, 0);

    /* Load the file image */
    FsReadFile(KernelImage, ImageSize, NULL, LoadBase);

    /* Map it */
    FrLdrMapImage(NtHeader, LoadBase);

    /* Calculate Difference between Real Base and Compiled Base*/
    LdrRelocateImageWithBias(LoadBase,
                             KernelBase - (ULONG_PTR)LoadBase,
                             "FreeLdr",
                             STATUS_SUCCESS,
                             STATUS_UNSUCCESSFUL,
                             STATUS_UNSUCCESSFUL);

    /* Fill out Module Data Structure */
    reactos_modules[0].ModStart = KernelBase;
    reactos_modules[0].ModEnd = KernelBase + ImageSize;
    strcpy(reactos_module_strings[0], "ntoskrnl.exe");
    reactos_modules[0].String = (ULONG_PTR)reactos_module_strings[0];
    LoaderBlock.ModsCount++;

    /* Increase the next Load Base */
    NextModuleBase = ROUND_UP(LoadBase + ImageSize, PAGE_SIZE);

    /*  Perform import fixups  */
    LdrPEFixupImports(LoadBase, "ntoskrnl.exe");

    /* Return Success */
    return TRUE;
}

BOOLEAN
NTAPI
FrLdrMapHal(FILE *HalImage)
{
    PIMAGE_NT_HEADERS NtHeader;
    PVOID ImageBase, LoadBase;
    ULONG ImageSize;

    /* Set the virtual (image) and physical (load) addresses */
    LoadBase = (PVOID)NextModuleBase;
    ImageBase  = RVA(LoadBase , -KERNEL_BASE_PHYS + KSEG0_BASE);

    /* Load the first 1024 bytes of the HAL image so we can read the PE header */
    if (!FsReadFile(HalImage, 1024, NULL, LoadBase)) return FALSE;

    /* Now read the MZ header to get the offset to the PE Header */
    NtHeader = RtlImageNtHeader(LoadBase);

    /* Save the Image Size */
    ImageSize = NtHeader->OptionalHeader.SizeOfImage;

    /* Set the file pointer to zero */
    FsSetFilePointer(HalImage, 0);

    /* Load the file image */
    FsReadFile(HalImage, ImageSize, NULL, LoadBase);

    /* Map it into virtual memory */
    FrLdrMapImage(NtHeader, LoadBase);

    /* Calculate Difference between Real Base and Compiled Base*/
    LdrRelocateImageWithBias(LoadBase,
                             (ULONG_PTR)ImageBase - (ULONG_PTR)LoadBase,
                             "FreeLdr",
                             STATUS_SUCCESS,
                             STATUS_UNSUCCESSFUL,
                             STATUS_UNSUCCESSFUL);

    /* Fill out Module Data Structure */
    reactos_modules[1].ModStart = (ULONG_PTR)ImageBase;
    reactos_modules[1].ModEnd = (ULONG_PTR)ImageBase + ImageSize;
    strcpy(reactos_module_strings[1], "hal.dll");
    reactos_modules[1].String = (ULONG_PTR)reactos_module_strings[1];
    LoaderBlock.ModsCount++;

    /*  Perform import fixups  */
    LdrPEFixupImports(LoadBase, "hal.dll");

    /* Increase the next Load Base */
    NextModuleBase = ROUND_UP(NextModuleBase + ImageSize, PAGE_SIZE);

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
//    DbgPrint("NextBase, ImageSize, ModStart, ModEnd %p %p %p %p\n",
  //           NextModuleBase, LocalModuleSize, ModuleData->ModStart, ModuleData->ModEnd);

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
