/* $Id: utils.c,v 1.31 2000/08/28 21:47:34 ekohl Exp $
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
#include <ddk/ntddk.h>
#include <windows.h>
#include <string.h>
#include <wchar.h>
#include <ntdll/ldr.h>
#include <ntos/minmax.h>

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
LdrFindDll (PDLL* Dll,PUNICODE_STRING Name);
//LdrFindDll (PDLL* Dll,PCHAR	Name);

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

NTSTATUS STDCALL
LdrLoadDll (IN PWSTR SearchPath OPTIONAL,
	    IN ULONG LoadFlags,
	    IN PUNICODE_STRING Name,
	    OUT PVOID *BaseAddress OPTIONAL)
{
	WCHAR			fqname [255] = L"\\SystemRoot\\system32\\";
	UNICODE_STRING		UnicodeString;
	OBJECT_ATTRIBUTES	FileObjectAttributes;
	char			BlockBuffer [1024];
	PIMAGE_DOS_HEADER	DosHeader;
	NTSTATUS		Status;
	PIMAGE_NT_HEADERS	NTHeaders;
	ULONG			ImageSize;
	ULONG			InitialViewSize;
	PVOID			ImageBase;
	HANDLE			FileHandle;
	HANDLE			SectionHandle;
	PDLLMAIN_FUNC		Entrypoint = NULL;
	PDLL			Dll;

	if ( Name == NULL )
	{
		*BaseAddress = LdrDllListHead.BaseAddress;
		return STATUS_SUCCESS;
	}

	*BaseAddress = NULL;

	DPRINT("LdrLoadDll(Name \"%wZ\" BaseAddress %x)\n",
	       Name, BaseAddress);

	/*
	 * Build the DLL's absolute name
	 */

	if (wcsncmp(Name->Buffer, L"\\??\\", 3) != 0)
	{
		wcscat(fqname, Name->Buffer);
	}
	else
		wcsncpy(fqname, Name->Buffer, 256);

	DPRINT("fqname \"%S\"\n", fqname);

	/*
	 * Open the DLL's image file.
	 */
	if (LdrFindDll(&Dll, Name) == STATUS_SUCCESS)
	{
		DPRINT ("DLL %wZ already loaded.\n", Name);
		*BaseAddress = Dll->BaseAddress;
		return STATUS_SUCCESS;
	}

	RtlInitUnicodeString(&UnicodeString,
			     fqname);

	InitializeObjectAttributes(
		& FileObjectAttributes,
		& UnicodeString,
		0,
		NULL,
		NULL
		);

	DPRINT("Opening dll \"%wZ\"\n", &UnicodeString);
	
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
		DbgPrint("Dll open of %wZ failed: Status = 0x%08x\n", 
		         &UnicodeString, Status);
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
	if ((DosHeader->e_magic != IMAGE_DOS_MAGIC)
	    || (DosHeader->e_lfanew == 0L)
	    || (*(PULONG)(NTHeaders) != IMAGE_PE_MAGIC))
	{
		DPRINT("NTDLL format invalid\n");
		ZwClose(FileHandle);

		return STATUS_UNSUCCESSFUL;
	}

	ImageBase = (PVOID) NTHeaders->OptionalHeader.ImageBase;
	ImageSize = NTHeaders->OptionalHeader.SizeOfImage;

	DPRINT("ImageBase 0x%08x\n", ImageBase);
	
	/*
	 * Create a section for dll.
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
	 * Map the dll into the process.
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
		DbgPrint("NTDLL.LDR: map view of section failed (Status %x)\n",
		       Status);
		ZwClose(FileHandle);
		return(Status);
	}
	ZwClose(FileHandle);

	Dll = RtlAllocateHeap(
			RtlGetProcessHeap(),
			0,
			sizeof (DLL)
			);
	Dll->Headers = NTHeaders;
	Dll->BaseAddress = (PVOID)ImageBase;
	Dll->Next = LdrDllListHead.Next;
	Dll->Prev = & LdrDllListHead;
	Dll->ReferenceCount = 1;
	LdrDllListHead.Next->Prev = Dll;
	LdrDllListHead.Next = Dll;


   if ((Dll->Headers->FileHeader.Characteristics & IMAGE_FILE_DLL) ==
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
				Dll->BaseAddress,
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

	*BaseAddress = Dll->BaseAddress;
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
static NTSTATUS LdrFindDll(PDLL* Dll, PUNICODE_STRING Name)
{
   PIMAGE_EXPORT_DIRECTORY	ExportDir;
   PIMAGE_OPTIONAL_HEADER	OptionalHeader;
   DLL			* current;
   UNICODE_STRING DllName;

   DPRINT("NTDLL.LdrFindDll(Name %wZ)\n", Name);

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

	RtlCreateUnicodeStringFromAsciiz (&DllName,
		ExportDir->Name + current->BaseAddress);

	DPRINT("Scanning %wZ %wZ\n", &DllName, Name);

	if (RtlCompareUnicodeString(&DllName, Name, TRUE) == 0)
	  {
	     *Dll = current;
	     current->ReferenceCount++;
	     RtlFreeUnicodeString (&DllName);
	     return STATUS_SUCCESS;
	  }

	current = current->Next;
	
     } while (current != & LdrDllListHead);

   RtlFreeUnicodeString (&DllName);

   DPRINT("Failed to find dll %wZ\n", Name);

   return STATUS_UNSUCCESSFUL;
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
   ULONG	i;
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
static PVOID
LdrGetExportByOrdinal (
	PVOID	BaseAddress,
	ULONG	Ordinal
	)
{
	PIMAGE_EXPORT_DIRECTORY	ExportDir;
	PDWORD			* ExFunctions;
	USHORT			* ExOrdinals;

	ExportDir = (PIMAGE_EXPORT_DIRECTORY)
		RtlImageDirectoryEntryToData (BaseAddress,
					      TRUE,
					      IMAGE_DIRECTORY_ENTRY_EXPORT,
					      NULL);


	ExOrdinals = (USHORT *)
		RVA(
			BaseAddress,
			ExportDir->AddressOfNameOrdinals
			);
	ExFunctions = (PDWORD *)
		RVA(
			BaseAddress,
			ExportDir->AddressOfFunctions
			);
	DbgPrint(
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
static PVOID
LdrGetExportByName (
	PVOID  BaseAddress,
	PUCHAR SymbolName
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

	ExportDir = (PIMAGE_EXPORT_DIRECTORY)
		RtlImageDirectoryEntryToData (BaseAddress,
					      TRUE,
					      IMAGE_DIRECTORY_ENTRY_EXPORT,
					      NULL);

	/*
	 * Get header pointers
	 */
	ExNames = (PDWORD *)
		RVA(
			BaseAddress,
			ExportDir->AddressOfNames
			);
	ExOrdinals = (USHORT *)
		RVA(
			BaseAddress,
			ExportDir->AddressOfNameOrdinals
			);
	ExFunctions = (PDWORD *)
		RVA(
			BaseAddress,
			ExportDir->AddressOfFunctions
			);
	for (	i = 0;
		( i < ExportDir->NumberOfFunctions);
		i++
		)
	{
		ExName = RVA(
				BaseAddress,
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
			return(RVA(BaseAddress, ExFunctions[Ordinal]));
		}
	}

	DbgPrint("LdrGetExportByName() = failed to find %s\n",SymbolName);

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
   PVOID BaseAddress;
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
	UNICODE_STRING DllName;
	DWORD	pName;
	PWORD	pHint;

	DPRINT("ImportModule->Directory->dwRVAModuleName %s\n",
	       (PCHAR)(ImageBase + ImportModuleDirectory->dwRVAModuleName));

	RtlCreateUnicodeStringFromAsciiz (&DllName,
		  (PCHAR)(ImageBase + ImportModuleDirectory->dwRVAModuleName));

	Status = LdrLoadDll(NULL,
			    0,
			    &DllName,
			    &BaseAddress);
	RtlFreeUnicodeString (&DllName);
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
		    LdrGetExportByOrdinal(BaseAddress,
					  Ordinal);
	       }
	     else
	       {
		  pName = (DWORD) (
				   ImageBase
				   + *FunctionNameList
				   + 2);
		  pHint = (PWORD) (
				   ImageBase
				   + *FunctionNameList);

		  *ImportAddressList =
		    LdrGetExportByName(BaseAddress,
				       (PUCHAR) pName);
		  if ((*ImportAddressList) == NULL)
		    {
		       DbgPrint("Failed to import %s\n", pName);
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
		      HANDLE SectionHandle)
{
   NTSTATUS		Status;
   PEPFUNC		EntryPoint = NULL;
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
   if (SectionHandle != NULL)
     {
	LdrMapSections(NtCurrentProcess(),
		       ImageBase,
		       SectionHandle,
		       NTHeaders);
     }

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
	     DbgPrint("LdrPerformRelocations() failed\n");
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
	     DbgPrint("LdrFixupImports() failed\n");
	     return NULL;
	  }
     }

   /*
    * Compute the DLL's entry point's address.
    */
   if (NTHeaders->OptionalHeader.AddressOfEntryPoint != 0)
     {
	EntryPoint = (PEPFUNC) (ImageBase
			   + NTHeaders->OptionalHeader.AddressOfEntryPoint);
     }
   DPRINT("LdrPEStartup() = %x\n",EntryPoint);
   return EntryPoint;
}


