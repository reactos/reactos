/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ldr/loader.c
 * PURPOSE:         Loaders for PE executables
 * PROGRAMMER:      Rex Jolliff (rex@lvcablemodem.com)
 * UPDATE HISTORY:
 *   DW   22/05/98  Created
 *   RJJ  10/12/98  Completed loader function and added hooks for MZ/PE 
 */

/* INCLUDES *****************************************************************/

#include <internal/i386/segment.h>
#include <internal/kernel.h>
#include <internal/linkage.h>
#include <internal/module.h>
#include <internal/string.h>
#include <internal/symbol.h>

#include <ddk/ntddk.h>
#include <ddk/li.h>

//#define NDEBUG
#include <internal/debug.h>

#include "pe.h"

/* FUNCTIONS *****************************************************************/

/* COFF Driver load support **************************************************/
static BOOLEAN LdrCOFFDoRelocations(module *Module, unsigned int SectionIndex);
static BOOLEAN LdrCOFFDoAddr32Reloc(module *Module, SCNHDR *Section, RELOC *Relocation);
static BOOLEAN LdrCOFFDoReloc32Reloc(module *Module, SCNHDR *Section, RELOC *Relocation);
static void LdrCOFFGetSymbolName(module *Module, unsigned int Idx, char *Name);
static unsigned int LdrCOFFGetSymbolValue(module *Module, unsigned int Idx);
static unsigned int LdrCOFFGetKernelSymbolAddr(char *Name);
static unsigned int LdrCOFFGetSymbolValueByName(module *Module, char *SymbolName, unsigned int Idx);

