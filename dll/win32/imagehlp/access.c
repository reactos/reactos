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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "precomp.h"

//#define NDEBUG
#include <debug.h>
#define _WINNT_H
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(imagehlp);

/***********************************************************************
 *           Data
 */

BOOLEAN DllListInitialized;
LIST_ENTRY ImageLoadListHead;

/***********************************************************************
 *		GetImageConfigInformation (IMAGEHLP.@)
 */
BOOL IMAGEAPI GetImageConfigInformation(
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
DWORD IMAGEAPI GetImageUnusedHeaderBytes(
  PLOADED_IMAGE LoadedImage,
  LPDWORD SizeUnusedHeaderBytes)
{
    SIZE_T FirstFreeByte;
    PIMAGE_OPTIONAL_HEADER OptionalHeader = NULL;
    PIMAGE_NT_HEADERS NtHeaders;
    ULONG i;

    /* Read the NT Headers */
    NtHeaders = LoadedImage->FileHeader;

    /* Find the first free byte, which is after all the headers and sections */
    FirstFreeByte = (ULONG_PTR)NtHeaders -
                    (ULONG_PTR)LoadedImage->MappedAddress +
                    FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) +
                    NtHeaders->FileHeader.SizeOfOptionalHeader +
                    NtHeaders->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);

    /* Get the Optional Header */
    OptionalHeader = &LoadedImage->FileHeader->OptionalHeader;

    /*
     * There is the possibilty that one of the Data Directories is in the PE Header
     * itself, so we'll need to find such a case and add it to our PE used space
     */
    for (i = 0; i < OptionalHeader->NumberOfRvaAndSizes; i++)
    {
        /* If the VA is less then the size of headers, then the data is inside the PE header */
        if (OptionalHeader->DataDirectory[i].VirtualAddress <
            OptionalHeader->SizeOfHeaders)
        {
            /* However, make sure it's not 0, which means it doesnt actually exist */
            if (OptionalHeader->DataDirectory[i].VirtualAddress >=
                FirstFreeByte)
            {
                /* Our first empty byte is after this Directory Data then */
                FirstFreeByte = OptionalHeader->DataDirectory[i].VirtualAddress +
                                OptionalHeader->DataDirectory[i].Size;
            }
        }
    }

    /* Return the unused Header Bytes */
    *SizeUnusedHeaderBytes = OptionalHeader->SizeOfHeaders - (DWORD)FirstFreeByte;

    /* And return the first free byte*/
    return (DWORD)FirstFreeByte;
}

/***********************************************************************
 *		ImageLoad (IMAGEHLP.@)
 */
PLOADED_IMAGE IMAGEAPI ImageLoad(LPSTR DllName, LPSTR DllPath)
{
    PLIST_ENTRY Head, Next;
    PLOADED_IMAGE LoadedImage;
    CHAR Drive[_MAX_DRIVE], Dir[_MAX_DIR], Filename[_MAX_FNAME], Ext[_MAX_EXT];
    BOOL CompleteName = TRUE;
    CHAR FullName[MAX_PATH];

    /* Initialize the List Head */
    if (!DllListInitialized)
    {
        InitializeListHead(&ImageLoadListHead);
        DllListInitialized = TRUE;
    }

    /* Move to the Next DLL */
    Head = &ImageLoadListHead;
    Next = Head->Flink;
    DPRINT("Trying to find library: %s in current ListHead \n", DllName);

    /* Split the path */
    _splitpath(DllName, Drive, Dir, Filename, Ext);

    /* Check if we only got a name */
    if (!strlen(Drive) && !strlen(Dir)) CompleteName = FALSE;

    /* Check if we already Loaded it */
    while (Next != Head)
    {
        /* Get the Loaded Image Structure */
        LoadedImage = CONTAINING_RECORD(Next, LOADED_IMAGE, Links);
        DPRINT("Found: %s in current ListHead \n", LoadedImage->ModuleName);

        /* Check if we didn't have a complete name */
        if (!CompleteName)
        {
            /* Split this module's name */
            _splitpath(LoadedImage->ModuleName, NULL, NULL, Filename, Ext);

            /* Use only the name and extension */
            strcpy(FullName, Filename);
            strcat(FullName, Ext);
        }
        else
        {
            /* Use the full untouched name */
            strcpy(FullName, LoadedImage->ModuleName);
        }

        /* Check if the Names Match */
        if (!_stricmp(DllName, FullName))
        {
            DPRINT("Found it, returning it\n");
            return LoadedImage;
        }

        /* Move to next Entry */
        Next = Next->Flink;
    }

    /* Allocate memory for the Structure, and write the Module Name under */
    DPRINT("Didn't find it...allocating it for you now\n");
    LoadedImage = HeapAlloc(IMAGEHLP_hHeap,
                            0,
                            sizeof(*LoadedImage) + strlen(DllName) + 1);
    if (LoadedImage)
    {
        /* Module Name will be after structure */
        LoadedImage->ModuleName = (LPSTR)(LoadedImage + 1);

        /* Copy the Module Name */
        strcpy(LoadedImage->ModuleName, DllName);

        /* Now Load it */
        if (MapAndLoad(DllName, DllPath, LoadedImage, TRUE, TRUE))
        {
            /* Add it to our list and return it */
            InsertTailList(&ImageLoadListHead, &LoadedImage->Links);
            return LoadedImage;
        }

        /* If we're here...there's been a failure */
        HeapFree(IMAGEHLP_hHeap, 0, LoadedImage);
        LoadedImage = NULL;
    }
    return LoadedImage;
}

