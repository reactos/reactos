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
#include <internal/mmhal.h>
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

#define  MODULE_ROOT_NAME  L"\\Modules\\"

/* GLOBALS *******************************************************************/

POBJECT_TYPE ObModuleType = NULL;

/* FORWARD DECLARATIONS ******************************************************/

NTSTATUS LdrLoadDriver(PUNICODE_STRING Filename);
NTSTATUS LdrProcessDriver(PVOID ModuleLoadBase);

PMODULE_OBJECT  LdrLoadModule(PUNICODE_STRING Filename);
PMODULE_OBJECT  LdrProcessModule(PVOID ModuleLoadBase);
PVOID  LdrGetExportAddress(PMODULE_OBJECT ModuleObject, char *Name, unsigned short Hint);
static PMODULE_OBJECT LdrOpenModule(PUNICODE_STRING  Filename);
static PIMAGE_SECTION_HEADER LdrPEGetEnclosingSectionHeader(DWORD  RVA,
                                                            PMODULE_OBJECT  ModuleObject);
static NTSTATUS LdrCreateModule(PVOID ObjectBody,
                                PVOID Parent,
                                PWSTR RemainingPath,
                                POBJECT_ATTRIBUTES ObjectAttributes);

/*  PE Driver load support  */
static PMODULE_OBJECT  LdrPEProcessModule(PVOID ModuleLoadBase);
static PVOID  LdrPEGetExportAddress(PMODULE_OBJECT ModuleObject, 
                                    char *Name, 
                                    unsigned short Hint);
static unsigned int LdrGetKernelSymbolAddr(char *Name);

/*  COFF Driver load support  */
static PMODULE_OBJECT  LdrCOFFProcessModule(PVOID ModuleLoadBase);
static BOOLEAN LdrCOFFDoRelocations(module *Module, unsigned int SectionIndex);
static BOOLEAN LdrCOFFDoAddr32Reloc(module *Module, SCNHDR *Section, RELOC *Relocation);
static BOOLEAN LdrCOFFDoReloc32Reloc(module *Module, SCNHDR *Section, RELOC *Relocation);
static void LdrCOFFGetSymbolName(module *Module, unsigned int Idx, char *Name);
static unsigned int LdrCOFFGetSymbolValue(module *Module, unsigned int Idx);
static unsigned int LdrCOFFGetSymbolValueByName(module *Module, char *SymbolName, unsigned int Idx);

/* FUNCTIONS *****************************************************************/