static NTSTATUS 
LdrCOFFProcessDriver(HANDLE FileHandle)
{
  BOOLEAN FoundEntry;
  char SymbolName[255];
  int i;
  NTSTATUS Status;
  PVOID ModuleLoadBase;
  ULONG EntryOffset;
  FILHDR *FileHeader;
  AOUTHDR *AOUTHeader;
  module *Module;
  FILE_STANDARD_INFORMATION FileStdInfo;
  PDRIVER_INITIALIZE EntryRoutine;

  /*  Get the size of the file for the section  */
  Status = ZwQueryInformationFile(FileHandle,
                                  NULL,
                                  &FileStdInfo,
                                  sizeof(FileStdInfo),
                                  FileStandardInformation);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }
  CHECKPOINT;

  /*  Allocate nonpageable memory for driver  */
  ModuleLoadBase = ExAllocatePool(NonPagedPool, 
                 GET_LARGE_INTEGER_LOW_PART(FileStdInfo.AllocationSize));
  if (ModuleLoadBase == NULL)
    {
      return STATUS_INSUFFICIENT_RESOURCES;
    }
  CHECKPOINT;

  /*  Load driver into memory chunk  */
  Status = ZwReadFile(FileHandle, 
                      0, 0, 0, 0, 
                      ModuleLoadBase, 
                      GET_LARGE_INTEGER_LOW_PART(FileStdInfo.AllocationSize), 
                      0, 0);
  if (!NT_SUCCESS(Status))
    {
      ExFreePool(ModuleLoadBase);
      return Status;
    }
  CHECKPOINT;

  /*  Get header pointers  */
  FileHeader = ModuleLoadBase;
  AOUTHeader = ModuleLoadBase + FILHSZ;
  CHECKPOINT;

  /*  Check COFF magic value  */
  if (I386BADMAG(*FileHeader))
    {
      DbgPrint("Module has bad magic value (%x)\n", 
               FileHeader->f_magic);
      ExFreePool(ModuleLoadBase);
      return STATUS_UNSUCCESSFUL;
    }
  CHECKPOINT;
   
  /*  Allocate and initialize a module definition structure  */
  Module = (module *) ExAllocatePool(NonPagedPool, sizeof(module));
  if (Module == NULL)
    {
      ExFreePool(ModuleLoadBase);
      return STATUS_INSUFFICIENT_RESOURCES;
    }
  Module->sym_list = (SYMENT *)(ModuleLoadBase + FileHeader->f_symptr);
  Module->str_tab = (char *)(ModuleLoadBase + FileHeader->f_symptr +
    FileHeader->f_nsyms * SYMESZ);
  Module->scn_list = (SCNHDR *)(ModuleLoadBase + FILHSZ + 
    FileHeader->f_opthdr);
  Module->size = 0;
  Module->raw_data_off = (ULONG) ModuleLoadBase;
  Module->nsyms = FileHeader->f_nsyms;
  CHECKPOINT;

  /*  Determine the length of the module  */
  for (i = 0; i < FileHeader->f_nscns; i++)
    {
      DPRINT("Section name: %.8s\n", Module->scn_list[i].s_name);
      DPRINT("size %x vaddr %x size %x\n",
             Module->size,
             Module->scn_list[i].s_vaddr,
             Module->scn_list[i].s_size);
      if (Module->scn_list[i].s_flags & STYP_TEXT)
        {
          Module->text_base = Module->scn_list[i].s_vaddr;
        }
      if (Module->scn_list[i].s_flags & STYP_DATA)
        {
          Module->data_base = Module->scn_list[i].s_vaddr;
        }
      if (Module->scn_list[i].s_flags & STYP_BSS)
        {
          Module->bss_base = Module->scn_list[i].s_vaddr;
        }
      if (Module->size <
          (Module->scn_list[i].s_vaddr + Module->scn_list[i].s_size))
        {
          Module->size = Module->size + Module->scn_list[i].s_vaddr +
            Module->scn_list[i].s_size;
        }
    }
  CHECKPOINT;

  /*  Allocate a section for the module  */
  Module->base = (unsigned int) MmAllocateSection(Module->size);
  if (Module->base == 0)
    {
      DbgPrint("Failed to alloc section for module\n");
      ExFreePool(Module);
      ExFreePool(ModuleLoadBase);
      return STATUS_INSUFFICIENT_RESOURCES;
    }
  CHECKPOINT;

  /*  Adjust section vaddrs for allocated area  */
  Module->data_base = Module->data_base + Module->base;
  Module->text_base = Module->text_base + Module->base;
  Module->bss_base = Module->bss_base + Module->base;
   
  /*  Relocate module and fixup imports  */
  for (i = 0; i < FileHeader->f_nscns; i++)
    {
      if (Module->scn_list[i].s_flags & STYP_TEXT ||
          Module->scn_list[i].s_flags & STYP_DATA)
        {
          memcpy((PVOID)(Module->base + Module->scn_list[i].s_vaddr),
                 (PVOID)(ModuleLoadBase + Module->scn_list[i].s_scnptr),
                 Module->scn_list[i].s_size);
          if (!LdrCOFFDoRelocations(Module, i))
            {
              DPRINT("Relocation failed for section %s\n",
                     Module->scn_list[i].s_name);
              ExFreePool(Module);
              ExFreePool(ModuleLoadBase);
              return STATUS_UNSUCCESSFUL;
            }
        }
      if (Module->scn_list[i].s_flags & STYP_BSS)
        {
          memset((PVOID)(Module->base + Module->scn_list[i].s_vaddr),
                 0,
                 Module->scn_list[i].s_size);
        }
    }
   
  DbgPrint("Module base: %x\n", Module->base);
   
  /*  Find the entry point  */
  EntryOffset = 0L;
  FoundEntry = FALSE;
  for (i = 0; i < FileHeader->f_nsyms; i++)
    {
      LdrCOFFGetSymbolName(Module, i, SymbolName);
      if (!strcmp(SymbolName, "_DriverEntry"))
        {
          EntryOffset = Module->sym_list[i].e_value;
          FoundEntry = TRUE;
          DPRINT("Found entry at %x\n", EntryOffset);
        }
    }
  if (!FoundEntry)
    {
      DbgPrint("No module entry point defined\n");
      ExFreePool(Module);
      ExFreePool(ModuleLoadBase);
      return STATUS_UNSUCCESSFUL;
    }
   
  /*  Call the module initalization routine  */
  EntryRoutine = (PDRIVER_INITIALIZE)(Module->base + EntryOffset);

  return InitalizeLoadedDriver(EntryRoutine);
}

