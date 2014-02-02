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

#include <winternl.h>

/***********************************************************************
 *           Data
 */

static PLOADED_IMAGE IMAGEHLP_pFirstLoadedImage=NULL;

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
  FALSE,      /* fReadOnly */
  0,          /* Version */
  { &IMAGEHLP_EmptyLoadedImage.Links, &IMAGEHLP_EmptyLoadedImage.Links }, /* Links */
  148,        /* SizeOfImage; */
};

DECLSPEC_HIDDEN extern HANDLE IMAGEHLP_hHeap;

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
PLOADED_IMAGE WINAPI ImageLoad(PCSTR DllName, PCSTR DllPath)
{
  PLOADED_IMAGE pLoadedImage;

  FIXME("(%s, %s): stub\n", DllName, DllPath);
	  
  pLoadedImage = HeapAlloc(IMAGEHLP_hHeap, 0, sizeof(LOADED_IMAGE));
  if (pLoadedImage)
    pLoadedImage->FileHeader = HeapAlloc(IMAGEHLP_hHeap, 0, sizeof(IMAGE_NT_HEADERS));
  
  return pLoadedImage;
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

  return FALSE;
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
        WARN("CreateFile: Error = %d\n", GetLastError());
        goto Error;
    }

    hFileMapping = CreateFileMappingA(hFile, NULL, 
                                      (bReadOnly ? PAGE_READONLY : PAGE_READWRITE) | SEC_COMMIT,
                                      0, 0, NULL);
    if (!hFileMapping)
    {
        WARN("CreateFileMapping: Error = %d\n", GetLastError());
        goto Error;
    }

    mapping = MapViewOfFile(hFileMapping, bReadOnly ? FILE_MAP_READ : FILE_MAP_WRITE, 0, 0, 0);
    CloseHandle(hFileMapping);
    if (!mapping)
    {
        WARN("MapViewOfFile: Error = %d\n", GetLastError());
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
    pLoadedImage->Sections         = (PIMAGE_SECTION_HEADER)
        ((LPBYTE) &pNtHeader->OptionalHeader +
         pNtHeader->FileHeader.SizeOfOptionalHeader);
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