VOID LdrInitModuleManagement(VOID)
{
  HANDLE DirHandle, ModuleHandle;
  NTSTATUS Status;
  WCHAR NameBuffer[60];
  ANSI_STRING AnsiString;
  UNICODE_STRING ModuleName;
  OBJECT_ATTRIBUTES ObjectAttributes;
  PIMAGE_DOS_HEADER DosHeader;
  PMODULE_OBJECT ModuleObject;

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
  ObModuleType->Create = LdrCreateModule;
  RtlInitAnsiString(&AnsiString, "Module");
  RtlAnsiStringToUnicodeString(&ObModuleType->TypeName, &AnsiString, TRUE);

  /*  Create Modules object directory  */
  wcscpy(NameBuffer, MODULE_ROOT_NAME);
  *(wcsrchr(NameBuffer, L'\\')) = 0;
  ModuleName.Length = ModuleName.MaximumLength = wcslen(NameBuffer);
  ModuleName.Buffer = NameBuffer;
  InitializeObjectAttributes(&ObjectAttributes, 
                             &ModuleName, 
                             0, 
                             NULL, 
                             NULL);
  DPRINT("Create dir: %W\n", &ModuleName);
  Status = ZwCreateDirectoryObject(&DirHandle, 0, &ObjectAttributes);
  assert(NT_SUCCESS(Status));

  /*  Add module entry for NTOSKRNL  */
  wcscpy(NameBuffer, MODULE_ROOT_NAME);
  wcscat(NameBuffer, L"ntoskrnl.exe");
  ModuleName.Length = ModuleName.MaximumLength = wcslen(NameBuffer);
  ModuleName.Buffer = NameBuffer;
  DPRINT("Kernel's Module name is: %W\n", &ModuleName);
  
  /*  Initialize ObjectAttributes for ModuleObject  */
  InitializeObjectAttributes(&ObjectAttributes, 
                             &ModuleName, 
                             0, 
                             NULL, 
                             NULL);

  /*  Create module object  */
  ModuleHandle = 0;
  ModuleObject = ObCreateObject(&ModuleHandle,
                                STANDARD_RIGHTS_REQUIRED,
                                &ObjectAttributes,
                                ObModuleType);
  assert(ModuleObject != NULL);

  /*  Initialize ModuleObject data  */
  ModuleObject->Base = (PVOID) KERNEL_BASE;
  ModuleObject->Flags = MODULE_FLAG_PE;
  DosHeader = (PIMAGE_DOS_HEADER) KERNEL_BASE;
  ModuleObject->Image.PE.FileHeader = 
    (PIMAGE_FILE_HEADER) ((DWORD) ModuleObject->Base + 
    DosHeader->e_lfanew + sizeof(ULONG));
  ModuleObject->Image.PE.OptionalHeader = (PIMAGE_OPTIONAL_HEADER) 
    ((DWORD)ModuleObject->Image.PE.FileHeader + sizeof(IMAGE_FILE_HEADER));
  ModuleObject->Image.PE.SectionList = (PIMAGE_SECTION_HEADER) 
    ((DWORD)ModuleObject->Image.PE.OptionalHeader + sizeof(IMAGE_OPTIONAL_HEADER));
  ModuleObject->EntryPoint = (PVOID) ((DWORD) ModuleObject->Base + 
    ModuleObject->Image.PE.OptionalHeader->AddressOfEntryPoint);
  DPRINT("ModuleObject:%08x  entrypoint at %x\n", ModuleObject, ModuleObject->EntryPoint);

  /* FIXME: Add fake module entry for HAL */

}

static NTSTATUS 
LdrCreateModule(PVOID ObjectBody,
                PVOID Parent,
                PWSTR RemainingPath,
                POBJECT_ATTRIBUTES ObjectAttributes)
{
  DPRINT("LdrCreateModule(ObjectBody %x, Parent %x, RemainingPath %w)\n",
         ObjectBody, 
         Parent, 
         RemainingPath);
  if (RemainingPath != NULL && wcschr(RemainingPath + 1, '\\') != NULL)
    {
      return  STATUS_UNSUCCESSFUL;
    }
  if (Parent != NULL && RemainingPath != NULL)
    {
      ObAddEntryDirectory(Parent, ObjectBody, RemainingPath + 1);
    }

  return  STATUS_SUCCESS;
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
  PMODULE_OBJECT  ModuleObject;

  ModuleObject = LdrLoadModule(Filename);
  if (ModuleObject == 0)
    {
      return  STATUS_UNSUCCESSFUL;
    }

  /* FIXME: should we dereference the ModuleObject here?  */

  return IoInitializeDriver(ModuleObject->EntryPoint); 
}
  