/*   LdrCOFFDoRelocations
 * FUNCTION: Do the relocations for a module section
 * ARGUMENTS:
 *          Module = Pointer to the module 
 *          SectionIndex = Index of the section to be relocated
 * RETURNS: Success or failure
 */

static BOOLEAN 
LdrCOFFDoRelocations(module *Module, unsigned int SectionIndex)
{
  SCNHDR *Section = &Module->scn_list[SectionIndex];
  RELOC *Relocation = (RELOC *)(Module->raw_data_off + Section->s_relptr);
  int j;
   
  DPRINT("SectionIndex %d Name %.8s Relocs %d\n",
         SectionIndex,
         Module->scn_list[SectionIndex].s_name,
         Section->s_nreloc);

  for (j = 0; j < Section->s_nreloc; j++)
    {
      DbgPrint("vaddr %x ", Relocation->r_vaddr);
      DbgPrint("symndex %x ", Relocation->r_symndx);

      switch (Relocation->r_type)
        {
          case RELOC_ADDR32:
            if (!LdrCOFFDoAddr32Reloc(Module, Section, Relocation))
              {
                return FALSE;
              }
            break;
	     
          case RELOC_REL32:
            if (!LdrCOFFDoReloc32Reloc(Module, Section, Relocation))
              {
                return FALSE;
              }
            break;

          default:
            DbgPrint("%.8s: Unknown relocation type %x at %d in module\n",
                     Module->scn_list[SectionIndex].s_name,
		     Relocation->r_type,
                     j);
            return FALSE;
        }
      Relocation++;
    }
   DPRINT("%.8s: relocations done\n", Module->scn_list[SectionIndex].s_name);
   
   return TRUE;
}                   

/*
 * FUNCTION: Performs a addr32 relocation on a loaded module
 * ARGUMENTS:
 *         mod = module to perform the relocation on
 *         scn = Section to perform the relocation in
 *         reloc = Pointer to a data structure describing the relocation
 * RETURNS: Success or failure
 * NOTE: This fixes up a relocation needed when changing the base address of a
 * module
 */

static BOOLEAN
LdrCOFFDoAddr32Reloc(module *Module, SCNHDR *Section, RELOC *Relocation)
{
  unsigned int Value;
  unsigned int *Location;
   
  Value = LdrCOFFGetSymbolValue(Module, Relocation->r_symndx);
  Location = (unsigned int *)(Module->base + Relocation->r_vaddr);
  DbgPrint("ADDR32 loc %x value %x *loc %x ", Location, Value, *Location);
  *Location = (*Location) + Module->base;
   
  return TRUE;
}

/*
 * FUNCTION: Performs a reloc32 relocation on a loaded module
 * ARGUMENTS:
 *         mod = module to perform the relocation on
 *         scn = Section to perform the relocation in
 *         reloc = Pointer to a data structure describing the relocation
 * RETURNS: Success or failure
 * NOTE: This fixes up an undefined reference to a kernel function in a module
 */

