/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/ldr/startup.c
 * PURPOSE:         Process startup for PE executables
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 */

/* INCLUDES *****************************************************************/

#define WIN32_NO_PEHDR
#include <windows.h>
#include <ddk/ntddk.h>
#include <pe.h>
#include <string.h>
#include <internal/string.h>
#include <wchar.h>

//#define NDEBUG
#include <ntdll/ntdll.h>

PVOID WINAPI __RtlInitHeap(LPVOID base, ULONG minsize, ULONG maxsize);

/* MACROS ********************************************************************/

#define RVA(m, b) ((ULONG)b + m)

/* TYPEDEFS ******************************************************************/

typedef NTSTATUS (*PEPFUNC)(VOID);

typedef struct _DLL
{
   PIMAGE_NT_HEADERS Headers;
   PVOID BaseAddress;
   struct _DLL* Prev;
   struct _DLL* Next;
} DLL, *PDLL;

/* GLOBALS *******************************************************************/

static DLL DllListHead;

#define HEAP_BASE (0xa0000000)

/* FORWARD DECLARATIONS ******************************************************/

static PEPFUNC LdrPEStartup(DWORD ImageBase, HANDLE SectionHandle);

/* FUNCTIONS *****************************************************************/

static NTSTATUS LdrMapSections(PVOID ImageBase, HANDLE SectionHandle,
			       PIMAGE_NT_HEADERS NTHeaders)
{
   ULONG i;
   NTSTATUS Status;
   
   for (i=0; i<NTHeaders->FileHeader.NumberOfSections; i++)
     {
	PIMAGE_SECTION_HEADER Sections;
	LARGE_INTEGER Offset;
	ULONG Base;
	
	Sections = (PIMAGE_SECTION_HEADER)SECHDROFFSET(ImageBase);
	Base = Sections[i].VirtualAddress + ImageBase;
	SET_LARGE_INTEGER_HIGH_PART(Offset,0);
	SET_LARGE_INTEGER_LOW_PART(Offset,Sections[i].PointerToRawData);
	Status = ZwMapViewOfSection(SectionHandle,
				    NtCurrentProcess(),
				    (PVOID *)&Base,
				    0,
				    Sections[i].Misc.VirtualSize,
				    &Offset,
				    &Sections[i].Misc.VirtualSize,
				    0,
				    MEM_COMMIT,
				    PAGE_READWRITE);
	if (!NT_SUCCESS(Status))
	  {
	     return Status;
	  }
     }
   return(STATUS_SUCCESS);
}

static NTSTATUS LdrLoadDll(PDLL* Base, PCHAR Name)
{
   char fqname[255] = "\\??\\C:\\reactos\\system\\";
   ANSI_STRING AnsiString;
   UNICODE_STRING UnicodeString;
   OBJECT_ATTRIBUTES FileObjectAttributes;
   char BlockBuffer[1024];
   PIMAGE_DOS_HEADER DosHeader;
   NTSTATUS Status;
   PIMAGE_NT_HEADERS NTHeaders;
   PEPFUNC DllStartupAddr;
   ULONG ImageBase, ImageSize, InitialViewSize;
   HANDLE FileHandle, SectionHandle;
   PDLL DllDesc;
   
   DPRINT("LdrLoadDll(Base %x, Name %s)\n",Base,Name);
   
   strcat(fqname, Name);
   
   DPRINT("fqname %s\n",fqname);
   
   RtlInitAnsiString(&AnsiString,fqname);
   RtlAnsiStringToUnicodeString(&UnicodeString,&AnsiString,TRUE);
   
   InitializeObjectAttributes(&FileObjectAttributes,
			      &UnicodeString,
			      0,
			      NULL,
			      NULL);
   DPRINT("Opening dll\n");
   Status = ZwOpenFile(&FileHandle, FILE_ALL_ACCESS, &FileObjectAttributes, 
		       NULL, 0, 0);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("Dll open failed ");
	return Status;
     }
   Status = ZwReadFile(FileHandle, 0, 0, 0, 0, BlockBuffer, 1024, 0, 0);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("Dll header read failed ");
	ZwClose(FileHandle);
	return Status;
    }    
   
   DosHeader = (PIMAGE_DOS_HEADER) BlockBuffer;
   if (DosHeader->e_magic != IMAGE_DOS_MAGIC || 
       DosHeader->e_lfanew == 0L ||
       *(PULONG)((PUCHAR)BlockBuffer + DosHeader->e_lfanew) != IMAGE_PE_MAGIC)
     {
	DPRINT("NTDLL format invalid\n");
	ZwClose(FileHandle);
	
	return STATUS_UNSUCCESSFUL;
     }
   NTHeaders = (PIMAGE_NT_HEADERS)(BlockBuffer + DosHeader->e_lfanew);
   ImageBase = NTHeaders->OptionalHeader.ImageBase;
   ImageSize = NTHeaders->OptionalHeader.SizeOfImage;

   DPRINT("ImageBase %x\n",ImageBase);
   DllStartupAddr = ImageBase + NTHeaders->OptionalHeader.AddressOfEntryPoint;
   
   /* Create a section for NTDLL */
   Status = ZwCreateSection(&SectionHandle,
			    SECTION_ALL_ACCESS,
			    NULL,
			    NULL,
			    PAGE_READWRITE,
			    MEM_COMMIT,
			    FileHandle);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("NTDLL create section failed ");
	ZwClose(FileHandle);       
	return Status;
     }
   
   /*  Map the NTDLL into the process  */
   InitialViewSize = DosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS) 
     + sizeof(IMAGE_SECTION_HEADER) * NTHeaders->FileHeader.NumberOfSections;
   Status = ZwMapViewOfSection(SectionHandle,
			       NtCurrentProcess(),
			       (PVOID *)&ImageBase,
			       0,
			       InitialViewSize,
			       NULL,
			       &InitialViewSize,
			       0,
			       MEM_COMMIT,
                              PAGE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("NTDLL map view of secion failed ");
	ZwClose(FileHandle);
	
	return Status;
     }
   ZwClose(FileHandle);
   
   DllDesc = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(DLL));
   DllDesc->Headers = NTHeaders;
   DllDesc->BaseAddress = ImageBase;
   DllDesc->Next = DllListHead.Next;
   DllDesc->Prev = &DllListHead;
   DllListHead.Next->Prev = DllDesc;
   DllListHead.Next = DllDesc;
   
   LdrPEStartup(ImageBase, SectionHandle);
   
   *Base = DllDesc;
   
   return(STATUS_SUCCESS);
}