NTSTATUS STDCALL
LdrUnloadDll (IN PVOID BaseAddress)
{
   PDLLMAIN_FUNC Entrypoint;
   PDLL Dll;
   NTSTATUS Status;

   if (BaseAddress == NULL)
     return STATUS_SUCCESS;

   Dll = &LdrDllListHead;

   do
     {
	if (Dll->BaseAddress == BaseAddress)
	  {

	     if ( Dll->ReferenceCount > 1 )
	       {
		  Dll->ReferenceCount--;
		  return STATUS_SUCCESS;
	       }


	     if ((Dll->Headers->FileHeader.Characteristics & IMAGE_FILE_DLL ) == IMAGE_FILE_DLL)
	       {
		  Entrypoint = (PDLLMAIN_FUNC) LdrPEStartup(Dll->BaseAddress,
							    Dll->SectionHandle);
		  if (Entrypoint != NULL)
		    {
		       DPRINT("Calling entry point at 0x%08x\n", Entrypoint);
		       Entrypoint(Dll->BaseAddress,
				  DLL_PROCESS_DETACH,
				  NULL);
		    }
		  else
		    {
		       DPRINT("NTDLL.LDR: Entrypoint is NULL for \n");
		    }
	       }
	     Status = ZwUnmapViewOfSection (NtCurrentProcess (),
					    Dll->BaseAddress);

	     ZwClose (Dll->SectionHandle);
	     return Status;
	  }

	Dll = Dll->Next;
     } while (Dll != & LdrDllListHead);

   DPRINT("NTDLL.LDR: Dll not found\n")

   return STATUS_UNSUCCESSFUL;
}