static BOOLEAN
LdrCOFFDoReloc32Reloc(module *Module, SCNHDR *Section, RELOC *Relocation)
{
  char Name[255];
  unsigned int Value;
  unsigned int *Location;
   
  memset(Name, 0, 255);
  LdrCOFFGetSymbolName(Module, Relocation->r_symndx, Name);
  Value = (unsigned int) LdrCOFFGetKernelSymbolAddr(Name);
  if (Value == 0L)
    {
      Value = LdrCOFFGetSymbolValueByName(Module, Name, Relocation->r_symndx);
      if (Value == 0L)
        {
          DbgPrint("Undefined symbol %s in module\n", Name);
          return FALSE;
        }
      Location = (unsigned int *)(Module->base + Relocation->r_vaddr);
//	   (*Location) = (*Location) + Value + Module->base - Section->s_vaddr;
      (*Location) = (*Location);
      DPRINT("Module->base %x Section->s_vaddr %x\n", 
             Module->base, 
             Section->s_vaddr);
    }
  else
    {
      DPRINT("REL32 value %x name %s\n", Value, Name);
      Location = (unsigned int *)(Module->base + Relocation->r_vaddr);
      DPRINT("old %x ", *Location);
      DPRINT("Module->base %x Section->s_vaddr %x\n",
             Module->base,
             Section->s_vaddr);
      (*Location) = (*Location) + Value - Module->base + Section->s_vaddr;
      DPRINT("new %x\n", *Location);
    }

  return TRUE;
}

/*
 * FUNCTION: Get the name of a symbol from a loaded module by ordinal
 * ARGUMENTS:
 *      mod = module
 *      i = index of symbol
 *      name (OUT) = pointer to a string where the symbol name will be
 *                   stored
 */

static void 
LdrCOFFGetSymbolName(module *Module, unsigned int Idx, char *Name)
{
  if (Module->sym_list[Idx].e.e_name[0] != 0)
    {
      strncpy(Name, Module->sym_list[Idx].e.e_name, 8);
      Name[8] = '\0';
    }
  else
    {
      strcpy(Name, &Module->str_tab[Module->sym_list[Idx].e.e.e_offset]);
    }
}

/*
 * FUNCTION: Get the value of a module defined symbol
 * ARGUMENTS:
 *      mod = module
 *      i = index of symbol
 * RETURNS: The value of the symbol
 * NOTE: This fixes up references to known sections
 */

static unsigned int 
LdrCOFFGetSymbolValue(module *Module, unsigned int Idx)
{
  char Name[255];

  LdrCOFFGetSymbolName(Module, Idx, Name);
  DbgPrint("name %s ", Name);
   
  /*  Check if the symbol is a section we have relocated  */
  if (strcmp(Name, ".text") == 0)
    {
      return Module->text_base;
    }
  if (strcmp(Name, ".data") == 0)
    {
      return Module->data_base;
    }
  if (strcmp(Name, ".bss") == 0)
    {
      return Module->bss_base;
    }

  return Module->sym_list[Idx].e_value;
}

/*
 * FUNCTION: Get the address of a kernel symbol
 * ARGUMENTS:
 *      name = symbol name
 * RETURNS: The address of the symbol on success
 *          NULL on failure
 */

static unsigned int  
LdrCOFFGetKernelSymbolAddr(char *Name)
{
  int i = 0;

  while (symbol_table[i].name != NULL)
    {
      if (strcmp(symbol_table[i].name, Name) == 0)
        {
          return symbol_table[i].value;
        }
      i++;
    }

  return 0L;
}

static unsigned int 
LdrCOFFGetSymbolValueByName(module *Module, 
                            char *SymbolName,
                            unsigned int Idx)
{
  unsigned int i;
  char Name[255];
   
  DPRINT("LdrCOFFGetSymbolValueByName(sname %s, idx %x)\n", SymbolName, Idx);
   
  for (i = 0; i < Module->nsyms; i++)
    {
      LdrCOFFGetSymbolName(Module, i, Name);
      DPRINT("Scanning %s Value %x\n", Name, Module->sym_list[i].e_value);
      if (strcmp(Name, SymbolName) == 0)
        {
          DPRINT("Returning %x\n", Module->sym_list[i].e_value);
          return Module->sym_list[i].e_value;
        }
    }

  return 0L;
}




