/* $Id: utils.c,v 1.22 1999/12/26 15:50:46 dwelch Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/ldr/startup.c
 * PURPOSE:         Process startup for PE executables
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 */

/* INCLUDES *****************************************************************/

#include <reactos/config.h>
#define WIN32_NO_STATUS
#define WIN32_NO_PEHDR
#include <windows.h>
#include <ddk/ntddk.h>
#include <pe.h>
#include <string.h>
#include <internal/string.h>
#include <wchar.h>
#include <ntdll/ldr.h>

#ifdef DBG_NTDLL_LDR_UTILS
#define NDEBUG
#endif
#include <ntdll/ntdll.h>

/* FUNCTIONS *****************************************************************/


/* Type for a DLL's entry point */
typedef
WINBOOL
STDCALL
(* PDLLMAIN_FUNC) (
	HANDLE	hInst, 
	ULONG	ul_reason_for_call,
	LPVOID	lpReserved
	);

static
NTSTATUS
LdrFindDll (PDLL* Dll,PCHAR	Name);

/**********************************************************************
 * NAME
 *	LdrLoadDll
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * NOTE
 *
 */

NTSTATUS LdrLoadDll (PDLL* Dll,
		     PCHAR	Name)
{
	char			fqname [255] = "\\??\\C:\\reactos\\system32\\";
	ANSI_STRING		AnsiString;
	UNICODE_STRING		UnicodeString;
	OBJECT_ATTRIBUTES	FileObjectAttributes;
	char			BlockBuffer [1024];
	PIMAGE_DOS_HEADER	DosHeader;
	NTSTATUS		Status;
	PIMAGE_NT_HEADERS	NTHeaders;
	PEPFUNC			DllStartupAddr;
	ULONG			ImageSize;
	ULONG			InitialViewSize;
	PVOID			ImageBase;
	HANDLE			FileHandle;
	HANDLE			SectionHandle;
	PDLLMAIN_FUNC		Entrypoint;

	if ( Dll == NULL )
		return -1;

	if ( Name == NULL ) {
		*Dll = &LdrDllListHead;
		return STATUS_SUCCESS;
	}

	DPRINT("LdrLoadDll(Base %x, Name \"%s\")\n", Dll, Name);

	/*
	 * Build the DLL's absolute name
	 */

	if ( strncmp(Name,"\\??\\",3) != 0 ) {
	
		strcat(fqname, Name);
	}
	else
		strncpy(fqname, Name, 256);

	DPRINT("fqname \"%s\"\n", fqname);
	/*
	 * Open the DLL's image file.
	 */

   if (LdrFindDll(Dll, Name) == STATUS_SUCCESS)
     return STATUS_SUCCESS;


	RtlInitAnsiString(
		& AnsiString,
		fqname
		);
	RtlAnsiStringToUnicodeString(
		& UnicodeString,
		& AnsiString,
		TRUE
		);

	InitializeObjectAttributes(
		& FileObjectAttributes,
		& UnicodeString,
		0,
		NULL,
		NULL
		);

	DPRINT("Opening dll \"%s\"\n", fqname);
	
	Status = ZwOpenFile(
			& FileHandle,
			FILE_ALL_ACCESS,
			& FileObjectAttributes, 
			NULL,
			0,
			0
			);
	if (!NT_SUCCESS(Status))
	{
		dprintf("Dll open of %s failed: Status = 0x%08x\n", 
		       fqname, Status);
		return Status;
	}
	Status = ZwReadFile(
			FileHandle,
			0,
			0,
			0,
			0,
			BlockBuffer,
			sizeof BlockBuffer,
			0,
			0
			);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("Dll header read failed: Status = 0x%08x\n", Status);
		ZwClose(FileHandle);
		return Status;
	}
	/*
	 * Overlay DOS and NT headers structures to the 
	 * buffer with DLL's header raw data.
	 */
	DosHeader = (PIMAGE_DOS_HEADER) BlockBuffer;
	NTHeaders = (PIMAGE_NT_HEADERS) (BlockBuffer + DosHeader->e_lfanew);
	/*
	 * Check it is a PE image file.
	 */
	if (	(DosHeader->e_magic != IMAGE_DOS_MAGIC)
		|| (DosHeader->e_lfanew == 0L)
//		|| (*(PULONG)((PUCHAR)BlockBuffer + DosHeader->e_lfanew) != IMAGE_PE_MAGIC)
		|| (*(PULONG)(NTHeaders) != IMAGE_PE_MAGIC)
		)
	{
		DPRINT("NTDLL format invalid\n");
		ZwClose(FileHandle);
	
		return STATUS_UNSUCCESSFUL;
	}

