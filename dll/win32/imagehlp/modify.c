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

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winerror.h"
#include "wine/debug.h"
#include "imagehlp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imagehlp);

static WORD CalcCheckSum(DWORD StartValue, LPVOID BaseAddress, DWORD WordCount);

/***********************************************************************
 *		BindImage (IMAGEHLP.@)
 *
 * NOTES
 *   See BindImageEx
 */
BOOL WINAPI BindImage(
  PCSTR ImageName, PCSTR DllPath, PCSTR SymbolPath)
{
  return BindImageEx(0, ImageName, DllPath, SymbolPath, NULL);
}

/***********************************************************************
 *		BindImageEx (IMAGEHLP.@)
 *
 * Compute the virtual address of each function imported by a PE image
 *
 * PARAMS
 *
 *   Flags         [in] Bind options
 *   ImageName     [in] File name of the image to be bound
 *   DllPath       [in] Root of the fallback search path in case the ImageName file cannot be opened
 *   SymbolPath    [in] Symbol file root search path
 *   StatusRoutine [in] Pointer to a status routine which will be called during the binding process
 *
 * RETURNS
 *   Success: TRUE
 *   Failure: FALSE
 *
 * NOTES
 *  Binding is not implemented yet, so far this function only enumerates
 *  all imported dlls/functions and returns TRUE.
 */
BOOL WINAPI BindImageEx(
  DWORD Flags, PCSTR ImageName, PCSTR DllPath, PCSTR SymbolPath,
  PIMAGEHLP_STATUS_ROUTINE StatusRoutine)
{
    LOADED_IMAGE loaded_image;
    const IMAGE_IMPORT_DESCRIPTOR *import_desc;
    ULONG size;

    FIXME("(%d, %s, %s, %s, %p): semi-stub\n",
        Flags, debugstr_a(ImageName), debugstr_a(DllPath),
        debugstr_a(SymbolPath), StatusRoutine
    );

    if (!(MapAndLoad(ImageName, DllPath, &loaded_image, TRUE, TRUE))) return FALSE;

    if (!(import_desc = RtlImageDirectoryEntryToData((HMODULE)loaded_image.MappedAddress, FALSE,
                                                     IMAGE_DIRECTORY_ENTRY_IMPORT, &size)))
    {
        UnMapAndLoad(&loaded_image);
        return TRUE; /* No imported modules means nothing to bind, so we're done. */
    }

    /* FIXME: Does native imagehlp support both 32-bit and 64-bit PE executables? */
#ifdef _WIN64
    if (loaded_image.FileHeader->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
#else
    if (loaded_image.FileHeader->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
#endif
    {
        FIXME("Wrong architecture in PE header, unable to enumerate imports\n");
        UnMapAndLoad(&loaded_image);
        return TRUE;
    }

    for (; import_desc->Name && import_desc->FirstThunk; ++import_desc)
    {
        IMAGE_THUNK_DATA *thunk;
        char dll_fullname[MAX_PATH];
        const char *dll_name;

        if (!(dll_name = ImageRvaToVa(loaded_image.FileHeader, loaded_image.MappedAddress,
                                      import_desc->Name, 0)))
        {
            UnMapAndLoad(&loaded_image);
            SetLastError(ERROR_INVALID_ACCESS); /* FIXME */
            return FALSE;
        }

        if (StatusRoutine)
            StatusRoutine(BindImportModule, ImageName, dll_name, 0, 0);

        if (!SearchPathA(DllPath, dll_name, 0, sizeof(dll_fullname), dll_fullname, 0))
        {
            UnMapAndLoad(&loaded_image);
            SetLastError(ERROR_FILE_NOT_FOUND);
            return FALSE;
        }

        if (!(thunk = ImageRvaToVa(loaded_image.FileHeader, loaded_image.MappedAddress,
                                   import_desc->OriginalFirstThunk ? import_desc->OriginalFirstThunk :
                                   import_desc->FirstThunk, 0)))
        {
            ERR("Can't grab thunk data of %s, going to next imported DLL\n", dll_name);
            continue;
        }

        for (; thunk->u1.Ordinal; ++thunk)
        {
            /* Ignoring ordinal imports for now */
            if(!IMAGE_SNAP_BY_ORDINAL(thunk->u1.Ordinal))
            {
                IMAGE_IMPORT_BY_NAME *iibn;

                if (!(iibn = ImageRvaToVa(loaded_image.FileHeader, loaded_image.MappedAddress,
                                          thunk->u1.AddressOfData, 0)))
                {
                    ERR("Can't grab import by name info, skipping to next ordinal\n");
                    continue;
                }

                if (StatusRoutine)
                    StatusRoutine(BindImportProcedure, ImageName, dll_fullname, 0, (ULONG_PTR)iibn->Name);
            }
        }
    }

    UnMapAndLoad(&loaded_image);
    return TRUE;
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
  PIMAGE_NT_HEADERS header;
  DWORD CalcSum;
  DWORD HdrSum;

  TRACE("(%p, %d, %p, %p)\n", BaseAddress, FileLength, HeaderSum, CheckSum);

  CalcSum = CalcCheckSum(0, BaseAddress, (FileLength + 1) / sizeof(WORD));
  header = RtlImageNtHeader(BaseAddress);

  if (!header)
    return NULL;

  *HeaderSum = HdrSum = header->OptionalHeader.CheckSum;

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

  return header;
}

/***********************************************************************
 *		MapFileAndCheckSumA (IMAGEHLP.@)
 */
DWORD WINAPI MapFileAndCheckSumA(
  PCSTR Filename, PDWORD HeaderSum, PDWORD CheckSum)
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
  if (BaseAddress == 0)
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
  PCWSTR Filename, PDWORD HeaderSum, PDWORD CheckSum)
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
  if (BaseAddress == 0)
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
  PCSTR CurrentImageName, PCSTR SymbolPath, BOOL fReBase,
  BOOL fRebaseSysfileOk, BOOL fGoingDown, ULONG CheckImageSize,
  ULONG *OldImageSize, ULONG_PTR *OldImageBase, ULONG *NewImageSize,
  ULONG_PTR *NewImageBase, ULONG TimeStamp)
{
  FIXME(
    "(%s, %s, %d, %d, %d, %d, %p, %p, %p, %p, %d): stub\n",
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
  PSTR ImageName, PCSTR SymbolsPath,
  PSTR SymbolFilePath, ULONG Flags)
{
  FIXME("(%s, %s, %s, %d): stub\n",
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
  PCSTR ImageFileName, PCSTR SymbolPath,
  PSTR DebugFilePath, PIMAGE_NT_HEADERS32 NtHeaders)
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
  PCSTR ImageFileName, PCSTR SymbolPath, PSTR DebugFilePath,
  PIMAGE_NT_HEADERS32 NtHeaders, DWORD OldChecksum)
{
  FIXME("(%s, %s, %s, %p, %d): stub\n",
    debugstr_a(ImageFileName), debugstr_a(SymbolPath),
    debugstr_a(DebugFilePath), NtHeaders, OldChecksum
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