PMODULE_OBJECT 
LdrLoadModule(PUNICODE_STRING Filename)
{
  PVOID ModuleLoadBase;
  NTSTATUS Status;
  HANDLE FileHandle;
  OBJECT_ATTRIBUTES ObjectAttributes;
  PMODULE_OBJECT  ModuleObject;
  FILE_STANDARD_INFORMATION FileStdInfo;
  WCHAR  NameBuffer[60];
  PWSTR  RemainingPath;
  UNICODE_STRING  ModuleName;

  /*  Check for module already loaded  */
  if ((ModuleObject = LdrOpenModule(Filename)) != NULL)
    {
      return  ModuleObject;
    }

  DPRINT("Loading Module %W...\n", Filename);

  /*  Open the Module  */
  InitializeObjectAttributes(&ObjectAttributes,
                             Filename, 
                             0,
                             NULL,
                             NULL);
  CHECKPOINT;
  Status = ZwOpenFile(&FileHandle, 
                      FILE_ALL_ACCESS, 
                      &ObjectAttributes, 
                      NULL, 0, 0);
  CHECKPOINT;
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Could not open module file: %W\n", Filename);
      return  0;
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
      DbgPrint("Could not get file size\n");
      return  0;
    }
  CHECKPOINT;

  /*  Allocate nonpageable memory for driver  */
  ModuleLoadBase = ExAllocatePool(NonPagedPool,
                                  FileStdInfo.EndOfFile.u.LowPart);
  if (ModuleLoadBase == NULL)
    {
      DbgPrint("could not allocate memory for module");
      return  0;
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
      DbgPrint("could not read module file into memory");
      ExFreePool(ModuleLoadBase);

      return  0;
    }
  CHECKPOINT;

  ZwClose(FileHandle);

  ModuleObject = LdrProcessModule(ModuleLoadBase);

  /*  Cleanup  */
  ExFreePool(ModuleLoadBase);

  return  ModuleObject;
}

NTSTATUS
LdrProcessDriver(PVOID ModuleLoadBase)
{
  PMODULE_OBJECT ModuleObject;

  ModuleObject = LdrProcessModule(ModuleLoadBase);
  if (ModuleObject == 0)
    {
      return STATUS_UNSUCCESSFUL;
    }

  /* FIXME: should we dereference the ModuleObject here?  */

  return IoInitializeDriver(ModuleObject->EntryPoint); 
}

PMODULE_OBJECT
LdrProcessModule(PVOID ModuleLoadBase)
{
  PIMAGE_DOS_HEADER PEDosHeader;

  /*  If MZ header exists  */
  PEDosHeader = (PIMAGE_DOS_HEADER) ModuleLoadBase;
  if (PEDosHeader->e_magic == IMAGE_DOS_MAGIC && PEDosHeader->e_lfanew != 0L)
    {
      return LdrPEProcessModule(ModuleLoadBase);
    }
#if 0
  if (PEDosHeader->e_magic == IMAGE_DOS_MAGIC)
    {
      return 0;
    }
  else  /*  Assume COFF format and load  */
    {
      return LdrCOFFProcessModule(ModuleLoadBase);
    }
#endif

  return 0;
}

static PMODULE_OBJECT 
LdrOpenModule(PUNICODE_STRING  Filename)
{
  NTSTATUS  Status;
  WCHAR  NameBuffer[60];
  UNICODE_STRING  ModuleName;
  OBJECT_ATTRIBUTES  ObjectAttributes;
  PMODULE_OBJECT  ModuleObject;
  PWSTR  RemainingPath;

  wcscpy(NameBuffer, MODULE_ROOT_NAME);
  if (wcsrchr(Filename->Buffer, '\\') != 0)
    {
      wcscat(NameBuffer, wcsrchr(Filename->Buffer, '\\') + 1);
    }
  else
    {
      wcscat(NameBuffer, Filename->Buffer);
    }
  ModuleName.Length = ModuleName.MaximumLength = wcslen(NameBuffer);
  ModuleName.Buffer = NameBuffer;
  InitializeObjectAttributes(&ObjectAttributes,
                             &ModuleName, 
                             0,
                             NULL,
                             NULL);
  Status = ObFindObject(&ObjectAttributes,
                        (PVOID *) &ModuleObject,
                        &RemainingPath);
  CHECKPOINT;
  if (NT_SUCCESS(Status) && (RemainingPath == NULL || *RemainingPath == 0))
    {
      DPRINT("Module %W at %p\n", Filename, ModuleObject);

      return  ModuleObject;
    }

  return  NULL;
}

PVOID  
LdrGetExportAddress(PMODULE_OBJECT ModuleObject, 
                    char *Name, 
                    unsigned short Hint)
{
  if (ModuleObject->Flags & MODULE_FLAG_PE)
    {
      return LdrPEGetExportAddress(ModuleObject, Name, Hint);
    }
  else
    {
      return 0;
    }
}