//	NTHeaders = (PIMAGE_NT_HEADERS) (BlockBuffer + DosHeader->e_lfanew);
	ImageBase = (PVOID) NTHeaders->OptionalHeader.ImageBase;
	ImageSize = NTHeaders->OptionalHeader.SizeOfImage;

	DPRINT("ImageBase 0x%08x\n", ImageBase);
	
	DllStartupAddr =
		(PEPFUNC) (
			ImageBase
			+ NTHeaders->OptionalHeader.AddressOfEntryPoint
			);
	/*
	 * Create a section for NTDLL.
	 */
	Status = ZwCreateSection(
			& SectionHandle,
			SECTION_ALL_ACCESS,
			NULL,
			NULL,
			PAGE_READWRITE,
			MEM_COMMIT,
			FileHandle
			);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("NTDLL create section failed: Status = 0x%08x\n", Status);
		ZwClose(FileHandle);
		return Status;
	}
	/*
	 * Map the NTDLL into the process.
	 */
	InitialViewSize =
		DosHeader->e_lfanew
		+ sizeof (IMAGE_NT_HEADERS)
		+ sizeof (IMAGE_SECTION_HEADER) * NTHeaders->FileHeader.NumberOfSections;
	Status = ZwMapViewOfSection(
			SectionHandle,
			NtCurrentProcess(),
			(PVOID*)&ImageBase,
			0,
			InitialViewSize,
			NULL,
			&InitialViewSize,
			0,
			MEM_COMMIT,
			PAGE_READWRITE
			);
	if (!NT_SUCCESS(Status))
	{
		dprintf("NTDLL.LDR: map view of section failed (Status %x)\n",
		       Status);
		ZwClose(FileHandle);
		return(Status);
	}
	ZwClose(FileHandle);

	(*Dll) = RtlAllocateHeap(
			RtlGetProcessHeap(),
			0,
			sizeof (DLL)
			);
	(*Dll)->Headers = NTHeaders;
	(*Dll)->BaseAddress = (PVOID)ImageBase;
	(*Dll)->Next = LdrDllListHead.Next;
	(*Dll)->Prev = & LdrDllListHead;
	(*Dll)->ReferenceCount = 1;
	LdrDllListHead.Next->Prev = (*Dll);
	LdrDllListHead.Next = (*Dll);


   if (((*Dll)->Headers->FileHeader.Characteristics & IMAGE_FILE_DLL) ==
       IMAGE_FILE_DLL)
     {

		Entrypoint =
		(PDLLMAIN_FUNC) LdrPEStartup(
					ImageBase,
					SectionHandle
					);
		if (Entrypoint != NULL)
		{
			DPRINT("Calling entry point at 0x%08x\n", Entrypoint);
			if (FALSE == Entrypoint(
				Dll,
				DLL_PROCESS_ATTACH,
				NULL
				))
			{
				DPRINT("NTDLL.LDR: DLL \"%s\" failed to initialize\n", fqname);
				/* FIXME: should clean up and fail */
			}
			else
			{
				DPRINT("NTDLL.LDR: DLL \"%s\" initialized successfully\n", fqname);
			}
		}
		else
		{
			DPRINT("NTDLL.LDR: Entrypoint is NULL for \"%s\"\n", fqname);
		}
	}
	return STATUS_SUCCESS;
}


/**********************************************************************
 * NAME								LOCAL
 *	LdrFindDll
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * NOTE
 *
 */