NTSTATUS STDCALL
LdrFindResource_U(PVOID BaseAddress,
                  PLDR_RESOURCE_INFO ResourceInfo,
                  ULONG Level,
                  PIMAGE_RESOURCE_DATA_ENTRY *ResourceDataEntry)
{
   PIMAGE_RESOURCE_DIRECTORY ResDir;
   PIMAGE_RESOURCE_DIRECTORY ResBase;
   PIMAGE_RESOURCE_DIRECTORY_ENTRY ResEntry;
   NTSTATUS Status = STATUS_SUCCESS;
   ULONG EntryCount;
   PWCHAR ws;
   ULONG i;
   ULONG Id;

   DPRINT ("LdrFindResource_U()\n");

   /* Get the pointer to the resource directory */
   ResDir = (PIMAGE_RESOURCE_DIRECTORY)
	RtlImageDirectoryEntryToData (BaseAddress,
				      TRUE,
				      IMAGE_DIRECTORY_ENTRY_RESOURCE,
				      &i);
   if (ResDir == NULL)
     {
	return STATUS_RESOURCE_DATA_NOT_FOUND;
     }

   DPRINT("ResourceDirectory: %x\n", (ULONG)ResDir);

   ResBase = ResDir;

   /* Let's go into resource tree */
   for (i = 0; i < Level; i++)
     {
	DPRINT("ResDir: %x\n", (ULONG)ResDir);
	Id = ((PULONG)ResourceInfo)[i];
	EntryCount = ResDir->NumberOfNamedEntries;
	ResEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(ResDir + 1);
	DPRINT("ResEntry %x\n", (ULONG)ResEntry);
	if (Id & 0xFFFF0000)
	  {
	     /* Resource name is a unicode string */
	     for (; EntryCount--; ResEntry++)
	       {
		  /* Scan entries for equal name */
		  if (ResEntry->Name & 0x80000000)
		    {
		       ws = (PWCHAR)((ULONG)ResDir + (ResEntry->Name & 0x7FFFFFFF));
		       if (!wcsncmp((PWCHAR)Id, ws + 1, *ws ) &&
			   wcslen((PWCHAR)Id) == (int)*ws )
			 {
			    goto found;
			 }
		    }
	       }
	  }
	else
	  {
	     /* We use ID number instead of string */
	     ResEntry += EntryCount;
	     EntryCount = ResDir->NumberOfIdEntries;
	     for (; EntryCount--; ResEntry++)
	       {
		  /* Scan entries for equal name */
		  if (ResEntry->Name == Id)
		    {
		     DPRINT("ID entry found %x\n", Id);
		     goto found;
		    }
	       }
	  }
	DPRINT("Error %lu\n", i);

	  switch (i)
	  {
	     case 0:
		return STATUS_RESOURCE_TYPE_NOT_FOUND;

	     case 1:
		return STATUS_RESOURCE_NAME_NOT_FOUND;

	     case 2:
		if (ResDir->NumberOfNamedEntries || ResDir->NumberOfIdEntries)
		  {
		     /* Use the first available language */
		     ResEntry = (IMAGE_RESOURCE_DIRECTORY_ENTRY*)(ResDir + 1);
		     break;
		  }
		return STATUS_RESOURCE_LANG_NOT_FOUND;

	     case 3:
		return STATUS_RESOURCE_DATA_NOT_FOUND;

	     default:
		return STATUS_INVALID_PARAMETER;
	  }
found:;
	ResDir = (PIMAGE_RESOURCE_DIRECTORY)((ULONG)ResBase +
		(ResEntry->OffsetToData & 0x7FFFFFFF));
     }
   DPRINT("ResourceDataEntry: %x\n", (ULONG)ResDir);

   if (ResourceDataEntry)
     {
	*ResourceDataEntry = (PVOID)ResDir;
     }

  return Status;
}


