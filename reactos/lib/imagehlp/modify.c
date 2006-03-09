/*
 *	IMAGEHLP library
 *
 *	Copyright 1998	Patrik Stridvall
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/debug.h"
#include "imagehlp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imagehlp);

static WORD CalcCheckSum(DWORD StartValue, LPVOID BaseAddress, DWORD WordCount);
extern PIMAGE_NT_HEADERS WINAPI ImageNtHeader(PVOID Base);

/* Internal Structures we use to keep track of the Bound Imports */
typedef struct _BOUND_FORWARDER_REFS {
    ULONG	TimeDateStamp;
    LPSTR	ModuleName;
} BOUND_FORWARDER_REFS, *PBOUND_FORWARDER_REFS;

typedef struct _BOUND_IMPORT_DESCRIPTOR {
    LPSTR	ModuleName;
    ULONG	TimeDateStamp;
    USHORT	ForwaderReferences;
    PBOUND_FORWARDER_REFS Forwarders;
} BOUND_IMPORT_DESCRIPTOR, *PBOUND_IMPORT_DESCRIPTOR;


VOID
STDCALL
BindpWalkAndBindImports(
    PLOADED_IMAGE File,
    LPSTR DllPath
    );

PBOUND_IMPORT_DESCRIPTOR
STDCALL
BindpCreateBoundImportDescriptor(
	LPSTR LibraryName,
	PLOADED_IMAGE Library,
	PULONG BoundImportDescriptor
    );

BOOL
STDCALL
BindpBindThunk(
    PIMAGE_THUNK_DATA Thunk,
    PLOADED_IMAGE Image,
    PIMAGE_THUNK_DATA BoundThunk,
    PLOADED_IMAGE LoadedLibrary,
    PIMAGE_EXPORT_DIRECTORY Exports,
    PBOUND_IMPORT_DESCRIPTOR BoundImportDescriptor,
    LPSTR DllPath
    );

PIMAGE_BOUND_IMPORT_DESCRIPTOR
STDCALL
BindpCreateBoundImportSection(
    PULONG BoundImportDescriptor,
    PULONG BoundImportsSize
    );

ULONG
STDCALL
BindpAddBoundForwarder(
    PBOUND_IMPORT_DESCRIPTOR BoundImportDescriptor,
    LPSTR DllPath,
    PUCHAR ForwarderString
    );

UCHAR	BoundLibraries[4096];
LPSTR	BoundLibrariesPointer = BoundLibraries;
PULONG	BoundImportDescriptors;

/*
 * BindImageEx
 *
 * FUNCTION:
 *      Binds a PE Image File to its imported Libraries
 * ARGUMENTS:
 *      Flags			- Caller Specified Flags
 *      ImageName		- Name of Imagefile to Bind
 *      DllPath			- Path to search DLL Files in, can be NULL to use Default
 *      SymbolPath		- Path to search Symbol Files in, can be NULL to use Default
 *      StatusRoutine	- Callback routine to notify of Bind Events, can be NULL to disable.
 *
 * RETURNS:
 *      TRUE if Success.
 */
BOOL 
WINAPI 
BindImageEx(
    IN DWORD Flags,
    IN LPSTR ImageName,
    IN LPSTR DllPath,
    IN LPSTR SymbolPath,
    IN PIMAGEHLP_STATUS_ROUTINE StatusRoutine
    )
{
	LOADED_IMAGE				FileData;
	PLOADED_IMAGE				File;
	ULONG						CheckSum, HeaderSum, OldChecksum;
    SYSTEMTIME					SystemTime;
    FILETIME					LastWriteTime;
    
	FIXME("BindImageEx Called for: %s \n", ImageName);

	/* Set and Clear Buffer */
	File = &FileData;
	RtlZeroMemory(File, sizeof(*File));

	/* Request Image Data */
	if (MapAndLoad(ImageName, DllPath, File, TRUE, FALSE)) {

		//FIXME("Image Mapped and Loaded\n");

		/* Read Import Table */
		BindpWalkAndBindImports(File, DllPath);

		//FIXME("Binding Completed, getting Checksum\n");
		
		/* Update Checksum */
		OldChecksum = File->FileHeader->OptionalHeader.CheckSum;
	    CheckSumMappedFile (File->MappedAddress,
							GetFileSize(File->hFile, NULL),
                            &HeaderSum,
                            &CheckSum);
        File->FileHeader->OptionalHeader.CheckSum = CheckSum;

		//FIXME("Saving Changes to file\n");

		/* Save Changes */
        UnmapViewOfFile(File->MappedAddress);

		//FIXME("Setting time\n");

		/* Save new Modified Time */
        GetSystemTime(&SystemTime);
		SystemTimeToFileTime(&SystemTime, &LastWriteTime);
        SetFileTime(File->hFile, NULL, NULL, &LastWriteTime);

		/* Close Handle */
		CloseHandle(File->hFile);
	}

	FIXME("Done\n");
	return TRUE;
}

