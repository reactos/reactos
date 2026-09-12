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

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winternl.h"
#include "winerror.h"
#include "winver.h"
#include "wine/debug.h"
#include "imagehlp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imagehlp);

/***********************************************************************
 *           Data
 */
static LIST_ENTRY image_list = { &image_list, &image_list };


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
DWORD WINAPI GetImageUnusedHeaderBytes(
  PLOADED_IMAGE LoadedImage,
  LPDWORD SizeUnusedHeaderBytes)
{
  FIXME("(%p, %p): stub\n",
    LoadedImage, SizeUnusedHeaderBytes
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImageLoad (IMAGEHLP.@)
 */
PLOADED_IMAGE WINAPI ImageLoad(PCSTR dll_name, PCSTR dll_path)
{
    LOADED_IMAGE *image;

    TRACE("(%s, %s)\n", dll_name, dll_path);

    image = HeapAlloc(GetProcessHeap(), 0, sizeof(*image));
    if (!image) return NULL;

    if (!MapAndLoad(dll_name, dll_path, image, TRUE, TRUE))
    {
        HeapFree(GetProcessHeap(), 0, image);
        return NULL;
    }

    image->Links.Flink = image_list.Flink;
    image->Links.Blink = &image_list;
    image_list.Flink->Blink = &image->Links;
    image_list.Flink = &image->Links;

    return image;
}

/***********************************************************************
 *		ImageUnload (IMAGEHLP.@)
 */
BOOL WINAPI ImageUnload(PLOADED_IMAGE loaded_image)
{
    LIST_ENTRY *entry, *mark;
    PLOADED_IMAGE image;

    TRACE("(%p)\n", loaded_image);

    /* FIXME: do we really need to check this? */
    mark = &image_list;
    for (entry = mark->Flink; entry != mark; entry = entry->Flink)
    {
        image = CONTAINING_RECORD(entry, LOADED_IMAGE, Links);
        if (image == loaded_image)
            break;
    }

    if (entry == mark)
    {
        /* Not found */
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    entry->Blink->Flink = entry->Flink;
    entry->Flink->Blink = entry->Blink;

    UnMapAndLoad(loaded_image);
    HeapFree(GetProcessHeap(), 0, loaded_image);

    return TRUE;
}

/***********************************************************************
 *		MapAndLoad (IMAGEHLP.@)
 */
BOOL WINAPI MapAndLoad(PCSTR pszImageName, PCSTR pszDllPath, PLOADED_IMAGE pLoadedImage,
                       BOOL bDotDll, BOOL bReadOnly)
{
    CHAR szFileName[MAX_PATH];
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hFileMapping = NULL;
    PVOID mapping = NULL;
    PIMAGE_NT_HEADERS pNtHeader = NULL;

    TRACE("(%s, %s, %p, %d, %d)\n",
          pszImageName, pszDllPath, pLoadedImage, bDotDll, bReadOnly);

    if (!SearchPathA(pszDllPath, pszImageName, bDotDll ? ".DLL" : ".EXE",
                     sizeof(szFileName), szFileName, NULL))
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        goto Error;
    }

    hFile = CreateFileA(szFileName,
                        GENERIC_READ | (bReadOnly ? 0 : GENERIC_WRITE),
                        FILE_SHARE_READ,
                        NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        WARN("CreateFile: Error = %ld\n", GetLastError());
        goto Error;
    }

    hFileMapping = CreateFileMappingA(hFile, NULL, 
                                      (bReadOnly ? PAGE_READONLY : PAGE_READWRITE) | SEC_COMMIT,
                                      0, 0, NULL);
    if (!hFileMapping)
    {
        WARN("CreateFileMapping: Error = %ld\n", GetLastError());
        goto Error;
    }

    mapping = MapViewOfFile(hFileMapping, bReadOnly ? FILE_MAP_READ : FILE_MAP_WRITE, 0, 0, 0);
    CloseHandle(hFileMapping);
    if (!mapping)
    {
        WARN("MapViewOfFile: Error = %ld\n", GetLastError());
        goto Error;
    }

    if (!(pNtHeader = RtlImageNtHeader(mapping)))
    {
        WARN("Not an NT header\n");
        UnmapViewOfFile(mapping);
        goto Error;
    }

    pLoadedImage->ModuleName       = HeapAlloc(GetProcessHeap(), 0,
                                               strlen(szFileName) + 1);
    if (pLoadedImage->ModuleName) strcpy(pLoadedImage->ModuleName, szFileName);
    pLoadedImage->hFile            = hFile;
    pLoadedImage->MappedAddress    = mapping;
    pLoadedImage->FileHeader       = pNtHeader;
    pLoadedImage->Sections         = IMAGE_FIRST_SECTION(pNtHeader);
    pLoadedImage->NumberOfSections = pNtHeader->FileHeader.NumberOfSections;
    pLoadedImage->SizeOfImage      = GetFileSize(hFile, NULL);
    pLoadedImage->Characteristics  = pNtHeader->FileHeader.Characteristics;
    pLoadedImage->LastRvaSection   = pLoadedImage->Sections;

    pLoadedImage->fSystemImage     = FALSE; /* FIXME */
    pLoadedImage->fDOSImage        = FALSE; /* FIXME */

    pLoadedImage->Links.Flink      = &pLoadedImage->Links;
    pLoadedImage->Links.Blink      = &pLoadedImage->Links;

    return TRUE;

Error:
    if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
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
BOOL WINAPI UnMapAndLoad(PLOADED_IMAGE pLoadedImage)
{
    HeapFree(GetProcessHeap(), 0, pLoadedImage->ModuleName);
    /* FIXME: MSDN states that a new checksum is computed and stored into the file */
    if (pLoadedImage->MappedAddress) UnmapViewOfFile(pLoadedImage->MappedAddress);
    if (pLoadedImage->hFile != INVALID_HANDLE_VALUE) CloseHandle(pLoadedImage->hFile);
    return TRUE;
}