NTSTATUS STDCALL
LdrAccessResource(IN  PVOID BaseAddress,
                  IN  PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry,
                  OUT PVOID *Resource OPTIONAL,
                  OUT PULONG Size OPTIONAL)
{
   PIMAGE_SECTION_HEADER Section;
   PIMAGE_NT_HEADERS NtHeader;
   ULONG SectionRva;
   ULONG SectionVa;
   ULONG DataSize;
   ULONG Offset = 0;
   ULONG Data;

   Data = (ULONG)RtlImageDirectoryEntryToData (BaseAddress,
					       TRUE,
					       IMAGE_DIRECTORY_ENTRY_RESOURCE,
					       &DataSize);
   if (Data == 0)
	return STATUS_RESOURCE_DATA_NOT_FOUND;

   if ((ULONG)BaseAddress & 1)
     {
	/* loaded as ordinary file */
	NtHeader = RtlImageNtHeader((PVOID)((ULONG)BaseAddress & ~1UL));
	Offset = (ULONG)BaseAddress - Data + NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
	Section = RtlImageRvaToSection (NtHeader, BaseAddress, NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress);
	if (Section == NULL)
	  {
	     return STATUS_RESOURCE_DATA_NOT_FOUND;
	  }

	if (Section->Misc.VirtualSize < ResourceDataEntry->OffsetToData)
	  {
	     SectionRva = RtlImageRvaToSection (NtHeader, BaseAddress, ResourceDataEntry->OffsetToData)->VirtualAddress;
	     SectionVa = RtlImageRvaToVa(NtHeader, BaseAddress, SectionRva, NULL);
	     Offset = SectionRva - SectionVa + Data - Section->VirtualAddress;
	  }
     }

   if (Resource)
     {
	*Resource = (PVOID)(ResourceDataEntry->OffsetToData - Offset + (ULONG)BaseAddress);
     }

   if (Size)
     {
	*Size = ResourceDataEntry->Size;
     }

   return STATUS_SUCCESS;
}