/*  ----------------------------------------------  PE Module support */

PMODULE_OBJECT
LdrPEProcessModule(PVOID ModuleLoadBase)
{
  unsigned int DriverSize, Idx, Idx2;
  ULONG RelocDelta, NumRelocs;
  DWORD CurrentSize, TotalRelocs;
  PVOID DriverBase, CurrentBase;
  PULONG PEMagic;
  PIMAGE_DOS_HEADER PEDosHeader;
  PIMAGE_FILE_HEADER PEFileHeader;
  PIMAGE_OPTIONAL_HEADER PEOptionalHeader;
  PIMAGE_SECTION_HEADER PESectionHeaders;
  PIMAGE_EXPORT_DIRECTORY  ExportDirectory;
  PRELOCATION_DIRECTORY RelocDir;
  PRELOCATION_ENTRY RelocEntry;
  PMODULE_OBJECT  LibraryModuleObject;
  HANDLE  ModuleHandle;
  PMODULE_OBJECT  ModuleObject;
  PVOID *ImportAddressList;
  PULONG FunctionNameList;
  PCHAR pName, SymbolNameBuf;
  WORD Hint;
  OBJECT_ATTRIBUTES  ObjectAttributes;
  UNICODE_STRING  ModuleName;
  WCHAR  NameBuffer[60];
  
  DPRINT("Processing PE Module at module base:%08lx\n", ModuleLoadBase);

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
      DbgPrint("Incorrect MZ magic: %04x\n", PEDosHeader->e_magic);
      return 0;
    }
  if (PEDosHeader->e_lfanew == 0)
    {
      DbgPrint("Invalid lfanew offset: %08x\n", PEDosHeader->e_lfanew);
      return 0;
    }
  if (*PEMagic != IMAGE_PE_MAGIC)
    {
      DbgPrint("Incorrect PE magic: %08x\n", *PEMagic);
      return 0;
    }
  if (PEFileHeader->Machine != IMAGE_FILE_MACHINE_I386)
    {
      DbgPrint("Incorrect Architechture: %04x\n", PEFileHeader->Machine);
      return 0;
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
      return 0;
    }
  CHECKPOINT;
   DPRINT("Module is at base %x\n",DriverBase);
   
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
              DbgPrint("Unknown relocation type %x\n",Type);
              return 0;
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
      DPRINT("Processeing import directory at %p\n", ImportModuleDirectory);
      while (ImportModuleDirectory->dwRVAModuleName)
        {
          /*  Check to make sure that import lib is kernel  */
          pName = (PCHAR) DriverBase + 
            ImportModuleDirectory->dwRVAModuleName;
#if 0
          if (!strcmp(pName, "ntoskrnl.exe") || !strcmp(pName, "HAL.dll"))
            {
              LibraryModuleObject = NULL;
              DPRINT("Kernel imports\n");
            }
          else
#endif
            {
              wcscpy(NameBuffer, MODULE_ROOT_NAME);
              for (Idx = 0; NameBuffer[Idx] != 0; Idx++)
                ;
              for (Idx2 = 0; pName[Idx2] != '\0'; Idx2++)
                {
                  NameBuffer[Idx + Idx2] = (WCHAR) pName[Idx2];
                }
              NameBuffer[Idx + Idx2] = 0;
              ModuleName.Length = ModuleName.MaximumLength = wcslen(NameBuffer);
              ModuleName.Buffer = NameBuffer;
              DPRINT("Import module: %W\n", &ModuleName);
              LibraryModuleObject = LdrLoadModule(&ModuleName);
              if (LibraryModuleObject == 0)
                {
                  DbgPrint("Unknown import module: %W\n", &ModuleName);
                }
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
          while (*FunctionNameList != 0L)
            {
              if ((*FunctionNameList) & 0x80000000) // hint
                {
                  pName = NULL;
                  Hint = (*FunctionNameList) & 0xffff;
                }
              else // hint-name
                {
                  pName = (PCHAR)((DWORD)DriverBase + 
                                  *FunctionNameList + 2);
                  Hint = *(PWORD)((DWORD)DriverBase + *FunctionNameList);
                }
              DPRINT("  Hint:%04x  Name:%s\n", Hint, pName);

              /*  Fixup the current import symbol  */
              if (LibraryModuleObject != NULL)
                {
                  *ImportAddressList = LdrGetExportAddress(LibraryModuleObject, 
                                                           pName, 
                                                           Hint);
                }
              else
                {
                  /*  Get address for symbol  */
                  *SymbolNameBuf = '_';
                  strcpy(SymbolNameBuf + 1, pName);
                  *ImportAddressList = (PVOID) LdrGetKernelSymbolAddr(SymbolNameBuf);
                  if (*ImportAddressList == 0L)
                    {
                     DbgPrint("Unresolved kernel symbol: %s\n", pName);
                    }
                }
              ImportAddressList++;
              FunctionNameList++;
            }
          ImportModuleDirectory++;
        }

      ExFreePool(SymbolNameBuf);
    }

  /*  Create ModuleName string  */
  wcscpy(NameBuffer, MODULE_ROOT_NAME);
  if (PEOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]
        .VirtualAddress != 0)
    {
      ExportDirectory = (PIMAGE_EXPORT_DIRECTORY) (DriverBase +
        PEOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]
          .VirtualAddress);
      wcscat(NameBuffer, DriverBase + ExportDirectory->Name);
    }
  else
    {
      char buf[12];

      sprintf(buf, "%08X", (DWORD) DriverBase);
      for (Idx = 0; NameBuffer[Idx] != 0; Idx++)
        ;
      Idx2 = 0;
      while ((NameBuffer[Idx + Idx2] = (WCHAR) buf[Idx2]) != 0)
        Idx2++;
    }
  ModuleName.Length = ModuleName.MaximumLength = wcslen(NameBuffer);
  ModuleName.Buffer = NameBuffer;
  DPRINT("Module name is: %W\n", &ModuleName);
  
  /*  Initialize ObjectAttributes for ModuleObject  */
  InitializeObjectAttributes(&ObjectAttributes, 
                             &ModuleName, 
                             0, 
                             NULL, 
                             NULL);

  /*  Create module object  */
  ModuleHandle = 0;
  ModuleObject = ObCreateObject(&ModuleHandle,
                                STANDARD_RIGHTS_REQUIRED,
                                &ObjectAttributes,
                                ObModuleType);

  /*  Initialize ModuleObject data  */
  ModuleObject->Base = DriverBase;
  ModuleObject->Flags = MODULE_FLAG_PE;
  ModuleObject->EntryPoint = (PVOID) ((DWORD)DriverBase + 
    PEOptionalHeader->AddressOfEntryPoint);
  DPRINT("entrypoint at %x\n", ModuleObject->EntryPoint);
  ModuleObject->Image.PE.FileHeader = 
    (PIMAGE_FILE_HEADER) ((unsigned int) DriverBase + 
    PEDosHeader->e_lfanew + sizeof(ULONG));
  DPRINT("FileHeader at %x\n", ModuleObject->Image.PE.FileHeader);
  ModuleObject->Image.PE.OptionalHeader = 
    (PIMAGE_OPTIONAL_HEADER) ((unsigned int) DriverBase + 
    PEDosHeader->e_lfanew + sizeof(ULONG) + sizeof(IMAGE_FILE_HEADER));
  DPRINT("OptionalHeader at %x\n", ModuleObject->Image.PE.OptionalHeader);
  ModuleObject->Image.PE.SectionList = 
    (PIMAGE_SECTION_HEADER) ((unsigned int) DriverBase + 
    PEDosHeader->e_lfanew + sizeof(ULONG) + sizeof(IMAGE_FILE_HEADER) +
    sizeof(IMAGE_OPTIONAL_HEADER));
  DPRINT("SectionList at %x\n", ModuleObject->Image.PE.SectionList);

  return  ModuleObject;
}