static NTSTATUS LdrFindDll(PDLL* Dll, PCHAR Name)
{
   PIMAGE_EXPORT_DIRECTORY	ExportDir;
   DLL			* current;
   PIMAGE_OPTIONAL_HEADER	OptionalHeader;


   DPRINT("NTDLL.LdrFindDll(Name %s)\n", Name);

   current = & LdrDllListHead;

   // NULL is the current process

   if ( Name == NULL )
     {
	*Dll = current;
	return STATUS_SUCCESS;
     }

   do
     {
	OptionalHeader = & current->Headers->OptionalHeader;
	ExportDir = (PIMAGE_EXPORT_DIRECTORY)
	  OptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]
	  .VirtualAddress;
	ExportDir = (PIMAGE_EXPORT_DIRECTORY)
	  ((ULONG)ExportDir + (ULONG)current->BaseAddress);
	
	DPRINT("Scanning  %x %x %x\n",ExportDir->Name,
	       current->BaseAddress,
	       (ExportDir->Name + current->BaseAddress));
	DPRINT("Scanning %s %s\n",
	       ExportDir->Name + current->BaseAddress, Name);
	
	if (_stricmp(ExportDir->Name + current->BaseAddress, Name) == 0)
	  {
	     *Dll = current;
	     current->ReferenceCount++;
	     return STATUS_SUCCESS;
	  }
	
	current = current->Next;
	
     } while (current != & LdrDllListHead);

   DPRINT("Failed to find dll %s\n",Name);

   return -1;
}


/**********************************************************************
 * NAME
 *	LdrMapSections
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * NOTE
 *
 */
NTSTATUS LdrMapSections(HANDLE			ProcessHandle,
			PVOID			ImageBase,
			HANDLE			SectionHandle,
			PIMAGE_NT_HEADERS	NTHeaders)
{
   ULONG		i;
   NTSTATUS	Status;
   
   
   for (i = 0; (i < NTHeaders->FileHeader.NumberOfSections); i++)
     {
	PIMAGE_SECTION_HEADER	Sections;
	LARGE_INTEGER		Offset;	
	ULONG			Base;
	ULONG Size;
	
	Sections = (PIMAGE_SECTION_HEADER) SECHDROFFSET(ImageBase);
	Base = (ULONG) (Sections[i].VirtualAddress + ImageBase);
	Offset.u.LowPart = Sections[i].PointerToRawData;
	Offset.u.HighPart = 0;
	
	Size = max(Sections[i].Misc.VirtualSize, Sections[i].SizeOfRawData);
	
	DPRINT("Mapping section %d offset %x base %x size %x\n",
	        i, Offset.u.LowPart, Base, Sections[i].Misc.VirtualSize);
	DPRINT("Size %x\n", Sections[i].SizeOfRawData);
	
	Status = ZwMapViewOfSection(SectionHandle,
				    ProcessHandle,
				    (PVOID*)&Base,
				    0,
				    Size,
				    &Offset,
				    (PULONG)&Size,
				    0,
				    MEM_COMMIT,
				    PAGE_READWRITE);
	if (!NT_SUCCESS(Status))
	  {
	     DPRINT("Failed to map section");
	     return(Status);
	  }
     }
   return STATUS_SUCCESS;
}


/**********************************************************************
 * NAME								LOCAL
 *	LdrGetExportByOrdinal
 *	
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * NOTE
 *
 */

PVOID
LdrGetExportByOrdinal (
	PDLL	Module,
	ULONG	Ordinal
	)
{
	PIMAGE_EXPORT_DIRECTORY	ExportDir;
	PDWORD			* ExFunctions;
	USHORT			* ExOrdinals;

	ExportDir = (
		Module->BaseAddress
		+ (Module->Headers->OptionalHeader
			.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]
				.VirtualAddress
				)
		);

	ExOrdinals = (USHORT *)
		RVA(
			Module->BaseAddress,
			ExportDir->AddressOfNameOrdinals
			);
	ExFunctions = (PDWORD *)
		RVA(
			Module->BaseAddress,
			ExportDir->AddressOfFunctions
			);
	dprintf(
		"LdrGetExportByOrdinal(Ordinal %d) = %x\n",
		Ordinal,
		ExFunctions[ExOrdinals[Ordinal - ExportDir->Base]]
		);
	return(ExFunctions[ExOrdinals[Ordinal - ExportDir->Base]]);
}


/**********************************************************************
 * NAME								LOCAL
 *	LdrGetExportByName
 *	
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * NOTE
 *
 */

