/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ldr/loader.c
 * PURPOSE:         Loaders for PE executables
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 * UPDATE HISTORY:
 *   DW   22/05/98  Created
 *   RJJ  10/12/98  Completed image loader function and added hooks for MZ/PE
 *   RJJ  10/12/98  Built driver loader function and added hooks for PE/COFF
 *   RJJ  10/12/98  Rolled in David's code to load COFF drivers
 *   JM   14/12/98  Built initail PE user module loader
 */

/* INCLUDES *****************************************************************/

#include <internal/i386/segment.h>
#include <internal/kernel.h>
#include <internal/linkage.h>
#include <internal/module.h>
#include <internal/ob.h>
#include <internal/string.h>
#include <internal/symbol.h>

#include <ddk/ntddk.h>

//#define NDEBUG
#include <internal/debug.h>

/* MACROS ********************************************************************/

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))

/* GLOBALS *******************************************************************/

POBJECT_TYPE ObModuleType = NULL;

/* FORWARD DECLARATIONS ******************************************************/

NTSTATUS LdrCOFFProcessDriver(PVOID ModuleLoadBase);
NTSTATUS LdrPEProcessDriver(PVOID ModuleLoadBase);

/*  COFF Driver load support  */
static BOOLEAN LdrCOFFDoRelocations(module *Module, unsigned int SectionIndex);
static BOOLEAN LdrCOFFDoAddr32Reloc(module *Module, SCNHDR *Section, RELOC *Relocation);
static BOOLEAN LdrCOFFDoReloc32Reloc(module *Module, SCNHDR *Section, RELOC *Relocation);
static void LdrCOFFGetSymbolName(module *Module, unsigned int Idx, char *Name);
static unsigned int LdrCOFFGetSymbolValue(module *Module, unsigned int Idx);
static unsigned int LdrCOFFGetKernelSymbolAddr(char *Name);
static unsigned int LdrCOFFGetSymbolValueByName(module *Module, char *SymbolName, unsigned int Idx);

/*  Image loader forward delcarations  */
static NTSTATUS LdrProcessMZImage(HANDLE ProcessHandle, HANDLE ModuleHandle, HANDLE FileHandle);
static NTSTATUS LdrProcessPEImage(HANDLE ProcessHandle, HANDLE ModuleHandle, HANDLE FileHandle);
static NTSTATUS LdrProcessBinImage(HANDLE ProcessHandle, HANDLE ModuleHandle, HANDLE FileHandle);

/* FUNCTIONS *****************************************************************/

VOID LdrInitModuleManagment(VOID)
{
  ANSI_STRING AnsiString;

  /*  Register the process object type  */   
  ObModuleType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));
  ObModuleType->TotalObjects = 0;
  ObModuleType->TotalHandles = 0;
  ObModuleType->MaxObjects = ULONG_MAX;
  ObModuleType->MaxHandles = ULONG_MAX;
  ObModuleType->PagedPoolCharge = 0;
  ObModuleType->NonpagedPoolCharge = sizeof(MODULE);
  ObModuleType->Dump = NULL;
  ObModuleType->Open = NULL;
  ObModuleType->Close = NULL;
  ObModuleType->Delete = NULL;
  ObModuleType->Parse = NULL;
  ObModuleType->Security = NULL;
  ObModuleType->QueryName = NULL;
  ObModuleType->OkayToClose = NULL;
  RtlInitAnsiString(&AnsiString, "Module");
  RtlAnsiStringToUnicodeString(&ObModuleType->TypeName, &AnsiString, TRUE);
}

/*
 * FUNCTION: Loads a kernel driver
 * ARGUMENTS:
 *         FileName = Driver to load
 * RETURNS: Status
 */

