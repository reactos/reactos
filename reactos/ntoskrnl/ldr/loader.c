/* $Id: loader.c,v 1.46 2000/02/13 16:05:18 dwelch Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ldr/loader.c
 * PURPOSE:         Loaders for PE executables
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 *                  Jason Filby (jasonfilby@yahoo.com)
 * UPDATE HISTORY:
 *   DW   22/05/98   Created
 *   RJJ  10/12/98   Completed image loader function and added hooks for MZ/PE
 *   RJJ  10/12/98   Built driver loader function and added hooks for PE/COFF
 *   RJJ  10/12/98   Rolled in David's code to load COFF drivers
 *   JM   14/12/98   Built initial PE user module loader
 *   RJJ  06/03/99   Moved user PE loader into NTDLL
 *   JF   26/01/2000 Recoded some parts to retrieve export details correctly
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
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

#define NDEBUG
#include <internal/debug.h>

#include "syspath.h"


/* FIXME: this should appear in a kernel header file  */
NTSTATUS IoInitializeDriver(PDRIVER_INITIALIZE DriverEntry);

/* MACROS ********************************************************************/

#define  MODULE_ROOT_NAME  L"\\Modules\\"

/* GLOBALS *******************************************************************/

LIST_ENTRY ModuleListHead;
POBJECT_TYPE ObModuleType = NULL;

/* FORWARD DECLARATIONS ******************************************************/

NTSTATUS LdrLoadDriver(PUNICODE_STRING Filename);
NTSTATUS LdrProcessDriver(PVOID ModuleLoadBase);

PMODULE_OBJECT  LdrLoadModule(PUNICODE_STRING Filename);
PMODULE_OBJECT  LdrProcessModule(PVOID ModuleLoadBase, PUNICODE_STRING ModuleName);
PVOID  LdrGetExportAddress(PMODULE_OBJECT ModuleObject, char *Name, unsigned short Hint);
static PMODULE_OBJECT LdrOpenModule(PUNICODE_STRING  Filename);
static NTSTATUS LdrCreateModule(PVOID ObjectBody,
                                PVOID Parent,
                                PWSTR RemainingPath,
                                POBJECT_ATTRIBUTES ObjectAttributes);

/*  PE Driver load support  */
static PMODULE_OBJECT  LdrPEProcessModule(PVOID ModuleLoadBase, PUNICODE_STRING ModuleName);
static PVOID  LdrPEGetExportAddress(PMODULE_OBJECT ModuleObject, 
                                    char *Name, 
                                    unsigned short Hint);
#if 0
static unsigned int LdrGetKernelSymbolAddr(char *Name);
#endif
static PIMAGE_SECTION_HEADER LdrPEGetEnclosingSectionHeader(DWORD  RVA,
                                                            PMODULE_OBJECT  ModuleObject);

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
  DPRINT("Create dir: %wZ\n", &ModuleName);
  Status = ZwCreateDirectoryObject(&DirHandle, 0, &ObjectAttributes);
  assert(NT_SUCCESS(Status));

  /*  Add module entry for NTOSKRNL  */
  wcscpy(NameBuffer, MODULE_ROOT_NAME);
  wcscat(NameBuffer, L"ntoskrnl.exe");
  ModuleName.Length = ModuleName.MaximumLength = wcslen(NameBuffer);
  ModuleName.Buffer = NameBuffer;
  DPRINT("Kernel's Module name is: %wZ\n", &ModuleName);
  
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

   InitializeListHead(&ModuleListHead);
   
   /*  Initialize ModuleObject data  */
  ModuleObject->Base = (PVOID) KERNEL_BASE;
  ModuleObject->Flags = MODULE_FLAG_PE;
   InsertTailList(&ModuleListHead, &ModuleObject->ListEntry);
   ModuleObject->Name = wcsdup(L"ntoskrnl.exe");   
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
   ModuleObject->Length = ModuleObject->Image.PE.OptionalHeader->SizeOfImage;
   
  /* FIXME: Add fake module entry for HAL */

}

/*
 * load the auto config drivers.
 */
static VOID LdrLoadAutoConfigDriver (LPWSTR	RelativeDriverName)
{
   WCHAR		TmpFileName [MAX_PATH];
   NTSTATUS	Status;
   UNICODE_STRING	DriverName;
   
   DbgPrint("Loading %S\n",RelativeDriverName);
   
   LdrGetSystemDirectory(TmpFileName, (MAX_PATH * sizeof(WCHAR)));
   wcscat(TmpFileName, L"\\drivers\\");
   wcscat(TmpFileName, RelativeDriverName);

   DriverName.Buffer = TmpFileName;
   DriverName.Length = wcslen(TmpFileName) * sizeof (WCHAR);
   DriverName.MaximumLength = DriverName.Length + sizeof(WCHAR);

	
   Status = LdrLoadDriver(&DriverName);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("driver load failed, status (%x)\n", Status);
	KeBugCheck(0);
     }
}