/***********************************************************************
 *		ImageUnload (IMAGEHLP.@)
 */
BOOL IMAGEAPI ImageUnload(PLOADED_IMAGE pLoadedImage)
{
    /* If the image list isn't empty, remove this entry */
    if (!IsListEmpty(&pLoadedImage->Links)) RemoveEntryList(&pLoadedImage->Links);

    /* Unmap and unload it */
    UnMapAndLoad(pLoadedImage);

    /* Free the structure */
    HeapFree(IMAGEHLP_hHeap, 0, pLoadedImage);

    /* Return success */
    return TRUE;
}

/***********************************************************************
 *		MapAndLoad (IMAGEHLP.@)
 */
BOOL IMAGEAPI MapAndLoad(
  LPSTR ImageName, LPSTR DllPath, PLOADED_IMAGE pLoadedImage,
  BOOL DotDll, BOOL ReadOnly)
{
    HANDLE hFile;
    HANDLE hFileMapping;
    ULONG Tried = 0;
    UCHAR Buffer[MAX_PATH];
    LPSTR FilePart;
    LPSTR FileToOpen;
    PIMAGE_NT_HEADERS NtHeader;

    /* So we can add the DLL Path later */
    FileToOpen = ImageName;

    /* Assume failure */
    pLoadedImage->hFile = INVALID_HANDLE_VALUE;

    /* Start open loop */
    while (TRUE)
    {
        /* Get a handle to the file */
        hFile = CreateFileA(FileToOpen,
                            ReadOnly ? GENERIC_READ :
                                       GENERIC_READ | GENERIC_WRITE,
                            ReadOnly ? FILE_SHARE_READ :
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            /* Check if we already tried this once */
            if (!Tried)
            {
                /* We didn't do do a path search now */
                Tried = SearchPath(DllPath,
                                   ImageName,
                                   DotDll ? ".dll" : ".exe",
                                   MAX_PATH,
                                   Buffer,
                                   &FilePart);

                /* Check if it was successful */
                if (Tried && (Tried < MAX_PATH))
                {
                    /* Change the filename to use, and try again */
                    FileToOpen = Buffer;
                    continue;
                }
            }

            /* Fail */
            return FALSE;
        }

        /* Success, break out */
        break;
    }

    /* Create the File Mapping */
    hFileMapping = CreateFileMappingA(hFile,
                                      NULL,
                                      ReadOnly ? PAGE_READONLY :
                                                 PAGE_READWRITE,
                                      0,
                                      0,
                                      NULL);
    if (!hFileMapping)
    {
        /* Fail */
        SetLastError(GetLastError());
        CloseHandle(hFile);
        return FALSE;
    }

    /* Get a pointer to the file */
    pLoadedImage->MappedAddress = MapViewOfFile(hFileMapping,
                                               ReadOnly ? FILE_MAP_READ :
                                                          FILE_MAP_WRITE,
                                               0,
                                               0,
                                               0);

    /* Close the handle to the map, we don't need it anymore */
    CloseHandle(hFileMapping);

    /* Write the image size */
    pLoadedImage->SizeOfImage = GetFileSize(hFile, NULL);

    /* Get the Nt Header */
    NtHeader = ImageNtHeader(pLoadedImage->MappedAddress);

    /* Allocate memory for the name and save it */
    pLoadedImage->ModuleName = HeapAlloc(IMAGEHLP_hHeap,
                                        0,
                                        strlen(FileToOpen) + 16);
    strcpy(pLoadedImage->ModuleName, FileToOpen);

    /* Save the NT Header */
    pLoadedImage->FileHeader = NtHeader;

    /* Save the section data */
    pLoadedImage->Sections = IMAGE_FIRST_SECTION(NtHeader);
    pLoadedImage->NumberOfSections = NtHeader->FileHeader.NumberOfSections;

    /* Setup other data */
    pLoadedImage->SizeOfImage = NtHeader->OptionalHeader.SizeOfImage;
    pLoadedImage->Characteristics = NtHeader->FileHeader.Characteristics;
    pLoadedImage->LastRvaSection = pLoadedImage->Sections;
    pLoadedImage->fSystemImage = FALSE; /* FIXME */
    pLoadedImage->fDOSImage = FALSE; /* FIXME */
    InitializeListHead(&pLoadedImage->Links);

    /* Check if it was read-only */
    if (ReadOnly)
    {
        /* It was, so close our handle and write it as invalid */
        CloseHandle(hFile);
        pLoadedImage->hFile = INVALID_HANDLE_VALUE;
    }
    else
    {
        /* Write our file handle */
        pLoadedImage->hFile = hFile;
    }

    /* Return Success */
    return TRUE;
}