NTSTATUS 
LdrLoadDriver(PUNICODE_STRING Filename)
{
  PVOID ModuleLoadBase;
  NTSTATUS Status;
  HANDLE FileHandle;
  OBJECT_ATTRIBUTES FileObjectAttributes;
  PIMAGE_DOS_HEADER PEDosHeader;
  FILE_STANDARD_INFORMATION FileStdInfo;

  DbgPrint("Loading Driver %W...\n", Filename);

  /*  Open the Driver  */
  InitializeObjectAttributes(&FileObjectAttributes,
                             Filename, 
                             0,
                             NULL,
                             NULL);
  CHECKPOINT;
  Status = ZwOpenFile(&FileHandle, 
                      FILE_ALL_ACCESS, 
                      &FileObjectAttributes, 
                      NULL, 0, 0);
  CHECKPOINT;
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }
  CHECKPOINT;

  /*  Get the size of the file  */
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

  ZwClose(FileHandle);

  /*  If MZ header exists  */
  PEDosHeader = (PIMAGE_DOS_HEADER) ModuleLoadBase;
  if (PEDosHeader->e_magic == IMAGE_DOS_MAGIC && PEDosHeader->e_lfanew != 0L)
    {
      Status = LdrPEProcessDriver(ModuleLoadBase);
      if (!NT_SUCCESS(Status))
        {
          ExFreePool(ModuleLoadBase);
          return Status;
        }
    }
  if (PEDosHeader->e_magic == IMAGE_DOS_MAGIC)
    {
      ExFreePool(ModuleLoadBase);
      return STATUS_NOT_IMPLEMENTED;
    }
  else  /*  Assume COFF format and load  */
    {
      Status = LdrCOFFProcessDriver(ModuleLoadBase);
      if (!NT_SUCCESS(Status))
        {
          ExFreePool(ModuleLoadBase);
          return Status;
        }
    }

  /*  Cleanup  */
  ExFreePool(ModuleLoadBase);

  return STATUS_SUCCESS;
}

