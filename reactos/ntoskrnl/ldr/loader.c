/* $Id: loader.c,v 1.70 2001/03/27 21:43:43 dwelch Exp $
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
 *   DW   27/06/00   Removed redundant header files
 */

/* INCLUDES *****************************************************************/

#include <limits.h>
#include <ddk/ntddk.h>
#include <internal/module.h>
#include <internal/ntoskrnl.h>
#include <internal/mm.h>
#include <internal/ob.h>
#include <internal/ps.h>
#include <internal/ldr.h>
#include <internal/pool.h>

#define NDEBUG
#include <internal/debug.h>


/* FIXME: this should appear in a kernel header file  */
NTSTATUS IoInitializeDriver(PDRIVER_INITIALIZE DriverEntry);

/* MACROS ********************************************************************/

#define  MODULE_ROOT_NAME  L"\\Modules\\"

/* GLOBALS *******************************************************************/

LIST_ENTRY ModuleListHead;
POBJECT_TYPE EXPORTED IoDriverObjectType = NULL;
LIST_ENTRY ModuleTextListHead;
STATIC MODULE_TEXT_SECTION NtoskrnlTextSection;
/* STATIC MODULE_TEXT_SECTION HalTextSection; */

#define TAG_DRIVER_MEM  TAG('D', 'R', 'V', 'M')
#define TAG_SYM_BUF     TAG('S', 'Y', 'M', 'B')

/* FORWARD DECLARATIONS ******************************************************/

PMODULE_OBJECT  LdrLoadModule(PUNICODE_STRING Filename);
PMODULE_OBJECT  LdrProcessModule(PVOID ModuleLoadBase, PUNICODE_STRING ModuleName);
PVOID  LdrGetExportAddress(PMODULE_OBJECT ModuleObject, char *Name, unsigned short Hint);
static PMODULE_OBJECT LdrOpenModule(PUNICODE_STRING  Filename);
static NTSTATUS LdrCreateModule(PVOID ObjectBody,
                                PVOID Parent,
                                PWSTR RemainingPath,
                                POBJECT_ATTRIBUTES ObjectAttributes);
static VOID LdrpBuildModuleBaseName(PUNICODE_STRING BaseName,
				    PUNICODE_STRING FullName);

/*  PE Driver load support  */
static PMODULE_OBJECT  LdrPEProcessModule(PVOID ModuleLoadBase, PUNICODE_STRING FileName);
static PVOID  LdrPEGetExportAddress(PMODULE_OBJECT ModuleObject,
                                    char *Name,
                                    unsigned short Hint);
static PMODULE_OBJECT LdrPEGetModuleObject(PUNICODE_STRING ModuleName);
static PVOID LdrPEFixupForward(PCHAR ForwardName);


/* FUNCTIONS *****************************************************************/

VOID
LdrInit1(VOID)
{
  PIMAGE_DOS_HEADER DosHeader;
  PIMAGE_FILE_HEADER FileHeader;
  PIMAGE_OPTIONAL_HEADER OptionalHeader;
  PIMAGE_SECTION_HEADER SectionList;

  InitializeListHead(&ModuleTextListHead);

  DosHeader = (PIMAGE_DOS_HEADER) KERNEL_BASE;
  FileHeader =
    (PIMAGE_FILE_HEADER) ((DWORD)KERNEL_BASE + 
			  DosHeader->e_lfanew + sizeof(ULONG));
  OptionalHeader = (PIMAGE_OPTIONAL_HEADER)
    ((DWORD)FileHeader + sizeof(IMAGE_FILE_HEADER));
  SectionList = (PIMAGE_SECTION_HEADER)
    ((DWORD)OptionalHeader + sizeof(IMAGE_OPTIONAL_HEADER));
  NtoskrnlTextSection.Base = KERNEL_BASE;
  NtoskrnlTextSection.Length = SectionList[0].Misc.VirtualSize +
    SectionList[0].VirtualAddress;
  NtoskrnlTextSection.Name = L"ntoskrnl.exe";
  InsertTailList(&ModuleTextListHead, &NtoskrnlTextSection.ListEntry);
}

