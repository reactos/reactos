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
 *		ImageDirectoryEntryToData (IMAGEHLP.@)
 */
PVOID WINAPI ImageDirectoryEntryToData(
  PVOID Base, BOOLEAN MappedAsImage, USHORT DirectoryEntry, PULONG Size)
{
  FIXME("(%p, %d, %hd, %p): stub\n",
    Base, MappedAsImage, DirectoryEntry, Size
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return NULL;
}

/***********************************************************************
 *		ImageLoad (IMAGEHLP.@)
 */
PLOADED_IMAGE WINAPI ImageLoad(LPSTR DllName, LPSTR DllPath)
{
  PLOADED_IMAGE pLoadedImage;

  FIXME("(%s, %s): stub\n", DllName, DllPath);
	  
  pLoadedImage = HeapAlloc(IMAGEHLP_hHeap, 0, sizeof(LOADED_IMAGE));
  if (pLoadedImage)
    pLoadedImage->FileHeader = HeapAlloc(IMAGEHLP_hHeap, 0, sizeof(IMAGE_NT_HEADERS));
  
  return pLoadedImage;
}

/***********************************************************************
 *		ImageRvaToSection (IMAGEHLP.@)
 */
PIMAGE_SECTION_HEADER WINAPI ImageRvaToSection(
  PIMAGE_NT_HEADERS NtHeaders, PVOID Base, ULONG Rva)
{
  FIXME("(%p, %p, %ld): stub\n", NtHeaders, Base, Rva);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return NULL;
}

/***********************************************************************
 *		ImageNtHeader (IMAGEHLP.@)
 */
PIMAGE_NT_HEADERS WINAPI ImageNtHeader(PVOID Base)
{
  TRACE("(%p)\n", Base);
  return (PIMAGE_NT_HEADERS)
    ((LPBYTE) Base + ((PIMAGE_DOS_HEADER) Base)->e_lfanew);
}

/***********************************************************************
 *		ImageRvaToVa (IMAGEHLP.@)
 */
PVOID WINAPI ImageRvaToVa(
  PIMAGE_NT_HEADERS NtHeaders, PVOID Base, ULONG Rva,
  PIMAGE_SECTION_HEADER *LastRvaSection)
{
  FIXME("(%p, %p, %ld, %p): stub\n",
    NtHeaders, Base, Rva, LastRvaSection
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return NULL;
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
  LPSTR pszImageName, LPSTR pszDllPath, PLOADED_IMAGE pLoadedImage,
  BOOL bDotDll, BOOL bReadOnly)
{
  CHAR szFileName[MAX_PATH];
  HANDLE hFile = NULL;
  HANDLE hFileMapping = NULL;
  HMODULE hModule = NULL;
  PIMAGE_NT_HEADERS pNtHeader = NULL;

  TRACE("(%s, %s, %p, %d, %d)\n", pszImageName, pszDllPath, pLoadedImage,
                                    bDotDll, bReadOnly);
  
  /* PathCombine(&szFileName, pszDllPath, pszImageName); */
  /* PathRenameExtension(&szFileName, bDotDll?:"dll":"exe"); */

  /* FIXME: Check if the file already loaded (use IMAGEHLP_pFirstLoadedImage) */
  if(!(hFile = CreateFileA(
    szFileName, GENERIC_READ, 1, /* FIXME: FILE_SHARE_READ not defined */
    NULL, OPEN_EXISTING, 0, NULL)))
    {
      SetLastError(ERROR_FILE_NOT_FOUND);
      goto Error;
    }

  if(!(hFileMapping = CreateFileMappingA(
    hFile, NULL, PAGE_READONLY | SEC_COMMIT, 0, 0, NULL)))
    {
      DWORD dwLastError = GetLastError();
      WARN("CreateFileMapping: Error = %ld\n", dwLastError);
      SetLastError(dwLastError);
      goto Error;
    }
  CloseHandle(hFile);
  hFile = NULL;

  if(!(hModule = (HMODULE) MapViewOfFile(
    hFileMapping, FILE_MAP_READ, 0, 0, 0)))
    {
      DWORD dwLastError = GetLastError();
      WARN("MapViewOfFile: Error = %ld\n", dwLastError);
      SetLastError(dwLastError);
      goto Error;
    }

  CloseHandle(hFileMapping);
  hFileMapping=NULL;

  pLoadedImage = (PLOADED_IMAGE) HeapAlloc(
    IMAGEHLP_hHeap, 0, sizeof(LOADED_IMAGE)
  );

  pNtHeader = ImageNtHeader((PVOID) hModule);

  pLoadedImage->ModuleName = HeapAlloc(IMAGEHLP_hHeap, 0, strlen(pszDllPath)+1); /* FIXME: Correct? */
  strcpy( pLoadedImage->ModuleName, pszDllPath );
  pLoadedImage->hFile = hFile;
  pLoadedImage->MappedAddress = (PUCHAR) hModule;
  pLoadedImage->FileHeader = pNtHeader;
  pLoadedImage->Sections = (PIMAGE_SECTION_HEADER)
    ((LPBYTE) &pNtHeader->OptionalHeader +
      pNtHeader->FileHeader.SizeOfOptionalHeader);
  pLoadedImage->NumberOfSections =
    pNtHeader->FileHeader.NumberOfSections;
  pLoadedImage->SizeOfImage =
    pNtHeader->OptionalHeader.SizeOfImage;
  pLoadedImage->Characteristics =
    pNtHeader->FileHeader.Characteristics;
  pLoadedImage->LastRvaSection = pLoadedImage->Sections;

  pLoadedImage->fSystemImage = FALSE; /* FIXME */
  pLoadedImage->fDOSImage = FALSE;    /* FIXME */

  /* FIXME: Make thread safe */
  pLoadedImage->Links.Flink = NULL;
  pLoadedImage->Links.Blink = &IMAGEHLP_pLastLoadedImage->Links;
  if(IMAGEHLP_pLastLoadedImage)
    IMAGEHLP_pLastLoadedImage->Links.Flink = &pLoadedImage->Links;
  IMAGEHLP_pLastLoadedImage = pLoadedImage;
  if(!IMAGEHLP_pFirstLoadedImage)
    IMAGEHLP_pFirstLoadedImage = pLoadedImage;

  return TRUE;

Error:
  if(hModule)
    UnmapViewOfFile((PVOID) hModule);
  if(hFileMapping)
    CloseHandle(hFileMapping);
  if(hFile)
    CloseHandle(hFile);
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