static NTSTATUS 
LdrProcessPEDriver(HANDLE FileHandle, PIMAGE_DOS_HEADER DosHeader)
{
  return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS 
LdrLoadDriver(PUNICODE_STRING Filename)
/*
 * FUNCTION: Loads a kernel driver
 * ARGUMENTS:
 *         FileName = Driver to load
 * RETURNS: Status
 */
{
  char BlockBuffer[512];
  NTSTATUS Status;
  HANDLE FileHandle;
  OBJECT_ATTRIBUTES FileObjectAttributes;
  PIMAGE_DOS_HEADER PEDosHeader;

  /*  Open the Driver  */
  InitializeObjectAttributes(&FileObjectAttributes,
                             Filename, 
                             0,
                             NULL,
                             NULL);
  Status = ZwOpenFile(&FileHandle, 0, &FileObjectAttributes, NULL, 0, 0);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  /*  Read first block of image to determine type  */
  Status = ZwReadFile(FileHandle, 0, 0, 0, 0, BlockBuffer, 512, 0, 0);
  if (!NT_SUCCESS(Status))
    {
      ZwClose(FileHandle);
      return Status;
    }    

  /*  If MZ header exists  */
  PEDosHeader = (PIMAGE_DOS_HEADER) BlockBuffer;
  if (PEDosHeader->e_magic == 0x54AD && PEDosHeader->e_lfanew != 0L)
    {
      Status = LdrProcessPEDriver(FileHandle, PEDosHeader);
      if (!NT_SUCCESS(Status))
        {
          ZwClose(FileHandle);
          return Status;
        }
    }
  if (PEDosHeader->e_magic == 0x54AD)
    {
      ZwClose(FileHandle);
      return STATUS_NOT_IMPLEMENTED;
    }
  else /*  Assume coff format and load  */
    {
      Status = LdrCOFFProcessDriver(FileHandle);
      if (!NT_SUCCESS(Status))
        {
          ZwClose(FileHandle);
          return Status;
        }
    }

  return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS 
LdrProcessMZImage(HANDLE ProcessHandle, 
                  HANDLE FileHandle, 
                  PIMAGE_DOS_HEADER DosHeader)
{

  /* FIXME: map VDM into low memory  */
  /* FIXME: Build/Load image sections  */
   
  return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS 
LdrProcessPEImage(HANDLE ProcessHandle, 
                  HANDLE FileHandle, 
                  PIMAGE_DOS_HEADER DosHeader)
{
//  PIMAGE_NT_HEADERS PEHeader;
//  PIMAGE_SECTION_HEADER Sections;

  // FIXME: Check architechture
  // FIXME: Build/Load image sections
  // FIXME: do relocations code sections
  // FIXME: resolve imports
  // FIXME: do fixups
   
  return STATUS_NOT_IMPLEMENTED;
}

/*
 * FUNCTION: Loads a PE executable into the specified process
 * ARGUMENTS:
 *        Filename = File to load
 *        ProcessHandle = handle 
 * RETURNS: Status
 */

NTSTATUS 
LdrLoadImage(PUNICODE_STRING Filename, HANDLE ProcessHandle)
{
  char BlockBuffer[512];
  NTSTATUS Status;
  ULONG SectionSize;
  HANDLE FileHandle;
  HANDLE ThreadHandle;   
  OBJECT_ATTRIBUTES FileObjectAttributes;
  PIMAGE_DOS_HEADER PEDosHeader;
  CONTEXT Context;
  HANDLE SectionHandle;
  PVOID BaseAddress;

  /*  FIXME: should DLLs be named sections?  */

  /*  Open the image file  */
  InitializeObjectAttributes(&FileObjectAttributes,
                             Filename, 
                             0,
                             NULL,
                             NULL);
  Status = ZwOpenFile(&FileHandle, 0, &FileObjectAttributes, NULL, 0, 0);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  /*  Read first block of image to determine type  */
  Status = ZwReadFile(FileHandle, 0, 0, 0, 0, BlockBuffer, 512, 0, 0);
  if (!NT_SUCCESS(Status))
    {
      ZwClose(FileHandle);
      return Status;
    }    

  /*  If MZ header exists  */
  PEDosHeader = (PIMAGE_DOS_HEADER) BlockBuffer;
  if (PEDosHeader->e_magic == 0x54AD && PEDosHeader->e_lfanew != 0L)
    {
      Status = LdrProcessPEImage(ProcessHandle, 
                                 FileHandle, 
                                 PEDosHeader);
      if (!NT_SUCCESS(Status))
        {
          return Status;
        }
    }
  else if (PEDosHeader->e_magic == 0x54AD)
    {
      Status = LdrProcessMZImage(ProcessHandle, 
                                 FileHandle, 
                                 PEDosHeader);
      if (!NT_SUCCESS(Status))
        {
          return Status;
        }
    }
  else /*  Assume bin format and load  */
    {
      FILE_STANDARD_INFORMATION FileStdInfo;

      /*  Get the size of the file for the section  */
      Status = ZwQueryInformationFile(FileHandle,
                                      NULL,
                                      &FileStdInfo,
                                      sizeof(FileStdInfo),
                                      FileStandardInformation);
      if (!NT_SUCCESS(Status))
        {
          ZwClose(FileHandle);
          return Status;
        }

      /*  Create the section for the code  */
      Status = ZwCreateSection(&SectionHandle,
                               SECTION_ALL_ACCESS,
                               NULL,
                               NULL,
                               PAGE_READWRITE,
                               MEM_COMMIT,
                               FileHandle);
      ZwClose(FileHandle);
      if (!NT_SUCCESS(Status))
        {
          return Status;
        }

      /*  Map a view of the section into the desired process  */
      BaseAddress = (PVOID)0x10000;
      SectionSize = GET_LARGE_INTEGER_LOW_PART(FileStdInfo.AllocationSize);
      Status = ZwMapViewOfSection(SectionHandle,
                                  ProcessHandle,
                                  &BaseAddress,
                                  0,
                                  SectionSize,
                                  NULL,
                                  &SectionSize,
                                  0,
                                  MEM_COMMIT,
                                  PAGE_READWRITE);
      if (!NT_SUCCESS(Status))
        {
          /* FIXME: destroy the section here  */

          return Status;
        }
   
      /*  Setup the context for the initial thread  */
      memset(&Context,0,sizeof(CONTEXT));
      Context.SegSs = USER_DS;
      Context.Esp = 0x2000;
      Context.EFlags = 0x202;
      Context.SegCs = USER_CS;
      Context.Eip = 0x10000;
      Context.SegDs = USER_DS;
      Context.SegEs = USER_DS;
      Context.SegFs = USER_DS;
      Context.SegGs = USER_DS;
   
      /*  Create the stack for the process  */
      BaseAddress = (PVOID) 0x1000;
      SectionSize = 0x1000;
      Status = ZwAllocateVirtualMemory(ProcessHandle,
                                       &BaseAddress,
                                       0,
                                       &SectionSize,
                                       MEM_COMMIT,
                                       PAGE_READWRITE);
      if (!NT_SUCCESS(Status))
        {
          /* FIXME: unmap the section here  */
          /* FIXME: destroy the section here  */

          return Status;
        }
   
      /*  Create the initial thread  */
      Status = ZwCreateThread(&ThreadHandle,
                              THREAD_ALL_ACCESS,
                              NULL,
                              ProcessHandle,
                              NULL,
                              &Context,
                              NULL,
                              FALSE);
      if (!NT_SUCCESS(Status))
        {
          /* FIXME: destroy the stack memory block here  */
          /* FIXME: unmap the section here  */
          /* FIXME: destroy the section here  */

          return Status;
        }
    }
  /* FIXME: {else} could check for a.out, ELF, COFF, etc. images here... */

  return Status;
}