VOID LdrLoadAutoConfigDrivers (VOID)
{
	/*
	 * Keyboard driver
	 */
	LdrLoadAutoConfigDriver( L"keyboard.sys" );
	/*
	 * Raw console driver
	 */
	LdrLoadAutoConfigDriver( L"blue.sys" );
        /*
         * VideoPort driver
         */
        LdrLoadAutoConfigDriver( L"vidport.sys" );
        /*
         * VGA Miniport driver
         */
        LdrLoadAutoConfigDriver( L"vgamp.sys" );
}


static NTSTATUS 
LdrCreateModule(PVOID ObjectBody,
                PVOID Parent,
                PWSTR RemainingPath,
                POBJECT_ATTRIBUTES ObjectAttributes)
{
  DPRINT("LdrCreateModule(ObjectBody %x, Parent %x, RemainingPath %S)\n",
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
 * FUNCTION: Loads a kernel driver
 * ARGUMENTS:
 *         FileName = Driver to load
 * RETURNS: Status
 */

NTSTATUS LdrLoadDriver(PUNICODE_STRING Filename)
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
//  PWSTR  RemainingPath;
  UNICODE_STRING  ModuleName;

  /*  Check for module already loaded  */
  if ((ModuleObject = LdrOpenModule(Filename)) != NULL)
    {
      return  ModuleObject;
    }

  DPRINT("Loading Module %wZ...\n", Filename);

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
      DbgPrint("Could not open module file: %wZ\n", Filename);
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

  /*  Build module object name  */
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
   
   
  ModuleObject = LdrProcessModule(ModuleLoadBase, &ModuleName);

  /*  Cleanup  */
  ExFreePool(ModuleLoadBase);

  return  ModuleObject;
}

NTSTATUS
LdrProcessDriver(PVOID ModuleLoadBase)
{
  PMODULE_OBJECT ModuleObject;

  ModuleObject = LdrProcessModule(ModuleLoadBase, 0);
  if (ModuleObject == 0)
    {
      return STATUS_UNSUCCESSFUL;
    }

  /* FIXME: should we dereference the ModuleObject here?  */

  return IoInitializeDriver(ModuleObject->EntryPoint); 
}

PMODULE_OBJECT
LdrProcessModule(PVOID ModuleLoadBase, PUNICODE_STRING ModuleName)
{
  PIMAGE_DOS_HEADER PEDosHeader;

  /*  If MZ header exists  */
  PEDosHeader = (PIMAGE_DOS_HEADER) ModuleLoadBase;
  if (PEDosHeader->e_magic == IMAGE_DOS_MAGIC && PEDosHeader->e_lfanew != 0L)
    {
      return LdrPEProcessModule(ModuleLoadBase, ModuleName);
    }
#if 0
  if (PEDosHeader->e_magic == IMAGE_DOS_MAGIC)
    {
      return 0;
    }
  else  /*  Assume COFF format and load  */
    {
      return LdrCOFFProcessModule(ModuleLoadBase, ModuleName);
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
      DPRINT("Module %wZ at %p\n", Filename, ModuleObject);

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

typedef char *PSTR;

PMODULE_OBJECT
LdrPEProcessModule(PVOID ModuleLoadBase, PUNICODE_STRING pModuleName)
{
  unsigned int DriverSize, Idx, Idx2;
  ULONG RelocDelta, NumRelocs;
  DWORD CurrentSize, TotalRelocs;
  PVOID DriverBase;
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
  /*  Copy headers over */
  memcpy(DriverBase, ModuleLoadBase, PEOptionalHeader->SizeOfHeaders);
   CurrentSize = 0;
  /*  Copy image sections into virtual section  */
  for (Idx = 0; Idx < PEFileHeader->NumberOfSections; Idx++)
    {
      //  Copy current section into current offset of virtual section
      if (PESectionHeaders[Idx].Characteristics & 
          (IMAGE_SECTION_CHAR_CODE | IMAGE_SECTION_CHAR_DATA))
        {
	   DPRINT("PESectionHeaders[Idx].VirtualAddress + DriverBase %x\n",
		  PESectionHeaders[Idx].VirtualAddress + DriverBase);
           memcpy(PESectionHeaders[Idx].VirtualAddress + DriverBase,
                  (PVOID)(ModuleLoadBase + PESectionHeaders[Idx].PointerToRawData),
                  PESectionHeaders[Idx].Misc.VirtualSize > PESectionHeaders[Idx].SizeOfRawData ? PESectionHeaders[Idx].SizeOfRawData : PESectionHeaders[Idx].Misc.VirtualSize );
        }
      else
        {
	   DPRINT("PESectionHeaders[Idx].VirtualAddress + DriverBase %x\n",
		  PESectionHeaders[Idx].VirtualAddress + DriverBase);
	   memset(PESectionHeaders[Idx].VirtualAddress + DriverBase, 
		  '\0', PESectionHeaders[Idx].Misc.VirtualSize);

        }
      CurrentSize += ROUND_UP(PESectionHeaders[Idx].Misc.VirtualSize,
                              PEOptionalHeader->SectionAlignment);


//      CurrentBase = (PVOID)((DWORD)CurrentBase + 
  //      ROUND_UP(PESectionHeaders[Idx].SizeOfRawData.Misc.VirtualSize,
    //             PEOptionalHeader->SectionAlignment));
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
  if (PEOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress)
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
          DPRINT("Import module: %wZ\n", &ModuleName);

          LibraryModuleObject = LdrLoadModule(&ModuleName);
          if (LibraryModuleObject == 0)
            {
              DbgPrint("Unknown import module: %wZ\n", &ModuleName);
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
		   DbgPrint("Unresolved kernel symbol: %s\n", pName);
                }
              ImportAddressList++;
              FunctionNameList++;
            }
          ImportModuleDirectory++;
        }

      ExFreePool(SymbolNameBuf);
    }

  /*  Create ModuleName string  */
  if (pModuleName != 0)
    {
      wcscpy(NameBuffer, pModuleName->Buffer);
    }
  else
    {
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
    }
  ModuleName.Length = ModuleName.MaximumLength = wcslen(NameBuffer);
  ModuleName.Buffer = NameBuffer;
  DbgPrint("Module name is: %wZ\n", &ModuleName);
  
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
  InsertTailList(&ModuleListHead, &ModuleObject->ListEntry);
   ModuleObject->Name = wcsdup(NameBuffer);
  ModuleObject->EntryPoint = (PVOID) ((DWORD)DriverBase + 
    PEOptionalHeader->AddressOfEntryPoint);
   ModuleObject->Length = DriverSize;
  DPRINT("entrypoint at %x\n", ModuleObject->EntryPoint);

  ModuleObject->Image.PE.FileHeader =
    (PIMAGE_FILE_HEADER) ((unsigned int) DriverBase + PEDosHeader->e_lfanew + sizeof(ULONG));

  DPRINT("FileHeader at %x\n", ModuleObject->Image.PE.FileHeader);
  ModuleObject->Image.PE.OptionalHeader = 
    (PIMAGE_OPTIONAL_HEADER) ((unsigned int) DriverBase + PEDosHeader->e_lfanew + sizeof(ULONG) +
    sizeof(IMAGE_FILE_HEADER));
  DPRINT("OptionalHeader at %x\n", ModuleObject->Image.PE.OptionalHeader);
  ModuleObject->Image.PE.SectionList = 
    (PIMAGE_SECTION_HEADER) ((unsigned int) DriverBase + PEDosHeader->e_lfanew + sizeof(ULONG) +
    sizeof(IMAGE_FILE_HEADER) + sizeof(IMAGE_OPTIONAL_HEADER));
  DPRINT("SectionList at %x\n", ModuleObject->Image.PE.SectionList);

  return  ModuleObject;
}

static PVOID  
LdrPEGetExportAddress(PMODULE_OBJECT ModuleObject, 
                      char *Name, 
                      unsigned short Hint)
{
  WORD  Idx;
  DWORD  ExportsStartRVA, ExportsEndRVA;
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

  ExportDirectory = MakePtr(PIMAGE_EXPORT_DIRECTORY,
                            ModuleObject->Base,
                            SectionHeader->VirtualAddress);

  FunctionList = (PDWORD)((DWORD)ExportDirectory->AddressOfFunctions + ModuleObject->Base);
  NameList = (PDWORD)((DWORD)ExportDirectory->AddressOfNames + ModuleObject->Base);
  OrdinalList = (PWORD)((DWORD)ExportDirectory->AddressOfNameOrdinals + ModuleObject->Base);

  ExportAddress = 0;

  if (Name != NULL)
    {
      for (Idx = 0; Idx < ExportDirectory->NumberOfNames; Idx++)
        {
#if 0
          DPRINT("  Name:%s  NameList[%d]:%s\n", 
                 Name, 
                 Idx, 
                 (DWORD) ModuleObject->Base + NameList[Idx]);

#endif
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
for(;;) ;
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
