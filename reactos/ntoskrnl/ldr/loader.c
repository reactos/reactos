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
 *   JM   14/12/98  Built initial PE user module loader
 *   RJJ  06/03/99  Moved user PE loader into NTDLL
 */

/* INCLUDES *****************************************************************/

#include <windows.h>

#include <internal/i386/segment.h>
#include <internal/linkage.h>
#include <internal/module.h>
#include <internal/ntoskrnl.h>
#include <internal/ob.h>
#include <internal/ps.h>
#include <string.h>
#include <internal/string.h>
#include <internal/symbol.h>

#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

/* FIXME: this should appear in a kernel header file  */
NTSTATUS IoInitializeDriver(PDRIVER_INITIALIZE DriverEntry);

/* MACROS ********************************************************************/

/* GLOBALS *******************************************************************/

POBJECT_TYPE ObModuleType = NULL;

/* FORWARD DECLARATIONS ******************************************************/

NTSTATUS LdrLoadDriver(PUNICODE_STRING Filename);
NTSTATUS LdrProcessDriver(PVOID ModuleLoadBase);

/*  PE Driver load support  */
static NTSTATUS LdrPEProcessDriver(PVOID ModuleLoadBase);
static unsigned int LdrGetKernelSymbolAddr(char *Name);

/*  COFF Driver load support  */
static NTSTATUS LdrCOFFProcessDriver(PVOID ModuleLoadBase);
static BOOLEAN LdrCOFFDoRelocations(module *Module, unsigned int SectionIndex);
static BOOLEAN LdrCOFFDoAddr32Reloc(module *Module, SCNHDR *Section, RELOC *Relocation);
static BOOLEAN LdrCOFFDoReloc32Reloc(module *Module, SCNHDR *Section, RELOC *Relocation);
static void LdrCOFFGetSymbolName(module *Module, unsigned int Idx, char *Name);
static unsigned int LdrCOFFGetSymbolValue(module *Module, unsigned int Idx);
static unsigned int LdrCOFFGetSymbolValueByName(module *Module, char *SymbolName, unsigned int Idx);

/* FUNCTIONS *****************************************************************/

VOID LdrInitModuleManagement(VOID)
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
 * load the auto config drivers.
 */
VOID LdrLoadAutoConfigDrivers(VOID)
{
  NTSTATUS Status;
  ANSI_STRING AnsiDriverName;
  UNICODE_STRING DriverName;

  RtlInitAnsiString(&AnsiDriverName,"\\??\\C:\\reactos\\system\\drivers\\keyboard.sys"); 
  RtlAnsiStringToUnicodeString(&DriverName, &AnsiDriverName, TRUE);
  Status = LdrLoadDriver(&DriverName);
  RtlFreeUnicodeString(&DriverName);
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("driver load failed, status;%d(%x)\n", Status, Status);
      DbgPrintErrorMessage(Status);
    }
  RtlInitAnsiString(&AnsiDriverName,"\\??\\C:\\reactos\\system\\drivers\\blue.sys");
  RtlAnsiStringToUnicodeString(&DriverName, &AnsiDriverName, TRUE);
  Status = LdrLoadDriver(&DriverName);
  RtlFreeUnicodeString(&DriverName);
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("driver load failed, status;%d(%x)\n", Status, Status);
      DbgPrintErrorMessage(Status);
    }
 
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
                                  FileStdInfo.EndOfFile.u.LowPart);
  if (ModuleLoadBase == NULL)
    {
      return STATUS_INSUFFICIENT_RESOURCES;
    }
  CHECKPOINT;

  /*  Load driver into memory chunk  */
  Status = ZwReadFile(FileHandle, 
                      0, 0, 0, 0, 
                      ModuleLoadBase, 
                      FileStdInfo.EndOfFile.u.LowPart, 
                      0, 0);
  if (!NT_SUCCESS(Status))
    {
      ExFreePool(ModuleLoadBase);
      return Status;
    }
  CHECKPOINT;

  ZwClose(FileHandle);

  Status = LdrProcessDriver(ModuleLoadBase);

  /*  Cleanup  */
  ExFreePool(ModuleLoadBase);

  return STATUS_SUCCESS;
}

NTSTATUS
LdrProcessDriver(PVOID ModuleLoadBase)
{
  PIMAGE_DOS_HEADER PEDosHeader;

  /*  If MZ header exists  */
  PEDosHeader = (PIMAGE_DOS_HEADER) ModuleLoadBase;
  if (PEDosHeader->e_magic == IMAGE_DOS_MAGIC && PEDosHeader->e_lfanew != 0L)
    {
      return LdrPEProcessDriver(ModuleLoadBase);
    }
  if (PEDosHeader->e_magic == IMAGE_DOS_MAGIC)
    {
      return STATUS_NOT_IMPLEMENTED;
    }
  else  /*  Assume COFF format and load  */
    {
      return LdrCOFFProcessDriver(ModuleLoadBase);
    }
}

