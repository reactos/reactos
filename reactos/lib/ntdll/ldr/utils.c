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
#include <ntdll/ldr.h>

#define NDEBUG
#include <ntdll/ntdll.h>

/* FUNCTIONS *****************************************************************/

typedef WINBOOL STDCALL (*PDLLMAIN_FUNC)(HANDLE hInst, 
					 ULONG ul_reason_for_call,
					 LPVOID lpReserved);


static NTSTATUS LdrLoadDll(PDLL* Dll, PCHAR Name)
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
   ULONG ImageSize, InitialViewSize;
   PVOID ImageBase;
   HANDLE FileHandle, SectionHandle;
   PDLLMAIN_FUNC Entrypoint;
   
   dprintf("LdrLoadDll(Base %x, Name %s)\n",Dll,Name);
   
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
   ImageBase = (PVOID)NTHeaders->OptionalHeader.ImageBase;
   ImageSize = NTHeaders->OptionalHeader.SizeOfImage;

   DPRINT("ImageBase %x\n",ImageBase);
   DllStartupAddr = (PEPFUNC)(ImageBase + 
			      NTHeaders->OptionalHeader.AddressOfEntryPoint);
   
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
   
   (*Dll) = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(DLL));
   (*Dll)->Headers = NTHeaders;
   (*Dll)->BaseAddress = (PVOID)ImageBase;
   (*Dll)->Next = LdrDllListHead.Next;
   (*Dll)->Prev = &LdrDllListHead;
   LdrDllListHead.Next->Prev = (*Dll);
   LdrDllListHead.Next = (*Dll);
   
   Entrypoint = (PDLLMAIN_FUNC)LdrPEStartup(ImageBase, SectionHandle);
   if (Entrypoint != NULL)
     {
	dprintf("Calling entry point at %x\n",Entrypoint);
	Entrypoint(ImageBase, DLL_PROCESS_ATTACH, NULL);
	dprintf("Successful called entrypoint\n");
     }
   
   return(STATUS_SUCCESS);
}

static NTSTATUS LdrFindDll(PDLL* Dll, PCHAR Name)
{
   PIMAGE_EXPORT_DIRECTORY ExportDir;
   DLL* current;
   PIMAGE_OPTIONAL_HEADER OptionalHeader;
   
   DPRINT("LdrFindDll(Name %s)\n",Name);
   
   current = &LdrDllListHead;
   do
     {
	OptionalHeader = &current->Headers->OptionalHeader;
	ExportDir = (PIMAGE_EXPORT_DIRECTORY)OptionalHeader->DataDirectory[
                     IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	ExportDir = (PIMAGE_EXPORT_DIRECTORY)
	  ((ULONG)ExportDir + (ULONG)current->BaseAddress);
	
	DPRINT("Scanning  %x %x %x\n", ExportDir->Name,
	       current->BaseAddress, ExportDir->Name + current->BaseAddress);
	DPRINT("Scanning %s\n", ExportDir->Name + current->BaseAddress);
	if (strcmp(ExportDir->Name + current->BaseAddress, Name) == 0)
	  {
	     *Dll = current;
	     return(STATUS_SUCCESS);
	  }
	
	current = current->Next;
     } while (current != &LdrDllListHead);
   
   DPRINT("Failed to find dll %s\n",Name);
   
   return(LdrLoadDll(Dll, Name));
}

NTSTATUS LdrMapSections(HANDLE ProcessHandle,
			PVOID ImageBase,
			HANDLE SectionHandle,
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
	Base = (ULONG)(Sections[i].VirtualAddress + ImageBase);
	SET_LARGE_INTEGER_HIGH_PART(Offset,0);
	SET_LARGE_INTEGER_LOW_PART(Offset,Sections[i].PointerToRawData);
	Status = ZwMapViewOfSection(SectionHandle,
				    ProcessHandle,
				    (PVOID *)&Base,
				    0,
				    Sections[i].Misc.VirtualSize,
				    &Offset,
				    (PULONG)&Sections[i].Misc.VirtualSize,
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

static PVOID LdrGetExportByOrdinal(PDLL Module, ULONG Ordinal)
{
   PIMAGE_EXPORT_DIRECTORY ExportDir;
   PDWORD* ExFunctions;
   USHORT* ExOrdinals;

   ExportDir = (Module->BaseAddress + 
		(Module->Headers->OptionalHeader.
		 DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress));
   
   ExOrdinals = (USHORT*)RVA(Module->BaseAddress, 
			     ExportDir->AddressOfNameOrdinals);
   ExFunctions = (PDWORD*)RVA(Module->BaseAddress, 
			      ExportDir->AddressOfFunctions);
   dprintf("LdrGetExportByOrdinal(Ordinal %d) = %x\n",
	   Ordinal,ExFunctions[ExOrdinals[Ordinal - ExportDir->Base]]);
   return(ExFunctions[ExOrdinals[Ordinal - ExportDir->Base]]);
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
	DPRINT("Comparing '%s' '%s'\n",ExName,SymbolName);
	if (strcmp(ExName,SymbolName) == 0)
	  {
	     Ordinal = ExOrdinals[i];
	     return(RVA(Module->BaseAddress, ExFunctions[Ordinal]));
	  }
     }
   dprintf("LdrGetExportByName() = failed to find %s\n",SymbolName);
   return(NULL);
}

static NTSTATUS LdrPerformRelocations(PIMAGE_NT_HEADERS NTHeaders,
				      PVOID ImageBase)
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
				PVOID ImageBase)
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
	     return(Status);
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
		  		  
		  *ImportAddressList = LdrGetExportByName(Module,
							  (PUCHAR)pName);
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

PEPFUNC LdrPEStartup(PVOID ImageBase, HANDLE SectionHandle)
{
   NTSTATUS Status;
   PEPFUNC EntryPoint;
   PIMAGE_DOS_HEADER DosHeader;
   PIMAGE_NT_HEADERS NTHeaders;
   
   DosHeader = (PIMAGE_DOS_HEADER) ImageBase;
   NTHeaders = (PIMAGE_NT_HEADERS)(ImageBase + DosHeader->e_lfanew);
   
   /*  Initialize Image sections  */
   LdrMapSections(NtCurrentProcess(), ImageBase, SectionHandle, NTHeaders);
   
   if (ImageBase != (PVOID)NTHeaders->OptionalHeader.ImageBase)
     {
	Status = LdrPerformRelocations(NTHeaders, ImageBase);
	if (!NT_SUCCESS(Status))
	  {
	     dprintf("LdrPerformRelocations() failed\n");
	     return(NULL);
	  }
     }
   
   if (NTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].
       VirtualAddress != 0)
     {
	Status = LdrFixupImports(NTHeaders, ImageBase);
	if (!NT_SUCCESS(Status))
	  {
	     dprintf("LdrFixupImports() failed\n");
	     return(NULL);
	  }
     }
   
   EntryPoint = (PEPFUNC)(ImageBase + 
			  NTHeaders->OptionalHeader.AddressOfEntryPoint);
   dprintf("LdrPEStartup() = %x\n",EntryPoint);
   return(EntryPoint);
}



