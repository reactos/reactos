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
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winerror.h"
#include "wine/debug.h"
#include "imagehlp.h"

/* Couple of Hacks */
extern inline DWORD WINAPI GetLastError(void)
{
    DWORD ret;
    __asm__ __volatile__( ".byte 0x64\n\tmovl 0x60,%0" : "=r" (ret) );
    return ret;
}

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }

WINE_DEFAULT_DEBUG_CHANNEL(imagehlp);

/***********************************************************************
 *           Data
 */

static PLOADED_IMAGE IMAGEHLP_pFirstLoadedImage=NULL;
static PLOADED_IMAGE IMAGEHLP_pLastLoadedImage=NULL;

static LOADED_IMAGE IMAGEHLP_EmptyLoadedImage = {
  NULL,       /* ModuleName */
  0,          /* hFile */
  NULL,       /* MappedAddress */
  NULL,       /* FileHeader */
  NULL,       /* LastRvaSection */
  0,          /* NumberOfSections */
  NULL,       /* Sections */
  1,          /* Characteristics */
  FALSE,      /* fSystemImage */
  FALSE,      /* fDOSImage */
  { &IMAGEHLP_EmptyLoadedImage.Links, &IMAGEHLP_EmptyLoadedImage.Links }, /* Links */
  148,        /* SizeOfImage; */
};

extern HANDLE IMAGEHLP_hHeap;
BOOLEAN DllListInitialized;
LIST_ENTRY ImageLoadListHead;

/***********************************************************************
 *		EnumerateLoadedModules (IMAGEHLP.@)
 */