NTSTATUS
STDCALL
LdrDisableThreadCalloutsForDll (
	IN	PVOID	BaseAddress,
	IN	BOOLEAN	Disable
	)
{
	/* FIXME: implement it! */

	return STATUS_SUCCESS;
}


NTSTATUS STDCALL
LdrFindResourceDirectory_U (IN PVOID BaseAddress,
                            WCHAR **name,
                            DWORD level,
                            OUT PVOID *addr)
{
   PIMAGE_RESOURCE_DIRECTORY ResDir;
   PIMAGE_RESOURCE_DIRECTORY_ENTRY ResEntry;
   ULONG EntryCount;
   ULONG i;
   NTSTATUS Status = STATUS_SUCCESS;
   WCHAR *ws;

   /* Get the pointer to the resource directory */
   ResDir = (PIMAGE_RESOURCE_DIRECTORY)
	RtlImageDirectoryEntryToData (BaseAddress,
				      TRUE,
				      IMAGE_DIRECTORY_ENTRY_RESOURCE,
				      &i);
   if (ResDir == NULL)
     {
	return STATUS_RESOURCE_DATA_NOT_FOUND;
     }

   /* Let's go into resource tree */
   for (i = 0; i < level; i++, name++)
     {
	EntryCount = ResDir->NumberOfNamedEntries;
	ResEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(ResDir + 1);
	if ((ULONG)(*name) & 0xFFFF0000)
	  {
	     /* Resource name is a unicode string */
	     for (; EntryCount--; ResEntry++)
	       {
		  /* Scan entries for equal name */
		  if (ResEntry->Name & 0x80000000)
		    {
		       ws = (WCHAR*)((ULONG)ResDir + (ResEntry->Name & 0x7FFFFFFF));
		       if (!wcsncmp( *name, ws + 1, *ws ) && wcslen( *name ) == (int)*ws )
			 {
			    goto found;
			 }
		    }
	       }
	  }
	else
	  {
	     /* We use ID number instead of string */
	     ResEntry += EntryCount;
	     EntryCount = ResDir->NumberOfIdEntries;
	     for (; EntryCount--; ResEntry++)
	       {
		  /* Scan entries for equal name */
		  if (ResEntry->Name == (ULONG)(*name))
		     goto found;
	       }
	  }

	  switch (i)
	  {
	     case 0:
		return STATUS_RESOURCE_TYPE_NOT_FOUND;

	     case 1:
		return STATUS_RESOURCE_NAME_NOT_FOUND;

	     case 2:
		Status = STATUS_RESOURCE_LANG_NOT_FOUND;
		/* Just use first language entry */
		if (ResDir->NumberOfNamedEntries || ResDir->NumberOfIdEntries)
		  {
		     ResEntry = (IMAGE_RESOURCE_DIRECTORY_ENTRY*)(ResDir + 1);
		     break;
		  }
		return Status;

	     case 3:
		return STATUS_RESOURCE_DATA_NOT_FOUND;

	     default:
		return STATUS_INVALID_PARAMETER;
	  }
found:;
	ResDir = (PIMAGE_RESOURCE_DIRECTORY)((ULONG)ResDir + ResEntry->OffsetToData);
     }

   if (addr)
     {
	*addr = (PVOID)ResDir;
     }

  return Status;
}