VOID LdrInitModuleManagement(VOID)
{
  HANDLE DirHandle, ModuleHandle;
  NTSTATUS Status;
  WCHAR NameBuffer[60];
  UNICODE_STRING ModuleName;
  OBJECT_ATTRIBUTES ObjectAttributes;
  PIMAGE_DOS_HEADER DosHeader;
  PMODULE_OBJECT ModuleObject;

  /*  Register the process object type  */
  IoDriverObjectType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));
  IoDriverObjectType->Tag = TAG('D', 'R', 'V', 'T');
  IoDriverObjectType->TotalObjects = 0;
  IoDriverObjectType->TotalHandles = 0;
  IoDriverObjectType->MaxObjects = ULONG_MAX;
  IoDriverObjectType->MaxHandles = ULONG_MAX;
  IoDriverObjectType->PagedPoolCharge = 0;
  IoDriverObjectType->NonpagedPoolCharge = sizeof(MODULE);
  IoDriverObjectType->Dump = NULL;
  IoDriverObjectType->Open = NULL;
  IoDriverObjectType->Close = NULL;
  IoDriverObjectType->Delete = NULL;
  IoDriverObjectType->Parse = NULL;
  IoDriverObjectType->Security = NULL;
  IoDriverObjectType->QueryName = NULL;
  IoDriverObjectType->OkayToClose = NULL;
  IoDriverObjectType->Create = LdrCreateModule;
  RtlInitUnicodeString(&IoDriverObjectType->TypeName, L"Driver");

  /*  Create Modules object directory  */
  wcscpy(NameBuffer, MODULE_ROOT_NAME);
  *(wcsrchr(NameBuffer, L'\\')) = 0;
  RtlInitUnicodeString (&ModuleName, NameBuffer);
  InitializeObjectAttributes(&ObjectAttributes,
                             &ModuleName,
                             0,
                             NULL,
                             NULL);
  DPRINT("Create dir: %wZ\n", &ModuleName);
  Status = NtCreateDirectoryObject(&DirHandle, 0, &ObjectAttributes);
  assert(NT_SUCCESS(Status));

  /*  Add module entry for NTOSKRNL  */
  wcscpy(NameBuffer, MODULE_ROOT_NAME);
  wcscat(NameBuffer, L"ntoskrnl.exe");
  RtlInitUnicodeString (&ModuleName, NameBuffer);
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
                                IoDriverObjectType);
  assert(ModuleObject != NULL);

   InitializeListHead(&ModuleListHead);

   /*  Initialize ModuleObject data  */
   ModuleObject->Base = (PVOID) KERNEL_BASE;
   ModuleObject->Flags = MODULE_FLAG_PE;
   InsertTailList(&ModuleListHead,
		  &ModuleObject->ListEntry);
   RtlCreateUnicodeString(&ModuleObject->FullName,
			  L"ntoskrnl.exe");
   LdrpBuildModuleBaseName(&ModuleObject->BaseName,
			   &ModuleObject->FullName);

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

   wcscpy(TmpFileName, L"\\SystemRoot\\system32\\drivers\\");
   wcscat(TmpFileName, RelativeDriverName);
   RtlInitUnicodeString (&DriverName, TmpFileName);

   Status = LdrLoadDriver(&DriverName);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("driver load failed, status (%x)\n", Status);
//	KeBugCheck(0);
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
    * 
    */
   LdrLoadAutoConfigDriver(L"vidport.sys");
   
   /*
    * 
    */
   LdrLoadAutoConfigDriver(L"vgamp.sys");
   
   /*
    * Minix filesystem driver
    */
   LdrLoadAutoConfigDriver(L"minixfs.sys");
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