NTSTATUS 
LdrPEProcessDriver(PVOID ModuleLoadBase)
{
  unsigned int DriverSize;
  PVOID DriverBase, CodeBase, InitializedDataBase, UninitializedDataBase;
  PULONG PEMagic;
  PIMAGE_DOS_HEADER PEDosHeader;
  PIMAGE_FILE_HEADER PEFileHeader;
  PIMAGE_OPTIONAL_HEADER PEOptionalHeader;
  
  DbgPrint("Processing PE Driver at module base:%08lx\n", ModuleLoadBase);

  /*  Get header pointers  */
  PEDosHeader = (PIMAGE_DOS_HEADER) ModuleLoadBase;
  PEMagic = (PULONG) ((unsigned int) ModuleLoadBase + 
    PEDosHeader->e_lfanew);
  PEFileHeader = (PIMAGE_FILE_HEADER) ((unsigned int) ModuleLoadBase + 
    PEDosHeader->e_lfanew + sizeof(ULONG));
  PEOptionalHeader = (PIMAGE_OPTIONAL_HEADER) ((unsigned int) ModuleLoadBase + 
    PEDosHeader->e_lfanew + sizeof(ULONG) + sizeof(IMAGE_FILE_HEADER));
  CHECKPOINT;

  /*  Check file magic numbers  */
  if (PEDosHeader->e_magic != IMAGE_DOS_MAGIC || 
      PEDosHeader->e_lfanew == 0 ||
      *PEMagic != IMAGE_PE_MAGIC ||
      PEFileHeader->Machine != IMAGE_FILE_MACHINE_I386)
    {
      return STATUS_UNSUCCESSFUL;
    }
  CHECKPOINT;

#if 0
  /* FIXME: if image is fixed-address load, then fail  */

  /* FIXME: check/verify OS version number  */

  DbgPrint("OptionalHdrMagic:%04x LinkVersion:%d.%d\n", 
           PEOptionalHeader->Magic,
           PEOptionalHeader->MajorLinkerVersion,
           PEOptionalHeader->MinorLinkerVersion);
  DbgPrint("Size: CODE:%08lx(%d) DATA:%08lx(%d) BSS:%08lx(%d)\n",
           PEOptionalHeader->SizeOfCode,
           PEOptionalHeader->SizeOfCode,
           PEOptionalHeader->SizeOfInitializedData,
           PEOptionalHeader->SizeOfInitializedData,
           PEOptionalHeader->SizeOfUninitializedData,
           PEOptionalHeader->SizeOfUninitializedData);
  DbgPrint("Entry Point:%08lx\n", PEOptionalHeader->AddressOfEntryPoint);
  CHECKPOINT;

  /*  Determine the size of the module  */
  DriverSize = ROUND_UP(PEOptionalHeader->SizeOfCode, 
                        PEOptionalHeader->SectionAlignment) +
    ROUND_UP(PEOptionalHeader->SizeOfInitializedData, 
             PEOptionalHeader->SectionAlignment) +
    ROUND_UP(PEOptionalHeader->SizeOfUninitializedData, 
             PEOptionalHeader->SectionAlignment);
  CHECKPOINT;

  /*  Allocate a virtual section for the module  */  
  DriverBase = MmAllocateSection(DriverSize);
  if (DriverBase == 0)
    {
      DbgPrint("Failed to allocate a virtual section for driver\n");
      return STATUS_INSUFFICIENT_RESOURCES;
    }
  CHECKPOINT;

  /*  Compute addresses for driver sections  */
  CodeBase = DriverBase;
  InitializedDataBase = (PUCHAR) DriverBase + 
    (PUCHAR) ROUND_UP(PEOptionalHeader->SizeOfCode, 
                     PEOptionalHeader->SectionAlignment);
  UninitializedDataBase = (PUCHAR) InitializedDataBase + 
    (PUCHAR) ROUND_UP(PEOptionalHeader->SizeOfInitializedData, 
                     PEOptionalHeader->SectionAlignment);

  /* FIXME: Copy code section into virtual section  */
  memcpy(CodeBase,
         (PVOID)(ModuleLoadBase + ???),
         ROUND_UP(PEOptionalHeader->SizeOfCode, 
                  PEOptionalHeader->FileAlignment));
#endif

  /* FIXME: Copy initialized data section into virtual section  */
  /* FIXME: Perform relocations fixups  */
  /* FIXME: compute address of entry point  */

  /* return InitializeLoadedDriver(EntryPoint); */
  return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS 
LdrCOFFProcessDriver(PVOID ModuleLoadBase)
{
  BOOLEAN FoundEntry;
  char SymbolName[255];
  int i;
  ULONG EntryOffset;
  FILHDR *FileHeader;
  AOUTHDR *AOUTHeader;
  module *Module;
  PDRIVER_INITIALIZE EntryRoutine;

  /*  Get header pointers  */
  FileHeader = ModuleLoadBase;
  AOUTHeader = ModuleLoadBase + FILHSZ;
  CHECKPOINT;

  /*  Check COFF magic value  */
  if (I386BADMAG(*FileHeader))
    {
      DbgPrint("Module has bad magic value (%x)\n", 
               FileHeader->f_magic);
      return STATUS_UNSUCCESSFUL;
    }
  CHECKPOINT;
   
  /*  Allocate and initialize a module definition structure  */
  Module = (module *) ExAllocatePool(NonPagedPool, sizeof(module));
  if (Module == NULL)
    {
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

              /* FIXME: unallocate all sections here  */

              ExFreePool(Module);

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

      /* FIXME: unallocate all sections here  */

      return STATUS_UNSUCCESSFUL;
    }
   
  /*  Get the address of the module initalization routine  */
  EntryRoutine = (PDRIVER_INITIALIZE)(Module->base + EntryOffset);

  /*  Cleanup  */
  ExFreePool(Module);

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

NTSTATUS LdrLoadLibrary(HANDLE ProcessHandle,
                        PHANDLE ModuleHandle,
                        PCHAR Name)
{
#if 0
  NTSTATUS Status;
  ANSI_STRING afilename;
  UNICODE_STRING ufilename,umodName;
  PMODULE *Library, *Module;
  OBJECT_ATTRIBUTES attr;
  PWSTR Ignored;
  char name2[512];

  /* FIXME: this is broke  */
  /* FIXME: check for module already loaded  */
  /* FIXME: otherwise load module */
  /* FIXME: we need to fix how modules are loaded so that they can
            be shared... :(  */

  /*  If module is already loaded, get a reference and return it  */
  strcpy(name2, "\\modules\\");
  strcat(name2, Name);
  RtlInitAnsiString(&afilename, name2);
  RtlAnsiStringToUnicodeString(&umodName, &afilename, TRUE);
  InitializeObjectAttributes(&attr, &umodName, 0, NULL, NULL);
  Status = ObOpenObjectByName(&attr, (PVOID *) &Library, &Ignored);
  DPRINT("LoadLibrary : Status=%x,pLibrary=%x\n",Status, Library);
  if (!NT_SUCCESS(Status) || Library == NULL)
    {
      strcpy(name2, "\\??\\C:\\reactos\\system\\");
      strcat(name2, name);
      RtlInitAnsiString(&afilename, name2);
      RtlAnsiStringToUnicodeString(&ufilename, &afilename, TRUE);
      DPRINT("LoadLibrary,load %s\n", name2);
      Library = LdrLoadImage(&ufilename);
      /* FIXME: execute start code ? */
      Module = ObGenericCreateObject(NULL, PROCESS_ALL_ACCESS, &attr, ObModuleType);
      if (Module)
        {
          memcpy(Module, Library, PMODULE);
        }
      else
        {
          DbgPrint("library object not created\n");
        }
      RtlFreeUnicodeString(&ufilename);
      Status = ObOpenObjectByName(&attr, (PVOID *)&Library, &Ignored);
    }
  else
    {
      DbgPrint("Library already loaded\n");
      *Module = Library
    }
  RtlFreeUnicodeString(&umodName);

  return STATUS_SUCCESS;
#endif
  UNIMPLEMENTED;
}

/*   LdrLoadImage
 * FUNCTION:
 *   Loads a module into the specified process
 * ARGUMENTS:
 *   HANDLE   ProcessHandle  handle of the process to load the module into
 *   PHANDLE  ModuleHandle   handle of the loaded module
 *   PUNICODE_STRING  Filename  name of the module to load
 * RETURNS: 
 *   NTSTATUS
 */

NTSTATUS 
LdrLoadImage(HANDLE ProcessHandle, 
             PHANDLE ModuleHandle, 
             PUNICODE_STRING Filename)
{
  char BlockBuffer[1024];
  NTSTATUS Status;
  OBJECT_ATTRIBUTES FileObjectAttributes;
  HANDLE FileHandle;
  PMODULE Module;
  PIMAGE_DOS_HEADER PEDosHeader;

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

  /*  Build a module structure for the image  */
  Module = ObGenericCreateObject(ModuleHandle, 
                                 PROCESS_ALL_ACCESS, 
                                 NULL,
                                 ObModuleType);
  if (Module == NULL)
    {
      ZwClose(FileHandle);
      return Status;
    }

  /*  Read first block of image to determine type  */
  Status = ZwReadFile(FileHandle, 0, 0, 0, 0, BlockBuffer, 1024, 0, 0);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject(*ModuleHandle);
      *ModuleHandle = NULL;
      ZwClose(FileHandle);

      return Status;
    }    

  /*  If MZ header exists  */
  PEDosHeader = (PIMAGE_DOS_HEADER) BlockBuffer;
  if (PEDosHeader->e_magic == IMAGE_DOS_MAGIC && 
      PEDosHeader->e_lfanew != 0L &&
      *(PULONG)((PUCHAR)BlockBuffer + PEDosHeader->e_lfanew) == IMAGE_PE_MAGIC)
    {
      Status = LdrProcessPEImage(ProcessHandle, 
                                 ModuleHandle,
                                 FileHandle);
    }
  else if (PEDosHeader->e_magic == 0x54AD)
    {
      Status = LdrProcessMZImage(ProcessHandle, 
                                 ModuleHandle,
                                 FileHandle);
    }
  else /*  Assume bin format and load  */
    {
      Status = LdrProcessBinImage(ProcessHandle, 
                                  ModuleHandle,
                                  FileHandle);
    }
  /* FIXME: {else} could check for a.out, ELF, COFF, etc. images here... */

  /* FIXME: should we unconditionally dereference the module handle here?  */
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject(*ModuleHandle);
      *ModuleHandle = NULL;
    }
  ZwClose(FileHandle);

  return Status;
}