NTSTATUS STDCALL
LdrGetDllHandle (IN ULONG Unknown1,
                 IN ULONG Unknown2,
                 IN PUNICODE_STRING DllName,
                 OUT PVOID *BaseAddress)
{
   PIMAGE_EXPORT_DIRECTORY ExportDir;
   PIMAGE_OPTIONAL_HEADER OptionalHeader;
   ANSI_STRING AnsiName;
   DLL *current;

   DPRINT("LdrGetDllHandle (Unknown1 %x Unknown2 %x DllName %wZ BaseAddress %p)\n",
          Unknown1, Unknown2, DllName, BaseAddress);

   current = &LdrDllListHead;

   // NULL is the current process
   if ( DllName == NULL )
     {
	*BaseAddress = current->BaseAddress;
	DPRINT1("BaseAddress %x\n", *BaseAddress);
	return STATUS_SUCCESS;
     }

   RtlUnicodeStringToAnsiString (&AnsiName, DllName, TRUE);

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
	       ExportDir->Name + current->BaseAddress, AnsiName.Buffer);

	if (!_stricmp(ExportDir->Name + current->BaseAddress, AnsiName.Buffer))
	  {
	     RtlFreeAnsiString (&AnsiName);
	     *BaseAddress = current->BaseAddress;
	     DPRINT1("BaseAddress %x\n", *BaseAddress);
	     return STATUS_SUCCESS;
	  }

	current = current->Next;
	
     } while (current != & LdrDllListHead);

   DbgPrint("Failed to find dll %s\n", AnsiName.Buffer);
   RtlFreeAnsiString (&AnsiName);
   *BaseAddress = NULL;

   return STATUS_UNSUCCESSFUL;
}


NTSTATUS STDCALL
LdrGetProcedureAddress (IN PVOID BaseAddress,
                        IN PANSI_STRING Name,
                        IN ULONG Ordinal,
                        OUT PVOID *ProcedureAddress)
{
   PIMAGE_EXPORT_DIRECTORY ExportDir;
   PUSHORT OrdinalPtr;
   PULONG NamePtr;
   PULONG AddressPtr;
   ULONG i = 0;

   DPRINT("LdrGetProcedureAddress (BaseAddress %x Name %Z Ordinal %lu ProcedureAddress %x)\n",
          BaseAddress, Name, Ordinal, ProcedureAddress);

   /* Get the pointer to the export directory */
   ExportDir = (PIMAGE_EXPORT_DIRECTORY)
		RtlImageDirectoryEntryToData (BaseAddress,
					      TRUE,
					      IMAGE_DIRECTORY_ENTRY_EXPORT,
					      &i);

   DPRINT("ExportDir %x i %lu\n", ExportDir, i);

   if (!ExportDir || !i || !ProcedureAddress)
     {
	return STATUS_INVALID_PARAMETER;
     }

   AddressPtr = (PULONG)((ULONG)BaseAddress + (ULONG)ExportDir->AddressOfFunctions);
   if (Name && Name->Length)
     {
	/* by name */
	OrdinalPtr = (PUSHORT)((ULONG)BaseAddress + (ULONG)ExportDir->AddressOfNameOrdinals);
	NamePtr = (PULONG)((ULONG)BaseAddress + (ULONG)ExportDir->AddressOfNames);
	for( i = 0; i < ExportDir->NumberOfNames; i++, NamePtr++, OrdinalPtr++)
	  {
	     if (!_strnicmp(Name->Buffer, (char*)(BaseAddress + *NamePtr), Name->Length))
	       {
		  *ProcedureAddress = (PVOID)((ULONG)BaseAddress + (ULONG)AddressPtr[*OrdinalPtr]);
		  return STATUS_SUCCESS;
	       }
	  }
	DbgPrint("LdrGetProcedureAddress: Can't resolve symbol '%Z'\n", Name);
     }
   else
     {
	/* by ordinal */
	Ordinal &= 0x0000FFFF;
	if (Ordinal - ExportDir->Base < ExportDir->NumberOfFunctions)
	  {
	     *ProcedureAddress = (PVOID)((ULONG)BaseAddress + (ULONG)AddressPtr[Ordinal - ExportDir->Base]);
	     return STATUS_SUCCESS;
	  }
	DbgPrint("LdrGetProcedureAddress: Can't resolve symbol @%d\n", Ordinal);
  }

   return STATUS_PROCEDURE_NOT_FOUND;
}

/* EOF */