static NTSTATUS LdrFindDll(PDLL* Base, PCHAR Name)
{
   PIMAGE_EXPORT_DIRECTORY ExportDir;
   DLL* current;
   PIMAGE_OPTIONAL_HEADER OptionalHeader;
   
   DPRINT("LdrFindDll(Name %s)\n",Name);
   
   current = &DllListHead;
   do
     {
	OptionalHeader = &current->Headers->OptionalHeader;
	ExportDir = (PIMAGE_EXPORT_DIRECTORY)OptionalHeader->DataDirectory[
                     IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	ExportDir = ((ULONG)ExportDir + (ULONG)current->BaseAddress);
	
	DPRINT("Scanning %s\n",ExportDir->Name + current->BaseAddress);
	if (strcmp(ExportDir->Name + current->BaseAddress, Name) == 0)
	  {
	     *Base = current;
	     return(STATUS_SUCCESS);
	  }
	
	current = current->Next;
     } while (current != &DllListHead);
   
   DPRINT("Failed to find dll %s\n",Name);
   
   return(LdrLoadDll(Base, Name));
}

/*   LdrStartup
 * FUNCTION:
 *   Handles Process Startup Activities.
 * ARGUMENTS:
 *   DWORD    ImageBase  The base address of the process image
 */
VOID LdrStartup(HANDLE SectionHandle, DWORD ImageBase)
{
   PEPFUNC EntryPoint;
   PIMAGE_DOS_HEADER PEDosHeader;
   NTSTATUS Status;
   PIMAGE_NT_HEADERS NTHeaders;
   
   DPRINT("LdrStartup(ImageBase %x, SectionHandle %x)\n",ImageBase,
	   SectionHandle);
   
   DllListHead.BaseAddress = 0x80000000;
   DllListHead.Prev = &DllListHead;
   DllListHead.Next = &DllListHead;
   PEDosHeader = (PIMAGE_DOS_HEADER)DllListHead.BaseAddress;
   DllListHead.Headers = (PIMAGE_NT_HEADERS)(DllListHead.BaseAddress +
					     PEDosHeader->e_lfanew);
   
  /*  If MZ header exists  */
  PEDosHeader = (PIMAGE_DOS_HEADER) ImageBase;
  if (PEDosHeader->e_magic != IMAGE_DOS_MAGIC ||
      PEDosHeader->e_lfanew == 0L ||
      *(PULONG)((PUCHAR)ImageBase + PEDosHeader->e_lfanew) != IMAGE_PE_MAGIC)
     {
	DPRINT("Image has bad header\n");
	ZwTerminateProcess(NULL,STATUS_UNSUCCESSFUL);
     }

   NTHeaders = (PIMAGE_NT_HEADERS)(ImageBase + PEDosHeader->e_lfanew);
   __RtlInitHeap(HEAP_BASE,
		 NTHeaders->OptionalHeader.SizeOfHeapCommit, 
		 NTHeaders->OptionalHeader.SizeOfHeapReserve);
   EntryPoint = LdrPEStartup(ImageBase, SectionHandle);

   if (EntryPoint == NULL)
     {
	DPRINT("Failed to initialize image\n");
	ZwTerminateProcess(NULL,STATUS_UNSUCCESSFUL);
     }
   
   DPRINT("Transferring control to image at %x\n",EntryPoint);
   Status = EntryPoint();
   ZwTerminateProcess(NtCurrentProcess(),Status);
}

static PVOID LdrGetExportByOrdinal(PDLL Module, ULONG Ordinal)
{
   PIMAGE_EXPORT_DIRECTORY ExportDir;
   USHORT* ExOrdinals;

   ExportDir = (Module->BaseAddress + 
		(Module->Headers->OptionalHeader.
		 DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress));
   
   ExOrdinals = (USHORT*)RVA(Module->BaseAddress, 
			     ExportDir->AddressOfNameOrdinals);
   return(ExOrdinals[Ordinal - ExportDir->Base]);
}

static PVOID LdrGetExportByName(PDLL Module, PUCHAR SymbolName)
{
   PIMAGE_EXPORT_DIRECTORY ExportDir;
   PDWORD* ExFunctions;
   PDWORD* ExNames;
   USHORT* ExOrdinals;
   ULONG i;
   PVOID ExName;
   ULONG Ordinal;
   
   DPRINT("LdrFindExport(Module %x, SymbolName %s)\n",
	  Module, SymbolName);
   
   ExportDir = (Module->BaseAddress + 
		(Module->Headers->OptionalHeader.
		 DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress));
   
   /*  Get header pointers  */
   ExNames = (PDWORD*)RVA(Module->BaseAddress, ExportDir->AddressOfNames);
   ExOrdinals = (USHORT*)RVA(Module->BaseAddress, 
			     ExportDir->AddressOfNameOrdinals);
   ExFunctions = (PDWORD*)RVA(Module->BaseAddress, 
			      ExportDir->AddressOfFunctions);
   for (i=0; i<ExportDir->NumberOfFunctions; i++)
     {
	ExName = RVA(Module->BaseAddress, ExNames[i]);
	if (strcmp(ExName,SymbolName) == 0)
	  {
	     Ordinal = ExOrdinals[i];
	     return(RVA(Module->BaseAddress, ExFunctions[Ordinal]));
	  }
     }
   return(NULL);
}

static NTSTATUS LdrPerformRelocations(PIMAGE_NT_HEADERS NTHeaders,
				      DWORD ImageBase)
{
   USHORT NumberOfEntries;
   PUSHORT pValue16;
   ULONG RelocationRVA;
   ULONG Delta32, Offset;
   PULONG pValue32;
   PRELOCATION_DIRECTORY RelocationDir;
   PRELOCATION_ENTRY RelocationBlock;
   int i;
   
   RelocationRVA = NTHeaders->OptionalHeader.DataDirectory[
                      IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
   if (RelocationRVA)
     {
	RelocationDir = (PRELOCATION_DIRECTORY)((PCHAR)ImageBase + 
						RelocationRVA);
	while (RelocationDir->SizeOfBlock)
	  {
	     Delta32 = (unsigned long)(ImageBase - 
				       NTHeaders->OptionalHeader.ImageBase);
	     RelocationBlock = (PRELOCATION_ENTRY) 
	       (RelocationRVA + ImageBase + sizeof(RELOCATION_DIRECTORY));
	     NumberOfEntries = 
	       (RelocationDir->SizeOfBlock - sizeof(RELOCATION_DIRECTORY)) / 
	       sizeof(RELOCATION_ENTRY);
	     for (i = 0; i < NumberOfEntries; i++)
	       {
                  Offset = (RelocationBlock[i].TypeOffset & 0xfff) + 
                    RelocationDir->VirtualAddress;
                  switch (RelocationBlock[i].TypeOffset >> 12)
                    {
                      case TYPE_RELOC_ABSOLUTE:
                        break;
		       
		     case TYPE_RELOC_HIGH:
		       pValue16 = (PUSHORT) (ImageBase + Offset);
		       *pValue16 += Delta32 >> 16;
		       break;
		       
		     case TYPE_RELOC_LOW:
		       pValue16 = (PUSHORT)(ImageBase + Offset);
		       *pValue16 += Delta32 & 0xffff;
		       break;
		       
		     case TYPE_RELOC_HIGHLOW:
		       pValue32 = (PULONG) (ImageBase + Offset);
		       *pValue32 += Delta32;
		       break;
		       
                      case TYPE_RELOC_HIGHADJ:
		       /* FIXME: do the highadjust fixup  */
                        DPRINT("TYPE_RELOC_HIGHADJ fixup not implemented"
			       ", sorry\n");
		       return(STATUS_UNSUCCESSFUL);
		       
                      default:
		       DPRINT("unexpected fixup type\n");
		       return(STATUS_UNSUCCESSFUL);
                    }
	       }
	     RelocationRVA += RelocationDir->SizeOfBlock;
	     RelocationDir = (PRELOCATION_DIRECTORY)(ImageBase + 
						     RelocationRVA);
	  }
     }
   return(STATUS_SUCCESS);
}

static NTSTATUS LdrFixupImports(PIMAGE_NT_HEADERS NTHeaders,
				DWORD ImageBase)
{
   PIMAGE_IMPORT_MODULE_DIRECTORY ImportModuleDirectory;
   ULONG Ordinal;
   PDLL Module;
   NTSTATUS Status;
   
   /*  Process each import module  */
   ImportModuleDirectory = (PIMAGE_IMPORT_MODULE_DIRECTORY)
     (ImageBase + NTHeaders->OptionalHeader.
      DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
   while (ImportModuleDirectory->dwRVAModuleName)
     {
	PVOID *ImportAddressList; 
	PULONG FunctionNameList;
	DWORD pName;
	PWORD pHint;
	
	Status = LdrFindDll(&Module,
			    (PCHAR)(ImageBase +
				    ImportModuleDirectory->dwRVAModuleName));
	if (!NT_SUCCESS(Status))
	  {
	     return 0;
	  }    
	
	/*  Get the import address list  */
	ImportAddressList = (PVOID *)
	  (NTHeaders->OptionalHeader.ImageBase + 
	   ImportModuleDirectory->dwRVAFunctionAddressList);
	
	/*  Get the list of functions to import  */
	if (ImportModuleDirectory->dwRVAFunctionNameList != 0)
	  {
	     FunctionNameList = (PULONG) (ImageBase + 
					  ImportModuleDirectory->dwRVAFunctionNameList);
	  }
	else
	  {
	     FunctionNameList = (PULONG) (ImageBase + 
					  ImportModuleDirectory->dwRVAFunctionAddressList);
	  }
	
	/*  Walk through function list and fixup addresses  */
	while(*FunctionNameList != 0L)
	  {
	     if ((*FunctionNameList) & 0x80000000)
	       {
		  Ordinal = (*FunctionNameList) & 0x7fffffff;
		  *ImportAddressList = LdrGetExportByOrdinal(Module, Ordinal);
	       }
	     else 
	       {
		  pName = (DWORD)(ImageBase + *FunctionNameList + 2);
		  pHint = (PWORD)(ImageBase + *FunctionNameList);
		  		  
		  *ImportAddressList = LdrGetExportByName(Module,pName);
		  if ((*ImportAddressList) == NULL)
		    {
		       return(STATUS_UNSUCCESSFUL);
		    }
	       }
	     
	     ImportAddressList++;
	     FunctionNameList++;
	  }
	ImportModuleDirectory++;
     }
   return(STATUS_SUCCESS);
}

static PEPFUNC LdrPEStartup(DWORD ImageBase, HANDLE SectionHandle)
{
   NTSTATUS Status;
   PEPFUNC EntryPoint;
   PIMAGE_DOS_HEADER DosHeader;
   PIMAGE_NT_HEADERS NTHeaders;
   
   DosHeader = (PIMAGE_DOS_HEADER) ImageBase;
   NTHeaders = (PIMAGE_NT_HEADERS)(ImageBase + DosHeader->e_lfanew);
   
   /*  Initialize Image sections  */
   LdrMapSections(ImageBase, SectionHandle, NTHeaders);
   
   if (ImageBase != (DWORD) NTHeaders->OptionalHeader.ImageBase)
     {
	Status = LdrPerformRelocations(NTHeaders, ImageBase);
	if (!NT_SUCCESS(Status))
	  {
	     return(NULL);
	  }
     }
   
   if (NTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].
       VirtualAddress != 0)
     {
	Status = LdrFixupImports(NTHeaders, ImageBase);
	if (!NT_SUCCESS(Status))
	  {
	     return(NULL);
	  }
     }
   
   EntryPoint = ImageBase + NTHeaders->OptionalHeader.AddressOfEntryPoint;
   
   return(EntryPoint);
}