PVOID
LdrGetExportByName (
	PDLL	Module,
	PUCHAR	SymbolName
	)
{
	PIMAGE_EXPORT_DIRECTORY	ExportDir;
	PDWORD			* ExFunctions;
	PDWORD			* ExNames;
	USHORT			* ExOrdinals;
	ULONG			i;
	PVOID			ExName;
	ULONG			Ordinal;

//	DPRINT(
//		"LdrFindExport(Module %x, SymbolName %s)\n",
//		Module,
//		SymbolName
//		);

	ExportDir = (
		Module->BaseAddress
		+ (Module->Headers->OptionalHeader
			.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]
				.VirtualAddress
				)
		);
	/*
	 * Get header pointers
	 */
	ExNames = (PDWORD *)
		RVA(
			Module->BaseAddress,
			ExportDir->AddressOfNames
			);
	ExOrdinals = (USHORT *)
		RVA(
			Module->BaseAddress,
			ExportDir->AddressOfNameOrdinals
			);
	ExFunctions = (PDWORD *)
		RVA(
			Module->BaseAddress,
			ExportDir->AddressOfFunctions
			);
	for (	i = 0;
		( i < ExportDir->NumberOfFunctions);
		i++
		)
	{
		ExName = RVA(
				Module->BaseAddress,
				ExNames[i]
				);
//		DPRINT(
//			"Comparing '%s' '%s'\n",
//			ExName,
//			SymbolName
//			);
		if (strcmp(ExName,SymbolName) == 0)
		{
			Ordinal = ExOrdinals[i];
			return(RVA(Module->BaseAddress, ExFunctions[Ordinal]));
		}
	}

	dprintf("LdrGetExportByName() = failed to find %s\n",SymbolName);

	return NULL;
}


/**********************************************************************
 * NAME								LOCAL
 *	LdrPerformRelocations
 *	
 * DESCRIPTION
 *	Relocate a DLL's memory image.
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * NOTE
 *
 */
static NTSTATUS LdrPerformRelocations (PIMAGE_NT_HEADERS	NTHeaders,
				       PVOID			ImageBase)
{
   USHORT			NumberOfEntries;
   PUSHORT			pValue16;
   ULONG			RelocationRVA;
   ULONG			Delta32;
   ULONG			Offset;
   PULONG			pValue32;
   PRELOCATION_DIRECTORY	RelocationDir;
   PRELOCATION_ENTRY	RelocationBlock;
   int			i;


   RelocationRVA = NTHeaders->OptionalHeader
     .DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC]
     .VirtualAddress;
   
   if (RelocationRVA)
	{
	   RelocationDir = (PRELOCATION_DIRECTORY)
	     ((PCHAR)ImageBase + RelocationRVA);

	   while (RelocationDir->SizeOfBlock)
	     {
		Delta32 = (ULONG)(ImageBase - 
				  NTHeaders->OptionalHeader.ImageBase);
		RelocationBlock = (PRELOCATION_ENTRY) (
						       RelocationRVA
						       + ImageBase
						       + sizeof (RELOCATION_DIRECTORY)
						       );
		NumberOfEntries = (
				   RelocationDir->SizeOfBlock
				   - sizeof (RELOCATION_DIRECTORY)
				   )
		  / sizeof (RELOCATION_ENTRY);
		
		for (	i = 0;
		     (i < NumberOfEntries);
		     i++
		     )
		  {
		     Offset = (
			       RelocationBlock[i].TypeOffset
			       & 0xfff
			       )
		       + RelocationDir->VirtualAddress;
		     /*
		      * What kind of relocations should we perform
		      * for the current entry?
		      */
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
			  DPRINT(
				 "TYPE_RELOC_HIGHADJ fixup not implemented"
				 ", sorry\n"
				 );
			  return(STATUS_UNSUCCESSFUL);
			  
			default:
			  DPRINT("unexpected fixup type\n");
			  return STATUS_UNSUCCESSFUL;
		       }
		  }
		RelocationRVA += RelocationDir->SizeOfBlock;
		RelocationDir = (PRELOCATION_DIRECTORY) (
				ImageBase
							 + RelocationRVA
							 );
	     }
	}
   return STATUS_SUCCESS;
}


/**********************************************************************
 * NAME								LOCAL
 *	LdrFixupImports
 *	
 * DESCRIPTION
 *	Compute the entry point for every symbol the DLL imports
 *	from other modules.
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * NOTE
 *
 */