BOOL WINAPI EnumerateLoadedModules(
  HANDLE hProcess,
  PENUMLOADED_MODULES_CALLBACK EnumLoadedModulesCallback,
  PVOID UserContext)
{
  FIXME("(%p, %p, %p): stub\n",
    hProcess, EnumLoadedModulesCallback, UserContext
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		GetTimestampForLoadedLibrary (IMAGEHLP.@)
 */
DWORD WINAPI GetTimestampForLoadedLibrary(HMODULE Module)
{
  FIXME("(%p): stub\n", Module);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		GetImageConfigInformation (IMAGEHLP.@)
 */
BOOL WINAPI GetImageConfigInformation(
  PLOADED_IMAGE LoadedImage,
  PIMAGE_LOAD_CONFIG_DIRECTORY ImageConfigInformation)
{
  FIXME("(%p, %p): stub\n",
    LoadedImage, ImageConfigInformation
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		GetImageUnusedHeaderBytes (IMAGEHLP.@)
 */
DWORD 
WINAPI 
GetImageUnusedHeaderBytes(
    PLOADED_IMAGE LoadedImage,
    LPDWORD SizeUnusedHeaderBytes
    )
{
    DWORD						FirstFreeByte;
    PIMAGE_OPTIONAL_HEADER		OptionalHeader32 = NULL;
    PIMAGE_NT_HEADERS			NtHeaders;
	ULONG						i;

	/* Read the NT Headers */
    NtHeaders = LoadedImage->FileHeader;

	/* Find the first free byte, which is after all the headers and sections */
    FirstFreeByte = (ULONG_PTR)NtHeaders - (ULONG_PTR)LoadedImage->MappedAddress +
					FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) +
					NtHeaders->FileHeader.SizeOfOptionalHeader +
					NtHeaders->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);

	/* Get the Optional Header */
	OptionalHeader32 = &LoadedImage->FileHeader->OptionalHeader;

	/*	There is the possibilty that one of the Data Directories is in the PE Header
		itself, so we'll need to find such a case and add it to our PE used space */
    for ( i = 0; i<OptionalHeader32->NumberOfRvaAndSizes; i++ ) {

		/* If the VA is less then the size of headers, then the data is inside the PE header */
        if (OptionalHeader32->DataDirectory[i].VirtualAddress < OptionalHeader32->SizeOfHeaders) {

			/* However, make sure it's not 0, which means it doesnt actually exist */
            if (OptionalHeader32->DataDirectory[i].VirtualAddress >= FirstFreeByte) {

				/* Our first empty byte is after this Directory Data then */
                FirstFreeByte = OptionalHeader32->DataDirectory[i].VirtualAddress +
												OptionalHeader32->DataDirectory[i].Size;
                }
            }
        }

	/* Return the unused Header Bytes */
    *SizeUnusedHeaderBytes = OptionalHeader32->SizeOfHeaders - FirstFreeByte;

	/* And return the first free byte*/
    return FirstFreeByte;
}

/***********************************************************************
 *		ImageDirectoryEntryToData (IMAGEHLP.@)
 */
PVOID 
WINAPI
ImageDirectoryEntryToData(
	PVOID Base,
    BOOLEAN MappedAsImage,
    USHORT DirectoryEntry,
    PULONG Size
    )
{
    return ImageDirectoryEntryToDataEx(Base, MappedAsImage, DirectoryEntry, Size, NULL);
}

/***********************************************************************
 *		RosImageDirectoryEntryToDataEx (IMAGEHLP.@)
 */
PVOID
WINAPI
ImageDirectoryEntryToDataEx (
    IN PVOID Base,
    IN BOOLEAN MappedAsImage,
    IN USHORT DirectoryEntry,
    OUT PULONG Size,
    OUT PIMAGE_SECTION_HEADER *FoundSection OPTIONAL
    )
{
    PIMAGE_NT_HEADERS			NtHeader;
    PIMAGE_FILE_HEADER			FileHeader;
	PIMAGE_OPTIONAL_HEADER		OptionalHeader;

    NtHeader = ImageNtHeader(Base);
    FileHeader = &NtHeader->FileHeader;
    OptionalHeader = &NtHeader->OptionalHeader;

    return (ImageDirectoryEntryToData32(Base,
                                        MappedAsImage,
                                        DirectoryEntry,
                                        Size,
                                        FoundSection,
                                        FileHeader,
                                        OptionalHeader));
}

/***********************************************************************
 *		RosImageDirectoryEntryToDataEx (IMAGEHLP.@)
 */
PVOID
STDCALL
ImageDirectoryEntryToData32 (
    PVOID Base,
    BOOLEAN MappedAsImage,
    USHORT DirectoryEntry,
    PULONG Size,
    PIMAGE_SECTION_HEADER *FoundHeader OPTIONAL,
    PIMAGE_FILE_HEADER FileHeader,
    PIMAGE_OPTIONAL_HEADER OptionalHeader
    )
{
    ULONG i;
    PIMAGE_SECTION_HEADER	CurrentSection;
    ULONG					DirectoryEntryVA;

	/* Get the VA of the Directory Requested */
	DirectoryEntryVA = OptionalHeader->DataDirectory[DirectoryEntry].VirtualAddress;

	/* Get the size of the Directory Requested */
    *Size = OptionalHeader->DataDirectory[DirectoryEntry].Size;

	/* Return VA if Mapped as Image*/
    if (MappedAsImage || DirectoryEntryVA < OptionalHeader->SizeOfHeaders) {
        if (FoundHeader) {
            *FoundHeader = NULL;
        }
        return (PVOID)((ULONG_PTR)Base + DirectoryEntryVA);
    }

	/* Read the first Section */
    CurrentSection = (PIMAGE_SECTION_HEADER)((ULONG_PTR)OptionalHeader + FileHeader->SizeOfOptionalHeader);
	
	/* Loop through every section*/
    for (i=0; i<FileHeader->NumberOfSections; i++) {
		
		/* If the Directory VA is located inside this section's VA, then this section belongs to this Directory */
        if (DirectoryEntryVA >= CurrentSection->VirtualAddress && 
								DirectoryEntryVA < CurrentSection->VirtualAddress + CurrentSection->SizeOfRawData) {
            if (FoundHeader) {
                *FoundHeader = CurrentSection;
            }
			 //return( (PVOID)((ULONG_PTR)Base + (DirectoryAddress - NtSection->VirtualAddress) + NtSection->PointerToRawData) );
            return ((PVOID)((ULONG_PTR)Base + (DirectoryEntryVA - CurrentSection->VirtualAddress) + CurrentSection->PointerToRawData));
        }
        ++CurrentSection;
    }
    return(NULL);
}
/***********************************************************************
 *		ImageLoad (IMAGEHLP.@)
 */
PLOADED_IMAGE WINAPI ImageLoad(LPSTR DllName, LPSTR DllPath)
{
    PLIST_ENTRY Head,Next;
    PLOADED_IMAGE LoadedImage;

	/* Initialize the List Head */
    if (!DllListInitialized) {
        InitializeListHead(&ImageLoadListHead);
        DllListInitialized = TRUE;
    }

	/* Move to the Next DLL */
    Head = &ImageLoadListHead;
    Next = Head->Flink;

	//FIXME("Trying to find library: %s in current ListHead \n", DllName);

	/* Check if we already Loaded it */
    while (Next != Head) {

		/* Get the Loaded Image Structure */
        LoadedImage = CONTAINING_RECORD(Next, LOADED_IMAGE, Links);
		//FIXME("Found: %s in current ListHead \n", LoadedImage->ModuleName);

		/* Check if the Names Match */
        if (!lstrcmpi( DllName, LoadedImage->ModuleName )) {
			//FIXME("Found it, returning it\n");
            return LoadedImage;
        }

		/* Move to next Entry */
        Next = Next->Flink;
		//FIXME("Moving to next List Entry\n");
    }

	//FIXME("Didn't find it...allocating it for you now\n");

	/* Allocate memory for the Structure, and write the Module Name under */
    LoadedImage = HeapAlloc(IMAGEHLP_hHeap, 0, sizeof(*LoadedImage) + lstrlen(DllName) + 1);

	/* Module Name will be after structure */
    LoadedImage->ModuleName = (LPSTR)LoadedImage + 1;

	/* Copy the Moduel Name */
    lstrcpy(LoadedImage->ModuleName, DllName);

	/* Now Load it and add it to our list*/
    if (MapAndLoad(DllName, DllPath, LoadedImage, TRUE, TRUE)) {
        InsertTailList(&ImageLoadListHead, &LoadedImage->Links);
        return LoadedImage;
    }

	/* If we're here...there's been a failure */
    HeapFree(IMAGEHLP_hHeap, 0, LoadedImage);
    LoadedImage = NULL;
    return LoadedImage;
}

/***********************************************************************
 *		ImageRvaToSection (IMAGEHLP.@)
 */
PIMAGE_SECTION_HEADER 
WINAPI
ImageRvaToSection(
    IN PIMAGE_NT_HEADERS NtHeaders,
    IN PVOID Base,
    IN ULONG Rva
    )
{
	PIMAGE_SECTION_HEADER Section;
    ULONG i;

	/* Get the First Section */
    Section = IMAGE_FIRST_SECTION(NtHeaders);

	/* Look through each section and check if the RVA is in between */
    for (i=0; i < NtHeaders->FileHeader.NumberOfSections; i++) {
        if (Rva >= Section->VirtualAddress && Rva < Section->VirtualAddress +
													Section->SizeOfRawData) {
            return Section;
            }
        ++Section;
        }

	/* Not Found */
    return NULL;
}

/***********************************************************************
 *		ImageNtHeader (IMAGEHLP.@)
 */
PIMAGE_NT_HEADERS WINAPI ImageNtHeader(PVOID Base)
{
  TRACE("(%p)\n", Base);

	/* Just return the e_lfanew Offset VA */
	return (PIMAGE_NT_HEADERS)((LPBYTE)Base + 
			((PIMAGE_DOS_HEADER)Base)->e_lfanew);
}

/***********************************************************************
 *		ImageRvaToVa (IMAGEHLP.@)
 */
PVOID 
WINAPI 
ImageRvaToVa(
	IN PIMAGE_NT_HEADERS NtHeaders,
    IN PVOID Base,
    IN ULONG Rva,
    IN OUT PIMAGE_SECTION_HEADER *LastRvaSection OPTIONAL
    )
{
    PIMAGE_SECTION_HEADER Section;

	/* Get the Section Associated */
	Section = ImageRvaToSection(NtHeaders, Base, Rva);

	/* Return it, if specified */
    if (LastRvaSection) *LastRvaSection = Section;

	/* Return the VA */
    return (PVOID)((ULONG_PTR)Base + (Rva - Section->VirtualAddress) +
											Section->PointerToRawData);
}

/***********************************************************************
 *		ImageUnload (IMAGEHLP.@)
 */
BOOL WINAPI ImageUnload(PLOADED_IMAGE pLoadedImage)
{
  LIST_ENTRY *pCurrent, *pFind;

  TRACE("(%p)\n", pLoadedImage);
  
  if(!IMAGEHLP_pFirstLoadedImage || !pLoadedImage)
    {
      /* No image loaded or null pointer */
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
    }

  pFind=&pLoadedImage->Links;
  pCurrent=&IMAGEHLP_pFirstLoadedImage->Links;
  while((pCurrent != pFind) &&
    (pCurrent != NULL))
      pCurrent = pCurrent->Flink;
  if(!pCurrent)
    {
      /* Not found */
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
    }

  if(pCurrent->Blink)
    pCurrent->Blink->Flink = pCurrent->Flink;
  else
    IMAGEHLP_pFirstLoadedImage = pCurrent->Flink?CONTAINING_RECORD(
      pCurrent->Flink, LOADED_IMAGE, Links):NULL;

  if(pCurrent->Flink)
    pCurrent->Flink->Blink = pCurrent->Blink;
  else
    IMAGEHLP_pLastLoadedImage = pCurrent->Blink?CONTAINING_RECORD(
      pCurrent->Blink, LOADED_IMAGE, Links):NULL;

  return FALSE;
}

/***********************************************************************
 *		MapAndLoad (IMAGEHLP.@)
 */
BOOL WINAPI MapAndLoad(
	LPSTR ImageName, 
	LPSTR DllPath, 
	PLOADED_IMAGE LoadedImage,
	BOOL DotDll, 
	BOOL ReadOnly)
{
  HANDLE hFile = NULL;
  HANDLE hFileMapping = NULL;
  HMODULE hModule = NULL;
  PIMAGE_NT_HEADERS NtHeader = NULL;
  ULONG	Tried = 0;
  UCHAR Buffer[MAX_PATH];
  LPSTR FilePart;
  LPSTR FileToOpen;

  /* So we can add the DLL Path later */
  FileToOpen = ImageName;
  
TryAgain:
	/* Get a handle to the file */
	if ((LoadedImage->hFile = CreateFileA (FileToOpen, 
											ReadOnly ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE, 
											ReadOnly ? FILE_SHARE_READ : FILE_SHARE_READ | FILE_SHARE_WRITE,
											NULL, 
											OPEN_EXISTING, 
											0, 
											NULL)) == INVALID_HANDLE_VALUE)
    {
		/* It Failed, use the DLL Search Path then (make sure we haven't already) */
        if (!Tried) {
            Tried = SearchPath(DllPath, ImageName, DotDll ? ".dll" : ".exe", MAX_PATH, Buffer, &FilePart);
            if (Tried) {
                FileToOpen = Buffer;
                goto TryAgain;
            }
        }
		/* Fail */
        return FALSE;
    }

	/* Create the File Mapping */
	if (!(hFileMapping = CreateFileMappingA (LoadedImage->hFile,
											NULL, 
											ReadOnly ? PAGE_READONLY : PAGE_READWRITE, 
											0, 
											0, 
											NULL)))
    {
      DWORD dwLastError = GetLastError();
      SetLastError(dwLastError);
      goto Error;
    }

	/* Get a pointer to the file */
	if(!(LoadedImage->MappedAddress = MapViewOfFile(hFileMapping, 
													ReadOnly ? FILE_MAP_READ : FILE_MAP_WRITE, 
													0, 
													0, 
													0)))
    {
      DWORD dwLastError = GetLastError();
      SetLastError(dwLastError);
      goto Error;
    }

	/* Close the handle to the map, we don't need it anymore */
	CloseHandle(hFileMapping);
	hFileMapping=NULL;

	/* Get the Nt Header */
	NtHeader = ImageNtHeader(LoadedImage->MappedAddress);

	/* Write data */
	LoadedImage->ModuleName = HeapAlloc(IMAGEHLP_hHeap, 0, lstrlen(ImageName) + 1);
	lstrcpy(LoadedImage->ModuleName, ImageName);
	LoadedImage->FileHeader = NtHeader;
	LoadedImage->Sections = (PIMAGE_SECTION_HEADER)
								((LPBYTE)&NtHeader->OptionalHeader +
								NtHeader->FileHeader.SizeOfOptionalHeader);
	LoadedImage->NumberOfSections = NtHeader->FileHeader.NumberOfSections;
	LoadedImage->SizeOfImage =	NtHeader->OptionalHeader.SizeOfImage;
	LoadedImage->Characteristics =	NtHeader->FileHeader.Characteristics;
	LoadedImage->LastRvaSection = LoadedImage->Sections;
	LoadedImage->fSystemImage = FALSE; /* FIXME */
	LoadedImage->fDOSImage = FALSE;    /* FIXME */

	/* Read only, so no sense in keeping the handle alive */
	if (ReadOnly) CloseHandle(LoadedImage->hFile);

	/* Return Success */
	return TRUE;

Error:
	if(LoadedImage->MappedAddress)
		UnmapViewOfFile(LoadedImage->MappedAddress);
	if(hFileMapping)
		CloseHandle(hFileMapping);
	if(LoadedImage->hFile)
		CloseHandle(LoadedImage->hFile);
	return FALSE;
}

/***********************************************************************
 *		SetImageConfigInformation (IMAGEHLP.@)
 */
BOOL WINAPI SetImageConfigInformation(
  PLOADED_IMAGE LoadedImage,
  PIMAGE_LOAD_CONFIG_DIRECTORY ImageConfigInformation)
{
  FIXME("(%p, %p): stub\n",
    LoadedImage, ImageConfigInformation
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		UnMapAndLoad (IMAGEHLP.@)
 */
BOOL WINAPI UnMapAndLoad(PLOADED_IMAGE LoadedImage)
{
  FIXME("(%p): stub\n", LoadedImage);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