NTSTATUS 
LdrPEProcessDriver(PVOID ModuleLoadBase)
{
  unsigned int DriverSize, Idx;
  ULONG RelocDelta, NumRelocs;
  DWORD CurrentSize, TotalRelocs;
  PVOID DriverBase, CurrentBase, EntryPoint;
  PULONG PEMagic;
  PIMAGE_DOS_HEADER PEDosHeader;
  PIMAGE_FILE_HEADER PEFileHeader;
  PIMAGE_OPTIONAL_HEADER PEOptionalHeader;
  PIMAGE_SECTION_HEADER PESectionHeaders;
  PRELOCATION_DIRECTORY RelocDir;
  PRELOCATION_ENTRY RelocEntry;
  PMODULE Library;
  PVOID *ImportAddressList;
  PULONG FunctionNameList;
  PCHAR pName, SymbolNameBuf;
  PWORD pHint;
  
  /* FIXME: this could be used to load kernel DLLs also, however */
  /*        the image headers should be preserved in such a case */

  DPRINT("Processing PE Driver at module base:%08lx\n", ModuleLoadBase);

  /*  Get header pointers  */
  PEDosHeader = (PIMAGE_DOS_HEADER) ModuleLoadBase;
  PEMagic = (PULONG) ((unsigned int) ModuleLoadBase + 
    PEDosHeader->e_lfanew);
  PEFileHeader = (PIMAGE_FILE_HEADER) ((unsigned int) ModuleLoadBase + 
    PEDosHeader->e_lfanew + sizeof(ULONG));
  PEOptionalHeader = (PIMAGE_OPTIONAL_HEADER) ((unsigned int) ModuleLoadBase + 
    PEDosHeader->e_lfanew + sizeof(ULONG) + sizeof(IMAGE_FILE_HEADER));
  PESectionHeaders = (PIMAGE_SECTION_HEADER) ((unsigned int) ModuleLoadBase + 
    PEDosHeader->e_lfanew + sizeof(ULONG) + sizeof(IMAGE_FILE_HEADER) +
    sizeof(IMAGE_OPTIONAL_HEADER));
  CHECKPOINT;

  /*  Check file magic numbers  */
  if (PEDosHeader->e_magic != IMAGE_DOS_MAGIC)
    {
      DPRINT("Incorrect MZ magic: %04x\n", PEDosHeader->e_magic);
      return STATUS_UNSUCCESSFUL;
    }
  if (PEDosHeader->e_lfanew == 0)
    {
      DPRINT("Invalid lfanew offset: %08x\n", PEDosHeader->e_lfanew);
      return STATUS_UNSUCCESSFUL;
    }
  if (*PEMagic != IMAGE_PE_MAGIC)
    {
      DPRINT("Incorrect PE magic: %08x\n", *PEMagic);
      return STATUS_UNSUCCESSFUL;
    }
  if (PEFileHeader->Machine != IMAGE_FILE_MACHINE_I386)
    {
      DPRINT("Incorrect Architechture: %04x\n", PEFileHeader->Machine);
      return STATUS_UNSUCCESSFUL;
    }
  CHECKPOINT;

  /* FIXME: if image is fixed-address load, then fail  */

  /* FIXME: check/verify OS version number  */

  DPRINT("OptionalHdrMagic:%04x LinkVersion:%d.%d\n", 
         PEOptionalHeader->Magic,
         PEOptionalHeader->MajorLinkerVersion,
         PEOptionalHeader->MinorLinkerVersion);
  DPRINT("Entry Point:%08lx\n", PEOptionalHeader->AddressOfEntryPoint);
  CHECKPOINT;

  /*  Determine the size of the module  */
     
   DriverSize = PEOptionalHeader->SizeOfImage;
   DPRINT("DriverSize %x\n",DriverSize);
   
  /*  Allocate a virtual section for the module  */  
  DriverBase = MmAllocateSection(DriverSize);
  if (DriverBase == 0)
    {
      DbgPrint("Failed to allocate a virtual section for driver\n");
      return STATUS_INSUFFICIENT_RESOURCES;
    }
  CHECKPOINT;
   DbgPrint("Module is at base %x\n",DriverBase);
   
  /*  Copy image sections into virtual section  */
  memcpy(DriverBase, ModuleLoadBase, PESectionHeaders[0].PointerToRawData);
  CurrentBase = (PVOID) ((DWORD)DriverBase + PESectionHeaders[0].PointerToRawData);
   CurrentSize = 0;
  for (Idx = 0; Idx < PEFileHeader->NumberOfSections; Idx++)
    {
      /*  Copy current section into current offset of virtual section  */
      if (PESectionHeaders[Idx].Characteristics & 
          (IMAGE_SECTION_CHAR_CODE | IMAGE_SECTION_CHAR_DATA))
        {
	   DPRINT("PESectionHeaders[Idx].VirtualAddress + DriverBase %x\n",
		  PESectionHeaders[Idx].VirtualAddress + DriverBase);
          memcpy(PESectionHeaders[Idx].VirtualAddress + DriverBase,
                 (PVOID)(ModuleLoadBase + PESectionHeaders[Idx].PointerToRawData),
                 PESectionHeaders[Idx].SizeOfRawData);
        }
      else
        {
	   DPRINT("PESectionHeaders[Idx].VirtualAddress + DriverBase %x\n",
		  PESectionHeaders[Idx].VirtualAddress + DriverBase);
	   memset(PESectionHeaders[Idx].VirtualAddress + DriverBase, 
		  '\0', PESectionHeaders[Idx].SizeOfRawData);
        }
      CurrentSize += ROUND_UP(PESectionHeaders[Idx].SizeOfRawData,
                              PEOptionalHeader->SectionAlignment);
      CurrentBase = (PVOID)((DWORD)CurrentBase + 
        ROUND_UP(PESectionHeaders[Idx].SizeOfRawData,
                 PEOptionalHeader->SectionAlignment));
    }

  /*  Perform relocation fixups  */
  RelocDelta = (DWORD) DriverBase - PEOptionalHeader->ImageBase;
  RelocDir = (PRELOCATION_DIRECTORY)(PEOptionalHeader->DataDirectory[
    IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
  DPRINT("DrvrBase:%08lx ImgBase:%08lx RelocDelta:%08lx\n", 
         DriverBase,
         PEOptionalHeader->ImageBase,
         RelocDelta);
  DPRINT("RelocDir %x\n",RelocDir);
  for (Idx = 0; Idx < PEFileHeader->NumberOfSections; Idx++)
    {
       if (PESectionHeaders[Idx].VirtualAddress == (DWORD)RelocDir)
	 {
	    DPRINT("Name %.8s PESectionHeader[Idx].PointerToRawData %x\n",
		   PESectionHeaders[Idx].Name,
		   PESectionHeaders[Idx].PointerToRawData);
	    RelocDir = PESectionHeaders[Idx].PointerToRawData +
	      ModuleLoadBase;
            CurrentSize = PESectionHeaders[Idx].Misc.VirtualSize;
	    break;
	 }
    }
  DPRINT("RelocDir %08lx CurrentSize %08lx\n", RelocDir, CurrentSize);
  TotalRelocs = 0;
  while (TotalRelocs < CurrentSize && RelocDir->SizeOfBlock != 0)
    {
      NumRelocs = (RelocDir->SizeOfBlock - sizeof(RELOCATION_DIRECTORY)) / 
        sizeof(WORD);
/*      DPRINT("RelocDir at %08lx for VA %08lx with %08lx relocs\n",
             RelocDir, 
             RelocDir->VirtualAddress,
             NumRelocs);*/
      RelocEntry = (PRELOCATION_ENTRY) ((DWORD)RelocDir + 
        sizeof(RELOCATION_DIRECTORY));
      for (Idx = 0; Idx < NumRelocs; Idx++)
        {
	   ULONG Offset;
	   ULONG Type;
	   PDWORD RelocItem;
	   
	   Offset = RelocEntry[Idx].TypeOffset & 0xfff;
	   Type = (RelocEntry[Idx].TypeOffset >> 12) & 0xf;
	   RelocItem = (PDWORD)(DriverBase + RelocDir->VirtualAddress + 
				Offset);
/*	   DPRINT("  reloc at %08lx %x %s old:%08lx new:%08lx\n", 
		  RelocItem,
		  Type,
		  Type ? "HIGHLOW" : "ABS",
		  *RelocItem,
		  (*RelocItem) + RelocDelta); */
          if (Type == 3)
            {
              (*RelocItem) += RelocDelta;
            }
          else if (Type != 0)
            {
              DPRINT("Unknown relocation type %x\n",Type);
              return STATUS_UNSUCCESSFUL;
            }
        }
      TotalRelocs += RelocDir->SizeOfBlock;
      RelocDir = (PRELOCATION_DIRECTORY)((DWORD)RelocDir + 
        RelocDir->SizeOfBlock);
//      DPRINT("TotalRelocs: %08lx  CurrentSize: %08lx\n", TotalRelocs, CurrentSize);
    }
   
  DPRINT("PEOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT] %x\n",
         PEOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
         .VirtualAddress);
  /*  Perform import fixups  */
  if (PEOptionalHeader->DataDirectory[
      IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress)
    {
      PIMAGE_IMPORT_MODULE_DIRECTORY ImportModuleDirectory;

      SymbolNameBuf = ExAllocatePool(NonPagedPool, 512);

      /*  Process each import module  */
      ImportModuleDirectory = (PIMAGE_IMPORT_MODULE_DIRECTORY)
        ((DWORD)DriverBase + PEOptionalHeader->
          DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
      while (ImportModuleDirectory->dwRVAModuleName)
        {
          /* FIXME: handle kernel mode DLLs  */

          /*  Check to make sure that import lib is kernel  */
          Library = NULL;
          pName = (PCHAR) DriverBase + 
            ImportModuleDirectory->dwRVAModuleName;
//          DPRINT("Import module: %s\n", pName);
          if (strcmp(pName, "ntoskrnl.exe")!=0 && 
              strcmp(pName, "HAL.dll")!=0)
            {
              DPRINT("Kernel mode DLLs are currently unsupported\n");
            }

          /*  Get the import address list  */
          ImportAddressList = (PVOID *) ((DWORD)DriverBase + 
            ImportModuleDirectory->dwRVAFunctionAddressList);

          /*  Get the list of functions to import  */
          if (ImportModuleDirectory->dwRVAFunctionNameList != 0)
            {
              FunctionNameList = (PULONG) ((DWORD)DriverBase + 
                ImportModuleDirectory->dwRVAFunctionNameList);
            }
          else
            {
              FunctionNameList = (PULONG) ((DWORD)DriverBase + 
                ImportModuleDirectory->dwRVAFunctionAddressList);
            }

          /*  Walk through function list and fixup addresses  */
          while(*FunctionNameList != 0L)
            {
              if ((*FunctionNameList) & 0x80000000) // hint
                {
//                  DPRINT("  Hint: %08lx\n", *FunctionNameList);
                  if (Library == NULL)
                    {
                      DPRINT("Hints for kernel symbols are not handled.\n");
                      *ImportAddressList = 0;
                    }
                }
              else // hint-name
                {
                  pName = (PCHAR)((DWORD)DriverBase+ 
                                  *FunctionNameList + 2);
                  pHint = (PWORD)((DWORD)DriverBase + *FunctionNameList);
 //                 DPRINT("  Hint:%04x  Name:%s\n", pHint, pName);
                  
                  /*  Get address for symbol  */
                  if (Library == NULL)
                    {
                      *SymbolNameBuf = '_';
                      strcpy(SymbolNameBuf + 1, pName);
                      *ImportAddressList = (PVOID) LdrGetKernelSymbolAddr(SymbolNameBuf);                      if (*ImportAddressList == 0L)
                        {
                          DPRINT("Unresolved kernel symbol: %s\n", pName);
                        }
                    }
                }
              ImportAddressList++;
              FunctionNameList++;
            }
          ImportModuleDirectory++;
        }

      ExFreePool(SymbolNameBuf);
    }

  /*  Compute address of entry point  */
  EntryPoint = (PVOID) ((DWORD)DriverBase + PEOptionalHeader->AddressOfEntryPoint);
   DbgPrint("Calling entrypoint at %x\n",EntryPoint);

  return IoInitializeDriver(EntryPoint); 
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

  return IoInitializeDriver(EntryRoutine);
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
      DPRINT("vaddr %x symndex %x", 
             Relocation->r_vaddr, 
             Relocation->r_symndx);

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
            DPRINT("%.8s: Unknown relocation type %x at %d in module\n",
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
  DPRINT("ADDR32 loc %x value %x *loc %x\n", Location, Value, *Location);
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
  Value = (unsigned int) LdrGetKernelSymbolAddr(Name);
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
  DPRINT("name %s ", Name);
   
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
LdrGetKernelSymbolAddr(char *Name)
{
  int i = 0;
   char* s;
   
   if ((s=strchr(Name,'@'))!=NULL)
     {
	*s=0;
	DbgPrint("Name %s ",Name);
     }
  while (symbol_table[i].name != NULL)
    {
      if (strcmp(symbol_table[i].name, Name) == 0)
        {
	   if (s!=NULL)
	     {
		*s=0;
		DbgPrint("Matched with %s\n",symbol_table[i].name);
	     }
          return symbol_table[i].value;
        }
      i++;
    }
   if (s!=NULL)
     {
	*s=0;
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

