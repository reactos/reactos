/* $Id: utils.c,v 1.13 1999/11/02 08:55:38 dwelch Exp $
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

NTSTATUS
LdrLoadDll (
	PDLL	* Dll,
	PCHAR	Name
	)
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
   

	DPRINT("LdrLoadDll(Base %x, Name \"%s\")\n", Dll, Name);

	if ( LdrFindDll(Dll,Name) == STATUS_SUCCESS )
		return STATUS_SUCCESS;

	/*
	 * Build the DLL's absolute name
	 */
	strcat(fqname, Name);
   
	DPRINT("fqname \"%s\"\n", fqname);
  	/*
	 * Open the DLL's image file.
	 */
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
		DPRINT("Dll open failed: Status = 0x%08x\n", Status);
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
			(PVOID *) & ImageBase,
			0,
			InitialViewSize,
			NULL,
			& InitialViewSize,
			0,
			MEM_COMMIT,
                        PAGE_READWRITE
			);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("NTDLL.LDR: map view of section failed ");
		ZwClose(FileHandle);
	
		return Status;
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
static
NTSTATUS
LdrFindDll (
	PDLL	* Dll,
	PCHAR	Name
	)
{
	PIMAGE_EXPORT_DIRECTORY	ExportDir;
	DLL			* current;
	PIMAGE_OPTIONAL_HEADER	OptionalHeader;


//	DPRINT("NTDLL.LdrFindDll(Name %s)\n", Name);
   
	current = & LdrDllListHead;
	do
	{
		OptionalHeader = & current->Headers->OptionalHeader;
		ExportDir = (PIMAGE_EXPORT_DIRECTORY)
			OptionalHeader->DataDirectory[
				IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
		ExportDir = (PIMAGE_EXPORT_DIRECTORY)
			((ULONG)ExportDir + (ULONG)current->BaseAddress);
	
//		DPRINT("Scanning  %x %x %x\n",
//			ExportDir->Name,
//			current->BaseAddress,
//			(ExportDir->Name + current->BaseAddress)
//			);
//		DPRINT("Scanning %s\n",
//			ExportDir->Name + current->BaseAddress
//			);
		if (strcmp(ExportDir->Name + current->BaseAddress, Name) == 0)
		{
			*Dll = current;
			current->ReferenceCount++;
			return STATUS_SUCCESS;
		}
	
		current = current->Next;
		
	} while (current != & LdrDllListHead);
   
	dprintf("Failed to find dll %s\n",Name);
   
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
NTSTATUS
LdrMapSections (
	HANDLE			ProcessHandle,
	PVOID			ImageBase,
	HANDLE			SectionHandle,
	PIMAGE_NT_HEADERS	NTHeaders
	)
{
	ULONG		i;
	NTSTATUS	Status;


	for (	i = 0;
		(i < NTHeaders->FileHeader.NumberOfSections);
		i++
		)
	{
		PIMAGE_SECTION_HEADER	Sections;
		LARGE_INTEGER		Offset;
		ULONG			Base;
	
		Sections = (PIMAGE_SECTION_HEADER) SECHDROFFSET(ImageBase);
		Base = (ULONG) (Sections[i].VirtualAddress + ImageBase);
		Offset.u.LowPart = Sections[i].PointerToRawData;
		Offset.u.HighPart = 0;
		Status = ZwMapViewOfSection(
				SectionHandle,
				ProcessHandle,
				(PVOID *) & Base,
				0,
				Sections[i].Misc.VirtualSize,
				& Offset,
				(PULONG) & Sections[i].Misc.VirtualSize,
				0,
				MEM_COMMIT,
				PAGE_READWRITE
				);
		if (!NT_SUCCESS(Status))
		{
			return Status;
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
static
NTSTATUS
LdrPerformRelocations (
	PIMAGE_NT_HEADERS	NTHeaders,
	PVOID			ImageBase
	)
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


	RelocationRVA =
		NTHeaders->OptionalHeader
			.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC]
				.VirtualAddress;
	if (RelocationRVA)
	{
		RelocationDir = (PRELOCATION_DIRECTORY)
			((PCHAR)ImageBase + RelocationRVA);

		while (RelocationDir->SizeOfBlock)
		{
			Delta32 = (unsigned long) (
				ImageBase
				- NTHeaders->OptionalHeader.ImageBase
				);
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
static
NTSTATUS
LdrFixupImports (
	PIMAGE_NT_HEADERS	NTHeaders,
	PVOID			ImageBase
	)
{
	PIMAGE_IMPORT_MODULE_DIRECTORY	ImportModuleDirectory;
	ULONG				Ordinal;
	PDLL				Module;
	NTSTATUS			Status;


	/*
	 * Process each import module.
	 */
	ImportModuleDirectory = (PIMAGE_IMPORT_MODULE_DIRECTORY) (
		ImageBase
		+ NTHeaders->OptionalHeader
			.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
				.VirtualAddress
		);
	while (ImportModuleDirectory->dwRVAModuleName)
	{
		PVOID	* ImportAddressList; 
		PULONG	FunctionNameList;
		DWORD	pName;
		PWORD	pHint;
	
		Status = LdrLoadDll(
				& Module,
				(PCHAR) (
					ImageBase
					+ ImportModuleDirectory->dwRVAModuleName
					)
				);
		if (!NT_SUCCESS(Status))
		{
			return Status;
		}
		/*
		 * Get the import address list.
		 */
		ImportAddressList = (PVOID *) (
			NTHeaders->OptionalHeader.ImageBase
			+ ImportModuleDirectory->dwRVAFunctionAddressList
			);
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
PEPFUNC
LdrPEStartup (
	PVOID	ImageBase,
	HANDLE	SectionHandle
	)
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
	LdrMapSections(
		NtCurrentProcess(),
		ImageBase,
		SectionHandle,
		NTHeaders
		);
	/*
	 * If the base address is different from the
	 * one the DLL is actually loaded, perform any
	 * relocation.
	 */
	if (ImageBase != (PVOID) NTHeaders->OptionalHeader.ImageBase)
	{
		Status = LdrPerformRelocations(
				NTHeaders,
				ImageBase
				);
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
	if (NTHeaders->OptionalHeader
			.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
				.VirtualAddress != 0)
	{
		Status = LdrFixupImports(
				NTHeaders,
				ImageBase
				);
		if (!NT_SUCCESS(Status))
		{
			dprintf("LdrFixupImports() failed\n");
			return NULL;
		}
	}
	/*
	 * Compute the DLL's entry point's address.
	 */
	EntryPoint = (PEPFUNC) (
		ImageBase
		+ NTHeaders->OptionalHeader.AddressOfEntryPoint
		);
	dprintf("LdrPEStartup() = %x\n",EntryPoint);
	return EntryPoint;
}

NTSTATUS LdrUnloadDll(PDLL Dll)
{

	PDLLMAIN_FUNC		Entrypoint;
	NTSTATUS		Status;


	if ( Dll->ReferenceCount > 1 ) {
		Dll->ReferenceCount--;
		return STATUS_SUCCESS;
	}

	Entrypoint =
		(PDLLMAIN_FUNC) LdrPEStartup(
					Dll->BaseAddress,
					Dll->SectionHandle
					);
	if (Entrypoint != NULL)
	{
		DPRINT("Calling entry point at 0x%08x\n", Entrypoint);
		if (FALSE == Entrypoint(
				Dll,
				DLL_PROCESS_DETACH,
				NULL
				))
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


	Status = ZwUnmapViewOfSection(
		NtCurrentProcess(),
		Dll->BaseAddress
	);

	ZwClose(Dll->SectionHandle);
   
	return Status;
}

/* EOF */