static NTSTATUS LdrFixupImports(PIMAGE_NT_HEADERS	NTHeaders,
				PVOID			ImageBase)
{
   PIMAGE_IMPORT_MODULE_DIRECTORY ImportModuleDirectory;
   ULONG Ordinal;
   PDLL	Module;
   NTSTATUS Status;
   
   DPRINT("LdrFixupImports(NTHeaders %x, ImageBase %x)\n", NTHeaders, 
	   ImageBase);
   
   /*
    * Process each import module.
    */
   ImportModuleDirectory = (PIMAGE_IMPORT_MODULE_DIRECTORY)(
			       ImageBase + NTHeaders->OptionalHeader
				 .DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
			     .VirtualAddress);
   DPRINT("ImportModuleDirectory %x\n", ImportModuleDirectory);
   
   while (ImportModuleDirectory->dwRVAModuleName)
     {
	PVOID	* ImportAddressList; 
	PULONG	FunctionNameList;
	DWORD	pName;
	PWORD	pHint;
	
	DPRINT("ImportModule->Directory->dwRVAModuleName %s\n",
	       (PCHAR)(ImageBase + ImportModuleDirectory->dwRVAModuleName));
	
	Status = LdrLoadDll(&Module,
			    (PCHAR)(ImageBase
				    +ImportModuleDirectory->dwRVAModuleName));
	if (!NT_SUCCESS(Status))
	  {
	     return Status;
	  }
	/*
	 * Get the import address list.
	 */
	ImportAddressList = (PVOID *)(NTHeaders->OptionalHeader.ImageBase
			+ ImportModuleDirectory->dwRVAFunctionAddressList);
	
	/*
	 * Get the list of functions to import.
	 */
	if (ImportModuleDirectory->dwRVAFunctionNameList != 0)
	  {
	     FunctionNameList = (PULONG) (
					  ImageBase
					  + ImportModuleDirectory->dwRVAFunctionNameList
					  );
	  }
	else
	  {
	     FunctionNameList = (PULONG) (
					  ImageBase
					  + ImportModuleDirectory->dwRVAFunctionAddressList
					  );
	  }
	/*
	 * Walk through function list and fixup addresses.
	 */
	while (*FunctionNameList != 0L)
	  {
	     if ((*FunctionNameList) & 0x80000000)
	       {
		  Ordinal = (*FunctionNameList) & 0x7fffffff;
				*ImportAddressList = 
		    LdrGetExportByOrdinal(
					  Module,
					  Ordinal
					  );
	       }
	     else
	       {
		  pName = (DWORD) (
				   ImageBase
				   + *FunctionNameList
				   + 2
				   );
		  pHint = (PWORD) (
				   ImageBase
				   + *FunctionNameList
						);
		  
		  *ImportAddressList =
		    LdrGetExportByName(
						Module,
				       (PUCHAR) pName
				       );
		  if ((*ImportAddressList) == NULL)
		    {
				   dprintf("Failed to import %s\n", pName);
		       return STATUS_UNSUCCESSFUL;
		    }
	       }
			ImportAddressList++;
	     FunctionNameList++;
	  }
	ImportModuleDirectory++;
     }
   return STATUS_SUCCESS;
}


/**********************************************************************
 * NAME
 *	LdrPEStartup
 *
 * DESCRIPTION
 * 	1. Map the DLL's sections into memory.
 * 	2. Relocate, if needed the DLL.
 * 	3. Fixup any imported symbol.
 * 	4. Compute the DLL's entry point.
 *
 * ARGUMENTS
 *	ImageBase
 *		Address at which the DLL's image
 *		is loaded.
 *		
 *	SectionHandle
 *		Handle of the section that contains
 *		the DLL's image.
 *
 * RETURN VALUE
 *	NULL on error; otherwise the entry point
 *	to call for initializing the DLL.
 *
 * REVISIONS
 *
 * NOTE
 *
 */