static NTSTATUS 
LdrProcessMZImage(HANDLE ProcessHandle, 
                  HANDLE ModuleHandle,
                  HANDLE FileHandle)
{

  /* FIXME: map VDM into low memory  */
  /* FIXME: Build/Load image sections  */
   
  return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS 
LdrProcessPEImage(HANDLE ProcessHandle, 
                  HANDLE ModuleHandle,
                  HANDLE FileHandle)
{
  int i;
  NTSTATUS Status;
  PVOID BaseSection;
  PIMAGE_DOS_HEADER DosHeader;
  PIMAGE_NT_HEADERS NTHeaders;
  PMODULE Module;
  LARGE_INTEGER SectionOffset;

  /*  Allocate memory for headers  */
  Module = HEADER_TO_BODY(ModuleHandle);
  if (Module == NULL)
    {
      return STATUS_UNSUCCESSFUL;
    }
  DosHeader = (PIMAGE_DOS_HEADER)ExAllocatePool(NonPagedPool, 
                                                sizeof(IMAGE_DOS_HEADER) +
                                                sizeof(IMAGE_NT_HEADERS));
  if (DosHeader == NULL)
    {
      return STATUS_UNSUCCESSFUL;
    }
  NTHeaders = (PIMAGE_NT_HEADERS)((PUCHAR) DosHeader + sizeof(IMAGE_DOS_HEADER));
  
  /*  Read the headers into memory  */
  memset(Module, '\0', sizeof(PMODULE));
  Status = ZwReadFile(FileHandle,
                      NULL, NULL, NULL, NULL,
                      DosHeader, 
                      sizeof(IMAGE_DOS_HEADER),
                      0, 0);
  if (!NT_SUCCESS(Status))
    {
      ExFreePool(DosHeader);
      return Status;
    }
  SET_LARGE_INTEGER_HIGH_PART(SectionOffset, 0);
  SET_LARGE_INTEGER_LOW_PART(SectionOffset, DosHeader->e_lfanew);
  Status = ZwReadFile(FileHandle, 
                      NULL, NULL, NULL, NULL,
                      NTHeaders,
                      sizeof(IMAGE_NT_HEADERS), 
                      &SectionOffset, 
                      0);
  if (!NT_SUCCESS(Status))
    {
      ExFreePool(DosHeader);
      return Status;
    }
      
  /*  Allocate memory in process for image  */
  Module->Flags = MODULE_FLAG_PE;
  Module->Base = (PVOID) NTHeaders->OptionalHeader.ImageBase;
  Module->Size = NTHeaders->OptionalHeader.SizeOfImage;
  Status = ZwAllocateVirtualMemory(ProcessHandle,
                                   &Module->Base,
                                   0,
                                   NULL,
                                   MEM_COMMIT,
                                   PAGE_READWRITE);
  if (!NT_SUCCESS(Status))
    {
      ExFreePool(DosHeader);
      return Status;
    }

  /*  Load headers into virtual memory  */
  Status = ZwReadFile(FileHandle, 
                      NULL, NULL, NULL, NULL,
                      Module->Base,
                      NTHeaders->OptionalHeader.SizeOfHeaders,
                      0, 0);
  if (!NT_SUCCESS(Status))
    {
      ZwFreeVirtualMemory(ProcessHandle, 
                          Module->Base, 
                          0, 
                          MEM_RELEASE);
      ExFreePool(DosHeader);
      return Status;
    }

  /*  Adjust module pointers into virtual memory  */
  DosHeader = (PIMAGE_DOS_HEADER) Module->Base;
  NTHeaders = (PIMAGE_NT_HEADERS) ((PUCHAR)Module->Base + 
    DosHeader->e_lfanew);
  Module->Image.PE.FileHeader = (PIMAGE_FILE_HEADER) ((PUCHAR)NTHeaders + 
    sizeof(DWORD));
  Module->Image.PE.OptionalHeader = (PIMAGE_OPTIONAL_HEADER)
    ((PUCHAR)Module->Image.PE.FileHeader + sizeof(IMAGE_FILE_HEADER));
  Module->Image.PE.SectionList = (PCOFF_SECTION_HEADER) ((PUCHAR)NTHeaders + 
    sizeof(IMAGE_NT_HEADERS));

  /*  Build Image Sections  */
  /* FIXME: should probably use image directory to load sections. */
  for (i = 0; i < Module->Image.PE.FileHeader->NumberOfSections; i++)
    {
      DPRINT("section %d\n", i);
      BaseSection = (PVOID)((PCHAR) Module->Base + 
        Module->Image.PE.SectionList[i].s_vaddr);

      /* Load code and initialized data sections from disk  */
      if ((Module->Image.PE.SectionList[i].s_flags & STYP_TEXT) ||
          (Module->Image.PE.SectionList[i].s_flags & STYP_DATA))
        {
          SET_LARGE_INTEGER_HIGH_PART(SectionOffset, 0);
          SET_LARGE_INTEGER_LOW_PART(SectionOffset, 
                                     Module->Image.PE.SectionList[i].s_scnptr);

          /* FIXME: should probably map sections into sections */
          Status = ZwReadFile(FileHandle,
                              NULL, NULL, NULL, NULL,
                              Module->Base + Module->Image.PE.SectionList[i].s_vaddr,
                              min(Module->Image.PE.SectionList[i].s_size,
                                  Module->Image.PE.SectionList[i].s_paddr),
                              &SectionOffset, 0);
          if (!NT_SUCCESS(Status))
            {
              ZwFreeVirtualMemory(ProcessHandle, 
                                  Module->Base, 
                                  0, 
                                  MEM_RELEASE);
              ExFreePool(DosHeader);
              return Status;
            }
        }
      else if (Module->Image.PE.SectionList[i].s_flags & STYP_BSS)
        {
          memset((PVOID)(Module->Base + 
                   Module->Image.PE.SectionList[i].s_vaddr), 
                 0,
                 Module->Image.PE.SectionList[i].s_size);
        }
    }

  /*  Resolve Import Library references  */
  if (Module->Image.PE.OptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress)
    {
      PIMAGE_IMPORT_MODULE_DIRECTORY ImportModuleDirectory;

      /*  Process each import module  */
      ImportModuleDirectory = (PIMAGE_IMPORT_MODULE_DIRECTORY)
        ((PUCHAR)Module->Base + Module->Image.PE.OptionalHeader->
          DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
      while (ImportModuleDirectory->dwRVAModuleName)
        {
          PMODULE Library;
          PVOID *LibraryExports;
          PVOID *ImportAddressList; // was pImpAddr
          PULONG FunctionNameList;
          DWORD pName;
          PWORD pHint;

          /*  Load the library module into the process  */
          /* FIXME: this should take a UNICODE string  */
          Status = LdrLoadLibrary(ProcessHandle,
                                  &Library,
                                  (PCHAR)(Module->Base + 
                                    ImportModuleDirectory->dwRVAModuleName));
          if (!NT_SUCCESS(Status))
            {
              /* FIXME: Dereference all loaded modules  */
              ZwFreeVirtualMemory(ProcessHandle, 
                                  Module->Base, 
                                  0, 
                                  MEM_RELEASE);
              ExFreePool(DosHeader);

              return Status;              
            }          

          /*  Get the address of the export list for the library  */
          LibraryExports = (PVOID *)(Library->Base + 
            Library->Image.PE.OptionalHeader->
            DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress +
            sizeof(IMAGE_EXPORT_DIRECTORY));

          /*  Get the import address list  */
          ImportAddressList = (PVOID *)
            ((PCHAR)Module->Image.PE.OptionalHeader->ImageBase + 
            ImportModuleDirectory->dwRVAFunctionAddressList);

          /*  Get the list of functions to import  */
          if (ImportModuleDirectory->dwRVAFunctionNameList != 0)
            {
              FunctionNameList = (PULONG) ((PCHAR)Module->Base + 
                ImportModuleDirectory->dwRVAFunctionNameList);
            }
          else
            {
              FunctionNameList = (PULONG) ((PCHAR)Module->Base + 
                ImportModuleDirectory->dwRVAFunctionAddressList);
            }

          /*  Walk through function list and fixup addresses  */
          while(*FunctionNameList != 0L)
            {
              if ((*FunctionNameList) & 0x80000000) // hint
                {
                  *ImportAddressList = LibraryExports[(*FunctionNameList) & 0x7fffffff];
                }
              else // hint-name
                {
                  pName = (DWORD)((PCHAR)Module->Base + *FunctionNameList + 2);
                  pHint = (PWORD)((PCHAR)Module->Base + *FunctionNameList);

                  /* FIXME: verify name  */

                  *ImportAddressList = LibraryExports[*pHint];
                }

              /* FIXME: verify value of hint  */

              ImportAddressList++;
              FunctionNameList++;
            }
          ImportModuleDirectory++;
        }
    }

  /*  Do fixups  */
  if (Module->Base != (PVOID)Module->Image.PE.OptionalHeader->ImageBase)
    {
      USHORT NumberOfEntries;
      PUSHORT pValue16;
      ULONG RelocationRVA;
      ULONG Delta32, Offset;
      PULONG pValue32;
      PRELOCATION_DIRECTORY RelocationDir;
      PRELOCATION_ENTRY RelocationBlock;

      RelocationRVA = NTHeaders->OptionalHeader.DataDirectory[
        IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
      if (RelocationRVA)
        {
          RelocationDir = (PRELOCATION_DIRECTORY)
            ((PCHAR)Module->Base + RelocationRVA);
          while (RelocationDir->SizeOfBlock)
            {
              Delta32 = (unsigned long)(Module->Base - NTHeaders->OptionalHeader.ImageBase);
              RelocationBlock = (PRELOCATION_ENTRY) 
                (RelocationRVA + Module->Base + sizeof(RELOCATION_DIRECTORY));
              NumberOfEntries = 
                (RelocationDir->SizeOfBlock - sizeof(RELOCATION_DIRECTORY)) / 
                sizeof(RELOCATION_ENTRY);
              for (i = 0; i < NumberOfEntries; i++)
                {
                  Offset = (RelocationBlock[i].TypeOffset & 0xfff) + RelocationDir->VirtualAddress;
                  switch (RelocationBlock[i].TypeOffset >> 12)
                    {
                      case TYPE_RELOC_ABSOLUTE:
                        break;

                      case TYPE_RELOC_HIGH:
                        pValue16 = (PUSHORT) (Module->Base + Offset);
                        *pValue16 += Delta32 >> 16;
                        break;

                      case TYPE_RELOC_LOW:
                        pValue16 = (PUSHORT)(Module->Base + Offset);
                        *pValue16 += Delta32 & 0xffff;
                        break;

                      case TYPE_RELOC_HIGHLOW:
                        pValue32 = (PULONG) (Module->Base + Offset);
                        *pValue32 += Delta32;
                        break;

                      case TYPE_RELOC_HIGHADJ:
                        /* FIXME: do the highadjust fixup  */
                        DbgPrint("TYPE_RELOC_HIGHADJ fixup not implemented, sorry\n");
//                        break;

                      default:
                        DbgPrint("unexpected fixup type\n");

                        /* FIXME: Dereference all loaded modules  */

                        ZwFreeVirtualMemory(ProcessHandle, 
                                            Module->Base, 
                                            0, 
                                            MEM_RELEASE);
                        ExFreePool(DosHeader);
                        return STATUS_UNSUCCESSFUL;
                    }
                }
              RelocationRVA += RelocationDir->SizeOfBlock;
              RelocationDir = (PRELOCATION_DIRECTORY)((PCHAR)Module->Base + 
                RelocationRVA);
            }
        }
    }

  /* FIXME: Create the stack for the process  */
  /* FIXME: Setup the context for the initial thread  */
  /* FIXME: Create the initial thread  */

// fail: ZwFreeVirtualMemory(ProcessHandle, Module->ImageBase, 0, MEM_RELEASE);
  ExFreePool(DosHeader);

  return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
LdrProcessBinImage(HANDLE ProcessHandle, 
                   HANDLE ModuleHandle,
                   HANDLE FileHandle)
{
  NTSTATUS Status;
  FILE_STANDARD_INFORMATION FileStdInfo;
  ULONG SectionSize;
  HANDLE ThreadHandle;   
  CONTEXT Context;
  HANDLE SectionHandle;
  PVOID BaseAddress;

  /* FIXME: should set module pointers  */

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

  /*  Create the section for the code  */
  Status = ZwCreateSection(&SectionHandle,
                           SECTION_ALL_ACCESS,
                           NULL,
                           NULL,
                           PAGE_READWRITE,
                           MEM_COMMIT,
                           FileHandle);
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

  return STATUS_SUCCESS;
}