NTSTATUS LdrLoadGdiDriver (PUNICODE_STRING DriverName,
			   PVOID *ImageAddress,
			   PVOID *SectionPointer,
			   PVOID *EntryPoint,
			   PVOID *ExportSectionPointer)
{
  PMODULE_OBJECT  ModuleObject;

  ModuleObject = LdrLoadModule(DriverName);
  if (ModuleObject == 0)
    {
      return  STATUS_UNSUCCESSFUL;
    }

  if (ImageAddress)
    *ImageAddress = ModuleObject->Base;

//  if (SectionPointer)
//    *SectionPointer = ModuleObject->

  if (EntryPoint)
    *EntryPoint = ModuleObject->EntryPoint;

//  if (ExportSectionPointer)
//    *ExportSectionPointer = ModuleObject->

  return STATUS_SUCCESS;
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
  Status = NtOpenFile(&FileHandle,
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
  Status = NtQueryInformationFile(FileHandle,
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
  ModuleLoadBase = ExAllocatePoolWithTag(NonPagedPool,
					 FileStdInfo.EndOfFile.u.LowPart,
					 TAG_DRIVER_MEM);

  if (ModuleLoadBase == NULL)
    {
      DbgPrint("could not allocate memory for module");
      return  0;
    }
  CHECKPOINT;
   
  /*  Load driver into memory chunk  */
  Status = NtReadFile(FileHandle, 
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

  NtClose(FileHandle);

  ModuleObject = LdrProcessModule(ModuleLoadBase, Filename);

  /*  Cleanup  */
  ExFreePool(ModuleLoadBase);

  return  ModuleObject;
}

NTSTATUS
LdrProcessDriver(PVOID ModuleLoadBase, PCHAR FileName)
{
   PMODULE_OBJECT ModuleObject;
   UNICODE_STRING ModuleName;

   RtlCreateUnicodeStringFromAsciiz(&ModuleName,
				    FileName);
   ModuleObject = LdrProcessModule(ModuleLoadBase,
				   &ModuleName);
   RtlFreeUnicodeString(&ModuleName);
   if (ModuleObject == NULL)
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
  UNICODE_STRING RemainingPath;

  wcscpy(NameBuffer, MODULE_ROOT_NAME);
  if (wcsrchr(Filename->Buffer, '\\') != 0)
    {
      wcscat(NameBuffer, wcsrchr(Filename->Buffer, '\\') + 1);
    }
  else
    {
      wcscat(NameBuffer, Filename->Buffer);
    }
  RtlInitUnicodeString (&ModuleName, NameBuffer);
  InitializeObjectAttributes(&ObjectAttributes,
                             &ModuleName, 
                             0,
                             NULL,
                             NULL);

  Status = ObFindObject(&ObjectAttributes,
                        (PVOID *) &ModuleObject,
                        &RemainingPath,
                        NULL);
  CHECKPOINT;
  if (NT_SUCCESS(Status) && (RemainingPath.Buffer == NULL || *(RemainingPath.Buffer) == 0))
    {
      DPRINT("Module %wZ at %p\n", Filename, ModuleObject);
      RtlFreeUnicodeString (&RemainingPath);

      return  ModuleObject;
    }

  RtlFreeUnicodeString (&RemainingPath);

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

NTSTATUS
LdrpQueryModuleInformation(PVOID Buffer,
			   ULONG Size,
			   PULONG ReqSize)
{
   PLIST_ENTRY current_entry;
   PMODULE_OBJECT current;
   ULONG ModuleCount = 0;
   PSYSTEM_MODULE_INFORMATION Smi;
   ANSI_STRING AnsiName;
   PCHAR p;

//   KeAcquireSpinLock(&ModuleListLock,&oldlvl);

   /* calculate required size */
   current_entry = ModuleListHead.Flink;
   while (current_entry != (&ModuleListHead))
     {
	ModuleCount++;
	current_entry = current_entry->Flink;
     }

   *ReqSize = sizeof(SYSTEM_MODULE_INFORMATION)+
	(ModuleCount - 1) * sizeof(SYSTEM_MODULE_ENTRY);

   if (Size < *ReqSize)
     {
//	KeReleaseSpinLock(&ModuleListLock,oldlvl);
	return STATUS_INFO_LENGTH_MISMATCH;
     }

   /* fill the buffer */
   memset(Buffer, '=', Size);

   Smi = (PSYSTEM_MODULE_INFORMATION)Buffer;
   Smi->Count = ModuleCount;

   ModuleCount = 0;
   current_entry = ModuleListHead.Flink;
   while (current_entry != (&ModuleListHead))
     {
	current = CONTAINING_RECORD(current_entry,MODULE_OBJECT,ListEntry);

	Smi->Module[ModuleCount].Unknown2 = 0;		/* Always 0 */
	Smi->Module[ModuleCount].BaseAddress = current->Base;
	Smi->Module[ModuleCount].Size = current->Length;
	Smi->Module[ModuleCount].Unknown3 = 0;		/* Flags ??? */
	Smi->Module[ModuleCount].EntryIndex = ModuleCount;

	AnsiName.Length = 0;
	AnsiName.MaximumLength = 256;
	AnsiName.Buffer = Smi->Module[ModuleCount].Name;
	RtlUnicodeStringToAnsiString(&AnsiName,
				     &current->FullName,
				     FALSE);

	p = strrchr (AnsiName.Buffer, '\\');
	if (p == NULL)
	  {
	     Smi->Module[ModuleCount].PathLength = 0;
	     Smi->Module[ModuleCount].NameLength = strlen(AnsiName.Buffer);
	  }
	else
	  {
	     p++;
	     Smi->Module[ModuleCount].PathLength = p - AnsiName.Buffer;
	     Smi->Module[ModuleCount].NameLength = strlen(p);
	  }

	ModuleCount++;
	current_entry = current_entry->Flink;
     }

//   KeReleaseSpinLock(&ModuleListLock,oldlvl);

   return STATUS_SUCCESS;
}


static VOID
LdrpBuildModuleBaseName(PUNICODE_STRING BaseName,
			PUNICODE_STRING FullName)
{
   UNICODE_STRING Name;
   PWCHAR p;
   PWCHAR q;

   DPRINT("LdrpBuildModuleBaseName()\n");
   DPRINT("FullName %wZ\n", FullName);

   p = wcsrchr(FullName->Buffer, '\\');
   if (p == NULL)
     {
	p = FullName->Buffer;
     }
   else
     {
	p++;
     }

   DPRINT("p %S\n", p);

   RtlCreateUnicodeString(&Name, p);

   q = wcschr(p, '.');
   if (q != NULL)
     {
	*q = (WCHAR)0;
     }

   DPRINT("p %S\n", p);

   RtlCreateUnicodeString(BaseName, p);
   RtlFreeUnicodeString(&Name);
}


/*  ----------------------------------------------  PE Module support */

PMODULE_OBJECT
LdrPEProcessModule(PVOID ModuleLoadBase, PUNICODE_STRING FileName)
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
  MODULE_TEXT_SECTION* ModuleTextSection;

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
   DbgPrint("DriverBase: %x\n", DriverBase);
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
#if 1
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
#else
   RelocDir = RelocDir + (ULONG)DriverBase;
   CurrentSize = PEOptionalHeader->DataDirectory
		  [IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
#endif   
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
              DbgPrint("Unknown relocation type %x at %x\n",Type, &Type);
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

      SymbolNameBuf = ExAllocatePoolWithTag(NonPagedPool, 512, TAG_SYM_BUF);

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
          RtlInitUnicodeString (&ModuleName, NameBuffer);
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
		   return(NULL);
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
  if (wcsrchr(FileName->Buffer, '\\') != 0)
    {
      wcscat(NameBuffer, wcsrchr(FileName->Buffer, '\\') + 1);
    }
  else
    {
      wcscat(NameBuffer, FileName->Buffer);
    }
  RtlInitUnicodeString (&ModuleName, NameBuffer);
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
                                IoDriverObjectType);

   /*  Initialize ModuleObject data  */
   ModuleObject->Base = DriverBase;
   ModuleObject->Flags = MODULE_FLAG_PE;
   InsertTailList(&ModuleListHead,
		  &ModuleObject->ListEntry);
   RtlCreateUnicodeString(&ModuleObject->FullName,
			  FileName->Buffer);
   LdrpBuildModuleBaseName(&ModuleObject->BaseName,
			   &ModuleObject->FullName);

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

  ModuleTextSection = ExAllocatePool(NonPagedPool, 
				     sizeof(MODULE_TEXT_SECTION));
  ModuleTextSection->Base = (ULONG)DriverBase;
  ModuleTextSection->Length = DriverSize;
  ModuleTextSection->Name = 
    ExAllocatePool(NonPagedPool, 
		   (wcslen(NameBuffer) + 1) * sizeof(WCHAR));
  wcscpy(ModuleTextSection->Name, NameBuffer);
  InsertTailList(&ModuleTextListHead, &ModuleTextSection->ListEntry);

  return  ModuleObject;
}

static PVOID
LdrPEGetExportAddress(PMODULE_OBJECT ModuleObject,
                      char *Name,
                      unsigned short Hint)
{
  WORD  Idx;
  PVOID  ExportAddress;
  PWORD  OrdinalList;
  PDWORD  FunctionList, NameList;
   PIMAGE_EXPORT_DIRECTORY  ExportDir;
   ULONG ExportDirSize;

   ExportDir = (PIMAGE_EXPORT_DIRECTORY)
     RtlImageDirectoryEntryToData(ModuleObject->Base,
				  TRUE,
				  IMAGE_DIRECTORY_ENTRY_EXPORT,
				  &ExportDirSize);
   DPRINT("ExportDir %p ExportDirSize %lx\n", ExportDir, ExportDirSize);
   if (ExportDir == NULL)
     {
	return NULL;
     }


   FunctionList = (PDWORD)((DWORD)ExportDir->AddressOfFunctions + ModuleObject->Base);
   NameList = (PDWORD)((DWORD)ExportDir->AddressOfNames + ModuleObject->Base);
   OrdinalList = (PWORD)((DWORD)ExportDir->AddressOfNameOrdinals + ModuleObject->Base);

  ExportAddress = 0;

  if (Name != NULL)
    {
      for (Idx = 0; Idx < ExportDir->NumberOfNames; Idx++)
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
		  if (((ULONG)ExportAddress >= (ULONG)ExportDir) &&
		      ((ULONG)ExportAddress < (ULONG)ExportDir + ExportDirSize))
		    {
		       DPRINT("Forward: %s\n", (PCHAR)ExportAddress);
		       ExportAddress = LdrPEFixupForward((PCHAR)ExportAddress);
		       DPRINT("ExportAddress: %p\n", ExportAddress);
		    }

              break;
            }
        }
    }
  else  /*  use hint  */
    {
      ExportAddress = (PVOID) ((DWORD)ModuleObject->Base +
        FunctionList[Hint - ExportDir->Base]);
    }

   if (ExportAddress == 0)
     {
	DbgPrint("Export not found for %d:%s\n",
		 Hint,
		 Name != NULL ? Name : "(Ordinal)");
	KeBugCheck(0);
     }

   return ExportAddress;
}


static PMODULE_OBJECT
LdrPEGetModuleObject(PUNICODE_STRING ModuleName)
{
   PLIST_ENTRY Entry;
   PMODULE_OBJECT Module;

   DPRINT("LdrPEGetModuleObject (ModuleName %wZ)\n",
          ModuleName);

   Entry = ModuleListHead.Flink;

   while (Entry != &ModuleListHead)
     {
	Module = CONTAINING_RECORD(Entry, MODULE_OBJECT, ListEntry);

	DPRINT("Comparing %wZ and %wZ\n",
	       &Module->BaseName,
	       ModuleName);

	if (!RtlCompareUnicodeString(&Module->BaseName, ModuleName, TRUE))
	  {
	     DPRINT("Module %x\n", Module);
	     return Module;
	  }

	Entry = Entry->Flink;
     }

   DbgPrint("LdrPEGetModuleObject: Failed to find dll %wZ\n", ModuleName);

   return NULL;
}


static PVOID
LdrPEFixupForward(PCHAR ForwardName)
{
   CHAR NameBuffer[128];
   UNICODE_STRING ModuleName;
   PCHAR p;
   PMODULE_OBJECT ModuleObject;

   DPRINT("LdrPEFixupForward (%s)\n", ForwardName);

   strcpy(NameBuffer, ForwardName);
   p = strchr(NameBuffer, '.');
   if (p == NULL)
     {
	return NULL;
     }

   *p = 0;

   DPRINT("Driver: %s  Function: %s\n", NameBuffer, p+1);

   RtlCreateUnicodeStringFromAsciiz(&ModuleName,
				    NameBuffer);
   ModuleObject = LdrPEGetModuleObject(&ModuleName);
   RtlFreeUnicodeString(&ModuleName);

   DPRINT("ModuleObject: %p\n", ModuleObject);

   if (ModuleObject == NULL)
     {
	DbgPrint("LdrPEFixupForward: failed to find module %s\n", NameBuffer);
	return NULL;
     }

   return LdrPEGetExportAddress(ModuleObject, p+1, 0);
}


/* EOF */