/***********************************************************************
 *		SetImageConfigInformation (IMAGEHLP.@)
 */
BOOL IMAGEAPI SetImageConfigInformation(
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
BOOL IMAGEAPI UnMapAndLoad(PLOADED_IMAGE Image)
{
    PIMAGE_NT_HEADERS NtHeader;
    DWORD HeaderCheckSum, CheckSum;

    /* Check if the image was read-only */
    if (Image->hFile == INVALID_HANDLE_VALUE)
    {
        /* We'll only unmap the view */
        UnmapViewOfFile(Image->MappedAddress);
    }
    else
    {
        /* Calculate the checksum */
        CheckSumMappedFile(Image->MappedAddress,
                           Image->SizeOfImage,
                           &HeaderCheckSum,
                           &CheckSum);

        /* Get the NT Header */
        NtHeader = Image->FileHeader;

        /* Write the new checksum to it */
        NtHeader->OptionalHeader.CheckSum = CheckSum;

        /* Now flush and unmap the image */
        FlushViewOfFile(Image->MappedAddress, Image->SizeOfImage);
        UnmapViewOfFile(Image->MappedAddress);

        /* Check if the size changed */
        if (Image->SizeOfImage != GetFileSize(Image->hFile, NULL))
        {
            /* Update the file pointer */
            SetFilePointer(Image->hFile, Image->SizeOfImage, NULL, FILE_BEGIN);
            SetEndOfFile(Image->hFile);
        }
    }

    /* Check if the image had a valid handle, and close it */
    if (Image->hFile != INVALID_HANDLE_VALUE) CloseHandle(Image->hFile);

    /* Return success */
    return TRUE;
}

PVOID
IMAGEAPI
ImageDirectoryEntryToData32(PVOID Base,
                            BOOLEAN MappedAsImage,
                            USHORT DirectoryEntry,
                            PULONG Size,
                            PIMAGE_SECTION_HEADER *FoundHeader OPTIONAL,
                            PIMAGE_FILE_HEADER FileHeader,
                            PIMAGE_OPTIONAL_HEADER OptionalHeader)
{
    ULONG i;
    PIMAGE_SECTION_HEADER CurrentSection;
    ULONG DirectoryEntryVA;

    /* Check if this entry is invalid */
    if (DirectoryEntry >= OptionalHeader->NumberOfRvaAndSizes)
    {
        /* Nothing found */
        *Size = 0;
        return NULL;
    }

    /* Get the VA of the Directory Requested */
    DirectoryEntryVA = OptionalHeader->DataDirectory[DirectoryEntry].VirtualAddress;
    if (!DirectoryEntryVA)
    {
        /* It doesn't exist */
        *Size = 0;
        return NULL;
    }

    /* Get the size of the Directory Requested */
    *Size = OptionalHeader->DataDirectory[DirectoryEntry].Size;

    /* Check if it was mapped as an image or if the entry is within the headers */
    if ((MappedAsImage) || (DirectoryEntryVA < OptionalHeader->SizeOfHeaders))
    {
        /* No header found */
        if (FoundHeader) *FoundHeader = NULL;

        /* And simply return the VA */
        return (PVOID)((ULONG_PTR)Base + DirectoryEntryVA);
    }

    /* Read the first Section */
    CurrentSection = (PIMAGE_SECTION_HEADER)((ULONG_PTR)OptionalHeader +
                                             FileHeader->SizeOfOptionalHeader);

    /* Loop through every section*/
    for (i = 0; i < FileHeader->NumberOfSections; i++)
    {
        /* If the Directory VA is located inside this section's VA, then this section belongs to this Directory */
        if ((DirectoryEntryVA >= CurrentSection->VirtualAddress) &&
            (DirectoryEntryVA < (CurrentSection->VirtualAddress +
                                 CurrentSection->SizeOfRawData)))
        {
            /* Return the section header */
            if (FoundHeader) *FoundHeader = CurrentSection;
            return ((PVOID)((ULONG_PTR)Base +
                            (DirectoryEntryVA - CurrentSection->VirtualAddress) +
                            CurrentSection->PointerToRawData));
        }

        /* Move to the next section */
        CurrentSection++;
    }

    /* If we got here, then we didn't find anything */
    return NULL;
}

/*
 * @unimplemented
 */
DWORD
IMAGEAPI
GetTimestampForLoadedLibrary(HMODULE Module)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @implemented
 */
PVOID
IMAGEAPI
ImageDirectoryEntryToData(PVOID Base,
                          BOOLEAN MappedAsImage,
                          USHORT DirectoryEntry,
                          PULONG Size)
{
    /* Let the extended function handle it */
    return ImageDirectoryEntryToDataEx(Base,
                                       MappedAsImage,
                                       DirectoryEntry,
                                       Size,
                                       NULL);
}

/*
 * @implemented
 */
PVOID
IMAGEAPI
ImageDirectoryEntryToDataEx(IN PVOID Base,
                            IN BOOLEAN MappedAsImage,
                            IN USHORT DirectoryEntry,
                            OUT PULONG Size,
                            OUT PIMAGE_SECTION_HEADER *FoundSection OPTIONAL)
{
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_FILE_HEADER FileHeader;
    PIMAGE_OPTIONAL_HEADER OptionalHeader;

    /* Get the optional header ourselves */
    NtHeader = ImageNtHeader(Base);
    FileHeader = &NtHeader->FileHeader;
    OptionalHeader = &NtHeader->OptionalHeader;

    /* FIXME: Read image type and call appropriate function (32, 64, ROM) */
    return ImageDirectoryEntryToData32(Base,
                                       MappedAsImage,
                                       DirectoryEntry,
                                       Size,
                                       FoundSection,
                                       FileHeader,
                                       OptionalHeader);
}

/*
 * @implemented
 */
PIMAGE_SECTION_HEADER
IMAGEAPI
ImageRvaToSection(IN PIMAGE_NT_HEADERS NtHeaders,
                  IN PVOID Base,
                  IN ULONG Rva)
{
    PIMAGE_SECTION_HEADER Section;
    ULONG i;

    /* Get the First Section */
    Section = IMAGE_FIRST_SECTION(NtHeaders);

    /* Look through each section */
    for (i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++)
    {
        /* Check if the RVA is in between */
        if ((Rva >= Section->VirtualAddress) &&
            (Rva < (Section->VirtualAddress + Section->SizeOfRawData)))
        {
            /* Return this section */
            return Section;
        }

        /* Move to the next section */
        Section++;
    }

    /* Not Found */
    return NULL;
}

/*
 * @implemented
 */
PIMAGE_NT_HEADERS
IMAGEAPI
ImageNtHeader(PVOID Base)
{
    /* Let RTL do it */
    return RtlImageNtHeader(Base);
}

/*
 * @implemented
 */
PVOID
IMAGEAPI
ImageRvaToVa(IN PIMAGE_NT_HEADERS NtHeaders,
             IN PVOID Base,
             IN ULONG Rva,
             IN OUT PIMAGE_SECTION_HEADER *LastRvaSection OPTIONAL)
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

BOOL
IMAGEAPI
UnloadAllImages(VOID)
{
    PLIST_ENTRY Head, Entry;
    PLOADED_IMAGE CurrentImage;

    /* Make sure we're initialized */
    if (!DllListInitialized) return TRUE;

    /* Get the list pointers and loop */
    Head = &ImageLoadListHead;
    Entry = Head->Flink;
    while (Entry != Head)
    {
        /* Get this image */
        CurrentImage = CONTAINING_RECORD(Entry, LOADED_IMAGE, Links);

        /* Move to the next entry */
        Entry = Entry->Flink;

        /* Unload it */
        ImageUnload(CurrentImage);
    }

    /* We are not initialized anymore */
    DllListInitialized = FALSE;
    return TRUE;
}