static PVOID  
LdrPEGetExportAddress(PMODULE_OBJECT ModuleObject, 
                      char *Name, 
                      unsigned short Hint)
{
  WORD  Idx;
  DWORD  ExportsStartRVA, ExportsEndRVA, Delta;
  PVOID  ExportAddress;
  PWORD  OrdinalList;
  PDWORD  FunctionList, NameList;
  PIMAGE_SECTION_HEADER  SectionHeader;
  PIMAGE_EXPORT_DIRECTORY  ExportDirectory;

  ExportsStartRVA = ModuleObject->Image.PE.OptionalHeader->DataDirectory
    [IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
  ExportsEndRVA = ExportsStartRVA + 
    ModuleObject->Image.PE.OptionalHeader->DataDirectory
      [IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
  /*  Get the IMAGE_SECTION_HEADER that contains the exports.  This is
      usually the .edata section, but doesn't have to be.  */
  SectionHeader = LdrPEGetEnclosingSectionHeader(ExportsStartRVA, ModuleObject);
  if (!SectionHeader)
    {
      return 0;
    }
  Delta = (DWORD)(SectionHeader->VirtualAddress - 
    SectionHeader->PointerToRawData);
  ExportDirectory = MakePtr(PIMAGE_EXPORT_DIRECTORY, 
                            ModuleObject->Base,
                            ExportsStartRVA - Delta);

  FunctionList = (PDWORD)((DWORD)ExportDirectory->AddressOfFunctions - 
    Delta + ModuleObject->Base);
  NameList = (PDWORD)((DWORD)ExportDirectory->AddressOfNames - 
    Delta + ModuleObject->Base);
  OrdinalList = (PWORD)((DWORD)ExportDirectory->AddressOfNameOrdinals - 
    Delta + ModuleObject->Base);
  DPRINT("Delta:%08x\n", Delta);
  DPRINT("Func:%08x  RVA:%08x  Name:%08x  RVA:%08x\nOrd:%08x  RVA:%08x  ", 
         FunctionList, ExportDirectory->AddressOfFunctions, 
         NameList, ExportDirectory->AddressOfNames, 
         OrdinalList, ExportDirectory->AddressOfNameOrdinals);
  DPRINT("NumNames:%d NumFuncs:%d\n", ExportDirectory->NumberOfNames, 
         ExportDirectory->NumberOfFunctions);
  ExportAddress = 0;
  if (Name != NULL)
    {
      for (Idx = 0; Idx < ExportDirectory->NumberOfNames; Idx++)
        {
          DPRINT("  Name:%s  NameList[%d]:%s\n", 
                 Name, 
                 Idx, 
                 (DWORD) ModuleObject->Base + NameList[Idx]);
          if (!strcmp(Name, (PCHAR) ((DWORD)ModuleObject->Base + NameList[Idx])))
            {
              ExportAddress = (PVOID) ((DWORD)ModuleObject->Base +
                FunctionList[OrdinalList[Idx]]);
              break;
            }
        }
    }
  else  /*  use hint  */
    {
      ExportAddress = (PVOID) ((DWORD)ModuleObject->Base +
        FunctionList[Hint - ExportDirectory->Base]);
    }
  if (ExportAddress == 0)
    {
      DbgPrint("Export not found for %d:%s\n", Hint, Name != NULL ? Name : "(Ordinal)");
    }

  return  ExportAddress;
}

static PIMAGE_SECTION_HEADER 
LdrPEGetEnclosingSectionHeader(DWORD  RVA,
                               PMODULE_OBJECT  ModuleObject)
{
  PIMAGE_SECTION_HEADER  SectionHeader = SECHDROFFSET(ModuleObject->Base);
  unsigned  i;
    
  for (i = 0; i < ModuleObject->Image.PE.FileHeader->NumberOfSections; 
       i++, SectionHeader++)
    {
      /*  Is the RVA within this section?  */
      if ((RVA >= SectionHeader->VirtualAddress) && 
          (RVA < (SectionHeader->VirtualAddress + SectionHeader->Misc.VirtualSize)))
        {
          return SectionHeader;
        }
    }
    
  return 0;
}

/*  -------------------------------------------  COFF Module support */

PMODULE_OBJECT 
LdrCOFFProcessModule(PVOID ModuleLoadBase)
{
  BOOLEAN FoundEntry;
  char SymbolName[255];
  int i;
  ULONG EntryOffset;
  FILHDR *FileHeader;
  AOUTHDR *AOUTHeader;
  module *Module;
  PVOID  EntryRoutine;
  HANDLE  ModuleHandle;
  PMODULE_OBJECT  ModuleObject;
  OBJECT_ATTRIBUTES  ObjectAttributes;
  UNICODE_STRING  ModuleName;
  WCHAR  NameBuffer[60];

  /*  Get header pointers  */
  FileHeader = ModuleLoadBase;
  AOUTHeader = ModuleLoadBase + FILHSZ;
  CHECKPOINT;

  /*  Check COFF magic value  */
  if (I386BADMAG(*FileHeader))
    {
      DbgPrint("Module has bad magic value (%x)\n", 
               FileHeader->f_magic);
      return 0;
    }
  CHECKPOINT;
   
  /*  Allocate and initialize a module definition structure  */
  Module = (module *) ExAllocatePool(NonPagedPool, sizeof(module));
  if (Module == NULL)
    {
      return 0;
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
      return 0;
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
              DbgPrint("Relocation failed for section %s\n",
                     Module->scn_list[i].s_name);

              /* FIXME: unallocate all sections here  */

              ExFreePool(Module);

              return 0;
            }
        }
      if (Module->scn_list[i].s_flags & STYP_BSS)
        {
          memset((PVOID)(Module->base + Module->scn_list[i].s_vaddr),
                 0,
                 Module->scn_list[i].s_size);
        }
    }
   
  DPRINT("Module base: %x\n", Module->base);
   
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

      return 0;
    }
   
  /*  Get the address of the module initalization routine  */
  EntryRoutine = (PVOID)(Module->base + EntryOffset);

  /*  Create ModuleName string  */
  wcscpy(NameBuffer, MODULE_ROOT_NAME);
  /* FIXME: someone who is interested needs to fix this.  */
  wcscat(NameBuffer, L"BOGUS.o");
  ModuleName.Length = ModuleName.MaximumLength = wcslen(NameBuffer);
  ModuleName.Buffer = NameBuffer;
  DPRINT("Module name is: %w", NameBuffer);
  
  /*  Initialize ObjectAttributes for ModuleObject  */
  InitializeObjectAttributes(&ObjectAttributes, 
                             &ModuleName, 
                             0, 
                             NULL, 
                             NULL);

  /*  Create module object  */
  ModuleHandle = 0;
  ModuleObject = ObCreateObject(&ModuleHandle,
                                OBJECT_TYPE_ALL_ACCESS,
                                &ObjectAttributes,
                                ObModuleType);

  /*  Initialize ModuleObject data  */
  ModuleObject->Base = (PVOID) Module->base;
  ModuleObject->Flags = MODULE_FLAG_COFF;
  ModuleObject->EntryPoint = (PVOID) (Module->base + EntryOffset);
  DPRINT("entrypoint at %x\n", ModuleObject->EntryPoint);
  /* FIXME: the COFF headers need to be copied into the module 
     space, and the ModuleObject needs to be set to point to them  */

  /*  Cleanup  */
  ExFreePool(Module);

  return  ModuleObject;
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
	DPRINT("Name %s ",Name);
     }
  while (symbol_table[i].name != NULL)
    {
      if (strcmp(symbol_table[i].name, Name) == 0)
        {
	   if (s!=NULL)
	     {
		*s=0;
		DPRINT("Matched with %s\n",symbol_table[i].name);
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