PEPFUNC LdrPEStartup (PVOID	ImageBase,
		      HANDLE	SectionHandle)
{
   NTSTATUS		Status;
   PEPFUNC			EntryPoint;
   PIMAGE_DOS_HEADER	DosHeader;
   PIMAGE_NT_HEADERS	NTHeaders;
   

   /*
    * Overlay DOS and WNT headers structures
    * to the DLL's image.
    */
   DosHeader = (PIMAGE_DOS_HEADER) ImageBase;
   NTHeaders = (PIMAGE_NT_HEADERS) (ImageBase + DosHeader->e_lfanew);
   
   /*
    * Initialize image sections.
    */
   LdrMapSections(NtCurrentProcess(),
		  ImageBase,
		  SectionHandle,
		  NTHeaders);
   
   /*
    * If the base address is different from the
    * one the DLL is actually loaded, perform any
    * relocation.
    */
   if (ImageBase != (PVOID) NTHeaders->OptionalHeader.ImageBase)
     {
	Status = LdrPerformRelocations(NTHeaders, ImageBase);
	if (!NT_SUCCESS(Status))
	  {
	     dprintf("LdrPerformRelocations() failed\n");
	     return NULL;
	  }
     }
   
   /*
    * If the DLL's imports symbols from other
    * modules, fixup the imported calls entry points.
    */
   if (NTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
       .VirtualAddress != 0)
     {
	DPRINT("About to fixup imports\n");
	Status = LdrFixupImports(NTHeaders, ImageBase);
	if (!NT_SUCCESS(Status))
	  {
	     dprintf("LdrFixupImports() failed\n");
	     return NULL;
	  }
     }
   
   /*
    * Compute the DLL's entry point's address.
    */
   EntryPoint = (PEPFUNC) (ImageBase
			   + NTHeaders->OptionalHeader.AddressOfEntryPoint);
   DPRINT("LdrPEStartup() = %x\n",EntryPoint);
   return EntryPoint;
}

NTSTATUS LdrUnloadDll(PDLL Dll)
{
   PDLLMAIN_FUNC		Entrypoint;
   NTSTATUS		Status;

   if ( Dll == NULL || Dll == &LdrDllListHead )
     return -1;


   if ( Dll->ReferenceCount > 1 )
     {
	Dll->ReferenceCount--;
	return STATUS_SUCCESS;
     }

   if (( Dll->Headers->FileHeader.Characteristics & IMAGE_FILE_DLL ) == IMAGE_FILE_DLL ) {

      Entrypoint = (PDLLMAIN_FUNC) LdrPEStartup(Dll->BaseAddress,
						Dll->SectionHandle);
      if (Entrypoint != NULL)
	{
	   DPRINT("Calling entry point at 0x%08x\n", Entrypoint);
	   if (FALSE == Entrypoint(Dll,
				   DLL_PROCESS_DETACH,
				   NULL))
	     {
		DPRINT("NTDLL.LDR: DLL failed to detach\n");
		return -1;
	     }
	   else
	     {
		DPRINT("NTDLL.LDR: DLL  detached successfully\n");
	     }
	}
      else
	{
	   DPRINT("NTDLL.LDR: Entrypoint is NULL for \n");
	}
      
   }
   Status = ZwUnmapViewOfSection(NtCurrentProcess(),
				 Dll->BaseAddress);

   ZwClose(Dll->SectionHandle);

   return Status;
}

static IMAGE_RESOURCE_DIRECTORY_ENTRY * LdrGetNextEntry(IMAGE_RESOURCE_DIRECTORY *ResourceDir, LPCWSTR ResourceName, ULONG Offset)
{


	WORD    NumberOfNamedEntries;
	WORD    NumberOfIdEntries;
	WORD    Entries;
	ULONG   Length;

	if ( (((ULONG)ResourceDir) & 0xF0000000) != 0 ) {
		return (IMAGE_RESOURCE_DIRECTORY_ENTRY *)NULL;
	}


	NumberOfIdEntries = ResourceDir->NumberOfIdEntries;
	NumberOfNamedEntries = ResourceDir->NumberOfNamedEntries;
	if ( (	NumberOfIdEntries + NumberOfNamedEntries) == 0) {
		return &ResourceDir->DirectoryEntries[0];
	}

	if ( HIWORD(ResourceName) != 0 ) {
		Length = wcslen(ResourceName);
		Entries = ResourceDir->NumberOfNamedEntries;
		do {
		        IMAGE_RESOURCE_DIR_STRING_U *DirString;

			Entries--;
			DirString =  (IMAGE_RESOURCE_DIR_STRING_U *)(((ULONG)ResourceDir->DirectoryEntries[Entries].Name &  (~0xF0000000)) + Offset);
			
			if ( DirString->Length == Length && wcscmp(DirString->NameString, ResourceName ) == 0 ) {
				return  &ResourceDir->DirectoryEntries[Entries];
			}
		} while (Entries > 0);
	}
	else {
			Entries = ResourceDir->NumberOfIdEntries + ResourceDir->NumberOfNamedEntries;
			do {
				Entries--;

				if ( (LPWSTR)ResourceDir->DirectoryEntries[Entries].Name == ResourceName ) {
					return &ResourceDir->DirectoryEntries[Entries];
				}
			} while (Entries > ResourceDir->NumberOfNamedEntries);
		
	}

	

	return NULL;
		
}