/*
 * BindpWalkAndBindImports
 *
 * FUNCTION:
 *      Does the actual Binding of the Imports and Forward-Referencing
 *
 * ARGUMENTS:
 *      File			- Name of Imagefile to Bind
 *      DllPath			- Path to search DLL Files in, can be NULL to use Default
 *
 * RETURNS:
 *      Nothing
 */
VOID
STDCALL
BindpWalkAndBindImports(
    PLOADED_IMAGE File,
    LPSTR DllPath
    )
{
	PIMAGE_IMPORT_DESCRIPTOR	Imports;
	PIMAGE_EXPORT_DIRECTORY		Exports;
	ULONG						SizeOfImports;
	ULONG						SizeOfExports;
	ULONG						SizeOfThunks;
	PIMAGE_OPTIONAL_HEADER		OptionalHeader32 = NULL;
    PIMAGE_FILE_HEADER			FileHeader;
	LPSTR						ImportedLibrary;
	PLOADED_IMAGE				LoadedLibrary;
	PBOUND_IMPORT_DESCRIPTOR	BoundImportDescriptor = NULL;
	PIMAGE_BOUND_IMPORT_DESCRIPTOR BoundImportTable;
	PIMAGE_THUNK_DATA			Thunks, TempThunk;
    PIMAGE_THUNK_DATA			BoundThunks, TempBoundThunk;
    ULONG						ThunkCount = 0;
	ULONG						Thunk;
	ULONG						BoundImportTableSize;
	ULONG						VirtBytesFree, HeaderBytesFree, FirstFreeByte, PhysBytesFree;
	
	//FIXME("BindpWalkAndBindImports Called\n");

	/* Load the Import Descriptor */
	Imports = ImageDirectoryEntryToData (File->MappedAddress,
											FALSE, 
											IMAGE_DIRECTORY_ENTRY_IMPORT, 
											&SizeOfImports);

	/* Read the File Header */
	FileHeader = &File->FileHeader->FileHeader;
	OptionalHeader32 = &File->FileHeader->OptionalHeader;

	/* Support for up to 32 imported DLLs */
	BoundImportDescriptors = GlobalAlloc(GMEM_ZEROINIT, 32* sizeof(*BoundImportDescriptor));

	//FIXME("BoundImportDescriptors Allocated\n");

	/* For each Import */
	for(; Imports->Name ; Imports++) {
		
		/* Which DLL is being Imported */
		ImportedLibrary = ImageRvaToVa (File->FileHeader, 
										File->MappedAddress, 
										Imports->Name, 
										&File->LastRvaSection);

		FIXME("Loading Imported DLL: %s \n", ImportedLibrary);

		/* Load the DLL */
		LoadedLibrary = ImageLoad(ImportedLibrary, DllPath);

		FIXME("DLL Loaded at: %p \n", LoadedLibrary->MappedAddress);

		/* Now load the Exports */
		Exports = ImageDirectoryEntryToData (LoadedLibrary->MappedAddress, 
											FALSE, 
											IMAGE_DIRECTORY_ENTRY_EXPORT, 
											&SizeOfExports);

		/* And load the Thunks */
		Thunks = ImageRvaToVa (File->FileHeader, 
								File->MappedAddress, 
								(ULONG)Imports->OriginalFirstThunk, 
								&File->LastRvaSection);

		/* No actual Exports (UPX Packer can do this */
		if (!Thunks) continue;

		//FIXME("Creating Bound Descriptor for this DLL\n");
		
		/* Create Bound Import Descriptor */
		BoundImportDescriptor = BindpCreateBoundImportDescriptor (ImportedLibrary, 
															LoadedLibrary, 
															BoundImportDescriptors);

		/* Count how many Thunks we have */
		ThunkCount = 0;
		TempThunk = Thunks;
		while (TempThunk->u1.AddressOfData) {
			ThunkCount++;
			TempThunk++;
		}

		/* Allocate Memory for the Thunks we will Bind */
		SizeOfThunks = ThunkCount * sizeof(*TempBoundThunk);
		BoundThunks = GlobalAlloc(GMEM_ZEROINIT, SizeOfThunks);

		//FIXME("Binding Thunks\n");

		/* Bind the Thunks */
		TempThunk = Thunks;
		TempBoundThunk = BoundThunks;
		for (Thunk=0; Thunk < ThunkCount; Thunk++) {
			BindpBindThunk (TempThunk, 
						File, 
						TempBoundThunk, 
						LoadedLibrary, 
						Exports, 
						BoundImportDescriptor, 
						DllPath);
			TempThunk++;
			TempBoundThunk++;
		}

		/* Load the Second Thunk Array */
		TempThunk = ImageRvaToVa (File->FileHeader, 
									File->MappedAddress, 
									(ULONG)Imports->FirstThunk, 
									&File->LastRvaSection);

		//FIXME("Copying Bound Thunks\n");

		/* Copy the Pointers */
		if (memcmp(TempThunk, BoundThunks, SizeOfThunks)) {
			RtlCopyMemory(TempThunk, BoundThunks, SizeOfThunks);
		}
		
		/* Set the TimeStamp */
		if (Imports->TimeDateStamp != 0xFFFFFFFF) {
            Imports->TimeDateStamp = 0xFFFFFFFF;
        }

		/* Free the Allocated Memory */
		GlobalFree(BoundThunks);

		//FIXME("Moving to next File\n");
	}

	//FIXME("Creating Bound Import Section\n");

	/* Create the Bound Import Table */
	BoundImportTable = BindpCreateBoundImportSection(BoundImportDescriptors, &BoundImportTableSize);

	/* Zero out the Bound Import Table */
	File->FileHeader->OptionalHeader.DataDirectory
		[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = 0;
	File->FileHeader->OptionalHeader.DataDirectory
		[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size = 0;

	/* Check if we have enough space */
    FirstFreeByte = GetImageUnusedHeaderBytes(File, &VirtBytesFree);
    HeaderBytesFree = File->Sections->VirtualAddress - 
						File->FileHeader->OptionalHeader.SizeOfHeaders + VirtBytesFree;
    PhysBytesFree = File->Sections->PointerToRawData -
						File->FileHeader->OptionalHeader.SizeOfHeaders + VirtBytesFree;

	//FIXME("Calculating Space\n");

    if (BoundImportTableSize > VirtBytesFree) {
        if (BoundImportTableSize > HeaderBytesFree) {
			FIXME("Not enough Space\n");
			return; /* Fail...not enough space */
        }
        if (BoundImportTableSize <= PhysBytesFree) {
            
			FIXME("Header Recalculation\n");
			/* We have enough NULLs to add it, simply enlarge header data */
            File->FileHeader->OptionalHeader.SizeOfHeaders = File->FileHeader->OptionalHeader.SizeOfHeaders - 
															VirtBytesFree +
															BoundImportTableSize +
															((File->FileHeader->OptionalHeader.FileAlignment - 1)
															& ~(File->FileHeader->OptionalHeader.FileAlignment - 1));

        } else  {

			FIXME("Header Resizing\n");

			/* Resize the Headers */
			//UNIMPLEMENTED

            /* Recalculate Headers */
			FileHeader = &File->FileHeader->FileHeader;
			OptionalHeader32 = &File->FileHeader->OptionalHeader;
		}
	}
	
	/* Set Bound Import Table Data */
	File->FileHeader->OptionalHeader.DataDirectory
		[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = FirstFreeByte;
	File->FileHeader->OptionalHeader.DataDirectory
		[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size = BoundImportTableSize;

	//FIXME("Copying Bound Import Table\n");
	
	/* Copy the Bound Import Table */
	RtlCopyMemory(File->MappedAddress + FirstFreeByte, BoundImportTable, BoundImportTableSize);

	/* Free out local copy */
	GlobalFree(BoundImportTable);
}


/*
 * BindpCreateBoundImportSection
 *
 * FUNCTION:
 *      Creates a 32-bit PE Bound Import Table
 *
 * ARGUMENTS:
 *      BoundImportDescriptor		- Pointer to the Bound Import Table
 *      BoundImportsSize			- Size of the Bound Import Table
 *
 * RETURNS:
 *      PIMAGE_BOUND_IMPORT_DESCRIPTOR	- The Bound Import Table
 */
PIMAGE_BOUND_IMPORT_DESCRIPTOR
STDCALL
BindpCreateBoundImportSection(
    PULONG BoundImportDescriptor,
    PULONG BoundImportsSize
    )
{
    ULONG BoundLibraryNamesSize, BoundImportTableSize;
    PBOUND_FORWARDER_REFS BoundForwarder;
	PBOUND_IMPORT_DESCRIPTOR CurrentBoundImportDescriptor;
    PVOID BoundLibraryNames;
    PIMAGE_BOUND_IMPORT_DESCRIPTOR CurrentBoundImportTableEntry, BoundImportTable;
    PIMAGE_BOUND_FORWARDER_REF NewForwarder;

	/* Zero the Sizes */
    *BoundImportsSize = 0;
    BoundLibraryNamesSize = 0;
    BoundImportTableSize = 0;

	/* Start with the first Internal Descriptor */
    CurrentBoundImportDescriptor = (PBOUND_IMPORT_DESCRIPTOR)BoundImportDescriptor;

	/* Loop through every Descriptor we loaded */
    while (CurrentBoundImportDescriptor->ModuleName) {

		/* Add to the size of the Bound Import Table */
        BoundImportTableSize += sizeof(IMAGE_BOUND_IMPORT_DESCRIPTOR);

		/* Check Forwarders */
        BoundForwarder = CurrentBoundImportDescriptor->Forwarders;
		while (BoundForwarder->ModuleName) {

			/* Add to size of Bound Import Table */
            BoundImportTableSize += sizeof(IMAGE_BOUND_FORWARDER_REF);

			/* Next Forwarder */
            BoundForwarder++;
            }

		/* Read Next Internal Descriptor */
        CurrentBoundImportDescriptor++;
        }

	/* Add Terminator for PE Loader*/
    BoundImportTableSize += sizeof(IMAGE_BOUND_IMPORT_DESCRIPTOR);

	/* Name of Libraries Bound in Bound Import Table */
    BoundLibraryNamesSize = ((ULONG)BoundLibrariesPointer - (ULONG)(&BoundLibraries));

	/* Size of the whole table, dword aligned */
    *BoundImportsSize = BoundImportTableSize + 
						((BoundLibraryNamesSize + sizeof(ULONG) - 1) & ~(sizeof(ULONG)-1));

	/* Allocate it */
    BoundImportTable = GlobalAlloc(GMEM_ZEROINIT, *BoundImportsSize);
    
	/* Pointer Library Names inside the Bound Import Table */
    BoundLibraryNames = (PIMAGE_BOUND_IMPORT_DESCRIPTOR)((ULONG_PTR)BoundImportTable + 
																	BoundImportTableSize);

	/* Copy the Library Names */
    RtlCopyMemory(BoundLibraryNames, BoundLibraries, BoundLibraryNamesSize);

	/* Go back to first Internal Descriptor and load first entry in the Bound Import Table */
    CurrentBoundImportTableEntry = (PIMAGE_BOUND_IMPORT_DESCRIPTOR)BoundImportTable;
    CurrentBoundImportDescriptor = (PBOUND_IMPORT_DESCRIPTOR)BoundImportDescriptor;

	/* Copy the data from our Internal Structure to the Bound Import Table */
    while (CurrentBoundImportDescriptor->ModuleName) {
        CurrentBoundImportTableEntry->TimeDateStamp = CurrentBoundImportDescriptor->TimeDateStamp;
        CurrentBoundImportTableEntry->OffsetModuleName = (USHORT)(BoundImportTableSize + 
																(CurrentBoundImportDescriptor->ModuleName - 
																(LPSTR) BoundLibraries));
		CurrentBoundImportTableEntry->NumberOfModuleForwarderRefs = CurrentBoundImportDescriptor->ForwaderReferences;

		/* Copy the data from our Forwader Entries to the Bound Import Table */
        NewForwarder = (PIMAGE_BOUND_FORWARDER_REF)(CurrentBoundImportTableEntry+1);
        BoundForwarder = CurrentBoundImportDescriptor->Forwarders;
		while (BoundForwarder->ModuleName) {
            NewForwarder->TimeDateStamp =BoundForwarder->TimeDateStamp;
            NewForwarder->OffsetModuleName = (USHORT)(BoundImportTableSize + 
													(BoundForwarder->ModuleName - 
													(LPSTR) BoundLibraries));
            NewForwarder++;
            BoundForwarder++;
		}

		/* Move to next Bound Import Table Entry */
        CurrentBoundImportTableEntry = (PIMAGE_BOUND_IMPORT_DESCRIPTOR)NewForwarder;

		/* move to next Internal Descriptor */
        CurrentBoundImportDescriptor++;
	}

	/* Now put the pointer back at the beginning and clear the buffer */
	RtlZeroMemory(BoundLibraries, 4096);
	BoundLibrariesPointer = BoundLibraries;

    return BoundImportTable;
}

/*
 * BindpBindThunk
 *
 * FUNCTION:
 *      Finds the pointer of the Imported function, and writes it into the Thunk,
 *		thus making the Thunk Bound.
 *
 * ARGUMENTS:
 *      Thunk					- Current Thunk in Unbound File
 *      File					- File containing the Thunk
 *      BoundThunk				- Pointer to the corresponding Bound Thunk
 *      LoadedLibrary			- Library containing the Exported Function
 *      Exports					- Export Directory of LoadedLibrary
 *      BoundImportDescriptor	- Internal Bound Import Descriptor of LoadedLibrary
 *      DllPath					- DLL Search Path
 *
 * RETURNS:
 *      TRUE if Suceeded
 */
BOOL
STDCALL
BindpBindThunk(
    PIMAGE_THUNK_DATA Thunk,
    PLOADED_IMAGE File,
    PIMAGE_THUNK_DATA BoundThunk,
    PLOADED_IMAGE LoadedLibrary,
    PIMAGE_EXPORT_DIRECTORY Exports,
    PBOUND_IMPORT_DESCRIPTOR BoundImportDescriptor,
    LPSTR DllPath
    )
{
	PULONG					AddressOfNames;
    PUSHORT					AddressOfOrdinals;
    PULONG					AddressOfPointers;
    PIMAGE_IMPORT_BY_NAME	ImportName;
	ULONG					OrdinalNumber = 0;
    USHORT					HintIndex;
    LPSTR					ExportName;
    ULONG					ExportsBase;
    ULONG					ExportSize;
    UCHAR					NameBuffer[32];
    PIMAGE_OPTIONAL_HEADER	OptionalHeader32 = NULL;
    PIMAGE_OPTIONAL_HEADER	LibraryOptionalHeader32 = NULL;

	/* Get the Pointers to the Tables */
	AddressOfNames = ImageRvaToVa (LoadedLibrary->FileHeader, 
									LoadedLibrary->MappedAddress,
									Exports->AddressOfNames,
									&LoadedLibrary->LastRvaSection);
	AddressOfOrdinals = ImageRvaToVa (LoadedLibrary->FileHeader,
										LoadedLibrary->MappedAddress, 
										Exports->AddressOfNameOrdinals, 
										&LoadedLibrary->LastRvaSection);
	AddressOfPointers = ImageRvaToVa (LoadedLibrary->FileHeader, 
										LoadedLibrary->MappedAddress, 
										Exports->AddressOfFunctions, 
										&LoadedLibrary->LastRvaSection);

	//FIXME("Binding a Thunk\n");

	/* Get the Optional Header */
	OptionalHeader32 = &File->FileHeader->OptionalHeader;
	LibraryOptionalHeader32 = &LoadedLibrary->FileHeader->OptionalHeader;
    
	/* Import by Ordinal */
	if (IMAGE_SNAP_BY_ORDINAL(Thunk->u1.Ordinal) == TRUE) {
        OrdinalNumber = (IMAGE_ORDINAL(Thunk->u1.Ordinal) - Exports->Base);
        ImportName = (PIMAGE_IMPORT_BY_NAME)NameBuffer;
	} else {
		
		/* Import by Name */
		ImportName = ImageRvaToVa (File->FileHeader,
									File->MappedAddress, 
									(ULONG)Thunk->u1.AddressOfData, 
									&File->LastRvaSection);

        for (HintIndex = 0; HintIndex < Exports->NumberOfNames; HintIndex++) {
            
			/* Get the Export Name */
			ExportName = ImageRvaToVa (LoadedLibrary->FileHeader,
										LoadedLibrary->MappedAddress, 
										(ULONG)AddressOfNames[HintIndex], 
										&LoadedLibrary->LastRvaSection);
            
			/* Check if it's the one we want */
			if (!strcmp(ImportName->Name, ExportName)) {
				OrdinalNumber = AddressOfOrdinals[HintIndex];
                break;
            }
		}
	}

	/* Fail if we still didn't find anything */
	if (!OrdinalNumber) return FALSE;

	/* Write the Pointer */
	BoundThunk->u1.Function = (PDWORD)(AddressOfPointers[OrdinalNumber] + LibraryOptionalHeader32->ImageBase);

	/* Load DLL's Exports */
    ExportsBase = (ULONG)ImageDirectoryEntryToData (LoadedLibrary->MappedAddress, 
													TRUE, 
													IMAGE_DIRECTORY_ENTRY_EXPORT, 
													&ExportSize) - 
													(ULONG_PTR)LoadedLibrary->MappedAddress;
	/* RVA to VA */
    ExportsBase += LibraryOptionalHeader32->ImageBase;

	/* Check if the Export is forwarded (meaning that it's pointer is inside the Export Table) */
    if (BoundThunk->u1.Function > (PDWORD)ExportsBase && BoundThunk->u1.Function < (PDWORD)(ExportsBase + ExportSize)) {
        
		//FIXME("This Thunk is a forward...calling forward thunk bounder\n");

		/* Replace the Forwarder String by the actual Pointer */
        BoundThunk->u1.Function = (PDWORD)BindpAddBoundForwarder (BoundImportDescriptor,
														DllPath,
														ImageRvaToVa (LoadedLibrary->FileHeader,
																		LoadedLibrary->MappedAddress,
																		AddressOfPointers[OrdinalNumber],
																		&LoadedLibrary->LastRvaSection));

	}

	/* Return Success */
    return TRUE;
}

/*
 * BindpAddBoundForwarder
 *
 * FUNCTION:
 *      Finds the pointer of the Forwarded function, and writes it into the Thunk,
 *		thus making the Thunk Bound.
 *
 * ARGUMENTS:
 *      BoundImportDescriptor	- Internal Bound Import Descriptor of LoadedLibrary
 *      DllPath					- DLL Search Path
 *		ForwarderString			- Name of the Forwader String
 *
 * RETURNS:
 *      Pointer to the Forwaded Function.
 */
ULONG
STDCALL
BindpAddBoundForwarder(
    PBOUND_IMPORT_DESCRIPTOR BoundImportDescriptor,
    LPSTR DllPath,
    PUCHAR ForwarderString
    )
{
    CHAR					DllName[256];
	PUCHAR					TempDllName;
    PLOADED_IMAGE			LoadedLibrary;
    ULONG					DllNameSize;
    USHORT					OrdinalNumber = 0;
    USHORT					HintIndex;
    ULONG					ExportSize;
    PIMAGE_EXPORT_DIRECTORY Exports;
	ULONG					ExportsBase;
	PULONG					AddressOfNames;
    PUSHORT					AddressOfOrdinals;
    PULONG					AddressOfPointers;
    LPSTR					ExportName;
    ULONG					ForwardedAddress;
    PBOUND_FORWARDER_REFS	BoundForwarder;
    PIMAGE_OPTIONAL_HEADER	OptionalHeader32 = NULL;

NextForwarder:

	/* Get the DLL Name */
    TempDllName = ForwarderString;
    while (*TempDllName && *TempDllName != '.') TempDllName++;
	DllNameSize = (ULONG) (TempDllName - ForwarderString);
    lstrcpyn(DllName, ForwarderString, DllNameSize + 1);

	/* Append .DLL extension */
    DllName[DllNameSize] = '\0';
    strcat(DllName, ".DLL" );

	/* Load it */
	FIXME("Loading the Thunk Library: %s \n", DllName);
    LoadedLibrary = ImageLoad(DllName, DllPath);
    TempDllName += 1;

	/* Return whatever we got back in case of failure*/
	if (!LoadedLibrary) return (ULONG)ForwarderString;
	FIXME("It Loaded at: %p \n", LoadedLibrary->MappedAddress);

	/* Load Exports */
    Exports = ImageDirectoryEntryToData(LoadedLibrary->MappedAddress, FALSE, IMAGE_DIRECTORY_ENTRY_EXPORT, &ExportSize);

	/* Get the Pointers to the Tables */
	AddressOfNames = ImageRvaToVa (LoadedLibrary->FileHeader, 
									LoadedLibrary->MappedAddress, 
									Exports->AddressOfNames, 
									&LoadedLibrary->LastRvaSection);
	AddressOfOrdinals = ImageRvaToVa (LoadedLibrary->FileHeader, 
										LoadedLibrary->MappedAddress, 
										Exports->AddressOfNameOrdinals, 
										&LoadedLibrary->LastRvaSection);
	AddressOfPointers = ImageRvaToVa (LoadedLibrary->FileHeader, 
										LoadedLibrary->MappedAddress, 
										Exports->AddressOfFunctions, 
										&LoadedLibrary->LastRvaSection);

	/* Get the Optional Header */
	OptionalHeader32 = &LoadedLibrary->FileHeader->OptionalHeader;

	/* Get the Ordinal Number */
    for (HintIndex = 0; HintIndex < Exports->NumberOfNames; HintIndex++) {
		
		/* Get the Export Name */
		ExportName = ImageRvaToVa (LoadedLibrary->FileHeader, 
									LoadedLibrary->MappedAddress, 
									AddressOfNames[HintIndex],
									&LoadedLibrary->LastRvaSection);

		/* Check if it matches */
        if (!strcmp(TempDllName, ExportName)) {
			OrdinalNumber = AddressOfOrdinals[HintIndex];
            break;
        }
	}

    do {
		/* Get the Forwarded Address */
		ForwardedAddress = AddressOfPointers[OrdinalNumber] + OptionalHeader32->ImageBase;

		/* Load the First Bound Forward Structure */
		BoundForwarder = BoundImportDescriptor->Forwarders;

		/* Check if we already have the Module Name written */
		while (BoundForwarder->ModuleName) {
			if (!lstrcmpi(DllName, BoundForwarder->ModuleName)) break;
			BoundForwarder++;
		}

		if (!BoundForwarder->ModuleName) {

			/* Save Library Name in Bound Libraries Buffer */
			strcat((char *)BoundLibrariesPointer, DllName);

			/* Set Data */
			BoundForwarder->ModuleName = BoundLibrariesPointer;
			BoundForwarder->TimeDateStamp = LoadedLibrary->FileHeader->FileHeader.TimeDateStamp;
	
			/* Next String */
			BoundLibrariesPointer = BoundLibrariesPointer + strlen((char *)BoundLibrariesPointer) + 1;
			BoundImportDescriptor->ForwaderReferences += 1;
       }

		/* Load DLL's Exports */
		ExportsBase = (ULONG)ImageDirectoryEntryToData (LoadedLibrary->MappedAddress, 
														TRUE, 
														IMAGE_DIRECTORY_ENTRY_EXPORT, 
														&ExportSize) - 
														(ULONG_PTR)LoadedLibrary->MappedAddress;
		ExportsBase += OptionalHeader32->ImageBase;

		//FIXME("I've thunked it\n");

		/* Is this yet another Forward? */
		if (ForwardedAddress > ExportsBase && ForwardedAddress < (ExportsBase + ExportSize)) {
			ForwarderString = ImageRvaToVa (LoadedLibrary->FileHeader, 
											LoadedLibrary->MappedAddress, 
											AddressOfPointers[OrdinalNumber], 
											&LoadedLibrary->LastRvaSection); 
			goto NextForwarder;
		}
	}
	while (0);
    return ForwardedAddress;
}

/*
 * BindpCreateBoundImportDescriptor
 *
 * FUNCTION:
 *      Creates an Internal Structure for the Bound Library
 *
 * ARGUMENTS:
 *      LibraryName				- Name of the Library
 *		Library					- Loaded Library
 *		BoundImportDescriptor	- Internal Bound Import Descriptor of Library
 *
 * RETURNS:
 *      PBOUND_IMPORT_DESCRIPTOR	- Pointer to the Internal Bind Structure
 */
PBOUND_IMPORT_DESCRIPTOR
STDCALL
BindpCreateBoundImportDescriptor(
	LPSTR LibraryName,
	PLOADED_IMAGE Library,
	PULONG BoundImportDescriptor
    )
{
    PBOUND_IMPORT_DESCRIPTOR CurrentBoundImportDescriptor;

	/* Load the First Descriptor */
	CurrentBoundImportDescriptor = (PBOUND_IMPORT_DESCRIPTOR)BoundImportDescriptor;

	/* Check if we've already bound this library */
	while (CurrentBoundImportDescriptor->ModuleName) {
		if (!lstrcmpi(CurrentBoundImportDescriptor->ModuleName, LibraryName)) {
				return CurrentBoundImportDescriptor;
		}
		CurrentBoundImportDescriptor++;
	}

	/* Save Library Name in Bound Libraries Buffer */
	strcat((char *)BoundLibrariesPointer, LibraryName);

	/* Set Data */
	CurrentBoundImportDescriptor->ModuleName = BoundLibrariesPointer;
	CurrentBoundImportDescriptor->TimeDateStamp = Library->FileHeader->FileHeader.TimeDateStamp;	
	
	/* Support for up to 32 Forwarded DLLs */
	CurrentBoundImportDescriptor->Forwarders = GlobalAlloc(GMEM_ZEROINIT, 32* sizeof(BOUND_FORWARDER_REFS));
	
	/* Next String */
	BoundLibrariesPointer = BoundLibrariesPointer + strlen((char *)BoundLibrariesPointer) + 1;

	return CurrentBoundImportDescriptor;
}

/***********************************************************************
 *		BindImage (IMAGEHLP.@)
 */
BOOL WINAPI BindImage(
  LPSTR ImageName, LPSTR DllPath, LPSTR SymbolPath)
{
  return BindImageEx(0, ImageName, DllPath, SymbolPath, NULL);
}

/***********************************************************************
 *		CheckSum (internal)
 */
static WORD CalcCheckSum(
  DWORD StartValue, LPVOID BaseAddress, DWORD WordCount)
{
   LPWORD Ptr;
   DWORD Sum;
   DWORD i;

   Sum = StartValue;
   Ptr = (LPWORD)BaseAddress;
   for (i = 0; i < WordCount; i++)
     {
	Sum += *Ptr;
	if (HIWORD(Sum) != 0)
	  {
	     Sum = LOWORD(Sum) + HIWORD(Sum);
	  }
	Ptr++;
     }

   return (WORD)(LOWORD(Sum) + HIWORD(Sum));
}


/***********************************************************************
 *		CheckSumMappedFile (IMAGEHLP.@)
 */
PIMAGE_NT_HEADERS WINAPI CheckSumMappedFile(
  LPVOID BaseAddress, DWORD FileLength,
  LPDWORD HeaderSum, LPDWORD CheckSum)
{
  PIMAGE_NT_HEADERS Header;
  DWORD CalcSum;
  DWORD HdrSum;

  FIXME("stub\n");

  CalcSum = (DWORD)CalcCheckSum(0,
				BaseAddress,
				(FileLength + 1) / sizeof(WORD));

  Header = ImageNtHeader(BaseAddress);
  HdrSum = Header->OptionalHeader.CheckSum;

  /* Subtract image checksum from calculated checksum. */
  /* fix low word of checksum */
  if (LOWORD(CalcSum) >= LOWORD(HdrSum))
  {
    CalcSum -= LOWORD(HdrSum);
  }
  else
  {
    CalcSum = ((LOWORD(CalcSum) - LOWORD(HdrSum)) & 0xFFFF) - 1;
  }

   /* fix high word of checksum */
  if (LOWORD(CalcSum) >= HIWORD(HdrSum))
  {
    CalcSum -= HIWORD(HdrSum);
  }
  else
  {
    CalcSum = ((LOWORD(CalcSum) - HIWORD(HdrSum)) & 0xFFFF) - 1;
  }

  /* add file length */
  CalcSum += FileLength;

  *CheckSum = CalcSum;
  *HeaderSum = Header->OptionalHeader.CheckSum;

  return Header;
}

/***********************************************************************
 *		MapFileAndCheckSumA (IMAGEHLP.@)
 */
DWORD WINAPI MapFileAndCheckSumA(
  LPSTR Filename, LPDWORD HeaderSum, LPDWORD CheckSum)
{
  HANDLE hFile;
  HANDLE hMapping;
  LPVOID BaseAddress;
  DWORD FileLength;

  TRACE("(%s, %p, %p): stub\n",
    debugstr_a(Filename), HeaderSum, CheckSum
  );

  hFile = CreateFileA(Filename,
		      GENERIC_READ,
		      FILE_SHARE_READ | FILE_SHARE_WRITE,
		      NULL,
		      OPEN_EXISTING,
		      FILE_ATTRIBUTE_NORMAL,
		      0);
  if (hFile == INVALID_HANDLE_VALUE)
  {
    return CHECKSUM_OPEN_FAILURE;
  }

  hMapping = CreateFileMappingW(hFile,
			       NULL,
			       PAGE_READONLY,
			       0,
			       0,
			       NULL);
  if (hMapping == 0)
  {
    CloseHandle(hFile);
    return CHECKSUM_MAP_FAILURE;
  }

  BaseAddress = MapViewOfFile(hMapping,
			      FILE_MAP_READ,
			      0,
			      0,
			      0);
  if (hMapping == 0)
  {
    CloseHandle(hMapping);
    CloseHandle(hFile);
    return CHECKSUM_MAPVIEW_FAILURE;
  }

  FileLength = GetFileSize(hFile,
			   NULL);

  CheckSumMappedFile(BaseAddress,
		     FileLength,
		     HeaderSum,
		     CheckSum);

  UnmapViewOfFile(BaseAddress);
  CloseHandle(hMapping);
  CloseHandle(hFile);

  return 0;
}

/***********************************************************************
 *		MapFileAndCheckSumW (IMAGEHLP.@)
 */
DWORD WINAPI MapFileAndCheckSumW(
  LPWSTR Filename, LPDWORD HeaderSum, LPDWORD CheckSum)
{
  HANDLE hFile;
  HANDLE hMapping;
  LPVOID BaseAddress;
  DWORD FileLength;

  TRACE("(%s, %p, %p): stub\n",
    debugstr_w(Filename), HeaderSum, CheckSum
  );

  hFile = CreateFileW(Filename,
		      GENERIC_READ,
		      FILE_SHARE_READ | FILE_SHARE_WRITE,
		      NULL,
		      OPEN_EXISTING,
		      FILE_ATTRIBUTE_NORMAL,
		      0);
  if (hFile == INVALID_HANDLE_VALUE)
  {
  return CHECKSUM_OPEN_FAILURE;
  }

  hMapping = CreateFileMappingW(hFile,
			       NULL,
			       PAGE_READONLY,
			       0,
			       0,
			       NULL);
  if (hMapping == 0)
  {
    CloseHandle(hFile);
    return CHECKSUM_MAP_FAILURE;
  }

  BaseAddress = MapViewOfFile(hMapping,
			      FILE_MAP_READ,
			      0,
			      0,
			      0);
  if (hMapping == 0)
  {
    CloseHandle(hMapping);
    CloseHandle(hFile);
    return CHECKSUM_MAPVIEW_FAILURE;
  }

  FileLength = GetFileSize(hFile,
			   NULL);

  CheckSumMappedFile(BaseAddress,
		     FileLength,
		     HeaderSum,
		     CheckSum);

  UnmapViewOfFile(BaseAddress);
  CloseHandle(hMapping);
  CloseHandle(hFile);

  return 0;
}

/***********************************************************************
 *		ReBaseImage (IMAGEHLP.@)
 */
BOOL WINAPI ReBaseImage(
  LPSTR CurrentImageName, LPSTR SymbolPath, BOOL fReBase,
  BOOL fRebaseSysfileOk, BOOL fGoingDown, ULONG CheckImageSize,
  ULONG *OldImageSize, ULONG *OldImageBase, ULONG *NewImageSize,
  ULONG *NewImageBase, ULONG TimeStamp)
{
  FIXME(
    "(%s, %s, %d, %d, %d, %ld, %p, %p, %p, %p, %ld): stub\n",
      debugstr_a(CurrentImageName),debugstr_a(SymbolPath), fReBase,
      fRebaseSysfileOk, fGoingDown, CheckImageSize, OldImageSize,
      OldImageBase, NewImageSize, NewImageBase, TimeStamp
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		RemovePrivateCvSymbolic (IMAGEHLP.@)
 */
BOOL WINAPI RemovePrivateCvSymbolic(
  PCHAR DebugData, PCHAR *NewDebugData, ULONG *NewDebugSize)
{
  FIXME("(%p, %p, %p): stub\n",
    DebugData, NewDebugData, NewDebugSize
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		RemoveRelocations (IMAGEHLP.@)
 */
VOID WINAPI RemoveRelocations(PCHAR ImageName)
{
  FIXME("(%p): stub\n", ImageName);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/***********************************************************************
 *		SplitSymbols (IMAGEHLP.@)
 */
BOOL WINAPI SplitSymbols(
  LPSTR ImageName, LPSTR SymbolsPath,
  LPSTR SymbolFilePath, DWORD Flags)
{
  FIXME("(%s, %s, %s, %ld): stub\n",
    debugstr_a(ImageName), debugstr_a(SymbolsPath),
    debugstr_a(SymbolFilePath), Flags
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		UpdateDebugInfoFile (IMAGEHLP.@)
 */
BOOL WINAPI UpdateDebugInfoFile(
  LPSTR ImageFileName, LPSTR SymbolPath,
  LPSTR DebugFilePath, PIMAGE_NT_HEADERS NtHeaders)
{
  FIXME("(%s, %s, %s, %p): stub\n",
    debugstr_a(ImageFileName), debugstr_a(SymbolPath),
    debugstr_a(DebugFilePath), NtHeaders
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		UpdateDebugInfoFileEx (IMAGEHLP.@)
 */
BOOL WINAPI UpdateDebugInfoFileEx(
  LPSTR ImageFileName, LPSTR SymbolPath, LPSTR DebugFilePath,
  PIMAGE_NT_HEADERS NtHeaders, DWORD OldChecksum)
{
  FIXME("(%s, %s, %s, %p, %ld): stub\n",
    debugstr_a(ImageFileName), debugstr_a(SymbolPath),
    debugstr_a(DebugFilePath), NtHeaders, OldChecksum
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