NTSTATUS LdrFindResource_U(DLL *Dll, IMAGE_RESOURCE_DATA_ENTRY **ResourceDataEntry,LPCWSTR ResourceName, ULONG ResourceType,ULONG Language)
{
	IMAGE_RESOURCE_DIRECTORY *ResourceTypeDir;
	IMAGE_RESOURCE_DIRECTORY *ResourceNameDir;
	IMAGE_RESOURCE_DIRECTORY *ResourceLangDir;

	IMAGE_RESOURCE_DIRECTORY_ENTRY *ResourceTypeDirEntry;
	IMAGE_RESOURCE_DIRECTORY_ENTRY *ResourceNameDirEntry;
	IMAGE_RESOURCE_DIRECTORY_ENTRY *ResourceLangDirEntry;

	PIMAGE_OPTIONAL_HEADER	OptionalHeader;



	ULONG Offset;
		
	OptionalHeader = & Dll->Headers->OptionalHeader;
	ResourceTypeDir = (PIMAGE_RESOURCE_DIRECTORY)
		OptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
	ResourceTypeDir = (PIMAGE_RESOURCE_DIRECTORY)
		((ULONG)ResourceTypeDir + (ULONG)Dll->BaseAddress);


	Offset = (ULONG)ResourceTypeDir;

	ResourceTypeDirEntry = LdrGetNextEntry(ResourceTypeDir, (LPWSTR)ResourceType, Offset);
	
	if ( ResourceTypeDirEntry != NULL ) {
		ResourceNameDir = (IMAGE_RESOURCE_DIRECTORY*)((ResourceTypeDirEntry->OffsetToData & (~0xF0000000)) + Offset);

		ResourceNameDirEntry = LdrGetNextEntry(ResourceNameDir, ResourceName, Offset);

		if ( ResourceNameDirEntry != NULL ) {
		
			ResourceLangDir = (IMAGE_RESOURCE_DIRECTORY*)((ResourceNameDirEntry->OffsetToData & (~0xF0000000)) + Offset);

			ResourceLangDirEntry = LdrGetNextEntry(ResourceLangDir, (LPWSTR)Language, Offset);
			if ( ResourceLangDirEntry != NULL ) {

				*ResourceDataEntry = (IMAGE_RESOURCE_DATA_ENTRY *)(ResourceLangDirEntry->OffsetToData +
									(ULONG)ResourceTypeDir);
				return STATUS_SUCCESS;
			}
			else {
				return -1;
			}
		}	
		else {
				return -1;	
		}
			
	}

	return -1;

}

NTSTATUS LdrAccessResource(DLL *Dll, IMAGE_RESOURCE_DATA_ENTRY *ResourceDataEntry, void **Data)
{
	PIMAGE_SECTION_HEADER	Sections;
	int i;

	if ( Data == NULL )
		return -1;

	if ( ResourceDataEntry == NULL )
		return -1;

	if ( Dll == NULL )
		return -1;

	Sections = (PIMAGE_SECTION_HEADER) SECHDROFFSET(Dll->BaseAddress);

	for (	i = 0;
		(i < Dll->Headers->FileHeader.NumberOfSections);
		i++
		)
	{
		if (Sections[i].VirtualAddress <= ResourceDataEntry->OffsetToData 
			&& Sections[i].VirtualAddress + Sections[i].Misc.VirtualSize > ResourceDataEntry->OffsetToData )
			break;
	}

	if ( i == Dll->Headers->FileHeader.NumberOfSections ) {
		*Data = NULL;
		return -1;
	}

	*Data = (void *)(((ULONG)Dll->BaseAddress + ResourceDataEntry->OffsetToData - (ULONG)Sections[i].VirtualAddress) +
				   (ULONG)Sections[i].PointerToRawData);


	return STATUS_SUCCESS;
}

/* EOF */
